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

namespace
{

//==============================================================================
// Shader bundles built with the module, see shaderBundles in yup_renderer.h: the template
// (shaders/yup_MaterialShader.frag) compiled around its stock material

constexpr uint8_t materialShaderBundle[] = {
#include "yup_renderer_shaders/yup_MaterialShader.inc"
};
constexpr uint8_t materialShaderFullBundle[] = {
#include "yup_renderer_shaders/yup_MaterialShader_tier_full.inc"
};

// The last MaterialShader::getIdentity() given: shaders are created on any thread
std::atomic<uint64> lastMaterialShaderIdentity { 0 };

#if YUP_ENABLE_SHADER_TRANSPILER
// The folder an application gave with MaterialShader::setSourceDirectory(), a default File without one
File& getMaterialShaderSourceDirectory()
{
    static File directory;
    return directory;
}
#endif

/** Checks that two stages are the template compiled around a material for one tier, whose fragment
    stage writes @p numOutputs outputs, and returns the uniforms of the block the material
    declares, empty without one.
*/
ResultValue<ShaderUniforms> validateMaterialStages (const ShaderReflection& vertex, const ShaderReflection& reflection, const char* tier, size_t numOutputs)
{
    const auto declaresBlock = [] (const ShaderReflection& stage, uint32 set, uint32 binding)
    {
        return std::any_of (stage.uniformBuffers.begin(), stage.uniformBuffers.end(), [set, binding] (const auto& block)
        {
            return block.set == set && block.binding == binding;
        });
    };

    // The template reads the DrawBlock in its vertex stage and the MaterialBlock in its fragment
    // stage, which writes the color, and the motion with it on the full tier
    if (! declaresBlock (vertex, 2, 0) || ! declaresBlock (reflection, 2, 1) || reflection.stageOutputs.size() != numOutputs)
        return makeResultValueFail (String ("The ") + tier + " variant is not the material shader template compiled for the " + tier + " tier");

    ShaderUniforms uniforms;

    for (const auto& block : reflection.uniformBuffers)
    {
        // The blocks of the template: the view, the lighting and the material of the mesh
        if ((block.set == 0 && block.binding == 0) || (block.set == 1 && block.binding == 0) || (block.set == 2 && block.binding == 1))
            continue;

        if (block.set != 2 || block.binding != MaterialShader::uniformBlockBinding)
            return makeResultValueFail ("The material declares the uniform block " + block.name + " at set " + String (block.set) + ", binding " + String (block.binding)
                                        + ": it has one block at most, opened with YUP_MATERIAL_PARAMS");

        // A bundle can come from a file: a block whose values uniforms cannot hold would be left unbound
        uniforms = ShaderUniforms (block);

        if (uniforms.isEmpty())
            return makeResultValueFail ("The material declares a uniform block of " + String (block.blockSize) + " bytes: it takes from 1 to "
                                        + String (static_cast<int> (ShaderUniforms::maxSizeInBytes)) + " bytes");
    }

    // The textures of the template are the five of the material of the mesh, at set 2 bindings 2 to 6:
    // any other is one the material declares
    const auto isOwnTexture = [] (const ShaderReflection::ResourceBinding& texture)
    {
        return texture.set != 2 || texture.binding < 2 || texture.binding > 6;
    };

    // GLSL and ESSL variants also list the samplers they fold each texture and sampler into
    if (reflection.sampledImages.size() > reflection.glCombinedSamplers.size()
        || std::any_of (reflection.separateImages.begin(), reflection.separateImages.end(), isOwnTexture))
        return makeResultValueFail ("The material declares a texture, which nothing binds: it reads the textures of the Material of the mesh through its YupSurface");

    // Nor does anything bind the other resources a stage can declare, none of which the template has
    for (const auto* stage : { &vertex, &reflection })
    {
        for (const auto& [resources, kind] : { std::pair { &stage->storageBuffers, "storage buffer" },
                                               std::pair { &stage->storageImages, "storage image" },
                                               std::pair { &stage->subpassInputs, "subpass input" },
                                               std::pair { &stage->pushConstantBuffers, "push constant block" } })
        {
            if (! resources->empty())
                return makeResultValueFail (String ("The material declares the ") + kind + " " + resources->front().name
                                            + ", which nothing binds: it takes its values through the uniform block YUP_MATERIAL_PARAMS opens");
        }
    }

    return makeResultValueOk (std::move (uniforms));
}

/** Checks that @p bundle is the template compiled around a material for one tier, as
    validateMaterialStages() does, and returns the uniforms of the block the material declares.

    The first variant of each stage tells what it declares. A bundle compiled here for one language
    that is not GLSL, which names some members its own way, is told as its GLSL variant would tell
    it, from the SPIR-V both are made of: the material then has the same uniform block whichever
    graphics API it is compiled for.
*/
ResultValue<ShaderUniforms> validateMaterialVariant (const ShaderBundle& bundle, const char* tier, size_t numOutputs, [[maybe_unused]] bool describedAsGlsl)
{
#if YUP_ENABLE_SHADER_TRANSPILER
    const auto* vertexSpirv = describedAsGlsl ? bundle.getSPIRV (ShaderStage::vertex) : nullptr;
    const auto* fragmentSpirv = describedAsGlsl ? bundle.getSPIRV (ShaderStage::fragment) : nullptr;

    if (vertexSpirv != nullptr && fragmentSpirv != nullptr)
    {
        ShaderTranspiler transpiler;

        const auto vertex = transpiler.reflectFromSPIRV (*vertexSpirv, ShaderLanguage::glsl);
        const auto fragment = transpiler.reflectFromSPIRV (*fragmentSpirv, ShaderLanguage::glsl);

        if (vertex.failed() || fragment.failed())
            return makeResultValueFail ((vertex.failed() ? vertex : fragment).getErrorMessage());

        return validateMaterialStages (vertex.getReference(), fragment.getReference(), tier, numOutputs);
    }
#endif

    // Every variant of a stage reflects the same shader, so the first one tells what it declares
    const auto findStage = [&bundle] (ShaderStage stage) -> const ShaderInfo*
    {
        for (const auto& shader : bundle.getShaders())
        {
            if (shader.stage == stage)
                return &shader;
        }

        return nullptr;
    };

    const auto* vertex = findStage (ShaderStage::vertex);
    if (vertex == nullptr)
        return makeResultValueFail (String ("The ") + tier + " variant of the material shader has no vertex stage");

    const auto* fragment = findStage (ShaderStage::fragment);
    if (fragment == nullptr)
        return makeResultValueFail (String ("The ") + tier + " variant of the material shader has no fragment stage");

    return validateMaterialStages (vertex->reflection, fragment->reflection, tier, numOutputs);
}

} // namespace

//==============================================================================
MaterialShader::MaterialShader()
    : identity (++lastMaterialShaderIdentity)
{
}

//==============================================================================
ResultValue<MaterialShader::Ptr> MaterialShader::createFromBundles (ShaderBundle::Ptr rasterBundle, ShaderBundle::Ptr fullBundle)
{
    return createFromVariants (std::move (rasterBundle), std::move (fullBundle), false);
}

ResultValue<MaterialShader::Ptr> MaterialShader::createFromVariants (ShaderBundle::Ptr rasterBundle, ShaderBundle::Ptr fullBundle, bool describedAsGlsl)
{
    if (rasterBundle == nullptr && fullBundle == nullptr)
        return makeResultValueFail ("No shader bundle");

    Ptr shader (new MaterialShader());
    bool hasUniforms = false;

    for (const auto& [bundle, tier, numOutputs] : { std::tuple { rasterBundle, "raster", size_t (1) }, std::tuple { fullBundle, "full", size_t (2) } })
    {
        if (bundle == nullptr)
            continue;

        auto uniforms = validateMaterialVariant (*bundle, tier, numOutputs, describedAsGlsl);
        if (uniforms.failed())
            return makeResultValueFail (uniforms.getErrorMessage());

        // A node holds one set of values, bound on either tier
        if (hasUniforms && uniforms.getReference() != shader->uniforms)
            return makeResultValueFail ("The raster and the full variant of the material shader declare different uniform blocks");

        shader->uniforms = std::move (uniforms.getReference());
        hasUniforms = true;
    }

    shader->rasterBundle = std::move (rasterBundle);
    shader->fullBundle = std::move (fullBundle);
    return makeResultValueOk (std::move (shader));
}

ResultValue<MaterialShader::Ptr> MaterialShader::createFromData (const void* rasterData, size_t rasterSize, const void* fullData, size_t fullSize)
{
    const auto load = [] (const void* data, size_t size) -> ResultValue<ShaderBundle::Ptr>
    {
        if (data == nullptr || size == 0)
            return makeResultValueOk (ShaderBundle::Ptr());

        auto loaded = ShaderBundle::loadFromData (data, size);
        if (loaded.failed())
            return makeResultValueFail (loaded.getErrorMessage());

        return makeResultValueOk (ShaderBundle::Ptr (new ShaderBundle (std::move (loaded.getReference()))));
    };

    auto raster = load (rasterData, rasterSize);
    if (raster.failed())
        return makeResultValueFail (raster.getErrorMessage());

    auto full = load (fullData, fullSize);
    if (full.failed())
        return makeResultValueFail (full.getErrorMessage());

    return createFromBundles (raster.getReference(), full.getReference());
}

#if YUP_ENABLE_SHADER_TRANSPILER

ResultValue<MaterialShader::Ptr> MaterialShader::createFromSource (const String& source, const File& shaderDirectory, std::optional<GpuPlatform> platform)
{
    // Where shaders/yup_MaterialShader.frag includes the material file
    static constexpr const char* sourceInclude = "#include \"yup_MaterialSource.glsl\"";

    const auto loadStage = [&shaderDirectory] (const char* fileName)
    {
        return shaderDirectory.isDirectory() ? shaderDirectory.getChildFile (fileName).loadFileAsString() : String();
    };

    const auto vertex = loadStage ("yup_MaterialShader.vert");
    const auto fragment = loadStage ("yup_MaterialShader.frag");

    if (vertex.isEmpty() || ! fragment.contains (sourceInclude))
        return makeResultValueFail ("The material shader template was not found in \"" + shaderDirectory.getFullPathName()
                                    + "\": a material is compiled from its source where the shaders of yup_renderer are, and created from its compiled variants elsewhere");

    // The material takes the place of its #include as an included file would: its lines are
    // numbered from 1 under a name of its own, and the template goes on from its next line
    const auto templateStart = fragment.upToFirstOccurrenceOf (sourceInclude, false, false).toStdString();
    const auto includeLine = 1 + static_cast<int> (std::count (templateStart.begin(), templateStart.end(), '\n'));
    const auto materialFragment = fragment.replace (sourceInclude, "#line 1 \"material\"\n" + source + "\n#line " + String (includeLine + 1) + " 0");

    // The language of the graphics API that is to draw the material, or the ones yup_shader_bundler targets by default
    const auto languages = platform.has_value()
                             ? std::vector<ShaderLanguage> { GpuImageShader::getShaderLanguage (*platform) }
                             : std::vector<ShaderLanguage> { ShaderLanguage::glsl, ShaderLanguage::essl, ShaderLanguage::hlsl, ShaderLanguage::msl, ShaderLanguage::spirv, ShaderLanguage::wgsl };

    const auto compile = [&] (bool full) -> ResultValue<ShaderBundle::Ptr>
    {
        ShaderBundleCompileRequest request;

        // Both stages go in one request
        for (const auto& [stage, stageSource] : { std::pair { ShaderStage::vertex, vertex }, std::pair { ShaderStage::fragment, materialFragment } })
        {
            auto& entry = request.entries.emplace_back();
            entry.stage = stage;
            entry.source = stageSource;
            entry.targetLanguages = languages;
            entry.options.includePaths.push_back (shaderDirectory.getFullPathName());
            entry.options.defines.set ("YUP_3D_MATERIAL_SOURCE", {});

            if (full)
                entry.options.defines.set ("YUP_3D_TIER_FULL", {});
        }

        auto compiled = ShaderBundleCompiler().compile (request);
        if (compiled.failed())
            return makeResultValueFail (compiled.getErrorMessage());

        return makeResultValueOk (ShaderBundle::Ptr (new ShaderBundle (std::move (compiled.getReference()))));
    };

    auto raster = compile (false);
    if (raster.failed())
        return makeResultValueFail (raster.getErrorMessage());

    auto full = compile (true);
    if (full.failed())
        return makeResultValueFail (full.getErrorMessage());

    return createFromVariants (raster.getReference(), full.getReference(), languages.front() != ShaderLanguage::glsl);
}

File MaterialShader::getSourceDirectory()
{
    if (const auto& directory = getMaterialShaderSourceDirectory(); directory != File())
        return directory;

    // This file is compiled from a folder beside the shaders of the module
    const String thisFile (__FILE__);
    return File::isAbsolutePath (thisFile) ? File (thisFile).getParentDirectory().getSiblingFile ("shaders") : File();
}

void MaterialShader::setSourceDirectory (const File& directory)
{
    getMaterialShaderSourceDirectory() = directory;
}

#endif

MaterialShader::Ptr MaterialShader::createDefault()
{
    auto created = createFromData (materialShaderBundle, sizeof (materialShaderBundle), materialShaderFullBundle, sizeof (materialShaderFullBundle));

    // The bundles are built with the module, from the template this class checks them against
    jassert (created.wasOk());
    return created.wasOk() ? created.getValue() : nullptr;
}

} // namespace yup
