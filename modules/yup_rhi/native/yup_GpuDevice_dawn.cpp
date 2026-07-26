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

#if RIVE_DAWN
#include "dawn/native/DawnNative.h"
#include "dawn/dawn_proc.h"

#include "rive/pls/pls_factory.hpp"
#include "rive/pls/pls_renderer.hpp"
#include "rive/pls/webgpu/pls_render_context_webgpu_impl.hpp"

#include <cstdio>
#include <memory>
#include <vector>

namespace yup
{

static void print_device_error (WGPUErrorType errorType, const char* message, void*)
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
            return;
    }
    printf ("%s error: %s\n", errorTypeName, message);
}

static void device_lost_callback (WGPUDeviceLostReason reason, const char* message, void*)
{
    printf ("device lost: %s\n", message);
}

class GpuDeviceDawn : public GpuDevice
{
public:
    GpuDeviceDawn (Options options)
        : m_options (options)
    {
        WGPUInstanceDescriptor instanceDescriptor {};
        instanceDescriptor.features.timedWaitAnyEnable = true;
        m_instance = std::make_unique<dawn::native::Instance> (&instanceDescriptor);

        wgpu::RequestAdapterOptions adapterOptions = {
            .powerPreference = wgpu::PowerPreference::HighPerformance,
        };

        auto adapters = m_instance->EnumerateAdapters (&adapterOptions);

        wgpu::DawnAdapterPropertiesPowerPreference power_props {};
        wgpu::AdapterProperties adapterProperties {};
        adapterProperties.nextInChain = &power_props;

        auto isAdapterType = [&adapterProperties] (const auto& adapter) -> bool
        {
            adapter.GetProperties (&adapterProperties);
            return adapterProperties.adapterType == wgpu::AdapterType::DiscreteGPU;
        };

        auto preferredAdapter = std::find_if (adapters.begin(), adapters.end(), isAdapterType);
        if (preferredAdapter == adapters.end())
        {
            fprintf (stderr, "Failed to find an adapter!\n");
            return;
        }

        std::vector<const char*> enableToggleNames = {
            "allow_unsafe_apis",
            "turn_off_vsync",
        };

        WGPUDawnTogglesDescriptor toggles = {
            .chain = { .next = nullptr, .sType = WGPUSType_DawnTogglesDescriptor },
            .enabledToggleCount = enableToggleNames.size(),
            .enabledToggles = enableToggleNames.data(),
            .disabledToggleCount = 0,
            .disabledToggles = nullptr,
        };

        std::vector<WGPUFeatureName> requiredFeatures = {
            WGPUFeatureName_SurfaceCapabilities,
        };

        WGPUDeviceDescriptor deviceDesc = {
            .nextInChain = reinterpret_cast<WGPUChainedStruct*> (&toggles),
            .requiredFeatureCount = requiredFeatures.size(),
            .requiredFeatures = requiredFeatures.data(),
        };

        m_backendDevice = preferredAdapter->CreateDevice (&deviceDesc);

        DawnProcTable backendProcs = dawn::native::GetProcs();
        dawnProcSetProcs (&backendProcs);
        backendProcs.deviceSetUncapturedErrorCallback (m_backendDevice, print_device_error, nullptr);
        backendProcs.deviceSetDeviceLostCallback (m_backendDevice, device_lost_callback, nullptr);

        m_device = wgpu::Device::Acquire (m_backendDevice);
        m_queue = m_device.GetQueue();
    }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::WebGPU; }

    rive::ore::Context* gpuContext() const noexcept override { return nullptr; }

    bool isComputeAvailable() const noexcept override { return true; }

    ReferenceCountedObjectPtr<GpuBuffer> createBuffer (GpuBufferType type, const void* data, size_t byteSize) override
    {
        if (type == GpuBufferType::storage)
        {
            jassert (data != nullptr && byteSize > 0);
            if (data == nullptr || byteSize == 0)
                return nullptr;

            wgpu::BufferDescriptor bufDesc {};
            bufDesc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
            bufDesc.size = byteSize;
            bufDesc.label = "GpuBuffer storage";

            wgpu::Buffer wgpuBuffer = m_device.CreateBuffer (&bufDesc);
            if (wgpuBuffer == nullptr)
                return nullptr;

            m_queue.WriteBuffer (wgpuBuffer, 0, data, byteSize);

            return GpuBuffer::createWithImpl (GpuBuffer::Impl { type, byteSize, {}, std::move (wgpuBuffer) });
        }

        return GpuDevice::createBuffer (type, data, byteSize);
    }

    bool updateBuffer (GpuBuffer::Ptr buffer, const void* data, size_t byteSize) override
    {
        if (buffer == nullptr || data == nullptr || byteSize == 0)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr || impl->webgpuStorageBuffer == nullptr)
            return false;

        if (byteSize > buffer->getSizeInBytes())
            return false;

        m_queue.WriteBuffer (impl->webgpuStorageBuffer, 0, data, byteSize);
        return true;
    }

    // Dawn doesn't support PLS offscreen targets through ore yet.
    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int, int) override { return nullptr; }

    std::unique_ptr<RenderableTarget> createRenderableTarget (int, int) override { return nullptr; }

    void beginOffscreen (OffscreenTarget&, const rive::gpu::RenderContext::FrameDescriptor&) override {}

    void endOffscreen (OffscreenTarget&) override {}

    bool readOffscreenPixels (OffscreenTarget&, void*, size_t) override { return false; }

    /** Returns the native WGPU device for compute operations. */
    WGPUDevice getBackendDevice() const noexcept { return m_backendDevice; }

    /** Returns the wgpu::Device for compute operations. */
    wgpu::Device getDevice() const noexcept { return m_device; }

    /** Returns the wgpu::Queue for compute operations. */
    wgpu::Queue getQueue() const noexcept { return m_queue; }

private:
    Options m_options;
    WGPUDevice m_backendDevice = {};
    wgpu::Device m_device = {};
    wgpu::Queue m_queue = {};
    std::unique_ptr<dawn::native::Instance> m_instance;
};

//==============================================================================

std::unique_ptr<GpuDevice> yup_constructDawnGpuDevice (GpuDevice::Options options)
{
    return std::make_unique<GpuDeviceDawn> (options);
}

} // namespace yup
#endif
