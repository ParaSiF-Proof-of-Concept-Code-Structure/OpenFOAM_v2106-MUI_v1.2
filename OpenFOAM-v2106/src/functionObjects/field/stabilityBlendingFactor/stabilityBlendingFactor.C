/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018-2021 OpenCFD Ltd.
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

#include "stabilityBlendingFactor.H"
#include "addToRunTimeSelectionTable.H"
#include "zeroGradientFvPatchFields.H"
#include "fvcGrad.H"
#include "surfaceInterpolate.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(stabilityBlendingFactor, 0);
    addToRunTimeSelectionTable
    (
        functionObject,
        stabilityBlendingFactor,
        dictionary
    );
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::functionObjects::stabilityBlendingFactor::calcStats
(
    label& nCellsScheme1,
    label& nCellsScheme2,
    label& nCellsBlended
) const
{
    forAll(indicator_, celli)
    {
        scalar i = indicator_[celli];

        if (i < tolerance_)
        {
            nCellsScheme2++;
        }
        else if (i > (1 - tolerance_))
        {
            nCellsScheme1++;
        }
        else
        {
            nCellsBlended++;
        }
    }

    reduce(nCellsScheme1, sumOp<label>());
    reduce(nCellsScheme2, sumOp<label>());
    reduce(nCellsBlended, sumOp<label>());
}


void Foam::functionObjects::stabilityBlendingFactor::writeFileHeader
(
    Ostream& os
) const
{
    writeHeader(os, "Stabilization blending factor");
    writeCommented(os, "Time");
    writeTabbed(os, "Scheme1");
    writeTabbed(os, "Scheme2");
    writeTabbed(os, "Blended");
    os  << endl;
}


bool Foam::functionObjects::stabilityBlendingFactor::calc()
{
    init(false);
    return true;
}


bool Foam::functionObjects::stabilityBlendingFactor::init(bool first)
{
    const auto* residualPtr = mesh_.findObject<IOField<scalar>>(residualName_);

    if (residuals_)
    {
        if (!residualPtr)
        {
             WarningInFunction
                << "Could not find residual field : " << residualName_
                << ". The residual mode won't be " << nl
                << "    considered for the blended field in the stability "
                << "blending factor. " << nl
                << "    Please add the corresponding 'solverInfo'"
                << " function object with 'writeResidualFields true'." << nl
                << "    If the solverInfo function object is already enabled "
                << "you might need to wait " << nl
                << "    for the first iteration."
                << nl << endl;
        }
        else
        {
            scalar meanRes = gAverage(mag(*residualPtr)) + VSMALL;

            Log << nl << name() << " : " << nl
                << "    Average(mag(residuals)) :  " << meanRes << endl;

            oldError_ = error_;
            oldErrorIntegral_ = errorIntegral_;
            error_ = mag(meanRes - mag(*residualPtr));
            errorIntegral_ = oldErrorIntegral_ + 0.5*(error_ + oldError_);
            const scalarField errorDifferential(error_ - oldError_);

            const scalarField factorList
            (
                + P_*error_
                + I_*errorIntegral_
                + D_*errorDifferential
            );

            const scalarField indicatorResidual
            (
                max
                (
                    min
                    (
                        mag(factorList - meanRes)/(maxResidual_*meanRes),
                        scalar(1)
                    ),
                    scalar(0)
                )
            );

            forAll(indicator_, i)
            {
                indicator_[i] = indicatorResidual[i];
            }
        }
    }

    const volScalarField* nonOrthPtr =
        mesh_.findObject<volScalarField>(nonOrthogonalityName_);

    if (nonOrthogonality_)
    {
        if (maxNonOrthogonality_ >= minNonOrthogonality_)
        {
            FatalErrorInFunction
                << "    minNonOrthogonality should be larger than "
                << "maxNonOrthogonality."
                << exit(FatalError);
        }

        indicator_ =
        max
        (
            indicator_,
            min
            (
                max
                (
                    scalar(0),
                    (*nonOrthPtr - maxNonOrthogonality_)
                  / (minNonOrthogonality_ - maxNonOrthogonality_)
                ),
                scalar(1)
            )
        );

        if (first)
        {
            Log << "    Max non-orthogonality :  " << max(*nonOrthPtr).value()
                << endl;
        }
    }

    const volScalarField* skewnessPtr =
        mesh_.findObject<volScalarField>(skewnessName_);

    if (skewness_)
    {
        if (maxSkewness_ >= minSkewness_)
        {
            FatalErrorInFunction
                << "    minSkewness should be larger than maxSkewness."
                << exit(FatalError);
        }

        indicator_ =
        max
        (
            indicator_,
            min
            (
                max
                (
                    scalar(0),
                    (*skewnessPtr - maxSkewness_)
                  / (minSkewness_ - maxSkewness_)
                ),
                scalar(1)
            )
        );

        if (first)
        {
            Log << "    Max skewness :  " << max(*skewnessPtr).value()
                << endl;
        }
    }

    const volScalarField* faceWeightsPtr =
        mesh_.findObject<volScalarField>(faceWeightName_);

    if (faceWeight_)
    {
        if (maxFaceWeight_ >= minFaceWeight_)
        {
            FatalErrorInFunction
                << "    minFaceWeight should be larger than maxFaceWeight."
                << exit(FatalError);
        }

        indicator_ =
            max
            (
                indicator_,
                min
                (
                    max
                    (
                        scalar(0),
                        (minFaceWeight_ - *faceWeightsPtr)
                      / (minFaceWeight_ - maxFaceWeight_)
                    ),
                    scalar(1)
                )
            );

        if (first)
        {
            Log << "    Min face weight:  " << min(*faceWeightsPtr).value()
                << endl;
        }
    }


    if (gradCc_)
    {
        if (maxGradCc_ >= minGradCc_)
        {
            FatalErrorInFunction
                << "    minGradCc should be larger than maxGradCc."
                << exit(FatalError);
        }

        auto tmagGradCC = tmp<volScalarField>::New
        (
            IOobject
            (
                "magGradCC",
                time_.timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh_,
            dimensionedScalar(dimless, Zero),
            zeroGradientFvPatchScalarField::typeName
        );
        auto& magGradCC = tmagGradCC.ref();

        for (direction i=0; i<vector::nComponents; i++)
        {
            // Create field with zero grad
            volScalarField cci
            (
                IOobject
                (
                    "cc" + word(vector::componentNames[i]),
                    mesh_.time().timeName(),
                    mesh_,
                    IOobject::NO_READ,
                    IOobject::NO_WRITE
                ),
                mesh_,
                dimensionedScalar(dimLength, Zero),
                zeroGradientFvPatchScalarField::typeName
            );
            cci = mesh_.C().component(i);
            cci.correctBoundaryConditions();
            magGradCC += mag(fvc::grad(cci)).ref();
        }

        if (first)
        {
            Log << "    Max magGradCc :  " << max(magGradCC.ref()).value()
                << endl;
        }

        indicator_ =
            max
            (
                indicator_,
                min
                (
                    max
                    (
                        scalar(0),
                        (magGradCC - maxGradCc_)
                      / (minGradCc_ - maxGradCc_)
                    ),
                    scalar(1)
                )
            );
    }


    const volVectorField* UNamePtr =
        mesh_.findObject<volVectorField>(UName_);

    if (Co_)
    {
        if (Co1_ >= Co2_)
        {
            FatalErrorInFunction
                << "    Co2 should be larger than Co1."
                << exit(FatalError);
        }

        auto CoPtr = tmp<volScalarField>::New
        (
            IOobject
            (
                "Co",
                time_.timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh_,
            dimensionedScalar(dimless, Zero),
            zeroGradientFvPatchScalarField::typeName
        );

        auto& Co = CoPtr.ref();

        Co.primitiveFieldRef() =
            mesh_.time().deltaT()*mag(*UNamePtr)/cbrt(mesh_.V());

        indicator_ =
            max
            (
                indicator_,
                min(max(scalar(0), (Co - Co1_)/(Co2_ - Co1_)), scalar(1))
            );

        if (first)
        {
            Log << "    Max Co :  " << max(Co).value()
                << endl;
        }
    }

    if (first)
    {
        Log << nl;
    }

    if (log)
    {
        label nCellsScheme1 = 0;
        label nCellsScheme2 = 0;
        label nCellsBlended = 0;

        calcStats(nCellsScheme1, nCellsScheme2, nCellsBlended);

        Log << nl << name() << " execute :" << nl
        << "    scheme 1 cells :  " << nCellsScheme1 << nl
        << "    scheme 2 cells :  " << nCellsScheme2 << nl
        << "    blended cells  :  " << nCellsBlended << nl
        << endl;
    }

    indicator_.correctBoundaryConditions();
    indicator_.min(1.0);
    indicator_.max(0.0);

    // Update the blended surface field
    surfaceScalarField& surBlended =
        mesh_.lookupObjectRef<surfaceScalarField>(resultName_);

    surBlended = fvc::interpolate(indicator_);

    return true;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::stabilityBlendingFactor::stabilityBlendingFactor
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    fieldExpression(name, runTime, dict),
    writeFile(obr_, name, typeName, dict),
    indicator_
    (
        IOobject
        (
            "blendedIndicator" + fieldName_,
            time_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar(dimless, Zero),
        zeroGradientFvPatchScalarField::typeName
    ),
    nonOrthogonality_(dict.getOrDefault<Switch>("switchNonOrtho", false)),
    gradCc_(dict.getOrDefault<Switch>("switchGradCc", false)),
    residuals_(dict.getOrDefault<Switch>("switchResiduals", false)),
    faceWeight_(dict.getOrDefault<Switch>("switchFaceWeight", false)),
    skewness_(dict.getOrDefault<Switch>("switchSkewness", false)),
    Co_(dict.getOrDefault<Switch>("switchCo", false)),

    maxNonOrthogonality_
    (
        dict.getOrDefault<scalar>("maxNonOrthogonality", 20.0)
    ),
    minNonOrthogonality_
    (
        dict.getOrDefault<scalar>("minNonOrthogonality", 60.0)
    ),
    maxGradCc_(dict.getOrDefault<scalar>("maxGradCc", 3.0)),
    minGradCc_(dict.getOrDefault<scalar>("minGradCc", 4.0)),
    maxResidual_(dict.getOrDefault<scalar>("maxResidual", 10.0)),
    minFaceWeight_(dict.getOrDefault<scalar>("minFaceWeight", 0.3)),
    maxFaceWeight_(dict.getOrDefault<scalar>("maxFaceWeight", 0.2)),
    maxSkewness_(dict.getOrDefault<scalar>("maxSkewness", 2.0)),
    minSkewness_(dict.getOrDefault<scalar>("minSkewness", 3.0)),
    Co1_(dict.getOrDefault<scalar>("Co1", 1.0)),
    Co2_(dict.getOrDefault<scalar>("Co2", 10.0)),

    nonOrthogonalityName_
    (
        dict.getOrDefault<word>("nonOrthogonality", "nonOrthoAngle")
    ),
    faceWeightName_
    (
        dict.getOrDefault<word>("faceWeight", "faceWeight")
    ),
    skewnessName_
    (
        dict.getOrDefault<word>("skewness", "skewness")
    ),
    residualName_
    (
        dict.getOrDefault<word>("residual", "initialResidual:p")
    ),
    UName_
    (
         dict.getOrDefault<word>("U", "U")
    ),

    tolerance_(0.001),
    error_(mesh_.nCells(), Zero),
    errorIntegral_(mesh_.nCells(), Zero),
    oldError_(mesh_.nCells(), Zero),
    oldErrorIntegral_(mesh_.nCells(), Zero),
    P_(dict.getOrDefault<scalar>("P", 3)),
    I_(dict.getOrDefault<scalar>("I", 0.0)),
    D_(dict.getOrDefault<scalar>("D", 0.25))
{
    read(dict);
    setResultName(typeName, "");

    auto faceBlendedPtr = tmp<surfaceScalarField>::New
    (
        IOobject
        (
            resultName_,
            time_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar(dimless, Zero)
    );
    store(resultName_, faceBlendedPtr);

    const volScalarField* nonOrthPtr =
        mesh_.findObject<volScalarField>(nonOrthogonalityName_);

    if (nonOrthogonality_)
    {
        if (!nonOrthPtr)
        {
            IOobject fieldHeader
            (
                nonOrthogonalityName_,
                mesh_.time().constant(),
                mesh_,
                IOobject::MUST_READ,
                IOobject::NO_WRITE
            );

            if (fieldHeader.typeHeaderOk<volScalarField>(true, true, false))
            {
                volScalarField* vfPtr(new volScalarField(fieldHeader, mesh_));
                mesh_.objectRegistry::store(vfPtr);
            }
            else
            {
                FatalErrorInFunction
                    << "    Field : " << nonOrthogonalityName_ << " not found."
                    << " The function object will not be used"
                    << exit(FatalError);
            }
        }
    }


    const volScalarField* faceWeightsPtr =
        mesh_.findObject<volScalarField>(faceWeightName_);

    if (faceWeight_)
    {
        if (!faceWeightsPtr)
        {
            IOobject fieldHeader
            (
                faceWeightName_,
                mesh_.time().constant(),
                mesh_,
                IOobject::MUST_READ,
                IOobject::NO_WRITE
            );

            if (fieldHeader.typeHeaderOk<volScalarField>(true, true, false))
            {
                volScalarField* vfPtr(new volScalarField(fieldHeader, mesh_));
                mesh_.objectRegistry::store(vfPtr);
            }
            else
            {
                FatalErrorInFunction
                    << "    Field : " << faceWeightName_ << " not found."
                    << " The function object will not be used"
                    << exit(FatalError);
            }
        }
    }

    const volScalarField* skewnessPtr =
        mesh_.findObject<volScalarField>(skewnessName_);

    if (skewness_)
    {
        if (!skewnessPtr)
        {
            IOobject fieldHeader
            (
                skewnessName_,
                mesh_.time().constant(),
                mesh_,
                IOobject::MUST_READ,
                IOobject::NO_WRITE
            );

            if (fieldHeader.typeHeaderOk<volScalarField>(true, true, false))
            {
                volScalarField* vfPtr(new volScalarField(fieldHeader, mesh_));
                mesh_.objectRegistry::store(vfPtr);
            }
            else
            {
                FatalErrorInFunction
                    << "    Field : " << skewnessName_ << " not found."
                    << " The function object will not be used"
                    << exit(FatalError);
            }
        }
    }

    if (log)
    {
        indicator_.writeOpt(IOobject::AUTO_WRITE);

    }

    if (writeToFile_)
    {
        writeFileHeader(file());
    }

    init(true);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::stabilityBlendingFactor::read
(
    const dictionary& dict
)
{
    if (fieldExpression::read(dict) && writeFile::read(dict))
    {
        dict.readEntry("switchNonOrtho", nonOrthogonality_);
        dict.readEntry("switchGradCc", gradCc_);
        dict.readEntry("switchResiduals", residuals_);
        dict.readEntry("switchFaceWeight", faceWeight_);
        dict.readEntry("switchSkewness", skewness_);
        dict.readEntry("switchCo", Co_);

        dict.readIfPresent("maxNonOrthogonality", maxNonOrthogonality_);
        dict.readIfPresent("maxGradCc", maxGradCc_);
        dict.readIfPresent("maxResidual", maxResidual_);
        dict.readIfPresent("maxSkewness", maxSkewness_);
        dict.readIfPresent("maxFaceWeight", maxFaceWeight_);
        dict.readIfPresent("Co2", Co2_);

        dict.readIfPresent("minFaceWeight", minFaceWeight_);
        dict.readIfPresent("minNonOrthogonality", minNonOrthogonality_);
        dict.readIfPresent("minGradCc", minGradCc_);
        dict.readIfPresent("minSkewness", minSkewness_);
        dict.readIfPresent("Co1", Co1_);


        dict.readIfPresent("P", P_);
        dict.readIfPresent("I", I_);
        dict.readIfPresent("D", D_);

        tolerance_ = 0.001;
        if
        (
            dict.readIfPresent("tolerance", tolerance_)
         && (tolerance_ < 0 || tolerance_ > 1)
        )
        {
            FatalErrorInFunction
                << "tolerance must be in the range 0 to 1.  Supplied value: "
                << tolerance_ << exit(FatalError);
        }

        Info<< type() << " " << name() << ":" << nl;
        if (nonOrthogonality_)
        {
            Info<< "    Including nonOrthogonality between: "
                << minNonOrthogonality_ <<  " and " << maxNonOrthogonality_
                << endl;
        }
        if (gradCc_)
        {
            Info<< "    Including gradient between: "
                << minGradCc_ << " and " << maxGradCc_ << endl;
        }
        if (residuals_)
        {
            Info<< "    Including residuals" << endl;
        }
        if (faceWeight_)
        {
            Info<< "    Including faceWeight between: "
                << minFaceWeight_ << " and " << maxFaceWeight_ << endl;
        }
        if (skewness_)
        {
            Info<< "    Including skewness between: "
                << minSkewness_ << " and " << maxSkewness_ << endl;
        }
        if (Co_)
        {
            Info<< "    Including Co between: "
                << Co2_ << " and " << Co1_ << endl;
        }

        return true;
    }

    return false;
}


bool Foam::functionObjects::stabilityBlendingFactor::write()
{
    label nCellsScheme1 = 0;
    label nCellsScheme2 = 0;
    label nCellsBlended = 0;

    calcStats(nCellsScheme1, nCellsScheme2, nCellsBlended);

    if (writeToFile_)
    {
        writeCurrentTime(file());

        file()
            << tab << nCellsScheme1
            << tab << nCellsScheme2
            << tab << nCellsBlended
            << endl;
    }

    return true;
}


// ************************************************************************* //
