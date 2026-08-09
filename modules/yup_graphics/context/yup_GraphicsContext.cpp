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
bool GraphicsContext::isGpuAvailable() const noexcept
{
    if (auto device = getGpuDevice())
        return device->getGpuContext() != nullptr;

    return false;
}

//==============================================================================
std::unique_ptr<GraphicsContext> yup_constructHeadlessGraphicsContext (GpuDevice::Options, GpuDevice::Ptr = {});
#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
std::unique_ptr<GraphicsContext> yup_constructMetalGraphicsContext (GpuDevice::Options, GpuDevice::Ptr = {});
#endif
#if YUP_RIVE_USE_D3D && YUP_WINDOWS
std::unique_ptr<GraphicsContext> yup_constructDirect3DGraphicsContext (GpuDevice::Options, GpuDevice::Ptr = {});
#endif
#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
std::unique_ptr<GraphicsContext> yup_constructOpenGLGraphicsContext (GpuDevice::Options, GpuDevice::Ptr = {});
#endif
#if YUP_EMSCRIPTEN && RIVE_WEBGPU
std::unique_ptr<GraphicsContext> yup_constructWebGPUGraphicsContext (GpuDevice::Options, GpuDevice::Ptr = {});
#elif YUP_RIVE_USE_DAWN
std::unique_ptr<GraphicsContext> yup_constructDawnGraphicsContext (GpuDevice::Options, GpuDevice::Ptr = {});
#endif

//==============================================================================
std::unique_ptr<GraphicsContext> GraphicsContext::createContext (GpuPlatform graphicsApi,
                                                                 Options options,
                                                                 GpuDevice::Ptr existingGpu)
{
    switch (graphicsApi)
    {
        case GpuPlatform::Headless:
            return yup_constructHeadlessGraphicsContext (options, std::move (existingGpu));

#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
        case GpuPlatform::Metal:
            return yup_constructMetalGraphicsContext (options, std::move (existingGpu));
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
        case GpuPlatform::Direct3D:
            return yup_constructDirect3DGraphicsContext (options, std::move (existingGpu));
#endif

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
        case GpuPlatform::OpenGL:
        case GpuPlatform::OpenGLES:
            return yup_constructOpenGLGraphicsContext (options, std::move (existingGpu));
#endif

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        case GpuPlatform::WebGPU:
            return yup_constructWebGPUGraphicsContext (options, std::move (existingGpu));
#elif YUP_RIVE_USE_DAWN
        case GpuPlatform::WebGPU:
            return yup_constructDawnGraphicsContext (options, std::move (existingGpu));
#endif

        default:
            Logger::outputDebugString ("Invalid API requested for current platform");
            return nullptr;
    }

    Logger::outputDebugString ("Failed to create the graphics context");
    return nullptr;
}

} // namespace yup
