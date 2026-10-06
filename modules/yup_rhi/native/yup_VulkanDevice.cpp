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

#if YUP_RIVE_USE_VULKAN

namespace yup
{

namespace
{

//==============================================================================

constexpr uint32_t vulkanVendorImagination = 0x1010;
constexpr uint32_t vulkanMaximumApiVersion = VK_API_VERSION_1_3;
constexpr const char* vulkanValidationLayerName = "VK_LAYER_KHRONOS_validation";
constexpr const char* vulkanPortabilitySubsetExtensionName = "VK_KHR_portability_subset";

bool hasVulkanExtension (const std::vector<VkExtensionProperties>& extensions, const char* name)
{
    return std::any_of (extensions.begin(), extensions.end(), [name] (const auto& e)
                        {
        return std::strcmp (e.extensionName, name) == 0;
    });
}

/** Rive needs Android 10 for its Vulkan renderer, whatever the driver reports. Mirrors
    the check in RenderContextVulkanImpl::MakeContext, without its log line. */
bool isVulkanPlatformVersionSupported()
{
#if YUP_ANDROID
#if __ANDROID_API__ >= 29
    const int deviceApiLevel = android_get_device_api_level();
#else
    char propertyValue[PROP_VALUE_MAX] {};
    __system_property_get ("ro.build.version.sdk", propertyValue);
    const int deviceApiLevel = atoi (propertyValue);
#endif

    return deviceApiLevel >= 29;
#else
    return true;
#endif
}

VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugMessage (VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                   VkDebugUtilsMessageTypeFlagsEXT,
                                                   const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                   void*)
{
    if (data != nullptr && data->pMessage != nullptr)
    {
        const char* level = (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0 ? "error" : "warning";
        Logger::outputDebugString (String ("Vulkan ") + level + ": " + data->pMessage);
    }

    return VK_FALSE;
}

/** The physical device picked for a set of options. */
struct VulkanDeviceCandidate
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;
    uint32_t apiVersion = 0;
    std::vector<VkExtensionProperties> extensions;
};

} // namespace

//==============================================================================

bool VulkanDevice::isSupported (const Options& options)
{
    // Without a loader there is nothing to probe, and nothing worth caching either
    if (options.getInstanceProcAddr == nullptr)
        return false;

    static std::once_flag probed;
    static bool supported = false;

    std::call_once (probed, [&]
                    {
        if (! isVulkanPlatformVersionSupported())
            return;

        // Creating the instance and picking a physical device runs every check the real
        // creation does, the logical device is left out
        auto probeOptions = options;
        probeOptions.enableValidation = false;
        supported = createDevice (probeOptions, true) != nullptr;
    });

    return supported;
}

//==============================================================================

std::unique_ptr<VulkanDevice> VulkanDevice::create (const Options& options)
{
    return createDevice (options, false);
}

std::unique_ptr<VulkanDevice> VulkanDevice::createDevice (const Options& options, bool physicalDeviceOnly)
{
    auto gipa = options.getInstanceProcAddr;
    if (gipa == nullptr || ! isVulkanPlatformVersionSupported())
        return nullptr;

    const auto enumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion> (gipa (VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
    const auto enumerateInstanceExtensions = reinterpret_cast<PFN_vkEnumerateInstanceExtensionProperties> (gipa (VK_NULL_HANDLE, "vkEnumerateInstanceExtensionProperties"));
    const auto enumerateInstanceLayers = reinterpret_cast<PFN_vkEnumerateInstanceLayerProperties> (gipa (VK_NULL_HANDLE, "vkEnumerateInstanceLayerProperties"));
    const auto createInstance = reinterpret_cast<PFN_vkCreateInstance> (gipa (VK_NULL_HANDLE, "vkCreateInstance"));

    // A loader without vkEnumerateInstanceVersion only knows Vulkan 1.0
    if (enumerateInstanceVersion == nullptr || enumerateInstanceExtensions == nullptr || createInstance == nullptr)
        return nullptr;

    uint32_t loaderVersion = VK_API_VERSION_1_0;
    if (enumerateInstanceVersion (&loaderVersion) != VK_SUCCESS || loaderVersion < VK_API_VERSION_1_1)
        return nullptr;

    //==============================================================================
    // Instance

    uint32_t count = 0;
    enumerateInstanceExtensions (nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> instanceExtensions (count);
    enumerateInstanceExtensions (nullptr, &count, instanceExtensions.data());

    std::vector<std::string> extensionNames;
    for (const auto& name : options.instanceExtensions)
    {
        if (! hasVulkanExtension (instanceExtensions, name.toRawUTF8()))
        {
            Logger::outputDebugString ("Vulkan: missing instance extension " + name);
            return nullptr;
        }

        extensionNames.push_back (name.toStdString());
    }

    VkInstanceCreateFlags instanceFlags = 0;
    if (hasVulkanExtension (instanceExtensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME))
    {
        // A no-op on conformant drivers, lists MoltenVK on Apple platforms
        extensionNames.push_back (VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instanceFlags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    }

    std::vector<const char*> layerNames;
    bool debugUtilsEnabled = false;
    if (options.enableValidation && enumerateInstanceLayers != nullptr)
    {
        enumerateInstanceLayers (&count, nullptr);
        std::vector<VkLayerProperties> layers (count);
        enumerateInstanceLayers (&count, layers.data());

        const bool hasValidationLayer = std::any_of (layers.begin(), layers.end(), [] (const auto& layer)
                                                     {
            return std::strcmp (layer.layerName, vulkanValidationLayerName) == 0;
        });

        if (hasValidationLayer)
            layerNames.push_back (vulkanValidationLayerName);

        if (hasVulkanExtension (instanceExtensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
        {
            extensionNames.push_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            debugUtilsEnabled = true;
        }
    }

    std::vector<const char*> extensionPointers;
    for (const auto& name : extensionNames)
        extensionPointers.push_back (name.c_str());

    const uint32_t instanceApiVersion = jmin (loaderVersion, vulkanMaximumApiVersion);

    VkApplicationInfo applicationInfo {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "YUP";
    applicationInfo.pEngineName = "YUP";
    applicationInfo.apiVersion = instanceApiVersion;

    VkInstanceCreateInfo instanceInfo {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.flags = instanceFlags;
    instanceInfo.pApplicationInfo = &applicationInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t> (extensionPointers.size());
    instanceInfo.ppEnabledExtensionNames = extensionPointers.data();
    instanceInfo.enabledLayerCount = static_cast<uint32_t> (layerNames.size());
    instanceInfo.ppEnabledLayerNames = layerNames.data();

    auto result = std::unique_ptr<VulkanDevice> (new VulkanDevice());
    result->getInstanceProcAddrFn = gipa;

    if (createInstance (&instanceInfo, nullptr, &result->instance) != VK_SUCCESS)
    {
        Logger::outputDebugString ("Vulkan: unable to create the instance");
        return nullptr;
    }

    const auto instance = result->instance;
    auto& fn = result->functions;

#define YUP_VULKAN_LOAD_INSTANCE_FUNCTION(name) fn.name = reinterpret_cast<PFN_vk##name> (gipa (instance, "vk" #name));
    YUP_VULKAN_INSTANCE_FUNCTIONS (YUP_VULKAN_LOAD_INSTANCE_FUNCTION)
#undef YUP_VULKAN_LOAD_INSTANCE_FUNCTION

    if (debugUtilsEnabled)
    {
        const auto createMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT> (gipa (instance, "vkCreateDebugUtilsMessengerEXT"));

        VkDebugUtilsMessengerCreateInfoEXT messengerInfo {};
        messengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        messengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        messengerInfo.pfnUserCallback = vulkanDebugMessage;

        if (createMessenger != nullptr)
            createMessenger (instance, &messengerInfo, nullptr, &result->debugMessenger);
    }

    //==============================================================================
    // Physical device

    const auto enumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices> (gipa (instance, "vkEnumeratePhysicalDevices"));
    const auto getProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties> (gipa (instance, "vkGetPhysicalDeviceProperties"));
    const auto getQueueFamilies = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties> (gipa (instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    const auto enumerateDeviceExtensions = reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties> (gipa (instance, "vkEnumerateDeviceExtensionProperties"));
    const auto getFeatures2 = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2> (gipa (instance, "vkGetPhysicalDeviceFeatures2"));
    const auto createDevice = reinterpret_cast<PFN_vkCreateDevice> (gipa (instance, "vkCreateDevice"));
    const auto getDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr> (gipa (instance, "vkGetDeviceProcAddr"));

    if (enumeratePhysicalDevices == nullptr || getProperties == nullptr || getQueueFamilies == nullptr
        || enumerateDeviceExtensions == nullptr || getFeatures2 == nullptr || createDevice == nullptr || getDeviceProcAddr == nullptr)
    {
        return nullptr;
    }

    count = 0;
    enumeratePhysicalDevices (instance, &count, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices (count);
    enumeratePhysicalDevices (instance, &count, physicalDevices.data());

    const bool wantsPresentation = static_cast<bool> (options.presentationSupport);

    std::optional<VulkanDeviceCandidate> chosen;
    int chosenScore = -1;

    for (auto physicalDevice : physicalDevices)
    {
        VkPhysicalDeviceProperties properties {};
        getProperties (physicalDevice, &properties);

        // Rive's own requirements, checked again by RenderContextVulkanImpl::MakeContext
        if (properties.apiVersion < VK_API_VERSION_1_1)
            continue;

        if (properties.vendorID == vulkanVendorImagination && properties.apiVersion < VK_API_VERSION_1_3)
            continue;

        enumerateDeviceExtensions (physicalDevice, nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> deviceExtensions (count);
        enumerateDeviceExtensions (physicalDevice, nullptr, &count, deviceExtensions.data());

        if (wantsPresentation && ! hasVulkanExtension (deviceExtensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
            continue;

        getQueueFamilies (physicalDevice, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families (count);
        getQueueFamilies (physicalDevice, &count, families.data());

        std::optional<uint32_t> family;
        for (uint32_t i = 0; i < count && ! family.has_value(); ++i)
        {
            constexpr VkQueueFlags wanted = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
            if ((families[i].queueFlags & wanted) != wanted)
                continue;

            if (wantsPresentation && ! options.presentationSupport (instance, physicalDevice, i))
                continue;

            family = i;
        }

        if (! family.has_value())
            continue;

        int score = 0;
        switch (properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                score = 3;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                score = 2;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                score = 1;
                break;
            default:
                break;
        }

        if (score > chosenScore)
        {
            chosenScore = score;
            chosen = VulkanDeviceCandidate { physicalDevice, *family, jmin (properties.apiVersion, instanceApiVersion), std::move (deviceExtensions) };
        }
    }

    if (! chosen.has_value())
    {
        Logger::outputDebugString ("Vulkan: no physical device meets the renderer requirements");
        return nullptr;
    }

    result->physicalDevice = chosen->physicalDevice;
    result->queueFamilyIndex = chosen->queueFamilyIndex;

    if (physicalDeviceOnly)
        return result;

    //==============================================================================
    // Features and extensions

    const auto& available = chosen->extensions;
    std::vector<const char*> deviceExtensions;

    VkPhysicalDeviceFeatures2 supported {};
    supported.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT rasterOrder {};
    rasterOrder.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_EXT;

    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT interlock {};
    interlock.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT;

    VkPhysicalDeviceColorWriteEnableFeaturesEXT colorWrite {};
    colorWrite.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COLOR_WRITE_ENABLE_FEATURES_EXT;

    void** nextFeature = &supported.pNext;
    auto chainFeature = [&nextFeature] (auto& feature)
    {
        *nextFeature = &feature;
        nextFeature = &feature.pNext;
    };

    // Some Mali drivers only expose the ARM flavour, which shares the EXT structures and flags
    const char* rasterOrderExtension = hasVulkanExtension (available, VK_EXT_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_EXTENSION_NAME)
                                         ? VK_EXT_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_EXTENSION_NAME
                                     : hasVulkanExtension (available, VK_ARM_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_EXTENSION_NAME)
                                         ? VK_ARM_RASTERIZATION_ORDER_ATTACHMENT_ACCESS_EXTENSION_NAME
                                         : nullptr;

    const bool hasInterlock = hasVulkanExtension (available, VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME);
    const bool hasColorWrite = hasVulkanExtension (available, VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME);

    if (rasterOrderExtension != nullptr)
        chainFeature (rasterOrder);

    if (hasInterlock)
        chainFeature (interlock);

    if (hasColorWrite)
        chainFeature (colorWrite);

    getFeatures2 (chosen->physicalDevice, &supported);

    const auto& core = supported.features;
    auto& features = result->features;
    features.apiVersion = chosen->apiVersion;
    features.independentBlend = core.independentBlend == VK_TRUE;
    features.fillModeNonSolid = core.fillModeNonSolid == VK_TRUE;
    features.fragmentStoresAndAtomics = core.fragmentStoresAndAtomics == VK_TRUE;
    features.shaderClipDistance = core.shaderClipDistance == VK_TRUE;
    features.samplerAnisotropy = core.samplerAnisotropy == VK_TRUE;
    features.depthBiasClamp = core.depthBiasClamp == VK_TRUE;
    features.textureCompressionBC = core.textureCompressionBC == VK_TRUE;
    features.textureCompressionASTC_LDR = core.textureCompressionASTC_LDR == VK_TRUE;
    features.textureCompressionETC2 = core.textureCompressionETC2 == VK_TRUE;
    features.rasterizationOrderColorAttachmentAccess = rasterOrderExtension != nullptr && rasterOrder.rasterizationOrderColorAttachmentAccess == VK_TRUE;
    features.fragmentShaderPixelInterlock = hasInterlock && interlock.fragmentShaderPixelInterlock == VK_TRUE;
    features.colorWriteEnable = hasColorWrite && colorWrite.colorWriteEnable == VK_TRUE;

    // Rive builds no pipeline needing the remaining core features, so only these get enabled
    VkPhysicalDeviceFeatures2 enabled {};
    enabled.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    enabled.features.independentBlend = core.independentBlend;
    enabled.features.fillModeNonSolid = core.fillModeNonSolid;
    enabled.features.fragmentStoresAndAtomics = core.fragmentStoresAndAtomics;
    enabled.features.shaderClipDistance = core.shaderClipDistance;
    enabled.features.samplerAnisotropy = core.samplerAnisotropy;
    enabled.features.depthBiasClamp = core.depthBiasClamp;
    enabled.features.textureCompressionBC = core.textureCompressionBC;
    enabled.features.textureCompressionASTC_LDR = core.textureCompressionASTC_LDR;
    enabled.features.textureCompressionETC2 = core.textureCompressionETC2;

    nextFeature = &enabled.pNext;

    if (features.rasterizationOrderColorAttachmentAccess)
    {
        deviceExtensions.push_back (rasterOrderExtension);
        rasterOrder.pNext = nullptr;
        rasterOrder.rasterizationOrderDepthAttachmentAccess = VK_FALSE;
        rasterOrder.rasterizationOrderStencilAttachmentAccess = VK_FALSE;
        chainFeature (rasterOrder);
    }

    if (features.fragmentShaderPixelInterlock)
    {
        deviceExtensions.push_back (VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME);
        interlock.pNext = nullptr;
        interlock.fragmentShaderSampleInterlock = VK_FALSE;
        interlock.fragmentShaderShadingRateInterlock = VK_FALSE;
        chainFeature (interlock);
    }

    if (features.colorWriteEnable)
    {
        deviceExtensions.push_back (VK_EXT_COLOR_WRITE_ENABLE_EXTENSION_NAME);
        colorWrite.pNext = nullptr;
        chainFeature (colorWrite);
    }

    // A non-conformant implementation (MoltenVK) must have this enabled whenever it exposes it
    if (hasVulkanExtension (available, vulkanPortabilitySubsetExtensionName))
    {
        deviceExtensions.push_back (vulkanPortabilitySubsetExtensionName);
        features.VK_KHR_portability_subset = true;
    }

    if (wantsPresentation)
    {
        deviceExtensions.push_back (VK_KHR_SWAPCHAIN_EXTENSION_NAME);
        result->swapchainEnabled = true;
    }

    //==============================================================================
    // Logical device

    const float queuePriority = 1.0f;

    VkDeviceQueueCreateInfo queueInfo {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = chosen->queueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceInfo {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pNext = &enabled;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t> (deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (createDevice (chosen->physicalDevice, &deviceInfo, nullptr, &result->device) != VK_SUCCESS)
    {
        Logger::outputDebugString ("Vulkan: unable to create the logical device");
        return nullptr;
    }

    const auto device = result->device;

#define YUP_VULKAN_LOAD_DEVICE_FUNCTION(name) fn.name = reinterpret_cast<PFN_vk##name> (getDeviceProcAddr (device, "vk" #name));
    YUP_VULKAN_DEVICE_FUNCTIONS (YUP_VULKAN_LOAD_DEVICE_FUNCTION)
#undef YUP_VULKAN_LOAD_DEVICE_FUNCTION

    const auto getDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue> (getDeviceProcAddr (device, "vkGetDeviceQueue"));
    if (getDeviceQueue == nullptr)
        return nullptr;

    getDeviceQueue (device, chosen->queueFamilyIndex, 0, &result->queue);

    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = chosen->queueFamilyIndex;

    if (fn.CreateCommandPool (device, &poolInfo, nullptr, &result->commandPool) != VK_SUCCESS)
        return nullptr;

    return result;
}

//==============================================================================

VulkanDevice::Commands VulkanDevice::beginCommands()
{
    jassert (frameClock != nullptr);
    if (frameClock == nullptr)
        return {};

    const ScopedLock sl (lock);

    const auto generation = frameClock();

    Commands commands;
    commands.commandBuffer = beginCommands (generation);
    commands.frameNumber = generation;
    commands.safeFrameNumber = getSafeFrameNumber (generation);
    return commands;
}

VkCommandBuffer VulkanDevice::beginCommands (uint64_t generation)
{
    const ScopedLock sl (lock);

    reclaimCompletedSlots();

    CommandSlot* slot = nullptr;
    for (const auto& candidate : commandSlots)
    {
        if (candidate->state == CommandSlot::State::idle)
        {
            slot = candidate.get();
            break;
        }
    }

    // Keep a few submissions in flight before waiting on any. Then wait on the oldest
    // submitted command buffer, never on one still recording: that belongs to an open
    // frame further up the stack and would never complete
    if (slot == nullptr && commandSlots.size() >= minimumSlotsInFlight)
    {
        for (const auto& candidate : commandSlots)
        {
            if (candidate->state == CommandSlot::State::submitted && (slot == nullptr || candidate->generation < slot->generation))
                slot = candidate.get();
        }

        if (slot != nullptr)
        {
            functions.WaitForFences (device, 1, &slot->fence, VK_TRUE, UINT64_MAX);
            completeSlot (*slot);
        }
    }

    if (slot == nullptr)
    {
        auto newSlot = std::make_unique<CommandSlot>();

        VkCommandBufferAllocateInfo allocateInfo {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = 1;

        if (functions.AllocateCommandBuffers (device, &allocateInfo, &newSlot->commandBuffer) != VK_SUCCESS)
            return VK_NULL_HANDLE;

        VkFenceCreateInfo fenceInfo {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        if (functions.CreateFence (device, &fenceInfo, nullptr, &newSlot->fence) != VK_SUCCESS)
        {
            functions.FreeCommandBuffers (device, commandPool, 1, &newSlot->commandBuffer);
            return VK_NULL_HANDLE;
        }

        slot = newSlot.get();
        commandSlots.push_back (std::move (newSlot));
    }

    functions.ResetFences (device, 1, &slot->fence);
    functions.ResetCommandBuffer (slot->commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (functions.BeginCommandBuffer (slot->commandBuffer, &beginInfo) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    slot->generation = generation;
    slot->state = CommandSlot::State::recording;

    if (frameObserver != nullptr)
        frameObserver (generation, getSafeFrameNumber (generation));

    return slot->commandBuffer;
}

bool VulkanDevice::submitCommands (VkCommandBuffer commandBuffer,
                                   VkSemaphore waitSemaphore,
                                   VkPipelineStageFlags waitStage,
                                   VkSemaphore signalSemaphore)
{
    const ScopedLock sl (lock);

    auto* slot = findSlot (commandBuffer);
    jassert (slot != nullptr && slot->state == CommandSlot::State::recording);
    if (slot == nullptr || slot->state != CommandSlot::State::recording)
        return false;

    // A command buffer that cannot be submitted completes immediately, so its handlers still run
    slot->state = CommandSlot::State::idle;

    if (functions.EndCommandBuffer (commandBuffer) != VK_SUCCESS)
    {
        completeSlot (*slot);
        return false;
    }

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (waitSemaphore != VK_NULL_HANDLE)
    {
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = &waitStage;
    }

    if (signalSemaphore != VK_NULL_HANDLE)
    {
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;
    }

    if (functions.QueueSubmit (queue, 1, &submitInfo, slot->fence) != VK_SUCCESS)
    {
        Logger::outputDebugString ("Vulkan: queue submission failed");
        completeSlot (*slot);
        return false;
    }

    slot->state = CommandSlot::State::submitted;
    return true;
}

void VulkanDevice::waitForCommands (VkCommandBuffer commandBuffer)
{
    const ScopedLock sl (lock);

    auto* slot = findSlot (commandBuffer);
    if (slot == nullptr || slot->state != CommandSlot::State::submitted)
        return;

    functions.WaitForFences (device, 1, &slot->fence, VK_TRUE, UINT64_MAX);
    completeSlot (*slot);
}

void VulkanDevice::runWhenComplete (VkCommandBuffer commandBuffer, std::function<void()> fn)
{
    const ScopedLock sl (lock);

    auto* slot = findSlot (commandBuffer);
    if (slot == nullptr || slot->state == CommandSlot::State::idle)
    {
        fn();
        return;
    }

    slot->completionHandlers.push_back (std::move (fn));
}

uint64_t VulkanDevice::getSafeFrameNumber (uint64_t currentGeneration) const
{
    const ScopedLock sl (lock);

    reclaimCompletedSlots();

    // Everything before the oldest generation still recording or executing is done
    auto safe = currentGeneration > 0 ? currentGeneration - 1 : 0;

    for (const auto& slot : commandSlots)
    {
        if (slot->state != CommandSlot::State::idle && slot->generation > 0)
            safe = jmin (safe, slot->generation - 1);
    }

    lastSafeFrameNumber = jmax (lastSafeFrameNumber, safe);
    return lastSafeFrameNumber;
}

void VulkanDevice::waitIdle()
{
    const ScopedLock sl (lock);

    functions.QueueWaitIdle (queue);

    // Command buffers still recording belong to open frames and stay theirs
    for (const auto& slot : commandSlots)
    {
        if (slot->state == CommandSlot::State::submitted)
            completeSlot (*slot);
    }
}

//==============================================================================

VulkanDevice::CommandSlot* VulkanDevice::findSlot (VkCommandBuffer commandBuffer) const
{
    for (const auto& slot : commandSlots)
    {
        if (slot->commandBuffer == commandBuffer)
            return slot.get();
    }

    return nullptr;
}

void VulkanDevice::reclaimCompletedSlots() const
{
    for (const auto& slot : commandSlots)
    {
        if (slot->state == CommandSlot::State::submitted && functions.GetFenceStatus (device, slot->fence) == VK_SUCCESS)
            completeSlot (*slot);
    }
}

void VulkanDevice::completeSlot (CommandSlot& slot)
{
    slot.state = CommandSlot::State::idle;

    auto handlers = std::move (slot.completionHandlers);
    slot.completionHandlers.clear();

    for (auto& handler : handlers)
        handler();
}

//==============================================================================

VulkanDevice::~VulkanDevice()
{
    if (device != VK_NULL_HANDLE && functions.DestroyDevice != nullptr)
    {
        functions.DeviceWaitIdle (device);

        for (const auto& slot : commandSlots)
        {
            completeSlot (*slot);
            functions.DestroyFence (device, slot->fence, nullptr);
        }

        commandSlots.clear();

        // Destroying the pool frees its command buffers
        if (commandPool != VK_NULL_HANDLE)
            functions.DestroyCommandPool (device, commandPool, nullptr);

        functions.DestroyDevice (device, nullptr);
    }

    if (debugMessenger != VK_NULL_HANDLE && functions.DestroyDebugUtilsMessengerEXT != nullptr)
        functions.DestroyDebugUtilsMessengerEXT (instance, debugMessenger, nullptr);

    if (instance != VK_NULL_HANDLE && functions.DestroyInstance != nullptr)
        functions.DestroyInstance (instance, nullptr);
}

} // namespace yup

#endif // YUP_RIVE_USE_VULKAN
