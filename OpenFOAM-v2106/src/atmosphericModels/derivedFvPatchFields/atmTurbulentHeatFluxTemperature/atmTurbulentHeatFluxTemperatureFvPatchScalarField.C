/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2020 ENERCON GmbH
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

\*---------------------------------------------------------------------------*/

#include "atmTurbulentHeatFluxTemperatureFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "turbulenceModel.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

const Foam::Enum
<
    Foam::atmTurbulentHeatFluxTemperatureFvPatchScalarField::heatSourceType
>
Foam::atmTurbulentHeatFluxTemperatureFvPatchScalarField::heatSourceTypeNames
({
    { heatSourceType::POWER , "power" },
    { heatSourceType::FLUX , "flux" }
});


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

atmTurbulentHeatFluxTemperatureFvPatchScalarField::
atmTurbulentHeatFluxTemperatureFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedGradientFvPatchScalarField(p, iF),
    heatSource_(heatSourceType::POWER),
    alphaEffName_("undefinedAlphaEff"),
    Cp0_(db().time(), "Cp0"),
    q_(nullptr)
{}


atmTurbulentHeatFluxTemperatureFvPatchScalarField::
atmTurbulentHeatFluxTemperatureFvPatchScalarField
(
    const atmTurbulentHeatFluxTemperatureFvPatchScalarField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedGradientFvPatchScalarField(ptf, p, iF, mapper),
    heatSource_(ptf.heatSource_),
    alphaEffName_(ptf.alphaEffName_),
    Cp0_(ptf.Cp0_),
    q_(ptf.q_.clone(p.patch()))
{}


atmTurbulentHeatFluxTemperatureFvPatchScalarField::
atmTurbulentHeatFluxTemperatureFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    fixedGradientFvPatchScalarField(p, iF),
    heatSource_
    (
        heatSourceTypeNames.getOrDefault
        (
            "heatSource",
            dict,
            heatSourceType::POWER
        )
    ),
    alphaEffName_(dict.get<word>("alphaEff")),
    Cp0_(TimeFunction1<scalar>(db().time(), "Cp0", dict)),
    q_(PatchFunction1<scalar>::New(p.patch(), "q", dict))
{
    if (dict.found("value") && dict.found("gradient"))
    {
        fvPatchField<scalar>::operator =
            (
                Field<scalar>("value", dict, p.size())
            );
        gradient() = Field<scalar>("gradient", dict, p.size());
    }
    else
    {
        fvPatchField<scalar>::operator=(patchInternalField());
        gradient() = 0.0;
    }
}


atmTurbulentHeatFluxTemperatureFvPatchScalarField::
atmTurbulentHeatFluxTemperatureFvPatchScalarField
(
    const atmTurbulentHeatFluxTemperatureFvPatchScalarField& atmpsf
)
:
    fixedGradientFvPatchScalarField(atmpsf),
    heatSource_(atmpsf.heatSource_),
    alphaEffName_(atmpsf.alphaEffName_),
    Cp0_(atmpsf.Cp0_),
    q_(atmpsf.q_.clone(this->patch().patch()))
{}


atmTurbulentHeatFluxTemperatureFvPatchScalarField::
atmTurbulentHeatFluxTemperatureFvPatchScalarField
(
    const atmTurbulentHeatFluxTemperatureFvPatchScalarField& atmpsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedGradientFvPatchScalarField(atmpsf, iF),
    heatSource_(atmpsf.heatSource_),
    alphaEffName_(atmpsf.alphaEffName_),
    Cp0_(atmpsf.Cp0_),
    q_(atmpsf.q_.clone(this->patch().patch()))
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void atmTurbulentHeatFluxTemperatureFvPatchScalarField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    scalarField::autoMap(m);
    q_->autoMap(m);
}


void atmTurbulentHeatFluxTemperatureFvPatchScalarField::rmap
(
    const fvPatchScalarField& ptf,
    const labelList& addr
)
{
    fixedGradientFvPatchScalarField::rmap(ptf, addr);

    const atmTurbulentHeatFluxTemperatureFvPatchScalarField& atmptf =
        refCast<const atmTurbulentHeatFluxTemperatureFvPatchScalarField>(ptf);

    q_->rmap(atmptf.q_(), addr);
}


void atmTurbulentHeatFluxTemperatureFvPatchScalarField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    const scalarField& alphaEffp =
        patch().lookupPatchField<volScalarField, scalar>(alphaEffName_);

    const scalar t = db().time().timeOutputValue();
    const scalar Cp0 = Cp0_.value(t);

    if (Cp0 < SMALL)
    {
        FatalErrorInFunction
            << "Cp0 = " << Cp0 << " must be positive."
            << exit(FatalError);
    }

    const scalarField q(q_->value(t));

    switch (heatSource_)
    {
        case heatSourceType::POWER:
        {
            const scalar Ap = gSum(patch().magSf());
            gradient() = q/(Ap*Cp0*alphaEffp + SMALL);
            break;
        }

        case heatSourceType::FLUX:
        {
            gradient() = q/(Cp0*alphaEffp + SMALL);
            break;
        }

        default:
        {
            FatalErrorInFunction
                << "Unknown heat source type. Valid types are: "
                << heatSourceTypeNames << nl
                << exit(FatalError);
        }
    }

    fixedGradientFvPatchScalarField::updateCoeffs();
}


void atmTurbulentHeatFluxTemperatureFvPatchScalarField::write(Ostream& os) const
{
    fixedGradientFvPatchScalarField::write(os);
    os.writeEntry("heatSource", heatSourceTypeNames[heatSource_]);
    os.writeEntry("alphaEff", alphaEffName_);
    Cp0_.writeData(os);
    q_->writeData(os);
    writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makePatchTypeField
(
    fvPatchScalarField,
    atmTurbulentHeatFluxTemperatureFvPatchScalarField
);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
