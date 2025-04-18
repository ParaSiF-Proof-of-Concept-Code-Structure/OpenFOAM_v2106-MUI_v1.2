/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2019-2020 OpenCFD Ltd.
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
    Foam::regionFaModels::KirchhoffShell

Description

Usage
    Example of the boundary condition specification:
    \verbatim
    <patchName>
    {
        // Mandatory/Optional (inherited) entries
        ...

        // Mandatory entries (unmodifiable)
        vibrationShellModel   KirchhoffShell;
        f0                    0.04;
        f1                    0.0;
        f2                    0.0;
    }
    \endverbatim

    where the entries mean:
    \table
      Property   | Description                         | Type  | Reqd | Dflt
      vibrationShellModel | Type name: KirchhoffShell  | word  | yes  | -
      f0         | Damping coefficient [1/s]           | scalar | yes | -
      f1         | Damping coefficient [1/s]           | scalar | yes | -
      f2         | Damping coefficient [1/s]           | scalar | yes | -
    \endtable

    The inherited entries are elaborated in:
      - \link vibrationShellModel.H \endlink

SourceFiles
    KirchhoffShell.C

\*---------------------------------------------------------------------------*/

#ifndef KirchhoffShell_H
#define KirchhoffShell_H

#include "volFieldsFwd.H"
#include "vibrationShellModel.H"
#include "faMesh.H"
#include "faCFD.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace regionModels
{

/*---------------------------------------------------------------------------*\
                        Class KirchhoffShell Declaration
\*---------------------------------------------------------------------------*/

class KirchhoffShell
:
    public vibrationShellModel
{
    // Private Data

        //- Damping coefficients [1/s]
        dimensionedScalar f0_;
        dimensionedScalar f1_;
        dimensionedScalar f2_;


    // Private Member Functions

        //- Initialize KirchhoffShell
        void init();


protected:

    // Protected Data

        // Solution parameters

            //- Number of non orthogonal correctors
            label nNonOrthCorr_;

            //- Sub cycles
            label nSubCycles_;


        // Source term fields

            //- External surface source [Pa]
            areaScalarField ps_;

            //- Thickness [m]
            areaScalarField h_;

            //- Laplace of the displacement
            areaScalarField laplaceW_;

            //- Laplace of the Laplace for the displacement
            areaScalarField laplace2W_;

            //- Cache w.oldTime() in sub-cycling
            areaScalarField w0_;

            //- Cache w.oldTime.oldTime() in sub-cycling
            areaScalarField w00_;

            //- Cache laplaceW.oldTime() in sub-cycling
            areaScalarField laplaceW0_;

            //- Cache laplace2.oldTime() in sub-cycling
            areaScalarField laplace2W0_;


    // Protected Member Functions

        //- Read control parameters from dictionary
        virtual bool read(const dictionary& dict);


        // Equations

            //- Solve energy equation
            void solveDisplacement();


public:

    //- Runtime type information
    TypeName("KirchhoffShell");


    // Constructors

        //- Construct from components and dict
        KirchhoffShell
        (
            const word& modelType,
            const fvPatch& patch,
            const dictionary& dict
        );

        //- No copy construct
        KirchhoffShell(const KirchhoffShell&) = delete;

        //- No copy assignment
        void operator=(const KirchhoffShell&) = delete;


    //- Destructor
    virtual ~KirchhoffShell() = default;


    // Member Functions

        // Fields

            //- Return stiffness
            const tmp<areaScalarField> D() const;

            //- Return density [Kg/m3]
            const tmp<areaScalarField> rho() const;


        // Evolution

            //- Pre-evolve  thermal baffle
            virtual void preEvolveRegion();

            //- Evolve the thermal baffle
            virtual void evolveRegion();


       // IO

            //- Provide some feedback
            virtual void info();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace regionModels
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


#endif

// ************************************************************************* //
