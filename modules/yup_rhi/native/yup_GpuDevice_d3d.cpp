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

#if YUP_RIVE_USE_D3D
#include "rive/renderer/d3d11/render_context_d3d_impl.hpp"
#include "rive/renderer/d3d11/d3d11.hpp"
#include "rive/renderer/ore/ore_context_d3d11.hpp"

#include <dxgi1_2.h>

#include <vector>

namespace yup
{

class GpuDeviceD3D : public GpuDevice
{
public:
    GpuDeviceD3D (ComPtr<ID3D11Device> gpuToUse,
                  ComPtr<ID3D11DeviceContext> gpuContextToUse,
                  const rive::gpu::D3DContextOptions& contextOptions,
                  Options options)
        : options (options)
        , renderContextOptions (contextOptions)
        , gpu (std::move (gpuToUse))
        , gpuContext (std::move (gpuContextToUse))
        , renderContext (rive::gpu::RenderContextD3DImpl::MakeContext (gpu, gpuContext, renderContextOptions))
        , oreContext (rive::ore::ContextD3D11::Make (gpu.Get(), gpuContext.Get()))
    {
    }

    ~GpuDeviceD3D() override { releasePooledResources(); }

    GpuPlatform getPlatform() const noexcept override { return GpuPlatform::Direct3D; }

    rive::gpu::RenderContext* getRenderContext() const override { return renderContext.get(); }

    rive::ore::Context* getGpuContext() const noexcept override { return oreContext.get(); }

    bool isComputeAvailable() const noexcept override { return true; }

    /** Returns the native ID3D11Device for compute operations. */
    ID3D11Device* getD3DDevice() const noexcept { return gpu.Get(); }

    /** Returns the native ID3D11DeviceContext for compute operations. */
    ID3D11DeviceContext* getD3DDeviceContext() const noexcept { return gpuContext.Get(); }

    //==============================================================================

    ReferenceCountedObjectPtr<GpuBuffer> createBuffer (GpuBufferType type, const void* data, size_t byteSize) override
    {
        if (type == GpuBufferType::storage)
        {
            jassert (data != nullptr && byteSize > 0);
            if (data == nullptr || byteSize == 0)
                return nullptr;

            D3D11_BUFFER_DESC bufDesc {};
            bufDesc.ByteWidth = static_cast<UINT> (byteSize);
            bufDesc.Usage = D3D11_USAGE_DEFAULT;
            bufDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
            bufDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
            bufDesc.StructureByteStride = 0;

            D3D11_SUBRESOURCE_DATA initData {};
            initData.pSysMem = data;

            ComPtr<ID3D11Buffer> d3dBuffer;
            HRESULT hr = gpu->CreateBuffer (&bufDesc, &initData, d3dBuffer.ReleaseAndGetAddressOf());
            if (FAILED (hr) || d3dBuffer == nullptr)
                return nullptr;

            D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc {};
            uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.NumElements = static_cast<UINT> (byteSize / 4);
            uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

            ComPtr<ID3D11UnorderedAccessView> uav;
            hr = gpu->CreateUnorderedAccessView (d3dBuffer.Get(), &uavDesc, uav.ReleaseAndGetAddressOf());
            if (FAILED (hr) || uav == nullptr)
                return nullptr;

            return GpuBuffer::createWithImpl (GpuBuffer::Impl { type, byteSize, {}, std::move (d3dBuffer), std::move (uav) });
        }

        return GpuDevice::createBuffer (type, data, byteSize);
    }

    //==============================================================================

    bool readBuffer (GpuBuffer::Ptr buffer, void* dst, size_t dstSize) override
    {
        if (buffer == nullptr || dst == nullptr)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr || impl->d3dStorageBuffer == nullptr)
            return false;

        const auto byteSize = buffer->getSizeInBytes();
        if (dstSize < byteSize)
            return false;

        // The storage buffer is D3D11_USAGE_DEFAULT and so not CPU accessible; a
        // staging copy is the only way to reach its contents.
        if (impl->d3dReadbackStaging == nullptr)
        {
            D3D11_BUFFER_DESC stagingDesc {};
            stagingDesc.ByteWidth = static_cast<UINT> (byteSize);
            stagingDesc.Usage = D3D11_USAGE_STAGING;
            stagingDesc.BindFlags = 0;
            stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            stagingDesc.MiscFlags = 0;
            stagingDesc.StructureByteStride = 0;

            HRESULT hr = gpu->CreateBuffer (&stagingDesc, nullptr, impl->d3dReadbackStaging.ReleaseAndGetAddressOf());
            if (FAILED (hr) || impl->d3dReadbackStaging == nullptr)
                return false;
        }

        // The compute dispatch was issued on this same immediate context, so the
        // copy is ordered after it, and Map (without DO_NOT_WAIT) blocks until the
        // copy has retired — which is the blocking contract readBuffer documents.
        gpuContext->CopyResource (impl->d3dReadbackStaging.Get(), impl->d3dStorageBuffer.Get());

        D3D11_MAPPED_SUBRESOURCE mapped {};
        HRESULT hr = gpuContext->Map (impl->d3dReadbackStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
        if (FAILED (hr) || mapped.pData == nullptr)
            return false;

        memcpy (dst, mapped.pData, byteSize);
        gpuContext->Unmap (impl->d3dReadbackStaging.Get(), 0);
        return true;
    }

    //==============================================================================

    bool updateBuffer (GpuBuffer::Ptr buffer, const void* data, size_t byteSize) override
    {
        if (buffer == nullptr || data == nullptr || byteSize == 0)
            return false;

        auto* impl = buffer->getImpl();
        if (impl == nullptr)
            return false;

        // For ore-backed buffers (vertex, index, uniform), delegate to base class.
        if (impl->d3dStorageBuffer == nullptr)
            return GpuDevice::updateBuffer (buffer, data, byteSize);

        const auto fullSize = buffer->getSizeInBytes();
        if (byteSize > fullSize)
            return false;

        if (byteSize == fullSize)
        {
            gpuContext->UpdateSubresource (impl->d3dStorageBuffer.Get(), 0, nullptr, data, static_cast<UINT> (byteSize), 0);
        }
        else
        {
            D3D11_BOX box { 0, 0, 0, static_cast<UINT> (byteSize), 1, 1 };
            gpuContext->UpdateSubresource (impl->d3dStorageBuffer.Get(), 0, &box, data, static_cast<UINT> (byteSize), 0);
        }

        return true;
    }

    //==============================================================================

    struct OffscreenContextSlot
    {
        std::unique_ptr<rive::gpu::RenderContext> renderContext;
        bool frameActive = false;
        bool leased = false;
    };

    struct OffscreenTargetD3D : public RenderableTarget
    {
        ~OffscreenTargetD3D() override
        {
            if (contextSlot != nullptr)
                contextSlot->leased = false;
        }

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

    ComPtr<ID3D11Texture2D> createStagingTexture (int width, int height)
    {
        D3D11_TEXTURE2D_DESC stagingDesc {};
        stagingDesc.Width = static_cast<UINT> (width);
        stagingDesc.Height = static_cast<UINT> (height);
        stagingDesc.MipLevels = 1;
        stagingDesc.ArraySize = 1;
        stagingDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        ComPtr<ID3D11Texture2D> staging;
        auto hr = gpu->CreateTexture2D (&stagingDesc, nullptr, staging.ReleaseAndGetAddressOf());
        if (FAILED (hr))
            return nullptr;
        return staging;
    }

    std::unique_ptr<OffscreenTarget> createOffscreenTarget (int width, int height) override
    {
        if (width <= 0 || height <= 0 || renderContext == nullptr)
            return nullptr;

        auto target = std::make_unique<OffscreenTargetD3D>();
        target->width = width;
        target->height = height;
        target->renderContext = nullptr;
        target->contextSlot = nullptr;

        target->renderCanvas = renderContext->makeRenderCanvas (static_cast<uint32_t> (width),
                                                                static_cast<uint32_t> (height));
        if (target->renderCanvas == nullptr)
            return nullptr;

        target->stagingTexture = createStagingTexture (width, height);
        if (target->stagingTexture == nullptr)
            return nullptr;

        return target;
    }

    std::unique_ptr<RenderableTarget> createRenderableTarget (int width, int height) override
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

        target->stagingTexture = createStagingTexture (width, height);
        if (target->stagingTexture == nullptr)
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
            gpuContext->CopyResource (target.stagingTexture.Get(), renderTarget->targetTexture());

        target.contextSlot->frameActive = false;
    }

    bool clearOffscreen (OffscreenTarget& baseTarget, GpuColor color) override
    {
        auto& target = static_cast<OffscreenTargetD3D&> (baseTarget);

        auto* renderTarget = static_cast<rive::gpu::RenderTargetD3D*> (target.getRenderTarget());
        if (renderTarget == nullptr || renderTarget->targetTexture() == nullptr)
            return false;

        ComPtr<ID3D11RenderTargetView> renderTargetView;
        if (FAILED (gpu->CreateRenderTargetView (renderTarget->targetTexture(), nullptr, renderTargetView.ReleaseAndGetAddressOf())))
            return false;

        const FLOAT rgba[4] { color.red, color.green, color.blue, color.alpha };
        gpuContext->ClearRenderTargetView (renderTargetView.Get(), rgba);

        return true;
    }

    bool readOffscreenPixels (OffscreenTarget& baseTarget, void* dst, size_t dstSize) override
    {
        auto& target = static_cast<OffscreenTargetD3D&> (baseTarget);

        if (target.stagingTexture == nullptr || dst == nullptr)
            return false;

        const size_t bytesPerRow = static_cast<size_t> (target.width) * 4u;
        if (dstSize < bytesPerRow * static_cast<size_t> (target.height))
            return false;

        if (target.getRenderContext() == nullptr)
        {
            if (auto* renderTarget = static_cast<rive::gpu::RenderTargetD3D*> (target.getRenderTarget()))
                gpuContext->CopyResource (target.stagingTexture.Get(), renderTarget->targetTexture());
        }

        D3D11_MAPPED_SUBRESOURCE mapped {};
        HRESULT hr = gpuContext->Map (target.stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
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

        gpuContext->Unmap (target.stagingTexture.Get(), 0);
        return true;
    }

private:
    OffscreenContextSlot* acquireOffscreenContext()
    {
        for (const auto& slot : offscreenContextPool)
        {
            if (! slot->leased)
            {
                slot->leased = true;
                return slot.get();
            }
        }

        auto slot = std::make_unique<OffscreenContextSlot>();
        slot->renderContext = rive::gpu::RenderContextD3DImpl::MakeContext (gpu, gpuContext, renderContextOptions);
        if (slot->renderContext == nullptr)
            return nullptr;

        slot->leased = true;

        auto* result = slot.get();
        offscreenContextPool.push_back (std::move (slot));
        return result;
    }

    Options options;
    rive::gpu::D3DContextOptions renderContextOptions;
    ComPtr<ID3D11Device> gpu;
    ComPtr<ID3D11DeviceContext> gpuContext;
    std::unique_ptr<rive::gpu::RenderContext> renderContext;
    std::vector<std::unique_ptr<OffscreenContextSlot>> offscreenContextPool;
    std::unique_ptr<rive::ore::ContextD3D11> oreContext;
};

//==============================================================================

ID3D11Device* yup_getDirect3DDevice (GpuDevice& gpuDevice)
{
    if (gpuDevice.getPlatform() != GpuPlatform::Direct3D)
        return nullptr;

    return static_cast<GpuDeviceD3D&> (gpuDevice).getD3DDevice();
}

ID3D11DeviceContext* yup_getDirect3DDeviceContext (GpuDevice& gpuDevice)
{
    if (gpuDevice.getPlatform() != GpuPlatform::Direct3D)
        return nullptr;

    return static_cast<GpuDeviceD3D&> (gpuDevice).getD3DDeviceContext();
}

//==============================================================================

std::unique_ptr<GpuDevice> yup_constructDirect3DGpuDevice (GpuDevice::Options fiddleOptions)
{
    ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC adapterDesc {};
    rive::gpu::D3DContextOptions contextOptions;

    if (fiddleOptions.disableRasterOrdering)
    {
        contextOptions.disableRasterizerOrderedViews = true;
        contextOptions.disableTypedUAVLoadStore = true;
    }

    // Create a temporary factory just to enumerate adapters
    ComPtr<IDXGIFactory2> factory;
    VERIFY_OK (CreateDXGIFactory (__uuidof (IDXGIFactory2), reinterpret_cast<void**> (factory.ReleaseAndGetAddressOf())));

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

    return std::make_unique<GpuDeviceD3D> (std::move (gpu), std::move (gpuContext), contextOptions, fiddleOptions);
}

} // namespace yup
#endif
