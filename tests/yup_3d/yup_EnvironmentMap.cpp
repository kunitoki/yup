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

#include <yup_3d/yup_3d.h>

#if YUP_LINUX
#include <SDL3/SDL.h>

#include <cstdlib>
#endif

#include <cstring>

using namespace yup;

class EnvironmentMapTests : public ::testing::Test
{
protected:
    static Vector3<float> constantRadiance (const Vector3<float>&)
    {
        return { 0.5f, 0.25f, 1.0f };
    }

    static Vector3<float> upperHemisphere (const Vector3<float>& direction)
    {
        return direction.getY() > 0.0f ? Vector3<float> (1.0f, 1.0f, 1.0f) : Vector3<float>();
    }
};

TEST_F (EnvironmentMapTests, FacesAreAQuarterOfTheWidth)
{
    EnvironmentMap environment (constantRadiance, 64, 4);
    EXPECT_EQ (environment.getFaceSize(), 16);
    EXPECT_EQ (environment.getNumLevels(), 4);
}

TEST_F (EnvironmentMapTests, NumberOfLevelsIsLimitedByTheFaceSize)
{
    EnvironmentMap environment (constantRadiance, 32, 10);
    EXPECT_EQ (environment.getNumLevels(), 4); // 8, 4, 2, 1
}

TEST_F (EnvironmentMapTests, IrradianceMatchesTheCpuReference)
{
    const EnvironmentMap environment (upperHemisphere, 128, 1);
    const auto expected = SphericalHarmonics::toIrradiance (SphericalHarmonics::project (upperHemisphere));
    const auto& actual = environment.getIrradianceCoefficients();

    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_NEAR (actual[i].getX(), expected[i].getX(), 0.02f) << "coefficient " << i;
        EXPECT_NEAR (actual[i].getY(), expected[i].getY(), 0.02f) << "coefficient " << i;
        EXPECT_NEAR (actual[i].getZ(), expected[i].getZ(), 0.02f) << "coefficient " << i;
    }

    // Not vacuous: the sky lights from above
    EXPECT_GT (actual[1].getX(), 0.1f);
}

TEST_F (EnvironmentMapTests, RoughnessSpansTheLevels)
{
    EXPECT_FLOAT_EQ (EnvironmentMap::getRoughnessForLevel (0, 6), 0.0f);
    EXPECT_FLOAT_EQ (EnvironmentMap::getRoughnessForLevel (5, 6), 1.0f);
    EXPECT_FLOAT_EQ (EnvironmentMap::getRoughnessForLevel (0, 1), 0.0f);
}

TEST_F (EnvironmentMapTests, TextureCoordinatesRoundTrip)
{
    for (const auto& direction : { Vector3<float> (0.0f, 0.0f, 1.0f), Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (-0.48f, 0.6f, -0.64f), Vector3<float> (0.0f, -0.8f, -0.6f) })
    {
        const auto uv = EnvironmentMap::getTextureCoordinates (direction);
        EXPECT_GE (uv.getX(), 0.0f);
        EXPECT_LT (uv.getX(), 1.0f);

        const auto back = EnvironmentMap::getDirection (uv);
        EXPECT_NEAR (back.getX(), direction.getX(), 1.0e-5f);
        EXPECT_NEAR (back.getY(), direction.getY(), 1.0e-5f);
        EXPECT_NEAR (back.getZ(), direction.getZ(), 1.0e-5f);
    }

    EXPECT_NEAR (EnvironmentMap::getTextureCoordinates ({ 0.0f, 1.0f, 0.0f }).getY(), 0.0f, 1.0e-6f);
    EXPECT_NEAR (EnvironmentMap::getTextureCoordinates ({ 0.0f, -1.0f, 0.0f }).getY(), 1.0f, 1.0e-6f);
}

TEST_F (EnvironmentMapTests, NoGpuObjectsWithoutDevice)
{
    EnvironmentMap environment (constantRadiance, 16, 2);

    ProbeRenderer probes (nullptr);
    const auto build = environment.buildCube (probes);
    EXPECT_EQ (build.cube, nullptr);
    EXPECT_FALSE (build.status.isValid());
}

#if (YUP_APPLE || YUP_LINUX) && YUP_ENABLE_SHADER_TRANSPILER
/** Builds environments on a device and reads their cubes back exactly: every channel of every
    lookup is written as the four bytes of its float into an rgba8 target. Devices come from the
    Metal and GL fixtures below.
*/
class EnvironmentMapGpuTests : public ::testing::Test
{
protected:
    static Vector3<float> constantRadiance (const Vector3<float>&)
    {
        return { 0.5f, 0.25f, 1.0f };
    }

    /** A smooth sky varying along every axis, quadratic towards the zenith in red. */
    static Vector3<float> skyGradient (const Vector3<float>& direction)
    {
        const auto up = 0.5f + 0.5f * direction.getY();
        return { 0.3f + 0.5f * up * up, 0.4f + 0.3f * direction.getX(), 0.6f + 0.3f * direction.getZ() };
    }

    /** The six axes, slightly off the face centers so each lookup stays on one face. */
    static std::vector<Vector3<float>> testDirections()
    {
        std::vector<Vector3<float>> directions { { 1.0f, 0.1f, 0.2f },
                                                 { -1.0f, -0.2f, 0.1f },
                                                 { 0.2f, 1.0f, -0.1f },
                                                 { -0.1f, -1.0f, 0.2f },
                                                 { 0.1f, 0.2f, 1.0f },
                                                 { -0.2f, 0.1f, -1.0f } };

        for (auto& direction : directions)
            direction = direction.normalized();

        return directions;
    }

    /** The decoded radiance of @p cube along each direction at @p lod, read through a trilinear sampler. */
    std::vector<Vector3<float>> sampleCube (const GpuTexture::Ptr& cube, const std::vector<Vector3<float>>& directions, float lod)
    {
        struct SampleBlock
        {
            float params[4] {}; // x lod, y 1 when the cube holds RGBM
            float directions[16][4] {};
        };

        if (cube == nullptr || directions.empty() || directions.size() > 16)
        {
            ADD_FAILURE() << "Nothing to sample";
            return {};
        }

        SampleBlock block;
        block.params[0] = lod;
        block.params[1] = cube->getFormat() == GpuTextureFormat::rgba8unorm ? 1.0f : 0.0f;
        for (size_t i = 0; i < directions.size(); ++i)
        {
            block.directions[i][0] = directions[i].getX();
            block.directions[i][1] = directions[i].getY();
            block.directions[i][2] = directions[i].getZ();
        }

        const auto root = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory();
        const auto shaders = root.getChildFile ("modules/yup_3d/shaders");

        // lib/color.glsl carries an include guard, so it is pasted in right after the version line
        const auto source = String ("#version 450\n") + shaders.getChildFile ("lib/color.glsl").loadFileAsString() + R"(
layout(set = 0, binding = 0) uniform textureCube source;
layout(set = 0, binding = 1) uniform sampler lookupSampler;
layout(set = 1, binding = 0) uniform SampleBlock { vec4 params; vec4 directions[16]; } block;
layout(location = 0) out vec4 color;
void main()
{
    int x = int(gl_FragCoord.x);
    int i = x / 3;
    vec4 texel = textureLod(samplerCube(source, lookupSampler), normalize(block.directions[i].xyz), block.params.x);
    float v = yupDecodeRgbm(texel, block.params.y)[x - i * 3];
    uvec4 bytes = (uvec4(floatBitsToUint(v)) >> uvec4(0u, 8u, 16u, 24u)) & uvec4(255u);
    color = vec4(bytes) / 255.0;
})";

        if (compiledBundle.getSize() == 0)
        {
            ShaderBundleCompileRequest request;
            request.sourceLanguage = ShaderLanguage::glsl;

            for (const auto& [stage, code] : { std::pair { ShaderStage::vertex, shaders.getChildFile ("yup_Fullscreen.vert").loadFileAsString() },
                                               std::pair { ShaderStage::fragment, source } })
            {
                ShaderBundleEntry entry;
                entry.stage = stage;
                entry.source = code;
                entry.targetLanguages = { ShaderLanguage::msl, ShaderLanguage::glsl };
                request.entries.push_back (entry);
            }

            ShaderBundleCompiler compiler;
            auto bundle = compiler.compile (request);
            if (! bundle.wasOk() || ! bundle.getReference().saveToMemoryBlock (compiledBundle).wasOk())
            {
                ADD_FAILURE() << bundle.getErrorMessage();
                return {};
            }
        }

        const auto width = static_cast<int> (directions.size()) * 3;
        auto target = GpuTarget::create (device, width, 1);
        if (target == nullptr)
            return {};

        GpuSamplerDesc samplerDesc (GpuFilter::linear, GpuWrapMode::clampToEdge);
        samplerDesc.mipmapFilter = GpuFilter::linear;
        auto sampler = GpuSampler::create (device, samplerDesc);

        FullscreenPass pass;
        const auto prepared = pass.prepare (device, static_cast<const uint8*> (compiledBundle.getData()), compiledBundle.getSize(), target->asTexture()->getFormat());
        if (prepared.failed() || sampler == nullptr)
        {
            ADD_FAILURE() << prepared.getErrorMessage();
            return {};
        }

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (pass.draw (frame, target->asTexture(), {}, [&] (GpuRenderPass& renderPass)
        {
            renderPass.setTexture (0, 0, cube);
            renderPass.setSampler (0, 1, sampler);
            renderPass.setUniformBuffer (1, 0, &block, sizeof (block));
        }, GpuColor::transparentBlack()));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        std::vector<uint8> pixels (static_cast<size_t> (width * 4));
        if (! target->readPixels (pixels.data(), pixels.size()))
        {
            ADD_FAILURE() << "Unable to read the target back";
            return {};
        }

        std::vector<Vector3<float>> result;
        for (size_t i = 0; i < directions.size(); ++i)
        {
            float rgb[3] {};
            for (size_t c = 0; c < 3; ++c)
            {
                const auto* p = pixels.data() + (i * 3 + c) * 4;
                const auto bits = static_cast<uint32> (p[0]) | (static_cast<uint32> (p[1]) << 8u) | (static_cast<uint32> (p[2]) << 16u) | (static_cast<uint32> (p[3]) << 24u);
                std::memcpy (&rgb[c], &bits, sizeof (float));
            }

            result.emplace_back (rgb[0], rgb[1], rgb[2]);
        }

        return result;
    }

    //==============================================================================
    /** Builds @p environment's cube and waits for it. */
    GpuTexture::Ptr buildAndWait (const EnvironmentMap& environment, ProbeRenderer& renderer)
    {
        const auto build = environment.buildCube (renderer);
        EXPECT_TRUE (build.status.isValid());
        EXPECT_TRUE (build.status.wait().wasOk());
        return build.cube;
    }

    void checkBuildsAPrefilteredCube()
    {
        const EnvironmentMap environment (constantRadiance, 64, 4);

        const auto cube = buildAndWait (environment, *probes);
        ASSERT_NE (cube, nullptr);
        EXPECT_EQ (cube->getType(), GpuTextureType::cube);
        EXPECT_EQ (cube->getFormat(), probes->getRadianceFormat());
        EXPECT_EQ (cube->getDepthOrArrayLayers(), 6u);
        EXPECT_EQ (cube->getWidth(), 16);
        EXPECT_EQ (cube->getHeight(), 16);
        EXPECT_EQ (cube->getMipLevels(), 4u);

        // The environment never changes: every build is a new cube the caller keeps
        const auto again = buildAndWait (environment, *probes);
        ASSERT_NE (again, nullptr);
        EXPECT_NE (again, cube);

        // The default size: faces of 128 texels, 7 roughness levels
        const EnvironmentMap defaultEnvironment (constantRadiance);
        const auto defaultCube = buildAndWait (defaultEnvironment, *probes);
        ASSERT_NE (defaultCube, nullptr);
        EXPECT_EQ (defaultCube->getWidth(), 128);
        EXPECT_EQ (defaultCube->getMipLevels(), 7u);

        for (const auto level : { 0.0f, 6.0f })
        {
            const auto values = sampleCube (defaultCube, testDirections(), level);
            ASSERT_EQ (values.size(), testDirections().size());
            for (const auto& value : values)
                EXPECT_NEAR (value.getZ(), 1.0f, 2.0e-3f) << "level " << level;
        }
    }

    void checkConstantEnvironmentStaysConstantAtEveryLevel (float tolerance)
    {
        const EnvironmentMap environment (constantRadiance, 64, 4);
        const auto cube = buildAndWait (environment, *probes);
        ASSERT_NE (cube, nullptr);

        for (int level = 0; level < environment.getNumLevels(); ++level)
        {
            const auto values = sampleCube (cube, testDirections(), static_cast<float> (level));
            ASSERT_EQ (values.size(), testDirections().size());

            for (size_t i = 0; i < values.size(); ++i)
            {
                EXPECT_NEAR (values[i].getX(), 0.5f, tolerance) << "level " << level << ", direction " << i;
                EXPECT_NEAR (values[i].getY(), 0.25f, tolerance) << "level " << level << ", direction " << i;
                EXPECT_NEAR (values[i].getZ(), 1.0f, tolerance) << "level " << level << ", direction " << i;
            }
        }
    }

    void checkBuildKeepsWhatTheFrameDoesNotRetain()
    {
        const EnvironmentMap environment (constantRadiance, 64, 4);

        // The float image owns the pipeline of its generateMips() pass, which the frame does not
        // retain: the build hands it to the caller, to keep as long as the cube
        const auto build = environment.buildCube (*probes);
        ASSERT_NE (build.cube, nullptr);
        ASSERT_NE (build.image, nullptr);
        EXPECT_EQ (build.image->getWidth(), 64);
        EXPECT_GT (build.image->getMipLevels(), 1u);
        EXPECT_TRUE (build.status.wait().wasOk());

        // The RGBM image has no mips to generate, so nothing is left to keep
        ProbeRenderer rgbmProbes (device);
        rgbmProbes.setForceRgbmForTesting (true);
        const auto rgbmBuild = environment.buildCube (rgbmProbes);
        ASSERT_NE (rgbmBuild.cube, nullptr);
        EXPECT_EQ (rgbmBuild.image, nullptr);
        EXPECT_TRUE (rgbmBuild.status.wait().wasOk());
    }

    void checkRgbmEnvironmentMatchesFloat()
    {
        const EnvironmentMap environment (skyGradient, 128, 5);

        ProbeRenderer rgbmProbes (device);
        rgbmProbes.setForceRgbmForTesting (true);

        const auto floatCube = buildAndWait (environment, *probes);
        const auto rgbmCube = buildAndWait (environment, rgbmProbes);
        ASSERT_NE (floatCube, nullptr);
        ASSERT_NE (rgbmCube, nullptr);
        ASSERT_EQ (floatCube->getFormat(), GpuTextureFormat::rgba16float);
        ASSERT_EQ (rgbmCube->getFormat(), GpuTextureFormat::rgba8unorm);
        ASSERT_EQ (rgbmCube->getMipLevels(), floatCube->getMipLevels());

        auto worstError = 0.0f;

        // Integer levels, and one between two levels as the shading reads them
        for (const auto level : { 0.0f, 2.0f, 2.5f, 4.0f })
        {
            const auto expected = sampleCube (floatCube, testDirections(), level);
            const auto actual = sampleCube (rgbmCube, testDirections(), level);
            ASSERT_EQ (expected.size(), testDirections().size());
            ASSERT_EQ (actual.size(), expected.size());

            for (size_t i = 0; i < actual.size(); ++i)
            {
                // RGBM shares one multiplier between the channels, so its precision follows the largest
                const auto largest = jmax (expected[i].getX(), expected[i].getY(), expected[i].getZ());
                EXPECT_GT (largest, 0.3f) << "level " << level << ", direction " << i;

                EXPECT_NEAR (actual[i].getX(), expected[i].getX(), 0.03f * largest) << "level " << level << ", direction " << i;
                EXPECT_NEAR (actual[i].getY(), expected[i].getY(), 0.03f * largest) << "level " << level << ", direction " << i;
                EXPECT_NEAR (actual[i].getZ(), expected[i].getZ(), 0.03f * largest) << "level " << level << ", direction " << i;

                worstError = jmax (worstError, std::abs (actual[i].getX() - expected[i].getX()) / largest,
                                   jmax (std::abs (actual[i].getY() - expected[i].getY()), std::abs (actual[i].getZ() - expected[i].getZ())) / largest);
            }
        }

        ::testing::Test::RecordProperty ("rgbmWorstErrorOfTheLargestChannel", String (worstError, 4).toStdString());
    }

    void checkFailedBuildReportsItsStatus()
    {
        const EnvironmentMap environment (constantRadiance, 64, 4);

        // Submitted without waiting: the failure shows in the status only, for the caller to build again
        device->failNextFrameForTesting ("injected failure");
        const auto build = environment.buildCube (*probes);
        EXPECT_NE (build.cube, nullptr);
        ASSERT_TRUE (build.status.isValid());
        EXPECT_EQ (build.status.wait().getErrorMessage(), "injected failure");

        const auto rebuilt = buildAndWait (environment, *probes);
        ASSERT_NE (rebuilt, nullptr);

        const auto values = sampleCube (rebuilt, testDirections(), 0.0f);
        ASSERT_EQ (values.size(), testDirections().size());
        for (const auto& value : values)
            EXPECT_NEAR (value.getX(), 0.5f, 2.0e-3f);
    }

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<ProbeRenderer> probes;
    MemoryBlock compiledBundle;
};

//==============================================================================
#if YUP_APPLE
class EnvironmentMapMetalTests : public EnvironmentMapGpuTests
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr || ! device->isGpuAvailable())
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";

        probes = std::make_unique<ProbeRenderer> (device);
    }
};

TEST_F (EnvironmentMapMetalTests, BuildsAPrefilteredCube)
{
    checkBuildsAPrefilteredCube();
    EXPECT_EQ (probes->getRadianceFormat(), GpuTextureFormat::rgba16float); // Half floats render on Metal
}

TEST_F (EnvironmentMapMetalTests, ConstantEnvironmentStaysConstantAtEveryLevel)
{
    checkConstantEnvironmentStaysConstantAtEveryLevel (1.0e-3f);
}

TEST_F (EnvironmentMapMetalTests, BuildKeepsWhatTheFrameDoesNotRetain)
{
    checkBuildKeepsWhatTheFrameDoesNotRetain();
}

TEST_F (EnvironmentMapMetalTests, RgbmEnvironmentMatchesFloat)
{
    checkRgbmEnvironmentMatchesFloat();
}

TEST_F (EnvironmentMapMetalTests, FailedBuildReportsItsStatus)
{
    checkFailedBuildReportsItsStatus();
}
#endif

//==============================================================================
#if YUP_LINUX
class EnvironmentMapGLTests : public EnvironmentMapGpuTests
{
protected:
    void SetUp() override
    {
        setenv ("LIBGL_ALWAYS_SOFTWARE", "1", 0);
        setenv ("GALLIUM_DRIVER", "llvmpipe", 0);

        SDL_SetHint (SDL_HINT_RENDER_DRIVER, "opengl");
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, YUP_RIVE_OPENGL_MAJOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, YUP_RIVE_OPENGL_MINOR);
        SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        window = SDL_CreateWindow ("yup_environment_map_gl_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (window == nullptr)
            GTEST_SKIP() << "Cannot create an SDL window: " << SDL_GetError();

        glContext = SDL_GL_CreateContext (window);
        if (glContext == nullptr)
            GTEST_SKIP() << "Cannot create an OpenGL context: " << SDL_GetError();

        SDL_GL_MakeCurrent (window, glContext);

        GpuDevice::Options options;
        options.loaderFunction = (GpuDevice::LoaderFunction) SDL_GL_GetProcAddress;
        options.readableFramebuffer = true;

        device = GpuDevice::create (GpuPlatform::OpenGL, options);
        if (device == nullptr)
            GTEST_SKIP() << "No OpenGL device available";

        context = GraphicsContext::createContext (GpuPlatform::OpenGL, options, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No OpenGL graphics context available";

        probes = std::make_unique<ProbeRenderer> (device);
    }

    void TearDown() override
    {
        probes = nullptr;
        context = nullptr;
        device = nullptr;

        if (glContext != nullptr)
            SDL_GL_DestroyContext (glContext);

        if (window != nullptr)
            SDL_DestroyWindow (window);
    }

    SDL_Window* window = nullptr;
    SDL_GLContext glContext = nullptr;
};

TEST_F (EnvironmentMapGLTests, BuildsAPrefilteredCube)
{
    checkBuildsAPrefilteredCube();
}

TEST_F (EnvironmentMapGLTests, ConstantEnvironmentStaysConstantAtEveryLevel)
{
    checkConstantEnvironmentStaysConstantAtEveryLevel (2.0e-3f);
}

TEST_F (EnvironmentMapGLTests, BuildKeepsWhatTheFrameDoesNotRetain)
{
    checkBuildKeepsWhatTheFrameDoesNotRetain();
}

TEST_F (EnvironmentMapGLTests, RgbmEnvironmentMatchesFloat)
{
    checkRgbmEnvironmentMatchesFloat();
}

TEST_F (EnvironmentMapGLTests, FailedBuildReportsItsStatus)
{
    checkFailedBuildReportsItsStatus();
}
#endif
#endif
