/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

#include "vtkSurfaceWriter.H"
#include "foamVtkSurfaceWriter.H"
#include "surfaceWriterMethods.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace surfaceWriters
{
    defineTypeName(vtkWriter);
    addToRunTimeSelectionTable(surfaceWriter, vtkWriter, word);
    addToRunTimeSelectionTable(surfaceWriter, vtkWriter, wordDict);

    // Accept vtp ending as well
    addNamedToRunTimeSelectionTable
    (
        surfaceWriter,
        vtkWriter,
        word,
        vtp
    );
    addNamedToRunTimeSelectionTable
    (
        surfaceWriter,
        vtkWriter,
        wordDict,
        vtp
    );
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::surfaceWriters::vtkWriter::vtkWriter()
:
    surfaceWriter(),
    fmtType_(static_cast<unsigned>(vtk::formatType::INLINE_BASE64)),
    precision_(IOstream::defaultPrecision()),
    fieldScale_(),
    writer_(nullptr)
{}


Foam::surfaceWriters::vtkWriter::vtkWriter
(
    const vtk::outputOptions& opts
)
:
    surfaceWriter(),
    fmtType_(static_cast<unsigned>(opts.fmt())),
    precision_(opts.precision()),
    fieldScale_(),
    writer_(nullptr)
{}


Foam::surfaceWriters::vtkWriter::vtkWriter
(
    const dictionary& options
)
:
    surfaceWriter(options),
    fmtType_(static_cast<unsigned>(vtk::formatType::INLINE_BASE64)),
    precision_
    (
        options.getOrDefault("precision", IOstream::defaultPrecision())
    ),
    fieldScale_(options.subOrEmptyDict("fieldScale")),
    writer_(nullptr)
{
    // format: ascii | binary
    // legacy: true | false

    vtk::outputOptions opts(vtk::formatType::INLINE_BASE64);

    opts.ascii
    (
        IOstream::ASCII
     == IOstream::formatEnum("format", options, IOstream::BINARY)
    );

    opts.legacy(options.getOrDefault("legacy", false));

    // Convert back to raw data type
    fmtType_ = static_cast<unsigned>(opts.fmt());
}


Foam::surfaceWriters::vtkWriter::vtkWriter
(
    const meshedSurf& surf,
    const fileName& outputPath,
    bool parallel,
    const dictionary& options
)
:
    vtkWriter(options)
{
    open(surf, outputPath, parallel);
}


Foam::surfaceWriters::vtkWriter::vtkWriter
(
    const pointField& points,
    const faceList& faces,
    const fileName& outputPath,
    bool parallel,
    const dictionary& options
)
:
    vtkWriter(options)
{
    open(points, faces, outputPath, parallel);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::surfaceWriters::vtkWriter::~vtkWriter()
{
    close();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::surfaceWriters::vtkWriter::close()
{
    writer_.clear();
    surfaceWriter::close();
}


void Foam::surfaceWriters::vtkWriter::beginTime(const Time& t)
{
    writer_.clear();
    surfaceWriter::beginTime(t);
}


void Foam::surfaceWriters::vtkWriter::beginTime(const instant& inst)
{
    writer_.clear();
    surfaceWriter::beginTime(inst);
}


void Foam::surfaceWriters::vtkWriter::endTime()
{
    writer_.clear();
    surfaceWriter::endTime();
}


Foam::fileName Foam::surfaceWriters::vtkWriter::write()
{
    checkOpen();

    if (needsUpdate())
    {
        writer_.clear();
    }
    merge();

    // From raw unsigned values to vtk::outputOptions
    vtk::outputOptions opts(static_cast<vtk::formatType>(fmtType_), precision_);


    // Geometry:  rootdir/<TIME>/surfaceName.{vtk|vtp}

    fileName outputFile = outputPath_;
    if (useTimeDir() && !timeName().empty())
    {
        // Splice in time-directory
        outputFile = outputPath_.path() / timeName() / outputPath_.name();
    }
    outputFile.ext(vtk::surfaceWriter::ext(opts));

    if (verbose_)
    {
        Info<< "Writing geometry to " << outputFile << endl;
    }

    const meshedSurf& surf = surface();

    if (!writer_ && (Pstream::master() || !parallel_))
    {
        writer_.reset
        (
            new vtk::surfaceWriter
            (
                surf.points(),
                surf.faces(),
                opts,
                outputFile,
                false  // serial!
            )
        );

        if (this->hasTime())
        {
            // Time name in title
            writer_->setTime(currTime_);
            writer_->writeTimeValue();
        }
        else
        {
            // Surface name in title
            writer_->beginFile(outputPath_.nameLessExt());
        }

        writer_->writeGeometry();
    }

    wroteGeom_ = true;
    return outputFile;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
Foam::fileName Foam::surfaceWriters::vtkWriter::writeTemplate
(
    const word& fieldName,
    const Field<Type>& localValues
)
{
    // Field:  rootdir/<TIME>/surfaceName.{vtk|vtp}

    // Open file, writing geometry (if required)
    fileName outputFile = this->write();


    // Output scaling for the variable, but not for integer types.
    // could also solve with clever templating

    const scalar varScale =
    (
        std::is_integral<Type>::value
      ? scalar(1)
      : fieldScale_.getOrDefault<scalar>(fieldName, 1)
    );

    if (verbose_)
    {
        Info<< "Writing field " << fieldName;
        if (!equal(varScale, 1))
        {
            Info<< " (scaling " << varScale << ')';
        }
        Info<< " to " << outputFile << endl;
    }


    // Implicit geometry merge()
    tmp<Field<Type>> tfield = mergeField(localValues) * varScale;

    if (Pstream::master() || !parallel_)
    {
        if (!nFields_ && writer_->legacy())
        {
            // Emit error message, but attempt to recover anyhow
            nFields_ = 1;

            FatalErrorInFunction
                << "Using VTK legacy format, but did not define nFields!"
                << nl
                << "Assuming nFields=1 (may be incorrect) and continuing..."
                << nl
                << "    Field " << fieldName << " to " << outputFile << nl;

            Info<< FatalError;
            Info<< endl;
        }

        if (this->isPointData())
        {
            writer_->beginPointData(nFields_);
        }
        else
        {
            writer_->beginCellData(nFields_);
        }

        writer_->write(fieldName, tfield());
    }

    wroteGeom_ = true;
    return outputFile;
}


// Field writing methods
defineSurfaceWriterWriteFields(Foam::surfaceWriters::vtkWriter);


// ************************************************************************* //
