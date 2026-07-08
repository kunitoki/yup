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

using namespace yup;
using ::testing::_;
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
    src.code = code;
    src.codeSize = (uint32_t) strlen (code);
    src.bindingMap = sBlob.data();
    src.bindingMapSize = (uint32_t) sBlob.size();

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
        ctx = std::make_unique<OreInjectedGraphicsContext> (mockOreCtx.get());
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    std::unique_ptr<OreInjectedGraphicsContext> ctx;
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

    auto result = GpuPipeline::compile (*ctx, vs, fs);
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
}

TEST_F (GpuPipelineMockTests, CompileFailsWhenVertexModuleIsNull)
{
    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (ReturnNull());

    auto vs = makeShaderSource ("// VS");
    auto fs = makeShaderSource ("// FS");

    auto result = GpuPipeline::compile (*ctx, vs, fs);
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

    auto result = GpuPipeline::compile (*ctx, vs, fs);
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

    auto result = GpuPipeline::compile (*ctx, vs, fs);
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
    vs.bindingMapSize = makeShaderSource().bindingMapSize;

    auto fs = makeShaderSource ("// FS");

    auto result = GpuPipeline::compile (*ctx, vs, fs);
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
    options.colorTargets[0] = colorTarget;
    options.colorTargetCount = 1;
    options.depthStencil.enabled = true;
    options.depthStencil.format = GpuTextureFormat::depth32float;
    options.depthStencil.depthCompare = GpuCompareFunction::less;
    options.depthStencil.depthWriteEnabled = true;
    options.sampleCount = 4;

    auto result = GpuPipeline::compile (*ctx, vs, fs, options);
    ASSERT_TRUE (result.wasOk());
    ASSERT_NE (result.getValue(), nullptr);
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

    GpuVertexAttribute attr { GpuVertexFormat::float3, 0, 0 };
    GpuVertexBufferLayout layout { 12, GpuVertexStepMode::vertex, &attr, 1 };

    GpuPipelineOptions options;
    options.vertexBuffers = &layout;
    options.vertexBufferCount = 1;

    auto result = GpuPipeline::compile (*ctx, vs, fs, options);
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
    auto result = GpuPipeline::compile (*ctx, vs, fs, options);
    ASSERT_TRUE (result.wasOk());
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
        ctx = std::make_unique<OreInjectedGraphicsContext> (mockOreCtx.get());
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    std::unique_ptr<OreInjectedGraphicsContext> ctx;
};

TEST_F (GpuBufferMockTests, CreateSucceedsWithValidData)
{
    auto oreBuf = rive::make_rcp<MockOreBuffer>();

    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (Return (oreBuf));

    const float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buf = GpuBuffer::create (*ctx, GpuBufferType::vertex, data, sizeof (data));
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
    auto buf = GpuBuffer::create (*ctx, GpuBufferType::index, data, sizeof (data));
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
    auto buf = GpuBuffer::create (*ctx, GpuBufferType::uniform, data, sizeof (data));
    ASSERT_NE (buf, nullptr);
    EXPECT_EQ (buf->getType(), GpuBufferType::uniform);
    EXPECT_TRUE (buf->isValid());
}

TEST_F (GpuBufferMockTests, CreateReturnsNullWhenMakeBufferFails)
{
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .WillOnce (ReturnNull());

    const float data[] = { 1.0f };
    auto buf = GpuBuffer::create (*ctx, GpuBufferType::vertex, data, sizeof (data));
    EXPECT_EQ (buf, nullptr);
}

// ==============================================================================
// GpuFrame mock-based tests
// ==============================================================================

class GpuFrameMockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        ctx = std::make_unique<OreInjectedGraphicsContext> (mockOreCtx.get());
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    std::unique_ptr<OreInjectedGraphicsContext> ctx;
};

TEST_F (GpuFrameMockTests, BeginCallsOreBeginFrame)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));

    auto frame = GpuFrame::begin (*ctx);
    EXPECT_TRUE (frame.isValid());
}

TEST_F (GpuFrameMockTests, SubmitCallsOreEndFrame)
{
    // beginFrame + endFrame
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());

    auto frame = GpuFrame::begin (*ctx);
    ASSERT_TRUE (frame.isValid());
    EXPECT_TRUE (frame.submit());
}

TEST_F (GpuFrameMockTests, SubmitIsIdempotent)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());

    auto frame = GpuFrame::begin (*ctx);
    ASSERT_TRUE (frame.isValid());
    EXPECT_TRUE (frame.submit());
    EXPECT_FALSE (frame.submit());
}

TEST_F (GpuFrameMockTests, WaitForGpuCallsOreWaitForGPU)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());
    EXPECT_CALL (*mockOreCtx, waitForGPU());

    auto frame = GpuFrame::begin (*ctx);
    ASSERT_TRUE (frame.isValid());
    frame.submit();
    frame.waitForGPU();
}

TEST_F (GpuFrameMockTests, DestructorSubmitsIfNotSubmitted)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());

    {
        auto frame = GpuFrame::begin (*ctx);
        ASSERT_TRUE (frame.isValid());
        // Not explicitly submitted — destructor does it.
    }
}

TEST_F (GpuFrameMockTests, MoveAssignmentSubmitsExisting)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_)).Times (2);
    EXPECT_CALL (*mockOreCtx, endFrame()).Times (2);

    auto src = GpuFrame::begin (*ctx);
    auto dst = GpuFrame::begin (*ctx);

    dst = std::move (src);
}
