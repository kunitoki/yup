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

#if YUP_RIVE_USE_D3D
#include "rive/renderer/rive_renderer.hpp"
#include "rive/renderer/d3d11/render_context_d3d_impl.hpp"
#include "rive/renderer/d3d11/d3d11.hpp"
#include <dxgi1_2.h>
#include <dxgi1_3.h>

namespace yup
{

ID3D11Device* yup_getDirect3DDevice (GpuDevice&);
ID3D11DeviceContext* yup_getDirect3DDeviceContext (GpuDevice&);

//==============================================================================

class GraphicsContextD3D : public GraphicsContext
{
public:
    GraphicsContextD3D (ComPtr<IDXGIFactory2> d3dFactory,
                        ComPtr<ID3D11Device> device,
                        ComPtr<ID3D11DeviceContext> deviceContext,
                        bool isHeadless,
                        Options options,
                        GpuDevice::Ptr gpuDevice)
        : isHeadless (isHeadless)
        , options (options)
        , gpuDevice (std::move (gpuDevice))
        , d3dFactory (std::move (d3dFactory))
        , device (std::move (device))
        , deviceContext (std::move (deviceContext))
    {
        QueryPerformanceFrequency (&qpcFrequency);
    }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Direct3D; }

    GpuDevice::Ptr getGpuDevice() const noexcept override { return gpuDevice; }

    rive::Factory* getFactory() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderContext* getRenderContext() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderTarget* getRenderTarget() override { return renderTarget.get(); }

    FrameTimingCapabilities getFrameTimingCapabilities() const noexcept override
    {
        const CriticalSection::ScopedLockType sl (frameTimingLock);
        return frameTimingCapabilities;
    }

    FrameTimingInfo getLastFrameTimingInfo() const noexcept override
    {
        const CriticalSection::ScopedLockType sl (frameTimingLock);
        return lastFrameTimingInfo;
    }

    bool waitForFrameLatency (uint32_t timeoutMilliseconds) override
    {
        if (frameLatencyWaitableObject == nullptr)
            return false;

        return WaitForSingleObjectEx (frameLatencyWaitableObject, timeoutMilliseconds, FALSE) == WAIT_OBJECT_0;
    }

    void setMaximumFramesInFlight (std::optional<uint32_t> newMaximumFramesInFlight) override
    {
        maximumFramesInFlight = newMaximumFramesInFlight;
        applyMaximumFrameLatency();
    }

    void onSizeChanged (void* window, int width, int height, float dpiScale, uint32_t sampleCount) override
    {
        if (! isHeadless)
        {
            swapchain.Reset();
            swapchain2.Reset();
            frameLatencyWaitableObject = nullptr;
            cachedBackbuffer.Reset();
            DXGI_SWAP_CHAIN_DESC1 scd {};
            scd.Width = width;
            scd.Height = height;
            scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            scd.SampleDesc.Count = 1;
            scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
            scd.BufferCount = 2;
            scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
            scd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

            if ((GetWindowLongPtrW ((HWND) window, GWL_EXSTYLE) & WS_EX_NOREDIRECTIONBITMAP) != 0)
                scd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

            auto hr = d3dFactory->CreateSwapChainForHwnd (device.Get(),
                                                          (HWND) window,
                                                          &scd,
                                                          nullptr,
                                                          nullptr,
                                                          swapchain.ReleaseAndGetAddressOf());

            if (FAILED (hr))
            {
                scd.Flags = 0;
                scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

                VERIFY_OK (d3dFactory->CreateSwapChainForHwnd (device.Get(),
                                                               (HWND) window,
                                                               &scd,
                                                               nullptr,
                                                               nullptr,
                                                               swapchain.ReleaseAndGetAddressOf()));
            }

            if (swapchain != nullptr)
                swapchain.As (&swapchain2);

            applyMaximumFrameLatency();
            updateFrameTimingCapabilities();
        }
        else
        {
            D3D11_TEXTURE2D_DESC desc {};
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.MipLevels = 1;
            desc.Width = width;
            desc.Height = height;
            desc.SampleDesc.Count = 1;
            desc.ArraySize = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_RENDER_TARGET;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;
            VERIFY_OK (device->CreateTexture2D (&desc, nullptr, &headlessDrawTexture));
        }

        auto renderContextImpl = getRenderContext()->static_impl_cast<rive::gpu::RenderContextD3DImpl>();
        renderTarget = renderContextImpl->makeRenderTarget (width, height);
        readbackTexture = nullptr;
    }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (getRenderContext());
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        getRenderContext()->beginFrame (frameDescriptor);
    }

    void end (void*) override
    {
        if (renderTarget->targetTexture() == nullptr)
        {
            if (isHeadless)
                renderTarget->setTargetTexture (headlessDrawTexture);
            else
            {
                if (cachedBackbuffer == nullptr)
                {
                    HRESULT hr = swapchain->GetBuffer (0, __uuidof (ID3D11Texture2D), reinterpret_cast<void**> (cachedBackbuffer.ReleaseAndGetAddressOf()));
                    if (FAILED (hr))
                    {
                        auto reason = device->GetDeviceRemovedReason();
                        fprintf (stderr, "D3D: GetBuffer failed: hr=0x%08X, deviceRemovedReason=0x%08X\n", static_cast<unsigned> (hr), static_cast<unsigned> (reason));
                        cachedBackbuffer.Reset();
                        renderTarget->setTargetTexture (nullptr);
                        return;
                    }

                    renderTarget->setTargetTexture (cachedBackbuffer);
                }
            }
        }

        rive::gpu::RenderContext::FlushResources flushDesc;
        flushDesc.renderTarget = renderTarget.get();
        getRenderContext()->flush (flushDesc);

        if (! isHeadless)
        {
            const double submissionStartedAtSeconds = getCurrentTimeSeconds();
            HRESULT hr = swapchain->Present (options.vsync ? 1 : 0, 0);
            const double submissionCompletedAtSeconds = getCurrentTimeSeconds();
            updateSubmissionTiming (submissionStartedAtSeconds, submissionCompletedAtSeconds);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                auto reason = device->GetDeviceRemovedReason();
                fprintf (stderr, "D3D: Present returned device removed/reset: hr=0x%08X, deviceRemovedReason=0x%08X\n", static_cast<unsigned> (hr), static_cast<unsigned> (reason));
                markPresentationDisjoint();
            }
            else if (FAILED (hr))
            {
                fprintf (stderr, "D3D: Present failed: hr=0x%08X\n", static_cast<unsigned> (hr));
                markPresentationDisjoint();
            }
            else
            {
                updatePresentationTiming();
            }
        }

    }

private:
    static double getCurrentTimeSeconds() noexcept
    {
        return yup::Time::getMillisecondCounterHiRes() / 1000.0;
    }

    void updateSubmissionTiming (double startedAtSeconds, double completedAtSeconds)
    {
        const CriticalSection::ScopedLockType sl (frameTimingLock);
        lastFrameTimingInfo.submissionStartedAtSeconds = startedAtSeconds;
        lastFrameTimingInfo.submissionCompletedAtSeconds = completedAtSeconds;
        lastFrameTimingInfo.hasSubmissionTimestamps = true;
    }

    void markPresentationDisjoint()
    {
        const CriticalSection::ScopedLockType sl (frameTimingLock);
        lastFrameTimingInfo.isDisjoint = true;
        lastFrameTimingInfo.hasPresentationTimestamp = false;
        hasValidPresentationTiming = false;
        updateFrameTimingCapabilitiesUnlocked();
    }

    void updatePresentationTiming()
    {
        DXGI_FRAME_STATISTICS statistics {};
        const HRESULT hr = swapchain->GetFrameStatistics (&statistics);

        const CriticalSection::ScopedLockType sl (frameTimingLock);

        lastFrameTimingInfo.isDisjoint = hr == DXGI_ERROR_FRAME_STATISTICS_DISJOINT;

        if (FAILED (hr) || statistics.PresentCount == 0 || qpcFrequency.QuadPart <= 0)
        {
            lastFrameTimingInfo.presentedAtSeconds = 0.0;
            lastFrameTimingInfo.presentationIntervalSeconds = 0.0;
            lastFrameTimingInfo.presentationCount = 0;
            lastFrameTimingInfo.hasPresentationTimestamp = false;
            hasValidPresentationTiming = false;
            updateFrameTimingCapabilitiesUnlocked();
            return;
        }

        const double presentedAtSeconds = static_cast<double> (statistics.SyncQPCTime.QuadPart)
                                        / static_cast<double> (qpcFrequency.QuadPart);

        lastFrameTimingInfo.presentationIntervalSeconds = 0.0;

        if (lastPresentedAtSeconds > 0.0 && statistics.PresentCount > lastPresentedCount)
            lastFrameTimingInfo.presentationIntervalSeconds = presentedAtSeconds - lastPresentedAtSeconds;

        lastFrameTimingInfo.presentedAtSeconds = presentedAtSeconds;
        lastFrameTimingInfo.presentationCount = statistics.PresentCount;
        lastFrameTimingInfo.hasPresentationTimestamp = true;
        hasValidPresentationTiming = true;
        updateFrameTimingCapabilitiesUnlocked();

        lastPresentedAtSeconds = presentedAtSeconds;
        lastPresentedCount = statistics.PresentCount;
    }

    void applyMaximumFrameLatency()
    {
        if (isHeadless)
            return;

        static constexpr UINT defaultMaximumFrameLatency = 3u;

        const bool usesDefaultLatency = ! maximumFramesInFlight.has_value();
        UINT latency = usesDefaultLatency
                         ? defaultMaximumFrameLatency
                         : static_cast<UINT> (jlimit<uint32_t> (1, 16, *maximumFramesInFlight));
        frameLatencyWaitableObject = nullptr;
        canControlMaximumFramesInFlight = false;

        if (swapchain2 != nullptr)
        {
            if (SUCCEEDED (swapchain2->SetMaximumFrameLatency (latency)))
            {
                canControlMaximumFramesInFlight = true;
                frameLatencyWaitableObject = swapchain2->GetFrameLatencyWaitableObject();
            }
        }
        else if (auto dxgiDevice = ComPtr<IDXGIDevice1>(); SUCCEEDED (device.As (&dxgiDevice)))
        {
            canControlMaximumFramesInFlight = SUCCEEDED (dxgiDevice->SetMaximumFrameLatency (latency));
        }

        updateFrameTimingCapabilities();
    }

    void updateFrameTimingCapabilities()
    {
        const CriticalSection::ScopedLockType sl (frameTimingLock);
        updateFrameTimingCapabilitiesUnlocked();
    }

    void updateFrameTimingCapabilitiesUnlocked() noexcept
    {
        frameTimingCapabilities.hasPresentationTiming = hasValidPresentationTiming;
        frameTimingCapabilities.hasFrameLatencyWait = frameLatencyWaitableObject != nullptr;
        frameTimingCapabilities.hasGpuCompletionTiming = false;
        frameTimingCapabilities.hasMaximumFramesInFlight = canControlMaximumFramesInFlight;
        frameTimingCapabilities.presentBlocksForDisplay = options.vsync;
    }

    const bool isHeadless;
    Options options;
    GpuDevice::Ptr gpuDevice;
    ComPtr<IDXGIFactory2> d3dFactory;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> deviceContext;
    ComPtr<IDXGISwapChain1> swapchain;
    ComPtr<IDXGISwapChain2> swapchain2;
    ComPtr<ID3D11Texture2D> cachedBackbuffer;
    ComPtr<ID3D11Texture2D> readbackTexture;
    ComPtr<ID3D11Texture2D> headlessDrawTexture;
    rive::rcp<rive::gpu::RenderTargetD3D> renderTarget;
    std::optional<uint32_t> maximumFramesInFlight;
    mutable CriticalSection frameTimingLock;
    FrameTimingCapabilities frameTimingCapabilities;
    FrameTimingInfo lastFrameTimingInfo;
    HANDLE frameLatencyWaitableObject = nullptr;
    bool canControlMaximumFramesInFlight = false;
    LARGE_INTEGER qpcFrequency {};
    double lastPresentedAtSeconds = 0.0;
    uint64_t lastPresentedCount = 0;
    bool hasValidPresentationTiming = false;
};

std::unique_ptr<GraphicsContext> yup_constructDirect3DGraphicsContext (GpuDevice::Options options, GpuDevice::Ptr existingGpu)
{
    // The swapchain (and the textures obtained from it) must belong to the very same ID3D11Device
    // that owns the render context drawing into them, so the GpuDevice is resolved first and its
    // native device is reused here instead of creating a second one.
    auto gpuDevice = existingGpu != nullptr ? std::move (existingGpu)
                                            : GpuDevice::create (GpuPlatform::Direct3D, options);
    if (gpuDevice == nullptr)
        return nullptr;

    ComPtr<ID3D11Device> device = yup_getDirect3DDevice (*gpuDevice);
    ComPtr<ID3D11DeviceContext> deviceContext = yup_getDirect3DDeviceContext (*gpuDevice);

    if (! device || ! deviceContext)
        return nullptr;

    ComPtr<IDXGIFactory2> factory;
    VERIFY_OK (CreateDXGIFactory (__uuidof (IDXGIFactory2), reinterpret_cast<void**> (factory.ReleaseAndGetAddressOf())));

    return std::make_unique<GraphicsContextD3D> (
        std::move (factory), std::move (device), std::move (deviceContext), options.allowHeadlessRendering, options, std::move (gpuDevice));
}

} // namespace yup
#endif
