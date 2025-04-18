/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2021 OpenCFD Ltd.
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

#include "faFieldDecomposer.H"
#include "GeometricField.H"
#include "IOobjectList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type, template<class> class PatchField, class GeoMesh>
void Foam::faFieldDecomposer::readFields
(
    const typename GeoMesh::Mesh& mesh,
    const IOobjectList& objects,
    PtrList<GeometricField<Type, PatchField, GeoMesh>>& fields,
    const bool readOldTime
)
{
    typedef GeometricField<Type, PatchField, GeoMesh> GeoField;

    // Search list of objects for fields of type GeoField
    IOobjectList fieldObjects(objects.lookupClass<GeoField>());

    /// // Remove the cellDist field
    /// auto iter = fieldObjects.find("cellDist");
    /// if (iter.found())
    /// {
    ///     fieldObjects.erase(iter);
    /// }

    // Use sorted set of names
    // (different processors might read objects in different order)
    const wordList masterNames(fieldObjects.sortedNames());

    // Construct the fields
    fields.resize(masterNames.size());

    forAll(masterNames, i)
    {
        const IOobject& io = *fieldObjects[masterNames[i]];

        fields.set(i, new GeoField(io, mesh, readOldTime));
    }
}


template<class Mesh, class GeoField>
void Foam::faFieldDecomposer::readFields
(
    const Mesh& mesh,
    const IOobjectList& objects,
    PtrList<GeoField>& fields
)
{
    // Search list of objects for fields of type GeomField
    IOobjectList fieldObjects(objects.lookupClass<GeoField>());

    // Use sorted set of names
    // (different processors might read objects in different order)
    const wordList masterNames(fieldObjects.sortedNames());

    // Construct the fields
    fields.resize(masterNames.size());

    forAll(masterNames, i)
    {
        const IOobject& io = *fieldObjects[masterNames[i]];

        fields.set(i, new GeoField(io, mesh));
    }
}


// ************************************************************************* //
