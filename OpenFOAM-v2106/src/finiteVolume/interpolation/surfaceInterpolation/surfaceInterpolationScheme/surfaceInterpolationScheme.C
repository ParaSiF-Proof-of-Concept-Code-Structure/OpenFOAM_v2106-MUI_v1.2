/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

\*---------------------------------------------------------------------------*/

#include "surfaceInterpolationScheme.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "geometricOneField.H"
#include "coupledFvPatchField.H"

// * * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::surfaceInterpolationScheme<Type>>
Foam::surfaceInterpolationScheme<Type>::New
(
    const fvMesh& mesh,
    Istream& schemeData
)
{
    if (schemeData.eof())
    {
        FatalIOErrorInFunction(schemeData)
            << "Discretisation scheme not specified\n\n"
            << "Valid schemes:\n"
            << MeshConstructorTablePtr_->sortedToc()
            << exit(FatalIOError);
    }

    const word schemeName(schemeData);

    if (surfaceInterpolation::debug || surfaceInterpolationScheme<Type>::debug)
    {
        InfoInFunction << "Discretisation scheme = " << schemeName << endl;
    }

    auto cstrIter = MeshConstructorTablePtr_->cfind(schemeName);

    if (!cstrIter.found())
    {
        FatalIOErrorInLookup
        (
            schemeData,
            "discretisation",
            schemeName,
            *MeshConstructorTablePtr_
        ) << exit(FatalIOError);
    }

    return cstrIter()(mesh, schemeData);
}


template<class Type>
Foam::tmp<Foam::surfaceInterpolationScheme<Type>>
Foam::surfaceInterpolationScheme<Type>::New
(
    const fvMesh& mesh,
    const surfaceScalarField& faceFlux,
    Istream& schemeData
)
{
    if (schemeData.eof())
    {
        FatalIOErrorInFunction(schemeData)
            << "Discretisation scheme not specified"
            << endl << endl
            << "Valid schemes are :" << endl
            << MeshConstructorTablePtr_->sortedToc()
            << exit(FatalIOError);
    }

    const word schemeName(schemeData);

    if (surfaceInterpolation::debug || surfaceInterpolationScheme<Type>::debug)
    {
        InfoInFunction << "Discretisation scheme = " << schemeName << endl;
    }

    auto cstrIter = MeshFluxConstructorTablePtr_->cfind(schemeName);

    if (!cstrIter.found())
    {
        FatalIOErrorInLookup
        (
            schemeData,
            "discretisation",
            schemeName,
            *MeshFluxConstructorTablePtr_
        ) << exit(FatalIOError);
    }

    return cstrIter()(mesh, faceFlux, schemeData);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::surfaceInterpolationScheme<Type>::interpolate
(
    const GeometricField<Type, fvPatchField, volMesh>& vf,
    const tmp<surfaceScalarField>& tlambdas,
    const tmp<surfaceScalarField>& tys
)
{
    if (surfaceInterpolation::debug)
    {
        InfoInFunction
            << "Interpolating "
            << vf.type() << " "
            << vf.name()
            << " from cells to faces without explicit correction"
            << endl;
    }

    const surfaceScalarField& lambdas = tlambdas();
    const surfaceScalarField& ys = tys();

    const Field<Type>& vfi = vf;
    const scalarField& lambda = lambdas;
    const scalarField& y = ys;

    const fvMesh& mesh = vf.mesh();
    const labelUList& P = mesh.owner();
    const labelUList& N = mesh.neighbour();

    tmp<GeometricField<Type, fvsPatchField, surfaceMesh>> tsf
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "interpolate("+vf.name()+')',
                vf.instance(),
                vf.db()
            ),
            mesh,
            vf.dimensions()
        )
    );
    GeometricField<Type, fvsPatchField, surfaceMesh>& sf = tsf.ref();

    Field<Type>& sfi = sf.primitiveFieldRef();

    for (label fi=0; fi<P.size(); fi++)
    {
        sfi[fi] = lambda[fi]*vfi[P[fi]] + y[fi]*vfi[N[fi]];
    }


    // Interpolate across coupled patches using given lambdas and ys
    typename GeometricField<Type, fvsPatchField, surfaceMesh>::
        Boundary& sfbf = sf.boundaryFieldRef();

    forAll(lambdas.boundaryField(), pi)
    {
        const fvsPatchScalarField& pLambda = lambdas.boundaryField()[pi];
        const fvsPatchScalarField& pY = ys.boundaryField()[pi];

        if (vf.boundaryField()[pi].coupled())
        {
            sfbf[pi] =
                pLambda*vf.boundaryField()[pi].patchInternalField()
              + pY*vf.boundaryField()[pi].patchNeighbourField();
        }
        else
        {
            sfbf[pi] = vf.boundaryField()[pi];
        }
    }

    tlambdas.clear();
    tys.clear();

    return tsf;
}


template<class Type>
template<class SFType>
Foam::tmp
<
    Foam::GeometricField
    <
        typename Foam::innerProduct<typename SFType::value_type, Type>::type,
        Foam::fvsPatchField,
        Foam::surfaceMesh
    >
>
Foam::surfaceInterpolationScheme<Type>::dotInterpolate
(
    const SFType& Sf,
    const GeometricField<Type, fvPatchField, volMesh>& vf,
    const tmp<surfaceScalarField>& tlambdas
)
{
    if (surfaceInterpolation::debug)
    {
        InfoInFunction
            << "Interpolating "
            << vf.type() << " "
            << vf.name()
            << " from cells to faces without explicit correction"
            << endl;
    }

    typedef typename Foam::innerProduct<typename SFType::value_type, Type>::type
        RetType;

    const surfaceScalarField& lambdas = tlambdas();

    const Field<Type>& vfi = vf;
    const scalarField& lambda = lambdas;

    const fvMesh& mesh = vf.mesh();
    const labelUList& P = mesh.owner();
    const labelUList& N = mesh.neighbour();

    tmp<GeometricField<RetType, fvsPatchField, surfaceMesh>> tsf
    (
        new GeometricField<RetType, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "interpolate("+vf.name()+')',
                vf.instance(),
                vf.db()
            ),
            mesh,
            Sf.dimensions()*vf.dimensions()
        )
    );
    GeometricField<RetType, fvsPatchField, surfaceMesh>& sf = tsf.ref();

    Field<RetType>& sfi = sf.primitiveFieldRef();

    const typename SFType::Internal& Sfi = Sf();

    for (label fi=0; fi<P.size(); fi++)
    {
        sfi[fi] = Sfi[fi] & (lambda[fi]*(vfi[P[fi]] - vfi[N[fi]]) + vfi[N[fi]]);
    }

    // Interpolate across coupled patches using given lambdas

    typename GeometricField<RetType, fvsPatchField, surfaceMesh>::
        Boundary& sfbf = sf.boundaryFieldRef();

    forAll(lambdas.boundaryField(), pi)
    {
        const fvsPatchScalarField& pLambda = lambdas.boundaryField()[pi];
        const typename SFType::Patch& pSf = Sf.boundaryField()[pi];
        fvsPatchField<RetType>& psf = sfbf[pi];

        if (vf.boundaryField()[pi].coupled())
        {
            psf =
                pSf
              & (
                    pLambda*vf.boundaryField()[pi].patchInternalField()
                  + (1.0 - pLambda)*vf.boundaryField()[pi].patchNeighbourField()
                );
        }
        else
        {
            psf = pSf & vf.boundaryField()[pi];
        }
    }

    tlambdas.clear();

//    tsf.ref().oriented() = Sf.oriented();

    return tsf;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::surfaceInterpolationScheme<Type>::interpolate
(
    const GeometricField<Type, fvPatchField, volMesh>& vf,
    const tmp<surfaceScalarField>& tlambdas
)
{
    return dotInterpolate(geometricOneField(), vf, tlambdas);
}


template<class Type>
Foam::tmp
<
    Foam::GeometricField
    <
        typename Foam::innerProduct<Foam::vector, Type>::type,
        Foam::fvsPatchField,
        Foam::surfaceMesh
    >
>
Foam::surfaceInterpolationScheme<Type>::dotInterpolate
(
    const surfaceVectorField& Sf,
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    if (surfaceInterpolation::debug)
    {
        InfoInFunction
            << "Interpolating "
            << vf.type() << " "
            << vf.name()
            << " from cells to faces"
            << endl;
    }

    tmp
    <
        GeometricField
        <
            typename Foam::innerProduct<Foam::vector, Type>::type,
            fvsPatchField,
            surfaceMesh
        >
    > tsf = dotInterpolate(Sf, vf, weights(vf));

    tsf.ref().oriented() = Sf.oriented();

    if (corrected())
    {
        tsf.ref() += Sf & correction(vf);
    }

    return tsf;
}


template<class Type>
Foam::tmp
<
    Foam::GeometricField
    <
        typename Foam::innerProduct<Foam::vector, Type>::type,
        Foam::fvsPatchField,
        Foam::surfaceMesh
    >
>
Foam::surfaceInterpolationScheme<Type>::dotInterpolate
(
    const surfaceVectorField& Sf,
    const tmp<GeometricField<Type, fvPatchField, volMesh>>& tvf
) const
{
    tmp
    <
        GeometricField
        <
            typename Foam::innerProduct<Foam::vector, Type>::type,
            fvsPatchField,
            surfaceMesh
        >
    > tSfDotinterpVf = dotInterpolate(Sf, tvf());

    tvf.clear();
    return tSfDotinterpVf;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::surfaceInterpolationScheme<Type>::interpolate
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    if (surfaceInterpolation::debug)
    {
        InfoInFunction
            << "Interpolating "
            << vf.type() << " "
            << vf.name()
            << " from cells to faces"
            << endl;
    }

    tmp<GeometricField<Type, fvsPatchField, surfaceMesh>> tsf
        = interpolate(vf, weights(vf));

    if (corrected())
    {
        tsf.ref() += correction(vf);
    }

    return tsf;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::surfaceInterpolationScheme<Type>::interpolate
(
    const tmp<GeometricField<Type, fvPatchField, volMesh>>& tvf
) const
{
    tmp<GeometricField<Type, fvsPatchField, surfaceMesh>> tinterpVf
        = interpolate(tvf());
    tvf.clear();
    return tinterpVf;
}


// ************************************************************************* //
