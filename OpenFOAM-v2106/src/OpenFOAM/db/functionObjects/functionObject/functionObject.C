/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017-2020 OpenCFD Ltd.
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

#include "functionObject.H"
#include "dictionary.H"
#include "dlLibraryTable.H"
#include "Time.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineDebugSwitchWithName(functionObject, "functionObject", 0);
    defineRunTimeSelectionTable(functionObject, dictionary);
}

bool Foam::functionObject::postProcess(false);

Foam::word Foam::functionObject::outputPrefix("postProcessing");


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

Foam::word Foam::functionObject::scopedName(const word& name) const
{
    return name_ + ":" + name;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObject::functionObject(const word& name)
:
    name_(name),
    log(postProcess)
{}


// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

Foam::autoPtr<Foam::functionObject> Foam::functionObject::New
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
{
    const word functionType(dict.get<word>("type"));

    DebugInfo
        << "Selecting function " << functionType << endl;


    // Load any additional libraries
    {
        const auto finder =
            dict.csearchCompat("libs", {{"functionObjectLibs", 1612}});

        if (finder.found())
        {
            runTime.libs().open
            (
                dict,
                finder.ref().keyword(),
                dictionaryConstructorTablePtr_
            );
        }
    }

    // This is the simplified version without compatibility messages
    // runTime.libs().open
    // (
    //     dict,
    //     "libs",
    //     dictionaryConstructorTablePtr_
    // );

    if (!dictionaryConstructorTablePtr_)
    {
        FatalErrorInFunction
            << "Cannot load function type " << functionType << nl << nl
            << "Table of functionObjects is empty" << endl
            << exit(FatalError);
    }

    auto cstrIter = dictionaryConstructorTablePtr_->cfind(functionType);

    if (!cstrIter.found())
    {
        // FatalError (not FatalIOError) to ensure it can be caught
        // as an exception and ignored
        FatalErrorInLookup
        (
            "function",
            functionType,
            *dictionaryConstructorTablePtr_
        ) << exit(FatalError);
    }

    return autoPtr<functionObject>(cstrIter()(name, runTime, dict));
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

const Foam::word& Foam::functionObject::name() const
{
    return name_;
}


bool Foam::functionObject::read(const dictionary& dict)
{
    if (!postProcess)
    {
        log = dict.getOrDefault("log", true);
    }

    return true;
}


bool Foam::functionObject::execute(const label)
{
    return true;
}


bool Foam::functionObject::end()
{
    return true;
}


bool Foam::functionObject::adjustTimeStep()
{
    return false;
}


bool Foam::functionObject::filesModified() const
{
    return false;
}


void Foam::functionObject::updateMesh(const mapPolyMesh&)
{}


void Foam::functionObject::movePoints(const polyMesh&)
{}


// * * * * * * * * * * * * unavailableFunctionObject * * * * * * * * * * * * //

Foam::functionObject::unavailableFunctionObject::unavailableFunctionObject
(
    const word& name
)
:
    functionObject(name)
{}


void Foam::functionObject::unavailableFunctionObject::carp
(
    std::string message
) const
{
    FatalError
        << "####" << nl
        << "    " << type() << " not available" << nl
        << "####" << nl;

    if (message.size())
    {
        FatalError
            << message.c_str() << nl;
    }

    FatalError
        << exit(FatalError);
}


bool Foam::functionObject::unavailableFunctionObject::execute()
{
    return true;
}


bool Foam::functionObject::unavailableFunctionObject::write()
{
    return true;
}


// ************************************************************************* //
