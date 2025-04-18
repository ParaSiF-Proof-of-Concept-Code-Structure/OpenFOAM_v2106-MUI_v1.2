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

Class
    Foam::regionModels::thermalShellModels::vibrationShellModel

Description
    Intermediate class for vibration-shell finite-area models.

Usage
    Example of the boundary condition specification:
    \verbatim
    <patchName>
    {
        // Mandatory/Optional (inherited) entries
        ...

        // Mandatory entries (unmodifiable)
        vibrationShellModel     <thermalShellModelName>;
        p                       <pName>;

        solid
        {
            // subdictionary entries
        }

        // Mandatory/Optional (derived) entries
        ...
    }
    \endverbatim

    where the entries mean:
    \table
      Property   | Description                            | Type  | Reqd | Dflt
      vibrationShellModel | Name of vibration-shell model | word | yes | -
      p          | Name of the coupled field in the primary <!--
                 --> region                               | word | yes | -
      solid      | Solid properties                 | dictionary | yes | -
    \endtable

    The inherited entries are elaborated in:
      - \link regionFaModel.H \endlink

SourceFiles
    vibrationShellModel.C

\*---------------------------------------------------------------------------*/

#ifndef thermalShellModel_H
#define thermalShellModel_H

#include "runTimeSelectionTables.H"
#include "autoPtr.H"
#include "areaFieldsFwd.H"
#include "volFieldsFwd.H"
#include "regionFaModel.H"
#include "faOptions.H"
#include "solidProperties.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace regionModels
{

/*---------------------------------------------------------------------------*\
                      Class vibrationShellModel Declaration
\*---------------------------------------------------------------------------*/

class vibrationShellModel
:
    public regionFaModel
{
    // Private Member Functions

        //- No copy construct
        vibrationShellModel(const vibrationShellModel&) = delete;

        //- No copy assignment
        void operator=(const vibrationShellModel&) = delete;


protected:

    // Protected Data

        //- Name of the coupled field in the primary region
        word pName_;

        //- Primary region acoustic pressure
        const volScalarField& pa_;

        //- Shell displacement
        areaScalarField w_;

        //- Shell acceleration
        areaScalarField a_;

        //- Pointer to faOptions
        Foam::fa::options& faOptions_;

        //- Solid properties
        solidProperties solid_;


    // Protected Member Functions

        //- Read control parameters from dictionary
        virtual bool read(const dictionary&);


public:

    //- Runtime type information
    TypeName("vibrationShellModel");


    // Declare runtime constructor selection tables

         declareRunTimeSelectionTable
         (
             autoPtr,
             vibrationShellModel,
             dictionary,
             (
                const word& modelType,
                const fvPatch& patch,
                const dictionary& dict
             ),
             (modelType, patch, dict)
         );


    // Constructors

        //- Construct from type name and mesh and dict
        vibrationShellModel
        (
            const word& modelType,
            const fvPatch& patch,
            const dictionary& dict
        );


    // Selectors

        //- Return a reference to the selected model using dictionary
        static autoPtr<vibrationShellModel> New
        (
            const fvPatch& patch,
            const dictionary& dict
        );


    //- Destructor
    virtual ~vibrationShellModel() = default;


    // Member Functions

        // Access

            //- Return primary region pa
            const volScalarField& pa() const;

            //- Return shell displacement
            const areaScalarField& w() const;

            //- Return shell acceleration
            const areaScalarField& a() const;

            //- Return faOptions
            Foam::fa::options& faOptions();

            //- Return solid properties
            const solidProperties& solid() const;


        // Evolution

            //- Pre-evolve region
            virtual void preEvolveRegion();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace regionModels
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
