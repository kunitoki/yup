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

#if YUP_LINUX
#include <SDL3/SDL.h>

#include <cstdlib>
#endif

#include <cstring>
#include <map>

using namespace yup;

class ProbeRendererTests : public ::testing::Test
{
};

TEST_F (ProbeRendererTests, NothingWithoutDevice)
{
    ProbeRenderer probes (nullptr);

    EXPECT_EQ (probes.createCube (16), nullptr);
}

TEST_F (ProbeRendererTests, NoEnvironmentCubeWithoutDevice)
{
    EnvironmentMap environment ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.25f, 1.0f); }, 16, 2);

    ProbeRenderer probes (nullptr);
    const auto build = probes.buildEnvironmentCube (environment);
    EXPECT_EQ (build.cube, nullptr);
    EXPECT_FALSE (build.status.isValid());
}

#if (YUP_APPLE || YUP_LINUX) && YUP_ENABLE_SHADER_TRANSPILER
/** Builds radiance cubes through ProbeRenderer and reads them back exactly: every channel of every
    lookup is written as the four bytes of its float into an rgba8 target, so no test depends on an
    8-bit quantization. Devices come from the Metal and GL fixtures below.
*/
class ProbeRendererGpuTests : public ::testing::Test
{
protected:
    using RadianceFunction = EnvironmentMap::RadianceFunction;

    static Vector3<float> constantRadiance (const Vector3<float>&)
    {
        return { 0.5f, 0.25f, 1.0f };
    }

    /** A small bright spot towards +X, like a sun. */
    static Vector3<float> brightSpot (const Vector3<float>& direction)
    {
        return direction.getX() > 0.995f ? Vector3<float> (100.0f, 100.0f, 100.0f) : Vector3<float> (0.1f, 0.1f, 0.1f);
    }

    /** A constant far from constantRadiance, for neighboring tiles. */
    static Vector3<float> redRadiance (const Vector3<float>&)
    {
        return { 4.0f, 0.0f, 0.0f };
    }

    /** A smooth sky varying along every axis, quadratic towards the zenith in red. */
    static Vector3<float> skyGradient (const Vector3<float>& direction)
    {
        const auto up = 0.5f + 0.5f * direction.getY();
        return { 0.3f + 0.5f * up * up, 0.4f + 0.3f * direction.getX(), 0.6f + 0.3f * direction.getZ() };
    }

    /** Directions off the face centers, edges and corners, plus one corner. */
    static std::vector<Vector3<float>> testDirections()
    {
        std::vector<Vector3<float>> directions { { 0.3f, 0.8f, 0.52f },
                                                 { -0.6f, 0.2f, 0.77f },
                                                 { 0.1f, -0.9f, -0.42f },
                                                 { -0.7f, -0.3f, -0.65f },
                                                 { 0.9f, 0.1f, -0.4f },
                                                 { -0.2f, 0.95f, -0.25f },
                                                 { 1.0f, 1.0f, 1.0f } };

        for (auto& direction : directions)
            direction = direction.normalized();

        return directions;
    }

    //==============================================================================
    /** The unit direction through @p st (0..1 across a face, t along its rows) of a cube face,
        with the face order and orientation of every GPU API. */
    static Vector3<float> cubeDirection (int face, float s, float t)
    {
        const auto sc = s * 2.0f - 1.0f;
        const auto tc = t * 2.0f - 1.0f;

        switch (face)
        {
            case 0: return Vector3<float> (1.0f, -tc, -sc).normalized();
            case 1: return Vector3<float> (-1.0f, -tc, sc).normalized();
            case 2: return Vector3<float> (sc, 1.0f, tc).normalized();
            case 3: return Vector3<float> (sc, -1.0f, -tc).normalized();
            case 4: return Vector3<float> (sc, -tc, 1.0f).normalized();
            default: return Vector3<float> (-sc, -tc, -1.0f).normalized();
        }
    }

    /** The direction through the center of the texel that @p direction falls in, on faces of @p faceSize. */
    static Vector3<float> texelCenter (const Vector3<float>& direction, int faceSize)
    {
        const auto x = direction.getX(), y = direction.getY(), z = direction.getZ();
        const auto ax = std::abs (x), ay = std::abs (y), az = std::abs (z);

        int face = 0;
        float sc = 0.0f, tc = 0.0f, ma = 1.0f;

        if (ax >= ay && ax >= az)
            std::tie (face, sc, tc, ma) = x > 0.0f ? std::tuple { 0, -z, -y, ax } : std::tuple { 1, z, -y, ax };
        else if (ay >= az)
            std::tie (face, sc, tc, ma) = y > 0.0f ? std::tuple { 2, x, z, ay } : std::tuple { 3, x, -z, ay };
        else
            std::tie (face, sc, tc, ma) = z > 0.0f ? std::tuple { 4, x, -y, az } : std::tuple { 5, -x, -y, az };

        const auto size = static_cast<float> (faceSize);
        const auto snap = [size] (float c)
        {
            return (jlimit (0.0f, size - 1.0f, std::floor ((c * 0.5f + 0.5f) * size)) + 0.5f) / size;
        };

        return cubeDirection (face, snap (sc / ma), snap (tc / ma));
    }

    /** The solid angle of texel (i, j) of a face of @p faceSize texels. */
    static double texelSolidAngle (int i, int j, int faceSize)
    {
        const auto corner = [] (double x, double y) { return std::atan2 (x * y, std::sqrt (x * x + y * y + 1.0)); };
        const auto step = 2.0 / faceSize;
        const auto x0 = -1.0 + i * step, y0 = -1.0 + j * step;

        return corner (x0, y0) - corner (x0, y0 + step) - corner (x0 + step, y0) + corner (x0 + step, y0 + step);
    }

    /** The GGX lobe around @p direction (n = v = r) averaged by brute force: 4096 Hammersley
        samples of the half vector distribution, weighted by n.l (Karis 2013, "Real Shading in
        Unreal Engine 4"), reading the radiance function itself. */
    static Vector3<float> cpuPrefilteredRadiance (const RadianceFunction& radiance, const Vector3<float>& n, float roughness)
    {
        if (roughness <= 0.0f)
            return radiance (n);

        constexpr int numSamples = 4096;
        const auto alpha2 = std::pow (roughness, 4.0f);
        const auto up = std::abs (n.getY()) < 0.999f ? Vector3<float> (0.0f, 1.0f, 0.0f) : Vector3<float> (1.0f, 0.0f, 0.0f);
        const auto tangent = up.crossProduct (n).normalized();
        const auto bitangent = n.crossProduct (tangent);

        Vector3<double> sum;
        double weight = 0.0;

        for (uint32 i = 0; i < numSamples; ++i)
        {
            uint32 bits = i;
            bits = (bits << 16u) | (bits >> 16u);
            bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
            bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
            bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
            bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

            const auto u = (static_cast<float> (i) + 0.5f) / static_cast<float> (numSamples);
            const auto v = static_cast<float> (bits) * 2.3283064365386963e-10f;

            const auto phi = MathConstants<float>::twoPi * v;
            const auto cosTheta = std::sqrt ((1.0f - u) / (1.0f + (alpha2 - 1.0f) * u));
            const auto sinTheta = std::sqrt (1.0f - cosTheta * cosTheta);

            const auto h = tangent * (sinTheta * std::cos (phi)) + bitangent * (sinTheta * std::sin (phi)) + n * cosTheta;
            const auto l = h * (2.0f * cosTheta) - n;
            const auto nDotL = n.dotProduct (l);

            if (nDotL <= 0.0f)
                continue;

            const auto value = radiance (l.normalized());
            sum = sum + Vector3<double> (value.getX(), value.getY(), value.getZ()) * static_cast<double> (nDotL);
            weight += nDotL;
        }

        return (sum / weight).to<float>();
    }

    //==============================================================================
    /** A latitude-longitude texture of @p radiance in the radiance format, with every mip, sampled
        four times per texel on the rotated grid of EnvironmentMap. */
    GpuTexture::Ptr makeEquirect (const RadianceFunction& radiance, int width = 256)
    {
        static constexpr std::array<std::array<float, 2>, 4> offsets { { { 0.375f, 0.125f }, { 0.875f, 0.375f }, { 0.125f, 0.625f }, { 0.625f, 0.875f } } };
        const auto height = width / 2;

        std::vector<float> pixels (static_cast<size_t> (width * height * 4));
        for (int row = 0; row < height; ++row)
        {
            for (int column = 0; column < width; ++column)
            {
                Vector3<float> sum;
                for (const auto& offset : offsets)
                    sum = sum + radiance (EnvironmentMap::getDirection ({ (static_cast<float> (column) + offset[0]) / static_cast<float> (width),
                                                                          (static_cast<float> (row) + offset[1]) / static_cast<float> (height) }));

                auto* texel = pixels.data() + (row * width + column) * 4;
                texel[0] = sum.getX() * 0.25f;
                texel[1] = sum.getY() * 0.25f;
                texel[2] = sum.getZ() * 0.25f;
                texel[3] = 1.0f;
            }
        }

        GpuTextureDesc desc (static_cast<uint32> (width), static_cast<uint32> (height), probes->getRadianceFormat(), true);
        desc.mipLevels = static_cast<uint32> (std::log2 (width)) + 1;

        auto texture = GpuTexture::create (device, desc);
        if (texture == nullptr)
            return nullptr;

        std::vector<uint16> halves;
        std::vector<uint8> rgbm;
        GpuTextureDataDesc data;

        if (desc.format == GpuTextureFormat::rgba16float)
        {
            halves.resize (pixels.size());
            std::transform (pixels.begin(), pixels.end(), halves.begin(), floatToHalf);
            data.data = halves.data();
        }
        else
        {
            rgbm.resize (pixels.size());
            for (size_t i = 0; i < pixels.size(); i += 4)
                encodeRgbm (pixels.data() + i, rgbm.data() + i);

            data.data = rgbm.data();
        }

        if (! texture->upload (data))
            return nullptr;

        auto frame = GpuFrame::begin (device);
        const auto generated = texture->generateMips (frame);
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        return generated ? texture : nullptr;
    }

    /** equirect -> cube -> downsample: the radiance with its mip chain. */
    GpuTexture::Ptr makeSourceCube (const RadianceFunction& radiance, int faceSize)
    {
        auto equirect = makeEquirect (radiance);
        auto cube = probes->createCube (faceSize);
        if (equirect == nullptr || cube == nullptr)
        {
            ADD_FAILURE() << "Unable to create the source textures";
            return nullptr;
        }

        auto frame = GpuFrame::begin (device);
        const auto converted = probes->equirectToCube (frame, equirect, cube);
        const auto downsampled = converted && probes->downsampleCube (frame, cube);
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        EXPECT_TRUE (converted);
        EXPECT_TRUE (downsampled);
        return downsampled ? cube : nullptr;
    }

    /** equirect -> cube -> downsample -> prefilter into a cube of @p faceSize with @p numMips mips (0 for all). */
    GpuTexture::Ptr makeCubeFrom (const RadianceFunction& radiance, int faceSize, int numMips = 0)
    {
        auto source = makeSourceCube (radiance, faceSize);
        auto destination = numMips > 0 ? makeCube (faceSize, numMips) : probes->createCube (faceSize);
        if (source == nullptr || destination == nullptr)
            return nullptr;

        auto frame = GpuFrame::begin (device);
        const auto filtered = probes->prefilterToCube (frame, source, destination);
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        EXPECT_TRUE (filtered);
        return filtered ? destination : nullptr;
    }

    GpuTexture::Ptr makeCube (int faceSize, int numMips)
    {
        GpuTextureDesc desc (static_cast<uint32> (faceSize), static_cast<uint32> (faceSize), probes->getRadianceFormat(), true);
        desc.type = GpuTextureType::cube;
        desc.depthOrArrayLayers = 6;
        desc.mipLevels = static_cast<uint32> (numMips);
        return GpuTexture::create (device, desc);
    }

    GpuTexture::Ptr makeAtlasWithMips (int width, int height, int numMips)
    {
        GpuTextureDesc desc (static_cast<uint32> (width), static_cast<uint32> (height), probes->getRadianceFormat(), true);
        desc.mipLevels = static_cast<uint32> (numMips);
        return GpuTexture::create (device, desc);
    }

    //==============================================================================
    /** Draws @p fragment over a width x height canvas target and returns the float each pixel packs. */
    std::vector<float> readFloats (const String& fragment, int width, int height, const std::function<void (GpuRenderPass&)>& bind)
    {
        const auto root = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory();
        const auto shaders = root.getChildFile ("modules/yup_renderer/shaders");

        // The lib files carry include guards, so they are pasted in right after the version line
        const auto source = String ("#version 450\n")
                          + shaders.getChildFile ("lib/color.glsl").loadFileAsString()
                          + shaders.getChildFile ("lib/octahedral.glsl").loadFileAsString()
                          + packingFunctions
                          + fragment;

        // Compiled once per source: the lookups of a test differ only by their uniforms
        auto& block = compiledBundles[source];
        if (block.getSize() == 0)
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
            if (! bundle.wasOk())
            {
                ADD_FAILURE() << bundle.getErrorMessage();
                return {};
            }

            if (! bundle.getReference().saveToMemoryBlock (block).wasOk())
                return {};
        }

        auto target = GpuTarget::create (device, width, height);
        if (target == nullptr)
            return {};

        FullscreenPass pass;
        const auto prepared = pass.prepare (device, static_cast<const uint8*> (block.getData()), block.getSize(), target->asTexture()->getFormat());
        if (prepared.failed())
        {
            ADD_FAILURE() << prepared.getErrorMessage();
            return {};
        }

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (pass.draw (frame, target->asTexture(), {}, bind, GpuColor::transparentBlack()));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        std::vector<uint8> pixels (static_cast<size_t> (width * height * 4));
        if (! target->readPixels (pixels.data(), pixels.size()))
        {
            ADD_FAILURE() << "Unable to read the target back";
            return {};
        }

        std::vector<float> values (static_cast<size_t> (width * height));
        for (size_t i = 0; i < values.size(); ++i)
        {
            const auto bits = static_cast<uint32> (pixels[i * 4]) | (static_cast<uint32> (pixels[i * 4 + 1]) << 8u)
                            | (static_cast<uint32> (pixels[i * 4 + 2]) << 16u) | (static_cast<uint32> (pixels[i * 4 + 3]) << 24u);
            std::memcpy (&values[i], &bits, sizeof (float));
        }

        return values;
    }

    /** One float per channel: the decoded rgb of each direction, three pixels each. */
    struct SampleBlock
    {
        float params[4] {};    // x lod, y 1 when the texture holds RGBM, z mips of an atlas tile
        float tile[4] {};      // the tile at mip 0, in texels
        float atlasSize[4] {}; // xy the atlas size at mip 0
        float directions[64][4] {};
    };

    std::vector<Vector3<float>> sample (const String& lookup, const GpuTexture::Ptr& texture, SampleBlock block, const std::vector<Vector3<float>>& directions)
    {
        if (texture == nullptr || directions.empty() || directions.size() > 64)
        {
            ADD_FAILURE() << "Nothing to sample";
            return {};
        }

        block.params[1] = texture->getFormat() == GpuTextureFormat::rgba8unorm ? 1.0f : 0.0f;
        for (size_t i = 0; i < directions.size(); ++i)
        {
            block.directions[i][0] = directions[i].getX();
            block.directions[i][1] = directions[i].getY();
            block.directions[i][2] = directions[i].getZ();
        }

        const auto values = readFloats (R"(
layout(set = 0, binding = 1) uniform sampler lookupSampler;
layout(set = 1, binding = 0) uniform SampleBlock { vec4 params; vec4 tile; vec4 atlasSize; vec4 directions[64]; } block;
layout(location = 0) out vec4 color;
)" + lookup + R"(
void main()
{
    int x = int(gl_FragCoord.x);
    int i = x / 3;
    vec3 rgb = yupDecodeRgbm(lookup(normalize(block.directions[i].xyz)), block.params.y);
    color = packFloat(rgb[x - i * 3]);
})",
                                        static_cast<int> (directions.size()) * 3, 1, [&] (GpuRenderPass& pass)
        {
            pass.setTexture (0, 0, texture);
            pass.setSampler (0, 1, sampler());
            pass.setUniformBuffer (1, 0, &block, sizeof (block));
        });

        std::vector<Vector3<float>> result;
        for (size_t i = 0; i + 2 < values.size(); i += 3)
            result.emplace_back (values[i], values[i + 1], values[i + 2]);

        return result;
    }

    /** The decoded radiance of @p cube along each direction, at @p lod. */
    std::vector<Vector3<float>> sampleCube (const GpuTexture::Ptr& cube, const std::vector<Vector3<float>>& directions, float lod)
    {
        SampleBlock block;
        block.params[0] = lod;

        return sample (R"(
layout(set = 0, binding = 0) uniform textureCube source;
vec4 lookup(vec3 d) { return textureLod(samplerCube(source, lookupSampler), d, block.params.x); }
)",
                       cube, block, directions);
    }

    /** The decoded radiance of an octahedral atlas tile of @p numMips mips along each direction,
        at a lod that may fall between mips, read as octahedralTileTaps() says. */
    std::vector<Vector3<float>> sampleAtlas (const GpuTexture::Ptr& atlas, Rectangle<int> tile, const std::vector<Vector3<float>>& directions, float lod, int numMips)
    {
        SampleBlock block;
        block.params[0] = lod;
        block.params[2] = static_cast<float> (numMips);
        block.tile[0] = static_cast<float> (tile.getX());
        block.tile[1] = static_cast<float> (tile.getY());
        block.tile[2] = static_cast<float> (tile.getWidth());
        block.tile[3] = static_cast<float> (tile.getHeight());
        block.atlasSize[0] = static_cast<float> (atlas->getWidth());
        block.atlasSize[1] = static_cast<float> (atlas->getHeight());

        return sample (R"(
layout(set = 0, binding = 0) uniform texture2D source;
vec4 lookup(vec3 d)
{
    OctahedralTileTaps taps = octahedralTileTaps(d, block.tile, block.atlasSize.xy, block.params.x, int(block.params.z));
    return mix(textureLod(sampler2D(source, lookupSampler), taps.uv0, taps.lod0),
               textureLod(sampler2D(source, lookupSampler), taps.uv1, taps.lod1),
               taps.weight);
}
)",
                       atlas, block, directions);
    }

    float sampleRed (const GpuTexture::Ptr& cube, const Vector3<float>& direction, int mip)
    {
        const auto values = sampleCube (cube, { direction }, static_cast<float> (mip));
        return values.empty() ? 0.0f : values[0].getX();
    }

    /** The red channel of one mip integrated over the sphere: every texel times its solid angle. */
    double integrateRed (const GpuTexture::Ptr& cube, int mip)
    {
        const auto faceSize = jmax (1, cube->getWidth() >> mip);

        const float block[4] = { static_cast<float> (faceSize), static_cast<float> (mip), cube->getFormat() == GpuTextureFormat::rgba8unorm ? 1.0f : 0.0f, 0.0f };

        const auto values = readFloats (R"(
layout(set = 0, binding = 0) uniform textureCube source;
layout(set = 0, binding = 1) uniform sampler lookupSampler;
layout(set = 1, binding = 0) uniform IntegrateBlock { vec4 params; } block;
layout(location = 0) out vec4 color;
void main()
{
    int n = int(block.params.x);
    ivec2 p = ivec2(gl_FragCoord.xy);
    int face = p.x / n;
    vec2 c = (vec2(float(p.x - face * n), float(p.y)) + 0.5) / float(n) * 2.0 - 1.0;
    vec3 d = face == 0 ? vec3(1.0, -c.y, -c.x)
           : face == 1 ? vec3(-1.0, -c.y, c.x)
           : face == 2 ? vec3(c.x, 1.0, c.y)
           : face == 3 ? vec3(c.x, -1.0, -c.y)
           : face == 4 ? vec3(c.x, -c.y, 1.0)
           : vec3(-c.x, -c.y, -1.0);
    vec4 texel = textureLod(samplerCube(source, lookupSampler), normalize(d), block.params.y);
    color = packFloat(yupDecodeRgbm(texel, block.params.z).r);
})",
                                        faceSize * 6, faceSize, [&] (GpuRenderPass& pass)
        {
            pass.setTexture (0, 0, cube);
            pass.setSampler (0, 1, sampler());
            pass.setUniformBuffer (1, 0, block, sizeof (block));
        });

        // Texel solid angles are symmetric across each face, so the row order of the read back does not matter
        double sum = 0.0;
        for (size_t i = 0; i < values.size(); ++i)
        {
            const auto x = static_cast<int> (i) % (faceSize * 6);
            const auto y = static_cast<int> (i) / (faceSize * 6);
            sum += values[i] * texelSolidAngle (x % faceSize, y, faceSize);
        }

        return sum;
    }

    GpuSampler::Ptr sampler()
    {
        if (lookupSampler == nullptr)
        {
            GpuSamplerDesc desc (GpuFilter::linear, GpuWrapMode::clampToEdge);
            desc.mipmapFilter = GpuFilter::linear;
            lookupSampler = GpuSampler::create (device, desc);
        }

        return lookupSampler;
    }

    //==============================================================================
    void checkConstantRadianceStaysConstantAtEveryMip (float tolerance)
    {
        auto cube = makeCubeFrom (constantRadiance, 64);
        ASSERT_NE (cube, nullptr);
        ASSERT_EQ (cube->getMipLevels(), 7u);

        auto directions = testDirections();
        for (const auto& axis : { Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (-1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 1.0f, 0.0f), Vector3<float> (0.0f, -1.0f, 0.0f), Vector3<float> (0.0f, 0.0f, 1.0f), Vector3<float> (0.0f, 0.0f, -1.0f) })
            directions.push_back (axis);

        for (int mip = 0; mip < static_cast<int> (cube->getMipLevels()); ++mip)
        {
            const auto values = sampleCube (cube, directions, static_cast<float> (mip));
            ASSERT_EQ (values.size(), directions.size());

            for (size_t i = 0; i < values.size(); ++i)
            {
                EXPECT_NEAR (values[i].getX(), 0.5f, tolerance) << "mip " << mip << ", direction " << i;
                EXPECT_NEAR (values[i].getY(), 0.25f, tolerance) << "mip " << mip << ", direction " << i;
                EXPECT_NEAR (values[i].getZ(), 1.0f, tolerance) << "mip " << mip << ", direction " << i;
            }
        }
    }

    void checkPrefilterMatchesTheCpuGgxReference (float relativeTolerance)
    {
        auto cube = makeCubeFrom (skyGradient, 64);
        ASSERT_NE (cube, nullptr);

        for (int mip : { 1, 3, 5 })
        {
            const auto roughness = static_cast<float> (mip) / static_cast<float> (cube->getMipLevels() - 1);
            const auto faceSize = cube->getWidth() >> mip;

            // Texel centers, so the comparison sees the filter and not the bilinear lookup between texels
            std::vector<Vector3<float>> directions;
            for (const auto& direction : testDirections())
                directions.push_back (texelCenter (direction, faceSize));

            const auto actual = sampleCube (cube, directions, static_cast<float> (mip));
            ASSERT_EQ (actual.size(), directions.size());

            for (size_t i = 0; i < directions.size(); ++i)
            {
                const auto expected = cpuPrefilteredRadiance (skyGradient, directions[i], roughness);
                const auto allowed = [&] (float e) { return relativeTolerance * jmax (0.05f, e) + 2.0f / 255.0f; };

                EXPECT_NEAR (actual[i].getX(), expected.getX(), allowed (expected.getX())) << "mip " << mip << ", direction " << i;
                EXPECT_NEAR (actual[i].getY(), expected.getY(), allowed (expected.getY())) << "mip " << mip << ", direction " << i;
                EXPECT_NEAR (actual[i].getZ(), expected.getZ(), allowed (expected.getZ())) << "mip " << mip << ", direction " << i;
            }
        }
    }

    void checkOctahedralTileMatchesTheCube (float tolerance)
    {
        // Off the atlas origin on both axes, so a flipped tile origin reads another tile
        constexpr int numMips = 5;
        const Rectangle<int> tile (128, 128, 128, 128);

        auto source = makeSourceCube (skyGradient, 64);
        auto cube = makeCube (64, numMips);
        auto atlas = makeAtlasWithMips (256, 256, numMips);
        ASSERT_NE (source, nullptr);
        ASSERT_NE (cube, nullptr);
        ASSERT_NE (atlas, nullptr);

        // Both filtered from the same source, for the same roughness at every mip
        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (probes->prefilterToCube (frame, source, cube));
        EXPECT_TRUE (probes->prefilterToAtlas (frame, source, atlas, tile, numMips));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        for (int mip : { 0, 2, 4 })
        {
            const auto fromAtlas = sampleAtlas (atlas, tile, testDirections(), static_cast<float> (mip), numMips);
            const auto fromCube = sampleCube (cube, testDirections(), static_cast<float> (mip));
            ASSERT_EQ (fromAtlas.size(), testDirections().size());
            ASSERT_EQ (fromCube.size(), testDirections().size());

            for (size_t i = 0; i < fromAtlas.size(); ++i)
            {
                EXPECT_NEAR (fromAtlas[i].getX(), fromCube[i].getX(), tolerance) << "mip " << mip << ", direction " << i;
                EXPECT_NEAR (fromAtlas[i].getY(), fromCube[i].getY(), tolerance) << "mip " << mip << ", direction " << i;
                EXPECT_NEAR (fromAtlas[i].getZ(), fromCube[i].getZ(), tolerance) << "mip " << mip << ", direction " << i;
            }
        }
    }

    /** Directions on the octahedral edges and corners of a tile, where a lookup is closest to its neighbors. */
    static std::vector<Vector3<float>> tileEdgeDirections()
    {
        std::vector<Vector3<float>> directions { { 1.0f, 0.0f, 0.0f },
                                                 { -1.0f, 0.0f, 0.0f },
                                                 { 0.0f, 1.0f, 0.0f },
                                                 { 0.0f, -1.0f, 0.0f },
                                                 { 0.0f, 0.0f, -1.0f },
                                                 { 0.01f, 0.01f, -1.0f },
                                                 { 0.7f, 0.1f, -0.7f },
                                                 { -0.1f, 0.7f, -0.7f },
                                                 { 0.6f, -0.6f, -0.5f } };

        for (auto& direction : directions)
            direction = direction.normalized();

        for (const auto& direction : testDirections())
            directions.push_back (direction);

        return directions;
    }

    void expectConstantRadiance (const std::vector<Vector3<float>>& values, size_t expectedSize, const String& where)
    {
        ASSERT_EQ (values.size(), expectedSize) << where;

        for (size_t i = 0; i < values.size(); ++i)
        {
            EXPECT_NEAR (values[i].getX(), 0.5f, 2.0e-3f) << where << ", direction " << i;
            EXPECT_NEAR (values[i].getY(), 0.25f, 2.0e-3f) << where << ", direction " << i;
            EXPECT_NEAR (values[i].getZ(), 1.0f, 2.0e-3f) << where << ", direction " << i;
        }
    }

    void checkPrefilterToAtlasKeepsTheOtherTiles()
    {
        // Stacked vertically, so a flipped tile origin writes one tile over the other
        constexpr int numMips = 5;
        const Rectangle<int> first (0, 0, 128, 128);
        const Rectangle<int> second (0, 128, 128, 128);

        auto constant = makeSourceCube (constantRadiance, 32);
        auto sky = makeSourceCube (skyGradient, 32);
        auto atlas = makeAtlasWithMips (256, 256, numMips);
        ASSERT_NE (constant, nullptr);
        ASSERT_NE (sky, nullptr);
        ASSERT_NE (atlas, nullptr);

        for (const auto& [source, tile] : { std::pair { constant, first }, std::pair { sky, second } })
        {
            auto frame = GpuFrame::begin (device);
            EXPECT_TRUE (probes->prefilterToAtlas (frame, source, atlas, tile, numMips));
            frame.submit();
            EXPECT_TRUE (frame.waitForGPU().wasOk());
        }

        for (int mip = 0; mip < numMips; ++mip)
            expectConstantRadiance (sampleAtlas (atlas, first, tileEdgeDirections(), static_cast<float> (mip), numMips), tileEdgeDirections().size(), "mip " + String (mip));
    }

    void checkFractionalLodReadsNoNeighborTile()
    {
        // The center tile of a 3x3 grid, its eight neighbors far from its color
        constexpr int numMips = 5;
        const Rectangle<int> center (128, 128, 128, 128);

        auto constant = makeSourceCube (constantRadiance, 32);
        auto red = makeSourceCube (redRadiance, 32);
        auto atlas = makeAtlasWithMips (384, 384, numMips);
        ASSERT_NE (constant, nullptr);
        ASSERT_NE (red, nullptr);
        ASSERT_NE (atlas, nullptr);

        auto frame = GpuFrame::begin (device);
        for (int row = 0; row < 3; ++row)
        {
            for (int column = 0; column < 3; ++column)
            {
                const Rectangle<int> tile (column * 128, row * 128, 128, 128);
                EXPECT_TRUE (probes->prefilterToAtlas (frame, tile == center ? constant : red, atlas, tile, numMips));
            }
        }
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        for (const auto lod : { 0.5f, 1.25f, 2.5f, 3.5f, 3.9f, 4.0f })
            expectConstantRadiance (sampleAtlas (atlas, center, tileEdgeDirections(), lod, numMips), tileEdgeDirections().size(), "lod " + String (lod));
    }

    void checkPrefilterMipZeroIsACopyOfTheSource()
    {
        auto source = makeSourceCube (brightSpot, 64);
        auto destination = probes->createCube (64);
        ASSERT_NE (source, nullptr);
        ASSERT_NE (destination, nullptr);

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (probes->prefilterToCube (frame, source, destination));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        // Texel centers around the edge of the spot (0.1 radians off +X, at the face center) and elsewhere
        std::vector<Vector3<float>> directions;
        for (const auto angle : { 0.0f, 0.08f, 0.095f, 0.1f, 0.105f, 0.12f })
            for (const auto around : { 0.0f, 1.3f, 2.6f, 3.9f, 5.2f })
                directions.push_back (texelCenter (Vector3<float> (std::cos (angle), std::sin (angle) * std::cos (around), std::sin (angle) * std::sin (around)), 64));

        for (const auto& direction : testDirections())
            directions.push_back (texelCenter (direction, 64));

        const auto copied = sampleCube (destination, directions, 0.0f);
        const auto original = sampleCube (source, directions, 0.0f);
        ASSERT_EQ (copied.size(), directions.size());
        ASSERT_EQ (original.size(), directions.size());

        for (size_t i = 0; i < directions.size(); ++i)
        {
            EXPECT_NEAR (copied[i].getX(), original[i].getX(), 1.0e-3f * original[i].getX() + 1.0e-4f) << "direction " << i;
            EXPECT_NEAR (copied[i].getY(), original[i].getY(), 1.0e-3f * original[i].getY() + 1.0e-4f) << "direction " << i;
            EXPECT_NEAR (copied[i].getZ(), original[i].getZ(), 1.0e-3f * original[i].getZ() + 1.0e-4f) << "direction " << i;
        }
    }

    void checkPrefilteredCubeHasNoSeams()
    {
        auto cube = makeCubeFrom (skyGradient, 64);
        ASSERT_NE (cube, nullptr);

        // Pairs a hundredth apart on either side of the +X/+Z, +X/+Y and -Z/-Y edges
        std::vector<Vector3<float>> directions;
        constexpr auto e = 0.01f;

        for (const auto along : { -0.5f, 0.0f, 0.5f })
        {
            directions.push_back (Vector3<float> (1.0f, along, 1.0f - e).normalized());
            directions.push_back (Vector3<float> (1.0f - e, along, 1.0f).normalized());
            directions.push_back (Vector3<float> (1.0f, 1.0f - e, along).normalized());
            directions.push_back (Vector3<float> (1.0f - e, 1.0f, along).normalized());
            directions.push_back (Vector3<float> (along, -1.0f, -1.0f + e).normalized());
            directions.push_back (Vector3<float> (along, -1.0f + e, -1.0f).normalized());
        }

        for (int mip : { 4, 5 })
        {
            const auto values = sampleCube (cube, directions, static_cast<float> (mip));
            ASSERT_EQ (values.size(), directions.size());

            for (size_t i = 0; i < values.size(); i += 2)
            {
                EXPECT_NEAR (values[i].getX(), values[i + 1].getX(), 3.0f / 255.0f) << "mip " << mip << ", pair " << i / 2;
                EXPECT_NEAR (values[i].getY(), values[i + 1].getY(), 3.0f / 255.0f) << "mip " << mip << ", pair " << i / 2;
                EXPECT_NEAR (values[i].getZ(), values[i + 1].getZ(), 3.0f / 255.0f) << "mip " << mip << ", pair " << i / 2;
            }
        }
    }

    //==============================================================================
    /** The four channels of the texels of one row of @p texture, from @p x, @p count of them, read exactly. */
    std::vector<std::array<float, 4>> fetchRow (const GpuTexture::Ptr& texture, int x, int y, int count)
    {
        struct FetchBlock
        {
            int area[4] {};
        } block;

        block.area[0] = x;
        block.area[1] = y;

        const auto values = readFloats (R"(
layout(set = 0, binding = 1) uniform sampler lookupSampler;
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 1, binding = 0) uniform FetchBlock { ivec4 area; } block;
layout(location = 0) out vec4 color;
void main()
{
    int x = int(gl_FragCoord.x);
    int texel = x / 4;
    vec4 value = texelFetch(sampler2D(source, lookupSampler), block.area.xy + ivec2(texel, 0), 0);
    color = packFloat(value[x - texel * 4]);
})",
                                        count * 4, 1, [&] (GpuRenderPass& pass)
        {
            pass.setTexture (0, 0, texture);
            pass.setSampler (0, 1, sampler());
            pass.setUniformBuffer (1, 0, &block, sizeof (block));
        });

        std::vector<std::array<float, 4>> texels;
        for (size_t i = 0; i + 3 < values.size(); i += 4)
            texels.push_back ({ values[i], values[i + 1], values[i + 2], values[i + 3] });

        return texels;
    }

    /** The four irradiance coefficients projectIrradiance() wrote in the column at @p texel of
        @p atlas, decoded from RGBM and ratios when the atlas holds them. */
    std::array<Vector3<float>, 4> readIrradiance (const GpuTexture::Ptr& atlas, Point<int> texel)
    {
        std::array<std::array<float, 4>, 4> raw {};
        for (int c = 0; c < 4; ++c)
        {
            const auto row = fetchRow (atlas, texel.getX(), texel.getY() + c, 1);
            if (row.size() == 1)
                raw[static_cast<size_t> (c)] = row[0];
        }

        std::array<Vector3<float>, 4> coefficients;
        const auto rgbm = atlas->getFormat() == GpuTextureFormat::rgba8unorm;
        const auto multiplier = rgbm ? raw[0][3] * 8.0f : 1.0f;
        coefficients[0] = Vector3<float> (raw[0][0], raw[0][1], raw[0][2]) * multiplier;

        for (size_t c = 1; c < 4; ++c)
        {
            const auto& t = raw[c];
            coefficients[c] = rgbm ? Vector3<float> (SphericalHarmonics::decodeRatio (t[0], coefficients[0].getX()),
                                                     SphericalHarmonics::decodeRatio (t[1], coefficients[0].getY()),
                                                     SphericalHarmonics::decodeRatio (t[2], coefficients[0].getZ()))
                                   : Vector3<float> (t[0], t[1], t[2]);
        }

        return coefficients;
    }

    /** Projects @p radiance through a 32 texel cube into an atlas, and compares the four
        coefficients with the CPU projection of the function, within @p tolerance of coefficient 0. */
    void checkIrradianceMatchesTheCpuProjection (const RadianceFunction& radiance, float tolerance)
    {
        auto cube = makeSourceCube (radiance, 32);
        GpuTextureDesc desc (8, 8, probes->getRadianceFormat(), true);
        auto atlas = GpuTexture::create (device, desc);
        ASSERT_NE (cube, nullptr);
        ASSERT_NE (atlas, nullptr);

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (probes->projectIrradiance (frame, cube, atlas, { 3, 2 }));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        const auto expected = SphericalHarmonics::toIrradiance (SphericalHarmonics::project (radiance, 256));
        const auto actual = readIrradiance (atlas, { 3, 2 });

        for (size_t c = 0; c < 4; ++c)
        {
            RecordProperty (("coefficient" + String ((int) c)).toStdString(), (String (actual[c].getX()) + " " + String (actual[c].getY()) + " " + String (actual[c].getZ())).toStdString());

            EXPECT_NEAR (actual[c].getX(), expected[c].getX(), tolerance * expected[0].getX()) << "coefficient " << c;
            EXPECT_NEAR (actual[c].getY(), expected[c].getY(), tolerance * expected[0].getY()) << "coefficient " << c;
            EXPECT_NEAR (actual[c].getZ(), expected[c].getZ(), tolerance * expected[0].getZ()) << "coefficient " << c;
        }
    }

    /** A distance cube in ProbeRenderer::visibilityFormat holding 2 + d.x + 0.25 d.y along each direction d. */
    GpuTexture::Ptr makeDistanceCube (int faceSize)
    {
        const auto root = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory();
        const auto shaders = root.getChildFile ("modules/yup_renderer/shaders");
        const auto source = String ("#version 450\n") + shaders.getChildFile ("lib/cube.glsl").loadFileAsString() + R"(
layout(set = 1, binding = 0) uniform FaceBlock { vec4 params; } block;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    vec3 d = cubeDirection(int(block.params.x), uv);
    float distance = 2.0 + d.x + 0.25 * d.y;
    color = vec4(distance, distance * distance, 0.0, 1.0);
})";

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
        MemoryBlock block;
        if (! bundle.wasOk() || ! bundle.getReference().saveToMemoryBlock (block).wasOk())
        {
            ADD_FAILURE() << bundle.getErrorMessage();
            return nullptr;
        }

        GpuTextureDesc desc (static_cast<uint32> (faceSize), static_cast<uint32> (faceSize), ProbeRenderer::visibilityFormat, true);
        desc.type = GpuTextureType::cube;
        desc.depthOrArrayLayers = 6;
        auto cube = GpuTexture::create (device, desc);

        FullscreenPass pass;
        if (cube == nullptr || pass.prepare (device, static_cast<const uint8*> (block.getData()), block.getSize(), ProbeRenderer::visibilityFormat).failed())
        {
            ADD_FAILURE() << "Unable to create the distance cube";
            return nullptr;
        }

        auto frame = GpuFrame::begin (device);
        for (int face = 0; face < 6; ++face)
        {
            const float params[4] { static_cast<float> (face), 0.0f, 0.0f, 0.0f };
            EXPECT_TRUE (pass.draw (frame, cube, GpuTextureViewDesc (0, static_cast<uint32> (face)), [&] (GpuRenderPass& renderPass)
            {
                renderPass.setUniformBuffer (1, 0, params, sizeof (params));
            }, GpuColor::transparentBlack()));
        }

        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());
        return cube;
    }

    /** Renders the visibility of makeDistanceCube() into a 16 texel tile at (2, 3) of an atlas,
        scaled by 0.5, and returns the 18 rows of the bordered tile. */
    std::vector<std::vector<std::array<float, 4>>> renderVisibilityTile()
    {
        auto cube = makeDistanceCube (32);
        GpuTextureDesc desc (24, 24, ProbeRenderer::visibilityFormat, true);
        auto atlas = GpuTexture::create (device, desc);
        if (cube == nullptr || atlas == nullptr)
        {
            ADD_FAILURE() << "Unable to create the visibility textures";
            return {};
        }

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (probes->renderVisibility (frame, cube, atlas, { 2, 3 }, 16, 0.5f));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        std::vector<std::vector<std::array<float, 4>>> rows;
        for (int y = 0; y < 18; ++y)
            rows.push_back (fetchRow (atlas, 2, 3 + y, 18));

        return rows;
    }

    void checkVisibilityHoldsTheMomentsOfTheDistance()
    {
        ASSERT_TRUE (probes->canRenderVisibility());
        const auto rows = renderVisibilityTile();
        ASSERT_EQ (rows.size(), 18u);

        // The mean of a linear function under a cos^50 lobe around n is the function at n scaled
        // by the lobe's mean cosine, 51 / 52, along its gradient
        constexpr auto meanCosine = 51.0f / 52.0f;
        auto spread = 0.0f;

        for (int j = 0; j < 16; ++j)
        {
            ASSERT_EQ (rows[static_cast<size_t> (j + 1)].size(), 18u);

            for (int i = 0; i < 16; ++i)
            {
                const auto n = GpuSceneData::decodeOctahedral ({ (static_cast<float> (i) + 0.5f) / 8.0f - 1.0f, (static_cast<float> (j) + 0.5f) / 8.0f - 1.0f });
                const auto expected = 0.5f * (2.0f + meanCosine * (n.getX() + 0.25f * n.getY()));
                const auto& texel = rows[static_cast<size_t> (j + 1)][static_cast<size_t> (i + 1)];

                EXPECT_NEAR (texel[0], expected, 0.01f) << "texel " << i << ", " << j;
                EXPECT_GE (texel[1], texel[0] * texel[0] - 0.002f) << "texel " << i << ", " << j;
                EXPECT_LT (texel[1] - texel[0] * texel[0], 0.01f) << "texel " << i << ", " << j;
                spread = jmax (spread, std::abs (texel[0] - 1.0f));
            }
        }

        // Not vacuous: the distance changes across the tile
        EXPECT_GT (spread, 0.4f);
    }

    void checkVisibilityBorderMirrorsTheTileEdges()
    {
        const auto rows = renderVisibilityTile();
        ASSERT_EQ (rows.size(), 18u);

        const auto at = [&rows] (int x, int y) { return rows[static_cast<size_t> (y)][static_cast<size_t> (x)]; };

        for (int k = 1; k <= 16; ++k)
        {
            EXPECT_EQ (at (k, 0), at (17 - k, 1)) << "top " << k;
            EXPECT_EQ (at (k, 17), at (17 - k, 16)) << "bottom " << k;
            EXPECT_EQ (at (0, k), at (1, 17 - k)) << "left " << k;
            EXPECT_EQ (at (17, k), at (16, 17 - k)) << "right " << k;
        }

        EXPECT_EQ (at (0, 0), at (16, 16));
        EXPECT_EQ (at (17, 0), at (1, 16));
        EXPECT_EQ (at (0, 17), at (16, 1));
        EXPECT_EQ (at (17, 17), at (1, 1));

        // Not vacuous: the mirrored texels differ from their neighbors inside
        EXPECT_NE (at (1, 0), at (1, 1));
    }

    static inline const String packingFunctions = R"(
vec4 packFloat(float v)
{
    uvec4 bytes = (uvec4(floatBitsToUint(v)) >> uvec4(0u, 8u, 16u, 24u)) & uvec4(255u);
    return vec4(bytes) / 255.0;
}
)";

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;
    std::unique_ptr<ProbeRenderer> probes;
    GpuSampler::Ptr lookupSampler;
    std::map<String, MemoryBlock> compiledBundles;
};
#endif

#if YUP_APPLE && YUP_ENABLE_SHADER_TRANSPILER
class ProbeRendererMetalTests : public ProbeRendererGpuTests
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

TEST_F (ProbeRendererMetalTests, RadianceIsHalfFloatWhenRenderable)
{
    EXPECT_EQ (probes->getRadianceFormat(), GpuTextureFormat::rgba16float);

    auto cube = probes->createCube (64);
    ASSERT_NE (cube, nullptr);
    EXPECT_EQ (cube->getType(), GpuTextureType::cube);
    EXPECT_EQ (cube->getFormat(), GpuTextureFormat::rgba16float);
    EXPECT_EQ (cube->getWidth(), 64);
    EXPECT_EQ (cube->getDepthOrArrayLayers(), 6u);
    EXPECT_EQ (cube->getMipLevels(), 7u);

    probes->setForceRgbmForTesting (true);
    EXPECT_EQ (probes->getRadianceFormat(), GpuTextureFormat::rgba8unorm);
    auto rgbmCube = probes->createCube (8);
    ASSERT_NE (rgbmCube, nullptr);
    EXPECT_EQ (rgbmCube->getFormat(), GpuTextureFormat::rgba8unorm);

    EXPECT_EQ (probes->createCube (0), nullptr);

    // A shorter mip chain on request, never a longer one
    probes->setForceRgbmForTesting (false);
    auto shortCube = probes->createCube (64, 4);
    ASSERT_NE (shortCube, nullptr);
    EXPECT_EQ (shortCube->getMipLevels(), 4u);
    EXPECT_EQ (probes->createCube (8, 5), nullptr);
}

TEST_F (ProbeRendererMetalTests, RejectsMissingOrWrongShapedTextures)
{
    auto cube = probes->createCube (16);
    auto other = probes->createCube (16);
    auto plane = makeAtlasWithMips (64, 64, 3);
    auto atlas = makeAtlasWithMips (256, 128, 5);
    ASSERT_NE (cube, nullptr);
    ASSERT_NE (other, nullptr);
    ASSERT_NE (plane, nullptr);
    ASSERT_NE (atlas, nullptr);

    GpuTextureDesc notTargetDesc (16, 16, GpuTextureFormat::rgba16float);
    notTargetDesc.type = GpuTextureType::cube;
    notTargetDesc.depthOrArrayLayers = 6;
    auto notTarget = GpuTexture::create (device, notTargetDesc);
    ASSERT_NE (notTarget, nullptr);

    auto wrongFormat = GpuTexture::create (device, [] { GpuTextureDesc d (16, 16, GpuTextureFormat::rgba8unorm, true); d.type = GpuTextureType::cube; d.depthOrArrayLayers = 6; return d; }());
    ASSERT_NE (wrongFormat, nullptr);

    auto frame = GpuFrame::begin (device);

    EXPECT_FALSE (probes->equirectToCube (frame, nullptr, cube));
    EXPECT_FALSE (probes->equirectToCube (frame, plane, nullptr));
    EXPECT_FALSE (probes->equirectToCube (frame, cube, other));
    EXPECT_FALSE (probes->equirectToCube (frame, plane, plane));
    EXPECT_FALSE (probes->equirectToCube (frame, plane, notTarget));
    EXPECT_FALSE (probes->equirectToCube (frame, plane, wrongFormat));

    EXPECT_FALSE (probes->downsampleCube (frame, nullptr));
    EXPECT_FALSE (probes->downsampleCube (frame, plane));
    EXPECT_FALSE (probes->downsampleCube (frame, notTarget));

    EXPECT_FALSE (probes->prefilterToCube (frame, nullptr, cube));
    EXPECT_FALSE (probes->prefilterToCube (frame, cube, nullptr));
    EXPECT_FALSE (probes->prefilterToCube (frame, cube, cube));
    EXPECT_FALSE (probes->prefilterToCube (frame, plane, cube));
    EXPECT_FALSE (probes->prefilterToCube (frame, cube, plane));

    EXPECT_FALSE (probes->prefilterToAtlas (frame, nullptr, atlas, { 0, 0, 128, 128 }, 5));
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, nullptr, { 0, 0, 128, 128 }, 5));
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, cube, { 0, 0, 16, 16 }, 1));
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 0, 0, 128, 128 }, 6));   // more mips than the atlas
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 0, 0, 128, 128 }, 0));   // no mip
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 192, 0, 128, 128 }, 5)); // outside
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 8, 0, 128, 128 }, 5));   // not aligned
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 0, 0, 128, 64 }, 5));    // not square
    EXPECT_FALSE (probes->prefilterToAtlas (frame, cube, atlas, { 0, 0, 16, 16 }, 5));     // 1 texel at the last mip

    frame.submit();
    EXPECT_TRUE (frame.waitForGPU().wasOk());
}

TEST_F (ProbeRendererMetalTests, ConstantRadianceStaysConstantAtEveryMip)
{
    checkConstantRadianceStaysConstantAtEveryMip (2.0e-3f);
}

TEST_F (ProbeRendererMetalTests, RgbmConstantRadianceStaysConstantAtEveryMip)
{
    probes->setForceRgbmForTesting (true);
    checkConstantRadianceStaysConstantAtEveryMip (0.02f);
}

TEST_F (ProbeRendererMetalTests, DownsampleKeepsTheEnergyOfABrightSpot)
{
    auto cube = makeSourceCube (brightSpot, 64);
    ASSERT_NE (cube, nullptr);

    const auto total = integrateRed (cube, 0);
    EXPECT_GT (total, 3.0); // the spot alone holds about 3.1

    for (int mip = 1; mip < static_cast<int> (cube->getMipLevels()); ++mip)
        EXPECT_NEAR (integrateRed (cube, mip) / total, 1.0, 0.01) << "mip " << mip;
}

TEST_F (ProbeRendererMetalTests, PrefilterKeepsTheEnergyOfABrightSpot)
{
    auto cube = makeCubeFrom (brightSpot, 64);
    ASSERT_NE (cube, nullptr);

    const auto total = integrateRed (cube, 0);
    EXPECT_GT (total, 3.0);

    for (int mip = 1; mip < static_cast<int> (cube->getMipLevels()); ++mip)
        EXPECT_NEAR (integrateRed (cube, mip) / total, 1.0, 0.08) << "mip " << mip;
}

TEST_F (ProbeRendererMetalTests, PrefilterSpreadsABrightSpot)
{
    auto cube = makeCubeFrom (brightSpot, 64);
    ASSERT_NE (cube, nullptr);

    // Faces of two texels have none near +X, so a lookup there averages texels far from the spot
    // (a single texel faces it again): the peak is followed down to faces of four texels
    auto previousPeak = sampleRed (cube, { 1.0f, 0.0f, 0.0f }, 0);

    for (int mip = 1; (cube->getWidth() >> mip) >= 4; ++mip)
    {
        const auto peak = sampleRed (cube, { 1.0f, 0.0f, 0.0f }, mip);
        EXPECT_LT (peak, previousPeak) << "mip " << mip;
        previousPeak = peak;
    }

    // The long GGX tail reaches 20 degrees off the spot, three texels of mip 2 away, where the
    // copy and a plain mip chain still read the 0.1 background
    const auto offAxis = Vector3<float> (std::cos (0.35f), std::sin (0.35f), 0.0f);
    EXPECT_LT (sampleRed (cube, offAxis, 0), 0.2f);
    EXPECT_GT (sampleRed (cube, offAxis, 2), 0.5f);

    // The spot reaches the copy at full strength and the roughest mip much dimmer
    EXPECT_GT (sampleRed (cube, { 1.0f, 0.0f, 0.0f }, 0), 50.0f);
    EXPECT_LT (sampleRed (cube, { 1.0f, 0.0f, 0.0f }, static_cast<int> (cube->getMipLevels()) - 1), 5.0f);
}

TEST_F (ProbeRendererMetalTests, PrefilterMatchesTheCpuGgxReference)
{
    checkPrefilterMatchesTheCpuGgxReference (0.05f);
}

TEST_F (ProbeRendererMetalTests, RgbmPrefilterMatchesTheCpuGgxReference)
{
    probes->setForceRgbmForTesting (true);
    checkPrefilterMatchesTheCpuGgxReference (0.08f);
}

TEST_F (ProbeRendererMetalTests, OctahedralTileMatchesTheCube)
{
    checkOctahedralTileMatchesTheCube (4.0f / 255.0f);
}

TEST_F (ProbeRendererMetalTests, PrefilterToAtlasKeepsTheOtherTiles)
{
    checkPrefilterToAtlasKeepsTheOtherTiles();
}

TEST_F (ProbeRendererMetalTests, FractionalLodReadsNoNeighborTile)
{
    checkFractionalLodReadsNoNeighborTile();
}

TEST_F (ProbeRendererMetalTests, PrefilterMipZeroIsACopyOfTheSource)
{
    checkPrefilterMipZeroIsACopyOfTheSource();
}

TEST_F (ProbeRendererMetalTests, PrefilteredCubeHasNoSeams)
{
    checkPrefilteredCubeHasNoSeams();
}

TEST_F (ProbeRendererMetalTests, IrradianceOfAConstantCubeIsItsRadiance)
{
    // Over a constant radiance the first band is the radiance itself, the second is zero
    checkIrradianceMatchesTheCpuProjection (constantRadiance, 0.01f);
}

TEST_F (ProbeRendererMetalTests, IrradianceMatchesTheCpuProjection)
{
    checkIrradianceMatchesTheCpuProjection (skyGradient, 0.01f);
}

TEST_F (ProbeRendererMetalTests, RgbmIrradianceMatchesTheCpuProjection)
{
    probes->setForceRgbmForTesting (true);
    ASSERT_EQ (probes->getRadianceFormat(), GpuTextureFormat::rgba8unorm);
    checkIrradianceMatchesTheCpuProjection (skyGradient, 0.03f);
}

TEST_F (ProbeRendererMetalTests, IrradianceRejectsMisplacedTexels)
{
    auto cube = probes->createCube (32);
    GpuTextureDesc desc (8, 8, probes->getRadianceFormat(), true);
    auto atlas = GpuTexture::create (device, desc);
    ASSERT_NE (cube, nullptr);
    ASSERT_NE (atlas, nullptr);

    auto frame = GpuFrame::begin (device);
    EXPECT_FALSE (probes->projectIrradiance (frame, cube, atlas, { 0, 5 })); // Four rows from 5 leave the atlas
    EXPECT_FALSE (probes->projectIrradiance (frame, cube, atlas, { -1, 0 }));
    EXPECT_FALSE (probes->projectIrradiance (frame, nullptr, atlas, { 0, 0 }));
    EXPECT_FALSE (probes->projectIrradiance (frame, cube, nullptr, { 0, 0 }));
    frame.submit();
}

TEST_F (ProbeRendererMetalTests, VisibilityHoldsTheMomentsOfTheDistance)
{
    checkVisibilityHoldsTheMomentsOfTheDistance();
}

TEST_F (ProbeRendererMetalTests, VisibilityBorderMirrorsTheTileEdges)
{
    checkVisibilityBorderMirrorsTheTileEdges();
}

TEST_F (ProbeRendererMetalTests, VisibilityRejectsMisplacedTiles)
{
    auto cube = makeDistanceCube (8);
    GpuTextureDesc desc (24, 24, ProbeRenderer::visibilityFormat, true);
    auto atlas = GpuTexture::create (device, desc);
    ASSERT_NE (cube, nullptr);
    ASSERT_NE (atlas, nullptr);

    auto frame = GpuFrame::begin (device);
    EXPECT_FALSE (probes->renderVisibility (frame, cube, atlas, { 8, 0 }, 16, 1.0f)); // 18 texels from 8 leave the atlas
    EXPECT_FALSE (probes->renderVisibility (frame, cube, atlas, { 0, 0 }, 1, 1.0f));
    EXPECT_FALSE (probes->renderVisibility (frame, nullptr, atlas, { 0, 0 }, 16, 1.0f));
    frame.submit();
}

#endif

#if YUP_LINUX && YUP_ENABLE_SHADER_TRANSPILER
class ProbeRendererGLTests : public ProbeRendererGpuTests
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

        window = SDL_CreateWindow ("yup_probe_renderer_gl_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
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
        lookupSampler = nullptr;
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

TEST_F (ProbeRendererGLTests, ConstantRadianceStaysConstantAtEveryMip)
{
    checkConstantRadianceStaysConstantAtEveryMip (2.0e-3f);
}

TEST_F (ProbeRendererGLTests, PrefilterMatchesTheCpuGgxReference)
{
    checkPrefilterMatchesTheCpuGgxReference (0.05f);
}

TEST_F (ProbeRendererGLTests, OctahedralTileMatchesTheCube)
{
    checkOctahedralTileMatchesTheCube (4.0f / 255.0f);
}

TEST_F (ProbeRendererGLTests, PrefilterToAtlasKeepsTheOtherTiles)
{
    checkPrefilterToAtlasKeepsTheOtherTiles();
}

TEST_F (ProbeRendererGLTests, FractionalLodReadsNoNeighborTile)
{
    checkFractionalLodReadsNoNeighborTile();
}

TEST_F (ProbeRendererGLTests, PrefilterMipZeroIsACopyOfTheSource)
{
    checkPrefilterMipZeroIsACopyOfTheSource();
}

TEST_F (ProbeRendererGLTests, PrefilteredCubeHasNoSeams)
{
    checkPrefilteredCubeHasNoSeams();
}

TEST_F (ProbeRendererGLTests, IrradianceMatchesTheCpuProjection)
{
    checkIrradianceMatchesTheCpuProjection (skyGradient, 0.01f);
}

TEST_F (ProbeRendererGLTests, VisibilityHoldsTheMomentsOfTheDistance)
{
    checkVisibilityHoldsTheMomentsOfTheDistance();
}

TEST_F (ProbeRendererGLTests, VisibilityBorderMirrorsTheTileEdges)
{
    checkVisibilityBorderMirrorsTheTileEdges();
}
#endif

//==============================================================================
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
        const auto shaders = root.getChildFile ("modules/yup_renderer/shaders");

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
        const auto build = renderer.buildEnvironmentCube (environment);
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
        const auto build = probes->buildEnvironmentCube (environment);
        ASSERT_NE (build.cube, nullptr);
        ASSERT_NE (build.image, nullptr);
        EXPECT_EQ (build.image->getWidth(), 64);
        EXPECT_GT (build.image->getMipLevels(), 1u);
        EXPECT_TRUE (build.status.wait().wasOk());

        // The RGBM image has no mips to generate, so nothing is left to keep
        ProbeRenderer rgbmProbes (device);
        rgbmProbes.setForceRgbmForTesting (true);
        const auto rgbmBuild = rgbmProbes.buildEnvironmentCube (environment);
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
        const auto build = probes->buildEnvironmentCube (environment);
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
