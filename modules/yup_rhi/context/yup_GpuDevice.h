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
        Options() noexcept = default;

        bool retinaDisplay = true;                  ///< Whether the context supports Retina or high-DPI displays.
        bool readableFramebuffer = false;           ///< Allows the framebuffer to be readable.
        bool synchronousShaderCompilations = false; ///< Controls whether shader compilations are done synchronously.
        bool disableRasterOrdering = false;         ///< Disables specific raster ordering features for performance.
        bool allowHeadlessRendering = false;        ///< Allows rendering without a visible window (headless mode).
        bool vsync = false;                         ///< Synchronizes presentation to the display refresh (vsync).
        LoaderFunction loaderFunction = nullptr;    ///< Loader function (used by GL/Vulkan).

        /** Optional callback that runs GPU work with the native rendering context made
            current on the calling thread.

            GL contexts are thread-affine and the windowing layer may bind the context
            to a dedicated render thread. Offscreen GPU work initiated from other threads
            (e.g. snapshots taken from the message thread) must go through this callback
            so the context is bound — and its access serialized with the render thread —
            for the duration of the work. Backends without a thread-affine context
            (D3D, Metal, WebGPU) ignore it. When null, GPU work runs as-is.

            @param fn The GPU work to run with the context current.
        */
        std::function<void (const std::function<void()>&)> contextActivator;

        /** Optional callback that runs GPU compute work with a dedicated compute
            context made current on the calling thread.

            Some GL drivers stop executing compute dispatches when they are
            interleaved with rendering on the same context — or on any context of
            the same share group. Providing a dedicated, unshared context isolates
            the compute command stream from the render pass; every compute resource
            (program, storage buffers) then lives exclusively on that context.
            When null, compute work falls back to contextActivator (or runs as-is
            when that is also null). Backends without a thread-affine context
            (D3D, Metal, WebGPU) ignore it.

            @param fn The GPU compute work to run with the compute context current.
        */
        std::function<void (const std::function<void()>&)> computeContextActivator;
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
    /** Destructor.

        Asserts that the concrete device released the resources this base class
        pools on its behalf.

        @see releasePooledResources
    */
    ~GpuDevice() override;

    //==============================================================================
    /** Not movable: GpuDevice is reference-counted, and moving one out from under
        live GpuDevice::Ptr holders would leave them pointing at a husk while the
        refcount kept the moved-from object alive. */
    GpuDevice (GpuDevice&&) = delete;
    GpuDevice& operator= (GpuDevice&&) = delete;

    //==============================================================================
    /** Returns the GPU API used by this context.

        @return The GpuPlatform enum value identifying the active rendering backend.
    */
    virtual GpuPlatform getPlatform() const noexcept = 0;

    //==============================================================================
    /** Returns true if a GPU (ore) context is available for RHI operations.

        Equivalent to getGpuContext() != nullptr but without referencing any ore
        type, so user code and examples can probe GPU capability ore-free.
    */
    bool isGpuAvailable() const noexcept { return getGpuContext() != nullptr; }

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

    //==============================================================================
    /** Returns the backend's native device object, or nullptr where the backend
        has no such handle.

        Metal returns the @c MTLDevice every resource this GpuDevice owns was
        created on. A windowing layer that allocates its own textures must use it
        rather than creating a second device, since resources are not shared
        across Metal devices.

        @returns An opaque native handle, or nullptr.
    */
    virtual void* getNativeDevice() const noexcept { return nullptr; }

    /** Returns the backend's native command queue, or nullptr where the backend
        has no such handle.

        Metal returns the @c MTLCommandQueue that both the Rive render context and
        the ore context submit to. A windowing layer that encodes its own present
        work **must** submit on this queue rather than creating its own: Metal
        does not order work between two queues, so a second queue would let a
        frame present before the offscreen work it samples has finished. Every
        other backend has a single command stream and returns nullptr.

        @returns An opaque native handle, or nullptr.
    */
    virtual void* getNativeCommandQueue() const noexcept { return nullptr; }

    //==============================================================================
    /** Runs GPU work with the backend's rendering context current on the calling
        thread.

        On OpenGL this routes @p fn through Options::contextActivator so GPU
        resources can be created or released from threads that do not own the GL
        context (e.g. offscreen targets and textures dropped on the message thread
        while a render thread owns the context). On every other backend @p fn runs
        directly.

        @param fn The GPU work to run with the rendering context current.
    */
    virtual void runOnGraphicsContext (const std::function<void()>& fn) const { fn(); }

    //==============================================================================
    /** Returns true if compute shaders are available on this backend.

        Implemented on Metal, Direct3D 11, WebGPU and OpenGL / OpenGL ES; the GL
        path additionally probes the runtime version, since compute needs desktop
        GL 4.3 or GLES 3.1 and WebGL2 has no compute at all. Always false on the
        headless backend.
    */
    virtual bool isComputeAvailable() const noexcept { return false; }

    //==============================================================================
    /** Returns true if textures of the given format can be created and sampled.

        Only the block-compressed formats are ever unavailable; every uncompressed
        format ore exposes is required on all backends.

        @param format  The format to probe.
    */
    bool isFormatSupported (GpuTextureFormat format) const noexcept;

    /** Returns true if textures of the given format can be used as a color or
        depth/stencil render attachment.

        Float render targets are extension-gated on OpenGL ES and WebGL2
        (@c GL_EXT_color_buffer_float), so a demo that renders into an
        rgba16float or rg16float target must probe this and fall back to an
        8-bit encoding rather than silently rendering black.

        @param format  The format to probe.
    */
    bool isFormatRenderable (GpuTextureFormat format) const noexcept;

    /** Returns true if GpuSamplerDesc::maxAnisotropy above 1 has any effect. */
    bool isAnisotropicFilteringAvailable() const noexcept;

    /** Returns the highest MSAA sample count supported for color render targets.

        Always a power of two, and 1 when no GPU context is available.
    */
    uint32_t getMaximumSampleCount() const noexcept;

    /** Runs GPU compute work with the backend's compute context current on the
        calling thread.

        On OpenGL this routes @p fn through Options::computeContextActivator so
        the work is encoded on a dedicated compute context, isolated from the
        rendering command stream (falling back to Options::contextActivator when
        no compute activator was provided). On every other backend @p fn runs
        directly.

        Used internally by GpuComputePass and GpuComputePipeline; user code
        normally never needs to call this.

        @param fn The GPU compute work to run with the compute context current.
    */
    virtual void runOnComputeContext (const std::function<void()>& fn) const { fn(); }

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
    virtual void beginOffscreen (OffscreenTarget& target, const GpuFrameDescriptor& frameDesc) = 0;

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

    /** Reads storage buffer contents back to CPU memory.

        The buffer must have been created with GpuBufferType::storage.

        Whether this blocks is a property of the backend, because not all of them
        can map a buffer synchronously:

        - Metal, D3D11 and OpenGL read back in lockstep — the call waits for the
          GPU and fills @p dst every time.
        - WebGPU maps buffers through a promise that only resolves on a later turn
          of the JavaScript event loop, so there the readback is pipelined over
          several staging buffers instead. The call never blocks; it writes @p dst
          once a snapshot has finished mapping, which trails the GPU by a frame or
          two.

        Callers must therefore keep @p dst around between calls and treat a false
        return as "no new data yet" rather than as an error — the previous contents
        remain valid and usable. Nothing here retains the pointer; @p dst is written
        within the call or not at all. A per-frame reader that redraws its last
        snapshot works on every backend; one that demands fresh data on every single
        call does not.

        @param buffer   A storage buffer to read from.
        @param dst      Destination buffer in CPU memory, reused across calls.
        @param dstSize  Size in bytes (must be at least the buffer's byte size).

        @returns true if @p dst was filled with buffer contents.
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

    /** Releases the GPU resources this base class pools on the backend's behalf.

        Every concrete device must call this from its own destructor. Base-class
        members are destroyed after the derived class', so pooled resources would
        otherwise be released after the ore context that created them - which is
        fatal on any backend whose ore resource destructors reach back into
        context-owned state, as ore's Vulkan buffer does when it frees through the
        context's VMA allocator.

        ~GpuDevice() asserts this has happened, so a backend that forgets the call
        fails loudly in debug on every platform instead of crashing at shutdown on
        one.
    */
    void releasePooledResources() noexcept;

private:
    friend class GpuFrame;

    //==============================================================================
    /** Pool of small uniform buffers recycled across frames.

        Every draw in a render pass needs its own uniform buffer, and creating one
        per draw is a native GPU allocation on all backends. Buffers are handed out
        with a power-of-two capacity and returned once the frame that bound them is
        finished, so a steady-state workload stops allocating after its first few
        frames.

        Rewriting a returned buffer is safe even when the GPU may still be reading
        it: the ore Buffer implementations orphan onto a fresh backing when a buffer
        is updated after having been bound (D3D11 instead lets the driver rename
        it), and that is what makes reuse across frames sound.

        Pooled buffers must not outlive the ore context that created them, because
        an ore Buffer destructor may reach back into context-owned state - ore's
        Vulkan buffer frees through the context's VMA allocator. That is why the
        pool is emptied by releasePooledResources() rather than by its own
        destructor, which would run too late.
    */
    class UniformBufferPool
    {
    public:
        /** Returns a buffer with room for at least byteSize bytes.

            Creates one only when no returned buffer of that capacity is available.

            @param oreCtx    Context used to create a buffer on a pool miss.
            @param byteSize  Minimum required capacity in bytes.

            @returns A uniform buffer, or nullptr if creation failed.
        */
        rive::rcp<rive::ore::Buffer> acquire (rive::ore::Context& oreCtx, size_t byteSize);

        /** Takes a buffer back so a later acquire() of the same capacity reuses it. */
        void release (rive::rcp<rive::ore::Buffer> buffer);

        /** Drops every pooled buffer. */
        void clear() noexcept;

        /** Returns true when the pool holds no buffers. */
        bool isEmpty() const noexcept;

    private:
        /** Smallest capacity handed out - uniform blocks are 16-byte aligned. */
        static constexpr size_t minimumCapacity = 16;

        /** Index of the bucket holding buffers big enough for byteSize. */
        static size_t bucketFor (size_t byteSize) noexcept;

        std::vector<std::vector<rive::rcp<rive::ore::Buffer>>> buckets;
    };

    UniformBufferPool uniformBufferPool;

    //==============================================================================
    /** Transient resources of one submitted frame, held until the GPU is known to
        have finished with them.

        A frame's texture views, samplers and pooled uniform buffers are still
        referenced by the commands it submitted, so they cannot be released the
        moment submit() returns. Blocking on the GPU to find out when they are
        free costs a full pipeline stall every frame; instead each frame hands its
        resources here tagged with a generation, and a generation is released once
        enough later frames have begun that the GPU cannot still be reading it.

        The count is a safety margin, not a correctness requirement on the
        backends YUP ships: Metal retains resources on the command buffer, OpenGL
        defers deletion of names still referenced by queued commands, and D3D11
        and WebGPU hold their own references. It *is* what a manager-backed
        backend (ore's Vulkan and D3D12 paths) would rely on, which is why the
        matching safeFrameNumber is also reported to the ore context.
    */
    static constexpr uint64_t framesInFlight = 3;

    struct RetiredFrame
    {
        uint64_t generation = 0;
        std::vector<rive::rcp<rive::ore::Buffer>> buffers;
        std::vector<rive::rcp<rive::ore::TextureView>> views;
        std::vector<rive::rcp<rive::ore::Sampler>> samplers;
    };

    /** Starts a new frame generation and releases any that are now safe.

        @returns The generation number the new frame should be tagged with.
    */
    uint64_t beginFrameGeneration();

    /** Returns the generation whose work the GPU is guaranteed to have finished. */
    uint64_t getSafeFrameGeneration() const noexcept;

    /** Takes ownership of a submitted frame's transient resources. */
    void retireFrameResources (uint64_t generation,
                               std::vector<rive::rcp<rive::ore::Buffer>> buffers,
                               std::vector<rive::rcp<rive::ore::TextureView>> views,
                               std::vector<rive::rcp<rive::ore::Sampler>> samplers);

    /** Releases every retired generation up to and including @p generation. */
    void releaseRetiredFrames (uint64_t generation);

    uint64_t frameGeneration = 0;
    std::vector<RetiredFrame> retiredFrames;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuDevice)
};

} // namespace yup
