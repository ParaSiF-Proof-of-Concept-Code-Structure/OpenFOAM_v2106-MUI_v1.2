/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

#include "Function1Expression.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::Function1Types::Function1Expression<Type>::Function1Expression
(
    const word& entryName,
    const dictionary& dict
)
:
    Function1<Type>(entryName, dict),
    dict_(dict),
    valueExpr_(),
    driver_(1, dict_)
{
    if (dict.getOrDefault("debug", false))
    {
        debug |= 1;
    }

    string expr;
    dict.readEntry("expression", expr);
    valueExpr_ = expressions::exprString(expr, dict);

    // Basic sanity
    if (valueExpr_.empty())
    {
        FatalIOErrorInFunction(dict_)
            << "The expression was not defined!" << nl
            << exit(FatalIOError);
    }

    driver_.readDict(dict_);
}


template<class Type>
Foam::Function1Types::Function1Expression<Type>::Function1Expression
(
    const Function1Expression<Type>& rhs
)
:
    Function1<Type>(rhs),
    dict_(rhs.dict_),
    valueExpr_(rhs.valueExpr_),
    driver_(1, rhs.driver_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
Type Foam::Function1Types::Function1Expression<Type>::value
(
    const scalar x
) const
{
    // Expression evaluation
    driver_.clearVariables();

    driver_.setArgument(x);

    driver_.parse(this->valueExpr_);

    expressions::exprResult result(driver_.result());

    DebugInfo
        << "Evaluated: " << result << nl;

    if (!result.hasValue() || !result.size() || !result.isType<Type>())
    {
        FatalErrorInFunction
            << "Could not evaluate: " << this->valueExpr_
            << exit(FatalError);
    }

    return result.cref<Type>().first();
}


template<class Type>
Type Foam::Function1Types::Function1Expression<Type>::integrate
(
    const scalar x1,
    const scalar x2
) const
{
    NotImplemented;
    return Zero;
}


template<class Type>
void Foam::Function1Types::Function1Expression<Type>::writeData
(
    Ostream& os
) const
{
    // Function1-from-subdict so out dictionary contains
    // only the relevant entries.
    dict_.writeEntry(this->name(), os);
}


// ************************************************************************* //
