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

namespace yup
{

class GraphicsContextD3D : public GraphicsContext
{
public:
    GraphicsContextD3D (ComPtr<IDXGIFactory2> d3dFactory,
                        ComPtr<ID3D11Device> device,
                        ComPtr<ID3D11DeviceContext> deviceContext,
                        bool isHeadless,
                        const rive::gpu::D3DContextOptions& contextOptions,
                        Options options,
                        GpuDevice::Ptr existingGpu = {})
        : isHeadless (isHeadless)
        , options (options)
        , d3dFactory (std::move (d3dFactory))
        , device (std::move (device))
        , deviceContext (std::move (deviceContext))
    {
        if (existingGpu != nullptr)
            gpuDevice = std::move (existingGpu);
        else
            gpuDevice = GpuDevice::create (GpuPlatform::Direct3D, options);
    }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Direct3D; }

    GpuDevice::Ptr getGpuDevice() const noexcept override { return gpuDevice; }

    rive::Factory* getFactory() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderContext* getRenderContext() override { return gpuDevice->getRenderContext(); }

    rive::gpu::RenderTarget* getRenderTarget() override { return renderTarget.get(); }

    void onSizeChanged (void* window, int width, int height, float dpiScale, uint32_t sampleCount) override
    {
        if (! isHeadless)
        {
            swapchain.Reset();
            DXGI_SWAP_CHAIN_DESC1 scd {};
            scd.Width = width;
            scd.Height = height;
            scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            scd.SampleDesc.Count = 1;
            scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
            scd.BufferCount = 2;
            scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            VERIFY_OK (d3dFactory->CreateSwapChainForHwnd (device.Get(),
                                                           (HWND) window,
                                                           &scd,
                                                           nullptr,
                                                           nullptr,
                                                           swapchain.ReleaseAndGetAddressOf()));
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
                ComPtr<ID3D11Texture2D> backbuffer;
                HRESULT hr = swapchain->GetBuffer (0, __uuidof (ID3D11Texture2D), reinterpret_cast<void**> (backbuffer.ReleaseAndGetAddressOf()));
                if (FAILED (hr))
                {
                    auto reason = device->GetDeviceRemovedReason();
                    fprintf (stderr, "D3D: GetBuffer failed: hr=0x%08X, deviceRemovedReason=0x%08X\n", static_cast<unsigned> (hr), static_cast<unsigned> (reason));
                    renderTarget->setTargetTexture (nullptr);
                    return;
                }
                renderTarget->setTargetTexture (backbuffer);
            }
        }

        rive::gpu::RenderContext::FlushResources flushDesc;
        flushDesc.renderTarget = renderTarget.get();
        getRenderContext()->flush (flushDesc);

        if (! isHeadless)
        {
            HRESULT hr = swapchain->Present (0, 0);
            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                auto reason = device->GetDeviceRemovedReason();
                fprintf (stderr, "D3D: Present returned device removed/reset: hr=0x%08X, deviceRemovedReason=0x%08X\n", static_cast<unsigned> (hr), static_cast<unsigned> (reason));
            }
            else if (FAILED (hr))
            {
                fprintf (stderr, "D3D: Present failed: hr=0x%08X\n", static_cast<unsigned> (hr));
            }
        }

        renderTarget->setTargetTexture (nullptr);
    }

private:
    const bool isHeadless;
    Options options;
    GpuDevice::Ptr gpuDevice;
    ComPtr<IDXGIFactory2> d3dFactory;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> deviceContext;
    ComPtr<IDXGISwapChain1> swapchain;
    ComPtr<ID3D11Texture2D> readbackTexture;
    ComPtr<ID3D11Texture2D> headlessDrawTexture;
    rive::rcp<rive::gpu::RenderTargetD3D> renderTarget;
};

std::unique_ptr<GraphicsContext> yup_constructDirect3DGraphicsContext (GpuDevice::Options options, GpuDevice::Ptr existingGpu)
{
    ComPtr<IDXGIFactory2> factory;
    VERIFY_OK (CreateDXGIFactory (__uuidof (IDXGIFactory2), reinterpret_cast<void**> (factory.ReleaseAndGetAddressOf())));

    ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC adapterDesc {};
    rive::gpu::D3DContextOptions contextOptions;

    if (options.disableRasterOrdering)
    {
        contextOptions.disableRasterizerOrderedViews = true;
        contextOptions.disableTypedUAVLoadStore = true;
    }

    for (UINT i = 0; factory->EnumAdapters (i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        adapter->GetDesc (&adapterDesc);
        contextOptions.isIntel = adapterDesc.VendorId == 0x163C || adapterDesc.VendorId == 0x8086 || adapterDesc.VendorId == 0x8087;
        break;
    }

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> deviceContext;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1 };

    UINT creationFlags = 0;
#ifdef DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    VERIFY_OK (D3D11CreateDevice (adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, creationFlags, featureLevels, std::size (featureLevels), D3D11_SDK_VERSION, device.ReleaseAndGetAddressOf(), nullptr, deviceContext.ReleaseAndGetAddressOf()));

    if (! device || ! deviceContext)
        return nullptr;

    printf ("D3D device: %S\n", adapterDesc.Description);

    return std::make_unique<GraphicsContextD3D> (
        std::move (factory), std::move (device), std::move (deviceContext), options.allowHeadlessRendering, contextOptions, options, std::move (existingGpu));
}

} // namespace yup
#endif
