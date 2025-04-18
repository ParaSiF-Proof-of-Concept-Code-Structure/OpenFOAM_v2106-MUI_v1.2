/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2017 OpenFOAM Foundation
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

Class
    Foam::Function1Types::halfCosineRamp

Description
    Half-cosine ramp function starting from 0 and increasing to 1 from \c start
    over the \c duration and remaining at 1 thereafter.

See also
    Foam::Function1Types::ramp

SourceFiles
    halfCosineRamp.C

\*---------------------------------------------------------------------------*/

#ifndef Function1Types_halfCosineRamp_H
#define Function1Types_halfCosineRamp_H

#include "ramp.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace Function1Types
{

/*---------------------------------------------------------------------------*\
                       Class halfCosineRamp Declaration
\*---------------------------------------------------------------------------*/

class halfCosineRamp
:
    public ramp
{
public:

    //- Runtime type information
    TypeName("halfCosineRamp");


    // Constructors

        //- Construct from entry name and dictionary
        halfCosineRamp(const word& entryName, const dictionary& dict);


    //- Destructor
    virtual ~halfCosineRamp() = default;


    // Member Functions

        //- Return value for time t
        virtual inline scalar value(const scalar t) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Function1Types
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "halfCosineRampI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
