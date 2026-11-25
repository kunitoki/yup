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

/* The time given to a shader starts again after an hour: the seconds a component has been showing
   for, as a float, would only have a resolution of milliseconds after a day. */
inline double advanceShaderTime (double time, double elapsedSeconds) noexcept
{
    return std::fmod (time + elapsedSeconds, 3600.0);
}

inline bool hasShaderProperty (const std::vector<ShaderEffect::Property>& properties, StringRef name)
{
    return std::any_of (properties.begin(), properties.end(), [&name] (const ShaderEffect::Property& property)
    {
        return property.name == name;
    });
}

/* A shader is given the time when its uniform block has it and no property takes its name: what
   draws it then needs every frame. */
inline bool isShaderGivenTime (const GpuImageShader& shader, const std::vector<ShaderEffect::Property>& properties)
{
    return ! hasShaderProperty (properties, "time") && shader.createUniforms().hasMember ("time");
}

/* Fills the members a shader is given beside its properties, each when the uniform block has it and
   no property takes its name: the size of its picture in pixels with the reciprocals of both, the
   pixels a unit takes, and the seconds gone by. */
inline void setShaderStandardUniforms (ShaderUniforms& uniforms,
                                       const std::vector<ShaderEffect::Property>& properties,
                                       int width,
                                       int height,
                                       float scale,
                                       double time)
{
    if (! hasShaderProperty (properties, "resolution"))
    {
        const float resolution[] = { static_cast<float> (width),
                                     static_cast<float> (height),
                                     1.0f / static_cast<float> (width),
                                     1.0f / static_cast<float> (height) };

        uniforms.set ("resolution", resolution);
    }

    if (! hasShaderProperty (properties, "scale"))
        uniforms.set ("scale", scale);

    if (! hasShaderProperty (properties, "time"))
        uniforms.set ("time", static_cast<float> (time));
}

/* Draws an image shader into a target kept at a size for a device, and returns its texture, or
   nullptr when there is none. A shader that could not be drawn is not tried again with that device
   until `failed` is cleared: it keeps no target, and no frame is begun for it. */
inline GpuTexture::Ptr runImageShader (GraphicsContext& context,
                                       GpuDevice::Ptr& targetDevice,
                                       GpuTarget::Ptr& target,
                                       bool& failed,
                                       const GpuImageShader& shader,
                                       const ShaderUniforms& uniforms,
                                       int width,
                                       int height,
                                       const GpuTexture::Ptr& input = nullptr)
{
    const auto device = context.getGpuDevice();

    // A target belongs to its device, and so does what prevented a draw: the component may be painted by another window
    if (device != targetDevice)
    {
        targetDevice = device;
        target = nullptr;
        failed = false;
    }

    if (device == nullptr || failed)
        return nullptr;

    if (! ensureComponentEffectTarget (device, target, width, height))
        return nullptr;

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return nullptr;

    failed = shader.draw (device, frame, *target, uniforms, input).failed();
    frame.submit();

    if (failed)
        target = nullptr;

    return target != nullptr ? target->asTexture() : nullptr;
}

} // namespace detail

//==============================================================================

ShaderEffect::ShaderEffect (Component& componentToUse, GpuImageShader::Ptr shaderToUse, std::vector<Property> propertiesToUse)
    : component (&componentToUse)
    , shader (std::move (shaderToUse))
    , properties (std::move (propertiesToUse))
{
    updateFrameAttachment();
}

ShaderEffect::~ShaderEffect() = default;

//==============================================================================

void ShaderEffect::setShader (GpuImageShader::Ptr newShader)
{
    if (shader == newShader)
        return;

    shader = std::move (newShader);
    drawFailed = false;
    updateFrameAttachment();

    if (auto* shown = component.get())
        shown->repaint();
}

//==============================================================================

int ShaderEffect::getNumUniformComponents (ComponentProperty::Type type) noexcept
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

bool ShaderEffect::setUniform (ShaderUniforms& uniforms, const Property& property, const var& value)
{
    using Type = ComponentProperty::Type;

    const auto& name = property.name.toString();

    switch (property.type)
    {
        case Type::number:
            return uniforms.set (name, detail::ComponentPropertyValueTraits<float>::fromVar (value));

        case Type::integer:
            return uniforms.set (name, detail::ComponentPropertyValueTraits<int>::fromVar (value));

        case Type::boolean:
            return uniforms.set (name, detail::ComponentPropertyValueTraits<bool>::fromVar (value));

        case Type::color:
        {
            const auto color = VariantConverter<Color>::fromVar (value);
            const float components[] = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat(), color.getAlphaFloat() };

            return uniforms.set (name, components);
        }

        // A point, a rectangle and an array of numbers are held as the array of their numbers
        case Type::point:
        case Type::rectangle:
        case Type::array:
            return uniforms.set (name, value);

        default:
            return false;
    }
}

//==============================================================================

void ShaderEffect::updateFrameAttachment()
{
    auto* shown = component.get();

    // Only a shader that is given the time needs every frame of the window
    const bool isAnimated = shown != nullptr
                         && shader != nullptr
                         && detail::isShaderGivenTime (*shader, properties);

    if (! isAnimated)
        frameAttachment.reset();
    else if (! frameAttachment.has_value())
        frameAttachment.emplace (*shown, bindFront (&ShaderEffect::advance, this));
}

void ShaderEffect::advance (double elapsedSeconds)
{
    auto* shown = component.get();

    // The time stands still while the component is hidden, once it was given another effect, and
    // while the shader can't be drawn: repainting would show nothing new
    if (shown == nullptr || ! shown->isShowing() || shown->getComponentEffect().get() != this || drawFailed)
        return;

    time = detail::advanceShaderTime (time, elapsedSeconds);
    shown->repaint();
}

//==============================================================================

GpuTexture::Ptr ShaderEffect::render (Graphics& g, const GpuTexture::Ptr& input)
{
    if (shader == nullptr)
        return nullptr;

    auto uniforms = shader->createUniforms();

    detail::setShaderStandardUniforms (uniforms, properties, input->getWidth(), input->getHeight(), g.getContextScale(), time);

    // A window is painted while the message thread waits, so what that thread writes is read here
    // as it is: the properties of the component, and the shader
    if (const auto* shown = component.get())
    {
        for (const auto& property : properties)
            setUniform (uniforms, property, shown->findProperty (property.name));
    }

    return detail::runImageShader (g.getGraphicsContext(), targetDevice, target, drawFailed, *shader, uniforms, input->getWidth(), input->getHeight(), input);
}

void ShaderEffect::apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds)
{
    if (inputTexture == nullptr || ! inputTexture->isValid() || bounds.isEmpty())
        return;

    if (const auto output = render (g, inputTexture))
        g.drawTexture (output, bounds);
    else
        g.drawTexture (inputTexture, bounds);
}

} // namespace yup
