/*
 * Copyright 2024 Rive
 */

#pragma once

#include "rive/refcnt.hpp"
#include "rive/pls/pls.hpp"
#include <cassert>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>

struct VmaAllocator_T;
using VmaAllocator = VmaAllocator_T*;

struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;

namespace rive::pls
{
class PLSRenderContextVulkanImpl;
}

namespace rive::pls::vkutil
{
inline static void vk_check(VkResult res, const char* file, int line)
{
    if (res != VK_SUCCESS)
    {
        fprintf(stderr, "Vulkan error %i at line: %i in file: %s\n", res, line, file);
        abort();
    }
}

#define VK_CHECK(x) ::rive::pls::vkutil::vk_check(x, __FILE__, __LINE__)

class Buffer;
class Texture;
class TextureView;
class Framebuffer;

constexpr static VkColorComponentFlags ColorWriteMaskRGBA =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
    VK_COLOR_COMPONENT_A_BIT;

enum class Mappability
{
    none,
    writeOnly,
    readWrite,
};

class Allocator : public RefCnt<Allocator>
{
public:
    Allocator(VkInstance, VkPhysicalDevice, VkDevice, uint32_t vulkanApiVersion);
    ~Allocator();

    VkDevice device() const { return m_device; }

    void setPLSContextImpl(PLSRenderContextVulkanImpl* plsImplVulkan)
    {
        assert(m_plsImplVulkan == nullptr);
        assert(plsImplVulkan != nullptr);
        m_plsImplVulkan = plsImplVulkan;
    }

    void didDestroyPLSContext()
    {
        assert(m_plsImplVulkan != nullptr);
        m_plsImplVulkan = nullptr;
    }

    // Weak pointer (not-thread-safe) back to the PLS context. Becomes null once
    // the context is destroyed.
    PLSRenderContextVulkanImpl* plsImplVulkan() const { return m_plsImplVulkan; }

    VmaAllocator vmaAllocator() const { return m_vmaAllocator; }

    rcp<Buffer> makeBuffer(const VkBufferCreateInfo&, Mappability);

    rcp<Texture> makeTexture(const VkImageCreateInfo&);

    rcp<TextureView> makeTextureView(rcp<Texture>);
    rcp<TextureView> makeTextureView(rcp<Texture> textureRefOrNull, const VkImageViewCreateInfo&);

    rcp<Framebuffer> makeFramebuffer(const VkFramebufferCreateInfo&);

private:
    VkDevice m_device;
    VmaAllocator m_vmaAllocator;
    // Weak pointer back to the PLS context.
    PLSRenderContextVulkanImpl* m_plsImplVulkan = nullptr;
};

// Base class for a GPU resource that needs to be kept alive until any in-flight
// command buffers that reference it have completed.
class RenderingResource : public RefCnt<RenderingResource>
{
public:
    virtual ~RenderingResource() {}

protected:
    RenderingResource(rcp<Allocator> allocator) : m_allocator(std::move(allocator)) {}

    VkDevice device() const { return m_allocator->device(); }

    // Weak pointer (not-thread-safe) back to the PLS context. Becomes null once
    // the context is destroyed.
    PLSRenderContextVulkanImpl* plsImplVulkan() const { return m_allocator->plsImplVulkan(); }

    const rcp<Allocator> m_allocator;

private:
    friend class RefCnt<RenderingResource>;

    // Don't delete RenderingResources immediately when their ref count reaches
    // zero; wait until any in-flight command buffers are done referencing their
    // underlying Vulkan objects.
    void onRefCntReachedZero() const;
};

class Buffer : public RenderingResource
{
public:
    ~Buffer() override;

    VkBufferCreateInfo info() const { return m_info; }
    operator VkBuffer() const { return m_vkBuffer; }
    const VkBuffer* vkBufferAddressOf() const { return &m_vkBuffer; }

    // Resize the underlying VkBuffer without waiting for any pipeline
    // synchronization. The caller is responsible to guarantee the underlying
    // VkBuffer is not queued up in any in-flight command buffers.
    void resizeImmediately(size_t sizeInBytes);

    void* contents()
    {
        assert(m_contents != nullptr);
        return m_contents;
    }

    void flushMappedContents(size_t updatedSizeInBytes);

private:
    friend class Allocator;

    Buffer(rcp<Allocator>, const VkBufferCreateInfo&, Mappability);

    void init();

    const Mappability m_mappability;
    VkBufferCreateInfo m_info;
    VmaAllocation m_vmaAllocation;
    VkBuffer m_vkBuffer;
    void* m_contents;
};

// RAII utility to call flushMappedContents() on a buffer when the class goes out
// of scope.
class ScopedBufferFlush
{
public:
    ScopedBufferFlush(Buffer& buff, size_t mapSizeInBytes = VK_WHOLE_SIZE) :
        m_buff(buff), m_mapSizeInBytes(mapSizeInBytes)
    {}
    ~ScopedBufferFlush() { m_buff.flushMappedContents(m_mapSizeInBytes); }

    operator void*() { return m_buff.contents(); }
    template <typename T> T as() { return reinterpret_cast<T>(m_buff.contents()); }

private:
    Buffer& m_buff;
    const size_t m_mapSizeInBytes;
};

// Wraps a ring of VkBuffers so we can map one while other(s) are in-flight.
class BufferRing
{
public:
    BufferRing(rcp<vkutil::Allocator> allocator,
               VkBufferUsageFlags usage,
               Mappability mappability,
               size_t size = 0) :
        m_targetSize(size)
    {
        VkBufferCreateInfo bufferCreateInfo = {
            .size = size,
            .usage = usage,
        };
        for (int i = 0; i < pls::kBufferRingSize; ++i)
        {
            m_buffers[i] = allocator->makeBuffer(bufferCreateInfo, mappability);
        }
    }

    size_t size() const { return m_targetSize; }

    void setTargetSize(size_t size)
    {
        if (m_buffers[0]->info().usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        {
            // Uniform buffers always get bound, even if unused, so make sure they
            // aren't empty and we get a valid Vulkan handle.
            size = std::max<size_t>(size, 256);
            // Uniform blocks must be multiples of 256 bytes in size.
            assert(size % 256 == 0);
        }
        m_targetSize = size;
    }

    void synchronizeSizeAt(int bufferRingIdx)
    {
        if (m_buffers[bufferRingIdx]->info().size != m_targetSize)
        {
            m_buffers[bufferRingIdx]->resizeImmediately(m_targetSize);
        }
    }

    void* contentsAt(int bufferRingIdx, size_t dirtySize = VK_WHOLE_SIZE)
    {
        m_pendingFlushSize = dirtySize;
        return m_buffers[bufferRingIdx]->contents();
    }

    void flushMappedContentsAt(int bufferRingIdx)
    {
        assert(m_pendingFlushSize > 0);
        m_buffers[bufferRingIdx]->flushMappedContents(m_pendingFlushSize);
        m_pendingFlushSize = 0;
    }

    VkBuffer vkBufferAt(int bufferRingIdx) const { return *m_buffers[bufferRingIdx]; }

    const VkBuffer* vkBufferAtAddressOf(int bufferRingIdx) const
    {
        return m_buffers[bufferRingIdx]->vkBufferAddressOf();
    }

private:
    size_t m_targetSize;
    size_t m_pendingFlushSize = 0;
    rcp<vkutil::Buffer> m_buffers[pls::kBufferRingSize];
};

class Texture : public RenderingResource
{
public:
    ~Texture() override;

    const VkImageCreateInfo& info() { return m_info; }
    operator VkImage() const { return m_vkImage; }
    const VkImage* vkImageAddressOf() const { return &m_vkImage; }

private:
    friend class Allocator;

    Texture(rcp<Allocator>, const VkImageCreateInfo&);

    VkImageCreateInfo m_info;
    VmaAllocation m_vmaAllocation;
    VkImage m_vkImage;
};

class TextureView : public RenderingResource
{
public:
    ~TextureView() override;

    const VkImageViewCreateInfo& info() { return m_info; }
    operator VkImageView() const { return m_vkImageView; }
    const VkImageView* vkImageViewAddressOf() const { return &m_vkImageView; }

private:
    friend class Allocator;

    TextureView(rcp<Allocator>, rcp<Texture> textureRefOrNull, const VkImageViewCreateInfo&);

    const rcp<Texture> m_textureRefOrNull;
    VkImageViewCreateInfo m_info;
    VkImageView m_vkImageView;
};

class Framebuffer : public RenderingResource
{
public:
    ~Framebuffer() override;

    const VkFramebufferCreateInfo& info() const { return m_info; }
    operator VkFramebuffer() const { return m_vkFramebuffer; }

private:
    friend class Allocator;

    Framebuffer(rcp<Allocator>, const VkFramebufferCreateInfo&);

    VkFramebufferCreateInfo m_info;
    VkFramebuffer m_vkFramebuffer;
};

// Utility to generate a simple 2D VkViewport from a VkRect2D.
class ViewportFromRect2D
{
public:
    ViewportFromRect2D(const VkRect2D rect) :
        m_viewport{
            .x = static_cast<float>(rect.offset.x),
            .y = static_cast<float>(rect.offset.y),
            .width = static_cast<float>(rect.extent.width),
            .height = static_cast<float>(rect.extent.height),
            .minDepth = 0,
            .maxDepth = 1,
        }
    {}

    operator const VkViewport*() const { return &m_viewport; }

private:
    VkViewport m_viewport;
};

void update_image_descriptor_sets(VkDevice,
                                  VkDescriptorSet,
                                  VkWriteDescriptorSet,
                                  std::initializer_list<VkDescriptorImageInfo>);

void update_buffer_descriptor_sets(VkDevice,
                                   VkDescriptorSet,
                                   VkWriteDescriptorSet,
                                   std::initializer_list<VkDescriptorBufferInfo>);

void insert_image_memory_barrier(VkCommandBuffer,
                                 VkImage,
                                 VkImageLayout oldLayout,
                                 VkImageLayout newLayout,
                                 uint32_t mipLevel = 0,
                                 uint32_t levelCount = 1);

void insert_buffer_memory_barrier(VkCommandBuffer,
                                  VkAccessFlags srcAccessMask,
                                  VkAccessFlags dstAccessMask,
                                  VkBuffer,
                                  VkDeviceSize offset = 0,
                                  VkDeviceSize size = VK_WHOLE_SIZE);
} // namespace rive::pls::vkutil
