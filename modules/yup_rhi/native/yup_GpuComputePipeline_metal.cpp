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

#if YUP_RIVE_USE_METAL && (YUP_MAC || YUP_IOS)

namespace yup
{

//==============================================================================

class GpuComputePipelineMetal final : public GpuComputePipeline
{
public:
    GpuComputePipelineMetal (id<MTLComputePipelineState> state, GpuWorkgroupSize wgs)
        : pipelineState (state)
        , workgroupSize (wgs)
    {
    }

    GpuWorkgroupSize getWorkgroupSize() const noexcept override { return workgroupSize; }

    id<MTLComputePipelineState> getPipelineState() const noexcept { return pipelineState; }

private:
    id<MTLComputePipelineState> pipelineState;
    GpuWorkgroupSize workgroupSize;
};

//==============================================================================

ResultValue<GpuComputePipeline::Ptr> yup_constructComputePipelineMetal (GpuDevice& ctx,
                                                                        const GpuShaderSource& source,
                                                                        const GpuWorkgroupSize& workgroupSize)
{
    if (source.code == nullptr || source.codeSize == 0)
        return makeResultValueFail ("Compute shader source is empty");

    auto& metalCtx = static_cast<GpuDeviceMetal&> (ctx);
    id<MTLDevice> device = metalCtx.getDevice();

    NSString* mslSource = [[NSString alloc] initWithBytes:source.code
                                                   length:source.codeSize
                                                 encoding:NSUTF8StringEncoding];
    if (mslSource == nil)
        return makeResultValueFail ("Failed to create MSL source string");

    MTLCompileOptions* compileOptions = [[MTLCompileOptions alloc] init];

    NSError* error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:mslSource
                                                  options:compileOptions
                                                    error:&error];

    if (library == nil)
    {
        String errMsg = "Metal compute shader compilation failed: ";
        errMsg += error != nil ? [error.localizedDescription UTF8String] : "unknown error";
        return makeResultValueFail (errMsg);
    }

    const char* entryPointName = source.entryPoint != nullptr ? source.entryPoint : "main0";
    NSString* entryPoint = [NSString stringWithUTF8String:entryPointName];

    id<MTLFunction> function = [library newFunctionWithName:entryPoint];

    if (function == nil)
        return makeResultValueFail (String ("Metal compute function not found: ") + entryPointName);

    id<MTLComputePipelineState> pipelineState = [device newComputePipelineStateWithFunction:function
                                                                                      error:&error];

    if (pipelineState == nil)
    {
        String errMsg = "Metal compute pipeline creation failed: ";
        errMsg += error != nil ? [error.localizedDescription UTF8String] : "unknown error";
        return makeResultValueFail (errMsg);
    }

    return makeResultValueOk (GpuComputePipeline::Ptr (new GpuComputePipelineMetal (pipelineState, workgroupSize)));
}

} // namespace yup

#endif // YUP_RIVE_USE_METAL
