/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2015 OpenFOAM Foundation
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

Namespace
    Foam::PstreamGlobals

Description
    Global functions and variables for working with parallel streams,
    but principally for mpi

SourceFiles
    PstreamGlobals.C

\*---------------------------------------------------------------------------*/

#ifndef PstreamGlobals_H
#define PstreamGlobals_H

#include "DynamicList.H"
#include <mpi.h>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace PstreamGlobals
{

//- Outstanding non-blocking operations.
extern DynamicList<MPI_Request> outstandingRequests_;
extern DynamicList<label> freedRequests_;

//- Max outstanding message tag operations.
extern int nTags_;

//- Free'd message tags
extern DynamicList<int> freedTags_;

//Default MPI communicator (either allocated by MUI if in use or becomes MPI_COMM_WORLD)
extern MPI_Comm commWorld_;

// Current communicators. First element will be MPI_COMM_WORLD
extern DynamicList<MPI_Comm> MPICommunicators_;
extern DynamicList<MPI_Group> MPIGroups_;


void checkCommunicator(const label comm, const label toProcNo);

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
