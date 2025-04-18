//
// createCoupling.H
// ~~~~~~~~~~~~

    {
#ifdef USE_MUI
        //- If this is not a parallel run then need to first call MPI_Init for MUI (otherwise this is called during PStream creation)
        if (!args.parRunControl().parRun())
        {
            mui::mpi_split_by_app(argc, argv);
        }
#endif

        twoDInterfaces.interfaces = NULL;
        threeDInterfaces.interfaces = NULL;
        bool twoDCreated = false;
        bool threeDCreated = false;

        point meshMin(VGREAT, VGREAT, VGREAT);
        point meshMax(-VSMALL, -VSMALL, -VSMALL);

        const pointField& meshPoints = mesh.points();

        forAll(meshPoints, pts)
        {
            if(meshPoints[pts][0] < meshMin[0])
            {
                meshMin[0] = meshPoints[pts][0];
            }

            if(meshPoints[pts][1] < meshMin[1])
            {
                meshMin[1] = meshPoints[pts][1];
            }

            if(meshPoints[pts][2] < meshMin[2])
            {
                meshMin[2] = meshPoints[pts][2];
            }

            if(meshPoints[pts][0] > meshMax[0])
            {
                meshMax[0] = meshPoints[pts][0];
            }

            if(meshPoints[pts][1] > meshMax[1])
            {
                meshMax[1] = meshPoints[pts][1];
            }

            if(meshPoints[pts][2] > meshMax[2])
            {
                meshMax[2] = meshPoints[pts][2];
            }
        }

        //Determine mesh extents
        vector extents;
        extents[0] = meshMax[0] - meshMin[0];
        extents[1] = meshMax[1] - meshMin[1];
        extents[2] = meshMax[2] - meshMin[2];

        string inputFile("couplingDict");

        IOdictionary couplingDict
        (
            IOobject
            (
                inputFile,
                runTime.system(),
                mesh,
                IOobject::READ_IF_PRESENT,
                IOobject::NO_WRITE
            )
        );

        //Read coupling dictionary file if it exists
        if (couplingDict.headerOk())
        {
            word mainCouplingName;

            if (!(couplingDict.readIfPresent("couplingName", mainCouplingName)))
            {
                FatalIOErrorIn("", couplingDict)
                               << "Missing couplingName entry" << exit(FatalIOError);
            }

            twoDInterfaces.domainName = mainCouplingName;
            threeDInterfaces.domainName = mainCouplingName;

            scalar refLength = 1.0;

            couplingDict.readIfPresent("refLength", refLength);

            twoDInterfaces.refLength = refLength;
            threeDInterfaces.refLength = refLength;

            scalar refTime = 1.0;

            couplingDict.readIfPresent("refTime", refTime);

            twoDInterfaces.refTime = refTime;
            threeDInterfaces.refTime = refTime;

            const PtrList<entry> couplingConfigurationsList
            (
                couplingDict.lookup("couplingConfigurations")
            );

            //Iterate through configuration lists (2D/3D)
            forAll(couplingConfigurationsList, cC)
            {
                const entry& couplingConfigI = couplingConfigurationsList[cC];
                const dictionary& couplingConfigIDict = couplingConfigI.dict();

                wordList iFaceTOC(couplingConfigIDict.toc());

                List<word> interfaceNames(iFaceTOC.size());
                List<bool> send(iFaceTOC.size());
                List<vector> sendStart(iFaceTOC.size());
                List<vector> sendEnd(iFaceTOC.size());
                List<bool> receive(iFaceTOC.size());
                List<vector> rcvStart(iFaceTOC.size());
				List<vector> rcvEnd(iFaceTOC.size());
                List<bool> smart_send(iFaceTOC.size());
                List<bool> iterationCoupling(iFaceTOC.size());

                //Iterate through interfaces
                forAll(iFaceTOC, cI)
                {
                    const dictionary& interfaceDict = couplingConfigIDict.subDict(iFaceTOC[cI]);

                    interfaceNames[cI] = static_cast<word>(interfaceDict.dictName());

                    if (interfaceDict.found("smartSend"))
                    {
                        smart_send[cI] = Switch(interfaceDict.lookup("smartSend"));
                    }
                    else
                    {
                        FatalIOErrorIn("", couplingDict)
                                       << "Missing interface smartSend entry" << exit(FatalIOError);
                    }

                    if (interfaceDict.found("sending"))
                    {
                        send[cI] = Switch(interfaceDict.lookup("sending"));
                    }
                    else
                    {
                        FatalIOErrorIn("", couplingDict)
                                       << "Missing interface sending entry" << exit(FatalIOError);
                    }

                    // Smart send enabled and interface set to send so define domain values according to dictionary input
                    if(smart_send[cI] && send[cI])
                    {
                        bool sendDomDefined = false;

                        if (interfaceDict.found("domainSendStart"))
                        {
                            sendStart[cI] = vector(interfaceDict.lookup("domainSendStart"));
                            sendDomDefined = true;
                        }
                        else
                        {
                            sendDomDefined = false;
                        }

                        if(sendDomDefined)
                        {
                            if (interfaceDict.found("domainSendEnd"))
                            {
                                sendEnd[cI] = vector(interfaceDict.lookup("domainSendEnd"));
                            }
                            else
                            {
                                FatalIOErrorIn("", couplingDict)
                                               << "Missing corresponding interface domainSendEnd entry " << exit(FatalIOError);
                            }

                            // Both start and end defined for send domain so truncate extents to fit mesh bounds if region not completely outside mesh
                            if(sendStart[cI][0] < meshMax[0] && sendEnd[cI][0] > meshMin[0] &&
                               sendStart[cI][1] < meshMax[1] && sendEnd[cI][1] > meshMin[1] &&
                               sendStart[cI][2] < meshMax[2] && sendEnd[cI][2] > meshMin[2])
                            {
                                if(sendStart[cI][0] < meshMin[0]) //Snap to mesh bound minimum - 0.5%
                                {
                                    sendStart[cI][0] = meshMin[0] - extents[0]*0.005;
                                }

                                if(sendEnd[cI][0] > meshMax[0]) //Snap to mesh bound maximum + 0.5%
                                {
                                    sendEnd[cI][0] = meshMax[0] + extents[0]*0.005;
                                }

                                if(sendStart[cI][1] < meshMin[1]) //Snap to mesh bound minimum - 0.5%
                                {
                                    sendStart[cI][1] = meshMin[1] - extents[1]*0.005;
                                }

                                if(sendEnd[cI][1] > meshMax[1]) //Snap to mesh bound maximum + 0.5%
                                {
                                    sendEnd[cI][1] = meshMax[1] + extents[1]*0.005;
                                }

                                if(sendStart[cI][2] < meshMin[2]) //Snap to mesh bound minimum - 0.5%
                                {
                                    sendStart[cI][2] = meshMin[2] - extents[2]*0.005;
                                }

                                if(sendEnd[cI][2] > meshMax[2]) //Snap to mesh bound maximum + 0.5%
                                {
                                    sendEnd[cI][2] = meshMax[2] + extents[2]*0.005;
                                }
                            }
                            else //Set send region to mesh bounds (this mesh is outside of the sending region so need to avoid overlap with other regions this rank shouldn't interact with)
                            {
                                sendStart[cI][0] = meshMin[0];
                                sendStart[cI][1] = meshMin[1];
                                sendStart[cI][2] = meshMin[2];
                                sendEnd[cI][0] = meshMax[0];
                                sendEnd[cI][1] = meshMax[1];
                                sendEnd[cI][2] = meshMax[2];
                            }
                        }
                        else
                        {
                            if (interfaceDict.found("domainSendEnd"))
                            {
                                FatalIOErrorIn("", couplingDict)
                                               << "Missing corresponding interface domainSendStart entry " << exit(FatalIOError);
                            }

                            // "domain_send_start" and "domain_send_end" not set in dictionary, so initialise domain values to mesh bounds +/- 0.5%
                            sendStart[cI][0] = meshMin[0] - extents[0]*0.005;
                            sendStart[cI][1] = meshMin[1] - extents[1]*0.005;
                            sendStart[cI][2] = meshMin[2] - extents[2]*0.005;
                            sendEnd[cI][0] = meshMax[0] + extents[0]*0.005;
                            sendEnd[cI][1] = meshMax[1] + extents[1]*0.005;
                            sendEnd[cI][2] = meshMax[2] + extents[2]*0.005;
                        }
                    }
                    else // Smart send not enabled and/or interface not set to send so just initialise domain values to zero as they wont be used
                    {
                        sendStart[cI][0] = 0;
                        sendStart[cI][1] = 0;
                        sendStart[cI][2] = 0;
                        sendEnd[cI][0] = 0;
                        sendEnd[cI][1] = 0;
                        sendEnd[cI][2] = 0;
                    }

                    if (interfaceDict.found("receiving"))
                    {
                        receive[cI] = Switch(interfaceDict.lookup("receiving"));
                    }
                    else
                    {
                        FatalIOErrorIn("", couplingDict)
                                       << "Missing interface receiving entry" << exit(FatalIOError);
                    }

                    // Smart send enabled and interface set to receive so define domain values according to dictionary input
                    if(smart_send[cI] && receive[cI])
                    {
                        bool rcvDomDefined = false;

                        if (interfaceDict.found("domainReceiveStart"))
                        {
                            rcvStart[cI] = vector(interfaceDict.lookup("domainReceiveStart"));
                            rcvDomDefined = true;
                        }
                        else
                        {
                            rcvDomDefined = false;
                        }

                        if(rcvDomDefined)
                        {
                            if (interfaceDict.found("domainReceiveEnd"))
                            {
                                rcvEnd[cI] = vector(interfaceDict.lookup("domainReceiveEnd"));
                            }
                            else
                            {
                                FatalIOErrorIn("", couplingDict)
                                               << "Missing corresponding interface domainReceiveEnd entry " << exit(FatalIOError);
                            }

                            // Both start and end defined for receive domain so truncate extents to fit mesh bounds if region not completely outside mesh
                            if(rcvStart[cI][0] < meshMax[0] && rcvEnd[cI][0] > meshMin[0] &&
                               rcvStart[cI][1] < meshMax[1] && rcvEnd[cI][1] > meshMin[1] &&
                               rcvStart[cI][2] < meshMax[2] && rcvEnd[cI][2] > meshMin[2])
                            {
                                if(rcvStart[cI][0] < meshMin[0]) //Snap to mesh bound minimum - 0.5%
                                {
                                    rcvStart[cI][0] = meshMin[0] - extents[0]*0.005;
                                }

                                if(rcvEnd[cI][0] > meshMax[0]) //Snap to mesh bound maximum + 0.5%
                                {
                                    rcvEnd[cI][0] = meshMax[0] + extents[0]*0.005;
                                }

                                if(rcvStart[cI][1] < meshMin[1]) //Snap to mesh bound minimum - 0.5%
                                {
                                    rcvStart[cI][1] = meshMin[1] - extents[1]*0.005;
                                }

                                if(rcvEnd[cI][1] > meshMax[1]) //Snap to mesh bound maximum + 0.5%
                                {
                                    rcvEnd[cI][1] = meshMax[1] + extents[1]*0.005;
                                }

                                if(rcvStart[cI][2] < meshMin[2]) //Snap to mesh bound minimum - 0.5%
                                {
                                    rcvStart[cI][2] = meshMin[2] - extents[2]*0.005;
                                }

                                if(rcvEnd[cI][2] > meshMax[2]) //Snap to mesh bound maximum + 0.5%
                                {
                                    rcvEnd[cI][2] = meshMax[2] + extents[2]*0.005;
                                }
                            }
                            else  //Set receive region to mesh bounds (this mesh is outside of the sending region so need to avoid overlap with other regions this rank shouldn't interact with)
                            {
                                rcvStart[cI][0] = meshMin[0];
                                rcvStart[cI][1] = meshMin[1];
                                rcvStart[cI][2] = meshMin[2];
                                rcvEnd[cI][0] = meshMax[0];
                                rcvEnd[cI][1] = meshMax[1];
                                rcvEnd[cI][2] = meshMax[2];
                            }
                        }
                        else
                        {
                            if (interfaceDict.found("domainReceiveEnd"))
                            {
                                FatalIOErrorIn("", couplingDict)
                                               << "Missing corresponding interface domainReceiveStart entry " << exit(FatalIOError);
                            }

                            // "domain_receive_start" and "domain_receive_end" not set in dictionary, so initialise domain values to mesh bounds +/- 0.5%
                            rcvStart[cI][0] = meshMin[0] - extents[0]*0.005;
                            rcvStart[cI][1] = meshMin[1] - extents[1]*0.005;
                            rcvStart[cI][2] = meshMin[2] - extents[2]*0.005;
                            rcvEnd[cI][0] = meshMax[0] + extents[0]*0.005;
                            rcvEnd[cI][1] = meshMax[1] + extents[1]*0.005;
                            rcvEnd[cI][2] = meshMax[2] + extents[2]*0.005;
                        }
                    }
                    else // Smart send not enabled and/or interface not set to send so just initialise domain values to zero as they wont be used
                    {
                        rcvStart[cI][0] = 0;
                        rcvStart[cI][1] = 0;
                        rcvStart[cI][2] = 0;
                        rcvEnd[cI][0] = 0;
                        rcvEnd[cI][1] = 0;
                        rcvEnd[cI][2] = 0;
                    }

                    if (interfaceDict.found("iterationCoupling"))
                    {
                        iterationCoupling[cI] = Switch(interfaceDict.lookup("iterationCoupling"));
                    }
                    else
                    {
                        FatalIOErrorIn("", couplingDict)
                                       << "Missing interface iterationCoupling entry" << exit(FatalIOError);
                    }
                }

                if(iFaceTOC.size() > 0)
                {
                    if(couplingConfigIDict.dictName() == "TwoDInterfaces")
                    {
                        twoDInterfaces.interfaces = new coupling2d(mainCouplingName, interfaceNames, send, receive, smart_send, sendStart, sendEnd, rcvStart, rcvEnd, iterationCoupling);
                        twoDCreated = true;
                    }

                    if(couplingConfigIDict.dictName() == "ThreeDInterfaces")
                    {
                    	threeDInterfaces.interfaces = new coupling3d(mainCouplingName, interfaceNames, send, receive, smart_send, sendStart, sendEnd, rcvStart, rcvEnd, iterationCoupling);
                        threeDCreated = true;
                    }
                }
            }
        }

        couplingDict.close();

        if(twoDCreated)
        {
            //Announce send and receive spans for 2D interfaces that have smart send enabled
            for(size_t i=0; i<twoDInterfaces.interfaces->size(); i++)
            {
                //Check if smart send should be enabled for this interface
                if(twoDInterfaces.interfaces->getInterfaceSmartSendStatus(i))
                {
                    scalar oneOverRefLength = 1.0 / twoDInterfaces.refLength;

                    label extentsDir = -1; //0 = 3D; 1 = Y/Z; 2 = X/Z; 3 = X/Y

                    if(extents[0] >= (0.0 - VSMALL) && extents[0] <= (0.0 + VSMALL)) // X extent is zero, 2D plane is in Y/Z
                    {
                        extentsDir = 1;
                    }
                    else if(extents[1] >= (0.0 - VSMALL) && extents[1] <= (0.0 + VSMALL)) // Y extent is zero, 2D plane is in X/Z
                    {
                        extentsDir = 2;
                    }
                    else if(extents[2] >= (0.0 - VSMALL) && extents[2] <= (0.0 + VSMALL)) // Z extent is zero, 2D plane is in X/Y
                    {
                        extentsDir = 3;
                    }
                    else // Mesh is 3D
                    {
                        extentsDir = 0;
                    }

                    //Check mesh is 2D or 3D
                    if(extentsDir > 0)
                    {
                        mui::point2d start, end;

                        // Interface is set to send
                        if(twoDInterfaces.interfaces->getInterfaceSendStatus(i))
                        {
                            if(extentsDir == 1) //Extents in Y/Z
                            {
                                start[0] = twoDInterfaces.interfaces->getInterfaceSendDomStart(i)[1];
                                start[1] = twoDInterfaces.interfaces->getInterfaceSendDomStart(i)[2];

                                end[0] = twoDInterfaces.interfaces->getInterfaceSendDomEnd(i)[1];
                                end[1] = twoDInterfaces.interfaces->getInterfaceSendDomEnd(i)[2];
                            }
                            else if (extentsDir == 2) //Extents in X/Z
                            {
                                start[0] = twoDInterfaces.interfaces->getInterfaceSendDomStart(i)[0];
                                start[1] = twoDInterfaces.interfaces->getInterfaceSendDomStart(i)[2];

                                end[0] = twoDInterfaces.interfaces->getInterfaceSendDomEnd(i)[0];
                                end[1] = twoDInterfaces.interfaces->getInterfaceSendDomEnd(i)[2];
                            }
                            else if (extentsDir == 3) //Extents in X/Y
                            {
                                start[0] = twoDInterfaces.interfaces->getInterfaceSendDomStart(i)[0];
                                start[1] = twoDInterfaces.interfaces->getInterfaceSendDomStart(i)[1];

                                end[0] = twoDInterfaces.interfaces->getInterfaceSendDomEnd(i)[0];
                                end[1] = twoDInterfaces.interfaces->getInterfaceSendDomEnd(i)[1];
                            }

                            start[0] *= oneOverRefLength;
                            start[1] *= oneOverRefLength;
                            end[0] *= oneOverRefLength;
                            end[1] *= oneOverRefLength;

                            mui::geometry::box2d region_2d(start, end);

                            //- Using iteration based coupling
                            if(twoDInterfaces.interfaces->getInterfaceItCouplingStatus(i))
                            {
                                label totalIter = label((runTime.endTime().value() - runTime.startTime().value()) / runTime.deltaT().value());
                                twoDInterfaces.interfaces->getInterface(i)->announce_send_span(0, totalIter, region_2d);
                            }
                            else //- Using direct time based coupling
                            {
                                twoDInterfaces.interfaces->getInterface(i)->announce_send_span((runTime.startTime().value() / twoDInterfaces.refTime), (runTime.endTime().value() / twoDInterfaces.refTime), region_2d);
                            }
                        }

                        //Interface is set to receive
                        if(twoDInterfaces.interfaces->getInterfaceReceiveStatus(i))
                        {
                            if(extentsDir == 1) //Extents in Y/Z
                            {
                                start[0] = twoDInterfaces.interfaces->getInterfaceReceiveDomStart(i)[1];
                                start[1] = twoDInterfaces.interfaces->getInterfaceReceiveDomStart(i)[2];

                                end[0] = twoDInterfaces.interfaces->getInterfaceReceiveDomEnd(i)[1];
                                end[1] = twoDInterfaces.interfaces->getInterfaceReceiveDomEnd(i)[2];
                            }
                            else if (extentsDir == 2) //Extents in X/Z
                            {
                                start[0] = twoDInterfaces.interfaces->getInterfaceReceiveDomStart(i)[0];
                                start[1] = twoDInterfaces.interfaces->getInterfaceReceiveDomStart(i)[2];

                                end[0] = twoDInterfaces.interfaces->getInterfaceReceiveDomEnd(i)[0];
                                end[1] = twoDInterfaces.interfaces->getInterfaceReceiveDomEnd(i)[2];
                            }
                            else if (extentsDir == 3) //Extents in X/Y
                            {
                                start[0] = twoDInterfaces.interfaces->getInterfaceReceiveDomStart(i)[0];
                                start[1] = twoDInterfaces.interfaces->getInterfaceReceiveDomStart(i)[1];

                                end[0] = twoDInterfaces.interfaces->getInterfaceReceiveDomEnd(i)[0];
                                end[1] = twoDInterfaces.interfaces->getInterfaceReceiveDomEnd(i)[1];
                            }

                            start[0] *= oneOverRefLength;
                            start[1] *= oneOverRefLength;
                            end[0] *= oneOverRefLength;
                            end[1] *= oneOverRefLength;

                            mui::geometry::box2d region_2d(start, end);

                            //- Using iteration based coupling
                            if(twoDInterfaces.interfaces->getInterfaceItCouplingStatus(i))
                            {
                                label totalIter = label((runTime.endTime().value() - runTime.startTime().value()) / runTime.deltaT().value());
                                twoDInterfaces.interfaces->getInterface(i)->announce_recv_span(0, totalIter, region_2d);
                            }
                            else //- Using direct time based coupling
                            {
                                twoDInterfaces.interfaces->getInterface(i)->announce_recv_span((runTime.startTime().value() / twoDInterfaces.refTime), (runTime.endTime().value() / twoDInterfaces.refTime), region_2d);
                            }
                        }

                        //- Using iteration based coupling
                        if(twoDInterfaces.interfaces->getInterfaceItCouplingStatus(i))
                        {
                            twoDInterfaces.interfaces->getInterface(i)->commit(0);
                            twoDInterfaces.interfaces->getInterface(i)->barrier(0);
                            twoDInterfaces.interfaces->getInterface(i)->forget(0, true);
                        }
                        else //- Using direct time based coupling
                        {
                            twoDInterfaces.interfaces->getInterface(i)->commit(runTime.startTime().value() / twoDInterfaces.refTime);
                            twoDInterfaces.interfaces->getInterface(i)->barrier(runTime.startTime().value() / twoDInterfaces.refTime);
                            twoDInterfaces.interfaces->getInterface(i)->forget(runTime.startTime().value() / twoDInterfaces.refTime, true);
                        }
                    }
                    else if (extentsDir == 0) //Mesh is 3D
                    {
                        FatalIOErrorIn("", couplingDict)
                                       << "Trying to create a 2D interface in a 3D mesh" << exit(FatalIOError);
                    }
                }
            }
        }

        if(threeDCreated)
        {
            //Announce send and receive spans for 3D interfaces that have domain extents to enable MUI smart send
            for(size_t i=0; i<threeDInterfaces.interfaces->size(); i++)
            {
                if(threeDInterfaces.interfaces->getInterfaceSmartSendStatus(i)) //Check if smart send should be enabled for this interface
                {
                    scalar oneOverRefLength = 1.0 / threeDInterfaces.refLength;

                    bool isThreeD = true;

                    //If one (or more) of the extents of the mesh is zero then mesh is not 3D
                    if((extents[0] >= (0.0 - VSMALL) && extents[0]) <= (0.0 + VSMALL)
                      || (extents[1] >= (0.0 - VSMALL) && extents[1]) <= (0.0 + VSMALL)
                      || (extents[2] >= (0.0 - VSMALL) && extents[2]) <= (0.0 + VSMALL))
                    {
                        isThreeD = false;
                    }

                    if(isThreeD)
                    {
                        //Interface is set to send
                        if(threeDInterfaces.interfaces->getInterfaceSendStatus(i))
                        {
			                //Create a 3D box geometry based on pre-computed send domain
                            mui::point3d start(threeDInterfaces.interfaces->getInterfaceSendDomStart(i)[0],
                                               threeDInterfaces.interfaces->getInterfaceSendDomStart(i)[1],
                                               threeDInterfaces.interfaces->getInterfaceSendDomStart(i)[2]);
                            mui::point3d end(threeDInterfaces.interfaces->getInterfaceSendDomEnd(i)[0],
                                             threeDInterfaces.interfaces->getInterfaceSendDomEnd(i)[1],
                                             threeDInterfaces.interfaces->getInterfaceSendDomEnd(i)[2]);

                            start[0] *= oneOverRefLength;
                            start[1] *= oneOverRefLength;
                            start[2] *= oneOverRefLength;
                            end[0] *= oneOverRefLength;
                            end[1] *= oneOverRefLength;
                            end[2] *= oneOverRefLength;

                            mui::geometry::box3d region_3d(start, end);

                            //- Using iteration based coupling
                            if(threeDInterfaces.interfaces->getInterfaceItCouplingStatus(i))
                            {
                                label totalIter = label((runTime.endTime().value() - runTime.startTime().value()) / runTime.deltaT().value());
                                threeDInterfaces.interfaces->getInterface(i)->announce_send_span(0, totalIter, region_3d);
                            }
                            else //- Using direct time based coupling
                            {
                                threeDInterfaces.interfaces->getInterface(i)->announce_send_span((runTime.startTime().value() / threeDInterfaces.refTime), (runTime.endTime().value() / threeDInterfaces.refTime), region_3d);
                            }
                        }

                        //Interface is set to receive
                        if(threeDInterfaces.interfaces->getInterfaceReceiveStatus(i))
                        {
                            //Create a 3D box geometry based on pre-computed receive domain
                            mui::point3d start(threeDInterfaces.interfaces->getInterfaceReceiveDomStart(i)[0],
                                               threeDInterfaces.interfaces->getInterfaceReceiveDomStart(i)[1],
                                               threeDInterfaces.interfaces->getInterfaceReceiveDomStart(i)[2]);
                            mui::point3d end(threeDInterfaces.interfaces->getInterfaceReceiveDomEnd(i)[0],
                                             threeDInterfaces.interfaces->getInterfaceReceiveDomEnd(i)[1],
                                             threeDInterfaces.interfaces->getInterfaceReceiveDomEnd(i)[2]);

                            start[0] *= oneOverRefLength;
                            start[1] *= oneOverRefLength;
                            start[2] *= oneOverRefLength;
                            end[0] *= oneOverRefLength;
                            end[1] *= oneOverRefLength;
                            end[2] *= oneOverRefLength;

                            mui::geometry::box3d region_3d(start, end);

                            //- Using iteration based coupling
                            if(threeDInterfaces.interfaces->getInterfaceItCouplingStatus(i))
                            {
                                label totalIter = label((runTime.endTime().value() - runTime.startTime().value()) / runTime.deltaT().value());
                                threeDInterfaces.interfaces->getInterface(i)->announce_recv_span(0, totalIter, region_3d);
                            }
                            else //- Using direct time based coupling
                            {
                                threeDInterfaces.interfaces->getInterface(i)->announce_recv_span((runTime.startTime().value() / threeDInterfaces.refTime), (runTime.endTime().value() / threeDInterfaces.refTime), region_3d);
                            }
                        }

                        //- Using iteration based coupling
                        if(threeDInterfaces.interfaces->getInterfaceItCouplingStatus(i))
                        {
                            threeDInterfaces.interfaces->getInterface(i)->commit(0);
                            threeDInterfaces.interfaces->getInterface(i)->barrier(0);
                            threeDInterfaces.interfaces->getInterface(i)->forget(0, true);
                        }
                        else //- Using direct time based coupling
                        {
                            threeDInterfaces.interfaces->getInterface(i)->commit(runTime.startTime().value() / threeDInterfaces.refTime);
                            threeDInterfaces.interfaces->getInterface(i)->barrier(runTime.startTime().value() / threeDInterfaces.refTime);
                            threeDInterfaces.interfaces->getInterface(i)->forget(runTime.startTime().value() / threeDInterfaces.refTime, true);
                        }
                    }
                    else
                    {
                        FatalIOErrorIn("", couplingDict)
                                       << "Trying to create a 3D interface in a 2D mesh" << exit(FatalIOError);
                    }
                }
            }
        }
    }
