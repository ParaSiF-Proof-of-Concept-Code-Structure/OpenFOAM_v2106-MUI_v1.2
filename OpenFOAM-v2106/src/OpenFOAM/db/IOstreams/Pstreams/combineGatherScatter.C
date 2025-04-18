/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2019-2021 OpenCFD Ltd.
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

Description
    Variant of gather, scatter.
    Normal gather uses:
    - construct null and read (>>) from Istream
    - binary operator and assignment operator to combine values

    combineGather uses:
    - construct from Istream
    - modify operator which modifies its lhs

\*---------------------------------------------------------------------------*/

#include "OPstream.H"
#include "IPstream.H"
#include "IOstreams.H"
#include "contiguous.H"

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class T, class CombineOp>
void Foam::Pstream::combineGather
(
    const List<UPstream::commsStruct>& comms,
    T& Value,
    const CombineOp& cop,
    const int tag,
    const label comm
)
{
    if (UPstream::parRun() && UPstream::nProcs(comm) > 1)
    {
        // Get my communication order
        const commsStruct& myComm = comms[UPstream::myProcNo(comm)];

        // Receive from my downstairs neighbours
        forAll(myComm.below(), belowI)
        {
            label belowID = myComm.below()[belowI];

            if (is_contiguous<T>::value)
            {
                T value;
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    belowID,
                    reinterpret_cast<char*>(&value),
                    sizeof(T),
                    tag,
                    comm
                );

                if (debug & 2)
                {
                    Pout<< " received from "
                        << belowID << " data:" << value << endl;
                }

                cop(Value, value);
            }
            else
            {
                IPstream fromBelow
                (
                    UPstream::commsTypes::scheduled,
                    belowID,
                    0,
                    tag,
                    comm
                );
                T value(fromBelow);

                if (debug & 2)
                {
                    Pout<< " received from "
                        << belowID << " data:" << value << endl;
                }

                cop(Value, value);
            }
        }

        // Send up Value
        if (myComm.above() != -1)
        {
            if (debug & 2)
            {
                Pout<< " sending to " << myComm.above()
                    << " data:" << Value << endl;
            }

            if (is_contiguous<T>::value)
            {
                UOPstream::write
                (
                    UPstream::commsTypes::scheduled,
                    myComm.above(),
                    reinterpret_cast<const char*>(&Value),
                    sizeof(T),
                    tag,
                    comm
                );
            }
            else
            {
                OPstream toAbove
                (
                    UPstream::commsTypes::scheduled,
                    myComm.above(),
                    0,
                    tag,
                    comm
                );
                toAbove << Value;
            }
        }
    }
}


template<class T, class CombineOp>
void Foam::Pstream::combineGather
(
    T& Value,
    const CombineOp& cop,
    const int tag,
    const label comm
)
{
    if (UPstream::nProcs(comm) < UPstream::nProcsSimpleSum)
    {
        combineGather
        (
            UPstream::linearCommunication(comm),
            Value,
            cop,
            tag,
            comm
        );
    }
    else
    {
        combineGather
        (
            UPstream::treeCommunication(comm),
            Value,
            cop,
            tag,
            comm
        );
    }
}


template<class T>
void Foam::Pstream::combineScatter
(
    const List<UPstream::commsStruct>& comms,
    T& Value,
    const int tag,
    const label comm
)
{
    if (UPstream::parRun() && UPstream::nProcs(comm) > 1)
    {
        // Get my communication order
        const UPstream::commsStruct& myComm = comms[UPstream::myProcNo(comm)];

        // Receive from up
        if (myComm.above() != -1)
        {
            if (is_contiguous<T>::value)
            {
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    myComm.above(),
                    reinterpret_cast<char*>(&Value),
                    sizeof(T),
                    tag,
                    comm
                );
            }
            else
            {
                IPstream fromAbove
                (
                    UPstream::commsTypes::scheduled,
                    myComm.above(),
                    0,
                    tag,
                    comm
                );
                Value = T(fromAbove);
            }

            if (debug & 2)
            {
                Pout<< " received from "
                    << myComm.above() << " data:" << Value << endl;
            }
        }

        // Send to my downstairs neighbours
        forAllReverse(myComm.below(), belowI)
        {
            label belowID = myComm.below()[belowI];

            if (debug & 2)
            {
                Pout<< " sending to " << belowID << " data:" << Value << endl;
            }

            if (is_contiguous<T>::value)
            {
                UOPstream::write
                (
                    UPstream::commsTypes::scheduled,
                    belowID,
                    reinterpret_cast<const char*>(&Value),
                    sizeof(T),
                    tag,
                    comm
                );
            }
            else
            {
                OPstream toBelow
                (
                    UPstream::commsTypes::scheduled,
                    belowID,
                    0,
                    tag,
                    comm
                );
                toBelow << Value;
            }
        }
    }
}


template<class T>
void Foam::Pstream::combineScatter
(
    T& Value,
    const int tag,
    const label comm
)
{
    if (UPstream::nProcs(comm) < UPstream::nProcsSimpleSum)
    {
        combineScatter(UPstream::linearCommunication(comm), Value, tag, comm);
    }
    else
    {
        combineScatter(UPstream::treeCommunication(comm), Value, tag, comm);
    }
}


template<class T, class CombineOp>
void Foam::Pstream::listCombineGather
(
    const List<UPstream::commsStruct>& comms,
    List<T>& Values,
    const CombineOp& cop,
    const int tag,
    const label comm
)
{
    if (UPstream::parRun() && UPstream::nProcs(comm) > 1)
    {
        // Get my communication order
        const commsStruct& myComm = comms[UPstream::myProcNo(comm)];

        // Receive from my downstairs neighbours
        forAll(myComm.below(), belowI)
        {
            label belowID = myComm.below()[belowI];

            if (is_contiguous<T>::value)
            {
                List<T> receivedValues(Values.size());

                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    belowID,
                    reinterpret_cast<char*>(receivedValues.data()),
                    receivedValues.size_bytes(),
                    tag,
                    comm
                );

                if (debug & 2)
                {
                    Pout<< " received from "
                        << belowID << " data:" << receivedValues << endl;
                }

                forAll(Values, i)
                {
                    cop(Values[i], receivedValues[i]);
                }
            }
            else
            {
                IPstream fromBelow
                (
                    UPstream::commsTypes::scheduled,
                    belowID,
                    0,
                    tag,
                    comm
                );
                List<T> receivedValues(fromBelow);

                if (debug & 2)
                {
                    Pout<< " received from "
                        << belowID << " data:" << receivedValues << endl;
                }

                forAll(Values, i)
                {
                    cop(Values[i], receivedValues[i]);
                }
            }
        }

        // Send up Value
        if (myComm.above() != -1)
        {
            if (debug & 2)
            {
                Pout<< " sending to " << myComm.above()
                    << " data:" << Values << endl;
            }

            if (is_contiguous<T>::value)
            {
                UOPstream::write
                (
                    UPstream::commsTypes::scheduled,
                    myComm.above(),
                    reinterpret_cast<const char*>(Values.cdata()),
                    Values.size_bytes(),
                    tag,
                    comm
                );
            }
            else
            {
                OPstream toAbove
                (
                    UPstream::commsTypes::scheduled,
                    myComm.above(),
                    0,
                    tag,
                    comm
                );
                toAbove << Values;
            }
        }
    }
}


template<class T, class CombineOp>
void Foam::Pstream::listCombineGather
(
    List<T>& Values,
    const CombineOp& cop,
    const int tag,
    const label comm
)
{
    if (UPstream::nProcs(comm) < UPstream::nProcsSimpleSum)
    {
        listCombineGather
        (
            UPstream::linearCommunication(comm),
            Values,
            cop,
            tag,
            comm
        );
    }
    else
    {
        listCombineGather
        (
            UPstream::treeCommunication(comm),
            Values,
            cop,
            tag,
            comm
        );
    }
}


template<class T>
void Foam::Pstream::listCombineScatter
(
    const List<UPstream::commsStruct>& comms,
    List<T>& Values,
    const int tag,
    const label comm
)
{
    if (UPstream::parRun() && UPstream::nProcs(comm) > 1)
    {
        // Get my communication order
        const UPstream::commsStruct& myComm = comms[UPstream::myProcNo(comm)];

        // Receive from up
        if (myComm.above() != -1)
        {
            if (is_contiguous<T>::value)
            {
                UIPstream::read
                (
                    UPstream::commsTypes::scheduled,
                    myComm.above(),
                    reinterpret_cast<char*>(Values.data()),
                    Values.size_bytes(),
                    tag,
                    comm
                );
            }
            else
            {
                IPstream fromAbove
                (
                    UPstream::commsTypes::scheduled,
                    myComm.above(),
                    0,
                    tag,
                    comm
                );
                fromAbove >> Values;
            }

            if (debug & 2)
            {
                Pout<< " received from "
                    << myComm.above() << " data:" << Values << endl;
            }
        }

        // Send to my downstairs neighbours
        forAllReverse(myComm.below(), belowI)
        {
            label belowID = myComm.below()[belowI];

            if (debug & 2)
            {
                Pout<< " sending to " << belowID << " data:" << Values << endl;
            }

            if (is_contiguous<T>::value)
            {
                UOPstream::write
                (
                    UPstream::commsTypes::scheduled,
                    belowID,
                    reinterpret_cast<const char*>(Values.cdata()),
                    Values.size_bytes(),
                    tag,
                    comm
                );
            }
            else
            {
                OPstream toBelow
                (
                    UPstream::commsTypes::scheduled,
                    belowID,
                    0,
                    tag,
                    comm
                );
                toBelow << Values;
            }
        }
    }
}


template<class T>
void Foam::Pstream::listCombineScatter
(
    List<T>& Values,
    const int tag,
    const label comm
)
{
    if (UPstream::nProcs(comm) < UPstream::nProcsSimpleSum)
    {
        listCombineScatter
        (
            UPstream::linearCommunication(comm),
            Values,
            tag,
            comm
        );
    }
    else
    {
        listCombineScatter
        (
            UPstream::treeCommunication(comm),
            Values,
            tag,
            comm
        );
    }
}


template<class Container, class CombineOp>
void Foam::Pstream::mapCombineGather
(
    const List<UPstream::commsStruct>& comms,
    Container& Values,
    const CombineOp& cop,
    const int tag,
    const label comm
)
{
    if (UPstream::parRun() && UPstream::nProcs(comm) > 1)
    {
        // Get my communication order
        const commsStruct& myComm = comms[UPstream::myProcNo(comm)];

        // Receive from my downstairs neighbours
        forAll(myComm.below(), belowI)
        {
            label belowID = myComm.below()[belowI];

            IPstream fromBelow
            (
                UPstream::commsTypes::scheduled,
                belowID,
                0,
                tag,
                comm
            );
            Container receivedValues(fromBelow);

            if (debug & 2)
            {
                Pout<< " received from "
                    << belowID << " data:" << receivedValues << endl;
            }

            for
            (
                typename Container::const_iterator slaveIter =
                    receivedValues.begin();
                slaveIter != receivedValues.end();
                ++slaveIter
            )
            {
                typename Container::iterator
                    masterIter = Values.find(slaveIter.key());

                if (masterIter != Values.end())
                {
                    cop(masterIter(), slaveIter());
                }
                else
                {
                    Values.insert(slaveIter.key(), slaveIter());
                }
            }
        }

        // Send up Value
        if (myComm.above() != -1)
        {
            if (debug & 2)
            {
                Pout<< " sending to " << myComm.above()
                    << " data:" << Values << endl;
            }

            OPstream toAbove
            (
                UPstream::commsTypes::scheduled,
                myComm.above(),
                0,
                tag,
                comm
            );
            toAbove << Values;
        }
    }
}


template<class Container, class CombineOp>
void Foam::Pstream::mapCombineGather
(
    Container& Values,
    const CombineOp& cop,
    const int tag,
    const label comm
)
{
    if (UPstream::nProcs(comm) < UPstream::nProcsSimpleSum)
    {
        mapCombineGather
        (
            UPstream::linearCommunication(comm),
            Values,
            cop,
            tag,
            comm
        );
    }
    else
    {
        mapCombineGather
        (
            UPstream::treeCommunication(comm),
            Values,
            cop,
            tag,
            comm
        );
    }
}


template<class Container>
void Foam::Pstream::mapCombineScatter
(
    const List<UPstream::commsStruct>& comms,
    Container& Values,
    const int tag,
    const label comm
)
{
    if (UPstream::parRun() && UPstream::nProcs(comm) > 1)
    {
        // Get my communication order
        const UPstream::commsStruct& myComm = comms[UPstream::myProcNo(comm)];

        // Receive from up
        if (myComm.above() != -1)
        {
            IPstream fromAbove
            (
                UPstream::commsTypes::scheduled,
                myComm.above(),
                0,
                tag,
                comm
            );
            fromAbove >> Values;

            if (debug & 2)
            {
                Pout<< " received from "
                    << myComm.above() << " data:" << Values << endl;
            }
        }

        // Send to my downstairs neighbours
        forAllReverse(myComm.below(), belowI)
        {
            label belowID = myComm.below()[belowI];

            if (debug & 2)
            {
                Pout<< " sending to " << belowID << " data:" << Values << endl;
            }

            OPstream toBelow
            (
                UPstream::commsTypes::scheduled,
                belowID,
                0,
                tag,
                comm
            );
            toBelow << Values;
        }
    }
}


template<class Container>
void Foam::Pstream::mapCombineScatter
(
    Container& Values,
    const int tag,
    const label comm
)
{
    if (UPstream::nProcs(comm) < UPstream::nProcsSimpleSum)
    {
        mapCombineScatter
        (
            UPstream::linearCommunication(comm),
            Values,
            tag,
            comm
        );
    }
    else
    {
        mapCombineScatter
        (
            UPstream::treeCommunication(comm),
            Values,
            tag,
            comm
        );
    }
}


// ************************************************************************* //
