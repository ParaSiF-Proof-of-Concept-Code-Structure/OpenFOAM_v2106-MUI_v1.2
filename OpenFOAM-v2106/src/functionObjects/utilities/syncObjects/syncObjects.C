/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

#include "syncObjects.H"
#include "Time.H"
#include "polyMesh.H"
#include "addToRunTimeSelectionTable.H"
#include "objectRegistry.H"
#include "mappedPatchBase.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
    defineTypeNameAndDebug(syncObjects, 0);

    addToRunTimeSelectionTable
    (
        functionObject,
        syncObjects,
        dictionary
    );
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::syncObjects::syncObjects
(
    const word& name,
    const Time& runTime,
    const dictionary& dict
)
:
    functionObject(name),
    obr_
    (
        //runTime.lookupObject<objectRegistry>
        //(
        //    dict.lookupOrDefault("region", polyMesh::defaultRegion)
        //)
        runTime
    )
{
    read(dict);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::functionObjects::syncObjects::sync()
{
    if (debug)
    {
        Pout<< type() << " : sync()"
            << " root:" << root_ << endl;
    }

    const label oldWarnComm = UPstream::warnComm;
    UPstream::warnComm = 0;

    if (!Pstream::parRun())
    {
        return;
    }


    // Send my data to all other processors
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // Note provision of explicit all-world communicator
    PstreamBuffers pBufs
    (
        Pstream::commsTypes::nonBlocking,
        UPstream::msgType(),
        0
    );


    const label nProcs = Pstream::nProcs(pBufs.comm());
    for (label proci = 0; proci < nProcs; proci++)
    {
        // Get database to send
        const objectRegistry& sendObr = mappedPatchBase::subRegistry
        (
            obr_,
            mappedPatchBase::sendPath(root_, proci)
        );

        // Pack into dictionary
        dictionary sendDataDict;
        mappedPatchBase::writeDict(sendObr, sendDataDict);

        //Pout<< "** to processor " << proci
        //    << " sendObr:" << sendObr.objectPath()
        //    << " sending dictionary:" << sendDataDict << endl;
        UOPstream os(proci, pBufs);
        os << sendDataDict;
    }

    // Start sending and receiving and block
    pBufs.finishedSends();

    for (label proci = 0; proci < nProcs; proci++)
    {
        // Get database to receive data into
        const objectRegistry& receiveObr = mappedPatchBase::subRegistry
        (
            obr_,
            mappedPatchBase::receivePath(root_, proci)
        );
        //Pout<< "** from processor " << proci
        //        << " receiveObr:" << receiveObr.objectPath()
        //    << " receiving dictionary" << endl;
        UIPstream is(proci, pBufs);
        const dictionary fromProcDict(is);
        //Pout<< "** from processor " << proci
        //    << " received dictionary:" << fromProcDict << endl;

        mappedPatchBase::readDict
        (
            fromProcDict,
            const_cast<objectRegistry&>(receiveObr)
        );
    }

    //if (debug)
    //{
    //    dictionary allDict;
    //    // Add send subdictionary
    //    dictionary& sendDict = allDict.subDictOrAdd("send");
    //    mappedPatchBase::writeDict
    //    (
    //        mappedPatchBase::subRegistry(obr_, "send"),
    //        sendDict
    //    );
    //    // Add receive subdictionary
    //    dictionary& receiveDict = allDict.subDictOrAdd("receive");
    //    mappedPatchBase::writeDict
    //    (
    //        mappedPatchBase::subRegistry(obr_, "receive"),
    //        receiveDict
    //    );
    //    Pout<< type() << " : after synchronisation:" << allDict << endl;
    //}

    UPstream::warnComm = oldWarnComm;
}


bool Foam::functionObjects::syncObjects::read(const dictionary& dict)
{
    if (debug)
    {
        Pout<< type() << " : read(const dictionary&)" << endl;
    }

    functionObject::read(dict);
    root_ = dict.getOrDefault<fileName>("root", fileName::null);

    if (debug)
    {
        Pout<< type() << " : root:" << root_ << endl;
    }

    // Make sure that at startup we're doing a sync (execute below only gets
    // called at end of timeloop)
    sync();

    return true;
}


bool Foam::functionObjects::syncObjects::execute()
{
    if (debug)
    {
        Pout<< type() << " : execute()" << endl;
    }

    sync();

    return true;
}


bool Foam::functionObjects::syncObjects::write()
{
    if (debug)
    {
        Pout<< type() << " : write()" << endl;
    }

    return true;
}


// ************************************************************************* //
