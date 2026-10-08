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

#include <algorithm>
#include <array>
#include <bit>
#include <map>
#include <optional>
#include <set>

using namespace yup;

class SceneRendererTests : public ::testing::Test
{
};

TEST_F (SceneRendererTests, UsesFourSamplesByDefault)
{
    const SceneRenderer renderer;

    EXPECT_EQ (renderer.getSampleCount(), 4);
    EXPECT_TRUE (renderer.getLastError().isEmpty());
}

TEST_F (SceneRendererTests, SampleCountIsAtLeastOne)
{
    SceneRenderer renderer;

    renderer.setSampleCount (8);
    EXPECT_EQ (renderer.getSampleCount(), 8);

    renderer.setSampleCount (0);
    EXPECT_EQ (renderer.getSampleCount(), 1);
}

TEST_F (SceneRendererTests, RenderWithoutDeviceFails)
{
    SceneRenderer renderer;
    Scene scene;

    EXPECT_EQ (renderer.render (nullptr, scene, 64, 64), nullptr);
    EXPECT_FALSE (renderer.getLastError().isEmpty());
}

/** CPU only: mirrors the AgX tone mapping of lib/color.glsl. */
class SceneRendererAgxMirrorTests : public ::testing::Test
{
public:
    /** Mirrors yupToneMapAgx and the sRGB encoding of lib/color.glsl, returning 8 bit sRGB levels. */
    static std::array<int, 3> toneMapAgx (const std::array<float, 3>& color)
    {
        static constexpr float inset[3][3] = { { 0.83947207f, 0.03456180f, 0.12596613f },
                                               { 0.02293524f, 0.94392547f, 0.03313929f },
                                               { 0.04314896f, 0.05587855f, 0.90097249f } };
        static constexpr float outset[3][3] = { { 1.12753300f, -0.00675187f, -0.12078112f },
                                                { -0.00651416f, 1.00599316f, 0.00052100f },
                                                { -0.03292859f, -0.03460768f, 1.06753627f } };
        static constexpr float polynomial[] = { -0.0046820379f, 0.66635377f, -2.4249506f, 16.346045f, -28.204515f, 19.31242f, -4.6886051f };

        std::array<float, 3> linear {};
        for (int i = 0; i < 3; ++i)
        {
            const auto shaped = inset[i][0] * color[0] + inset[i][1] * color[1] + inset[i][2] * color[2];
            const auto x = std::clamp ((std::log2 (std::max (shaped, 1.0e-10f) / 0.18f) + 5.0f) / 11.0f, 0.0f, 1.0f);

            auto y = 0.0f;
            for (int k = 6; k >= 0; --k)
                y = y * x + polynomial[k];

            y = std::clamp (y, 0.0f, 1.0f);
            linear[i] = y < 0.04045f ? y / 12.92f : std::pow ((y + 0.055f) / 1.055f, 2.4f);
        }

        std::array<int, 3> levels {};
        for (int i = 0; i < 3; ++i)
        {
            const auto c = std::clamp (outset[i][0] * linear[0] + outset[i][1] * linear[1] + outset[i][2] * linear[2], 0.0f, 1.0f);
            const auto encoded = c < 0.0031308f ? c * 12.92f : 1.055f * std::pow (c, 1.0f / 2.4f) - 0.055f;
            levels[i] = static_cast<int> (std::round (encoded * 255.0f));
        }

        return levels;
    }
};

TEST_F (SceneRendererAgxMirrorTests, MirrorMatchesFitScriptOnSaturatedRed)
{
    // Expected levels are agx() of tools/agx_fit.py for (1, 0, 0): 0.59121, 0.00821, 0.01525 display linear.
    // Grey inputs cannot catch a transposed inset or outset matrix, a saturated primary does.
    const auto levels = toneMapAgx ({ 1.0f, 0.0f, 0.0f });
    EXPECT_NEAR (levels[0], 202, 1);
    EXPECT_NEAR (levels[1], 22, 1);
    EXPECT_NEAR (levels[2], 33, 1);
}

/** CPU only: the reflection of the precompiled forward bundles. */
class SceneRendererBundleTests : public ::testing::Test
{
protected:
    struct Budget
    {
        int sampledImages = 0;
        int uniformBlocks = 0;
        std::map<uint32_t, int> texturesPerSet;
        std::map<uint32_t, int> uniformBlocksPerSet;
    };

    static ShaderBundle load (const uint8* data, size_t size)
    {
        auto bundle = ShaderBundle::loadFromData (data, size);
        EXPECT_TRUE (bundle.wasOk()) << bundle.getErrorMessage();
        return bundle.wasOk() ? std::move (bundle.getReference()) : ShaderBundle();
    }

    static ShaderBundle forwardPass()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle forwardPassFull()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_ForwardPass_full.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle background()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_Background.inc>
        };
        return load (data, sizeof (data));
    }

    static ShaderBundle backgroundFull()
    {
        static constexpr uint8 data[] = {
#include <yup_3d/shaders/yup_Background_full.inc>
        };
        return load (data, sizeof (data));
    }

    static Budget countResources (const ShaderReflection& reflection)
    {
        Budget budget;

        for (const auto* images : { &reflection.sampledImages, &reflection.separateImages })
        {
            for (const auto& image : *images)
            {
                ++budget.sampledImages;
                ++budget.texturesPerSet[image.set];
            }
        }

        for (const auto& block : reflection.uniformBuffers)
        {
            ++budget.uniformBlocks;
            ++budget.uniformBlocksPerSet[block.set];
        }

        return budget;
    }

    static bool writesLocation (const ShaderReflection& reflection, uint32_t location)
    {
        return std::any_of (reflection.stageOutputs.begin(), reflection.stageOutputs.end(), [location] (const auto& output)
        {
            return output.location == location;
        });
    }
};

TEST_F (SceneRendererBundleTests, RasterBundlesStayWithinBudget)
{
    // The reflection counts what the shaders declare, so the budget check cannot pass on empty lists
    {
        const auto bundle = forwardPass();
        const auto* vertex = bundle.findShader (ShaderStage::vertex, ShaderLanguage::spirv);
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (vertex, nullptr);
        ASSERT_NE (fragment, nullptr);

        EXPECT_EQ (countResources (vertex->reflection).sampledImages, 0);
        EXPECT_EQ (countResources (vertex->reflection).uniformBlocks, 2);
        EXPECT_EQ (countResources (fragment->reflection).sampledImages, 7);
        EXPECT_EQ (countResources (fragment->reflection).uniformBlocks, 3);
    }

    const std::pair<const char*, ShaderBundle> bundles[] = {
        { "ForwardPass", forwardPass() },
        { "Background", background() },
    };

    for (const auto& [name, bundle] : bundles)
    {
        for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
        {
            const auto* shader = bundle.findShader (stage, ShaderLanguage::spirv);
            ASSERT_NE (shader, nullptr) << name << " " << toString (stage);

            const auto budget = countResources (shader->reflection);
            EXPECT_LE (budget.sampledImages, 16) << name << " " << toString (stage);
            EXPECT_LE (budget.uniformBlocks, 12) << name << " " << toString (stage);

            for (const auto& [set, count] : budget.texturesPerSet)
                EXPECT_LE (count, 8) << name << " " << toString (stage) << " set " << (int) set;

            for (const auto& [set, count] : budget.uniformBlocksPerSet)
                EXPECT_LE (count, 8) << name << " " << toString (stage) << " set " << (int) set;
        }
    }
}

TEST_F (SceneRendererBundleTests, OnlyFullVariantsWriteVelocity)
{
    const auto fragmentOf = [] (const ShaderBundle& bundle) -> const ShaderReflection*
    {
        const auto* shader = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        return shader != nullptr ? &shader->reflection : nullptr;
    };

    const auto raster = forwardPass();
    const auto full = forwardPassFull();
    const auto rasterBackground = background();
    const auto fullBackground = backgroundFull();

    for (const auto* reflection : { fragmentOf (raster), fragmentOf (rasterBackground) })
    {
        ASSERT_NE (reflection, nullptr);
        EXPECT_TRUE (writesLocation (*reflection, 0));
        EXPECT_FALSE (writesLocation (*reflection, 1));
    }

    for (const auto* reflection : { fragmentOf (full), fragmentOf (fullBackground) })
    {
        ASSERT_NE (reflection, nullptr);
        EXPECT_TRUE (writesLocation (*reflection, 0));
        EXPECT_TRUE (writesLocation (*reflection, 1));
    }
}

TEST_F (SceneRendererBundleTests, MaterialBlockEndsWithTheGpuMaterial)
{
    const ShaderBundle bundles[] = { forwardPass(), forwardPassFull() };

    for (const auto& bundle : bundles)
    {
        const auto* fragment = bundle.findShader (ShaderStage::fragment, ShaderLanguage::spirv);
        ASSERT_NE (fragment, nullptr);

        const auto& blocks = fragment->reflection.uniformBuffers;
        const auto block = std::find_if (blocks.begin(), blocks.end(), [] (const auto& b) { return b.set == 2 && b.binding == 1; });
        ASSERT_NE (block, blocks.end());
        EXPECT_EQ (block->blockSize, 96u);

        std::map<String, uint32_t> offsets;
        for (const auto& member : block->members)
            offsets[member.name] = member.offset;

        EXPECT_EQ (offsets["specular"], 64u);
        EXPECT_EQ (offsets["extended"], 64u + static_cast<uint32_t> (offsetof (GpuMaterial, extended)));
    }
}

#if YUP_APPLE || YUP_LINUX
class SceneRendererBaselineScenes : public ::testing::Test
{
protected:
    /** A plate facing the default camera, with a smaller one floating over it to cast a shadow. */
    static EntityNode::Ptr makePlates()
    {
        const auto quad = [] (float halfSize, float z)
        {
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = { -halfSize, -halfSize, z };
            vertices[1].position = { halfSize, -halfSize, z };
            vertices[2].position = { halfSize, halfSize, z };
            vertices[3].position = { -halfSize, halfSize, z };

            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            return vertices;
        };

        auto mesh = Mesh::Ptr (new Mesh ("plates"));
        mesh->addPrimitive (quad (1.0f, 0.0f), { 0, 1, 2, 0, 2, 3 });
        mesh->addPrimitive (quad (0.3f, 0.5f), { 0, 1, 2, 0, 2, 3 });

        auto entity = EntityNode::Ptr (new EntityNode ("plates"));
        entity->attach<MeshNode> (mesh);
        return entity;
    }

    /** Sun shadow over the environment: the existing plates scene with ACES. */
    static void buildShadowsScene (Scene& scene)
    {
        scene.getRoot()->addChild (makePlates());

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().castsShadows = true;
        sun->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));
        scene.getRoot()->addChild (sun);

        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.setEnvironmentVisible (true);
        scene.setToneMapping (Scene::ToneMapping::aces);
    }

    /** Emissive, alpha-masked and blended quads over a white plate, lit by eight colored point lights. */
    static void buildMaterialsScene (Scene& scene)
    {
        const auto quadAt = [] (float x, float halfSize, float z, Material::Ptr material, float leftAlpha = 1.0f, float rightAlpha = 1.0f)
        {
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = { x - halfSize, -halfSize, z };
            vertices[1].position = { x + halfSize, -halfSize, z };
            vertices[2].position = { x + halfSize, halfSize, z };
            vertices[3].position = { x - halfSize, halfSize, z };

            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            vertices[0].color[3] = vertices[3].color[3] = leftAlpha;
            vertices[1].color[3] = vertices[2].color[3] = rightAlpha;

            auto mesh = Mesh::Ptr (new Mesh ("quad"));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

            auto entity = EntityNode::Ptr (new EntityNode ("quad"));
            entity->attach<MeshNode> (mesh);
            return entity;
        };

        auto plate = Material::Ptr (new Material());
        plate->metallicFactor = 0.0f;
        plate->roughnessFactor = 0.4f;

        auto emissive = Material::Ptr (new Material());
        emissive->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        emissive->emissiveFactor = { 1.0f, 0.5f, 0.1f };

        // Alpha runs from 0.2 to 0.8 across the quad, so the cutoff discards its left half
        auto masked = Material::Ptr (new Material());
        masked->baseColorFactor = { 0.2f, 0.8f, 0.2f, 1.0f };
        masked->alphaMode = Material::AlphaMode::mask;

        auto blended = Material::Ptr (new Material());
        blended->baseColorFactor = { 0.2f, 0.2f, 0.9f, 0.5f };
        blended->alphaMode = Material::AlphaMode::blend;

        scene.getRoot()->addChild (quadAt (0.0f, 1.2f, -0.3f, plate));
        scene.getRoot()->addChild (quadAt (-0.7f, 0.3f, 0.0f, emissive));
        scene.getRoot()->addChild (quadAt (0.0f, 0.3f, 0.0f, masked, 0.2f, 0.8f));
        scene.getRoot()->addChild (quadAt (0.7f, 0.3f, 0.0f, blended));

        for (int i = 0; i < 8; ++i)
        {
            const auto angle = MathConstants<float>::twoPi * static_cast<float> (i) / 8.0f;

            auto light = EntityNode::Ptr (new EntityNode ("point"));
            auto& node = light->attach<LightNode>();
            node.type = LightNode::Type::point;
            node.color = { 0.5f + 0.5f * std::cos (angle), 0.5f + 0.5f * std::sin (angle), 0.5f };
            node.intensity = 4.0f;
            node.range = 1.2f;
            light->setPosition ({ 0.9f * std::cos (angle), 0.9f * std::sin (angle), 0.3f });
            scene.getRoot()->addChild (light);
        }
    }

    /** A rotated plate against a dark background, so silhouettes show the MSAA coverage. */
    static void buildEdgesScene (Scene& scene)
    {
        auto plates = makePlates();
        plates->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.3f));
        scene.getRoot()->addChild (plates);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>();
        scene.getRoot()->addChild (sun);
    }

    /** The folder holding the baseline images, tests/data/yup_3d/baseline. */
    static File getBaselineDirectory()
    {
        return File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("data/yup_3d/baseline");
    }

    /** Loads a baseline image, or returns an invalid image if it is missing. */
    static Image loadBaseline (const String& fileName)
    {
        auto stream = getBaselineDirectory().getChildFile (fileName).createInputStream();
        if (stream == nullptr)
            return {};

        PngImageFormatReader reader (stream.release());
        return reader.readImage();
    }

    /** Writes the image when YUP_3D_WRITE_BASELINE=1, into tests/data/yup_3d/baseline. */
    static void writeBaselineIfRequested (const Image& image, const String& platform, const String& sceneName)
    {
        if (SystemStats::getEnvironmentVariable ("YUP_3D_WRITE_BASELINE", {}) != "1")
            return;

        const auto file = getBaselineDirectory().getChildFile (platform + "_" + sceneName + ".png");
        file.getParentDirectory().createDirectory();
        file.deleteFile();

        auto stream = std::make_unique<FileOutputStream> (file);
        ASSERT_TRUE (stream->openedOk()) << file.getFullPathName();

        PngImageFormatWriter writer (stream.release(), PixelFormat::RGBA);
        ASSERT_TRUE (writer.writeImage (image)) << file.getFullPathName();
    }

    /** The region beside the small plate, where its shadow falls, is clearly darker than the lit big plate. */
    static void checkShadows (const Image& image)
    {
        const auto lit = static_cast<int> (image.getPixelColor (40, 60).getRed());
        const auto shadowed = static_cast<int> (image.getPixelColor (50, 64).getRed());

        RecordProperty ("shadowMargin", lit - shadowed);
        EXPECT_GE (lit - shadowed, 6) << "shadows: lit " << lit << ", shadowed " << shadowed;
    }

    /** The emissive quad is orange, the mask discards half of its quad, and the lights tint the plate with several hues. */
    static void checkMaterials (const Image& image)
    {
        const auto emissive = image.getPixelColor (42, 62);
        EXPECT_GT (emissive.getRed(), emissive.getGreen()) << "materials: emissive";
        EXPECT_GT (emissive.getGreen(), emissive.getBlue()) << "materials: emissive";

        // The left half of the masked quad shows the bluish plate behind, the right half the green quad
        const auto discarded = image.getPixelColor (56, 62);
        const auto kept = image.getPixelColor (68, 62);
        const auto plate = image.getPixelColor (53, 62); // The plate between the emissive and the masked quads
        EXPECT_NEAR (discarded.getRed(), plate.getRed(), 12) << "materials: masked left half";
        EXPECT_NEAR (discarded.getGreen(), plate.getGreen(), 12) << "materials: masked left half";
        EXPECT_NEAR (discarded.getBlue(), plate.getBlue(), 12) << "materials: masked left half";
        EXPECT_LT (kept.getBlue(), 110) << "materials: masked right half";
        EXPECT_GT (kept.getGreen(), kept.getRed() + 30) << "materials: masked right half";

        std::set<int> hues;
        for (int i = 0; i < 12; ++i)
        {
            const auto angle = MathConstants<float>::twoPi * static_cast<float> (i) / 12.0f;
            const auto x = 64 + static_cast<int> (std::round (34.0f * std::cos (angle)));
            const auto y = 64 + static_cast<int> (std::round (34.0f * std::sin (angle)));
            if (std::abs (y - 64) < 16 || y > 96)
                continue;

            const auto color = image.getPixelColor (x, y);
            if (color.getSaturation() > 0.1f)
                hues.insert (static_cast<int> (color.getHue() * 12.0f));
        }

        RecordProperty ("plateHues", static_cast<int> (hues.size()));
        EXPECT_GE (hues.size(), 3u) << "materials: distinct hues on the plate";
    }

    /** Silhouettes have intermediate gray levels, which only MSAA coverage produces. */
    static void checkEdges (const Image& image)
    {
        std::set<int> levels;
        for (int y = 30; y < 100; ++y)
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto red = static_cast<int> (image.getPixelColor (x, y).getRed());
                if (red > 30 && red < 147)
                    levels.insert (red);
            }

        EXPECT_GE (levels.size(), 3u) << "edges: intermediate levels";
    }

    /** Selects the rendering tier of a scene. */
    static void setTier (Scene& scene, RenderSettings::TierPreference tier)
    {
        auto settings = scene.getRenderSettings();
        settings.tier = tier;
        scene.setRenderSettings (settings);
    }

    /** The mean Rec.709 luma of the 8 bit levels within @p area. */
    static double meanLuminance (const Image& image, Rectangle<int> area)
    {
        auto sum = 0.0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
        {
            for (int x = area.getX(); x < area.getRight(); ++x)
            {
                const auto color = image.getPixelColor (x, y);
                sum += 0.2126 * color.getRed() + 0.7152 * color.getGreen() + 0.0722 * color.getBlue();
            }
        }

        return sum / static_cast<double> (area.getWidth() * area.getHeight());
    }

    /** A small emissive quad, (20, 20, 20) by default, about 15 px wide at 128x128, in the middle of a black background. */
    static void buildGlowScene (Scene& scene, float emission = 20.0f)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.25f, -0.25f, 0.0f };
        vertices[1].position = { 0.25f, -0.25f, 0.0f };
        vertices[2].position = { 0.25f, 0.25f, 0.0f };
        vertices[3].position = { -0.25f, 0.25f, 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto emissive = Material::Ptr (new Material());
        emissive->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        emissive->emissiveFactor = { emission, emission, emission };

        auto mesh = Mesh::Ptr (new Mesh ("glow"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, emissive);

        auto entity = EntityNode::Ptr (new EntityNode ("glow"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
        scene.setBackgroundColor (Colors::black);
    }

    /** Turns bloom on or off in the render settings of a scene. */
    static void setBloom (Scene& scene, bool bloom)
    {
        auto settings = scene.getRenderSettings();
        settings.bloom = bloom;
        scene.setRenderSettings (settings);
    }

    /** The bounds of the pixels brighter than half the range: the emissive quad of buildGlowScene(). */
    static Rectangle<int> findBrightBounds (const Image& image)
    {
        auto left = image.getWidth(), top = image.getHeight(), right = 0, bottom = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                if (image.getPixelColor (x, y).getRed() > 127)
                {
                    left = std::min (left, x);
                    top = std::min (top, y);
                    right = std::max (right, x + 1);
                    bottom = std::max (bottom, y + 1);
                }
            }
        }

        return right > left ? Rectangle<int> (left, top, right - left, bottom - top) : Rectangle<int>();
    }

    /** The mean luma of the ring from 4 to 12 px around @p quad. */
    static double ringLuminance (const Image& image, Rectangle<int> quad)
    {
        const auto outer = quad.enlarged (12);
        const auto inner = quad.enlarged (4);
        const auto outerArea = static_cast<double> (outer.getWidth() * outer.getHeight());
        const auto innerArea = static_cast<double> (inner.getWidth() * inner.getHeight());

        return (meanLuminance (image, outer) * outerArea - meanLuminance (image, inner) * innerArea) / (outerArea - innerArea);
    }

    /** The pixels of @p image more than 2 levels away from @p reference in any channel. */
    static int countDifferences (const Image& image, const Image& reference)
    {
        int differing = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto a = image.getPixelColor (x, y);
                const auto b = reference.getPixelColor (x, y);
                if (std::abs (a.getRed() - b.getRed()) > 2 || std::abs (a.getGreen() - b.getGreen()) > 2 || std::abs (a.getBlue() - b.getBlue()) > 2)
                    ++differing;
            }
        }

        return differing;
    }

    /** The pixels of @p image more than 2 levels away from the Phase 0 baseline @p fileName, or -1 without the baseline. */
    static int countBaselineDifferences (const Image& image, const String& fileName)
    {
        const auto baseline = loadBaseline (fileName);
        return baseline.isValid() ? countDifferences (image, baseline) : -1;
    }

    /** How far a readVelocity() level is from zero velocity (127.5): 0 for 127 and 128, up to 127. */
    static int velocityDeviation (uint8 level)
    {
        return std::abs (2 * static_cast<int> (level) - 255) / 2;
    }

    /** The largest velocityDeviation() of the red or green level over an image. */
    static int maxVelocityDeviation (const Image& image)
    {
        int deviation = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto color = image.getPixelColor (x, y);
                deviation = std::max ({ deviation, velocityDeviation (color.getRed()), velocityDeviation (color.getGreen()) });
            }
        }

        return deviation;
    }

    /** Renders each baseline scene at 128x128, checks it drew something and optionally writes it. */
    void captureBaselineScenes (const String& platform)
    {
        const std::pair<const char*, void (*) (Scene&)> scenes[] = {
            { "shadows", buildShadowsScene },
            { "materials", buildMaterialsScene },
            { "edges", buildEdgesScene },
        };

        for (const auto& [name, build] : scenes)
        {
            Scene scene;
            build (scene);

            // The baselines are raster frames, whichever tier the device would pick
            setTier (scene, RenderSettings::TierPreference::raster);

            SceneRenderer renderer;
            const auto image = renderToImage (renderer, scene, 128);

            ASSERT_TRUE (renderer.getLastError().isEmpty()) << name << ": " << renderer.getLastError();
            ASSERT_TRUE (image.isValid()) << name;

            // Something other than the background was drawn in the middle of the frame
            const auto background = scene.getBackgroundColor();
            bool drewSomething = false;
            for (int y = 32; y < 96 && ! drewSomething; y += 4)
                for (int x = 32; x < 96 && ! drewSomething; x += 4)
                    drewSomething = image.getPixelColor (x, y) != background;

            EXPECT_TRUE (drewSomething) << name;

            if (String (name) == "shadows")
                checkShadows (image);
            else if (String (name) == "materials")
                checkMaterials (image);
            else
                checkEdges (image);

            writeBaselineIfRequested (image, platform, name);
        }
    }

    /** Renders and reads the pixels back through a canvas. */
    Image renderToImage (SceneRenderer& renderer, Scene& scene, int size)
    {
        auto texture = renderer.render (device, scene, size, size);
        return texture != nullptr ? readBack (texture) : Image();
    }

    /** Draws an rgba8 texture through a canvas and reads it back: row 0 of the image is the top of the frame. */
    Image readBack (const GpuTexture::Ptr& texture)
    {
        const auto width = static_cast<int> (texture->getWidth());
        const auto height = static_cast<int> (texture->getHeight());

        auto canvas = GpuCanvas::create (*context, width, height);
        if (canvas == nullptr)
            return {};

        auto& g = canvas->beginDraw();
        g.drawTexture (texture, { 0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height) });
        return canvas->asImage();
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** Runs @p fragment over @p source (texture at set 0 binding 0, sampler at binding 1) into an
        rgba8 target of its size, and reads the result back. */
    Image runFullscreen (const String& fragment, const GpuTexture::Ptr& source)
    {
        if (source == nullptr)
            return {};

        auto target = GpuTarget::create (device, static_cast<int> (source->getWidth()), static_cast<int> (source->getHeight()));
        if (target == nullptr || ! drawFullscreen (fragment, source, target->asTexture()))
            return {};

        return readBack (target->asTexture());
    }

    /** Draws @p fragment into the render target @p target, binding @p source (if any) like runFullscreen(). */
    bool drawFullscreen (const String& fragment, const GpuTexture::Ptr& source, const GpuTexture::Ptr& target)
    {
        if (target == nullptr)
            return false;

        const auto vertex = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
                                .getChildFile ("modules/yup_3d/shaders/yup_Fullscreen.vert")
                                .loadFileAsString();

        ShaderBundleCompileRequest request;
        request.sourceLanguage = ShaderLanguage::glsl;

        for (const auto& [stage, code] : { std::pair { ShaderStage::vertex, vertex }, std::pair { ShaderStage::fragment, fragment } })
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
            return false;
        }

        MemoryBlock block;
        if (! bundle.getReference().saveToMemoryBlock (block).wasOk())
            return false;

        FullscreenPass pass;
        const auto prepared = pass.prepare (device, static_cast<const uint8*> (block.getData()), block.getSize(), target->getFormat());
        if (prepared.failed())
        {
            ADD_FAILURE() << prepared.getErrorMessage();
            return false;
        }

        auto sampler = GpuSampler::create (device, GpuSamplerDesc (GpuFilter::nearest, GpuWrapMode::clampToEdge));
        if (sampler == nullptr)
            return false;

        auto frame = GpuFrame::begin (device);
        const auto drawn = pass.draw (frame, target, {}, [&] (GpuRenderPass& renderPass)
        {
            if (source != nullptr)
            {
                renderPass.setTexture (0, 0, source);
                renderPass.setSampler (0, 1, sampler);
            }
        }, GpuColor::transparentBlack());
        frame.submit();
        frame.waitForGPU();

        return drawn;
    }

    /** True when no texel of @p texture is NaN or infinite. */
    bool isFiniteTexture (const GpuTexture::Ptr& texture)
    {
        // An all-ones exponent is NaN or infinity. Testing the bits keeps fast math from folding isnan away
        const auto image = runFullscreen (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main()
{
    vec4 c = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0);
    bvec4 nonFinite = equal(floatBitsToUint(c) & uvec4(0x7f800000u), uvec4(0x7f800000u));
    color = any(nonFinite) ? vec4(1.0) : vec4(0.0, 0.0, 0.0, 1.0);
})",
                                          texture);

        if (! image.isValid())
            return false;

        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                if (image.getPixelColor (x, y).getRed() > 127)
                    return false;

        return true;
    }

    /** True when any texel of @p texture has a color channel at or beyond the half float range.
        Metal returns 65504 for min (NaN, 65504.0), so a clamped NaN reaches the texture as this. */
    bool isSaturatedTexture (const GpuTexture::Ptr& texture)
    {
        const auto image = runFullscreen (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main()
{
    vec4 c = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0);
    color = any(greaterThanEqual(abs(c.rgb), vec3(65504.0))) ? vec4(1.0) : vec4(0.0, 0.0, 0.0, 1.0);
})",
                                          texture);

        if (! image.isValid())
            return true;

        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                if (image.getPixelColor (x, y).getRed() > 127)
                    return true;

        return false;
    }

    /** The rg16float velocity as signed 8 bit levels of v * @p scale + 0.5, x in red and y in green:
        zero velocity reads 127 or 128. */
    Image readVelocity (const GpuTexture::Ptr& velocity, float scale)
    {
        return runFullscreen (R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main()
{
    vec2 v = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0).xy;
    color = vec4(v * float()" + String (scale)
                                  + R"() + 0.5, 0.0, 1.0);
})",
                              velocity);
    }
#endif

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;
};
#endif

#if YUP_APPLE
class SceneRendererMetalTests : public SceneRendererBaselineScenes
{
protected:
    void SetUp() override
    {
        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";
    }

    /** Selects the full tier, accumulating @p taaSamples jittered frames (1 disables TAA). */
    static void setFullTierTaa (Scene& scene, int taaSamples)
    {
        auto settings = scene.getRenderSettings();
        settings.tier = RenderSettings::TierPreference::full;
        settings.taaSamples = taaSamples;
        scene.setRenderSettings (settings);
    }

    /** The gray levels along row 40 of the edges scene at 128x128, which crosses the background (red
        26), the plate (red 151 to 153) and two silhouettes. The flat areas vary by a level or two, so
        levels within 2 of either count as that area: 2 without antialiasing.
    */
    static size_t countEdgeLevels (const Image& image)
    {
        std::set<int> levels;
        for (int x = 0; image.isValid() && x < 128; ++x)
        {
            const auto red = static_cast<int> (image.getPixelColor (x, 40).getRed());
            levels.insert (red <= 28 ? 26 : (red >= 149 ? 151 : red));
        }

        return levels.size();
    }

    /** Renders the shadows scene with a uniform environment of the given radiance and returns a corner, which shows the environment. */
    Color renderEnvironmentCorner (float radiance, Scene::ToneMapping toneMapping)
    {
        Scene scene;
        buildShadowsScene (scene);
        scene.setEnvironment (new EnvironmentMap ([radiance] (const Vector3<float>&) { return Vector3<float> (radiance, radiance, radiance); }, 32, 3));
        scene.setToneMapping (toneMapping);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);

        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_TRUE (image.isValid());
        return image.isValid() ? image.getPixelColor (1, 1) : Color();
    }

    /** Adds a square plate facing the default camera, 2 units wide, drawn with @p material. */
    static void addPlate (Scene& scene, Material::Ptr material)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -1.0f, -1.0f, 0.0f };
        vertices[1].position = { 1.0f, -1.0f, 0.0f };
        vertices[2].position = { 1.0f, 1.0f, 0.0f };
        vertices[3].position = { -1.0f, 1.0f, 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("plate"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode ("plate"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
    }

    /** The linear radiance behind an 8 bit level that was Reinhard tone mapped and sRGB encoded. */
    static double reinhardRadiance (int level)
    {
        const auto encoded = level / 255.0;
        const auto mapped = encoded <= 0.04045 ? encoded / 12.92 : std::pow ((encoded + 0.055) / 1.055, 2.4);
        return mapped / (1.0 - mapped);
    }

    /** A rough (roughness 1) plate drawn with @p material and lit head-on by a white sun of
        intensity 2, without ambient light: the mean linear radiance of the 8x8 pixels at the
        center, where the normal, the view and the light directions coincide. There
        D = 1 / pi, V = 1 / 4 and F = f0, so the radiance is 2 (f0 / 4 + diffuseColor * weight) / pi.
    */
    std::array<double, 3> renderHeadOnRadiance (Material::Ptr material, RenderSettings::TierPreference tier)
    {
        material->roughnessFactor = 1.0f;

        Scene scene;
        addPlate (scene, std::move (material));
        scene.setAmbientColor (Colors::black);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        setTier (scene, tier);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 2.0f;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();

        std::array<double, 3> radiance { -1.0, -1.0, -1.0 };
        if (! image.isValid())
            return radiance;

        radiance = { 0.0, 0.0, 0.0 };
        for (int y = 28; y < 36; ++y)
        {
            for (int x = 28; x < 36; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                radiance[0] += reinhardRadiance (color.getRed()) / 64.0;
                radiance[1] += reinhardRadiance (color.getGreen()) / 64.0;
                radiance[2] += reinhardRadiance (color.getBlue()) / 64.0;
            }
        }

        return radiance;
    }

    /** A dielectric material of the given gray base color, for renderHeadOnRadiance. */
    static Material::Ptr makeDielectric (float base, float ior, float specularFactor, std::array<float, 3> specularColor = { 1.0f, 1.0f, 1.0f })
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { base, base, base, 1.0f };
        material->metallicFactor = 0.0f;
        material->ior = ior;
        material->specularFactor = specularFactor;
        material->specularColorFactor = specularColor;
        return material;
    }

    /** The pixels of row @p y more than 3 levels of red away from both the darkest and the
        brightest pixel of the row: partially covered silhouette pixels, 0 without antialiasing.
    */
    static int countIntermediatePixels (const Image& image, int y)
    {
        int darkest = 255, brightest = 0;
        for (int x = 0; x < image.getWidth(); ++x)
        {
            darkest = jmin (darkest, static_cast<int> (image.getPixelColor (x, y).getRed()));
            brightest = jmax (brightest, static_cast<int> (image.getPixelColor (x, y).getRed()));
        }

        int count = 0;
        for (int x = 0; x < image.getWidth(); ++x)
        {
            const auto red = static_cast<int> (image.getPixelColor (x, y).getRed());
            count += red > darkest + 3 && red < brightest - 3 ? 1 : 0;
        }

        return count;
    }

    /** Two emissive quads standing apart, rotated so their silhouettes cross pixels at every
        coverage: a bright one (10) on the left half of the frame and a mid-gray one (0.2) on the
        right half, over a blue-gray background.
    */
    static void buildSilhouettesScene (Scene& scene)
    {
        const auto addQuad = [&] (const char* name, float emission, float x)
        {
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = { -0.4f, -0.4f, 0.0f };
            vertices[1].position = { 0.4f, -0.4f, 0.0f };
            vertices[2].position = { 0.4f, 0.4f, 0.0f };
            vertices[3].position = { -0.4f, 0.4f, 0.0f };
            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
            material->emissiveFactor = { emission, emission, emission };

            auto mesh = Mesh::Ptr (new Mesh (name));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);
            auto entity = EntityNode::Ptr (new EntityNode (name));
            entity->attach<MeshNode> (mesh);
            entity->setPosition ({ x, 0.0f, 0.0f });
            entity->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.3f));
            scene.getRoot()->addChild (entity);
        };

        addQuad ("bright", 10.0f, -0.8f);
        addQuad ("gray", 0.2f, 0.8f);
        scene.setBackgroundColor (Color (static_cast<uint8> (40), static_cast<uint8> (70), static_cast<uint8> (110)));
    }

    /** The mean luma of @p image over the band within 2 px of the silhouettes of @p reference, in
        the columns from @p left to @p right. A silhouette pixel differs by more than 3 levels of
        luma from a 4-neighbor. The band reaches the flat areas on both sides, so it holds every
        partially covered pixel whatever the antialiasing, and its mean is linear in the coverage.
    */
    static double silhouetteBandLuminance (const Image& image, const Image& reference, int left, int right)
    {
        const auto luma = [] (const Image& source, int x, int y)
        {
            const auto color = source.getPixelColor (jlimit (0, source.getWidth() - 1, x), jlimit (0, source.getHeight() - 1, y));
            return 0.2126 * color.getRed() + 0.7152 * color.getGreen() + 0.0722 * color.getBlue();
        };

        const auto isSilhouette = [&] (int x, int y)
        {
            const auto center = luma (reference, x, y);
            return std::abs (center - luma (reference, x - 1, y)) > 3.0 || std::abs (center - luma (reference, x + 1, y)) > 3.0
                || std::abs (center - luma (reference, x, y - 1)) > 3.0 || std::abs (center - luma (reference, x, y + 1)) > 3.0;
        };

        auto sum = 0.0;
        int count = 0;
        for (int y = 0; y < reference.getHeight(); ++y)
        {
            for (int x = left; x < right; ++x)
            {
                bool inBand = false;
                for (int dy = -2; dy <= 2 && ! inBand; ++dy)
                    for (int dx = -2; dx <= 2 && ! inBand; ++dx)
                        inBand = isSilhouette (x + dx, y + dy);

                if (inBand)
                {
                    sum += luma (image, x, y);
                    ++count;
                }
            }
        }

        return count > 0 ? sum / count : -1.0;
    }
};

TEST_F (SceneRendererMetalTests, AgxToneMapsTheEnvironment)
{
    const auto mid = renderEnvironmentCorner (0.5f, Scene::ToneMapping::agx);
    const auto expected = SceneRendererAgxMirrorTests::toneMapAgx ({ 0.5f, 0.5f, 0.5f });

    // 0.5 through AgX lands inside the displayable range, neither crushed nor clipped
    EXPECT_GT (mid.getRed(), 0);
    EXPECT_LT (mid.getRed(), 255);
    EXPECT_NEAR (mid.getRed(), expected[0], 3);
    EXPECT_NEAR (mid.getGreen(), expected[1], 3);
    EXPECT_NEAR (mid.getBlue(), expected[2], 3);

    const auto aces = renderEnvironmentCorner (0.5f, Scene::ToneMapping::aces);
    EXPECT_GE (std::abs (static_cast<int> (aces.getRed()) - static_cast<int> (mid.getRed())), 3);

    const auto dim = renderEnvironmentCorner (0.25f, Scene::ToneMapping::agx);
    const auto bright = renderEnvironmentCorner (1.0f, Scene::ToneMapping::agx);
    EXPECT_NEAR (dim.getRed(), SceneRendererAgxMirrorTests::toneMapAgx ({ 0.25f, 0.25f, 0.25f })[0], 3);
    EXPECT_NEAR (bright.getRed(), SceneRendererAgxMirrorTests::toneMapAgx ({ 1.0f, 1.0f, 1.0f })[0], 3);
    EXPECT_GT (bright.getRed(), mid.getRed());
    EXPECT_GT (mid.getRed(), dim.getRed());
}

TEST_F (SceneRendererMetalTests, RendersEnvironmentShadowsAndBackground)
{
    Scene scene;
    scene.getRoot()->addChild (makePlates());

    // Shining down -Z, tilted so the shadow of the small plate falls beside it
    auto sun = EntityNode::Ptr (new EntityNode ("sun"));
    sun->attach<LightNode>().castsShadows = true;
    sun->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));
    scene.getRoot()->addChild (sun);

    scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
    scene.setEnvironmentVisible (true);
    scene.setToneMapping (Scene::ToneMapping::aces);

    SceneRenderer renderer;
    const auto image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
    ASSERT_TRUE (image.isValid());

    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    const auto corner = image.getPixelColor (1, 1);
    EXPECT_NEAR (corner.getRed(), 206, 3);
    EXPECT_NEAR (corner.getGreen(), 206, 3);
    EXPECT_NEAR (corner.getBlue(), 206, 3);
}

TEST_F (SceneRendererMetalTests, CapturesBaselineScenes)
{
    captureBaselineScenes ("metal");
}

TEST_F (SceneRendererMetalTests, MatchesPhaseZeroBaselineOnRaster)
{
    const std::pair<const char*, void (*) (Scene&)> scenes[] = {
        { "shadows", buildShadowsScene },
        { "materials", buildMaterialsScene },
        { "edges", buildEdgesScene },
    };

    for (const auto& [name, build] : scenes)
    {
        Scene scene;
        build (scene);

        RenderSettings settings;
        settings.tier = RenderSettings::TierPreference::raster;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();

        const auto differing = countBaselineDifferences (image, String ("metal_") + name + ".png");
        ASSERT_GE (differing, 0) << "capture the Phase 0 baselines first";

        RecordProperty ((String (name) + "Differing").toStdString(), differing);
        EXPECT_LE (differing, 128 * 128 / 200) << name; // At most 0.5% of the pixels beyond 2 levels
    }
}

TEST_F (SceneRendererMetalTests, RasterBloomOffStaysOnTheBaseline)
{
    const std::pair<const char*, void (*) (Scene&)> scenes[] = {
        { "shadows", buildShadowsScene },
        { "materials", buildMaterialsScene },
        { "edges", buildEdgesScene },
    };

    for (const auto& [name, build] : scenes)
    {
        Scene scene;
        build (scene);

        // Bloom explicitly off, its other settings changed: the frame stays on the rgba8 inline path
        RenderSettings settings;
        settings.tier = RenderSettings::TierPreference::raster;
        settings.bloom = false;
        settings.bloomThreshold = 0.1f;
        settings.bloomIntensity = 1.0f;
        settings.bloomRadius = 1.0f;
        scene.setRenderSettings (settings);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr) << name;
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba8unorm) << name;
        EXPECT_FALSE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::hdrSceneColor, settings)) << name;

        const auto differing = countBaselineDifferences (image, String ("metal_") + name + ".png");
        ASSERT_GE (differing, 0) << "capture the Phase 0 baselines first";

        RecordProperty ((String (name) + "Differing").toStdString(), differing);
        EXPECT_EQ (differing, 0) << name;
    }
}

TEST_F (SceneRendererMetalTests, RasterBloomIsUnavailableWhenHalfFloatCannotMultisample)
{
    // Metal multisamples rgba16float at 4: pretend it allows only 2, below the default 4 samples
    const auto halfFloatTakesTwoSamples = [] (RenderCapabilities::DeviceFacts& facts)
    {
        facts.rgba16floatSamples = 2;
    };

    Scene scene;
    buildGlowScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    SceneRenderer withoutBloom;
    withoutBloom.setDeviceFactsOverride (halfFloatTakesTwoSamples);
    const auto reference = renderToImage (withoutBloom, scene, 64);
    ASSERT_TRUE (reference.isValid()) << withoutBloom.getLastError();

    setBloom (scene, true);

    SceneRenderer withBloom;
    withBloom.setDeviceFactsOverride (halfFloatTakesTwoSamples);
    const auto image = renderToImage (withBloom, scene, 64);
    ASSERT_TRUE (image.isValid()) << withBloom.getLastError();

    const auto* capabilities = withBloom.getCapabilities();
    ASSERT_NE (capabilities, nullptr);
    EXPECT_EQ (capabilities->getTier(), RenderCapabilities::Tier::raster);
    EXPECT_FALSE (capabilities->isAvailable (RenderCapabilities::Feature::bloom));
    EXPECT_FALSE (capabilities->isEnabled (RenderCapabilities::Feature::hdrSceneColor, scene.getRenderSettings()));

    // Still the multisampled rgba8 inline frame: not black, not one sample, identical to bloom off
    const auto sceneColor = withBloom.getPassOutput (SceneRenderer::PassOutput::sceneColor);
    ASSERT_NE (sceneColor, nullptr);
    EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba8unorm);

    // Two renders of one raster frame can differ by a level here and there
    EXPECT_EQ (countDifferences (image, reference), 0);
    EXPECT_GT (meanLuminance (image, findBrightBounds (image)), 200.0);
}

TEST_F (SceneRendererMetalTests, BloomStaysFiniteOverSaturatedHighlights)
{
    // Exposed radiance far above the half float range: the scene color and the TAA history hold
    // the largest half float, and the glow added over it must not overflow to infinity
    Scene scene;
    buildGlowScene (scene, 1.0e5f);
    setFullTierTaa (scene, 4);

    SceneRenderer withoutBloom;
    const auto off = renderToImage (withoutBloom, scene, 128);
    ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

    const auto area = findBrightBounds (off).enlarged (8);
    ASSERT_FALSE (area.isEmpty());

    setBloom (scene, true);

    SceneRenderer withBloom;
    for (int frame = 1; frame <= 4; ++frame)
    {
        const auto image = renderToImage (withBloom, scene, 128);
        ASSERT_TRUE (image.isValid()) << withBloom.getLastError();

        int dark = 0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
            for (int x = area.getX(); x < area.getRight(); ++x)
                dark += image.getPixelColor (x, y).getRed() < 100 ? 1 : 0;

        EXPECT_EQ (dark, 0) << "frame " << frame;
    }
}

TEST_F (SceneRendererMetalTests, RasterBloomRingMatchesTheFullTier)
{
    std::map<RenderSettings::TierPreference, double> rings;

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        Scene scene;
        buildGlowScene (scene);
        setTier (scene, tier);

        SceneRenderer withoutBloom;
        const auto off = renderToImage (withoutBloom, scene, 128);
        ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

        setBloom (scene, true);

        SceneRenderer withBloom;
        const auto on = renderToImage (withBloom, scene, 128);
        ASSERT_TRUE (on.isValid()) << withBloom.getLastError();

        rings[tier] = ringLuminance (on, findBrightBounds (off));
    }

    const auto raster = rings[RenderSettings::TierPreference::raster];
    const auto full = rings[RenderSettings::TierPreference::full];
    RecordProperty ("rasterRing", String (raster, 2).toStdString());
    RecordProperty ("fullRing", String (full, 2).toStdString());

    EXPECT_GT (full, 5.0);
    EXPECT_NEAR (raster, full, full * 0.1);
}

TEST_F (SceneRendererMetalTests, RasterRendererTogglesBloomOnOffOn)
{
    Scene scene;
    buildGlowScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    SceneRenderer reference;
    const auto inline8 = renderToImage (reference, scene, 128);
    ASSERT_TRUE (inline8.isValid()) << reference.getLastError();
    const auto quad = findBrightBounds (inline8);

    SceneRenderer renderer;
    for (const auto bloom : { true, false, true })
    {
        setBloom (scene, bloom);

        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        // Bloom renders the reversible half-float scene color, without it the rgba8 inline frame
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), bloom ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm) << "bloom " << bloom;
        EXPECT_EQ (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::hdrSceneColor, scene.getRenderSettings()), bloom);

        const auto ring = ringLuminance (image, quad);
        RecordProperty ((String ("ring") + (bloom ? "On" : "Off")).toStdString(), String (ring, 2).toStdString());

        if (bloom)
            EXPECT_GE (ring, 5.0);
        else
            EXPECT_EQ (countDifferences (image, inline8), 0);

        EXPECT_GE (meanLuminance (image, quad.reduced (2)), 245.0) << "bloom " << bloom;
    }
}

TEST_F (SceneRendererMetalTests, BloomChangesKeepTheTaaHistory)
{
    Scene scene;
    buildGlowScene (scene);
    setFullTierTaa (scene, 4);
    setBloom (scene, true);

    SceneRenderer renderer;
    Image converged;
    for (int i = 0; i < 4; ++i)
        converged = renderToImage (renderer, scene, 128);

    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Bloom runs after temporal anti-aliasing, so its settings apply to the kept history
    auto settings = scene.getRenderSettings();
    settings.bloomIntensity = 0.5f;
    scene.setRenderSettings (settings);

    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    const auto quad = findBrightBounds (converged);
    const auto before = ringLuminance (converged, quad);
    const auto after = ringLuminance (image, quad);
    RecordProperty ("ringBefore", String (before, 2).toStdString());
    RecordProperty ("ringAfter", String (after, 2).toStdString());
    EXPECT_GT (after, before + 5.0);
}

TEST_F (SceneRendererMetalTests, RendersWithoutMultisamplingIntoTheReturnedTexture)
{
    // An emissive quad above and left of the center makes the frame asymmetric, so a flip shows
    const auto buildScene = [] (Scene& scene)
    {
        buildShadowsScene (scene);

        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.7f, 0.3f, 0.6f };
        vertices[1].position = { -0.3f, 0.3f, 0.6f };
        vertices[2].position = { -0.3f, 0.7f, 0.6f };
        vertices[3].position = { -0.7f, 0.7f, 0.6f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto emissive = Material::Ptr (new Material());
        emissive->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        emissive->emissiveFactor = { 1.0f, 0.5f, 0.1f };

        auto mesh = Mesh::Ptr (new Mesh ("marker"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, emissive);

        auto marker = EntityNode::Ptr (new EntityNode ("marker"));
        marker->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (marker);
    };

    // Orange marker pixels in the top-left and bottom-left quadrants
    const auto countMarker = [] (const Image& image, bool top)
    {
        int count = 0;
        for (int y = top ? 0 : 32; y < (top ? 32 : 64); ++y)
        {
            for (int x = 0; x < 32; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                if (static_cast<int> (color.getRed()) - static_cast<int> (color.getBlue()) > 80)
                    ++count;
            }
        }

        return count;
    };

    // The sample count applies to the raster tier only
    Scene scene;
    buildScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    SceneRenderer single;
    single.setSampleCount (1);
    const auto image = renderToImage (single, scene, 64);
    ASSERT_TRUE (single.getLastError().isEmpty()) << single.getLastError();
    ASSERT_TRUE (image.isValid());

    SceneRenderer multisampled;
    const auto reference = renderToImage (multisampled, scene, 64);
    ASSERT_TRUE (reference.isValid());

    // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
    EXPECT_NEAR (image.getPixelColor (1, 1).getRed(), 206, 3);

    // The marker lands at the top on both paths, never at the bottom
    EXPECT_GT (countMarker (reference, true), 4);
    EXPECT_EQ (countMarker (reference, false), 0);
    EXPECT_GT (countMarker (image, true), 4);
    EXPECT_EQ (countMarker (image, false), 0);
}

TEST_F (SceneRendererMetalTests, FullTierMatchesRasterExposureAndToneMapping)
{
    std::map<RenderSettings::TierPreference, double> luminance;

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        Scene scene;
        buildShadowsScene (scene);
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto* capabilities = renderer.getCapabilities();
        ASSERT_NE (capabilities, nullptr);
        ASSERT_EQ (capabilities->getTier() == RenderCapabilities::Tier::full, tier == RenderSettings::TierPreference::full);

        // The full frame really went through the HDR scene color, not a raster fallback
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), tier == RenderSettings::TierPreference::full ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm);

        // A corner shows the environment: 0.5 through ACES is 0.616, 206 once sRGB encoded
        const auto corner = image.getPixelColor (1, 1);
        EXPECT_NEAR (corner.getRed(), 206, 3);
        EXPECT_NEAR (corner.getGreen(), 206, 3);
        EXPECT_NEAR (corner.getBlue(), 206, 3);
        RecordProperty (tier == RenderSettings::TierPreference::full ? "fullCorner" : "rasterCorner", static_cast<int> (corner.getRed()));

        luminance[tier] = meanLuminance (image, { 32, 32, 64, 64 });
    }

    const auto raster = luminance[RenderSettings::TierPreference::raster];
    const auto full = luminance[RenderSettings::TierPreference::full];
    RecordProperty ("rasterLuminance", String (raster, 3).toStdString());
    RecordProperty ("fullLuminance", String (full, 3).toStdString());

    // Same shading, different antialiasing
    EXPECT_NEAR (full, raster, raster * 0.02);
}

TEST_F (SceneRendererMetalTests, FullTierBackgroundColorMatchesRaster)
{
    // No environment: the background color shows around the plates, display-referred on both tiers
    const auto background = Color (static_cast<uint8> (200), static_cast<uint8> (80), static_cast<uint8> (40));

    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        const auto tierName = tier == RenderSettings::TierPreference::full ? String ("full") : String ("raster");

        Scene scene;
        scene.getRoot()->addChild (makePlates());
        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>();
        scene.getRoot()->addChild (sun);
        scene.setBackgroundColor (background);
        setTier (scene, tier);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << tierName << ": " << renderer.getLastError();
        ASSERT_EQ (renderer.getCapabilities()->getTier() == RenderCapabilities::Tier::full, tier == RenderSettings::TierPreference::full);

        const auto corner = image.getPixelColor (1, 1);
        RecordProperty ((tierName + "Corner").toStdString(), (String (static_cast<int> (corner.getRed())) + "," + String (static_cast<int> (corner.getGreen())) + "," + String (static_cast<int> (corner.getBlue()))).toStdString());
        EXPECT_NEAR (corner.getRed(), background.getRed(), 1) << tierName;
        EXPECT_NEAR (corner.getGreen(), background.getGreen(), 1) << tierName;
        EXPECT_NEAR (corner.getBlue(), background.getBlue(), 1) << tierName;

        // The lit white plate covers the center
        const auto center = image.getPixelColor (32, 32);
        EXPECT_GT (std::abs (static_cast<int> (center.getBlue()) - static_cast<int> (background.getBlue())), 40) << tierName;
    }
}

TEST_F (SceneRendererMetalTests, FullTierBackgroundColorSurvivesTaaConvergence)
{
    // Temporal anti-aliasing resolves the coverage alpha too, so the background still shows exactly
    const auto background = Color (static_cast<uint8> (200), static_cast<uint8> (80), static_cast<uint8> (40));

    Scene scene;
    buildEdgesScene (scene);
    scene.setBackgroundColor (background);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    for (int frame = 1; frame <= 6; ++frame)
    {
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto corner = image.getPixelColor (1, 1);
        EXPECT_NEAR (corner.getRed(), background.getRed(), 1) << "frame " << frame;
        EXPECT_NEAR (corner.getGreen(), background.getGreen(), 1) << "frame " << frame;
        EXPECT_NEAR (corner.getBlue(), background.getBlue(), 1) << "frame " << frame;
    }

    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, RasterIsAlwaysConverged)
{
    Scene scene;
    buildEdgesScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    SceneRenderer renderer;
    EXPECT_TRUE (renderer.isConverged());

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    // Changing the scene changes nothing on raster
    scene.setExposure (2.0f);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, ConvergesThenRestartsOnChange)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    auto material = Material::Ptr (new Material());
    auto* plates = scene.getRoot()->findChild ("plates");
    auto* sun = scene.getRoot()->findChild ("sun");
    ASSERT_NE (plates, nullptr);
    ASSERT_NE (sun, nullptr);
    plates->attach<MaterialNode> (material);

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    SceneRenderer renderer;
    const auto expectConvergesAfterFourRenders = [&] (const char* step)
    {
        for (int i = 1; i <= 4; ++i)
        {
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << step << ": " << renderer.getLastError();
            EXPECT_EQ (renderer.isConverged(), i == 4) << step << ", render " << i;
        }

        // Nothing changed: still converged
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << step << ": " << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged()) << step;
    };

    expectConvergesAfterFourRenders ("first");

    scene.setExposure (2.0f);
    expectConvergesAfterFourRenders ("exposure");

    material->roughnessFactor = 0.5f;
    expectConvergesAfterFourRenders ("material");

    sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 0.4f));
    expectConvergesAfterFourRenders ("light");

    camera->setPosition ({ 0.1f, 0.0f, 5.0f });
    expectConvergesAfterFourRenders ("camera");
}

TEST_F (SceneRendererMetalTests, ConvergedFrameStopsChanging)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    Image converged;
    for (int i = 0; i < 4; ++i)
        converged = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Further frames keep the history as it is, whatever the jitter
    for (int i = 0; i < 3; ++i)
    {
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        int differing = 0;
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                differing += image.getPixelColor (x, y) != converged.getPixelColor (x, y) ? 1 : 0;

        EXPECT_EQ (differing, 0) << "frame " << i;
    }
}

TEST_F (SceneRendererMetalTests, SignedZeroAndNanPayloadsAreNoChange)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 2);

    // No normal texture: the normal scale is never read, so a NaN there renders nothing wrong
    auto material = Material::Ptr (new Material());
    material->metallicFactor = 0.0f;
    material->normalScale = std::bit_cast<float> (0x7fc00000u);
    scene.getRoot()->findChild ("plates")->attach<MaterialNode> (material);

    SceneRenderer renderer;
    for (int i = 0; i < 2; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    material->metallicFactor = -0.0f;
    material->normalScale = std::bit_cast<float> (0xffc00001u);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, SampleCountChangesKeepTheFullTierConverged)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    // The full tier renders one sample per pixel whatever the count, so its accumulation is kept,
    // and so is the texture it renders into
    SceneRenderer renderer;
    GpuTexture::Ptr output;
    for (int i = 0; i < 4; ++i)
        ASSERT_NE (output = renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    renderer.setSampleCount (1);
    EXPECT_EQ (renderer.render (device, scene, 64, 64), output) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());

    // The raster tier recreates its multisampled targets for the new count
    setTier (scene, RenderSettings::TierPreference::raster);
    renderer.setSampleCount (4);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    renderer.setSampleCount (1);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    renderer.setSampleCount (4);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
}

TEST_F (SceneRendererMetalTests, TaaSmoothsEdges)
{
    const auto edgeLevels = [this] (int taaSamples)
    {
        Scene scene;
        buildEdgesScene (scene);
        setFullTierTaa (scene, taaSamples);

        SceneRenderer renderer;
        Image image;
        for (int i = 0; i < jmax (1, taaSamples); ++i)
            image = renderToImage (renderer, scene, 128);

        EXPECT_TRUE (renderer.isConverged()) << taaSamples;
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return countEdgeLevels (image);
    };

    const auto antialiased = edgeLevels (16);
    const auto aliased = edgeLevels (1);
    RecordProperty ("antialiasedLevels", static_cast<int> (antialiased));
    RecordProperty ("aliasedLevels", static_cast<int> (aliased));

    EXPECT_GE (antialiased, 4u);
    EXPECT_LE (aliased, 2u);
}

TEST_F (SceneRendererMetalTests, MovingObjectKeepsStillPartsAntialiased)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 16);

    // A small quad spinning far below row 40, whose transform changes on every frame
    std::vector<Mesh::Vertex> vertices (4);
    vertices[0].position = { -0.15f, -0.15f, 0.0f };
    vertices[1].position = { 0.15f, -0.15f, 0.0f };
    vertices[2].position = { 0.15f, 0.15f, 0.0f };
    vertices[3].position = { -0.15f, 0.15f, 0.0f };
    for (auto& vertex : vertices)
        vertex.normal = { 0.0f, 0.0f, 1.0f };

    auto mesh = Mesh::Ptr (new Mesh ("spinner"));
    mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 });
    auto spinner = EntityNode::Ptr (new EntityNode ("spinner"));
    spinner->attach<MeshNode> (mesh);
    spinner->setPosition ({ -1.6f, -1.6f, 0.0f });
    scene.getRoot()->addChild (spinner);

    SceneRenderer renderer;
    Image image;
    for (int i = 0; i < 24; ++i)
    {
        spinner->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 0.0f, 1.0f }, 0.2f * static_cast<float> (i)));
        image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    }

    // Moving things never converge, but the still plate keeps accumulating
    EXPECT_FALSE (renderer.isConverged());

    const auto levels = countEdgeLevels (image);
    RecordProperty ("levelsWithMovingObject", static_cast<int> (levels));
    EXPECT_GE (levels, 4u);
}

TEST_F (SceneRendererMetalTests, RedrawnTextureRestartsAndShows)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    const auto upload = [] (const GpuTexture::Ptr& texture, uint8 r, uint8 g, uint8 b)
    {
        std::vector<uint8> texels;
        for (int i = 0; i < 4; ++i)
            texels.insert (texels.end(), { r, g, b, 255 });

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data);
    };

    auto gpuTexture = GpuTexture::create (device, GpuTextureDesc (2, 2, GpuTextureFormat::rgba8unorm));
    ASSERT_NE (gpuTexture, nullptr);
    ASSERT_TRUE (upload (gpuTexture, 255, 255, 255));

    auto texture = Texture::Ptr (new Texture (gpuTexture));
    auto material = Material::Ptr (new Material());
    material->baseColorTexture = texture;
    scene.getRoot()->findChild ("plates")->attach<MaterialNode> (material);

    SceneRenderer renderer;
    Image image;
    for (int i = 0; i < 4; ++i)
        image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (renderer.isConverged()) << renderer.getLastError();
    EXPECT_NEAR (image.getPixelColor (32, 32).getRed(), image.getPixelColor (32, 32).getGreen(), 4);

    // Redrawn in place: the same GpuTexture handed over again
    ASSERT_TRUE (upload (gpuTexture, 255, 0, 0));
    texture->setGpuTexture (gpuTexture);

    image = renderToImage (renderer, scene, 64);
    EXPECT_FALSE (renderer.isConverged());

    for (int i = 0; i < 4 && ! renderer.isConverged(); ++i)
        image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (image.getPixelColor (32, 32).getRed(), image.getPixelColor (32, 32).getGreen() + 60);
}

TEST_F (SceneRendererMetalTests, SwappingTransformsOfTwoMeshesIsAChange)
{
    Scene scene;
    setFullTierTaa (scene, 4);

    // Two emissive quads, orange on the left and blue on the right
    const auto addQuad = [&] (const char* name, std::array<float, 3> emissive, float x)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.4f, -0.4f, 0.0f };
        vertices[1].position = { 0.4f, -0.4f, 0.0f };
        vertices[2].position = { 0.4f, 0.4f, 0.0f };
        vertices[3].position = { -0.4f, 0.4f, 0.0f };
        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->emissiveFactor = emissive;

        auto mesh = Mesh::Ptr (new Mesh (name));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);
        auto entity = EntityNode::Ptr (new EntityNode (name));
        entity->attach<MeshNode> (mesh);
        entity->setPosition ({ x, 0.0f, 0.0f });
        scene.getRoot()->addChild (entity);
        return entity;
    };

    auto orange = addQuad ("orange", { 1.0f, 0.5f, 0.1f }, -0.6f);
    auto blue = addQuad ("blue", { 0.1f, 0.2f, 1.0f }, 0.6f);

    SceneRenderer renderer;
    Image image;
    for (int i = 0; i < 4; ++i)
        image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (renderer.isConverged()) << renderer.getLastError();

    const auto isBlue = [] (Color color) { return color.getBlue() > color.getRed() + 60; };
    ASSERT_FALSE (isBlue (image.getPixelColor (20, 32)));
    ASSERT_TRUE (isBlue (image.getPixelColor (44, 32)));

    // The same transforms, the same meshes and materials, but paired the other way round
    orange->setPosition ({ 0.6f, 0.0f, 0.0f });
    blue->setPosition ({ -0.6f, 0.0f, 0.0f });

    image = renderToImage (renderer, scene, 64);
    EXPECT_FALSE (renderer.isConverged());

    for (int i = 0; i < 4 && ! renderer.isConverged(); ++i)
        image = renderToImage (renderer, scene, 64);

    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (isBlue (image.getPixelColor (20, 32)));
    EXPECT_FALSE (isBlue (image.getPixelColor (44, 32)));
}

TEST_F (SceneRendererMetalTests, RestartAccumulationStartsOver)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    renderer.restartAccumulation(); // Nothing rendered yet: nothing to restart

    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    renderer.restartAccumulation();
    EXPECT_FALSE (renderer.isConverged());

    for (int i = 1; i <= 4; ++i)
    {
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (renderer.isConverged(), i == 4) << "render " << i;
    }
}

TEST_F (SceneRendererMetalTests, LightRadiusChangeRestartsTheFullTier)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    auto* light = scene.getRoot()->findChild ("sun")->getNode<LightNode>();
    ASSERT_NE (light, nullptr);

    SceneRenderer renderer;
    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    light->radius = 0.05f;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, KhrMaterialChangesRestartAccumulation)
{
    const std::pair<const char*, std::function<void (Material&)>> changes[] = {
        { "emissiveStrength", [] (Material& material) { material.emissiveStrength = 2.0f; } },
        { "ior", [] (Material& material) { material.ior = 1.33f; } },
        { "specularFactor", [] (Material& material) { material.specularFactor = 0.5f; } },
        { "specularColorFactor", [] (Material& material) { material.specularColorFactor[1] = 0.5f; } },
    };

    for (const auto& [name, change] : changes)
    {
        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;
        material->emissiveFactor = { 0.1f, 0.1f, 0.1f };

        Scene scene;
        addPlate (scene, material);
        setFullTierTaa (scene, 4);

        SceneRenderer renderer;
        for (int i = 0; i < 4; ++i)
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.isConverged()) << name;

        change (*material);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_FALSE (renderer.isConverged()) << name;
    }
}

TEST_F (SceneRendererMetalTests, SilhouetteEdgesMatchRasterOnBothTiers)
{
    Scene scene;
    buildSilhouettesScene (scene);
    setTier (scene, RenderSettings::TierPreference::raster);

    // The reference: raster with bloom off, tone mapped per sample and resolved in display space
    SceneRenderer rasterRenderer;
    const auto raster = renderToImage (rasterRenderer, scene, 128);
    ASSERT_TRUE (raster.isValid()) << rasterRenderer.getLastError();

    // Raster with bloom at zero intensity: the half-float path through the same samples
    auto settings = scene.getRenderSettings();
    settings.bloom = true;
    settings.bloomIntensity = 0.0f;
    scene.setRenderSettings (settings);

    SceneRenderer bloomRenderer;
    const auto bloom = renderToImage (bloomRenderer, scene, 128);
    ASSERT_TRUE (bloom.isValid()) << bloomRenderer.getLastError();
    ASSERT_EQ (bloomRenderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float);

    // The full tier, converged: its jittered frames estimate each pixel's coverage differently
    setBloom (scene, false);
    setFullTierTaa (scene, 16);

    SceneRenderer fullRenderer;
    Image full;
    for (int i = 0; i < 16; ++i)
        full = renderToImage (fullRenderer, scene, 128);
    ASSERT_TRUE (full.isValid()) << fullRenderer.getLastError();
    ASSERT_TRUE (fullRenderer.isConverged());

    // The same samples as the reference: every pixel matches
    const auto bloomDiffering = countDifferences (bloom, raster);
    RecordProperty ("bloomDiffering", bloomDiffering);
    EXPECT_EQ (bloomDiffering, 0);

    // Other samples: the mean over the silhouettes, where the coverage averages out. Each pixel
    // rounds to its own 8 bit level on each path, so the means may differ by a fraction of a level;
    // tone mapping the premultiplied color instead moved them by 2.3 to 10.8 levels
    for (const auto& [name, left, right] : { std::tuple { "bright", 0, 64 }, std::tuple { "gray", 64, 128 } })
    {
        const auto reference = silhouetteBandLuminance (raster, raster, left, right);
        const auto fullBand = silhouetteBandLuminance (full, raster, left, right);
        const auto bloomBand = silhouetteBandLuminance (bloom, raster, left, right);
        RecordProperty ((String (name) + "Raster").toStdString(), String (reference, 2).toStdString());
        RecordProperty ((String (name) + "Full").toStdString(), String (fullBand, 2).toStdString());
        RecordProperty ((String (name) + "RasterBloom").toStdString(), String (bloomBand, 2).toStdString());

        EXPECT_NEAR (fullBand, reference, 1.0) << name;
        EXPECT_NEAR (bloomBand, reference, 1.0) << name;
    }
}

TEST_F (SceneRendererMetalTests, FullRasterFullRoundTripStartsOver)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer reference;
    Image converged;
    for (int i = 0; i < 8; ++i)
        converged = renderToImage (reference, scene, 128);
    ASSERT_TRUE (converged.isValid()) << reference.getLastError();
    ASSERT_TRUE (reference.isConverged());

    SceneRenderer renderer;
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    setTier (scene, RenderSettings::TierPreference::raster);
    ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_EQ (renderer.getCapabilities()->getTier(), RenderCapabilities::Tier::raster);

    // Back on the full tier the history is gone: the first frame stands alone, so no pixel of its
    // silhouettes is partially covered yet
    setTier (scene, RenderSettings::TierPreference::full);
    auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());
    EXPECT_EQ (countIntermediatePixels (image, 40), 0);

    for (int i = 2; i <= 8; ++i)
    {
        image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.isConverged(), i == 8) << "render " << i;
    }

    // The jitter sequence continued through the raster frame, so edges may differ by a level or two
    const auto differing = countDifferences (image, converged);
    RecordProperty ("differingFromAFreshRenderer", differing);
    EXPECT_LE (differing, 16);
    EXPECT_GE (countIntermediatePixels (image, 40), 4);
}

TEST_F (SceneRendererMetalTests, ToneMappingChangeKeepsTheFullTierConverged)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 8);

    SceneRenderer renderer;
    Image converged;
    for (int i = 0; i < 8; ++i)
        converged = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (converged.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // Tone mapping runs after the accumulation: the converged history shows through the new operator
    scene.setToneMapping (Scene::ToneMapping::aces);
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_GT (countDifferences (image, converged), 1000);
    EXPECT_GE (countIntermediatePixels (image, 40), 4);
}

TEST_F (SceneRendererMetalTests, ExposureAndLightChangesRestartButKeepTheHistory)
{
    // Each change sets a value, applied to the edges scene converged with 8 TAA samples
    const std::pair<const char*, std::function<void (Scene&, float)>> changes[] = {
        { "exposure", [] (Scene& scene, float value) { scene.setExposure (value); } },
        { "light", [] (Scene& scene, float value) { scene.getRoot()->findChild ("sun")->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, value)); } },
    };

    const std::map<String, std::pair<float, float>> values { { "exposure", { 1.0f, 1.5f } }, { "light", { 0.0f, 0.4f } } };

    // Converges at @p before, changes to @p after and renders once, then until converged again
    const auto renderChange = [&] (const std::function<void (Scene&, float)>& change, float before, float after, bool restart, Image& first)
    {
        Scene scene;
        buildEdgesScene (scene);
        setFullTierTaa (scene, 8);
        change (scene, before);

        SceneRenderer renderer;
        for (int i = 0; i < 8; ++i)
            EXPECT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.isConverged());

        change (scene, after);
        if (restart)
            renderer.restartAccumulation();

        first = renderToImage (renderer, scene, 128);
        EXPECT_FALSE (renderer.isConverged());

        Image converged;
        for (int i = 0; i < 8 && ! renderer.isConverged(); ++i)
            converged = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (renderer.isConverged());
        return converged;
    };

    for (const auto& [name, change] : changes)
    {
        const auto [before, after] = values.at (name);

        // The signal is the first frame after the change. Blended into the kept history, its
        // silhouettes still have partially covered pixels. Started over from that frame alone, as
        // restartAccumulation() does, every pixel is either covered or not
        Image keptFirst, restartedFirst;
        const auto converged = renderChange (change, before, after, false, keptFirst);
        renderChange (change, before, after, true, restartedFirst);
        ASSERT_TRUE (keptFirst.isValid() && restartedFirst.isValid() && converged.isValid()) << name;

        RecordProperty ((name + String ("Kept")).toStdString(), countIntermediatePixels (keptFirst, 40));
        RecordProperty ((name + String ("Restarted")).toStdString(), countIntermediatePixels (restartedFirst, 40));
        EXPECT_GE (countIntermediatePixels (keptFirst, 40), 4) << name;
        EXPECT_EQ (countIntermediatePixels (restartedFirst, 40), 0) << name;

        // Converged again, what remains of the image before the change. The control goes through
        // the same frames and the same restart, from a value too close to tell apart, so only the
        // remains of the change differ. Counting the history as one sample once the scene is still
        // leaves about 1/8 of the change on edge pixels, where the clip box is wide
        Image controlFirst;
        const auto control = renderChange (change, after + 1.0e-4f, after, false, controlFirst);
        ASSERT_TRUE (control.isValid()) << name;

        int largest = 0;
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 128; ++x)
                largest = jmax (largest, std::abs (static_cast<int> (converged.getPixelColor (x, y).getRed()) - static_cast<int> (control.getPixelColor (x, y).getRed())));

        // Keeping the history's blend weight instead left 236 pixels beyond 2 levels, up to 7
        const auto differing = countDifferences (converged, control);
        RecordProperty ((name + String ("DifferingFromControl")).toStdString(), differing);
        RecordProperty ((name + String ("LargestDifference")).toStdString(), largest);
        EXPECT_LE (differing, 16) << name;
        EXPECT_LE (largest, 4) << name;
    }
}

TEST_F (SceneRendererMetalTests, UncoveredPixelsShowTheBackgroundExactly)
{
    // The plate moves right: the pixels it leaves keep no trace of it, because the history
    // coverage is clamped to the coverage around them
    const auto background = Color (static_cast<uint8> (200), static_cast<uint8> (80), static_cast<uint8> (40));
    const auto isBackground = [&] (Color color)
    {
        return std::abs (color.getRed() - background.getRed()) <= 1
            && std::abs (color.getGreen() - background.getGreen()) <= 1
            && std::abs (color.getBlue() - background.getBlue()) <= 1;
    };

    Scene scene;
    buildEdgesScene (scene);
    scene.setBackgroundColor (background);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    Image image;
    for (int i = 0; i < 4; ++i)
        image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (renderer.isConverged()) << renderer.getLastError();

    // Just inside the left silhouette on row 64
    EXPECT_FALSE (isBackground (image.getPixelColor (34, 64)));

    scene.getRoot()->findChild ("plates")->setPosition ({ 0.5f, 0.0f, 0.0f });

    for (int frame = 1; frame <= 6; ++frame)
    {
        image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto color = image.getPixelColor (34, 64);
        EXPECT_TRUE (isBackground (color)) << "frame " << frame << ": " << static_cast<int> (color.getRed()) << ","
                                           << static_cast<int> (color.getGreen()) << "," << static_cast<int> (color.getBlue());
    }

    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, CameraMotionReorderingBlendedItemsKeepsTheHistory)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 16);

    // Two blended quads below row 40, 0.005 apart in depth: turning the camera by 0.01 swaps
    // which one is farther, and so their drawing order
    auto blended = Material::Ptr (new Material());
    blended->baseColorFactor = { 0.2f, 0.2f, 0.9f, 0.5f };
    blended->alphaMode = Material::AlphaMode::blend;

    for (const auto& [x, z] : { std::pair { -0.5f, 0.0f }, std::pair { 0.5f, -0.005f } })
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.2f, -0.2f, 0.0f };
        vertices[1].position = { 0.2f, -0.2f, 0.0f };
        vertices[2].position = { 0.2f, 0.2f, 0.0f };
        vertices[3].position = { -0.2f, 0.2f, 0.0f };
        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("blended"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, blended);
        auto quad = EntityNode::Ptr (new EntityNode ("blended"));
        quad->attach<MeshNode> (mesh);
        quad->setPosition ({ x, -0.7f, 0.6f + z });
        scene.getRoot()->addChild (quad);
    }

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    SceneRenderer renderer;
    for (int i = 0; i < 16; ++i)
        ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    camera->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.01f));
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    const auto levels = countEdgeLevels (image);
    RecordProperty ("levelsAfterReorder", static_cast<int> (levels));
    EXPECT_GE (levels, 4u);
}

TEST_F (SceneRendererMetalTests, CameraMotionKeepsTheHistory)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 16);

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    SceneRenderer renderer;
    for (int i = 0; i < 16; ++i)
        ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    // The first frame after a small camera move blends into the reprojected history, so the edges
    // stay antialiased; starting over from that frame alone would alias them
    camera->setPosition ({ 0.01f, 0.0f, 5.0f });
    const auto image = renderToImage (renderer, scene, 128);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());

    const auto levels = countEdgeLevels (image);
    RecordProperty ("levelsAfterCameraMove", static_cast<int> (levels));
    EXPECT_GE (levels, 4u);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_F (SceneRendererMetalTests, NonFiniteCheckFindsNanAndInfinity)
{
    // Half floats: 1.0, NaN and +infinity
    const auto check = [this] (uint16 bits)
    {
        std::vector<uint16> texels (4 * 4 * 4, 0x3c00);
        texels[5 * 4 + 1] = bits;

        auto texture = GpuTexture::create (device, GpuTextureDesc (4, 4, GpuTextureFormat::rgba16float));
        if (texture == nullptr)
            return false;

        GpuTextureDataDesc data;
        data.data = texels.data();
        return texture->upload (data) && isFiniteTexture (texture);
    };

    EXPECT_TRUE (check (0x3c00));
    EXPECT_FALSE (check (0x7e00));
    EXPECT_FALSE (check (0x7c00));
}

TEST_F (SceneRendererMetalTests, VelocityIsZeroForAStaticSceneAndCameraMotionOtherwise)
{
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);

    auto camera = EntityNode::Ptr (new EntityNode ("camera"));
    camera->attach<CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 5.0f });
    scene.getRoot()->addChild (camera);
    scene.setActiveCamera (camera.get());

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto still = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (still.isValid());
    EXPECT_EQ (maxVelocityDeviation (still), 0);

    // Velocity is current uv minus previous uv, with uv from the top-left. Moving the camera right
    // moves the scene left on screen: x decreases, y is unchanged
    camera->setPosition ({ 0.1f, 0.0f, 5.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto right = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (right.isValid());
    EXPECT_LT (right.getPixelColor (32, 32).getRed(), 64);
    EXPECT_EQ (velocityDeviation (right.getPixelColor (32, 32).getGreen()), 0);

    // Moving the camera up moves the scene down on screen, towards larger uv.y: a missing y flip fails here
    camera->setPosition ({ 0.1f, 0.1f, 5.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto up = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (up.isValid());
    EXPECT_GT (up.getPixelColor (32, 32).getGreen(), 192);
    EXPECT_EQ (velocityDeviation (up.getPixelColor (32, 32).getRed()), 0);
}

TEST_F (SceneRendererMetalTests, TwoRenderersKeepIndependentHistory)
{
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);

    auto* plates = scene.getRoot()->findChild ("plates");
    ASSERT_NE (plates, nullptr);

    const auto velocityAfterMove = [&] (SceneRenderer& renderer, float x)
    {
        plates->setPosition ({ x, 0.0f, 0.0f });
        if (renderer.render (device, scene, 64, 64) == nullptr)
            return -1;

        const auto velocity = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
        return velocity.isValid() ? maxVelocityDeviation (velocity) : -1;
    };

    // A single renderer sees the plates move between its frames
    SceneRenderer single;
    EXPECT_EQ (velocityAfterMove (single, 0.0f), 0);
    EXPECT_GT (velocityAfterMove (single, 0.1f), 64);

    // Alternating renderers never take the other's transform as their previous frame
    SceneRenderer a;
    SceneRenderer b;
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ (velocityAfterMove (a, 0.1f * static_cast<float> (2 * i)), 0) << "a, frame " << i;
        EXPECT_EQ (velocityAfterMove (b, 0.1f * static_cast<float> (2 * i + 1)), 0) << "b, frame " << i;
    }
}

TEST_F (SceneRendererMetalTests, FirstFrameHasNoObjectMotion)
{
    // A transform set before the first render is not motion, whatever the node held before
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);
    scene.getRoot()->findChild ("plates")->setPosition ({ 0.3f, 0.2f, 0.0f });

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto velocity = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (velocity.isValid());
    EXPECT_EQ (maxVelocityDeviation (velocity), 0);
}

TEST_F (SceneRendererMetalTests, NodeReappearingMovedHasNoMotion)
{
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);

    auto* plates = scene.getRoot()->findChild ("plates");
    ASSERT_NE (plates, nullptr);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    // Hidden for one frame, then shown somewhere else: its last drawn transform is two frames old
    plates->setVisible (false);
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    plates->setVisible (true);
    plates->setPosition ({ 0.1f, 0.0f, 0.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto velocity = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (velocity.isValid());
    EXPECT_EQ (maxVelocityDeviation (velocity), 0);
}

TEST_F (SceneRendererMetalTests, FailedRenderClearsThePassOutputs)
{
    Scene scene;
    buildShadowsScene (scene);
    setTier (scene, RenderSettings::TierPreference::full);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);
    EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), nullptr);

    EXPECT_EQ (renderer.render (device, scene, 0, 64), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);
    EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), nullptr);

    // Rendering again would fail again: nothing to wait for
    EXPECT_TRUE (renderer.isConverged());
}

TEST_F (SceneRendererMetalTests, ResizeResetsAccumulation)
{
    Scene scene;
    buildEdgesScene (scene);
    setFullTierTaa (scene, 4);

    SceneRenderer renderer;
    for (int i = 0; i < 4; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());

    ASSERT_NE (renderer.render (device, scene, 80, 80), nullptr) << renderer.getLastError();
    EXPECT_FALSE (renderer.isConverged());

    const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
    ASSERT_NE (sceneColor, nullptr);
    EXPECT_EQ (sceneColor->getWidth(), 80);
    EXPECT_TRUE (isFiniteTexture (sceneColor));
}

TEST_F (SceneRendererMetalTests, NonFiniteInputDoesNotPoisonHistory)
{
    // A huge point light just above a plate corner, a zero-area triangle, and a blended quad with
    // zero normals, a NaN emissive factor and a NaN alpha. Metal saturates overflowing half floats,
    // and max() and the clamp of the shaded color drop NaN, so the alpha is what reliably reaches
    // the scene color, through the blending
    const auto buildScene = [] (Scene& scene, int taaSamples)
    {
        scene.getRoot()->addChild (makePlates());

        std::vector<Mesh::Vertex> vertices (7);
        for (auto& vertex : vertices)
        {
            vertex.position = { 0.2f, 0.2f, 0.6f };
            vertex.normal = {};
        }

        vertices[3].position = { -0.6f, -0.6f, 0.6f };
        vertices[4].position = { -0.4f, -0.6f, 0.6f };
        vertices[5].position = { -0.4f, -0.4f, 0.6f };
        vertices[6].position = { -0.6f, -0.4f, 0.6f };

        auto material = Material::Ptr (new Material());
        material->emissiveFactor = { std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.0f };
        material->baseColorFactor[3] = std::numeric_limits<float>::quiet_NaN();
        material->alphaMode = Material::AlphaMode::blend;

        auto mesh = Mesh::Ptr (new Mesh ("degenerate"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 3, 4, 5, 3, 5, 6 }, material);
        auto degenerate = EntityNode::Ptr (new EntityNode ("degenerate"));
        degenerate->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (degenerate);

        auto light = EntityNode::Ptr (new EntityNode ("point"));
        auto& node = light->attach<LightNode>();
        node.type = LightNode::Type::point;
        node.range = 0.0f;
        node.intensity = 1.0e8f;
        light->setPosition ({ 1.0f, 1.0f, 0.01f });
        scene.getRoot()->addChild (light);

        setFullTierTaa (scene, taaSamples);
    };

    // Without temporal anti-aliasing the scene color really holds non-finite values
    {
        Scene scene;
        buildScene (scene, 1);

        SceneRenderer renderer;
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_FALSE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));
    }

    Scene scene;
    buildScene (scene, 16);

    SceneRenderer renderer;
    for (int i = 0; i < 8; ++i)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));
}

TEST_F (SceneRendererMetalTests, FullTierWithoutTaaKeepsOverflowingHighlightsFinite)
{
    // Without TAA or bloom nothing sanitizes the scene color before tone mapping. Metal saturates
    // half floats written above their range, so radiance just past 65504 can't show the overflow
    // here; a light this strong overflows the 32 bit shading math itself to infinity
    Scene scene;
    scene.getRoot()->addChild (makePlates());

    auto light = EntityNode::Ptr (new EntityNode ("point"));
    auto& node = light->attach<LightNode>();
    node.type = LightNode::Type::point;
    node.range = 0.0f;
    node.intensity = 1.0e38f;
    light->setPosition ({ 0.5f, 0.5f, 0.05f });
    scene.getRoot()->addChild (light);

    setFullTierTaa (scene, 1);

    SceneRenderer renderer;
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    ASSERT_FALSE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::temporalAntiAliasing, scene.getRenderSettings()));
    EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));
}
#endif

class SceneRendererTierTests : public SceneRendererMetalTests
    , public ::testing::WithParamInterface<RenderSettings::TierPreference>
{
protected:
#if YUP_ENABLE_SHADER_TRANSPILER
    /** Renders a 2x2 plate, u along +x and v down, lit by a sun turned by @p sunRotation, and returns
        the mean luma of its middle half at 128x128. The plate uses @p tangent at every vertex, a 1x1
        normal map of @p normalTexel if there is one, and an x scale of @p xScale.
        Scene color holds raw radiance and is checked for non-finite and saturated values.
    */
    double renderNormalMappedPlate (std::array<float, 4> tangent, std::optional<Color> normalTexel, const Quaternion& sunRotation, float xScale = 1.0f)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -1.0f, -1.0f, 0.0f };
        vertices[1].position = { 1.0f, -1.0f, 0.0f };
        vertices[2].position = { 1.0f, 1.0f, 0.0f };
        vertices[3].position = { -1.0f, 1.0f, 0.0f };
        vertices[0].uv = { 0.0f, 1.0f };
        vertices[1].uv = { 1.0f, 1.0f };
        vertices[2].uv = { 1.0f, 0.0f };
        vertices[3].uv = { 0.0f, 0.0f };

        for (auto& vertex : vertices)
        {
            vertex.normal = { 0.0f, 0.0f, 1.0f };
            vertex.tangent = tangent;
        }

        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;

        if (normalTexel.has_value())
        {
            Image normal (1, 1, PixelFormat::RGBA);
            normal.setPixelColor (0, 0, *normalTexel);
            material->normalTexture = Texture::Ptr (new Texture (std::move (normal)));
        }

        auto mesh = Mesh::Ptr (new Mesh ("plate"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

        Scene scene;
        auto plate = EntityNode::Ptr (new EntityNode ("plate"));
        plate->attach<MeshNode> (mesh);
        plate->setScale ({ xScale, 1.0f, 1.0f });
        scene.getRoot()->addChild (plate);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>();
        sun->setRotation (sunRotation);
        scene.getRoot()->addChild (sun);

        // Scene color must hold raw radiance: no temporal anti-aliasing on full, rgba16float on raster
        if (GetParam() == RenderSettings::TierPreference::full)
            setFullTierTaa (scene, 1);
        else
        {
            setTier (scene, RenderSettings::TierPreference::raster);
            setBloom (scene, true);
        }

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        if (! image.isValid() || sceneColor == nullptr)
        {
            ADD_FAILURE() << renderer.getLastError();
            return 0.0;
        }

        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));

        // The plate covers about 60 px of the frame
        return meanLuminance (image, { 49, 49, 30, 30 });
    }
#endif
};

TEST_P (SceneRendererTierTests, ReportsTheRequestedTier)
{
    Scene scene;
    buildEdgesScene (scene);
    setTier (scene, GetParam());

    SceneRenderer renderer;
    EXPECT_EQ (renderer.getCapabilities(), nullptr);

    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto* capabilities = renderer.getCapabilities();
    ASSERT_NE (capabilities, nullptr);
    EXPECT_EQ (capabilities->getTier(), GetParam() == RenderSettings::TierPreference::full ? RenderCapabilities::Tier::full : RenderCapabilities::Tier::raster);
}

TEST_P (SceneRendererTierTests, BloomBrightensAroundAnEmissiveQuad)
{
    const auto tierName = GetParam() == RenderSettings::TierPreference::full ? String ("full") : String ("raster");

    Scene scene;
    buildGlowScene (scene);
    setTier (scene, GetParam());

    SceneRenderer withoutBloom;
    const auto off = renderToImage (withoutBloom, scene, 128);
    ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

    const auto quad = findBrightBounds (off);
    ASSERT_GE (quad.getWidth(), 12);
    ASSERT_GE (quad.getHeight(), 12);

    setBloom (scene, true);

    SceneRenderer withBloom;
    const auto on = renderToImage (withBloom, scene, 128);
    ASSERT_TRUE (on.isValid()) << withBloom.getLastError();
    EXPECT_TRUE (withBloom.getCapabilities()->isEnabled (RenderCapabilities::Feature::bloom, scene.getRenderSettings()));

    const auto ringOff = ringLuminance (off, quad);
    const auto ringOn = ringLuminance (on, quad);
    const auto inside = meanLuminance (on, quad.reduced (2));
    RecordProperty ((tierName + "RingOff").toStdString(), String (ringOff, 2).toStdString());
    RecordProperty ((tierName + "RingOn").toStdString(), String (ringOn, 2).toStdString());
    RecordProperty ((tierName + "Inside").toStdString(), String (inside, 2).toStdString());

    EXPECT_LT (ringOff, 1.0);
    EXPECT_GE (ringOn, 5.0);
    EXPECT_GE (inside, 245.0);
}

TEST_P (SceneRendererTierTests, BloomThresholdAboveTheHighlightAddsNoGlow)
{
    Scene scene;
    buildGlowScene (scene);
    setTier (scene, GetParam());

    SceneRenderer withoutBloom;
    const auto off = renderToImage (withoutBloom, scene, 128);
    ASSERT_TRUE (off.isValid()) << withoutBloom.getLastError();

    // The quad emits 20: a threshold of 50 keeps nothing, even with the soft knee from 25
    auto settings = scene.getRenderSettings();
    settings.bloom = true;
    settings.bloomThreshold = 50.0f;
    settings.bloomIntensity = 1.0f;
    scene.setRenderSettings (settings);

    SceneRenderer withBloom;
    const auto on = renderToImage (withBloom, scene, 128);
    ASSERT_TRUE (on.isValid()) << withBloom.getLastError();
    ASSERT_TRUE (withBloom.getCapabilities()->isEnabled (RenderCapabilities::Feature::bloom, settings));

    const auto ring = ringLuminance (on, findBrightBounds (off));
    RecordProperty ("ring", String (ring, 2).toStdString());
    EXPECT_LT (ring, 1.0);
}

TEST_P (SceneRendererTierTests, EmissiveStrengthScalesEmission)
{
    // An unlit black metal plate: no diffuse and no reflectance, only its emission
    const auto renderCenter = [this] (float emission, float strength)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->emissiveFactor = { emission, emission, emission };
        material->emissiveStrength = strength;

        Scene scene;
        addPlate (scene, material);
        scene.setUsingDefaultLight (false);
        scene.setExposure (1.0f);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        setTier (scene, GetParam());

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return image.isValid() ? image.getPixelColor (32, 32) : Color();
    };

    const auto plain = renderCenter (0.1f, 1.0f);
    const auto strengthened = renderCenter (0.1f, 4.0f);
    const auto reference = renderCenter (0.4f, 1.0f);
    RecordProperty ("plain", static_cast<int> (plain.getRed()));
    RecordProperty ("strengthened", static_cast<int> (strengthened.getRed()));

    EXPECT_NEAR (strengthened.getRed(), reference.getRed(), 2);
    EXPECT_NEAR (strengthened.getGreen(), reference.getGreen(), 2);
    EXPECT_NEAR (strengthened.getBlue(), reference.getBlue(), 2);
    EXPECT_GT (strengthened.getRed(), plain.getRed() + 20);
}

TEST_P (SceneRendererTierTests, HigherIorBrightensDielectricSpecular)
{
    // A black dielectric only reflects: under a grazing sun, its highlight follows the reflectance
    const auto renderMean = [this] (float ior)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.5f;
        material->ior = ior;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 10.0f;
        sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 1.3f));
        scene.getRoot()->addChild (sun);
        setTier (scene, GetParam());

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();
        return image.isValid() ? meanLuminance (image, { 24, 24, 16, 16 }) : -1.0;
    };

    const auto water = renderMean (1.33f);
    const auto diamond = renderMean (2.4f);
    RecordProperty ("water", String (water, 2).toStdString());
    RecordProperty ("diamond", String (diamond, 2).toStdString());

    EXPECT_GT (water, 0.0);
    EXPECT_GT (diamond, water + 5.0);
}

TEST_P (SceneRendererTierTests, SpecularFactorAndColorShapeTheDielectricHighlight)
{
    // The highlight of HigherIorBrightensDielectricSpecular, as its mean red and green levels
    const auto renderHighlight = [this] (float specularFactor, std::array<float, 3> specularColor)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.5f;
        material->ior = 2.4f;
        material->specularFactor = specularFactor;
        material->specularColorFactor = specularColor;

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 10.0f;
        sun->setRotation (Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 1.3f));
        scene.getRoot()->addChild (sun);
        setTier (scene, GetParam());

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);
        EXPECT_TRUE (image.isValid()) << renderer.getLastError();

        std::pair<double, double> redGreen { -1.0, -1.0 };
        if (! image.isValid())
            return redGreen;

        redGreen = { 0.0, 0.0 };
        for (int y = 24; y < 40; ++y)
        {
            for (int x = 24; x < 40; ++x)
            {
                redGreen.first += image.getPixelColor (x, y).getRed() / 256.0;
                redGreen.second += image.getPixelColor (x, y).getGreen() / 256.0;
            }
        }

        return redGreen;
    };

    const auto white = renderHighlight (1.0f, { 1.0f, 1.0f, 1.0f });
    const auto none = renderHighlight (0.0f, { 1.0f, 1.0f, 1.0f });
    const auto red = renderHighlight (1.0f, { 1.0f, 0.0f, 0.0f });
    RecordProperty ("whiteRed", String (white.first, 2).toStdString());
    RecordProperty ("noneRed", String (none.first, 2).toStdString());
    RecordProperty ("redRed", String (red.first, 2).toStdString());
    RecordProperty ("redGreen", String (red.second, 2).toStdString());

    // A zero factor removes the reflection, grazing incidence included; a red color keeps only red
    EXPECT_GT (white.first, 20.0);
    EXPECT_LE (none.first, 1.0);
    EXPECT_GT (red.first, red.second + 20.0);
}

TEST_P (SceneRendererTierTests, SpecularFactorDimsTheDiffuseOnce)
{
    // An ior of 0 reflects f0 = f90 = specularFactor at every angle. The diffuse is what a white base
    // adds over a black one, whose specular is the same
    const auto diffuseAt = [&] (float specularFactor)
    {
        return renderHeadOnRadiance (makeDielectric (1.0f, 0.0f, specularFactor), GetParam())[1]
             - renderHeadOnRadiance (makeDielectric (0.0f, 0.0f, specularFactor), GetParam())[1];
    };

    const auto unreflected = diffuseAt (0.0f);
    const auto ratio = diffuseAt (0.5f) / unreflected;
    RecordProperty ("unreflected", String (unreflected, 3).toStdString());
    RecordProperty ("ratio", String (ratio, 3).toStdString());

    // 2 / pi without any specular; dimmed by 1 - 0.5, where counting the factor twice gives 1 - 0.25
    EXPECT_NEAR (unreflected, 2.0 / MathConstants<double>::pi, 0.03);
    EXPECT_NEAR (ratio, 0.5, 0.05);
}

TEST_P (SceneRendererTierTests, ColoredSpecularDimsEveryDiffuseChannel)
{
    // A red specular color at an ior of 0 reflects all the red: KHR_materials_specular takes the
    // largest channel, so nothing is left to the diffuse, in green either
    const auto white = renderHeadOnRadiance (makeDielectric (1.0f, 0.0f, 1.0f, { 1.0f, 0.0f, 0.0f }), GetParam());
    const auto black = renderHeadOnRadiance (makeDielectric (0.0f, 0.0f, 1.0f, { 1.0f, 0.0f, 0.0f }), GetParam());
    const auto greenDiffuse = white[1] - black[1];
    RecordProperty ("greenDiffuse", String (greenDiffuse, 3).toStdString());

    // Weighting each channel on its own leaves a green diffuse of about 2 / pi
    EXPECT_LT (greenDiffuse, 0.03);
    EXPECT_NEAR (black[0], 2.0 / 4.0 / MathConstants<double>::pi, 0.03); // The red specular, f0 = 1
}

TEST_P (SceneRendererTierTests, DiffuseLiesUnderTheDielectricLayerOnly)
{
    // Half metallic red: the metal reflects f0 = 1 in red, but the diffuse half sits under the
    // dielectric layer alone (f0 = 0.04), as in the glTF mix of the dielectric and metal BRDFs
    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 1.0f, 0.0f, 0.0f, 1.0f };
    material->metallicFactor = 0.5f;

    const auto red = renderHeadOnRadiance (material, GetParam())[0];
    const auto f0 = 0.5 * 0.04 + 0.5 * 1.0;
    const auto expected = 2.0 * (f0 / 4.0 + 0.5 * (1.0 - 0.04)) / MathConstants<double>::pi;
    RecordProperty ("red", String (red, 3).toStdString());
    RecordProperty ("expected", String (expected, 3).toStdString());

    // Dimming the diffuse by the mixed reflectance instead (1 - 0.52) gives 0.236
    EXPECT_NEAR (red, expected, 0.02);
}

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, RendersEveryBaselineSceneWithoutNonFiniteValues)
{
    const std::pair<const char*, void (*) (Scene&)> scenes[] = {
        { "shadows", buildShadowsScene },
        { "materials", buildMaterialsScene },
        { "edges", buildEdgesScene },
    };

    const auto full = GetParam() == RenderSettings::TierPreference::full;

    for (const auto& [name, build] : scenes)
    {
        Scene scene;
        build (scene);
        setTier (scene, GetParam());

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << name << ": " << renderer.getLastError();
        ASSERT_TRUE (image.isValid()) << name;

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        const auto velocity = renderer.getPassOutput (SceneRenderer::PassOutput::velocity);
        ASSERT_NE (sceneColor, nullptr) << name;
        EXPECT_EQ (sceneColor->getFormat(), full ? GpuTextureFormat::rgba16float : GpuTextureFormat::rgba8unorm) << name;
        EXPECT_EQ (velocity != nullptr, full) << name;

        if (full)
            EXPECT_TRUE (isFiniteTexture (sceneColor)) << name;
    }
}
#endif

#if YUP_ENABLE_SHADER_TRANSPILER
TEST_P (SceneRendererTierTests, ZeroTangentFallsBackToDerivativeFrame)
{
    // The normal map tilts the plate towards +x, where a grazing sun shines from: the tangent
    // (1, 0, 0, 1) is the frame the derivatives build
    const auto tiltedTowardsX = Color (191, 128, 247);
    const auto sunFromX = Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 1.2f);

    const auto tangentFrame = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsX, sunFromX);
    const auto derivativeFrame = renderNormalMappedPlate ({ 0.0f, 0.0f, 0.0f, 0.0f }, tiltedTowardsX, sunFromX);
    const auto unmapped = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, std::nullopt, sunFromX);
    const auto flipped = renderNormalMappedPlate ({ -1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsX, sunFromX);
    RecordProperty ("tangentFrame", String (tangentFrame, 2).toStdString());
    RecordProperty ("derivativeFrame", String (derivativeFrame, 2).toStdString());
    RecordProperty ("unmapped", String (unmapped, 2).toStdString());
    RecordProperty ("flipped", String (flipped, 2).toStdString());

    EXPECT_NEAR (derivativeFrame, tangentFrame, tangentFrame * 0.02);

    // Both frames really apply the map
    EXPECT_GT (tangentFrame, unmapped * 1.1);

    // Authored tangents are really used: reversed, they tilt the plate away from the sun
    EXPECT_LT (flipped, unmapped);
}

TEST_P (SceneRendererTierTests, BitangentHandednessSurvivesMirroring)
{
    // The normal map tilts the plate towards +y (decreasing v), where a grazing sun shines from:
    // only the bitangent, and so the handedness, decides the shading
    const auto tiltedTowardsY = Color (128, 191, 247);
    const auto sunFromY = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -1.2f);

    const auto tangentFrame = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsY, sunFromY);
    const auto derivativeFrame = renderNormalMappedPlate ({ 0.0f, 0.0f, 0.0f, 0.0f }, tiltedTowardsY, sunFromY);
    const auto unmapped = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, std::nullopt, sunFromY);
    const auto leftHanded = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, -1.0f }, tiltedTowardsY, sunFromY);
    const auto mirroredTangentFrame = renderNormalMappedPlate ({ 1.0f, 0.0f, 0.0f, 1.0f }, tiltedTowardsY, sunFromY, -1.0f);
    const auto mirroredDerivativeFrame = renderNormalMappedPlate ({ 0.0f, 0.0f, 0.0f, 0.0f }, tiltedTowardsY, sunFromY, -1.0f);
    RecordProperty ("tangentFrame", String (tangentFrame, 2).toStdString());
    RecordProperty ("derivativeFrame", String (derivativeFrame, 2).toStdString());
    RecordProperty ("unmapped", String (unmapped, 2).toStdString());
    RecordProperty ("leftHanded", String (leftHanded, 2).toStdString());
    RecordProperty ("mirroredTangentFrame", String (mirroredTangentFrame, 2).toStdString());
    RecordProperty ("mirroredDerivativeFrame", String (mirroredDerivativeFrame, 2).toStdString());

    EXPECT_NEAR (derivativeFrame, tangentFrame, tangentFrame * 0.02);
    EXPECT_GT (tangentFrame, unmapped * 1.1);

    // w = -1 turns the bitangent, and the tilt, away from the sun
    EXPECT_LT (leftHanded, unmapped);

    // A mirroring transform keeps the bitangent pointing up the texture, with or without tangents
    EXPECT_NEAR (mirroredTangentFrame, tangentFrame, tangentFrame * 0.02);
    EXPECT_NEAR (mirroredDerivativeFrame, tangentFrame, tangentFrame * 0.02);
}
#endif

INSTANTIATE_TEST_SUITE_P (SceneRendererTiers,
                          SceneRendererTierTests,
                          ::testing::Values (RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full),
                          [] (const auto& info)
                          {
                              return info.param == RenderSettings::TierPreference::full ? "full" : "raster";
                          });

#endif

#if YUP_LINUX
class SceneRendererGLTests : public SceneRendererBaselineScenes
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

        window = SDL_CreateWindow ("yup_3d_gl_test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
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
    }

    void TearDown() override
    {
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

TEST_F (SceneRendererGLTests, CapturesBaselineScenes)
{
    captureBaselineScenes ("gl");
}
#endif
