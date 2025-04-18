/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2020 OpenCFD Ltd.
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

#include "fvGeometryScheme.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(fvGeometryScheme, 0);

    defineRunTimeSelectionTable(fvGeometryScheme, dict);
}


// * * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * //

Foam::tmp<Foam::fvGeometryScheme>
Foam::fvGeometryScheme::New
(
    const fvMesh& mesh,
    const dictionary& dict,
    const word& defaultScheme
)
{
    const entry* ePtr = dict.findEntry("method");
    const word schemeName
    (
        ePtr
      ? word(ePtr->stream())
      : dict.getOrDefault<word>("type", defaultScheme)
    );

    if (debug)
    {
        InfoInFunction << "Geometry scheme = " << schemeName << endl;
    }

    auto cstrIter = dictConstructorTablePtr_->cfind(schemeName);

    if (!cstrIter.found())
    {
        FatalIOErrorInLookup
        (
            dict,
            "fvGeometryScheme",
            schemeName,
            *dictConstructorTablePtr_
        ) << exit(FatalIOError);
    }

    return cstrIter()(mesh, dict);
}


// ************************************************************************* //
