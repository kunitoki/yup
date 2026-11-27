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

namespace detail
{

/* Returns the property a texture of a shader is filled from: the array of numbers that has its name. Returns -1
   for a texture no property names, which is a picture the shader reads. */
inline int findShaderSeriesProperty (const std::vector<ShaderProperty>& properties, const String& textureName)
{
    for (size_t i = 0; i < properties.size(); ++i)
    {
        if (properties[i].type == ComponentProperty::Type::array && properties[i].name.toString() == textureName)
            return static_cast<int> (i);
    }

    return -1;
}

} // namespace detail

//==============================================================================

int ShaderProperty::getNumUniformComponents (ComponentProperty::Type type) noexcept
{
    using Type = ComponentProperty::Type;

    switch (type)
    {
        case Type::number:
        case Type::integer:
        case Type::boolean:
        case Type::array:
            return 1;

        case Type::point:
            return 2;

        case Type::color:
            return 3;

        case Type::rectangle:
            return 4;

        default:
            return 0;
    }
}

int ShaderProperty::getNumPictures (const GpuImageShader& shader, const std::vector<ShaderProperty>& properties)
{
    int numPictures = 0;

    for (const auto& textureName : shader.getInputTextureNames())
        numPictures += detail::findShaderSeriesProperty (properties, textureName) < 0 ? 1 : 0;

    return numPictures;
}

bool ShaderProperty::setUniform (ShaderUniforms& uniforms, const ShaderProperty& property, const var& value)
{
    return setUniform (uniforms, uniforms.findMember (property.name.toString()), property.type, value);
}

bool ShaderProperty::setUniform (ShaderUniforms& uniforms, ShaderUniforms::MemberHandle member, ComponentProperty::Type type, const var& value)
{
    using Type = ComponentProperty::Type;

    const auto setScalar = [&uniforms, member] (float scalar)
    {
        return uniforms.set (member, Span<const float> (&scalar, 1));
    };

    switch (type)
    {
        case Type::number:
            return setScalar (detail::ComponentPropertyValueTraits<float>::fromVar (value));

        case Type::integer:
            return setScalar (static_cast<float> (detail::ComponentPropertyValueTraits<int>::fromVar (value)));

        case Type::boolean:
            return setScalar (detail::ComponentPropertyValueTraits<bool>::fromVar (value) ? 1.0f : 0.0f);

        case Type::color:
        {
            const auto color = VariantConverter<Color>::fromVar (value);
            const float components[] = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat(), color.getAlphaFloat() };

            return uniforms.set (member, components);
        }

        // A point, a rectangle and an array of numbers are held as the array of their numbers
        case Type::point:
        case Type::rectangle:
        case Type::array:
            return uniforms.set (member, value);

        default:
            return false;
    }
}

std::vector<ShaderProperty::Declaration> ShaderProperty::readBlock (const DataTree& block,
                                                                    const String& shaderName,
                                                                    const ShaderUniforms* uniforms,
                                                                    const StringArray& textureNames)
{
    std::vector<Declaration> declared;

    for (const auto& element : block)
    {
        const auto name = element.getProperty ("name").toString().trim();

        if (element.getType().toString() != "property" || name.isEmpty())
            continue;

        const auto typeName = element.getProperty ("type").toString().trim();
        const auto type = ComponentProperty::getTypeFromName (typeName);

        // Of the arrays, the one of numbers fills a uniform
        const bool isFilling = type != ComponentProperty::Type::array
                            || ComponentProperty::getElementTypeFromName (typeName) == ComponentProperty::Type::number;

        const auto numComponents = isFilling ? getNumUniformComponents (type) : 0;

        // An array named as a texture of the shader fills that texture, not a member of the block
        const bool needsMember = uniforms != nullptr
                              && ! (type == ComponentProperty::Type::array && textureNames.contains (name));

        String problem;

        if (numComponents == 0)
            problem = "The property '" + name + "' of the shader '" + shaderName + "' can't be of type '" + typeName + "': a shader takes a number, an integer, a boolean, a color, a point, a rectangle or a number[]";
        else if (needsMember && ! uniforms->hasMember (name))
            problem = "The uniform block of the shader '" + shaderName + "' has no member '" + name + "'";
        else if (needsMember && uniforms->getNumComponents (name) < numComponents)
            problem = "The member '" + name + "' of the uniform block of the shader '" + shaderName + "' is too small for a " + typeName;

        declared.push_back ({ Identifier (name), type, element, std::move (problem) });
    }

    return declared;
}

} // namespace yup
