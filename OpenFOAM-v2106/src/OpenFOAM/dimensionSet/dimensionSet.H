/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2017-2019 OpenCFD Ltd.
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
    Foam::dimensionSet

Description
    Dimension set for the base types.

    This type may be used to implement rigorous dimension checking
    for algebraic manipulation.

SourceFiles
    dimensionSet.C
    dimensionSetIO.C

\*---------------------------------------------------------------------------*/

#ifndef dimensionSet_H
#define dimensionSet_H

#include "bool.H"
#include "dimensionedScalarFwd.H"
#include "className.H"
#include "scalarField.H"
#include "PtrList.H"
#include "HashTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declarations

class dimensionSet;
class dimensionSets;

/*---------------------------------------------------------------------------*\
                        Class dimensionSet Declaration
\*---------------------------------------------------------------------------*/

class dimensionSet
{
public:

    //- The array of dimension exponents
    typedef FixedList<scalar,7> list_type;


    // Member Constants

        //- There are 7 base dimensions
        static constexpr int nDimensions = 7;

        //- Enumeration for the dimension exponents
        enum dimensionType
        {
            MASS,               //!< kilogram   kg
            LENGTH,             //!< metre      m
            TIME,               //!< second     s
            TEMPERATURE,        //!< Kelvin     K
            MOLES,              //!< mole       mol
            CURRENT,            //!< Ampere     A
            LUMINOUS_INTENSITY  //!< Candela    Cd
        };


    // Static Data Members

        //- Tolerance for 'small' exponents, for near-zero rounding
        static const scalar smallExponent;


private:

    // Private data

        //- The array of dimension exponents
        list_type exponents_;


    // Private classes

        class tokeniser
        {
            // Private data

                Istream& is_;

                List<token> tokens_;

                label start_;

                label size_;


            // Private Member Functions

                void push(const token& t);

                token pop();

                void unpop(const token& t);

        public:

            // Constructors

                 tokeniser(Istream& is);


            // Member Functions

                Istream& stream()
                {
                    return is_;
                }

                bool hasToken() const;

                token nextToken();

                void putBack(const token&);

                void splitWord(const word&);

                static bool valid(char c);

                static label priority(const token& t);
        };


        //- Reset exponents to nearest integer if close to it. Used to
        //  handle reading with insufficient precision.
        void round(const scalar tol);

        dimensionedScalar parse
        (
            const label lastPrior,
            tokeniser& tis,
            const HashTable<dimensionedScalar>&
        ) const;

public:

    // Declare name of the class and its debug switch
    ClassName("dimensionSet");


    // Constructors

        //- Default construct (dimensionless).
        dimensionSet();

        //- Construct from exponents for the first five or all seven dimensions
        dimensionSet
        (
            const scalar mass,
            const scalar length,
            const scalar time,
            const scalar temperature,
            const scalar moles,
            const scalar current = 0,
            const scalar luminousIntensity = 0
        );

        //- Construct from exponents for all seven dimensions
        dimensionSet(const FixedList<scalar,7>& dimensions);

        //- Copy construct
        dimensionSet(const dimensionSet& ds);

        //- Construct from dictionary entry - usually "dimensions".
        dimensionSet(const dictionary& dict, const word& entryName);

        //- Construct and return a clone
        autoPtr<dimensionSet> clone() const
        {
            return autoPtr<dimensionSet>::New(*this);
        }

        //- Construct from Istream
        explicit dimensionSet(Istream& is);


    // Member Functions

        //- Return true if it is dimensionless
        bool dimensionless() const;

        //- Return const access to the exponents as a list
        const FixedList<scalar,7>& values() const;

        //- Return non-const access to the exponents as a list
        FixedList<scalar,7>& values();

        //- Reset exponents to be dimensionless
        void clear();

        //- Copy assign the exponents from the dimensionSet
        void reset(const dimensionSet& ds);


    // IO

        //- Read using provided units. Used only in initial parsing
        Istream& read
        (
            Istream& is,
            scalar& multiplier,
            const dictionary&
        );

        //- Read using provided units
        Istream& read
        (
            Istream& is,
            scalar& multiplier,
            const HashTable<dimensionedScalar>&
        );

        //- Read using system units
        Istream& read
        (
            Istream& is,
            scalar& multiplier
        );

        //- Write using provided units
        Ostream& write
        (
            Ostream& os,
            scalar& multiplier,
            const dimensionSets&
        ) const;

        //- Write using system units
        Ostream& write
        (
            Ostream& os,
            scalar& multiplier
        ) const;


    // Member Operators

        scalar operator[](const dimensionType) const;
        scalar& operator[](const dimensionType);

        scalar operator[](const label) const;
        scalar& operator[](const label);

        bool operator==(const dimensionSet&) const;
        bool operator!=(const dimensionSet&) const;

        bool operator=(const dimensionSet&) const;

        bool operator+=(const dimensionSet&) const;
        bool operator-=(const dimensionSet&) const;
        bool operator*=(const dimensionSet&);
        bool operator/=(const dimensionSet&);
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// IOstream Operators

Istream& operator>>(Istream& is, dimensionSet& ds);
Ostream& operator<<(Ostream& os, const dimensionSet& ds);


// Global Functions

dimensionSet min(const dimensionSet& ds1, const dimensionSet& ds2);
dimensionSet max(const dimensionSet& ds1, const dimensionSet& ds2);
dimensionSet clip(const dimensionSet& ds1, const dimensionSet& ds2);

dimensionSet cmptMultiply(const dimensionSet& ds1, const dimensionSet& ds2);
dimensionSet cmptDivide(const dimensionSet& ds1, const dimensionSet& ds2);


dimensionSet pow(const dimensionSet& ds, const scalar p);
dimensionSet pow(const dimensionSet& ds, const dimensionedScalar& dS);
dimensionSet pow(const dimensionedScalar& dS, const dimensionSet& ds);

dimensionSet sqr(const dimensionSet& ds);
dimensionSet pow2(const dimensionSet& ds);
dimensionSet pow3(const dimensionSet& ds);
dimensionSet pow4(const dimensionSet& ds);
dimensionSet pow5(const dimensionSet& ds);
dimensionSet pow6(const dimensionSet& ds);
dimensionSet pow025(const dimensionSet& ds);

dimensionSet sqrt(const dimensionSet& ds);
dimensionSet cbrt(const dimensionSet& ds);

dimensionSet magSqr(const dimensionSet& ds);
dimensionSet mag(const dimensionSet& ds);
dimensionSet sign(const dimensionSet&);
dimensionSet pos(const dimensionSet&);
dimensionSet pos0(const dimensionSet&);
dimensionSet neg(const dimensionSet&);
dimensionSet neg0(const dimensionSet&);
dimensionSet posPart(const dimensionSet&);
dimensionSet negPart(const dimensionSet&);

//- The dimensionSet inverted
dimensionSet inv(const dimensionSet& ds);

//- Check the argument is dimensionless (for transcendental functions)
dimensionSet trans(const dimensionSet& ds);

//- Arguments need the same dimensions. Return dimensionless.
dimensionSet atan2(const dimensionSet& ds1, const dimensionSet& ds2);

//- Arguments need the same dimensions. Does not change the dimension.
dimensionSet hypot(const dimensionSet& ds1, const dimensionSet& ds2);

//- Return the argument; transformations do not change the dimensions
dimensionSet transform(const dimensionSet& ds);

//- Return the argument; transformations do not change the dimensions
dimensionSet invTransform(const dimensionSet& ds);


// Global Operators

//- The dimensionSet inverted
dimensionSet operator~(const dimensionSet& ds);

//- Unary negation.
dimensionSet operator-(const dimensionSet& ds);

dimensionSet operator+(const dimensionSet& ds1, const dimensionSet& ds2);
dimensionSet operator-(const dimensionSet& ds1, const dimensionSet& ds2);
dimensionSet operator*(const dimensionSet& ds1, const dimensionSet& ds2);
dimensionSet operator/(const dimensionSet& ds1, const dimensionSet& ds2);
dimensionSet operator&(const dimensionSet& ds1, const dimensionSet& ds2);
dimensionSet operator^(const dimensionSet& ds1, const dimensionSet& ds2);
dimensionSet operator&&(const dimensionSet& ds1, const dimensionSet& ds2);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "dimensionSets.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
