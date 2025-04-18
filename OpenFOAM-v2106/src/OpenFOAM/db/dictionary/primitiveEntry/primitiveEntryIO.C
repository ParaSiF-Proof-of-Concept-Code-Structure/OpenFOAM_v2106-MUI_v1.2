/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
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

\*---------------------------------------------------------------------------*/

#include "primitiveEntry.H"
#include "functionEntry.H"
#include "evalEntry.H"

// * * * * * * * * * * * * * * * Local Functions * * * * * * * * * * * * * * //

namespace
{
    // This is akin to a SafeIOWarning, which does not yet exist
    inline void safeIOWarning
    (
        const Foam::IOstream& is,
        const std::string& msg
    )
    {
        std::cerr
            << "--> FOAM Warning :\n"
            << "    Reading \"" << is.name() << "\" at line "
            << is.lineNumber() << '\n'
            << "    " << msg << std::endl;
    }

} // End anonymous namespace


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::primitiveEntry::acceptToken
(
    const token& tok,
    const dictionary& dict,
    Istream& is
)
{
    bool accept = tok.good();

    if (tok.isDirective())
    {
        // Directive (wordToken) begins with '#'. Eg, "#include"
        // Remove leading '#' sigil before dispatching

        const word& key = tok.wordToken();

        // Min-size is 2: sigil '#' with any content
        accept =
        (
            (disableFunctionEntries || key.size() < 2)
         || !expandFunction(key.substr(1), dict, is)
        );
    }
    else if (tok.isExpression())
    {
        // Expression (stringToken): ${{ expr }}
        // Surrounding delimiters are stripped as required in evalEntry

        const string& key = tok.stringToken();

        // Min-size is 6: decorators '${{}}' with any content
        accept =
        (
            (disableFunctionEntries || key.size() < 6)
         || !functionEntries::evalEntry::execute
            (
                dict,
                *this,
                key,
                1,      // Field width is 1
                is      // For error messages
            )
        );
    }
    else if (tok.isVariable())
    {
        // Variable (stringToken): starts with '$'
        // Eg, "$varName" or "${varName}"
        // Remove leading '$' sigil before dispatching

        const string& key = tok.stringToken();

        // Min-size is 2: sigil '$' with any content
        accept =
        (
            (disableFunctionEntries || key.size() < 2)
         || !expandVariable(key.substr(1), dict)
        );
    }

    return accept;
}


bool Foam::primitiveEntry::expandFunction
(
    const word& functionName,
    const dictionary& dict,
    Istream& is
)
{
    return functionEntry::execute(functionName, dict, *this, is);
}


bool Foam::primitiveEntry::read(const dictionary& dict, Istream& is)
{
    is.fatalCheck(FUNCTION_NAME);

    // Track balanced bracket/brace pairs, with max stack depth of 60.
    // Use a bitmask to track the opening char: 0 = '()', 1 = '{}'
    //
    // Notes
    // - the bitmask is set *before* increasing the depth since the left
    //   shift implicitly carries a 1-offset with it.
    //   Eg, (1u << 0) already corresponds to depth=1 (the first bit)
    //
    // - similarly, the bitmask is tested *after* decreasing depth

    uint64_t balanced = 0u;
    int depth = 0;
    token tok;

    while
    (
        !is.read(tok).bad() && tok.good()
     && !(tok == token::END_STATEMENT && depth == 0)
    )
    {
        if (tok.isPunctuation())
        {
            const char c = tok.pToken();
            switch (c)
            {
                case token::BEGIN_LIST:
                {
                    if (depth >= 0 && depth < 61)
                    {
                        balanced &= ~(1u << depth); // clear bit
                    }
                    ++depth;
                }
                break;

                case token::BEGIN_BLOCK:
                {
                    if (depth >= 0 && depth < 61)
                    {
                        balanced |= (1u << depth); // set bit
                    }
                    ++depth;
                }
                break;

                case token::END_LIST:
                {
                    --depth;
                    if (depth < 0)
                    {
                        safeIOWarning
                        (
                            is,
                            "Too many closing ')' ... was a ';' forgotten?"
                        );
                    }
                    else if (depth < 61 && ((balanced >> depth) & 1u))
                    {
                        // Bit was set, but expected it to be unset.
                        safeIOWarning(is, "Imbalanced '{' with ')'");
                    }
                }
                break;

                case token::END_BLOCK:
                {
                    --depth;
                    if (depth < 0)
                    {
                        safeIOWarning
                        (
                            is,
                            "Too many closing '}' ... was a ';' forgotten?"
                        );
                    }
                    else if (depth < 61 && !((balanced >> depth) & 1u))
                    {
                        // Bit was unset, but expected it to be set.
                        safeIOWarning(is, "Imbalanced '(' with '}'");
                    }
                }
                break;
            }
        }

        if (acceptToken(tok, dict, is))
        {
            newElmt(tokenIndex()++) = std::move(tok);
        }

        // With/without move: clear any old content and force to have a
        // known good token so that we can rely on it for the return value.

        tok = token::punctuationToken::NULL_TOKEN;
    }

    if (depth)
    {
        safeIOWarning(is, "Imbalanced brackets");
    }

    is.fatalCheck(FUNCTION_NAME);
    return tok.good();
}


void Foam::primitiveEntry::readEntry(const dictionary& dict, Istream& is)
{
    const label keywordLineNumber = is.lineNumber();
    tokenIndex() = 0;

    if (read(dict, is))
    {
        setSize(tokenIndex());
        tokenIndex() = 0;
    }
    else
    {
        std::ostringstream os;
        os  << "ill defined primitiveEntry starting at keyword '"
            << keyword() << '\''
            << " on line " << keywordLineNumber
            << " and ending at line " << is.lineNumber();

        SafeFatalIOErrorInFunction
        (
            is,
            os.str()
        );
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::primitiveEntry::primitiveEntry
(
    const keyType& key,
    const dictionary& dict,
    Istream& is
)
:
    entry(key),
    ITstream
    (
        is.name() + '.' + key,
        tokenList(10),
        static_cast<IOstreamOption>(is)
    )
{
    readEntry(dict, is);
}


Foam::primitiveEntry::primitiveEntry(const keyType& key, Istream& is)
:
    primitiveEntry(key, dictionary::null, is)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::primitiveEntry::write(Ostream& os, const bool contentsOnly) const
{
    if (!contentsOnly)
    {
        os.writeKeyword(keyword());
    }

    bool addSpace = false;  // Separate from previous token with a space
    for (const token& tok : *this)
    {
        if (addSpace) os << token::SPACE;
        addSpace = true;

        // Output token with direct handling in Ostream(s),
        // or use normal '<<' output operator
        if (!os.write(tok))
        {
            os  << tok;
        }
    }

    if (!contentsOnly)
    {
        os  << token::END_STATEMENT << endl;
    }
}


void Foam::primitiveEntry::write(Ostream& os) const
{
    this->write(os, false);
}


// * * * * * * * * * * * * * * * IOstream Operators  * * * * * * * * * * * * //

template<>
Foam::Ostream& Foam::operator<<
(
    Ostream& os,
    const InfoProxy<primitiveEntry>& ip
)
{
    const primitiveEntry& e = ip.t_;

    e.print(os);

    const label nPrintTokens = 10;

    os  << "    primitiveEntry '" << e.keyword() << "' comprises ";

    for (label i=0; i<min(e.size(), nPrintTokens); ++i)
    {
        os  << nl << "        " << e[i].info();
    }

    if (e.size() > nPrintTokens)
    {
        os  << " ...";
    }

    os  << endl;

    return os;
}


// ************************************************************************* //
