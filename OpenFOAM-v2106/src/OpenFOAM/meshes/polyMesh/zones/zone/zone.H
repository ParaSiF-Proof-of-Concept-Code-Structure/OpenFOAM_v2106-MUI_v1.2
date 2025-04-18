/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017 OpenCFD Ltd.
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
    Foam::zone

Description
    Base class for mesh zones.

    A zone is a list of labels (eg, cells, faces, points) with
    a name and associated with an index within another list.

SourceFiles
    zone.C

\*---------------------------------------------------------------------------*/

#ifndef zone_H
#define zone_H

#include "labelList.H"
#include "typeInfo.H"
#include "dictionary.H"
#include "Map.H"
#include "pointFieldFwd.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of friend functions and operators

class zone;
Ostream& operator<<(Ostream& os, const zone& zn);

/*---------------------------------------------------------------------------*\
                           Class zone Declaration
\*---------------------------------------------------------------------------*/

class zone
:
    public labelList
{
    // Private Member Functions

        //- No copy construct
        zone(const zone&) = delete;


protected:

    // Protected data

        //- Name of zone
        word name_;

        //- Index of zone
        label index_;


      // Demand-driven private data

        //- Map of labels in zone for fast location lookup
        mutable Map<label>* lookupMapPtr_;


    // Protected Member Functions

        //- Construct the look-up map
        void calcLookupMap() const;


public:

    //- Runtime type information
    TypeName("zone");


    // Constructors

        //- Construct an empty zone
        zone(const word& name, const label index);

        //- Copy construct from components
        zone
        (
            const word& name,
            const labelUList& addr,
            const label index
        );

        //- Move construct from components
        zone
        (
            const word& name,
            labelList&& addr,
            const label index
        );

        //- Construct from dictionary
        zone
        (
            const word& name,
            const dictionary& dict,
            const word& labelsName,
            const label index
        );

        //- Construct given the name of the original zone (name is used)
        //- and resetting addressing and index.
        zone
        (
            const zone& origZone,
            const labelUList& addr,
            const label index
        );

        //- Construct given the name of the original zone (name is used)
        //- and (move) resetting addressing and index.
        zone
        (
            const zone& origZone,
            labelList&& addr,
            const label index
        );


    //- Destructor
    virtual ~zone();


    // Member Functions

        //- Return name
        const word& name() const
        {
            return name_;
        }

        //- Map storing the local index for every global index.  Used to find
        //  the index of the item in the zone from the known global index. If
        //  the item is not in the zone, returns -1
        label localID(const label globalID) const;

        //- Return the index of this zone in zone list
        label index() const
        {
            return index_;
        }

        //- Return the index of this zone in zone list
        label& index()
        {
            return index_;
        }

        //- Return a reference to the look-up map
        const Map<label>& lookupMap() const;

        //- Clear addressing
        virtual void clearAddressing();

        //- Check zone definition. Return true if in error.
        virtual bool checkDefinition(const bool report = false) const = 0;

        //- Check zone definition with max size given. Return true if in error.
        virtual bool checkDefinition
        (
            const label maxSize,
            const bool report = false
        ) const;

        //- Correct patch after moving points
        virtual void movePoints(const pointField& pts)
        {}

        //- Write
        virtual void write(Ostream& os) const;

        //- Write dictionary
        virtual void writeDict(Ostream& os) const = 0;


    // I-O

        //- Ostream Operator
        friend Ostream& operator<<(Ostream& os, const zone& zn);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
