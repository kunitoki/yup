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

#include <gtest/gtest.h>

#include <yup_renderer/yup_renderer.h>

using namespace yup;

/** CPU only: creating a MaterialShader from its variants, and from source with the transpiler.
    What it draws is tested on Metal, in native/yup_MaterialShader_apple.cpp. */
class MaterialShaderTests : public ::testing::Test
{
protected:
    /** The reflection of a uniform block holding one `vec4 rim`. */
    static ShaderReflection::ResourceBinding makeBlock (const String& name, uint32 set, uint32 binding)
    {
        ShaderReflection::ResourceMember rim;
        rim.name = "rim";
        rim.baseType = ShaderReflection::BaseType::float32;
        rim.vecSize = 4;
        rim.size = 16;

        ShaderReflection::ResourceBinding block;
        block.name = name;
        block.type = ShaderReflection::ResourceType::uniformBuffer;
        block.set = set;
        block.binding = binding;
        block.members = { rim };
        block.blockSize = 16;
        return block;
    }

    /** The reflection of a `texture2D`. */
    static ShaderReflection::ResourceBinding makeTexture (const String& name, uint32 set, uint32 binding)
    {
        ShaderReflection::ResourceBinding texture;
        texture.name = name;
        texture.type = ShaderReflection::ResourceType::separateImage;
        texture.set = set;
        texture.binding = binding;
        texture.baseType = ShaderReflection::BaseType::image;
        texture.imageDim = ShaderReflection::ImageDimension::dim2D;
        return texture;
    }

    /** A bundle holding one variant of each stage given. */
    static ShaderBundle::Ptr makeBundle (std::optional<ShaderReflection> vertex, std::optional<ShaderReflection> fragment)
    {
        ShaderBundle::Ptr bundle (new ShaderBundle());

        for (auto& [stage, reflection] : { std::pair { ShaderStage::vertex, vertex }, std::pair { ShaderStage::fragment, fragment } })
        {
            if (! reflection.has_value())
                continue;

            ShaderInfo info;
            info.stage = stage;
            info.source = "void main() {}";
            info.reflection = *reflection;
            bundle->addShader (std::move (info));
        }

        return bundle;
    }

    /** What the vertex stage of the template declares. */
    static ShaderReflection makeVertexReflection()
    {
        ShaderReflection reflection;
        reflection.uniformBuffers = { makeBlock ("ViewBlock", 0, 0), makeBlock ("DrawBlock", 2, 0) };
        return reflection;
    }

    /** What the fragment stage of the template declares around a material: @p numOutputs outputs,
        its three blocks and the five textures of the material of the mesh. */
    static ShaderReflection makeFragmentReflection (size_t numOutputs)
    {
        ShaderReflection reflection;
        reflection.uniformBuffers = { makeBlock ("ViewBlock", 0, 0), makeBlock ("LightingBlock", 1, 0), makeBlock ("MaterialBlock", 2, 1) };

        for (uint32 binding = 2; binding <= 6; ++binding)
            reflection.separateImages.push_back (makeTexture ("u_texture", 2, binding));

        for (uint32 location = 0; location < numOutputs; ++location)
            reflection.stageOutputs.emplace_back().location = location;

        return reflection;
    }

    /** The template around a material declaring @p blocks and @p textures, for the raster tier
        (one output) or the full one (two). */
    static ShaderBundle::Ptr makeVariant (size_t numOutputs, std::vector<ShaderReflection::ResourceBinding> blocks = {}, std::vector<ShaderReflection::ResourceBinding> textures = {})
    {
        auto fragment = makeFragmentReflection (numOutputs);
        fragment.uniformBuffers.insert (fragment.uniformBuffers.end(), blocks.begin(), blocks.end());
        fragment.separateImages.insert (fragment.separateImages.end(), textures.begin(), textures.end());
        return makeBundle (makeVertexReflection(), fragment);
    }

    /** The reflection of a resource of another kind, which the template has none of. */
    static ShaderReflection::ResourceBinding makeResource (const String& name, ShaderReflection::ResourceType type)
    {
        ShaderReflection::ResourceBinding resource;
        resource.name = name;
        resource.type = type;
        resource.set = 2;
        resource.binding = 20;
        return resource;
    }

    /** The block a material opens with YUP_MATERIAL_PARAMS. */
    static ShaderReflection::ResourceBinding makeParams()
    {
        return makeBlock ("MaterialParams", 2, MaterialShader::uniformBlockBinding);
    }

    static String getFailure (const ResultValue<MaterialShader::Ptr>& created)
    {
        return created.failed() ? created.getErrorMessage() : String();
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** The shaders of the module, which a material is compiled inside. */
    static File getShaderDirectory()
    {
        return File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getChildFile ("modules/yup_renderer/shaders");
    }

    /** A material with a uniform block. */
    static constexpr const char* hologramSource = R"(YUP_MATERIAL_PARAMS
{
    vec4 rim;
    float time;
} params;

vec4 yupMaterial (YupSurface surface)
{
    float fresnel = pow (1.0 - max (dot (surface.normal, surface.view), 0.0), 3.0);
    return vec4 (params.rim.rgb * fresnel, fresnel * params.time);
}
)";

    /** A material without a block. */
    static constexpr const char* normalSource = R"(vec4 yupMaterial (YupSurface surface)
{
    return vec4 (surface.normal * 0.5 + 0.5, surface.baseColor.a);
}
)";
#endif
};

TEST_F (MaterialShaderTests, StockShaderDrawsOnBothTiersWithoutUniforms)
{
    const auto shader = MaterialShader::createDefault();
    ASSERT_NE (shader, nullptr);
    EXPECT_NE (shader->getRasterBundle(), nullptr);
    EXPECT_NE (shader->getFullBundle(), nullptr);
    EXPECT_TRUE (shader->createUniforms().isEmpty());
}

TEST_F (MaterialShaderTests, LifetimeExpiresWithTheShader)
{
    auto shader = MaterialShader::createDefault();
    ASSERT_NE (shader, nullptr);

    // Held by a node alone, then by nothing
    const auto lifetime = shader->getLifetime();
    MaterialShaderNode node (shader);
    shader = nullptr;
    EXPECT_FALSE (lifetime.expired());

    node.setShader (nullptr);
    EXPECT_TRUE (lifetime.expired());
}

TEST_F (MaterialShaderTests, IdentityIsNeverTheOneOfAnotherShader)
{
    // Not even of one that died, at whose address the next shader can be created
    const auto first = MaterialShader::createDefault()->getIdentity();
    const auto second = MaterialShader::createDefault();
    const auto third = MaterialShader::createDefault();

    EXPECT_NE (second->getIdentity(), first);
    EXPECT_NE (third->getIdentity(), first);
    EXPECT_NE (third->getIdentity(), second->getIdentity());
}

TEST_F (MaterialShaderTests, NodeHoldsItsOwnValuesOfTheUniformBlock)
{
    const auto created = MaterialShader::createFromBundles (makeVariant (1, { makeParams() }), makeVariant (2, { makeParams() }));
    ASSERT_TRUE (created.wasOk()) << getFailure (created);
    const auto shader = created.getValue();

    MaterialShaderNode first (shader), second (shader);
    EXPECT_EQ (first.getShader(), shader);
    EXPECT_TRUE (first.getUniforms().set ("rim", { 1.0f, 0.5f, 0.25f, 1.0f }));
    EXPECT_FALSE (first.getUniforms() == second.getUniforms());
    EXPECT_TRUE (second.getUniforms() == shader->createUniforms());

    // Another shader, or none, starts from its own zeroed values
    first.setShader (shader);
    EXPECT_TRUE (first.getUniforms() == shader->createUniforms());

    first.setShader (nullptr);
    EXPECT_EQ (first.getShader(), nullptr);
    EXPECT_TRUE (first.getUniforms().isEmpty());
    EXPECT_TRUE (MaterialShaderNode().getUniforms().isEmpty());
}

TEST_F (MaterialShaderTests, CreationWithoutAVariantFails)
{
    EXPECT_EQ (getFailure (MaterialShader::createFromBundles (nullptr, nullptr)), "No shader bundle");
    EXPECT_EQ (getFailure (MaterialShader::createFromData (nullptr, 0, nullptr, 0)), "No shader bundle");
}

TEST_F (MaterialShaderTests, VariantWithoutBothStagesFails)
{
    const auto noFragment = MaterialShader::createFromBundles (makeBundle (makeVertexReflection(), std::nullopt), nullptr);
    EXPECT_TRUE (getFailure (noFragment).contains ("raster variant")) << getFailure (noFragment);
    EXPECT_TRUE (getFailure (noFragment).contains ("no fragment stage")) << getFailure (noFragment);

    const auto noVertex = MaterialShader::createFromBundles (nullptr, makeBundle (std::nullopt, makeFragmentReflection (2)));
    EXPECT_TRUE (getFailure (noVertex).contains ("full variant")) << getFailure (noVertex);
    EXPECT_TRUE (getFailure (noVertex).contains ("no vertex stage")) << getFailure (noVertex);
}

TEST_F (MaterialShaderTests, VariantOfTheOtherTierIsRefused)
{
    // The stock bundles the wrong way around: the raster one writes no motion, the full one does
    const auto stock = MaterialShader::createDefault();
    ASSERT_NE (stock, nullptr);

    const auto swapped = MaterialShader::createFromBundles (stock->getFullBundle(), stock->getRasterBundle());
    EXPECT_TRUE (getFailure (swapped).contains ("not the material shader template compiled for the raster tier")) << getFailure (swapped);

    const auto rasterAsFull = MaterialShader::createFromBundles (nullptr, stock->getRasterBundle());
    EXPECT_TRUE (getFailure (rasterAsFull).contains ("not the material shader template compiled for the full tier")) << getFailure (rasterAsFull);

    EXPECT_TRUE (MaterialShader::createFromBundles (stock->getRasterBundle(), stock->getFullBundle()).wasOk());
}

TEST_F (MaterialShaderTests, BundleOfAnotherShaderIsRefused)
{
    // A fullscreen shader writes one color too, but reads neither the draw nor the material of a mesh
    ShaderReflection fragment;
    fragment.uniformBuffers = { makeBlock ("Parameters", 0, 0) };
    fragment.stageOutputs.emplace_back();

    const auto fullscreen = MaterialShader::createFromBundles (makeBundle (ShaderReflection(), fragment), nullptr);
    EXPECT_TRUE (getFailure (fullscreen).contains ("not the material shader template")) << getFailure (fullscreen);

    // The template without the draw of the mesh in its vertex stage
    const auto noDraw = MaterialShader::createFromBundles (makeBundle (ShaderReflection(), makeFragmentReflection (1)), nullptr);
    EXPECT_TRUE (getFailure (noDraw).contains ("not the material shader template")) << getFailure (noDraw);
}

TEST_F (MaterialShaderTests, ShaderWithOneVariantHasNoneForTheOtherTier)
{
    const auto rasterOnly = MaterialShader::createFromBundles (makeVariant (1), nullptr);
    ASSERT_TRUE (rasterOnly.wasOk()) << getFailure (rasterOnly);
    EXPECT_NE (rasterOnly.getReference()->getRasterBundle(), nullptr);
    EXPECT_EQ (rasterOnly.getReference()->getFullBundle(), nullptr);

    const auto fullOnly = MaterialShader::createFromBundles (nullptr, makeVariant (2));
    ASSERT_TRUE (fullOnly.wasOk()) << getFailure (fullOnly);
    EXPECT_EQ (fullOnly.getReference()->getRasterBundle(), nullptr);
    EXPECT_NE (fullOnly.getReference()->getFullBundle(), nullptr);
}

TEST_F (MaterialShaderTests, ReadsTheUniformBlockOfTheMaterial)
{
    const auto created = MaterialShader::createFromBundles (makeVariant (1, { makeParams() }), makeVariant (2, { makeParams() }));
    ASSERT_TRUE (created.wasOk()) << getFailure (created);

    auto uniforms = created.getReference()->createUniforms();
    EXPECT_EQ (uniforms.getSizeInBytes(), 16u);
    EXPECT_TRUE (uniforms.hasMember ("rim"));
    EXPECT_EQ (uniforms.getNumComponents ("rim"), 4);

    // Zeroed, and each copy its own
    EXPECT_TRUE (uniforms.set ("rim", { 1.0f, 0.5f, 0.25f, 1.0f }));
    EXPECT_FALSE (uniforms == created.getReference()->createUniforms());

    // A material without a block has none
    const auto plain = MaterialShader::createFromBundles (makeVariant (1), makeVariant (2));
    ASSERT_TRUE (plain.wasOk()) << getFailure (plain);
    EXPECT_TRUE (plain.getReference()->createUniforms().isEmpty());
}

TEST_F (MaterialShaderTests, BlockThatIsNotTheOneOfTheMaterialIsRefused)
{
    // A second block
    const auto second = MaterialShader::createFromBundles (makeVariant (1, { makeParams(), makeBlock ("Extra", 2, 15) }), nullptr);
    EXPECT_TRUE (getFailure (second).contains ("uniform block Extra at set 2, binding 15")) << getFailure (second);
    EXPECT_TRUE (getFailure (second).contains ("YUP_MATERIAL_PARAMS")) << getFailure (second);

    // One block, declared without the macro
    const auto elsewhere = MaterialShader::createFromBundles (nullptr, makeVariant (2, { makeBlock ("Parameters", 0, 5) }));
    EXPECT_TRUE (getFailure (elsewhere).contains ("uniform block Parameters at set 0, binding 5")) << getFailure (elsewhere);
}

TEST_F (MaterialShaderTests, BlockThatUniformsCannotHoldIsRefused)
{
    // As a bundle read from a file can declare it: no size, or more than ShaderUniforms::maxSizeInBytes
    for (const auto size : { 0u, 128u * 1024u })
    {
        auto block = makeParams();
        block.blockSize = size;

        const auto created = MaterialShader::createFromBundles (makeVariant (1, { block }), nullptr);
        EXPECT_TRUE (getFailure (created).contains ("uniform block of " + String (size) + " bytes")) << getFailure (created);
    }
}

TEST_F (MaterialShaderTests, TextureOfTheMaterialItselfIsRefused)
{
    const auto created = MaterialShader::createFromBundles (makeVariant (1, {}, { makeTexture ("noise", 2, 15) }), nullptr);
    EXPECT_TRUE (getFailure (created).contains ("declares a texture")) << getFailure (created);

    // A combined sampler2D too
    auto fragment = makeFragmentReflection (1);
    fragment.sampledImages.push_back (makeTexture ("noise", 2, 15));

    const auto combined = MaterialShader::createFromBundles (makeBundle (makeVertexReflection(), fragment), nullptr);
    EXPECT_TRUE (getFailure (combined).contains ("declares a texture")) << getFailure (combined);
}

TEST_F (MaterialShaderTests, StorageBufferOfTheMaterialIsRefused)
{
    auto fragment = makeFragmentReflection (1);
    fragment.storageBuffers.push_back (makeResource ("Palette", ShaderReflection::ResourceType::storageBuffer));

    const auto created = MaterialShader::createFromBundles (makeBundle (makeVertexReflection(), fragment), nullptr);
    EXPECT_TRUE (getFailure (created).contains ("storage buffer Palette, which nothing binds")) << getFailure (created);
}

TEST_F (MaterialShaderTests, StorageImageOfTheMaterialIsRefused)
{
    auto fragment = makeFragmentReflection (2);
    fragment.storageImages.push_back (makeResource ("canvas", ShaderReflection::ResourceType::storageImage));

    const auto created = MaterialShader::createFromBundles (nullptr, makeBundle (makeVertexReflection(), fragment));
    EXPECT_TRUE (getFailure (created).contains ("storage image canvas, which nothing binds")) << getFailure (created);
}

TEST_F (MaterialShaderTests, SubpassInputOfTheMaterialIsRefused)
{
    auto fragment = makeFragmentReflection (1);
    fragment.subpassInputs.push_back (makeResource ("previous", ShaderReflection::ResourceType::subpassInput));

    const auto created = MaterialShader::createFromBundles (makeBundle (makeVertexReflection(), fragment), nullptr);
    EXPECT_TRUE (getFailure (created).contains ("subpass input previous, which nothing binds")) << getFailure (created);
}

TEST_F (MaterialShaderTests, PushConstantBlockOfTheMaterialIsRefused)
{
    // In the vertex stage, which a bundle read from a file can hold anything in
    auto vertex = makeVertexReflection();
    vertex.pushConstantBuffers.push_back (makeResource ("Constants", ShaderReflection::ResourceType::pushConstant));

    const auto created = MaterialShader::createFromBundles (makeBundle (vertex, makeFragmentReflection (1)), nullptr);
    EXPECT_TRUE (getFailure (created).contains ("push constant block Constants, which nothing binds")) << getFailure (created);
}

TEST_F (MaterialShaderTests, VariantsDeclaringDifferentBlocksAreRefused)
{
    const auto created = MaterialShader::createFromBundles (makeVariant (1, { makeParams() }), makeVariant (2));
    EXPECT_TRUE (getFailure (created).contains ("different uniform blocks")) << getFailure (created);
}

TEST_F (MaterialShaderTests, LoadsTheBytesOfItsVariants)
{
    const auto stock = MaterialShader::createDefault();
    ASSERT_NE (stock, nullptr);

    MemoryBlock raster, full;
    ASSERT_TRUE (stock->getRasterBundle()->saveToMemoryBlock (raster).wasOk());
    ASSERT_TRUE (stock->getFullBundle()->saveToMemoryBlock (full).wasOk());

    const auto both = MaterialShader::createFromData (raster.getData(), raster.getSize(), full.getData(), full.getSize());
    ASSERT_TRUE (both.wasOk()) << getFailure (both);
    EXPECT_NE (both.getReference()->getRasterBundle(), nullptr);
    EXPECT_NE (both.getReference()->getFullBundle(), nullptr);

    const auto fullOnly = MaterialShader::createFromData (nullptr, 0, full.getData(), full.getSize());
    ASSERT_TRUE (fullOnly.wasOk()) << getFailure (fullOnly);
    EXPECT_EQ (fullOnly.getReference()->getRasterBundle(), nullptr);

    // Bytes that are no bundle fail with the message of the loader
    const char text[] = "vec4 yupMaterial (YupSurface surface) { return vec4 (1.0); }";
    EXPECT_FALSE (getFailure (MaterialShader::createFromData (text, sizeof (text), nullptr, 0)).isEmpty());
    EXPECT_FALSE (getFailure (MaterialShader::createFromData (raster.getData(), raster.getSize(), text, sizeof (text))).isEmpty());
}

#if YUP_ENABLE_SHADER_TRANSPILER

TEST_F (MaterialShaderTests, CompilesAMaterialFromSourceAndExposesItsUniforms)
{
    const auto created = MaterialShader::createFromSource (hologramSource, getShaderDirectory());
    ASSERT_TRUE (created.wasOk()) << getFailure (created);

    const auto shader = created.getValue();
    ASSERT_NE (shader->getRasterBundle(), nullptr);
    ASSERT_NE (shader->getFullBundle(), nullptr);

    // vec4 rim, then float time: 20 bytes, rounded up to 16
    auto uniforms = shader->createUniforms();
    EXPECT_EQ (uniforms.getSizeInBytes(), 32u);
    EXPECT_EQ (uniforms.getNumComponents ("rim"), 4);
    EXPECT_EQ (uniforms.getNumComponents ("time"), 1);
    EXPECT_EQ (uniforms.getMemberType ("time"), ShaderReflection::BaseType::float32);
    EXPECT_FALSE (uniforms.hasMember ("params"));

    // Both variants for every language yup_shader_bundler targets, the full one writing the motion too
    for (const auto language : { ShaderLanguage::glsl, ShaderLanguage::essl, ShaderLanguage::hlsl, ShaderLanguage::msl, ShaderLanguage::spirv, ShaderLanguage::wgsl })
    {
        for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
        {
            EXPECT_NE (shader->getRasterBundle()->findShader (stage, language), nullptr) << toString (language) << " " << toString (stage);
            EXPECT_NE (shader->getFullBundle()->findShader (stage, language), nullptr) << toString (language) << " " << toString (stage);
        }
    }

    const auto* rasterFragment = shader->getRasterBundle()->findShader (ShaderStage::fragment, ShaderLanguage::spirv);
    const auto* fullFragment = shader->getFullBundle()->findShader (ShaderStage::fragment, ShaderLanguage::spirv);
    ASSERT_NE (rasterFragment, nullptr);
    ASSERT_NE (fullFragment, nullptr);
    EXPECT_EQ (rasterFragment->reflection.stageOutputs.size(), 1u);
    EXPECT_EQ (fullFragment->reflection.stageOutputs.size(), 2u);
}

TEST_F (MaterialShaderTests, MaterialWithoutABlockHasNoUniforms)
{
    const auto created = MaterialShader::createFromSource (normalSource, getShaderDirectory());
    ASSERT_TRUE (created.wasOk()) << getFailure (created);
    EXPECT_TRUE (created.getReference()->createUniforms().isEmpty());
}

TEST_F (MaterialShaderTests, CompiledVariantsLoadAgainFromTheirBytes)
{
    const auto compiled = MaterialShader::createFromSource (hologramSource, getShaderDirectory());
    ASSERT_TRUE (compiled.wasOk()) << getFailure (compiled);

    MemoryBlock raster, full;
    ASSERT_TRUE (compiled.getReference()->getRasterBundle()->saveToMemoryBlock (raster).wasOk());
    ASSERT_TRUE (compiled.getReference()->getFullBundle()->saveToMemoryBlock (full).wasOk());

    const auto loaded = MaterialShader::createFromData (raster.getData(), raster.getSize(), full.getData(), full.getSize());
    ASSERT_TRUE (loaded.wasOk()) << getFailure (loaded);
    EXPECT_TRUE (loaded.getReference()->createUniforms() == compiled.getReference()->createUniforms());
}

TEST_F (MaterialShaderTests, SourceIsCompiledInsideTheShadersOfTheModuleByDefault)
{
    // Where the module was compiled from, which this checkout is
    const auto shaders = MaterialShader::getSourceDirectory();
    ASSERT_TRUE (shaders.isDirectory()) << shaders.getFullPathName();
    EXPECT_TRUE (shaders.getChildFile ("yup_MaterialShader.frag").existsAsFile()) << shaders.getFullPathName();

    const auto created = MaterialShader::createFromSource (normalSource);
    EXPECT_TRUE (created.wasOk()) << getFailure (created);
}

TEST_F (MaterialShaderTests, GlslErrorsFailWithTheMessageOfTheCompilerAtTheLineOfTheMaterial)
{
    const auto created = MaterialShader::createFromSource (R"(vec4 yupMaterial (YupSurface surface)
{
    return vec4 (missingValue);
}
)",
                                                           getShaderDirectory());

    ASSERT_TRUE (created.failed());
    EXPECT_TRUE (created.getErrorMessage().contains ("missingValue")) << created.getErrorMessage();
    EXPECT_TRUE (created.getErrorMessage().contains ("material:3")) << created.getErrorMessage();
}

TEST_F (MaterialShaderTests, SourceWithoutTheFunctionFails)
{
    const auto created = MaterialShader::createFromSource ("vec4 anotherFunction (YupSurface surface) { return surface.baseColor; }", getShaderDirectory());
    EXPECT_TRUE (getFailure (created).contains ("yupMaterial")) << getFailure (created);
}

TEST_F (MaterialShaderTests, BlockAndTextureOfItsOwnAreRefusedFromSource)
{
    const auto block = MaterialShader::createFromSource (R"(layout(set = 2, binding = 20) uniform Extra { vec4 tint; } extra;
vec4 yupMaterial (YupSurface surface) { return extra.tint; })",
                                                         getShaderDirectory());
    EXPECT_TRUE (getFailure (block).contains ("uniform block")) << getFailure (block);
    EXPECT_TRUE (getFailure (block).contains ("set 2, binding 20")) << getFailure (block);

    const auto texture = MaterialShader::createFromSource (R"(layout(set = 2, binding = 21) uniform texture2D noise;
vec4 yupMaterial (YupSurface surface) { return texture (sampler2D (noise, linearClamp), surface.uv); })",
                                                           getShaderDirectory());
    EXPECT_TRUE (getFailure (texture).contains ("declares a texture")) << getFailure (texture);
}

TEST_F (MaterialShaderTests, SourceFailsWhereTheTemplateIsMissing)
{
    // This folder holds no shader
    const auto created = MaterialShader::createFromSource (normalSource, File (__FILE__).getParentDirectory());
    EXPECT_TRUE (getFailure (created).contains ("template was not found")) << getFailure (created);
}

TEST_F (MaterialShaderTests, SourceIsCompiledInsideTheFolderAnApplicationGives)
{
    const auto builtFrom = MaterialShader::getSourceDirectory();
    const auto bundled = File::createTempFile ("shaders");

    const ScopeGuard restore { [&bundled]
    {
        MaterialShader::setSourceDirectory (File());
        bundled.deleteRecursively();
    } };

    // A copy of what an application bundles: the two stages of the template and the library they include
    const auto shaders = getShaderDirectory();
    ASSERT_TRUE (bundled.createDirectory().wasOk());
    ASSERT_TRUE (shaders.getChildFile ("yup_MaterialShader.vert").copyFileTo (bundled.getChildFile ("yup_MaterialShader.vert")));
    ASSERT_TRUE (shaders.getChildFile ("yup_MaterialShader.frag").copyFileTo (bundled.getChildFile ("yup_MaterialShader.frag")));
    ASSERT_TRUE (shaders.getChildFile ("lib").copyDirectoryTo (bundled.getChildFile ("lib")));

    MaterialShader::setSourceDirectory (bundled);
    EXPECT_EQ (MaterialShader::getSourceDirectory(), bundled);

    const auto created = MaterialShader::createFromSource (hologramSource);
    EXPECT_TRUE (created.wasOk()) << getFailure (created);

    // Once the folder is gone, it is the one told to be missing
    ASSERT_TRUE (bundled.deleteRecursively());

    const auto missing = MaterialShader::createFromSource (hologramSource);
    EXPECT_TRUE (getFailure (missing).contains (bundled.getFullPathName())) << getFailure (missing);

    // Without one, the folder the module was built from is back
    MaterialShader::setSourceDirectory (File());
    EXPECT_EQ (MaterialShader::getSourceDirectory(), builtFrom);
}

TEST_F (MaterialShaderTests, MaterialCompiledForAGraphicsApiHasItsVariantAloneOnBothTiersAndTheSameUniforms)
{
    /** Members named as other languages name things of their own: `point` in HLSL and `level` in MSL, whose variants
        declare them under other names.
    */
    static constexpr const char* source = R"(YUP_MATERIAL_PARAMS
{
    vec4 point;
    float level;
} params;

vec4 yupMaterial (YupSurface surface)
{
    return vec4 (params.point.rgb * params.level, surface.baseColor.a);
}
)";

    const auto everywhere = MaterialShader::createFromSource (source, getShaderDirectory());
    ASSERT_TRUE (everywhere.wasOk()) << getFailure (everywhere);

    const auto expected = everywhere.getReference()->createUniforms();
    ASSERT_EQ (expected.getNumComponents ("point"), 4);
    ASSERT_EQ (expected.getNumComponents ("level"), 1);

    for (const auto platform : { GpuPlatform::OpenGL, GpuPlatform::OpenGLES, GpuPlatform::Direct3D, GpuPlatform::Metal, GpuPlatform::WebGPU, GpuPlatform::Vulkan })
    {
        const auto created = MaterialShader::createFromSource (source, getShaderDirectory(), platform);
        ASSERT_TRUE (created.wasOk()) << static_cast<int> (platform) << ": " << getFailure (created);

        const auto& shader = created.getReference();
        ASSERT_NE (shader->getRasterBundle(), nullptr);
        ASSERT_NE (shader->getFullBundle(), nullptr);

        // Both tiers, in the language of the API and in no other
        for (const auto& bundle : { shader->getRasterBundle(), shader->getFullBundle() })
        {
            for (const auto language : { ShaderLanguage::glsl, ShaderLanguage::essl, ShaderLanguage::hlsl, ShaderLanguage::msl, ShaderLanguage::spirv, ShaderLanguage::wgsl })
            {
                for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
                {
                    EXPECT_EQ (bundle->findShader (stage, language) != nullptr, language == GpuImageShader::getShaderLanguage (platform))
                        << static_cast<int> (platform) << ": " << toString (language) << " " << toString (stage);
                }
            }
        }

        // The block the material declares, by the names it gives its members, whatever the language calls them
        EXPECT_TRUE (shader->createUniforms() == expected) << static_cast<int> (platform);
    }
}

TEST_F (MaterialShaderTests, GlslErrorsAreTheSameWhicheverGraphicsApiIsNamed)
{
    static constexpr const char* broken = R"(vec4 yupMaterial (YupSurface surface)
{
    return vec4 (missingValue);
}
)";

    const auto everywhere = MaterialShader::createFromSource (broken, getShaderDirectory());
    ASSERT_TRUE (everywhere.failed());

    for (const auto platform : { GpuPlatform::Metal, GpuPlatform::Vulkan })
    {
        const auto created = MaterialShader::createFromSource (broken, getShaderDirectory(), platform);
        ASSERT_TRUE (created.failed()) << static_cast<int> (platform);
        EXPECT_EQ (created.getErrorMessage(), everywhere.getErrorMessage()) << static_cast<int> (platform);
    }
}

#endif
