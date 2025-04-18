/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Class
    Foam::exprValuePointPatchField

Description
    A fixed value point boundary condition with expressions.

Usage
    \table
        Property     | Description                          | Required | Default
        value        | fixed value                          | yes |
        valueExpr    | expression for uniformValue          | yes |
    \endtable

SourceFiles
    exprValuePointPatchField.C

\*---------------------------------------------------------------------------*/

#ifndef exprValuePointPatchField_H
#define exprValuePointPatchField_H

#include "valuePointPatchField.H"
#include "patchExprFieldBase.H"
#include "patchExprDriver.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                  Class exprValuePointPatchField Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class exprValuePointPatchField
:
    public valuePointPatchField<Type>,
    public expressions::patchExprFieldBase
{
protected:

    // Protected Data

        //- The expression driver
        expressions::patchExpr::parseDriver driver_;


public:

    //- Runtime type information
    TypeName("exprValue");


    // Constructors

        //- Construct from patch and internal field
        exprValuePointPatchField
        (
            const pointPatch&,
            const DimensionedField<Type, pointMesh>&
        );

        //- Construct from patch, internal field and dictionary
        exprValuePointPatchField
        (
            const pointPatch&,
            const DimensionedField<Type, pointMesh>&,
            const dictionary&
        );

        //- Construct by mapping onto a new patch
        exprValuePointPatchField
        (
            const exprValuePointPatchField<Type>&,
            const pointPatch&,
            const DimensionedField<Type, pointMesh>&,
            const pointPatchFieldMapper&
        );

        //- Construct as copy setting internal field reference
        exprValuePointPatchField
        (
            const exprValuePointPatchField<Type>&,
            const DimensionedField<Type, pointMesh>&
        );

        //- Construct as copy
        exprValuePointPatchField
        (
            const exprValuePointPatchField<Type>&
        );


        //- Construct and return a clone
        virtual autoPtr<pointPatchField<Type>> clone() const
        {
            return autoPtr<pointPatchField<Type>>
            (
                new exprValuePointPatchField<Type>
                (
                    *this
                )
            );
        }


        //- Construct and return a clone setting internal field reference
        virtual autoPtr<pointPatchField<Type>> clone
        (
            const DimensionedField<Type, pointMesh>& iF
        ) const
        {
            return autoPtr<pointPatchField<Type>>
            (
                new exprValuePointPatchField<Type>
                (
                    *this,
                    iF
                )
            );
        }


    // Member Functions

        //- Update the patch field
        virtual void updateCoeffs();

        //- Write
        virtual void write(Ostream& os) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
#    include "exprValuePointPatchField.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
