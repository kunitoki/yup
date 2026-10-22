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

#if YUP_RIVE_USE_D3D && YUP_WINDOWS

#include "ore/d3d11/ore_sampler_d3d11.hpp"
#include "ore/d3d11/ore_texture_d3d11.hpp"

namespace yup
{

//==============================================================================

class GpuComputePassImplD3D11 final : public GpuComputePass::Impl
{
public:
    GpuComputePassImplD3D11 (ID3D11Device* dev, ID3D11DeviceContext* ctx, rive::ore::Context* ore)
        : device (dev)
        , context (ctx)
        , oreCtx (ore)
    {
    }

    bool isValid() const override { return device != nullptr && context != nullptr; }

    //==========================================================================

    bool dispatch (uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) override
    {
        if (device == nullptr || context == nullptr || pipelineRef == nullptr)
            return false;

        auto* pipe = dynamic_cast<GpuComputePipelineD3D11*> (pipelineRef.get());
        if (pipe == nullptr || pipe->getComputeShader() == nullptr)
            return false;

        context->CSSetShader (pipe->getComputeShader(), nullptr, 0);

        // UAVs (storage buffers).
        for (auto& sb : storageBindings)
        {
            if (sb.buffer == nullptr)
                continue;
            auto* bufImpl = sb.buffer->getImpl();
            if (bufImpl == nullptr || bufImpl->d3dUav == nullptr)
                continue;

            bindUav (slotIndex (sb.group, sb.binding), bufImpl->d3dUav.Get());
        }

        // Constant buffers (uniforms) — 16-byte aligned, DYNAMIC + CPU_WRITE.
        for (auto& ub : uboBindings)
        {
            if (ub.data.empty())
                continue;

            UINT cbSize = static_cast<UINT> ((ub.data.size() + 15) & ~15u);

            D3D11_BUFFER_DESC cbDesc {};
            cbDesc.ByteWidth = cbSize;
            cbDesc.Usage = D3D11_USAGE_DYNAMIC;
            cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            D3D11_SUBRESOURCE_DATA initData {};
            initData.pSysMem = ub.data.data();

            ComPtr<ID3D11Buffer> cb;
            HRESULT hr = device->CreateBuffer (&cbDesc, &initData, cb.ReleaseAndGetAddressOf());
            if (FAILED (hr) || cb == nullptr)
                continue;

            context->CSSetConstantBuffers (slotIndex (ub.group, ub.binding), 1, cb.GetAddressOf());
            tempBuffers.push_back (std::move (cb));
        }

        for (auto& tb : texBindings)
        {
            if (tb.texture == nullptr || oreCtx == nullptr)
                continue;

            auto view = tb.texture->getOrCreateSamplingView (*oreCtx);
            auto* srv = shaderResourceViewOf (view.get());
            if (srv == nullptr)
                continue;

            const auto slot = slotIndex (tb.group, tb.binding);
            context->CSSetShaderResources (slot, 1, &srv);
            rememberSlot (boundSrvSlots, slot);
            boundViews.push_back (std::move (view));
        }

        for (auto& sb : storageTexBindings)
        {
            if (sb.texture == nullptr)
                continue;

            if (auto* uav = getOrCreateStorageUav (sb.texture, sb.view))
                bindUav (slotIndex (sb.group, sb.binding), uav);
        }

        for (auto& sb : samplerBindings)
        {
            if (sb.sampler == nullptr)
                continue;

            auto* samplerD3D = rive::lite_rtti_cast<rive::ore::SamplerD3D11*> (sb.sampler->getOreSampler().get());
            ID3D11SamplerState* samplerState = samplerD3D != nullptr ? samplerD3D->d3dSampler() : nullptr;
            if (samplerState == nullptr)
                continue;

            context->CSSetSamplers (slotIndex (sb.group, sb.binding), 1, &samplerState);
        }

        context->Dispatch (groupsX, groupsY, groupsZ);
        return true;
    }

    //==========================================================================

    void finish() override
    {
        // Leaving the storage buffers and textures bound as UAVs, or the textures
        // as SRVs, makes any later use of them (a staging copy in
        // GpuDevice::readBuffer(), a render pass sampling or drawing into the same
        // resource) a simultaneous write/read binding, which the debug layer
        // flags. Only the slots this pass actually bound are cleared, so the
        // render context's own state is left alone.
        if (context != nullptr)
        {
            ID3D11UnorderedAccessView* nullUav[1] = { nullptr };
            ID3D11ShaderResourceView* nullSrv[1] = { nullptr };

            for (auto slot : boundUavSlots)
                context->CSSetUnorderedAccessViews (slot, 1, nullUav, nullptr);

            for (auto slot : boundSrvSlots)
                context->CSSetShaderResources (slot, 1, nullSrv);
        }

        boundUavSlots.clear();
        boundSrvSlots.clear();
        boundViews.clear();
        storageUavs.clear();
        tempBuffers.clear();
        device = nullptr;
        context = nullptr;
        oreCtx = nullptr;
    }

private:
    struct StorageUav
    {
        GpuTexture::Ptr texture;
        GpuTextureViewDesc view;
        ComPtr<ID3D11UnorderedAccessView> uav;
    };

    /** The register of a (group, binding) slot, the same for buffers, textures and samplers.
        The transpiled HLSL (shader model 5.0) has no register spaces and numbers every
        resource by its binding alone, ignoring the set, so only group 0 lines up with it. */
    static UINT slotIndex (int group, int binding)
    {
        jassert (group == 0);
        return static_cast<UINT> (group * 16 + binding);
    }

    static ID3D11ShaderResourceView* shaderResourceViewOf (rive::ore::TextureView* view)
    {
        auto* viewD3D = rive::lite_rtti_cast<rive::ore::TextureViewD3D11*> (view);
        return viewD3D != nullptr ? viewD3D->d3dSRV() : nullptr;
    }

    /** Records a slot for unbinding in finish(), ignoring repeats across dispatches. */
    static void rememberSlot (std::vector<UINT>& slots, UINT slot)
    {
        if (std::find (slots.begin(), slots.end(), slot) == slots.end())
            slots.push_back (slot);
    }

    void bindUav (UINT slot, ID3D11UnorderedAccessView* uav)
    {
        context->CSSetUnorderedAccessViews (slot, 1, &uav, nullptr);
        rememberSlot (boundUavSlots, slot);
    }

    /** Returns the UAV writing the single level @p view selects, created on first use
        in this pass. */
    ID3D11UnorderedAccessView* getOrCreateStorageUav (const GpuTexture::Ptr& texture, const GpuTextureViewDesc& view)
    {
        for (const auto& cached : storageUavs)
        {
            if (cached.texture == texture && cached.view == view)
                return cached.uav.Get();
        }

        if (oreCtx == nullptr)
            return nullptr;

        // The view's SRV leads to the texture resource, and the resource to its format
        auto* srv = shaderResourceViewOf (texture->getOrCreateView (*oreCtx, false, view).get());
        if (srv == nullptr)
            return nullptr;

        ComPtr<ID3D11Resource> resource;
        srv->GetResource (resource.ReleaseAndGetAddressOf());

        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc {};

        if (view.dimension == GpuTextureViewDimension::texture3D)
        {
            ComPtr<ID3D11Texture3D> texture3D;
            if (FAILED (resource.As (&texture3D)))
                return nullptr;

            D3D11_TEXTURE3D_DESC desc {};
            texture3D->GetDesc (&desc);

            uavDesc.Format = desc.Format;
            uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            uavDesc.Texture3D.MipSlice = view.baseMipLevel;
            uavDesc.Texture3D.FirstWSlice = 0;
            uavDesc.Texture3D.WSize = static_cast<UINT> (-1);
        }
        else
        {
            ComPtr<ID3D11Texture2D> texture2D;
            if (FAILED (resource.As (&texture2D)))
                return nullptr;

            D3D11_TEXTURE2D_DESC desc {};
            texture2D->GetDesc (&desc);

            uavDesc.Format = desc.Format;

            if (view.dimension == GpuTextureViewDimension::texture2D)
            {
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                uavDesc.Texture2D.MipSlice = view.baseMipLevel;
            }
            else
            {
                uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                uavDesc.Texture2DArray.MipSlice = view.baseMipLevel;
                uavDesc.Texture2DArray.FirstArraySlice = view.baseLayer;
                uavDesc.Texture2DArray.ArraySize = view.layerCount;
            }
        }

        ComPtr<ID3D11UnorderedAccessView> uav;
        if (FAILED (device->CreateUnorderedAccessView (resource.Get(), &uavDesc, uav.ReleaseAndGetAddressOf())) || uav == nullptr)
            return nullptr;

        storageUavs.push_back ({ texture, view, uav });
        return uav.Get();
    }

    ID3D11Device* device;
    ID3D11DeviceContext* context;
    rive::ore::Context* oreCtx;
    std::vector<ComPtr<ID3D11Buffer>> tempBuffers;
    std::vector<UINT> boundUavSlots;
    std::vector<UINT> boundSrvSlots;
    std::vector<rive::rcp<rive::ore::TextureView>> boundViews;
    std::vector<StorageUav> storageUavs;
};

//==============================================================================

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplD3D11 (GpuDevice& ctx)
{
    auto& d3dCtx = static_cast<GpuDeviceD3D&> (ctx);
    return std::make_unique<GpuComputePassImplD3D11> (d3dCtx.getD3DDevice(),
                                                      d3dCtx.getD3DDeviceContext(),
                                                      d3dCtx.getGpuContext());
}

} // namespace yup

#endif // YUP_RIVE_USE_D3D
