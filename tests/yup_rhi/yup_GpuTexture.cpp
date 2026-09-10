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

using namespace yup;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

namespace
{

/** A 2D rgba8 render-target descriptor, the starting point for most cases here. */
GpuTextureDesc makeRenderTargetDesc (uint32_t width = 64, uint32_t height = 64)
{
    GpuTextureDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = GpuTextureFormat::rgba8unorm;
    desc.renderTarget = true;
    return desc;
}

/** A six-face cube descriptor with a mip chain. */
GpuTextureDesc makeCubeDesc (uint32_t size = 64, uint32_t mips = 1)
{
    GpuTextureDesc desc;
    desc.width = desc.height = size;
    desc.depthOrArrayLayers = 6;
    desc.type = GpuTextureType::cube;
    desc.format = GpuTextureFormat::rgba16float;
    desc.mipLevels = mips;
    desc.renderTarget = true;
    return desc;
}

} // namespace

// ==============================================================================
// GpuTexture — creation and descriptor translation
// ==============================================================================

class GpuTextureMockTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockOreCtx = std::make_unique<NiceMock<MockOreContext>>();
        ctx = new OreInjectedGpuDevice (mockOreCtx.get());
    }

    /** Makes makeTexture() succeed, recording the descriptor it was handed. */
    rive::rcp<NiceMock<MockOreTexture>> expectTextureCreation (rive::ore::TextureDesc* captured)
    {
        auto oreTexture = rive::make_rcp<NiceMock<MockOreTexture>>();

        EXPECT_CALL (*mockOreCtx, makeTexture (_))
            .WillOnce (DoAll (SaveArg<0> (captured), Return (oreTexture)));

        return oreTexture;
    }

    /** Creates a texture through a succeeding, unobserved makeTexture(). */
    GpuTexture::Ptr createTexture (const GpuTextureDesc& desc)
    {
        ON_CALL (*mockOreCtx, makeTexture (_))
            .WillByDefault (Invoke ([] (const rive::ore::TextureDesc&)
        {
            return rive::rcp<rive::ore::Texture> (rive::make_rcp<NiceMock<MockOreTexture>>());
        }));

        return GpuTexture::create (ctx, desc);
    }

    std::unique_ptr<NiceMock<MockOreContext>> mockOreCtx;
    GpuDevice::Ptr ctx;
};

TEST_F (GpuTextureMockTests, CreateTranslatesEveryDescriptorFieldToOre)
{
    rive::ore::TextureDesc captured {};
    auto oreTexture = expectTextureCreation (&captured);

    auto texture = GpuTexture::create (ctx, makeCubeDesc (128, 4));
    ASSERT_NE (texture, nullptr);

    EXPECT_EQ (captured.width, 128u);
    EXPECT_EQ (captured.height, 128u);
    EXPECT_EQ (captured.depthOrArrayLayers, 6u);
    EXPECT_EQ (captured.type, rive::ore::TextureType::cube);
    EXPECT_EQ (captured.format, rive::ore::TextureFormat::rgba16float);
    EXPECT_EQ (captured.numMipmaps, 4u);
    EXPECT_EQ (captured.sampleCount, 1u);
    EXPECT_TRUE (captured.renderTarget);
}

TEST_F (GpuTextureMockTests, CreateExposesDescriptorThroughAccessors)
{
    auto texture = createTexture (makeCubeDesc (128, 4));
    ASSERT_NE (texture, nullptr);

    EXPECT_EQ (texture->getWidth(), 128);
    EXPECT_EQ (texture->getHeight(), 128);
    EXPECT_EQ (texture->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (texture->getType(), GpuTextureType::cube);
    EXPECT_EQ (texture->getMipLevels(), 4u);
    EXPECT_EQ (texture->getDepthOrArrayLayers(), 6u);
    EXPECT_EQ (texture->getSampleCount(), 1u);
    EXPECT_TRUE (texture->isValid());
    EXPECT_TRUE (texture->isRenderTarget());
}

TEST_F (GpuTextureMockTests, CreateWithZeroExtentReturnsNull)
{
    EXPECT_CALL (*mockOreCtx, makeTexture (_)).Times (0);

    auto desc = makeRenderTargetDesc();
    desc.width = 0;
    EXPECT_EQ (GpuTexture::create (ctx, desc), nullptr);

    desc = makeRenderTargetDesc();
    desc.height = 0;
    EXPECT_EQ (GpuTexture::create (ctx, desc), nullptr);
}

TEST_F (GpuTextureMockTests, CreateWithZeroMipLevelsReturnsNull)
{
    EXPECT_CALL (*mockOreCtx, makeTexture (_)).Times (0);

    auto desc = makeRenderTargetDesc();
    desc.mipLevels = 0;
    EXPECT_EQ (GpuTexture::create (ctx, desc), nullptr);
}

TEST_F (GpuTextureMockTests, CreateCubeWithWrongLayerCountReturnsNull)
{
    EXPECT_CALL (*mockOreCtx, makeTexture (_)).Times (0);

    auto desc = makeCubeDesc();
    desc.depthOrArrayLayers = 1;
    EXPECT_EQ (GpuTexture::create (ctx, desc), nullptr);
}

TEST_F (GpuTextureMockTests, CreateWithoutGpuContextReturnsNull)
{
    auto headless = GpuDevice::create (GpuPlatform::Headless, {});
    ASSERT_NE (headless, nullptr);

    EXPECT_EQ (GpuTexture::create (headless, makeRenderTargetDesc()), nullptr);
}

TEST_F (GpuTextureMockTests, CreateReturnsNullWhenBackendCannotAllocate)
{
    EXPECT_CALL (*mockOreCtx, makeTexture (_))
        .WillOnce (Return (rive::rcp<rive::ore::Texture> (nullptr)));

    EXPECT_EQ (GpuTexture::create (ctx, makeRenderTargetDesc()), nullptr);
}

// ---------------------------------------------------------------------------
// Format coverage: the newly exposed formats must reach their ore counterparts
// rather than silently collapsing onto the rgba8unorm default.

TEST_F (GpuTextureMockTests, CreateMapsTheExtendedFormatSetOntoOre)
{
    const std::pair<GpuTextureFormat, rive::ore::TextureFormat> cases[] = {
        { GpuTextureFormat::r8unorm, rive::ore::TextureFormat::r8unorm },
        { GpuTextureFormat::rg8unorm, rive::ore::TextureFormat::rg8unorm },
        { GpuTextureFormat::rgba8snorm, rive::ore::TextureFormat::rgba8snorm },
        { GpuTextureFormat::r16float, rive::ore::TextureFormat::r16float },
        { GpuTextureFormat::rg16float, rive::ore::TextureFormat::rg16float },
        { GpuTextureFormat::rgba16float, rive::ore::TextureFormat::rgba16float },
        { GpuTextureFormat::r32float, rive::ore::TextureFormat::r32float },
        { GpuTextureFormat::rg32float, rive::ore::TextureFormat::rg32float },
        { GpuTextureFormat::rgba32float, rive::ore::TextureFormat::rgba32float },
        { GpuTextureFormat::rgb10a2unorm, rive::ore::TextureFormat::rgb10a2unorm },
        { GpuTextureFormat::r11g11b10float, rive::ore::TextureFormat::r11g11b10float },
        { GpuTextureFormat::depth16unorm, rive::ore::TextureFormat::depth16unorm },
        { GpuTextureFormat::depth32float, rive::ore::TextureFormat::depth32float },
        { GpuTextureFormat::depth32floatStencil8, rive::ore::TextureFormat::depth32floatStencil8 },
        { GpuTextureFormat::bc7unorm, rive::ore::TextureFormat::bc7unorm },
        { GpuTextureFormat::etc2rgba8, rive::ore::TextureFormat::etc2rgba8 },
        { GpuTextureFormat::astc8x8, rive::ore::TextureFormat::astc8x8 },
    };

    for (const auto& [yupFormat, oreFormat] : cases)
    {
        rive::ore::TextureDesc captured {};
        auto oreTexture = expectTextureCreation (&captured);

        auto desc = makeRenderTargetDesc();
        desc.format = yupFormat;

        ASSERT_NE (GpuTexture::create (ctx, desc), nullptr);
        EXPECT_EQ (captured.format, oreFormat) << "for GpuTextureFormat " << (int) yupFormat;
    }
}

TEST_F (GpuTextureMockTests, DepthStencilFormatsAreRecognised)
{
    EXPECT_TRUE (isDepthStencilFormat (GpuTextureFormat::depth16unorm));
    EXPECT_TRUE (isDepthStencilFormat (GpuTextureFormat::depth24plusStencil8));
    EXPECT_TRUE (isDepthStencilFormat (GpuTextureFormat::depth32float));
    EXPECT_TRUE (isDepthStencilFormat (GpuTextureFormat::depth32floatStencil8));

    EXPECT_FALSE (isDepthStencilFormat (GpuTextureFormat::rgba8unorm));
    EXPECT_FALSE (isDepthStencilFormat (GpuTextureFormat::rgba16float));
}

// ==============================================================================
// GpuTexture::upload
// ==============================================================================

TEST_F (GpuTextureMockTests, UploadDefaultsCoverTheWholeMipLevel)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);

    rive::ore::TextureDataDesc captured {};
    EXPECT_CALL (*oreTexture, upload (_)).WillOnce (SaveArg<0> (&captured));

    auto desc = makeRenderTargetDesc (16, 8);
    auto texture = GpuTexture::create (ctx, desc);
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> pixels (16 * 8 * 4, 0);

    GpuTextureDataDesc data;
    data.data = pixels.data();
    EXPECT_TRUE (texture->upload (data));

    EXPECT_EQ (captured.width, 16u);
    EXPECT_EQ (captured.height, 8u);
    EXPECT_EQ (captured.bytesPerRow, 16u * 4u);
    EXPECT_EQ (captured.rowsPerImage, 8u);
    EXPECT_EQ (captured.mipLevel, 0u);
    EXPECT_EQ (captured.layer, 0u);
}

TEST_F (GpuTextureMockTests, UploadDefaultsToTheSizeOfTheRequestedMipLevel)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);

    rive::ore::TextureDataDesc captured {};
    EXPECT_CALL (*oreTexture, upload (_)).WillOnce (SaveArg<0> (&captured));

    auto desc = makeRenderTargetDesc (16, 16);
    desc.mipLevels = 3;
    auto texture = GpuTexture::create (ctx, desc);
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> pixels (4 * 4 * 4, 0);

    GpuTextureDataDesc data;
    data.data = pixels.data();
    data.mipLevel = 2;
    EXPECT_TRUE (texture->upload (data));

    EXPECT_EQ (captured.width, 4u);
    EXPECT_EQ (captured.height, 4u);
    EXPECT_EQ (captured.mipLevel, 2u);
}

TEST_F (GpuTextureMockTests, UploadTargetsTheRequestedCubeFace)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);

    rive::ore::TextureDataDesc captured {};
    EXPECT_CALL (*oreTexture, upload (_)).WillOnce (SaveArg<0> (&captured));

    auto texture = GpuTexture::create (ctx, makeCubeDesc (8));
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> pixels (8 * 8 * 8, 0);

    GpuTextureDataDesc data;
    data.data = pixels.data();
    data.layer = 4;
    EXPECT_TRUE (texture->upload (data));

    EXPECT_EQ (captured.layer, 4u);
}

TEST_F (GpuTextureMockTests, UploadRejectsOutOfRangeMipLevelAndLayer)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);
    EXPECT_CALL (*oreTexture, upload (_)).Times (0);

    auto texture = GpuTexture::create (ctx, makeRenderTargetDesc (16, 16));
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> pixels (16 * 16 * 4, 0);

    GpuTextureDataDesc data;
    data.data = pixels.data();

    data.mipLevel = 1;
    EXPECT_FALSE (texture->upload (data));

    data.mipLevel = 0;
    data.layer = 1;
    EXPECT_FALSE (texture->upload (data));
}

TEST_F (GpuTextureMockTests, UploadRejectsRegionsRunningPastTheEdge)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);
    EXPECT_CALL (*oreTexture, upload (_)).Times (0);

    auto texture = GpuTexture::create (ctx, makeRenderTargetDesc (16, 16));
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> pixels (16 * 16 * 4, 0);

    GpuTextureDataDesc data;
    data.data = pixels.data();
    data.x = 8;
    data.width = 16;
    data.height = 16;
    EXPECT_FALSE (texture->upload (data));
}

TEST_F (GpuTextureMockTests, UploadRejectsNullData)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);
    EXPECT_CALL (*oreTexture, upload (_)).Times (0);

    auto texture = GpuTexture::create (ctx, makeRenderTargetDesc());
    ASSERT_NE (texture, nullptr);

    EXPECT_FALSE (texture->upload ({}));
}

// No bytes-per-texel exists for a block-compressed format, so an implicit row
// stride cannot be derived and must be refused rather than guessed.

TEST_F (GpuTextureMockTests, UploadOfCompressedDataWithoutARowStrideIsRefused)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);
    EXPECT_CALL (*oreTexture, upload (_)).Times (0);

    auto desc = makeRenderTargetDesc (16, 16);
    desc.format = GpuTextureFormat::bc7unorm;
    desc.renderTarget = false;

    auto texture = GpuTexture::create (ctx, desc);
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> blocks (256, 0);

    GpuTextureDataDesc data;
    data.data = blocks.data();
    EXPECT_FALSE (texture->upload (data));
}

TEST_F (GpuTextureMockTests, UploadOfCompressedDataWithARowStrideSucceeds)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);

    rive::ore::TextureDataDesc captured {};
    EXPECT_CALL (*oreTexture, upload (_)).WillOnce (SaveArg<0> (&captured));

    auto desc = makeRenderTargetDesc (16, 16);
    desc.format = GpuTextureFormat::bc7unorm;
    desc.renderTarget = false;

    auto texture = GpuTexture::create (ctx, desc);
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> blocks (256, 0);

    GpuTextureDataDesc data;
    data.data = blocks.data();
    data.bytesPerRow = 64;
    EXPECT_TRUE (texture->upload (data));

    EXPECT_EQ (captured.bytesPerRow, 64u);
}

// ==============================================================================
// GpuSampler
// ==============================================================================

TEST_F (GpuTextureMockTests, SamplerTranslatesEveryDescriptorFieldToOre)
{
    rive::ore::SamplerDesc captured {};
    auto oreSampler = rive::make_rcp<TestOreSampler>();

    EXPECT_CALL (*mockOreCtx, makeSampler (_))
        .WillOnce (DoAll (SaveArg<0> (&captured), Return (oreSampler)));

    GpuSamplerDesc desc;
    desc.minFilter = GpuFilter::linear;
    desc.magFilter = GpuFilter::linear;
    desc.mipmapFilter = GpuFilter::linear;
    desc.wrapU = GpuWrapMode::repeat;
    desc.wrapV = GpuWrapMode::mirrorRepeat;
    desc.wrapW = GpuWrapMode::clampToEdge;
    desc.minLod = 1.0f;
    desc.maxLod = 4.0f;
    desc.maxAnisotropy = 8;

    auto sampler = GpuSampler::create (ctx, desc);
    ASSERT_NE (sampler, nullptr);
    EXPECT_TRUE (sampler->isValid());

    EXPECT_EQ (captured.minFilter, rive::ore::Filter::linear);
    EXPECT_EQ (captured.magFilter, rive::ore::Filter::linear);
    EXPECT_EQ (captured.mipmapFilter, rive::ore::Filter::linear);
    EXPECT_EQ (captured.wrapU, rive::ore::WrapMode::repeat);
    EXPECT_EQ (captured.wrapV, rive::ore::WrapMode::mirrorRepeat);
    EXPECT_EQ (captured.wrapW, rive::ore::WrapMode::clampToEdge);
    EXPECT_EQ (captured.compare, rive::ore::CompareFunction::none);
    EXPECT_FLOAT_EQ (captured.minLod, 1.0f);
    EXPECT_FLOAT_EQ (captured.maxLod, 4.0f);
    EXPECT_EQ (captured.maxAnisotropy, 8u);
}

TEST_F (GpuTextureMockTests, SamplerWithComparisonSetsTheCompareFunction)
{
    rive::ore::SamplerDesc captured {};
    auto oreSampler = rive::make_rcp<TestOreSampler>();

    EXPECT_CALL (*mockOreCtx, makeSampler (_))
        .WillOnce (DoAll (SaveArg<0> (&captured), Return (oreSampler)));

    GpuSamplerDesc desc;
    desc.compare = GpuCompareFunction::lessEqual;

    ASSERT_NE (GpuSampler::create (ctx, desc), nullptr);
    EXPECT_EQ (captured.compare, rive::ore::CompareFunction::lessEqual);
}

TEST_F (GpuTextureMockTests, SamplerWithoutGpuContextReturnsNull)
{
    auto headless = GpuDevice::create (GpuPlatform::Headless, {});
    ASSERT_NE (headless, nullptr);

    EXPECT_EQ (GpuSampler::create (headless, {}), nullptr);
}

// ==============================================================================
// GpuTarget::createFromTexture — render-to-mip and render-to-face
// ==============================================================================

TEST_F (GpuTextureMockTests, CreateFromTextureRejectsNonRenderTargets)
{
    auto desc = makeRenderTargetDesc();
    desc.renderTarget = false;

    auto texture = createTexture (desc);
    ASSERT_NE (texture, nullptr);

    EXPECT_EQ (GpuTarget::createFromTexture (ctx, texture), nullptr);
}

TEST_F (GpuTextureMockTests, CreateFromTextureRejectsOutOfRangeViews)
{
    auto texture = createTexture (makeCubeDesc (64, 3));
    ASSERT_NE (texture, nullptr);

    EXPECT_EQ (GpuTarget::createFromTexture (ctx, texture, { 3, 0 }), nullptr);
    EXPECT_EQ (GpuTarget::createFromTexture (ctx, texture, { 0, 6 }), nullptr);
    EXPECT_NE (GpuTarget::createFromTexture (ctx, texture, { 2, 5 }), nullptr);
}

TEST_F (GpuTextureMockTests, CreateFromTextureReportsTheSizeOfItsMipLevel)
{
    auto texture = createTexture (makeCubeDesc (64, 4));
    ASSERT_NE (texture, nullptr);

    auto base = GpuTarget::createFromTexture (ctx, texture, { 0, 0 });
    ASSERT_NE (base, nullptr);
    EXPECT_EQ (base->getWidth(), 64);
    EXPECT_EQ (base->getHeight(), 64);

    auto mip = GpuTarget::createFromTexture (ctx, texture, { 3, 0 });
    ASSERT_NE (mip, nullptr);
    EXPECT_EQ (mip->getWidth(), 8);
    EXPECT_EQ (mip->getHeight(), 8);
}

TEST_F (GpuTextureMockTests, CreateFromTextureExposesItsBackingTexture)
{
    auto texture = createTexture (makeRenderTargetDesc());
    ASSERT_NE (texture, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, texture);
    ASSERT_NE (target, nullptr);
    EXPECT_EQ (target->asTexture(), texture);
}

TEST_F (GpuTextureMockTests, ReadPixelsIsUnsupportedForDirectlyAllocatedTargets)
{
    auto texture = createTexture (makeRenderTargetDesc (8, 8));
    ASSERT_NE (texture, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, texture);
    ASSERT_NE (target, nullptr);

    std::vector<uint8_t> pixels (8 * 8 * 4, 0);
    EXPECT_FALSE (target->readPixels (pixels.data(), pixels.size()));
}

// ==============================================================================
// GpuRenderPass — attachment descriptors
//
// tests/yup_rhi runs against MockOreContext with no real GPU, and ore has no
// texture readback path, so these assert on the descriptors handed to the
// backend rather than on read-back pixels.
// ==============================================================================

#if 0

class GpuAttachmentMockTests : public GpuTextureMockTests
{
protected:
    /** Arguments of one encoded indexed draw. */
    struct IndexedDraw
    {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t baseVertex;
        uint32_t firstInstance;
    };

    /** Records every TextureViewDesc created, RenderPassDesc begun, and draw encoded. */
    void expectAttachmentCapture()
    {
        ON_CALL (*mockOreCtx, makeTextureView (_))
            .WillByDefault (Invoke ([this] (const rive::ore::TextureViewDesc& desc)
        {
            viewDescs.push_back (desc);
            return rive::rcp<rive::ore::TextureView> (rive::make_rcp<TestOreTextureView>());
        }));

        ON_CALL (*mockOreCtx, beginRenderPass (_, _))
            .WillByDefault (Invoke ([this] (const rive::ore::RenderPassDesc& desc, std::string*)
        {
            passDescs.push_back (desc);

            auto* renderPass = new NiceMock<MockOreRenderPass>();

            ON_CALL (*renderPass, drawIndexed (_, _, _, _, _))
                .WillByDefault (Invoke ([this] (uint32_t indexCount,
                                                uint32_t instanceCount,
                                                uint32_t firstIndex,
                                                int32_t baseVertex,
                                                uint32_t firstInstance)
            {
                indexedDraws.push_back ({ indexCount, instanceCount, firstIndex, baseVertex, firstInstance });
            }));

            return std::unique_ptr<rive::ore::RenderPass> (renderPass);
        }));
    }

    /** Compiles a GpuPipeline whose whole backend is mocked, so a pass can encode. */
    GpuPipeline::Ptr makeMockedPipeline()
    {
        auto makeModule = []
        {
            auto module = rive::make_rcp<TestOreShaderModule>();

            rive::ore::BindingMap::Entry entry {};
            entry.group = 0;
            entry.binding = 0;
            entry.kind = rive::ore::ResourceKind::UniformBuffer;
            entry.stageMask = rive::ore::BindingMap::kStageVertex | rive::ore::BindingMap::kStageFragment;
            entry.backendSlot[0] = 0;
            entry.backendSlot[1] = 0;

            module->m_bindingMap.push (entry);
            module->m_bindingMap.finalize();
            return module;
        };

        EXPECT_CALL (*mockOreCtx, makeShaderModule (_))
            .WillOnce (Return (makeModule()))
            .WillOnce (Return (makeModule()));
        EXPECT_CALL (*mockOreCtx, makeBindGroupLayout (_))
            .WillOnce (Return (rive::make_rcp<TestOreBindGroupLayout>()));
        EXPECT_CALL (*mockOreCtx, makePipeline (_, _))
            .WillOnce (Return (rive::make_rcp<TestOrePipeline>()));

        static rive::ore::BindingMap blobMap;
        static std::vector<uint8_t> blob;

        if (blob.empty())
        {
            rive::ore::BindingMap::Entry entry {};
            entry.group = 0;
            entry.binding = 0;
            entry.kind = rive::ore::ResourceKind::UniformBuffer;
            entry.stageMask = rive::ore::BindingMap::kStageVertex | rive::ore::BindingMap::kStageFragment;
            entry.backendSlot[0] = 0;
            entry.backendSlot[1] = 0;
            blobMap.push (entry);
            blobMap.finalize();
            blob = blobMap.toBlob();
        }

        auto makeSource = [] (const char* code)
        {
            GpuShaderSource source;
            source.language = GpuShaderLanguage::glsl;
            source.code = gpuShaderSourceBytes (code);
            source.bindingMap = blob;
            return source;
        };

        auto result = GpuPipeline::compile (ctx, makeSource ("// vs"), makeSource ("// fs"));
        return result.wasOk() ? result.getReference() : nullptr;
    }

    GpuFrame beginFrame()
    {
        return GpuFrame::begin (ctx);
    }

    GpuTextureDesc depthDesc() const
    {
        auto desc = makeRenderTargetDesc (32, 32);
        desc.format = GpuTextureFormat::depth24plusStencil8;
        return desc;
    }

    std::vector<rive::ore::TextureViewDesc> viewDescs;
    std::vector<rive::ore::RenderPassDesc> passDescs;
    std::vector<IndexedDraw> indexedDraws;
};

// Every draw of a pass encodes into one backend render pass - one command
// encoder - rather than opening and closing a fresh one per draw.

TEST_F (GpuAttachmentMockTests, AllDrawsOfAPassShareOneBackendRenderPass)
{
    expectAttachmentCapture();

    auto color = createTexture (makeRenderTargetDesc (32, 32));
    auto depth = createTexture (depthDesc());
    ASSERT_NE (color, nullptr);
    ASSERT_NE (depth, nullptr);

    auto pipeline = makeMockedPipeline();
    ASSERT_NE (pipeline, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, color);
    ASSERT_NE (target, nullptr);

    {
        auto frame = beginFrame();
        auto pass = target->beginRenderPass (frame, { true, GpuColor::black() });
        pass.setDepthStencilAttachment (depth);
        pass.setPipeline (pipeline);

        EXPECT_TRUE (pass.draw (3));
        EXPECT_TRUE (pass.draw (3));

        pass.finish();
        frame.submit();
    }

    // Two draws, one encoder. Both attachments are cleared exactly once, when it
    // opens, so the second draw cannot wipe what the first one wrote.
    ASSERT_EQ (passDescs.size(), 1u);

    EXPECT_EQ (passDescs[0].colorAttachments[0].loadOp, rive::ore::LoadOp::clear);
    EXPECT_EQ (passDescs[0].colorAttachments[0].storeOp, rive::ore::StoreOp::store);
    EXPECT_EQ (passDescs[0].depthStencil.depthLoadOp, rive::ore::LoadOp::clear);

    // One attachment view per attachment, not per draw.
    EXPECT_EQ (viewDescs.size(), 2u);
}

TEST_F (GpuAttachmentMockTests, AttachmentViewsAreCreatedOncePerTexture)
{
    expectAttachmentCapture();

    auto color = createTexture (makeRenderTargetDesc (32, 32));
    ASSERT_NE (color, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, color);
    ASSERT_NE (target, nullptr);

    for (int i = 0; i < 4; ++i)
    {
        auto frame = beginFrame();
        auto pass = target->beginRenderPass (frame, { true, GpuColor::black() });
        pass.finish();
        frame.submit();
    }

    // Four passes over four frames, but the attachment descriptor is identical
    // every time, so the backend view is built once and reused.
    ASSERT_EQ (passDescs.size(), 4u);
    EXPECT_EQ (viewDescs.size(), 1u);
}

TEST_F (GpuAttachmentMockTests, DistinctViewDescriptorsGetDistinctViews)
{
    expectAttachmentCapture();

    auto cube = createTexture (makeCubeDesc (32, 3));
    ASSERT_NE (cube, nullptr);

    // Same texture, different mip/layer selections - the cache must not collapse
    // these onto one view, or every face would render into the same place.
    for (uint32_t mip = 0; mip < 3; ++mip)
    {
        for (uint32_t face = 0; face < 6; ++face)
        {
            auto target = GpuTarget::createFromTexture (ctx, cube, { mip, face });
            ASSERT_NE (target, nullptr);

            auto frame = beginFrame();
            auto pass = target->beginRenderPass (frame, { true, GpuColor::black() });
            pass.finish();
            frame.submit();
        }
    }

    EXPECT_EQ (viewDescs.size(), 18u);
}

TEST_F (GpuAttachmentMockTests, InstancedDrawParametersReachTheBackend)
{
    expectAttachmentCapture();

    auto color = createTexture (makeRenderTargetDesc (32, 32));
    ASSERT_NE (color, nullptr);

    auto pipeline = makeMockedPipeline();
    ASSERT_NE (pipeline, nullptr);

    ON_CALL (*mockOreCtx, makeBuffer (_))
        .WillByDefault (Invoke ([] (const rive::ore::BufferDesc& desc)
    {
        return rive::rcp<rive::ore::Buffer> (rive::make_rcp<NiceMock<MockOreBuffer>> (desc.size, desc.usage));
    }));

    const std::vector<uint16_t> indices (6, 0);
    auto indexBuffer = ctx->createBuffer (GpuBufferType::index, indices.data(), indices.size() * sizeof (uint16_t));
    ASSERT_NE (indexBuffer, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, color);
    ASSERT_NE (target, nullptr);

    auto frame = beginFrame();
    auto pass = target->beginRenderPass (frame, { true, GpuColor::black() });
    pass.setPipeline (pipeline);
    pass.setIndexBuffer (GpuIndexFormat::uint16, indexBuffer);

    EXPECT_TRUE (pass.drawIndexed (6, 15, 1, 2, 3));

    pass.finish();
    frame.submit();

    ASSERT_EQ (indexedDraws.size(), 1u);
    EXPECT_EQ (indexedDraws[0].indexCount, 6u);
    EXPECT_EQ (indexedDraws[0].instanceCount, 15u);
    EXPECT_EQ (indexedDraws[0].firstIndex, 1u);
    EXPECT_EQ (indexedDraws[0].baseVertex, 2);
    EXPECT_EQ (indexedDraws[0].firstInstance, 3u);
}

TEST_F (GpuAttachmentMockTests, RenderingToACubeFaceNarrowsTheAttachmentViewToThatLayer)
{
    expectAttachmentCapture();

    auto cube = createTexture (makeCubeDesc (32, 1));
    ASSERT_NE (cube, nullptr);

    for (uint32_t face = 0; face < 6; ++face)
    {
        auto target = GpuTarget::createFromTexture (ctx, cube, { 0, face });
        ASSERT_NE (target, nullptr);

        auto frame = beginFrame();
        auto pass = target->beginRenderPass (frame, { true, GpuColor::black() });
        pass.finish();
        frame.submit();
    }

    ASSERT_EQ (viewDescs.size(), 6u);

    for (uint32_t face = 0; face < 6; ++face)
    {
        EXPECT_EQ (viewDescs[face].baseLayer, face);
        EXPECT_EQ (viewDescs[face].layerCount, 1u);
        EXPECT_EQ (viewDescs[face].baseMipLevel, 0u);
    }
}

TEST_F (GpuAttachmentMockTests, RenderingToAMipLevelNarrowsTheAttachmentViewToThatLevel)
{
    expectAttachmentCapture();

    auto texture = createTexture ([]
    {
        auto desc = makeRenderTargetDesc (64, 64);
        desc.mipLevels = 4;
        return desc;
    }());

    ASSERT_NE (texture, nullptr);

    for (uint32_t mip = 0; mip < 4; ++mip)
    {
        auto target = GpuTarget::createFromTexture (ctx, texture, { mip, 0 });
        ASSERT_NE (target, nullptr);

        auto frame = beginFrame();
        auto pass = target->beginRenderPass (frame, { true, GpuColor::black() });
        pass.finish();
        frame.submit();
    }

    ASSERT_EQ (viewDescs.size(), 4u);

    for (uint32_t mip = 0; mip < 4; ++mip)
        EXPECT_EQ (viewDescs[mip].baseMipLevel, mip);
}

TEST_F (GpuAttachmentMockTests, ClearOnlyPassCarriesTheLoadOpAndClearColour)
{
    expectAttachmentCapture();

    auto texture = createTexture (makeRenderTargetDesc (32, 32));
    ASSERT_NE (texture, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, texture);
    ASSERT_NE (target, nullptr);

    auto frame = beginFrame();
    auto pass = target->beginRenderPass (frame, { true, GpuColor (0.25f, 0.5f, 0.75f, 1.0f) });
    pass.finish();
    frame.submit();

    ASSERT_EQ (passDescs.size(), 1u);
    EXPECT_EQ (passDescs[0].colorCount, 1u);
    EXPECT_EQ (passDescs[0].colorAttachments[0].loadOp, rive::ore::LoadOp::clear);
    EXPECT_EQ (passDescs[0].colorAttachments[0].storeOp, rive::ore::StoreOp::store);
    EXPECT_FLOAT_EQ (passDescs[0].colorAttachments[0].clearColor.r, 0.25f);
    EXPECT_FLOAT_EQ (passDescs[0].colorAttachments[0].clearColor.g, 0.5f);
    EXPECT_FLOAT_EQ (passDescs[0].colorAttachments[0].clearColor.b, 0.75f);
    EXPECT_EQ (passDescs[0].depthStencil.view, nullptr);
}

TEST_F (GpuAttachmentMockTests, LoadOpLoadEncodesNoPassWhenNothingIsDrawn)
{
    expectAttachmentCapture();

    auto texture = createTexture (makeRenderTargetDesc (32, 32));
    ASSERT_NE (texture, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, texture);
    ASSERT_NE (target, nullptr);

    auto frame = beginFrame();
    auto pass = target->beginRenderPass (frame, { false, GpuColor::black() });
    pass.finish();
    frame.submit();

    EXPECT_TRUE (passDescs.empty());
}

TEST_F (GpuAttachmentMockTests, MultipleColourAttachmentsAreAllBound)
{
    expectAttachmentCapture();

    auto first = createTexture (makeRenderTargetDesc (32, 32));
    auto second = createTexture (makeRenderTargetDesc (32, 32));
    auto third = createTexture (makeRenderTargetDesc (32, 32));
    ASSERT_NE (first, nullptr);
    ASSERT_NE (second, nullptr);
    ASSERT_NE (third, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, first);
    ASSERT_NE (target, nullptr);

    auto frame = beginFrame();
    auto pass = target->beginRenderPass (frame, { true, GpuColor::black() });
    pass.setColorAttachment (1, second);
    pass.setColorAttachment (2, third);
    pass.finish();
    frame.submit();

    ASSERT_EQ (passDescs.size(), 1u);
    EXPECT_EQ (passDescs[0].colorCount, 3u);
    EXPECT_NE (passDescs[0].colorAttachments[0].view, nullptr);
    EXPECT_NE (passDescs[0].colorAttachments[1].view, nullptr);
    EXPECT_NE (passDescs[0].colorAttachments[2].view, nullptr);
}

TEST_F (GpuAttachmentMockTests, DepthAttachmentIsBoundWithItsLoadAndClearState)
{
    expectAttachmentCapture();

    auto color = createTexture (makeRenderTargetDesc (32, 32));

    auto depth = createTexture ([]
    {
        auto desc = makeRenderTargetDesc (32, 32);
        desc.format = GpuTextureFormat::depth24plusStencil8;
        return desc;
    }());

    ASSERT_NE (color, nullptr);
    ASSERT_NE (depth, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, color);
    ASSERT_NE (target, nullptr);

    auto frame = beginFrame();
    auto pass = target->beginRenderPass (frame, { true, GpuColor::black() });
    pass.setDepthStencilAttachment (depth, GpuDepthStencilOptions { 0.5f });
    pass.finish();
    frame.submit();

    ASSERT_EQ (passDescs.size(), 1u);
    EXPECT_NE (passDescs[0].depthStencil.view, nullptr);
    EXPECT_EQ (passDescs[0].depthStencil.depthLoadOp, rive::ore::LoadOp::clear);
    EXPECT_EQ (passDescs[0].depthStencil.depthStoreOp, rive::ore::StoreOp::store);
    EXPECT_FLOAT_EQ (passDescs[0].depthStencil.depthClearValue, 0.5f);
}

TEST_F (GpuAttachmentMockTests, ClearOnlyPassCreatesTheAttachmentViewOnly)
{
    expectAttachmentCapture();

    auto color = createTexture (makeRenderTargetDesc (32, 32));
    auto cube = createTexture (makeCubeDesc (32, 3));
    ASSERT_NE (color, nullptr);
    ASSERT_NE (cube, nullptr);

    auto target = GpuTarget::createFromTexture (ctx, color);
    ASSERT_NE (target, nullptr);

    auto frame = beginFrame();
    auto pass = target->beginRenderPass (frame, { true, GpuColor::black() });
    pass.setTexture (0, 0, cube);
    pass.finish();
    frame.submit();

    // No pipeline was bound, so the clear-only path runs and only the attachment
    // view is created - the sampling view is built at draw time.
    ASSERT_FALSE (viewDescs.empty());
    EXPECT_EQ (viewDescs[0].baseMipLevel, 0u);
    EXPECT_EQ (viewDescs[0].layerCount, 1u);
}

#endif
