/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2018 OpenFOAM Foundation
    Copyright (C) 2015-2021 OpenCFD Ltd.
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

#include "regIOobject.H"
#include "IFstream.H"
#include "Time.H"
#include "Pstream.H"
#include "HashSet.H"
#include "fileOperation.H"

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

bool Foam::regIOobject::readHeaderOk
(
    const IOstreamOption::streamFormat fmt,
    const word& typeName
)
{
    // Everyone check or just master
    bool masterOnly =
        global()
     && (
            IOobject::fileModificationChecking == IOobject::timeStampMaster
         || IOobject::fileModificationChecking == IOobject::inotifyMaster
        );


    // Check if header is ok for READ_IF_PRESENT
    bool isHeaderOk = false;
    if (readOpt() == IOobject::READ_IF_PRESENT)
    {
        if (masterOnly)
        {
            if (Pstream::master())
            {
                isHeaderOk = headerOk();
            }
            Pstream::scatter(isHeaderOk);
        }
        else
        {
            isHeaderOk = headerOk();
        }
    }

    if
    (
        (
            readOpt() == IOobject::MUST_READ
         || readOpt() == IOobject::MUST_READ_IF_MODIFIED
        )
     || isHeaderOk
    )
    {
        return fileHandler().read(*this, masterOnly, fmt, typeName);
    }

    return false;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::Istream& Foam::regIOobject::readStream(const bool valid)
{
    if (IFstream::debug)
    {
        Pout<< "regIOobject::readStream() : "
            << "reading object " << name()
            << " (global " << global() << ")"
            << " from file " << objectPath()
            << endl;
    }

    if (readOpt() == NO_READ)
    {
        FatalErrorInFunction
            << "NO_READ specified for read-constructor of object " << name()
            << " of class " << headerClassName()
            << abort(FatalError);
    }

    // Construct object stream and read header if not already constructed
    if (!isPtr_)
    {
        fileName objPath;
        if (watchIndices_.size())
        {
            // File is being watched. Read exact file that is being watched.
            objPath = fileHandler().getFile(watchIndices_.last());
        }
        else
        {
            // Search intelligently for file
            objPath = filePath();

            if (IFstream::debug)
            {
                Pout<< "regIOobject::readStream() : "
                    << "found object " << name()
                    << " (global " << global() << ")"
                    << " in file " << objPath
                    << endl;
            }
        }

        isPtr_ = fileHandler().readStream(*this, objPath, type(), valid);
    }

    return *isPtr_;
}


Foam::Istream& Foam::regIOobject::readStream
(
    const word& expectName,
    const bool valid
)
{
    if (IFstream::debug)
    {
        Pout<< "regIOobject::readStream(const word&) : "
            << "reading object " << name()
            << " of type " << type()
            << " from file " << filePath()
            << endl;
    }

    // Construct IFstream if not already constructed
    if (!isPtr_)
    {
        readStream(valid);

        // Check the className of the regIOobject
        // dictionary is an allowable name in case the actual class
        // instantiated is a dictionary
        if
        (
            valid
         && expectName.size()
         && headerClassName() != expectName
         && headerClassName() != "dictionary"
        )
        {
            FatalIOErrorInFunction(isPtr_())
                << "unexpected class name " << headerClassName()
                << " expected " << expectName << endl
                << "    while reading object " << name()
                << exit(FatalIOError);
        }
    }

    return *isPtr_;
}


void Foam::regIOobject::close()
{
    if (IFstream::debug)
    {
        Pout<< "regIOobject::close() : "
            << "finished reading "
            << (isPtr_ ? isPtr_->name() : "dummy")
            << endl;
    }

    isPtr_.reset(nullptr);
}


bool Foam::regIOobject::readData(Istream&)
{
    return false;
}


bool Foam::regIOobject::read()
{
    // Note: cannot do anything in readStream itself since this is used by
    // e.g. GeometricField.


    // Save old watchIndices and clear (so the list of included files can
    // change)
    fileNameList oldWatchFiles;
    if (watchIndices_.size())
    {
        oldWatchFiles.setSize(watchIndices_.size());
        forAll(watchIndices_, i)
        {
            oldWatchFiles[i] = fileHandler().getFile(watchIndices_[i]);
        }
        forAllReverse(watchIndices_, i)
        {
            fileHandler().removeWatch(watchIndices_[i]);
        }
        watchIndices_.clear();
    }


    // Read
    bool masterOnly =
        global()
     && (
            IOobject::fileModificationChecking == IOobject::timeStampMaster
         || IOobject::fileModificationChecking == IOobject::inotifyMaster
        );

    // Note: IOstream::binary flag is for all the processor comms. (Only for
    //       dictionaries should it be ascii)
    bool ok = fileHandler().read(*this, masterOnly, IOstream::BINARY, type());

    if (oldWatchFiles.size())
    {
        // Re-watch master file
        addWatch();
    }

    return ok;
}


bool Foam::regIOobject::modified() const
{
    forAllReverse(watchIndices_, i)
    {
        if (fileHandler().getState(watchIndices_[i]) != fileMonitor::UNMODIFIED)
        {
            return true;
        }
    }

    return false;
}


bool Foam::regIOobject::readIfModified()
{
    // Get index of modified file so we can give nice message. Could instead
    // just call above modified()
    label modified = -1;
    forAllReverse(watchIndices_, i)
    {
        if (fileHandler().getState(watchIndices_[i]) != fileMonitor::UNMODIFIED)
        {
            modified = watchIndices_[i];
            break;
        }
    }

    if (modified != -1)
    {
        const fileName fName = fileHandler().getFile(watchIndices_.last());

        if (modified == watchIndices_.last())
        {
            InfoInFunction
                << "    Re-reading object " << name()
                << " from file " << fName << endl;
        }
        else
        {
            InfoInFunction
                << "    Re-reading object " << name()
                << " from file " << fName
                << " because of modified file "
                << fileHandler().getFile(modified)
                << endl;
        }

        return read();
    }

    return false;
}


// ************************************************************************* //
