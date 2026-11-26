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

/* Returns the property a texture of a shader is filled from: the array of numbers that has its name. Returns -1
   for a texture no property names, which is a picture the shader reads. */
inline int findShaderSeriesProperty (const std::vector<ShaderEffect::Property>& properties, const String& textureName)
{
    for (size_t i = 0; i < properties.size(); ++i)
    {
        if (properties[i].type == ComponentProperty::Type::array && properties[i].name.toString() == textureName)
            return static_cast<int> (i);
    }

    return -1;
}

/* Returns a number as the 16 bit float a texture keeps it as, rounded to the nearest one. What is too large for
   one is the largest there is, and what is not a number is zero. */
inline uint16 toHalfFloat (float value) noexcept
{
    if (std::isnan (value))
        return 0;

    uint32 bits = 0;
    std::memcpy (&bits, &value, sizeof (bits));

    const auto sign = (bits >> 16) & 0x8000u;
    const auto exponent = static_cast<int> ((bits >> 23) & 0xffu) - 112;
    const auto mantissa = bits & 0x007fffffu;

    if (exponent >= 31)
        return static_cast<uint16> (sign | 0x7bffu);

    if (exponent <= 0)
    {
        // Too small for a normal number: what is left of its digits, or zero
        if (exponent < -10)
            return static_cast<uint16> (sign);

        const auto shifted = (mantissa | 0x00800000u) >> (1 - exponent);
        return static_cast<uint16> (sign | ((shifted + 0x00001000u) >> 13));
    }

    // Rounding up can carry into the exponent, which the sum takes as it is
    const auto rounded = (static_cast<uint32> (exponent) << 10) + ((mantissa + 0x00001000u) >> 13);
    return static_cast<uint16> (sign | jmin (rounded, 0x7bffu));
}

/* The numbers of a series as a shader samples them: a texture one pixel high holding one number per pixel, kept
   until the series changes. */
struct ShaderSeriesTexture
{
    GpuTexture::Ptr texture;
    GpuDevice::Ptr device;
    uint32 revision = 0;
    std::vector<float> values;
    std::vector<uint16> pixels;
};

/* Returns the texture holding a series, uploading it when it is not the one it holds: when its revision changed,
   or its numbers when the revision is not tracked. `changed` is set when the texture was written. Returns nullptr
   when the device can't make the texture. A series longer than a texture is wide keeps the numbers it has room for,
   spread over all of it, and an empty one is a single zero. */
inline GpuTexture::Ptr updateShaderSeriesTexture (ShaderSeriesTexture& kept,
                                                  const GpuDevice::Ptr& device,
                                                  Span<const float> values,
                                                  uint32 revision,
                                                  bool& changed)
{
    if (device == nullptr || ! device->isGpuAvailable())
        return nullptr;

    const auto numValues = static_cast<int> (values.size());
    const auto maxWidth = jmax (1, static_cast<int> (device->getMaximumTextureSize (GpuTextureType::texture2D)));
    const auto width = jlimit (1, maxWidth, numValues);

    const bool isMade = kept.texture != nullptr && kept.device == device && kept.texture->getWidth() == width;

    const bool isSame = isMade
                     && revision == kept.revision
                     && (revision != 0 || (kept.values.size() == values.size() && std::equal (values.begin(), values.end(), kept.values.begin())));

    if (isSame)
        return kept.texture;

    if (! isMade)
    {
        kept.texture = GpuTexture::create (device, GpuTextureDesc (static_cast<uint32_t> (width), 1, GpuTextureFormat::r16float));
        kept.device = device;
    }

    if (kept.texture == nullptr)
        return nullptr;

    kept.pixels.resize (static_cast<size_t> (width));

    for (int x = 0; x < width; ++x)
    {
        // The numbers fit the texture, but for a series longer than a texture is wide
        const auto index = numValues <= width ? x : static_cast<int> (static_cast<int64> (x) * (numValues - 1) / jmax (1, width - 1));

        kept.pixels[static_cast<size_t> (x)] = toHalfFloat (index < numValues ? values[static_cast<size_t> (index)] : 0.0f);
    }

    GpuTextureDataDesc data;
    data.data = kept.pixels.data();

    if (! kept.texture->upload (data))
    {
        kept.texture = nullptr;
        return nullptr;
    }

    // Only the numbers of a series whose revision is not tracked are compared, so only those are kept
    kept.revision = revision;
    kept.values.assign (revision == 0 ? values.begin() : values.end(), values.end());
    changed = true;

    return kept.texture;
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
                                       Span<const GpuTexture::Ptr> inputs)
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

    failed = shader.draw (device, frame, *target, uniforms, inputs).failed();
    frame.submit();

    if (failed)
        target = nullptr;

    return target != nullptr ? target->asTexture() : nullptr;
}

/* Draws an image shader reading one picture, or none. */
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
    const GpuTexture::Ptr inputs[] = { input };

    return runImageShader (context, targetDevice, target, failed, shader, uniforms, width, height, Span<const GpuTexture::Ptr> (inputs, 1));
}

} // namespace detail

//==============================================================================

/* The textures the series of the component are kept in, one for each texture of the shader, and the numbers of the
   last array that was given as a var. */
struct ShaderEffect::SeriesTextures
{
    std::vector<detail::ShaderSeriesTexture> textures;
    std::vector<float> converted;
};

//==============================================================================

ShaderEffect::ShaderEffect (Component& componentToUse, GpuImageShader::Ptr shaderToUse, std::vector<Property> propertiesToUse)
    : component (&componentToUse)
    , shader (std::move (shaderToUse))
    , properties (std::move (propertiesToUse))
    , series (std::make_unique<SeriesTextures>())
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
    series->textures.clear();
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

int ShaderEffect::getNumPictures (const GpuImageShader& shader, const std::vector<Property>& properties)
{
    int numPictures = 0;

    for (const auto& textureName : shader.getInputTextureNames())
        numPictures += detail::findShaderSeriesProperty (properties, textureName) < 0 ? 1 : 0;

    return numPictures;
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
    const auto* shown = component.get();
    const auto& textureNames = shader->getInputTextureNames();

    if (shown != nullptr)
    {
        for (const auto& property : properties)
        {
            // An array is read where the component keeps it when it can be, and not at all when it fills a texture
            if (property.type == ComponentProperty::Type::array)
            {
                if (uniforms.hasMember (property.name.toString()))
                    uniforms.set (property.name.toString(), readSeries (*shown, property.name).first);

                continue;
            }

            setUniform (uniforms, property, shown->findProperty (property.name));
        }
    }

    // The textures of the shader are filled in the order it declares them: a series for the ones an array property
    // names, and what the component painted for the first of the others. A second picture is left without a
    // texture, which the shader can't be drawn without
    std::array<GpuTexture::Ptr, GpuImageShader::maxInputTextures> inputs;
    const auto device = g.getGraphicsContext().getGpuDevice();
    bool isPictureGiven = false;

    series->textures.resize (static_cast<size_t> (textureNames.size()));

    for (int i = 0; i < textureNames.size(); ++i)
    {
        const auto property = detail::findShaderSeriesProperty (properties, textureNames[i]);

        if (property < 0)
        {
            if (! std::exchange (isPictureGiven, true))
                inputs[static_cast<size_t> (i)] = input;

            continue;
        }

        const auto numbers = shown != nullptr ? readSeries (*shown, properties[static_cast<size_t> (property)].name) : std::pair<Span<const float>, uint32>();
        bool changed = false;

        inputs[static_cast<size_t> (i)] = detail::updateShaderSeriesTexture (series->textures[static_cast<size_t> (i)], device, numbers.first, numbers.second, changed);
    }

    return detail::runImageShader (g.getGraphicsContext(),
                                   targetDevice,
                                   target,
                                   drawFailed,
                                   *shader,
                                   uniforms,
                                   input->getWidth(),
                                   input->getHeight(),
                                   Span<const GpuTexture::Ptr> (inputs.data(), static_cast<size_t> (textureNames.size())));
}

/* Returns the numbers of an array property of a component with their revision: where the component keeps them when
   it gives them as a series of numbers, else converted from the var it gives, which has no revision. */
std::pair<Span<const float>, uint32> ShaderEffect::readSeries (const Component& shown, const Identifier& name)
{
    const auto* descriptor = shown.getTypeInfo().findProperty (name);

    if (descriptor != nullptr && descriptor->isSeries() && descriptor->elementType == ComponentProperty::Type::number)
    {
        const auto view = descriptor->arrayGetter (shown);
        return { view.as<float>(), view.revision };
    }

    const auto value = shown.findProperty (name);

    auto& converted = series->converted;
    converted.clear();

    if (const auto* numbers = value.getArray())
        for (const auto& number : *numbers)
            converted.push_back (static_cast<float> (static_cast<double> (number)));

    return { Span<const float> (converted.data(), converted.size()), 0 };
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
