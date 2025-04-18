/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2019-2021 OpenCFD Ltd.
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
    Foam::radiation::fvDOM

Group
    grpRadiationModels

Description

    Finite Volume Discrete Ordinates Method. Solves the RTE equation for n
    directions in a participating media, not including scatter and reflective
    walls.

    Available absorption models:

    - constantAbsorptionEmission
    - greyMeanAbsoprtionEmission
    - wideBandAbsorptionEmission
    - multiBandAbsorptionEmission

    This model can handle non-grey participating media using
    multiBandAbsorptionEmission model. Accordingly the BC for rays should
    be wideBandDiffussive type

Usage
    \verbatim
        fvDOMCoeffs
        {
            nPhi        4;          // azimuthal angles in PI/2 on X-Y.
                                    //(from Y to X)
            nTheta      0;          // polar angles in PI (from Z to X-Y plane)
            tolerance   1e-3;       // convergence tolerance for radiation
                                    // iteration
            maxIter     4;          // maximum number of iterations
            meshOrientation    (1 1 1); //Mesh orientation used for 2D and 1D

            useSolarLoad      false;
            useExternalBeam   true;
            spectralDistribution (2 1);
        }

        solverFreq   1; // Number of flow iterations per radiation iteration
    \endverbatim

    The total number of solid angles is  4*nPhi*nTheta in 3-D.

    Operating modes:
    - 1-D:
      - ray directions are on X, Y or Z
      - \c nPhi and \c nTheta entries are ignored
      - \c meshOrientation vector can be used for any other 1-D direction.
    - 2-D:
      - ray directions are on X-Y, X-Z or Y-Z planes
      - only the \c nPhi entry is considered
      - \c meshOrientation vector can be used for non-aligned planes
        specifying the plane normal vector.
    - 3-D:
      - rays geberated in 3-D using the \c nPhi and \c nTheta entries
      - \c meshOrientation vector is not applicable.

    useSolarLoad calculates the primary and diffusive Sun fluxes on walls in
    addition to the RTE equations

    useExternalBeam add an external collimated beam to the domain. This option
    is not available if useSolarLoad is true.

    spectralDistribution is the energy spectral distribution of the collimated
    external beam.

SourceFiles
    fvDOM.C

\*---------------------------------------------------------------------------*/

#ifndef radiation_fvDOM_H
#define radiation_fvDOM_H

#include "radiativeIntensityRay.H"
#include "radiationModel.H"
#include "fvMatrices.H"
#include "solarLoad.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace radiation
{

/*---------------------------------------------------------------------------*\
                           Class fvDOM Declaration
\*---------------------------------------------------------------------------*/

class fvDOM
:
    public radiationModel
{
    // Private Data

        //- Incident radiation  [W/m2]
        volScalarField G_;

        //- Total radiative heat flux [W/m2]
        volScalarField qr_;

        //- Emitted radiative heat flux [W/m2]
        volScalarField qem_;

        //- Incident radiative heat flux [W/m2]
        volScalarField qin_;

        //- Total absorption coefficient [1/m]
        volScalarField a_;

        //- Number of solid angles in theta
        label nTheta_;

        //- Number of solid angles in phi
        label nPhi_ ;

        //- Total number of rays (1 per direction)
        label nRay_;

        //- Number of wavelength bands
        label nLambda_;

        //- Wavelength total absorption coefficient [1/m]
        PtrList<volScalarField> aLambda_;

        //- Black body
        blackBodyEmission blackBody_;

        //- List of pointers to radiative intensity rays
        PtrList<radiativeIntensityRay> IRay_;

        //- Convergence tolerance
        scalar tolerance_;

        //- Maximum number of iterations
        label maxIter_;

        //- Maximum omega weight
        scalar omegaMax_;

        //- Use Solar Load model
        bool useSolarLoad_;

        //- Solar load radiation model
        autoPtr<solarLoad> solarLoad_;

        //- Mesh orientation vector
        vector meshOrientation_;

        //- Use external parallel irradiation beam
        bool useExternalBeam_;

        //- Spectral energy distribution for the external beam
        scalarList spectralDistribution_;

        //- Time-dependent spectral distributions
        autoPtr<TimeFunction1<scalarField>> spectralDistributions_;

        //- Solar calculator
        autoPtr<solarCalculator> solarCalculator_;

        //- Update Sun position index
        label updateTimeIndex_;


    // Private Member Functions

        //- Initialise
        void initialise();

        //- No copy construct
        fvDOM(const fvDOM&) = delete;

        //- No copy assignment
        void operator=(const fvDOM&) = delete;

        //- Update black body emission
        void updateBlackBodyEmission();


public:

    //- Runtime type information
    TypeName("fvDOM");


    // Constructors

        //- Construct from components
        fvDOM(const volScalarField& T);

        //- Construct from components
        fvDOM(const dictionary& dict, const volScalarField& T);


    //- Destructor
    virtual ~fvDOM() = default;


    // Member Functions

        // Edit

            //- Solve radiation equation(s)
            void calculate();

            //- Read radiation properties dictionary
            bool read();

            //- Update G and calculate total heat flux on boundary
            void updateG();

            //- Set the rayId and lambdaId from by decomposing an intensity
            //  field name
            void setRayIdLambdaId
            (
                const word& name,
                label& rayId,
                label& lambdaId
            ) const;


            //- Rotate rays according to Sun direction
            void updateRaysDir();

            //- Rotate rays spheric equator to sunDir
            void rotateInitialRays(const vector& sunDir);

            //- Align closest ray to sunDir
            void alignClosestRayToSun(const vector& sunDir);

            //- Source term component (for power of T^4)
            virtual tmp<volScalarField> Rp() const;

            //- Source term component (constant)
            virtual tmp<volScalarField::Internal> Ru() const;


        // Access

            //- Solar calculator
            const solarCalculator& solarCalc() const;

            //- Ray intensity for rayI
            inline const radiativeIntensityRay& IRay(const label rayI) const;

            //- Ray intensity for rayI and lambda bandwidth
            inline const volScalarField& IRayLambda
            (
                const label rayI,
                const label lambdaI
            ) const;

            //- Number of angles in theta
            inline label nTheta() const;

            //- Number of angles in phi
            inline label nPhi() const;

            //- Number of rays
            inline label nRay() const;

            //- Number of wavelengths
            inline label nLambda() const;

            //- Number of bands
            inline label nBands() const;

            //- Const access to total absorption coefficient
            inline const volScalarField& a() const;

            //- Const access to wavelength total absorption coefficient
            inline const volScalarField& aLambda(const label lambdaI) const;

            //- Const access to incident radiation field
            inline const volScalarField& G() const;

            //- Const access to total radiative heat flux field
            inline const volScalarField& qr() const;

            //- Const access to incident radiative heat flux field
            inline const volScalarField& qin() const;

            //- Const access to emitted radiative heat flux field
            inline const volScalarField& qem() const;

            //- Const access to black body
            inline const blackBodyEmission& blackBody() const;

            //- Return omegaMax
            inline scalar omegaMax() const;

            //- Return meshOrientation
            inline vector meshOrientation() const;

            //- Use solar load
            inline bool useSolarLoad() const;

            //- Use external beam
            inline bool useExternalBeam() const;

            //- Energy spectral distribution for external beam
            inline const scalarList& spectralDistribution() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "fvDOMI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace radiation
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
