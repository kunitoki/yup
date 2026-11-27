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

#pragma once

namespace yup
{

//==============================================================================
/**
    An input of a shader: a value of a type, written to the member of the uniform block of the
    shader that has its name.

    A `shader` block of a `.yui` file declares one with each of its `property` lines (see
    readBlock), and from C++ a ShaderEffect is given the ones its component fills. What draws
    the shader writes the value of each to its uniforms with setUniform():

    @code
    const std::vector<ShaderProperty> properties { { "intensity", ComponentProperty::Type::number },
                                                   { "tint", ComponentProperty::Type::color } };

    auto uniforms = shader->createUniforms();

    ShaderProperty::setUniform (uniforms, properties[0], 0.5);
    ShaderProperty::setUniform (uniforms, properties[1], VariantConverter<Color>::toVar (Colors::orange));
    @endcode

    @see ShaderEffect, ShaderUniforms, GpuImageShader, StyleContext::findShader

    @tags{UI}
*/
struct YUP_API ShaderProperty
{
    /** The name of the property, and of the member of the uniform block it fills. */
    Identifier name;

    /** The type of the property, which tells how its value fills the member:
        ComponentProperty::Type::number, integer, boolean, color, point or rectangle, or
        ComponentProperty::Type::array for an array of numbers. */
    ComponentProperty::Type type = ComponentProperty::Type::number;

    //==============================================================================
    /** Returns how many scalars the member of a uniform block has to hold at least to take a
        property of a type.

        @param type  The type of the property.

        @returns 1 for a number, an integer, a boolean and an array, 2 for a point, 3 for a
                 color, which a `vec3` takes without its alpha, 4 for a rectangle, and 0 for the
                 types a uniform can't take.
    */
    static int getNumUniformComponents (ComponentProperty::Type type) noexcept;

    /** Writes the value of a property to the member of a uniform block that has its name,
        converted by the type of the property.

        This is the conversion a ShaderEffect makes at every paint, for what else fills the
        uniforms of a shader from typed values:

        - a number, an integer and a boolean set the first scalar of the member;
        - a color sets four: red, green, blue and alpha from 0 to 1, as the color is written,
          sRGB encoded and not premultiplied by its alpha;
        - a point sets two, and a rectangle four: x, y, width and height;
        - an array of numbers sets as many scalars as it holds.

        The scalars past the end of the member are ignored, and the ones a value does not set are
        left as they are (see ShaderUniforms::set).

        @param uniforms  The uniforms to write into.
        @param property  The name of the member and the type of the value.
        @param value     The value, as a property of that type holds it (see VariantConverter).

        @returns False when nothing was written: the block has no member of that name, the type is
                 not one a uniform takes, or the value of a point, a rectangle or an array is not
                 made of numbers.
    */
    static bool setUniform (ShaderUniforms& uniforms, const ShaderProperty& property, const var& value);

    /** Writes the value of a property to a member of a uniform block found once, converted by
        the type of the property as the overload taking the name of the member does.

        Setting by name looks for the member each time. What fills the same uniforms again and
        again finds each member once, with ShaderUniforms::findMember() on the uniforms of the
        shader (see GpuImageShader::getUniformLayout), and gives its handle here.

        @param uniforms  The uniforms to write into.
        @param member    The handle of the member to write.
        @param type      The type of the value.
        @param value     The value, as a property of that type holds it (see VariantConverter).

        @returns False when nothing was written: the handle is not valid, the type is not one a
                 uniform takes, or the value of a point, a rectangle or an array is not made of
                 numbers.
    */
    static bool setUniform (ShaderUniforms& uniforms, ShaderUniforms::MemberHandle member, ComponentProperty::Type type, const var& value);

    /** Returns how many pictures a shader reads, given the properties it is filled from.

        A texture of the shader that an array property has the name of is no picture: it is
        filled with the numbers of that property, one per pixel (see ShaderEffect). The other
        textures are the pictures the shader reads: what the component painted, for an effect.

        @param shader      The shader.
        @param properties  The properties the shader is filled from.
    */
    static int getNumPictures (const GpuImageShader& shader, const std::vector<ShaderProperty>& properties);

    //==============================================================================
    /** A property a `shader` element declares, as readBlock() reads it. */
    struct Declaration
    {
        /** The name the element declares. */
        Identifier name;

        /** The type the element declares, ComponentProperty::Type::any for a name that is no type. */
        ComponentProperty::Type type = ComponentProperty::Type::any;

        /** The `property` element, the DataTree form of a `property` line of a `shader` block. It
            holds the type as it is written as its `type` and, as its `value`, the expression of
            the default of the property when it has one.
        */
        DataTree element;

        /** Why the property can't fill the shader, to tell at the element. Empty when it can. */
        String problem;
    };

    /** Reads the properties a `shader` element declares, in the order it declares them, and
        finds the ones that can't fill the shader.

        @param block         The `shader` element, the DataTree form of a `shader` block of a
                             `.yui` file. Each of its `property` children that has a `name` is a
                             property, and its other children are skipped.
        @param shaderName    The name of the shader, which the problems tell.
        @param uniforms      The uniforms of the shader compiled from the block (see
                             GpuImageShader::getUniformLayout), or nullptr when it is not
                             compiled: only the types are then checked.
        @param textureNames  The textures the shader reads (see GpuImageShader::getInputTextureNames).
                             An array of numbers named as one of them fills that texture, and
                             needs no member of the uniform block.

        @returns A Declaration for each property. The problem of one tells a type a uniform can't
                 take (see getNumUniformComponents), an array of anything else than numbers
                 among them, a name no member of the uniform block has, or a member holding fewer
                 scalars than the type needs.
    */
    static std::vector<Declaration> readBlock (const DataTree& block,
                                               const String& shaderName,
                                               const ShaderUniforms* uniforms = nullptr,
                                               const StringArray& textureNames = {});
};

} // namespace yup
