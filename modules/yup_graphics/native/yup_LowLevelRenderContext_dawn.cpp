/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include "yup_LowLevelRenderContext.h"

#if RIVE_DAWN
#include "dawn/native/DawnNative.h"
#include "dawn/dawn_proc.h"

#include "rive/pls/pls_factory.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/webgpu/pls_render_context_webgpu_impl.hpp"

#include <array>
#include <thread>

namespace juce
{

using namespace rive;
using namespace rive::pls;

static void print_device_error(WGPUErrorType errorType, const char* message, void*)
{
    const char* errorTypeName = "";
    switch (errorType)
    {
        case WGPUErrorType_Validation:
            errorTypeName = "Validation";
            break;
        case WGPUErrorType_OutOfMemory:
            errorTypeName = "Out of memory";
            break;
        case WGPUErrorType_Unknown:
            errorTypeName = "Unknown";
            break;
        case WGPUErrorType_DeviceLost:
            errorTypeName = "Device lost";
            break;
        default:
            RIVE_UNREACHABLE();
            return;
    }
    printf("%s error: %s\n", errorTypeName, message);
}

static void device_lost_callback(WGPUDeviceLostReason reason, const char* message, void*)
{
    printf("device lost: %s\n", message);
}

static void device_log_callback(WGPULoggingType type, const char* message, void*)
{
    printf("Device log %s\n", message);
}

#ifdef __APPLE__
extern float GetDawnWindowBackingScaleFactor(GLFWwindow*, bool retina);
extern std::unique_ptr<wgpu::ChainedStruct> SetupDawnWindowAndGetSurfaceDescriptor(GLFWwindow*,
                                                                                   bool retina);
#else

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

static float GetDawnWindowBackingScaleFactor(GLFWwindow*, bool retina) { return 1; }

static std::unique_ptr<wgpu::ChainedStruct> SetupDawnWindowAndGetSurfaceDescriptor(
    GLFWwindow* window,
    bool retina)
{
    std::unique_ptr<wgpu::SurfaceDescriptorFromWindowsHWND> desc =
        std::make_unique<wgpu::SurfaceDescriptorFromWindowsHWND>();
    desc->hwnd = glfwGetWin32Window(window);
    desc->hinstance = GetModuleHandle(nullptr);
    return std::move(desc);
}
#endif

class LowLevelRenderContextDawnPLS : public LowLevelRenderContext
{
public:
    LowLevelRenderContextDawnPLS(Options options) : m_options(options)
    {
        WGPUInstanceDescriptor instanceDescriptor{};
        instanceDescriptor.features.timedWaitAnyEnable = true;
        m_instance = std::make_unique<dawn::native::Instance>(&instanceDescriptor);

        wgpu::RequestAdapterOptions adapterOptions = {
            .powerPreference = wgpu::PowerPreference::HighPerformance,
        };

        // Get an adapter for the backend to use, and create the device.
        auto adapters = m_instance->EnumerateAdapters(&adapterOptions);
        wgpu::DawnAdapterPropertiesPowerPreference power_props{};
        wgpu::AdapterProperties adapterProperties{};
        adapterProperties.nextInChain = &power_props;
        // Find the first adapter which satisfies the adapterType requirement.
        auto isAdapterType = [&adapterProperties](const auto& adapter) -> bool {
            adapter.GetProperties(&adapterProperties);
            return adapterProperties.adapterType == wgpu::AdapterType::DiscreteGPU;
        };
        auto preferredAdapter = std::find_if(adapters.begin(), adapters.end(), isAdapterType);
        if (preferredAdapter == adapters.end())
        {
            fprintf(stderr, "Failed to find an adapter! Please try another adapter type.\n");
            return;
        }

        std::vector<const char*> enableToggleNames = {
            "allow_unsafe_apis",
            "turn_off_vsync",
            // "skip_validation",
        };
        std::vector<const char*> disabledToggleNames;

        WGPUDawnTogglesDescriptor toggles = {
            .chain =
                {
                    .next = nullptr,
                    .sType = WGPUSType_DawnTogglesDescriptor,
                },
            .enabledToggleCount = enableToggleNames.size(),
            .enabledToggles = enableToggleNames.data(),
            .disabledToggleCount = disabledToggleNames.size(),
            .disabledToggles = disabledToggleNames.data(),
        };

        std::vector<WGPUFeatureName> requiredFeatures = {
            // WGPUFeatureName_IndirectFirstInstance,
            // WGPUFeatureName_ShaderF16,
            // WGPUFeatureName_BGRA8UnormStorage,
            // WGPUFeatureName_Float32Filterable,
            // WGPUFeatureName_DawnInternalUsages,
            // WGPUFeatureName_DawnMultiPlanarFormats,
            // WGPUFeatureName_DawnNative,
            // WGPUFeatureName_ImplicitDeviceSynchronization,
            WGPUFeatureName_SurfaceCapabilities,
            // WGPUFeatureName_TransientAttachments,
            // WGPUFeatureName_DualSourceBlending,
            // WGPUFeatureName_Norm16TextureFormats,
            // WGPUFeatureName_HostMappedPointer,
            // WGPUFeatureName_ChromiumExperimentalReadWriteStorageTexture,
        };

        WGPUDeviceDescriptor deviceDesc = {
            .nextInChain = reinterpret_cast<WGPUChainedStruct*>(&toggles),
            .requiredFeatureCount = requiredFeatures.size(),
            .requiredFeatures = requiredFeatures.data(),
        };

        m_backendDevice = preferredAdapter->CreateDevice(&deviceDesc);
        DawnProcTable backendProcs = dawn::native::GetProcs();
        dawnProcSetProcs(&backendProcs);
        backendProcs.deviceSetUncapturedErrorCallback(m_backendDevice, print_device_error, nullptr);
        backendProcs.deviceSetDeviceLostCallback(m_backendDevice, device_lost_callback, nullptr);
        backendProcs.deviceSetLoggingCallback(m_backendDevice, device_log_callback, nullptr);
        m_device = wgpu::Device::Acquire(m_backendDevice);
        m_queue = m_device.GetQueue();
        m_plsContext =
            PLSRenderContextWebGPUImpl::MakeContext(m_device,
                                                    m_queue,
                                                    PLSRenderContextWebGPUImpl::ContextOptions());
    }

    float dpiScale(GLFWwindow* window) const override
    {
        return GetDawnWindowBackingScaleFactor(window, m_options.retinaDisplay);
    }

    Factory* factory() override { return m_plsContext.get(); }

    rive::pls::PLSRenderContext* plsContextOrNull() override { return m_plsContext.get(); }

    void onSizeChanged(GLFWwindow* window, int width, int height, uint32_t sampleCount) override
    {
        DawnProcTable backendProcs = dawn::native::GetProcs();

        // Create the swapchain
        auto surfaceChainedDesc =
            SetupDawnWindowAndGetSurfaceDescriptor(window, m_options.retinaDisplay);
        WGPUSurfaceDescriptor surfaceDesc = {
            .nextInChain = reinterpret_cast<WGPUChainedStruct*>(surfaceChainedDesc.get()),
        };
        WGPUSurface surface = backendProcs.instanceCreateSurface(m_instance->Get(), &surfaceDesc);

        WGPUSwapChainDescriptor swapChainDesc = {
            .usage = WGPUTextureUsage_RenderAttachment,
            .format = WGPUTextureFormat_BGRA8Unorm,
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .presentMode = WGPUPresentMode_Immediate, // No vsync.
        };
        if (m_options.enableReadPixels)
        {
            swapChainDesc.usage |= WGPUTextureUsage_CopySrc;
        }

        WGPUSwapChain backendSwapChain =
            backendProcs.deviceCreateSwapChain(m_backendDevice, surface, &swapChainDesc);
        m_swapchain = wgpu::SwapChain::Acquire(backendSwapChain);

        m_renderTarget = m_plsContext->static_impl_cast<PLSRenderContextWebGPUImpl>()
                             ->makeRenderTarget(wgpu::TextureFormat::BGRA8Unorm, width, height);
        m_pixelReadBuff = {};
    }

    std::unique_ptr<Renderer> makeRenderer(int width, int height) override
    {
        return std::make_unique<PLSRenderer>(m_plsContext.get());
    }

    void begin(PLSRenderContext::FrameDescriptor&& frameDescriptor) override
    {
        assert(m_swapchain.GetCurrentTexture().GetWidth() == m_renderTarget->width());
        assert(m_swapchain.GetCurrentTexture().GetHeight() == m_renderTarget->height());
        m_renderTarget->setTargetTextureView(m_swapchain.GetCurrentTextureView());
        frameDescriptor.renderTarget = m_renderTarget;
        m_plsContext->beginFrame(std::move(frameDescriptor));
    }

    void end(GLFWwindow* window) final
    {
        m_plsContext->flush();

        m_swapchain.Present();
    }

    void tick() override { m_device.Tick(); }

private:
    const LowLevelRenderContext::Options m_options;
    WGPUDevice m_backendDevice = {};
    wgpu::Device m_device = {};
    wgpu::Queue m_queue = {};
    wgpu::SwapChain m_swapchain = {};
    std::unique_ptr<dawn::native::Instance> m_instance;
    std::unique_ptr<PLSRenderContext> m_plsContext;
    rcp<PLSRenderTargetWebGPU> m_renderTarget;
    wgpu::Buffer m_pixelReadBuff;
};

std::unique_ptr<LowLevelRenderContext> LowLevelRenderContext::makeDawnPLS(Options options)
{
    return std::make_unique<LowLevelRenderContextDawnPLS>(options);
}

} // namespace juce

#else

namespace juce {

std::unique_ptr<LowLevelRenderContext> LowLevelRenderContext::makeDawnPLS(Options options)
{
    return nullptr;
}

} // namespace juce

#endif
