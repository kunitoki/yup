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
/** Identifies the shading language of a GpuShaderSource code block. */
enum class GpuShaderLanguage : uint8_t
{
    wgsl = 0, ///< WGSL (WebGPU Shading Language).
    glsl = 1, ///< GLSL (GLES 3.0+, GL path only).
    msl = 2,  ///< MSL (Metal Shading Language, Metal backend only).
    hlsl = 3, ///< HLSL (DirectX Shading Language, DirectX backend only).
};

//==============================================================================
/** Compiled shader source for one pipeline stage (vertex or fragment).

    The binding-map sidecar (@c bindingMap / @c bindingMapSize) is mandatory.
    It is produced offline by the Rive scripting-workspace RSTB toolchain and
    must accompany the shader code. GpuProgram::compile() will assert and fail
    if the sidecar is missing.

    @see GpuProgram
*/
struct GpuShaderSource
{
    GpuShaderLanguage language = GpuShaderLanguage::wgsl;

    /** Shader source code bytes. */
    const void* code = nullptr;

    /** Number of bytes in @c code. */
    uint32_t codeSize = 0;

    /** Mandatory pre-compiled RSTB binding-map sidecar blob. */
    const uint8_t* bindingMap = nullptr;

    /** Number of bytes in @c bindingMap. */
    uint32_t bindingMapSize = 0;

    /** Override the stage entry-point name. nullptr → "vs_main" / "fs_main". */
    const char* entryPoint = nullptr;
};

class GraphicsContext;
class Texture;
class GpuCanvas;

//==============================================================================
/** A compiled GPU render pipeline for custom shader dispatch.

    GpuProgram wraps an ore (Rive's backend-agnostic GPU layer) render pipeline
    consisting of a vertex shader and a fragment shader. Intended for fullscreen
    post-process effects and custom image filters that read from one or more
    Texture inputs and write into a GpuCanvas output.

    Typical usage:
    @code
    // 1. Compile once (entrypoints, binding-map sidecars provided by RSTB toolchain).
    auto prog = yup::GpuProgram::compile (ctx, vertSource, fragSource);

    // 2. Each frame: create an empty canvas, bind inputs, dispatch, composite.
    auto canvas = yup::GpuCanvas::create (ctx, w, h);
    canvas->commit();                                 // commit Rive frame first
    prog->setTexture (0, 0, inputTexture);
    prog->setUniformBuffer (0, 1, &params, sizeof (params));
    prog->beginFrame();
    prog->dispatch (*canvas);                         // ore renders into canvas texture
    prog->endFrame();
    prog->waitForGPU();                               // omit if compositing before readPixels
    g.drawImage (canvas->asImage(), bounds);          // composite
    @endcode

    Requires the GraphicsContext to have been created with
    Options::enableOreContext = true.

    @see GpuCanvas, GpuShaderSource, GraphicsContext::Options
*/
class YUP_API GpuProgram : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuProgram>;

    //==============================================================================
    ~GpuProgram();

    //==============================================================================
    /** Binds a texture to the given (group, binding) slot.

        The texture may come from GpuCanvas::asTexture() (after commit) or from
        Image::getTexture(). If the same slot is set more than once the later call wins.
    */
    void setTexture (int group, int binding, Texture::Ptr texture);

    /** Uploads raw uniform data to the given (group, binding) slot.

        The data is copied immediately; the caller need not keep it alive.
        If the same slot is set more than once the later call wins.
    */
    void setUniformBuffer (int group, int binding, const void* data, size_t byteSize);

    //==============================================================================
    /** Begins an ore GPU frame.

        Must be called once before one or more dispatch() calls. Clears any GPU
        resources retained from the previous frame. Pair with endFrame().

        @return true on success; false if ore is unavailable.
    */
    bool beginFrame();

    /** Submits all render passes recorded since beginFrame().

        Must be called after all dispatch() calls for this frame. Does not block
        the CPU — call waitForGPU() afterwards if you need results immediately
        (e.g., before readPixels()).

        @return true on success; false if ore is unavailable.
    */
    bool endFrame();

    /** Blocks the calling thread until all submitted GPU work has completed.

        Call after endFrame() when you need the GPU results to be available on the
        CPU (e.g., before canvas.readPixels()). Also releases GPU resources that
        were kept alive for the duration of the frame.
    */
    void waitForGPU();

    //==============================================================================
    /** Encodes a fullscreen render pass into the committed GpuCanvas.

        The canvas must already be committed (canvas.commit() was called). Must be
        called between beginFrame() and endFrame(). Ore renders a single fullscreen
        triangle into the canvas's backing texture, reading from the textures and
        uniform buffers previously bound via setTexture() / setUniformBuffer().

        Multiple dispatch() calls may be issued within a single beginFrame()/endFrame()
        scope to batch several passes into one GPU submission.

        Call canvas.asTexture() / canvas.asImage() / Graphics::drawTexture() after
        endFrame() (and waitForGPU() if needed) to composite the result.

        @return true on success; false if the program is invalid, ore is unavailable,
                or the canvas has not been committed.
    */
    bool dispatch (GpuCanvas& output);

    //==============================================================================
    /** Returns the ore Context used to compile this program, or nullptr. */
    rive::ore::Context* oreContext() const noexcept;

    /** Returns the compiled ore Pipeline for advanced vertex / 3D draw calls. */
    rive::ore::Pipeline* orePipeline() const noexcept;

    //==============================================================================
    /** Compiles a GpuProgram from vertex and fragment shader sources.

        Both shaders must supply pre-compiled RSTB binding-map blobs via
        GpuShaderSource::bindingMap. Returns nullptr on failure; if @c outError is
        non-null it receives a human-readable description of the failure.

        Requires ctx.gpuContext() != nullptr (enableOreContext = true).
    */
    static GpuProgram::Ptr compile (GraphicsContext& ctx,
                                    const GpuShaderSource& vertexShader,
                                    const GpuShaderSource& fragmentShader,
                                    std::string* outError = nullptr);

private:
    GpuProgram() = default;

    struct Impl;
    std::unique_ptr<Impl> impl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuProgram)
};

} // namespace yup
