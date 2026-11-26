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

#include <gtest/gtest.h>

#include <yup_rhi/yup_rhi.h>

#include <array>
#include <cstring>
#include <vector>

using namespace yup;

//==============================================================================
// ShaderUniforms - packing the members of a reflected uniform block, no GPU involved
//==============================================================================

class ShaderUniformsTests : public ::testing::Test
{
protected:
    using BaseType = ShaderReflection::BaseType;

    /** A member as the reflection of a block lists it: a scalar, a vector of @p vecSize
        components or a matrix of @p columns columns placed @p matrixStride bytes apart. */
    static ShaderReflection::ResourceMember makeMember (const String& name,
                                                        BaseType type,
                                                        uint32 offset,
                                                        uint32 vecSize = 1,
                                                        uint32 columns = 1,
                                                        uint32 matrixStride = 0)
    {
        ShaderReflection::ResourceMember member;
        member.name = name;
        member.baseType = type;
        member.offset = offset;
        member.vecSize = vecSize;
        member.columns = columns;
        member.matrixStride = matrixStride;
        member.size = columns > 1 ? columns * matrixStride : vecSize * 4;
        return member;
    }

    /** Turns a member into an array of @p count elements placed @p stride bytes apart. */
    static ShaderReflection::ResourceMember makeArray (ShaderReflection::ResourceMember member, uint32 count, uint32 stride)
    {
        member.arraySizes = { count };
        member.arrayStride = stride;
        member.size = count * stride;
        return member;
    }

    static ShaderReflection::ResourceBinding makeBlock (uint32 blockSize, std::vector<ShaderReflection::ResourceMember> members)
    {
        ShaderReflection::ResourceBinding block;
        block.name = "Parameters";
        block.type = ShaderReflection::ResourceType::uniformBuffer;
        block.blockSize = blockSize;
        block.members = std::move (members);
        return block;
    }

    /** The std140 block { float intensity; vec2 point; vec3 direction; vec4 tint; }. */
    static ShaderReflection::ResourceBinding makeVectorBlock()
    {
        return makeBlock (48, { makeMember ("intensity", BaseType::float32, 0), makeMember ("point", BaseType::float32, 8, 2), makeMember ("direction", BaseType::float32, 16, 3), makeMember ("tint", BaseType::float32, 32, 4) });
    }

    template <typename Type>
    static Type read (const ShaderUniforms& uniforms, size_t offset)
    {
        Type value {};
        std::memcpy (&value, uniforms.getData() + offset, sizeof (Type));
        return value;
    }

    static bool isZeroed (const ShaderUniforms& uniforms)
    {
        return std::all_of (uniforms.getData(), uniforms.getData() + uniforms.getSizeInBytes(), [] (uint8 byte)
        {
            return byte == 0;
        });
    }
};

TEST_F (ShaderUniformsTests, DefaultConstructedHoldsNothing)
{
    ShaderUniforms uniforms;

    EXPECT_TRUE (uniforms.isEmpty());
    EXPECT_EQ (uniforms.getSizeInBytes(), 0u);
    EXPECT_FALSE (uniforms.hasMember ("intensity"));
    EXPECT_FALSE (uniforms.set ("intensity", 1.0f));
}

TEST_F (ShaderUniformsTests, BlockWithoutSizeIsEmpty)
{
    const ShaderUniforms uniforms (makeBlock (0, {}));

    EXPECT_TRUE (uniforms.isEmpty());
    EXPECT_EQ (uniforms.getSizeInBytes(), 0u);
}

TEST_F (ShaderUniformsTests, BlockAboveTheLargestSizeIsEmpty)
{
    const ShaderUniforms largest (makeBlock (static_cast<uint32> (ShaderUniforms::maxSizeInBytes), { makeMember ("intensity", BaseType::float32, 0) }));

    EXPECT_EQ (largest.getSizeInBytes(), ShaderUniforms::maxSizeInBytes);
    EXPECT_TRUE (largest.hasMember ("intensity"));

    // A reflection read from a file can declare any size: nothing is allocated for one no backend binds
    for (const auto blockSize : { static_cast<uint32> (ShaderUniforms::maxSizeInBytes) + 1, 0xffffffffu })
    {
        ShaderUniforms uniforms (makeBlock (blockSize, { makeMember ("intensity", BaseType::float32, 0) }));

        EXPECT_TRUE (uniforms.isEmpty()) << blockSize;
        EXPECT_EQ (uniforms.getSizeInBytes(), 0u);
        EXPECT_FALSE (uniforms.hasMember ("intensity"));
        EXPECT_FALSE (uniforms.set ("intensity", 1.0f));
    }
}

TEST_F (ShaderUniformsTests, StartsZeroedWithTheSizeRoundedToSixteenBytes)
{
    const ShaderUniforms uniforms (makeBlock (20, { makeMember ("intensity", BaseType::float32, 16) }));

    EXPECT_FALSE (uniforms.isEmpty());
    EXPECT_EQ (uniforms.getSizeInBytes(), 32u);
    EXPECT_TRUE (isZeroed (uniforms));

    EXPECT_EQ (ShaderUniforms (makeVectorBlock()).getSizeInBytes(), 48u);
}

TEST_F (ShaderUniformsTests, SetsScalarsAndVectorsAtTheirOffsets)
{
    ShaderUniforms uniforms (makeVectorBlock());

    EXPECT_TRUE (uniforms.set ("intensity", { 0.5f }));
    EXPECT_TRUE (uniforms.set ("point", { 1.0f, 2.0f }));
    EXPECT_TRUE (uniforms.set ("direction", { 3.0f, 4.0f, 5.0f }));
    EXPECT_TRUE (uniforms.set ("tint", { 0.25f, 0.5f, 0.75f, 1.0f }));

    EXPECT_FLOAT_EQ (read<float> (uniforms, 0), 0.5f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 8), 1.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 12), 2.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 16), 3.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 20), 4.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 24), 5.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 32), 0.25f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 36), 0.5f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 40), 0.75f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 44), 1.0f);

    // The padding std140 leaves between the members is not written
    EXPECT_EQ (read<uint32> (uniforms, 4), 0u);
    EXPECT_EQ (read<uint32> (uniforms, 28), 0u);
}

TEST_F (ShaderUniformsTests, AcceptsArraysBracesAndContainers)
{
    ShaderUniforms uniforms (makeVectorBlock());

    const float fromArray[] = { 1.0f, 2.0f };
    EXPECT_TRUE (uniforms.set ("point", fromArray));
    EXPECT_FLOAT_EQ (read<float> (uniforms, 12), 2.0f);

    const std::array<float, 2> fromStdArray { 3.0f, 4.0f };
    EXPECT_TRUE (uniforms.set ("point", fromStdArray));
    EXPECT_FLOAT_EQ (read<float> (uniforms, 12), 4.0f);

    const std::vector<float> fromVector { 5.0f, 6.0f };
    EXPECT_TRUE (uniforms.set ("point", fromVector));
    EXPECT_FLOAT_EQ (read<float> (uniforms, 12), 6.0f);

    EXPECT_TRUE (uniforms.set ("point", Span<const float> (fromArray, 2)));
    EXPECT_FLOAT_EQ (read<float> (uniforms, 12), 2.0f);
}

TEST_F (ShaderUniformsTests, ConvertsToIntegerAndBooleanMembers)
{
    ShaderUniforms uniforms (makeBlock (32, { makeMember ("count", BaseType::int32, 0), makeMember ("mask", BaseType::uint32, 4), makeMember ("enabled", BaseType::boolean, 8), makeMember ("cell", BaseType::int32, 16, 2) }));

    EXPECT_TRUE (uniforms.set ("count", { -3.6f }));
    EXPECT_TRUE (uniforms.set ("mask", { 7.6f }));
    EXPECT_TRUE (uniforms.set ("enabled", { 0.25f }));
    EXPECT_TRUE (uniforms.set ("cell", { 2.4f, -1.0f }));

    EXPECT_EQ (read<int32> (uniforms, 0), -4);
    EXPECT_EQ (read<uint32> (uniforms, 4), 8u);
    EXPECT_EQ (read<uint32> (uniforms, 8), 1u);
    EXPECT_EQ (read<int32> (uniforms, 16), 2);
    EXPECT_EQ (read<int32> (uniforms, 20), -1);

    EXPECT_TRUE (uniforms.set ("enabled", { 0.0f }));
    EXPECT_EQ (read<uint32> (uniforms, 8), 0u);
}

TEST_F (ShaderUniformsTests, KeepsIntegersInsideTheirRange)
{
    ShaderUniforms uniforms (makeBlock (16, { makeMember ("count", BaseType::int32, 0), makeMember ("mask", BaseType::uint32, 4) }));

    EXPECT_TRUE (uniforms.set ("count", { 1.0e20f }));
    EXPECT_TRUE (uniforms.set ("mask", { -5.0f }));
    EXPECT_EQ (read<int32> (uniforms, 0), std::numeric_limits<int32>::max());
    EXPECT_EQ (read<uint32> (uniforms, 4), 0u);

    EXPECT_TRUE (uniforms.set ("count", { -1.0e20f }));
    EXPECT_TRUE (uniforms.set ("mask", { 1.0e20f }));
    EXPECT_EQ (read<int32> (uniforms, 0), std::numeric_limits<int32>::min());
    EXPECT_EQ (read<uint32> (uniforms, 4), std::numeric_limits<uint32>::max());
}

TEST_F (ShaderUniformsTests, SetsMatrixColumnsAtTheMatrixStride)
{
    // { float intensity; mat4 transform; mat3 rotation; }: the columns of a mat3 are 16 bytes apart too
    ShaderUniforms uniforms (makeBlock (128, { makeMember ("intensity", BaseType::float32, 0), makeMember ("transform", BaseType::float32, 16, 4, 4, 16), makeMember ("rotation", BaseType::float32, 80, 3, 3, 16) }));

    std::vector<float> transform (16);
    for (size_t i = 0; i < transform.size(); ++i)
        transform[i] = static_cast<float> (i + 1);

    EXPECT_TRUE (uniforms.set ("transform", transform));
    EXPECT_TRUE (uniforms.set ("rotation", { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f }));

    for (size_t column = 0; column < 4; ++column)
    {
        for (size_t row = 0; row < 4; ++row)
            EXPECT_FLOAT_EQ (read<float> (uniforms, 16 + column * 16 + row * 4), static_cast<float> (column * 4 + row + 1)) << "column " << column << " row " << row;
    }

    for (size_t column = 0; column < 3; ++column)
    {
        for (size_t row = 0; row < 3; ++row)
            EXPECT_FLOAT_EQ (read<float> (uniforms, 80 + column * 16 + row * 4), static_cast<float> (column * 3 + row + 1)) << "column " << column << " row " << row;

        EXPECT_EQ (read<uint32> (uniforms, 80 + column * 16 + 12), 0u) << "the padding of column " << column;
    }

    EXPECT_EQ (uniforms.getNumComponents ("transform"), 16);
    EXPECT_EQ (uniforms.getNumComponents ("rotation"), 9);
}

TEST_F (ShaderUniformsTests, SetsArrayElementsAtTheArrayStride)
{
    // { float weights[3]; vec2 points[2]; }: std140 places the elements of both 16 bytes apart
    ShaderUniforms uniforms (makeBlock (80, { makeArray (makeMember ("weights", BaseType::float32, 0), 3, 16), makeArray (makeMember ("points", BaseType::float32, 48, 2), 2, 16) }));

    EXPECT_TRUE (uniforms.set ("weights", { 1.0f, 2.0f, 3.0f }));
    EXPECT_TRUE (uniforms.set ("points", { 4.0f, 5.0f, 6.0f, 7.0f }));

    EXPECT_FLOAT_EQ (read<float> (uniforms, 0), 1.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 16), 2.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 32), 3.0f);
    EXPECT_EQ (read<uint32> (uniforms, 4), 0u);

    EXPECT_FLOAT_EQ (read<float> (uniforms, 48), 4.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 52), 5.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 64), 6.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 68), 7.0f);

    EXPECT_EQ (uniforms.getNumComponents ("weights"), 3);
    EXPECT_EQ (uniforms.getNumComponents ("points"), 4);
}

TEST_F (ShaderUniformsTests, FewerValuesLeaveTheRestAndExtraOnesAreIgnored)
{
    ShaderUniforms uniforms (makeBlock (32, { makeMember ("tint", BaseType::float32, 0, 4), makeMember ("intensity", BaseType::float32, 16) }));

    EXPECT_TRUE (uniforms.set ("tint", { 1.0f, 2.0f, 3.0f, 4.0f }));
    EXPECT_TRUE (uniforms.set ("tint", { 9.0f, 8.0f }));

    EXPECT_FLOAT_EQ (read<float> (uniforms, 0), 9.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 4), 8.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 8), 3.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 12), 4.0f);

    EXPECT_TRUE (uniforms.set ("tint", { 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f }));

    EXPECT_FLOAT_EQ (read<float> (uniforms, 12), 8.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 16), 0.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 20), 0.0f);

    EXPECT_TRUE (uniforms.set ("tint", Span<const float>()));
    EXPECT_FLOAT_EQ (read<float> (uniforms, 0), 5.0f);
}

TEST_F (ShaderUniformsTests, UnknownMemberReturnsFalseAndWritesNothing)
{
    ShaderUniforms uniforms (makeVectorBlock());

    EXPECT_FALSE (uniforms.hasMember ("time"));
    EXPECT_EQ (uniforms.getMemberType ("time"), BaseType::unknown);
    EXPECT_EQ (uniforms.getNumComponents ("time"), 0);

    EXPECT_FALSE (uniforms.set ("time", { 1.0f }));
    EXPECT_FALSE (uniforms.set ("time", 1.0f));
    EXPECT_FALSE (uniforms.set ("Intensity", 1.0f));
    EXPECT_TRUE (isZeroed (uniforms));
}

TEST_F (ShaderUniformsTests, DescribesItsMembers)
{
    const ShaderUniforms uniforms (makeBlock (48, { makeMember ("intensity", BaseType::float32, 0), makeMember ("count", BaseType::int32, 4), makeMember ("mask", BaseType::uint32, 8), makeMember ("enabled", BaseType::boolean, 12), makeMember ("direction", BaseType::float32, 16, 3), makeMember ("cell", BaseType::int32, 32, 2) }));

    EXPECT_TRUE (uniforms.hasMember ("intensity"));
    EXPECT_TRUE (uniforms.hasMember (String ("direction")));

    EXPECT_EQ (uniforms.getMemberType ("intensity"), BaseType::float32);
    EXPECT_EQ (uniforms.getMemberType ("count"), BaseType::int32);
    EXPECT_EQ (uniforms.getMemberType ("mask"), BaseType::uint32);
    EXPECT_EQ (uniforms.getMemberType ("enabled"), BaseType::boolean);
    EXPECT_EQ (uniforms.getMemberType ("cell"), BaseType::int32);

    EXPECT_EQ (uniforms.getNumComponents ("intensity"), 1);
    EXPECT_EQ (uniforms.getNumComponents ("direction"), 3);
    EXPECT_EQ (uniforms.getNumComponents ("cell"), 2);
}

TEST_F (ShaderUniformsTests, LeavesOutMembersItCannotSet)
{
    auto nested = makeArray (makeMember ("grid", BaseType::float32, 16), 2, 16);
    nested.arraySizes = { 2, 2 };

    ShaderUniforms uniforms (makeBlock (64, { makeMember ("light", BaseType::structType, 0), makeMember ("precise", BaseType::float64, 8), nested, makeMember ("outside", BaseType::float32, 64), makeMember ("straddling", BaseType::float32, 56, 4) }));

    for (const auto* name : { "light", "precise", "grid", "outside", "straddling" })
    {
        EXPECT_FALSE (uniforms.hasMember (name)) << name;
        EXPECT_FALSE (uniforms.set (name, { 1.0f, 2.0f, 3.0f, 4.0f })) << name;
    }

    // The block still has the room of every member
    EXPECT_EQ (uniforms.getSizeInBytes(), 64u);
    EXPECT_TRUE (isZeroed (uniforms));
}

TEST_F (ShaderUniformsTests, LeavesOutAnArrayDeclaringNoStride)
{
    // A reflection read from a file: with no stride every element would be the first one, however many it declares
    for (const auto count : { 2u, 65536u, 0xffffffffu })
    {
        const ShaderUniforms uniforms (makeBlock (16, { makeArray (makeMember ("levels", BaseType::float32, 0), count, 0), makeMember ("gain", BaseType::float32, 4) }));

        EXPECT_FALSE (uniforms.hasMember ("levels")) << count;
        EXPECT_EQ (uniforms.getNumComponents ("levels"), 0) << count;
        EXPECT_TRUE (uniforms.hasMember ("gain")) << count;
    }

    // An array of one element needs none
    const ShaderUniforms single (makeBlock (16, { makeArray (makeMember ("levels", BaseType::float32, 0), 1, 0) }));
    EXPECT_EQ (single.getNumComponents ("levels"), 1);
}

TEST_F (ShaderUniformsTests, SetsFromVarNumbersBooleansAndArrays)
{
    ShaderUniforms uniforms (makeBlock (48, { makeMember ("intensity", BaseType::float32, 0), makeMember ("count", BaseType::int32, 4), makeMember ("enabled", BaseType::boolean, 8), makeMember ("tint", BaseType::float32, 16, 4), makeMember ("cell", BaseType::int32, 32, 2) }));

    EXPECT_TRUE (uniforms.set ("intensity", var (0.5)));
    EXPECT_TRUE (uniforms.set ("count", var (3)));
    EXPECT_TRUE (uniforms.set ("enabled", var (true)));
    EXPECT_TRUE (uniforms.set ("tint", var::array ({ 0.25, 0.5, 0.75, 1.0 })));
    EXPECT_TRUE (uniforms.set ("cell", var::array ({ 7, true })));

    EXPECT_FLOAT_EQ (read<float> (uniforms, 0), 0.5f);
    EXPECT_EQ (read<int32> (uniforms, 4), 3);
    EXPECT_EQ (read<uint32> (uniforms, 8), 1u);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 16), 0.25f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 20), 0.5f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 24), 0.75f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 28), 1.0f);
    EXPECT_EQ (read<int32> (uniforms, 32), 7);
    EXPECT_EQ (read<int32> (uniforms, 36), 1);

    // The numbers and booleans a caller passes as they are go through var too
    EXPECT_TRUE (uniforms.set ("intensity", 0.75f));
    EXPECT_TRUE (uniforms.set ("enabled", false));
    EXPECT_FLOAT_EQ (read<float> (uniforms, 0), 0.75f);
    EXPECT_EQ (read<uint32> (uniforms, 8), 0u);

    // A number only sets the first scalar of a vector
    EXPECT_TRUE (uniforms.set ("tint", var (2.0)));
    EXPECT_FLOAT_EQ (read<float> (uniforms, 16), 2.0f);
    EXPECT_FLOAT_EQ (read<float> (uniforms, 20), 0.5f);
}

TEST_F (ShaderUniformsTests, RefusesVarsThatAreNotNumbers)
{
    ShaderUniforms uniforms (makeVectorBlock());

    EXPECT_FALSE (uniforms.set ("intensity", var ("0.5")));
    EXPECT_FALSE (uniforms.set ("intensity", var()));
    EXPECT_FALSE (uniforms.set ("tint", var::array ({ 0.25, "0.5", 0.75, 1.0 })));
    EXPECT_FALSE (uniforms.set ("time", var (1.0)));
    EXPECT_TRUE (isZeroed (uniforms));

    EXPECT_TRUE (uniforms.set ("tint", var::array ({})));
    EXPECT_TRUE (isZeroed (uniforms));
}

TEST_F (ShaderUniformsTests, ComparesMembersAndBytes)
{
    ShaderUniforms first (makeVectorBlock());
    ShaderUniforms second (makeVectorBlock());

    EXPECT_TRUE (first == second);
    EXPECT_TRUE (ShaderUniforms() == ShaderUniforms());
    EXPECT_FALSE (first == ShaderUniforms());

    EXPECT_TRUE (first.set ("intensity", 0.5f));
    EXPECT_FALSE (first == second);
    EXPECT_TRUE (first != second);

    const auto copy = first;
    EXPECT_TRUE (copy == first);

    EXPECT_TRUE (second.set ("intensity", 0.5f));
    EXPECT_TRUE (first == second);

    // The same bytes laid out for other members
    auto otherBlock = makeVectorBlock();
    otherBlock.members.front().name = "strength";

    ShaderUniforms other (otherBlock);
    EXPECT_TRUE (other.set ("strength", 0.5f));
    EXPECT_FALSE (first == other);
}

TEST_F (ShaderUniformsTests, TellsTheUniformsOfTheSameBlockWhateverTheyHold)
{
    ShaderUniforms first (makeVectorBlock());
    ShaderUniforms second (makeVectorBlock());

    EXPECT_TRUE (first.set ("intensity", 0.5f));
    EXPECT_FALSE (first == second);
    EXPECT_TRUE (first.hasSameLayoutAs (second));
    EXPECT_TRUE (second.hasSameLayoutAs (first));
    EXPECT_TRUE (ShaderUniforms().hasSameLayoutAs (ShaderUniforms()));
    EXPECT_FALSE (first.hasSameLayoutAs (ShaderUniforms()));

    // As many bytes, laid out for a member of another name, then for one at another place
    auto renamed = makeVectorBlock();
    renamed.members.front().name = "strength";
    EXPECT_FALSE (first.hasSameLayoutAs (ShaderUniforms (renamed)));

    auto moved = makeVectorBlock();
    moved.members.front().offset = 4;
    EXPECT_FALSE (first.hasSameLayoutAs (ShaderUniforms (moved)));

    // The same members in a larger block
    auto larger = makeVectorBlock();
    larger.blockSize = 64;
    EXPECT_FALSE (first.hasSameLayoutAs (ShaderUniforms (larger)));
}
