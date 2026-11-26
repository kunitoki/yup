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

#include <yup_rhi/yup_rhi.h>

#include <cstring>

using namespace yup;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

//==============================================================================
// GpuImageShader - creation and reflection, no GPU involved
//==============================================================================

class GpuImageShaderTests : public ::testing::Test
{
protected:
    /** A bundle holding one GLSL variant of each stage, reflecting what it is given. */
    static ShaderBundle::Ptr makeStages (ShaderReflection vertexReflection, ShaderReflection fragmentReflection)
    {
        ShaderBundle::Ptr bundle (new ShaderBundle());

        ShaderInfo vertex;
        vertex.stage = ShaderStage::vertex;
        vertex.source = "void main() {}";
        vertex.reflection = std::move (vertexReflection);
        bundle->addShader (std::move (vertex));

        ShaderInfo fragment;
        fragment.stage = ShaderStage::fragment;
        fragment.source = "void main() {}";
        fragment.reflection = std::move (fragmentReflection);
        bundle->addShader (std::move (fragment));

        return bundle;
    }

    /** A bundle holding one GLSL variant of each stage, the fragment one reflecting @p reflection
        and, when it tells no output, the pixel every fragment stage writes. */
    static ShaderBundle::Ptr makeBundle (ShaderReflection reflection)
    {
        if (reflection.stageOutputs.empty())
            reflection.stageOutputs = { makeVariable ("color", ShaderReflection::ResourceType::stageOutput, 0, 4) };

        return makeStages ({}, std::move (reflection));
    }

    /** The reflection of an input or an output of a stage: a vector of floats at a location. */
    static ShaderReflection::ResourceBinding makeVariable (const String& name, ShaderReflection::ResourceType type, uint32 location, uint32 vecSize)
    {
        ShaderReflection::ResourceBinding variable;
        variable.name = name;
        variable.type = type;
        variable.location = location;
        variable.baseType = ShaderReflection::BaseType::float32;
        variable.vecSize = vecSize;
        return variable;
    }

    /** The reflection of `uniform Parameters { float intensity; } params;`. */
    static ShaderReflection::ResourceBinding makeBlock (uint32 set, uint32 binding)
    {
        ShaderReflection::ResourceMember intensity;
        intensity.name = "intensity";
        intensity.baseType = ShaderReflection::BaseType::float32;
        intensity.size = 4;

        ShaderReflection::ResourceBinding block;
        block.name = "Parameters";
        block.type = ShaderReflection::ResourceType::uniformBuffer;
        block.set = set;
        block.binding = binding;
        block.members = { intensity };
        block.blockSize = 4;
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

    /** The reflection of a fragment stage declaring nothing but the pixel it writes. */
    static ShaderReflection makeFragment()
    {
        ShaderReflection reflection;
        reflection.stageOutputs = { makeVariable ("color", ShaderReflection::ResourceType::stageOutput, 0, 4) };
        return reflection;
    }

    static String getFailure (const ResultValue<GpuImageShader::Ptr>& created)
    {
        return created.failed() ? created.getErrorMessage() : String();
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Tints the picture it reads: one uniform block and one input. */
    static constexpr const char* tintFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;
    float intensity;
    int steps;
} params;
layout(set = 0, binding = 1) uniform texture2D source;
layout(set = 0, binding = 2) uniform sampler sourceSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    vec4 pixel = texture (sampler2D (source, sourceSampler), uv);
    color = mix (pixel, pixel * params.tint, params.intensity) * float (params.steps);
})";

    /** Makes its picture alone: no block and no input. */
    static constexpr const char* proceduralFragment = R"(#version 450
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = vec4 (uv, 0.0, 1.0); })";

    /** Members and a texture named as other languages name things of their own: `point` in HLSL, `level`, `bias`
        and `kernel` in MSL, whose variants declare them under other names.
    */
    static constexpr const char* keywordFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters
{
    vec4 point;
    float level;
    float bias;
} params;
layout(set = 0, binding = 1) uniform texture2D kernel;
layout(set = 0, binding = 2) uniform sampler kernelSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    vec4 pixel = texture (sampler2D (kernel, kernelSampler), uv);
    color = mix (pixel, pixel * params.point, params.level) * params.bias;
})";

    /** Compares two structures, which WGSL does not take: a source every graphics API compiles but WebGPU. */
    static constexpr const char* notForWebGpuFragment = R"(#version 450
struct Level { float value; };
layout(location = 0) out vec4 color;
void main()
{
    Level low = Level (0.0);
    Level high = Level (1.0);
    bool same = low == high;
    color = same ? vec4 (1.0) : vec4 (0.0, 0.0, 0.0, 1.0);
})";

    /** The graphics APIs a shader is drawn with. */
    static constexpr GpuPlatform drawingPlatforms[] = { GpuPlatform::OpenGL, GpuPlatform::OpenGLES, GpuPlatform::Direct3D, GpuPlatform::Metal, GpuPlatform::WebGPU, GpuPlatform::Vulkan };

    /** Compiles @p fragment behind a fullscreen vertex stage for the languages yup_shader_bundler targets by default. */
    static ResultValue<ShaderBundle> compileBundle (const char* fragment)
    {
        static constexpr const char* vertex = R"(#version 450
layout(location = 0) out vec2 uv;
void main()
{
    vec2 p = vec2 (float ((gl_VertexIndex << 1) & 2), float (gl_VertexIndex & 2));
    uv = vec2 (p.x, 1.0 - p.y);
    gl_Position = vec4 (p * 2.0 - 1.0, 0.0, 1.0);
})";

        ShaderBundleCompileRequest request;

        for (const auto& [stage, source] : { std::pair (ShaderStage::vertex, vertex), std::pair (ShaderStage::fragment, fragment) })
        {
            ShaderBundleEntry entry;
            entry.stage = stage;
            entry.source = source;
            entry.targetLanguages = { ShaderLanguage::glsl, ShaderLanguage::essl, ShaderLanguage::hlsl, ShaderLanguage::msl, ShaderLanguage::spirv, ShaderLanguage::wgsl };
            request.entries.push_back (entry);
        }

        return ShaderBundleCompiler().compile (request);
    }
#endif
};

TEST_F (GpuImageShaderTests, InvalidBytesFailWithAMessage)
{
    // The RIFF magic of a bundle, but not one
    const uint8 notABundle[] = { 'R', 'I', 'F', 'F', 4, 0, 0, 0, 'N', 'O', 'P', 'E' };
    const auto fromRiff = GpuImageShader::createFromData (notABundle, sizeof (notABundle));
    ASSERT_TRUE (fromRiff.failed());
    EXPECT_TRUE (fromRiff.getErrorMessage().isNotEmpty());

    // Neither a bundle nor GLSL
    const char notAShader[] = "this is not a shader";
    const auto fromText = GpuImageShader::createFromData (notAShader, std::strlen (notAShader));
    ASSERT_TRUE (fromText.failed());
    EXPECT_TRUE (fromText.getErrorMessage().isNotEmpty());

    EXPECT_TRUE (GpuImageShader::createFromData (nullptr, 0).failed());
    EXPECT_TRUE (GpuImageShader::createFromData (notAShader, 0).failed());
}

TEST_F (GpuImageShaderTests, BundleWithoutBothStagesFails)
{
    EXPECT_TRUE (GpuImageShader::createFromBundle (nullptr).failed());
    EXPECT_TRUE (GpuImageShader::createFromBundle (new ShaderBundle()).failed());

    ShaderBundle::Ptr vertexOnly (new ShaderBundle());
    ShaderInfo vertex;
    vertex.stage = ShaderStage::vertex;
    vertexOnly->addShader (std::move (vertex));
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (vertexOnly)).contains ("fragment"));

    ShaderBundle::Ptr fragmentOnly (new ShaderBundle());
    ShaderInfo fragment;
    fragment.stage = ShaderStage::fragment;
    fragmentOnly->addShader (std::move (fragment));
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (fragmentOnly)).contains ("vertex"));
}

TEST_F (GpuImageShaderTests, ReadsTheBlockAndTheTexturesOfTheFragmentStage)
{
    ShaderReflection reflection;
    reflection.uniformBuffers = { makeBlock (1, 3) };
    reflection.separateImages = { makeTexture ("overlay", 0, 2), makeTexture ("source", 0, 1) };

    const auto created = GpuImageShader::createFromBundle (makeBundle (reflection));
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    const auto shader = created.getValue();
    EXPECT_EQ (shader->getNumInputTextures(), 2);

    // The inputs are told in the order they are given: by set, then by binding
    EXPECT_EQ (shader->getInputTextureNames(), StringArray ({ "source", "overlay" }));

    const auto uniforms = shader->createUniforms();
    EXPECT_EQ (uniforms.getSizeInBytes(), 16u);
    EXPECT_TRUE (uniforms.hasMember ("intensity"));
    EXPECT_EQ (uniforms.getNumComponents ("intensity"), 1);
}

TEST_F (GpuImageShaderTests, ShaderDeclaringNothingHasNoInputNorUniforms)
{
    const auto created = GpuImageShader::createFromBundle (makeBundle ({}));
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    EXPECT_EQ (created.getValue()->getNumInputTextures(), 0);
    EXPECT_TRUE (created.getValue()->getInputTextureNames().isEmpty());
    EXPECT_TRUE (created.getValue()->createUniforms().isEmpty());
    EXPECT_TRUE (created.getValue()->getUniformLayout().isEmpty());
}

TEST_F (GpuImageShaderTests, TellsTheMembersOfItsBlockWithoutCreatingUniforms)
{
    ShaderReflection reflection;
    reflection.uniformBuffers = { makeBlock (0, 0) };

    const auto created = GpuImageShader::createFromBundle (makeBundle (reflection));
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    const auto shader = created.getValue();

    // The same uniforms at every call, zeroed, where each createUniforms() makes others
    const auto& layout = shader->getUniformLayout();
    EXPECT_EQ (std::addressof (layout), std::addressof (shader->getUniformLayout()));
    EXPECT_EQ (layout.getSizeInBytes(), 16u);
    EXPECT_TRUE (layout.hasMember ("intensity"));
    EXPECT_FALSE (layout.hasMember ("time"));

    ASSERT_EQ (layout.getMembers().size(), 1u);
    EXPECT_EQ (layout.getMembers().front().name, String ("intensity"));

    // The uniforms the shader creates are copies of them sharing their members, so a member found once in the
    // shader is set in each with the handle it was found with
    const auto intensity = layout.findMember ("intensity");
    ASSERT_TRUE (intensity.isValid());

    auto uniforms = shader->createUniforms();
    EXPECT_TRUE (uniforms == layout);
    EXPECT_EQ (uniforms.getMembers().data(), layout.getMembers().data());
    EXPECT_EQ (shader->createUniforms().getMembers().data(), layout.getMembers().data());

    EXPECT_TRUE (uniforms.set (intensity, { 0.5f }));
    EXPECT_FALSE (uniforms == layout);
    EXPECT_TRUE (uniforms.hasSameLayoutAs (layout));
    EXPECT_TRUE (shader->createUniforms() == layout);

    float stored = 0.0f;
    std::memcpy (&stored, uniforms.getData() + layout.getMembers().front().offset, sizeof (stored));
    EXPECT_FLOAT_EQ (stored, 0.5f);
}

TEST_F (GpuImageShaderTests, RefusesWhatADrawCannotBind)
{
    ShaderReflection combined;
    combined.sampledImages = { makeTexture ("source", 0, 0) };
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeBundle (combined))).contains ("sampler2D"));

    ShaderReflection twoBlocks;
    twoBlocks.uniformBuffers = { makeBlock (0, 0), makeBlock (0, 1) };
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeBundle (twoBlocks))).contains ("uniform blocks"));

    // As many textures as an image shader takes, then one more
    ShaderReflection manyTextures;

    for (int i = 0; i < GpuImageShader::maxInputTextures; ++i)
        manyTextures.separateImages.push_back (makeTexture ("texture" + String (i), 0, static_cast<uint32> (i)));

    const auto most = GpuImageShader::createFromBundle (makeBundle (manyTextures));
    ASSERT_TRUE (most.wasOk()) << most.getErrorMessage();
    EXPECT_EQ (most.getValue()->getNumInputTextures(), GpuImageShader::maxInputTextures);

    manyTextures.separateImages.push_back (makeTexture ("oneTooMany", 0, static_cast<uint32> (GpuImageShader::maxInputTextures)));
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeBundle (manyTextures))).contains ("textures"));

    ShaderReflection cube;
    cube.separateImages = { makeTexture ("environment", 0, 0) };
    cube.separateImages.front().imageDim = ShaderReflection::ImageDimension::cube;
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeBundle (cube))).contains ("environment"));
}

TEST_F (GpuImageShaderTests, RefusesAUniformBlockLargerThanUniformsHold)
{
    // A bundle read from a file can reflect any size: the largest block is taken, nothing above it
    ShaderReflection reflection;
    reflection.uniformBuffers = { makeBlock (0, 0) };
    reflection.uniformBuffers.front().blockSize = static_cast<uint32> (ShaderUniforms::maxSizeInBytes);

    const auto largest = GpuImageShader::createFromBundle (makeBundle (reflection));
    ASSERT_TRUE (largest.wasOk()) << largest.getErrorMessage();
    EXPECT_EQ (largest.getValue()->createUniforms().getSizeInBytes(), ShaderUniforms::maxSizeInBytes);

    for (const auto blockSize : { static_cast<uint32> (ShaderUniforms::maxSizeInBytes) + 1, 0xffffffffu })
    {
        reflection.uniformBuffers.front().blockSize = blockSize;
        EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeBundle (reflection))).contains ("uniform block of")) << blockSize;
    }
}

TEST_F (GpuImageShaderTests, AcceptsTheSamplersAGLVariantFoldsItsTexturesInto)
{
    // A GLSL variant lists the combined sampler it emits for each texture and sampler pair
    ShaderReflection reflection;
    reflection.separateImages = { makeTexture ("source", 0, 0) };
    reflection.sampledImages = { makeTexture ("yup_combined_source_sourceSampler", 0, 0) };
    reflection.glCombinedSamplers.push_back ({ "yup_combined_source_sourceSampler", 0 });

    const auto created = GpuImageShader::createFromBundle (makeBundle (reflection));
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();
    EXPECT_EQ (created.getValue()->getNumInputTextures(), 1);
}

TEST_F (GpuImageShaderTests, RefusesTheResourcesNoDrawBinds)
{
    using ResourceType = ShaderReflection::ResourceType;

    const auto makeResource = [] (const char* name, ResourceType type)
    {
        ShaderReflection::ResourceBinding resource;
        resource.name = name;
        resource.type = type;
        return resource;
    };

    ShaderReflection storageBuffer, storageImage, subpassInput, pushConstants;
    storageBuffer.storageBuffers = { makeResource ("Particles", ResourceType::storageBuffer) };
    storageImage.storageImages = { makeResource ("history", ResourceType::storageImage) };
    subpassInput.subpassInputs = { makeResource ("depth", ResourceType::subpassInput) };
    pushConstants.pushConstantBuffers = { makeResource ("Constants", ResourceType::pushConstant) };

    for (const auto& [reflection, declared] : { std::pair { storageBuffer, "storage buffer Particles" },
                                                std::pair { storageImage, "storage image history" },
                                                std::pair { subpassInput, "subpass input depth" },
                                                std::pair { pushConstants, "push constant block Constants" } })
    {
        // Whichever stage declares it
        EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeBundle (reflection))).contains (String ("fragment stage declares the ") + declared)) << declared;
        EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeStages (reflection, makeFragment()))).contains (String ("vertex stage declares the ") + declared)) << declared;
    }
}

TEST_F (GpuImageShaderTests, VertexStageReadsWhatTheFragmentStageDeclaresOnly)
{
    auto fragment = makeFragment();
    fragment.uniformBuffers = { makeBlock (0, 0) };
    fragment.separateImages = { makeTexture ("source", 0, 1) };

    // The block and the texture both stages declare are bound once, for both
    ShaderReflection vertex;
    vertex.uniformBuffers = fragment.uniformBuffers;
    vertex.separateImages = fragment.separateImages;

    const auto created = GpuImageShader::createFromBundle (makeStages (vertex, fragment));
    EXPECT_TRUE (created.wasOk()) << getFailure (created);

    // What the vertex stage declares alone, or elsewhere, is bound by nothing
    vertex.uniformBuffers = { makeBlock (0, 3) };
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeStages (vertex, fragment))).contains ("vertex stage declares the uniform block Parameters"));

    vertex.uniformBuffers = fragment.uniformBuffers;
    vertex.separateImages = { makeTexture ("lookup", 1, 1) };
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeStages (vertex, fragment))).contains ("vertex stage declares the texture lookup"));

    vertex.separateImages.clear();
    vertex.sampledImages = { makeTexture ("lookup", 0, 4) };
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeStages (vertex, fragment))).contains ("vertex stage declares a combined sampler2D"));
}

TEST_F (GpuImageShaderTests, RefusesInputsAndOutputsOtherThanUvAndThePixel)
{
    using ResourceType = ShaderReflection::ResourceType;

    const auto failureOf = [] (const ShaderReflection& fragment)
    {
        return getFailure (GpuImageShader::createFromBundle (makeStages ({}, fragment)));
    };

    // What the vertex stage writes at location 0, under any name
    auto reflection = makeFragment();
    reflection.stageInputs = { makeVariable ("coordinates", ResourceType::stageInput, 0, 2) };
    EXPECT_EQ (failureOf (reflection), String());

    // Another location, another size, another type, and one more than it writes
    reflection.stageInputs = { makeVariable ("normal", ResourceType::stageInput, 1, 2) };
    EXPECT_TRUE (failureOf (reflection).contains ("input normal")) << failureOf (reflection);

    reflection.stageInputs = { makeVariable ("tint", ResourceType::stageInput, 0, 4) };
    EXPECT_TRUE (failureOf (reflection).contains ("input tint")) << failureOf (reflection);

    reflection.stageInputs = { makeVariable ("index", ResourceType::stageInput, 0, 2) };
    reflection.stageInputs.front().baseType = ShaderReflection::BaseType::int32;
    EXPECT_TRUE (failureOf (reflection).contains ("input index")) << failureOf (reflection);

    reflection.stageInputs = { makeVariable ("uv", ResourceType::stageInput, 0, 2), makeVariable ("depth", ResourceType::stageInput, 1, 1) };
    EXPECT_TRUE (failureOf (reflection).contains ("input depth")) << failureOf (reflection);

    // One output, the pixel: not none, not a vec3, not at another location, not two
    reflection = makeFragment();
    reflection.stageOutputs.clear();
    EXPECT_TRUE (failureOf (reflection).contains ("one output")) << failureOf (reflection);

    reflection.stageOutputs = { makeVariable ("color", ResourceType::stageOutput, 0, 3) };
    EXPECT_TRUE (failureOf (reflection).contains ("one output")) << failureOf (reflection);

    reflection.stageOutputs = { makeVariable ("color", ResourceType::stageOutput, 1, 4) };
    EXPECT_TRUE (failureOf (reflection).contains ("one output")) << failureOf (reflection);

    reflection.stageOutputs = { makeVariable ("color", ResourceType::stageOutput, 0, 4), makeVariable ("motion", ResourceType::stageOutput, 1, 4) };
    EXPECT_TRUE (failureOf (reflection).contains ("one output")) << failureOf (reflection);
}

TEST_F (GpuImageShaderTests, TellsTheGraphicsApisItHasAVariantOfBothStagesFor)
{
    const auto makeVariant = [] (ShaderStage stage, ShaderLanguage language)
    {
        ShaderInfo info;
        info.stage = stage;
        info.language = language;
        info.source = "void main() {}";

        if (stage == ShaderStage::fragment)
            info.reflection = makeFragment();

        return info;
    };

    // Both stages for OpenGL and for Metal, and the fragment stage alone for Direct3D
    ShaderBundle::Ptr bundle (new ShaderBundle());

    for (const auto language : { ShaderLanguage::glsl, ShaderLanguage::msl })
    {
        for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
            bundle->addShader (makeVariant (stage, language));
    }

    bundle->addShader (makeVariant (ShaderStage::fragment, ShaderLanguage::hlsl));

    const auto created = GpuImageShader::createFromBundle (bundle);
    ASSERT_TRUE (created.wasOk()) << getFailure (created);

    const auto shader = created.getValue();
    EXPECT_TRUE (shader->canDrawWith (GpuPlatform::OpenGL));
    EXPECT_TRUE (shader->canDrawWith (GpuPlatform::Metal));
    EXPECT_FALSE (shader->canDrawWith (GpuPlatform::Direct3D));
    EXPECT_FALSE (shader->canDrawWith (GpuPlatform::Vulkan));
    EXPECT_FALSE (shader->canDrawWith (GpuPlatform::WebGPU));

    // OpenGL ES takes the GLSL variant of a bundle without an ESSL one, as a draw does
    EXPECT_TRUE (shader->canDrawWith (GpuPlatform::OpenGLES));
}

#if YUP_ENABLE_SHADER_TRANSPILER

TEST_F (GpuImageShaderTests, CreatesFromGlslWithABlockAndOneInput)
{
    const auto created = GpuImageShader::createFromGlsl (tintFragment);
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    const auto shader = created.getValue();
    EXPECT_EQ (shader->getNumInputTextures(), 1);

    auto uniforms = shader->createUniforms();
    EXPECT_EQ (uniforms.getSizeInBytes(), 32u);

    EXPECT_EQ (uniforms.getMemberType ("tint"), ShaderReflection::BaseType::float32);
    EXPECT_EQ (uniforms.getNumComponents ("tint"), 4);
    EXPECT_EQ (uniforms.getMemberType ("intensity"), ShaderReflection::BaseType::float32);
    EXPECT_EQ (uniforms.getNumComponents ("intensity"), 1);
    EXPECT_EQ (uniforms.getMemberType ("steps"), ShaderReflection::BaseType::int32);
    EXPECT_EQ (uniforms.getNumComponents ("steps"), 1);
    EXPECT_FALSE (uniforms.hasMember ("params"));

    // Each call returns zeroed uniforms of its own
    EXPECT_TRUE (uniforms.set ("intensity", 0.5f));
    EXPECT_FALSE (uniforms == shader->createUniforms());
    EXPECT_TRUE (shader->createUniforms() == shader->createUniforms());
}

TEST_F (GpuImageShaderTests, ProceduralShaderHasNoInputNorUniforms)
{
    const auto created = GpuImageShader::createFromGlsl (proceduralFragment);
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    EXPECT_EQ (created.getValue()->getNumInputTextures(), 0);
    EXPECT_TRUE (created.getValue()->createUniforms().isEmpty());

    // Compiled for every language, it draws with every graphics API
    for (const auto platform : { GpuPlatform::OpenGL, GpuPlatform::OpenGLES, GpuPlatform::Direct3D, GpuPlatform::Metal, GpuPlatform::WebGPU, GpuPlatform::Vulkan })
        EXPECT_TRUE (created.getValue()->canDrawWith (platform)) << static_cast<int> (platform);
}

TEST_F (GpuImageShaderTests, GlslWithOtherInputsOrOutputsThanUvAndThePixelIsRefused)
{
    const auto twoOutputs = GpuImageShader::createFromGlsl (R"(#version 450
layout(location = 0) out vec4 color;
layout(location = 1) out vec4 motion;
void main() { color = vec4 (1.0); motion = vec4 (0.0); })");

    EXPECT_TRUE (getFailure (twoOutputs).contains ("one output")) << getFailure (twoOutputs);

    const auto otherInput = GpuImageShader::createFromGlsl (R"(#version 450
layout(location = 0) in vec2 uv;
layout(location = 1) in vec4 tint;
layout(location = 0) out vec4 color;
void main() { color = tint * uv.x; })");

    EXPECT_TRUE (getFailure (otherInput).contains ("input tint")) << getFailure (otherInput);
}

TEST_F (GpuImageShaderTests, BoolMembersOfGlslAreUnsignedIntegers)
{
    const auto created = GpuImageShader::createFromGlsl (R"(#version 450
layout(set = 0, binding = 0) uniform Parameters { bool inverted; } params;
layout(location = 0) out vec4 color;
void main() { color = params.inverted ? vec4 (1.0) : vec4 (0.0, 0.0, 0.0, 1.0); })");

    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    auto uniforms = created.getValue()->createUniforms();
    EXPECT_EQ (uniforms.getMemberType ("inverted"), ShaderReflection::BaseType::uint32);
    EXPECT_TRUE (uniforms.set ("inverted", true));

    uint32 stored = 0;
    ASSERT_GE (uniforms.getSizeInBytes(), sizeof (stored));
    std::memcpy (&stored, uniforms.getData(), sizeof (stored));
    EXPECT_EQ (stored, 1u);
}

TEST_F (GpuImageShaderTests, GlslErrorsFailWithTheMessageOfTheCompiler)
{
    const auto created = GpuImageShader::createFromGlsl (R"(#version 450
layout(location = 0) out vec4 color;
void main() { color = vec4 (missingValue); })");

    ASSERT_TRUE (created.failed());
    EXPECT_TRUE (created.getErrorMessage().contains ("missingValue")) << created.getErrorMessage();
}

TEST_F (GpuImageShaderTests, CombinedSamplerInGlslIsRefused)
{
    const auto created = GpuImageShader::createFromGlsl (R"(#version 450
layout(set = 0, binding = 0) uniform sampler2D source;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = texture (source, uv); })");

    EXPECT_TRUE (getFailure (created).contains ("sampler2D")) << getFailure (created);
}

TEST_F (GpuImageShaderTests, GlslBytesAreCompiledAsGlslSource)
{
    const auto created = GpuImageShader::createFromData (tintFragment, std::strlen (tintFragment));
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();

    const auto fromSource = GpuImageShader::createFromGlsl (tintFragment);
    ASSERT_TRUE (fromSource.wasOk()) << fromSource.getErrorMessage();

    EXPECT_EQ (created.getValue()->getNumInputTextures(), 1);
    EXPECT_TRUE (created.getValue()->createUniforms() == fromSource.getValue()->createUniforms());
}

TEST_F (GpuImageShaderTests, BundleBytesGiveTheReflectionOfTheSource)
{
    const auto fromSource = GpuImageShader::createFromGlsl (tintFragment);
    ASSERT_TRUE (fromSource.wasOk()) << fromSource.getErrorMessage();

    const auto compiled = compileBundle (tintFragment);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    for (const auto compression : { ShaderBundle::Compression::none, ShaderBundle::Compression::high })
    {
        MemoryBlock bytes;
        ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes, compression).wasOk());

        const auto loaded = GpuImageShader::createFromData (bytes.getData(), bytes.getSize());
        ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

        EXPECT_EQ (loaded.getValue()->getNumInputTextures(), fromSource.getValue()->getNumInputTextures());
        EXPECT_TRUE (loaded.getValue()->createUniforms() == fromSource.getValue()->createUniforms());
        EXPECT_FALSE (loaded.getValue()->createUniforms().isEmpty());
    }
}

TEST_F (GpuImageShaderTests, GlslCompiledForAGraphicsApiHasItsVariantAloneAndDeclaresTheSame)
{
    const auto everywhere = GpuImageShader::createFromGlsl (keywordFragment);
    ASSERT_TRUE (everywhere.wasOk()) << everywhere.getErrorMessage();

    const auto& expected = everywhere.getReference();
    ASSERT_EQ (expected->getNumInputTextures(), 1);

    for (const auto platform : drawingPlatforms)
    {
        const auto created = GpuImageShader::createFromGlsl (keywordFragment, platform);
        ASSERT_TRUE (created.wasOk()) << static_cast<int> (platform) << ": " << created.getErrorMessage();

        const auto& shader = created.getReference();

        // OpenGL ES takes the GLSL variant of a shader compiled for OpenGL, as a draw does
        for (const auto other : drawingPlatforms)
        {
            const bool canDraw = other == platform || (platform == GpuPlatform::OpenGL && other == GpuPlatform::OpenGLES);
            EXPECT_EQ (shader->canDrawWith (other), canDraw) << static_cast<int> (platform) << " drawn with " << static_cast<int> (other);
        }

        // The block and the texture the source declares, by the names it gives them, whatever the language calls them
        const auto& uniforms = shader->getUniformLayout();
        EXPECT_TRUE (uniforms.hasSameLayoutAs (expected->getUniformLayout())) << static_cast<int> (platform);
        EXPECT_EQ (uniforms.getNumComponents ("point"), 4) << static_cast<int> (platform);
        EXPECT_EQ (uniforms.getNumComponents ("level"), 1) << static_cast<int> (platform);
        EXPECT_EQ (uniforms.getNumComponents ("bias"), 1) << static_cast<int> (platform);
        EXPECT_TRUE (shader->getInputTextureNames() == expected->getInputTextureNames()) << static_cast<int> (platform);
        EXPECT_EQ (shader->getInputTextureNames()[0], String ("kernel")) << static_cast<int> (platform);
    }
}

TEST_F (GpuImageShaderTests, GlslErrorsAreTheSameWhicheverGraphicsApiIsNamedAndWhatAnotherApiRefusesIsNotTold)
{
    static constexpr const char* broken = R"(#version 450
layout(location = 0) out vec4 color;
void main() { color = vec4 (missingValue); })";

    const auto everywhere = GpuImageShader::createFromGlsl (broken);
    ASSERT_TRUE (everywhere.failed());

    for (const auto platform : drawingPlatforms)
    {
        const auto created = GpuImageShader::createFromGlsl (broken, platform);
        ASSERT_TRUE (created.failed()) << static_cast<int> (platform);
        EXPECT_EQ (created.getErrorMessage(), everywhere.getErrorMessage()) << static_cast<int> (platform);
    }

    // What one language alone does not take fails the source compiled for every API, and for the API of that language
    const auto refused = GpuImageShader::createFromGlsl (notForWebGpuFragment);
    ASSERT_TRUE (refused.failed());
    EXPECT_TRUE (refused.getErrorMessage().containsIgnoreCase ("wgsl")) << refused.getErrorMessage();
    EXPECT_EQ (getFailure (GpuImageShader::createFromGlsl (notForWebGpuFragment, GpuPlatform::WebGPU)), refused.getErrorMessage());

    // Compiled for another API it is a shader, which that API draws
    for (const auto platform : { GpuPlatform::OpenGL, GpuPlatform::OpenGLES, GpuPlatform::Direct3D, GpuPlatform::Metal, GpuPlatform::Vulkan })
    {
        const auto created = GpuImageShader::createFromGlsl (notForWebGpuFragment, platform);
        ASSERT_TRUE (created.wasOk()) << static_cast<int> (platform) << ": " << getFailure (created);
        EXPECT_TRUE (created.getReference()->canDrawWith (platform)) << static_cast<int> (platform);
    }
}

TEST_F (GpuImageShaderTests, BytesOfASourceAreCompiledForTheGraphicsApiNamedAndABundleIsLoadedAsItIs)
{
    EXPECT_FALSE (GpuImageShader::isCompiledBundle (tintFragment, std::strlen (tintFragment)));

    const auto fromSource = GpuImageShader::createFromData (tintFragment, std::strlen (tintFragment), GpuPlatform::Vulkan);
    ASSERT_TRUE (fromSource.wasOk()) << fromSource.getErrorMessage();
    EXPECT_TRUE (fromSource.getReference()->canDrawWith (GpuPlatform::Vulkan));
    EXPECT_FALSE (fromSource.getReference()->canDrawWith (GpuPlatform::Metal));

    const auto compiled = compileBundle (tintFragment);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    EXPECT_TRUE (GpuImageShader::isCompiledBundle (bytes.getData(), bytes.getSize()));

    // A bundle holds the variants it was built with, whatever the API it is loaded for
    const auto loaded = GpuImageShader::createFromData (bytes.getData(), bytes.getSize(), GpuPlatform::Vulkan);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    for (const auto platform : drawingPlatforms)
        EXPECT_TRUE (loaded.getReference()->canDrawWith (platform)) << static_cast<int> (platform);

    EXPECT_TRUE (loaded.getReference()->getUniformLayout().hasSameLayoutAs (fromSource.getReference()->getUniformLayout()));
}

#endif // YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// GpuImageShader - draws on devices whose GPU is mocked
//==============================================================================

class GpuImageShaderMockedTests : public GpuImageShaderTests
{
protected:
    /** A mocked device telling when it is destroyed. */
    class TrackedDevice : public OreAndTargetGpuDevice
    {
    public:
        TrackedDevice (rive::ore::Context* oreCtx, bool& wasDestroyed)
            : OreAndTargetGpuDevice (oreCtx, MockOffscreenTarget::withGpuTexture (16, 16))
            , destroyed (wasDestroyed)
        {
        }

        ~TrackedDevice() override { destroyed = true; }

        /** Releases what the device pools, as every backend does before it goes. */
        void releaseResources() { releasePooledResources(); }

    private:
        bool& destroyed;
    };

    /** A mocked pipeline counting itself when it is destroyed. */
    struct TrackedPipeline : public TestOrePipeline
    {
        explicit TrackedPipeline (std::shared_ptr<int> numDestroyed)
            : destroyed (std::move (numDestroyed))
        {
        }

        ~TrackedPipeline() override { ++*destroyed; }

        std::shared_ptr<int> destroyed;
    };

    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        device = new OreAndTargetGpuDevice (mockOreCtx.get(), MockOffscreenTarget::withGpuTexture (16, 16));

        target = GpuTarget::create (device, 16, 16);
        ASSERT_NE (target, nullptr);
    }

    static GpuImageShader::Ptr createShader (ShaderReflection reflection)
    {
        const auto created = GpuImageShader::createFromBundle (makeBundle (std::move (reflection)));
        EXPECT_TRUE (created.wasOk()) << getFailure (created);
        return created.wasOk() ? created.getValue() : nullptr;
    }

    /** Makes @p oreCtx serve what compiling a pipeline and drawing with it ask a GPU for. */
    static void serveDraws (NiceMock<MockOreContext>& oreCtx)
    {
        ON_CALL (oreCtx, makeShaderModule (_))
            .WillByDefault (Return (rive::make_rcp<TestOreShaderModule>()));
        ON_CALL (oreCtx, makePipeline (_, _))
            .WillByDefault (Return (rive::make_rcp<TestOrePipeline>()));
        ON_CALL (oreCtx, wrapRiveTexture (_, _, _))
            .WillByDefault (Invoke ([] (rive::gpu::Texture*, uint32_t, uint32_t)
        {
            return rive::make_rcp<TestOreTextureView>();
        }));
        ON_CALL (oreCtx, beginRenderPass (_, _))
            .WillByDefault (Invoke ([] (const rive::ore::RenderPassDesc&, std::string*)
        {
            return std::unique_ptr<rive::ore::RenderPass> (new NiceMock<MockOreRenderPass>());
        }));
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    GpuDevice::Ptr device;
    GpuTarget::Ptr target;
};

TEST_F (GpuImageShaderMockedTests, DrawFailsWithoutAGpu)
{
    const auto shader = createShader ({});
    ASSERT_NE (shader, nullptr);

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_)).Times (0);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    const auto headless = shader->draw (GpuDevice::create (GpuPlatform::Headless, {}), frame, *target, shader->createUniforms());
    ASSERT_TRUE (headless.failed());
    EXPECT_TRUE (headless.getErrorMessage().isNotEmpty());

    EXPECT_TRUE (shader->draw (nullptr, frame, *target, shader->createUniforms()).failed());
}

TEST_F (GpuImageShaderMockedTests, DrawFailsWithoutTheInputsTheShaderDeclares)
{
    ShaderReflection reflection;
    reflection.separateImages = { makeTexture ("source", 0, 0), makeTexture ("overlay", 0, 1) };

    const auto shader = createShader (reflection);
    ASSERT_NE (shader, nullptr);

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_)).Times (0);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    EXPECT_TRUE (shader->draw (device, frame, *target, shader->createUniforms()).failed());
    EXPECT_TRUE (shader->draw (device, frame, *target, shader->createUniforms(), target->asTexture()).failed());

    // The inputs given together are checked the same: there has to be one for every texture
    const GpuTexture::Ptr one[] = { target->asTexture() };
    EXPECT_TRUE (shader->draw (device, frame, *target, shader->createUniforms(), Span<const GpuTexture::Ptr> (one, 1)).failed());
    EXPECT_TRUE (shader->draw (device, frame, *target, shader->createUniforms(), Span<const GpuTexture::Ptr>()).failed());
}

TEST_F (GpuImageShaderMockedTests, DrawFailsWhenAnInputIsTheTextureOfTheTarget)
{
    ShaderReflection reflection;
    reflection.separateImages = { makeTexture ("source", 0, 0) };

    const auto shader = createShader (reflection);
    ASSERT_NE (shader, nullptr);

    // Refused before anything is compiled or recorded
    EXPECT_CALL (*mockOreCtx, makeShaderModule (_)).Times (0);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    const auto drawn = shader->draw (device, frame, *target, shader->createUniforms(), target->asTexture());
    ASSERT_TRUE (drawn.failed());
    EXPECT_TRUE (drawn.getErrorMessage().contains ("texture of its target")) << drawn.getErrorMessage();
}

TEST_F (GpuImageShaderMockedTests, DrawFailsWithTheUniformsOfAnotherBlock)
{
    ShaderReflection reflection;
    reflection.uniformBuffers = { makeBlock (0, 0) };

    // As many bytes, laid out for a member of another name
    auto otherReflection = reflection;
    otherReflection.uniformBuffers.front().members.front().name = "amount";

    const auto shader = createShader (reflection);
    const auto other = createShader (otherReflection);
    ASSERT_NE (shader, nullptr);
    ASSERT_NE (other, nullptr);
    ASSERT_EQ (shader->createUniforms().getSizeInBytes(), other->createUniforms().getSizeInBytes());

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_)).Times (0);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    for (const auto& uniforms : { other->createUniforms(), ShaderUniforms() })
    {
        const auto drawn = shader->draw (device, frame, *target, uniforms);
        ASSERT_TRUE (drawn.failed());
        EXPECT_TRUE (drawn.getErrorMessage().contains ("uniforms")) << drawn.getErrorMessage();
    }
}

TEST_F (GpuImageShaderMockedTests, DrawTakesTheUniformsOfTheSameBlockMadeApart)
{
    ShaderReflection reflection;
    reflection.uniformBuffers = { makeBlock (0, 0) };

    // Another shader of the same block, whose uniforms describe their members apart, and the block itself
    const auto shader = createShader (reflection);
    const auto twin = createShader (reflection);
    ASSERT_NE (shader, nullptr);
    ASSERT_NE (twin, nullptr);
    ASSERT_NE (twin->getUniformLayout().getMembers().data(), shader->getUniformLayout().getMembers().data());

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    // The mocked GPU compiles no pipeline, which is where a draw given the uniforms of the shader stops
    const auto own = shader->draw (device, frame, *target, shader->createUniforms());
    ASSERT_TRUE (own.failed());

    // Uniforms laid out the same get as far, and not refused before for not being the ones of the shader
    for (const auto& uniforms : { twin->createUniforms(), ShaderUniforms (reflection.uniformBuffers.front()) })
    {
        const auto drawn = shader->draw (device, frame, *target, uniforms);
        ASSERT_TRUE (drawn.failed());
        EXPECT_EQ (drawn.getErrorMessage(), own.getErrorMessage());
    }

    const auto refused = shader->draw (device, frame, *target, ShaderUniforms());
    ASSERT_TRUE (refused.failed());
    EXPECT_NE (refused.getErrorMessage(), own.getErrorMessage());
}

TEST_F (GpuImageShaderMockedTests, PipelineThatCannotBeCompiledIsNotCompiledAgain)
{
    const auto shader = createShader ({});
    ASSERT_NE (shader, nullptr);

    // The mocked GPU makes no shader module, so the pipeline fails to compile: once, not at every draw
    EXPECT_CALL (*mockOreCtx, makeShaderModule (_)).Times (1);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    const auto first = shader->draw (device, frame, *target, shader->createUniforms());
    const auto second = shader->draw (device, frame, *target, shader->createUniforms());

    ASSERT_TRUE (first.failed());
    ASSERT_TRUE (second.failed());
    EXPECT_EQ (first.getErrorMessage(), second.getErrorMessage());
}

TEST_F (GpuImageShaderMockedTests, DrawingAlternatelyOnTwoDevicesCompilesEachPipelineOnce)
{
    const auto otherOreCtx = std::make_unique<NiceMock<MockOreContext>>();
    const GpuDevice::Ptr otherDevice (new OreAndTargetGpuDevice (otherOreCtx.get(), MockOffscreenTarget::withGpuTexture (16, 16)));

    const auto otherTarget = GpuTarget::create (otherDevice, 16, 16);
    ASSERT_NE (otherTarget, nullptr);

    const auto shader = createShader ({});
    ASSERT_NE (shader, nullptr);

    serveDraws (*mockOreCtx);
    serveDraws (*otherOreCtx);

    EXPECT_CALL (*mockOreCtx, makePipeline (_, _)).Times (1);
    EXPECT_CALL (*otherOreCtx, makePipeline (_, _)).Times (1);

    auto frame = GpuFrame::begin (device);
    auto otherFrame = GpuFrame::begin (otherDevice);

    for (int i = 0; i < 3; ++i)
    {
        const auto drawn = shader->draw (device, frame, *target, shader->createUniforms());
        EXPECT_TRUE (drawn.wasOk()) << drawn.getErrorMessage();

        const auto drawnOnOther = shader->draw (otherDevice, otherFrame, *otherTarget, shader->createUniforms());
        EXPECT_TRUE (drawnOnOther.wasOk()) << drawnOnOther.getErrorMessage();
    }
}

TEST_F (GpuImageShaderMockedTests, DeviceGoesWithItsOwnersWhileAShaderDrawnWithItLives)
{
    const auto closingOreCtx = std::make_unique<NiceMock<MockOreContext>>();
    serveDraws (*closingOreCtx);

    const auto shader = createShader ({});
    ASSERT_NE (shader, nullptr);

    bool destroyed = false;

    {
        const GpuDevice::Ptr closing (new TrackedDevice (closingOreCtx.get(), destroyed));

        const auto closingTarget = GpuTarget::create (closing, 16, 16);
        ASSERT_NE (closingTarget, nullptr);

        auto frame = GpuFrame::begin (closing);
        ASSERT_TRUE (shader->draw (closing, frame, *closingTarget, shader->createUniforms()).wasOk());
        EXPECT_FALSE (destroyed);
    }

    // The shader holds nothing of the device, which went with the last of its owners
    EXPECT_TRUE (destroyed);
}

TEST_F (GpuImageShaderMockedTests, PipelinesOfAShaderThatIsGoneAreReleasedByTheNextDrawOnTheirDevice)
{
    serveDraws (*mockOreCtx);

    const auto pipelinesDestroyed = std::make_shared<int> (0);
    ON_CALL (*mockOreCtx, makePipeline (_, _))
        .WillByDefault (Invoke ([pipelinesDestroyed] (const rive::ore::PipelineDesc&, std::string*)
    {
        return rive::make_rcp<TrackedPipeline> (pipelinesDestroyed);
    }));

    // The second shader compiles a pipeline of its own: what the first one left is not taken for it
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _)).Times (2);

    auto first = createShader ({});
    ASSERT_NE (first, nullptr);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (first->draw (device, frame, *target, first->createUniforms()).wasOk());

    // The frame is done with the pipeline and the shader goes: the device alone keeps it
    EXPECT_TRUE (frame.waitForGPU().wasOk());
    first = nullptr;
    EXPECT_EQ (*pipelinesDestroyed, 0);

    const auto second = createShader ({});
    ASSERT_NE (second, nullptr);

    auto laterFrame = GpuFrame::begin (device);
    EXPECT_TRUE (second->draw (device, laterFrame, *target, second->createUniforms()).wasOk());
    EXPECT_EQ (*pipelinesDestroyed, 1);
}

TEST_F (GpuImageShaderMockedTests, DrawCompilesAgainOnceTheDeviceReleasedItsPooledResources)
{
    const auto pooledOreCtx = std::make_unique<NiceMock<MockOreContext>>();
    serveDraws (*pooledOreCtx);

    EXPECT_CALL (*pooledOreCtx, makePipeline (_, _)).Times (2);

    bool destroyed = false;
    auto* tracked = new TrackedDevice (pooledOreCtx.get(), destroyed);
    const GpuDevice::Ptr pooled (tracked);

    const auto pooledTarget = GpuTarget::create (pooled, 16, 16);
    ASSERT_NE (pooledTarget, nullptr);

    const auto shader = createShader ({});
    ASSERT_NE (shader, nullptr);

    {
        auto frame = GpuFrame::begin (pooled);
        EXPECT_TRUE (shader->draw (pooled, frame, *pooledTarget, shader->createUniforms()).wasOk());
        EXPECT_TRUE (shader->draw (pooled, frame, *pooledTarget, shader->createUniforms()).wasOk());
    }

    // The pipelines go with what the device pools, while its backend is alive
    tracked->releaseResources();

    auto frame = GpuFrame::begin (pooled);
    EXPECT_TRUE (shader->draw (pooled, frame, *pooledTarget, shader->createUniforms()).wasOk());
}
