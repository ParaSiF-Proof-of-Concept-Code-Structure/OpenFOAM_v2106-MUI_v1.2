/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Class
    Foam::PatchFunction1Types::UniformValueField

Description
    Templated function that returns a uniform field based on a run-time
    selectable Function1 entry.

    Usage - for entry \<entryName\> returning the value <value>:
    \verbatim
        <entryName>    uniformValue
    \endverbatim

SourceFiles
    UniformValueField.C

\*---------------------------------------------------------------------------*/

#ifndef PatchFunction1Types_UniformValueField_H
#define PatchFunction1Types_UniformValueField_H

#include "PatchFunction1.H"
#include "Function1.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace PatchFunction1Types
{

/*---------------------------------------------------------------------------*\
                     Class UniformValueField Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class UniformValueField
:
    public PatchFunction1<Type>
{
    // Private Data

        //- Source of uniform values (in local coordinate system)
        autoPtr<Foam::Function1<Type>> uniformValuePtr_;

public:

    // Runtime type information
    TypeName("uniformValue");


    // Generated Methods

        //- No copy assignment
        void operator=(const UniformValueField<Type>&) = delete;


    // Constructors

        //- Construct from entry name and dictionary
        UniformValueField
        (
            const polyPatch& pp,
            const word& redirectType,
            const word& entryName,
            const dictionary& dict,
            const bool faceValues = true
        );

        //- Copy construct
        explicit UniformValueField(const UniformValueField<Type>& rhs);

        //- Copy construct setting patch
        explicit UniformValueField
        (
            const UniformValueField<Type>& rhs,
            const polyPatch& pp
        );

        //- Construct and return a clone
        virtual tmp<PatchFunction1<Type>> clone() const
        {
            return tmp<PatchFunction1<Type>>
            (
                new UniformValueField<Type>(*this)
            );
        }

        //- Construct and return a clone setting patch
        virtual tmp<PatchFunction1<Type>> clone(const polyPatch& pp) const
        {
            return tmp<PatchFunction1<Type>>
            (
                new UniformValueField<Type>(*this, pp)
            );
        }


    //- Destructor
    virtual ~UniformValueField() = default;


    // Member Functions

        // Evaluation

            //- Return UniformValueField value
            virtual inline tmp<Field<Type>> value(const scalar x) const;

            //- Is value constant (i.e. independent of x)
            virtual inline bool constant() const;

            //- Is value uniform (i.e. independent of coordinate)
            virtual inline bool uniform() const
            {
                return PatchFunction1<Type>::uniform();
            }

            //- Integrate between two values
            virtual inline tmp<Field<Type>> integrate
            (
                const scalar x1,
                const scalar x2
            ) const;


        // Mapping

            //- Map (and resize as needed) from self given a mapping object
            virtual void autoMap(const FieldMapper& mapper);

            //- Reverse map the given PatchFunction1 onto this PatchFunction1
            virtual void rmap
            (
                const PatchFunction1<Type>& pf1,
                const labelList& addr
            );


        // I-O

            //- Write in dictionary format
            virtual void writeData(Ostream& os) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace PatchFunction1Types
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "UniformValueFieldI.H"

#ifdef NoRepository
    #include "UniformValueField.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
