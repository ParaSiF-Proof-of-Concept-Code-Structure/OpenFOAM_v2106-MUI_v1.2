/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2018-2020 OpenCFD Ltd.
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

Namespace
    Foam::coordSystem

Description
    Namespace for coordinate systems.

Class
    Foam::coordinateSystem

Description
    Base class for coordinate system specification,
    the default coordinate system type is
    \link coordSystem::cartesian cartesian \endlink.

    All systems are defined by an origin point and a coordinate rotation
    By default, the \link coordinateRotations::axes axes \endlink
    specification can be used directly as part of the
    coordinate system specification. For example,

    \verbatim
    coordinateSystem
    {
        origin  (0 0 0);
        e1      (0 1 0);
        e3      (1 0 0);
    }
    \endverbatim

    The same, but in more verbose format:
    \verbatim
    coordinateSystem
    {
        type    cartesian;
        origin  (0 0 0);
        rotation
        {
            type    axes;
            e1      (0 1 0);
            e3      (1 0 0);
        }
    }
    \endverbatim

    Types of coordinateRotation:
      -# \link coordinateRotations::identity none \endlink
      -# \link coordinateRotations::axes axes \endlink
      -# \link coordinateRotations::axisAngle axisAngle \endlink
      -# \link coordinateRotations::euler euler \endlink
      -# \link coordinateRotations::starcd starcd \endlink

    Type of coordinateSystem:
      -# \link coordSystem::cartesian cartesian \endlink
      -# \link coordSystem::cylindrical cylindrical \endlink
      -# \link coordSystem::indirect indirect \endlink (references
      an entry in coordinateSystems).

SourceFiles
    coordinateSystem.C
    coordinateSystemNew.C
    coordinateSystemTransform.C

\*---------------------------------------------------------------------------*/

#ifndef coordinateSystem_H
#define coordinateSystem_H

#include "vector.H"
#include "point.H"
#include "tensor.H"
#include "vectorField.H"
#include "pointField.H"
#include "tensorField.H"
#include "pointIndList.H"
#include "coordinateRotation.H"
#include "autoPtr.H"
#include "tmp.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class coordinateSystem;
class objectRegistry;

namespace coordSystem
{
class indirect;
}


/*---------------------------------------------------------------------------*\
                      Class coordinateSystem Declaration
\*---------------------------------------------------------------------------*/

class coordinateSystem
{
    // Private Member Functions

        //- Use 'coordinateSystem' sub-dictionary if present
        static const dictionary* subDictCompat(const dictionary* dictPtr);


protected:

    //- Friendship with indirect for dispatching to its underlying system
    friend coordSystem::indirect;


    // Protected Data

        //- User specification of the coordinate rotation
        //  May be invalid after a move assignment or transfer
        autoPtr<coordinateRotation> spec_;

        //- The coordinate system origin
        point origin_;

        //- The rotation tensor
        tensor rot_;

        //- The name of the coordinate system (optional)
        word name_;

        //- An optional note describing the coordinate system
        string note_;

        //- Dummy coordinate system for suppressed manipulation
        static coordinateSystem dummy_;


    // Protected Member Functions

        //- Implementation for R() methods
        template<class PointField>
        tmp<tensorField> rotationsImpl(const PointField& global) const;

        //- Implementation for transformPoint() methods
        template<class PointField>
        tmp<pointField> transformPointImpl(const PointField& localCart) const;

        //- Implementation for transformPosition() methods
        template<class PointField>
        tmp<pointField> invTransformPointImpl(const PointField& global) const;

        //- Apply single transform tensor for multiple inputs
        template<class RetType, class Type, class BinaryOp>
        static tmp<Field<RetType>> manyTimesImpl
        (
            const tensor& tt,
            const UList<Type>& input,
            const BinaryOp& bop
        );

        //- Use position-dependent transform tensors for multiple inputs
        template<class RetType, class PointField, class Type, class BinaryOp>
        tmp<Field<RetType>> oneToOneImpl
        (
            const PointField& global,
            const UList<Type>& input,
            const BinaryOp& bop
        ) const;

        //- Use position-dependent transform tensors for single input
        template<class RetType, class PointField, class Type, class BinaryOp>
        tmp<Field<RetType>> oneToManyImpl
        (
            const PointField& global,
            const Type& input,
            const BinaryOp& bop
        ) const;


        //- From local coordinate system to the global Cartesian system
        //- with optional translation for the origin
        virtual vector localToGlobal
        (
            const vector& local,
            bool translate
        ) const;

        //- From local coordinate system to the global Cartesian system
        //- with optional translation for the origin
        virtual tmp<vectorField> localToGlobal
        (
            const vectorField& local,
            bool translate
        ) const;

        //- From global Cartesian system to the local coordinate system
        //- with optional translation for the origin
        virtual vector globalToLocal
        (
            const vector& global,
            bool translate
        ) const;

        //- From global Cartesian system to the local coordinate system
        //- with optional translation for the origin
        virtual tmp<vectorField> globalToLocal
        (
            const vectorField& global,
            bool translate
        ) const;


        //- Assign from dictionary content
        void assign(const dictionary& dict);


    // Constructors

        //- Construct null, without allocating a coordinateRotation
        //- specification.
        coordinateSystem(std::nullptr_t);


public:

    //- Runtime type information
    TypeName("coordinateSystem");

        //- Helper for construction of coordinateSystem PtrList
        //  The Istream contains a word followed by a dictionary
        struct iNew
        {
            autoPtr<coordinateSystem> operator()(Istream& is) const
            {
                return coordinateSystem::New(is);
            }
        };


    // Constructors

        //- Default construct. This is an identity coordinate system
        coordinateSystem();

        //- Copy construct from rotation with origin=0
        explicit coordinateSystem(const coordinateRotation& crot);

        //- Move construct from rotation with origin=0
        explicit coordinateSystem(coordinateRotation&& crot);

        //- Copy construct
        coordinateSystem(const coordinateSystem& csys);

        //- Move construct
        coordinateSystem(coordinateSystem&& csys);

        //- Move construct from autoPtr
        explicit coordinateSystem(autoPtr<coordinateSystem>&& csys);

        //- Copy construct with a different name
        coordinateSystem
        (
            const word& name,
            const coordinateSystem& csys
        );

        //- Construct from origin and rotation
        coordinateSystem
        (
            const point& origin,
            const coordinateRotation& crot
        );

        //- Construct from origin and 2 axes
        coordinateSystem
        (
            const point& origin,
            const vector& axis,
            const vector& dirn
        );

        //- Construct from origin and rotation
        coordinateSystem
        (
            const word& name,
            const point& origin,
            const coordinateRotation& crot
        );

        //- Construct from origin and 2 axes
        coordinateSystem
        (
            const word& name,
            const point& origin,
            const vector& axis,
            const vector& dirn
        );

        //- Construct from dictionary with a given name
        coordinateSystem(const word& name, const dictionary& dict);

        //- Construct from dictionary without a name
        explicit coordinateSystem(const dictionary& dict);

        //- Construct from dictionary with optional subDict lookup.
        //
        //  \param dictName If non-empty, the sub-dictionary to use.
        coordinateSystem(const dictionary& dict, const word& dictName);


    //- Return clone
    virtual autoPtr<coordinateSystem> clone() const
    {
        return autoPtr<coordinateSystem>::New(*this);
    }


    // Declare run-time constructor selection table
    declareRunTimeSelectionTable
    (
        autoPtr,
        coordinateSystem,
        dictionary,
        (
            const dictionary& dict
        ),
        (dict)
    );

    // Declare run-time constructor selection table
    declareRunTimeSelectionTable
    (
        autoPtr,
        coordinateSystem,
        registry,
        (
            const objectRegistry& obr,
            const dictionary& dict
        ),
        (obr, dict)
    );


    // Selectors

        //- Select construct the specified coordinate system type
        //- with reference to objectRegistry for indirect entries.
        //
        //  An empty modelType will be treated as "cartesian"
        static autoPtr<coordinateSystem> New
        (
            word modelType,
            const objectRegistry& obr,
            const dictionary& dict
        );

        //- Select construct the specified coordinate system type
        //
        //  An empty modelType will be treated as "cartesian"
        static autoPtr<coordinateSystem> New
        (
            word modelType,
            const dictionary& dict
        );

        //- Select construct from dictionary with reference to objectRegistry
        //- for indirect entries.
        //
        //  \param dictName If non-empty, the sub-dictionary name to use
        //      for the coordinate system description.
        //
        //  \note When the dictName is empty, it includes an implicit search
        //      for a "coordinateSystem" sub-dictionary for convenience and
        //      compatibility with previous versions (1806 and earlier).
        static autoPtr<coordinateSystem> New
        (
            const objectRegistry& obr,
            const dictionary& dict,
            const word& dictName = ""
        );

        //- Select constructed from dictionary
        //  \param dictName If non-empty, the sub-dictionary name to use
        //      for the coordinate system description.
        //
        //  \note When the dictName is empty, it includes an implicit search
        //      for a "coordinateSystem" sub-dictionary for convenience and
        //      compatibility with previous versions (1806 and earlier).
        static autoPtr<coordinateSystem> New
        (
            const dictionary& dict,
            const word& dictName = ""
        );

        //- Select constructed from Istream
        //  Expects a name/dictionary as input
        static autoPtr<coordinateSystem> New(Istream& is);


    //- Destructor
    virtual ~coordinateSystem() = default;


    // Member Functions

    // Access

        //- Considered valid if it has a specification
        virtual bool valid() const
        {
            return spec_;
        }

        //- True if the rotation tensor is uniform for all locations
        virtual bool uniform() const
        {
            return true;
        }

        //- The rotation specification
        virtual const coordinateRotation& rotation() const
        {
            return *spec_;
        }

        //- Return the name
        virtual const word& name() const
        {
            return name_;
        }

        //- Return the optional note
        virtual const string& note() const
        {
            return note_;
        }

        //- Return origin
        virtual const point& origin() const
        {
            return origin_;
        }

        //- Return const reference to the rotation tensor
        virtual const tensor& R() const
        {
            return rot_;
        }

        //- The local Cartesian x-axis in global coordinates
        virtual const vector e1() const
        {
            return rot_.cx();
        }

        //- The local Cartesian y-axis in global coordinates
        virtual const vector e2() const
        {
            return rot_.cy();
        }

        //- The local Cartesian z-axis in global coordinates
        virtual const vector e3() const
        {
            return rot_.cz();
        }


    // Edit

        //- Rename
        virtual void rename(const word& newName)
        {
            name_ = newName;
        }

        //- Provide non-constant access to the optional note
        virtual string& note()
        {
            return note_;
        }

        //- Edit access to origin
        virtual point& origin()
        {
            return origin_;
        }

        //- Reset origin and rotation to an identity coordinateSystem
        //  Also resets the note
        virtual void clear();

        //- Change the rotation
        virtual void rotation(autoPtr<coordinateRotation>&& crot);


    // Write

        //- Write
        virtual void write(Ostream& os) const;

        //- Write dictionary entry
        virtual void writeEntry(const word& keyword, Ostream& os) const;


    // Member Operators

        //- Copy assignment
        void operator=(const coordinateSystem& csys);

        //- Move assignment
        void operator=(coordinateSystem&& csys);

        //- Copy assignment from autoPtr
        void operator=(const autoPtr<coordinateSystem>& csys);

        //- Move assignment from autoPtr
        void operator=(autoPtr<coordinateSystem>&& csys);


    // Rotation

        //- Position-dependent rotation tensor (when uniform = false)
        //- \return tensor
        virtual tensor R(const point& global) const;

        //- Position-dependent rotation tensors (when uniform = false)
        //- \return tensorField
        virtual tmp<tensorField> R(const UList<point>& global) const;

        //- Position-dependent rotation tensors (when uniform = false)
        //- \return tensorField
        virtual tmp<tensorField> R(const pointUIndList& global) const;


    // Position

        //- Transform point and add origin offset.
        //  Corresponds to a local-to-global transformation using Cartesian
        //  coordinates for both local and global.
        point transformPoint(const point& localCart) const;

        //- Transform points and add origin offset.
        tmp<pointField> transformPoint(const UList<point>& localCart) const;

        //- Transform points and add origin offset.
        tmp<pointField> transformPoint(const pointUIndList& localCart) const;


        //- Remove origin offset and inverse transform point.
        //  Corresponds to a global-to-local transformation using Cartesian
        //  coordinates for both local and global.
        point invTransformPoint(const point& global) const;

        //- Remove origin offset and inverse transform points.
        tmp<pointField> invTransformPoint(const UList<point>& global) const;

        //- Remove origin offset and inverse transform points.
        tmp<pointField> invTransformPoint(const pointUIndList& global) const;


    // Transformations with change of coordinate types

        //- From local coordinate position to global (cartesian) position
        point globalPosition(const point& local) const
        {
            return localToGlobal(local, true);
        }

        //- From local coordinate position to global (cartesian) position
        tmp<pointField> globalPosition(const pointField& local) const
        {
            return localToGlobal(local, true);
        }

        //- From global (cartesian) position to local coordinate position
        point localPosition(const point& global) const
        {
            return globalToLocal(global, true);
        }

        //- From global (cartesian) position to local coordinate position
        tmp<pointField> localPosition(const pointField& global) const
        {
            return globalToLocal(global, true);
        }



        //- From local to global (cartesian) vector components
        vector globalVector(const vector& local) const
        {
            return localToGlobal(local, false);
        }

        //- From local to global (cartesian) vector components
        tmp<vectorField> globalVector(const vectorField& local) const
        {
            return localToGlobal(local, false);
        }

        //- From global (cartesian) to local vector components
        vector localVector(const vector& global) const
        {
            return globalToLocal(global, false);
        }

        //- From global (cartesian) to local vector components
        tmp<vectorField> localVector(const vectorField& global) const
        {
            return globalToLocal(global, false);
        }


    // Transformations (input and output are Cartesian)

#undef  defineCoordinateSystemTransform
#define defineCoordinateSystemTransform(Op, RetType, Type)                    \
                                                                              \
    /*! \brief With constant rotation tensor */                               \
    virtual RetType Op(const Type& input) const;                              \
                                                                              \
    /*! \brief With constant rotation tensor */                               \
    virtual tmp<Field<RetType>> Op(const UList<Type>& input) const;           \
                                                                              \
    /*! \brief With rotation tensor at given global position */               \
    virtual RetType Op(const point& global, const Type& input) const;         \
                                                                              \
    /*! \brief With rotation tensors at given global positions */             \
    virtual tmp<Field<RetType>> Op                                            \
    (                                                                         \
        const UList<point>& global,                                           \
        const Type& input                                                     \
    ) const;                                                                  \
                                                                              \
    /*! \brief With rotation tensors at given global positions */             \
    virtual tmp<Field<RetType>> Op                                            \
    (                                                                         \
        const pointUIndList& global,                                          \
        const Type& input                                                     \
    ) const;                                                                  \
                                                                              \
    /*! \brief With rotation tensors at given global positions */             \
    virtual tmp<Field<RetType>> Op                                            \
    (                                                                         \
        const UList<point>& global,                                           \
        const UList<Type>& input                                              \
    ) const;                                                                  \
                                                                              \
    /*! \brief With rotation tensors at given global positions */             \
    virtual tmp<Field<RetType>> Op                                            \
    (                                                                         \
        const pointUIndList& global,                                          \
        const UList<Type>& input                                              \
    ) const;


    defineCoordinateSystemTransform(transformPrincipal, symmTensor, vector);

    defineCoordinateSystemTransform(transform, scalar, scalar);
    defineCoordinateSystemTransform(transform, vector, vector);
    defineCoordinateSystemTransform
    (
        transform,
        sphericalTensor,
        sphericalTensor
    );
    defineCoordinateSystemTransform(transform, symmTensor, symmTensor);
    defineCoordinateSystemTransform(transform, tensor, tensor);

    defineCoordinateSystemTransform(invTransform, scalar, scalar);
    defineCoordinateSystemTransform(invTransform, vector, vector);
    defineCoordinateSystemTransform
    (
        invTransform,
        sphericalTensor,
        sphericalTensor
    );
    defineCoordinateSystemTransform(invTransform, symmTensor, symmTensor);
    defineCoordinateSystemTransform(invTransform, tensor, tensor);

    #undef defineCoordinateSystemTransform
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Global Operators

//- Compare inequality
bool operator!=(const coordinateSystem& a, const coordinateSystem& b);

//- Output operator
Ostream& operator<<(Ostream& os, const coordinateSystem& csys);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "coordinateSystemTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
