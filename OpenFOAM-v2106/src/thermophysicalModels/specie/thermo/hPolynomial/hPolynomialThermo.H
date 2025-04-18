/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
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
    Foam::hPolynomialThermo

Group
    grpSpecieThermo

Description
    Thermodynamics package templated on the equation of state, using polynomial
    functions for \c cp, \c h and \c s.

    Polynomials for \c h and \c s derived from \c cp.

Usage

    \table
        Property     | Description
        Hf           | Heat of formation
        Sf           | Standard entropy
        CpCoeffs<8>  | Specific heat at constant pressure polynomial coeffs
    \endtable

    Example of the specification of the thermodynamic properties:
    \verbatim
    thermodynamics
    {
        Hf              0;
        Sf              0;
        CpCoeffs<8>     ( 1000 -0.05 0.003 0 0 0 0 0 );
    }
    \endverbatim

    The polynomial expression is evaluated as so:

        \f[
            Cp = 1000 - 0.05 T + 0.003 T^2
        \f]

Note
    - Heat of formation is inputted in [J/kg], but internally uses [J/kmol]
    - Standard entropy is inputted in [J/kg/K], but internally uses [J/kmol/K]
    - Specific heat at constant pressure polynomial coefficients evaluate to an
      expression in [J/(kg.K)].

SourceFiles
    hPolynomialThermoI.H
    hPolynomialThermo.C

See also
    Foam::Polynomial

\*---------------------------------------------------------------------------*/

#ifndef hPolynomialThermo_H
#define hPolynomialThermo_H

#include "scalar.H"
#include "Polynomial.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations

template<class EquationOfState, int PolySize>
class hPolynomialThermo;

template<class EquationOfState, int PolySize>
inline hPolynomialThermo<EquationOfState, PolySize> operator+
(
    const hPolynomialThermo<EquationOfState, PolySize>&,
    const hPolynomialThermo<EquationOfState, PolySize>&
);

template<class EquationOfState, int PolySize>
inline hPolynomialThermo<EquationOfState, PolySize> operator*
(
    const scalar,
    const hPolynomialThermo<EquationOfState, PolySize>&
);

template<class EquationOfState, int PolySize>
inline hPolynomialThermo<EquationOfState, PolySize> operator==
(
    const hPolynomialThermo<EquationOfState, PolySize>&,
    const hPolynomialThermo<EquationOfState, PolySize>&
);

template<class EquationOfState, int PolySize>
Ostream& operator<<
(
    Ostream&,
    const hPolynomialThermo<EquationOfState, PolySize>&
);


/*---------------------------------------------------------------------------*\
                      Class hPolynomialThermo Declaration
\*---------------------------------------------------------------------------*/

template<class EquationOfState, int PolySize=8>
class hPolynomialThermo
:
    public EquationOfState
{
    // Private Data

        //- Heat of formation
        scalar Hf_;

        //- Standard entropy
        scalar Sf_;

        //- Specific heat at constant pressure polynomial coeffs
        Polynomial<PolySize> CpCoeffs_;

        //- Enthalpy polynomial coeffs - derived from cp [J/kg]
        //  NOTE: relative to Tstd
        typename Polynomial<PolySize>::intPolyType hCoeffs_;

        //- Entropy - derived from Cp [J/(kg.K)] - relative to Tstd
        Polynomial<PolySize> sCoeffs_;


    // Private Member Functions

        //- Coeffs name. Eg, "CpCoeffs<10>"
        inline static word coeffsName(const char* name)
        {
            return word(name + ("Coeffs<" + std::to_string(PolySize) + '>'));
        }

        //- Construct from components
        inline hPolynomialThermo
        (
            const EquationOfState& pt,
            const scalar Hf,
            const scalar Sf,
            const Polynomial<PolySize>& CpCoeffs,
            const typename Polynomial<PolySize>::intPolyType& hCoeffs,
            const Polynomial<PolySize>& sCoeffs
        );


public:

    // Generated Methods: copy construct, copy assignment


    // Constructors

        //- Construct from dictionary
        hPolynomialThermo(const dictionary& dict);

        //- Construct as a named copy
        inline hPolynomialThermo(const word&, const hPolynomialThermo&);


    // Member Functions

        //- Return the instantiated type name
        static word typeName()
        {
            return "hPolynomial<" + EquationOfState::typeName() + '>';
        }

        //- Limit temperature to be within the range
        inline scalar limit(const scalar) const;


        // Fundamental properties

            //- Heat capacity at constant pressure [J/(kg K)]
            inline scalar Cp(const scalar p, const scalar T) const;

            //- Absolute Enthalpy [J/kg]
            inline scalar Ha(const scalar p, const scalar T) const;

            //- Sensible enthalpy [J/kg]
            inline scalar Hs(const scalar p, const scalar T) const;

            //- Chemical enthalpy [J/kg]
            inline scalar Hc() const;

            //- Entropy [J/(kg K)]
            inline scalar S(const scalar p, const scalar T) const;

            //- Gibbs free energy of the mixture in the standard state [J/kg]
            inline scalar Gstd(const scalar T) const;


            #include "HtoEthermo.H"


        // Derivative term used for Jacobian


            //- Temperature derivative of heat capacity at constant pressure
            inline scalar dCpdT(const scalar p, const scalar T) const;



        // IO

            //- Write to Ostream
            void write(Ostream& os) const;


    // Member Operators

        inline void operator+=(const hPolynomialThermo&);
        inline void operator*=(const scalar);


    // Friend Operators

        friend hPolynomialThermo operator+ <EquationOfState, PolySize>
        (
            const hPolynomialThermo&,
            const hPolynomialThermo&
        );

        friend hPolynomialThermo operator* <EquationOfState, PolySize>
        (
            const scalar,
            const hPolynomialThermo&
        );

        friend hPolynomialThermo operator== <EquationOfState, PolySize>
        (
            const hPolynomialThermo&,
            const hPolynomialThermo&
        );


    // IOstream Operators

        friend Ostream& operator<< <EquationOfState, PolySize>
        (
            Ostream&,
            const hPolynomialThermo&
        );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "hPolynomialThermoI.H"

#ifdef NoRepository
    #include "hPolynomialThermo.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
