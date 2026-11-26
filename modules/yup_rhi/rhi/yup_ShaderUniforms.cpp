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

    std::vector<Member> members;

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

    if (! members.empty())
        layout = std::make_shared<std::vector<Member>> (std::move (members));
}

//==============================================================================

Span<const ShaderUniforms::Member> ShaderUniforms::getMembers() const noexcept
{
    return layout != nullptr ? Span<const Member> (layout->data(), layout->size()) : Span<const Member>();
}

ShaderUniforms::MemberHandle ShaderUniforms::findMember (StringRef name) const noexcept
{
    const auto members = getMembers();

    for (size_t index = 0; index < members.size(); ++index)
    {
        if (name == members[index].name)
            return MemberHandle (static_cast<int> (index));
    }

    return {};
}

const ShaderUniforms::Member* ShaderUniforms::getMember (MemberHandle member) const noexcept
{
    // A handle found in uniforms laid out another way may be past the members of these ones
    const auto members = getMembers();

    return member.isValid() && static_cast<size_t> (member.index) < members.size() ? members.data() + member.index : nullptr;
}

bool ShaderUniforms::hasMember (StringRef name) const noexcept
{
    return findMember (name).isValid();
}

ShaderReflection::BaseType ShaderUniforms::getMemberType (StringRef name) const noexcept
{
    const auto* member = getMember (findMember (name));
    return member != nullptr ? member->type : ShaderReflection::BaseType::unknown;
}

int ShaderUniforms::getNumComponents (StringRef name) const noexcept
{
    const auto* member = getMember (findMember (name));
    return member != nullptr ? member->getNumComponents() : 0;
}

bool ShaderUniforms::hasSameLayoutAs (const ShaderUniforms& other) const noexcept
{
    if (bytes.size() != other.bytes.size())
        return false;

    // Copies share their members, which are only compared for uniforms made apart
    return layout == other.layout || (layout != nullptr && other.layout != nullptr && *layout == *other.layout);
}

bool ShaderUniforms::operator== (const ShaderUniforms& other) const noexcept
{
    return bytes == other.bytes && hasSameLayoutAs (other);
}

//==============================================================================

void ShaderUniforms::write (const Member& member, size_t index, float value) noexcept
{
    using BaseType = ShaderReflection::BaseType;

    const size_t numRows = member.numRows;
    const size_t numColumns = member.numColumns;
    const auto column = index / numRows;
    const auto offset = member.offset
                      + (column / numColumns) * member.elementStride
                      + (column % numColumns) * member.columnStride
                      + (index % numRows) * sizeof (uint32);

    uint32 scalar = 0;

    switch (member.type)
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

bool ShaderUniforms::set (MemberHandle member, Span<const float> values)
{
    const auto* found = getMember (member);
    if (found == nullptr)
        return false;

    const auto numValues = jmin (values.size(), static_cast<size_t> (found->getNumComponents()));

    for (size_t index = 0; index < numValues; ++index)
        write (*found, index, values[index]);

    return true;
}

bool ShaderUniforms::set (MemberHandle member, const var& value)
{
    const auto isNumber = [] (const var& v)
    {
        return v.isInt() || v.isInt64() || v.isDouble() || v.isBool();
    };

    if (isNumber (value))
    {
        const float scalar = static_cast<float> (value);
        return set (member, Span<const float> (&scalar, 1));
    }

    const auto* elements = value.getArray();
    const auto* found = getMember (member);

    // Nothing is written of an array holding anything but numbers, so it is checked before its first number is
    if (elements == nullptr || found == nullptr || ! std::all_of (elements->begin(), elements->end(), isNumber))
        return false;

    const auto numValues = jmin (static_cast<size_t> (elements->size()), static_cast<size_t> (found->getNumComponents()));

    for (size_t index = 0; index < numValues; ++index)
        write (*found, index, static_cast<float> (elements->begin()[index]));

    return true;
}

} // namespace yup
