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

//==============================================================================
// GpuComputePass — headless path (compute not available)
//==============================================================================

class GpuComputePassHeadlessTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Headless, {});
        ASSERT_NE (device, nullptr);
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuComputePassHeadlessTests, BeginWithNullDeviceReturnsInvalidPass)
{
    auto pass = GpuComputePass::begin (nullptr);
    EXPECT_FALSE (pass.isValid());
}

TEST_F (GpuComputePassHeadlessTests, BeginWithHeadlessDeviceReturnsInvalidPass)
{
    // Headless backend does not support compute shaders.
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
}

TEST_F (GpuComputePassHeadlessTests, BeginOnInvalidFrameReturnsInvalidPass)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_FALSE (frame.isValid());

    auto pass = GpuComputePass::begin (frame);
    EXPECT_FALSE (pass.isValid());
}

TEST_F (GpuComputePassHeadlessTests, RunOnComputeContextRunsWorkSynchronously)
{
    // Backends without a dedicated compute context run the work directly.
    bool ran = false;
    device->runOnComputeContext ([&]
    {
        ran = true;
    });
    EXPECT_TRUE (ran);
}

TEST_F (GpuComputePassHeadlessTests, SetPipelineOnInvalidPassDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_NO_THROW (pass.setPipeline (nullptr));
}

TEST_F (GpuComputePassHeadlessTests, SetStorageBufferOnInvalidPassDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_NO_THROW (pass.setStorageBuffer (0, 0, nullptr));
}

TEST_F (GpuComputePassHeadlessTests, SetUniformBufferOnInvalidPassDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());

    float data[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    EXPECT_NO_THROW (pass.setUniformBuffer (0, 0, data, sizeof (data)));
}

TEST_F (GpuComputePassHeadlessTests, SetTextureOnInvalidPassDoesNotCrash)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_NO_THROW (pass.setTexture (0, 0, nullptr));
}

TEST_F (GpuComputePassHeadlessTests, SetStorageTextureOnUnsupportedDeviceIsIgnored)
{
    EXPECT_FALSE (device->isStorageTextureAvailable());

    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_NO_THROW ({
        pass.setStorageTexture (0, 0, nullptr);
        pass.setSampler (0, 1, nullptr);
    });
}

TEST_F (GpuComputePassHeadlessTests, DispatchOnInvalidPassReturnsFalse)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_FALSE (pass.dispatch (1, 1, 1));
    EXPECT_FALSE (pass.dispatch (16, 8, 4));
}

TEST_F (GpuComputePassHeadlessTests, FinishOnInvalidPassReturnsFalse)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.isValid());
    EXPECT_FALSE (pass.finish());
}

TEST_F (GpuComputePassHeadlessTests, FinishIsIdempotentOnInvalidPass)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_FALSE (pass.finish());
    EXPECT_FALSE (pass.finish());
}

TEST_F (GpuComputePassHeadlessTests, MoveConstructionFromInvalidPass)
{
    auto src = GpuComputePass::begin (device);
    EXPECT_FALSE (src.isValid());

    GpuComputePass dst (std::move (src));
    EXPECT_FALSE (dst.isValid());
    EXPECT_FALSE (src.isValid());

    EXPECT_FALSE (dst.finish());
}

TEST_F (GpuComputePassHeadlessTests, MoveAssignmentFromInvalidPass)
{
    auto src = GpuComputePass::begin (device);
    auto dst = GpuComputePass::begin (device);

    dst = std::move (src);
    EXPECT_FALSE (dst.isValid());
    EXPECT_FALSE (src.isValid());
}

TEST_F (GpuComputePassHeadlessTests, DestructorOnInvalidPassDoesNotCrash)
{
    {
        auto pass = GpuComputePass::begin (device);
        EXPECT_FALSE (pass.isValid());
        // Destructor should not crash.
    }
    EXPECT_TRUE (true);
}

TEST_F (GpuComputePassHeadlessTests, SetPipelineWithNonNullDoesNotCrash)
{
    // Even though pipeline is null (no compute support), the call should not crash.
    auto pass = GpuComputePass::begin (device);
    EXPECT_NO_THROW (pass.setPipeline (GpuComputePipeline::Ptr (nullptr)));
}

TEST_F (GpuComputePassHeadlessTests, SetStorageBufferCoversMultipleGroups)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_NO_THROW ({
        pass.setStorageBuffer (0, 0, nullptr);
        pass.setStorageBuffer (0, 1, nullptr);
        pass.setStorageBuffer (1, 0, nullptr);
        pass.setStorageBuffer (1, 1, nullptr);
    });
}

TEST_F (GpuComputePassHeadlessTests, SetUniformBufferCoversMultipleGroups)
{
    auto pass = GpuComputePass::begin (device);
    float data = 42.0f;

    EXPECT_NO_THROW ({
        pass.setUniformBuffer (0, 0, &data, sizeof (data));
        pass.setUniformBuffer (0, 1, &data, sizeof (data));
        pass.setUniformBuffer (1, 0, &data, sizeof (data));
    });
}

TEST_F (GpuComputePassHeadlessTests, SetTextureCoversMultipleGroups)
{
    auto pass = GpuComputePass::begin (device);
    EXPECT_NO_THROW ({
        pass.setTexture (0, 0, nullptr);
        pass.setTexture (0, 1, nullptr);
        pass.setTexture (1, 0, nullptr);
    });
}

//==============================================================================
// GpuComputePipeline — headless path (compute not available)
//==============================================================================

class GpuComputePipelineHeadlessTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Headless, {});
        ASSERT_NE (device, nullptr);
    }

    GpuDevice::Ptr device;
};

TEST_F (GpuComputePipelineHeadlessTests, CompileWithNullDeviceReturnsFailure)
{
    GpuShaderSource source;
    source.language = GpuShaderLanguage::glsl;
    source.code = gpuShaderSourceBytes ("void main() {}");

    GpuWorkgroupSize wgs { 16, 1, 1 };
    auto result = GpuComputePipeline::compile (nullptr, source, wgs);
    EXPECT_TRUE (result.failed());
    EXPECT_FALSE (result.getErrorMessage().isEmpty());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileWithHeadlessDeviceReturnsFailure)
{
    GpuShaderSource source;
    source.language = GpuShaderLanguage::glsl;
    source.code = gpuShaderSourceBytes ("void main() {}");

    GpuWorkgroupSize wgs { 16, 1, 1 };
    auto result = GpuComputePipeline::compile (device, source, wgs);
    EXPECT_TRUE (result.failed());
    EXPECT_STRNE (result.getErrorMessage().toRawUTF8(), "");
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromBundleWithNullDeviceReturnsFailure)
{
    ShaderBundle bundle;
    auto result = GpuComputePipeline::compileFromBundle (nullptr, bundle);
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromBundleWithHeadlessDeviceReturnsFailure)
{
    ShaderBundle bundle;
    auto result = GpuComputePipeline::compileFromBundle (device, bundle);
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromBundleWithDefaultWorkgroupSize)
{
    ShaderBundle bundle;
    auto result = GpuComputePipeline::compileFromBundle (nullptr, bundle, GpuWorkgroupSize { 8, 8, 1 });
    EXPECT_TRUE (result.failed());
}

#if YUP_ENABLE_SHADER_TRANSPILER

TEST_F (GpuComputePipelineHeadlessTests, CompileFromGlslWithNullDeviceReturnsFailure)
{
    auto result = GpuComputePipeline::compileFromGlsl (nullptr, "#version 450\nvoid main() {}");
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromGlslWithHeadlessDeviceReturnsFailure)
{
    auto result = GpuComputePipeline::compileFromGlsl (device, "#version 450\nvoid main() {}");
    EXPECT_TRUE (result.failed());
}

TEST_F (GpuComputePipelineHeadlessTests, CompileFromGlslWithWorkgroupSize)
{
    auto result = GpuComputePipeline::compileFromGlsl (
        nullptr,
        "#version 450\nlayout(local_size_x = 8) in; void main() {}",
        GpuWorkgroupSize { 8, 1, 1 });
    EXPECT_TRUE (result.failed());
}

#endif // YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// Real compute devices - one suite over every backend compiled in, and a small
// fixture per backend for the contracts only that backend has
//==============================================================================

#if YUP_ENABLE_SHADER_TRANSPILER && (YUP_APPLE || (YUP_WINDOWS && YUP_RIVE_USE_D3D) || YUP_RIVE_USE_VULKAN || (YUP_EMSCRIPTEN && RIVE_WEBGPU))

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
#include <emscripten/emscripten.h>
#endif

class GpuComputePassDeviceFixture : public GpuReadbackFixture
{
protected:
    using Texel = std::array<float, 4>;

    static std::string getPlatformName (GpuPlatform platform)
    {
        switch (platform)
        {
            case GpuPlatform::Metal:
                return "Metal";
            case GpuPlatform::Direct3D:
                return "D3D11";
            case GpuPlatform::Vulkan:
                return "Vulkan";
            case GpuPlatform::WebGPU:
                return "WebGPU";
            default:
                return "Unknown";
        }
    }

    /** Opens a compute device on @p platform, skipping the test where the platform has none. */
    void openDevice (GpuPlatform platform)
    {
        if (platform == GpuPlatform::Vulkan)
        {
            if (! GpuDevice::isPlatformSupported (GpuPlatform::Vulkan, {}))
                GTEST_SKIP() << "no Vulkan loader or driver in this environment";

            device = GpuDevice::create (GpuPlatform::Vulkan, {});
            ASSERT_NE (device, nullptr);
            ASSERT_TRUE (device->isComputeAvailable());
            return;
        }

#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        if (platform == GpuPlatform::WebGPU)
        {
            if (EM_ASM_INT ({ return Module.preinitializedWebGPUDevice ? 1 : 0; }) == 0)
                GTEST_SKIP() << "No WebGPU device: run yup_tests.html in a WebGPU browser";

            device = GpuDevice::create (GpuPlatform::WebGPU, {});
            if (device == nullptr || device->getGpuContext() == nullptr || ! device->isComputeAvailable())
                GTEST_SKIP() << "No WebGPU compute device available";

            return;
        }
#endif

        device = GpuDevice::create (platform, {});
        if (device == nullptr || ! device->isComputeAvailable())
            GTEST_SKIP() << "No " << getPlatformName (platform) << " compute device available";
    }

    /** Returns @p succeeded, failing the test when a readback did not complete.

        WebGPU is exempt: the test runner blocks the JavaScript event loop, so buffer mapping
        never completes while a test runs and its pixel checks only run when one does.
    */
    bool readbackCompleted (bool succeeded) const
    {
        if (device->getPlatform() != GpuPlatform::WebGPU)
            EXPECT_TRUE (succeeded) << "readback did not complete";

        return succeeded;
    }

    /** Expects every channel of every RGBA8 pixel to be within 5 of @p expected. */
    static void expectEveryPixel (const std::vector<uint8_t>& pixels, std::array<int, 4> expected)
    {
        for (size_t i = 0; i < pixels.size(); i += 4)
        {
            for (size_t channel = 0; channel < 4; ++channel)
                EXPECT_NEAR (pixels[i + channel], expected[channel], 5) << "pixel " << i / 4 << " channel " << channel;
        }
    }

    /** Copies texel (1, 1) of the mip level held by the `mip` uniform into `texel`. */
    static constexpr const char* fetchTexelSource = R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(std430, set = 0, binding = 2) buffer Result { vec4 texel; };
layout(std140, set = 0, binding = 3) uniform Params { int mip; };
void main() { texel = texelFetch (sampler2D (source, sourceSampler), ivec2 (1, 1), mip); })";

    /** Writes green into every texel of the bound rgba8 storage image. */
    static constexpr const char* fillGreenSource = R"(#version 450
layout(local_size_x = 4, local_size_y = 4) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2D target;
void main() { imageStore (target, ivec2 (gl_GlobalInvocationID.xy), vec4 (0.0, 1.0, 0.0, 1.0)); })";

    /** Returns colors[0] of the read-only storage buffer at set 0 binding 0. */
    static constexpr const char* storageColorFragment = R"(#version 450
layout(std430, set = 0, binding = 0) readonly buffer Colors { vec4 colors[]; };
layout(location = 0) out vec4 color;
void main() { color = colors[0]; })";

    /** Reads texel (1, 1) of mip level @p mip of @p texture in a compute dispatch, or nothing
        when the readback did not complete. */
    std::optional<Texel> fetchTexel (GpuTexture::Ptr texture, int mip)
    {
        Texel texel {};

        auto fetch = GpuComputePipeline::compileFromGlsl (device, fetchTexelSource, GpuWorkgroupSize { 1, 1, 1 });
        EXPECT_TRUE (fetch.wasOk()) << fetch.getErrorMessage();
        if (fetch.failed())
            return std::nullopt;

        auto result = GpuBuffer::create (device, GpuBufferType::storage, texel.data(), sizeof (float) * texel.size());
        auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
        EXPECT_NE (result, nullptr);
        EXPECT_NE (sampler, nullptr);

        const int32_t params[4] = { mip, 0, 0, 0 };

        // texelFetch never uses the sampler, but every declared resource is bound
        auto pass = GpuComputePass::begin (device);
        EXPECT_TRUE (pass.isValid());
        pass.setPipeline (fetch.getValue());
        pass.setTexture (0, 0, texture);
        pass.setSampler (0, 1, sampler);
        pass.setStorageBuffer (0, 2, result);
        pass.setUniformBuffer (0, 3, params, sizeof (params));
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
        EXPECT_TRUE (pass.finish());

        if (! readbackCompleted (device->readBuffer (result, texel.data(), sizeof (float) * texel.size())))
            return std::nullopt;

        return texel;
    }
};

//==============================================================================

class GpuComputePassBackendTests
    : public GpuComputePassDeviceFixture
    , public ::testing::WithParamInterface<GpuPlatform>
{
public:
    static std::vector<GpuPlatform> getCompiledPlatforms()
    {
        std::vector<GpuPlatform> platforms;

#if YUP_APPLE
        platforms.push_back (GpuPlatform::Metal);
#endif
#if YUP_WINDOWS && YUP_RIVE_USE_D3D
        platforms.push_back (GpuPlatform::Direct3D);
#endif
#if YUP_RIVE_USE_VULKAN
        platforms.push_back (GpuPlatform::Vulkan);
#endif
#if YUP_EMSCRIPTEN && RIVE_WEBGPU
        platforms.push_back (GpuPlatform::WebGPU);
#endif

        return platforms;
    }

    static std::string getTestNameSuffix (const ::testing::TestParamInfo<GpuPlatform>& info)
    {
        return getPlatformName (info.param);
    }

protected:
    void SetUp() override
    {
        openDevice (GetParam());
    }

    /** True when the tested platform is one of @p platforms. */
    bool runsOn (std::initializer_list<GpuPlatform> platforms) const
    {
        return std::find (platforms.begin(), platforms.end(), GetParam()) != platforms.end();
    }

    static constexpr std::array<float, 4> red { 1.0f, 0.0f, 0.0f, 1.0f };
    static constexpr std::array<float, 4> green { 0.0f, 1.0f, 0.0f, 1.0f };
    static constexpr std::array<float, 4> blue { 0.0f, 0.0f, 1.0f, 1.0f };

    /** Copies source[0] into destination[0]. */
    static constexpr const char* copyColorSource = R"(#version 450
layout(local_size_x = 1) in;
layout(std430, set = 0, binding = 0) readonly buffer Source { vec4 source[]; };
layout(std430, set = 0, binding = 1) buffer Destination { vec4 destination[]; };
void main() { destination[0] = source[0]; })";

    /** Fills @p target with colors[0] of @p colors, read by storageColorFragment, in @p frame. */
    void drawStorageColor (GpuFrame& frame, GpuTarget& target, const GpuPipeline::Ptr& pipeline, const GpuBuffer::Ptr& colors)
    {
        auto pass = target.beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (pipeline);
        pass.setStorageBuffer (0, 0, colors);
        EXPECT_TRUE (pass.draw (3));
        pass.finish();
    }

    /** The RGBA8 pixels of a 4x4 @p target. */
    static std::vector<uint8_t> readTarget (GpuTarget& target)
    {
        std::vector<uint8_t> pixels (4 * 4 * 4);
        EXPECT_TRUE (target.readPixels (pixels.data(), pixels.size()));
        return pixels;
    }

    /** Expects the pixel at (x, y) of a 4x4 RGBA8 readback to be @p expected. */
    static void expectPixel (const std::vector<uint8_t>& pixels, int x, int y, const std::array<float, 4>& expected)
    {
        const auto index = static_cast<size_t> ((y * 4 + x) * 4);
        for (size_t channel = 0; channel < 4; ++channel)
            EXPECT_NEAR (pixels[index + channel], expected[channel] * 255.0f, 5.0f) << "pixel " << x << "," << y << " channel " << channel;
    }
};

INSTANTIATE_TEST_SUITE_P (GpuComputePass,
                          GpuComputePassBackendTests,
                          ::testing::ValuesIn (GpuComputePassBackendTests::getCompiledPlatforms()),
                          GpuComputePassBackendTests::getTestNameSuffix);

TEST_P (GpuComputePassBackendTests, ReportsStorageTextureSupport)
{
    EXPECT_TRUE (device->isStorageTextureAvailable());

    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba16float);
    desc.storage = true;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    EXPECT_TRUE (texture->isStorage());
}

TEST_P (GpuComputePassBackendTests, ReportsFragmentStorageBufferSupport)
{
    // Core WebGPU guarantees fragment storage buffers; compatibility mode may expose none
    if (device->isCompatibilityMode())
        GTEST_SKIP() << "Compatibility mode device";

    EXPECT_TRUE (device->isFragmentStorageBufferAvailable());
}

TEST_P (GpuComputePassBackendTests, ComputeWriteIsSampledByLaterRenderPass)
{
    // WebGPU has its own version, which fills blue
    if (! runsOn ({ GpuPlatform::Metal, GpuPlatform::Direct3D, GpuPlatform::Vulkan }))
        GTEST_SKIP() << "Covered by GpuComputePassWebGPUTests";

    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba16float);
    desc.storage = true;
    auto written = GpuTexture::create (device, desc);
    ASSERT_NE (written, nullptr);

    auto fill = GpuComputePipeline::compileFromGlsl (device, R"(#version 450
layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba16f) uniform writeonly image2D target;
void main() { imageStore (target, ivec2 (gl_GlobalInvocationID.xy), vec4 (0.0, 1.0, 0.0, 1.0)); })",
                                                     GpuWorkgroupSize { 8, 8, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    // One frame: the dispatch, then the sampling pass, so the frame orders them
    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);
    auto sampling = compileRenderPipeline (sampleLodZeroFragment);
    ASSERT_NE (sampling, nullptr);

    auto frame = GpuFrame::begin (device);
    {
        auto compute = GpuComputePass::begin (frame);
        ASSERT_TRUE (compute.isValid());
        compute.setPipeline (fill.getValue());
        compute.setStorageTexture (0, 0, written);
        EXPECT_TRUE (compute.dispatch (1, 1, 1));
    }

    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (sampling);
    pass.setTexture (0, 0, written);
    EXPECT_TRUE (pass.draw (3));
    pass.finish();
    EXPECT_TRUE (frame.submit());
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8_t> pixels (4 * 4 * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));
    expectEveryPixel (pixels, { 0, 255, 0, 255 });
}

TEST_P (GpuComputePassBackendTests, ComputeSamplesABoundTexture)
{
    auto source = GpuTexture::create (device, GpuTextureDesc (8, 8, GpuTextureFormat::rgba8unorm, true));
    ASSERT_NE (source, nullptr);
    clearMip (source, 0, GpuColor (0.0f, 0.0f, 1.0f, 1.0f));

    if (const auto texel = fetchTexel (source, 0))
        EXPECT_EQ (*texel, (Texel { 0.0f, 0.0f, 1.0f, 1.0f }));
}

TEST_P (GpuComputePassBackendTests, FragmentReadsAStorageBuffer)
{
    if (! device->isFragmentStorageBufferAvailable())
        GTEST_SKIP() << "The device exposes no fragment storage buffers";

    const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    auto colors = device->createBuffer (GpuBufferType::storage, color, sizeof (color));
    ASSERT_NE (colors, nullptr);

    const auto pixels = drawIntoReadback (storageColorFragment, 4, [&] (GpuRenderPass& pass)
    {
        pass.setStorageBuffer (0, 0, colors);
    });

    if (readbackCompleted (! pixels.empty()))
    {
        ASSERT_EQ (pixels.size(), 4u * 4u * 4u);
        expectEveryPixel (pixels, { 0, 0, 255, 255 });
    }
}

TEST_P (GpuComputePassBackendTests, StorageUpdateBetweenFramesKeepsWhatTheEarlierFrameRead)
{
    if (! runsOn ({ GpuPlatform::Metal }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    auto colors = device->createBuffer (GpuBufferType::storage, red.data(), sizeof (red));
    auto pipeline = compileRenderPipeline (storageColorFragment);
    auto first = GpuTarget::create (device, 4, 4);
    auto second = GpuTarget::create (device, 4, 4);
    ASSERT_TRUE (colors != nullptr && pipeline != nullptr && first != nullptr && second != nullptr);

    // The first frame is submitted only after the update, so its draw reads the buffer for sure
    // once the update has happened: the update cannot write where that draw reads
    auto firstFrame = GpuFrame::begin (device);
    drawStorageColor (firstFrame, *first, pipeline, colors);

    EXPECT_TRUE (device->updateBuffer (colors, blue.data(), sizeof (blue)));

    auto secondFrame = GpuFrame::begin (device);
    drawStorageColor (secondFrame, *second, pipeline, colors);

    EXPECT_TRUE (device->updateBuffer (colors, green.data(), sizeof (green)));

    firstFrame.submit();
    secondFrame.submit();
    EXPECT_TRUE (firstFrame.waitForGPU().wasOk());
    EXPECT_TRUE (secondFrame.waitForGPU().wasOk());

    expectPixel (readTarget (*first), 1, 1, red);
    expectPixel (readTarget (*second), 1, 1, blue);
}

TEST_P (GpuComputePassBackendTests, StorageUpdateWithinAFrameKeepsWhatEarlierPassesRead)
{
    if (! runsOn ({ GpuPlatform::Metal }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    auto colors = device->createBuffer (GpuBufferType::storage, red.data(), sizeof (red));
    auto pipeline = compileRenderPipeline (storageColorFragment);
    auto first = GpuTarget::create (device, 4, 4);
    auto second = GpuTarget::create (device, 4, 4);
    ASSERT_TRUE (colors != nullptr && pipeline != nullptr && first != nullptr && second != nullptr);

    auto frame = GpuFrame::begin (device);
    drawStorageColor (frame, *first, pipeline, colors);
    EXPECT_TRUE (device->updateBuffer (colors, blue.data(), sizeof (blue)));
    drawStorageColor (frame, *second, pipeline, colors);
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    expectPixel (readTarget (*first), 1, 1, red);
    expectPixel (readTarget (*second), 1, 1, blue);
}

TEST_P (GpuComputePassBackendTests, StorageUpdateWithinAPassIsSeenByLaterDraws)
{
    if (! runsOn ({ GpuPlatform::Metal }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    auto colors = device->createBuffer (GpuBufferType::storage, red.data(), sizeof (red));
    auto pipeline = compileRenderPipeline (storageColorFragment);
    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_TRUE (colors != nullptr && pipeline != nullptr && target != nullptr);

    // The second draw reuses the pass's bindings, and has to pick up the buffer's new contents
    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (pipeline);
    pass.setStorageBuffer (0, 0, colors);
    pass.setViewport (0.0f, 0.0f, 2.0f, 4.0f);
    EXPECT_TRUE (pass.draw (3));
    EXPECT_TRUE (device->updateBuffer (colors, blue.data(), sizeof (blue)));
    pass.setViewport (2.0f, 0.0f, 2.0f, 4.0f);
    EXPECT_TRUE (pass.draw (3));
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    const auto pixels = readTarget (*target);
    expectPixel (pixels, 0, 1, red);
    expectPixel (pixels, 3, 1, blue);
}

TEST_P (GpuComputePassBackendTests, StorageUpdateAfterAStandaloneDispatchKeepsWhatAFrameRead)
{
    if (! runsOn ({ GpuPlatform::Metal }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    auto colors = device->createBuffer (GpuBufferType::storage, red.data(), sizeof (red));
    auto copied = device->createBuffer (GpuBufferType::storage, green.data(), sizeof (green));
    auto pipeline = compileRenderPipeline (storageColorFragment);
    auto copy = GpuComputePipeline::compileFromGlsl (device, copyColorSource, GpuWorkgroupSize { 1, 1, 1 });
    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_TRUE (colors != nullptr && copied != nullptr && pipeline != nullptr && target != nullptr);
    ASSERT_TRUE (copy.wasOk()) << copy.getErrorMessage();

    // The frame reads the buffer, then a standalone pass submitted and finished after it uses it
    // too: the update still must not write where the not yet submitted frame reads
    auto frame = GpuFrame::begin (device);
    drawStorageColor (frame, *target, pipeline, colors);

    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (copy.getValue());
    pass.setStorageBuffer (0, 0, colors);
    pass.setStorageBuffer (0, 1, copied);
    EXPECT_TRUE (pass.dispatch (1, 1, 1));
    EXPECT_TRUE (pass.finish());

    std::array<float, 4> copiedBack {};
    EXPECT_TRUE (device->readBuffer (copied, copiedBack.data(), sizeof (copiedBack)));
    EXPECT_EQ (copiedBack, red);

    EXPECT_TRUE (device->updateBuffer (colors, blue.data(), sizeof (blue)));

    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());
    expectPixel (readTarget (*target), 1, 1, red);
}

TEST_P (GpuComputePassBackendTests, StorageUpdateKeepsWhatAnEarlierDispatchOfTheFrameRead)
{
    if (! runsOn ({ GpuPlatform::Metal }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    auto colors = device->createBuffer (GpuBufferType::storage, red.data(), sizeof (red));
    auto copied = device->createBuffer (GpuBufferType::storage, green.data(), sizeof (green));
    auto copy = GpuComputePipeline::compileFromGlsl (device, copyColorSource, GpuWorkgroupSize { 1, 1, 1 });
    ASSERT_TRUE (colors != nullptr && copied != nullptr);
    ASSERT_TRUE (copy.wasOk()) << copy.getErrorMessage();

    auto frame = GpuFrame::begin (device);
    {
        auto pass = GpuComputePass::begin (frame);
        ASSERT_TRUE (pass.isValid());
        pass.setPipeline (copy.getValue());
        pass.setStorageBuffer (0, 0, colors);
        pass.setStorageBuffer (0, 1, copied);
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
    }

    EXPECT_TRUE (device->updateBuffer (colors, blue.data(), sizeof (blue)));

    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::array<float, 4> copiedBack {};
    EXPECT_TRUE (device->readBuffer (copied, copiedBack.data(), sizeof (copiedBack)));
    EXPECT_EQ (copiedBack, red);
}

TEST_P (GpuComputePassBackendTests, RepeatedStorageUpdatesWhileInFlightKeepEveryFramesContents)
{
    if (! runsOn ({ GpuPlatform::Metal }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    const std::array<std::array<float, 4>, 3> cycle { red, green, blue };

    auto colors = device->createBuffer (GpuBufferType::storage, cycle[0].data(), sizeof (cycle[0]));
    auto pipeline = compileRenderPipeline (storageColorFragment);
    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_TRUE (colors != nullptr && pipeline != nullptr && target != nullptr);

    // Every update happens while a frame still records a read, so each one moves to a backing
    // the GPU finished with: after the first frames those are recycled spares
    for (size_t index = 0; index < 12; ++index)
    {
        auto frame = GpuFrame::begin (device);
        drawStorageColor (frame, *target, pipeline, colors);

        const auto& next = cycle[(index + 1) % cycle.size()];
        EXPECT_TRUE (device->updateBuffer (colors, next.data(), sizeof (next)));

        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());
        expectPixel (readTarget (*target), 1, 1, cycle[index % cycle.size()]);
    }
}

TEST_P (GpuComputePassBackendTests, PartialStorageUpdateWhileInFlightKeepsTheRest)
{
    if (! runsOn ({ GpuPlatform::Metal }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    std::array<float, 8> contents {};
    std::copy (red.begin(), red.end(), contents.begin());
    std::copy (green.begin(), green.end(), contents.begin() + 4);

    auto colors = device->createBuffer (GpuBufferType::storage, contents.data(), sizeof (contents));
    auto pipeline = compileRenderPipeline (storageColorFragment);
    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_TRUE (colors != nullptr && pipeline != nullptr && target != nullptr);

    // The frame still records, so its pass reads the buffer when it runs: the update moves elsewhere
    auto frame = GpuFrame::begin (device);
    drawStorageColor (frame, *target, pipeline, colors);
    EXPECT_TRUE (device->updateBuffer (colors, blue.data(), sizeof (blue)));

    std::array<float, 8> readBack {};
    EXPECT_TRUE (device->readBuffer (colors, readBack.data(), sizeof (readBack)));
    EXPECT_EQ (std::vector<float> (readBack.begin(), readBack.begin() + 4), std::vector<float> (blue.begin(), blue.end()));
    EXPECT_EQ (std::vector<float> (readBack.begin() + 4, readBack.end()), std::vector<float> (green.begin(), green.end()));

    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());
    expectPixel (readTarget (*target), 1, 1, red);
}

TEST_P (GpuComputePassBackendTests, ComputeWriteIsReadByLaterFragmentPass)
{
    if (! runsOn ({ GpuPlatform::Metal, GpuPlatform::Vulkan }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    const float red[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
    auto colors = device->createBuffer (GpuBufferType::storage, red, sizeof (red));
    ASSERT_NE (colors, nullptr);

    auto fill = GpuComputePipeline::compileFromGlsl (device, R"(#version 450
layout(local_size_x = 1) in;
layout(std430, set = 0, binding = 0) buffer Colors { vec4 colors[]; };
void main() { colors[0] = vec4 (0.0, 0.0, 1.0, 1.0); })",
                                                     GpuWorkgroupSize { 1, 1, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);

    GpuPipelineOptions options;
    options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;
    auto reading = GpuPipeline::compileFromGlsl (device, fullscreenVertex, storageColorFragment, options);
    ASSERT_TRUE (reading.wasOk()) << reading.getErrorMessage();

    // One frame: the dispatch, then the reading pass, so the frame orders them
    auto frame = GpuFrame::begin (device);
    {
        auto compute = GpuComputePass::begin (frame);
        ASSERT_TRUE (compute.isValid());
        compute.setPipeline (fill.getValue());
        compute.setStorageBuffer (0, 0, colors);
        EXPECT_TRUE (compute.dispatch (1, 1, 1));
    }

    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (reading.getValue());
    pass.setStorageBuffer (0, 0, colors);
    EXPECT_TRUE (pass.draw (3));
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8_t> pixels (4 * 4 * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));
    EXPECT_LE (pixels[0], 5);
    EXPECT_LE (pixels[1], 5);
    EXPECT_GE (pixels[2], 250);
}

TEST_P (GpuComputePassBackendTests, StorageArrayTextureWritesEveryLayerOfTheLevel)
{
    // Vulkan refuses storage textures on a pass begun on the device
    if (! runsOn ({ GpuPlatform::Metal, GpuPlatform::Direct3D }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    GpuTextureDesc desc (4, 4, GpuTextureFormat::rgba8unorm);
    desc.type = GpuTextureType::array2D;
    desc.depthOrArrayLayers = 2;
    desc.storage = true;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    auto fill = GpuComputePipeline::compileFromGlsl (device, R"(#version 450
layout(local_size_x = 4, local_size_y = 4) in;
layout(set = 0, binding = 0, rgba8) uniform writeonly image2DArray target;
void main()
{
    const ivec3 texel = ivec3 (gl_GlobalInvocationID);
    imageStore (target, texel, texel.z == 0 ? vec4 (1.0, 0.0, 0.0, 1.0) : vec4 (0.0, 1.0, 0.0, 1.0));
})",
                                                     GpuWorkgroupSize { 4, 4, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    auto read = GpuComputePipeline::compileFromGlsl (device, R"(#version 450
layout(local_size_x = 1) in;
layout(set = 0, binding = 0) uniform texture2DArray source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(std430, set = 0, binding = 2) buffer Result { vec4 texels[2]; };
void main()
{
    texels[0] = texelFetch (sampler2DArray (source, sourceSampler), ivec3 (1, 1, 0), 0);
    texels[1] = texelFetch (sampler2DArray (source, sourceSampler), ivec3 (1, 1, 1), 0);
})",
                                                     GpuWorkgroupSize { 1, 1, 1 });
    ASSERT_TRUE (read.wasOk()) << read.getErrorMessage();

    std::array<float, 8> texels {};
    auto result = GpuBuffer::create (device, GpuBufferType::storage, texels.data(), sizeof (float) * texels.size());
    auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
    ASSERT_NE (result, nullptr);
    ASSERT_NE (sampler, nullptr);

    {
        // The default view selects mip 0; an array texture binds every layer of it
        auto pass = GpuComputePass::begin (device);
        ASSERT_TRUE (pass.isValid());
        pass.setPipeline (fill.getValue());
        pass.setStorageTexture (0, 0, texture);
        EXPECT_TRUE (pass.dispatch (1, 1, 2));
    }

    {
        auto pass = GpuComputePass::begin (device);
        ASSERT_TRUE (pass.isValid());
        pass.setPipeline (read.getValue());
        pass.setTexture (0, 0, texture);
        pass.setSampler (0, 1, sampler);
        pass.setStorageBuffer (0, 2, result);
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
    }

    ASSERT_TRUE (device->readBuffer (result, texels.data(), sizeof (float) * texels.size()));
    EXPECT_EQ (texels, (std::array<float, 8> { 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f }));
}

TEST_P (GpuComputePassBackendTests, StorageViewWritesOnlyTheSelectedMipLevel)
{
    // Vulkan refuses storage textures on a pass begun on the device
    if (! runsOn ({ GpuPlatform::Metal, GpuPlatform::Direct3D }))
        GTEST_SKIP() << "Not verified on " << getPlatformName (GetParam());

    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 2;
    desc.storage = true;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    clearMip (texture, 0, GpuColor (1.0f, 0.0f, 0.0f, 1.0f));
    clearMip (texture, 1, GpuColor (1.0f, 0.0f, 0.0f, 1.0f));

    auto fill = GpuComputePipeline::compileFromGlsl (device, fillGreenSource, GpuWorkgroupSize { 4, 4, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    {
        auto pass = GpuComputePass::begin (device);
        ASSERT_TRUE (pass.isValid());
        pass.setPipeline (fill.getValue());
        pass.setStorageTexture (0, 0, texture, GpuTextureViewDesc (1, 0));
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
    }

    if (const auto texel = fetchTexel (texture, 0))
        EXPECT_EQ (*texel, (Texel { 1.0f, 0.0f, 0.0f, 1.0f }));

    if (const auto texel = fetchTexel (texture, 1))
        EXPECT_EQ (*texel, (Texel { 0.0f, 1.0f, 0.0f, 1.0f }));
}

#endif // YUP_ENABLE_SHADER_TRANSPILER && (YUP_APPLE || (YUP_WINDOWS && YUP_RIVE_USE_D3D) || YUP_RIVE_USE_VULKAN || (YUP_EMSCRIPTEN && RIVE_WEBGPU))

//==============================================================================
// Vulkan - runs where a Vulkan loader and driver are available
//==============================================================================

#if YUP_RIVE_USE_VULKAN && YUP_ENABLE_SHADER_TRANSPILER

class GpuComputePassVulkanTests : public GpuComputePassDeviceFixture
{
protected:
    void SetUp() override
    {
        openDevice (GpuPlatform::Vulkan);
    }

    GpuComputePipeline::Ptr compileDoubler (bool withBindingMap)
    {
        static constexpr auto glsl = "#version 450\n"
                                     "layout(local_size_x = 4) in;\n"
                                     "layout(std430, set = 0, binding = 0) buffer Data { float values[]; };\n"
                                     "void main() { values[gl_GlobalInvocationID.x] *= 2.0; }\n";

        if (withBindingMap)
        {
            auto pipeline = GpuComputePipeline::compileFromGlsl (device, glsl);
            return pipeline.wasOk() ? pipeline.getValue() : nullptr;
        }

        auto spirv = ShaderTranspiler().compileToSPIRV (glsl, ShaderStage::compute, ShaderLanguage::glsl);
        if (spirv.failed())
            return nullptr;

        const auto& module = spirv.getReference();
        auto* bytes = static_cast<const uint8*> (module.getData());

        GpuShaderSource source;
        source.language = GpuShaderLanguage::spirv;
        source.code.assign (bytes, bytes + module.getSize());

        auto pipeline = GpuComputePipeline::compile (device, source, {});
        return pipeline.wasOk() ? pipeline.getValue() : nullptr;
    }
};

TEST_F (GpuComputePassVulkanTests, CompilesSpirvWithoutBindingMapByReflectingIt)
{
    auto pipeline = compileDoubler (false);
    ASSERT_NE (pipeline, nullptr);

    const auto size = pipeline->getWorkgroupSize();
    EXPECT_EQ (size.x, 4u);
    EXPECT_EQ (size.y, 1u);
    EXPECT_EQ (size.z, 1u);
}

TEST_F (GpuComputePassVulkanTests, PassBegunOnFrameRunsWhenTheFrameIsSubmitted)
{
    auto pipeline = compileDoubler (true);
    ASSERT_NE (pipeline, nullptr);

    float values[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buffer = GpuBuffer::create (device, GpuBufferType::storage, values, sizeof (values));
    ASSERT_NE (buffer, nullptr);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    {
        auto pass = GpuComputePass::begin (frame);
        ASSERT_TRUE (pass.isValid());

        pass.setPipeline (pipeline);
        pass.setStorageBuffer (0, 0, buffer);
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
        pass.finish();
    }

    ASSERT_TRUE (frame.submit());

    float result[4] = {};
    ASSERT_TRUE (device->readBuffer (buffer, result, sizeof (result)));
    EXPECT_FLOAT_EQ (result[0], 2.0f);
    EXPECT_FLOAT_EQ (result[1], 4.0f);
    EXPECT_FLOAT_EQ (result[2], 6.0f);
    EXPECT_FLOAT_EQ (result[3], 8.0f);
}

TEST_F (GpuComputePassVulkanTests, ComputeWriteToOneMipKeepsTheOtherLevels)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba8unorm, true);
    desc.mipLevels = 3;
    desc.storage = true;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    // Mip 2 is never written: it has no contents for the layout transition to keep
    clearMip (texture, 0, GpuColor (1.0f, 0.0f, 0.0f, 1.0f));

    auto fill = GpuComputePipeline::compileFromGlsl (device, fillGreenSource, GpuWorkgroupSize { 4, 4, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    auto frame = GpuFrame::begin (device);
    {
        auto pass = GpuComputePass::begin (frame);
        ASSERT_TRUE (pass.isValid());
        pass.setPipeline (fill.getValue());
        pass.setStorageTexture (0, 0, texture, GpuTextureViewDesc (1, 0));
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
    }
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    if (const auto texel = fetchTexel (texture, 0))
        EXPECT_EQ (*texel, (Texel { 1.0f, 0.0f, 0.0f, 1.0f }));

    if (const auto texel = fetchTexel (texture, 1))
        EXPECT_EQ (*texel, (Texel { 0.0f, 1.0f, 0.0f, 1.0f }));
}

TEST_F (GpuComputePassVulkanTests, StorageTextureOnAPassBegunOnTheDeviceIsRejected)
{
    GpuTextureDesc desc (4, 4, GpuTextureFormat::rgba8unorm);
    desc.storage = true;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);

    auto fill = GpuComputePipeline::compileFromGlsl (device, fillGreenSource, GpuWorkgroupSize { 4, 4, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    // Expect an assertion: the image layouts ore tracks belong to the frame's command buffer
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (fill.getValue());
    pass.setStorageTexture (0, 0, texture);
    EXPECT_FALSE (pass.dispatch (1, 1, 1));
}

TEST_F (GpuComputePassVulkanTests, RetiredFrameDoesNotKeepTheDeviceAlive)
{
    const auto referencesBefore = device->getReferenceCount();

    {
        const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
        auto colors = device->createBuffer (GpuBufferType::storage, color, sizeof (color));
        ASSERT_NE (colors, nullptr);

        auto target = GpuTarget::create (device, 4, 4);
        ASSERT_NE (target, nullptr);

        GpuPipelineOptions options;
        options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;
        auto compiled = GpuPipeline::compileFromGlsl (device, fullscreenVertex, storageColorFragment, options);
        ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

        // The frame is submitted and retired as it goes out of scope, without waiting on the GPU
        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (compiled.getValue());
        pass.setStorageBuffer (0, 0, colors);
        EXPECT_TRUE (pass.draw (3));
    }

    // No later frame begins, so the retired frame is not released yet: holding the buffer there
    // would hold the device through it, and the device could never be freed
    EXPECT_EQ (device->getReferenceCount(), referencesBefore);
}

TEST_F (GpuComputePassVulkanTests, StorageBufferDroppedRightAfterTheDrawIsStillRead)
{
    const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    auto colors = device->createBuffer (GpuBufferType::storage, color, sizeof (color));
    ASSERT_NE (colors, nullptr);

    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);

    GpuPipelineOptions options;
    options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;
    auto compiled = GpuPipeline::compileFromGlsl (device, fullscreenVertex, storageColorFragment, options);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    auto frame = GpuFrame::begin (device);
    {
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (compiled.getValue());
        pass.setStorageBuffer (0, 0, colors);
        EXPECT_TRUE (pass.draw (3));
    }

    // The last reference goes before the GPU has even seen the commands
    ASSERT_EQ (colors->getReferenceCount(), 1);
    colors = nullptr;

    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8_t> pixels (4 * 4 * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));
    EXPECT_LE (pixels[0], 5);
    EXPECT_LE (pixels[1], 5);
    EXPECT_GE (pixels[2], 250);
}

#endif // YUP_RIVE_USE_VULKAN && YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// Metal - passes begun on a frame encode into its command buffer
//==============================================================================

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER

class GpuComputePassMetalTests : public GpuComputePassDeviceFixture
{
protected:
    void SetUp() override
    {
        openDevice (GpuPlatform::Metal);
        if (IsSkipped() || HasFatalFailure())
            return;

        auto compiled = GpuComputePipeline::compileFromGlsl (device,
                                                             "#version 450\n"
                                                             "layout(local_size_x = 4) in;\n"
                                                             "layout(std430, set = 0, binding = 0) buffer Data { float values[]; };\n"
                                                             "void main() { values[gl_GlobalInvocationID.x] *= 2.0; }\n");
        ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
        pipeline = compiled.getValue();

        buffer = GpuBuffer::create (device, GpuBufferType::storage, initialValues, sizeof (initialValues));
        ASSERT_NE (buffer, nullptr);
    }

    void dispatchDoubler (GpuComputePass& pass)
    {
        pass.setPipeline (pipeline);
        pass.setStorageBuffer (0, 0, buffer);
        EXPECT_TRUE (pass.dispatch (1, 1, 1));
    }

    std::array<float, 4> readValues()
    {
        std::array<float, 4> values {};
        EXPECT_TRUE (device->readBuffer (buffer, values.data(), sizeof (float) * values.size()));
        return values;
    }

    static constexpr float initialValues[] = { 1.0f, 2.0f, 3.0f, 4.0f };

    GpuComputePipeline::Ptr pipeline;
    GpuBuffer::Ptr buffer;
};

TEST_F (GpuComputePassMetalTests, PassBegunOnFrameRunsWhenTheFrameIsSubmitted)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto pass = GpuComputePass::begin (frame);
    ASSERT_TRUE (pass.isValid());

    dispatchDoubler (pass);
    EXPECT_TRUE (pass.finish());

    // The dispatch waits for the frame's command buffer
    EXPECT_EQ (readValues(), (std::array<float, 4> { 1.0f, 2.0f, 3.0f, 4.0f }));

    ASSERT_TRUE (frame.submit());
    EXPECT_EQ (readValues(), (std::array<float, 4> { 2.0f, 4.0f, 6.0f, 8.0f }));
}

TEST_F (GpuComputePassMetalTests, SubmittingTheFrameFinishesItsOpenPass)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto pass = GpuComputePass::begin (frame);
    ASSERT_TRUE (pass.isValid());
    dispatchDoubler (pass);

    ASSERT_TRUE (frame.submit());

    EXPECT_FALSE (pass.isValid());
    EXPECT_FALSE (pass.dispatch (1, 1, 1));
    EXPECT_FALSE (pass.finish());
    EXPECT_EQ (readValues(), (std::array<float, 4> { 2.0f, 4.0f, 6.0f, 8.0f }));
}

TEST_F (GpuComputePassMetalTests, BeginningAnotherPassOnTheFrameFinishesTheOpenOne)
{
    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto first = GpuComputePass::begin (frame);
    ASSERT_TRUE (first.isValid());
    dispatchDoubler (first);

    auto second = GpuComputePass::begin (frame);
    ASSERT_TRUE (second.isValid());
    EXPECT_FALSE (first.isValid());
    dispatchDoubler (second);

    ASSERT_TRUE (frame.submit());
    EXPECT_EQ (readValues(), (std::array<float, 4> { 4.0f, 8.0f, 12.0f, 16.0f }));
}

TEST_F (GpuComputePassMetalTests, RenderPassOnTheFrameFinishesTheOpenPass)
{
    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);

    auto frame = GpuFrame::begin (device);
    ASSERT_TRUE (frame.isValid());

    auto pass = GpuComputePass::begin (frame);
    ASSERT_TRUE (pass.isValid());
    dispatchDoubler (pass);

    // A clearing pass opens even without draws
    auto renderPass = target->beginRenderPass (frame);
    ASSERT_TRUE (renderPass.isValid());
    EXPECT_TRUE (renderPass.finish());
    EXPECT_FALSE (pass.isValid());

    ASSERT_TRUE (frame.submit());
    EXPECT_EQ (readValues(), (std::array<float, 4> { 2.0f, 4.0f, 6.0f, 8.0f }));
}

TEST_F (GpuComputePassMetalTests, StorageTextureRejectsMultisampling)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba16float, true);
    desc.storage = true;
    desc.sampleCount = 4;
    EXPECT_EQ (GpuTexture::create (device, desc), nullptr);

    desc.sampleCount = 1;
    auto texture = GpuTexture::create (device, desc);
    ASSERT_NE (texture, nullptr);
    EXPECT_TRUE (texture->isStorage());
}

TEST_F (GpuComputePassMetalTests, StorageTextureRejectsDepthFormats)
{
    for (auto format : { GpuTextureFormat::depth16unorm, GpuTextureFormat::depth32float, GpuTextureFormat::depth24plusStencil8 })
    {
        GpuTextureDesc desc (8, 8, format, true);
        desc.storage = true;
        EXPECT_EQ (GpuTexture::create (device, desc), nullptr) << "format " << (int) format;
    }
}

TEST_F (GpuComputePassMetalTests, StorageCubeTextureIsIgnored)
{
#if YUP_ASSERTIONS_ENABLED
    GTEST_SKIP() << "A storage cube texture triggers an assertion in assertion-enabled builds";
#endif

    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba16float);
    desc.type = GpuTextureType::cube;
    desc.depthOrArrayLayers = 6;
    desc.storage = true;
    auto cube = GpuTexture::create (device, desc);
    ASSERT_NE (cube, nullptr);

    GpuTextureDesc planeDesc (8, 8, GpuTextureFormat::rgba16float);
    planeDesc.storage = true;
    auto plane = GpuTexture::create (device, planeDesc);
    ASSERT_NE (plane, nullptr);

    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());

    // A binding keeps its texture alive, so only the accepted plane gains a reference
    pass.setStorageTexture (0, 0, cube, GpuTextureViewDesc (0, 2));
    pass.setStorageTexture (0, 1, plane);
    EXPECT_EQ (cube->getReferenceCount(), 1);
    EXPECT_EQ (plane->getReferenceCount(), 2);
}

TEST_F (GpuComputePassMetalTests, VertexReadsAStorageBuffer)
{
    const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    auto colors = device->createBuffer (GpuBufferType::storage, color, sizeof (color));
    ASSERT_NE (colors, nullptr);

    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);

    GpuPipelineOptions options;
    options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;

    auto compiled = GpuPipeline::compileFromGlsl (device, R"(#version 450
layout(std430, set = 0, binding = 0) readonly buffer Colors { vec4 colors[]; };
layout(location = 0) out vec4 vertexColor;
void main()
{
    const vec2 uv = vec2 ((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4 (uv * 2.0 - 1.0, 0.0, 1.0);
    vertexColor = colors[0];
})",
                                                  R"(#version 450
layout(location = 0) in vec4 vertexColor;
layout(location = 0) out vec4 color;
void main() { color = vertexColor; })",
                                                  options);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    auto frame = GpuFrame::begin (device);
    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (compiled.getValue());
    pass.setStorageBuffer (0, 0, colors);
    EXPECT_TRUE (pass.draw (3));
    pass.finish();
    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());

    std::vector<uint8_t> pixels (4 * 4 * 4);
    ASSERT_TRUE (target->readPixels (pixels.data(), pixels.size()));
    EXPECT_LE (pixels[0], 5);
    EXPECT_LE (pixels[1], 5);
    EXPECT_GE (pixels[2], 250);
}

TEST_F (GpuComputePassMetalTests, FrameRetainsABoundStorageBufferUntilItsGenerationIsSafe)
{
    const float color[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
    auto colors = device->createBuffer (GpuBufferType::storage, color, sizeof (color));
    ASSERT_NE (colors, nullptr);

    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);

    GpuPipelineOptions options;
    options.colorTargets.emplace_back().format = GpuTextureFormat::rgba8unorm;
    auto compiled = GpuPipeline::compileFromGlsl (device, fullscreenVertex, storageColorFragment, options);
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();

    {
        auto frame = GpuFrame::begin (device);
        auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
        pass.setPipeline (compiled.getValue());
        pass.setStorageBuffer (0, 0, colors);
        EXPECT_TRUE (pass.draw (3));
    }

    // The pass is gone and the frame retired without waiting on the GPU, so later
    // frames have to begin before the buffer is let go
    int laterFrames = 0;
    for (; laterFrames < 16 && colors->getReferenceCount() > 1; ++laterFrames)
        GpuFrame::begin (device).submit();

    EXPECT_EQ (colors->getReferenceCount(), 1);
    EXPECT_GT (laterFrames, 1);
}

#endif // YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// WebGPU - runs in a WebGPU browser page; skipped under node, which has no device.
//
// The test runner blocks the JavaScript event loop, so buffer mapping never completes
// while a test runs: readBuffer() and readPixels() return false and the pixel checks
// below, and those of GpuComputePassBackendTests, are skipped. A run without WebGPU
// validation errors in the browser console is what verifies the bindings.
//==============================================================================

#if YUP_EMSCRIPTEN && RIVE_WEBGPU && YUP_ENABLE_SHADER_TRANSPILER

class GpuComputePassWebGPUTests : public GpuComputePassDeviceFixture
{
protected:
    void SetUp() override
    {
        openDevice (GpuPlatform::WebGPU);
    }
};

TEST_F (GpuComputePassWebGPUTests, ComputeWriteIsSampledByLaterRenderPass)
{
    GpuTextureDesc desc (8, 8, GpuTextureFormat::rgba16float);
    desc.storage = true;
    auto written = GpuTexture::create (device, desc);
    ASSERT_NE (written, nullptr);

    auto fill = GpuComputePipeline::compileFromGlsl (device, R"(#version 450
layout(local_size_x = 8, local_size_y = 8) in;
layout(set = 0, binding = 0, rgba16f) uniform writeonly image2D target;
void main() { imageStore (target, ivec2 (gl_GlobalInvocationID.xy), vec4 (0.0, 0.0, 1.0, 1.0)); })",
                                                     GpuWorkgroupSize { 8, 8, 1 });
    ASSERT_TRUE (fill.wasOk()) << fill.getErrorMessage();

    auto target = GpuTarget::create (device, 4, 4);
    ASSERT_NE (target, nullptr);
    auto sampling = compileRenderPipeline (sampleLodZeroFragment);
    ASSERT_NE (sampling, nullptr);

    // The compute pass submits when it finishes, ahead of the frame's own submission
    auto frame = GpuFrame::begin (device);
    {
        auto compute = GpuComputePass::begin (frame);
        ASSERT_TRUE (compute.isValid());
        compute.setPipeline (fill.getValue());
        compute.setStorageTexture (0, 0, written);
        EXPECT_TRUE (compute.dispatch (1, 1, 1));
    }

    auto pass = target->beginRenderPass (frame, { true, GpuColor::transparentBlack() });
    pass.setPipeline (sampling);
    pass.setTexture (0, 0, written);
    EXPECT_TRUE (pass.draw (3));
    pass.finish();
    EXPECT_TRUE (frame.submit());

    std::vector<uint8_t> pixels (4 * 4 * 4);
    if (target->readPixels (pixels.data(), pixels.size()))
        expectEveryPixel (pixels, { 0, 0, 255, 255 });
}

TEST_F (GpuComputePassWebGPUTests, DispatchFailsWhenADeclaredResourceIsUnbound)
{
    auto fetch = GpuComputePipeline::compileFromGlsl (device, fetchTexelSource, GpuWorkgroupSize { 1, 1, 1 });
    ASSERT_TRUE (fetch.wasOk()) << fetch.getErrorMessage();

    auto source = GpuTexture::create (device, GpuTextureDesc (8, 8, GpuTextureFormat::rgba8unorm));
    std::array<float, 4> texel {};
    auto result = GpuBuffer::create (device, GpuBufferType::storage, texel.data(), sizeof (float) * texel.size());
    const int32_t params[4] = { 0, 0, 0, 0 };
    ASSERT_NE (source, nullptr);
    ASSERT_NE (result, nullptr);

    // No sampler: the dispatch is refused (with an assertion) instead of failing on the GPU
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (fetch.getValue());
    pass.setTexture (0, 0, source);
    pass.setStorageBuffer (0, 2, result);
    pass.setUniformBuffer (0, 3, params, sizeof (params));
    EXPECT_FALSE (pass.dispatch (1, 1, 1));
}

TEST_F (GpuComputePassWebGPUTests, DispatchesAShaderUsingOnlyGroupOne)
{
    auto doubler = GpuComputePipeline::compileFromGlsl (device, R"(#version 450
layout(local_size_x = 4) in;
layout(std430, set = 1, binding = 0) buffer Data { float values[]; };
void main() { values[gl_GlobalInvocationID.x] *= 2.0; })",
                                                        GpuWorkgroupSize { 4, 1, 1 });
    ASSERT_TRUE (doubler.wasOk()) << doubler.getErrorMessage();

    std::array<float, 4> values { 1.0f, 2.0f, 3.0f, 4.0f };
    auto buffer = GpuBuffer::create (device, GpuBufferType::storage, values.data(), sizeof (float) * values.size());
    ASSERT_NE (buffer, nullptr);

    // Group 0 is empty, yet still needs a bind group of its own
    auto pass = GpuComputePass::begin (device);
    ASSERT_TRUE (pass.isValid());
    pass.setPipeline (doubler.getValue());
    pass.setStorageBuffer (1, 0, buffer);
    EXPECT_TRUE (pass.dispatch (1, 1, 1));
    EXPECT_TRUE (pass.finish());

    if (device->readBuffer (buffer, values.data(), sizeof (float) * values.size()))
        EXPECT_EQ (values, (std::array<float, 4> { 2.0f, 4.0f, 6.0f, 8.0f }));
}

#endif // YUP_EMSCRIPTEN && RIVE_WEBGPU && YUP_ENABLE_SHADER_TRANSPILER
