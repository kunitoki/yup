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
    return ! hasShaderProperty (properties, "time") && shader.getUniformLayout().hasMember ("time");
}

/* The members of a uniform block a shader is given beside its properties, found once: the size of its picture, the
   pixels a unit takes and the seconds gone by. One is not valid when the block does not have it, or when a property
   takes its name. */
struct ShaderStandardMembers
{
    ShaderUniforms::MemberHandle resolution;
    ShaderUniforms::MemberHandle scale;
    ShaderUniforms::MemberHandle time;
};

/* Finds once the members a shader is given beside its properties: each when the uniform block has it and no property
   takes its name. Uniforms of that layout are then given them with no search. */
inline ShaderStandardMembers findShaderStandardMembers (const ShaderUniforms& uniforms, const std::vector<ShaderEffect::Property>& properties)
{
    const auto find = [&uniforms, &properties] (const char* name)
    {
        return hasShaderProperty (properties, name) ? ShaderUniforms::MemberHandle() : uniforms.findMember (name);
    };

    return { find ("resolution"), find ("scale"), find ("time") };
}

/* Fills the members a shader is given beside its properties, found by findShaderStandardMembers(): the size of its
   picture in pixels with the reciprocals of both, the pixels a unit takes, and the seconds gone by. */
inline void setShaderStandardUniforms (ShaderUniforms& uniforms,
                                       const ShaderStandardMembers& members,
                                       int width,
                                       int height,
                                       float scale,
                                       double time)
{
    const float resolution[] = { static_cast<float> (width),
                                 static_cast<float> (height),
                                 1.0f / static_cast<float> (width),
                                 1.0f / static_cast<float> (height) };

    const auto seconds = static_cast<float> (time);

    uniforms.set (members.resolution, resolution);
    uniforms.set (members.scale, Span<const float> (&scale, 1));
    uniforms.set (members.time, Span<const float> (&seconds, 1));
}

/* Fills the members a shader is given beside its properties, each when the uniform block has it and
   no property takes its name, looking for them by name: what fills uniforms once. */
inline void setShaderStandardUniforms (ShaderUniforms& uniforms,
                                       const std::vector<ShaderEffect::Property>& properties,
                                       int width,
                                       int height,
                                       float scale,
                                       double time)
{
    setShaderStandardUniforms (uniforms, findShaderStandardMembers (uniforms, properties), width, height, scale, time);
}

/* A member a shader is given beside its properties, with what its uniform block has to declare it as. */
struct ShaderStandardUniform
{
    const char* name;
    bool isSize;
    const char* shape;
};

inline constexpr ShaderStandardUniform shaderStandardUniforms[] = { { "resolution", true, "a vec2 or a vec4" },
                                                                    { "scale", false, "a float" },
                                                                    { "time", false, "a float" } };

/* Returns whether a uniform block declares a member of the name of what a shader is given as something else: the size of
   the picture fills a vec2, or a vec4 with the reciprocals of both, and the others a float. Such a member is a value of
   the author, which what the shader is given would overwrite unless a property of its name fills it. */
inline bool isShaderStandardUniformMismatched (const ShaderUniforms& uniforms, const ShaderStandardUniform& given)
{
    if (! uniforms.hasMember (given.name))
        return false;

    const auto numComponents = uniforms.getNumComponents (given.name);
    const bool hasRoom = given.isSize ? (numComponents == 2 || numComponents == 4) : numComponents == 1;

    return ! hasRoom || uniforms.getMemberType (given.name) != ShaderReflection::BaseType::float32;
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

/* Returns whether an effect has what a shader reads: at most one picture, which is its component. */
inline bool isShaderDrawnByAnEffect (const GpuImageShader* shader, const std::vector<ShaderEffect::Property>& properties)
{
    return shader != nullptr && ShaderEffect::getNumPictures (*shader, properties) <= 1;
}

/* Returns the device of the window a component is in, or nullptr when it is in none. */
inline GpuDevice::Ptr findWindowDevice (Component* component)
{
    auto* native = component != nullptr ? component->getNativeComponent() : nullptr;
    auto* context = native != nullptr ? native->getGraphicsContext() : nullptr;

    return context != nullptr ? context->getGpuDevice() : nullptr;
}

/* Has the stylesheet of a component tell what a device refused of a shader it was painted with. That is found while the
   window is painted, where nothing is reported: the component is marked for the next restyle, which tells it. */
inline void requestShaderFailureReport (Component* component)
{
    if (component == nullptr)
        return;

    if (auto* context = component->findStyleContext())
        context->markDirty (*component);
}

/* Draws an image shader into a target kept at a size for a device, and returns its texture, or
   nullptr when there is none. A shader the device refuses is not tried again with it until `error`,
   which keeps why, is cleared: it keeps no target, and no frame is begun for it. What else a draw
   needs may be there at the next one, so its lack sets nothing: a GPU, a texture for each input, a
   target and a frame. */
inline GpuTexture::Ptr runImageShader (GraphicsContext& context,
                                       GpuDevice::Ptr& targetDevice,
                                       GpuTarget::Ptr& target,
                                       String& error,
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
        error.clear();
    }

    if (device == nullptr || error.isNotEmpty() || ! device->isGpuAvailable() || device->isDeviceLost())
        return nullptr;

    const auto numInputs = static_cast<size_t> (shader.getNumInputTextures());

    if (inputs.size() < numInputs)
        return nullptr;

    for (size_t i = 0; i < numInputs; ++i)
        if (inputs[i] == nullptr || ! inputs[i]->isValid())
            return nullptr;

    if (! ensureComponentEffectTarget (device, target, width, height))
        return nullptr;

    auto frame = GpuFrame::begin (device);
    if (! frame.isValid())
        return nullptr;

    // With all it needs at hand, a draw only fails for what the device refuses of the shader: a pipeline it can't compile
    const auto drawn = shader.draw (device, frame, *target, uniforms, inputs);
    frame.submit();

    if (drawn.failed())
    {
        error = drawn.getErrorMessage();
        target = nullptr;

        YUP_DBG ("An image shader can't be drawn by the device, what it is drawn for is left without it: " << error);
    }

    return target != nullptr ? target->asTexture() : nullptr;
}

/* Draws an image shader reading one picture, or none. */
inline GpuTexture::Ptr runImageShader (GraphicsContext& context,
                                       GpuDevice::Ptr& targetDevice,
                                       GpuTarget::Ptr& target,
                                       String& error,
                                       const GpuImageShader& shader,
                                       const ShaderUniforms& uniforms,
                                       int width,
                                       int height,
                                       const GpuTexture::Ptr& input = nullptr)
{
    const GpuTexture::Ptr inputs[] = { input };

    return runImageShader (context, targetDevice, target, error, shader, uniforms, width, height, Span<const GpuTexture::Ptr> (inputs, 1));
}

} // namespace detail

//==============================================================================

/* What is found once for the shader, so that no paint looks for a name: the member each property fills, the ones the
   effect fills itself, and the property each texture is filled from, negative for a picture. */
struct ShaderEffect::ShaderMembers
{
    std::vector<ShaderUniforms::MemberHandle> properties;
    detail::ShaderStandardMembers standard;
    std::array<int, GpuImageShader::maxInputTextures> textureProperties {};
};

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
    , members (std::make_unique<ShaderMembers>())
    , series (std::make_unique<SeriesTextures>())
    , canDraw (detail::isShaderDrawnByAnEffect (shader.get(), properties))
    , isGivenTime (canDraw && detail::isShaderGivenTime (*shader, properties))
{
    resolveShader();
}

ShaderEffect::~ShaderEffect() = default;

//==============================================================================

/* Finds once what every paint needs of the shader: its uniforms and its members. */
void ShaderEffect::resolveShader()
{
    filledUniforms = shader != nullptr ? shader->createUniforms() : ShaderUniforms();
    drawnUniforms = filledUniforms;

    members->standard = detail::findShaderStandardMembers (filledUniforms, properties);
    members->properties.clear();

    for (const auto& property : properties)
        members->properties.push_back (filledUniforms.findMember (property.name.toString()));

    members->textureProperties.fill (-1);

    if (shader == nullptr)
        return;

    const auto& textureNames = shader->getInputTextureNames();

    for (int i = 0; i < jmin (textureNames.size(), GpuImageShader::maxInputTextures); ++i)
        members->textureProperties[static_cast<size_t> (i)] = detail::findShaderSeriesProperty (properties, textureNames[i]);
}

void ShaderEffect::setShader (GpuImageShader::Ptr newShader)
{
    if (shader == newShader)
        return;

    shader = std::move (newShader);
    canDraw = detail::isShaderDrawnByAnEffect (shader.get(), properties);
    isGivenTime = canDraw && detail::isShaderGivenTime (*shader, properties);
    resolveShader();
    drawError.clear();
    isStalled = false;
    series->textures.clear();
    drawnInput = nullptr;
    updateFrameAttachment();
    invalidate();
}

bool ShaderEffect::isActive() const
{
    // What a device refused is asked again of another one: the component may be in another window by now
    return canDraw && (drawError.isEmpty() || detail::findWindowDevice (component.get()) != targetDevice);
}

void ShaderEffect::releaseResources()
{
    // With the device goes what it refused: the shader is asked of the device of the next window
    target = nullptr;
    drawnInput = nullptr;
    series->textures.clear();
    targetDevice = nullptr;
    drawError.clear();
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
    return setUniform (uniforms, uniforms.findMember (property.name.toString()), property.type, value);
}

bool ShaderEffect::setUniform (ShaderUniforms& uniforms, ShaderUniforms::MemberHandle member, ComponentProperty::Type type, const var& value)
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

Result ShaderEffect::checkProperties (const GpuImageShader& shader, const std::vector<Property>& properties)
{
    const auto& uniforms = shader.getUniformLayout();

    for (const auto& property : properties)
    {
        const auto& name = property.name.toString();
        const auto numComponents = getNumUniformComponents (property.type);

        if (numComponents == 0)
            return Result::fail ("The property '" + name + "' has a type a shader can't take: it takes a number, an integer, a boolean, a color, a point, a rectangle or an array of numbers");

        // An array named as a texture of the shader fills that texture, not a member of the block
        if (property.type == ComponentProperty::Type::array && shader.getInputTextureNames().contains (name))
            continue;

        if (! uniforms.hasMember (name))
            return Result::fail ("The uniform block of the shader has no member '" + name + "'");

        if (uniforms.getNumComponents (name) < numComponents)
            return Result::fail ("The member '" + name + "' of the uniform block of the shader is too small for the property");
    }

    for (const auto& given : detail::shaderStandardUniforms)
    {
        if (! detail::hasShaderProperty (properties, given.name) && detail::isShaderStandardUniformMismatched (uniforms, given))
            return Result::fail (String ("The member '") + given.name + "' of the uniform block of the shader is not " + given.shape
                                 + ", which is what a shader is given there: a property of that name has to fill it");
    }

    return Result::ok();
}

//==============================================================================

void ShaderEffect::updateFrameAttachment()
{
    auto* shown = component.get();

    // Only a shader that is given the time needs the frames of the window, and only while they show it: once it was
    // drawn, in a component that is showing with this effect. Without them the window is left to idle
    const bool isAnimated = isGivenTime
                         && ! isStalled
                         && shown != nullptr
                         && shown->isShowing()
                         && shown->getComponentEffect().get() == this;

    if (! isAnimated)
        frameAttachment.reset();
    else if (! frameAttachment.has_value())
        frameAttachment.emplace (*shown, bindFront (&ShaderEffect::advance, this));
}

void ShaderEffect::advance (double elapsedSeconds)
{
    // The time stands still while the component is hidden, once it was given another effect, and while the shader
    // is not drawn: the frame asked for last showed nothing of it. The paint that draws it starts the frames again
    updateFrameAttachment();

    if (! frameAttachment.has_value())
        return;

    time = detail::advanceShaderTime (time, elapsedSeconds);

    // Another frame is asked for once the shader was drawn in this one, over what the component painted last
    isStalled = true;
    invalidate();
}

//==============================================================================

GpuTexture::Ptr ShaderEffect::render (Graphics& g, const GpuTexture::Ptr& input)
{
    const auto device = g.getGraphicsContext().getGpuDevice();

    // What the device refused is not asked of it again, and nothing is read for it
    if (! canDraw || (drawError.isNotEmpty() && device == targetDevice))
        return nullptr;

    // The uniforms are kept from a paint to the next, not what they held: a member is zero when its property has
    // nothing to give it
    auto& uniforms = filledUniforms;
    uniforms.clear();

    detail::setShaderStandardUniforms (uniforms, members->standard, input->getWidth(), input->getHeight(), g.getContextScale(), time);

    // A window is painted while the message thread waits, so what that thread writes is read here
    // as it is: the properties of the component, and the shader
    const auto* shown = component.get();
    const auto& textureNames = shader->getInputTextureNames();

    if (shown != nullptr)
    {
        for (size_t i = 0; i < properties.size(); ++i)
        {
            const auto& property = properties[i];
            const auto member = members->properties[i];

            // A property the block has no member for is not read: an array filling a texture is, below
            if (! member.isValid())
                continue;

            // An array is read where the component keeps it when it can be
            if (property.type == ComponentProperty::Type::array)
                uniforms.set (member, readSeries (*shown, property.name).first);
            else
                setUniform (uniforms, member, property.type, shown->findProperty (property.name));
        }
    }

    // The textures of the shader are filled in the order it declares them: a series for the ones an array property
    // names, and what the component painted for the other one, as a shader that is drawn reads one picture at most
    std::array<GpuTexture::Ptr, GpuImageShader::maxInputTextures> inputs;
    bool isSameSeries = true;

    series->textures.resize (static_cast<size_t> (textureNames.size()));

    for (int i = 0; i < textureNames.size(); ++i)
    {
        const auto property = members->textureProperties[static_cast<size_t> (i)];

        if (property < 0)
        {
            inputs[static_cast<size_t> (i)] = input;
            continue;
        }

        const auto numbers = shown != nullptr ? readSeries (*shown, properties[static_cast<size_t> (property)].name) : std::pair<Span<const float>, uint32>();
        bool changed = false;

        inputs[static_cast<size_t> (i)] = detail::updateShaderSeriesTexture (series->textures[static_cast<size_t> (i)], device, numbers.first, numbers.second, changed);

        // A series written again, or one with no texture to be in, leaves nothing of the picture drawn last
        isSameSeries = isSameSeries && ! changed && inputs[static_cast<size_t> (i)] != nullptr;
    }

    // The picture drawn last is the one to draw while nothing it was made of changed: the uniforms, the series and what
    // the component painted, which is given as another texture each time it is painted again
    if (target != nullptr && device == targetDevice && input == drawnInput && uniforms == drawnUniforms && isSameSeries)
        return target->asTexture();

    auto output = detail::runImageShader (g.getGraphicsContext(),
                                          targetDevice,
                                          target,
                                          drawError,
                                          *shader,
                                          uniforms,
                                          input->getWidth(),
                                          input->getHeight(),
                                          Span<const GpuTexture::Ptr> (inputs.data(), static_cast<size_t> (textureNames.size())));

    // Only the draw above leaves an error: what the device refused is told where the shader is declared
    if (drawError.isNotEmpty())
        detail::requestShaderFailureReport (component.get());

    drawnInput = output != nullptr ? input : GpuTexture::Ptr();

    // Both are the uniforms of the shader: the ones drawn are kept to compare the next ones with, which are filled anew
    std::swap (filledUniforms, drawnUniforms);

    return output;
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

    const auto output = render (g, inputTexture);

    // A shader given the time takes the frames of its window once it is drawn, and none while it is not
    isStalled = output == nullptr;
    updateFrameAttachment();

    if (output != nullptr)
        g.drawTexture (output, bounds);
    else
        g.drawTexture (inputTexture, bounds);
}

} // namespace yup
