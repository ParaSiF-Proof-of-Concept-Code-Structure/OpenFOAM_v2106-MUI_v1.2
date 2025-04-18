/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2017-2021 OpenCFD Ltd.
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

#include "surfaceFieldValue.H"
#include "fvMesh.H"
#include "emptyPolyPatch.H"
#include "coupledPolyPatch.H"
#include "sampledSurface.H"
#include "mergePoints.H"
#include "uindirectPrimitivePatch.H"
#include "PatchTools.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
namespace fieldValues
{
    defineTypeNameAndDebug(surfaceFieldValue, 0);
    addToRunTimeSelectionTable(fieldValue, surfaceFieldValue, runTime);
    addToRunTimeSelectionTable(functionObject, surfaceFieldValue, dictionary);
}
}
}


const Foam::Enum
<
    Foam::functionObjects::fieldValues::surfaceFieldValue::regionTypes
>
Foam::functionObjects::fieldValues::surfaceFieldValue::regionTypeNames_
({
    { regionTypes::stFaceZone, "faceZone" },
    { regionTypes::stPatch, "patch" },
    { regionTypes::stObject, "functionObjectSurface" },
    { regionTypes::stSampled, "sampledSurface" },
});


const Foam::Enum
<
    Foam::functionObjects::fieldValues::surfaceFieldValue::operationType
>
Foam::functionObjects::fieldValues::surfaceFieldValue::operationTypeNames_
({
    // Normal operations
    { operationType::opNone, "none" },
    { operationType::opMin, "min" },
    { operationType::opMax, "max" },
    { operationType::opSum, "sum" },
    { operationType::opSumMag, "sumMag" },
    { operationType::opSumDirection, "sumDirection" },
    { operationType::opSumDirectionBalance, "sumDirectionBalance" },
    { operationType::opAverage, "average" },
    { operationType::opAreaAverage, "areaAverage" },
    { operationType::opAreaIntegrate, "areaIntegrate" },
    { operationType::opCoV, "CoV" },
    { operationType::opAreaNormalAverage, "areaNormalAverage" },
    { operationType::opAreaNormalIntegrate, "areaNormalIntegrate" },
    { operationType::opUniformity, "uniformity" },

    // Using weighting
    { operationType::opWeightedSum, "weightedSum" },
    { operationType::opWeightedAverage, "weightedAverage" },
    { operationType::opWeightedAreaAverage, "weightedAreaAverage" },
    { operationType::opWeightedAreaIntegrate, "weightedAreaIntegrate" },
    { operationType::opWeightedUniformity, "weightedUniformity" },

    // Using absolute weighting
    { operationType::opAbsWeightedSum, "absWeightedSum" },
    { operationType::opAbsWeightedAverage, "absWeightedAverage" },
    { operationType::opAbsWeightedAreaAverage, "absWeightedAreaAverage" },
    { operationType::opAbsWeightedAreaIntegrate, "absWeightedAreaIntegrate" },
    { operationType::opAbsWeightedUniformity, "absWeightedUniformity" },
});

const Foam::Enum
<
    Foam::functionObjects::fieldValues::surfaceFieldValue::postOperationType
>
Foam::functionObjects::fieldValues::surfaceFieldValue::postOperationTypeNames_
({
    { postOperationType::postOpNone, "none" },
    { postOperationType::postOpMag, "mag" },
    { postOperationType::postOpSqrt, "sqrt" },
});


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

const Foam::objectRegistry&
Foam::functionObjects::fieldValues::surfaceFieldValue::obr() const
{
    if (stObject == regionType_)
    {
        return storedObjects().lookupObject<polySurface>(regionName_);
    }

    return mesh_;
}


void Foam::functionObjects::fieldValues::surfaceFieldValue::setFaceZoneFaces()
{
    // Indices for all matches, already sorted
    const labelList zoneIds
    (
        mesh_.faceZones().indices(selectionNames_)
    );

    // Total number of faces selected
    label numFaces = 0;
    for (const label zoneId : zoneIds)
    {
        numFaces += mesh_.faceZones()[zoneId].size();
    }

    if (zoneIds.empty())
    {
        FatalErrorInFunction
            << type() << ' ' << name() << ": "
            << regionTypeNames_[regionType_] << '(' << regionName_ << "):" << nl
            << "    No matching face zone(s): "
            << flatOutput(selectionNames_)  << nl
            << "    Known face zones: "
            << flatOutput(mesh_.faceZones().names()) << nl
            << exit(FatalError);
    }

    // Could also check this
    #if 0
    if (!returnReduce(bool(numFaces), orOp<bool>()))
    {
        WarningInFunction
            << type() << ' ' << name() << ": "
            << regionTypeNames_[regionType_] << '(' << regionName_ << "):" << nl
            << "    The faceZone specification: "
            << flatOutput(selectionNames_) << nl
            << "    resulted in 0 faces" << nl
            << exit(FatalError);
    }
    #endif

    faceId_.resize(numFaces);
    facePatchId_.resize(numFaces);
    faceFlip_.resize(numFaces);

    numFaces = 0;

    for (const label zoneId : zoneIds)
    {
        const faceZone& fZone = mesh_.faceZones()[zoneId];

        forAll(fZone, i)
        {
            const label meshFacei = fZone[i];
            const bool isFlip = fZone.flipMap()[i];

            // Internal faces
            label faceId = meshFacei;
            label facePatchId = -1;

            // Boundary faces
            if (!mesh_.isInternalFace(meshFacei))
            {
                facePatchId = mesh_.boundaryMesh().whichPatch(meshFacei);
                const polyPatch& pp = mesh_.boundaryMesh()[facePatchId];
                const auto* cpp = isA<coupledPolyPatch>(pp);

                if (cpp)
                {
                    faceId = (cpp->owner() ? pp.whichFace(meshFacei) : -1);
                }
                else if (!isA<emptyPolyPatch>(pp))
                {
                    faceId = pp.whichFace(meshFacei);
                }
                else
                {
                    faceId = -1;
                    facePatchId = -1;
                }
            }

            if (faceId >= 0)
            {
                faceId_[numFaces] = faceId;
                facePatchId_[numFaces] = facePatchId;
                faceFlip_[numFaces] = isFlip;

                ++numFaces;
            }
        }
    }

    // Shrink to size used
    faceId_.resize(numFaces);
    facePatchId_.resize(numFaces);
    faceFlip_.resize(numFaces);
    nFaces_ = returnReduce(faceId_.size(), sumOp<label>());
}


void Foam::functionObjects::fieldValues::surfaceFieldValue::setPatchFaces()
{
    // Patch indices for all matches
    labelList patchIds;

    // Total number of faces selected
    label numFaces = 0;

    labelList selected
    (
        mesh_.boundaryMesh().patchSet
        (
            selectionNames_,
            false  // warnNotFound - we do that ourselves
        ).sortedToc()
    );

    DynamicList<label> bad;
    for (const label patchi : selected)
    {
        const polyPatch& pp = mesh_.boundaryMesh()[patchi];

        if (isA<emptyPolyPatch>(pp))
        {
            bad.append(patchi);
        }
        else
        {
            numFaces += pp.size();
        }
    }

    if (bad.size())
    {
        label nGood = (selected.size() - bad.size());

        auto& os = (nGood > 0 ? WarningInFunction : FatalErrorInFunction);

        os  << "Cannot sample an empty patch" << nl;

        for (const label patchi : bad)
        {
            os  << "    "
                << mesh_.boundaryMesh()[patchi].name() << nl;
        }

        if (nGood)
        {
            os  << "No non-empty patches selected" << endl
                << exit(FatalError);
        }
        else
        {
            os  << "Selected " << nGood << " non-empty patches" << nl;
        }

        patchIds.resize(nGood);
        nGood = 0;
        for (const label patchi : selected)
        {
            if (!bad.found(patchi))
            {
                patchIds[nGood] = patchi;
                ++nGood;
            }
        }
    }
    else
    {
        patchIds = std::move(selected);
    }

    if (patchIds.empty())
    {
        FatalErrorInFunction
            << type() << ' ' << name() << ": "
            << regionTypeNames_[regionType_] << '(' << regionName_ << "):" << nl
            << "    No matching patch name(s): "
            << flatOutput(selectionNames_)  << nl
            << "    Known patch names:" << nl
            << mesh_.boundaryMesh().names() << nl
            << exit(FatalError);
    }

    // Could also check this
    #if 0
    if (!returnReduce(bool(numFaces), orOp<bool>()))
    {
        WarningInFunction
            << type() << ' ' << name() << ": "
            << regionTypeNames_[regionType_] << '(' << regionName_ << "):" << nl
            << "    The patch specification: "
            << flatOutput(selectionNames_) << nl
            << "    resulted in 0 faces" << nl
            << exit(FatalError);
    }
    #endif

    faceId_.resize(numFaces);
    facePatchId_.resize(numFaces);
    faceFlip_.resize(numFaces);
    nFaces_ = returnReduce(faceId_.size(), sumOp<label>());

    numFaces = 0;
    for (const label patchi : patchIds)
    {
        const polyPatch& pp = mesh_.boundaryMesh()[patchi];
        const label len = pp.size();

        SubList<label>(faceId_, len, numFaces) = identity(len);
        SubList<label>(facePatchId_, len, numFaces) = patchi;
        SubList<bool>(faceFlip_, len, numFaces) = false;

        numFaces += len;
    }
}


void Foam::functionObjects::fieldValues::surfaceFieldValue::combineMeshGeometry
(
    faceList& faces,
    pointField& points
) const
{
    List<faceList> allFaces(Pstream::nProcs());
    List<pointField> allPoints(Pstream::nProcs());

    {
        IndirectList<face> selectedFaces(mesh_.faces(), labelList(faceId_));
        labelList& meshFaceIds = selectedFaces.addressing();

        forAll(meshFaceIds, i)
        {
            const label patchi = facePatchId_[i];
            if (patchi != -1)
            {
                meshFaceIds[i] += mesh_.boundaryMesh()[patchi].start();
            }
        }

        // Add local faces and points to the all* lists
        uindirectPrimitivePatch pp(selectedFaces, mesh_.points());

        allFaces[Pstream::myProcNo()] = pp.localFaces();
        allPoints[Pstream::myProcNo()] = pp.localPoints();
    }

    Pstream::gatherList(allFaces);
    Pstream::gatherList(allPoints);

    // Renumber and flatten
    label nFaces = 0;
    label nPoints = 0;
    forAll(allFaces, proci)
    {
        nFaces += allFaces[proci].size();
        nPoints += allPoints[proci].size();
    }

    faces.resize(nFaces);
    points.resize(nPoints);

    nFaces = 0;
    nPoints = 0;

    // My data first
    {
        for (const face& f : allFaces[Pstream::myProcNo()])
        {
            faces[nFaces++] = offsetOp<face>()(f, nPoints);
        }

        for (const point& p : allPoints[Pstream::myProcNo()])
        {
            points[nPoints++] = p;
        }
    }

    // Other proc data follows
    forAll(allFaces, proci)
    {
        if (proci == Pstream::myProcNo())
        {
            continue;
        }

        for (const face& f : allFaces[proci])
        {
            faces[nFaces++] = offsetOp<face>()(f, nPoints);
        }

        for (const point& p : allPoints[proci])
        {
            points[nPoints++] = p;
        }
    }

    // Merge
    labelList oldToNew;
    pointField newPoints;
    bool hasMerged = mergePoints
    (
        points,
        SMALL,
        false,
        oldToNew,
        newPoints
    );

    if (hasMerged)
    {
        if (debug)
        {
            Pout<< "Merged from " << points.size()
                << " down to " << newPoints.size() << " points" << endl;
        }

        points.transfer(newPoints);
        for (face& f : faces)
        {
            inplaceRenumber(oldToNew, f);
        }
    }
}


void Foam::functionObjects::fieldValues::surfaceFieldValue::
combineSurfaceGeometry
(
    faceList& faces,
    pointField& points
) const
{
    if (stObject == regionType_)
    {
        const polySurface& s = dynamicCast<const polySurface>(obr());

        if (Pstream::parRun())
        {
            // Dimension as fraction of surface
            const scalar mergeDim = 1e-10*boundBox(s.points(), true).mag();

            labelList pointsMap;

            PatchTools::gatherAndMerge
            (
                mergeDim,
                primitivePatch(SubList<face>(s.faces()), s.points()),
                points,
                faces,
                pointsMap
            );
        }
        else
        {
            faces = s.faces();
            points = s.points();
        }
    }
    else if (sampledPtr_)
    {
        const sampledSurface& s = *sampledPtr_;

        if (Pstream::parRun())
        {
            // Dimension as fraction of mesh bounding box
            const scalar mergeDim = 1e-10*mesh_.bounds().mag();

            labelList pointsMap;

            PatchTools::gatherAndMerge
            (
                mergeDim,
                primitivePatch(SubList<face>(s.faces()), s.points()),
                points,
                faces,
                pointsMap
            );
        }
        else
        {
            faces = s.faces();
            points = s.points();
        }
    }
}


Foam::scalar
Foam::functionObjects::fieldValues::surfaceFieldValue::totalArea() const
{
    scalar totalArea = 0;

    if (stObject == regionType_)
    {
        const polySurface& s = dynamicCast<const polySurface>(obr());

        totalArea = gSum(s.magSf());
    }
    else if (sampledPtr_)
    {
        totalArea = gSum(sampledPtr_->magSf());
    }
    else
    {
        totalArea = gSum(filterField(mesh_.magSf()));
    }

    return totalArea;
}


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

bool Foam::functionObjects::fieldValues::surfaceFieldValue::usesSf() const
{
    // Only a few operations do not require the Sf field
    switch (operation_)
    {
        case opNone:
        case opMin:
        case opMax:
        case opSum:
        case opSumMag:
        case opAverage:
        {
            return false;
        }
        default:
        {
            return true;
        }
    }
}


bool Foam::functionObjects::fieldValues::surfaceFieldValue::update()
{
    if (sampledPtr_)
    {
        sampledPtr_->update();
    }

    if (!needsUpdate_)
    {
        return false;
    }

    switch (regionType_)
    {
        case stFaceZone:
        {
            setFaceZoneFaces();
            break;
        }
        case stPatch:
        {
            setPatchFaces();
            break;
        }
        case stObject:
        {
            const polySurface& s = dynamicCast<const polySurface>(obr());
            nFaces_ = returnReduce(s.size(), sumOp<label>());
            break;
        }
        case stSampled:
        {
            nFaces_ = returnReduce(sampledPtr_->faces().size(), sumOp<label>());
            break;
        }

        // Compiler warning if we forgot an enumeration
    }

    if (nFaces_ == 0)
    {
        FatalErrorInFunction
            << type() << ' ' << name() << ": "
            << regionTypeNames_[regionType_] << '(' << regionName_ << "):" << nl
            << "    Region has no faces" << exit(FatalError);
    }

    totalArea_ = totalArea();

    Log << "    total faces   = " << nFaces_ << nl
        << "    total area    = " << totalArea_ << nl
        << endl;

    writeFileHeader(file());

    needsUpdate_ = false;
    return true;
}


void Foam::functionObjects::fieldValues::surfaceFieldValue::writeFileHeader
(
    Ostream& os
)
{
    if (canWriteHeader() && (operation_ != opNone))
    {
        writeCommented(os, "Region type : ");
        os << regionTypeNames_[regionType_] << ' ' << regionName_ << nl;

        writeHeaderValue(os, "Faces", nFaces_);
        writeHeaderValue(os, "Area", totalArea_);
        writeHeaderValue(os, "Scale factor", scaleFactor_);

        if (weightFieldNames_.size())
        {
            writeHeaderValue
            (
                os,
                "Weight field",
                flatOutput(weightFieldNames_, FlatOutput::BareComma{})
            );
        }

        writeCommented(os, "Time");
        if (writeArea_)
        {
            os  << tab << "Area";
        }

        // TBD: add in postOperation information?

        for (const word& fieldName : fields_)
        {
            os  << tab << operationTypeNames_[operation_]
                << '(' << fieldName << ')';
        }

        os  << endl;
    }

    writtenHeader_ = true;
}


template<>
Foam::scalar
Foam::functionObjects::fieldValues::surfaceFieldValue::processValues
(
    const Field<scalar>& values,
    const vectorField& Sf,
    const scalarField& weightField
) const
{
    switch (operation_)
    {
        case opSumDirection:
        {
            const vector n(dict_.get<vector>("direction"));
            return gSum(pos0(values*(Sf & n))*mag(values));
        }
        case opSumDirectionBalance:
        {
            const vector n(dict_.get<vector>("direction"));
            const scalarField nv(values*(Sf & n));

            return gSum(pos0(nv)*mag(values) - neg(nv)*mag(values));
        }

        case opUniformity:
        case opWeightedUniformity:
        case opAbsWeightedUniformity:
        {
            const scalar areaTotal = gSum(mag(Sf));
            tmp<scalarField> areaVal(values * mag(Sf));

            scalar mean, numer;

            if (canWeight(weightField))
            {
                // Weighted quantity = (Weight * phi * dA)

                tmp<scalarField> weight(weightingFactor(weightField));

                // Mean weighted value (area-averaged)
                mean = gSum(weight()*areaVal()) / areaTotal;

                // Abs. deviation from weighted mean value
                numer = gSum(mag(weight*areaVal - (mean * mag(Sf))));
            }
            else
            {
                // Unweighted quantity = (1 * phi * dA)

                // Mean value (area-averaged)
                mean = gSum(areaVal()) / areaTotal;

                // Abs. deviation from mean value
                numer = gSum(mag(areaVal - (mean * mag(Sf))));
            }

            // Uniformity index
            const scalar ui = 1 - numer/(2*mag(mean*areaTotal) + ROOTVSMALL);

            return min(max(ui, 0), 1);
        }

        default:
        {
            // Fall through to other operations
            return processSameTypeValues(values, Sf, weightField);
        }
    }
}


template<>
Foam::vector
Foam::functionObjects::fieldValues::surfaceFieldValue::processValues
(
    const Field<vector>& values,
    const vectorField& Sf,
    const scalarField& weightField
) const
{
    switch (operation_)
    {
        case opSumDirection:
        {
            const vector n(dict_.get<vector>("direction").normalise());

            const scalarField nv(n & values);
            return gSum(pos0(nv)*n*(nv));
        }
        case opSumDirectionBalance:
        {
            const vector n(dict_.get<vector>("direction").normalise());

            const scalarField nv(n & values);
            return gSum(pos0(nv)*n*(nv));
        }
        case opAreaNormalAverage:
        {
            const scalar val = gSum(values & Sf)/gSum(mag(Sf));
            return vector(val, 0, 0);
        }
        case opAreaNormalIntegrate:
        {
            const scalar val = gSum(values & Sf);
            return vector(val, 0, 0);
        }

        case opUniformity:
        case opWeightedUniformity:
        case opAbsWeightedUniformity:
        {
            const scalar areaTotal = gSum(mag(Sf));
            tmp<scalarField> areaVal(values & Sf);

            scalar mean, numer;

            if (canWeight(weightField))
            {
                // Weighted quantity = (Weight * phi . dA)

                tmp<scalarField> weight(weightingFactor(weightField));

                // Mean weighted value (area-averaged)
                mean = gSum(weight()*areaVal()) / areaTotal;

                // Abs. deviation from weighted mean value
                numer = gSum(mag(weight*areaVal - (mean * mag(Sf))));
            }
            else
            {
                // Unweighted quantity = (1 * phi . dA)

                // Mean value (area-averaged)
                mean = gSum(areaVal()) / areaTotal;

                // Abs. deviation from mean value
                numer = gSum(mag(areaVal - (mean * mag(Sf))));
            }

            // Uniformity index
            const scalar ui = 1 - numer/(2*mag(mean*areaTotal) + ROOTVSMALL);

            return vector(min(max(ui, 0), 1), 0, 0);
        }

        default:
        {
            // Fall through to other operations
            return processSameTypeValues(values, Sf, weightField);
        }
    }
}


template<>
Foam::tmp<Foam::scalarField>
Foam::functionObjects::fieldValues::surfaceFieldValue::weightingFactor
(
    const Field<scalar>& weightField
) const
{
    if (usesMag())
    {
        return mag(weightField);
    }

    // pass through
    return weightField;
}


template<>
Foam::tmp<Foam::scalarField>
Foam::functionObjects::fieldValues::surfaceFieldValue::weightingFactor
(
    const Field<scalar>& weightField,
    const vectorField& Sf
) const
{
    // scalar * Area

    if (returnReduce(weightField.empty(), andOp<bool>()))
    {
        // No weight field - revert to unweighted form
        return mag(Sf);
    }
    else if (usesMag())
    {
        return mag(weightField * mag(Sf));
    }

    return (weightField * mag(Sf));
}


template<>
Foam::tmp<Foam::scalarField>
Foam::functionObjects::fieldValues::surfaceFieldValue::weightingFactor
(
    const Field<vector>& weightField,
    const vectorField& Sf
) const
{
    // vector (dot) Area

    if (returnReduce(weightField.empty(), andOp<bool>()))
    {
        // No weight field - revert to unweighted form
        return mag(Sf);
    }
    else if (usesMag())
    {
        return mag(weightField & Sf);
    }

    return (weightField & Sf);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::fieldValues::surfaceFieldValue::surfaceFieldValue
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fieldValue(name, runTime, dict, typeName),
    regionType_(regionTypeNames_.get("regionType", dict)),
    operation_(operationTypeNames_.get("operation", dict)),
    postOperation_
    (
        postOperationTypeNames_.getOrDefault
        (
            "postOperation",
            dict,
            postOperationType::postOpNone,
            true  // Failsafe behaviour
        )
    ),
    needsUpdate_(true),
    writeArea_(false),
    selectionNames_(),
    weightFieldNames_(),
    totalArea_(0),
    nFaces_(0),
    faceId_(),
    facePatchId_(),
    faceFlip_()
{
    read(dict);
}


Foam::functionObjects::fieldValues::surfaceFieldValue::surfaceFieldValue
(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict
)
:
    fieldValue(name, obr, dict, typeName),
    regionType_(regionTypeNames_.get("regionType", dict)),
    operation_(operationTypeNames_.get("operation", dict)),
    postOperation_
    (
        postOperationTypeNames_.getOrDefault
        (
            "postOperation",
            dict,
            postOperationType::postOpNone,
            true  // Failsafe behaviour
        )
    ),
    needsUpdate_(true),
    writeArea_(false),
    selectionNames_(),
    weightFieldNames_(),
    totalArea_(0),
    nFaces_(0),
    faceId_(),
    facePatchId_(),
    faceFlip_()
{
    read(dict);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::fieldValues::surfaceFieldValue::read
(
    const dictionary& dict
)
{
    fieldValue::read(dict);

    needsUpdate_ = true;
    writeArea_ = dict.getOrDefault("writeArea", false);
    weightFieldNames_.clear();
    // future?
    // sampleFaceScheme_ = dict.getOrDefault<word>("sampleScheme", "cell");

    totalArea_ = 0;
    nFaces_ = 0;
    faceId_.clear();
    facePatchId_.clear();
    faceFlip_.clear();
    sampledPtr_.reset(nullptr);
    surfaceWriterPtr_.reset(nullptr);

    // Can have "name" (word) and/or "names" (wordRes)
    //
    // If "names" exists AND contains a literal (non-regex) that can be used
    // as a suitable value for "name", the "name" entry becomes optional.

    regionName_.clear();
    selectionNames_.clear();

    {
        dict.readIfPresent("names", selectionNames_);

        for (const auto& item : selectionNames_)
        {
            if (item.isLiteral())
            {
                regionName_ = item;
                break;
            }
        }

        // Mandatory if we didn't pick up a value from selectionNames_
        dict.readEntry
        (
            "name",
            regionName_,
            keyType::LITERAL,
            regionName_.empty()
        );

        // Ensure there is always content for selectionNames_
        if (selectionNames_.empty())
        {
            selectionNames_.resize(1);
            selectionNames_.first() = regionName_;
        }
    }


    // Create sampled surface, but leave 'expired' (ie, no update) since it
    // may depend on fields or data that do not yet exist
    if (stSampled == regionType_)
    {
        sampledPtr_ = sampledSurface::New
        (
            name(),
            mesh_,
            dict.subDict("sampledSurfaceDict")
        );

        // Internal consistency. Want face values, never point values!
        sampledPtr_->isPointData(false);
    }

    Info<< type() << ' ' << name() << ':' << nl
        << "    operation     = ";

    if (postOperation_ != postOpNone)
    {
        Info<< postOperationTypeNames_[postOperation_] << '('
            << operationTypeNames_[operation_] << ')'  << nl;
    }
    else
    {
        Info<< operationTypeNames_[operation_] << nl;
    }

    if (usesWeight())
    {
        // Can have "weightFields" or "weightField"

        bool missing = true;
        if (dict.readIfPresent("weightFields", weightFieldNames_))
        {
            missing = false;
        }
        else
        {
            weightFieldNames_.resize(1);

            if (dict.readIfPresent("weightField", weightFieldNames_.first()))
            {
                missing = false;
                if ("none" == weightFieldNames_.first())
                {
                    // "none" == no weighting
                    weightFieldNames_.clear();
                }
            }
        }

        if (missing)
        {
            // Suggest possible alternative unweighted operation?
            FatalIOErrorInFunction(dict)
                << "The '" << operationTypeNames_[operation_]
                << "' operation is missing a weightField." << nl
                << "Either provide the weightField, "
                << "use weightField 'none' to suppress weighting," << nl
                << "or use a different operation."
                << exit(FatalIOError);
        }

        Info<< "    weight field  = ";
        if (weightFieldNames_.empty())
        {
            Info<< "none" << nl;
        }
        else
        {
            Info<< flatOutput(weightFieldNames_) << nl;
        }
    }

    if (writeFields_)
    {
        const word formatName(dict.get<word>("surfaceFormat"));

        surfaceWriterPtr_.reset
        (
            surfaceWriter::New
            (
                formatName,
                dict.subOrEmptyDict("formatOptions").subOrEmptyDict(formatName)
            )
        );

        // Propagate field counts (per surface)
        surfaceWriterPtr_->nFields(fields_.size());

        if (debug)
        {
            surfaceWriterPtr_->verbose(true);
        }

        if (surfaceWriterPtr_->enabled())
        {
            Info<< "    surfaceFormat = " << formatName << nl;
        }
        else
        {
            surfaceWriterPtr_->clear();
        }
    }

    Info<< nl << endl;

    return true;
}


bool Foam::functionObjects::fieldValues::surfaceFieldValue::write()
{
    if (needsUpdate_ || operation_ != opNone)
    {
        fieldValue::write();
    }

    update();

    if (operation_ != opNone)
    {
        writeCurrentTime(file());
    }

    if (writeArea_)
    {
        totalArea_ = totalArea();
        Log << "    total area = " << totalArea_ << endl;

        if (operation_ != opNone && Pstream::master())
        {
            file() << tab << totalArea_;
        }
    }

    // Many operations use the Sf field
    vectorField Sf;
    if (usesSf())
    {
        if (stObject == regionType_)
        {
            const polySurface& s = dynamicCast<const polySurface>(obr());
            Sf = s.Sf();
        }
        else if (sampledPtr_)
        {
            Sf = sampledPtr_->Sf();
        }
        else
        {
            Sf = filterField(mesh_.Sf());
        }
    }

    // Faces and points for surface format (if specified)
    faceList faces;
    pointField points;

    if (surfaceWriterPtr_)
    {
        if (withTopologicalMerge())
        {
            combineMeshGeometry(faces, points);
        }
        else
        {
            combineSurfaceGeometry(faces, points);
        }
    }


    // Check availability and type of weight field
    // Only support a few weight types:
    // scalar: 0-N fields
    // vector: 0-1 fields

    // Default is a zero-size scalar weight field (ie, weight = 1)
    scalarField scalarWeights;
    vectorField vectorWeights;

    for (const word& weightName : weightFieldNames_)
    {
        if (validField<scalar>(weightName))
        {
            tmp<scalarField> tfld = getFieldValues<scalar>(weightName, true);

            if (scalarWeights.empty())
            {
                scalarWeights = tfld;
            }
            else
            {
                scalarWeights *= tfld;
            }
        }
        else if (validField<vector>(weightName))
        {
            tmp<vectorField> tfld = getFieldValues<vector>(weightName, true);

            if (vectorWeights.empty())
            {
                vectorWeights = tfld;
            }
            else
            {
                FatalErrorInFunction
                    << "weightField " << weightName
                    << " - only one vector weight field allowed. " << nl
                    << "weights: " << flatOutput(weightFieldNames_) << nl
                    << abort(FatalError);
            }
        }
        else if (weightName != "none")
        {
            // Silently ignore "none", flag everything else as an error

            // TBD: treat missing "rho" like incompressible with rho = 1
            // and/or provided rhoRef value

            FatalErrorInFunction
                << "weightField " << weightName
                << " not found or an unsupported type" << nl
                << abort(FatalError);
        }
    }


    // Process the fields
    if (returnReduce(!vectorWeights.empty(), orOp<bool>()))
    {
        if (scalarWeights.size())
        {
            vectorWeights *= scalarWeights;
        }

        writeAll(Sf, vectorWeights, points, faces);
    }
    else
    {
        writeAll(Sf, scalarWeights, points, faces);
    }


    if (operation_ != opNone)
    {
        file() << endl;
        Log << endl;
    }


    return true;
}


void Foam::functionObjects::fieldValues::surfaceFieldValue::updateMesh
(
    const mapPolyMesh& mpm
)
{
    needsUpdate_ = true;
}


void Foam::functionObjects::fieldValues::surfaceFieldValue::movePoints
(
    const polyMesh& mesh
)
{
    needsUpdate_ = true;
}


// ************************************************************************* //
