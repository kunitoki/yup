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
/** Encapsulates a graphics context that abstracts windowed rendering operations
    across various APIs, including Rive vector rendering and swapchain presentation.

    GraphicsContext wraps a GpuDevice (GPU device abstraction) and adds the
    window/swapchain layer plus Rive vector rendering support. It requires a
    native window handle (via onSizeChanged) to create a swapchain for presentation.

    For GPU compute without a window (e.g. audio DSP on the GPU), use GpuDevice
    directly — it does not require a window or Rive dependency.

    @see GpuDevice, ComponentNative::getGraphicsContext
*/
class YUP_API GraphicsContext
{
public:
    //==============================================================================
    /** Procedure load function used by GL and Vulkan to locate methods at runtime. */
    using LoaderFunction = void* (*) (const char*);

    //==============================================================================
    /** Configuration options for creating a graphics context.

        Extends GpuDevice::Options with window-specific settings. */
    using Options = GpuDevice::Options;

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
    /** Returns true if a GPU (ore) context is available for RHI operations. */
    bool isGpuAvailable() const noexcept;

    //==============================================================================
    /** Returns the graphics API used by this context.

        @return The GpuPlatform enum value identifying the active rendering backend.
    */
    virtual GpuPlatform getPlatform() const noexcept = 0;

    //==============================================================================
    /** Returns the underlying GpuDevice that owns the GPU device.

        The returned pointer is valid for the lifetime of this GraphicsContext.
        Use this to create GpuPipelines, GpuBuffers, and other RHI resources
        that can outlive the window.

        @return A shared pointer to the GpuDevice, or nullptr if unavailable.
    */
    virtual GpuDevice::Ptr getGpuDevice() const noexcept = 0;

    //==============================================================================
    /** Provides access to the associated factory for resource creation.

        @return Pointer to a rive::Factory object.
    */
    virtual rive::Factory* getFactory() = 0;

    /** Gets the PLS render context, if available.

        @return Pointer to a rive::pls::PLSRenderContext, or nullptr if not available.
    */
    virtual rive::gpu::RenderContext* getRenderContext() = 0;

    /** Gets the PLS render target, if available.

        @return Pointer to a rive::pls::PLSRenderTarget, or nullptr if not available.
    */
    virtual rive::gpu::RenderTarget* getRenderTarget() = 0;

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
        @param dpiScale The scale factor for high-DPI displays.
        @param sampleCount The number of samples per pixel, for anti-aliasing.
    */
    virtual void onSizeChanged (void* nativeHandle, int width, int height, float dpiScale, uint32_t sampleCount) = 0;

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
    /** Static factory method to create a graphics context using a specific graphics API.

        @param graphicsApi The graphics API to use.
        @param options Configuration options for the graphics context.
        @param existingGpu An optional existing GpuDevice to share. When provided,
                           the GraphicsContext borrows the GPU device instead of
                           creating a new one. Useful when an audio processor
                           already owns a GpuDevice for compute.

        @return A unique pointer to a GraphicsContext, using the specified graphics
                API and configured according to the options.
    */
    static std::unique_ptr<GraphicsContext> createContext (GpuPlatform graphicsApi,
                                                           Options options,
                                                           GpuDevice::Ptr existingGpu = {});
};

} // namespace yup
