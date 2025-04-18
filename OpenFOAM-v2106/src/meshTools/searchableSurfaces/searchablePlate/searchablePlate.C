/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
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

#include "searchablePlate.H"
#include "addToRunTimeSelectionTable.H"
#include "SortableList.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(searchablePlate, 0);
    addToRunTimeSelectionTable
    (
        searchableSurface,
        searchablePlate,
        dict
    );
    addNamedToRunTimeSelectionTable
    (
        searchableSurface,
        searchablePlate,
        dict,
        plate
    );
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::direction Foam::searchablePlate::calcNormal(const point& span)
{
    direction normalDir = 3;

    for (direction dir = 0; dir < vector::nComponents; ++dir)
    {
        if (span[dir] < 0)
        {
            // Negative entry. Flag and exit.
            normalDir = 3;
            break;
        }
        else if (span[dir] < VSMALL)
        {
            if (normalDir != 3)
            {
                // Multiple zero entries. Flag and exit.
                normalDir = 3;
                break;
            }

            normalDir = dir;
        }
    }

    if (normalDir == 3)
    {
        FatalErrorInFunction
            << "Span should have two positive and one zero entry: "
            << span << nl
            << exit(FatalError);
    }

    return normalDir;
}


// Returns miss or hit with face (always 0)
Foam::pointIndexHit Foam::searchablePlate::findNearest
(
    const point& sample,
    const scalar nearestDistSqr
) const
{
    // For every component direction can be
    // left of min, right of max or inbetween.
    // - outside points: project first one x plane (either min().x()
    // or max().x()), then onto y plane and finally z. You should be left
    // with intersection point
    // - inside point: find nearest side (compare to mid point). Project onto
    //   that.

    // Project point on plane.
    pointIndexHit info(true, sample, 0);
    info.rawPoint()[normalDir_] = origin_[normalDir_];

    // Clip to edges if outside
    for (direction dir = 0; dir < vector::nComponents; ++dir)
    {
        if (dir != normalDir_)
        {
            if (info.rawPoint()[dir] < origin_[dir])
            {
                info.rawPoint()[dir] = origin_[dir];
            }
            else if (info.rawPoint()[dir] > origin_[dir]+span_[dir])
            {
                info.rawPoint()[dir] = origin_[dir]+span_[dir];
            }
        }
    }

    // Check if outside. Optimisation: could do some checks on distance already
    // on components above
    if (magSqr(info.rawPoint() - sample) > nearestDistSqr)
    {
        info.setMiss();
        info.setIndex(-1);
    }

    return info;
}


Foam::pointIndexHit Foam::searchablePlate::findLine
(
    const point& start,
    const point& end
) const
{
    pointIndexHit info
    (
        true,
        Zero,
        0
    );

    const vector dir(end-start);

    if (mag(dir[normalDir_]) < VSMALL)
    {
        info.setMiss();
        info.setIndex(-1);
    }
    else
    {
        scalar t = (origin_[normalDir_]-start[normalDir_]) / dir[normalDir_];

        if (t < 0 || t > 1)
        {
            info.setMiss();
            info.setIndex(-1);
        }
        else
        {
            info.rawPoint() = start+t*dir;
            info.rawPoint()[normalDir_] = origin_[normalDir_];

            // Clip to edges
            for (direction dir = 0; dir < vector::nComponents; ++dir)
            {
                if (dir != normalDir_)
                {
                    if (info.rawPoint()[dir] < origin_[dir])
                    {
                        info.setMiss();
                        info.setIndex(-1);
                        break;
                    }
                    else if (info.rawPoint()[dir] > origin_[dir]+span_[dir])
                    {
                        info.setMiss();
                        info.setIndex(-1);
                        break;
                    }
                }
            }
        }
    }

    // Debug
    if (info.hit())
    {
        treeBoundBox bb(origin_, origin_+span_);
        bb.min()[normalDir_] -= 1e-6;
        bb.max()[normalDir_] += 1e-6;

        if (!bb.contains(info.hitPoint()))
        {
            FatalErrorInFunction
                << "bb:" << bb << endl
                << "origin_:" << origin_ << endl
                << "span_:" << span_ << endl
                << "normalDir_:" << normalDir_ << endl
                << "hitPoint:" << info.hitPoint()
                << abort(FatalError);
        }
    }

    return info;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::searchablePlate::searchablePlate
(
    const IOobject& io,
    const point& origin,
    const vector& span
)
:
    searchableSurface(io),
    origin_(origin),
    span_(span),
    normalDir_(calcNormal(span_))
{
    DebugInFunction
        << " origin:" << origin_
        << " origin+span:" << origin_+span_
        << " normal:" << vector::componentNames[normalDir_] << nl;

    bounds() = boundBox(origin_, origin_ + span_);
}


Foam::searchablePlate::searchablePlate
(
    const IOobject& io,
    const dictionary& dict
)
:
    searchablePlate(io, dict.get<point>("origin"), dict.get<vector>("span"))
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

const Foam::wordList& Foam::searchablePlate::regions() const
{
    if (regions_.empty())
    {
        regions_.resize(1);
        regions_.first() = "region0";
    }
    return regions_;
}


Foam::tmp<Foam::pointField> Foam::searchablePlate::coordinates() const
{
    return tmp<pointField>::New(1, origin_ + 0.5*span_);
}


void Foam::searchablePlate::boundingSpheres
(
    pointField& centres,
    scalarField& radiusSqr
) const
{
    centres.resize(1);
    radiusSqr.resize(1);

    centres[0] = origin_ + 0.5*span_;
    radiusSqr[0] = Foam::magSqr(0.5*span_);

    // Add a bit to make sure all points are tested inside
    radiusSqr += Foam::sqr(SMALL);
}


Foam::tmp<Foam::pointField> Foam::searchablePlate::points() const
{
    auto tpts = tmp<pointField>::New(4, origin_);
    auto& pts = tpts.ref();

    pts[2] += span_;

    if (span_.x() < span_.y() && span_.x() < span_.z())
    {
        pts[1].y() += span_.y();
        pts[3].z() += span_.z();
    }
    else if (span_.y() < span_.z())
    {
        pts[1].x() += span_.x();
        pts[3].z() += span_.z();
    }
    else
    {
        pts[1].x() += span_.x();
        pts[3].y() += span_.y();
    }

    return tpts;
}


bool Foam::searchablePlate::overlaps(const boundBox& bb) const
{
    return bb.overlaps(bounds());
}


void Foam::searchablePlate::findNearest
(
    const pointField& samples,
    const scalarField& nearestDistSqr,
    List<pointIndexHit>& info
) const
{
    info.setSize(samples.size());

    forAll(samples, i)
    {
        info[i] = findNearest(samples[i], nearestDistSqr[i]);
    }
}


void Foam::searchablePlate::findLine
(
    const pointField& start,
    const pointField& end,
    List<pointIndexHit>& info
) const
{
    info.setSize(start.size());

    forAll(start, i)
    {
        info[i] = findLine(start[i], end[i]);
    }
}


void Foam::searchablePlate::findLineAny
(
    const pointField& start,
    const pointField& end,
    List<pointIndexHit>& info
) const
{
    findLine(start, end, info);
}


void Foam::searchablePlate::findLineAll
(
    const pointField& start,
    const pointField& end,
    List<List<pointIndexHit>>& info
) const
{
    List<pointIndexHit> nearestInfo;
    findLine(start, end, nearestInfo);

    info.setSize(start.size());
    forAll(info, pointi)
    {
        if (nearestInfo[pointi].hit())
        {
            info[pointi].setSize(1);
            info[pointi][0] = nearestInfo[pointi];
        }
        else
        {
            info[pointi].clear();
        }
    }
}


void Foam::searchablePlate::getRegion
(
    const List<pointIndexHit>& info,
    labelList& region
) const
{
    region.setSize(info.size());
    region = 0;
}


void Foam::searchablePlate::getNormal
(
    const List<pointIndexHit>& info,
    vectorField& normal
) const
{
    normal.setSize(info.size());
    normal = Zero;
    forAll(normal, i)
    {
        normal[i][normalDir_] = 1.0;
    }
}


void Foam::searchablePlate::getVolumeType
(
    const pointField& points,
    List<volumeType>& volType
) const
{
    FatalErrorInFunction
        << "Volume type not supported for plate."
        << exit(FatalError);
}


// ************************************************************************* //
