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

    const auto sourceOf = [&request] (const ShaderBundleEntry& entry) -> const String&
    {
        return entry.source.isNotEmpty() ? entry.source : request.source;
    };

    // Compile source → SPIR-V
    std::vector<MemoryBlock> spirvs;
    spirvs.reserve (request.entries.size());

    for (const auto& entry : request.entries)
    {
        auto spirvResult = transpiler->compileToSPIRV (sourceOf (entry),
                                                       entry.stage,
                                                       request.sourceLanguage,
                                                       entry.options);
        if (spirvResult.failed())
            return makeResultValueFail (String ("ShaderBundleCompiler: SPIR-V compilation failed for stage ")
                                        + toString (entry.stage)
                                        + ": " + spirvResult.getErrorMessage());

        spirvs.push_back (std::move (spirvResult.getValue()));
    }

    // A vertex and a fragment stage form one graphics pipeline, whose native slots are assigned together
    const auto stageIndex = [&request] (ShaderStage stage)
    {
        const auto it = std::find_if (request.entries.begin(), request.entries.end(), [stage] (const ShaderBundleEntry& e)
        {
            return e.stage == stage;
        });

        return static_cast<size_t> (std::distance (request.entries.begin(), it));
    };

    for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
    {
        const auto count = std::count_if (request.entries.begin(), request.entries.end(), [stage] (const ShaderBundleEntry& e)
        {
            return e.stage == stage;
        });

        if (count > 1)
            return makeResultValueFail (String ("ShaderBundleCompiler: a request holds one pipeline, but it has more than one ")
                                        + toString (stage) + " entry");
    }

    const auto vertexIndex = stageIndex (ShaderStage::vertex);
    const auto fragmentIndex = stageIndex (ShaderStage::fragment);
    const bool isGraphicsPipeline = vertexIndex < request.entries.size() && fragmentIndex < request.entries.size();

    ShaderReflection vertexReflection;
    ShaderReflection fragmentReflection;

    if (isGraphicsPipeline)
    {
        auto vertexResult = transpiler->reflectFromSPIRV (spirvs[vertexIndex]);
        auto fragmentResult = transpiler->reflectFromSPIRV (spirvs[fragmentIndex]);

        if (vertexResult.failed() || fragmentResult.failed())
            return makeResultValueFail (String ("ShaderBundleCompiler: pipeline reflection failed: ")
                                        + (vertexResult.failed() ? vertexResult : fragmentResult).getErrorMessage());

        vertexReflection = std::move (vertexResult.getValue());
        fragmentReflection = std::move (fragmentResult.getValue());
    }

    for (size_t entryIndex = 0; entryIndex < request.entries.size(); ++entryIndex)
    {
        const auto& entry = request.entries[entryIndex];
        const auto& spirv = spirvs[entryIndex];
        bundle.setSPIRV (entry.stage, request.sourceLanguage, spirv);

        // Decompile + reflect for each target language
        for (const auto targetLang : entry.targetLanguages)
        {
            ShaderInfo info;
            info.stage = entry.stage;
            info.language = targetLang;
            info.entryPoint = entry.options.entryPoint;
            info.inputSource = sourceOf (entry);

            if (targetLang == ShaderLanguage::spirv)
            {
                // Vulkan's clip space is Y-down, so its module is compiled again with the flip baked in
                auto vulkanOptions = entry.options;
                vulkanOptions.spirvInvertY = true;

                auto vulkanResult = transpiler->compileToSPIRV (sourceOf (entry), entry.stage, request.sourceLanguage, vulkanOptions);
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

            const bool isPipelineStage = entry.stage == ShaderStage::vertex || entry.stage == ShaderStage::fragment;

            const auto nativeSlots = isGraphicsPipeline && isPipelineStage
                                       ? ShaderTranspiler::assignGraphicsNativeSlots (vertexReflection, fragmentReflection, targetLang)
                                       : ShaderNativeSlots {};

            const auto* slots = nativeSlots.entries.empty() ? nullptr : &nativeSlots;

            auto srcValue = [&]() -> ResultValue<String>
            {
                if (targetLang == ShaderLanguage::wgsl)
                    return transpiler->transpile (sourceOf (entry), entry.stage, request.sourceLanguage, targetLang, entry.options);

                return transpiler->decompileFromSPIRV (spirv, targetLang, entry.options, slots);
            }();

            if (srcValue.failed())
                return makeResultValueFail (String ("ShaderBundleCompiler: transpile to ")
                                            + toString (targetLang)
                                            + " failed: " + srcValue.getErrorMessage());

            auto reflResult = transpiler->reflectFromSPIRV (spirv, targetLang, entry.options, slots);
            if (reflResult.failed())
                return makeResultValueFail (String ("ShaderBundleCompiler: reflection for ")
                                            + toString (targetLang)
                                            + " failed: " + reflResult.getErrorMessage());

            // Metal binds vertex buffers from index 16 up (ore's kMetalVertexBufferBase), so the
            // pipeline's buffers must stay below it
            constexpr uint32_t metalVertexBufferBase = 16;

            if (slots != nullptr && targetLang == ShaderLanguage::msl)
            {
                for (const auto* buffers : { &reflResult.getReference().uniformBuffers, &reflResult.getReference().storageBuffers })
                {
                    for (const auto& b : *buffers)
                    {
                        if (b.backendSlot != ~0u && b.backendSlot + jmax (1u, b.descriptorCount) > metalVertexBufferBase)
                            return makeResultValueFail (String ("ShaderBundleCompiler: buffer ") + b.name + " of the "
                                                        + toString (entry.stage) + " stage needs Metal buffer slot "
                                                        + String (b.backendSlot) + ", but a pipeline has only "
                                                        + String (metalVertexBufferBase) + " buffer slots below its vertex buffers");
                    }
                }
            }

            info.source = srcValue.getValue();
            info.reflection = std::move (reflResult.getValue());

            bundle.addShader (std::move (info));
        }
    }

    return makeResultValueOk (std::move (bundle));
}

} // namespace yup
