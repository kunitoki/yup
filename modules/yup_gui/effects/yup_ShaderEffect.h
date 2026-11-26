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
    Draws a component through a fragment shader, fed by the properties of the component.

    The effect runs a GpuImageShader over what the component and its children painted, and
    draws the result in their place. At every paint it fills the uniform block of the shader
    from the component: each Property names a property of the component, read with
    Component::findProperty() and written to the member of the block that has its name. What
    sets the property then drives the shader, and repaints the component as any property
    change does: a stylesheet rule, a transition, a binding or Component::setProperty().

    It is what an `effect:` declaration installs when it names a `shader` block of the
    stylesheet, the properties being the ones the block declares (see StyleContext::findShader):

    @code
    shader Crt {
        fragment: "fx/crt.frag";
        property intensity: number = 0.5;
        property tint: color = palette.accent;
    }

    style {
        .display        { effect: Crt; intensity: 0.4; transition: intensity 200ms; }
        .display:hover  { intensity: 0.9; }
    }
    @endcode

    From C++, give it the shader and the properties to read:

    @code
    auto shader = GpuImageShader::createFromData (data, size);

    if (shader.wasOk())
    {
        display.setComponentEffect (new ShaderEffect (display, shader.getValue(), { { "intensity", ComponentProperty::Type::number },
                                                                                    { "tint", ComponentProperty::Type::color } }));

        display.setProperty ("intensity", 0.4);
        display.setColor ("tint", Colors::orange);
    }
    @endcode

    The shader is written as GpuImageShader describes: GLSL 450, with one uniform block, a
    `texture2D` and a `sampler` to read the picture of the component, `uv` going from its top
    left to its bottom right, and colors premultiplied by alpha, read and written.

    @code
    #version 450

    layout(set = 0, binding = 0) uniform Parameters
    {
        vec4 tint;          // property tint: color
        float intensity;    // property intensity: number
        vec2 resolution;    // filled by the effect
    } params;

    layout(set = 0, binding = 1) uniform texture2D source;
    layout(set = 0, binding = 2) uniform sampler sourceSampler;

    layout(location = 0) in vec2 uv;
    layout(location = 0) out vec4 color;

    void main()
    {
        vec4 pixel = texture (sampler2D (source, sourceSampler), uv);
        vec4 tint = vec4 (params.tint.rgb * params.tint.a, params.tint.a);    // premultiplied, as the picture is

        color = mix (pixel, pixel * tint, params.intensity);
    }
    @endcode

    A property fills the member by its type:

    - `number`, `integer` and `boolean`: a scalar, `float`, `int`, `uint` or `bool`;
    - `color`: a `vec4` of red, green, blue and alpha from 0 to 1, as the color is written:
      sRGB encoded, and not premultiplied by its alpha. A `vec3` takes it without the alpha;
    - `point`: a `vec2`;
    - `rectangle`: a `vec4` of x, y, width and height;
    - an array of numbers: an array, or a vector, of as many scalars.

    A property the component has no value for leaves the member at zero. A stylesheet gives
    the component the defaults of the `shader` block as styled values, so they are read here as
    any other value.

    An array of numbers also fills a texture: when the fragment stage declares a `texture2D` of
    the name of the property instead of a member of the block, the numbers are written to a
    texture one pixel high, one number per pixel, which the shader samples along `x` from 0 at
    the first number to 1 at the last. A texture takes any length, where a block is limited in
    size, and reading it between two pixels interpolates the numbers. The numbers are kept as 16
    bit floats. The texture is only written when the array changes: a component declaring the
    array as a series with a revision (see ComponentTypeBuilder::series) tells so with it, and
    any other array is compared.

    The effect fills three more members itself, when the block has them and no Property has
    their name:

    - `resolution`: the size of the picture in pixels, a `vec2`, or a `vec4` holding one over
      the width and one over the height in `z` and `w`;
    - `scale`: how many pixels a unit of the component takes, a `float`;
    - `time`: the seconds the component has been showing for, a `float`. It starts again from
      zero every hour, which keeps it precise to a fraction of a millisecond as a `float`.

    A shader whose block has a `time` member is animated: the component repaints at every
    frame of its window while it is showing. Any other shader only runs when the component is
    painted, and costs nothing between two paints.

    The picture has the size of the component in pixels, its paint margin included (see
    Component::setPaintMargin). A shader declaring no texture makes its picture alone: it fills
    that area, and what the component painted is not shown. A shader reading two pictures can't
    be drawn, as an effect has one to give: the textures filled from arrays don't count (see
    getNumPictures).

    When the shader can't run, the component is drawn as it is: without a shader, without a
    GPU, or when the shader fails to compile for the device. A shader that could not be drawn
    is not tried again at every paint, nor does it keep its component repainting: it is tried
    again once the component is painted by another device, or once the effect is given another
    shader.

    The effect is bound to the component it was created for: set it on that component only.
    Create it, change its shader and destroy it on the message thread. The effect is applied
    while the window is painted, which never happens while the message thread runs, so the
    properties of the component are read as they are, with no lock.

    @see ComponentEffect, GpuImageShader, ShaderUniforms, StyleContext::findShader, Component::setComponentEffect

    @tags{UI}
*/
class YUP_API ShaderEffect : public ComponentEffect
{
public:
    //==============================================================================
    /** An input of the shader: a property of the component, written to the member of the
        uniform block that has its name. */
    struct Property
    {
        /** The name of the property, and of the member of the uniform block it fills. */
        Identifier name;

        /** The type of the property, which tells how its value fills the member:
            ComponentProperty::Type::number, integer, boolean, color, point or rectangle, or
            ComponentProperty::Type::array for an array of numbers. */
        ComponentProperty::Type type = ComponentProperty::Type::number;
    };

    //==============================================================================
    /** Creates an effect drawing a component through a shader.

        @param component   The component the effect is set on, whose properties fill the shader.
                           It can be deleted before the effect, which then reads nothing.
        @param shader      The shader to run, or nullptr to draw the component as it is.
        @param properties  The properties of the component the shader reads, each filling the
                           member of the uniform block of the shader that has its name. One
                           naming no member of the block is ignored.
    */
    ShaderEffect (Component& component, GpuImageShader::Ptr shader, std::vector<Property> properties = {});

    /** Destructor. */
    ~ShaderEffect() override;

    //==============================================================================
    /** Replaces the shader, and repaints the component.

        The properties stay the ones the effect was created with: the new shader is expected
        to read the same ones, as the shader of a file loaded again does.

        @param newShader  The shader to run, or nullptr to draw the component as it is.
    */
    void setShader (GpuImageShader::Ptr newShader);

    /** Returns the shader the effect runs, or nullptr. */
    GpuImageShader::Ptr getShader() const noexcept { return shader; }

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

        This is the conversion the effect makes at every paint, for what else fills the uniforms
        of a shader from typed values:

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
    static bool setUniform (ShaderUniforms& uniforms, const Property& property, const var& value);

    /** Returns how many pictures a shader reads, given the properties it is filled from.

        A texture of the shader that an array Property has the name of is no picture: it is
        filled with the numbers of that property, one per pixel (see the class description). The
        other textures are the pictures the shader reads: what the component painted, for an
        effect.

        @param shader      The shader.
        @param properties  The properties the shader is filled from.
    */
    static int getNumPictures (const GpuImageShader& shader, const std::vector<Property>& properties);

    //==============================================================================
    /** @internal */
    void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) override;

private:
    struct SeriesTextures;

    void updateFrameAttachment();
    void advance (double elapsedSeconds);
    GpuTexture::Ptr render (Graphics& g, const GpuTexture::Ptr& input);
    std::pair<Span<const float>, uint32> readSeries (const Component& shown, const Identifier& name);

    WeakReference<Component> component;
    GpuImageShader::Ptr shader;
    std::vector<Property> properties;
    GpuDevice::Ptr targetDevice;
    GpuTarget::Ptr target;
    std::unique_ptr<SeriesTextures> series;
    std::optional<FrameAttachment> frameAttachment;
    double time = 0.0;
    bool drawFailed = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShaderEffect)
};

} // namespace yup
