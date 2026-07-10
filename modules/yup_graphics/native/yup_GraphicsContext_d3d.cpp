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
#include "rive/renderer/ore/ore_context_d3d11.hpp"
#include <dxgi1_2.h>
#include <vector>

namespace yup
{

class LowLevelRenderContextD3D : public GraphicsContext
{
public:
    LowLevelRenderContextD3D (ComPtr<IDXGIFactory2> d3dFactory,
                              ComPtr<ID3D11Device> gpu,
                              ComPtr<ID3D11DeviceContext> gpuContext,
                              bool isHeadless,
                              const rive::gpu::D3DContextOptions& contextOptions,
                              Options options)
        : m_isHeadless (isHeadless)
        , m_options (options)
        , m_renderContextOptions (contextOptions)
        , m_d3dFactory (std::move (d3dFactory))
        , m_gpu (std::move (gpu))
        , m_gpuContext (std::move (gpuContext))
        , m_renderContext (rive::gpu::RenderContextD3DImpl::MakeContext (m_gpu, m_gpuContext, m_renderContextOptions))
    {
        if (m_options.enableOreContext)
            m_oreContext = rive::ore::ContextD3D11::Make (m_gpu.Get(), m_gpuContext.Get());
    }

    Api getApi() const noexcept override { return Api::Direct3D; }

    float dpiScale (void*) const override { return 1.0f; }

    rive::Factory* factory() override { return m_renderContext.get(); }

    rive::gpu::RenderContext* renderContext() override { return m_renderContext.get(); }

    rive::gpu::RenderTarget* renderTarget() override { return m_renderTarget.get(); }

    rive::ore::Context* gpuContext() const noexcept override { return m_oreContext.get(); }

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

            fprintf (stderr, "D3D: CreateSwapChainForHwnd hwnd=%p size=%dx%d\n", window, width, height);
            fflush (stderr);

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
            VERIFY_OK (m_gpu->CreateTexture2D (&desc, nullptr, &m_headlessDrawTexture));
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
                HRESULT hr = m_swapchain->GetBuffer (0,
                                                     __uuidof (ID3D11Texture2D),
                                                     reinterpret_cast<void**> (backbuffer.ReleaseAndGetAddressOf()));

                if (FAILED (hr))
                {
                    auto reason = m_gpu->GetDeviceRemovedReason();
                    fprintf (stderr, "D3D: GetBuffer failed: hr=0x%08X, deviceRemovedReason=0x%08X\n", static_cast<unsigned> (hr), static_cast<unsigned> (reason));
                    fflush (stderr);
                    m_renderTarget->setTargetTexture (nullptr);
                    return;
                }

                m_renderTarget->setTargetTexture (backbuffer);
            }
        }

        rive::gpu::RenderContext::FlushResources flushDesc;
        flushDesc.renderTarget = m_renderTarget.get();
        m_renderContext->flush (flushDesc);

        if (! m_isHeadless)
        {
            HRESULT hr = m_swapchain->Present (0, 0);

            if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
            {
                auto reason = m_gpu->GetDeviceRemovedReason();
                fprintf (stderr, "D3D: Present returned device removed/reset: hr=0x%08X, deviceRemovedReason=0x%08X\n", static_cast<unsigned> (hr), static_cast<unsigned> (reason));
                fflush (stderr);
            }
            else if (FAILED (hr))
            {
                fprintf (stderr, "D3D: Present failed: hr=0x%08X\n", static_cast<unsigned> (hr));
                fflush (stderr);
            }
        }

        m_renderTarget->setTargetTexture (nullptr);
    }

    //==============================================================================

    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
    };

    struct OffscreenTargetD3D : public OffscreenTarget
    {
        int width = 0;
        int height = 0;
        ComPtr<ID3D11Texture2D> stagingTexture;
        rive::rcp<rive::gpu::RenderCanvas> renderCanvas;
        rive::gpu::RenderContext* renderContext = nullptr;
        OffscreenContextSlot* contextSlot = nullptr;

        int getWidth() const noexcept override { return width; }

        int getHeight() const noexcept override { return height; }

        rive::gpu::RenderTarget* getRenderTarget() noexcept override
        {
            return renderCanvas != nullptr ? renderCanvas->renderTarget() : nullptr;
        }

        rive::gpu::RenderContext* getRenderContext() noexcept override
        {
            return renderContext;
        }

        rive::rcp<rive::gpu::RenderCanvas> getRenderCanvas() noexcept override
        {
            return renderCanvas;
        }

        rive::rcp<rive::gpu::Texture> adoptAsTexture() override
        {
            if (renderCanvas == nullptr)
                return nullptr;

            return renderCanvas->renderImage()->refTexture();
        }
    };

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0)
            return nullptr;

        auto* contextSlot = acquireOffscreenContext();
        if (contextSlot == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetD3D>();
        target->width = width;
        target->height = height;
        target->renderContext = contextSlot->renderContext.get();
        target->contextSlot = contextSlot;

        target->renderCanvas = target->renderContext->makeRenderCanvas (static_cast<uint32_t> (width),
                                                                        static_cast<uint32_t> (height));
        if (target->renderCanvas == nullptr)
            return nullptr;

        D3D11_TEXTURE2D_DESC stagingDesc {};
        stagingDesc.Width = static_cast<UINT> (width);
        stagingDesc.Height = static_cast<UINT> (height);
        stagingDesc.MipLevels = 1;
        stagingDesc.ArraySize = 1;
        stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        auto hr = m_gpu->CreateTexture2D (&stagingDesc, nullptr, target->stagingTexture.ReleaseAndGetAddressOf());
        if (FAILED (hr))
            return nullptr;

        return target;
    }

    void beginOffscreen (OffscreenTarget& baseTarget, const rive::gpu::RenderContext::FrameDescriptor& frameDesc) override
    {
        auto& target = static_cast<OffscreenTargetD3D&> (baseTarget);
        auto* renderContext = target.getRenderContext();

        if (renderContext != nullptr)
        {
            if (target.contextSlot == nullptr || target.contextSlot->frameActive)
                return;

            renderContext->beginFrame (frameDesc);
            target.contextSlot->frameActive = true;
        }
    }

    void endOffscreen (OffscreenTarget& baseTarget) override
    {
        auto& target = static_cast<OffscreenTargetD3D&> (baseTarget);
        auto* renderContext = target.getRenderContext();

        if (renderContext == nullptr || target.contextSlot == nullptr || ! target.contextSlot->frameActive)
            return;

        rive::gpu::RenderContext::FlushResources flushDesc;
        flushDesc.renderTarget = target.getRenderTarget();
        renderContext->flush (flushDesc);

        if (auto* renderTarget = static_cast<rive::gpu::RenderTargetD3D*> (target.getRenderTarget()))
            m_gpuContext->CopyResource (target.stagingTexture.Get(), renderTarget->targetTexture());

        target.contextSlot->frameActive = false;
    }

    bool readOffscreenPixels (OffscreenTarget& baseTarget, void* dst, size_t dstSize) override
    {
        auto& target = static_cast<OffscreenTargetD3D&> (baseTarget);

        if (target.stagingTexture == nullptr || dst == nullptr)
            return false;

        const size_t bytesPerRow = static_cast<size_t> (target.width) * 4u;
        if (dstSize < bytesPerRow * static_cast<size_t> (target.height))
            return false;

        D3D11_MAPPED_SUBRESOURCE mapped {};
        HRESULT hr = m_gpuContext->Map (target.stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
        if (FAILED (hr))
            return false;

        auto* dstBytes = static_cast<uint8_t*> (dst);
        const auto* srcBytes = static_cast<const uint8_t*> (mapped.pData);

        for (int row = 0; row < target.height; ++row)
        {
            std::memcpy (dstBytes + static_cast<size_t> (row) * bytesPerRow,
                         srcBytes + static_cast<size_t> (row) * mapped.RowPitch,
                         bytesPerRow);
        }

        m_gpuContext->Unmap (target.stagingTexture.Get(), 0);

        return true;
    }

private:
    OffscreenContextSlot* acquireOffscreenContext()
    {
        for (const auto& slot : m_offscreenContextPool)
        {
            if (! slot->frameActive)
                return slot.get();
        }

        auto slot = std::make_unique<OffscreenContextSlot>();
        slot->renderContext = rive::gpu::RenderContextD3DImpl::MakeContext (m_gpu, m_gpuContext, m_renderContextOptions);
        if (slot->renderContext == nullptr)
            return nullptr;

        auto* result = slot.get();
        m_offscreenContextPool.push_back (std::move (slot));
        return result;
    }

    const bool m_isHeadless;

    Options m_options;
    rive::gpu::D3DContextOptions m_renderContextOptions;
    ComPtr<IDXGIFactory2> m_d3dFactory;
    ComPtr<ID3D11Device> m_gpu;
    ComPtr<ID3D11DeviceContext> m_gpuContext;
    ComPtr<IDXGISwapChain1> m_swapchain;
    ComPtr<ID3D11Texture2D> m_readbackTexture;
    ComPtr<ID3D11Texture2D> m_headlessDrawTexture;
    std::unique_ptr<rive::gpu::RenderContext> m_renderContext;
    std::vector<std::unique_ptr<OffscreenContextSlot>> m_offscreenContextPool;
    std::unique_ptr<rive::ore::ContextD3D11> m_oreContext;
    rive::rcp<rive::gpu::RenderTargetD3D> m_renderTarget;
};

std::unique_ptr<GraphicsContext> yup_constructDirect3DGraphicsContext (GraphicsContext::Options fiddleOptions)
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
        contextOptions,
        fiddleOptions);
}

} // namespace yup
#endif
