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

#include <yup_rhi/yup_rhi.h>
#include <yup_graphics/yup_graphics.h>

#if YUP_LINUX || YUP_WINDOWS
#include <SDL3/SDL.h>
#endif

#include <string>

namespace yup::test
{

//==============================================================================
/** Returns the GPU backend that a real device uses on this platform.

    This mirrors the choice the windowing layer makes in getGraphicsContextApi(),
    so a test device exercises the same backend the application does.
*/
inline GpuPlatform nativeGpuPlatform() noexcept
{
#if YUP_MAC || YUP_IOS
    return GpuPlatform::Metal;
#elif YUP_WINDOWS
    return GpuPlatform::Direct3D;
#else
    return GpuPlatform::OpenGL;
#endif
}

//==============================================================================
/** Owns a real, on-GPU GpuDevice for the current platform.

    Rendering tests need a device that actually produces pixels, which
    GpuPlatform::Headless does not. This creates the native resources each
    backend requires and reports failure rather than aborting, so a test can
    skip cleanly on a machine or CI runner with no usable GPU.

    Metal needs no window, so the Metal path is just a device. OpenGL contexts
    are bound to a drawable, so the GL path creates a hidden SDL window and a
    context, then makes it current on the calling thread.

    The device is only valid for as long as this object lives, and on the GL
    backend only on the thread that constructed it.

    Typical use:
    @code
    class MyRenderingTests : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            if (! surface.isValid())
                GTEST_SKIP() << surface.getFailureReason();
        }

        test::GpuTestDevice surface;
    };
    @endcode
*/
class GpuTestDevice
{
public:
    //==============================================================================
    /** Creates the native resources and the device. Check isValid() afterwards. */
    GpuTestDevice()
    {
        GpuDevice::Options options;
        options.allowHeadlessRendering = true;

#if YUP_LINUX || YUP_WINDOWS
        if (nativeGpuPlatform() == GpuPlatform::OpenGL)
        {
            if (! createOpenGLContext())
                return;

            options.loaderFunction = [] (const char* name) -> void*
            {
                return reinterpret_cast<void*> (SDL_GL_GetProcAddress (name));
            };
        }
#endif

        device = GpuDevice::create (nativeGpuPlatform(), options);

        if (device == nullptr)
        {
            failureReason = "GpuDevice::create returned null for the native backend";
            releaseNativeResources();
            return;
        }

        if (device->getRenderContext() == nullptr)
        {
            failureReason = "the native GpuDevice has no render context, so it cannot draw";
            device = nullptr;
            releaseNativeResources();
            return;
        }
    }

    /** Releases the device and any native resources. */
    ~GpuTestDevice()
    {
        device = nullptr;
        releaseNativeResources();
    }

    //==============================================================================
    /** Non-copyable and non-movable: it owns thread-affine native state. */
    GpuTestDevice (const GpuTestDevice&) = delete;
    GpuTestDevice& operator= (const GpuTestDevice&) = delete;

    //==============================================================================
    /** Returns true when a usable device was created. */
    bool isValid() const noexcept { return device != nullptr; }

    /** Returns why creation failed, for a skip message. Empty when valid. */
    const std::string& getFailureReason() const noexcept { return failureReason; }

    /** Returns the device. Only call this when isValid() is true. */
    GpuDevice::Ptr getDevice() const noexcept { return device; }

    /** Returns the device. Only call this when isValid() is true. */
    GpuDevice& operator*() const noexcept
    {
        jassert (device != nullptr);
        return *device;
    }

    /** Returns the device. Only call this when isValid() is true. */
    GpuDevice* operator->() const noexcept
    {
        jassert (device != nullptr);
        return device.get();
    }

    /** Returns true when the environment demands a working GPU.

        A test that skips is indistinguishable from a test that passed in most
        CI summaries, so a runner that quietly lost its GPU would report green
        while proving nothing. Set YUP_TEST_REQUIRE_GPU=1 on any runner that is
        supposed to have one, and a missing device becomes a failure instead.
    */
    static bool isGpuRequired()
    {
        return SystemStats::getEnvironmentVariable ("YUP_TEST_REQUIRE_GPU", {}).trim() == "1";
    }

    /** Returns a short name for the active backend, for test output. */
    static const char* getPlatformName() noexcept
    {
        switch (nativeGpuPlatform())
        {
            case GpuPlatform::Metal:    return "Metal";
            case GpuPlatform::OpenGL:   return "OpenGL";
            case GpuPlatform::OpenGLES: return "OpenGLES";
            case GpuPlatform::Direct3D: return "Direct3D";
            case GpuPlatform::WebGPU:   return "WebGPU";
            case GpuPlatform::Headless: return "Headless";
        }

        return "Unknown";
    }

private:
    //==============================================================================
#if YUP_LINUX || YUP_WINDOWS
    bool createOpenGLContext()
    {
        if (! SDL_InitSubSystem (SDL_INIT_VIDEO))
        {
            failureReason = std::string ("SDL_InitSubSystem(VIDEO) failed: ") + SDL_GetError();
            return false;
        }

        initialisedSdlVideo = true;

#if YUP_LINUX
        // Prefer Mesa's software rasterizer when no real driver is present, which
        // is the situation on CI. An installed driver still takes precedence
        // because these only apply when the variables are not already set.
        SDL_setenv_unsafe ("LIBGL_ALWAYS_SOFTWARE", "1", 0);
        SDL_setenv_unsafe ("GALLIUM_DRIVER", "llvmpipe", 0);
#endif

        // Request the same context the windowing layer asks for, so a test
        // exercises the version the application actually runs on.
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow ("yup_gpu_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (window == nullptr)
        {
            failureReason = std::string ("SDL_CreateWindow failed: ") + SDL_GetError();
            return false;
        }

        glContext = SDL_GL_CreateContext (window);
        if (glContext == nullptr)
        {
            failureReason = std::string ("SDL_GL_CreateContext failed: ") + SDL_GetError();
            return false;
        }

        if (! SDL_GL_MakeCurrent (window, glContext))
        {
            failureReason = std::string ("SDL_GL_MakeCurrent failed: ") + SDL_GetError();
            return false;
        }

        return true;
    }
#endif

    void releaseNativeResources()
    {
#if YUP_LINUX || YUP_WINDOWS
        if (glContext != nullptr)
        {
            SDL_GL_MakeCurrent (window, nullptr);
            SDL_GL_DestroyContext (glContext);
            glContext = nullptr;
        }

        if (window != nullptr)
        {
            SDL_DestroyWindow (window);
            window = nullptr;
        }

        if (initialisedSdlVideo)
        {
            SDL_QuitSubSystem (SDL_INIT_VIDEO);
            initialisedSdlVideo = false;
        }
#endif
    }

    //==============================================================================
    GpuDevice::Ptr device;
    std::string failureReason { "no native GPU backend is available on this platform" };

#if YUP_LINUX || YUP_WINDOWS
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
    bool initialisedSdlVideo = false;
#endif
};

} // namespace yup::test
