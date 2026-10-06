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

namespace yup
{

ShaderBundleCompiler::ShaderBundleCompiler (ShaderTranspiler::Ptr transpilerToUse)
    : transpiler (transpilerToUse != nullptr ? std::move (transpilerToUse)
                                             : ShaderTranspiler::Ptr { new ShaderTranspiler() })
{
}

ShaderBundleCompiler::~ShaderBundleCompiler() = default;

ResultValue<ShaderBundle> ShaderBundleCompiler::compile (const ShaderBundleCompileRequest& request)
{
    ShaderBundle bundle;

    for (const auto& entry : request.entries)
    {
        // Compile source → SPIR-V
        auto spirvResult = transpiler->compileToSPIRV (request.source,
                                                       entry.stage,
                                                       request.sourceLanguage,
                                                       entry.options);
        if (spirvResult.failed())
            return makeResultValueFail (String ("ShaderBundleCompiler: SPIR-V compilation failed for stage ")
                                        + toString (entry.stage)
                                        + ": " + spirvResult.getErrorMessage());

        const auto& spirv = spirvResult.getValue();
        bundle.setSPIRV (entry.stage, request.sourceLanguage, spirv);

        // Decompile + reflect for each target language
        for (const auto targetLang : entry.targetLanguages)
        {
            ShaderInfo info;
            info.stage = entry.stage;
            info.language = targetLang;
            info.entryPoint = entry.options.entryPoint;
            info.inputSource = request.source;

            if (targetLang == ShaderLanguage::spirv)
            {
                // Vulkan's clip space is Y-down, so its module is compiled again with the flip baked in
                auto vulkanOptions = entry.options;
                vulkanOptions.spirvInvertY = true;

                auto vulkanResult = transpiler->compileToSPIRV (request.source, entry.stage, request.sourceLanguage, vulkanOptions);
                if (vulkanResult.failed())
                    return makeResultValueFail (String ("ShaderBundleCompiler: SPIR-V compilation for Vulkan failed for stage ")
                                                + toString (entry.stage)
                                                + ": " + vulkanResult.getErrorMessage());

                auto reflResult = transpiler->reflectFromSPIRV (vulkanResult.getValue(), targetLang, entry.options);
                if (reflResult.failed())
                    return makeResultValueFail (String ("ShaderBundleCompiler: reflection for spirv failed: ") + reflResult.getErrorMessage());

                info.binary = std::move (vulkanResult.getValue());
                info.reflection = std::move (reflResult.getValue());

                bundle.addShader (std::move (info));
                continue;
            }

            auto srcValue = [&]() -> ResultValue<String>
            {
                if (targetLang == ShaderLanguage::wgsl)
                    return transpiler->transpile (request.source, entry.stage, request.sourceLanguage, targetLang, entry.options);

                return transpiler->decompileFromSPIRV (spirv, targetLang, entry.options);
            }();

            if (srcValue.failed())
                return makeResultValueFail (String ("ShaderBundleCompiler: transpile to ")
                                            + toString (targetLang)
                                            + " failed: " + srcValue.getErrorMessage());

            auto reflResult = transpiler->reflectFromSPIRV (spirv, targetLang, entry.options);
            if (reflResult.failed())
                return makeResultValueFail (String ("ShaderBundleCompiler: reflection for ")
                                            + toString (targetLang)
                                            + " failed: " + reflResult.getErrorMessage());

            info.source = srcValue.getValue();
            info.reflection = std::move (reflResult.getValue());

            bundle.addShader (std::move (info));
        }
    }

    return makeResultValueOk (std::move (bundle));
}

} // namespace yup
