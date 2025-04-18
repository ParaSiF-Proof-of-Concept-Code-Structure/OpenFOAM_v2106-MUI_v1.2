/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
    Copyright (C) 2021 OpenCFD Ltd.
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
    Foam::faMesh

Description
    Finite area mesh.  Used for 2-D non-Euclidian finite area method.

SourceFiles
    faMesh.C
    faMeshDemandDrivenData.C

Author
    Zeljko Tukovic, FMENA
    Hrvoje Jasak, Wikki Ltd.

\*---------------------------------------------------------------------------*/

#ifndef faMesh_H
#define faMesh_H

#include "MeshObject.H"
#include "polyMesh.H"
#include "lduMesh.H"
#include "faBoundaryMesh.H"
#include "edgeList.H"
#include "faceList.H"
#include "primitiveFieldsFwd.H"
#include "DimensionedField.H"
#include "areaFieldsFwd.H"
#include "edgeFieldsFwd.H"
#include "uindirectPrimitivePatch.H"
#include "edgeInterpolation.H"
#include "labelIOList.H"
#include "FieldFields.H"
#include "faGlobalMeshData.H"
#include "faSchemes.H"
#include "faSolution.H"
#include "data.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class faMeshLduAddressing;
class faMeshMapper;
class faPatchData;
template<class T> class LabelledItem;

/*---------------------------------------------------------------------------*\
                           Class faMesh Declaration
\*---------------------------------------------------------------------------*/

class faMesh
:
    public MeshObject<polyMesh, Foam::UpdateableMeshObject, faMesh>,
    public lduMesh,
    public edgeInterpolation,
    public faSchemes,
    public faSolution,
    public data
{
    // Private Data

        //- Face labels
        labelIOList faceLabels_;

        //- Boundary mesh
        faBoundaryMesh boundary_;


        // Primitive mesh data

            //- Edges, addressing into local point list
            edgeList edges_;

            //- Edge owner
            labelList edgeOwner_;

            //- Edge neighbour
            labelList edgeNeighbour_;


        // Primitive size data

            //- Number of points
            mutable label nPoints_;

            //- Number of edges
            mutable label nEdges_;

            //- Number of internal edges
            mutable label nInternalEdges_;

            //- Number of faces
            mutable label nFaces_;


        // Communication support

            //- Communicator used for parallel communication
            label comm_;


    // Demand-driven data

        //- Primitive patch
        mutable uindirectPrimitivePatch* patchPtr_;

        //- Ldu addressing data
        mutable faMeshLduAddressing* lduPtr_;

        //- Current time index for motion
        //  Note.  The whole mechanism will be replaced once the
        //  dimensionedField is created and the dimensionedField
        //  will take care of the old-time levels.
        mutable label curTimeIndex_;

        //- Face areas
        mutable DimensionedField<scalar, areaMesh>* SPtr_;

        //- Face areas old time level
        mutable DimensionedField<scalar, areaMesh>* S0Ptr_;

        //- Face areas old-old time level
        mutable DimensionedField<scalar, areaMesh>* S00Ptr_;

        //- Patch starts in the edge list
        mutable labelList* patchStartsPtr_;

        //- Edge length vectors
        mutable edgeVectorField* LePtr_;

        //- Mag edge length vectors
        mutable edgeScalarField* magLePtr_;

        //- Face centres
        mutable areaVectorField* centresPtr_;

        //- Edge centres
        mutable edgeVectorField* edgeCentresPtr_;

        //- Face area normals
        mutable areaVectorField* faceAreaNormalsPtr_;

        //- Edge area normals
        mutable edgeVectorField* edgeAreaNormalsPtr_;

        //- Edge area normals
        mutable vectorField* pointAreaNormalsPtr_;

        //- Face curvatures
        mutable areaScalarField* faceCurvaturesPtr_;

        //- Edge transformation tensors
        mutable FieldField<Field, tensor>* edgeTransformTensorsPtr_;

        //- Whether point normals must be corrected for a patch
        mutable boolList* correctPatchPointNormalsPtr_;


        // Other mesh-related data

        //- Parallel info
        mutable autoPtr<faGlobalMeshData> globalMeshDataPtr_;


    // Static Private Data

        //- Use quadrics fit
        static const int quadricsFit_;


    // Private Member Functions

        //- No copy construct
        faMesh(const faMesh&) = delete;

        //- No copy assignment
        void operator=(const faMesh&) = delete;

        //- Set indirect patch, removing any old one
        void initPatch() const;

        //- Set primitive mesh data
        void setPrimitiveMeshData();


        // Private member functions to calculate demand driven data

            //- Calculate ldu addressing
            void calcLduAddressing() const;

            //- Calculate patch starts in the edge list
            void calcPatchStarts() const;

            //- Calculate edge lengths
            void calcLe() const;

            //- Calculate mag edge lengths
            void calcMagLe() const;

            //- Calculate face centres
            void calcAreaCentres() const;

            //- Calculate edge centres
            void calcEdgeCentres() const;

            //- Calculate face areas
            void calcS() const;

            //- Calculate face area normals
            void calcFaceAreaNormals() const;

            //- Calculate edge area normals
            void calcEdgeAreaNormals() const;

            //- Calculate point area normals
            void calcPointAreaNormals() const;

            //- Calculate point area normals by quadrics fit
            void calcPointAreaNormalsByQuadricsFit() const;

            //- Calculate face curvatures
            void calcFaceCurvatures() const;

            //- Calculate edge transformation tensors
            void calcEdgeTransformTensors() const;

            //- Clear geometry but not the face areas
            void clearGeomNotAreas() const;

            //- Clear geometry
            void clearGeom() const;

            //- Clear addressing
            void clearAddressing() const;

            //- Clear demand-driven data
            void clearOut() const;


    // Helpers

        //- Get the polyPatch pairs for the boundary edges (natural order)
        List<LabelledItem<edge>> getBoundaryEdgePatchPairs() const;

        //- Create a single patch
        PtrList<faPatch> createOnePatch
        (
            const word& patchName,
            const word& patchType = ""
        ) const;

        //- Create list of patches from boundary definition
        PtrList<faPatch> createPatchList
        (
            const dictionary& bndDict,
            const word& emptyPatchName = "",
            const dictionary* defaultPatchDefinition = nullptr
        ) const;

        //- Reorder processor edges using order of the
        //- neighbour processorPolyPatch
        void reorderProcEdges
        (
            faPatchData& patchDef,
            const List<LabelledItem<edge>>& bndEdgePatchPairs
        ) const;


public:

    // Public Typedefs

        typedef faMesh Mesh;
        typedef faBoundaryMesh BoundaryMesh;


    //- Runtime type information
    TypeName("faMesh");

        //- The prefix to local: %finite-area
        static const word prefix;

        //- The mesh sub-directory name (usually "faMesh")
        static word meshSubDir;


    // Constructors

        //- Construct zero-sized from polyMesh
        //  Boundary is added using addFaPatches() member function
        faMesh(const polyMesh& pMesh, const Foam::zero);

        //- Construct from polyMesh
        explicit faMesh(const polyMesh& pMesh);

        //- Construct for specified face labels without boundary.
        //  Boundary is added using addFaPatches() member function
        faMesh
        (
            const polyMesh& pMesh,
            const UList<label>& faceLabels
        );

        //- Construct from single polyPatch
        explicit faMesh(const polyPatch& pp);

        //- Construct from definition
        faMesh
        (
            const polyMesh& pMesh,
            const dictionary& faMeshDefinition
        );


    //- Destructor
    virtual ~faMesh();


    // Member Functions

    // Helpers

        //- Add boundary patches. Constructor helper
        void addFaPatches
        (
            PtrList<faPatch>& plist,
            const bool validBoundary = true
        );

        //- Add boundary patches. Constructor helper
        void addFaPatches
        (
            const List<faPatch*>& p,
            const bool validBoundary = true
        );


        // Database

            //- Return access to polyMesh
            inline const polyMesh& mesh() const;

            //- Interface to referenced polyMesh (similar to GeoMesh)
            const polyMesh& operator()() const { return mesh(); }

            //- Return the local mesh directory (dbDir()/meshSubDir)
            fileName meshDir() const;

            //- Return reference to time
            const Time& time() const;

            //- Return the current instance directory for points
            //  Used in the construction of geometric mesh data dependent
            //  on points
            const fileName& pointsInstance() const;

            //- Return the current instance directory for faces
            const fileName& facesInstance() const;


        // Communication support

            //- Return communicator used for parallel communication
            inline label comm() const noexcept;

            //- Return communicator used for parallel communication
            inline label& comm() noexcept;


            // Mesh size parameters

                //- Number of local mesh points
                inline label nPoints() const noexcept;

                //- Number of local mesh edges
                inline label nEdges() const noexcept;

                //- Number of internal faces
                inline label nInternalEdges() const noexcept;

                //- Number of boundary edges (== nEdges - nInternalEdges)
                inline label nBoundaryEdges() const noexcept;

                //- Number of patch faces
                inline label nFaces() const noexcept;


            // Primitive mesh data

                //- Return local patch points
                inline const pointField& points() const;

                //- Return local patch edges with reordered boundary
                inline const edgeList& edges() const noexcept;

                //- Return local patch faces
                inline const faceList& faces() const;

                //- Edge owner addressing
                inline const labelList& edgeOwner() const noexcept;

                //- Edge neighbour addressing
                inline const labelList& edgeNeighbour() const noexcept;



        // Access

            //- Return true if thisDb() is a valid DB
            virtual bool hasDb() const;

            //- Return reference to the mesh database
            virtual const objectRegistry& thisDb() const;

            //- Name function is needed to disambiguate those inherited
            //  from base classes
            const word& name() const
            {
                return thisDb().name();
            }

            //- Return constant reference to boundary mesh
            inline const faBoundaryMesh& boundary() const noexcept;

            //- Return faMesh face labels
            inline const labelList& faceLabels() const noexcept;


            //- Return parallel info
            const faGlobalMeshData& globalData() const;

            //- Return ldu addressing
            virtual const lduAddressing& lduAddr() const;

            //- Return a list of pointers for each patch
            //  with only those pointing to interfaces being set
            virtual lduInterfacePtrsList interfaces() const
            {
                return boundary().interfaces();
            }

            //- Internal face owner
            const labelUList& owner() const
            {
                return lduAddr().lowerAddr();
            }

            //- Internal face neighbour
            const labelUList& neighbour() const
            {
                return lduAddr().upperAddr();
            }

            //- True if given edge label is internal to the mesh
            bool isInternalEdge(const label edgeIndex) const
            {
                return (edgeIndex < nInternalEdges_);
            }


        // Mesh motion and morphing

            //- Is mesh moving
            bool moving() const
            {
                return mesh().moving();
            }

            //- Update after mesh motion
            virtual bool movePoints();

            //- Update after topo change
            virtual void updateMesh(const mapPolyMesh&);


        // Mapping

            //- Map all fields in time using given map.
            virtual void mapFields(const faMeshMapper& mapper) const;

            //- Map face areas in time using given map.
            virtual void mapOldAreas(const faMeshMapper& mapper) const;


        // Demand-driven data

            //- Return constant reference to primitive patch
            inline const uindirectPrimitivePatch& patch() const;

            //- Return reference to primitive patch
            inline uindirectPrimitivePatch& patch();

            //- Return patch starts
            const labelList& patchStarts() const;

            //- Return edge length vectors
            const edgeVectorField& Le() const;

            //- Return edge length magnitudes
            const edgeScalarField& magLe() const;

            //- Return face centres as areaVectorField
            const areaVectorField& areaCentres() const;

            //- Return edge centres as edgeVectorField
            const edgeVectorField& edgeCentres() const;

            //- Return face areas
            const DimensionedField<scalar, areaMesh>& S() const;

            //- Return old-time face areas
            const DimensionedField<scalar, areaMesh>& S0() const;

            //- Return old-old-time face areas
            const DimensionedField<scalar, areaMesh>& S00() const;

            //- Return face area normals
            const areaVectorField& faceAreaNormals() const;

            //- Return edge area normals
            const edgeVectorField& edgeAreaNormals() const;

            //- Return point area normals
            const vectorField& pointAreaNormals() const;

            //- Return face curvatures
            const areaScalarField& faceCurvatures() const;

            //- Return edge transformation tensors
            const FieldField<Field, tensor>& edgeTransformTensors() const;

            //- Return internal point labels
            labelList internalPoints() const;

            //- Return boundary point labels
            labelList boundaryPoints() const;

            //- Return edge length correction
            tmp<edgeScalarField> edgeLengthCorrection() const;

            //- Whether point normals should be corrected for a patch
            bool correctPatchPointNormals(const label patchID) const;

            //- Set whether point normals should be corrected for a patch
            boolList& correctPatchPointNormals() const;


        //- Write mesh
        virtual bool write(const bool valid = true) const;


    // Member Operators

        bool operator!=(const faMesh& m) const;

        bool operator==(const faMesh& m) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "faMeshI.H"

#ifdef NoRepository
    #include "faPatchFaMeshTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
