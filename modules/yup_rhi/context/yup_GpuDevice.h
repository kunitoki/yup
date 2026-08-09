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
/** Encapsulates a GPU context that abstracts low-level GPU device operations
    across various graphics APIs without requiring a window or framebuffer.

    GpuDevice is the entry point for GPU compute and RHI operations. It owns
    the native GPU device, command queue, and the backend-agnostic ore context.
    Unlike GraphicsContext, it does NOT depend on windows, swapchains, or Rive
    vector rendering — it is suitable for headless GPU compute (e.g. audio DSP
    on the GPU).

    GpuDevice is reference-counted, allowing multiple owners (e.g. an audio
    processor and an optional UI) to share a single GPU device.

    @see GraphicsContext, GpuPipeline, GpuComputePipeline
*/
class YUP_API GpuDevice : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<GpuDevice>;

    //==============================================================================
    /** Procedure load function used by GL and Vulkan to locate methods at runtime. */
    using LoaderFunction = void* (*) (const char*);

    //==============================================================================
    /** Configuration options for creating a GPU context. */
    struct Options
    {
        /** Default constructor, initializes the options with default values. */
        constexpr Options() noexcept = default;

        bool retinaDisplay = true;                  ///< Whether the context supports Retina or high-DPI displays.
        bool readableFramebuffer = false;           ///< Allows the framebuffer to be readable.
        bool synchronousShaderCompilations = false; ///< Controls whether shader compilations are done synchronously.
        bool enableReadPixels = false;              ///< Enables reading pixels directly from the framebuffer.
        bool disableRasterOrdering = false;         ///< Disables specific raster ordering features for performance.
        bool allowHeadlessRendering = false;        ///< Allows rendering without a visible window (headless mode).
        LoaderFunction loaderFunction = nullptr;    ///< Loader function (used by GL/Vulkan).
    };

    //==============================================================================
    /** Default constructor. */
    GpuDevice() noexcept = default;

    /** Destructor. */
    ~GpuDevice() override = default;

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    GpuDevice (const GpuDevice& other) noexcept = delete;
    GpuDevice (GpuDevice&& other) noexcept = default;
    GpuDevice& operator= (const GpuDevice& other) noexcept = delete;
    GpuDevice& operator= (GpuDevice&& other) noexcept = default;

    //==============================================================================
    /** Returns the GPU API used by this context.

        @return The GpuPlatform enum value identifying the active rendering backend.
    */
    virtual GpuPlatform getPlatform() const noexcept = 0;

    //==============================================================================
    /** Returns the backend-agnostic ore GPU context, or nullptr when ore is
        unavailable on this backend.

        This is the single backend bridge used by the RHI layer (GpuPipeline,
        GpuFrame, GpuRenderPass, GpuBuffer). User code should prefer the
        dependency-free isGpuAvailable() capability probe instead.
    */
    virtual rive::ore::Context* gpuContext() const noexcept { return nullptr; }

    /** Returns true if a GPU (ore) context is available for RHI operations.

        Equivalent to gpuContext() != nullptr but without referencing any ore
        type, so user code and examples can probe GPU capability ore-free.
    */
    bool isGpuAvailable() const noexcept { return gpuContext() != nullptr; }

    /** Returns true if compute shaders are available on this backend.

        Compute shaders are available on Metal, D3D11, D3D12, Vulkan, and
        WebGPU backends. Not available on OpenGL/GLES or Headless.
    */
    virtual bool isComputeAvailable() const noexcept { return false; }

    //==============================================================================
    /** Creates platform-specific GPU offscreen resources for the given dimensions.

        Supported GPU backends may create targets while another offscreen target
        is rendering. Backends reserve a render context only while its target
        has an active frame, allowing sequential targets to share idle contexts.
        A target must outlive its corresponding beginOffscreen()/endOffscreen()
        pair and the GpuDevice must outlive every target it creates.

        @param width The width of the offscreen target in pixels.
        @param height The height of the offscreen target in pixels.

        @return A unique pointer to an OffscreenTarget object, or nullptr on failure.
    */
    virtual std::unique_ptr<OffscreenTarget> createOffscreenTarget (int width, int height) = 0;

    /** Creates platform-specific GPU offscreen resources backed by a dedicated render context.

        Unlike createOffscreenTarget(), the returned RenderableTarget reserves a
        backend-owned RenderContext, which is required to drive a 2D Graphics frame
        (GpuCanvas::beginDraw). Prefer createOffscreenTarget() for render-pass-only
        surfaces to avoid allocating a dedicated context.

        @param width The width of the offscreen target in pixels.
        @param height The height of the offscreen target in pixels.

        @return A unique pointer to a RenderableTarget object, or nullptr on failure.
    */
    virtual std::unique_ptr<RenderableTarget> createRenderableTarget (int width, int height) = 0;

    /** Begins a GPU frame targeting the given offscreen surface.

        A target may have only one active frame. Nested frames are supported when
        they use distinct OffscreenTarget instances.

        @param target The OffscreenTarget to render into.
        @param frameDesc The frame descriptor that contains frame-specific data.
    */
    virtual void beginOffscreen (OffscreenTarget& target, const rive::gpu::RenderContext::FrameDescriptor& frameDesc) = 0;

    /** Flushes GPU commands into the offscreen target.
    
        Must be called after beginOffscreen() and before endOffscreen().

        @param target The OffscreenTarget to flush commands into.
    */
    virtual void endOffscreen (OffscreenTarget& target) = 0;

    /** Reads RGBA pixels from the completed offscreen frame into CPU memory.

        Must be called after endOffscreen(). Rows are top-to-bottom.
    
        @param target The OffscreenTarget to read pixels from.
        @param dst Pointer to the destination buffer where pixel data will be stored.
        @param dstSize The size of the destination buffer in bytes.

        @return True if the pixel read operation was successful, false otherwise.
    */
    virtual bool readOffscreenPixels (OffscreenTarget& target, void* dst, size_t dstSize) = 0;

    //==============================================================================
    /** Static factory method to create a GPU context using a specific GPU API.

        @param gpuApi The GPU API to use.
        @param options Configuration options for the GPU context.

        @return A reference-counted pointer to a GpuDevice, using the specified
                GPU API and configured according to the options.
    */
    static GpuDevice::Ptr create (GpuPlatform gpuApi, Options options);
};

} // namespace yup
