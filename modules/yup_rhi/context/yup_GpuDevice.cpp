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

// Forward declarations for backend-specific factory functions
std::unique_ptr<GpuDevice> yup_constructHeadlessGpuDevice (GpuDevice::Options);
#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
std::unique_ptr<GpuDevice> yup_constructMetalGpuDevice (GpuDevice::Options);
#endif
#if YUP_RIVE_USE_D3D && YUP_WINDOWS
std::unique_ptr<GpuDevice> yup_constructDirect3DGpuDevice (GpuDevice::Options);
#endif
#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
std::unique_ptr<GpuDevice> yup_constructOpenGLGpuDevice (GpuDevice::Options);
#endif
#if YUP_EMSCRIPTEN && RIVE_WEBGPU
std::unique_ptr<GpuDevice> yup_constructWebGPUGpuDevice (GpuDevice::Options);
#elif YUP_RIVE_USE_DAWN
std::unique_ptr<GpuDevice> yup_constructDawnGpuDevice (GpuDevice::Options);
#endif

GpuDevice::Ptr GpuDevice::create (GpuPlatform gpuApi, Options options)
{
    std::unique_ptr<GpuDevice> ctx;

    switch (gpuApi)
    {
        case GpuPlatform::Headless:
            ctx = yup_constructHeadlessGpuDevice (options);
            break;

#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
        case GpuPlatform::Metal:
            ctx = yup_constructMetalGpuDevice (options);
            break;
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
        case GpuPlatform::Direct3D:
            ctx = yup_constructDirect3DGpuDevice (options);
            break;
#endif

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
        case GpuPlatform::OpenGL:
        case GpuPlatform::OpenGLES:
            ctx = yup_constructOpenGLGpuDevice (options);
            break;
#endif

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        case GpuPlatform::WebGPU:
            ctx = yup_constructWebGPUGpuDevice (options);
            break;
#elif YUP_RIVE_USE_DAWN
        case GpuPlatform::WebGPU:
            ctx = yup_constructDawnGpuDevice (options);
            break;
#endif

        default:
            Logger::outputDebugString ("Invalid GPU API requested for current platform");
            return nullptr;
    }

    if (ctx == nullptr)
    {
        Logger::outputDebugString ("Failed to create the GPU context");
        return nullptr;
    }

    return ctx.release();
}

} // namespace yup
