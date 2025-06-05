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
#include <array>
#include <dxgi1_2.h>

namespace yup
{

class LowLevelRenderContextD3D : public GraphicsContext
{
public:
    LowLevelRenderContextD3D (ComPtr<IDXGIFactory2> d3dFactory,
                              ComPtr<ID3D11Device> gpu,
                              ComPtr<ID3D11DeviceContext> gpuContext,
                              bool isHeadless,
                              const rive::gpu::D3DContextOptions& contextOptions)
        : m_isHeadless (isHeadless)
        ,
        , m_d3dFactory (std::move (d3dFactory))
        , m_gpu (std::move (gpu))
        , m_gpuContext (std::move (gpuContext))
        , m_renderContext (rive::gpu::RenderContextD3DImpl::MakeContext (m_gpu, m_gpuContext, contextOptions))
    {
    }

    float dpiScale (void*) const override { return 1.0f; }

    rive::Factory* factory() override { return m_renderContext.get(); }

    rive::gpu::RenderContext* renderContext() override { return m_renderContext.get(); }

    rive::gpu::RenderTarget* renderTarget() override { return m_renderTarget.get(); }

    void onSizeChanged (void* window, int width, int height, uint32_t sampleCount) override
    {
        if (! m_isHeadless)
        {
            m_swapchain.Reset();

            DXGI_SWAP_CHAIN_DESC1 scd {};
            scd.Width = width;
            scd.Height = height;
            scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            scd.SampleDesc.Count = 1;
            scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
            scd.BufferCount = 2;
            scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

            VERIFY_OK (m_d3dFactory->CreateSwapChainForHwnd (m_gpu.Get(),
                                                             (HWND) window,
                                                             &scd,
                                                             nullptr,
                                                             nullptr,
                                                             m_swapchain.ReleaseAndGetAddressOf()));
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
            VERIFY_OK (
                m_gpu->CreateTexture2D (&desc, NULL, &m_headlessDrawTexture));
        }

        auto renderContextImpl = m_renderContext->static_impl_cast<rive::gpu::RenderContextD3DImpl>();
        m_renderTarget = renderContextImpl->makeRenderTarget (width, height);
        m_readbackTexture = nullptr;
    }

    std::unique_ptr<rive::Renderer> makeRenderer (int width, int height) override
    {
        return std::make_unique<rive::RiveRenderer> (m_renderContext.get());
    }

    void begin (const rive::gpu::RenderContext::FrameDescriptor& frameDescriptor) override
    {
        m_renderContext->beginFrame (frameDescriptor);
    }

    void end (void*) override
    {
        if (m_renderTarget->targetTexture() == nullptr)
        {
            if (m_isHeadless)
            {
                m_renderTarget->setTargetTexture (m_headlessDrawTexture);
            }
            else
            {
                ComPtr<ID3D11Texture2D> backbuffer;
                VERIFY_OK (m_swapchain->GetBuffer (0,
                                                   __uuidof (ID3D11Texture2D),
                                                   reinterpret_cast<void**> (backbuffer.ReleaseAndGetAddressOf())));

                m_renderTarget->setTargetTexture (backbuffer);
            }
        }

        rive::gpu::RenderContext::FlushResources flushDesc;
        flushDesc.renderTarget = m_renderTarget.get();
        m_renderContext->flush (flushDesc);

        if (! m_isHeadless)
            m_swapchain->Present (0, 0);

        m_renderTarget->setTargetTexture (nullptr);
    }

private:
    const bool m_isHeadless;
    ComPtr<IDXGIFactory2> m_d3dFactory;
    ComPtr<ID3D11Device> m_gpu;
    ComPtr<ID3D11DeviceContext> m_gpuContext;
    ComPtr<IDXGISwapChain1> m_swapchain;
    ComPtr<ID3D11Texture2D> m_readbackTexture;
    ComPtr<ID3D11Texture2D> m_headlessDrawTexture;
    std::unique_ptr<rive::gpu::RenderContext> m_renderContext;
    rive::rcp<rive::gpu::RenderTargetD3D> m_renderTarget;
};

std::unique_ptr<GraphicsContext> juce_constructDirect3DGraphicsContext (GraphicsContext::Options fiddleOptions)
{
    // Create a DXGIFactory object.
    ComPtr<IDXGIFactory2> factory;
    VERIFY_OK (CreateDXGIFactory (__uuidof (IDXGIFactory2), reinterpret_cast<void**> (factory.ReleaseAndGetAddressOf())));

    ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC adapterDesc {};
    rive::gpu::D3DContextOptions contextOptions;

    if (fiddleOptions.disableRasterOrdering)
    {
        contextOptions.disableRasterizerOrderedViews = true;
        // Also disable typed UAVs in atomic mode, to get more complete test coverage.
        contextOptions.disableTypedUAVLoadStore = true;
    }

    for (UINT i = 0; factory->EnumAdapters (i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        adapter->GetDesc (&adapterDesc);
        contextOptions.isIntel = adapterDesc.VendorId == 0x163C || adapterDesc.VendorId == 0x8086 || adapterDesc.VendorId == 0x8087;

        break;
    }

    ComPtr<ID3D11Device> gpu;
    ComPtr<ID3D11DeviceContext> gpuContext;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1 };

    UINT creationFlags = 0;
#ifdef DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    VERIFY_OK (D3D11CreateDevice (adapter.Get(),
                                  D3D_DRIVER_TYPE_UNKNOWN,
                                  nullptr,
                                  creationFlags,
                                  featureLevels,
                                  std::size (featureLevels),
                                  D3D11_SDK_VERSION,
                                  gpu.ReleaseAndGetAddressOf(),
                                  nullptr,
                                  gpuContext.ReleaseAndGetAddressOf()));
    if (! gpu || ! gpuContext)
        return nullptr;

    printf ("D3D device: %S\n", adapterDesc.Description);

    return std::make_unique<LowLevelRenderContextD3D> (
        std::move (factory),
        std::move (gpu),
        std::move (gpuContext),
        fiddleOptions.allowHeadlessRendering,
        contextOptions);
}

} // namespace yup
#endif
