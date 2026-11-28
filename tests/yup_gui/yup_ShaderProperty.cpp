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

#include <yup_gui/yup_gui.h>

#include <gtest/gtest.h>

using namespace yup;

//==============================================================================
// ShaderProperty - the properties a `shader` block declares, read once for what loads the shader
//==============================================================================

class ShaderPropertyTests : public ::testing::Test
{
protected:
    using Type = ComponentProperty::Type;

    /** A `property` line of a block: its name, its type as written and the expression of its default. */
    static DataTree makeProperty (const String& name, const String& type, const String& value = {})
    {
        if (value.isEmpty())
            return DataTree ("property", { { "name", name }, { "type", type } });

        return DataTree ("property", { { "name", name }, { "type", type }, { "value", value } });
    }

    static DataTree makeBlock (const std::initializer_list<DataTree>& children)
    {
        return DataTree ("shader", { { "name", "Crt" }, { "fragment", "fx/crt.frag" } }, children);
    }

    /** The uniforms of the std140 block { vec4 tint; vec2 origin; float amount; }. */
    static ShaderUniforms makeUniforms()
    {
        const auto makeMember = [] (const String& name, uint32 offset, uint32 vecSize)
        {
            ShaderReflection::ResourceMember member;
            member.name = name;
            member.baseType = ShaderReflection::BaseType::float32;
            member.offset = offset;
            member.vecSize = vecSize;
            member.size = vecSize * 4;
            return member;
        };

        ShaderReflection::ResourceBinding block;
        block.name = "Parameters";
        block.type = ShaderReflection::ResourceType::uniformBuffer;
        block.blockSize = 32;
        block.members = { makeMember ("tint", 0, 4), makeMember ("origin", 16, 2), makeMember ("amount", 24, 1) };

        return ShaderUniforms (block);
    }
};

TEST_F (ShaderPropertyTests, ReadsThePropertiesOfABlockInTheOrderItDeclaresThem)
{
    const auto block = makeBlock ({ makeProperty ("amount", "number", "0.5"),
                                    DataTree ("declaration", { { "name", "ignored" }, { "type", "number" } }),
                                    makeProperty ("  ", "number"),
                                    makeProperty (" tint ", " color "),
                                    makeProperty ("levels", "number[]") });

    const auto declared = ShaderProperty::readBlock (block, "Crt");

    ASSERT_EQ (declared.size(), 3u);

    EXPECT_EQ (declared[0].name, Identifier ("amount"));
    EXPECT_EQ (declared[0].type, Type::number);
    EXPECT_EQ (declared[0].element.getProperty ("value").toString(), "0.5");

    EXPECT_EQ (declared[1].name, Identifier ("tint"));
    EXPECT_EQ (declared[1].type, Type::color);
    EXPECT_FALSE (declared[1].element.hasProperty ("value"));

    EXPECT_EQ (declared[2].name, Identifier ("levels"));
    EXPECT_EQ (declared[2].type, Type::array);

    // Without the uniforms of the shader only the types are checked
    for (const auto& property : declared)
        EXPECT_TRUE (property.problem.isEmpty()) << property.problem;
}

TEST_F (ShaderPropertyTests, TellsTheTypesAUniformCantTake)
{
    const auto declared = ShaderProperty::readBlock (makeBlock ({ makeProperty ("label", "string"),
                                                                  makeProperty ("names", "string[]"),
                                                                  makeProperty ("shape", "circle") }),
                                                     "Crt");

    ASSERT_EQ (declared.size(), 3u);

    EXPECT_EQ (declared[0].problem, "The property 'label' of the shader 'Crt' can't be of type 'string': a shader takes a number, an integer, a boolean, a color, a point, a rectangle or a number[]");
    EXPECT_TRUE (declared[1].problem.startsWith ("The property 'names' of the shader 'Crt' can't be of type 'string[]'")) << declared[1].problem;
    EXPECT_TRUE (declared[2].problem.startsWith ("The property 'shape' of the shader 'Crt' can't be of type 'circle'")) << declared[2].problem;

    EXPECT_EQ (declared[0].type, Type::string);
    EXPECT_EQ (declared[2].type, Type::any);
}

TEST_F (ShaderPropertyTests, ChecksThePropertiesAgainstTheUniformsOfTheShader)
{
    const auto uniforms = makeUniforms();

    const auto declared = ShaderProperty::readBlock (makeBlock ({ makeProperty ("tint", "color"),
                                                                  makeProperty ("amount", "number"),
                                                                  makeProperty ("missing", "number"),
                                                                  makeProperty ("origin", "rectangle"),
                                                                  makeProperty ("label", "string") }),
                                                     "Crt",
                                                     &uniforms);

    ASSERT_EQ (declared.size(), 5u);

    EXPECT_TRUE (declared[0].problem.isEmpty()) << declared[0].problem;
    EXPECT_TRUE (declared[1].problem.isEmpty()) << declared[1].problem;
    EXPECT_EQ (declared[2].problem, "The uniform block of the shader 'Crt' has no member 'missing'");
    EXPECT_EQ (declared[3].problem, "The member 'origin' of the uniform block of the shader 'Crt' is too small for a rectangle");

    // The type is told before the member is looked for
    EXPECT_TRUE (declared[4].problem.startsWith ("The property 'label' of the shader 'Crt' can't be of type 'string'")) << declared[4].problem;
}

TEST_F (ShaderPropertyTests, AnArrayOfNumbersNamedAsATextureNeedsNoMember)
{
    const auto uniforms = makeUniforms();
    const auto block = makeBlock ({ makeProperty ("levels", "number[]"), makeProperty ("amount", "number") });

    // A series fills the texture of its name, and any other property a member of the block whatever the textures are
    const auto filling = ShaderProperty::readBlock (block, "Crt", &uniforms, { "levels", "amount" });
    ASSERT_EQ (filling.size(), 2u);
    EXPECT_TRUE (filling[0].problem.isEmpty()) << filling[0].problem;
    EXPECT_TRUE (filling[1].problem.isEmpty()) << filling[1].problem;

    const auto withoutTexture = ShaderProperty::readBlock (block, "Crt", &uniforms);
    ASSERT_EQ (withoutTexture.size(), 2u);
    EXPECT_EQ (withoutTexture[0].problem, "The uniform block of the shader 'Crt' has no member 'levels'");

    // An array of anything else fills neither
    const auto colors = ShaderProperty::readBlock (makeBlock ({ makeProperty ("levels", "color[]") }), "Crt", &uniforms, { "levels" });
    ASSERT_EQ (colors.size(), 1u);
    EXPECT_TRUE (colors[0].problem.startsWith ("The property 'levels' of the shader 'Crt' can't be of type 'color[]'")) << colors[0].problem;
}
