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

//==============================================================================

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

//==============================================================================

ReferenceCountedObjectPtr<GpuBuffer> GpuDevice::createBuffer (GpuBufferType type,
                                                              const void* data,
                                                              size_t byteSize)
{
    jassert (data != nullptr && byteSize > 0);
    if (data == nullptr || byteSize == 0)
        return nullptr;

    // Storage buffers must be handled by backend overrides.
    if (type == GpuBufferType::storage)
        return nullptr;

    auto* oreCtx = getGpuContext();
    if (oreCtx == nullptr)
        return nullptr;

    rive::ore::BufferDesc desc;
    switch (type)
    {
        case GpuBufferType::vertex:
            desc.usage = rive::ore::BufferUsage::vertex;
            break;
        case GpuBufferType::index:
            desc.usage = rive::ore::BufferUsage::index;
            break;
        default:
            desc.usage = rive::ore::BufferUsage::uniform;
            break;
    }

    desc.size = (uint32_t) byteSize;
    desc.data = data;
    desc.immutable = true;
    desc.label = "GpuBuffer";

    auto buffer = oreCtx->makeBuffer (desc);
    if (buffer == nullptr)
        return nullptr;

    return GpuBuffer::createWithImpl (GpuBuffer::Impl { type, byteSize, std::move (buffer) });
}

bool GpuDevice::readBuffer (GpuBuffer::Ptr, void*, size_t)
{
    return false;
}

bool GpuDevice::updateBuffer (GpuBuffer::Ptr buffer, const void* data, size_t byteSize)
{
    if (buffer == nullptr || data == nullptr || byteSize == 0)
        return false;

    auto* impl = buffer->getImpl();
    if (impl == nullptr)
        return false;

    // For ore-backed buffers (vertex, index, uniform), update in place.
    if (impl->oreBuffer != nullptr)
    {
        if (byteSize > buffer->getSizeInBytes())
            return false;

        impl->oreBuffer->update (data, (uint32_t) byteSize);
        return true;
    }

    return false;
}

} // namespace yup
