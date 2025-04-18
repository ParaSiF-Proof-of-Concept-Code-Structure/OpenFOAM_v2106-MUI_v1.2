/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2020-2021 OpenCFD Ltd.
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

#include "topoSetFaceZoneSource.H"
#include "polyMesh.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineRunTimeSelectionTable(topoSetFaceZoneSource, word);
    defineRunTimeSelectionTable(topoSetFaceZoneSource, istream);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::topoSetFaceZoneSource::topoSetFaceZoneSource(const polyMesh& mesh)
:
    topoSetSource(mesh)
{}


Foam::topoSetFaceZoneSource::topoSetFaceZoneSource
(
    const polyMesh& mesh,
    const dictionary& dict
)
:
    topoSetSource(mesh, dict)
{}


// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

Foam::autoPtr<Foam::topoSetFaceZoneSource>
Foam::topoSetFaceZoneSource::New
(
    const word& sourceType,
    const polyMesh& mesh,
    const dictionary& dict
)
{
    auto cstrIter = wordConstructorTablePtr_->cfind(sourceType);

    if (!cstrIter.found())
    {
        FatalIOErrorInLookup
        (
            dict,
            "faceZoneSource",
            sourceType,
            *wordConstructorTablePtr_
        ) << exit(FatalIOError);
    }

    return autoPtr<topoSetFaceZoneSource>(cstrIter()(mesh, dict));
}


Foam::autoPtr<Foam::topoSetFaceZoneSource>
Foam::topoSetFaceZoneSource::New
(
    const word& sourceType,
    const polyMesh& mesh,
    Istream& is
)
{
    auto cstrIter = istreamConstructorTablePtr_->cfind(sourceType);

    if (!cstrIter.found())
    {
        FatalErrorInLookup
        (
            "faceZoneSource",
            sourceType,
            *istreamConstructorTablePtr_
        ) << exit(FatalError);
    }

    return autoPtr<topoSetFaceZoneSource>(cstrIter()(mesh, is));
}


// ************************************************************************* //
