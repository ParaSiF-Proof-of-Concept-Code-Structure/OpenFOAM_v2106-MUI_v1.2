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

Class
    Foam::vtk::fileWriter

Description
    Base class for VTK output writers that handle geometry and fields
    (eg, vtp, vtu data).
    These output formats are structured as DECLARED, FIELD_DATA, PIECE
    followed by any CELL_DATA or POINT_DATA.

    This writer base tracks these expected output states internally
    to help avoid logic errors in the callers.

    The FieldData element must be placed prior to writing any geometry
    Piece. This moves the information to the front of the output file
    for visibility and simplifies the logic when creating
    multi-piece geometries.

SourceFiles
    foamVtkFileWriter.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_vtk_fileWriter_H
#define Foam_vtk_fileWriter_H

#include <fstream>
#include "Enum.H"
#include "UPstream.H"
#include "foamVtkOutputOptions.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace vtk
{

/*---------------------------------------------------------------------------*\
                       Class vtk::fileWriter Declaration
\*---------------------------------------------------------------------------*/

class fileWriter
{
protected:

    // Protected Member Data

        //- Internal tracking of the output state.
        enum outputState
        {
            CLOSED = 0,     //!< File is closed
            OPENED,         //!< File is opened
            DECLARED,       //!< File contents declared (VTKFile header written)
            FIELD_DATA,     //!< Inside FieldData
            PIECE,          //!< Inside Piece (after geometry write)
            CELL_DATA,      //!< Inside CellData
            POINT_DATA      //!< Inside PointData
        };

        //- Names for the output state (for messages, not for file output).
        static const Enum<outputState> stateNames;


        //- The content type
        vtk::fileTag contentType_;

        //- The requested output options
        outputOptions opts_;

        //- Writing in parallel (via master)
        bool parallel_;

        //- The output state
        outputState state_;

        //- The number of CellData written for the Piece thus far.
        label nCellData_;

        //- The number of PointData written for the Piece thus far.
        label nPointData_;

        //- The output file name
        fileName outputFile_;

        //- The VTK formatter in use (master process)
        autoPtr<vtk::formatter> format_;

        //- The backend ostream in use (master process)
        std::ofstream os_;


    // Protected Member Functions

        //- Generate message reporting bad writer state
        Ostream& reportBadState(Ostream&, outputState expected) const;

        //- Generate message reporting bad writer state
        Ostream& reportBadState(Ostream&, outputState, outputState) const;

        //- The backend ostream in use
        inline std::ofstream& os();

        //- The VTK formatter in use
        inline vtk::formatter& format();

        //- True if the output state corresponds to the test state.
        inline bool isState(outputState test) const;

        //- True if the output state does not correspond to the test state.
        inline bool notState(outputState test) const;

        //- Start of a field or DataArray output (legacy or non-legacy).
        template<class Type>
        void beginDataArray
        (
            const word& fieldName,
            const label nValues
        );

        //- Flush formatter and end of DataArray output (non-legacy)
        void endDataArray();

        //- Start of a POINTS DataArray
        void beginPoints(const label nPoints);

        //- End of a POINTS DataArray
        void endPoints();


        //- Write uniform field content.
        //  No context checking (eg, file-open, CellData, PointData, etc)
        template<class Type>
        void writeUniform
        (
            const word& fieldName,
            const Type& val,
            const label nValues
        );

        //- Write basic (primitive) field content
        //  No context checking (eg, file-open, CellData, PointData, etc)
        template<class Type>
        void writeBasicField
        (
            const word& fieldName,
            const UList<Type>& field
        );

        //- Trigger change state to Piece. Resets nCellData_, nPointData_.
        bool enter_Piece();

        //- Explicitly end Piece output and switch to DECLARED state
        //  Ignored (no-op) if not currently in the PIECE state.
        bool endPiece();

        //- Trigger change state to CellData.
        //  Legacy requires both parameters. XML doesn't require either.
        //
        //  \return True if the state changed
        bool enter_CellData(label nEntries, label nFields);

        //- Trigger change state to PointData
        //  Legacy requires both parameters. XML doesn't require either.
        //
        //  \return True if the state changed
        bool enter_PointData(label nEntries, label nFields);

        //- Emit file footer (end data, end piece, end file)
        bool exit_File();


        //- No copy construct
        fileWriter(const fileWriter&) = delete;

        //- No copy assignment
        void operator=(const fileWriter&) = delete;


public:

    // Constructors

        //- Construct from components
        fileWriter
        (
            const vtk::fileTag contentType,
            const vtk::outputOptions opts
        );


    //- Destructor
    virtual ~fileWriter();


    // Member Functions

        //- The content type
        inline vtk::fileTag contentType() const;

        //- The output options in use
        inline vtk::outputOptions opts() const;

        //- File extension for current format type.
        inline word ext() const;

        //- Commonly used query
        inline bool legacy() const;

        //- Parallel output requested?
        inline bool parallel() const noexcept;

        //- The output state in printable format
        inline const word& state() const;

        //- The current output file name
        inline const fileName& output() const noexcept;


        //- Open file for writing (creates parent directory).
        //  The file name is normally without an extension, this will be added
        //  according to the content-type and the output format (legacy/xml).
        //  If the file name has an extension, it will be used where if
        //  appropriate or changed to suit the format (legacy/xml) type.
        //  \note Expected calling states: (CLOSED).
        bool open(const fileName& file, bool parallel=Pstream::parRun());

        //- End the file contents and close the file after writing.
        //  \note Expected calling states: (PIECE | CELL_DATA | POINT_DATA).
        void close();


        //- Write file header (non-collective)
        //  \note Expected calling states: (OPENED)
        virtual bool beginFile(std::string title = "");

        //- Begin FieldData output section for specified number of fields.
        //  \param nFields is for legacy format only.
        //      When nFields=0, this a no-op for legacy format.
        //  \note Expected calling states: (OPENED | DECLARED).
        bool beginFieldData(label nFields = 0);

        //- Write mesh topology.
        //  Also writes the file header if not previously written.
        //  \note Must be called prior to writing CellData or PointData
        virtual bool writeGeometry() = 0;


        //- Begin CellData output section for specified number of fields.
        //  Must be called prior to writing any cell data fields.
        //  \param nFields is for legacy format only.
        //      When nFields=0, this a no-op for legacy format.
        //  \note Expected calling states: (PIECE | POINT_DATA).
        //
        //  \return True if the state changed
        virtual bool beginCellData(label nFields = 0) = 0;

        //- Begin PointData for specified number of fields.
        //  Must be called prior to writing any point data fields.
        //  \param nFields is for legacy format only.
        //      When nFields=0, this a no-op for legacy format.
        //  \note Expected calling states: (PIECE | CELL_DATA).
        //
        //  \return True if the state changed
        virtual bool beginPointData(label nFields = 0) = 0;

        //- Return the number of CellData written for the Piece thus far.
        inline label nCellData() const noexcept;

        //- Return the number of PointData written for the Piece thus far.
        inline label nPointData() const noexcept;


        //- Explicitly end FieldData output and switch to DECLARED state
        //  Ignored (no-op) if not currently in the FIELD_DATA state.
        bool endFieldData();

        //- Explicitly end CellData output and switch to PIECE state
        //  Ignored (no-op) if not currently in the CELL_DATA state.
        bool endCellData();

        //- Explicitly end PointData output and switch to PIECE state
        //  Ignored (no-op) if not currently in the POINT_DATA state.
        bool endPointData();

        //- Write "TimeValue" FieldData (name as per Catalyst output)
        //  Must be called within the FIELD_DATA state.
        //  \note As a convenience this can also be called from
        //      (OPENED | DECLARED) states, in which case it invokes
        //      beginFieldData(1) internally.
        void writeTimeValue(scalar timeValue);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace vtk
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "foamVtkFileWriterI.H"

#ifdef NoRepository
    #include "foamVtkFileWriterTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
