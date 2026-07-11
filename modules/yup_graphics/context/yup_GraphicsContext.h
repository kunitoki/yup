/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
/** Encapsulates a graphics context that abstracts rendering operations across various APIs.

    This class serves as a base for implementing specific graphics context functionalities, such as rendering and resource management,
    across different graphics APIs like OpenGL, OpenGLES, Direct3D, Metal, and WebGPU. It offers a standardized interface for operations
    common to all graphics APIs.
*/
class YUP_API GraphicsContext
{
public:
    //==============================================================================
    /** Procedure load function used by GL and Vulkan to locate methods at runtime. */
    using LoaderFunction = void* (*) (const char*);

    //==============================================================================
    /** Enumerates supported graphics APIs. */
    enum Api
    {
        Headless, ///< Specifies the use of a headless context for rendering.
        OpenGL,   ///< Specifies the use of desktop OpenGL for rendering.
        OpenGLES, ///< Specifies the use of OpenGL ES (GLES 3.0+) for rendering (Android, WASM).
        Direct3D, ///< Specifies the use of Direct3D for rendering.
        Metal,    ///< Specifies the use of Metal for rendering.
        WebGPU    ///< Specifies the use of WebGPU, relying on dawn where not supported natively.
    };

    /** Configuration options for creating a graphics context. */
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
        bool enableOreContext = true;               ///< Enables the ore GPU context for GpuPipeline shader operations.
        LoaderFunction loaderFunction = nullptr;    ///< Loader function (used by GL/Vulkan).
    };

    //==============================================================================
    /** Default constructor. */
    GraphicsContext() noexcept = default;

    /** Destructor. */
    virtual ~GraphicsContext() = default;

    //==============================================================================
    /** Copy and move constructors and assignment operators. */
    GraphicsContext (const GraphicsContext& other) noexcept = delete;
    GraphicsContext (GraphicsContext&& other) noexcept = default;
    GraphicsContext& operator= (const GraphicsContext& other) noexcept = delete;
    GraphicsContext& operator= (GraphicsContext&& other) noexcept = default;

    //==============================================================================
    /** Returns the graphics API used by this context.

        @return The Api enum value identifying the active rendering backend.
    */
    virtual Api getApi() const noexcept = 0;

    //==============================================================================
    /** Returns the DPI scale associated with a native handle.

        @param nativeHandle A platform-specific handle to the native window or screen.

        @return The DPI scale factor.
    */
    virtual float dpiScale (void* nativeHandle) const = 0;

    //==============================================================================
    /** Provides access to the associated factory for resource creation.

        @return Pointer to a rive::Factory object.
    */
    virtual rive::Factory* factory() = 0;

    /** Gets the PLS render context, if available.

        @return Pointer to a rive::pls::PLSRenderContext, or nullptr if not available.
    */
    virtual rive::gpu::RenderContext* renderContext() = 0;

    /** Gets the PLS render target, if available.

        @return Pointer to a rive::pls::PLSRenderTarget, or nullptr if not available.
    */
    virtual rive::gpu::RenderTarget* renderTarget() = 0;

    /** Returns the ore GPU context, or nullptr when enableOreContext was false or ore is
        unavailable on this backend.

        This is the single backend bridge used by the RHI layer (GpuPipeline,
        GpuFrame, GpuRenderPass, GpuBuffer). User code should prefer the ore-free
        isGpuAvailable() capability probe instead.
    */
    virtual rive::ore::Context* gpuContext() const noexcept { return nullptr; }

    /** Returns true if a GPU (ore) context is available for RHI operations.

        Equivalent to gpuContext() != nullptr but without referencing any ore
        type, so user code and examples can probe GPU capability ore-free.
    */
    bool isGpuAvailable() const noexcept { return gpuContext() != nullptr; }

    /** Creates a renderer suitable for the specified dimensions.

        @param width The width of the render area.
        @param height The height of the render area.

        @return A unique pointer to a rive::Renderer object.
    */
    virtual std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) = 0;

    //==============================================================================
    /** Handles changes in the size of the rendering surface.

        @param nativeHandle A platform-specific handle to the native window or screen.
        @param width The new width of the surface.
        @param height The new height of the surface.
        @param sampleCount The number of samples per pixel, for anti-aliasing.
    */
    virtual void onSizeChanged (void* nativeHandle, int width, int height, uint32_t sampleCount) = 0;

    //==============================================================================
    /** Begins a rendering frame.

        @param descriptor The frame descriptor that contains frame-specific data.
    */
    virtual void begin (const rive::gpu::RenderContext::FrameDescriptor&) = 0;

    /** Ends a rendering frame.

        @param nativeHandle A platform-specific handle to the native window or screen.
    */
    virtual void end (void* nativeHandle) = 0;

    /** Performs periodic operations, potentially related to animation or state updates. */
    virtual void tick() {}

    //==============================================================================
    /** Creates platform-specific GPU offscreen resources for the given dimensions.

        Supported GPU backends may create targets while another offscreen target
        is rendering. Backends reserve a render context only while its target
        has an active frame, allowing sequential targets to share idle contexts.
        A target must outlive its corresponding beginOffscreen()/endOffscreen()
        pair and the GraphicsContext must outlive every target it creates.

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
    /** Static factory method to create a graphics context using a specific graphics API.

        @param graphicsApi The graphics API to use.
        @param options Configuration options for the graphics context.

        @return A unique pointer to a GraphicsContext, using the specified graphics API and configured according to the options.
    */
    static std::unique_ptr<GraphicsContext> createContext (Api graphicsApi, Options options);
};

} // namespace yup
