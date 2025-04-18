/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016 OpenFOAM Foundation
    Copyright (C) 2016-2021 OpenCFD Ltd.
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

#include "volRegion.H"
#include "volMesh.H"
#include "cellSet.H"
#include "globalMeshData.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(volRegion, 0);
}
}


const Foam::Enum
<
    Foam::functionObjects::volRegion::regionTypes
>
Foam::functionObjects::volRegion::regionTypeNames_
({
    { regionTypes::vrtAll, "all" },
    { regionTypes::vrtCellSet, "cellSet" },
    { regionTypes::vrtCellZone, "cellZone" },
});


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::functionObjects::volRegion::calculateCache()
{
    regionID_ = -1;
    cellIds_.clear();

    switch (regionType_)
    {
        case vrtAll:
        {
            nCells_ = volMesh_.globalData().nTotalCells();
            V_ = gSum(volMesh_.V());
            return;
            break;
        }

        case vrtCellSet:
        {
            cellIds_ = cellSet(volMesh_, regionName_).sortedToc();
            break;
        }

        case vrtCellZone:
        {
            regionID_ = volMesh_.cellZones().findZoneID(regionName_);

            if (regionID_ < 0)
            {
                FatalErrorInFunction
                    << "Unknown cell zone name: " << regionName_
                    << ". Valid cell zones    : "
                    << flatOutput(volMesh_.cellZones().names())
                    << exit(FatalError);
            }
            break;
        }
    }

    // Cached value for nCells()
    nCells_ = returnReduce(cellIDs().size(), sumOp<label>());

    // Cached value for V()
    V_ = 0;
    for (const label celli : cellIDs())
    {
        V_ += volMesh_.V()[celli];
    }
    reduce(V_, sumOp<scalar>());

    if (!nCells_)
    {
        FatalErrorInFunction
            << regionTypeNames_[regionType_]
            << '(' << regionName_ << "):" << nl
            << "    Region has no cells" << nl
            << exit(FatalError);
    }

    requireUpdate_ = false;
}


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void Foam::functionObjects::volRegion::writeFileHeader
(
    const writeFile& wf,
    Ostream& file
) const
{
    wf.writeCommented(file, "Region");
    file<< setw(1) << ':' << setw(1) << ' '
        << regionTypeNames_[regionType_] << ' ' << regionName_ << endl;
    wf.writeHeaderValue(file, "Cells", nCells_);
    wf.writeHeaderValue(file, "Volume", V_);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::volRegion::volRegion
(
    const fvMesh& mesh,
    const dictionary& dict
)
:
    volMesh_(mesh),
    requireUpdate_(true),
    cellIds_(),
    nCells_(0),
    V_(Zero),
    regionType_
    (
        regionTypeNames_.getOrDefault
        (
            "regionType",
            dict,
            regionTypes::vrtAll
        )
    ),
    regionName_(volMesh_.name()),
    regionID_(-1)
{
    read(dict);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::volRegion::read(const dictionary& dict)
{
    switch (regionType_)
    {
        case vrtAll:
        {
            regionName_ = volMesh_.name();
            break;
        }

        case vrtCellSet:
        case vrtCellZone:
        {
            dict.readEntry("name", regionName_);
            break;
        }

        default:
        {
            FatalIOErrorInFunction(dict)
                << "Unknown region type. Valid region types: "
                << flatOutput(regionTypeNames_.names()) << nl
                << exit(FatalIOError);
            break;
        }
    }

    calculateCache();
    return true;
}


const Foam::labelList& Foam::functionObjects::volRegion::cellIDs() const
{
    #ifdef FULLDEBUG
    if (requireUpdate_)
    {
        FatalErrorInFunction
            << "Retrieving cached values that are not up-to-date" << nl
            << exit(FatalError);
    }
    #endif

    switch (regionType_)
    {
        case vrtCellSet:
            return cellIds_;
            break;

        case vrtCellZone:
            return volMesh_.cellZones()[regionID_];
            break;

        default:
            break;
    }

    return labelList::null();
}


bool Foam::functionObjects::volRegion::update()
{
    if (requireUpdate_)
    {
        calculateCache();
        return true;
    }

    return false;
}


void Foam::functionObjects::volRegion::updateMesh(const mapPolyMesh&)
{
    requireUpdate_ = true;
}


void Foam::functionObjects::volRegion::movePoints(const polyMesh&)
{
    requireUpdate_ = true;
}


// ************************************************************************* //
