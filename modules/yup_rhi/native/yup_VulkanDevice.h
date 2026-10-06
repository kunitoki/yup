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

#pragma once

#if YUP_RIVE_USE_VULKAN

namespace yup
{

//==============================================================================
/** Owns a Vulkan instance, the chosen physical device, a logical device and its
    graphics queue.

    Platform-neutral by construction: the windowing layer hands in the loader
    entry point, the instance extensions its surfaces need and a presentation
    probe, so nothing here touches a window system. Every feature and extension
    the Rive renderer can use is enabled when the device exposes it, and reported
    through getFeatures().

    Every submission goes through a ring of command buffers, each with a fence:
    Rive flushes, GpuFrame command buffers, offscreen readbacks and compute passes.
    A submission takes a frame generation from the frame clock, the one monotonic
    counter Rive and ore share, and the safe generation reported to both is derived
    from the fences rather than assumed.

    One recursive lock guards the queue and every VMA allocator, which Rive creates
    externally synchronized.

    Internal to the Vulkan backend. GpuDevice::getNativeDevice() returns it, which is
    how the Vulkan GraphicsContext reaches the device.
*/
class VulkanDevice
{
public:
    //==============================================================================
    /** What the windowing layer provides to create a device. */
    struct Options
    {
        /** The loader entry point. Required. */
        PFN_vkGetInstanceProcAddr getInstanceProcAddr = nullptr;

        /** Instance extensions the window surfaces need, empty for a headless device. */
        StringArray instanceExtensions;

        /** Tells whether a queue family can present to the window system, or null for a headless device. */
        std::function<bool (VkInstance, VkPhysicalDevice, uint32_t)> presentationSupport;

        /** Enables the Khronos validation layer and its messenger when the layer is installed. */
        bool enableValidation = false;
    };

    //==============================================================================
    /** Returns true if a device meeting the Rive renderer's requirements exists.

        Cheap after the first call: the result is cached for the process. Used to
        pick a graphics API before any window is created.
    */
    static bool isSupported (const Options& options);

    /** Creates the device, or returns nullptr when no physical device qualifies. */
    static std::unique_ptr<VulkanDevice> create (const Options& options);

    /** Destroys the logical device and the instance. */
    ~VulkanDevice();

    //==============================================================================
    VkInstance getInstance() const noexcept { return instance; }

    VkPhysicalDevice getPhysicalDevice() const noexcept { return physicalDevice; }

    VkDevice getDevice() const noexcept { return device; }

    VkQueue getQueue() const noexcept { return queue; }

    uint32_t getQueueFamilyIndex() const noexcept { return queueFamilyIndex; }

    PFN_vkGetInstanceProcAddr getInstanceProcAddr() const noexcept { return getInstanceProcAddrFn; }

    /** The API version and the features the device was created with. */
    const rive::gpu::VulkanFeatures& getFeatures() const noexcept { return features; }

    /** True when VK_KHR_swapchain is enabled. */
    bool canPresent() const noexcept { return swapchainEnabled; }

    //==============================================================================
    /** A ring command buffer in the recording state, with its frame generation. */
    struct Commands
    {
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        uint64_t frameNumber = 0;     ///< The generation the recorded work belongs to.
        uint64_t safeFrameNumber = 0; ///< The newest generation the GPU has finished.

        bool isValid() const noexcept { return commandBuffer != VK_NULL_HANDLE; }
    };

    /** The lock serializing queue access and VMA allocations. Recursive. */
    CriticalSection& getLock() const noexcept { return lock; }

    /** Installs the function handing out new frame generations. */
    void setFrameClock (std::function<uint64_t()> newFrameClock) { frameClock = std::move (newFrameClock); }

    /** Installs the function told about every command buffer handed out, with its
        generation and the safe generation at that point. Called with getLock() held. */
    void setFrameObserver (std::function<void (uint64_t, uint64_t)> newFrameObserver) { frameObserver = std::move (newFrameObserver); }

    /** Takes a new frame generation from the frame clock and a ring command buffer.

        Must be called with getLock() held. Never waits on a command buffer that was
        not submitted, so any number of frames may be open at once.

        @returns The command buffer, invalid only if Vulkan could not provide one.
    */
    Commands beginCommands();

    /** Takes a ring command buffer for a generation the caller already took.

        Must be called with getLock() held.

        @returns The command buffer in the recording state, or VK_NULL_HANDLE.
    */
    VkCommandBuffer beginCommands (uint64_t generation);

    /** Ends and submits a ring command buffer.

        Must be called with getLock() held.

        @param commandBuffer   A command buffer from beginCommands().
        @param waitSemaphore   An optional semaphore the work waits on.
        @param waitStage       The stage that waits on @p waitSemaphore.
        @param signalSemaphore An optional semaphore signalled when the work completes.

        @returns True if the queue accepted the work.
    */
    bool submitCommands (VkCommandBuffer commandBuffer,
                         VkSemaphore waitSemaphore = VK_NULL_HANDLE,
                         VkPipelineStageFlags waitStage = 0,
                         VkSemaphore signalSemaphore = VK_NULL_HANDLE);

    /** Blocks until the GPU finished a submitted command buffer. Returns at once for
        a command buffer that is still recording. */
    void waitForCommands (VkCommandBuffer commandBuffer);

    /** Runs @p fn once the GPU finished the work recorded in @p commandBuffer, to
        release what its commands still reference. Must be called with getLock() held. */
    void runWhenComplete (VkCommandBuffer commandBuffer, std::function<void()> fn);

    /** Returns the newest generation whose work the GPU finished, never decreasing.

        @param currentGeneration The latest generation handed out by the frame clock.
    */
    uint64_t getSafeFrameNumber (uint64_t currentGeneration) const;

    /** Waits for the queue to drain and runs every pending completion handler. */
    void waitIdle();

    //==============================================================================
    /** The entry points YUP calls itself. Rive's VulkanContext loads its own. */
#define YUP_VULKAN_INSTANCE_FUNCTIONS(X)    \
X (DestroyInstance)                         \
X (GetPhysicalDeviceFormatProperties)       \
X (GetPhysicalDeviceSurfaceCapabilitiesKHR) \
X (GetPhysicalDeviceSurfaceFormatsKHR)      \
X (GetPhysicalDeviceSurfacePresentModesKHR) \
X (GetPhysicalDeviceSurfaceSupportKHR)      \
X (DestroyDebugUtilsMessengerEXT)

#define YUP_VULKAN_DEVICE_FUNCTIONS(X) \
X (DestroyDevice)                      \
X (DeviceWaitIdle)                     \
X (QueueWaitIdle)                      \
X (QueueSubmit)                        \
X (QueuePresentKHR)                    \
X (CreateSwapchainKHR)                 \
X (DestroySwapchainKHR)                \
X (GetSwapchainImagesKHR)              \
X (AcquireNextImageKHR)                \
X (CreateSemaphore)                    \
X (DestroySemaphore)                   \
X (CreateFence)                        \
X (DestroyFence)                       \
X (WaitForFences)                      \
X (ResetFences)                        \
X (GetFenceStatus)                     \
X (CreateCommandPool)                  \
X (DestroyCommandPool)                 \
X (AllocateCommandBuffers)             \
X (FreeCommandBuffers)                 \
X (ResetCommandBuffer)                 \
X (BeginCommandBuffer)                 \
X (EndCommandBuffer)                   \
X (CmdPipelineBarrier)                 \
X (CmdBlitImage)                       \
X (CmdCopyImageToBuffer)               \
X (CmdClearColorImage)                 \
X (CmdBindPipeline)                    \
X (CmdBindDescriptorSets)              \
X (CmdDispatch)                        \
X (CreateShaderModule)                 \
X (DestroyShaderModule)                \
X (CreateDescriptorSetLayout)          \
X (DestroyDescriptorSetLayout)         \
X (CreatePipelineLayout)               \
X (DestroyPipelineLayout)              \
X (CreateComputePipelines)             \
X (DestroyPipeline)                    \
X (CreateDescriptorPool)               \
X (DestroyDescriptorPool)              \
X (AllocateDescriptorSets)             \
X (UpdateDescriptorSets)

    struct Functions
    {
#define YUP_VULKAN_DECLARE_FUNCTION(name) PFN_vk##name name = nullptr;
        YUP_VULKAN_INSTANCE_FUNCTIONS (YUP_VULKAN_DECLARE_FUNCTION)
        YUP_VULKAN_DEVICE_FUNCTIONS (YUP_VULKAN_DECLARE_FUNCTION)
#undef YUP_VULKAN_DECLARE_FUNCTION
    };

    /** Returns the loaded entry points. */
    const Functions& getFunctions() const noexcept { return functions; }

private:
    struct CommandSlot
    {
        enum class State
        {
            idle,
            recording,
            submitted
        };

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        uint64_t generation = 0;
        State state = State::idle;
        std::vector<std::function<void()>> completionHandlers;
    };

    VulkanDevice() = default;

    static std::unique_ptr<VulkanDevice> createDevice (const Options& options, bool physicalDeviceOnly);

    /** Submissions the ring keeps in flight before waiting on the oldest one. */
    static constexpr size_t minimumSlotsInFlight = 3;

    CommandSlot* findSlot (VkCommandBuffer commandBuffer) const;
    void reclaimCompletedSlots() const;
    static void completeSlot (CommandSlot& slot);

    PFN_vkGetInstanceProcAddr getInstanceProcAddrFn = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;
    bool swapchainEnabled = false;
    rive::gpu::VulkanFeatures features;
    Functions functions;

    mutable CriticalSection lock;
    std::function<uint64_t()> frameClock;
    std::function<void (uint64_t, uint64_t)> frameObserver;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    mutable std::vector<std::unique_ptr<CommandSlot>> commandSlots;
    mutable uint64_t lastSafeFrameNumber = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VulkanDevice)
};

} // namespace yup

#endif // YUP_RIVE_USE_VULKAN
