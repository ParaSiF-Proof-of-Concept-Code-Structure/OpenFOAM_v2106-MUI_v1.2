/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2019-2021 OpenCFD Ltd.
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

Description
    Template for use with dynamic code generation of a
    mixed fvPatchField.

    - without state

SourceFiles
    mixedFvPatchFieldTemplate.C

\*---------------------------------------------------------------------------*/

#ifndef coded_mixedFvPatchTemplate${FieldType}_H
#define coded_mixedFvPatchTemplate${FieldType}_H

#include "mixedFvPatchFields.H"
#include "dictionaryContent.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     A templated MixedValueFvPatchField
\*---------------------------------------------------------------------------*/

class ${typeName}MixedValueFvPatch${FieldType}
:
    public mixedFvPatchField<${TemplateType}>,
    public dictionaryContent
{
    //- The parent boundary condition type
    typedef mixedFvPatchField<${TemplateType}> parent_bctype;


    // Private Member Functions

        //- Report a message with the SHA1sum
        inline static void printMessage(const char* message)
        {
            Info<< message << " sha1: " << SHA1sum << '\n';
        }

public:

    //- SHA1 representation of the code content
    static constexpr const char* const SHA1sum = "${SHA1sum}";

    //- Runtime type information
    TypeName("${typeName}");


    // Constructors

        //- Construct from patch and internal field
        ${typeName}MixedValueFvPatch${FieldType}
        (
            const fvPatch&,
            const DimensionedField<${TemplateType}, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        ${typeName}MixedValueFvPatch${FieldType}
        (
            const fvPatch&,
            const DimensionedField<${TemplateType}, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping a copy onto a new patch
        ${typeName}MixedValueFvPatch${FieldType}
        (
            const ${typeName}MixedValueFvPatch${FieldType}&,
            const fvPatch&,
            const DimensionedField<${TemplateType}, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        ${typeName}MixedValueFvPatch${FieldType}
        (
            const ${typeName}MixedValueFvPatch${FieldType}&
        );

        //- Construct and return a clone
        virtual tmp<fvPatch${FieldType}> clone() const
        {
            return tmp<fvPatch${FieldType}>
            (
                new ${typeName}MixedValueFvPatch${FieldType}(*this)
            );
        }

        //- Construct as copy setting internal field reference
        ${typeName}MixedValueFvPatch${FieldType}
        (
            const ${typeName}MixedValueFvPatch${FieldType}&,
            const DimensionedField<${TemplateType}, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatch${FieldType}> clone
        (
            const DimensionedField<${TemplateType}, volMesh>& iF
        ) const
        {
            return tmp<fvPatch${FieldType}>
            (
                new ${typeName}MixedValueFvPatch${FieldType}(*this, iF)
            );
        }


    //- Destructor
    virtual ~${typeName}MixedValueFvPatch${FieldType}();


    // Member Functions

        //- Code context as a dictionary
        const dictionary& codeContext() const
        {
            return dictionaryContent::dict();
        }

        //- Update the coefficients associated with the patch field
        virtual void updateCoeffs();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
