/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Class
    Foam::noiseModel

Description
    Base class for noise models.

    Data is read from a dictionary, e.g.

    \verbatim
    rhoRef          1;
    N               4096;
    fl              25;
    fu              10000;
    startTime       0;

    outputPrefix    "test1";

    SPLweighting    dBA;

    // Optional write options dictionary
    writeOptions
    {
        writePrmsf  no;
        writeSPL    yes;
        writePSD    yes;
        writePSDf   no;
        writeOctaves yes;
    }
    \endverbatim

    where
    \table
        Property     | Description                   | Required  | Default value
        rhoRef       | Reference density             | no        | 1
        N            | Number of samples in sampling window | no | 65536 (2^16)
        fl           | Lower frequency bounds        | no        | 25
        fu           | Upper frequency bounds        | no        | 10000
        startTime    | Start time                    | no        | 0
        outputPrefix | Prefix applied to output files| no        | ''
        SPLweighting | Weighting: dBA, dBB, dBC, DBD | no        | none
        graphFormat  | Graph format                  | no        | raw
        writePrmsf   | Write Prmsf data              | no        | yes
        writeSPL     | Write SPL data                | no        | yes
        writePSD     | Write PSD data                | no        | yes
        writePSDf    | Write PSDf data               | no        | yes
        writeOctaves | Write octaves data            | no        | yes
    \endtable

SourceFiles
    noiseModel.C

\*---------------------------------------------------------------------------*/

#ifndef noiseModel_H
#define noiseModel_H

#include "dictionary.H"
#include "scalarList.H"
#include "instantList.H"
#include "windowModel.H"
#include "Enum.H"
#include "runTimeSelectionTables.H"
#include "graph.H"
#include <fftw3.h>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                         Class noiseModel Declaration
\*---------------------------------------------------------------------------*/

class noiseModel
{
public:

    enum class weightingType
    {
        none,
        dBA,
        dBB,
        dBC,
        dBD
    };

    static const Enum<weightingType> weightingTypeNames_;

    //- FFTW planner information
    //  Note: storage uses double for use directly with FFTW
    struct planInfo
    {
        bool active;
        label windowSize;
        List<double> in;
        List<double> out;
        fftw_plan plan;
    };

    //- Octave band information
    struct octaveBandInfo
    {
        label octave;

        // IDs of bin boundaries in pressure data
        labelList binIDs;

        // Centre frequencies for each bin
        scalarField centreFreq;
    };


private:

    // Private Member Functions

        //- No copy construct
        noiseModel(const noiseModel&) = delete;

        //- No copy assignment
        void operator=(const noiseModel&) = delete;


protected:

    // Protected Data

        //- Copy of dictionary used for construction
        const dictionary dict_;

        //- Reference density (to convert from kinematic to static pressure)
        scalar rhoRef_;

        //- Number of samples in sampling window, default = 2^16
        label nSamples_;

        //- Lower frequency limit, default = 25Hz
        scalar fLower_;

        //- Upper frequency limit, default = 10kHz
        scalar fUpper_;

        //- Start time, default = 0s
        scalar startTime_;

        //- Window model
        autoPtr<windowModel> windowModelPtr_;

        //- Graph format
        word graphFormat_;

        //- Weighting
        weightingType SPLweighting_;


        // Data validation

            //- Min pressure value
            scalar minPressure_;

            //- Min pressure value
            scalar maxPressure_;


        // Write options

            //- Output  file prefix, default = ''
            fileName outputPrefix_;

            //- Write Prmsf; default = yes
            bool writePrmsf_;

            //- Write SPL; default = yes
            bool writeSPL_;

            //- Write PSD; default = yes
            bool writePSD_;

            //- Write PSDf; default = yes
            bool writePSDf_;

            //- Write writeOctaves; default = yes
            bool writeOctaves_;


        // FFT

            //- Plan information for FFTW
            mutable planInfo planInfo_;


    // Protected Member Functions

        //- Helper function to read write options and provide info feedback
        void readWriteOption
        (
            const dictionary& dict,
            const word& lookup,
            bool& option
        ) const;

        //- Check and return uniform time step
        scalar checkUniformTimeStep
        (
            const scalarList& times
        ) const;

        //- Return true if all pressure data is within min/max bounds
        bool validateBounds(const scalarList& p) const;

        //- Find and return start time index
        label findStartTimeIndex
        (
            const instantList& allTimes,
            const scalar startTime
        ) const;

        //- Return the base output directory
        fileName baseFileDir(const label dataseti) const;

        //- Create a field of equally spaced frequencies for the current set of
        //- data - assumes a constant time step
        tmp<scalarField> uniformFrequencies
        (
            const scalar deltaT,
            const bool check
        ) const;

        //- Return a list of the frequency indices wrt f field that correspond
        //- to the bands limits for a given octave
        static void setOctaveBands
        (
            const scalarField& f,
            const scalar fLower,
            const scalar fUpper,
            const scalar octave,
            labelList& fBandIDs,
            scalarField& fCentre
        );

        //- Generate octave data
        tmp<scalarField> octaves
        (
            const scalarField& data,
            const scalarField& f,
            const labelUList& freqBandIDs
        ) const;

        //- Return the fft of the given pressure data
        tmp<scalarField> Pf(const scalarField& p) const;

        //- Return the multi-window mean fft of the complete pressure data [Pa]
        tmp<scalarField> meanPf(const scalarField& p) const;

        //- Return the multi-window RMS mean fft of the complete pressure
        //- data [Pa]
        tmp<scalarField> RMSmeanPf(const scalarField& p) const;

        //- Return the multi-window Power Spectral Density (PSD) of the complete
        //- pressure data [Pa^2/Hz]
        tmp<scalarField> PSDf(const scalarField& p, const scalar deltaT) const;


        // Weightings

            //- A weighting function
            scalar RAf(const scalar f) const;

            //- A weighting as gain in dB
            scalar gainA(const scalar f) const;

            //- B weighting function
            scalar RBf(const scalar f) const;

            //- B weighting as gain in dB
            scalar gainB(const scalar f) const;

            //- C weighting function
            scalar RCf(const scalar f) const;

            //- C weighting as gain in dB
            scalar gainC(const scalar f) const;

            //- D weighting function
            scalar RDf(const scalar f) const;

            //- D weighting as gain in dB
            scalar gainD(const scalar f) const;


public:

    //- Runtime type information
    TypeName("noiseModel");

    //- Run time selection table
    declareRunTimeSelectionTable
    (
        autoPtr,
        noiseModel,
        dictionary,
        (
            const dictionary& dict
        ),
        (dict)
    );

    //- Selector
    static autoPtr<noiseModel> New(const dictionary& dict);

    //- Constructor
    noiseModel(const dictionary& dict, const bool readFields = true);

    //- Destructor
    virtual ~noiseModel() = default;


    // Public Member Functions

        //- Read from dictionary
        virtual bool read(const dictionary& dict);

        //- Abstract call to calculate
        virtual void calculate() = 0;

        //- PSD [dB/Hz]
        tmp<Foam::scalarField> PSD(const scalarField& PSDf) const;

        //- SPL [dB]
        tmp<scalarField> SPL
        (
            const scalarField& Prms2,
            const scalar f
        ) const;

        //- SPL [dB]
        tmp<scalarField> SPL
        (
            const scalarField& Prms2,
            const scalarField& f
        ) const;

        //- Clean up the FFTW
        void cleanFFTW();

        //- Helper function to check weightings
        void writeWeightings() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
