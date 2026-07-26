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

class GpuComputePipelineD3D11 final : public GpuComputePipeline
{
public:
    GpuComputePipelineD3D11 (ComPtr<ID3D11ComputeShader> shader, GpuWorkgroupSize wgs)
        : computeShader (std::move (shader))
        , workgroupSize_ (wgs)
    {
    }

    GpuWorkgroupSize getWorkgroupSize() const noexcept override { return workgroupSize_; }

    ID3D11ComputeShader* getComputeShader() const noexcept { return computeShader.Get(); }

private:
    ComPtr<ID3D11ComputeShader> computeShader;
    GpuWorkgroupSize workgroupSize_;
};

//==============================================================================

ResultValue<GpuComputePipeline::Ptr> yup_constructComputePipelineD3D11 (GpuDevice& ctx,
                                                                        const GpuShaderSource& source,
                                                                        const GpuWorkgroupSize& workgroupSize)
{
    if (source.code == nullptr || source.codeSize == 0)
        return makeResultValueFail ("Compute shader source is empty");

    if (source.language != GpuShaderLanguage::hlsl)
        return makeResultValueFail ("D3D11 compute shaders must be HLSL");

    auto& d3dCtx = static_cast<GpuDeviceD3D&> (ctx);

    ComPtr<ID3D11ComputeShader> computeShader;
    HRESULT hr = d3dCtx.getD3DDevice()->CreateComputeShader (source.code, source.codeSize, nullptr, computeShader.ReleaseAndGetAddressOf());
    if (FAILED (hr) || computeShader == nullptr)
        return makeResultValueFail ("D3D11 compute shader compilation failed");

    return makeResultValueOk (GpuComputePipeline::Ptr (new GpuComputePipelineD3D11 (std::move (computeShader), workgroupSize)));
}

} // namespace yup

#endif // YUP_RIVE_USE_D3D
