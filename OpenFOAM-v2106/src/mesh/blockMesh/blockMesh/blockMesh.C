/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2018-2020 OpenCFD Ltd.
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

#include "blockMesh.H"
#include "Time.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineDebugSwitch(blockMesh, 0);
}

bool Foam::blockMesh::verboseOutput = true;


const Foam::Enum<Foam::blockMesh::mergeStrategy>
Foam::blockMesh::strategyNames_
({
    { mergeStrategy::MERGE_TOPOLOGY, "topology" },
    { mergeStrategy::MERGE_POINTS, "points" },
});


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::blockMesh::blockMesh
(
    const IOdictionary& dict,
    const word& regionName,
    mergeStrategy strategy
)
:
    meshDict_(dict),
    verbose_(meshDict_.getOrDefault("verbose", verboseOutput)),
    checkFaceCorrespondence_
    (
        meshDict_.getOrDefault("checkFaceCorrespondence", true)
    ),
    geometry_
    (
        IOobject
        (
            "geometry",                 // dummy name
            meshDict_.time().constant(), // instance
            "geometry",                 // local
            meshDict_.time(),           // registry
            IOobject::MUST_READ,
            IOobject::NO_WRITE
        ),
        meshDict_.found("geometry")
      ? meshDict_.subDict("geometry")
      : dictionary(),
        true
    ),
    scaleFactor_(1),
    blockVertices_
    (
        meshDict_.lookup("vertices"),
        blockVertex::iNew(meshDict_, geometry_)
    ),
    vertices_(Foam::vertices(blockVertices_)),
    topologyPtr_(createTopology(meshDict_, regionName))
{
    // Command-line option has precedence over dictionary setting

    if (strategy == mergeStrategy::DEFAULT_MERGE)
    {
        strategyNames_.readIfPresent("mergeType", meshDict_, strategy);

        // Warn about fairly obscure old "fastMerge" option?
    }

    if (strategy == mergeStrategy::MERGE_POINTS)
    {
        // MERGE_POINTS
        calcGeometricalMerge();
    }
    else
    {
        // MERGE_TOPOLOGY
        calcTopologicalMerge();
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::blockMesh::valid() const noexcept
{
    return bool(topologyPtr_);
}


bool Foam::blockMesh::verbose() const noexcept
{
    return verbose_;
}


bool Foam::blockMesh::verbose(const bool on) noexcept
{
    bool old(verbose_);
    verbose_ = on;
    return old;
}


const Foam::pointField& Foam::blockMesh::vertices() const
{
    return vertices_;
}


const Foam::polyMesh& Foam::blockMesh::topology() const
{
    if (!topologyPtr_)
    {
        FatalErrorInFunction
            << "topologyPtr_ not allocated"
            << exit(FatalError);
    }

    return *topologyPtr_;
}


Foam::PtrList<Foam::dictionary> Foam::blockMesh::patchDicts() const
{
    const polyPatchList& patchTopologies = topology().boundaryMesh();

    PtrList<dictionary> patchDicts(patchTopologies.size());

    forAll(patchTopologies, patchi)
    {
        OStringStream os;
        patchTopologies[patchi].write(os);
        IStringStream is(os.str());
        patchDicts.set(patchi, new dictionary(is));
    }
    return patchDicts;
}


Foam::scalar Foam::blockMesh::scaleFactor() const
{
    return scaleFactor_;
}


const Foam::pointField& Foam::blockMesh::points() const
{
    if (points_.empty())
    {
        createPoints();
    }

    return points_;
}


const Foam::cellShapeList& Foam::blockMesh::cells() const
{
    if (cells_.empty())
    {
        createCells();
    }

    return cells_;
}


const Foam::faceListList& Foam::blockMesh::patches() const
{
    if (patches_.empty())
    {
        createPatches();
    }

    return patches_;
}


Foam::wordList Foam::blockMesh::patchNames() const
{
    return topology().boundaryMesh().names();
}


//Foam::wordList Foam::blockMesh::patchTypes() const
//{
//    return topology().boundaryMesh().types();
//}
//
//
//Foam::wordList Foam::blockMesh::patchPhysicalTypes() const
//{
//    return topology().boundaryMesh().physicalTypes();
//}


Foam::label Foam::blockMesh::numZonedBlocks() const
{
    const blockList& blocks = *this;

    label count = 0;

    for (const block& blk : blocks)
    {
        if (blk.zoneName().size())
        {
            ++count;
        }
    }

    return count;
}


// ************************************************************************* //
