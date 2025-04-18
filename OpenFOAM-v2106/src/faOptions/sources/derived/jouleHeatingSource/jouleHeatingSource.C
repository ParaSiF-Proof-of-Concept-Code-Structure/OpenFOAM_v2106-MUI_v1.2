/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

\*---------------------------------------------------------------------------*/

#include "jouleHeatingSource.H"
#include "fam.H"
#include "faScalarMatrix.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

namespace Foam
{
namespace fa
{
    defineTypeNameAndDebug(jouleHeatingSource, 0);
    addToRunTimeSelectionTable(option, jouleHeatingSource, dictionary);
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fa::jouleHeatingSource::jouleHeatingSource
(
    const word& sourceName,
    const word& modelType,
    const dictionary& dict,
    const fvPatch& patch
)
:
    faceSetOption(sourceName, modelType, dict, patch),
    TName_(dict.getOrDefault<word>("T", "T")),
    V_
    (
        IOobject
        (
            typeName + ":V_" + regionName_,
            mesh().time().timeName(),
            mesh(),
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE
        ),
        regionMesh()
    ),
    scalarSigmaVsTPtr_(nullptr),
    tensorSigmaVsTPtr_(nullptr),
    curTimeIndex_(-1),
    nIter_(1),
    anisotropicElectricalConductivity_(false)
{
    fieldNames_.resize(1, TName_);

    fa::option::resetApplied();

    if (anisotropicElectricalConductivity_)
    {
        Info<< "    Using tensor electrical conductivity" << endl;

        initialiseSigma(coeffs_, tensorSigmaVsTPtr_);
    }
    else
    {
        Info<< "    Using scalar electrical conductivity" << endl;

        initialiseSigma(coeffs_, scalarSigmaVsTPtr_);
    }

    read(dict);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::fa::jouleHeatingSource::addSup
(
    const areaScalarField& h,
    const areaScalarField& rho,
    faMatrix<scalar>& eqn,
    const label fieldi
)
{
    if (isActive())
    {
        DebugInfo<< name() << ": applying source to " << eqn.psi().name()
                 << endl;

        if (curTimeIndex_ != mesh().time().timeIndex())
        {
            for (label i = 0; i < nIter_; ++i)
            {
                if (anisotropicElectricalConductivity_)
                {
                    // Update sigma as a function of T if required
                    const areaTensorField& sigma =
                        updateSigma(tensorSigmaVsTPtr_);

                    // Solve the electrical potential equation
                    faScalarMatrix VEqn(fam::laplacian(h*sigma, V_));
                    VEqn.relax();
                    VEqn.solve();
                }
                else
                {
                    // Update sigma as a function of T if required
                    const areaScalarField& sigma =
                        updateSigma(scalarSigmaVsTPtr_);

                    // Solve the electrical potential equation
                    faScalarMatrix VEqn(fam::laplacian(h*sigma, V_));
                    VEqn.relax();
                    VEqn.solve();
                }
            }

            curTimeIndex_ = mesh().time().timeIndex();
        }

        // Add the Joule heating contribution
        areaVectorField gradV("gradV", fac::grad(V_));

        if (anisotropicElectricalConductivity_)
        {
            const auto& sigma =
                mesh_.lookupObject<areaTensorField>
                (
                    typeName + ":sigma_" + regionName_
                );

            eqn += (h*sigma & gradV) & gradV;
        }
        else
        {
            const auto& sigma =
                mesh_.lookupObject<areaScalarField>
                (
                    typeName + ":sigma_" + regionName_
                );

            eqn += (h*sigma*gradV) & gradV;

            if (mesh().time().outputTime() && debug)
            {
                areaScalarField qgradV("gradVSource", (gradV & gradV));
                qgradV.write();
            }
        }
    }
}


bool Foam::fa::jouleHeatingSource::read(const dictionary& dict)
{
    if (option::read(dict))
    {
        dict.readIfPresent("T", TName_);

        dict.readIfPresent("nIter", nIter_);

        anisotropicElectricalConductivity_ =
            dict.get<bool>("anisotropicElectricalConductivity");

        return true;
    }

    return false;
}


// ************************************************************************* //
