/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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
    Foam::SubField

Description
    SubField is a Field obtained as a section of another Field.

    Thus it is itself unallocated so that no storage is allocated or
    deallocated during its use.  To achieve this behaviour, SubField is
    derived from a SubList rather than a List.

SourceFiles
    SubFieldI.H

\*---------------------------------------------------------------------------*/

#ifndef SubField_H
#define SubField_H

#include "SubList.H"
#include "Field.H"
#include "VectorSpace.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declarations
template<class Type> class Field;
template<class Type> class SubField;

/*---------------------------------------------------------------------------*\
                           Class SubField Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class SubField
:
    public refCount,
    public SubList<Type>
{

public:

    //- Component type
    typedef typename pTraits<Type>::cmptType cmptType;


    // Constructors

        //- Copy construct (shallow copy)
        inline SubField(const SubField<Type>& sfield);

        //- Copy construct from SubList
        inline SubField(const SubList<Type>& list);

        //- Construct from UList, the entire size
        inline explicit SubField(const UList<Type>& list);

        //- Construct from UList with a given sub-list size, start at 0
        inline SubField
        (
            const UList<Type>& list,
            const label subSize
        );

        //- Construct from UList with a given size and start index
        inline SubField
        (
            const UList<Type>& list,
            const label subSize,
            const label startIndex
        );

        //- Construct from UList and a (start,size) range.
        //  The range is subsetted with the list size itself to ensure that the
        //  result always addresses a valid section of the list.
        inline SubField
        (
            const UList<Type>& list,
            const labelRange& range
        );

        //- Construct from UList and a (start,size) range, but bypassing
        //- run-time range checking.
        inline SubField
        (
            const labelRange& range,
            const UList<Type>& list
        );


    // Member Functions

        //- Return a null SubField
        static inline const SubField<Type>& null();

        //- Return a component field of the field
        inline tmp<Field<cmptType>> component(const direction) const;

        //- Return the field transpose (only defined for second rank tensors)
        tmp<Field<Type>> T() const;


    // Member Operators

        //- Allow cast to a const Field\<Type\>&
        inline operator const Foam::Field<Type>&() const;

        //- Copy assign via UList operator. Takes linear time.
        inline void operator=(const SubField<Type>&);

        //- Copy assign via UList operator. Takes linear time.
        inline void operator=(const Field<Type>&);

        //- Assign all entries to the given value
        inline void operator=(const Type& val);

        //- Assign all entries to zero
        inline void operator=(const Foam::zero);

        //- Copy assign via UList operator. Takes linear time.
        template<class Form, direction Ncmpts>
        inline void operator=(const VectorSpace<Form, Type, Ncmpts>& rhs);

        //- Add value to each entry
        inline void operator+=(const Type& val);

        //- Subtract value from each entry
        inline void operator-=(const Type& val);

        //- Multiply each entry by value
        inline void operator*=(const scalar& s);

        //- Divide each entry by value
        inline void operator/=(const scalar& s);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "SubFieldI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
