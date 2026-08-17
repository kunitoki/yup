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
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
    ComPtr<ID3D11Buffer> d3dStorageBuffer;
    ComPtr<ID3D11UnorderedAccessView> d3dUav;

    /** Staging copy used by readBuffer(), kept alive so a per-frame reader does not
        reallocate it every frame. Created on first readback. */
    ComPtr<ID3D11Buffer> d3dReadbackStaging;
#endif

#if (YUP_EMSCRIPTEN && RIVE_WEBGPU) || YUP_RIVE_USE_DAWN
    wgpu::Buffer webgpuStorageBuffer;

    /** One staging buffer in the pipelined readback ring.

        Held by shared_ptr so an in-flight map callback keeps its slot (and the
        staging buffer it unmaps) alive even if the GpuBuffer is released first.
    */
    struct ReadbackSlot
    {
        wgpu::Buffer staging;
        uint64_t serial = 0;
        bool mapPending = false;
        bool mapped = false;

        ~ReadbackSlot()
        {
            if (mapped && staging != nullptr)
                staging.Unmap();
        }
    };

    /** Three slots keep one copy in flight, one map pending and one ready to
        consume, so a snapshot lands every frame once the ring is primed. */
    static constexpr size_t numReadbackSlots = 3;

    std::vector<std::shared_ptr<ReadbackSlot>> readbackSlots;
    uint64_t nextReadbackSerial = 0;

    /** Set when the staging buffers could not be allocated, so a per-frame reader
        gives up instead of retrying the same failing allocation every frame. */
    bool readbackUnavailable = false;
#endif

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
    GLuint glBuffer = 0;
#endif

    ~Impl()
    {
#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
        if (glBuffer != 0)
            glDeleteBuffers (1, &glBuffer);
#endif
    }
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
        // Only the handle owned by the creating backend is ever non-empty; check
        // each enabled backend independently since several may coexist in a build.
#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)
        if (i->mtlStorageBuffer != nil)
            return true;
#endif
#if YUP_RIVE_USE_D3D && YUP_WINDOWS
        if (i->d3dStorageBuffer != nullptr)
            return true;
#endif
#if (YUP_EMSCRIPTEN && RIVE_WEBGPU) || YUP_RIVE_USE_DAWN
        if (i->webgpuStorageBuffer != nullptr)
            return true;
#endif
#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
        if (i->glBuffer != 0)
            return true;
#endif
        return false;
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
