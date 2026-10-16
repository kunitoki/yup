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
    /** Windowing hooks of the Vulkan backend, ignored by every other backend.

        Vulkan has no window system of its own: the windowing layer resolves the
        loader and creates the surfaces, and the device calls back into it. Handles
        are passed type-erased, so this header stays free of Vulkan types: a
        @c VkInstance or @c VkPhysicalDevice travels as @c void*, a
        @c VkSurfaceKHR as a 64-bit integer.
    */
    struct VulkanOptions
    {
        /** The loader entry point (@c PFN_vkGetInstanceProcAddr). When null, the
            device loads the system Vulkan loader itself, so a device without a
            window needs no windowing layer. */
        void* getInstanceProcAddr = nullptr;

        /** Instance extensions the window surfaces need, such as @c VK_KHR_surface
            and the platform surface extension. Empty for a headless device. */
        StringArray instanceExtensions;

        /** Tells whether a queue family of a physical device can present to the
            window system. Null for a headless device, which then has no swapchain. */
        std::function<bool (void* instance, void* physicalDevice, uint32 queueFamilyIndex)> presentationSupport;

        /** Creates the window surface on the given instance, returning the
            @c VkSurfaceKHR handle or 0 on failure. Called again whenever the window
            is attached after having been detached. */
        std::function<uint64 (void* instance)> createSurface;

        /** Destroys a surface returned by createSurface. */
        std::function<void (void* instance, uint64 surface)> destroySurface;

        /** True when the window composites with what is behind it, so the swapchain
            keeps the alpha of what is drawn instead of presenting opaque. */
        bool transparent = false;
    };

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

        /** Windowing hooks of the Vulkan backend. */
        VulkanOptions vulkan;
    };

    //==============================================================================
    /** Creates a GPU context using a specific GPU API.

        @param gpuApi The GPU API to use.
        @param options Configuration options for the GPU context.

        @return A reference-counted pointer to a GpuDevice, using the specified
                GPU API and configured according to the options.
    */
    static GpuDevice::Ptr create (GpuPlatform gpuApi, Options options);

    /** Returns true if a device of the given GPU API can be created with these options.

        A cheap probe meant for picking a graphics API before creating a window. Vulkan
        checks for a loader, a driver and a physical device that meets the renderer's
        requirements (Vulkan 1.1, Vulkan 1.3 on PowerVR, Android 10), caching the answer
        for the process. Every other API reports whether it was compiled in.

        @param gpuApi   The GPU API to probe.
        @param options  The options the device would be created with.
    */
    static bool isPlatformSupported (GpuPlatform gpuApi, const Options& options);

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

        Vulkan returns the backend's internal @c VulkanDevice, which bundles the
        instance, device, queue, entry points and the command ring every
        submission must go through; a bare @c VkDevice is not usable on its own.

        @returns An opaque native handle, or nullptr.
    */
    virtual void* getNativeDevice() const noexcept { return nullptr; }

    /** Returns the backend's native command queue, or nullptr where the backend
        has no such handle.

        Metal returns the @c MTLCommandQueue that both the Rive render context and
        the ore context submit to. A windowing layer that encodes its own present
        work **must** submit on this queue rather than creating its own: Metal
        does not order work between two queues, so a second queue would let a
        frame present before the offscreen work it samples has finished. Vulkan
        returns its single @c VkQueue. Every other backend has a single command
        stream and returns nullptr.

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

        Implemented on Metal, Direct3D 11, WebGPU, Vulkan and OpenGL / OpenGL ES; the GL
        path additionally probes the runtime version, since compute needs desktop
        GL 4.3 or GLES 3.1 and WebGL2 has no compute at all. Always false on the
        headless backend.
    */
    virtual bool isComputeAvailable() const noexcept { return false; }

    /** Returns true if fragment shaders can read storage buffers.

        Desktop OpenGL, OpenGL ES and WebGL2 are raster-only and always return
        false, as does the headless backend.
    */
    virtual bool isFragmentStorageBufferAvailable() const noexcept { return false; }

    /** Returns true if pipelines can bind uniform buffers at dynamic offsets
        (GpuPipelineOptions::dynamicUniformBuffers).

        True on Metal, Vulkan, WebGPU and OpenGL / OpenGL ES / WebGL2. Direct3D 11
        needs the 11.1 runtime with constant buffer offsetting and no-overwrite maps
        of dynamic constant buffers. Always false on the headless backend. When false,
        pipelines ignore GpuPipelineOptions::dynamicUniformBuffers.
    */
    virtual bool isDynamicUniformOffsetAvailable() const noexcept { return false; }

    /** Returns true if compute shaders can write storage textures that later
        passes sample.

        Desktop OpenGL, OpenGL ES and WebGL2 are raster-only and always return
        false, as does the headless backend.
    */
    virtual bool isStorageTextureAvailable() const noexcept { return false; }

    /** Returns true if the device is a WebGPU device created in compatibility mode.

        The web page falls back to compatibility mode when the browser cannot provide a
        full WebGPU device, as on older GPUs or browsers implementing WebGPU over OpenGL
        ES. Some GPU features and limits are then reduced. Always false on other backends.
    */
    virtual bool isCompatibilityMode() const noexcept { return false; }

    /** Returns true once the GPU device was lost, after a driver reset or a GPU hang.

        A lost device stays lost: its frames, offscreen renders and compute passes do
        nothing and its windows stop presenting, rather than hanging on the GPU. A new
        device has to be created to render again. Only reported by Vulkan; every other
        backend returns false.
    */
    virtual bool isDeviceLost() const noexcept { return false; }

    /** Returns true if this device measures the GPU time of the frames that ask for it
        with GpuFrame::setTimingEnabled().

        Only Metal measures for now; every other backend returns false, and its frames
        report their timings unavailable.

        @see GpuFrameStatus::getTimings
    */
    virtual bool isFrameTimingAvailable() const noexcept { return false; }

    /** Returns true if the frames that measure their GPU time also measure the time of
        their scopes (see GpuFrame::beginScope()).

        Only Metal devices that sample GPU timestamps at stage boundaries measure scopes;
        on every other device the timings of a frame list no scopes.

        @see GpuFrameTimings::scopes
    */
    virtual bool isScopeTimingAvailable() const noexcept { return false; }

    /** Returns the first GPU execution failure of a frame nobody owns, and clears them.

        Frames are submitted without waiting, so a frame the GPU fails to execute
        (out of memory, a page fault, a timeout) is only known once it completed:
        rendering that "succeeded" on the CPU may still have produced nothing. A
        frame that was waited on with GpuFrame::waitForGPU(), or whose status was
        taken with GpuFrame::getStatus(), reports its failure there only. Every other
        failed frame ends up here.

        Only Metal reports GPU execution failures; every other backend only fails
        frames it could not submit.

        @returns Result::ok() if no unowned frame failed, or the first failure.
    */
    Result takeGpuFailure();

    /** @internal Test hook, not for production use: makes the next frame begun on this
        device report @p error once it completes, as if the GPU had failed to execute it.
    */
    void failNextFrameForTesting (const String& error);

    /** @internal Test hook, not for production use: caps the GPU timestamp samples each frame
        measuring its scopes can take, so a test runs out of them with a few encoders. A render
        pass takes 4 samples and a compute pass 2. 0 restores the default. Only devices where
        isScopeTimingAvailable() is true take samples.
    */
    virtual void setScopeSampleCapacityForTesting (int samples) { ignoreUnused (samples); }

    /** @internal Test accessor, not for production use: the number of uniform buffers this device keeps for reuse, pooled or
        held by submitted frames until the GPU is done with them, and their total capacity in bytes.
    */
    std::pair<int, size_t> getUniformBufferPoolSizeForTesting() const noexcept;

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
        8-bit encoding rather than silently rendering black. With only
        @c GL_EXT_color_buffer_half_float the 16-bit float formats render, but
        r11g11b10float still needs @c GL_EXT_color_buffer_float.

        @param format  The format to probe.
    */
    bool isFormatRenderable (GpuTextureFormat format) const noexcept;

    /** Returns true if GpuSamplerDesc::maxAnisotropy above 1 has any effect. */
    bool isAnisotropicFilteringAvailable() const noexcept;

    /** Returns the highest MSAA sample count supported for color render targets.

        Always a power of two, and 1 when no GPU context is available.
    */
    uint32_t getMaximumSampleCount() const noexcept;

    /** Returns the highest MSAA sample count supported for render targets of the
        given format.

        On OpenGL ES and WebGL2 a renderable float format can allow fewer samples
        than rgba8unorm, so probe the format a target actually uses rather than
        relying on getMaximumSampleCount().

        @param format  The format to probe.

        @returns 0 when the format is not renderable (see isFormatRenderable()),
                 otherwise a power of two of at least 1.
    */
    virtual uint32_t getMaximumSampleCount (GpuTextureFormat format) const noexcept;

    /** Returns how many color attachments a render pass can bind at once.

        Between 1 and 4 on every backend, capped at 4 so the same pass layout
        works everywhere, and 0 when no GPU context is available.
    */
    uint32_t getMaximumColorAttachments() const noexcept;

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
        createBuffer(), this keeps the buffer and, when the GPU is done with it,
        writes into its existing native resource, which makes it the way to feed
        new data to a storage buffer every frame or audio callback.

        On Metal the update never waits for the GPU: while submitted or still
        recording work may read the buffer, it moves to another backing, so that
        work keeps the contents it was encoded with and later work sees the update.
        That move allocates a backing on a pool miss: a few spares per buffer are
        recycled once idle, so a steady per-frame update stops allocating after its
        first frames. GPU writes still in flight to the old backing are not carried
        over: read a buffer a compute pass writes back before updating it. One
        thread must own both the updates of a buffer and the passes that bind it.

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

    //==============================================================================
    /** Starts a new frame generation and releases any that are now safe.

        Every frame that submits GPU work takes one, so the generations of a device
        increase monotonically across all of its frames.

        @returns The generation number the new frame should be tagged with.
    */
    uint64_t beginFrameGeneration();

    /** Returns the generation whose work the GPU is guaranteed to have finished.

        The default assumes at most framesInFlight generations in flight. Backends
        that track completion themselves override it.
    */
    virtual uint64_t getSafeFrameGeneration() const noexcept;

    /** Returns the latest generation handed out by beginFrameGeneration(). */
    uint64_t getCurrentFrameGeneration() const noexcept { return frameGeneration; }

    //==============================================================================
    /** Provides the command buffer a GpuFrame records into, on backends where the
        host owns command buffers (Vulkan, Metal).

        Called by GpuFrame::begin() right after it took @p generation. The returned
        handle is passed back to submitFrameCommands() and waitFrameCommands(), and
        finally to releaseFrameCommands().

        @returns The native command buffer, or nullptr to let the ore context
                 manage its own (the default).
    */
    virtual void* beginFrameCommands (uint64_t generation);

    /** Submits the command buffer a frame recorded into. Does nothing by default.

        @returns Result::ok(), or why the frame could not be submitted.
    */
    virtual Result submitFrameCommands (void* commandBuffer);

    //==============================================================================
    /** The order the frames of the device were committed in, and the GPU timestamp the work of
        the latest one ended at, shared with the completion handlers, which may outlive the device.

        A tile-based GPU begins a pass while the passes before it still shade, those of earlier
        frames included, so a pass is credited only the time past the end of the work before it
        (see GpuFrameTimings::scopes). Within a frame the passes it sampled tell where that work
        ended; across frames this timeline does, fed by every frame once one has measured. A
        frame completing out of commit order, or GPU timestamps going back, makes it ignore the
        timeline rather than credit a pass 0 ms. Thread safe.
    */
    class YUP_API ScopeTimeline
    {
    public:
        /** One sampled pass, in GPU timestamp ticks. */
        struct Pass
        {
            int scopeIndex = -1; ///< The scope it is attributed to, or -1 for none.
            bool sampled = false; ///< False when the pass could not be sampled.
            uint64 start = 0;     ///< Its earliest sample, or 0 when it wrote none (no work).
            uint64 end = 0;       ///< Its latest sample.
        };

        /** Returns the sequence number of the frame committed next. */
        uint64 commit() noexcept;

        /** Starts following the frames, once the first one measures its scopes. */
        void activate() noexcept;

        /** True once a frame measured its scopes. */
        bool isActive() const noexcept;

        /** Records that the GPU work of the frame of @p sequence ended at @p end. Frames
            committed before the latest one recorded are ignored. */
        void advance (uint64 sequence, uint64 end);

        /** Returns the time of each scope the @p passes of the frame of @p sequence were
            attributed to, by scope index, each pass credited the part of it past the end of
            the work before it, and records where the frame's work ended.

            @param sequence            The frame's commit() number.
            @param passes              The frame's passes, in the order they were encoded.
            @param now                 A GPU timestamp taken after the frame completed.
            @param nanosecondsPerTick  The length of a GPU timestamp tick.
        */
        std::vector<GpuScopeTiming> attribute (uint64 sequence, const std::vector<Pass>& passes, uint64 now, double nanosecondsPerTick);

        /** Returns the GPU timestamp a frame's work ended at, for advance(), from the host time
            it ended at and a CPU and a GPU timestamp taken together after it completed.

            An end later than @p cpuNow by more than 1 microsecond means the two clocks disagree
            (after a system sleep, for one): it returns nothing then, so the timeline stays where
            it is rather than moving to now, past the next frame's work. A smaller excess is
            rounding, and counts as now.

            @param cpuNow              The CPU timestamp, in nanoseconds.
            @param gpuNow              The GPU timestamp taken with it, in ticks.
            @param hostEnd             When the frame's work ended, in nanoseconds of the clock of @p cpuNow.
            @param nanosecondsPerTick  The length of a GPU timestamp tick.

            @returns The end in GPU ticks, or nothing when it cannot be placed: the clocks
                     disagree, the tick length is unknown, or the end lies before the GPU
                     timestamps began.
        */
        static std::optional<uint64> getEndTicks (uint64 cpuNow, uint64 gpuNow, double hostEnd, double nanosecondsPerTick) noexcept;

    private:
        CriticalSection lock;
        uint64 nextSequence = 0;
        uint64 latestSequence = 0;
        uint64 latestEnd = 0;
        std::atomic<bool> active { false };
    };

    /** Returns the scope timeline of this device, for completion handlers to capture. */
    std::shared_ptr<ScopeTimeline> getScopeTimeline() const { return scopeTimeline; }

    /** Samples the GPU time of the native encoders of one frame, for its scopes.

        Created by a measuring frame when its first encoder opens, on devices where
        isScopeTimingAvailable() is true, and handed to the device when the frame is
        submitted, so the device resolves the samples once the frame completed. Every
        encoder of the frame is sampled, so each is credited only the time past the end
        of the work before it.
    */
    class ScopeSampler
    {
    public:
        virtual ~ScopeSampler() = default;

        /** Samples the render pass the frame opens next, attributing it to the scope of
            index @p scopeIndex, or to none when it is -1. Called right before the pass opens. */
        virtual void beginRenderPass (int scopeIndex) = 0;

        /** Called right after the render pass opened, or failed to: later passes are not sampled. */
        virtual void endRenderPass() = 0;

        /** Returns the native descriptor the compute pass the frame opens next is created with
            to be sampled for the scope of @p scopeIndex (-1 for none), or nullptr once the frame
            ran out of samples. On Metal a retained MTLComputePassDescriptor, which the
            backend creating the pass takes ownership of. */
        virtual void* beginComputePass (int scopeIndex) = 0;

        /** Called right after the compute pass opened: @p sampled is false when it could not
            take the descriptor, which leaves its scope unmeasured. */
        virtual void endComputePass (bool sampled) = 0;

        /** Returns the time of each scope a pass was sampled for, by scope index, once the frame
            of commit number @p sequence completed, attributed with @p timeline. Scopes no pass
            was sampled for, past the last sampled one, are left out. */
        virtual std::vector<GpuScopeTiming> resolveScopes (ScopeTimeline& timeline, uint64 sequence) const = 0;
    };

    /** Submits the command buffer the frame of @p generation recorded into.

        Backends that report completion arrange here to call completeFrameStatus() once the
        GPU finished the frame, when they know whether it measures its scopes. The default
        calls submitFrameCommands().

        @param commandBuffer   The handle beginFrameCommands() returned.
        @param generation      The generation of the frame.
        @param scopeSampler    The sampler the frame created when it measured, or nullptr. The
                               device keeps it until the GPU finished the frame.
        @param measuresScopes  False when the frame turned timing off since: the samples are
                               then not resolved.

        @returns Result::ok(), or why the frame could not be submitted.
    */
    virtual Result submitFrame (void* commandBuffer, uint64_t generation, std::shared_ptr<ScopeSampler> scopeSampler, bool measuresScopes);

    /** Creates the sampler of a frame that starts measuring its scopes, whose passes begin on
        @p frameContext.

        @returns The sampler, or nullptr when the device cannot measure scopes (the default)
                 or could not allocate the samples: the scopes then report unavailable.
    */
    virtual std::shared_ptr<ScopeSampler> createScopeSampler (rive::ore::Context& frameContext);

    /** Waits until the GPU finished a submitted frame command buffer.

        @returns Result::ok(), or the failure the GPU reported executing it. Does
                 nothing and returns Result::ok() by default.
    */
    virtual Result waitFrameCommands (void* commandBuffer);

    /** Releases a command buffer handed out by beginFrameCommands(), once its frame
        no longer needs it. Does nothing by default.
    */
    virtual void releaseFrameCommands (void* commandBuffer);

    /** True when the backend calls completeFrameStatus() once the GPU finished a frame.
        Otherwise a frame counts as complete as soon as it was submitted.
    */
    virtual bool reportsFrameCompletion() const noexcept { return false; }

    /** Records the GPU outcome of the frame of @p generation, for its status, its waiter
        or takeGpuFailure(), with the GPU time the backend measured for it. The timings are
        kept only when the frame asked for them and succeeded. Thread safe.

        The backend lists in GpuFrameTimings::scopes the time of each scope it sampled, by
        scope index and without names: the frame's own scope list provides the names.
    */
    void completeFrameStatus (uint64_t generation, const Result& result, GpuFrameTimings timings = {});

    /** Does what completeFrameStatus() does, and stays safe to call after the device is gone. */
    class YUP_API FrameCompletion;

    /** Returns a FrameCompletion for this device: completion handlers capture it rather than
        the device, as they may run after it was destroyed.
    */
    FrameCompletion getFrameCompletion() const;

    /** Pool of in-flight frames assumed by the default getSafeFrameGeneration(). */
    static constexpr uint64_t framesInFlight = 3;

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

        /** Adds the number of pooled buffers and their total capacity to @p size. */
        void addSize (std::pair<int, size_t>& size) const noexcept;

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

        A frame's texture views, samplers, pooled uniform buffers and bound storage
        buffers are still referenced by the commands it submitted, so they cannot be released the
        moment submit() returns. Blocking on the GPU to find out when they are
        free costs a full pipeline stall every frame; instead each frame hands its
        resources here tagged with a generation, and a generation is released once
        enough later frames have begun that the GPU cannot still be reading it.

        Vulkan storage buffers are not retired here: Rive's resource purgatory
        already keeps their native buffers until the GPU is done, and a retired
        GpuBuffer, which holds its device, would keep the device alive forever.

        The count is a safety margin, not a correctness requirement on the
        backends YUP ships: Metal retains resources on the command buffer, OpenGL
        defers deletion of names still referenced by queued commands, and D3D11
        and WebGPU hold their own references. A manager-backed backend (ore's
        Vulkan and D3D12 paths) relies on it, which is why the matching
        safeFrameNumber is also reported to the ore context, and why the Vulkan
        backend derives it from its command buffer fences instead of assuming
        framesInFlight (see getSafeFrameGeneration()).
    */
    struct RetiredFrame
    {
        uint64_t generation = 0;
        std::vector<rive::rcp<rive::ore::Buffer>> buffers;
        std::vector<rive::rcp<rive::ore::TextureView>> views;
        std::vector<rive::rcp<rive::ore::Sampler>> samplers;
        std::vector<ReferenceCountedObjectPtr<GpuBuffer>> storageBuffers;
    };

    /** Takes ownership of a submitted frame's transient resources. */
    void retireFrameResources (uint64_t generation,
                               std::vector<rive::rcp<rive::ore::Buffer>> buffers,
                               std::vector<rive::rcp<rive::ore::TextureView>> views,
                               std::vector<rive::rcp<rive::ore::Sampler>> samplers,
                               std::vector<ReferenceCountedObjectPtr<GpuBuffer>> storageBuffers);

    /** Releases every retired generation up to and including @p generation. */
    void releaseRetiredFrames (uint64_t generation);

    uint64_t frameGeneration = 0;
    std::vector<RetiredFrame> retiredFrames;

    /** Creates the status of the frame of @p generation, completed by completeFrameStatus(). */
    GpuFrameStatus beginFrameStatus (uint64_t generation);

    /** The frame statuses of this device, shared with the statuses and the completion handlers,
        which may all outlive it. */
    struct FrameStatuses
    {
        /** Guards every frame status of the device. */
        std::shared_ptr<CriticalSection> lock = std::make_shared<CriticalSection>();
        std::map<uint64_t, std::shared_ptr<GpuFrameStatus::Block>> pending;

        /** At most the first failure nobody claimed yet. */
        std::vector<std::shared_ptr<GpuFrameStatus::Block>> unclaimedFailures;
        String nextFrameFailure;

        void complete (uint64_t generation, const Result& result, GpuFrameTimings timings);
    };

    std::shared_ptr<FrameStatuses> frameStatuses = std::make_shared<FrameStatuses>();
    std::shared_ptr<ScopeTimeline> scopeTimeline = std::make_shared<ScopeTimeline>();

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GpuDevice)
};

//==============================================================================
class YUP_API GpuDevice::FrameCompletion
{
public:
    /** Completes the status of the frame of @p generation, as completeFrameStatus() does. */
    void operator() (uint64_t generation, const Result& result, GpuFrameTimings timings = {}) const;

private:
    friend class GpuDevice;

    std::shared_ptr<FrameStatuses> statuses;
};

} // namespace yup
