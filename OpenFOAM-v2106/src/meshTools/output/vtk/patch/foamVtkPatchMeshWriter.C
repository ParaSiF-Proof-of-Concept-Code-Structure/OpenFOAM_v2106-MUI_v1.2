/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

\*---------------------------------------------------------------------------*/

#include "foamVtkPatchMeshWriter.H"
#include "foamVtkOutput.H"
#include "globalIndex.H"
#include "Time.H"
#include "processorPolyPatch.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::vtk::patchMeshWriter::beginPiece()
{
    // Basic sizes
    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    nLocalPoints_ = nLocalFaces_ = nLocalVerts_ = 0;

    for (const label patchId : patchIDs_)
    {
        const polyPatch& pp = patches[patchId];

        nLocalPoints_ += pp.nPoints();
        nLocalFaces_  += pp.size();

        for (const face& f : pp)
        {
            nLocalVerts_ += f.size();
        }
    }

    numberOfPoints_ = nLocalPoints_;
    numberOfCells_ = nLocalFaces_;

    if (parallel_)
    {
        reduce(numberOfPoints_, sumOp<label>());
        reduce(numberOfCells_,  sumOp<label>());
    }


    // Nothing else to do for legacy
    if (legacy()) return;


    if (format_)
    {
        format()
            .tag
            (
                vtk::fileTag::PIECE,
                vtk::fileAttr::NUMBER_OF_POINTS, numberOfPoints_,
                vtk::fileAttr::NUMBER_OF_POLYS,  numberOfCells_
            );
    }
}


void Foam::vtk::patchMeshWriter::writePoints()
{
    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    this->beginPoints(numberOfPoints_);

    if (parallel_ ? Pstream::master() : true)
    {
        for (const label patchId : patchIDs_)
        {
            const polyPatch& pp = patches[patchId];

            vtk::writeList(format(), pp.localPoints());
        }
    }


    if (parallel_)
    {
        // Patch Ids are identical across all processes
        const label nPatches = patchIDs_.size();

        if (Pstream::master())
        {
            pointField recv;

            // Receive each point field and write
            for (const int subproci : Pstream::subProcs())
            {
                IPstream fromProc(Pstream::commsTypes::blocking, subproci);

                for (label i=0; i < nPatches; ++i)
                {
                    fromProc >> recv;

                    vtk::writeList(format(), recv);
                }
            }
        }
        else
        {
            // Send each point field
            OPstream toProc
            (
                Pstream::commsTypes::blocking,
                Pstream::masterNo()
            );

            for (const label patchId : patchIDs_)
            {
                const polyPatch& pp = patches[patchId];

                toProc << pp.localPoints();
            }
        }
    }


    this->endPoints();
}


void Foam::vtk::patchMeshWriter::writePolysLegacy(const label pointOffset)
{
    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    // Connectivity count without additional storage (done internally)

    label nFaces = nLocalFaces_;
    label nVerts = nLocalVerts_;

    if (parallel_)
    {
        reduce(nFaces, sumOp<label>());
        reduce(nVerts, sumOp<label>());
    }

    if (nFaces != numberOfCells_)
    {
        FatalErrorInFunction
            << "Expecting " << numberOfCells_
            << " faces, but found " << nFaces
            << exit(FatalError);
    }

    legacy::beginPolys(os_, nFaces, nVerts);

    labelList vertLabels(nLocalFaces_ + nLocalVerts_);

    {
        // Legacy: size + connectivity together
        // [nPts, id1, id2, ..., nPts, id1, id2, ...]

        auto iter = vertLabels.begin();

        label off = pointOffset;

        for (const label patchId : patchIDs_)
        {
            const polyPatch& pp = patches[patchId];

            for (const face& f : pp.localFaces())
            {
                *iter = f.size();       // The size prefix
                ++iter;

                for (const label pfi : f)
                {
                    *iter = pfi + off;  // Face vertex label
                    ++iter;
                }
            }
            off += pp.nPoints();
        }
    }


    if (parallel_)
    {
        vtk::writeListParallel(format_.ref(), vertLabels);
    }
    else
    {
        vtk::writeList(format(), vertLabels);
    }

    if (format_)
    {
        format().flush();
    }
}


void Foam::vtk::patchMeshWriter::writePolys(const label pointOffset)
{
    if (format_)
    {
        format().tag(vtk::fileTag::POLYS);
    }

    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    //
    // 'connectivity'
    //
    {
        labelList vertLabels(nLocalVerts_);

        label nVerts = nLocalVerts_;

        if (parallel_)
        {
            reduce(nVerts, sumOp<label>());
        }

        if (format_)
        {
            const uint64_t payLoad =
                vtk::sizeofData<label>(nVerts);

            format().beginDataArray<label>(vtk::dataArrayAttr::CONNECTIVITY);
            format().writeSize(payLoad);
        }

        {
            // XML: connectivity only
            // [id1, id2, ..., id1, id2, ...]

            auto iter = vertLabels.begin();

            label off = pointOffset;

            for (const label patchId : patchIDs_)
            {
                const polyPatch& pp = patches[patchId];

                for (const face& f : pp.localFaces())
                {
                    for (const label pfi : f)
                    {
                        *iter = pfi + off;  // Face vertex label
                        ++iter;
                    }
                }
                off += pp.nPoints();
            }
        }


        if (parallel_)
        {
            vtk::writeListParallel(format_.ref(), vertLabels);
        }
        else
        {
            vtk::writeList(format(), vertLabels);
        }

        if (format_)
        {
            format().flush();
            format().endDataArray();
        }
    }


    //
    // 'offsets'  (connectivity offsets)
    //
    {
        labelList vertOffsets(nLocalFaces_);
        label nOffs = vertOffsets.size();

        if (parallel_)
        {
            reduce(nOffs, sumOp<label>());
        }

        if (format_)
        {
            const uint64_t payLoad =
                vtk::sizeofData<label>(nOffs);

            format().beginDataArray<label>(vtk::dataArrayAttr::OFFSETS);
            format().writeSize(payLoad);
        }


        // processor-local connectivity offsets
        label off =
        (
            parallel_ ? globalIndex(nLocalVerts_).localStart() : 0
        );


        auto iter = vertOffsets.begin();

        for (const label patchId : patchIDs_)
        {
            const polyPatch& pp = patches[patchId];

            for (const face& f : pp)
            {
                off += f.size();   // End offset
                *iter = off;
                ++iter;
            }
        }


        if (parallel_)
        {
            vtk::writeListParallel(format_.ref(), vertOffsets);
        }
        else
        {
            vtk::writeList(format_.ref(), vertOffsets);
        }


        if (format_)
        {
            format().flush();
            format().endDataArray();
        }
    }

    if (format_)
    {
        format().endTag(vtk::fileTag::POLYS);
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::vtk::patchMeshWriter::patchMeshWriter
(
    const polyMesh& mesh,
    const labelList& patchIDs,
    const vtk::outputOptions opts
)
:
    vtk::fileWriter(vtk::fileTag::POLY_DATA, opts),
    numberOfPoints_(0),
    numberOfCells_(0),
    nLocalPoints_(0),
    nLocalFaces_(0),
    nLocalVerts_(0),

    mesh_(mesh),
    patchIDs_(patchIDs)
{
    // We do not currently support append mode
    opts_.append(false);
}


Foam::vtk::patchMeshWriter::patchMeshWriter
(
    const polyMesh& mesh,
    const labelList& patchIDs,
    const fileName& file,
    bool parallel
)
:
    patchMeshWriter(mesh, patchIDs)
{
    open(file, parallel);
}


Foam::vtk::patchMeshWriter::patchMeshWriter
(
    const polyMesh& mesh,
    const labelList& patchIDs,
    const vtk::outputOptions opts,
    const fileName& file,
    bool parallel
)
:
    patchMeshWriter(mesh, patchIDs, opts)
{
    open(file, parallel);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::vtk::patchMeshWriter::beginFile(std::string title)
{
    if (title.size())
    {
        return vtk::fileWriter::beginFile(title);
    }

    // Provide default title

    if (legacy())
    {
        title =
        (
            patchIDs_.size() == 1
          ? mesh_.boundaryMesh()[patchIDs_.first()].name()
          : "patches"
        );

        return vtk::fileWriter::beginFile(title);
    }


    // XML (inline)

    if (patchIDs_.size() == 1)
    {
        title =
        (
            "patch='" + mesh_.boundaryMesh()[patchIDs_.first()].name() + "'"
        );
    }
    else
    {
        title =
        (
            "npatches='" + Foam::name(patchIDs_.size()) + "'"
        );
    }

    title +=
    (
        " time='" + mesh_.time().timeName()
      + "' index='" + Foam::name(mesh_.time().timeIndex())
      + "'"
    );

    return vtk::fileWriter::beginFile(title);
}


bool Foam::vtk::patchMeshWriter::writeGeometry()
{
    enter_Piece();

    beginPiece();

    writePoints();

    const label pointOffset =
    (
        parallel_ ? globalIndex(nLocalPoints_).localStart() : 0
    );

    if (legacy())
    {
        writePolysLegacy(pointOffset);
    }
    else
    {
        writePolys(pointOffset);
    }

    return true;
}


bool Foam::vtk::patchMeshWriter::beginCellData(label nFields)
{
    return enter_CellData(numberOfCells_, nFields);
}


bool Foam::vtk::patchMeshWriter::beginPointData(label nFields)
{
    return enter_PointData(numberOfPoints_, nFields);
}


void Foam::vtk::patchMeshWriter::writePatchIDs()
{
    if (isState(outputState::CELL_DATA))
    {
        ++nCellData_;
    }
    else
    {
        reportBadState(FatalErrorInFunction, outputState::CELL_DATA)
            << " for patchID field" << nl << endl
            << exit(FatalError);
    }

    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    label nFaces = nLocalFaces_;

    if (parallel_)
    {
        reduce(nFaces, sumOp<label>());
    }


    this->beginDataArray<label>("patchID", nFaces);

    if (parallel_ ? Pstream::master() : true)
    {
        for (const label patchId : patchIDs_)
        {
            vtk::write(format(), patchId, patches[patchId].size());
        }
    }

    if (parallel_)
    {
        if (Pstream::master())
        {
            labelList recv;

            // Receive each pair
            for (const int subproci : Pstream::subProcs())
            {
                IPstream fromProc(Pstream::commsTypes::blocking, subproci);

                fromProc >> recv;

                // Receive as [size, id] pairs
                for (label i=0; i < recv.size(); i += 2)
                {
                    const label len = recv[i];
                    const label val = recv[i+1];

                    vtk::write(format(), val, len);
                }
            }
        }
        else
        {
            // Send
            OPstream toProc
            (
                Pstream::commsTypes::blocking,
                Pstream::masterNo()
            );

            // Encode as [size, id] pairs
            labelList send(2*patchIDs_.size());
            label i = 0;
            for (const label patchId : patchIDs_)
            {
                send[i] = patches[patchId].size();
                send[i+1] = patchId;

                i += 2;
            }

            toProc << send;
        }
    }


    this->endDataArray();
}


bool Foam::vtk::patchMeshWriter::writeProcIDs()
{
    // This is different than for internalWriter.
    // Here we allow procIDs whenever running in parallel, even if the
    // output is serial. This allow diagnosis of processor patches.

    if (!Pstream::parRun())
    {
        // Skip in non-parallel
        return false;
    }

    if (isState(outputState::CELL_DATA))
    {
        ++nCellData_;
    }
    else
    {
        reportBadState(FatalErrorInFunction, outputState::CELL_DATA)
            << " for patchID field" << nl << endl
            << exit(FatalError);
    }

    label nFaces = nLocalFaces_;

    if (parallel_)
    {
        reduce(nFaces, sumOp<label>());
    }


    this->beginDataArray<label>("procID", nFaces);

    bool good = false;

    if (parallel_)
    {
        globalIndex procSizes(nLocalFaces_);

        if (Pstream::master())
        {
            // Per-processor ids
            for (const int proci : Pstream::allProcs())
            {
                vtk::write(format(), label(proci), procSizes.localSize(proci));
            }

            good = true;
        }
    }
    else
    {
        vtk::write(format(), label(Pstream::myProcNo()), nLocalFaces_);

        good = true;
    }


    this->endDataArray();

    // MPI barrier
    return parallel_ ? returnReduce(good, orOp<bool>()) : good;
}


bool Foam::vtk::patchMeshWriter::writeNeighIDs()
{
    if (!Pstream::parRun())
    {
        // Skip in non-parallel
        return false;
    }

    if (isState(outputState::CELL_DATA))
    {
        ++nCellData_;
    }
    else
    {
        reportBadState(FatalErrorInFunction, outputState::CELL_DATA)
            << " for patchID field" << nl << endl
            << exit(FatalError);
    }

    const polyBoundaryMesh& patches = mesh_.boundaryMesh();

    label nFaces = nLocalFaces_;

    if (parallel_)
    {
        reduce(nFaces, sumOp<label>());
    }


    this->beginDataArray<label>("neighID", nFaces);

    bool good = false;

    if (parallel_ ? Pstream::master() : true)
    {
        for (const label patchId : patchIDs_)
        {
            const auto* pp = isA<processorPolyPatch>(patches[patchId]);

            const label val = (pp ? pp->neighbProcNo() : -1);

            vtk::write(format(), val, patches[patchId].size());
        }

        good = true;
    }

    if (parallel_)
    {
        if (Pstream::master())
        {
            labelList recv;

            // Receive each pair
            for (const int subproci : Pstream::subProcs())
            {
                IPstream fromProc(Pstream::commsTypes::blocking, subproci);

                fromProc >> recv;

                // Receive as [size, id] pairs
                for (label i=0; i < recv.size(); i += 2)
                {
                    const label len = recv[i];
                    const label val = recv[i+1];

                    vtk::write(format(), val, len);
                }
            }
        }
        else
        {
            // Send
            OPstream toProc
            (
                Pstream::commsTypes::blocking,
                Pstream::masterNo()
            );

            // Encode as [size, id] pairs
            labelList send(2*patchIDs_.size());
            label i = 0;
            for (const label patchId : patchIDs_)
            {
                const auto* pp = isA<processorPolyPatch>(patches[patchId]);

                send[i] = patches[patchId].size();
                send[i+1] = (pp ?  pp->neighbProcNo() : -1);

                i += 2;
            }

            toProc << send;
        }
    }

    this->endDataArray();

    // MPI barrier
    return parallel_ ? returnReduce(good, orOp<bool>()) : good;
}


// ************************************************************************* //
