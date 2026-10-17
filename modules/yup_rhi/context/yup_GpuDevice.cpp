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
#if YUP_RIVE_USE_METAL && YUP_APPLE
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
#endif
#if YUP_RIVE_USE_VULKAN
std::unique_ptr<GpuDevice> yup_constructVulkanGpuDevice (GpuDevice::Options);
bool yup_isVulkanGpuDeviceSupported (const GpuDevice::Options&);
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

#if YUP_RIVE_USE_METAL && YUP_APPLE
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
#endif

#if YUP_RIVE_USE_VULKAN
        case GpuPlatform::Vulkan:
            ctx = yup_constructVulkanGpuDevice (options);
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

bool GpuDevice::isPlatformSupported (GpuPlatform gpuApi, [[maybe_unused]] const Options& options)
{
    switch (gpuApi)
    {
        case GpuPlatform::Headless:
            return true;

#if YUP_RIVE_USE_METAL && YUP_APPLE
        case GpuPlatform::Metal:
            return true;
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
        case GpuPlatform::Direct3D:
            return true;
#endif

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
        case GpuPlatform::OpenGL:
        case GpuPlatform::OpenGLES:
            return true;
#endif

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        case GpuPlatform::WebGPU:
            return true;
#endif

#if YUP_RIVE_USE_VULKAN
        case GpuPlatform::Vulkan:
            return yup_isVulkanGpuDeviceSupported (options);
#endif

        default:
            return false;
    }
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
        // EXT_color_buffer_float lists R11F_G11F_B10F, EXT_color_buffer_half_float does not
        // (https://registry.khronos.org/OpenGL/extensions/EXT/EXT_color_buffer_float.txt,
        //  https://registry.khronos.org/OpenGL/extensions/EXT/EXT_color_buffer_half_float.txt).
        case GpuTextureFormat::r11g11b10float:
            return features.colorBufferFloat;

        // WebGL2 can expose half-float attachments without the full-float ones.
        case GpuTextureFormat::rgba16float:
        case GpuTextureFormat::rg16float:
        case GpuTextureFormat::r16float:
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
    return oreCtx != nullptr ? std::bit_floor (oreCtx->features().maxSamples) : 1u;
}

uint32_t GpuDevice::getMaximumSampleCount (GpuTextureFormat format) const noexcept
{
    return isFormatRenderable (format) ? getMaximumSampleCount() : 0u;
}

uint32_t GpuDevice::getMaximumColorAttachments() const noexcept
{
    auto* oreCtx = getGpuContext();
    return oreCtx != nullptr ? jlimit (1u, 4u, oreCtx->features().maxColorAttachments) : 0u;
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

    // A frame that never completed would leave its waiters blocked forever
    std::map<uint64_t, std::shared_ptr<GpuFrameStatus::Block>> abandoned;

    {
        const ScopedLock sl (*frameStatuses->lock);
        abandoned.swap (frameStatuses->pending);
        frameStatuses->unclaimedFailures.clear();

        for (auto& [generation, block] : abandoned)
        {
            block->completed = true;
            block->result = Result::fail ("The device was destroyed before the frame completed");
        }
    }

    for (auto& [generation, block] : abandoned)
        block->done.signal();
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

void* GpuDevice::beginFrameCommands (uint64_t)
{
    return nullptr;
}

Result GpuDevice::submitFrameCommands (void*)
{
    return Result::ok();
}

Result GpuDevice::submitFrame (void* commandBuffer, uint64_t, std::shared_ptr<ScopeSampler>, bool)
{
    return submitFrameCommands (commandBuffer);
}

std::shared_ptr<GpuDevice::ScopeSampler> GpuDevice::createScopeSampler (rive::ore::Context&, void*)
{
    return nullptr;
}

Result GpuDevice::waitFrameCommands (void*)
{
    return Result::ok();
}

void GpuDevice::releaseFrameCommands (void*)
{
}

//==============================================================================

Result GpuDevice::takeGpuFailure()
{
    const ScopedLock sl (*frameStatuses->lock);

    auto result = Result::ok();
    for (auto& block : frameStatuses->unclaimedFailures)
    {
        if (block->claimed)
            continue;

        block->claimed = true;
        result = block->result;
        break;
    }

    frameStatuses->unclaimedFailures.clear();
    return result;
}

void GpuDevice::failNextFrameForTesting (const String& error)
{
    const ScopedLock sl (*frameStatuses->lock);
    frameStatuses->nextFrameFailure = error;
}

GpuFrameStatus GpuDevice::beginFrameStatus (uint64_t generation)
{
    auto block = std::make_shared<GpuFrameStatus::Block>();
    block->lock = frameStatuses->lock;

    const ScopedLock sl (*frameStatuses->lock);
    block->injectedFailure = std::exchange (frameStatuses->nextFrameFailure, {});
    frameStatuses->pending[generation] = block;

    // The status may outlive the device, which clears its waiter before it goes
    if (frameStatuses->waiter != nullptr)
    {
        block->waitForGpu = [statuses = std::weak_ptr<FrameStatuses> (frameStatuses), generation]
        {
            std::function<void (uint64_t)> waiter;

            if (const auto locked = statuses.lock())
            {
                const ScopedLock waiterLock (*locked->lock);
                waiter = locked->waiter;
            }

            if (waiter != nullptr)
                waiter (generation);
        };
    }

    return GpuFrameStatus (std::move (block));
}

void GpuDevice::setFrameWaiter (std::function<void (uint64_t)> waiter)
{
    const ScopedLock sl (*frameStatuses->lock);
    frameStatuses->waiter = std::move (waiter);
}

void GpuDevice::completeFrameStatus (uint64_t generation, const Result& result, GpuFrameTimings timings)
{
    frameStatuses->complete (generation, result, std::move (timings));
}

//==============================================================================

uint64 GpuDevice::ScopeTimeline::commit() noexcept
{
    const ScopedLock sl (lock);
    return ++nextSequence;
}

void GpuDevice::ScopeTimeline::activate() noexcept
{
    active = true;
}

bool GpuDevice::ScopeTimeline::isActive() const noexcept
{
    return active;
}

void GpuDevice::ScopeTimeline::advance (uint64 sequence, uint64 end)
{
    const ScopedLock sl (lock);

    if (sequence > latestSequence)
    {
        latestSequence = sequence;
        latestEnd = end;
    }
    else if (sequence == latestSequence)
    {
        latestEnd = jmax (latestEnd, end);
    }
}

std::vector<GpuScopeTiming> GpuDevice::ScopeTimeline::attribute (uint64 sequence, const std::vector<Pass>& passes, uint64 now, double nanosecondsPerTick)
{
    std::vector<GpuScopeTiming> scopes;

    for (const auto& pass : passes)
    {
        if (pass.scopeIndex >= static_cast<int> (scopes.size()))
            scopes.resize (static_cast<size_t> (pass.scopeIndex) + 1, { {}, 0.0, true });
    }

    // The work of an earlier frame bounds this one's only when it was committed before it, and
    // ended before now: otherwise the frame completed out of order or the GPU clock went back
    uint64 frontier = 0;
    {
        const ScopedLock sl (lock);
        if (latestSequence < sequence && latestEnd <= now)
            frontier = latestEnd;
    }

    for (const auto& pass : passes)
    {
        auto* scope = pass.scopeIndex >= 0 ? &scopes[static_cast<size_t> (pass.scopeIndex)] : nullptr;

        if (! pass.sampled || nanosecondsPerTick <= 0.0)
        {
            if (scope != nullptr)
                scope->available = false;

            continue;
        }

        if (pass.start == 0 || pass.end < pass.start)
            continue;

        const auto from = jmax (pass.start, frontier);
        if (scope != nullptr && pass.end > from)
            scope->milliseconds += static_cast<double> (pass.end - from) * nanosecondsPerTick * 1.0e-6;

        frontier = jmax (frontier, pass.end);
    }

    if (frontier > 0)
        advance (sequence, frontier);

    for (auto& scope : scopes)
    {
        if (! scope.available)
            scope.milliseconds = 0.0;
    }

    return scopes;
}

std::optional<uint64> GpuDevice::ScopeTimeline::getEndTicks (uint64 cpuNow, uint64 gpuNow, double hostEnd, double nanosecondsPerTick) noexcept
{
    if (! (nanosecondsPerTick > 0.0))
        return std::nullopt;

    const auto sinceEnd = static_cast<double> (cpuNow) - hostEnd;
    if (sinceEnd < -1.0e3)
        return std::nullopt;

    const auto ticksSinceEnd = static_cast<uint64> (jmax (0.0, sinceEnd) / nanosecondsPerTick);
    if (gpuNow <= ticksSinceEnd)
        return std::nullopt;

    return gpuNow - ticksSinceEnd;
}

//==============================================================================

GpuDevice::FrameCompletion GpuDevice::getFrameCompletion() const
{
    FrameCompletion completion;
    completion.statuses = frameStatuses;
    return completion;
}

void GpuDevice::FrameCompletion::operator() (uint64_t generation, const Result& result, GpuFrameTimings timings) const
{
    statuses->complete (generation, result, std::move (timings));
}

void GpuDevice::FrameStatuses::complete (uint64_t generation, const Result& result, GpuFrameTimings timings)
{
    std::shared_ptr<GpuFrameStatus::Block> block;

    {
        const ScopedLock sl (*lock);

        const auto found = pending.find (generation);
        if (found == pending.end())
            return;

        block = std::move (found->second);
        pending.erase (found);

        block->completed = true;
        block->result = block->injectedFailure.isNotEmpty() ? Result::fail (block->injectedFailure) : result;

        if (block->timingEnabled && block->result.wasOk())
        {
            block->timings.available = timings.available;
            block->timings.frameMilliseconds = timings.frameMilliseconds;

            // The frame's own list names the scopes: only numbers are copied, nothing allocates
            if (timings.available)
            {
                for (size_t index = 0; index < block->scopes.size(); ++index)
                {
                    auto& scope = block->scopes[index];

                    if (index < timings.scopes.size())
                    {
                        scope.available = scope.available && timings.scopes[index].available;
                        scope.milliseconds = timings.scopes[index].milliseconds;
                    }

                    if (! scope.available)
                        scope.milliseconds = 0.0;
                }

                block->timings.scopes = std::move (block->scopes);
            }
        }

        if (block->result.failed())
        {
            YUP_DBG ("GpuDevice: frame " << (int64) generation << " failed: " << block->result.getErrorMessage());

            // Only the first unclaimed failure is reported, so the list never grows
            unclaimedFailures.erase (std::remove_if (unclaimedFailures.begin(), unclaimedFailures.end(), [] (const auto& unclaimed)
            {
                return unclaimed->claimed;
            }),
                                     unclaimedFailures.end());

            if (! block->claimed && unclaimedFailures.empty())
                unclaimedFailures.push_back (block);
        }
    }

    block->done.signal();
}

void GpuDevice::retireFrameResources (uint64_t generation,
                                      std::vector<rive::rcp<rive::ore::Buffer>> buffers,
                                      std::vector<rive::rcp<rive::ore::TextureView>> views,
                                      std::vector<rive::rcp<rive::ore::Sampler>> samplers,
                                      std::vector<ReferenceCountedObjectPtr<GpuBuffer>> storageBuffers,
                                      std::vector<ReferenceCountedObjectPtr<GpuPipeline>> pipelines)
{
    if (buffers.empty() && views.empty() && samplers.empty() && storageBuffers.empty() && pipelines.empty())
        return;

    retiredFrames.push_back ({ generation, std::move (buffers), std::move (views), std::move (samplers), std::move (storageBuffers), std::move (pipelines) });
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

void GpuDevice::UniformBufferPool::addSize (std::pair<int, size_t>& size) const noexcept
{
    for (const auto& bucket : buckets)
    {
        for (const auto& buffer : bucket)
        {
            ++size.first;
            size.second += buffer->size();
        }
    }
}

std::pair<int, size_t> GpuDevice::getUniformBufferPoolSizeForTesting() const noexcept
{
    std::pair<int, size_t> size;
    uniformBufferPool.addSize (size);

    for (const auto& frame : retiredFrames)
    {
        for (const auto& buffer : frame.buffers)
        {
            ++size.first;
            size.second += buffer->size();
        }
    }

    return size;
}

} // namespace yup
