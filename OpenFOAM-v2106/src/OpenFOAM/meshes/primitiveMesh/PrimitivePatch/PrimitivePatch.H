/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2016-2021 OpenCFD Ltd.
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
    Foam::PrimitivePatch

Description
    A list of faces which address into the list of points.

    The class is templated on the face type (e.g. triangle, polygon etc.)
    and on the list type of faces and points so that it can refer to
    existing lists using UList and const pointField& or hold the storage
    using List and pointField.

SourceFiles
    PrimitivePatch.C
    PrimitivePatchAddressing.C
    PrimitivePatchBdryFaces.C
    PrimitivePatchBdryPoints.C
    PrimitivePatchCheck.C
    PrimitivePatchClear.C
    PrimitivePatchEdgeLoops.C
    PrimitivePatchLocalPointOrder.C
    PrimitivePatchMeshData.C
    PrimitivePatchMeshEdges.C
    PrimitivePatchPointAddressing.C
    PrimitivePatchProjectPoints.C

\*---------------------------------------------------------------------------*/

#ifndef PrimitivePatch_H
#define PrimitivePatch_H

#include "boolList.H"
#include "labelList.H"
#include "edgeList.H"
#include "point.H"
#include "intersection.H"
#include "HashSet.H"
#include "objectHit.H"
#include "PrimitivePatchBase.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class face;
template<class T> class Map;

/*---------------------------------------------------------------------------*\
                           Class PrimitivePatch Declaration
\*---------------------------------------------------------------------------*/

template<class FaceList, class PointField>
class PrimitivePatch
:
    public PrimitivePatchBase,
    public FaceList
{
public:

    // Public Typedefs

        //- The face type
        typedef typename
            std::remove_reference<FaceList>::type::value_type face_type;

        //- The point type
        typedef typename
            std::remove_reference<PointField>::type::value_type point_type;

        //- The face list type
        typedef FaceList FaceListType;

        //- The point field type
        typedef PointField PointFieldType;

        //- Deprecated(2020-03) prefer face_type typedef
        // \deprecated(2020-03) prefer face_type typedef
        typedef face_type FaceType;


    // Public Data Types

        //- Enumeration defining the surface type. Used in check routines.
        enum surfaceTopo
        {
            MANIFOLD,
            OPEN,
            ILLEGAL
        };

private:

    // Private Data

        //- Reference to global list of points
        PointField points_;


    // Demand-driven Private Data

        //- Edges of the patch; address into local point list;
        //  sorted with internal edges first in upper-triangular order
        //  and external edges last.
        mutable unique_ptr<edgeList> edgesPtr_;

        //- Which part of edgesPtr_ is internal edges.
        mutable label nInternalEdges_;

        //- Boundary point labels, addressing into local point list
        mutable unique_ptr<labelList> boundaryPointsPtr_;

        //- Face-face addressing
        mutable unique_ptr<labelListList> faceFacesPtr_;

        //- Edge-face addressing
        mutable unique_ptr<labelListList> edgeFacesPtr_;

        //- Face-edge addressing
        mutable unique_ptr<labelListList> faceEdgesPtr_;

        //- Point-edge addressing
        mutable unique_ptr<labelListList> pointEdgesPtr_;

        //- Point-face addressing
        mutable unique_ptr<labelListList> pointFacesPtr_;

        //- Faces addressing into local point list
        mutable unique_ptr<List<face_type>> localFacesPtr_;

        //- Labels of mesh points
        mutable unique_ptr<labelList> meshPointsPtr_;

        //- Mesh point map. Given the global point index find its
        //- location in the patch
        mutable unique_ptr<Map<label>> meshPointMapPtr_;

        //- Outside edge loops
        mutable unique_ptr<labelListList> edgeLoopsPtr_;

        //- Points local to patch
        mutable unique_ptr<Field<point_type>> localPointsPtr_;

        //- Local point order for most efficient search
        mutable unique_ptr<labelList> localPointOrderPtr_;

        //- Face centres
        mutable unique_ptr<Field<point_type>> faceCentresPtr_;

        //- Face area vectors
        mutable unique_ptr<Field<point_type>> faceAreasPtr_;

        //- Mag face area
        mutable unique_ptr<Field<scalar>> magFaceAreasPtr_;

        //- Face unit normals
        mutable unique_ptr<Field<point_type>> faceNormalsPtr_;

        //- Point unit normals
        mutable unique_ptr<Field<point_type>> pointNormalsPtr_;


    // Private Member Functions

        //- Calculate internal points on a patch
        void calcInternPoints() const;

        //- Calculate boundary points on a patch
        void calcBdryPoints() const;

        //- Calculate addressing
        void calcAddressing() const;

        //- Calculate point-edge addressing
        void calcPointEdges() const;

        //- Calculate point-face addressing
        void calcPointFaces() const;

        //- Calculate mesh addressing
        void calcMeshData() const;

        //- Calculate mesh point map
        void calcMeshPointMap() const;

        //- Calculate outside edge loops
        void calcEdgeLoops() const;

        //- Calculate local points
        void calcLocalPoints() const;

        //- Calculate local point order
        void calcLocalPointOrder() const;

        //- Calculate face centres
        void calcFaceCentres() const;

        //- Calculate face area vectors
        void calcFaceAreas() const;

        //- Calculate face area magnitudes
        void calcMagFaceAreas() const;

        //- Calculate unit face normals
        void calcFaceNormals() const;

        //- Calculate unit point normals
        void calcPointNormals() const;


        //- Face-edge-face walk while remaining on a patch point.
        //  Used to determine if surface multiply connected through point.
        void visitPointRegion
        (
            const label pointi,
            const labelList& pFaces,
            const label startFacei,
            const label startEdgeI,
            boolList& pFacesHad
        ) const;


public:

    // Constructors

        //- Construct from components
        PrimitivePatch
        (
            const FaceList& faces,
            const PointField& points
        );

        //- Construct from components, transferring faces
        PrimitivePatch
        (
            FaceList&& faces,
            const PointField& points
        );

        //- Construct from components, reuse storage
        PrimitivePatch
        (
            FaceList& faces,
            PointField& points,
            const bool reuse
        );

        //- Copy construct
        PrimitivePatch(const PrimitivePatch<FaceList, PointField>& pp);


    //- Destructor
    virtual ~PrimitivePatch();

        void clearOut();

        void clearGeom();

        void clearTopology();

        void clearPatchMeshAddr();


    // Member Functions

        //- Suppress direct swapping, since storage containers may be const
        void swap(PrimitivePatch&) = delete;


    // Access

        //- Return reference to global points
        const Field<point_type>& points() const
        {
            return points_;
        }

        //- Number of faces in the patch
        label nFaces() const
        {
            return FaceList::size();
        }


    // Access functions for demand-driven data

        // Topological data; no mesh required.

            //- Number of points supporting patch faces
            label nPoints() const
            {
                return meshPoints().size();
            }

            //- Number of edges in patch
            label nEdges() const
            {
                return edges().size();
            }

            //- Return list of edges, address into LOCAL point list
            const edgeList& edges() const;

            //- Return sub-list of internal edges, address into LOCAL point list
            const edgeList::subList internalEdges() const;

            //- Return sub-list of boundary edges, address into LOCAL point list
            const edgeList::subList boundaryEdges() const;

            //- Number of internal edges
            label nInternalEdges() const;

            //- Number of boundary edges == (nEdges() - nInternalEdges())
            label nBoundaryEdges() const;

            //- Is internal edge?
            bool isInternalEdge(const label edgei) const
            {
                return edgei < nInternalEdges();
            }

            //- Return list of boundary points, address into LOCAL point list
            const labelList& boundaryPoints() const;

            //- Return face-face addressing
            const labelListList& faceFaces() const;

            //- Return edge-face addressing
            const labelListList& edgeFaces() const;

            //- Return face-edge addressing
            const labelListList& faceEdges() const;

            //- Return point-edge addressing
            const labelListList& pointEdges() const;

            //- Return point-face addressing
            const labelListList& pointFaces() const;

            //- Return patch faces addressing into local point list
            const List<face_type>& localFaces() const;

            //- Extract list of local faces corresponding to
            //- the boundary edges.
            labelList boundaryFaces() const;

            //- Extract sorted list of unique local faces associated with
            //- the boundary edges.
            labelList uniqBoundaryFaces() const;


        // Addressing into mesh

            //- Return labelList of mesh points in patch.
            //  They are constructed by walking through the faces in
            //  incremental order and not sorted anymore.
            const labelList& meshPoints() const;

            //- Mesh point map.
            //  Given the global point index find its location in the patch
            const Map<label>& meshPointMap() const;

            //- Return pointField of points in patch
            const Field<point_type>& localPoints() const;

            //- Return orders the local points for most efficient search
            const labelList& localPointOrder() const;

            //- Given a global point index, return the local point index.
            //  If the point is not found, return -1
            label whichPoint(const label gp) const;

            //- Return local edge in terms of the mesh points in patch.
            edge meshEdge(const label edgei) const;

            //- Return local edge in terms of the mesh points in patch.
            edge meshEdge(const edge& e) const;

            //- Search for edge (local point labels) and return its
            //- index in the edge list or -1 if not found.
            label findEdge(const edge& e) const;

            //- Return labels of patch edges in the global edge list using
            //- cell addressing
            labelList meshEdges
            (
                const edgeList& allEdges,
                const labelListList& cellEdges,
                const labelList& faceCells
            ) const;

            //- Return labels of patch edges into the global edge list using
            //- basic edge addressing.
            labelList meshEdges
            (
                const edgeList& allEdges,
                const labelListList& pointEdges
            ) const;

            //- Return label of the local patch edge
            //- into the global edge list using basic edge addressing.
            label meshEdge
            (
                const label edgei,
                const edgeList& allEdges,
                const labelListList& pointEdges
            ) const;

            //- Return labels of specified patch edges
            //- into the global edge list using basic edge addressing.
            labelList meshEdges
            (
                const labelUList& edgeLabels,
                const edgeList& allEdges,
                const labelListList& pointEdges
            ) const;


            //- Return face centres for patch
            const Field<point_type>& faceCentres() const;

            //- Return face area vectors for patch
            const Field<point_type>& faceAreas() const;

            //- Return face area magnitudes for patch
            const Field<scalar>& magFaceAreas() const;

            //- Return face unit normals for patch
            const Field<point_type>& faceNormals() const;

            //- Return point normals for patch
            const Field<point_type>& pointNormals() const;


        // Storage Management

            bool hasFaceAreas() const { return bool(faceAreasPtr_); }
            bool hasFaceCentres() const { return bool(faceCentresPtr_); }
            bool hasFaceNormals() const { return bool(faceNormalsPtr_); }
            bool hasPointNormals() const { return bool(pointNormalsPtr_); }


        // Other patch operations

            //- Project vertices of patch onto another patch
            template<class ToPatch>
            List<objectHit> projectPoints
            (
                const ToPatch& targetPatch,
                const Field<point_type>& projectionDirection,
                const intersection::algorithm = intersection::FULL_RAY,
                const intersection::direction = intersection::VECTOR
            ) const;

            //- Project vertices of patch onto another patch
            template<class ToPatch>
            List<objectHit> projectFaceCentres
            (
                const ToPatch& targetPatch,
                const Field<point_type>& projectionDirection,
                const intersection::algorithm = intersection::FULL_RAY,
                const intersection::direction = intersection::VECTOR
            ) const;

            //- Return list of closed loops of boundary vertices.
            //  Edge loops are given as ordered lists of vertices
            //  in local addressing
            const labelListList& edgeLoops() const;


    // Check

        //- Calculate surface type formed by patch.
        //  Types:
        //  - all edges have two neighbours (manifold)
        //  - some edges have more than two neighbours (illegal)
        //  - other (open)
        surfaceTopo surfaceType() const;

        //- Check surface formed by patch for manifoldness (see above).
        //  Return true if any incorrect edges are found.
        //  Insert vertices of incorrect edges into set.
        bool checkTopology
        (
            const bool report = false,
            labelHashSet* setPtr = nullptr
        ) const;

        //- Checks primitivePatch for faces sharing point but not edge.
        //  This denotes a surface that is pinched at a single point
        //  (test for pinched at single edge is already in PrimitivePatch)
        //  Returns true if this situation found and puts conflicting
        //  (mesh)point in set. Based on all the checking routines in
        //  primitiveMesh.
        bool checkPointManifold
        (
            const bool report = false,
            labelHashSet* setPtr = nullptr
        ) const;


    // Edit

        //- Correct patch after moving points
        virtual void movePoints(const Field<point_type>&);


    // Member Operators

        //- Copy assign faces. Leave points alone (could be a reference).
        void operator=(const PrimitivePatch<FaceList, PointField>& rhs);

        //- Move assign faces. Leave points alone (could be a reference).
        void operator=(PrimitivePatch<FaceList, PointField>&& rhs);


    // Housekeeping

        //- Identical to findEdge
        label whichEdge(const edge& e) const { return this->findEdge(e); }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "PrimitivePatch.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
