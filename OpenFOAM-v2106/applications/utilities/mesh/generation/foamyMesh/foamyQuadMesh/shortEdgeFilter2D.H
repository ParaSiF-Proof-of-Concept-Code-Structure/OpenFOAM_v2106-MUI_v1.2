/*--------------------------------*- C++ -*----------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
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
    Foam::shortEdgeFilter2D

Description
    This class filters short edges generated by the CV2D mesher.

SourceFiles
    shortEdgeFilter2D.C

\*---------------------------------------------------------------------------*/

#ifndef shortEdgeFilter2D_H
#define shortEdgeFilter2D_H

#include "MeshedSurfaces.H"
#include "CV2D.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class shortEdgeFilter2D Declaration
\*---------------------------------------------------------------------------*/

class shortEdgeFilter2D
{
    // Private Data

        const CV2D& cv2Dmesh_;

        MeshedSurface<face> ms_;

        const scalar shortEdgeFilterFactor_;

        const scalar edgeAttachedToBoundaryFactor_;

        wordList patchNames_;

        labelList patchSizes_;

        EdgeMap<label> mapEdgesRegion_;

        EdgeMap<label> indirectPatchEdge_;


    // Private Member Functions

        void assignBoundaryPointRegions
        (
            List<DynamicList<label>>& boundaryPointRegions
        ) const;

        void updateEdgeRegionMap
        (
            const MeshedSurface<face>& surfMesh,
            const List<DynamicList<label>>& boundaryPtRegions,
            const labelList& surfPtToBoundaryPt,
            EdgeMap<label>& mapEdgesRegion,
            labelList& patchSizes
        ) const;

        //- No copy construct
        shortEdgeFilter2D(const shortEdgeFilter2D&) = delete;

        //- No copy assignment
        void operator=(const shortEdgeFilter2D&) = delete;


public:

    //- Runtime type information
    ClassName("shortEdgeFilter2D");

    // Constructors

        shortEdgeFilter2D(const CV2D& cv2Dmesh, const dictionary& dict);


    //- Destructor
    ~shortEdgeFilter2D();


    // Access Functions

        const wordList& patchNames() const
        {
            return patchNames_;
        }

        const labelList& patchSizes() const
        {
            return patchSizes_;
        }

        const EdgeMap<label>& mapEdgesRegion() const
        {
            return mapEdgesRegion_;
        }

        const MeshedSurface<face>& fMesh() const
        {
            return ms_;
        }


    // Member Functions

        void filter();

        void writeInfo(Ostream& os);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
