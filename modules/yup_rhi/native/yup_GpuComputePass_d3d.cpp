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

namespace yup
{

//==============================================================================

class GpuComputePassImplD3D11 final : public GpuComputePass::Impl
{
public:
    GpuComputePassImplD3D11 (ID3D11Device* dev, ID3D11DeviceContext* ctx)
        : device (dev)
        , context (ctx)
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

            UINT slot = static_cast<UINT> (sb.group * 16 + sb.binding);
            ID3D11UnorderedAccessView* uavs[1] = { bufImpl->d3dUav.Get() };
            context->CSSetUnorderedAccessViews (slot, 1, uavs, nullptr);
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

            UINT slot = static_cast<UINT> (ub.group * 16 + ub.binding);
            context->CSSetConstantBuffers (slot, 1, cb.GetAddressOf());
            tempBuffers.push_back (std::move (cb));
        }

        context->Dispatch (groupsX, groupsY, groupsZ);
        return true;
    }

    //==========================================================================

    void finish() override
    {
        tempBuffers.clear();
        device = nullptr;
        context = nullptr;
    }

private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    std::vector<ComPtr<ID3D11Buffer>> tempBuffers;
};

//==============================================================================

std::unique_ptr<GpuComputePass::Impl> yup_createComputePassImplD3D11 (GpuDevice& ctx)
{
    auto& d3dCtx = static_cast<GpuDeviceD3D&> (ctx);
    return std::make_unique<GpuComputePassImplD3D11> (d3dCtx.getD3DDevice(),
                                                      d3dCtx.getD3DDeviceContext());
}

} // namespace yup

#endif // YUP_RIVE_USE_D3D
