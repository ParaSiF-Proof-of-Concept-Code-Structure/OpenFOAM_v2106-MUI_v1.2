/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
    Copyright (C) 2015-2020 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "advancingFrontAMI.H"
#include "meshTools.H"
#include "mapDistribute.H"
#include "unitConversion.H"

#include "findNearestMaskedOp.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(advancingFrontAMI, 0);
}

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void Foam::advancingFrontAMI::checkPatches() const
{
    const auto& src = srcPatch();
    const auto& tgt = tgtPatch();

    if (debug && (!src.size() || !tgt.size()))
    {
        Pout<< "AMI: Patches not on processor: Source faces = "
            << src.size() << ", target faces = " << tgt.size()
            << endl;
    }


    if (requireMatch_)
    {
        const scalar maxBoundsError = 0.05;

        // Check bounds of source and target
        boundBox bbSrc(src.points(), src.meshPoints(), true);
        boundBox bbTgt(tgt.points(), tgt.meshPoints(), true);

        boundBox bbTgtInf(bbTgt);
        bbTgtInf.inflate(maxBoundsError);

        if (!bbTgtInf.contains(bbSrc))
        {
            WarningInFunction
                << "Source and target patch bounding boxes are not similar"
                << nl
                << "    source box span     : " << bbSrc.span() << nl
                << "    target box span     : " << bbTgt.span() << nl
                << "    source box          : " << bbSrc << nl
                << "    target box          : " << bbTgt << nl
                << "    inflated target box : " << bbTgtInf << endl;
        }
    }
}


void Foam::advancingFrontAMI::createExtendedTgtPatch()
{
    // Create processor map of extended cells. This map gets (possibly
    // remote) cells from the src mesh such that they (together) cover
    // all of tgt
    extendedTgtMapPtr_.reset(calcProcMap(srcPatch0(), tgtPatch0()));
    const mapDistribute& map = extendedTgtMapPtr_();

    // Original faces from tgtPatch
    // Note: in globalIndexing since might be remote
    globalIndex globalTgtFaces(tgtPatch0().size());
    distributeAndMergePatches
    (
        map,
        tgtPatch0(),
        globalTgtFaces,
        extendedTgtFaces_,
        extendedTgtPoints_,
        extendedTgtFaceIDs_
    );

    // Create a representation of the tgt patch that is extended to overlap
    // the src patch
    extendedTgtPatchPtr_.reset
    (
        autoPtr<primitivePatch>::New
        (
            SubList<face>(extendedTgtFaces_),
            extendedTgtPoints_
        )
    );
}


bool Foam::advancingFrontAMI::initialiseWalk(label& srcFacei, label& tgtFacei)
{
    const auto& src = this->srcPatch();
    const auto& tgt = this->tgtPatch();

    bool foundFace = false;

    // Check that patch sizes are valid
    if (!src.size())
    {
        return foundFace;
    }
    else if (!tgt.size())
    {
        WarningInFunction
            << src.size() << " source faces but no target faces" << endl;

        return foundFace;
    }

    // Reset the octree
    treePtr_.reset(createTree(tgt));

    // Find initial face match using brute force/octree search
    if ((srcFacei == -1) || (tgtFacei == -1))
    {
        srcFacei = 0;
        tgtFacei = 0;
        forAll(src, facei)
        {
            tgtFacei = findTargetFace(facei);
            if (tgtFacei >= 0)
            {
                srcFacei = facei;
                foundFace = true;
                break;
            }
        }

        if (!foundFace)
        {
            if (requireMatch_)
            {
                FatalErrorInFunction
                    << "Unable to find initial target face"
                    << abort(FatalError);
            }

            return foundFace;
        }
    }

    if (debug)
    {
        Pout<< "AMI: initial target face = " << tgtFacei << endl;
    }

    return true;
}


void Foam::advancingFrontAMI::writeIntersectionOBJ
(
    const scalar area,
    const face& f1,
    const face& f2,
    const pointField& f1Points,
    const pointField& f2Points
) const
{
    static label count = 1;

    const pointField f1pts = f1.points(f1Points);
    const pointField f2pts = f2.points(f2Points);

    Pout<< "Face intersection area (" << count <<  "):" << nl
        << "    f1 face = " << f1 << nl
        << "    f1 pts  = " << f1pts << nl
        << "    f2 face = " << f2 << nl
        << "    f2 pts  = " << f2pts << nl
        << "    area    = " << area
        << endl;

    OFstream os("areas" + name(count) + ".obj");

    for (const point& pt : f1pts)
    {
        meshTools::writeOBJ(os, pt);
    }
    os<< "l";
    forAll(f1pts, i)
    {
        os<< " " << i + 1;
    }
    os<< " 1" << endl;

    for (const point& pt : f2pts)
    {
        meshTools::writeOBJ(os, pt);
    }
    os<< "l";
    const label n = f1pts.size();
    forAll(f2pts, i)
    {
        os<< " " << n + i + 1;
    }
    os<< " " << n + 1 << endl;

    ++count;
}


Foam::label Foam::advancingFrontAMI::findTargetFace
(
    const label srcFacei,
    const UList<label>& excludeFaces,
    const label srcFacePti
) const
{
    const auto& src = srcPatch();

    label targetFacei = -1;

    const pointField& srcPts = src.points();
    const face& srcFace = src[srcFacei];

    findNearestMaskedOp<primitivePatch> fnOp(*treePtr_, excludeFaces);

    const boundBox bb(srcPts, srcFace, false);

    const point srcPt =
        srcFacePti == -1 ? bb.centre() : srcPts[srcFace[srcFacePti]];

    const pointIndexHit sample =
        treePtr_->findNearest(srcPt, magSqr(bb.max() - bb.centre()), fnOp);

    if (sample.hit())
    {
        targetFacei = sample.index();

        if (debug)
        {
            Pout<< "Source point = " << srcPt << ", Sample point = "
                << sample.hitPoint() << ", Sample index = " << sample.index()
                << endl;
        }
    }

    return targetFacei;
}


void Foam::advancingFrontAMI::appendNbrFaces
(
    const label facei,
    const primitivePatch& patch,
    const DynamicList<label>& visitedFaces,
    DynamicList<label>& faceIDs
) const
{
    static const scalar thetaCos = Foam::cos(degToRad(89.0));

    const labelList& nbrFaces = patch.faceFaces()[facei];

    // Filter out faces already visited from face neighbours
    for (const label nbrFacei : nbrFaces)
    {
        // Prevent addition of face if it is not on the same plane-ish
        if (!visitedFaces.found(nbrFacei) && !faceIDs.found(nbrFacei))
        {
            const vector& n1 = patch.faceNormals()[facei];
            const vector& n2 = patch.faceNormals()[nbrFacei];

            const scalar cosI = n1 & n2;

            if (cosI > thetaCos)
            {
                faceIDs.append(nbrFacei);
            }
        }
    }
}


void Foam::advancingFrontAMI::triangulatePatch
(
    const primitivePatch& patch,
    List<DynamicList<face>>& tris,
    List<scalar>& magSf
) const
{
    const pointField& points = patch.points();
    tris.setSize(patch.size());
    magSf.setSize(patch.size());

    // Using methods that index into existing points
    forAll(patch, facei)
    {
        tris[facei].clear();

        switch (triMode_)
        {
            case faceAreaIntersect::tmFan:
            {
                faceAreaIntersect::triangleFan(patch[facei], tris[facei]);
                break;
            }
            case faceAreaIntersect::tmMesh:
            {
                patch[facei].triangles(points, tris[facei]);
                break;
            }
        }

        const DynamicList<face>& triFaces = tris[facei];
        magSf[facei] = 0;
        for (const face& f : triFaces)
        {
            magSf[facei] +=
                triPointRef
                (
                    points[f[0]],
                    points[f[1]],
                    points[f[2]]
                ).mag();
        }
    }
}


void Foam::advancingFrontAMI::nonConformalCorrection()
{
    if (!requireMatch_ && distributed())
    {
        scalarList newTgtMagSf(std::move(tgtMagSf_));

        // Assign default sizes. Override selected values with calculated
        // values. This is to support ACMI where some of the target faces
        // are never used (so never get sent over and hence never assigned
        // to)
        tgtMagSf_ = tgtPatch0().magFaceAreas();

        for (const labelList& smap : this->extendedTgtMapPtr_->subMap())
        {
            UIndirectList<scalar>(tgtMagSf_, smap) =
                UIndirectList<scalar>(newTgtMagSf, smap);
        }
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::advancingFrontAMI::advancingFrontAMI
(
    const dictionary& dict,
    const bool reverseTarget
)
:
    AMIInterpolation(dict, reverseTarget),
    srcTris_(),
    tgtTris_(),
    extendedTgtPatchPtr_(nullptr),
    extendedTgtFaces_(),
    extendedTgtPoints_(),
    extendedTgtFaceIDs_(),
    extendedTgtMapPtr_(nullptr),
    srcNonOverlap_(),
    triMode_
    (
        faceAreaIntersect::triangulationModeNames_.getOrDefault
        (
            "triMode",
            dict,
            faceAreaIntersect::tmMesh
        )
    )
{}


Foam::advancingFrontAMI::advancingFrontAMI
(
    const bool requireMatch,
    const bool reverseTarget,
    const scalar lowWeightCorrection,
    const faceAreaIntersect::triangulationMode triMode
)
:
    AMIInterpolation(requireMatch, reverseTarget, lowWeightCorrection),
    srcTris_(),
    tgtTris_(),
    extendedTgtPatchPtr_(nullptr),
    extendedTgtFaces_(),
    extendedTgtPoints_(),
    extendedTgtFaceIDs_(),
    extendedTgtMapPtr_(nullptr),
    srcNonOverlap_(),
    triMode_(triMode)
{}


Foam::advancingFrontAMI::advancingFrontAMI(const advancingFrontAMI& ami)
:
    AMIInterpolation(ami),
    srcTris_(),
    tgtTris_(),
    extendedTgtPatchPtr_(nullptr),
    extendedTgtFaces_(),
    extendedTgtPoints_(),
    extendedTgtFaceIDs_(),
    extendedTgtMapPtr_(nullptr),
    srcNonOverlap_(),
    triMode_(ami.triMode_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::advancingFrontAMI::calculate
(
    const primitivePatch& srcPatch,
    const primitivePatch& tgtPatch,
    const autoPtr<searchableSurface>& surfPtr
)
{
    if (AMIInterpolation::calculate(srcPatch, tgtPatch, surfPtr))
    {
        // Create a representation of the target patch that covers the source
        // patch
        if (distributed())
        {
            createExtendedTgtPatch();
        }

        const auto& src = this->srcPatch();
        const auto& tgt = this->tgtPatch();

        // Initialise area magnitudes
        srcMagSf_.setSize(src.size(), 1.0);
        tgtMagSf_.setSize(tgt.size(), 1.0);

        // Source and target patch triangulations
        triangulatePatch(src, srcTris_, srcMagSf_);
        triangulatePatch(tgt, tgtTris_, tgtMagSf_);

        checkPatches();

        // Set initial sizes for weights and addressing - must be done even if
        // returns false below
        srcAddress_.setSize(src.size());
        srcWeights_.setSize(src.size());
        tgtAddress_.setSize(tgt.size());
        tgtWeights_.setSize(tgt.size());

        return true;
    }

    return false;
}


// ************************************************************************* //
