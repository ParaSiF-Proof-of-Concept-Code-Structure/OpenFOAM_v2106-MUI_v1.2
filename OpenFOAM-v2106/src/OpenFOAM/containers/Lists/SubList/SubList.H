/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017-2021 OpenCFD Ltd.
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
    Foam::SubList

Description
    A List obtained as a section of another List.

    Since the SubList is itself unallocated, no storage is allocated or
    de-allocated during its use.  To achieve this behaviour, SubList is
    derived from UList rather than List.

SourceFiles
    SubListI.H

\*---------------------------------------------------------------------------*/

#ifndef SubList_H
#define SubList_H

#include "List.H"
#include "labelRange.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
template<class T> class SubList;

// Common list types
typedef SubList<bool> boolSubList;      //!< A SubList of bools
typedef SubList<char> charSubList;      //!< A SubList of chars
typedef SubList<label> labelSubList;    //!< A SubList of labels


/*---------------------------------------------------------------------------*\
                           Class SubList Declaration
\*---------------------------------------------------------------------------*/

template<class T>
class SubList
:
    public UList<T>
{
public:

    // Static Functions

        //- Return a null SubList
        inline static const SubList<T>& null();


    // Generated Methods

        //- Copy construct
        SubList(const SubList&) = default;


    // Constructors

        //- Construct from UList, the entire size
        inline explicit SubList(const UList<T>& list);

        //- Construct from UList and sub-list size, start at 0
        inline SubList
        (
            const UList<T>& list,
            const label subSize
        );

        //- Construct from UList, sub-list size and start index
        inline SubList
        (
            const UList<T>& list,
            const label subSize,
            const label startIndex
        );

        //- Construct from UList and a (start,size) range.
        //  The range is subsetted with the list size itself to ensure that the
        //  result always addresses a valid section of the list.
        inline SubList
        (
            const UList<T>& list,
            const labelRange& range
        );

        //- Construct from UList and a (start,size) range, but bypassing
        //- run-time range checking.
        inline SubList
        (
            const labelRange& range,
            const UList<T>& list
        );


    // Member Operators

        //- Allow cast to a const List\<T\>&
        inline operator const Foam::List<T>&() const;

        //- Copy assign entries from given sub-list. Sizes must match!
        inline void operator=(const SubList<T>& list);

        //- Copy assign entries from given list. Sizes must match!
        inline void operator=(const UList<T>& list);

        //- Copy assign entries from given indirect list. Sizes must match!
        template<class Addr>
        inline void operator=(const IndirectListBase<T, Addr>& list);

        //- Assign all entries to the given value
        inline void operator=(const T& val);

        //- Assign all entries to zero
        inline void operator=(const Foam::zero);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "SubListI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
