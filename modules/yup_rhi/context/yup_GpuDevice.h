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

class GpuBuffer;

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
        bool disableRasterOrdering = false;         ///< Disables specific raster ordering features for performance.
        bool allowHeadlessRendering = false;        ///< Allows rendering without a visible window (headless mode).
        LoaderFunction loaderFunction = nullptr;    ///< Loader function (used by GL/Vulkan).
    };

    //==============================================================================
    /** Creates a GPU context using a specific GPU API.

        @param gpuApi The GPU API to use.
        @param options Configuration options for the GPU context.

        @return A reference-counted pointer to a GpuDevice, using the specified
                GPU API and configured according to the options.
    */
    static GpuDevice::Ptr create (GpuPlatform gpuApi, Options options);

    //==============================================================================
    /** Destructor. */
    ~GpuDevice() override = default;

    //==============================================================================
    /** Move constructors and assignment operators. */
    GpuDevice (GpuDevice&& other) noexcept = default;
    GpuDevice& operator= (GpuDevice&& other) noexcept = default;

    //==============================================================================
    /** Returns the GPU API used by this context.

        @return The GpuPlatform enum value identifying the active rendering backend.
    */
    virtual GpuPlatform getPlatform() const noexcept = 0;

    //==============================================================================
    /** Returns the backend-specific GPU render context, or nullptr if unavailable.
    
        This is the native GPU context used by the Rive renderer. It may be
        nullptr on backends that do not support rendering (e.g., headless compute
        or OpenGL without a window).

        @return A pointer to the backend-specific RenderContext, or nullptr if unavailable.
    */
    virtual rive::gpu::RenderContext* getRenderContext() const { return nullptr; }

    /** Returns the backend-agnostic ore GPU context, or nullptr when ore is
        unavailable on this backend.

        This is the single backend bridge used by the RHI layer (GpuPipeline,
        GpuFrame, GpuRenderPass, GpuBuffer). User code should prefer the
        dependency-free isGpuAvailable() capability probe instead.

        @return A pointer to the ore::Context, or nullptr if unavailable.
    */
    virtual rive::ore::Context* getGpuContext() const noexcept { return nullptr; }

    /** Returns true if a GPU (ore) context is available for RHI operations.

        Equivalent to getGpuContext() != nullptr but without referencing any ore
        type, so user code and examples can probe GPU capability ore-free.
    */
    bool isGpuAvailable() const noexcept { return getGpuContext() != nullptr; }

    /** Returns true if compute shaders are available on this backend.

        Compute shaders are available on Metal, D3D11, D3D12, Vulkan, and
        WebGPU backends. Not available on OpenGL/GLES or Headless.
    */
    virtual bool isComputeAvailable() const noexcept { return false; }

    //==============================================================================
    /** Creates platform-specific GPU offscreen resources for the given dimensions.

        The returned target is backed by the device's main render context and does
        not reserve a dedicated one, so it cannot drive a 2D Graphics frame. Use it
        for render-pass-only surfaces. A target must outlive its corresponding
        beginOffscreen()/endOffscreen() pair and the GpuDevice must outlive every
        target it creates.

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

        The reservation is exclusive and lasts for the target's whole lifetime; the
        context returns to the pool when the target is destroyed. Two live targets
        therefore never share a context, which is what allows their frames to nest.

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

    /** Clears an offscreen target's color attachment to the given color.

        Encodes the clear with the backend's native API, outside any ore frame or
        render pass. A clear binds no pipeline, buffers or samplers, so nothing
        transient has to outlive the encoded work and no GPU sync is required -
        unlike a GpuRenderPass clear, whose descriptor references its texture view
        by raw pointer and so obliges the frame to wait before releasing it.

        Needs no active frame, and may be called immediately after the target is
        created to give it defined contents.

        @param target The OffscreenTarget whose color attachment to clear.
        @param color  The color to fill the attachment with.

        @return True if the clear was encoded, false if unsupported on this backend.
    */
    virtual bool clearOffscreen (OffscreenTarget&, GpuColor) { return false; }

    //==============================================================================
    /** Creates a GPU buffer of the given type with initial data.

        The default implementation routes vertex, index, and uniform buffers
        through the ore context. Backends that support compute override this to
        also handle storage buffers natively.

        @param type       The intended usage of the buffer.
        @param data       Pointer to the source data to upload (must be non-null).
        @param byteSize   Number of bytes to upload (must be greater than zero).

        @returns A valid GpuBuffer, or nullptr on failure.
    */
    virtual ReferenceCountedObjectPtr<GpuBuffer> createBuffer (GpuBufferType type,
                                                               const void* data,
                                                               size_t byteSize);

    /** Reads storage buffer contents back to CPU memory (blocking).

        The buffer must have been created with GpuBufferType::storage.

        @param buffer   A storage buffer to read from.
        @param dst      Destination buffer in CPU memory.
        @param dstSize  Size in bytes (must match the buffer's byte size).

        @returns true on success.
    */
    virtual bool readBuffer (ReferenceCountedObjectPtr<GpuBuffer> buffer, void* dst, size_t dstSize);

    /** Overwrites storage buffer contents in place, without reallocating the buffer.

        The buffer must have been created with GpuBufferType::storage. Unlike
        createBuffer(), this does not allocate a new native GPU resource — it
        writes into the existing one, which is the only allocation-free way to
        feed new data to a storage buffer every frame or audio callback.

        @param buffer   A storage buffer to write into.
        @param data     Pointer to the source data to upload (must be non-null).
        @param byteSize Number of bytes to upload (must be greater than zero and
                        not exceed the buffer's size).

        @returns true on success.
    */
    virtual bool updateBuffer (ReferenceCountedObjectPtr<GpuBuffer> buffer, const void* data, size_t byteSize);

protected:
    /** Default constructor. */
    GpuDevice() noexcept = default;

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuDevice)
};

} // namespace yup
