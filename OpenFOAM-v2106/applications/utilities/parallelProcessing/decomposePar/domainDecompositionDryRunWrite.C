/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

\*---------------------------------------------------------------------------*/

#include "domainDecompositionDryRun.H"
#include "foamVtkInternalMeshWriter.H"
#include "volFields.H"
#include "zeroGradientFvPatchFields.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::domainDecompositionDryRun::writeVolField
(
    const word& timeName,
    const labelUList& procIds
) const
{
    // Write decomposition as volScalarField for visualization
    volScalarField cellDist
    (
        IOobject
        (
            "cellDist",
            timeName,
            this->mesh(),
            IOobject::NO_READ,
            IOobject::NO_WRITE,
            false
        ),
        this->mesh(),
        dimensionedScalar("cellDist", dimless, -1),
        zeroGradientFvPatchScalarField::typeName
    );

    forAll(procIds, celli)
    {
        cellDist[celli] = procIds[celli];
    }

    cellDist.correctBoundaryConditions();
    cellDist.write();

    Info<< nl << "Wrote decomposition to "
        << this->mesh().time().relativePath(cellDist.objectPath())
        << " (volScalarField) for visualization."
        << endl;
}


void Foam::domainDecompositionDryRun::writeVTK
(
    const fileName& file,
    const labelUList& cellToProc
) const
{
    const vtk::vtuCells cells(this->mesh());

    // not parallel
    vtk::internalMeshWriter writer(this->mesh(), cells, file, false);

    writer.writeGeometry();
    writer.beginCellData();
    writer.writeCellData("procID", cellToProc);

    Info<< "Wrote decomposition to "
        << this->mesh().time().relativePath(writer.output())
        << " for visualization."
        << endl;
}


// ************************************************************************* //
