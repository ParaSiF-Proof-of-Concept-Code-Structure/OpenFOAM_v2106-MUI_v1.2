/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2012-2018 Bernhard Gschaider
    Copyright (C) 2019-2021 OpenCFD Ltd.
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

#include "exprResult.H"
#include "vector.H"
#include "tensor.H"
#include "symmTensor.H"
#include "sphericalTensor.H"
#include "Switch.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace expressions
{
    defineTypeNameAndDebug(exprResult,0);

    defineRunTimeSelectionTable(exprResult, dictionary);
    defineRunTimeSelectionTable(exprResult, empty);

    addToRunTimeSelectionTable(exprResult, exprResult, dictionary);
    addToRunTimeSelectionTable(exprResult, exprResult, empty);

} // End namespace expressions
} // End namespace Foam


const Foam::expressions::exprResult Foam::expressions::exprResult::null;


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::expressions::exprResult::setAverageValueCheckedBool
(
    const bool parRun
)
{
    typedef bool Type;

    if (!isType<Type>())
    {
        return false;
    }

    const Field<Type>& fld = *static_cast<const Field<Type>*>(fieldPtr_);
    label len = fld.size();

    // The average of a bool is slightly dodgy

    label nTrue = 0;
    for (const Type val : fld)
    {
        if (val)
        {
            ++nTrue;
        }
    }

    if (parRun)
    {
        reduce(nTrue, sumOp<label>());
    }

    if (!nTrue)
    {
        isUniform_ = true;
        single_.set(false);
        return true;
    }

    if (parRun)
    {
        reduce(len, sumOp<label>());
    }

    if (nTrue == len)
    {
        isUniform_ = true;
        single_.set(true);
    }
    else
    {
        isUniform_ = false;

        if (nTrue > len/2)
        {
            single_.set(true);
        }
        else
        {
            single_.set(false);
        }
    }

    return true;
}


bool Foam::expressions::exprResult::getUniformCheckedBool
(
    exprResult& result,
    const label size,
    const bool noWarn,
    const bool parRun
) const
{
    typedef bool Type;

    if (!isType<Type>())
    {
        return false;
    }

    result.clear();

    const Field<Type>& fld = *static_cast<const Field<Type>*>(fieldPtr_);
    label len = fld.size();

    // The average of a bool is slightly dodgy

    label nTrue = 0;
    for (const Type val : fld)
    {
        if (val)
        {
            ++nTrue;
        }
    }

    if (parRun)
    {
        reduce(nTrue, sumOp<label>());
        reduce(len, sumOp<label>());
    }

    const Type avg = (nTrue > len/2);

    if (!noWarn)
    {
        // TODO?
    }

    result.setResult<Type>(avg, size);

    return true;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::expressions::exprResult::singleValue::singleValue()
{
    std::memset(static_cast<void*>(this), '\0', sizeof(*this));
}


Foam::expressions::exprResult::singleValue::singleValue
(
    const singleValue& val
)
{
    std::memcpy(static_cast<void*>(this), &val, sizeof(*this));
}


void Foam::expressions::exprResult::singleValue::operator=
(
    const singleValue& val
)
{
    if (this != &val)
    {
        // Self-assignment is a no-op
        std::memcpy(static_cast<void*>(this), &val, sizeof(*this));
    }
}


Foam::expressions::exprResult::exprResult()
:
    refCount(),
    valType_(),
    isUniform_(false),
    isPointData_(false),
    noReset_(false),
    needsReset_(false),
    size_(0),
    single_(),
    fieldPtr_(nullptr),
    objectPtr_(nullptr)
{
    clear();
}


Foam::expressions::exprResult::exprResult(const exprResult& rhs)
:
    exprResult()
{
    this->operator=(rhs);
}


Foam::expressions::exprResult::exprResult(exprResult&& rhs)
:
    exprResult()
{
    this->operator=(std::move(rhs));
}


Foam::expressions::exprResult::exprResult
(
    const dictionary& dict,
    bool uniform,
    bool needsValue
)
:
    refCount(),
    valType_(dict.getOrDefault<word>("valueType", "")),
    isUniform_(dict.getOrDefault("isSingleValue", uniform)),
    isPointData_(dict.getOrDefault("isPointValue", false)),
    noReset_(dict.getOrDefault("noReset", false)),
    needsReset_(false),
    size_(0),
    single_(),
    fieldPtr_(nullptr),
    objectPtr_(nullptr)
{
    DebugInFunction << nl;

    if (dict.found("value"))
    {
        const bool uniform = isUniform_;

        const label len =
        (
            uniform
          ? dict.getOrDefault<label>("fieldSize", 1)
          : dict.get<label>("fieldSize")
        );

        const bool ok =
        (
            // Just use <scalar> for <label>?
            readChecked<scalar>("value", dict, len, uniform)
         || readChecked<vector>("value", dict, len, uniform)
         || readChecked<tensor>("value", dict, len, uniform)
         || readChecked<symmTensor>("value", dict, len, uniform)
         || readChecked<sphericalTensor>("value", dict, len, uniform)
         || readChecked<bool>("value", dict, len, uniform)
        );

        if (!ok)
        {
            if (valType_.empty())
            {
                // For error message only
                valType_ = "none";
            }

            FatalErrorInFunction
                << "Do not know how to read data type " << valType_
                << (uniform ? " as a single value." : ".") << nl
                << exit(FatalError);
        }
    }
    else if (needsValue)
    {
        FatalIOErrorInFunction(dict)
            << "No entry 'value' defined in " << dict.name() << nl
            << exit(FatalIOError);
    }
}


Foam::autoPtr<Foam::expressions::exprResult>
Foam::expressions::exprResult::New
(
    const dictionary& dict
)
{
    const word resultType
    (
        dict.getOrDefault<word>("resultType", "exprResult")
    );

    if (dict.getOrDefault("unsetValue", false))
    {
        auto cstrIter = emptyConstructorTablePtr_->cfind(resultType);

        if (!cstrIter.found())
        {
            FatalIOErrorInLookup
            (
                dict,
                "resultType",
                resultType,
                *emptyConstructorTablePtr_
            ) << exit(FatalIOError);
        }

        DebugInfo
            << "Creating unset result of type " << resultType << nl;

        return autoPtr<exprResult>(cstrIter()());
    }


    auto cstrIter = dictionaryConstructorTablePtr_->cfind(resultType);

    if (!cstrIter.found())
    {
        FatalIOErrorInLookup
        (
            dict,
            "resultType",
            resultType,
            *dictionaryConstructorTablePtr_
        ) << exit(FatalIOError);
    }

    DebugInfo
        << "Creating result of type " << resultType << nl;

    return autoPtr<exprResult>(cstrIter()(dict));
}


Foam::autoPtr<Foam::expressions::exprResult>
Foam::expressions::exprResult::New
(
    Istream& is
)
{
    dictionary dict(is);
    return New(dict);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::expressions::exprResult::~exprResult()
{
    DebugInFunction << nl;

    uglyDelete();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::expressions::exprResult::resetImpl()
{
    clear();
}


bool Foam::expressions::exprResult::reset(bool force)
{
    if (force || !noReset_ || needsReset_)
    {
        this->resetImpl();
        return true;
    }

    return false;
}


void Foam::expressions::exprResult::clear()
{
    uglyDelete();
    valType_.clear();
    objectPtr_.reset(nullptr);
    size_ = 0;
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void Foam::expressions::exprResult::uglyDelete()
{
    if (fieldPtr_)
    {
        const bool ok =
        (
            deleteChecked<scalar>()
         || deleteChecked<vector>()
         || deleteChecked<tensor>()
         || deleteChecked<symmTensor>()
         || deleteChecked<sphericalTensor>()
         || deleteChecked<bool>()
        );

        if (!ok)
        {
            FatalErrorInFunction
                << "Unknown type " << valType_
                << " probable memory loss" << nl
                << exit(FatalError);
        }

        fieldPtr_ = nullptr;
        size_ = 0;
    }
}


Foam::expressions::exprResult
Foam::expressions::exprResult::getUniform
(
    const label size,
    const bool noWarn,
    const bool parRun
) const
{
    if (fieldPtr_ == nullptr)
    {
        FatalErrorInFunction
            << "Not set. Cannot construct uniform value" << nl
            << exit(FatalError);
    }

    exprResult ret;

    const bool ok =
    (
        getUniformChecked<scalar>(ret, size, noWarn, parRun)
     || getUniformChecked<vector>(ret, size, noWarn, parRun)
     || getUniformChecked<tensor>(ret, size, noWarn, parRun)
     || getUniformChecked<symmTensor>(ret, size, noWarn, parRun)
     || getUniformChecked<sphericalTensor>(ret, size, noWarn, parRun)
    );

    if (!ok)
    {
        FatalErrorInFunction
            << "Cannot get uniform value for type " << valType_ << nl
            << exit(FatalError);
    }

    return ret;
}


void Foam::expressions::exprResult::testIfSingleValue(const bool parRun)
{
    if (fieldPtr_ == nullptr)
    {
        WarningInFunction
            << "Not set - cannot determine if uniform" << nl << endl;
        return;
    }

    const bool ok =
    (
        setAverageValueChecked<scalar>(parRun)
     || setAverageValueChecked<vector>(parRun)
     || setAverageValueChecked<tensor>(parRun)
     || setAverageValueChecked<symmTensor>(parRun)
     || setAverageValueChecked<sphericalTensor>(parRun)
     || setAverageValueCheckedBool(parRun)
    );

    if (!ok)
    {
        WarningInFunction
            << "Unknown type " << valType_ << nl << endl;
    }
}


void Foam::expressions::exprResult::operator=(const exprResult& rhs)
{
    if (this == &rhs)
    {
        return;  // Self-assignment is a no-op
    }

    DebugInFunction << "rhs:" << rhs << nl;

    clear();

    valType_ = rhs.valType_;
    isUniform_ = rhs.isUniform_;
    isPointData_ = rhs.isPointData_;
    single_ = rhs.single_;

    if (rhs.fieldPtr_)
    {
        const bool ok =
        (
            duplicateFieldChecked<scalar>(rhs.fieldPtr_)
         || duplicateFieldChecked<vector>(rhs.fieldPtr_)
         || duplicateFieldChecked<tensor>(rhs.fieldPtr_)
         || duplicateFieldChecked<symmTensor>(rhs.fieldPtr_)
         || duplicateFieldChecked<sphericalTensor>(rhs.fieldPtr_)
         || duplicateFieldChecked<bool>(rhs.fieldPtr_)
        );

        if (!ok)
        {
            FatalErrorInFunction
                << " Type " << valType_ << " can not be copied" << nl
                << exit(FatalError);
        }
    }
    else if (objectPtr_)
    {
        FatalErrorInFunction
            << "Assignment with general content not possible" << nl
            << exit(FatalError);
    }
}


void Foam::expressions::exprResult::operator=(exprResult&& rhs)
{
    if (this == &rhs)
    {
        return;  // Self-assignment is a no-op
    }

    clear();

    valType_ = rhs.valType_;
    isUniform_ = rhs.isUniform_;
    isPointData_ = rhs.isPointData_;
    noReset_ = rhs.noReset_;
    needsReset_ = rhs.needsReset_;
    size_ = rhs.size_;

    single_ = rhs.single_;
    fieldPtr_ = rhs.fieldPtr_;

    objectPtr_.reset(rhs.objectPtr_.release());

    rhs.fieldPtr_ = nullptr;  // Took ownership of field pointer
    rhs.clear();
}


void Foam::expressions::exprResult::writeEntry
(
    const word& keyword,
    Ostream& os
) const
{
    const bool ok =
    (
        writeEntryChecked<scalar>(keyword, os)
     || writeEntryChecked<vector>(keyword, os)
     || writeEntryChecked<tensor>(keyword, os)
     || writeEntryChecked<symmTensor>(keyword, os)
     || writeEntryChecked<sphericalTensor>(keyword, os)
     || writeEntryChecked<bool>(keyword, os)
    );

    if (!ok)
    {
        WarningInFunction
            << "Unknown data type " << valType_ << endl;
    }
}


void Foam::expressions::exprResult::writeDict
(
    Ostream& os,
    const bool subDict
) const
{
    // const auto oldFmt = os.format(IOstream::ASCII);

    DebugInFunction
        << Foam::name(this) << nl
        << "Format: "
        << IOstreamOption::formatNames[os.format()] << nl;

    if (subDict)
    {
        os.beginBlock();
    }

    os.writeEntry("resultType", valueType());
    os.writeEntryIfDifferent<Switch>("noReset", false, noReset_);

    if (fieldPtr_ == nullptr)
    {
        os.writeEntry<Switch>("unsetValue", true);
    }
    else
    {
        os.writeEntry("valueType", valType_);

        os.writeEntryIfDifferent<Switch>("isPointValue", false, isPointData_);
        os.writeEntry<Switch>("isSingleValue", isUniform_);

        this->writeField(os, "value");
    }

    if (subDict)
    {
        os.endBlock();
    }

    // os.format(oldFmt);
}


void Foam::expressions::exprResult::writeField
(
    Ostream& os,
    const word& keyword
) const
{
    // const auto oldFmt = os.format(IOstream::ASCII);

    DebugInFunction
        << Foam::name(this) << nl
        << "Format: "
        << IOstreamOption::formatNames[os.format()] << nl;

    const bool ok =
    (
        writeFieldChecked<scalar>(keyword, os)
     || writeFieldChecked<vector>(keyword, os)
     || writeFieldChecked<tensor>(keyword, os)
     || writeFieldChecked<symmTensor>(keyword, os)
     || writeFieldChecked<sphericalTensor>(keyword, os)
     || writeFieldChecked<label>(keyword, os)
     || writeFieldChecked<bool>(keyword, os)
    );

    if (!ok)
    {
        WarningInFunction
            << "Unknown data type " << valType_ << endl;
    }
}


void Foam::expressions::exprResult::writeValue
(
    Ostream& os
) const
{
    // const auto oldFmt = os.format(IOstream::ASCII);

    DebugInFunction
        << Foam::name(this) << nl
        << "Format: "
        << IOstreamOption::formatNames[os.format()] << nl;

    const bool ok =
    (
        writeSingleValueChecked<scalar>(os)
     || writeSingleValueChecked<vector>(os)
     || writeSingleValueChecked<tensor>(os)
     || writeSingleValueChecked<symmTensor>(os)
     || writeSingleValueChecked<sphericalTensor>(os)
     || writeSingleValueChecked<label>(os)
     || writeSingleValueChecked<bool>(os)
    );

    if (!ok)
    {
        WarningInFunction
            << "Unknown data type " << valType_ << endl;
    }
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

Foam::expressions::exprResult&
Foam::expressions::exprResult::operator*=
(
    const scalar& b
)
{
    if (isObject())
    {
        FatalErrorInFunction
            << "Can only multiply Field-type exprResult. Not "
            << valType_ << nl
            << exit(FatalError);
    }
    if (!fieldPtr_)
    {
        FatalErrorInFunction
            << "Can not multiply. Unallocated field of type" << valType_ << nl
            << exit(FatalError);
    }

    const bool ok =
    (
        multiplyEqChecked<scalar>(b)
     || multiplyEqChecked<vector>(b)
     || multiplyEqChecked<tensor>(b)
     || multiplyEqChecked<symmTensor>(b)
     || multiplyEqChecked<sphericalTensor>(b)
    );

    if (!ok)
    {
        FatalErrorInFunction
            << "Can not multiply field of type "
            << valType_ << nl
            << exit(FatalError);
    }

    return *this;
}


Foam::expressions::exprResult&
Foam::expressions::exprResult::operator+=
(
    const exprResult& b
)
{
    if (isObject())
    {
        FatalErrorInFunction
            << "Can only add Field-type, not type: "
            << valType_ << nl
            << exit(FatalError);
    }
    if (!fieldPtr_)
    {
        FatalErrorInFunction
            << "Can not add. Unallocated field of type " << valType_ << nl
            << exit(FatalError);
    }

    if (this->size() != b.size())
    {
        FatalErrorInFunction
            << "Different sizes " << this->size() << " and " << b.size() << nl
            << exit(FatalError);
    }

    const bool ok =
    (
        plusEqChecked<scalar>(b)
     || plusEqChecked<vector>(b)
     || plusEqChecked<tensor>(b)
     || plusEqChecked<symmTensor>(b)
     || plusEqChecked<sphericalTensor>(b)
    );

    if (!ok)
    {
        FatalErrorInFunction
            << "Can not add Field-type exprResult of type"
            << valType_ << nl
            << exit(FatalError);
    }

    return *this;
}


// * * * * * * * * * * * * * * * IOstream Operators  * * * * * * * * * * * * //

Foam::Istream& Foam::operator>>
(
    Istream& is,
    expressions::exprResult& data
)
{
    dictionary dict(is);

    data = expressions::exprResult(dict);

    return is;
}


Foam::Ostream& Foam::operator<<
(
    Ostream& os,
    const expressions::exprResult& data
)
{
    data.writeDict(os);

    return os;
}


Foam::expressions::exprResult Foam::operator*
(
    const scalar& a,
    const expressions::exprResult& b
)
{
    expressions::exprResult result(b);
    return result *= a;
}


Foam::expressions::exprResult Foam::operator*
(
    const expressions::exprResult& a,
    const scalar& b
)
{
    expressions::exprResult result(a);
    result *= b;

    return result;
}


Foam::expressions::exprResult Foam::operator+
(
    const expressions::exprResult& a,
    const expressions::exprResult& b
)
{
    expressions::exprResult result(a);
    result += b;

    return result;
}


const void* Foam::expressions::exprResult::dataAddress() const
{
    #undef  defineExpressionMethod
    #define defineExpressionMethod(Type)                                      \
        if (isType<Type>())                                                   \
        {                                                                     \
            return static_cast<Field<Type>*>(fieldPtr_)->cdata();             \
        }

    defineExpressionMethod(scalar);
    defineExpressionMethod(vector);
    defineExpressionMethod(tensor);
    defineExpressionMethod(symmTensor);
    defineExpressionMethod(sphericalTensor);

    #undef defineExpressionMethod

    FatalErrorInFunction
        << "Unsupported type" << valType_ << nl
        << exit(FatalError);

    return nullptr;
}


// ************************************************************************* //
