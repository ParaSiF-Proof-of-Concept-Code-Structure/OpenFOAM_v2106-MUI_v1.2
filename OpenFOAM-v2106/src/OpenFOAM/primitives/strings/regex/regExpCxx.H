/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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
    Foam::regExpCxx

Description
    Wrapper around C++11 regular expressions.

    Using either POSIX extended regular expressions or
    <a href=
    "http://www.cplusplus.com/reference/regex/ECMAScript"
    >modified ECMAScript regular expression grammar</a>

    Since ECMAScript grammar may not work correctly on all installations,
    the current default is to use extended regular expressions.

    The JAVA/PCRE '(?i)' extension is supported as a prefix to compile the
    regular expression as being case-insensitive.

Note
    The C++11 regular expressions may be broken on some compilers.
    For example, gcc 4.8 is known to fail.
    For these systems the POSIX implementation or alternative must be used.

Warning
    This class should not be used directly.
    Use the Foam::regExp typedef instead.

SourceFiles
    regExpCxxI.H
    regExpCxx.C

\*---------------------------------------------------------------------------*/

#ifndef regExpCxx_H
#define regExpCxx_H

#include <regex>
#include <string>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                          Class regExpCxx Declaration
\*---------------------------------------------------------------------------*/

class regExpCxx
{
    // Private Data

        //- Regular expression (using char type)
        std::regex re_;

        //- Track if input pattern was OK - ie, has a length
        bool ok_;


    // Private Member Functions

        //- Select grammar based on regExpCxx optimisationSwitch
        //  0 = extended, 1 = ECMAScript
        static inline std::regex::flag_type syntax();

public:

    // Public Types

        //- Type for matches
        typedef std::smatch results_type;


    // Static Member Data

        //- The default grammar (extended | ECMAScript).
        static int grammar;


    // Static Member Functions

        //- Test if character is a regex meta-character
        //  \return True if character is one of the following:
        //  - any character: '.' \n
        //  - quantifiers:  '*', '+', '?' \n
        //  - grouping: '(', '|', ')' \n
        //  - range: '[', ']' \n
        //
        // \note Regex bounds '{', '}' are not considered
        inline static bool is_meta(const char c) noexcept;

        //- Test if string contains any (unquoted) meta-characters
        inline static bool is_meta
        (
            const std::string& str,
            const char quote = '\\'
        );


    // Public Classes

        //- Functor wrapper for testing meta-characters
        struct meta
        {
            //- Test if character is a regex meta-character
            bool operator()(const char c) const noexcept
            {
                return is_meta(c);
            }

            //- Test string for meta-characters
            bool operator()(const std::string& s, const char q = '\\') const
            {
                return is_meta(s, q);
            }
        };


    // Constructors

        //- Default construct
        inline regExpCxx();

        //- Copy construct
        inline regExpCxx(const regExpCxx& rgx);

        //- Move construct
        inline regExpCxx(regExpCxx&& rgx) noexcept;

        //- Construct from character array, optionally ignore case
        inline explicit regExpCxx
        (
            const char* pattern,
            const bool ignoreCase = false
        );

        //- Construct from string, optionally ignore case
        inline explicit regExpCxx
        (
            const std::string& pattern,
            const bool ignoreCase = false
        );


    //- Destructor
    ~regExpCxx() = default;


    // Member Functions

    // Access

        //- Return true if expression is empty
        inline bool empty() const noexcept;

        //- Return true if expression is non-empty
        inline bool exists() const noexcept;

        //- The number of capture groups for a non-empty expression
        inline unsigned ngroups() const;

        //  \return True if the pattern was set with ignore-case.
        inline bool nocase() const;


    // Editing

        //- Clear expression.
        //  \return True if expression had existed prior to the clear.
        inline bool clear();

        //- Swap contents
        inline void swap(regExpCxx& rgx);

        //- Compile pattern into a regular expression, optionally ignore case.
        //  \return True if the pattern was compiled
        bool set(const char* pattern, bool ignoreCase=false);

        //- Compile pattern into a regular expression, optionally ignore case.
        //  \return True if the pattern was compiled
        bool set(const std::string& pattern, bool ignoreCase=false);


    // Matching/Searching

        //- Find position within the text.
        //  \return The index where it begins or string::npos if not found
        inline std::string::size_type find(const std::string& text) const;

        //- True if the regex matches the entire text.
        //  The begin-of-line (^) and end-of-line ($) anchors are implicit
        inline bool match(const std::string& text) const;

        //- True if the regex matches the text, set the matches.
        //  The first group starts at index 1 (0 is the entire match).
        //  The begin-of-line (^) and end-of-line ($) anchors are implicit
        inline bool match(const std::string& text, results_type& matches) const;

        //- Return true if the regex was found within the text
        inline bool search(const std::string& text) const;


    // Member Operators

        //- Perform match on text
        inline bool operator()(const std::string& text) const;

        //- Copy assignment
        inline void operator=(const regExpCxx& rgx);

        //- Move assignment
        inline void operator=(regExpCxx&& rgx);

        //- Assign and compile pattern from a character array.
        //  Matching is case sensitive.
        inline void operator=(const char* pattern);

        //- Assign and compile pattern from string.
        //  Matching is case sensitive.
        inline void operator=(const std::string& pattern);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "regExpCxxI.H"

#endif

// ************************************************************************* //
