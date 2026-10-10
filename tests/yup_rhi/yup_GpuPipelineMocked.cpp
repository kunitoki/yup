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

#include <yup_graphics/yup_graphics.h>
#include <yup_rhi/yup_rhi.h>

using namespace yup;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnNull;

namespace
{

// ==============================================================================
// Helpers
// ==============================================================================

/** Creates a test ShaderModule with a populated binding map. */
rive::rcp<TestOreShaderModule> makeShaderModuleWithBindingMap (float slotVS = 0.f)
{
    auto mod = rive::make_rcp<TestOreShaderModule>();

    // Add a minimal uniform buffer binding so compile() has something to merge.
    rive::ore::BindingMap::Entry e;
    e.group = 0;
    e.binding = 0;
    e.kind = rive::ore::ResourceKind::UniformBuffer;
    e.stageMask = rive::ore::BindingMap::kStageVertex | rive::ore::BindingMap::kStageFragment;
    e.backendSlot[0] = 0; // VS
    e.backendSlot[1] = 0; // FS

    mod->m_bindingMap.push (e);
    mod->m_bindingMap.finalize();

    return mod;
}

/** Creates a GpuShaderSource with a populated binding-map sidecar. */
GpuShaderSource makeShaderSource (const char* code = "void main() {}")
{
    static rive::ore::BindingMap sMap;
    static std::vector<uint8_t> sBlob;

    if (sMap.empty())
    {
        rive::ore::BindingMap::Entry e;
        e.group = 0;
        e.binding = 0;
        e.kind = rive::ore::ResourceKind::UniformBuffer;
        e.stageMask = rive::ore::BindingMap::kStageVertex | rive::ore::BindingMap::kStageFragment;
        e.backendSlot[0] = 0;
        e.backendSlot[1] = 0;
        sMap.push (e);
        sMap.finalize();
        sBlob = sMap.toBlob();
    }

    GpuShaderSource src;
    src.language = GpuShaderLanguage::glsl;
    src.code = gpuShaderSourceBytes (code);
    src.bindingMap = sBlob;

    return src;
}

} // namespace

// ==============================================================================
// GpuPipeline mock-based tests
// ==============================================================================

class GpuPipelineMockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        ctx = new OreInjectedGpuDevice (mockOreCtx.get());
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    GpuDevice::Ptr ctx;
};

// --------------------------------------------------------------------------
// compile – success / failure paths
// --------------------------------------------------------------------------

TEST_F (GpuPipelineMockTests, CompileSucceedsWithValidShaders)
{
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();

    // Expect two makeShaderModule calls (VS then FS)
    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));

    // Expect makeBindGroupLayout for the merged group (group 0).
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));

    // Expect makePipeline
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    auto vs = makeShaderSource ("// VS");
    auto fs = makeShaderSource ("// FS");

    auto result = GpuPipeline::compile (ctx, vs, fs);
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
}

TEST_F (GpuPipelineMockTests, CompileFailsWhenVertexModuleIsNull)
{
    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (ReturnNull());

    auto vs = makeShaderSource ("// VS");
    auto fs = makeShaderSource ("// FS");

    auto result = GpuPipeline::compile (ctx, vs, fs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuPipelineMockTests, CompileFailsWhenFragmentModuleIsNull)
{
    auto vsModule = makeShaderModuleWithBindingMap();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (ReturnNull());

    auto vs = makeShaderSource ("// VS");
    auto fs = makeShaderSource ("// FS");

    auto result = GpuPipeline::compile (ctx, vs, fs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuPipelineMockTests, CompileFailsWhenPipelineCreationFails)
{
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));

    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));

    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (ReturnNull());

    auto vs = makeShaderSource ("// VS");
    auto fs = makeShaderSource ("// FS");

    auto result = GpuPipeline::compile (ctx, vs, fs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuPipelineMockTests, CompileValidatesEmptyVertexCode)
{
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillRepeatedly (Return (vsModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillRepeatedly (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillRepeatedly (Return (pipeline));

    GpuShaderSource vs;
    vs.language = GpuShaderLanguage::glsl;
    vs.bindingMap = makeShaderSource().bindingMap;

    auto fs = makeShaderSource ("// FS");

    auto result = GpuPipeline::compile (ctx, vs, fs);
    EXPECT_TRUE (result.failed());
}

// --------------------------------------------------------------------------
// compile – color targets and depth/stencil
// --------------------------------------------------------------------------

TEST_F (GpuPipelineMockTests, CompileWithColorTargetAndDepthStencil)
{
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    auto vs = makeShaderSource ("// VS");
    auto fs = makeShaderSource ("// FS");

    GpuBlendState blend;
    blend.srcColor = GpuBlendFactor::srcAlpha;
    blend.dstColor = GpuBlendFactor::oneMinusSrcAlpha;
    blend.colorOp = GpuBlendOp::add;
    blend.srcAlpha = GpuBlendFactor::one;
    blend.dstAlpha = GpuBlendFactor::oneMinusSrcAlpha;
    blend.alphaOp = GpuBlendOp::add;

    GpuColorTarget colorTarget;
    colorTarget.format = GpuTextureFormat::bgra8unorm;
    colorTarget.blendEnabled = true;
    colorTarget.blend = blend;

    GpuPipelineOptions options;
    options.colorTargets.push_back (colorTarget);
    options.depthStencil.enabled = true;
    options.depthStencil.format = GpuTextureFormat::depth32float;
    options.depthStencil.depthCompare = GpuCompareFunction::less;
    options.depthStencil.depthWriteEnabled = true;
    options.sampleCount = 4;

    auto result = GpuPipeline::compile (ctx, vs, fs, options);
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
}

// --------------------------------------------------------------------------
// compile – depth-only pipelines and depth bias
// --------------------------------------------------------------------------

class GpuPipelineDepthMockTests : public ::testing::Test
{
protected:
    /** A mocked ore context reporting depth bias clamp support. */
    class BiasClampOreContext : public MockOreContext
    {
    public:
        BiasClampOreContext() { m_features.depthBiasClamp = true; }
    };

    /** Compiles through @p oreCtx and returns the PipelineDesc handed to ore. */
    static rive::ore::PipelineDesc compileCapturing (MockOreContext& oreCtx, const GpuPipelineOptions& options)
    {
        rive::ore::PipelineDesc captured;

        EXPECT_CALL (oreCtx, makeShaderModule (_))
            .WillOnce (Return (makeShaderModuleWithBindingMap()))
            .WillOnce (Return (makeShaderModuleWithBindingMap()));
        EXPECT_CALL (oreCtx, makeBindGroupLayout (_))
            .WillOnce (Return (rive::make_rcp<TestOreBindGroupLayout>()));
        EXPECT_CALL (oreCtx, makePipeline (_, _))
            .WillOnce (Invoke ([&captured] (const rive::ore::PipelineDesc& desc, std::string*)
        {
            captured = desc;
            return rive::rcp<rive::ore::Pipeline> (rive::make_rcp<TestOrePipeline>());
        }));

        GpuDevice::Ptr device = new OreInjectedGpuDevice (&oreCtx);
        const auto result = GpuPipeline::compile (device, makeShaderSource ("// VS"), makeShaderSource ("// FS"), options);
        EXPECT_TRUE (result.wasOk());
        return captured;
    }

    static GpuPipelineOptions depthOnlyOptions()
    {
        GpuPipelineOptions options;
        options.depthStencil.enabled = true;
        options.depthStencil.format = GpuTextureFormat::depth32float;
        options.depthStencil.depthCompare = GpuCompareFunction::lessEqual;
        return options;
    }

    static GpuPipelineOptions biasedOptions()
    {
        auto options = depthOnlyOptions();
        options.depthStencil.depthBias = 4;
        options.depthStencil.depthBiasSlopeScale = 1.5f;
        options.depthStencil.depthBiasClamp = 0.01f;
        return options;
    }
};

TEST_F (GpuPipelineDepthMockTests, NoColorTargetsWithDepthBuildsADepthOnlyPipeline)
{
    NiceMock<MockOreContext> oreCtx;
    const auto desc = compileCapturing (oreCtx, depthOnlyOptions());

    EXPECT_EQ (desc.colorCount, 0u);
    EXPECT_EQ (desc.depthStencil.format, rive::ore::TextureFormat::depth32float);
    EXPECT_EQ (desc.depthStencil.depthCompare, rive::ore::CompareFunction::lessEqual);
}

TEST_F (GpuPipelineDepthMockTests, NoColorTargetsWithoutDepthKeepsTheDefaultTarget)
{
    NiceMock<MockOreContext> oreCtx;
    const auto desc = compileCapturing (oreCtx, GpuPipelineOptions {});

    EXPECT_EQ (desc.colorCount, 1u);
    EXPECT_EQ (desc.colorTargets[0].format, rive::ore::TextureFormat::rgba8unorm);
    EXPECT_TRUE (desc.colorTargets[0].blendEnabled);
}

TEST_F (GpuPipelineDepthMockTests, DepthBiasReachesTheBackend)
{
    NiceMock<BiasClampOreContext> oreCtx;
    const auto desc = compileCapturing (oreCtx, biasedOptions());

    EXPECT_EQ (desc.depthStencil.depthBias, 4);
    EXPECT_FLOAT_EQ (desc.depthStencil.depthBiasSlopeScale, 1.5f);
    EXPECT_FLOAT_EQ (desc.depthStencil.depthBiasClamp, 0.01f);
}

TEST_F (GpuPipelineDepthMockTests, DepthBiasClampIsDroppedWithoutBackendSupport)
{
    NiceMock<MockOreContext> oreCtx;
    const auto desc = compileCapturing (oreCtx, biasedOptions());

    EXPECT_EQ (desc.depthStencil.depthBias, 4);
    EXPECT_FLOAT_EQ (desc.depthStencil.depthBiasSlopeScale, 1.5f);
    EXPECT_FLOAT_EQ (desc.depthStencil.depthBiasClamp, 0.0f);
}

TEST_F (GpuPipelineMockTests, CompileWithVertexBuffers)
{
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    auto vs = makeShaderSource ("// VS");
    auto fs = makeShaderSource ("// FS");

    GpuPipelineOptions options;
    options.vertexBuffers.emplace_back (12, GpuVertexStepMode::vertex,
                                        std::vector<GpuVertexAttribute> { { GpuVertexFormat::float3, 0, 0 } });

    auto result = GpuPipeline::compile (ctx, vs, fs, options);
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
}

// --------------------------------------------------------------------------
// compile – multiple bind group layouts
// --------------------------------------------------------------------------

TEST_F (GpuPipelineMockTests, CompileWithMultipleBindGroups)
{
    // Create a shader module with entries in two groups.
    auto modWithTwoGroups = rive::make_rcp<TestOreShaderModule>();
    {
        rive::ore::BindingMap::Entry e0;
        e0.group = 0;
        e0.binding = 0;
        e0.kind = rive::ore::ResourceKind::UniformBuffer;
        e0.stageMask = rive::ore::BindingMap::kStageVertex;
        e0.backendSlot[0] = 0;
        modWithTwoGroups->m_bindingMap.push (e0);

        rive::ore::BindingMap::Entry e1;
        e1.group = 1;
        e1.binding = 0;
        e1.kind = rive::ore::ResourceKind::SampledTexture;
        e1.stageMask = rive::ore::BindingMap::kStageFragment;
        e1.backendSlot[1] = 0;
        e1.textureViewDim = rive::ore::TextureViewDim::D2;
        e1.textureSampleType = rive::ore::TextureSampleType::Float;
        modWithTwoGroups->m_bindingMap.push (e1);

        modWithTwoGroups->m_bindingMap.finalize();
    }

    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (modWithTwoGroups))
        .WillOnce (Return (makeShaderModuleWithBindingMap()));
    // Two BindGroupLayouts (group 0 and group 1)
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .Times (2)
        .WillRepeatedly (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    auto vs = makeShaderSource ("// VS");
    auto fs = makeShaderSource ("// FS");

    GpuPipelineOptions options;
    auto result = GpuPipeline::compile (ctx, vs, fs, options);
    ASSERT_TRUE (result.wasOk());
}

TEST_F (GpuPipelineMockTests, WgslLoadedTexturesGetUnfilterableLayouts)
{
    // The reflection types every non-comparison texture as filterable float, but WebGPU binds a
    // depth texture only to an unfilterable or depth layout entry: a texture the WGSL only loads
    // (texelFetch) is laid out unfilterable, while one it samples stays filterable
    auto fragmentModule = rive::make_rcp<TestOreShaderModule>();
    for (const auto& [binding, kind, dimension] : { std::tuple { 0, rive::ore::ResourceKind::SampledTexture, rive::ore::TextureViewDim::D2Array },
                                                    std::tuple { 1, rive::ore::ResourceKind::SampledTexture, rive::ore::TextureViewDim::D2 },
                                                    std::tuple { 2, rive::ore::ResourceKind::Sampler, rive::ore::TextureViewDim::D2 } })
    {
        rive::ore::BindingMap::Entry e;
        e.group = 1;
        e.binding = static_cast<uint8_t> (binding);
        e.kind = kind;
        e.stageMask = rive::ore::BindingMap::kStageFragment;
        e.backendSlot[1] = static_cast<uint16_t> (binding);
        e.textureViewDim = dimension;
        e.textureSampleType = rive::ore::TextureSampleType::Float;
        fragmentModule->m_bindingMap.push (e);
    }
    fragmentModule->m_bindingMap.finalize();

    std::vector<rive::ore::BindGroupLayoutEntry> groupOne;
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (makeShaderModuleWithBindingMap()))
        .WillOnce (Return (fragmentModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillRepeatedly (Invoke ([&] (const rive::ore::BindGroupLayoutDesc& desc)
        {
            if (desc.groupIndex == 1)
                groupOne.assign (desc.entries, desc.entries + desc.entryCount);

            return bgl;
        }));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (rive::make_rcp<TestOrePipeline>()));

    auto vs = makeShaderSource ("@vertex fn main() -> @builtin(position) vec4f { return vec4f(0.0); }");
    vs.language = GpuShaderLanguage::wgsl;

    auto fs = makeShaderSource (R"(@group(1) @binding(0) var rawDepth : texture_2d_array<f32>;
@group(1) @binding(1) var color : texture_2d<f32>;
@group(1) @binding(2) var colorSampler : sampler;
@fragment fn main() -> @location(0) vec4f
{
    return textureLoad(rawDepth, vec2i(0), 0, 0) + textureSample(color, colorSampler, vec2f(0.5));
})");
    fs.language = GpuShaderLanguage::wgsl;

    ASSERT_TRUE (GpuPipeline::compile (ctx, vs, fs).wasOk());
    ASSERT_EQ (groupOne.size(), 3u);

    using SampleType = rive::ore::BindGroupLayoutEntry::SampleType;
    for (const auto& entry : groupOne)
    {
        if (entry.binding == 0)
            EXPECT_EQ (entry.textureSampleType, SampleType::floatUnfilterable);
        else if (entry.binding == 1)
            EXPECT_EQ (entry.textureSampleType, SampleType::floatFilterable);
        else
            EXPECT_EQ (entry.kind, rive::ore::BindingKind::sampler);
    }
}

// ==============================================================================
// GpuBuffer mock-based tests
// ==============================================================================

class GpuBufferMockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        ctx = new OreInjectedGpuDevice (mockOreCtx.get());
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    GpuDevice::Ptr ctx;
};

TEST_F (GpuBufferMockTests, CreateSucceedsWithValidData)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();

    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));

    const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buf = GpuBuffer::create (ctx, GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::vertex);
    EXPECT_EQ (buf->getSizeInBytes(), sizeof (data));
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuBufferMockTests, CreateSucceedsForIndexBuffer)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();

    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));

    const uint16_t data[] = { 0, 1, 2, 3 };
    auto buf = GpuBuffer::create (ctx, GpuBufferType::index, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::index);
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuBufferMockTests, CreateSucceedsForUniformBuffer)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();

    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));

    const int data[] = { 42 };
    auto buf = GpuBuffer::create (ctx, GpuBufferType::uniform, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::uniform);
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuBufferMockTests, CreateReturnsNullWhenMakeBufferFails)
{
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (ReturnNull());

    const float data[] = { 1.0f };
    auto buf = GpuBuffer::create (ctx, GpuBufferType::vertex, data, sizeof (data));
    EXPECT_EQ (buf, nullptr);
}

TEST_F (GpuBufferMockTests, UpdateBufferOnVertexBufferSucceeds)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));
    EXPECT_CALL (*oreBuf, update (_, _, _));

    const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buf = GpuBuffer::create (ctx, GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buf, nullptr);

    const float newData[] = { 5.0f, 6.0f, 7.0f, 8.0f };
    EXPECT_TRUE (ctx->updateBuffer (buf, newData, sizeof (newData)));
}

TEST_F (GpuBufferMockTests, CreateFailsWithStorageType)
{
    const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buf = GpuBuffer::create (ctx, GpuBufferType::storage, data, sizeof (data));
    EXPECT_EQ (buf, nullptr);
}

// ==============================================================================
// GpuDevice — mock-based createBuffer / updateBuffer tests
// ==============================================================================

class GpuDeviceMockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        ctx = new OreInjectedGpuDevice (mockOreCtx.get());
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    GpuDevice::Ptr ctx;
};

TEST_F (GpuDeviceMockTests, CreateBufferVertexSucceeds)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));

    const float data[] = { 1.0f, 2.0f };
    auto buf = ctx->createBuffer (GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::vertex);
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuDeviceMockTests, CreateBufferIndexSucceeds)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));

    const uint16_t data[] = { 0, 1, 2 };
    auto buf = ctx->createBuffer (GpuBufferType::index, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::index);
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuDeviceMockTests, CreateBufferUniformSucceeds)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));

    const int data[] = { 42, 43 };
    auto buf = ctx->createBuffer (GpuBufferType::uniform, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::uniform);
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuDeviceMockTests, CreateBufferReturnsNullWhenOreMakeBufferFails)
{
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (ReturnNull());

    const float data[] = { 1.0f };
    auto buf = ctx->createBuffer (GpuBufferType::vertex, data, sizeof (data));
    EXPECT_EQ (buf, nullptr);
}

TEST_F (GpuDeviceMockTests, UpdateBufferWithNullBufferReturnsFalse)
{
    const float data[] = { 1.0f };
    EXPECT_FALSE (ctx->updateBuffer (nullptr, data, sizeof (data)));
}

TEST_F (GpuDeviceMockTests, UpdateBufferWithNullDataReturnsFalse)
{
    EXPECT_FALSE (ctx->updateBuffer (nullptr, nullptr, 0));
}

TEST_F (GpuDeviceMockTests, UpdateBufferWithZeroSizeReturnsFalse)
{
    const float data[] = { 1.0f };
    EXPECT_FALSE (ctx->updateBuffer (nullptr, data, 0));
}

TEST_F (GpuDeviceMockTests, UpdateBufferOnValidOreBackedBufferSucceeds)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));
    EXPECT_CALL (*oreBuf, update (_, _, _));

    const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buf = ctx->createBuffer (GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buf, nullptr);

    const float newData[] = { 5.0f, 6.0f, 7.0f, 8.0f };
    EXPECT_TRUE (ctx->updateBuffer (buf, newData, sizeof (newData)));
}

TEST_F (GpuDeviceMockTests, UpdateBufferLargerThanOriginalReturnsFalse)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));

    const float data[] = { 1.0f };
    auto buf = ctx->createBuffer (GpuBufferType::vertex, data, sizeof (data));
    ASSERT_NE (buf, nullptr);

    const float largerData[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    EXPECT_FALSE (ctx->updateBuffer (buf, largerData, sizeof (largerData)));
}

class GpuFrameMockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        ctx = new OreInjectedGpuDevice (mockOreCtx.get());
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    GpuDevice::Ptr ctx;
};

TEST_F (GpuFrameMockTests, BeginCallsOreBeginFrame)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));

    auto frame = GpuFrame::begin (ctx);
    EXPECT_TRUE (frame.isValid());
}

TEST_F (GpuFrameMockTests, BeginWithNullGpuContextReturnsInvalidFrame)
{
    auto headless = GpuDevice::create (GpuPlatform::Headless, {});
    ASSERT_NE (headless, nullptr);

    auto frame = GpuFrame::begin (headless);
    EXPECT_FALSE (frame.isValid());
}

TEST_F (GpuFrameMockTests, MoveConstructionFromValidFrameTransfersState)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));

    auto src = GpuFrame::begin (ctx);
    ASSERT_TRUE (src.isValid());

    GpuFrame dst (std::move (src));
    EXPECT_TRUE (dst.isValid());
    EXPECT_FALSE (src.isValid());
}

TEST_F (GpuFrameMockTests, WaitForGpuOnInvalidFrameDoesNotCrash)
{
    auto headless = GpuDevice::create (GpuPlatform::Headless, {});
    auto frame = GpuFrame::begin (headless);
    EXPECT_FALSE (frame.isValid());

    EXPECT_NO_THROW (frame.waitForGPU());
}

TEST_F (GpuFrameMockTests, SubmitCallsOreEndFrame)
{
    // beginFrame + endFrame
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());

    auto frame = GpuFrame::begin (ctx);
    ASSERT_TRUE (frame.isValid());
    EXPECT_TRUE (frame.submit());
}

TEST_F (GpuFrameMockTests, ComputePassOnSubmittedFrameIsInvalid)
{
    auto frame = GpuFrame::begin (ctx);
    ASSERT_TRUE (frame.isValid());
    ASSERT_TRUE (frame.submit());

    auto pass = GpuComputePass::begin (frame);
    EXPECT_FALSE (pass.isValid());
}

TEST_F (GpuFrameMockTests, SubmitIsIdempotent)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());

    auto frame = GpuFrame::begin (ctx);
    ASSERT_TRUE (frame.isValid());
    EXPECT_TRUE (frame.submit());
    EXPECT_FALSE (frame.submit());
}

TEST_F (GpuFrameMockTests, WaitForGpuCallsOreWaitForGPU)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());
    EXPECT_CALL (*mockOreCtx, waitForGPU());

    auto frame = GpuFrame::begin (ctx);
    ASSERT_TRUE (frame.isValid());
    frame.submit();
    frame.waitForGPU();
}

TEST_F (GpuFrameMockTests, WaitForGpuIsIdempotent)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());

    // A GPU sync is expensive, so repeated waits must collapse into the single
    // stall the first one already paid for.
    EXPECT_CALL (*mockOreCtx, waitForGPU());

    auto frame = GpuFrame::begin (ctx);
    ASSERT_TRUE (frame.isValid());
    frame.submit();
    frame.waitForGPU();
    frame.waitForGPU();
}

TEST_F (GpuFrameMockTests, DestructorSubmitsWithoutStallingOnTheGpu)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());

    // The frame's transient resources are handed to the device to be freed a few
    // generations later, so the destructor must never pay for a pipeline stall.
    // Waiting here used to cost one on every rendered frame.
    EXPECT_CALL (*mockOreCtx, waitForGPU()).Times (0);

    {
        auto frame = GpuFrame::begin (ctx);
        ASSERT_TRUE (frame.isValid());
        // Not explicitly submitted — destructor does it.
    }
}

TEST_F (GpuFrameMockTests, FrameNumbersAdvanceAndTrailASafeGeneration)
{
    std::vector<rive::ore::Context::FrameDescriptor> descriptors;

    EXPECT_CALL (*mockOreCtx, beginFrame (_))
        .WillRepeatedly (Invoke ([&descriptors] (const rive::ore::Context::FrameDescriptor& desc)
    {
        descriptors.push_back (desc);
    }));

    for (int i = 0; i < 6; ++i)
    {
        auto frame = GpuFrame::begin (ctx);
        ASSERT_TRUE (frame.isValid());
        frame.submit();
    }

    ASSERT_EQ (descriptors.size(), 6u);

    // The current frame number is monotonic, and the safe frame number trails it
    // by the in-flight depth - it is what tells a manager-backed backend which of
    // its own resources the GPU can no longer be reading. Both used to be zero on
    // every single frame.
    for (size_t i = 0; i < descriptors.size(); ++i)
    {
        EXPECT_EQ (descriptors[i].currentFrameNumber, (uint64_t) (i + 1));
        EXPECT_LT (descriptors[i].safeFrameNumber, descriptors[i].currentFrameNumber);

        if (i > 0)
            EXPECT_GE (descriptors[i].safeFrameNumber, descriptors[i - 1].safeFrameNumber);
    }

    EXPECT_EQ (descriptors.front().safeFrameNumber, 0u);
    EXPECT_GT (descriptors.back().safeFrameNumber, 0u);
}

TEST_F (GpuFrameMockTests, MoveAssignmentSubmitsExisting)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_)).Times (2);
    EXPECT_CALL (*mockOreCtx, endFrame()).Times (2);

    auto src = GpuFrame::begin (ctx);
    auto dst = GpuFrame::begin (ctx);

    dst = std::move (src);
}

// ==============================================================================
// GpuPipeline::compileFromBundle — mock-based tests
// ==============================================================================

class GpuPipelineBundleMockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        ctx = new OreInjectedGpuDevice (mockOreCtx.get());
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    GpuDevice::Ptr ctx;
};

TEST_F (GpuPipelineBundleMockTests, CompileFromBundleSucceeds)
{
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    ShaderBundle bundle;

    ShaderInfo vs;
    vs.stage = ShaderStage::vertex;
    vs.language = ShaderLanguage::glsl;
    vs.entryPoint = "main";
    vs.source = "// vertex shader";

    ShaderReflection::ResourceBinding ub;
    ub.name = "Uniforms";
    ub.set = 0;
    ub.binding = 0;
    ub.backendSlot = 0;
    ub.type = ShaderReflection::ResourceType::uniformBuffer;
    vs.reflection.uniformBuffers.push_back (ub);
    bundle.addShader (vs);

    ShaderInfo fs;
    fs.stage = ShaderStage::fragment;
    fs.language = ShaderLanguage::glsl;
    fs.entryPoint = "main";
    fs.source = "// fragment shader";
    fs.reflection.uniformBuffers.push_back (ub);
    bundle.addShader (fs);

    auto result = GpuPipeline::compileFromBundle (ctx, bundle);
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
}

TEST_F (GpuPipelineBundleMockTests, CompileFromBundleFailsWhenNoVertexShader)
{
    ShaderBundle bundle;

    ShaderInfo fs;
    fs.stage = ShaderStage::fragment;
    fs.language = ShaderLanguage::glsl;
    fs.entryPoint = "main";
    fs.source = "// fragment shader";
    bundle.addShader (fs);

    auto result = GpuPipeline::compileFromBundle (ctx, bundle);
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuPipelineBundleMockTests, CompileFromBundleFailsWhenNoFragmentShader)
{
    ShaderBundle bundle;

    ShaderInfo vs;
    vs.stage = ShaderStage::vertex;
    vs.language = ShaderLanguage::glsl;
    vs.entryPoint = "main";
    vs.source = "// vertex shader";
    bundle.addShader (vs);

    auto result = GpuPipeline::compileFromBundle (ctx, bundle);
    EXPECT_TRUE (result.failed());
}

// ==============================================================================
// GpuRenderPass — mock-based tests
// ==============================================================================

class GpuRenderPassMockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        ctx = new OreAndTargetGpuDevice (mockOreCtx.get(), MockOffscreenTarget::withGpuTexture (256, 128));
        headlessCtx = yup::GpuDevice::create (yup::GpuPlatform::Headless, {});
    }

    GpuFrame makeValidFrame()
    {
        EXPECT_CALL (*mockOreCtx, beginFrame (_));
        return GpuFrame::begin (ctx);
    }

    GpuFrame makeInvalidFrame()
    {
        return GpuFrame::begin (headlessCtx);
    }

    /** A resource both stages of a pipeline declare, on the same native slot. */
    static rive::ore::BindingMap::Entry makeBinding (uint32_t group, uint32_t binding, rive::ore::ResourceKind kind, uint16_t slot)
    {
        rive::ore::BindingMap::Entry e;
        e.group = group;
        e.binding = binding;
        e.kind = kind;
        e.stageMask = rive::ore::BindingMap::kStageVertex | rive::ore::BindingMap::kStageFragment;
        e.backendSlot[0] = slot;
        e.backendSlot[1] = slot;
        return e;
    }

    /** Compiles a pipeline whose stages declare @p bindings, with one layout per declared group
        mirroring the description compile() asks for. */
    GpuPipeline::Ptr compilePipeline (const std::vector<rive::ore::BindingMap::Entry>& bindings)
    {
        auto module = rive::make_rcp<TestOreShaderModule>();
        for (const auto& e : bindings)
            module->m_bindingMap.push (e);
        module->m_bindingMap.finalize();

        EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
            .Times (2)
            .WillRepeatedly (Return (module));
        ON_CALL (*mockOreCtx, makeBindGroupLayout (_))
            .WillByDefault (Invoke ([] (const rive::ore::BindGroupLayoutDesc& desc) -> rive::rcp<rive::ore::BindGroupLayout>
        {
            return rive::make_rcp<TestOreBindGroupLayout> (desc);
        }));
        EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
            .WillOnce (Return (rive::make_rcp<TestOrePipeline>()));

        auto compileResult = GpuPipeline::compile (ctx, makeShaderSource ("// VS"), makeShaderSource ("// FS"));
        EXPECT_TRUE (compileResult.wasOk());
        return compileResult.getValue();
    }

    /** Compiles a pipeline declaring one uniform buffer at binding 0 of groups 0, 1 and 2, on
        native slots counted up from @p firstSlot. */
    GpuPipeline::Ptr compileThreeGroupPipeline (uint16_t firstSlot = 0)
    {
        std::vector<rive::ore::BindingMap::Entry> bindings;
        for (uint32_t group = 0; group < 3; ++group)
            bindings.push_back (makeBinding (group, 0, rive::ore::ResourceKind::UniformBuffer, (uint16_t) (firstSlot + group)));

        return compilePipeline (bindings);
    }

    /** Serves the passes, views and uniform buffers draws ask for, counting bind groups made and bound. */
    void serveDraws (int& bindGroupsMade, int& bindGroupsBound)
    {
        ON_CALL (*mockOreCtx, wrapRiveTexture (_, _, _))
            .WillByDefault (Invoke ([] (rive::gpu::Texture*, uint32_t, uint32_t)
        {
            return rive::make_rcp<TestOreTextureView>();
        }));
        ON_CALL (*mockOreCtx, beginRenderPass (_, _))
            .WillByDefault (Invoke ([&bindGroupsBound] (const rive::ore::RenderPassDesc&, std::string*)
        {
            auto* renderPass = new NiceMock<MockOreRenderPass>();
            ON_CALL (*renderPass, setBindGroup (_, _, _, _))
                .WillByDefault (Invoke ([&bindGroupsBound] (uint32_t, rive::ore::BindGroup*, const uint32_t*, uint32_t)
            {
                ++bindGroupsBound;
            }));
            return std::unique_ptr<rive::ore::RenderPass> (renderPass);
        }));
        ON_CALL (*mockOreCtx, makeBuffer (_))
            .WillByDefault (Invoke ([] (const rive::ore::BufferDesc& desc) -> rive::rcp<rive::ore::Buffer>
        {
            return rive::make_rcp<NiceMock<MockOreBuffer>> (desc.size);
        }));
        ON_CALL (*mockOreCtx, makeBindGroup (_))
            .WillByDefault (Invoke ([&bindGroupsMade] (const rive::ore::BindGroupDesc&) -> rive::rcp<rive::ore::BindGroup>
        {
            ++bindGroupsMade;
            return rive::make_rcp<TestOreBindGroup>();
        }));
    }

    /** The state calls the backend passes received, over every pass begun. */
    struct EncodedCalls
    {
        int passes = 0;
        int pipelines = 0;
        int bindGroups = 0;
        int viewports = 0;
        int scissors = 0;
        int stencilReferences = 0;
        int blendColors = 0;
        int bindGroupsToFail = 0; ///< The next binds to fail, as an out of memory descriptor pool does
    };

    /** A mocked backend pass that finish() finishes, as the real ones do. */
    class FinishingOreRenderPass : public NiceMock<MockOreRenderPass>
    {
    public:
        FinishingOreRenderPass()
        {
            ON_CALL (*this, finish()).WillByDefault (Invoke ([this] { m_finished = true; }));
        }
    };

    /** Like serveDraws(), counting the state calls every backend pass receives into @p calls. */
    void serveCountedDraws (EncodedCalls& calls)
    {
        serveDraws (unusedMade, unusedBound);

        ON_CALL (*mockOreCtx, beginRenderPass (_, _))
            .WillByDefault (Invoke ([&calls, oreCtx = mockOreCtx.get()] (const rive::ore::RenderPassDesc&, std::string*)
        {
            ++calls.passes;
            auto* renderPass = new FinishingOreRenderPass();
            ON_CALL (*renderPass, setPipeline (_)).WillByDefault (Invoke ([&calls] (rive::ore::Pipeline*) { ++calls.pipelines; }));
            ON_CALL (*renderPass, setBindGroup (_, _, _, _)).WillByDefault (Invoke ([&calls, oreCtx] (uint32_t, rive::ore::BindGroup*, const uint32_t*, uint32_t)
            {
                ++calls.bindGroups;
                if (calls.bindGroupsToFail > 0)
                {
                    --calls.bindGroupsToFail;
                    oreCtx->setLastError ("bind group failed");
                }
            }));
            ON_CALL (*renderPass, setStencilReference (_)).WillByDefault (Invoke ([&calls] (uint32_t) { ++calls.stencilReferences; }));
            ON_CALL (*renderPass, setBlendColor (_, _, _, _)).WillByDefault (Invoke ([&calls] (float, float, float, float) { ++calls.blendColors; }));
            ON_CALL (*renderPass, setViewport (_, _, _, _, _, _)).WillByDefault (Invoke ([&calls] (float, float, float, float, float, float) { ++calls.viewports; }));
            ON_CALL (*renderPass, setScissorRect (_, _, _, _)).WillByDefault (Invoke ([&calls] (uint32_t, uint32_t, uint32_t, uint32_t) { ++calls.scissors; }));
            return std::unique_ptr<rive::ore::RenderPass> (renderPass);
        }));
    }

    int unusedMade = 0;
    int unusedBound = 0;

    /** A mocked device reporting fragment storage buffers, so setStorageBuffer() keeps its bindings. */
    class StorageBufferGpuDevice : public OreAndTargetGpuDevice
    {
    public:
        using OreAndTargetGpuDevice::OreAndTargetGpuDevice;

        bool isFragmentStorageBufferAvailable() const noexcept override { return true; }
    };

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    GpuDevice::Ptr ctx;
    yup::GpuDevice::Ptr headlessCtx;
};

TEST_F (GpuRenderPassMockTests, BeginRenderPassWithInvalidFrameReturnsInvalidPass)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto invalid = makeInvalidFrame();
    auto pass = target->beginRenderPass (invalid);
    EXPECT_FALSE (pass.isValid());
    EXPECT_FALSE (pass.draw (3));
    EXPECT_FALSE (pass.drawIndexed (3));
    EXPECT_FALSE (pass.finish());
}

TEST_F (GpuRenderPassMockTests, BeginRenderPassWithValidFrameReturnsValidPass)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto valid = makeValidFrame();
    auto pass = target->beginRenderPass (valid);
    EXPECT_TRUE (pass.isValid());

    pass.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, SetPipelineOnValidPassDoesNotCrash)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    // Compile a real pipeline via mocks for the setPipeline test.
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    auto compileResult = GpuPipeline::compile (ctx, makeShaderSource ("// VS"), makeShaderSource ("// FS"));
    ASSERT_TRUE (compileResult.wasOk());
    auto* compiledPipeline = compileResult.getValue().get();

    auto valid = makeValidFrame();
    auto pass = target->beginRenderPass (valid);
    EXPECT_TRUE (pass.isValid());

    EXPECT_NO_THROW (pass.setPipeline (*compiledPipeline));

    int dummy = 0;
    EXPECT_NO_THROW ({
        pass.setTexture (0, 0, nullptr);
        pass.setUniformBuffer (0, 0, &dummy, sizeof (dummy));
    });

    pass.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, SetPipelineOnInvalidPassDoesNotCrash)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto invalid = makeInvalidFrame();
    auto pass = target->beginRenderPass (invalid);
    EXPECT_FALSE (pass.isValid());

    int dummy = 0;
    EXPECT_NO_THROW ({
        pass.setTexture (0, 0, nullptr);
        pass.setUniformBuffer (0, 0, &dummy, sizeof (dummy));
    });
}

TEST_F (GpuRenderPassMockTests, SetTextureOnInvalidPassDoesNotCrash)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto invalid = makeInvalidFrame();
    auto pass = target->beginRenderPass (invalid);
    EXPECT_FALSE (pass.isValid());

    int dummy = 0;
    EXPECT_NO_THROW ({
        pass.setTexture (0, 0, nullptr);
        pass.setUniformBuffer (0, 0, &dummy, sizeof (dummy));
        pass.setVertexBuffer (0, nullptr);
        pass.setIndexBuffer (GpuIndexFormat::uint16, nullptr);
    });
}

TEST_F (GpuRenderPassMockTests, MoveConstructionPreservesInvalidState)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto invalid = makeInvalidFrame();
    auto src = target->beginRenderPass (invalid);
    EXPECT_FALSE (src.isValid());

    GpuRenderPass dst (std::move (src));
    EXPECT_FALSE (dst.isValid());
    EXPECT_FALSE (dst.draw (3));
}

TEST_F (GpuRenderPassMockTests, MoveAssignmentPreservesInvalidState)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto invalid = makeInvalidFrame();
    auto src = target->beginRenderPass (invalid);
    auto dst = target->beginRenderPass (invalid);

    dst = std::move (src);
    EXPECT_FALSE (dst.isValid());
}

TEST_F (GpuRenderPassMockTests, FinishIsIdempotentOnInvalidPass)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto invalid = makeInvalidFrame();
    auto pass = target->beginRenderPass (invalid);
    EXPECT_FALSE (pass.finish());
    EXPECT_FALSE (pass.finish());
}

TEST_F (GpuRenderPassMockTests, DestructorDoesNotCrashOnInvalidPass)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    {
        auto invalid = makeInvalidFrame();
        auto pass = target->beginRenderPass (invalid);
        EXPECT_FALSE (pass.isValid());
    }
    EXPECT_TRUE (true);
}

TEST_F (GpuRenderPassMockTests, DrawEndToEndWithValidPipeline)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    // Compile a pipeline
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    auto compileResult = GpuPipeline::compile (ctx, makeShaderSource ("// VS"), makeShaderSource ("// FS"));
    ASSERT_TRUE (compileResult.wasOk());
    auto* compiledPipeline = compileResult.getValue().get();

    // Set up the ore render pass mock for the draw call.
    auto mockRenderPass = std::make_unique<NiceMock<MockOreRenderPass>>();
    EXPECT_CALL (*mockRenderPass, setPipeline (_));
    EXPECT_CALL (*mockRenderPass, setViewport (_, _, _, _, _, _));
    EXPECT_CALL (*mockRenderPass, draw (_, _, _, _));
    EXPECT_CALL (*mockRenderPass, finish());

    // wrapRiveTexture returns a valid texture view for the output texture.
    auto texView = rive::make_rcp<TestOreTextureView>();
    EXPECT_CALL (*mockOreCtx, wrapRiveTexture (_, _, _))
        .WillOnce (Return (texView));
    EXPECT_CALL (*mockOreCtx, beginRenderPass (_, _))
        .WillOnce (Return (std::move (mockRenderPass)));

    auto valid = makeValidFrame();
    auto pass = target->beginRenderPass (valid);
    ASSERT_TRUE (pass.isValid());

    pass.setPipeline (*compiledPipeline);
    EXPECT_TRUE (pass.draw (4));

    pass.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, DeclaredSamplerIsCreatedWhileCompilingAndReusedByEveryDraw)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();
    bgl->addEntry (1, rive::ore::BindingKind::sampler);

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (makeShaderModuleWithBindingMap()))
        .WillOnce (Return (makeShaderModuleWithBindingMap()));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (rive::make_rcp<TestOrePipeline>()));

    // The one declared sampler binding is filled by a sampler created up front,
    // so no draw ever asks for another one.
    EXPECT_CALL (*mockOreCtx, makeSampler (_))
        .Times (1)
        .WillOnce (Return (rive::make_rcp<TestOreSampler>()));

    auto compileResult = GpuPipeline::compile (ctx, makeShaderSource ("// VS"), makeShaderSource ("// FS"));
    ASSERT_TRUE (compileResult.wasOk());
    auto compiled = compileResult.getValue();

    ON_CALL (*mockOreCtx, wrapRiveTexture (_, _, _))
        .WillByDefault (Invoke ([] (rive::gpu::Texture*, uint32_t, uint32_t)
    {
        return rive::make_rcp<TestOreTextureView>();
    }));
    ON_CALL (*mockOreCtx, beginRenderPass (_, _))
        .WillByDefault (Invoke ([] (const rive::ore::RenderPassDesc&, std::string*)
    {
        return std::unique_ptr<rive::ore::RenderPass> (new NiceMock<MockOreRenderPass>());
    }));
    ON_CALL (*mockOreCtx, makeBindGroup (_))
        .WillByDefault (Invoke ([] (const rive::ore::BindGroupDesc& desc)
    {
        // The sampler slot is always populated from the pipeline.
        EXPECT_EQ (1u, desc.samplerCount);
        return rive::make_rcp<TestOreBindGroup>();
    }));

    auto frame = makeValidFrame();

    for (int draw = 0; draw < 3; ++draw)
    {
        auto pass = target->beginRenderPass (frame);
        ASSERT_TRUE (pass.isValid());
        pass.setPipeline (compiled);
        EXPECT_TRUE (pass.draw (3));
        pass.finish();
    }

    frame.submit();
}

TEST_F (GpuRenderPassMockTests, UniformBuffersAreRecycledAcrossFrames)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();
    bgl->addEntry (0, rive::ore::BindingKind::uniformBuffer);

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (makeShaderModuleWithBindingMap()))
        .WillOnce (Return (makeShaderModuleWithBindingMap()));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (rive::make_rcp<TestOrePipeline>()));

    // Two draws per frame need two distinct buffers, but the second frame gets
    // both of them back from the pool - so only two are ever created. The
    // uploads performed on each acquired buffer are incidental to what this
    // test verifies, so they use NiceMock to stay quiet.
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .Times (2)
        .WillRepeatedly (Invoke ([] (const rive::ore::BufferDesc& desc)
    {
        return rive::make_rcp<NiceMock<MockOreBuffer>> (desc.size);
    }));

    auto compileResult = GpuPipeline::compile (ctx, makeShaderSource ("// VS"), makeShaderSource ("// FS"));
    ASSERT_TRUE (compileResult.wasOk());
    auto compiled = compileResult.getValue();

    ON_CALL (*mockOreCtx, wrapRiveTexture (_, _, _))
        .WillByDefault (Invoke ([] (rive::gpu::Texture*, uint32_t, uint32_t)
    {
        return rive::make_rcp<TestOreTextureView>();
    }));
    ON_CALL (*mockOreCtx, beginRenderPass (_, _))
        .WillByDefault (Invoke ([] (const rive::ore::RenderPassDesc&, std::string*)
    {
        return std::unique_ptr<rive::ore::RenderPass> (new NiceMock<MockOreRenderPass>());
    }));
    ON_CALL (*mockOreCtx, makeBindGroup (_))
        .WillByDefault (Return (rive::make_rcp<TestOreBindGroup>()));

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    for (int frameIndex = 0; frameIndex < 2; ++frameIndex)
    {
        auto frame = makeValidFrame();

        for (int draw = 0; draw < 2; ++draw)
        {
            auto pass = target->beginRenderPass (frame);
            ASSERT_TRUE (pass.isValid());
            pass.setPipeline (compiled);
            pass.setUniformBuffer (0, 0, uniforms, sizeof (uniforms));
            EXPECT_TRUE (pass.draw (3));
            pass.finish();
        }

        frame.submit();
        frame.waitForGPU();
    }
}

TEST_F (GpuRenderPassMockTests, UnchangedGroupsReuseTheirBindGroup)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto compiled = compileThreeGroupPipeline();
    ASSERT_NE (compiled, nullptr);

    int made = 0, bound = 0;
    serveDraws (made, bound);

    const float view[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float drawA[4] = { 5.0f, 6.0f, 7.0f, 8.0f };
    const float drawB[4] = { 9.0f, 10.0f, 11.0f, 12.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (compiled);
    pass.setUniformBuffer (0, 0, view, sizeof (view));
    pass.setUniformBuffer (1, 0, view, sizeof (view));
    pass.setUniformBuffer (2, 0, drawA, sizeof (drawA));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 3);
    EXPECT_EQ (bound, 3);

    pass.setUniformBuffer (2, 0, drawB, sizeof (drawB));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 4);
    EXPECT_EQ (bound, 4);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, IdenticalUniformBytesDoNotRebuildTheGroup)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto compiled = compileThreeGroupPipeline();
    ASSERT_NE (compiled, nullptr);

    int made = 0, bound = 0;
    serveDraws (made, bound);

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float same[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (compiled);
    for (int group = 0; group < 3; ++group)
        pass.setUniformBuffer (group, 0, uniforms, sizeof (uniforms));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 3);

    pass.setUniformBuffer (0, 0, same, sizeof (same));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 3);
    EXPECT_EQ (bound, 3);

    // Same leading bytes but a different size is a change
    pass.setUniformBuffer (0, 0, same, sizeof (float) * 2);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 4);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, EverySetterDirtiesOnlyItsGroup)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto compiled = compileThreeGroupPipeline();
    ASSERT_NE (compiled, nullptr);

    int made = 0, bound = 0;
    serveDraws (made, bound);

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (compiled);
    for (int group = 0; group < 3; ++group)
        pass.setUniformBuffer (group, 0, uniforms, sizeof (uniforms));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 3);

    pass.setTexture (1, 1, nullptr);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 4);

    pass.setTexture (2, 1, nullptr, GpuTextureViewDesc());
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 5);

    pass.setSampler (0, 2, nullptr);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 6);

    // A group the pipeline does not declare touches no bind group
    pass.setTexture (7, 0, nullptr);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 6);
    EXPECT_EQ (bound, 6);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, ChangingThePipelineLayoutRebuildsEveryGroup)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    // Every group of the second pipeline declares the same resource on another native slot
    auto first = compileThreeGroupPipeline();
    auto second = compileThreeGroupPipeline (3);
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    int made = 0, bound = 0;
    serveDraws (made, bound);

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (first);
    for (int group = 0; group < 3; ++group)
        pass.setUniformBuffer (group, 0, uniforms, sizeof (uniforms));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 3);

    pass.setPipeline (first);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 3);

    pass.setPipeline (second);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 6);

    pass.setPipeline (first);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 9);
    EXPECT_EQ (bound, 9);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, DrawsEncodeOnlyTheStateThatChanged)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto first = compileThreeGroupPipeline();
    auto second = compileThreeGroupPipeline (3);
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    EncodedCalls calls;
    serveCountedDraws (calls);

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    const float changed[4] = { 5.0f, 6.0f, 7.0f, 8.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (first);
    for (int group = 0; group < 3; ++group)
        pass.setUniformBuffer (group, 0, uniforms, sizeof (uniforms));

    EXPECT_TRUE (pass.draw (3));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.pipelines, 1);
    EXPECT_EQ (calls.bindGroups, 3);
    EXPECT_EQ (calls.viewports, 1);

    // Only the group whose bind group changed is bound again
    pass.setUniformBuffer (2, 0, changed, sizeof (changed));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.pipelines, 1);
    EXPECT_EQ (calls.bindGroups, 4);
    EXPECT_EQ (calls.viewports, 1);

    // A new viewport or scissor is applied once, by the next draw
    pass.setViewport (0.0f, 0.0f, 64.0f, 64.0f);
    pass.setScissorRect (0, 0, 32, 32);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.viewports, 2);
    EXPECT_EQ (calls.scissors, 1);

    // So are a new stencil reference and blend color
    pass.setStencilReference (1);
    pass.setBlendColor (GpuColor (0.5f, 0.5f, 0.5f, 1.0f));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.stencilReferences, 1);
    EXPECT_EQ (calls.blendColors, 1);

    pass.setStencilReference (2);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.stencilReferences, 2);

    pass.setBlendColor (GpuColor (0.25f, 0.5f, 0.5f, 1.0f));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.blendColors, 3);
    EXPECT_EQ (calls.stencilReferences, 3);

    // Setting the same pipeline again encodes nothing
    pass.setPipeline (first);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.pipelines, 1);
    EXPECT_EQ (calls.bindGroups, 4);

    // Another pipeline is encoded with every group and the pass state again
    pass.setPipeline (second);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.pipelines, 2);
    EXPECT_EQ (calls.bindGroups, 7);
    EXPECT_EQ (calls.viewports, 6);
    EXPECT_EQ (calls.scissors, 5);
    EXPECT_EQ (calls.passes, 1);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, AFailedGroupBindIsRetriedByTheNextDraw)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto compiled = compileThreeGroupPipeline();
    ASSERT_NE (compiled, nullptr);

    EncodedCalls calls;
    serveCountedDraws (calls);

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (compiled);
    for (int group = 0; group < 3; ++group)
        pass.setUniformBuffer (group, 0, uniforms, sizeof (uniforms));

    calls.bindGroupsToFail = 1;
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.bindGroups, 3);

    // Only the group whose bind failed is bound again
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.bindGroups, 4);

    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.bindGroups, 4);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, AReopenedBackendPassIsEncodedAfresh)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto compiled = compileThreeGroupPipeline();
    ASSERT_NE (compiled, nullptr);

    EncodedCalls calls;
    serveCountedDraws (calls);

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    auto frame = makeValidFrame();
    auto passA = target->beginRenderPass (frame);
    auto passB = target->beginRenderPass (frame);
    ASSERT_TRUE (passA.isValid());
    ASSERT_TRUE (passB.isValid());

    for (auto* pass : { &passA, &passB })
    {
        pass->setPipeline (compiled);
        for (int group = 0; group < 3; ++group)
            pass->setUniformBuffer (group, 0, uniforms, sizeof (uniforms));
    }

    EXPECT_TRUE (passA.draw (3));
    EXPECT_EQ (calls.passes, 1);

    // B takes the backend pass over, so A's next draw opens a new one and encodes everything in it
    EXPECT_TRUE (passB.draw (3));
    EXPECT_TRUE (passA.draw (3));
    EXPECT_EQ (calls.passes, 3);
    EXPECT_EQ (calls.pipelines, 3);
    EXPECT_EQ (calls.bindGroups, 9);
    EXPECT_EQ (calls.viewports, 3);

    passA.finish();
    passB.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, OpenGLDrawsEncodeTheirWholeState)
{
    // OpenGL binds to global units, which uploads between draws can change behind the pass
    class OpenGLDevice : public OreAndTargetGpuDevice
    {
    public:
        using OreAndTargetGpuDevice::OreAndTargetGpuDevice;

        GpuPlatform getPlatform() const noexcept override { return GpuPlatform::OpenGL; }
    };

    ctx = new OpenGLDevice (mockOreCtx.get(), MockOffscreenTarget::withGpuTexture (256, 128));
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto compiled = compileThreeGroupPipeline();
    ASSERT_NE (compiled, nullptr);

    EncodedCalls calls;
    serveCountedDraws (calls);

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (compiled);
    for (int group = 0; group < 3; ++group)
        pass.setUniformBuffer (group, 0, uniforms, sizeof (uniforms));

    EXPECT_TRUE (pass.draw (3));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (calls.pipelines, 2);
    EXPECT_EQ (calls.bindGroups, 6);
    EXPECT_EQ (calls.viewports, 2);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, PipelinesSharingAGroupLayoutReuseItsBindGroup)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    // Every compile creates its own default samplers, equal in all but identity
    ON_CALL (*mockOreCtx, makeSampler (_))
        .WillByDefault (Invoke ([] (const rive::ore::SamplerDesc&) -> rive::rcp<rive::ore::Sampler>
    {
        return rive::make_rcp<TestOreSampler>();
    }));

    using RK = rive::ore::ResourceKind;
    auto first = compilePipeline ({ makeBinding (0, 0, RK::UniformBuffer, 0),
                                    makeBinding (0, 1, RK::Sampler, 0),
                                    makeBinding (2, 0, RK::UniformBuffer, 1) });
    auto second = compilePipeline ({ makeBinding (0, 0, RK::UniformBuffer, 0),
                                     makeBinding (0, 1, RK::Sampler, 0),
                                     makeBinding (2, 0, RK::UniformBuffer, 1),
                                     makeBinding (2, 1, RK::SampledTexture, 0) });
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    int made = 0, bound = 0;
    serveDraws (made, bound);

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (first);
    pass.setUniformBuffer (0, 0, uniforms, sizeof (uniforms));
    pass.setUniformBuffer (2, 0, uniforms, sizeof (uniforms));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 2);

    // Only group 2, whose layout differs, is rebuilt
    pass.setPipeline (second);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 3);

    pass.setPipeline (first);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 4);
    EXPECT_EQ (bound, 6);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, EqualLayoutsWithDifferentDefaultSamplersRebuildTheGroup)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    using RK = rive::ore::ResourceKind;
    const std::vector<rive::ore::BindingMap::Entry> bindings { makeBinding (0, 0, RK::UniformBuffer, 0),
                                                               makeBinding (0, 1, RK::Sampler, 0) };

    // The first pipeline is left without a default sampler, the second gets one
    EXPECT_CALL (*mockOreCtx, makeSampler (_))
        .WillOnce (Return (rive::rcp<rive::ore::Sampler>()));
    auto first = compilePipeline (bindings);

    auto secondSampler = rive::make_rcp<TestOreSampler>();
    EXPECT_CALL (*mockOreCtx, makeSampler (_))
        .WillOnce (Return (secondSampler));
    auto second = compilePipeline (bindings);

    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);

    int made = 0, bound = 0;
    serveDraws (made, bound);

    const rive::ore::Sampler* boundSampler = nullptr;
    ON_CALL (*mockOreCtx, makeBindGroup (_))
        .WillByDefault (Invoke ([&made, &boundSampler] (const rive::ore::BindGroupDesc& desc) -> rive::rcp<rive::ore::BindGroup>
    {
        ++made;
        boundSampler = desc.samplerCount > 0 ? desc.samplers[0].sampler : nullptr;
        return rive::make_rcp<TestOreBindGroup>();
    }));

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (first);
    pass.setUniformBuffer (0, 0, uniforms, sizeof (uniforms));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 1);
    EXPECT_EQ (boundSampler, nullptr);

    pass.setPipeline (second);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 2);
    EXPECT_EQ (boundSampler, secondSampler.get());

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, SetStorageBufferOnUnsupportedDeviceIsIgnored)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "Binding a storage buffer on a device without fragment storage buffers triggers an assertion in assertion-enabled builds";
#endif

    // A storage buffer with a native handle, which this mocked device must still refuse to bind
    auto storageDevice = GpuDevice::create (GpuPlatform::Metal, {});
    if (storageDevice == nullptr || ! storageDevice->isFragmentStorageBufferAvailable())
        GTEST_SKIP() << "No device with fragment storage buffers to create the buffer on";

    const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    auto buffer = storageDevice->createBuffer (GpuBufferType::storage, color, sizeof (color));
    ASSERT_NE (buffer, nullptr);

    ASSERT_FALSE (ctx->isFragmentStorageBufferAvailable());

    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();
    bgl->addEntry (0, rive::ore::BindingKind::storageBufferRO);

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (makeShaderModuleWithBindingMap()))
        .WillOnce (Return (makeShaderModuleWithBindingMap()));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (rive::make_rcp<TestOrePipeline>()));

    auto compileResult = GpuPipeline::compile (ctx, makeShaderSource ("// VS"), makeShaderSource ("// FS"));
    ASSERT_TRUE (compileResult.wasOk());

    ON_CALL (*mockOreCtx, wrapRiveTexture (_, _, _))
        .WillByDefault (Invoke ([] (rive::gpu::Texture*, uint32_t, uint32_t)
    {
        return rive::make_rcp<TestOreTextureView>();
    }));
    ON_CALL (*mockOreCtx, beginRenderPass (_, _))
        .WillByDefault (Invoke ([] (const rive::ore::RenderPassDesc&, std::string*)
    {
        return std::unique_ptr<rive::ore::RenderPass> (new NiceMock<MockOreRenderPass>());
    }));
    ON_CALL (*mockOreCtx, makeBindGroup (_))
        .WillByDefault (Invoke ([] (const rive::ore::BindGroupDesc& desc)
    {
        EXPECT_EQ (0u, desc.storageBufferCount);
        return rive::make_rcp<TestOreBindGroup>();
    }));

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (compileResult.getValue());
    pass.setStorageBuffer (0, 0, buffer);
    EXPECT_TRUE (pass.draw (3));
    pass.finish();
    frame.submit();
    frame.waitForGPU();

    EXPECT_EQ (buffer->getReferenceCount(), 1);
}

TEST_F (GpuRenderPassMockTests, StorageBufferMeetingAWritableBlockSkipsTheDraw)
{
#if YUP_ASSERTIONS_ENABLED_OR_LOGGED
    GTEST_SKIP() << "A storage buffer meeting a writable block triggers an assertion in assertion-enabled builds";
#endif

    auto storageDevice = GpuDevice::create (GpuPlatform::Metal, {});
    if (storageDevice == nullptr || ! storageDevice->isFragmentStorageBufferAvailable())
        GTEST_SKIP() << "No device with fragment storage buffers to create the buffer on";

    const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    auto buffer = storageDevice->createBuffer (GpuBufferType::storage, color, sizeof (color));
    ASSERT_NE (buffer, nullptr);

    ctx = new StorageBufferGpuDevice (mockOreCtx.get(), MockOffscreenTarget::withGpuTexture (256, 128));

    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    // Render passes only bind read-only storage buffers
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();
    bgl->addEntry (0, rive::ore::BindingKind::storageBufferRW);

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (makeShaderModuleWithBindingMap()))
        .WillOnce (Return (makeShaderModuleWithBindingMap()));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (rive::make_rcp<TestOrePipeline>()));

    auto compileResult = GpuPipeline::compile (ctx, makeShaderSource ("// VS"), makeShaderSource ("// FS"));
    ASSERT_TRUE (compileResult.wasOk());

    int made = 0, bound = 0, draws = 0;
    serveDraws (made, bound);
    ON_CALL (*mockOreCtx, beginRenderPass (_, _))
        .WillByDefault (Invoke ([&draws] (const rive::ore::RenderPassDesc&, std::string*)
    {
        auto* renderPass = new NiceMock<MockOreRenderPass>();
        ON_CALL (*renderPass, draw (_, _, _, _))
            .WillByDefault (Invoke ([&draws] (uint32_t, uint32_t, uint32_t, uint32_t)
        {
            ++draws;
        }));
        return std::unique_ptr<rive::ore::RenderPass> (renderPass);
    }));

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (compileResult.getValue());
    pass.setStorageBuffer (0, 0, buffer);
    EXPECT_FALSE (pass.draw (3));
    EXPECT_EQ (draws, 0);
    EXPECT_TRUE (pass.isValid());

    // The pass keeps drawing once the offending binding is cleared
    pass.setStorageBuffer (0, 0, nullptr);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (draws, 1);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, GroupMissingADeclaredResourceIsRebuiltByTheNextDraw)
{
    auto target = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (target, nullptr);

    auto compiled = compileThreeGroupPipeline();
    ASSERT_NE (compiled, nullptr);

    int made = 0, bound = 0;
    serveDraws (made, bound);

    // The first uniform buffer cannot be allocated, so its group is built without it
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (ReturnNull())
        .WillRepeatedly (Invoke ([] (const rive::ore::BufferDesc& desc) -> rive::rcp<rive::ore::Buffer>
    {
        return rive::make_rcp<NiceMock<MockOreBuffer>> (desc.size);
    }));

    const float uniforms[4] = { 1.0f, 2.0f, 3.0f, 4.0f };

    auto frame = makeValidFrame();
    auto pass = target->beginRenderPass (frame);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (compiled);
    for (int group = 0; group < 3; ++group)
        pass.setUniformBuffer (group, 0, uniforms, sizeof (uniforms));
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 3);

    // Nothing changed, but the incomplete group must not be reused
    EXPECT_TRUE (pass.draw (3));
    EXPECT_EQ (made, 4);

    pass.finish();
    frame.submit();
}

TEST_F (GpuRenderPassMockTests, SetPipelineOnValidPassStoresPipeline)
{
    auto canvas = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (canvas, nullptr);

    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    auto compileResult = GpuPipeline::compile (ctx, makeShaderSource ("// VS"), makeShaderSource ("// FS"));
    ASSERT_TRUE (compileResult.wasOk());

    auto valid = makeValidFrame();
    auto pass = canvas->beginRenderPass (valid);
    ASSERT_TRUE (pass.isValid());

    // Set pipeline twice — second call replaces the first.
    EXPECT_NO_THROW ({
        pass.setPipeline (*compileResult.getValue());
        pass.setPipeline (*compileResult.getValue());
    });

    pass.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, SetTextureOnValidPassStoresAndReplacesBinding)
{
    auto canvas = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (canvas, nullptr);

    auto valid = makeValidFrame();
    auto pass = canvas->beginRenderPass (valid);
    ASSERT_TRUE (pass.isValid());

    // Setting with null texture should not crash.
    EXPECT_NO_THROW (pass.setTexture (0, 0, nullptr));

    // Setting same group/binding again replaces.
    EXPECT_NO_THROW (pass.setTexture (0, 0, nullptr));

    pass.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, SetUniformBufferOnValidPassStoresAndReplacesBinding)
{
    auto canvas = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (canvas, nullptr);

    auto valid = makeValidFrame();
    auto pass = canvas->beginRenderPass (valid);
    ASSERT_TRUE (pass.isValid());

    float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    EXPECT_NO_THROW (pass.setUniformBuffer (0, 0, data, sizeof (data)));

    // Replacing same group/binding updates the data.
    float newData[] = { 5.0f };
    EXPECT_NO_THROW (pass.setUniformBuffer (0, 0, newData, sizeof (newData)));

    pass.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, SetVertexBufferOnValidPassStoresAndReplacesSlot)
{
    auto canvas = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (canvas, nullptr);

    auto valid = makeValidFrame();
    auto pass = canvas->beginRenderPass (valid);
    ASSERT_TRUE (pass.isValid());

    EXPECT_NO_THROW (pass.setVertexBuffer (0, nullptr));

    // Replace slot 0.
    EXPECT_NO_THROW (pass.setVertexBuffer (0, nullptr));

    pass.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, SetIndexBufferOnValidPassStoresFormat)
{
    auto canvas = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (canvas, nullptr);

    auto valid = makeValidFrame();
    auto pass = canvas->beginRenderPass (valid);
    ASSERT_TRUE (pass.isValid());

    EXPECT_NO_THROW (pass.setIndexBuffer (GpuIndexFormat::uint16, nullptr));
    EXPECT_NO_THROW (pass.setIndexBuffer (GpuIndexFormat::uint32, nullptr));

    pass.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, DrawWithoutPipelineReturnsFalse)
{
    auto canvas = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (canvas, nullptr);

    auto valid = makeValidFrame();
    auto pass = canvas->beginRenderPass (valid);
    ASSERT_TRUE (pass.isValid());

    // No pipeline set — draw should fail gracefully.
    EXPECT_FALSE (pass.draw (4));
    EXPECT_FALSE (pass.drawIndexed (6));

    pass.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, FinishOnValidPassReturnsTrueAndIsIdempotent)
{
    auto canvas = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (canvas, nullptr);

    auto valid = makeValidFrame();
    auto pass = canvas->beginRenderPass (valid);
    ASSERT_TRUE (pass.isValid());

    EXPECT_TRUE (pass.finish());
    EXPECT_FALSE (pass.finish()); // Idempotent.
}

TEST_F (GpuRenderPassMockTests, MoveConstructionFromValidPassClearsSource)
{
    auto canvas = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (canvas, nullptr);

    auto valid = makeValidFrame();
    auto src = canvas->beginRenderPass (valid);
    ASSERT_TRUE (src.isValid());

    GpuRenderPass dst (std::move (src));
    EXPECT_TRUE (dst.isValid());
    EXPECT_FALSE (src.isValid()); // After move, src is empty.

    dst.finish();
    valid.submit();
}

TEST_F (GpuRenderPassMockTests, MoveAssignmentFromValidPassClearsSource)
{
    auto canvas = GpuTarget::create (ctx, 256, 128);
    ASSERT_NE (canvas, nullptr);

    auto valid = makeValidFrame();
    auto src = canvas->beginRenderPass (valid);
    ASSERT_TRUE (src.isValid());

    auto invalidSrc = makeInvalidFrame();
    auto dst = canvas->beginRenderPass (invalidSrc);
    EXPECT_FALSE (dst.isValid());

    dst = std::move (src);
    EXPECT_TRUE (dst.isValid());
    EXPECT_FALSE (src.isValid());

    dst.finish();
    valid.submit();
}

#if YUP_ENABLE_SHADER_TRANSPILER

// ==============================================================================
// GpuPipeline::compileFromGlsl — only when shader transpiler is available
// ==============================================================================

TEST_F (GpuPipelineMockTests, CompileFromGlslSucceeds)
{
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    const String vertexGlsl = "\n"
                              "#version 450\n"
                              "layout(set = 0, binding = 0) uniform Uniforms { vec4 color; } ubo;\n"
                              "void main() { gl_Position = ubo.color; }\n";

    const String fragmentGlsl = "\n"
                                "#version 450\n"
                                "layout(location = 0) out vec4 fragColor;\n"
                                "void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }\n";

    auto result = GpuPipeline::compileFromGlsl (ctx, vertexGlsl, fragmentGlsl);
    EXPECT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
}

TEST_F (GpuPipelineMockTests, CompileFromGlslWithOptions)
{
    auto vsModule = makeShaderModuleWithBindingMap();
    auto fsModule = makeShaderModuleWithBindingMap();
    auto pipeline = rive::make_rcp<TestOrePipeline>();
    auto bgl = rive::make_rcp<TestOreBindGroupLayout>();

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (vsModule))
        .WillOnce (Return (fsModule));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (bgl));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (pipeline));

    GpuPipelineOptions options;
    options.cullMode = GpuCullMode::back;

    const String vs = "#version 450\n"
                      "layout(set = 0, binding = 0) uniform UB { vec4 pos; } u;\n"
                      "void main() { gl_Position = u.pos; }\n";

    const String fs = "#version 450\n"
                      "layout(location = 0) out vec4 c;\n"
                      "void main() { c = vec4(1); }\n";

    auto result = GpuPipeline::compileFromGlsl (ctx, vs, fs, options);
    EXPECT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
}

#endif // YUP_ENABLE_SHADER_TRANSPILER
