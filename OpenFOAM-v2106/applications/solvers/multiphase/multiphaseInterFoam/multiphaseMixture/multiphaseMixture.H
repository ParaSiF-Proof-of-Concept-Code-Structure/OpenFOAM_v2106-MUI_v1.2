/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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
    Foam::multiphaseMixture

Description
    Incompressible multi-phase mixture with built in solution for the
    phase fractions with interface compression for interface-capturing.

    Derived from transportModel so that it can be unused in conjunction with
    the incompressible turbulence models.

    Surface tension and contact-angle is handled for the interface
    between each phase-pair.

SourceFiles
    multiphaseMixture.C

\*---------------------------------------------------------------------------*/

#ifndef multiphaseMixture_H
#define multiphaseMixture_H

#include "incompressible/transportModel/transportModel.H"
#include "IOdictionary.H"
#include "phase.H"
#include "PtrDictionary.H"
#include "volFields.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                      Class multiphaseMixture Declaration
\*---------------------------------------------------------------------------*/

class multiphaseMixture
:
    public IOdictionary,
    public transportModel
{
public:

    //- Symmetric pair of interface names
    class interfacePair
    :
        public Pair<word>
    {
    public:

        // Always use symmetric hashing
        using hasher = Pair<word>::symmHasher;

        // Always use symmetric hashing (alias)
        using hash = Pair<word>::symmHasher;


        // Constructors

            interfacePair() = default;

            interfacePair(const word& alpha1Name, const word& alpha2Name)
            :
                Pair<word>(alpha1Name, alpha2Name)
            {}

            interfacePair(const phase& alpha1, const phase& alpha2)
            :
                Pair<word>(alpha1.name(), alpha2.name())
            {}


        // Friend Operators

            friend bool operator==
            (
                const interfacePair& a,
                const interfacePair& b
            )
            {
                return (0 != Pair<word>::compare(a, b));
            }

            friend bool operator!=
            (
                const interfacePair& a,
                const interfacePair& b
            )
            {
                return (!(a == b));
            }
    };


private:

    // Private data

        //- Dictionary of phases
        PtrDictionary<phase> phases_;

        const fvMesh& mesh_;
        const volVectorField& U_;
        const surfaceScalarField& phi_;

        surfaceScalarField rhoPhi_;
        volScalarField alphas_;

        volScalarField nu_;

        typedef HashTable<scalar, interfacePair, interfacePair::hash>
            sigmaTable;

        sigmaTable sigmas_;
        dimensionSet dimSigma_;

        //- Stabilisation for normalisation of the interface normal
        const dimensionedScalar deltaN_;


    // Private member functions

        void calcAlphas();

        void solveAlphas(const scalar cAlpha);

        tmp<surfaceVectorField> nHatfv
        (
            const volScalarField& alpha1,
            const volScalarField& alpha2
        ) const;

        tmp<surfaceScalarField> nHatf
        (
            const volScalarField& alpha1,
            const volScalarField& alpha2
        ) const;

        void correctContactAngle
        (
            const phase& alpha1,
            const phase& alpha2,
            surfaceVectorField::Boundary& nHatb
        ) const;

        tmp<volScalarField> K(const phase& alpha1, const phase& alpha2) const;


public:

    // Constructors

        //- Construct from components
        multiphaseMixture
        (
            const volVectorField& U,
            const surfaceScalarField& phi
        );


    //- Destructor
    virtual ~multiphaseMixture() = default;


    // Member Functions

        //- Return the phases
        const PtrDictionary<phase>& phases() const
        {
            return phases_;
        }

        //- Return the velocity
        const volVectorField& U() const
        {
            return U_;
        }

        //- Return the volumetric flux
        const surfaceScalarField& phi() const
        {
            return phi_;
        }

        const surfaceScalarField& rhoPhi() const
        {
            return rhoPhi_;
        }

        //- Return the mixture density
        tmp<volScalarField> rho() const;

        //- Return the mixture density for patch
        tmp<scalarField> rho(const label patchi) const;

        //- Return the dynamic laminar viscosity
        tmp<volScalarField> mu() const;

        //- Return the dynamic laminar viscosity for patch
        tmp<scalarField> mu(const label patchi) const;

        //- Return the face-interpolated dynamic laminar viscosity
        tmp<surfaceScalarField> muf() const;

        //- Return the kinematic laminar viscosity
        tmp<volScalarField> nu() const;

        //- Return the laminar viscosity for patch
        tmp<scalarField> nu(const label patchi) const;

        //- Return the face-interpolated dynamic laminar viscosity
        tmp<surfaceScalarField> nuf() const;

        tmp<surfaceScalarField> surfaceTensionForce() const;

        //- Indicator of the proximity of the interface
        //  Field values are 1 near and 0 away for the interface.
        tmp<volScalarField> nearInterface() const;

        //- Solve for the mixture phase-fractions
        void solve();

        //- Correct the mixture properties
        void correct();

        //- Read base transportProperties dictionary
        bool read();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
