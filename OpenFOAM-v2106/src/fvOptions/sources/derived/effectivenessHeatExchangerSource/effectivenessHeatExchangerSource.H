/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2017 OpenFOAM Foundation
    Copyright (C) 2016-2020 OpenCFD Ltd.
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
    Foam::fv::effectivenessHeatExchangerSource

Group
    grpFvOptionsSources

Description
    Heat exchanger source model for compressible flows, in which the heat
    exchanger is defined as an energy source using a selection of cells.

    The total heat exchange source is given by:
    \f[
        Q_t = e(\phi, \dot{m}_2) (T_2 - T_1) \phi c_p
    \f]

    where:
    \vartable
        Q_t       | total heat source
        e(\phi,\dot{m}_2) | effectivenes table
        \phi      | net mass flux entering heat exchanger [kg/s]
        \dot{m}_2 | secondary mass flow rate [kg/s]
        T_1       | primary inlet temperature [K]
        T_2       | secondary inlet temperature [K]
        c_p       | specific heat capacity [J/kg/K]
    \endvartable


    The distribution inside the heat exchanger is given by:
    \f[
        Q_c = \frac{V_c |U_c| (T_c - T_{ref})}{\sum(V_c |U_c| (T_c - T_{ref}))}
    \f]

    where:
    \vartable
        Q_c     | source for cell
        V_c     | volume of the cell [m3]
        U_c     | local cell velocity [m/s]
        T_c     | local call temperature [K]
        T_{ref} | min or max(T) in cell zone depending on the sign of Q_t [K]
    \endvartable

    Sources applied to either of the below, if exist:
    \verbatim
      e         | Internal energy                            [m2/s2]
      h         | Enthalphy                                  [m2/s2]
    \endverbatim

    Required fields:
    \verbatim
      T         | Temperature                                [K]
      U         | Velocity                                   [m/s]
      phi       | Mass flux                                  [kg/s]
    \endverbatim

Usage
    Minimal example by using \c constant/fvOptions:
    \verbatim
    effectivenessHeatExchangerSource1
    {
        // Mandatory entries (unmodifiable)
        type                effectivenessHeatExchangerSource;

        // Mandatory entries (runtime modifiable)
        faceZone                 <faceZoneName>;
        secondaryMassFlowRate    1.0;
        secondaryInletT          336;

        outOfBounds              clamp;
        file                     "effTable";

        // Optional entries (runtime modifiable)
        primaryInletT            293;
        targetQdot               1500;
        U                        <Uname>;
        T                        <Tname>;
        phi                      <phiName>;

        // Conditional optional entries (runtime modifiable)

            // when the entry "targetQdot" is present
            targetQdotCalcInterval  1;
            targetQdotRelax         1.0;

        // Mandatory/Optional (inherited) entries
        ...
    }
    \endverbatim

    where the entries mean:
    \table
      Property  | Description                       | Type  | Reqd | Dflt
      type      | Type name: effectivenessHeatExchangerSource <!--
                -->                                 | word  | yes  | -
      secondaryMassFlowRate | Secondary flow mass rate [kg/s] <!--
                -->                                 | scalar | yes | -
      secondaryInletT | Inlet secondary temperature [K]       <!--
                -->                                 | scalar | yes | -
      faceZone  | Name of the faceZone at the heat exchange inlet <!--
                -->                                 | word   | yes | -
      file       | 2D look up table efficiency = function of primary <!--
                --> and secondary mass flow rates [kg/s] | file | yes | -
      primaryInletT | Primary air temperature at the heat exchanger inlet <!--
                -->                                 | scalar | no  | -
      targetQdot |  Target heat rejection           | scalar | no  | -
      targetQdotCalcInterval | Target heat rejection calculation interval <!--
                -->                                 | label  | no  | -
      targetQdotRelax | Target heat rejection temperature <!--
                --> under-relaxation coefficient    | scalar | no  | -
      U          | Name of operand velocity field   | word   | no  | U
      T          | Name of operand temperature field  | word   | no  | T
      phi        | Name of operand flux field   | word   | no  | phi
    \endtable

    The inherited entries are elaborated in:
     - \link fvOption.H \endlink
     - \link cellSetOption.H \endlink

    The effectiveness table is described in terms of the primary and secondary
    mass flow rates.  For example, the table:

    \verbatim
                           secondary MFR
                       |  0.1   0.2   0.3
                  -----+-----------------
                  0.02 |   A     B     C
     primary MFR  0.04 |   D     E     F
                  0.06 |   G     H     I
    \endverbatim

    is specified by the following:

    \verbatim
        (
            (
                0.02
                (
                    (0.1    A)
                    (0.2    B)
                    (0.3    C)
                )
            )
            (
                0.04
                (
                    (0.1    D)
                    (0.2    E)
                    (0.3    F)
                )
            )
            (
                0.06
                (
                    (0.1    G)
                    (0.2    H)
                    (0.3    I)
                )
            )
        );
    \endverbatim

Note
  - The table with name \c file should have the same units as the
  secondary mass flow rate and kg/s for \c phi.
  - \c faceZone is the faces at the inlet of the \c cellZone, it needs to be
  created with flip map flags. It is used to integrate the net mass flow
  rate into the heat exchanger.
  - \c primaryInletT sets the primary inlet temperature.  If not set, the
  flux-averaged temperature is used.

SourceFiles
    effectivenessHeatExchangerSource.C

\*---------------------------------------------------------------------------*/

#ifndef fv_effectivenessHeatExchangerSource_H
#define fv_effectivenessHeatExchangerSource_H

#include "cellSetOption.H"
#include "autoPtr.H"
#include "interpolation2DTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

/*---------------------------------------------------------------------------*\
              Class effectivenessHeatExchangerSource Declaration
\*---------------------------------------------------------------------------*/

class effectivenessHeatExchangerSource
:
    public cellSetOption
{
protected:

    // Protected Data

        //- Secondary flow mass rate [kg/s]
        scalar secondaryMassFlowRate_;

        //- Inlet secondary temperature [K]
        scalar secondaryInletT_;

        //- Primary air temperature at the heat exchanger inlet [K]
        scalar primaryInletT_;

        //- Flag to use a user-specified primary inlet temperature
        bool userPrimaryInletT_;

        //- Flag to use target heat rejection
        bool targetQdotActive_;

        //- Target heat rejection
        scalar targetQdot_;

        //- Target heat rejection calculation interval
        label targetQdotCalcInterval_;

        //- Target heat rejection temperature under-relaxation coefficient
        scalar targetQdotRelax_;

        //- 2D look up table efficiency = function of primary and secondary
        //- mass flow rates [kg/s]
        autoPtr<interpolation2DTable<scalar>> eTable_;

        //- Name of velocity field; default = U
        word UName_;

        //- Name of temperature field; default = T
        word TName_;

        //- Name of the flux
        word phiName_;

        //- Name of the faceZone at the heat exchange inlet
        word faceZoneName_;

         //- Local list of face IDs
        labelList faceId_;

        //- Local list of patch ID per face
        labelList facePatchId_;

        //- List of +1/-1 representing face flip map (1 use as is, -1 negate)
        labelList faceSign_;


private:

    // Private Member Functions

        //- Initialise heat exchanger source model
        void initialise();

        //- Calculate total area of faceZone across processors
        void calculateTotalArea(scalar& area);


public:

    //- Runtime type information
    TypeName("effectivenessHeatExchangerSource");


    // Constructors

        //- Construct from components
        effectivenessHeatExchangerSource
        (
            const word& name,
            const word& modelType,
            const dictionary& dict,
            const fvMesh& mesh
        );

        //- No copy construct
        effectivenessHeatExchangerSource
        (
            const effectivenessHeatExchangerSource&
        ) = delete;

        //- No copy assignment
        void operator=(const effectivenessHeatExchangerSource&) = delete;


    //- Destructor
    virtual ~effectivenessHeatExchangerSource() = default;


    // Member Functions

        //- Add explicit/implicit contribution to momentum equation
        virtual void addSup
        (
            fvMatrix<scalar>& eqn,
            const label fieldi
        )
        {
            NotImplemented;
        }

        //- Add explicit/implicit contribution
        //- to compressible momentum equation
        virtual void addSup
        (
            const volScalarField& rho,
            fvMatrix<scalar>& eqn,
            const label fieldi
        );


        // IO

            //- Read dictionary
            virtual bool read(const dictionary& dict);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
