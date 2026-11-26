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

ShaderUniforms::ShaderUniforms (const ShaderReflection::ResourceBinding& block)
    : bytes (block.blockSize <= maxSizeInBytes ? (static_cast<size_t> (block.blockSize) + 15) / 16 * 16 : 0)
{
    using BaseType = ShaderReflection::BaseType;

    for (const auto& reflected : block.members)
    {
        const bool isSupported = (reflected.baseType == BaseType::float32
                                  || reflected.baseType == BaseType::int32
                                  || reflected.baseType == BaseType::uint32
                                  || reflected.baseType == BaseType::boolean)
                              && reflected.arraySizes.size() <= 1
                              && reflected.vecSize >= 1
                              && reflected.vecSize <= 4
                              && reflected.columns >= 1
                              && reflected.columns <= 4;

        if (! isSupported)
            continue;

        Member member;
        member.name = reflected.name;
        member.type = reflected.baseType;
        member.offset = reflected.offset;
        member.numElements = reflected.arraySizes.empty() ? 1 : reflected.arraySizes.front();
        member.elementStride = reflected.arrayStride;
        member.numColumns = reflected.columns;
        member.columnStride = reflected.matrixStride;
        member.numRows = reflected.vecSize;

        // An array without a stride would hold any number of elements in the room of one
        if (member.numElements == 0 || (member.numElements > 1 && member.elementStride == 0))
            continue;

        // The reflection may come from a file: a member reaching past the block is left out, so set() stays inside the bytes
        const uint64 size = bytes.size();
        const uint64 lastElement = static_cast<uint64> (member.numElements - 1) * member.elementStride;
        const uint64 elementSize = static_cast<uint64> (member.numColumns - 1) * member.columnStride + member.numRows * sizeof (uint32);

        if (lastElement > size || elementSize > size || member.offset + lastElement + elementSize > size)
            continue;

        members.push_back (std::move (member));
    }
}

//==============================================================================

const ShaderUniforms::Member* ShaderUniforms::findMember (StringRef name) const noexcept
{
    for (const auto& member : members)
    {
        if (name == member.name)
            return &member;
    }

    return nullptr;
}

bool ShaderUniforms::hasMember (StringRef name) const noexcept
{
    return findMember (name) != nullptr;
}

ShaderReflection::BaseType ShaderUniforms::getMemberType (StringRef name) const noexcept
{
    const auto* member = findMember (name);
    return member != nullptr ? member->type : ShaderReflection::BaseType::unknown;
}

int ShaderUniforms::getNumComponents (StringRef name) const noexcept
{
    const auto* member = findMember (name);
    return member != nullptr ? static_cast<int> (member->numElements * member->numColumns * member->numRows) : 0;
}

bool ShaderUniforms::hasSameLayoutAs (const ShaderUniforms& other) const noexcept
{
    return bytes.size() == other.bytes.size() && members == other.members;
}

//==============================================================================

bool ShaderUniforms::set (StringRef name, Span<const float> values)
{
    using BaseType = ShaderReflection::BaseType;

    const auto* member = findMember (name);
    if (member == nullptr)
        return false;

    const size_t numRows = member->numRows;
    const size_t numColumns = member->numColumns;
    const auto numValues = jmin (values.size(), numRows * numColumns * member->numElements);

    for (size_t index = 0; index < numValues; ++index)
    {
        const auto column = index / numRows;
        const auto offset = member->offset
                          + (column / numColumns) * member->elementStride
                          + (column % numColumns) * member->columnStride
                          + (index % numRows) * sizeof (uint32);

        const float value = values[index];
        uint32 scalar = 0;

        switch (member->type)
        {
            case BaseType::float32:
                scalar = std::bit_cast<uint32> (value);
                break;

            // Kept inside the range before rounding: converting a float out of it is not defined
            case BaseType::int32:
                scalar = static_cast<uint32> (std::llround (jlimit<double> (std::numeric_limits<int32>::min(), std::numeric_limits<int32>::max(), value)));
                break;

            case BaseType::uint32:
                scalar = static_cast<uint32> (std::llround (jlimit<double> (0.0, std::numeric_limits<uint32>::max(), value)));
                break;

            default:
                scalar = exactlyEqual (value, 0.0f) ? 0u : 1u;
                break;
        }

        std::memcpy (bytes.data() + offset, &scalar, sizeof (scalar));
    }

    return true;
}

bool ShaderUniforms::set (StringRef name, const var& value)
{
    const auto isNumber = [] (const var& v)
    {
        return v.isInt() || v.isInt64() || v.isDouble() || v.isBool();
    };

    if (isNumber (value))
    {
        const float scalar = static_cast<float> (value);
        return set (name, Span<const float> (&scalar, 1));
    }

    if (! value.isArray())
        return false;

    std::vector<float> values;
    values.reserve (static_cast<size_t> (value.size()));

    for (const auto& element : value)
    {
        if (! isNumber (element))
            return false;

        values.push_back (static_cast<float> (element));
    }

    return set (name, Span<const float> (values));
}

} // namespace yup
