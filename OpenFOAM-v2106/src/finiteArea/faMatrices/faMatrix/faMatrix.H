/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
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
    Foam::faMatrix

Description
    Finite-Area matrix.

SourceFiles
    faMatrix.C
    faMatrixSolve.C

Author
    Zeljko Tukovic, FMENA
    Hrvoje Jasak, Wikki Ltd.

\*---------------------------------------------------------------------------*/

#ifndef faMatrix_H
#define faMatrix_H

#include "areaFields.H"
#include "edgeFields.H"
#include "lduMatrix.H"
#include "tmp.H"
#include "autoPtr.H"
#include "dimensionedTypes.H"
#include "className.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
template<class Type> class faMatrix;
template<class T> class UIndirectList;

template<class Type>
Ostream& operator<<(Ostream&, const faMatrix<Type>&);


/*---------------------------------------------------------------------------*\
                          Class faMatrix Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class faMatrix
:
    public refCount,
    public lduMatrix
{
    // Private Data

        //- Const reference to field
        //  Converted into a non-const reference at the point of solution.
        const GeometricField<Type, faPatchField, areaMesh>& psi_;

        //- Dimension set
        dimensionSet dimensions_;

        //- Source term
        Field<Type> source_;

        //- Boundary scalar field containing pseudo-matrix coeffs
        //- for internal faces
        FieldField<Field, Type> internalCoeffs_;

        //- Boundary scalar field containing pseudo-matrix coeffs
        //- for boundary faces
        FieldField<Field, Type> boundaryCoeffs_;

        //- Face flux field for non-orthogonal correction
        mutable GeometricField<Type, faePatchField, edgeMesh>
            *faceFluxCorrectionPtr_;


protected:

    //- Declare friendship with the faSolver class
    friend class faSolver;


    // Protected Member Functions

        //- Add patch contribution to internal field
        template<class Type2>
        void addToInternalField
        (
            const labelUList& addr,
            const Field<Type2>& pf,
            Field<Type2>& intf
        ) const;

        template<class Type2>
        void addToInternalField
        (
            const labelUList& addr,
            const tmp<Field<Type2>>& tpf,
            Field<Type2>& intf
        ) const;

        //- Subtract patch contribution from internal field
        template<class Type2>
        void subtractFromInternalField
        (
            const labelUList& addr,
            const Field<Type2>& pf,
            Field<Type2>& intf
        ) const;

        template<class Type2>
        void subtractFromInternalField
        (
            const labelUList& addr,
            const tmp<Field<Type2>>& tpf,
            Field<Type2>& intf
        ) const;


        // Matrix completion functionality

            void addBoundaryDiag
            (
                scalarField& diag,
                const direction cmpt
            ) const;

            void addCmptAvBoundaryDiag(scalarField& diag) const;

            void addBoundarySource
            (
                Field<Type>& source,
                const bool couples = true
            ) const;


        // Matrix manipulation functionality

            //- Set solution in given faces to the specified values
            template<template<class> class ListType>
            void setValuesFromList
            (
                const labelUList& faceLabels,
                const ListType<Type>& values
            );


public:

    //- Solver class returned by the solver function
    //- used for systems in which it is useful to cache the solver for reuse.
    class faSolver
    {
        faMatrix<Type>& faMat_;

        autoPtr<lduMatrix::solver> solver_;

    public:

        // Constructors

            faSolver(faMatrix<Type>& faMat, autoPtr<lduMatrix::solver>&& sol)
            :
                faMat_(faMat),
                solver_(std::move(sol))
            {}


        // Member Functions

            //- Solve returning the solution statistics.
            //  Solver controls read from dictionary
            SolverPerformance<Type> solve(const dictionary& solverControls);

            //- Solve returning the solution statistics.
            //  Solver controls read from faSolution
            SolverPerformance<Type> solve();
    };


    ClassName("faMatrix");


    // Constructors

        //- Construct given a field to solve for
        faMatrix
        (
            const GeometricField<Type, faPatchField, areaMesh>& psi,
            const dimensionSet& ds
        );

        //- Copy construct
        faMatrix(const faMatrix<Type>&);

        //- Construct from Istream given field to solve for
        faMatrix
        (
            const GeometricField<Type, faPatchField, areaMesh>& psi,
            Istream& is
        );

        //- Clone
        tmp<faMatrix<Type>> clone() const;


    //- Destructor
    virtual ~faMatrix();


    // Member Functions

        // Access

            const GeometricField<Type, faPatchField, areaMesh>& psi() const
            {
                return psi_;
            }

            const dimensionSet& dimensions() const
            {
                return dimensions_;
            }

            Field<Type>& source()
            {
                return source_;
            }

            const Field<Type>& source() const
            {
                return source_;
            }

            //- faBoundary scalar field containing pseudo-matrix coeffs
            //- for internal cells
            const FieldField<Field, Type>& internalCoeffs() const
            {
                return internalCoeffs_;
            }

            //- faBoundary scalar field containing pseudo-matrix coeffs
            //- for internal cells
            FieldField<Field, Type>& internalCoeffs()
            {
                return internalCoeffs_;
            }

            //- faBoundary scalar field containing pseudo-matrix coeffs
            //- for boundary cells
            const FieldField<Field, Type>& boundaryCoeffs() const
            {
                return boundaryCoeffs_;
            }

            //- faBoundary scalar field containing pseudo-matrix coeffs
            //- for boundary cells
            FieldField<Field, Type>& boundaryCoeffs()
            {
                return boundaryCoeffs_;
            }


            //- Declare return type of the faceFluxCorrectionPtr() function
            typedef GeometricField<Type, faePatchField, edgeMesh>
                *edgeTypeFieldPtr;

            //- Return pointer to face-flux non-orthogonal correction field
            edgeTypeFieldPtr& faceFluxCorrectionPtr()
            {
                return faceFluxCorrectionPtr_;
            }


        // Operations

            //- Set solution in given faces to the specified value
            //- and eliminate the corresponding equations from the matrix.
            void setValues
            (
                const labelUList& faceLabels,
                const Type& value
            );

            //- Set solution in given faces to the specified values
            //- and eliminate the corresponding equations from the matrix.
            void setValues
            (
                const labelUList& faceLabels,
                const UList<Type>& values
            );

            //- Set solution in given faces to the specified values
            //- and eliminate the corresponding equations from the matrix.
            void setValues
            (
                const labelUList& faceLabels,
                const UIndirectList<Type>& values
            );

            //- Set reference level for solution
            void setReference
            (
                const label facei,
                const Type& value,
                const bool forceReference = false
            );

            //- Set reference level for solution
            void setReferences
            (
                const labelUList& faceLabels,
                const Type& value,
                const bool forceReference = false
            );

            //- Set reference level for solution
            void setReferences
            (
                const labelUList& faceLabels,
                const UList<Type>& values,
                const bool forceReference = false
            );

            //- Set reference level for a component of the solution
            //- on a given patch face
            void setComponentReference
            (
                const label patchi,
                const label facei,
                const direction cmpt,
                const scalar value
            );

            //- Relax matrix (for steady-state solution).
            //  alpha = 1 : diagonally equal
            //  alpha < 1 :    ,,      dominant
            //  alpha = 0 : do nothing
            //  Note: Requires positive diagonal.
            void relax(const scalar alpha);

            //- Relax matrix (for steady-state solution).
            //  alpha is read from controlDict
            void relax();

            //- Solve returning the solution statistics.
            //  Solver controls read Istream
            SolverPerformance<Type> solve(const dictionary&);

            //- Solve returning the solution statistics.
            //  Solver controls read from faSolution
            SolverPerformance<Type> solve();

            //- Return the matrix residual
            tmp<Field<Type>> residual() const;

            //- Return the matrix diagonal
            tmp<scalarField> D() const;

            //- Return the central coefficient
            tmp<areaScalarField> A() const;

            //- Return the H operation source
            tmp<GeometricField<Type, faPatchField, areaMesh>> H() const;

            //- Return the face-flux field from the matrix
            tmp<GeometricField<Type, faePatchField, edgeMesh>> flux() const;


    // Member Operators

        void operator=(const faMatrix<Type>&);
        void operator=(const tmp<faMatrix<Type>>&);

        void negate();

        void operator+=(const faMatrix<Type>&);
        void operator+=(const tmp<faMatrix<Type>>&);

        void operator-=(const faMatrix<Type>&);
        void operator-=(const tmp<faMatrix<Type>>&);

        void operator+=(const GeometricField<Type,faPatchField,areaMesh>&);
        void operator+=(const tmp<GeometricField<Type,faPatchField,areaMesh>>&);

        void operator-=(const GeometricField<Type,faPatchField,areaMesh>&);
        void operator-=(const tmp<GeometricField<Type,faPatchField,areaMesh>>&);

        void operator+=(const dimensioned<Type>&);
        void operator-=(const dimensioned<Type>&);

        void operator*=(const areaScalarField&);
        void operator*=(const tmp<areaScalarField>&);

        void operator*=(const dimensioned<scalar>&);


    // Ostream Operator

        friend Ostream& operator<< <Type>
        (
            Ostream&,
            const faMatrix<Type>&
        );
};


// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

template<class Type>
void checkMethod
(
    const faMatrix<Type>&,
    const faMatrix<Type>&,
    const char*
);

template<class Type>
void checkMethod
(
    const faMatrix<Type>&,
    const GeometricField<Type, faPatchField, areaMesh>&,
    const char*
);

template<class Type>
void checkMethod
(
    const faMatrix<Type>&,
    const dimensioned<Type>&,
    const char*
);


//- Solve returning the solution statistics given convergence tolerance
//  Solver controls read Istream
template<class Type>
SolverPerformance<Type> solve(faMatrix<Type>&, Istream&);


//- Solve returning the solution statistics given convergence tolerance,
//- deleting temporary matrix after solution.
//  Solver controls read Istream
template<class Type>
SolverPerformance<Type> solve(const tmp<faMatrix<Type>>&, Istream&);


//- Solve returning the solution statistics given convergence tolerance
//  Solver controls read faSolution
template<class Type>
SolverPerformance<Type> solve(faMatrix<Type>&);


//- Solve returning the solution statistics given convergence tolerance,
//- deleting temporary matrix after solution.
//  Solver controls read faSolution
template<class Type>
SolverPerformance<Type> solve(const tmp<faMatrix<Type>>&);


// * * * * * * * * * * * * * * * Global Operators  * * * * * * * * * * * * * //

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const faMatrix<Type>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const tmp<faMatrix<Type>>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const faMatrix<Type>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const tmp<faMatrix<Type>>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const faMatrix<Type>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const tmp<faMatrix<Type>>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const faMatrix<Type>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const tmp<faMatrix<Type>>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const faMatrix<Type>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const tmp<faMatrix<Type>>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const faMatrix<Type>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const tmp<faMatrix<Type>>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const faMatrix<Type>&,
    const GeometricField<Type, faPatchField, areaMesh>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const tmp<faMatrix<Type>>&,
    const GeometricField<Type, faPatchField, areaMesh>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const faMatrix<Type>&,
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const tmp<faMatrix<Type>>&,
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const GeometricField<Type, faPatchField, areaMesh>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const GeometricField<Type, faPatchField, areaMesh>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const faMatrix<Type>&,
    const GeometricField<Type, faPatchField, areaMesh>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const tmp<faMatrix<Type>>&,
    const GeometricField<Type, faPatchField, areaMesh>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const faMatrix<Type>&,
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const tmp<faMatrix<Type>>&,
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const GeometricField<Type, faPatchField, areaMesh>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const GeometricField<Type, faPatchField, areaMesh>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const faMatrix<Type>&,
    const dimensioned<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const tmp<faMatrix<Type>>&,
    const dimensioned<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const dimensioned<Type>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator+
(
    const dimensioned<Type>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const faMatrix<Type>&,
    const dimensioned<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const tmp<faMatrix<Type>>&,
    const dimensioned<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const dimensioned<Type>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator-
(
    const dimensioned<Type>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const faMatrix<Type>&,
    const GeometricField<Type, faPatchField, areaMesh>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const tmp<faMatrix<Type>>&,
    const GeometricField<Type, faPatchField, areaMesh>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const faMatrix<Type>&,
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const tmp<faMatrix<Type>>&,
    const tmp<GeometricField<Type, faPatchField, areaMesh>>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const faMatrix<Type>&,
    const dimensioned<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator==
(
    const tmp<faMatrix<Type>>&,
    const dimensioned<Type>&
);


template<class Type>
tmp<faMatrix<Type>> operator*
(
    const areaScalarField&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator*
(
    const areaScalarField&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator*
(
    const tmp<areaScalarField>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator*
(
    const tmp<areaScalarField>&,
    const tmp<faMatrix<Type>>&
);

template<class Type>
tmp<faMatrix<Type>> operator*
(
    const dimensioned<scalar>&,
    const faMatrix<Type>&
);

template<class Type>
tmp<faMatrix<Type>> operator*
(
    const dimensioned<scalar>&,
    const tmp<faMatrix<Type>>&
);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "faMatrix.C"
#endif

// Specialisation for scalars

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
