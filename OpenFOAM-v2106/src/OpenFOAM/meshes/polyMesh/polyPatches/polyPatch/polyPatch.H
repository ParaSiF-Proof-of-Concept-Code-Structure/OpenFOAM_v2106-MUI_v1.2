/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2015-2020 OpenCFD Ltd.
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
    Foam::polyPatch

Description
    A patch is a list of labels that address the faces in the global face list.

    The patch can calculate its own edges based on the global faces.
    Patch also contains all addressing between the faces.

SourceFiles
    polyPatch.C
    polyPatchNew.C

\*---------------------------------------------------------------------------*/

#ifndef polyPatch_H
#define polyPatch_H

#include "patchIdentifier.H"
#include "primitivePatch.H"
#include "typeInfo.H"
#include "runTimeSelectionTables.H"
#include "SubField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declarations
class polyBoundaryMesh;
class polyPatch;
class polyTopoChange;
class PstreamBuffers;

Ostream& operator<<(Ostream&, const polyPatch&);


/*---------------------------------------------------------------------------*\
                           Class polyPatch Declaration
\*---------------------------------------------------------------------------*/

class polyPatch
:
    public patchIdentifier,
    public primitivePatch
{
    // Private data

        //- Start label of this patch in the polyMesh face list
        label start_;

        //- Reference to boundary mesh
        const polyBoundaryMesh& boundaryMesh_;


        // Demand-driven private data

            //- face-cell addressing
            mutable labelList::subList* faceCellsPtr_;

            //- Global edge addressing
            mutable labelList* mePtr_;


protected:

    // Protected Member Functions

        //- Inherit movePoints from primitivePatch
        using primitivePatch::movePoints;

        // The polyPatch geometry initialisation is called by polyBoundaryMesh
        friend class polyBoundaryMesh;

        //- Initialise the calculation of the patch geometry
        virtual void initGeometry(PstreamBuffers&)
        {}

        //- Calculate the patch geometry
        virtual void calcGeometry(PstreamBuffers&)
        {}

        //- Initialise the patches for moving points
        virtual void initMovePoints(PstreamBuffers&, const pointField&)
        {}

        //- Correct patches after moving points
        virtual void movePoints(PstreamBuffers&, const pointField& p);

        //- Initialise the update of the patch topology
        virtual void initUpdateMesh(PstreamBuffers&)
        {}

        //- Update of the patch topology
        virtual void updateMesh(PstreamBuffers&);

        //- Clear geometry
        virtual void clearGeom();


public:

    //- Runtime type information
    TypeName("patch");

    //- Debug switch to disallow the use of genericPolyPatch
    static int disallowGenericPolyPatch;


    // Declare run-time constructor selection tables

        declareRunTimeSelectionTable
        (
            autoPtr,
            polyPatch,
            word,
            (
                const word& name,
                const label size,
                const label start,
                const label index,
                const polyBoundaryMesh& bm,
                const word& patchType
            ),
            (name, size, start, index, bm, patchType)
        );

        declareRunTimeSelectionTable
        (
            autoPtr,
            polyPatch,
            dictionary,
            (
                const word& name,
                const dictionary& dict,
                const label index,
                const polyBoundaryMesh& bm,
                const word& patchType
            ),
            (name, dict, index, bm, patchType)
        );


    // Constructors

        //- Construct from components
        polyPatch
        (
            const word& name,
            const label size,
            const label start,
            const label index,
            const polyBoundaryMesh& bm,
            const word& patchType
        );

        //- Construct from components
        polyPatch
        (
            const word& name,
            const label size,
            const label start,
            const label index,
            const polyBoundaryMesh& bm,
            const word& physicalType,
            const wordList& inGroups
        );

        //- Construct from dictionary
        polyPatch
        (
            const word& name,
            const dictionary& dict,
            const label index,
            const polyBoundaryMesh& bm,
            const word& patchType
        );

        //- Construct as copy, resetting the boundary mesh
        polyPatch(const polyPatch&, const polyBoundaryMesh&);

        //- Construct given the original patch and resetting the
        //  face list and boundary mesh information
        polyPatch
        (
            const polyPatch& pp,
            const polyBoundaryMesh& bm,
            const label index,
            const label newSize,
            const label newStart
        );

        //- Construct given the original patch and a map
        polyPatch
        (
            const polyPatch& pp,
            const polyBoundaryMesh& bm,
            const label index,
            const labelUList& mapAddressing,
            const label newStart
        );

        //- Construct as copy
        polyPatch(const polyPatch&);

        //- Construct and return a clone, resetting the boundary mesh
        virtual autoPtr<polyPatch> clone(const polyBoundaryMesh& bm) const
        {
            return autoPtr<polyPatch>::New(*this, bm);
        }

        //- Construct and return a clone, resetting the face list
        //  and boundary mesh
        virtual autoPtr<polyPatch> clone
        (
            const polyBoundaryMesh& bm,
            const label index,
            const label newSize,
            const label newStart
        ) const
        {
            return autoPtr<polyPatch>::New(*this, bm, index, newSize, newStart);
        }

        //- Construct and return a clone, resetting the face list
        //  and boundary mesh
        virtual autoPtr<polyPatch> clone
        (
            const polyBoundaryMesh& bm,
            const label index,
            const labelUList& mapAddressing,
            const label newStart
        ) const
        {
            return autoPtr<polyPatch>::New
                (*this, bm, index, mapAddressing, newStart);
        }


    // Selectors

        //- Return a pointer to a new patch created on freestore from
        //  components
        static autoPtr<polyPatch> New
        (
            const word& patchType,
            const word& name,
            const label size,
            const label start,
            const label index,
            const polyBoundaryMesh& bm
        );

        //- Return a pointer to a new patch created on freestore from
        //  dictionary
        static autoPtr<polyPatch> New
        (
            const word& name,
            const dictionary& dict,
            const label index,
            const polyBoundaryMesh& bm
        );

        //- Return a pointer to a new patch created on freestore from
        //  dictionary
        static autoPtr<polyPatch> New
        (
            const word& patchType,
            const word& name,
            const dictionary& dict,
            const label index,
            const polyBoundaryMesh& bm
        );


    //- Destructor
    virtual ~polyPatch();


    // Member Functions

        //- The offset where this patch starts in the boundary face list
        //  The value is the same as  patch.start() - mesh.nInternalFaces()
        label offset() const;

        //- Return start label of this patch in the polyMesh face list
        label start() const
        {
            return start_;
        }

        //- Return start/size range of this patch
        labelRange range() const
        {
            return labelRange(start_, this->size());
        }

        //- Return boundaryMesh reference
        const polyBoundaryMesh& boundaryMesh() const;

        //- Return true if this patch is geometrically coupled (i.e. faces and
        //  points correspondence)
        virtual bool coupled() const
        {
            return false;
        }

        //- Return true if the given type is a constraint type
        static bool constraintType(const word& pt);

        //- Return a list of all the constraint patch types
        static wordList constraintTypes();

        //- Extract face cell data
        template<class T>
        const UIndirectList<T> patchInternalList
        (
            const UList<T>& internalValues
        ) const
        {
            return UIndirectList<T>(internalValues, faceCells());
        }

        //- Slice list to patch
        template<class T>
        const typename List<T>::subList patchSlice(const UList<T>& l) const
        {
            return typename List<T>::subList(l, this->size(), start_);
        }

        //- Slice Field to patch
        template<class T>
        const typename Field<T>::subField patchSlice(const Field<T>& l) const
        {
            return typename Field<T>::subField(l, this->size(), start_);
        }


        //- Write the polyPatch data as a dictionary
        virtual void write(Ostream&) const;


        // Geometric data; point list required

            //- Return face centres
            const vectorField::subField faceCentres() const;

            //- Return face normals
            const vectorField::subField faceAreas() const;

            //- Return face cell centres
            tmp<vectorField> faceCellCentres() const;

            //- Return the area fraction as the ratio of the stored face area
            //- and the area given by the face points
            tmp<scalarField> areaFraction() const;


        // Addressing into mesh

            //- Return face-cell addressing
            const labelUList& faceCells() const;

            //- Return global edge index for local edges
            const labelList& meshEdges() const;

            //- Clear addressing
            virtual void clearAddressing();


        // Other patch operations

            //- Return label of face in patch from global face label
            inline label whichFace(const label l) const
            {
                return l - start_;
            }


        //- Initialize ordering for primitivePatch. Does not
        //  refer to *this (except for name() and type() etc.)
        virtual void initOrder(PstreamBuffers&, const primitivePatch&) const;

        //- Return new ordering for primitivePatch.
        //  Ordering is -faceMap: for every face
        //  index of the new face -rotation:for every new face the clockwise
        //  shift of the original face. Return false if nothing changes
        //  (faceMap is identity, rotation is 0), true otherwise.
        virtual bool order
        (
            PstreamBuffers&,
            const primitivePatch&,
            labelList& faceMap,
            labelList& rotation
        ) const;

        //- For dynamic mesh cases - return true if this patch will change the
        //- topology
        virtual bool changeTopology() const
        {
            return false;
        }

        //- Collect topology changes in a polyTopoChange object
        virtual bool setTopology(polyTopoChange&)
        {
            return false;
        }


    // Member operators

        //- Assignment
        void operator=(const polyPatch&);


    // Ostream Operator

        friend Ostream& operator<<(Ostream&, const polyPatch&);
};


// Global Functions

//- The labelRange of a polyPatch
template<>
struct labelRangeOp<polyPatch>
{
    labelRange operator()(const polyPatch& pp) const
    {
        return pp.range();
    }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
