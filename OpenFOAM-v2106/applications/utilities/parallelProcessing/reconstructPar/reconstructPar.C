/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
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

Application
    reconstructPar

Group
    grpParallelUtilities

Description
    Reconstructs fields of a case that is decomposed for parallel
    execution of OpenFOAM.

\*---------------------------------------------------------------------------*/

#include "argList.H"
#include "timeSelector.H"

#include "fvCFD.H"
#include "IOobjectList.H"
#include "processorMeshes.H"
#include "regionProperties.H"
#include "fvFieldReconstructor.H"
#include "pointFieldReconstructor.H"
#include "lagrangianReconstructor.H"

#include "faCFD.H"
#include "faMesh.H"
#include "processorFaMeshes.H"
#include "faFieldReconstructor.H"

#include "cellSet.H"
#include "faceSet.H"
#include "pointSet.H"

#include "hexRef8Data.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

bool haveAllTimes
(
    const wordHashSet& masterTimeDirSet,
    const instantList& timeDirs
)
{
    // Loop over all times
    for (const instant& t : timeDirs)
    {
        if (!masterTimeDirSet.found(t.name()))
        {
            return false;
        }
    }
    return true;
}


int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Reconstruct fields of a parallel case"
    );

    // Enable -constant ... if someone really wants it
    // Enable -withZero to prevent accidentally trashing the initial fields
    timeSelector::addOptions(true, true);  // constant(true), zero(true)
    argList::noParallel();

    #include "addAllRegionOptions.H"

    argList::addOption
    (
        "fields",
        "wordRes",
        "Specify single or multiple fields to reconstruct (all by default)."
        " Eg, 'T' or '(p T U \"alpha.*\")'"
    );
    argList::addBoolOption
    (
        "noFields",
        "Skip reconstructing fields"
    );
    argList::addOption
    (
        "lagrangianFields",
        "wordRes",
        "Specify single or multiple lagrangian fields to reconstruct"
        " (all by default)."
        " Eg, '(U d)'"
        " - Positions are always included."
    );
    argList::addBoolOption
    (
        "noLagrangian",
        "Skip reconstructing lagrangian positions and fields"
    );

    argList::addBoolOption
    (
        "noSets",
        "Skip reconstructing cellSets, faceSets, pointSets"
    );
    argList::addBoolOption
    (
        "newTimes",
        "Only reconstruct new times (i.e. that do not exist already)"
    );

    #include "setRootCase.H"
    #include "createTime.H"


    wordRes selectedFields;
    args.readListIfPresent<wordRe>("fields", selectedFields);

    const bool doFields = !args.found("noFields");

    if (!doFields)
    {
        Info<< "Skipping reconstructing fields"
            << nl << endl;
    }

    wordRes selectedLagrangianFields;
    args.readListIfPresent<wordRe>
    (
        "lagrangianFields", selectedLagrangianFields
    );

    const bool doLagrangian = !args.found("noLagrangian");

    if (!doLagrangian)
    {
        Info<< "Skipping reconstructing lagrangian positions and fields"
            << nl << endl;
    }

    const bool doReconstructSets = !args.found("noSets");

    if (!doReconstructSets)
    {
        Info<< "Skipping reconstructing cellSets, faceSets and pointSets"
            << nl << endl;
    }

    const bool newTimes = args.found("newTimes");

    // Get region names
    #include "getAllRegionOptions.H"

    // Determine the processor count
    label nProcs{0};

    if (regionNames.empty())
    {
        FatalErrorInFunction
            << "No regions specified or detected."
            << exit(FatalError);
    }
    else if (regionNames[0] == polyMesh::defaultRegion)
    {
        nProcs = fileHandler().nProcs(args.path());
    }
    else
    {
        nProcs = fileHandler().nProcs(args.path(), regionNames[0]);

        if (regionNames.size() == 1)
        {
            Info<< "Using region: " << regionNames[0] << nl << endl;
        }
    }

    if (!nProcs)
    {
        FatalErrorInFunction
            << "No processor* directories found"
            << exit(FatalError);
    }

    // Warn fileHandler of number of processors
    const_cast<fileOperation&>(fileHandler()).setNProcs(nProcs);

    // Create the processor databases
    PtrList<Time> databases(nProcs);

    forAll(databases, proci)
    {
        databases.set
        (
            proci,
            new Time
            (
                Time::controlDictName,
                args.rootPath(),
                args.caseName()/("processor" + Foam::name(proci))
            )
        );
    }

    // Use the times list from the master processor
    // and select a subset based on the command-line options
    instantList timeDirs = timeSelector::select
    (
        databases[0].times(),
        args
    );

    // Note that we do not set the runTime time so it is still the
    // one set through the controlDict. The -time option
    // only affects the selected set of times from processor0.
    // - can be illogical
    // + any point motion handled through mesh.readUpdate


    if (timeDirs.empty())
    {
        WarningInFunction << "No times selected";
        exit(1);
    }


    // Get current times if -newTimes
    instantList masterTimeDirs;
    if (newTimes)
    {
        masterTimeDirs = runTime.times();
    }
    wordHashSet masterTimeDirSet(2*masterTimeDirs.size());
    for (const instant& t : masterTimeDirs)
    {
        masterTimeDirSet.insert(t.name());
    }


    // Set all times on processor meshes equal to reconstructed mesh
    forAll(databases, proci)
    {
        databases[proci].setTime(runTime);
    }


    forAll(regionNames, regioni)
    {
        const word& regionName = regionNames[regioni];
        const word& regionDir =
        (
            regionName != polyMesh::defaultRegion
          ? regionName
          : word::null
        );

        Info<< "\n\nReconstructing fields" << nl
            << "region=" << regionName << nl << endl;

        if
        (
            newTimes
         && regionNames.size() == 1
         && regionDir.empty()
         && haveAllTimes(masterTimeDirSet, timeDirs)
        )
        {
            Info<< "Skipping region " << regionName
                << " since already have all times"
                << endl << endl;
            continue;
        }


        fvMesh mesh
        (
            IOobject
            (
                regionName,
                runTime.timeName(),
                runTime,
                Foam::IOobject::MUST_READ
            )
        );


        // Read all meshes and addressing to reconstructed mesh
        processorMeshes procMeshes(databases, regionName);


        // Check face addressing for meshes that have been decomposed
        // with a very old foam version
        #include "checkFaceAddressingComp.H"

        // Loop over all times
        forAll(timeDirs, timei)
        {
            if (newTimes && masterTimeDirSet.found(timeDirs[timei].name()))
            {
                Info<< "Skipping time " << timeDirs[timei].name()
                    << endl << endl;
                continue;
            }


            // Set time for global database
            runTime.setTime(timeDirs[timei], timei);

            Info<< "Time = " << runTime.timeName() << endl << endl;

            // Set time for all databases
            forAll(databases, proci)
            {
                databases[proci].setTime(timeDirs[timei], timei);
            }

            // Check if any new meshes need to be read.
            fvMesh::readUpdateState meshStat = mesh.readUpdate();

            fvMesh::readUpdateState procStat = procMeshes.readUpdate();

            if (procStat == fvMesh::POINTS_MOVED)
            {
                // Reconstruct the points for moving mesh cases and write
                // them out
                procMeshes.reconstructPoints(mesh);
            }
            else if (meshStat != procStat)
            {
                WarningInFunction
                    << "readUpdate for the reconstructed mesh:"
                    << meshStat << nl
                    << "readUpdate for the processor meshes  :"
                    << procStat << nl
                    << "These should be equal or your addressing"
                    << " might be incorrect."
                    << " Please check your time directories for any "
                    << "mesh directories." << endl;
            }


            // Get list of objects from processor0 database
            IOobjectList objects
            (
                procMeshes.meshes()[0],
                databases[0].timeName()
            );

            if (doFields)
            {
                // If there are any FV fields, reconstruct them
                Info<< "Reconstructing FV fields" << nl << endl;

                fvFieldReconstructor reconstructor
                (
                    mesh,
                    procMeshes.meshes(),
                    procMeshes.faceProcAddressing(),
                    procMeshes.cellProcAddressing(),
                    procMeshes.boundaryProcAddressing()
                );

                reconstructor.reconstructFvVolumeInternalFields<scalar>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvVolumeInternalFields<vector>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvVolumeInternalFields<sphericalTensor>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvVolumeInternalFields<symmTensor>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvVolumeInternalFields<tensor>
                (
                    objects,
                    selectedFields
                );

                reconstructor.reconstructFvVolumeFields<scalar>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvVolumeFields<vector>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvVolumeFields<sphericalTensor>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvVolumeFields<symmTensor>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvVolumeFields<tensor>
                (
                    objects,
                    selectedFields
                );

                reconstructor.reconstructFvSurfaceFields<scalar>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvSurfaceFields<vector>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvSurfaceFields<sphericalTensor>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvSurfaceFields<symmTensor>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFvSurfaceFields<tensor>
                (
                    objects,
                    selectedFields
                );

                if (reconstructor.nReconstructed() == 0)
                {
                    Info<< "No FV fields" << nl << endl;
                }
            }

            if (doFields)
            {
                Info<< "Reconstructing point fields" << nl << endl;

                const pointMesh& pMesh = pointMesh::New(mesh);
                PtrList<pointMesh> pMeshes(procMeshes.meshes().size());

                forAll(pMeshes, proci)
                {
                    pMeshes.set
                    (
                        proci,
                        new pointMesh(procMeshes.meshes()[proci])
                    );
                }

                pointFieldReconstructor reconstructor
                (
                    pMesh,
                    pMeshes,
                    procMeshes.pointProcAddressing(),
                    procMeshes.boundaryProcAddressing()
                );

                reconstructor.reconstructFields<scalar>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFields<vector>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFields<sphericalTensor>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFields<symmTensor>
                (
                    objects,
                    selectedFields
                );
                reconstructor.reconstructFields<tensor>
                (
                    objects,
                    selectedFields
                );

                if (reconstructor.nReconstructed() == 0)
                {
                    Info<< "No point fields" << nl << endl;
                }
            }


            // If there are any clouds, reconstruct them.
            // The problem is that a cloud of size zero will not get written so
            // in pass 1 we determine the cloud names and per cloud name the
            // fields. Note that the fields are stored as IOobjectList from
            // the first processor that has them. They are in pass2 only used
            // for name and type (scalar, vector etc).

            if (doLagrangian)
            {
                HashTable<IOobjectList> allCloudObjects;

                forAll(databases, proci)
                {
                    fileName lagrangianDir
                    (
                        fileHandler().filePath
                        (
                            databases[proci].timePath()
                          / regionDir
                          / cloud::prefix
                        )
                    );

                    fileNameList cloudDirs;
                    if (!lagrangianDir.empty())
                    {
                        cloudDirs = fileHandler().readDir
                        (
                            lagrangianDir,
                            fileName::DIRECTORY
                        );
                    }

                    for (const fileName& cloudDir : cloudDirs)
                    {
                        // Check if we already have cloud objects for this
                        // cloudname
                        if (!allCloudObjects.found(cloudDir))
                        {
                            // Do local scan for valid cloud objects
                            IOobjectList localObjs
                            (
                                procMeshes.meshes()[proci],
                                databases[proci].timeName(),
                                cloud::prefix/cloudDir
                            );

                            if
                            (
                                localObjs.found("coordinates")
                             || localObjs.found("positions")
                            )
                            {
                                allCloudObjects.insert(cloudDir, localObjs);
                            }
                        }
                    }
                }


                if (allCloudObjects.size())
                {
                    lagrangianReconstructor reconstructor
                    (
                        mesh,
                        procMeshes.meshes(),
                        procMeshes.faceProcAddressing(),
                        procMeshes.cellProcAddressing()
                    );

                    // Pass2: reconstruct the cloud
                    forAllConstIters(allCloudObjects, iter)
                    {
                        const word cloudName = word::validate(iter.key());

                        // Objects (on arbitrary processor)
                        const IOobjectList& cloudObjs = iter.val();

                        Info<< "Reconstructing lagrangian fields for cloud "
                            << cloudName << nl << endl;

                        reconstructor.reconstructPositions(cloudName);

                        reconstructor.reconstructFields<label>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );
                        reconstructor.reconstructFieldFields<label>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );

                        reconstructor.reconstructFields<scalar>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );
                        reconstructor.reconstructFieldFields<scalar>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );

                        reconstructor.reconstructFields<vector>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );
                        reconstructor.reconstructFieldFields<vector>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );

                        reconstructor.reconstructFields<sphericalTensor>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );
                        reconstructor.reconstructFieldFields<sphericalTensor>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );

                        reconstructor.reconstructFields<symmTensor>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );
                        reconstructor.reconstructFieldFields<symmTensor>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );

                        reconstructor.reconstructFields<tensor>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );
                        reconstructor.reconstructFieldFields<tensor>
                        (
                            cloudName,
                            cloudObjs,
                            selectedLagrangianFields
                        );
                    }
                }
                else
                {
                    Info<< "No lagrangian fields" << nl << endl;
                }
            }


            // If there are any FA fields, reconstruct them

            if
            (
                objects.lookupClass(areaScalarField::typeName).size()
             || objects.lookupClass(areaVectorField::typeName).size()
             || objects.lookupClass(areaSphericalTensorField::typeName).size()
             || objects.lookupClass(areaSymmTensorField::typeName).size()
             || objects.lookupClass(areaTensorField::typeName).size()
             || objects.lookupClass(edgeScalarField::typeName).size()
            )
            {
                Info << "Reconstructing FA fields" << nl << endl;

                faMesh aMesh(mesh);

                processorFaMeshes procFaMeshes(procMeshes.meshes());

                faFieldReconstructor reconstructor
                (
                    aMesh,
                    procFaMeshes.meshes(),
                    procFaMeshes.edgeProcAddressing(),
                    procFaMeshes.faceProcAddressing(),
                    procFaMeshes.boundaryProcAddressing()
                );

                reconstructor.reconstructFaAreaFields<scalar>(objects);
                reconstructor.reconstructFaAreaFields<vector>(objects);
                reconstructor.reconstructFaAreaFields<sphericalTensor>(objects);
                reconstructor.reconstructFaAreaFields<symmTensor>(objects);
                reconstructor.reconstructFaAreaFields<tensor>(objects);

                reconstructor.reconstructFaEdgeFields<scalar>(objects);
            }
            else
            {
                Info << "No FA fields" << nl << endl;
            }

            if (doReconstructSets)
            {
                // Scan to find all sets
                HashTable<label> cSetNames;
                HashTable<label> fSetNames;
                HashTable<label> pSetNames;

                forAll(procMeshes.meshes(), proci)
                {
                    const fvMesh& procMesh = procMeshes.meshes()[proci];

                    // Note: look at sets in current time only or between
                    // mesh and current time?. For now current time. This will
                    // miss out on sets in intermediate times that have not
                    // been reconstructed.
                    IOobjectList objects
                    (
                        procMesh,
                        databases[0].timeName(),    //procMesh.facesInstance()
                        polyMesh::meshSubDir/"sets"
                    );

                    IOobjectList cSets(objects.lookupClass(cellSet::typeName));
                    forAllConstIters(cSets, iter)
                    {
                        cSetNames.insert(iter.key(), cSetNames.size());
                    }

                    IOobjectList fSets(objects.lookupClass(faceSet::typeName));
                    forAllConstIters(fSets, iter)
                    {
                        fSetNames.insert(iter.key(), fSetNames.size());
                    }
                    IOobjectList pSets(objects.lookupClass(pointSet::typeName));
                    forAllConstIters(pSets, iter)
                    {
                        pSetNames.insert(iter.key(), pSetNames.size());
                    }
                }

                if (cSetNames.size() || fSetNames.size() || pSetNames.size())
                {
                    // Construct all sets
                    PtrList<cellSet> cellSets(cSetNames.size());
                    PtrList<faceSet> faceSets(fSetNames.size());
                    PtrList<pointSet> pointSets(pSetNames.size());

                    Info<< "Reconstructing sets:" << endl;
                    if (cSetNames.size())
                    {
                        Info<< "    cellSets "
                            << cSetNames.sortedToc() << endl;
                    }
                    if (fSetNames.size())
                    {
                        Info<< "    faceSets "
                            << fSetNames.sortedToc() << endl;
                    }
                    if (pSetNames.size())
                    {
                        Info<< "    pointSets "
                            << pSetNames.sortedToc() << endl;
                    }

                    // Load sets
                    forAll(procMeshes.meshes(), proci)
                    {
                        const fvMesh& procMesh = procMeshes.meshes()[proci];

                        IOobjectList objects
                        (
                            procMesh,
                            databases[0].timeName(),
                            polyMesh::meshSubDir/"sets"
                        );

                        // cellSets
                        const labelList& cellMap =
                            procMeshes.cellProcAddressing()[proci];

                        IOobjectList cSets
                        (
                            objects.lookupClass(cellSet::typeName)
                        );

                        forAllConstIters(cSets, iter)
                        {
                            // Load cellSet
                            const cellSet procSet(*iter());
                            label setI = cSetNames[iter.key()];
                            if (!cellSets.set(setI))
                            {
                                cellSets.set
                                (
                                    setI,
                                    new cellSet
                                    (
                                        mesh,
                                        iter.key(),
                                        procSet.size()
                                    )
                                );
                            }
                            cellSet& cSet = cellSets[setI];
                            cSet.instance() = runTime.timeName();

                            for (const label celli : procSet)
                            {
                                cSet.insert(cellMap[celli]);
                            }
                        }

                        // faceSets
                        const labelList& faceMap =
                            procMeshes.faceProcAddressing()[proci];

                        IOobjectList fSets
                        (
                            objects.lookupClass(faceSet::typeName)
                        );

                        forAllConstIters(fSets, iter)
                        {
                            // Load faceSet
                            const faceSet procSet(*iter());
                            label setI = fSetNames[iter.key()];
                            if (!faceSets.set(setI))
                            {
                                faceSets.set
                                (
                                    setI,
                                    new faceSet
                                    (
                                        mesh,
                                        iter.key(),
                                        procSet.size()
                                    )
                                );
                            }
                            faceSet& fSet = faceSets[setI];
                            fSet.instance() = runTime.timeName();

                            for (const label facei : procSet)
                            {
                                fSet.insert(mag(faceMap[facei])-1);
                            }
                        }
                        // pointSets
                        const labelList& pointMap =
                            procMeshes.pointProcAddressing()[proci];

                        IOobjectList pSets
                        (
                            objects.lookupClass(pointSet::typeName)
                        );
                        forAllConstIters(pSets, iter)
                        {
                            // Load pointSet
                            const pointSet propSet(*iter());
                            label setI = pSetNames[iter.key()];
                            if (!pointSets.set(setI))
                            {
                                pointSets.set
                                (
                                    setI,
                                    new pointSet
                                    (
                                        mesh,
                                        iter.key(),
                                        propSet.size()
                                    )
                                );
                            }
                            pointSet& pSet = pointSets[setI];
                            pSet.instance() = runTime.timeName();

                            for (const label pointi : propSet)
                            {
                                pSet.insert(pointMap[pointi]);
                            }
                        }
                    }

                    // Write sets

                    for (const auto& set : cellSets)
                    {
                        set.write();
                    }
                    for (const auto& set : faceSets)
                    {
                        set.write();
                    }
                    for (const auto& set : pointSets)
                    {
                        set.write();
                    }
                }


            // Reconstruct refinement data
            {
                PtrList<hexRef8Data> procData(procMeshes.meshes().size());

                forAll(procMeshes.meshes(), procI)
                {
                    const fvMesh& procMesh = procMeshes.meshes()[procI];

                    procData.set
                    (
                        procI,
                        new hexRef8Data
                        (
                            IOobject
                            (
                                "dummy",
                                procMesh.time().timeName(),
                                polyMesh::meshSubDir,
                                procMesh,
                                IOobject::READ_IF_PRESENT,
                                IOobject::NO_WRITE,
                                false
                            )
                        )
                    );
                }

                // Combine individual parts

                const PtrList<labelIOList>& cellAddr =
                    procMeshes.cellProcAddressing();

                UPtrList<const labelList> cellMaps(cellAddr.size());
                forAll(cellAddr, i)
                {
                    cellMaps.set(i, &cellAddr[i]);
                }

                const PtrList<labelIOList>& pointAddr =
                    procMeshes.pointProcAddressing();

                UPtrList<const labelList> pointMaps(pointAddr.size());
                forAll(pointAddr, i)
                {
                    pointMaps.set(i, &pointAddr[i]);
                }

                UPtrList<const hexRef8Data> procRefs(procData.size());
                forAll(procData, i)
                {
                    procRefs.set(i, &procData[i]);
                }

                hexRef8Data
                (
                    IOobject
                    (
                        "dummy",
                        mesh.time().timeName(),
                        polyMesh::meshSubDir,
                        mesh,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE,
                        false
                    ),
                    cellMaps,
                    pointMaps,
                    procRefs
                ).write();
            }
            }


            // Reconstruct refinement data
            {
                PtrList<hexRef8Data> procData(procMeshes.meshes().size());

                forAll(procMeshes.meshes(), procI)
                {
                    const fvMesh& procMesh = procMeshes.meshes()[procI];

                    procData.set
                    (
                        procI,
                        new hexRef8Data
                        (
                            IOobject
                            (
                                "dummy",
                                procMesh.time().timeName(),
                                polyMesh::meshSubDir,
                                procMesh,
                                IOobject::READ_IF_PRESENT,
                                IOobject::NO_WRITE,
                                false
                            )
                        )
                    );
                }

                // Combine individual parts

                const PtrList<labelIOList>& cellAddr =
                    procMeshes.cellProcAddressing();

                UPtrList<const labelList> cellMaps(cellAddr.size());
                forAll(cellAddr, i)
                {
                    cellMaps.set(i, &cellAddr[i]);
                }

                const PtrList<labelIOList>& pointAddr =
                    procMeshes.pointProcAddressing();

                UPtrList<const labelList> pointMaps(pointAddr.size());
                forAll(pointAddr, i)
                {
                    pointMaps.set(i, &pointAddr[i]);
                }

                UPtrList<const hexRef8Data> procRefs(procData.size());
                forAll(procData, i)
                {
                    procRefs.set(i, &procData[i]);
                }

                hexRef8Data
                (
                    IOobject
                    (
                        "dummy",
                        mesh.time().timeName(),
                        polyMesh::meshSubDir,
                        mesh,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE,
                        false
                    ),
                    cellMaps,
                    pointMaps,
                    procRefs
                ).write();
            }

            // If there is a "uniform" directory in the time region
            // directory copy from the master processor
            {
                fileName uniformDir0
                (
                    fileHandler().filePath
                    (
                        databases[0].timePath()/regionDir/"uniform"
                    )
                );

                if (!uniformDir0.empty() && fileHandler().isDir(uniformDir0))
                {
                    fileHandler().cp(uniformDir0, runTime.timePath()/regionDir);
                }
            }

            // For the first region of a multi-region case additionally
            // copy the "uniform" directory in the time directory
            if (regioni == 0 && regionDir != word::null)
            {
                fileName uniformDir0
                (
                    fileHandler().filePath
                    (
                        databases[0].timePath()/"uniform"
                    )
                );

                if (!uniformDir0.empty() && fileHandler().isDir(uniformDir0))
                {
                    fileHandler().cp(uniformDir0, runTime.timePath());
                }
            }
        }
    }

    Info<< "\nEnd\n" << endl;

    return 0;
}


// ************************************************************************* //
