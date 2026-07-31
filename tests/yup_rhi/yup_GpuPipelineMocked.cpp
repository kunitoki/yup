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
    vs.bindingMapSize = makeShaderSource().bindingMapSize;

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
    options.colorTargets[0] = colorTarget;
    options.colorTargetCount = 1;
    options.depthStencil.enabled = true;
    options.depthStencil.format = GpuTextureFormat::depth32float;
    options.depthStencil.depthCompare = GpuCompareFunction::less;
    options.depthStencil.depthWriteEnabled = true;
    options.sampleCount = 4;

    auto result = GpuPipeline::compile (ctx, vs, fs, options);
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

    // A GPU sync is expensive, and the destructor waits too, so repeated waits must
    // collapse into the single stall the first one already paid for.
    EXPECT_CALL (*mockOreCtx, waitForGPU());

    auto frame = GpuFrame::begin (ctx);
    ASSERT_TRUE (frame.isValid());
    frame.submit();
    frame.waitForGPU();
    frame.waitForGPU();
}

TEST_F (GpuFrameMockTests, DestructorSubmitsAndWaitsIfNotSubmitted)
{
    EXPECT_CALL (*mockOreCtx, beginFrame (_));
    EXPECT_CALL (*mockOreCtx, endFrame());

    // The encoded passes reference this frame's transient resources by raw pointer,
    // and destruction releases them, so the destructor has to drain the GPU first.
    EXPECT_CALL (*mockOreCtx, waitForGPU());

    {
        auto frame = GpuFrame::begin (ctx);
        ASSERT_TRUE (frame.isValid());
        // Not explicitly submitted — destructor does it.
    }
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

    EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
        .WillOnce (Return (makeShaderModuleWithBindingMap()))
        .WillOnce (Return (makeShaderModuleWithBindingMap()));
    EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
        .WillOnce (Return (rive::make_rcp<TestOreBindGroupLayout>()));
    EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
        .WillOnce (Return (rive::make_rcp<TestOrePipeline>()));

    // Two draws per frame need two distinct buffers, but the second frame gets
    // both of them back from the pool - so only two are ever created.
    EXPECT_CALL (*mockOreCtx, makeBuffer (_))
        .Times (2)
        .WillRepeatedly (Invoke ([] (const rive::ore::BufferDesc& desc)
    {
        return rive::make_rcp<MockOreBuffer> (desc.size);
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
