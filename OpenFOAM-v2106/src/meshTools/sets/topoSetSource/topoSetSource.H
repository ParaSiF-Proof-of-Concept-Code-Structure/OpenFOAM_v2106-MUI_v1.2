/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2018-2021 OpenCFD Ltd.
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
    Foam::topoSetSource

Description
    Base class of a source for a \c topoSet.

    Implementer must modify the given set (see \c applyToSet) according to
    its function and the \c setAction (one of add/delete/new).

SourceFiles
    topoSetSource.C

\*---------------------------------------------------------------------------*/

#ifndef topoSetSource_H
#define topoSetSource_H

#include "pointField.H"
#include "labelList.H"
#include "faceList.H"
#include "typeInfo.H"
#include "autoPtr.H"
#include "Enum.H"
#include "HashTable.H"
#include "runTimeSelectionTables.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class dictionary;
class polyMesh;
class bitSet;
class topoSet;

/*---------------------------------------------------------------------------*\
                        Class topoSetSource Declaration
\*---------------------------------------------------------------------------*/

class topoSetSource
{
public:

    // Public Data Types

        //- Enumeration defining the types of sources
        enum sourceType
        {
            UNKNOWN_SOURCE = 0, //!< Placeholder
            SET_SOURCE = 0x10,  //!< Source based on topoSet
            ZONE_SOURCE = 0x20, //!< Source based on mesh zone
            CELL_TYPE = 0x1,    //!< Geometric type is "cell"
            FACE_TYPE = 0x2,    //!< Geometric type is "face"
            POINT_TYPE = 0x4,   //!< Geometric type is "point"

            CELLSET_SOURCE = (CELL_TYPE | SET_SOURCE),      //!< Cells as set
            FACESET_SOURCE = (FACE_TYPE | SET_SOURCE),      //!< Faces as set
            POINTSET_SOURCE = (POINT_TYPE | SET_SOURCE),    //!< Points as set

            CELLZONE_SOURCE = (CELL_TYPE | ZONE_SOURCE),    //!< Cells as zone
            FACEZONE_SOURCE = (FACE_TYPE | ZONE_SOURCE),    //!< Faces as zone
            POINTZONE_SOURCE = (POINT_TYPE | ZONE_SOURCE),  //!< Points as zone

            CELLSETSOURCE = CELLSET_SOURCE,     // Compat (2019-11)
            FACESETSOURCE = FACESET_SOURCE,     // Compat (2019-11)
            POINTSETSOURCE = POINTSET_SOURCE,   // Compat (2019-11)
            CELLZONESOURCE = CELLZONE_SOURCE,   // Compat (2019-11)
            FACEZONESOURCE = FACEZONE_SOURCE,   // Compat (2019-11)
            POINTZONESOURCE = POINTZONE_SOURCE, // Compat (2019-11)
        };

        //- Enumeration defining the valid actions
        enum setAction
        {
            ADD,                //!< Add elements to the set
            SUBTRACT,           //!< Subtract elements from the set
            SUBSET,             //!< Subset with elements in the set
            INVERT,             //!< Invert the elements in the set
            CLEAR,              //!< Clear the set, possibly creating it
            NEW,                //!< Create a new set and ADD elements to it
            REMOVE,             //!< Remove the set (from the file system)
            LIST,               //!< Print contents of the set
            DELETE = SUBTRACT,  //!< \deprecated(2018-10) Alias for SUBTRACT
        };

        //- The setActions text representations
        static const Enum<setAction> actionNames;


protected:

        static const string illegalSource_;

        //- A table of usage strings
        static HashTable<string>* usageTablePtr_;

        //- Class with constructor to add usage string to table
        class addToUsageTable
        {
        public:

            addToUsageTable(const word& name, const string& msg)
            {
                if (!usageTablePtr_)
                {
                    usageTablePtr_ = new HashTable<string>();
                }
                usageTablePtr_->insert(name, msg);
            }

            ~addToUsageTable()
            {
                if (usageTablePtr_)
                {
                    delete usageTablePtr_;
                    usageTablePtr_ = nullptr;
                }
            }
        };


    // Protected Data

        //- Reference to the mesh
        const polyMesh& mesh_;

        //- Output verbosity (default: true)
        bool verbose_;


    // Protected Member Functions

        //- Detect and remove any values less than 0 or ge maxLabel.
        //  \return false if invalid elements were detected (and removed)
        static bool check(labelList& list, const label maxLabel);

        //- Add or delete id from set. Add when 'add' is true
        void addOrDelete(topoSet& set, const label id, const bool add) const;

        //- Add or delete labels from set. Add when 'add' is true
        void addOrDelete
        (
            topoSet& set,
            const labelUList& labels,
            const bool add
        ) const;

        //- Add or delete labels from set. Add when 'add' is true
        void addOrDelete
        (
            topoSet& set,
            const bitSet& labels,
            const bool add
        ) const;


        //- No copy construct
        topoSetSource(const topoSetSource&) = delete;

        //- No copy assignment
        void operator=(const topoSetSource&) = delete;


public:

    //- Runtime type information
    TypeName("topoSetSource");


    // Static Functions

        //- Check state of stream.
        static Istream& checkIs(Istream& is);

        //- True if a "set" source
        static inline bool isSetSource(const sourceType t)
        {
            return (t & SET_SOURCE);
        }

        //- True if a "zone" source
        static inline bool isZoneSource(const sourceType t)
        {
            return (t & ZONE_SOURCE);
        }

        //- True if "cell" geometric type
        static inline bool isCell(const sourceType t)
        {
            return (t & CELL_TYPE);
        }

        //- True if "face" geometric type
        static inline bool isFace(const sourceType t)
        {
            return (t & FACE_TYPE);
        }

        //- True if "point" geometric type
        static inline bool isPoint(const sourceType t)
        {
            return (t & POINT_TYPE);
        }


    // Declare run-time constructor selection table

        // For the dictionary constructor
        declareRunTimeSelectionTable
        (
            autoPtr,
            topoSetSource,
            word,
            (
                const polyMesh& mesh,
                const dictionary& dict
            ),
            (mesh, dict)
        );

        // For the Istream constructor
        declareRunTimeSelectionTable
        (
            autoPtr,
            topoSetSource,
            istream,
            (
                const polyMesh& mesh,
                Istream& is
            ),
            (mesh, is)
        );


        //- Class used for the read-construction of
        //  PtrLists of topoSetSource
        class iNew
        {
            const polyMesh& mesh_;

        public:

            iNew(const polyMesh& mesh)
            :
                mesh_(mesh)
            {}

            autoPtr<topoSetSource> operator()(Istream& is) const
            {
                const word sourceTypeName(is);
                dictionary dict(is);
                return topoSetSource::New(sourceTypeName, mesh_, dict);
            }
        };


        static const string& usage(const word& name)
        {
            if (!usageTablePtr_)
            {
                usageTablePtr_ = new HashTable<string>();
            }

            return usageTablePtr_->lookup(name, illegalSource_);
        }


    // Constructors

        //- Construct from mesh, with preferred verbosity
        explicit topoSetSource(const polyMesh& mesh, bool verbose = true);

        //- Construct from mesh, use "verbose" entry if present
        topoSetSource(const polyMesh& mesh, const dictionary& dict);


        //- Clone (disallowed)
        autoPtr<topoSetSource> clone() const
        {
            NotImplemented;
            return nullptr;
        }


    // Selectors

        //- Return a reference to the selected topoSetSource
        static autoPtr<topoSetSource> New
        (
            const word& topoSetSourceType,
            const polyMesh& mesh,
            const dictionary& dict
        );

        //- Return a reference to the selected topoSetSource
        static autoPtr<topoSetSource> New
        (
            const word& topoSetSourceType,
            const polyMesh& mesh,
            Istream& is
        );


    //- Destructor
    virtual ~topoSetSource() = default;


    // Member Functions

        //- Reference to the mesh
        const polyMesh& mesh() const noexcept
        {
            return mesh_;
        }

        //- Get output verbosity
        bool verbose() const noexcept
        {
            return verbose_;
        }

        //- Enable/disable verbose output
        //  \return old value
        bool verbose(bool on) noexcept
        {
            bool old(verbose_);
            verbose_ = on;
            return old;
        }

        //- Use "verbose" entry (if present) to enable/disable verbose output
        void verbose(const dictionary& dict);


    // Member Functions

        //- The source category (set/zone, cell/face/point)
        virtual sourceType setType() const = 0;

        //- Apply specified action to the topoSet
        virtual void applyToSet
        (
            const topoSetSource::setAction action,
            topoSet& set
        )  const = 0;


    // Housekeeping

        //- Deprecated(2018-07) convert string to action
        //  \deprecated(2018-07) - use actionNames[] directly
        static setAction toAction(const word& actionName)
        {
            return actionNames[actionName];
        }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
