/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2015-2020 OpenCFD Ltd.
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

#include "sampledSets.H"
#include "volFields.H"
#include "globalIndex.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class Type>
Foam::sampledSets::volFieldSampler<Type>::volFieldSampler
(
    const word& interpolationScheme,
    const GeometricField<Type, fvPatchField, volMesh>& field,
    const PtrList<sampledSet>& samplers
)
:
    List<Field<Type>>(samplers.size()),
    name_(field.name())
{
    autoPtr<interpolation<Type>> interpolator
    (
        interpolation<Type>::New(interpolationScheme, field)
    );

    forAll(samplers, setI)
    {
        Field<Type>& values = this->operator[](setI);
        const sampledSet& samples = samplers[setI];

        values.setSize(samples.size());
        forAll(samples, sampleI)
        {
            const point& samplePt = samples[sampleI];
            label celli = samples.cells()[sampleI];
            label facei = samples.faces()[sampleI];

            if (celli == -1 && facei == -1)
            {
                // Special condition for illegal sampling points
                values[sampleI] = pTraits<Type>::max;
            }
            else
            {
                values[sampleI] = interpolator().interpolate
                (
                    samplePt,
                    celli,
                    facei
                );
            }
        }
    }
}


template<class Type>
Foam::sampledSets::volFieldSampler<Type>::volFieldSampler
(
    const GeometricField<Type, fvPatchField, volMesh>& field,
    const PtrList<sampledSet>& samplers
)
:
    List<Field<Type>>(samplers.size()),
    name_(field.name())
{
    forAll(samplers, setI)
    {
        Field<Type>& values = this->operator[](setI);
        const sampledSet& samples = samplers[setI];

        values.setSize(samples.size());
        forAll(samples, sampleI)
        {
            label celli = samples.cells()[sampleI];

            if (celli ==-1)
            {
                values[sampleI] = pTraits<Type>::max;
            }
            else
            {
                values[sampleI] = field[celli];
            }
        }
    }
}


template<class Type>
Foam::sampledSets::volFieldSampler<Type>::volFieldSampler
(
    const List<Field<Type>>& values,
    const word& name
)
:
    List<Field<Type>>(values),
    name_(name)
{}


template<class Type>
Foam::fileName Foam::sampledSets::writeSampleFile
(
    const coordSet& masterSampleSet,
    const PtrList<volFieldSampler<Type>>& masterFields,
    const label setI,
    const fileName& timeDir,
    const writer<Type>& formatter
)
{
    wordList valueSetNames(masterFields.size());
    List<const Field<Type>*> valueSets(masterFields.size());

    forAll(masterFields, fieldi)
    {
        valueSetNames[fieldi] = masterFields[fieldi].name();
        valueSets[fieldi] = &masterFields[fieldi][setI];
    }

    fileName fName
    (
        timeDir/formatter.getFileName(masterSampleSet, valueSetNames)
    );

    OFstream ofs(fName);
    if (ofs.opened())
    {
        formatter.write
        (
            masterSampleSet,
            valueSetNames,
            valueSets,
            ofs
        );
        return fName;
    }
    else
    {
        WarningInFunction
            << "File " << ofs.name() << " could not be opened. "
            << "No data will be written" << endl;
        return fileName::null;
    }
}


template<class T>
void Foam::sampledSets::combineSampledValues
(
    const PtrList<volFieldSampler<T>>& sampledFields,
    const labelListList& indexSets,
    PtrList<volFieldSampler<T>>& masterFields
)
{
    forAll(sampledFields, fieldi)
    {
        List<Field<T>> masterValues(indexSets.size());

        forAll(indexSets, setI)
        {
            // Collect data from all processors

            Field<T> allData;
            globalIndex::gatherOp(sampledFields[fieldi][setI], allData);

            if (Pstream::master())
            {
                masterValues[setI] = UIndirectList<T>
                (
                    allData,
                    indexSets[setI]
                )();
            }
        }

        masterFields.set
        (
            fieldi,
            new volFieldSampler<T>
            (
                masterValues,
                sampledFields[fieldi].name()
            )
        );
    }
}


template<class Type>
void Foam::sampledSets::sampleAndWrite(fieldGroup<Type>& fields)
{
    if (fields.size())
    {
        const bool interpolate = interpolationScheme_ != "cell";

        // Create or use existing writer
        if (!fields.formatter)
        {
            fields = writeFormat_;
        }

        // Storage for interpolated values
        PtrList<volFieldSampler<Type>> sampledFields(fields.size());

        forAll(fields, fieldi)
        {
            if (Pstream::master() && verbose_)
            {
                Pout<< "sampledSets::sampleAndWrite: "
                    << fields[fieldi] << endl;
            }

            if (loadFromFiles_)
            {
                GeometricField<Type, fvPatchField, volMesh> vf
                (
                    IOobject
                    (
                        fields[fieldi],
                        mesh_.time().timeName(),
                        mesh_,
                        IOobject::MUST_READ,
                        IOobject::NO_WRITE,
                        false
                    ),
                    mesh_
                );

                if (interpolate)
                {
                    sampledFields.set
                    (
                        fieldi,
                        new volFieldSampler<Type>
                        (
                            interpolationScheme_,
                            vf,
                            *this
                        )
                    );
                }
                else
                {
                    sampledFields.set
                    (
                        fieldi,
                        new volFieldSampler<Type>(vf, *this)
                    );
                }
            }
            else
            {
                if (interpolate)
                {
                    sampledFields.set
                    (
                        fieldi,
                        new volFieldSampler<Type>
                        (
                            interpolationScheme_,
                            mesh_.lookupObject
                            <GeometricField<Type, fvPatchField, volMesh>>
                            (fields[fieldi]),
                            *this
                        )
                    );
                }
                else
                {
                    sampledFields.set
                    (
                        fieldi,
                        new volFieldSampler<Type>
                        (
                            mesh_.lookupObject
                            <GeometricField<Type, fvPatchField, volMesh>>
                            (fields[fieldi]),
                            *this
                        )
                    );
                }
            }
        }

        // Combine sampled fields from processors.
        // Note: only master results are valid

        PtrList<volFieldSampler<Type>> masterFields(sampledFields.size());
        combineSampledValues(sampledFields, indexSets_, masterFields);

        forAll(masterSampledSets_, setI)
        {
            fileName sampleFile;
            if (Pstream::master())
            {
                sampleFile = writeSampleFile
                (
                    masterSampledSets_[setI],
                    masterFields,
                    setI,
                    outputPath_/mesh_.time().timeName(),
                    fields.formatter()
                );
            }

            Pstream::scatter(sampleFile);
            if (sampleFile.size())
            {
                // Case-local file name with "<case>" to make relocatable

                forAll(masterFields, fieldi)
                {
                    dictionary propsDict;
                    propsDict.add
                    (
                        "file",
                        time_.relativePath(sampleFile, true)
                    );

                    const word& fieldName = masterFields[fieldi].name();
                    setProperty(fieldName, propsDict);
                }
            }
        }
    }
}


// ************************************************************************* //
