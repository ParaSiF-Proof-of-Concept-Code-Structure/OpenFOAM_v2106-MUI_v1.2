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
    Foam::PackedList

Description
    A dynamic list of packed unsigned integers, with the number of bits
    per item specified by the \<Width\> template parameter.

    Resizing is similar to DynamicList so that clear() and resize() affect
    the addressed size, but not the allocated size. The reserve() and
    setCapacity() methods can be used to influence the allocation.

Note
    In a const context, the '[]' operator simply returns the stored value,
    with out-of-range elements returned as zero.

    In a non-const context, the '[]' operator returns a reference to an
    existing value. When accessing out-of-range elements, some caution
    is required to ensure that the const version of the [] operator is actually
    being called.
    The get() method is functionally identical the the '[]' operator, but
    is always const access.

    The set() and unset() methods return a bool if the value changed.

    With const access, the get() method and 'operator[]' are identical.
    With non-const access, the 'operator[]' may be marginally slower get().

    The set() method may be marginally faster than using the 'operator[]'
    supports auto-vivification and also returns a bool if the value changed,
    which can be useful for branching on changed values.

    \code
        list.set(5, 4);
        changed = list.set(5, 8);
        if (changed) ...
    \endcode

    In a const context, reading an out-of-range element returns zero without
    affecting the list size.
    For example,
    \code
        list.resize(4);
        Info<< list.get(10) << "\n";    // print zero, but doesn't adjust list
        list.set(8);                    // auto-vivify
    \endcode

    Also note that all unused internal storage elements are guaranteed to
    always be bit-wise zero. This property must not be violated by any
    inheriting classes.

Note
    Iterators for this class have been intentionally removed, for performance
    reasons.

See also
    Foam::BitOps
    Foam::DynamicList

SourceFiles
    PackedListI.H
    PackedList.C
    PackedListIO.C

\*---------------------------------------------------------------------------*/

#ifndef PackedList_H
#define PackedList_H

#include "BitOps.H"
#include "labelList.H"
#include "IndirectListBase.H"
#include "InfoProxy.H"
#include "PackedListCore.H"

#include <type_traits>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
template<unsigned Width> class PackedList;
class labelRange;

template<unsigned Width>
Istream& operator>>(Istream& is, PackedList<Width>& list);

template<unsigned Width>
Ostream& operator<<(Ostream& os, const PackedList<Width>& list);

template<unsigned Width>
Ostream& operator<<(Ostream& os, const InfoProxy<PackedList<Width>>& info);


/*---------------------------------------------------------------------------*\
                         Class PackedList Declaration
\*---------------------------------------------------------------------------*/

template<unsigned Width>
class PackedList
:
    public Detail::PackedListCore
{
public:

    // Types and dimension information

        //- The storage block type for bit elements
        //  \note Type name compatibility with boost::dynamic_bitset
        typedef unsigned int block_type;

        //- The number of bits in a single block
        //  \note Type name compatibility with boost::dynamic_bitset
        static constexpr unsigned bits_per_block
            = (std::numeric_limits<block_type>::digits);

        //- The width of an individual element (in bits).
        static constexpr unsigned element_width = (Width);

        //- The number of elements stored per data block.
        static constexpr unsigned elem_per_block = (bits_per_block / Width);

        //- The max value for an element which is also the bit-mask of the
        //- individual element.
        //  Eg, for Width=2: ((1 << 2) - 1) == 0b0011
        static constexpr block_type max_value = ((1u << Width) - 1);

        //- Calculate the number of blocks required to _address_ the
        //- requested number of elements.
        //
        // We calculate this:
        // \code
        //     (numElem / elem_per_block)
        //   + (numElem % elem_per_block) ? 1 : 0
        // \endcode
        // But avoiding the modulus operation
        static constexpr label num_blocks(label numElem) noexcept
        {
            return ((numElem - 1 + elem_per_block) / elem_per_block);
        }

        //- Masking for all bits below the element offset.
        //  Ill-defined when elementOffset is out of range.
        static constexpr block_type mask_lower(unsigned elementOffset)
        {
            return (~0u >> (bits_per_block - Width * elementOffset));
        }


protected:

    // Protected Data

        //- The internal container for storing the blocks
        typedef List<block_type> block_container;

        //- The blocks of raw data
        block_container blocks_;

        //- Number of entries used
        label size_;

        //- Enforce non-zero Width to fit within the block storage and require
        //- at least 2 items per storage block for general efficiency.
        //
        //  Thus 1/2 of the base storage size is (sizeof(block_type)*8/2),
        //  or (sizeof(block_type) << 2)
        static_assert
        (
            Width && Width <= (sizeof(block_type) << 2),
            "Width must be > 0 and minimum of two items per data block"
        );


    // Protected Member Functions

        //- A fill value for complete blocks
        inline static unsigned int repeated_value(unsigned val);

        //- Read a list entry (allows for specialization)
        inline static unsigned int readValue(Istream& is);

        //- Read an index/value pair and set accordingly.
        //  For bool specialization, read a single index value
        inline void setPair(Istream& is);

        //- Write as a dictionary entry
        void writeEntry(Ostream& os) const;

        //- Clear any partial rubbish in the last addressable block
        //  This \a rubbish may have arisen from block-wise operations etc.
        inline void clear_trailing_bits();

        //- Copy assignment
        inline void copyAssign(const PackedList<Width>& rhs);


public:

    // Forward declaration of access classes

        class reference;
        typedef unsigned int const_reference;


    // Constructors

        //- Default construct, zero-sized and no allocation
        inline constexpr PackedList() noexcept;

        //- Construct for given number of elements, initializes values to 0
        inline explicit PackedList(const label numElem);

        //- Construct for given number of elements, and the specified
        //- value for each element
        inline PackedList(const label numElem, const unsigned int val);

        //- Construct from Istream
        inline PackedList(Istream& is);

        //- Copy construct
        inline PackedList(const PackedList<Width>& list);

        //- Move construct
        inline PackedList(PackedList<Width>&& list);

        //- Copy construct a subset
        PackedList(const PackedList<Width>& list, const labelUList& addr);

        //- Copy construct a subset
        template<class Addr>
        PackedList
        (
            const PackedList<Width>& list,
            const IndirectListBase<label, Addr>& addr
        );

        //- Copy construct a subset range
        PackedList(const PackedList<Width>& list, const labelRange& range);

        //- Construct from a list of values
        inline explicit PackedList(const labelUList& values);

        //- Construct from a indirect list of values
        template<class Addr>
        inline explicit PackedList(const IndirectListBase<label, Addr>& values);

        //- Clone
        inline autoPtr<PackedList<Width>> clone() const;


    // Member Functions

    // Query

        //- Check index is within valid range [0,size)
        inline void checkIndex(const label i) const;

        //- Number of entries.
        inline label size() const noexcept;

        //- True if the list is empty (ie, size() is zero).
        inline bool empty() const noexcept;

        //- The number of elements that can be stored with reallocating
        inline label capacity() const;

        //- True if all entries have identical values, and list is non-empty
        bool uniform() const;

        //- Test for equality of sizes and the bits set
        bool equal(const PackedList<Width>& other) const;


    // Access

        //- Get value at index i or 0 for out-of-range.
        //  Never auto-vivify entries.
        inline unsigned int get(const label i) const;

        //- Set value at index i, default value set is the max_value.
        //  Does auto-vivify for non-existent, non-zero entries.
        //  \return true if value changed.
        inline bool set(const label i, unsigned int val = ~0u);

        //- Unset the entry at index i.
        //  Never auto-vivify entries.
        //  \return true if the value changed.
        inline bool unset(const label i);

        //- Return the values as a list of labels
        labelList values() const;

        //- Return the values as a list of integral type.
        //  The default integral type is unsigned int.
        template<class IntType = unsigned int>
        List<IntType> unpack() const;

        //- Return the range of values as a list of integral type.
        //  The default integral type is unsigned int.
        template<class IntType = unsigned int>
        List<IntType> unpack(const labelRange& range) const;

        //- Extract the values for the specified locations as
        //- a list of integral type.
        //  The default integral type is unsigned int.
        template<class IntType = unsigned int>
        List<IntType> unpack(const labelUList& locations) const;


    // Edit

        //- Assign all entries to the given value.
        inline void fill(const unsigned int val);

        //- Trim any trailing zero elements, optionally specifying a
        //- a minimum position, below which trimming will not occur.
        //
        //  \return true if trimming changed the size.
        inline bool trim(label minpos=-1);

        //- Clear all bits but do not adjust the addressable size.
        //  \note Method name compatibility with boost::dynamic_bitset
        inline void reset();

        //- Alter the size of the underlying storage.
        //  The addressed size will be truncated if needed to fit, but will
        //  remain otherwise untouched.
        inline void setCapacity(const label nElem);

        //- Reset addressable list size, does not shrink the allocated size.
        //  Optionally specify a value for new elements.
        inline void resize(const label nElem, const unsigned int val = 0u);

        //- Reserve allocation space for at least this size.
        //  Never shrinks the allocated size.
        //  The list size is adjusted as per DynamicList with
        //  SizeInc=0, SizeMult=2, SizeDiv=1
        inline void reserve(const label nElem);

        //- Clear the list, i.e. set addressable size to zero.
        //  Does not adjust the underlying storage
        inline void clear();

        //- Clear the list and delete storage.
        inline void clearStorage();

        //- Shrink the allocated space to what is actually used.
        inline void shrink();

        //- Swap contents with argument
        inline void swap(PackedList<Width>& rhs);

        //- Transfer the contents of the argument list into this list
        //- and annul the argument list.
        inline void transfer(PackedList<Width>& rhs);


    // Low-level access

        //- The number of internal storage blocks
        inline label nBlocks() const;

        //- Return the underlying storage blocks
        inline const List<unsigned int>& storage() const;

        //- Return the underlying storage blocks
        //  Manipulate with utmost caution
        inline List<unsigned int>& storage();

        //- A const pointer to the raw storage
        inline const unsigned int* cdata() const noexcept;

        //- A pointer to the raw storage
        inline unsigned int* data() noexcept;

        //- The number of bytes used in the raw storage
        //- including any unused padding.
        inline std::streamsize size_bytes() const noexcept;

        //- The number of bytes used in the raw storage
        //- including any unused padding.
        inline std::streamsize byteSize() const noexcept;


    // IO

        //- Print bit patterns, optionally with extra debug
        Ostream& printBits(Ostream& os, bool debugOutput=false) const;

        //- Clear list and read from stream
        Istream& readList(Istream& is);

        //- Write List, with line-breaks in ASCII when length exceeds shortLen.
        //  Using '0' suppresses line-breaks entirely.
        Ostream& writeList(Ostream& os, const label shortLen=0) const;

        //- Write as a dictionary entry with keyword
        void writeEntry(const word& keyword, Ostream& os) const;


    // Member Operators

        //- Append a value at the end of the list
        inline PackedList<Width>& append(const unsigned int val);

        //- Remove and return the last element
        inline unsigned int remove();

        //- Identical to get() - get value at index.
        //  Never auto-vivify entries.
        inline unsigned int operator[](const label i) const;

        //- Non-const access to value at index.
        //  Fatal for out-of-range indices
        inline reference operator[](const label i);

        //- Copy assignment.
        inline void operator=(const PackedList<Width>& lst);

        //- Move assignment.
        inline void operator=(PackedList<Width>&& lst);

        //- Assign all entries to the given value. fill()
        inline void operator=(const unsigned int val);


    // Access helpers

        //- A reference supporting read/write access to an entry
        class reference
        {
        protected:

            friend class PackedList;    // Access for parent
            void operator&() = delete;  // Refuse to provide its address

            //- Reference to the block
            block_type& ref_;

            //- The bit shift to access the given sub-portion
            unsigned shift_;

            //- Construct by taking reference of block from within
            //- the list and the specified index.
            inline reference(PackedList* parent, const label index);

            //- Get value as unsigned, no range-checking
            inline unsigned int get() const;

            //- Set value, returning true if changed, no range-checking
            inline bool set(unsigned int val);

        public:

            //- Copy construct
            reference(const reference&) = default;

            //- Move construct
            reference(reference&&) = default;

            //- Value assignment
            inline void operator=(const reference& other);

            //- Value assignment
            inline void operator=(const unsigned int val);

            //- Conversion operator.
            inline operator unsigned int () const;
        };


    // IOstream Operators

        //- Return info proxy.
        InfoProxy<PackedList<Width>> info() const
        {
            return *this;
        }

        friend Ostream& operator<< <Width>
        (
            Ostream& os,
            const InfoProxy<PackedList<Width>>& info
        );

        friend Istream& operator>> <Width>
        (
            Istream& is,
            PackedList<Width>& list
        );


    // Hashing

        //- Hashing functor for PackedList
        //  Seeded with logical size for disambiguation of padding
        struct hasher
        {
            unsigned operator()(const PackedList<Width>& obj) const
            {
                return Foam::Hasher
                (
                    obj.cdata(),
                    obj.size_bytes(),
                    unsigned(obj.size())
                );
            }
        };


    // Housekeeping

        //- Deprecated(2020-11) use fill()
        //  \deprecated(2020-11) use fill()
        void assign(const unsigned int val) { this->fill(val); }

        //- Deprecated(2020-11) use operator=
        //  \deprecated(2020-11) use operator=
        void assign(const PackedList<Width>& rhs) { (*this) = rhs; }

        //- Alias for resize()
        void setSize(const label n, unsigned int val = 0u) { resize(n, val); }
};


// * * * * * * * * * * * * * * * * * Traits  * * * * * * * * * * * * * * * * //

//- Hashing for PackedList data
template<unsigned Width>
struct Hash<PackedList<Width>> : PackedList<Width>::hasher {};


// * * * * * * * * * * * * * * * Global Operators  * * * * * * * * * * * * * //

//- Write List to Ostream, as per UList::writeList() with default length.
//  The default short-length is given by Detail::ListPolicy::short_length
template<unsigned Width>
Ostream& operator<<(Ostream& os, const PackedList<Width>& list)
{
    return list.writeList(os, Detail::ListPolicy::short_length<void>::value);
}


//- Test for equality of sizes and the bits set
template<unsigned Width>
inline bool operator==(const PackedList<Width>& a, const PackedList<Width>& b);

//- Test for inequality of sizes or the bits set
template<unsigned Width>
inline bool operator!=(const PackedList<Width>& a, const PackedList<Width>& b);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "PackedListI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "PackedList.C"
    #include "PackedListIO.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
