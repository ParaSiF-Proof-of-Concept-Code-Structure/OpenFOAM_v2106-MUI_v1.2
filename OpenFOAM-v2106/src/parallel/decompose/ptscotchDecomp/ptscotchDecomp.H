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
    Foam::ptscotchDecomp

Description
    PTScotch domain decomposition.

    For the main details about how to define the strategies, see scotchDecomp.

    Coefficients dictionary: \a scotchCoeffs, \a coeffs.

    Nonetheless, when decomposing in parallel, using <tt>writeGraph=true</tt>
    will write out \c .dgr files for debugging. For example, use these files
    with \c dgpart as follows:

    \verbatim
    mpirun -np 4 dgpart 2 'region0_%r.dgr'
    \endverbatim

    where:
      - %r gets replaced by current processor rank
      - it will decompose into 2 domains

See also
    Foam::scotchDecomp

SourceFiles
    ptscotchDecomp.C

\*---------------------------------------------------------------------------*/

#ifndef ptscotchDecomp_H
#define ptscotchDecomp_H

#include "decompositionMethod.H"

namespace Foam
{

/*---------------------------------------------------------------------------*\
                       Class ptscotchDecomp Declaration
\*---------------------------------------------------------------------------*/

class ptscotchDecomp
:
    public decompositionMethod
{
    // Private Data

        //- Original coefficients for this method
        dictionary coeffsDict_;

        //- Output path and name for optional grf file.
        mutable fileName graphPath_;


    // Private Member Functions

        //- Set graph path and name (mutable)
        void graphPath(const polyMesh& mesh) const;

        //- Check and print error message
        static void check(const int, const char*);

        //- Decompose. Handles size 0 arrays
        label decompose
        (
            const labelList& adjncy,
            const labelList& xadj,
            const List<scalar>& cWeights,
            labelList& finalDecomp
        ) const;

        //- Low level decompose
        label decompose
        (
            const label adjncySize,
            const label adjncy[],
            const label xadjSize,
            const label xadj[],
            const List<scalar>& cWeights,
            labelList& finalDecomp
        ) const;


        //- No copy construct
        ptscotchDecomp(const ptscotchDecomp&) = delete;

        //- No copy assignment
        void operator=(const ptscotchDecomp&) = delete;


public:

    //- Runtime type information
    TypeName("ptscotch");


    // Constructors

        //- Construct given decomposition dictionary and optional region name
        explicit ptscotchDecomp
        (
            const dictionary& decompDict,
            const word& regionName = ""
        );


    //- Destructor
    virtual ~ptscotchDecomp() = default;


    // Member Functions

        virtual bool parallelAware() const
        {
            return true;
        }

        //- Inherit decompose from decompositionMethod
        using decompositionMethod::decompose;

        //- Return for every coordinate the wanted processor number.
        //  Use the mesh connectivity (if needed).
        //  See note on weights in scotchDecomp.H
        virtual labelList decompose
        (
            const polyMesh& mesh,
            const pointField& points,
            const scalarField& pointWeights
        ) const;

        //- Return for every coordinate the wanted processor number.
        //  Gets passed agglomeration map (from fine to coarse cells) and
        //  coarse cell location.
        //  Can be overridden by decomposers that provide this
        //  functionality natively. See note on weights in scotchDecomp.H
        virtual labelList decompose
        (
            const polyMesh& mesh,
            const labelList& agglom,
            const pointField& regionPoints,
            const scalarField& regionWeights
        ) const;

        //- Return for every coordinate the wanted processor number.
        //  Explicitly provided mesh connectivity.
        //  The connectivity is equal to mesh.cellCells() except for
        //  - in parallel the cell numbers are global cell numbers (starting
        //    from 0 at processor0 and then incrementing all through the
        //    processors)
        //  - the connections are across coupled patches
        //  See note on weights in scotchDecomp.H
        virtual labelList decompose
        (
            const labelListList& globalCellCells,
            const pointField& cc,
            const scalarField& cWeights
        ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
