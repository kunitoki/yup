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

bool GpuDevice::isFormatSupported (GpuTextureFormat format) const noexcept
{
    auto* oreCtx = getGpuContext();
    if (oreCtx == nullptr)
        return false;

    const auto& features = oreCtx->features();

    switch (format)
    {
        case GpuTextureFormat::bc1unorm:
        case GpuTextureFormat::bc3unorm:
        case GpuTextureFormat::bc7unorm:
            return features.bc;

        case GpuTextureFormat::etc2rgb8:
        case GpuTextureFormat::etc2rgba8:
            return features.etc2;

        case GpuTextureFormat::astc4x4:
        case GpuTextureFormat::astc6x6:
        case GpuTextureFormat::astc8x8:
            return features.astc;

        default:
            return true;
    }
}

bool GpuDevice::isFormatRenderable (GpuTextureFormat format) const noexcept
{
    auto* oreCtx = getGpuContext();
    if (oreCtx == nullptr)
        return false;

    const auto& features = oreCtx->features();

    switch (format)
    {
        // 32-bit float attachments are the strictest ask.
        case GpuTextureFormat::rgba32float:
        case GpuTextureFormat::rg32float:
        case GpuTextureFormat::r32float:
            return features.colorBufferFloat;

        // WebGL2 can expose half-float attachments without the full-float ones.
        case GpuTextureFormat::rgba16float:
        case GpuTextureFormat::rg16float:
        case GpuTextureFormat::r16float:
        case GpuTextureFormat::r11g11b10float:
            return features.colorBufferFloat || features.colorBufferHalfFloat;

        // Block-compressed formats are never renderable.
        case GpuTextureFormat::bc1unorm:
        case GpuTextureFormat::bc3unorm:
        case GpuTextureFormat::bc7unorm:
        case GpuTextureFormat::etc2rgb8:
        case GpuTextureFormat::etc2rgba8:
        case GpuTextureFormat::astc4x4:
        case GpuTextureFormat::astc6x6:
        case GpuTextureFormat::astc8x8:
            return false;

        default:
            return true;
    }
}

bool GpuDevice::isAnisotropicFilteringAvailable() const noexcept
{
    auto* oreCtx = getGpuContext();
    return oreCtx != nullptr && oreCtx->features().anisotropicFiltering;
}

uint32_t GpuDevice::getMaximumSampleCount() const noexcept
{
    auto* oreCtx = getGpuContext();
    return oreCtx != nullptr ? oreCtx->features().maxSamples : 1u;
}

//==============================================================================

ReferenceCountedObjectPtr<GpuBuffer> GpuDevice::createBuffer (GpuBufferType type,
                                                              const void* data,
                                                              size_t byteSize)
{
    if (data == nullptr || byteSize == 0)
        return nullptr;

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
    desc.immutable = false;
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

    if (impl->oreBuffer != nullptr)
    {
        if (byteSize > buffer->getSizeInBytes())
            return false;

        impl->oreBuffer->update (data, (uint32_t) byteSize);
        return true;
    }

    return false;
}

//==============================================================================

GpuDevice::~GpuDevice()
{
    // A backend that forgets releasePooledResources() would let pooled ore buffers
    // be destroyed after the ore context that created them.
    jassert (uniformBufferPool.isEmpty());
    jassert (retiredFrames.empty());
}

void GpuDevice::releasePooledResources() noexcept
{
    retiredFrames.clear();
    uniformBufferPool.clear();
}

//==============================================================================

uint64_t GpuDevice::beginFrameGeneration()
{
    ++frameGeneration;

    releaseRetiredFrames (getSafeFrameGeneration());

    return frameGeneration;
}

uint64_t GpuDevice::getSafeFrameGeneration() const noexcept
{
    return frameGeneration > framesInFlight ? frameGeneration - framesInFlight : 0;
}

void GpuDevice::retireFrameResources (uint64_t generation,
                                      std::vector<rive::rcp<rive::ore::Buffer>> buffers,
                                      std::vector<rive::rcp<rive::ore::TextureView>> views,
                                      std::vector<rive::rcp<rive::ore::Sampler>> samplers)
{
    if (buffers.empty() && views.empty() && samplers.empty())
        return;

    retiredFrames.push_back ({ generation, std::move (buffers), std::move (views), std::move (samplers) });
}

void GpuDevice::releaseRetiredFrames (uint64_t generation)
{
    if (retiredFrames.empty())
        return;

    auto isSafe = [generation] (const RetiredFrame& frame)
    {
        return frame.generation <= generation;
    };

    for (auto& frame : retiredFrames)
    {
        if (! isSafe (frame))
            continue;

        for (auto& buffer : frame.buffers)
            uniformBufferPool.release (std::move (buffer));
    }

    retiredFrames.erase (std::remove_if (retiredFrames.begin(), retiredFrames.end(), isSafe),
                         retiredFrames.end());
}

//==============================================================================

size_t GpuDevice::UniformBufferPool::bucketFor (size_t byteSize) noexcept
{
    size_t index = 0;

    for (size_t capacity = minimumCapacity; capacity < byteSize; capacity <<= 1)
        ++index;

    return index;
}

rive::rcp<rive::ore::Buffer> GpuDevice::UniformBufferPool::acquire (rive::ore::Context& oreCtx, size_t byteSize)
{
    if (byteSize == 0)
        return nullptr;

    const auto index = bucketFor (byteSize);

    if (index >= buckets.size())
        buckets.resize (index + 1);

    auto& bucket = buckets[index];

    if (! bucket.empty())
    {
        auto buffer = std::move (bucket.back());
        bucket.pop_back();
        return buffer;
    }

    rive::ore::BufferDesc desc;
    desc.usage = rive::ore::BufferUsage::uniform;
    desc.size = static_cast<uint32_t> (minimumCapacity << index);
    desc.data = nullptr;
    desc.immutable = false;
    desc.label = "GpuRenderPass uniform";

    return oreCtx.makeBuffer (desc);
}

void GpuDevice::UniformBufferPool::release (rive::rcp<rive::ore::Buffer> buffer)
{
    if (buffer == nullptr)
        return;

    // Capacities are exactly minimumCapacity << index, so the buffer lands back in
    // the bucket it came from, which acquire() has already created.
    const auto index = bucketFor (buffer->size());
    if (index < buckets.size())
        buckets[index].push_back (std::move (buffer));
}

void GpuDevice::UniformBufferPool::clear() noexcept
{
    buckets.clear();
}

bool GpuDevice::UniformBufferPool::isEmpty() const noexcept
{
    for (const auto& bucket : buckets)
        if (! bucket.empty())
            return false;

    return true;
}

} // namespace yup
