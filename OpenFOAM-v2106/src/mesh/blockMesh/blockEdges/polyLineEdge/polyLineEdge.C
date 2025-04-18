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

#include "polyLineEdge.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace blockEdges
{
    defineTypeNameAndDebug(polyLineEdge, 0);
    addToRunTimeSelectionTable(blockEdge, polyLineEdge, Istream);
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::blockEdges::polyLineEdge::polyLineEdge
(
    const pointField& ps,
    const label start,
    const label end,
    const pointField& intermediate
)
:
    blockEdge(ps, start, end),
    polyLine(ps[start_], intermediate, ps[end_])
{}


Foam::blockEdges::polyLineEdge::polyLineEdge
(
    const dictionary& dict,
    const label index,
    const searchableSurfaces& geometry,
    const pointField& ps,
    Istream& is
)
:
    blockEdge(dict, index, ps, is),
    polyLine(ps[start_], pointField(is), ps[end_])
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::point Foam::blockEdges::polyLineEdge::position(const scalar lambda) const
{
    return polyLine::position(lambda);
}


Foam::scalar Foam::blockEdges::polyLineEdge::length() const
{
    return polyLine::lineLength_;
}


// ************************************************************************* //
