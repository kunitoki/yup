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
    rive::rcp<rive::ore::Buffer> buffer;
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
    return i != nullptr && i->buffer != nullptr;
}

rive::ore::Buffer* GpuBuffer::oreBufferHandle() const noexcept
{
    auto* i = getImpl();
    return (i != nullptr) ? i->buffer.get() : nullptr;
}

//==============================================================================

GpuBuffer::Ptr GpuBuffer::create (GraphicsContext& ctx,
                                  GpuBufferType type,
                                  const void* data,
                                  size_t byteSize)
{
    auto* oreCtx = ctx.gpuContext();
    if (oreCtx == nullptr)
        return nullptr;

    jassert (data != nullptr && byteSize > 0);
    if (data == nullptr || byteSize == 0)
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
        case GpuBufferType::uniform:
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

    auto* result = new GpuBuffer();
    result->impl = TypeErasedObject<GpuBuffer::kImplSize> (GpuBuffer::Impl { type, byteSize, std::move (buffer) });
    return result;
}

} // namespace yup
