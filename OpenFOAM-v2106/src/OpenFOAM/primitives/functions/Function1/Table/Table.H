/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
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

Class
    Foam::Function1Types::Table

Description
    Templated table container function.

    Items are stored in a list of Tuple2, with the first column always
    being a scalar and the second column (the lookup value) in
    required data type.

    Usage:
    Inline specification.
    \verbatim
        <entryName>   table
        (
            (0.0 (1 2 3))
            (1.0 (4 5 6))
        );
    \endverbatim

    Dictionary specification, external data reference.
    \verbatim
        <entryName>
        {
            type table;
            file "<case>/path/tableValues";
        }
    \endverbatim

    Dictionary specification, embedded content
    Dictionary form.
    \verbatim
        <entryName>
        {
            type table;
            values
            (
                (0.0 (1 2 3))
                (1.0 (4 5 6))
            );
        }
    \endverbatim

Note
    The external data reference (using the \c file keyword) is
    used in preference to the \c values specification.

See Also
    Foam::Function1Types::TableFile

SourceFiles
    Table.C

\*---------------------------------------------------------------------------*/

#ifndef Function1Types_Table_H
#define Function1Types_Table_H

#include "TableBase.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace Function1Types
{

/*---------------------------------------------------------------------------*\
                           Class Table Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class Table
:
    public TableBase<Type>
{
    // Private Data

        //- Input name for file-based input (optional)
        fileName fName_;


    // Private Member Functions

        //- No copy assignment
        void operator=(const Table<Type>&) = delete;

public:

    //- Runtime type information
    TypeName("table");


    // Constructors

        //- Construct from entry name and dictionary.
        Table(const word& entryName, const dictionary& dict);

        //- Copy construct
        explicit Table(const Table<Type>& tbl);

        //- Construct and return a clone
        virtual tmp<Function1<Type>> clone() const
        {
            return tmp<Function1<Type>>(new Table<Type>(*this));
        }


    //- Destructor
    virtual ~Table() = default;


    // Member Functions

        //- Write coefficients in dictionary format
        virtual void writeData(Ostream& os) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Function1Types
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "Table.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
