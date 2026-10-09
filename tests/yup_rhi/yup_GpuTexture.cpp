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

    /** Makes makeTexture() succeed, recording the descriptor it was handed. The mock exists
        before that descriptor does, so it is sized for every upload these tests make. */
    rive::rcp<NiceMock<MockOreTexture>> expectTextureCreation (rive::ore::TextureDesc* captured)
    {
        rive::ore::TextureDesc uploadable;
        uploadable.width = uploadable.height = 16;
        uploadable.depthOrArrayLayers = 6;
        uploadable.type = rive::ore::TextureType::cube;
        uploadable.numMipmaps = 3;

        auto oreTexture = rive::make_rcp<NiceMock<MockOreTexture>> (uploadable);

        EXPECT_CALL (*mockOreCtx, makeTexture (_))
            .WillOnce (DoAll (SaveArg<0> (captured), Return (oreTexture)));

        return oreTexture;
    }

    /** Creates a texture through a succeeding, unobserved makeTexture(). */
    GpuTexture::Ptr createTexture (const GpuTextureDesc& desc)
    {
        ON_CALL (*mockOreCtx, makeTexture (_))
            .WillByDefault (Invoke ([] (const rive::ore::TextureDesc& desc)
        {
            return rive::rcp<rive::ore::Texture> (rive::make_rcp<NiceMock<MockOreTexture>> (desc));
        }));

        return GpuTexture::create (ctx, desc);
    }

    /** A mocked device reporting storage textures. */
    class StorageTextureGpuDevice : public OreInjectedGpuDevice
    {
    public:
        using OreInjectedGpuDevice::OreInjectedGpuDevice;

        bool isStorageTextureAvailable() const noexcept override { return true; }
    };

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

TEST_F (GpuTextureMockTests, CreateStorageTextureNeedsStorageTextureSupport)
{
    auto desc = makeRenderTargetDesc();
    desc.storage = true;

    ASSERT_FALSE (ctx->isStorageTextureAvailable());
    EXPECT_CALL (*mockOreCtx, makeTexture (_)).Times (0);
    EXPECT_EQ (GpuTexture::create (ctx, desc), nullptr);
    ::testing::Mock::VerifyAndClearExpectations (mockOreCtx.get());

    ctx = new StorageTextureGpuDevice (mockOreCtx.get());

    rive::ore::TextureDesc captured {};
    auto oreTexture = expectTextureCreation (&captured);

    auto texture = GpuTexture::create (ctx, desc);
    ASSERT_NE (texture, nullptr);
    EXPECT_TRUE (texture->isStorage());
    EXPECT_TRUE (captured.storage);
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
    EXPECT_CALL (*oreTexture, uploadImpl (_)).WillOnce (SaveArg<0> (&captured));

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
    EXPECT_CALL (*oreTexture, uploadImpl (_)).WillOnce (SaveArg<0> (&captured));

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
    EXPECT_CALL (*oreTexture, uploadImpl (_)).WillOnce (SaveArg<0> (&captured));

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
    EXPECT_CALL (*oreTexture, uploadImpl (_)).Times (0);

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
    EXPECT_CALL (*oreTexture, uploadImpl (_)).Times (0);

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
    EXPECT_CALL (*oreTexture, uploadImpl (_)).Times (0);

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
    EXPECT_CALL (*oreTexture, uploadImpl (_)).Times (0);

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
    EXPECT_CALL (*oreTexture, uploadImpl (_)).WillOnce (SaveArg<0> (&captured));

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

TEST_F (GpuTextureMockTests, UploadFailsWhenTheBackendReportsAnError)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);
    EXPECT_CALL (*oreTexture, uploadImpl (_)).WillOnce (Invoke ([this] (const rive::ore::TextureDataDesc&)
    {
        mockOreCtx->setLastError ("upload: rejected");
    }));

    auto texture = GpuTexture::create (ctx, makeRenderTargetDesc (16, 16));
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> pixels (16 * 16 * 4, 0);
    GpuTextureDataDesc data;
    data.data = pixels.data();
    EXPECT_FALSE (texture->upload (data));
}

TEST_F (GpuTextureMockTests, UploadIgnoresAnErrorLeftByEarlierWork)
{
    rive::ore::TextureDesc ignored {};
    auto oreTexture = expectTextureCreation (&ignored);
    EXPECT_CALL (*oreTexture, uploadImpl (_));

    auto texture = GpuTexture::create (ctx, makeRenderTargetDesc (16, 16));
    ASSERT_NE (texture, nullptr);

    mockOreCtx->setLastError ("stale");

    std::vector<uint8_t> pixels (16 * 16 * 4, 0);
    GpuTextureDataDesc data;
    data.data = pixels.data();
    EXPECT_TRUE (texture->upload (data));
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
        ON_CALL (*mockOreCtx, makeTextureViewImpl (_))
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

//==============================================================================
// Shared base for fixtures that render through a real device and read the result back.
// module.cpp includes this file ahead of yup_GpuComputePass.cpp and
// native/yup_GpuDevice_linux.cpp, whose fixtures derive from it.
//==============================================================================

class GpuReadbackFixture : public ::testing::Test
{
protected:
#if YUP_ENABLE_SHADER_TRANSPILER
    /** A fullscreen triangle with uv covering [0, 1] over the viewport. */
    static constexpr const char* fullscreenVertex = R"(#version 450
layout(location = 0) out vec2 uv;
void main()
{
    uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
})";

    /** Samples lod 0 of the texture at binding 0, so a narrowed view decides which mip is read. */
    static constexpr const char* sampleLodZeroFragment = R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = textureLod (sampler2D (source, sourceSampler), uv, 0.0); })";

    /** Compares references 0.2 (into red) and 0.3 (into green) against the depth at binding 0. */
    static constexpr const char* compareFragment = R"(#version 450
layout(set = 0, binding = 0) uniform texture2D shadowMap;
layout(set = 0, binding = 1) uniform samplerShadow shadowSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    float lit = texture (sampler2DShadow (shadowMap, shadowSampler), vec3 (uv, 0.2));
    float shadowed = texture (sampler2DShadow (shadowMap, shadowSampler), vec3 (uv, 0.3));
    color = vec4 (lit, shadowed, 0.0, 1.0);
})";

    /** Compiles @p fragment behind fullscreenVertex for a single rgba8 color target. */
    GpuPipeline::Ptr compileRenderPipeline (const String& fragment)
    {
        GpuPipelineOptions options;
        options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;

        auto compiled = GpuPipeline::compileFromGlsl (device, fullscreenVertex, fragment, options);
        EXPECT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
        return compiled.wasOk() ? compiled.getValue() : nullptr;
    }

    /** Runs a fullscreen fragment shader into a canvas-backed size x size target and returns
        its RGBA8 pixels, or nothing when the readback fails. @p bind sets the pass's textures,
        samplers, buffers and uniforms. */
    std::vector<uint8_t> drawIntoReadback (const String& fragment, int size, const std::function<void (GpuRenderPass&)>& bind)
    {
        auto target = GpuTarget::create (device, size, size);
        EXPECT_NE (target, nullptr);
        if (target == nullptr)
            return {};

        auto pipeline = compileRenderPipeline (fragment);
        if (pipeline == nullptr)
            return {};

        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (pipeline);
        bind (pass);
        EXPECT_TRUE (pass.draw (3));
        pass.finish();
        EXPECT_TRUE (frame.submit());
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        std::vector<uint8_t> pixels (static_cast<size_t> (size * size * 4));
        if (! target->readPixels (pixels.data(), pixels.size()))
            return {};

        return pixels;
    }

    /** Clears one mip level of a render-target texture. */
    void clearMip (GpuTexture::Ptr texture, uint32_t mip, GpuColor color)
    {
        auto target = GpuTarget::createFromTexture (device, texture, GpuTextureViewDesc (mip, 0));
        ASSERT_NE (target, nullptr);

        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, color });
        pass.finish();
        EXPECT_TRUE (frame.submit());
        EXPECT_TRUE (frame.waitForGPU().wasOk());
    }

    /** Clears one layer of a depth render target to @p depth. */
    bool clearDepth (const GpuTexture::Ptr& depthTexture, float depth, uint32_t layer = 0)
    {
        auto color = GpuTexture::create (device, GpuTextureDesc ((uint32_t) depthTexture->getWidth(), (uint32_t) depthTexture->getHeight(), GpuTextureFormat::rgba8unorm, true));
        auto target = GpuTarget::createFromTexture (device, color);
        if (target == nullptr)
            return false;

        GpuDepthStencilOptions depthOptions;
        depthOptions.depthClearValue = depth;

        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setDepthStencilAttachment (depthTexture, depthOptions, GpuTextureViewDesc (0, layer));
        pass.finish();
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());
        return true;
    }

    /** Allocates a depth render target and clears it to @p depth. */
    GpuTexture::Ptr makeClearedDepth (GpuTextureFormat format, float depth, int size)
    {
        auto depthTexture = GpuTexture::create (device, GpuTextureDesc ((uint32_t) size, (uint32_t) size, format, true));
        if (depthTexture == nullptr || ! clearDepth (depthTexture, depth))
            return nullptr;

        return depthTexture;
    }
#endif // YUP_ENABLE_SHADER_TRANSPILER

    GpuDevice::Ptr device;
};

//==============================================================================
// Metal - real device rendering, read back through a canvas-backed target
//==============================================================================

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER

class GpuTextureMetalTests : public GpuReadbackFixture
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";
    }

    /** The readback row showing texture row @p row of a 4x4 level: fullscreenVertex puts v = 0 at the bottom. */
    static int readbackRow (int row) { return 3 - row; }
};

TEST_F (GpuTextureMetalTests, SamplesOneMipThroughANarrowedView)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 2;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    clearMip (texture, 0, GpuColor (1.0f, 0.0f, 0.0f, 1.0f));
    clearMip (texture, 1, GpuColor (0.0f, 1.0f, 0.0f, 1.0f));

    GpuTextureViewDesc mipOne (1, 0);
    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, texture, mipOne);
    });

    ASSERT_FALSE (pixels.empty());
    EXPECT_LE (pixels[0], 5);
    EXPECT_GE (pixels[1], 250);
}

TEST_F (GpuTextureMetalTests, GenerateMipsFillsEveryLevelFromTheBase)
{
    GpuTextureDesc desc (16, 16, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 5;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    clearMip (texture, 0, GpuColor (0.0f, 0.0f, 1.0f, 1.0f));

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (texture->generateMips (frame));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 1, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, texture, GpuTextureViewDesc (4, 0));
    });

    ASSERT_FALSE (pixels.empty());
    EXPECT_GE (pixels[2], 250);
}

TEST_F (GpuTextureMetalTests, GenerateMipsKeepsTheOrientationOfEveryLevel)
{
    GpuTextureDesc desc (16, 16, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 3;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    std::vector<uint8_t> source (16 * 16 * 4);
    for (size_t row = 0; row < 16; ++row)
    {
        for (size_t column = 0; column < 16; ++column)
        {
            auto* texel = source.data() + (row * 16 + column) * 4;
            texel[0] = row < 8 ? 255 : 0;
            texel[2] = row < 8 ? 0 : 255;
            texel[3] = 255;
        }
    }

    GpuTextureDataDesc data;
    data.data = source.data();
    ASSERT_TRUE (texture->upload (data));

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (texture->generateMips (frame));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto readLevel = [&] (uint32_t level)
    {
        return drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass)
        {
            pass.setTexture (0, 0, texture, GpuTextureViewDesc (level, 0));
        });
    };

    const auto base = readLevel (0);
    ASSERT_FALSE (base.empty());
    const auto topRow = 0u;
    const auto bottomRow = 3u * 4u * 4u;

    for (uint32_t level = 1; level < 3; ++level)
    {
        const auto pixels = readLevel (level);
        ASSERT_FALSE (pixels.empty());

        for (uint32_t channel = 0; channel < 3; ++channel)
        {
            EXPECT_NEAR (pixels[topRow + channel], base[topRow + channel], 4) << "level " << level << " top channel " << channel;
            EXPECT_NEAR (pixels[bottomRow + channel], base[bottomRow + channel], 4) << "level " << level << " bottom channel " << channel;
        }
    }
}

TEST_F (GpuTextureMetalTests, UploadIntoARenderTargetWritesTheRequestedMipRegion)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 2;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    clearMip (texture, 1, GpuColor (0.0f, 0.0f, 0.0f, 1.0f));

    // Two rows of two texels, red green over blue white, with a padded row stride
    // and no padding after the last row.
    const uint8_t region[] = { 255, 0, 0, 255, 0, 255, 0, 255, 128, 128, 128, 128, 0, 0, 255, 255, 255, 255, 255, 255 };

    GpuTextureDataDesc data;
    data.data = region;
    data.mipLevel = 1;
    data.x = 2;
    data.y = 1;
    data.width = 2;
    data.height = 2;
    data.bytesPerRow = 12;
    ASSERT_TRUE (texture->upload (data));

    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, texture, GpuTextureViewDesc (1, 0));
    });
    ASSERT_FALSE (pixels.empty());

    for (int row = 0; row < 4; ++row)
    {
        for (int column = 0; column < 4; ++column)
        {
            uint8_t expected[4] = { 0, 0, 0, 255 };
            if (row >= 1 && row <= 2 && column >= 2)
                std::memcpy (expected, region + (row - 1) * 12 + (column - 2) * 4, 4);

            const auto* texel = pixels.data() + (readbackRow (row) * 4 + column) * 4;
            for (int channel = 0; channel < 4; ++channel)
                EXPECT_NEAR (texel[channel], expected[channel], 2) << "row " << row << " column " << column << " channel " << channel;
        }
    }
}

TEST_F (GpuTextureMetalTests, UploadIntoA3DRenderTargetWritesTheRequestedSlices)
{
    GpuTextureDesc desc (4, 4, GpuTextureFormat::rgba8unorm, true);
    desc.type = GpuTextureType::texture3D;
    desc.depthOrArrayLayers = 4;
    auto volume = GpuTexture::create (device, desc);
    ASSERT_NE (volume, nullptr);

    // Slices 1 and 2: red encodes the row, blue the column, green the slice
    std::vector<uint8_t> texels (4 * 4 * 2 * 4);
    for (int slice = 0; slice < 2; ++slice)
    {
        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 4; ++column)
            {
                auto* texel = texels.data() + ((slice * 4 + row) * 4 + column) * 4;
                texel[0] = static_cast<uint8_t> (row * 64);
                texel[1] = static_cast<uint8_t> (80 + slice * 80);
                texel[2] = static_cast<uint8_t> (column * 64);
                texel[3] = 255;
            }
        }
    }

    GpuTextureDataDesc data;
    data.data = texels.data();
    data.z = 1;
    data.depth = 2;
    ASSERT_TRUE (volume->upload (data));

    auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
    ASSERT_NE (sampler, nullptr);

    for (int slice = 0; slice < 2; ++slice)
    {
        const auto w = String ((slice + 1.5f) / 4.0f);
        const auto pixels = drawIntoReadback (R"(#version 450
layout(set = 0, binding = 0) uniform texture3D volume;
layout(set = 0, binding = 1) uniform sampler volumeSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = textureLod (sampler3D (volume, volumeSampler), vec3 (uv, )" + w + R"(), 0.0); })",
                                              4,
                                              [&] (GpuRenderPass& pass)
        {
            pass.setTexture (0, 0, volume);
            pass.setSampler (0, 1, sampler);
        });
        ASSERT_FALSE (pixels.empty());

        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 4; ++column)
            {
                const auto* expected = texels.data() + ((slice * 4 + row) * 4 + column) * 4;
                const auto* texel = pixels.data() + (readbackRow (row) * 4 + column) * 4;
                for (int channel = 0; channel < 4; ++channel)
                    EXPECT_NEAR (texel[channel], expected[channel], 2) << "slice " << slice << " row " << row << " column " << column << " channel " << channel;
            }
        }
    }
}

TEST_F (GpuTextureMetalTests, UploadIntoACombinedDepthStencilRenderTargetFails)
{
    for (auto format : { GpuTextureFormat::depth24plusStencil8, GpuTextureFormat::depth32floatStencil8 })
    {
        auto texture = GpuTexture::create (device, GpuTextureDesc (8, 8, format, true));
        ASSERT_NE (texture, nullptr);

        std::vector<uint8_t> texels (8 * 8 * 8, 0);
        GpuTextureDataDesc data;
        data.data = texels.data();
        EXPECT_FALSE (texture->upload (data)) << (int) format;
    }
}

TEST_F (GpuTextureMetalTests, GenerateMipsReachesTheLastLevelOfAnOddSizedTexture)
{
    GpuTextureDesc desc (5, 5, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 3;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    clearMip (texture, 0, GpuColor (0.0f, 1.0f, 0.0f, 1.0f));

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (texture->generateMips (frame));
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 1, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, texture, GpuTextureViewDesc (2, 0));
    });

    ASSERT_FALSE (pixels.empty());
    EXPECT_GE (pixels[1], 250);
}

TEST_F (GpuTextureMetalTests, GenerateMipsRejectsUnsupportedTextures)
{
    auto frame = GpuFrame::begin (device);

    auto single = GpuTexture::create (device, GpuTextureDesc (8, 8, GpuTextureFormat::rgba8unorm, true));
    ASSERT_NE (single, nullptr);
    EXPECT_FALSE (single->generateMips (frame));

    GpuTextureDesc notTarget (8, 8, GpuTextureFormat::rgba8unorm, false);
    notTarget.mipLevels = 2;
    auto notTargetTexture = GpuTexture::create (device, notTarget);
    ASSERT_NE (notTargetTexture, nullptr);
    EXPECT_FALSE (notTargetTexture->generateMips (frame));

    GpuTextureDesc cube (8, 8, GpuTextureFormat::rgba8unorm, true);
    cube.type = GpuTextureType::cube;
    cube.depthOrArrayLayers = 6;
    cube.mipLevels = 2;
    auto cubeTexture = GpuTexture::create (device, cube);
    ASSERT_NE (cubeTexture, nullptr);
    EXPECT_FALSE (cubeTexture->generateMips (frame));

    GpuTextureDesc depth (8, 8, GpuTextureFormat::depth32float, true);
    depth.mipLevels = 2;
    auto depthTexture = GpuTexture::create (device, depth);
    ASSERT_NE (depthTexture, nullptr);
    EXPECT_FALSE (depthTexture->generateMips (frame));

    GpuTextureDesc multisampled (8, 8, GpuTextureFormat::rgba8unorm, true);
    multisampled.mipLevels = 1;
    multisampled.sampleCount = 4;
    auto multisampledTexture = GpuTexture::create (device, multisampled);
    ASSERT_NE (multisampledTexture, nullptr);
    EXPECT_FALSE (multisampledTexture->generateMips (frame));

    frame.submit();
}

TEST_F (GpuTextureMetalTests, WholeViewAfterNarrowedViewSeesAllLevels)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 2;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    clearMip (texture, 0, GpuColor (1.0f, 0.0f, 0.0f, 1.0f));
    clearMip (texture, 1, GpuColor (0.0f, 1.0f, 0.0f, 1.0f));

    drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass) { pass.setTexture (0, 0, texture, GpuTextureViewDesc (1, 0)); });
    const auto pixels = drawIntoReadback (sampleLodZeroFragment, 4, [&] (GpuRenderPass& pass) { pass.setTexture (0, 0, texture); });

    ASSERT_FALSE (pixels.empty());
    EXPECT_GE (pixels[0], 250); // lod 0 of the whole texture is mip 0 again
    EXPECT_LE (pixels[1], 5);
}

TEST_F (GpuTextureMetalTests, ComparisonSamplerComparesAgainstStoredDepth)
{
    for (auto format : { GpuTextureFormat::depth16unorm, GpuTextureFormat::depth32float, GpuTextureFormat::depth24plusStencil8 })
    {
        auto depth = makeClearedDepth (format, 0.25f, 8);
        ASSERT_NE (depth, nullptr);

        GpuSamplerDesc compareDesc (GpuFilter::linear, GpuWrapMode::clampToEdge);
        compareDesc.compare = GpuCompareFunction::lessEqual;
        auto sampler = GpuSampler::create (device, compareDesc);
        ASSERT_NE (sampler, nullptr);

        const auto pixels = drawIntoReadback (compareFragment, 8, [&] (GpuRenderPass& pass)
        {
            pass.setTexture (0, 0, depth);
            pass.setSampler (0, 1, sampler);
        });

        ASSERT_EQ (pixels.size(), 8u * 8u * 4u);
        const auto center = (4u * 8u + 4u) * 4u;
        EXPECT_GE (pixels[center + 0], 250) << "format " << (int) format;
        EXPECT_LE (pixels[center + 1], 5) << "format " << (int) format;
    }
}

TEST_F (GpuTextureMetalTests, ShadowSamplerSlotDefaultsToAComparisonSampler)
{
    auto depth = makeClearedDepth (GpuTextureFormat::depth32float, 0.25f, 8);
    ASSERT_NE (depth, nullptr);

    // No setSampler: the pipeline's default sampler for the samplerShadow slot must compare
    const auto pixels = drawIntoReadback (compareFragment, 8, [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, depth);
    });

    ASSERT_EQ (pixels.size(), 8u * 8u * 4u);
    const auto center = (4u * 8u + 4u) * 4u;
    EXPECT_GE (pixels[center + 0], 250);
    EXPECT_LE (pixels[center + 1], 5);
}

TEST_F (GpuTextureMetalTests, ArrayComparisonSamplerComparesEachLayer)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::depth32float, true);
    desc.type = GpuTextureType::array2D;
    desc.depthOrArrayLayers = 2;

    auto depth = GpuTexture::create (device, desc);
    ASSERT_NE (depth, nullptr);
    ASSERT_TRUE (clearDepth (depth, 0.25f, 0));
    ASSERT_TRUE (clearDepth (depth, 0.75f, 1));

    GpuSamplerDesc compareDesc (GpuFilter::linear, GpuWrapMode::clampToEdge);
    compareDesc.compare = GpuCompareFunction::lessEqual;
    auto sampler = GpuSampler::create (device, compareDesc);
    ASSERT_NE (sampler, nullptr);

    // Reference 0.5 sits between the layers: it fails against 0.25 and passes against 0.75
    const auto pixels = drawIntoReadback (R"(#version 450
layout(set = 0, binding = 0) uniform texture2DArray shadowMaps;
layout(set = 0, binding = 1) uniform samplerShadow shadowSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    float layer0 = texture (sampler2DArrayShadow (shadowMaps, shadowSampler), vec4 (uv, 0.0, 0.5));
    float layer1 = texture (sampler2DArrayShadow (shadowMaps, shadowSampler), vec4 (uv, 1.0, 0.5));
    color = vec4 (layer0, layer1, 0.0, 1.0);
})",
                                          8,
                                          [&] (GpuRenderPass& pass)
    {
        pass.setTexture (0, 0, depth);
        pass.setSampler (0, 1, sampler);
    });

    ASSERT_EQ (pixels.size(), 8u * 8u * 4u);
    const auto center = (4u * 8u + 4u) * 4u;
    EXPECT_LE (pixels[center + 0], 5);
    EXPECT_GE (pixels[center + 1], 250);
}

#endif // YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER
