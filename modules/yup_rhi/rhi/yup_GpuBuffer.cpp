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

struct GpuBuffer::Impl
{
    GpuBufferType type = GpuBufferType::vertex;
    size_t byteSize = 0;

    // Ore buffer (for vertex, index, uniform).
    rive::rcp<rive::ore::Buffer> oreBuffer;

    // Native storage buffer handles (for compute).
#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
    id<MTLBuffer> mtlStorageBuffer = nil;

    ~Impl() = default;
#elif YUP_RIVE_USE_D3D && YUP_WINDOWS
    ComPtr<ID3D11Buffer> d3dStorageBuffer;
    ComPtr<ID3D11UnorderedAccessView> d3dUav;

    /** Staging copy used by readBuffer(), kept alive so a per-frame reader does not
        reallocate it every frame. Created on first readback. */
    ComPtr<ID3D11Buffer> d3dReadbackStaging;

    ~Impl() = default;
#elif (YUP_EMSCRIPTEN && RIVE_WEBGPU) || YUP_RIVE_USE_DAWN
    wgpu::Buffer webgpuStorageBuffer;

    ~Impl() = default;
#elif YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
    GLuint glBuffer = 0;

    ~Impl()
    {
        if (glBuffer != 0)
            glDeleteBuffers (1, &glBuffer);
    }
#else
    ~Impl() = default;
#endif
};

//==============================================================================

GpuBuffer::~GpuBuffer() = default;

//==============================================================================

GpuBuffer::Impl* GpuBuffer::getImpl() noexcept
{
    return impl.getPayload<Impl>();
}

const GpuBuffer::Impl* GpuBuffer::getImpl() const noexcept
{
    return impl.getPayload<Impl>();
}

//==============================================================================

GpuBufferType GpuBuffer::getType() const noexcept
{
    auto* i = getImpl();
    return i != nullptr ? i->type : GpuBufferType::vertex;
}

size_t GpuBuffer::getSizeInBytes() const noexcept
{
    auto* i = getImpl();
    return i != nullptr ? i->byteSize : 0;
}

bool GpuBuffer::isValid() const noexcept
{
    auto* i = getImpl();
    if (i == nullptr)
        return false;

    if (i->type == GpuBufferType::storage)
    {
#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
        return i->mtlStorageBuffer != nil;
#elif YUP_RIVE_USE_D3D && YUP_WINDOWS
        return i->d3dStorageBuffer != nullptr;
#elif (YUP_EMSCRIPTEN && RIVE_WEBGPU) || YUP_RIVE_USE_DAWN
        return i->webgpuStorageBuffer != nullptr;
#elif YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
        return i->glBuffer != 0;
#else
        return false;
#endif
    }

    return i->oreBuffer != nullptr;
}

//==============================================================================

GpuBuffer::Ptr GpuBuffer::create (GpuDevice::Ptr ctx,
                                  GpuBufferType type,
                                  const void* data,
                                  size_t byteSize)
{
    if (ctx == nullptr)
        return nullptr;

    return ctx->createBuffer (type, data, byteSize);
}

GpuBuffer::Ptr GpuBuffer::createWithImpl (Impl&& impl)
{
    auto* result = new GpuBuffer();
    result->impl = TypeErasedObject (std::move (impl));
    return result;
}

} // namespace yup
