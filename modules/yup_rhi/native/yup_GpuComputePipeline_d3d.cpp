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
        , workgroupSize (wgs)
    {
    }

    GpuWorkgroupSize getWorkgroupSize() const noexcept override { return workgroupSize; }

    ID3D11ComputeShader* getComputeShader() const noexcept { return computeShader.Get(); }

private:
    ComPtr<ID3D11ComputeShader> computeShader;
    GpuWorkgroupSize workgroupSize;
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

    std::string hlsl (static_cast<const char*> (source.code), source.codeSize);
    hlsl.erase (std::remove (hlsl.begin(), hlsl.end(), '\r'), hlsl.end());

    const char* entryPoint = source.entryPoint != nullptr ? source.entryPoint : "main";

    ComPtr<ID3DBlob> compiledBlob;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile (hlsl.data(), hlsl.size(), nullptr, nullptr, nullptr,
                             entryPoint, "cs_5_0",
                             D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3,
                             0, compiledBlob.ReleaseAndGetAddressOf(), errorBlob.ReleaseAndGetAddressOf());
    if (FAILED (hr) || compiledBlob == nullptr)
    {
        String errorMessage = "D3D11 compute shader compilation failed";

        if (errorBlob != nullptr && errorBlob->GetBufferSize() > 0)
            errorMessage << ": " << static_cast<const char*> (errorBlob->GetBufferPointer());

        return makeResultValueFail (errorMessage);
    }

    ComPtr<ID3D11ComputeShader> computeShader;
    hr = d3dCtx.getD3DDevice()->CreateComputeShader (compiledBlob->GetBufferPointer(),
                                                     compiledBlob->GetBufferSize(),
                                                     nullptr,
                                                     computeShader.ReleaseAndGetAddressOf());
    if (FAILED (hr) || computeShader == nullptr)
        return makeResultValueFail ("D3D11 compute shader creation failed");

    return makeResultValueOk (GpuComputePipeline::Ptr (new GpuComputePipelineD3D11 (std::move (computeShader), workgroupSize)));
}

} // namespace yup

#endif // YUP_RIVE_USE_D3D
