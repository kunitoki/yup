/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

//==============================================================================
ExpressionType::ExpressionType (const String& typeName, const std::type_info& type, size_t typeSize, size_t typeAlignment)
    : name (typeName)
    , cppType (type)
    , size (typeSize)
    , alignment (typeAlignment)
{
}

//==============================================================================
const ExpressionType& ExpressionType::boolType()
{
    static const auto type = create<bool> ("bool");
    return *type;
}

const ExpressionType& ExpressionType::intType()
{
    static const auto type = create<int> ("int");
    return *type;
}

const ExpressionType& ExpressionType::floatType()
{
    static const auto type = create<float> ("float");
    return *type;
}

const ExpressionType& ExpressionType::doubleType()
{
    static const auto type = create<double> ("double");
    return *type;
}

const ExpressionType& ExpressionType::stringType()
{
    static const auto type = create<String> ("string");
    return *type;
}

const ExpressionType* ExpressionType::findBuiltin (StringRef typeName) noexcept
{
    for (auto* type : { &boolType(), &intType(), &floatType(), &doubleType(), &stringType() })
        if (type->getName() == typeName)
            return type;

    return nullptr;
}

bool ExpressionType::isNumeric() const noexcept
{
    return this == &boolType() || this == &intType() || this == &floatType() || this == &doubleType();
}

//==============================================================================
std::shared_ptr<void> ExpressionType::createValue() const
{
    const auto valueAlignment = std::max (alignment, sizeof (void*));
    void* storage = ::operator new (size, std::align_val_t (valueAlignment));

    try
    {
        construct (storage, 1);
    }
    catch (...)
    {
        ::operator delete (storage, std::align_val_t (valueAlignment));
        throw;
    }

    return std::shared_ptr<void> (storage, [destroyFunction = destroyFunction, valueAlignment] (void* p)
    {
        destroyFunction (p, 1);
        ::operator delete (p, std::align_val_t (valueAlignment));
    });
}

} // namespace yup
