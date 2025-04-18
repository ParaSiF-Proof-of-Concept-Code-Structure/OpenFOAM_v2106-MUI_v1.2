/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2015-2021 OpenCFD Ltd.
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

#include "ptscotchDecomp.H"
#include "addToRunTimeSelectionTable.H"
#include "Time.H"
#include "OFstream.H"
#include "globalIndex.H"
#include "SubField.H"

// Avoid too many warnings from mpi.h
#pragma GCC diagnostic ignored "-Wold-style-cast"

#include <cstdio>
#include <mpi.h>
#include "ptscotch.h"

// Hack: scotch generates floating point errors so need to switch off error
//       trapping!
#ifdef __GLIBC__
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
    #include <fenv.h>
#endif

// Provide a clear error message if we have a size mismatch
static_assert
(
    sizeof(Foam::label) == sizeof(SCOTCH_Num),
    "sizeof(Foam::label) == sizeof(SCOTCH_Num), check your scotch headers"
);

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(ptscotchDecomp, 0);
    addToRunTimeSelectionTable
    (
        decompositionMethod,
        ptscotchDecomp,
        dictionary
    );
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::ptscotchDecomp::graphPath(const polyMesh& mesh) const
{
    graphPath_ = mesh.time().path()/mesh.name();
}


void Foam::ptscotchDecomp::check(const int retVal, const char* str)
{
    if (retVal)
    {
        FatalErrorInFunction
            << "Call to scotch routine " << str << " failed.\n"
            << exit(FatalError);
    }
}


////- Does prevention of 0 cell domains and calls ptscotch.
//Foam::label Foam::ptscotchDecomp::decomposeZeroDomains
//(
//    const labelList& initadjncy,
//    const labelList& initxadj,
//    const List<scalar>& initcWeights,
//    labelList& finalDecomp
//) const
//{
//    globalIndex globalCells(initxadj.size()-1);
//
//    bool hasZeroDomain = false;
//    for (const int proci : Pstream::allProcs())
//    {
//        if (globalCells.localSize(proci) == 0)
//        {
//            hasZeroDomain = true;
//            break;
//        }
//    }
//
//    if (!hasZeroDomain)
//    {
//        return decompose
//        (
//            initadjncy,
//            initxadj,
//            initcWeights,
//            finalDecomp
//        );
//    }
//
//
//    if (debug)
//    {
//        Info<< "ptscotchDecomp : have graphs with locally 0 cells."
//            << " trickling down." << endl;
//    }
//
//    // Make sure every domain has at least one cell
//    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//    // (scotch does not like zero sized domains)
//    // Trickle cells from processors that have them up to those that
//    // don't.
//
//
//    // Number of cells to send to the next processor
//    // (is same as number of cells next processor has to receive)
//    List<label> nSendCells(Pstream::nProcs(), Zero);
//
//    for (label proci = nSendCells.size()-1; proci >=1; proci--)
//    {
//        label nLocalCells = globalCells.localSize(proci);
//        if (nLocalCells-nSendCells[proci] < 1)
//        {
//            nSendCells[proci-1] = nSendCells[proci]-nLocalCells+1;
//        }
//    }
//
//    // First receive (so increasing the sizes of all arrays)
//
//    Field<int> xadj(initxadj);
//    Field<int> adjncy(initadjncy);
//    scalarField cWeights(initcWeights);
//
//    if (Pstream::myProcNo() >= 1 && nSendCells[Pstream::myProcNo()-1] > 0)
//    {
//        // Receive cells from previous processor
//        IPstream fromPrevProc(Pstream::commsTypes::blocking,
//            Pstream::myProcNo()-1);
//
//        Field<int> prevXadj(fromPrevProc);
//        Field<int> prevAdjncy(fromPrevProc);
//        scalarField prevCellWeights(fromPrevProc);
//
//        if (prevXadj.size() != nSendCells[Pstream::myProcNo()-1])
//        {
//            FatalErrorInFunction
//                << "Expected from processor " << Pstream::myProcNo()-1
//                << " connectivity for " << nSendCells[Pstream::myProcNo()-1]
//                << " nCells but only received " << prevXadj.size()
//                << abort(FatalError);
//        }
//
//        // Insert adjncy
//        prepend(prevAdjncy, adjncy);
//        // Adapt offsets and prepend xadj
//        xadj += prevAdjncy.size();
//        prepend(prevXadj, xadj);
//        // Weights
//        prepend(prevCellWeights, cWeights);
//    }
//
//
//    // Send to my next processor
//
//    if (nSendCells[Pstream::myProcNo()] > 0)
//    {
//        // Send cells to next processor
//        OPstream toNextProc(Pstream::commsTypes::blocking,
//            Pstream::myProcNo()+1);
//
//        label nCells = nSendCells[Pstream::myProcNo()];
//        label startCell = xadj.size()-1 - nCells;
//        label startFace = xadj[startCell];
//        label nFaces = adjncy.size()-startFace;
//
//        // Send for all cell data: last nCells elements
//        // Send for all face data: last nFaces elements
//        toNextProc
//            << Field<int>::subField(xadj, nCells, startCell)-startFace
//            << Field<int>::subField(adjncy, nFaces, startFace)
//            <<
//            (
//                cWeights.size()
//              ? static_cast<const scalarField&>
//                (
//                    scalarField::subField(cWeights, nCells, startCell)
//                )
//              : scalarField(0)
//            );
//
//        // Remove data that has been sent
//        if (cWeights.size())
//        {
//            cWeights.setSize(cWeights.size()-nCells);
//        }
//        adjncy.setSize(adjncy.size()-nFaces);
//        xadj.setSize(xadj.size() - nCells);
//    }
//
//
//    // Do decomposition as normal. Sets finalDecomp.
//    label result = decompose(adjncy, xadj, cWeights, finalDecomp);
//
//
//    if (debug)
//    {
//        Info<< "ptscotchDecomp : have graphs with locally 0 cells."
//            << " trickling up." << endl;
//    }
//
//
//    // If we sent cells across make sure we undo it
//    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    // Receive back from next processor if I sent something
//    if (nSendCells[Pstream::myProcNo()] > 0)
//    {
//        IPstream fromNextProc(Pstream::commsTypes::blocking,
//            Pstream::myProcNo()+1);
//
//        List<label> nextFinalDecomp(fromNextProc);
//
//        if (nextFinalDecomp.size() != nSendCells[Pstream::myProcNo()])
//        {
//            FatalErrorInFunction
//                << "Expected from processor " << Pstream::myProcNo()+1
//                << " decomposition for " << nSendCells[Pstream::myProcNo()]
//                << " nCells but only received " << nextFinalDecomp.size()
//                << abort(FatalError);
//        }
//
//        append(nextFinalDecomp, finalDecomp);
//    }
//
//    // Send back to previous processor.
//    if (Pstream::myProcNo() >= 1 && nSendCells[Pstream::myProcNo()-1] > 0)
//    {
//        OPstream toPrevProc(Pstream::commsTypes::blocking,
//            Pstream::myProcNo()-1);
//
//        label nToPrevious = nSendCells[Pstream::myProcNo()-1];
//
//        toPrevProc <<
//            SubList<label>
//            (
//                finalDecomp,
//                nToPrevious,
//                finalDecomp.size()-nToPrevious
//            );
//
//        // Remove locally what has been sent
//        finalDecomp.setSize(finalDecomp.size()-nToPrevious);
//    }
//    return result;
//}


Foam::label Foam::ptscotchDecomp::decompose
(
    const labelList& adjncy,
    const labelList& xadj,
    const List<scalar>& cWeights,
    labelList& finalDecomp
) const
{
    List<label> dummyAdjncy;
    List<label> dummyXadj;

    return decompose
    (
        adjncy.size(),
        (adjncy.size() ? adjncy.begin() : dummyAdjncy.begin()),
        xadj.size(),
        (xadj.size() ? xadj.begin() : dummyXadj.begin()),
        cWeights,
        finalDecomp
    );
}


Foam::label Foam::ptscotchDecomp::decompose
(
    const label adjncySize,
    const label adjncy[],
    const label xadjSize,
    const label xadj[],
    const List<scalar>& cWeights,
    labelList& finalDecomp
) const
{
    if (debug)
    {
        Pout<< "ptscotchDecomp : entering with xadj:" << xadjSize << endl;
    }

    // Dump graph
    if (coeffsDict_.getOrDefault("writeGraph", false))
    {
        OFstream str
        (
            graphPath_ + "_" + Foam::name(Pstream::myProcNo()) + ".dgr"
        );

        Pout<< "Dumping Scotch graph file to " << str.name() << endl
            << "Use this in combination with dgpart." << endl;

        globalIndex globalCells(xadjSize-1);

        // Distributed graph file (.grf)
        const label version = 2;
        str << version << nl;
        // Number of files (procglbnbr)
        str << Pstream::nProcs();
        // My file number (procloc)
        str << ' ' << Pstream::myProcNo() << nl;

        // Total number of vertices (vertglbnbr)
        str << globalCells.size();
        // Total number of connections (edgeglbnbr)
        str << ' ' << returnReduce(xadj[xadjSize-1], sumOp<label>()) << nl;
        // Local number of vertices (vertlocnbr)
        str << xadjSize-1;
        // Local number of connections (edgelocnbr)
        str << ' ' << xadj[xadjSize-1] << nl;
        // Numbering starts from 0
        label baseval = 0;
        // 100*hasVertlabels+10*hasEdgeWeights+1*hasVertWeighs
        str << baseval << ' ' << "000" << nl;
        for (label celli = 0; celli < xadjSize-1; celli++)
        {
            const label start = xadj[celli];
            const label end = xadj[celli+1];

            str << end-start; // size

            for (label i = start; i < end; i++)
            {
                str << ' ' << adjncy[i];
            }
            str << nl;
        }
    }


    // Make repeatable
    SCOTCH_randomReset();

    // Strategy
    // ~~~~~~~~

    // Default.
    SCOTCH_Strat stradat;
    check(SCOTCH_stratInit(&stradat), "SCOTCH_stratInit");

    string strategy;
    if (coeffsDict_.readIfPresent("strategy", strategy))
    {
        if (debug)
        {
            Info<< "ptscotchDecomp : Using strategy " << strategy << endl;
        }
        SCOTCH_stratDgraphMap(&stradat, strategy.c_str());
        //fprintf(stdout, "S\tStrat=");
        //SCOTCH_stratSave(&stradat, stdout);
        //fprintf(stdout, "\n");
    }

    // Graph
    // ~~~~~

    List<label> velotab;


    // Check for externally provided cellweights and if so initialise weights

    const scalar minWeights = gMin(cWeights);
    const scalar maxWeights = gMax(cWeights);

    if (maxWeights > minWeights)
    {
        if (minWeights <= 0)
        {
            WarningInFunction
                << "Illegal minimum weight " << minWeights
                << endl;
        }

        if (cWeights.size() != xadjSize-1)
        {
            FatalErrorInFunction
                << "Number of cell weights " << cWeights.size()
                << " does not equal number of cells " << xadjSize-1
                << exit(FatalError);
        }
    }

    scalar velotabSum = gSum(cWeights)/minWeights;

    scalar rangeScale(1.0);

    if (Pstream::master())
    {
        if (velotabSum > scalar(labelMax - 1))
        {
            // 0.9 factor of safety to avoid floating point round-off in
            // rangeScale tipping the subsequent sum over the integer limit.
            rangeScale = 0.9*scalar(labelMax - 1)/velotabSum;

            WarningInFunction
                << "Sum of weights has overflowed integer: " << velotabSum
                << ", compressing weight scale by a factor of " << rangeScale
                << endl;
        }
    }

    Pstream::scatter(rangeScale);

    if (maxWeights > minWeights)
    {
        if (cWeights.size())
        {
            // Convert to integers.
            velotab.setSize(cWeights.size());

            forAll(velotab, i)
            {
                velotab[i] = int((cWeights[i]/minWeights - 1)*rangeScale) + 1;
            }
        }
        else
        {
            // Locally zero cells but not globally. Make sure we have
            // some size so .begin() does not return null pointer. Data
            // itself is never used.
            velotab.setSize(1);
            velotab[0] = 1;
        }
    }


    if (debug)
    {
        Pout<< "SCOTCH_dgraphInit" << endl;
    }
    SCOTCH_Dgraph grafdat;
    check(SCOTCH_dgraphInit(&grafdat, MPI_COMM_WORLD), "SCOTCH_dgraphInit");


    if (debug)
    {
        Pout<< "SCOTCH_dgraphBuild with:" << nl
            << "xadjSize-1      : " << xadjSize-1 << nl
            << "xadj            : " << name(xadj) << nl
            << "velotab         : " << name(velotab.begin()) << nl
            << "adjncySize      : " << adjncySize << nl
            << "adjncy          : " << name(adjncy) << nl
            << endl;
    }

    check
    (
        SCOTCH_dgraphBuild
        (
            &grafdat,               // grafdat
            0,                      // baseval, c-style numbering
            xadjSize-1,             // vertlocnbr, nCells
            xadjSize-1,             // vertlocmax
            const_cast<SCOTCH_Num*>(xadj),
                                    // vertloctab, start index per cell into
                                    // adjncy
            const_cast<SCOTCH_Num*>(xadj+1),// vendloctab, end index  ,,

            const_cast<SCOTCH_Num*>(velotab.begin()),// veloloctab, vtx weights
            nullptr,                   // vlblloctab

            adjncySize,             // edgelocnbr, number of arcs
            adjncySize,             // edgelocsiz
            const_cast<SCOTCH_Num*>(adjncy),         // edgeloctab
            nullptr,                   // edgegsttab
            nullptr                    // edlotab, edge weights
        ),
        "SCOTCH_dgraphBuild"
    );


    if (debug)
    {
        Pout<< "SCOTCH_dgraphCheck" << endl;
    }
    check(SCOTCH_dgraphCheck(&grafdat), "SCOTCH_dgraphCheck");


    // Architecture
    // ~~~~~~~~~~~~
    // (fully connected network topology since using switch)

    if (debug)
    {
        Pout<< "SCOTCH_archInit" << endl;
    }
    SCOTCH_Arch archdat;
    check(SCOTCH_archInit(&archdat), "SCOTCH_archInit");

    List<label> processorWeights;
    if
    (
        coeffsDict_.readIfPresent("processorWeights", processorWeights)
     && processorWeights.size()
    )
    {
        if (debug)
        {
            Info<< "ptscotchDecomp : Using procesor weights "
                << processorWeights
                << endl;
        }

        if (processorWeights.size() != nDomains_)
        {
            FatalIOErrorInFunction(coeffsDict_)
                << "processorWeights not the same size"
                << " as the wanted number of domains " << nDomains_
                << exit(FatalIOError);
        }

        check
        (
            SCOTCH_archCmpltw
            (
                &archdat, nDomains_, processorWeights.begin()
            ),
            "SCOTCH_archCmpltw"
        );
    }
    else
    {
        if (debug)
        {
            Pout<< "SCOTCH_archCmplt" << endl;
        }
        check
        (
            SCOTCH_archCmplt(&archdat, nDomains_),
            "SCOTCH_archCmplt"
        );
    }


    // Hack:switch off fpu error trapping
    #ifdef  FE_NOMASK_ENV
    int oldExcepts = fedisableexcept
    (
        FE_DIVBYZERO
      | FE_INVALID
      | FE_OVERFLOW
    );
    #endif


    // Note: always provide allocated storage even if local size 0
    finalDecomp.setSize(max(1, xadjSize-1));
    finalDecomp = 0;

    if (debug)
    {
        Pout<< "SCOTCH_dgraphMap" << endl;
    }
    check
    (
        SCOTCH_dgraphMap
        (
            &grafdat,
            &archdat,
            &stradat,           // const SCOTCH_Strat *
            finalDecomp.begin() // parttab
        ),
        "SCOTCH_graphMap"
    );

    #ifdef  FE_NOMASK_ENV
    feenableexcept(oldExcepts);
    #endif

    // See above note to have size 1. Undo.
    finalDecomp.setSize(xadjSize-1);

    //check
    //(
    //    SCOTCH_dgraphPart
    //    (
    //        &grafdat,
    //        nDomains_,          // partnbr
    //        &stradat,           // const SCOTCH_Strat *
    //        finalDecomp.begin() // parttab
    //    ),
    //    "SCOTCH_graphPart"
    //);

    if (debug)
    {
        Pout<< "SCOTCH_dgraphExit" << endl;
    }
    // Release storage for graph
    SCOTCH_dgraphExit(&grafdat);
    // Release storage for strategy
    SCOTCH_stratExit(&stradat);
    // Release storage for network topology
    SCOTCH_archExit(&archdat);

    return 0;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::ptscotchDecomp::ptscotchDecomp
(
    const dictionary& decompDict,
    const word& regionName
)
:
    decompositionMethod(decompDict, regionName),
    coeffsDict_(findCoeffsDict("scotchCoeffs", selectionType::NULL_DICT))
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::labelList Foam::ptscotchDecomp::decompose
(
    const polyMesh& mesh,
    const pointField& points,
    const scalarField& pointWeights
) const
{
    // Where to write graph
    graphPath(mesh);

    if (points.size() != mesh.nCells())
    {
        FatalErrorInFunction
            << "Can only use this decomposition method for entire mesh" << nl
            << "and supply one coordinate (cellCentre) for every cell." << nl
            << "The number of coordinates " << points.size() << nl
            << "The number of cells in the mesh " << mesh.nCells() << nl
            << exit(FatalError);
    }


    // Make Metis CSR (Compressed Storage Format) storage
    //   adjncy      : contains neighbours (= edges in graph)
    //   xadj(celli) : start of information in adjncy for celli

    CompactListList<label> cellCells;
    calcCellCells
    (
        mesh,
        identity(mesh.nCells()),
        mesh.nCells(),
        true,
        cellCells
    );

    // Decompose using default weights
    labelList decomp;
    decompose
    (
        cellCells.m(),
        cellCells.offsets(),
        pointWeights,
        decomp
    );

    return decomp;
}


Foam::labelList Foam::ptscotchDecomp::decompose
(
    const polyMesh& mesh,
    const labelList& agglom,
    const pointField& agglomPoints,
    const scalarField& pointWeights
) const
{
    // Where to write graph
    graphPath(mesh);

    if (agglom.size() != mesh.nCells())
    {
        FatalErrorInFunction
            << "Size of cell-to-coarse map " << agglom.size()
            << " differs from number of cells in mesh " << mesh.nCells()
            << exit(FatalError);
    }


    // Make Metis CSR (Compressed Storage Format) storage
    //   adjncy      : contains neighbours (= edges in graph)
    //   xadj(celli) : start of information in adjncy for celli
    CompactListList<label> cellCells;
    calcCellCells
    (
        mesh,
        agglom,
        agglomPoints.size(),
        true,
        cellCells
    );

    // Decompose using weights
    labelList decomp;
    decompose
    (
        cellCells.m(),
        cellCells.offsets(),
        pointWeights,
        decomp
    );

    // Rework back into decomposition for original mesh
    labelList fineDistribution(agglom.size());

    forAll(fineDistribution, i)
    {
        fineDistribution[i] = decomp[agglom[i]];
    }

    return fineDistribution;
}


Foam::labelList Foam::ptscotchDecomp::decompose
(
    const labelListList& globalCellCells,
    const pointField& cellCentres,
    const scalarField& cWeights
) const
{
    // Where to write graph
    graphPath_ = "ptscotch";

    if (cellCentres.size() != globalCellCells.size())
    {
        FatalErrorInFunction
            << "Inconsistent number of cells (" << globalCellCells.size()
            << ") and number of cell centres (" << cellCentres.size()
            << ")." << exit(FatalError);
    }


    // Make Metis CSR (Compressed Storage Format) storage
    //   adjncy      : contains neighbours (= edges in graph)
    //   xadj(celli) : start of information in adjncy for celli

    CompactListList<label> cellCells(globalCellCells);

    // Decompose using weights
    labelList decomp;
    decompose
    (
        cellCells.m(),
        cellCells.offsets(),
        cWeights,
        decomp
    );

    return decomp;
}


// ************************************************************************* //
