/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2019-2020 OpenCFD Ltd.
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

#include "lineEdge.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace blockEdges
{
    defineTypeNameAndDebug(lineEdge, 0);
    addToRunTimeSelectionTable(blockEdge, lineEdge, Istream);
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::blockEdges::lineEdge::lineEdge
(
    const pointField& points,
    const label start,
    const label end
)
:
    blockEdge(points, start, end)
{}


Foam::blockEdges::lineEdge::lineEdge
(
    const dictionary& dict,
    const label index,
    const searchableSurfaces& geometry,
    const pointField& points,
    Istream& is
)
:
    blockEdge(dict, index, points, is)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::point Foam::blockEdges::lineEdge::position(const scalar lambda) const
{
    #ifdef FULLDEBUG
    if (lambda < -SMALL || lambda > 1 + SMALL)
    {
        InfoInFunction
            << "Limit parameter to [0-1] range: " << lambda << nl;
    }
    #endif

    if (lambda < SMALL)
    {
        return points_[start_];
    }
    else if (lambda >= 1 - SMALL)
    {
        return points_[end_];
    }

    return points_[start_] + lambda * (points_[end_] - points_[start_]);
}


Foam::scalar Foam::blockEdges::lineEdge::length() const
{
    return mag(points_[end_] - points_[start_]);
}


// ************************************************************************* //
