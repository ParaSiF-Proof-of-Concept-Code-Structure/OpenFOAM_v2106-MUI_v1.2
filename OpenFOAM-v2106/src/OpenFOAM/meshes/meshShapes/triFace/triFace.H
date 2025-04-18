/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017-2021 OpenCFD Ltd.
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
    Foam::triFace

Description
    A triangular face using a FixedList of labels corresponding to mesh
    vertices.

See also
    Foam::face, Foam::triangle

SourceFiles
    triFaceI.H
    triFaceTemplates.C

\*---------------------------------------------------------------------------*/

#ifndef triFace_H
#define triFace_H

#include "FixedList.H"
#include "edgeList.H"
#include "pointHit.H"
#include "intersection.H"
#include "pointField.H"
#include "triPointRef.H"
#include "ListListOps.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class face;
class triFace;

inline bool operator==(const triFace& a, const triFace& b);
inline bool operator!=(const triFace& a, const triFace& b);

/*---------------------------------------------------------------------------*\
                          Class triFace Declaration
\*---------------------------------------------------------------------------*/

class triFace
:
    public FixedList<label, 3>
{
public:

    // Constructors

        //- Default construct, with invalid point labels (-1)
        inline triFace();

        //- Construct from three point labels
        inline triFace
        (
            const label a,
            const label b,
            const label c
        );

        //- Construct from an initializer list of three point labels
        inline explicit triFace(std::initializer_list<label> list);

        //- Copy construct from a list of three point labels.
        inline explicit triFace(const labelUList& list);

        //- Copy construct from a subset of point labels
        inline triFace
        (
            const labelUList& list,
            const FixedList<label, 3>& indices
        );

        //- Construct from Istream
        inline triFace(Istream& is);


    // Member Functions

        //- 'Collapse' face by marking duplicate point labels.
        //  Duplicates point labels are marked with '-1'
        //  (the lower vertex is retained).
        //  Return the collapsed size.
        inline label collapse();

        //- Flip the face in-place.
        //  The starting points of the original and flipped face are identical.
        inline void flip();

        //- Return the points corresponding to this face
        inline pointField points(const UList<point>& pts) const;

        //- Return triangle as a face
        inline face triFaceFace() const;

        //- Return the triangle
        inline triPointRef tri(const UList<point>& points) const;

        //- Return centre (centroid)
        inline point centre(const UList<point>& points) const;

        //- Calculate average value at centroid of face
        template<class Type>
        Type average(const UList<point>& unused, const Field<Type>& fld) const;

        //- The area normal - with magnitude equal to area of face
        inline vector areaNormal(const UList<point>& points) const;

        //- The unit normal
        inline vector unitNormal(const UList<point>& points) const;

        //- Legacy name for areaNormal()
        //  \deprecated(2018-06) Deprecated for new use
        FOAM_DEPRECATED_FOR(2018-12, "areaNormal() or unitNormal()")
        vector normal(const UList<point>& points) const
        {
            return areaNormal(points); // Legacy definition
        }

        //- Magnitude of face area
        inline scalar mag(const UList<point>& points) const;

        //- Number of triangles after splitting == 1
        inline label nTriangles() const noexcept;

        //- Return face with reverse direction
        //  The starting points of the original and reverse face are identical.
        inline triFace reverseFace() const;

        //- Find local index on face for the point label, same as find()
        //  \return position in face (0,1,2) or -1 if not found.
        inline label which(const label pointLabel) const;

        //- Next vertex on face
        inline label nextLabel(const label i) const;

        //- Previous vertex on face
        inline label prevLabel(const label i) const;

        //- The vertex on face - identical to operator[], but with naming
        //- similar to nextLabel(), prevLabel()
        inline label thisLabel(const label i) const;

        //- Return swept-volume from old-points to new-points
        inline scalar sweptVol
        (
            const UList<point>& opts,
            const UList<point>& npts
        ) const;

        //- Return the inertia tensor, with optional reference
        //  point and density specification
        inline tensor inertia
        (
            const UList<point>& points,
            const point& refPt = vector::zero,
            scalar density = 1.0
        ) const;

        //- Return point intersection with a ray starting at p, in direction q.
        inline pointHit ray
        (
            const point& p,
            const vector& q,
            const UList<point>& points,
            const intersection::algorithm = intersection::FULL_RAY,
            const intersection::direction dir = intersection::VECTOR
        ) const;

        //- Fast intersection with a ray.
        inline pointHit intersection
        (
            const point& p,
            const vector& q,
            const UList<point>& points,
            const intersection::algorithm alg,
            const scalar tol = 0.0
        ) const;

        inline pointHit intersection
        (
            const point& p,
            const vector& q,
            const point& ctr,
            const UList<point>& points,
            const intersection::algorithm alg,
            const scalar tol = 0.0
        ) const;

        //- Return nearest point to face
        inline pointHit nearestPoint
        (
            const point& p,
            const UList<point>& points
        ) const;


        //- Return nearest point to face and classify it:
        //  + near point (nearType=POINT, nearLabel=0, 1, 2)
        //  + near edge (nearType=EDGE, nearLabel=0, 1, 2)
        //    Note: edges are counted from starting vertex so
        //    e.g. edge n is from f[n] to f[0], where the face has n + 1
        //    points
        inline pointHit nearestPointClassify
        (
            const point& p,
            const UList<point>& points,
            label& nearType,
            label& nearLabel
        ) const;

        //- The sign for which side of the face plane the point is on.
        //  Uses the supplied tolerance for rounding around zero.
        //  \return
        //  -  0: on plane
        //  - +1: front-side
        //  - -1: back-side
        inline int sign
        (
            const point& p,
            const UList<point>& points,
            const scalar tol = SMALL
        ) const;

        //- Return number of edges == 3
        inline label nEdges() const noexcept;

        //- Return i-th face edge in forward walk order.
        //- The faceEdge(0) is the edge between [0] and [1]
        inline Foam::edge faceEdge(const label edgei) const;

        //- Return i-th face edge in forward walk order.
        //- Identical to faceEdge() but with generic name
        inline Foam::edge edge(const label edgei) const;

        //- Return vector of i-th face edge in forward walk order.
        inline vector edge(const label edgei, const UList<point>& pts) const;

        //- Return i-th face edge in reverse walk order.
        //- The rcEdge(0) is the edge between [0] to [n-1]
        inline Foam::edge rcEdge(const label edgei) const;

        //- Return vector of i-th face edge in reverse walk order.
        inline vector rcEdge(const label edgei, const UList<point>& pts) const;

        //- Return list of edges in forward walk order.
        //  i.e. edges()[0] is edge between [0] and [1]
        inline edgeList edges() const;

        //- Return list of edges in reverse walk order.
        //  i.e. rcEdges()[0] is edge between [0] to [n-1]
        inline edgeList rcEdges() const;

        //- Test the edge direction on the face
        //  \return
        //  - +1: forward (counter-clockwise) on the face
        //  - -1: reverse (clockwise) on the face
        //  -  0: edge not found on the face
        inline int edgeDirection(const Foam::edge& e) const;

        //- Compare triFaces
        //  \return:
        //  -  0: different
        //  - +1: identical
        //  - -1: same face, but different orientation
        static inline int compare(const triFace& a, const triFace& b);


    // Hashing

        //- The (commutative) hash value for triFace
        inline unsigned hash_code(unsigned seed=0) const
        {
            // Fortunately we don't need this very often
            const uLabel t0((*this)[0]);
            const uLabel t1((*this)[1]);
            const uLabel t2((*this)[2]);

            const uLabel val(t0*t1*t2 + t0+t1+t2);

            return Foam::Hash<uLabel>()(val, seed);
        }

        //- Hashing functor for triFace (commutative)
        struct hasher
        {
            unsigned operator()(const triFace& obj, unsigned seed=0) const
            {
                return obj.hash_code(seed);
            }
        };

        //- Deprecated(2021-04) hashing functor. Use hasher()
        // \deprecated(2021-04) - use hasher() functor
        template<class Unused=bool>
        struct Hash : triFace::hasher
        {
            FOAM_DEPRECATED_FOR(2021-04, "hasher()") Hash() {}
        };
};


// * * * * * * * * * * * * * * * * * Traits  * * * * * * * * * * * * * * * * //

//- Contiguous data for triFace
template<> struct is_contiguous<triFace> : std::true_type {};

//- Contiguous label data for triFace
template<> struct is_contiguous_label<triFace> : std::true_type {};

//- Hashing for triFace uses commutative (incremental) hash
template<> struct Hash<triFace> : triFace::hasher {};


//- Specialization to offset faces, used in ListListOps::combineOffset
template<>
struct offsetOp<triFace>
{
    inline triFace operator()
    (
        const triFace& x,
        const label offset
    ) const
    {
        triFace result;

        forAll(x, i)
        {
            result[i] = x[i] + offset;
        }
        return result;
    }
};


// * * * * * * * * * * * * * * * Global Operators  * * * * * * * * * * * * * //

inline bool operator==(const triFace& a, const triFace& b);
inline bool operator!=(const triFace& a, const triFace& b);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "triFaceI.H"

#ifdef NoRepository
    #include "triFaceTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
