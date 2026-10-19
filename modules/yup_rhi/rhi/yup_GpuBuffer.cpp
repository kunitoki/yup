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

namespace
{

//==============================================================================

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
struct GlStorageBuffer
{
    GlStorageBuffer() = default;

    GlStorageBuffer (GLuint id, ReferenceCountedObjectPtr<GpuDevice> device)
        : id (id)
        , device (std::move (device))
    {
    }

    GlStorageBuffer (GlStorageBuffer&& other) noexcept
        : id (std::exchange (other.id, 0))
        , device (std::move (other.device))
    {
    }

    GlStorageBuffer& operator= (GlStorageBuffer&& other) noexcept
    {
        if (this != &other)
        {
            release();

            id = std::exchange (other.id, 0);
            device = std::move (other.device);
        }

        return *this;
    }

    ~GlStorageBuffer()
    {
        release();
    }

    GLuint id = 0;
    ReferenceCountedObjectPtr<GpuDevice> device;

    GlStorageBuffer (const GlStorageBuffer&) = delete;
    GlStorageBuffer& operator= (const GlStorageBuffer&) = delete;

private:
    void release() noexcept
    {
        if (id == 0)
            return;

        if (device != nullptr)
        {
            device->runOnComputeContext ([id = id]
            {
                GLuint bufferToDelete = id;
                glDeleteBuffers (1, &bufferToDelete);
            });
        }
        else
        {
            glDeleteBuffers (1, &id);
        }
    }
};
#endif

#if YUP_RIVE_USE_VULKAN
/** A Vulkan storage buffer, dropped through the device so its release serializes
    with the render thread: Rive's resource manager and VMA are not thread safe. */
struct VulkanStorageBuffer
{
    VulkanStorageBuffer() = default;

    VulkanStorageBuffer (rive::rcp<rive::gpu::vkutil::Buffer> buffer, ReferenceCountedObjectPtr<GpuDevice> device)
        : buffer (std::move (buffer))
        , device (std::move (device))
    {
    }

    VulkanStorageBuffer (VulkanStorageBuffer&& other) noexcept = default;

    VulkanStorageBuffer& operator= (VulkanStorageBuffer&& other) noexcept
    {
        if (this != &other)
        {
            release();

            buffer = std::move (other.buffer);
            device = std::move (other.device);
            lastUseCommands = std::exchange (other.lastUseCommands, VK_NULL_HANDLE);
            lastUseGeneration = std::exchange (other.lastUseGeneration, 0);
        }

        return *this;
    }

    ~VulkanStorageBuffer()
    {
        release();
    }

    rive::rcp<rive::gpu::vkutil::Buffer> buffer;
    ReferenceCountedObjectPtr<GpuDevice> device;

    /** The command buffer and generation of the last work that bound the buffer, which
        reads and updates wait for instead of the whole queue. */
    VkCommandBuffer lastUseCommands = VK_NULL_HANDLE;
    uint64_t lastUseGeneration = 0;

    VulkanStorageBuffer (const VulkanStorageBuffer&) = delete;
    VulkanStorageBuffer& operator= (const VulkanStorageBuffer&) = delete;

private:
    void release() noexcept
    {
        if (buffer == nullptr || device == nullptr)
            return;

        device->runOnGraphicsContext ([this]
        {
            buffer = nullptr;
        });
    }
};
#endif

} // namespace

//==============================================================================

struct GpuBuffer::Impl
{
    GpuBufferType type = GpuBufferType::vertex;
    size_t byteSize = 0;
    rive::rcp<rive::ore::Buffer> oreBuffer;

#if YUP_RIVE_USE_METAL && YUP_APPLE
    id<MTLBuffer> mtlStorageBuffer = nil;

    /** The command buffers that bound a backing, in any order: frames and standalone compute
        passes commit independently. Weak, as a released command buffer has completed. */
    struct MetalUses
    {
        std::vector<__weak id<MTLCommandBuffer>> commandBuffers;

        void add (id<MTLCommandBuffer> commandBuffer)
        {
            if (commandBuffer == nil)
                return;

            prune();

            for (id<MTLCommandBuffer> use : commandBuffers)
                if (use == commandBuffer)
                    return;

            commandBuffers.push_back (commandBuffer);
        }

        bool isInFlight() const
        {
            for (id<MTLCommandBuffer> use : commandBuffers)
                if (isPending (use))
                    return true;

            return false;
        }

        void prune()
        {
            commandBuffers.erase (std::remove_if (commandBuffers.begin(), commandBuffers.end(), [] (const auto& weakUse)
            {
                id<MTLCommandBuffer> use = weakUse;
                return ! isPending (use);
            }),
                                  commandBuffers.end());
        }

        static bool isPending (id<MTLCommandBuffer> use)
        {
            return use != nil && use.status < MTLCommandBufferStatusCompleted;
        }
    };

    /** The uses of mtlStorageBuffer, which an update must not overwrite while any is in flight. */
    MetalUses mtlUses;

    /** Backings an update swapped out while the GPU could still read them, reused once idle. */
    struct MetalBacking
    {
        id<MTLBuffer> buffer = nil;
        MetalUses uses;
    };

    std::vector<MetalBacking> mtlSpareBackings;
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
    ComPtr<ID3D11Buffer> d3dStorageBuffer;
    ComPtr<ID3D11UnorderedAccessView> d3dUav;
    ComPtr<ID3D11ShaderResourceView> d3dSrv;
    ComPtr<ID3D11Buffer> d3dReadbackStaging;
#endif

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
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

    wgpu::Buffer webgpuStorageBuffer;
    static constexpr size_t numReadbackSlots = 3;
    std::vector<std::shared_ptr<ReadbackSlot>> readbackSlots;
    uint64_t nextReadbackSerial = 0;
    bool readbackUnavailable = false;
#endif

#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
    GlStorageBuffer glStorageBuffer;
#endif

#if YUP_RIVE_USE_VULKAN
    VulkanStorageBuffer vkStorageBuffer;
#endif

    /** The native handle a render pass binds for read-only storage access, or nullptr. */
    void* getNativeStorageHandleForRead() const noexcept
    {
#if YUP_RIVE_USE_METAL && YUP_APPLE
        if (mtlStorageBuffer != nil)
            return (__bridge void*) mtlStorageBuffer;
#endif

#if YUP_RIVE_USE_D3D && YUP_WINDOWS
        if (d3dSrv != nullptr)
            return d3dSrv.Get();
#endif

#if YUP_RIVE_USE_VULKAN
        // A pointer to the VkBuffer, which is a 64-bit handle even on 32-bit ABIs
        if (vkStorageBuffer.buffer != nullptr)
            return const_cast<VkBuffer*> (vkStorageBuffer.buffer->vkBufferAddressOf());
#endif

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        if (webgpuStorageBuffer != nullptr)
            return webgpuStorageBuffer.Get();
#endif

        // createStorageForTesting(): the ore buffer stands in for a native one
        return type == GpuBufferType::storage ? oreBuffer.get() : nullptr;
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
#if YUP_RIVE_USE_METAL && YUP_APPLE
        if (i->mtlStorageBuffer != nil)
            return true;
#endif
#if YUP_RIVE_USE_D3D && YUP_WINDOWS
        if (i->d3dStorageBuffer != nullptr)
            return true;
#endif
#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        if (i->webgpuStorageBuffer != nullptr)
            return true;
#endif
#if YUP_RIVE_USE_OPENGL || YUP_LINUX || YUP_ANDROID || (YUP_WASM && RIVE_WEBGL && ! RIVE_WEBGPU)
        if (i->glStorageBuffer.id != 0)
            return true;
#endif
#if YUP_RIVE_USE_VULKAN
        if (i->vkStorageBuffer.buffer != nullptr)
            return true;
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

GpuBuffer::Ptr GpuBuffer::createStorageForTesting (GpuDevice::Ptr ctx, const void* data, size_t byteSize)
{
    auto* oreCtx = ctx != nullptr ? ctx->getGpuContext() : nullptr;
    if (oreCtx == nullptr || data == nullptr || byteSize == 0)
        return nullptr;

    rive::ore::BufferDesc desc;
    desc.usage = rive::ore::BufferUsage::uniform;
    desc.size = static_cast<uint32_t> (byteSize);
    desc.data = data;
    desc.label = "GpuBuffer storage for testing";

    auto buffer = oreCtx->makeBuffer (desc);
    if (buffer == nullptr)
        return nullptr;

    return createWithImpl (Impl { GpuBufferType::storage, byteSize, std::move (buffer) });
}

GpuBuffer::Ptr GpuBuffer::createWithImpl (Impl&& impl)
{
    auto* result = new GpuBuffer();
    result->impl = TypeErasedObject (std::move (impl));

    return result;
}

} // namespace yup
