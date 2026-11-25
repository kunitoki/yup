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
    /** A bundle holding one GLSL variant of each stage, the fragment one reflecting @p reflection. */
    static ShaderBundle::Ptr makeBundle (ShaderReflection reflection)
    {
        ShaderBundle::Ptr bundle (new ShaderBundle());

        ShaderInfo vertex;
        vertex.stage = ShaderStage::vertex;
        vertex.source = "void main() {}";
        bundle->addShader (std::move (vertex));

        ShaderInfo fragment;
        fragment.stage = ShaderStage::fragment;
        fragment.source = "void main() {}";
        fragment.reflection = std::move (reflection);
        bundle->addShader (std::move (fragment));

        return bundle;
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
    EXPECT_TRUE (created.getValue()->createUniforms().isEmpty());
}

TEST_F (GpuImageShaderTests, RefusesWhatADrawCannotBind)
{
    ShaderReflection combined;
    combined.sampledImages = { makeTexture ("source", 0, 0) };
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeBundle (combined))).contains ("sampler2D"));

    ShaderReflection twoBlocks;
    twoBlocks.uniformBuffers = { makeBlock (0, 0), makeBlock (0, 1) };
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeBundle (twoBlocks))).contains ("uniform blocks"));

    ShaderReflection threeTextures;
    threeTextures.separateImages = { makeTexture ("first", 0, 0), makeTexture ("second", 0, 1), makeTexture ("third", 0, 2) };
    EXPECT_TRUE (getFailure (GpuImageShader::createFromBundle (makeBundle (threeTextures))).contains ("textures"));

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
