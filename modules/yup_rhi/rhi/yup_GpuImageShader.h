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
/** A fragment shader drawn over a whole GpuTarget: an effect on one or two
    pictures, or a picture the shader makes alone.

    A GpuImageShader holds a compiled ShaderBundle with a vertex and a fragment
    stage. Drawing it covers the target with one triangle, and the fragment stage
    computes every pixel from its uniform block and from its input textures.

    It is immutable and reference counted: create it once and draw it from as
    many places as needed. Creating one takes no device and checks the shader, so
    a tool without a GPU can report what is wrong with it. A pipeline is only
    compiled when the shader is first drawn with a device, and it belongs to that
    device, which keeps it for the next draws and releases it itself. The shader
    holds no device and never keeps one alive, so the windows of an application,
    each with a device of its own, can draw the same shader.

    The fragment stage is written in GLSL 450, and what it declares decides what
    a draw binds. Nothing has a fixed set or binding number: sets 0 to 3 can be
    used, each block, texture and sampler taking a binding of its own in its set.

    - Its uniform block, if any, holds the parameters. createUniforms() returns
      its bytes to fill by member name, and draw() binds them. There can be one
      block at most, no larger than ShaderUniforms::maxSizeInBytes.
    - Its textures are the inputs: none for a shader that makes its picture alone,
      maxInputTextures at most. The first input is the texture with the lowest set,
      then the lowest binding, and getInputTextureNames() tells the name of each, for
      what gives a shader more than pictures: a texture can hold any numbers the
      shader samples. They are `texture2D`, declared apart from their `sampler`
      as Vulkan does and combined where they are read: a combined `sampler2D`
      cannot be bound and is refused.
    - Its samplers filter linearly and clamp to the edge. One sampler can read
      both textures.
    - `uv`, a `vec2` input at location 0, goes from (0, 0) at the top left of the
      target to (1, 1) at its bottom right, on every graphics API. Reading an
      input at `uv` reads it where the pixel is written. Declaring it is optional,
      and it is the only input there can be.
    - Its `vec4` output at location 0 is the pixel, and the only output.

    A draw binds nothing else, so nothing else can be declared: a storage buffer, a
    storage image, a subpass input or a push constant block, in either stage.

    Colors are premultiplied by alpha, both in the textures read and in the color
    written.

    @code
    #version 450

    layout(set = 0, binding = 0) uniform Parameters
    {
        vec4 tint;          // premultiplied by its alpha
        float intensity;
    } params;

    layout(set = 0, binding = 1) uniform texture2D source;
    layout(set = 0, binding = 2) uniform sampler sourceSampler;

    layout(location = 0) in vec2 uv;
    layout(location = 0) out vec4 color;

    void main()
    {
        vec4 pixel = texture (sampler2D (source, sourceSampler), uv);
        color = mix (pixel, pixel * params.tint, params.intensity);
    }
    @endcode

    Typical usage:
    @code
    auto created = GpuImageShader::createFromData (data, size);
    if (created.failed())
        return Result::fail (created.getErrorMessage());

    auto shader = created.getValue();

    auto uniforms = shader->createUniforms();
    uniforms.set ("tint", { 0.5f, 0.25f, 0.0f, 0.5f });
    uniforms.set ("intensity", 0.8f);

    auto target = GpuTarget::create (device, input->getWidth(), input->getHeight());
    auto frame = GpuFrame::begin (device);

    const auto drawn = shader->draw (device, frame, *target, uniforms, input);
    frame.submit();

    if (drawn.wasOk())
        graphics.drawTexture (target->asTexture(), bounds);
    @endcode

    @see ShaderUniforms, GpuTarget, GpuFrame, ShaderBundle
*/
class YUP_API GpuImageShader : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuImageShader>;

    /** The most textures the fragment stage can declare. */
    static constexpr int maxInputTextures = 4;

    //==============================================================================
    /** Destructor. Touches no device: each one releases the pipelines it compiled for the shader. */
    ~GpuImageShader() override;

    //==============================================================================
    /** Creates an image shader from a compiled bundle.

        The bundle has to hold a vertex and a fragment stage, as
        GpuPipeline::compileFromBundle() requires. The vertex stage is expected to
        be shaders/yup_GpuImageShader.vert of this module, the one to pair a
        fragment file with when a `.ysl` is compiled ahead of time, or one drawing
        the same triangle and declaring no resource the fragment stage lacks.

        @param bundle  The compiled shader, shared with the image shader.

        @returns The shader, or what prevents the bundle from being one: a missing
                 stage, or a fragment stage declaring a combined `sampler2D`,
                 several uniform blocks, a block larger than
                 ShaderUniforms::maxSizeInBytes, more than maxInputTextures textures,
                 a texture that is not a `texture2D`, an input other than a `vec2` at
                 location 0, or anything but one `vec4` output at location 0. So does
                 a stage declaring what no draw binds: a storage buffer, a storage
                 image, a subpass input or a push constant block, and for the vertex
                 stage a uniform block or a texture the fragment stage does not
                 declare at the same set and binding.
    */
    static ResultValue<GpuImageShader::Ptr> createFromBundle (ShaderBundle::Ptr bundle);

    /** Creates an image shader from the bytes of a shader file.

        Bytes starting with the RIFF magic are a compiled `.ysl` bundle, taken as
        createFromBundle() takes one. Any other bytes are the GLSL 450 source text
        of a fragment stage, compiled as createFromGlsl() does: that needs the
        shader transpiler (YUP_ENABLE_SHADER_TRANSPILER), and fails without it
        with a message asking for a compiled bundle.

        @param data  Start of the bytes.
        @param size  Number of bytes.

        @returns The shader, or why the bytes could not be loaded or compiled.
    */
    static ResultValue<GpuImageShader::Ptr> createFromData (const void* data, size_t size);

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Creates an image shader from the GLSL 450 source of its fragment stage.

        The source is paired with the stock vertex stage and compiled for the
        languages yup_shader_bundler targets by default (GLSL, ESSL, HLSL, MSL,
        SPIR-V and WGSL): the shader then draws with every graphics API, and what
        compiles here also compiles ahead of time. Only available when the shader
        transpiler is compiled in (YUP_ENABLE_SHADER_TRANSPILER = 1).

        @param fragmentGlsl  GLSL 450 fragment shader source.

        @returns The shader, or the errors of the compiler.
    */
    static ResultValue<GpuImageShader::Ptr> createFromGlsl (const String& fragmentGlsl);
#endif

    //==============================================================================
    /** Returns the number of input textures the fragment stage declares: 0 for a
        shader that makes its picture alone, up to maxInputTextures for one that
        reads textures. */
    int getNumInputTextures() const noexcept;

    /** Returns the names the fragment stage declares its input textures with, in
        the order draw() takes them: the first one is the name of the first input. */
    const StringArray& getInputTextureNames() const noexcept { return textureNames; }

    /** Returns the uniforms of the shader, zeroed: the bytes of its uniform block
        with the layout of its members, to fill by name and pass to draw().

        They are empty when the fragment stage declares no uniform block. All the
        ones the shader returns share the description of its members, so creating
        them copies the bytes only.
    */
    ShaderUniforms createUniforms() const;

    /** Returns the uniforms createUniforms() copies, zeroed, without copying them:
        what to ask which members the uniform block of the shader has, with
        ShaderUniforms::hasMember(), getMembers() and the like.

        A member that ShaderUniforms::findMember() finds in them is found for all the
        uniforms the shader creates: its handle sets it in each of them.
    */
    const ShaderUniforms& getUniformLayout() const noexcept { return blockUniforms; }

    /** Returns true if the shader has a variant of both stages for a graphics API,
        the one a pipeline is compiled from when the shader is drawn with a device of
        that API (see GpuDevice::getPlatform).

        A shader compiled from GLSL source has one for every graphics API. A bundle
        can be built for some of them only: drawing it with another fails, which
        this tells before any device draws it, such as when the shader is loaded.
        OpenGL ES takes the GLSL variant when the bundle has no ESSL one, as a draw
        does.

        @param platform  The graphics API.
    */
    bool canDrawWith (GpuPlatform platform) const noexcept;

    //==============================================================================
    /** Draws the shader over a whole target, as one render pass of a frame.

        The pass clears the target to transparent black, binds the uniforms and the
        input textures the fragment stage declares, and draws one triangle covering
        the target without blending. The frame is left open: submit it once every
        pass it holds is recorded.

        The pipeline is compiled the first time the shader is drawn with a device
        into a target of a format, and kept by that device for the next draws,
        whichever other devices draw the shader in between. One that cannot be
        compiled is not tried again by every draw.

        The pipelines belong to the devices, not to the shader. A device releases
        the ones it compiled for the shader when it is destroyed, or, once the
        shader is gone, the next time an image shader is drawn with it.

        Any thread drawing with the device can call it, also while other threads
        draw the same shader.

        @param device       The device @p frame was begun on and @p target was
                            created with.
        @param frame        The frame the pass is recorded into.
        @param target       The target to draw into, of a color format and not
                            multisampled.
        @param uniforms     The values of the uniform block, from createUniforms().
        @param firstInput   The texture the first input reads. Only needed, and
                            only read, by a shader with an input. An input cannot
                            be the texture of @p target.
        @param secondInput  The texture the second input reads, for a shader with
                            two.

        @returns Result::ok() once the pass is recorded, or why it was not: the
                 device has no GPU or was lost, an input the shader declares has
                 no valid texture or is the texture of the target, the uniforms
                 are not the ones of the shader, the target cannot be drawn into,
                 or the pipeline could not be compiled for the device.
    */
    Result draw (const GpuDevice::Ptr& device,
                 GpuFrame& frame,
                 GpuTarget& target,
                 const ShaderUniforms& uniforms,
                 const GpuTexture::Ptr& firstInput = nullptr,
                 const GpuTexture::Ptr& secondInput = nullptr) const;

    /** Draws the shader over a whole target, reading any number of inputs.

        As the overload taking two inputs, for a shader declaring more of them.

        @param device    The device @p frame was begun on and @p target was created with.
        @param frame     The frame the pass is recorded into.
        @param target    The target to draw into, of a color format and not multisampled.
        @param uniforms  The values of the uniform block, from createUniforms().
        @param inputs    The textures the inputs read, in the order of
                         getInputTextureNames(): as many as getNumInputTextures() at
                         least. None can be the texture of @p target.

        @returns Result::ok() once the pass is recorded, or why it was not.
    */
    Result draw (const GpuDevice::Ptr& device,
                 GpuFrame& frame,
                 GpuTarget& target,
                 const ShaderUniforms& uniforms,
                 Span<const GpuTexture::Ptr> inputs) const;

private:
    struct Binding
    {
        int group = 0;
        int binding = 0;
    };

    GpuImageShader() = default;

    ShaderBundle::Ptr bundle;
    ShaderUniforms blockUniforms;
    Binding blockBinding;
    std::vector<Binding> textureBindings;
    StringArray textureNames;

    // What every device keeps the pipelines of the shader under, so the shader holds no device
    const std::shared_ptr<void> pipelineOwner = std::make_shared<char>();

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuImageShader)
};

} // namespace yup
