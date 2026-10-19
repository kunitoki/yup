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

#pragma once

#include <gtest/gtest.h>

#include <yup_renderer/yup_renderer.h>

#if YUP_LINUX
#include <SDL3/SDL.h>

#include <cstdlib>
#endif

#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <map>
#include <numeric>
#include <optional>
#include <set>

using namespace yup;

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

    /** Places @p entity at @p eye with its -Z axis, which cameras look along and lights shine along, towards @p target. */
    static void aimCamera (EntityNode& entity, Vector3<float> eye, Vector3<float> target, Vector3<float> up = { 0.0f, 1.0f, 0.0f })
    {
        entity.setPosition (eye);
        entity.setRotation (Quaternion::fromRotationMatrix (Matrix4::lookAt (eye, target, up).inverted()));
    }

    /** A rough light gray dielectric, for the grounds, the walls and the blockers of the shadow scenes. */
    static Material::Ptr makeWallMaterial()
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 1.0f;
        return material;
    }

    /** A square ground at y = 0, 2 * @p halfSize wide, drawn with makeWallMaterial(): the entity "ground". */
    static EntityNode* addGround (Scene& scene, float halfSize)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -halfSize, 0.0f, halfSize };
        vertices[1].position = { halfSize, 0.0f, halfSize };
        vertices[2].position = { halfSize, 0.0f, -halfSize };
        vertices[3].position = { -halfSize, 0.0f, -halfSize };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 1.0f, 0.0f };

        auto mesh = Mesh::Ptr (new Mesh ("ground"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeWallMaterial());

        auto ground = EntityNode::Ptr (new EntityNode ("ground"));
        ground->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (ground);
        return ground.get();
    }

    /** The ground of the long shadow scenes: a 200 x 200 rough dielectric quad at y = 0, lit by a
        shadow casting sun tilted 0.6 rad from vertical, so shadows fall towards +x, under an ambient
        light of 0.05 and no environment. Returns the sun.
    */
    static LightNode& addLongGround (Scene& scene)
    {
        addGround (scene, 100.0f);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        auto& light = sun->attach<LightNode>();
        light.castsShadows = true;
        aimCamera (*sun, { -10.0f * std::sin (0.6f), 10.0f * std::cos (0.6f), 0.0f }, {});
        scene.getRoot()->addChild (sun);

        scene.setAmbientColor (Color (0xff3f3f3f)); // 0.05 once linear
        return light;
    }

    /** A cube of half size @p halfSize centered on the origin, its faces wound counterclockwise seen
        from outside, or facing its center when @p inward is set (a closed room). */
    static Mesh::Ptr makeBox (float halfSize, Material::Ptr material, bool inward = false)
    {
        // Each face: its normal n and two axes u, v with u x v = n
        const std::array<std::array<Vector3<float>, 3>, 6> faces { {
            { Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 0.0f, -1.0f), Vector3<float> (0.0f, 1.0f, 0.0f) },
            { Vector3<float> (-1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 0.0f, 1.0f), Vector3<float> (0.0f, 1.0f, 0.0f) },
            { Vector3<float> (0.0f, 1.0f, 0.0f), Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 0.0f, -1.0f) },
            { Vector3<float> (0.0f, -1.0f, 0.0f), Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 0.0f, 1.0f) },
            { Vector3<float> (0.0f, 0.0f, 1.0f), Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 1.0f, 0.0f) },
            { Vector3<float> (0.0f, 0.0f, -1.0f), Vector3<float> (-1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 1.0f, 0.0f) },
        } };

        std::vector<Mesh::Vertex> vertices;
        std::vector<uint32> indices;

        for (const auto& [n, u, v] : faces)
        {
            const auto first = static_cast<uint32> (vertices.size());
            for (const auto& [a, b] : { std::pair { -1.0f, -1.0f }, std::pair { 1.0f, -1.0f }, std::pair { 1.0f, 1.0f }, std::pair { -1.0f, 1.0f } })
            {
                auto& vertex = vertices.emplace_back();
                vertex.position = (n + u * a + v * b) * halfSize;
                vertex.normal = inward ? -n : n;
            }

            for (const auto index : inward ? std::array { 0u, 2u, 1u, 0u, 3u, 2u } : std::array { 0u, 1u, 2u, 0u, 2u, 3u })
                indices.push_back (first + index);
        }

        auto mesh = Mesh::Ptr (new Mesh ("box"));
        mesh->addPrimitive (std::move (vertices), std::move (indices), std::move (material));
        return mesh;
    }

    /** A cube, the entity "caster", standing on the long ground at z = -casterDistance, seen from
        above and in front. The cube and the camera scale with the distance, so the cube and the
        shadow it casts beside it (towards +x) keep their size on screen: with k = casterDistance / 2
        the cube is 0.25 k wide and the camera looks at its foot from (0, 2 k, 3 k) away. At 256 x 256
        the shadow beside the cube covers about 14 x 11 px. A single shadow map fitted to the whole
        ground has texels of about 0.14 m, so at 2 m the shadow spans about 1 x 2 of them.
        The cube is drawn with @p casterMaterial, or a light gray dielectric. Returns the sun.
    */
    static LightNode& buildLongGroundScene (Scene& scene, float casterDistance, Material::Ptr casterMaterial = nullptr)
    {
        auto& sun = addLongGround (scene);
        const auto k = casterDistance * 0.5f;
        const auto size = 0.25f * k;

        auto material = casterMaterial != nullptr ? std::move (casterMaterial) : Material::Ptr (new Material());
        if (material->alphaMode == Material::AlphaMode::opaque)
        {
            material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
            material->metallicFactor = 0.0f;
        }

        auto caster = EntityNode::Ptr (new EntityNode ("caster"));
        caster->attach<MeshNode> (makeBox (size * 0.5f, material));
        caster->setPosition ({ 0.0f, size * 0.5f, -casterDistance });
        scene.getRoot()->addChild (caster);

        const Vector3<float> foot (0.0f, 0.0f, -casterDistance);
        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        camera->attach<CameraNode>();
        aimCamera (*camera, foot + Vector3<float> (0.0f, 2.0f * k, 3.0f * k), foot);
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        return sun;
    }

    /** Adds a 1 m cube, the entity "farCaster", to buildLongGroundScene() 60 m away beside the line
        of sight, in the last cascade only. */
    static EntityNode& addFarCaster (Scene& scene)
    {
        auto caster = EntityNode::Ptr (new EntityNode ("farCaster"));
        caster->attach<MeshNode> (makeBox (0.5f, makeWallMaterial()));
        caster->setPosition ({ 0.0f, 0.5f, -60.0f });
        scene.getRoot()->addChild (caster);
        return *caster;
    }

    /** A compact scene: a 0.6 m box, the entity "box", on a 2 m ground under a shadow casting sun,
        without a camera. Returns the box. */
    static EntityNode& addCompactScene (Scene& scene)
    {
        addGround (scene, 1.0f);

        auto box = EntityNode::Ptr (new EntityNode ("box"));
        box->attach<MeshNode> (makeBox (0.3f, makeWallMaterial()));
        box->setPosition ({ 0.0f, 0.3f, 0.0f });
        scene.getRoot()->addChild (box);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().castsShadows = true;
        aimCamera (*sun, { -3.0f, 8.0f, 2.0f }, {});
        scene.getRoot()->addChild (sun);
        return *box;
    }

    /** The shadow renderer of @p renderer, which must have rendered: a test failure otherwise,
        read as a shadow renderer that drew nothing. */
    static const ShadowRenderer& shadows (const SceneRenderer& renderer)
    {
        if (const auto* shadowRenderer = renderer.getShadowRenderer())
            return *shadowRenderer;

        ADD_FAILURE() << "The renderer has not rendered yet";
        static const ShadowRenderer none;
        return none;
    }

    /** How many frames drew each of the first @p count sun cascades. */
    static std::vector<int> getCascadeRenderCounts (const SceneRenderer& renderer, int count)
    {
        std::vector<int> counts;
        for (int c = 0; c < count; ++c)
            counts.push_back (shadows (renderer).getNumCascadeRenders (c));

        return counts;
    }

    /** The largest relative darkening of an 8x8 pixel window from @p unshadowed to @p shadowed,
        (lit - shadowed) / lit in Rec.709 luma, over the windows with a lit luma above 10 levels.
        Only the shadow tells the two images apart, so the caster itself never counts.
    */
    static double shadowContrast (const Image& unshadowed, const Image& shadowed)
    {
        auto contrast = 0.0;
        for (int y = 0; y + 8 <= unshadowed.getHeight(); y += 2)
        {
            for (int x = 0; x + 8 <= unshadowed.getWidth(); x += 2)
            {
                const Rectangle<int> window (x, y, 8, 8);
                const auto lit = meanLuminance (unshadowed, window);
                if (lit > 10.0)
                    contrast = jmax (contrast, (lit - meanLuminance (shadowed, window)) / lit);
            }
        }

        return contrast;
    }

    /** The Rec.709 luma, from 0 to 1, of every pixel of column @p x from the top, 0 where it shows
        @p background (within 2 levels in every channel). */
    static std::vector<float> luminanceColumn (const Image& image, int x, Color background)
    {
        std::vector<float> column;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            const auto color = image.getPixelColor (x, y);
            const auto isBackground = std::abs (color.getRed() - background.getRed()) <= 2
                                   && std::abs (color.getGreen() - background.getGreen()) <= 2
                                   && std::abs (color.getBlue() - background.getBlue()) <= 2;

            column.push_back (isBackground ? 0.0f : static_cast<float> (meanLuminance (image, { x, y, 1, 1 }) / 255.0));
        }

        return column;
    }

    /** The median of the non-zero values of @p values, 0 without any. */
    static float median (std::vector<float> values)
    {
        values.erase (std::remove (values.begin(), values.end(), 0.0f), values.end());
        if (values.empty())
            return 0.0f;

        std::nth_element (values.begin(), values.begin() + static_cast<std::ptrdiff_t> (values.size() / 2), values.end());
        return values[values.size() / 2];
    }

    /** Pins the shadow settings of tests comparing the tiers: no soft shadows and the raster cascade count on both. */
    static void pinCrossTierShadows (SceneRenderer& renderer)
    {
        auto settings = renderer.getRenderSettings();
        settings.softShadows = false;
        settings.sunCascades = RenderCapabilities::defaultRasterCascades;
        renderer.setRenderSettings (settings);
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

    /** The red levels of the edges scene's pixels between the background and the lit plate, which
        only MSAA coverage produces along the silhouettes. */
    static std::vector<int> intermediateEdgeLevels (const Image& image)
    {
        std::vector<int> levels;
        for (int y = 30; y < 100; ++y)
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto red = static_cast<int> (image.getPixelColor (x, y).getRed());
                if (red > 30 && red < 147)
                    levels.push_back (red);
            }

        return levels;
    }

    /** Silhouettes have intermediate gray levels, which only MSAA coverage produces. */
    static void checkEdges (const Image& image)
    {
        const auto levels = intermediateEdgeLevels (image);
        EXPECT_GE (std::set<int> (levels.begin(), levels.end()).size(), 3u) << "edges: intermediate levels";
    }

    /** The pixels with a channel at 0 or 255 in @p image, where @p baseline has none. A raster rgba8
        frame hides non-finite values, but a channel holding one reads as either end, depending on
        where it enters the shading and on the backend (Metal returns the other operand of min (NaN, x)).
    */
    static int countNewExtremePixels (const Image& image, const Image& baseline)
    {
        const auto isExtreme = [] (Color color)
        {
            for (const auto level : { color.getRed(), color.getGreen(), color.getBlue() })
                if (level == 0 || level == 255)
                    return true;

            return false;
        };

        int extreme = 0;
        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                extreme += isExtreme (image.getPixelColor (x, y)) && ! isExtreme (baseline.getPixelColor (x, y)) ? 1 : 0;

        return extreme;
    }

    /** Selects the rendering tier of a renderer. */
    static void setTier (SceneRenderer& renderer, RenderSettings::TierPreference tier)
    {
        auto settings = renderer.getRenderSettings();
        settings.tier = tier;
        renderer.setRenderSettings (settings);
    }

    /** The linear radiance behind an 8 bit level that was Reinhard tone mapped and sRGB encoded. */
    static double reinhardRadiance (int level)
    {
        const auto encoded = level / 255.0;
        const auto mapped = encoded <= 0.04045 ? encoded / 12.92 : std::pow ((encoded + 0.055) / 1.055, 2.4);
        return mapped / (1.0 - mapped);
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

    /** Turns bloom on or off in the render settings of a renderer. */
    static void setBloom (SceneRenderer& renderer, bool bloom)
    {
        auto settings = renderer.getRenderSettings();
        settings.bloom = bloom;
        renderer.setRenderSettings (settings);
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

    /** True for the pixels of @p image more than 2 levels away from @p reference in any channel. */
    static std::vector<bool> differenceMask (const Image& image, const Image& reference)
    {
        std::vector<bool> mask;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto a = image.getPixelColor (x, y);
                const auto b = reference.getPixelColor (x, y);
                mask.push_back (std::abs (a.getRed() - b.getRed()) > 2 || std::abs (a.getGreen() - b.getGreen()) > 2 || std::abs (a.getBlue() - b.getBlue()) > 2);
            }
        }

        return mask;
    }

    /** The shadows scene against its Phase 0 baseline @p platform_shadows.png by statistics, since
        the cascades replaced the single map of Phase 0: the shadow darkens at least as much
        (checkShadows), covers about the same pixels, and nothing differs outside of the plates.
        The shadow is what tells each image apart from the scene rendered without it. The plates'
        shading under the environment changed in Phase 4 (lookup tables and multiple scattering),
        so their mean luminance is recorded for both images rather than compared.
    */
    void checkShadowsAgainstBaseline (const Image& image, const String& platform)
    {
        const auto baseline = loadBaseline (platform + "_shadows.png");
        ASSERT_TRUE (baseline.isValid()) << "capture the Phase 0 baselines first";

        Scene scene;
        buildShadowsScene (scene);
        scene.getRoot()->findChild ("sun")->getNode<LightNode>()->castsShadows = false;

        SceneRenderer renderer;
        setTier (renderer, RenderSettings::TierPreference::raster);
        const auto unshadowed = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (unshadowed.isValid()) << renderer.getLastError();

        // The plates are the pixels they change over the background, in either image
        scene.getRoot()->removeChild (scene.getRoot()->findChild ("plates"));
        SceneRenderer backgroundRenderer;
        setTier (backgroundRenderer, RenderSettings::TierPreference::raster);
        const auto background = renderToImage (backgroundRenderer, scene, 128);
        ASSERT_TRUE (background.isValid()) << backgroundRenderer.getLastError();

        checkShadows (image);

        const auto margin = [] (const Image& shadowed)
        {
            return static_cast<int> (shadowed.getPixelColor (40, 60).getRed()) - static_cast<int> (shadowed.getPixelColor (50, 64).getRed());
        };

        RecordProperty ("baselineShadowMargin", margin (baseline));
        EXPECT_GE (margin (image), margin (baseline));

        const auto shadow = differenceMask (image, unshadowed);
        const auto baselineShadow = differenceMask (baseline, unshadowed);
        const auto differing = differenceMask (image, baseline);
        const auto plates = differenceMask (image, background);
        const auto baselinePlates = differenceMask (baseline, background);

        int shadowPixels = 0, baselineShadowPixels = 0, differingOutside = 0, changedOutside = 0, platePixels = 0;
        double plateLuminance = 0.0, baselinePlateLuminance = 0.0;
        for (size_t i = 0; i < shadow.size(); ++i)
        {
            shadowPixels += shadow[i] ? 1 : 0;
            baselineShadowPixels += baselineShadow[i] ? 1 : 0;

            const auto onPlates = plates[i] || baselinePlates[i];
            differingOutside += differing[i] && ! onPlates ? 1 : 0;

            // The background stays exact, not just within 2 levels
            const auto x = static_cast<int> (i) % image.getWidth(), y = static_cast<int> (i) / image.getWidth();
            changedOutside += ! onPlates && image.getPixelColor (x, y) != baseline.getPixelColor (x, y) ? 1 : 0;

            if (onPlates)
            {
                const Rectangle<int> pixel (x, y, 1, 1);
                plateLuminance += meanLuminance (image, pixel);
                baselinePlateLuminance += meanLuminance (baseline, pixel);
                ++platePixels;
            }
        }

        RecordProperty ("shadowsDifferingFromBaseline", static_cast<int> (std::count (differing.begin(), differing.end(), true)));
        RecordProperty ("shadowPixels", shadowPixels);
        RecordProperty ("baselineShadowPixels", baselineShadowPixels);
        RecordProperty ("differingOutsidePlates", differingOutside);
        RecordProperty ("changedOutsidePlates", changedOutside);
        RecordProperty ("platePixels", platePixels);
        RecordProperty ("platesMeanLuminance", String (plateLuminance / jmax (platePixels, 1), 2).toStdString());
        RecordProperty ("baselinePlatesMeanLuminance", String (baselinePlateLuminance / jmax (platePixels, 1), 2).toStdString());
        RecordProperty ("platesLuminanceChangePercent", String (100.0 * (plateLuminance - baselinePlateLuminance) / jmax (baselinePlateLuminance, 1.0), 2).toStdString());

        EXPECT_GT (baselineShadowPixels, 0);
        EXPECT_GE (shadowPixels, baselineShadowPixels * 4 / 5);
        EXPECT_LE (shadowPixels, baselineShadowPixels * 5 / 4);
        EXPECT_GT (platePixels, 128 * 128 / 8);
        EXPECT_EQ (differingOutside, 0);
        EXPECT_EQ (changedOutside, 0);
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

            SceneRenderer renderer;

            // The baselines are raster frames, whichever tier the device would pick
            setTier (renderer, RenderSettings::TierPreference::raster);

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

    /** The raster no-regression gate, at the default settings: the materials and edges scenes match
        the Phase 0 images of @p platform exactly (no pixel beyond 2 levels), the shadows scene is
        checked by statistics (checkShadowsAgainstBaseline()), the silhouettes of the edges scene
        keep at least the baseline's MSAA coverage, and no scene has a channel at 0 or 255 where the
        baseline has none, as a non-finite value would leave.
    */
    void checkMatchesPhaseZeroBaseline (const String& platform)
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

            SceneRenderer renderer;
            renderer.setRenderSettings (settings);
            const auto image = renderToImage (renderer, scene, 128);
            ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();

            const auto baseline = loadBaseline (platform + "_" + name + ".png");
            ASSERT_TRUE (baseline.isValid()) << "capture the Phase 0 baselines first";

            const auto newExtreme = countNewExtremePixels (image, baseline);
            RecordProperty ((String (name) + "NewExtremePixels").toStdString(), newExtreme);
            EXPECT_EQ (newExtreme, 0) << name;

            if (String (name) == "shadows")
            {
                checkShadowsAgainstBaseline (image, platform);
                continue;
            }

            if (String (name) == "edges")
            {
                const auto coverage = static_cast<int> (intermediateEdgeLevels (image).size());
                const auto baselineCoverage = static_cast<int> (intermediateEdgeLevels (baseline).size());
                RecordProperty ("edgeCoveragePixels", coverage);
                RecordProperty ("baselineEdgeCoveragePixels", baselineCoverage);
                EXPECT_GE (coverage, baselineCoverage);
            }

            const auto differing = countDifferences (image, baseline);
            RecordProperty ((String (name) + "Differing").toStdString(), differing);
            EXPECT_EQ (differing, 0) << name;
        }
    }

    /** The baseline scenes at the default settings on @p tier, without a probe or a volume, add
        none of the opt-in passes: no depth and normal prepass, no probe baker, no probe list (only
        the local indirect variants declare its block, and the default variants shade), no
        screen-space GI, no depth of field, no motion blur, even on a frame that moved, no fog,
        disabled or without density, and no upscaling. On raster, no ambient occlusion, screen-space reflections or
        Hi-Z pyramid either; the full tier traces its ambient occlusion and reflections, on there by
        default. No pass or allocation count is exposed, so the getters of each pass stand in for them.
    */
    void checkDefaultSettingsAddNoOptInPass (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        const std::pair<const char*, void (*) (Scene&)> scenes[] = {
            { "shadows", buildShadowsScene },
            { "materials", buildMaterialsScene },
            { "edges", buildEdgesScene },
            { "disabled fog", [] (Scene& scene)
              {
                  // Every field but enabled away from its default
                  buildEdgesScene (scene);
                  WorldFog fog;
                  fog.color = { 1.0f, 0.5f, 0.2f };
                  fog.density = 0.5f;
                  fog.heightFalloff = 0.3f;
                  fog.baseHeight = 1.0f;
                  fog.anisotropy = 0.6f;
                  scene.setWorldFog (fog);
              } },
            { "fog without density", [] (Scene& scene)
              {
                  buildEdgesScene (scene);
                  WorldFog fog;
                  fog.enabled = true;
                  fog.density = 0.0f;
                  scene.setWorldFog (fog);
              } },
        };

        for (const auto& [name, build] : scenes)
        {
            Scene scene;
            build (scene);

            // The reflections need the history of an earlier frame, and the last frame moves
            // everything, which motion blur would smear
            SceneRenderer renderer;
            setTier (renderer, tier);
            for (int frame = 0; frame < 3; ++frame)
            {
                if (frame == 2)
                    scene.getRoot()->setPosition ({ 0.05f, 0.0f, 0.0f });

                ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << name << ": " << renderer.getLastError();
            }

            EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;

            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
            EXPECT_EQ (renderer.getProbeBaker(), nullptr) << name;

            ASSERT_NE (renderer.getForwardPass(), nullptr) << name;
            EXPECT_FALSE (renderer.getForwardPass()->usesLocalIndirect()) << name;
            EXPECT_FALSE (renderer.getForwardPass()->usesMaterialLayers()) << name;
            EXPECT_FALSE (renderer.getLightingPass()->usesLocalIndirect()) << name;

            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr, full) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections) != nullptr, full) << name;
            EXPECT_EQ (renderer.getReflectionPass() != nullptr, full) << name;
            EXPECT_EQ (renderer.getHiZPyramid() != nullptr, full) << name;
            EXPECT_EQ (renderer.getDepthOfField(), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::circleOfConfusion), nullptr) << name;
            EXPECT_EQ (renderer.getMotionBlur(), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << name;
            EXPECT_EQ (renderer.getFogApply(), nullptr) << name;

            // Raster keeps its inline frame: no half float scene color
            ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr) << name;
            if (! full)
                EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float) << name;

            // Shaded at full size, without compiling the upscaling
            ASSERT_NE (renderer.getTemporalAA(), nullptr) << name;
            EXPECT_FALSE (renderer.getTemporalAA()->isUpscalingPrepared()) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getWidth(), 64) << name;
            if (full)
            {
                ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), nullptr) << name;
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::velocity)->getWidth(), 64) << name;
            }

            const auto* ambientOcclusion = renderer.getAmbientOcclusionPass();
            EXPECT_EQ (ambientOcclusion != nullptr, full) << name;
            if (ambientOcclusion != nullptr)
            {
                EXPECT_EQ (ambientOcclusion->getIndirectDiffuse(), nullptr) << name;
                EXPECT_EQ (ambientOcclusion->getNumCachedIndirectDiffuseTargets(), 0u) << name;
            }

            // Not vacuous: with motion blur on, the full tier smears the next move
            if (full)
            {
                auto settings = renderer.getRenderSettings();
                settings.motionBlur = true;
                renderer.setRenderSettings (settings);
                scene.getRoot()->setPosition ({ 0.1f, 0.0f, 0.0f });

                ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << name << ": " << renderer.getLastError();
                EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;
                EXPECT_NE (renderer.getMotionBlur(), nullptr) << name;
            }
        }
    }

    /** Renders and reads the pixels back through a canvas. */
    Image renderToImage (SceneRenderer& renderer, Scene& scene, int size)
    {
        auto texture = renderer.render (device, scene, size, size);

        // A frame the GPU failed to execute reads back blank, without a CPU side error
        const auto gpuResult = renderer.waitForGPU();
        EXPECT_TRUE (gpuResult.wasOk()) << gpuResult.getErrorMessage();

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

    /** Adds a square plate facing the default camera, 2 units wide, drawn with @p material, and returns its entity. */
    static EntityNode* addPlate (Scene& scene, Material::Ptr material)
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
        return entity.get();
    }

    /** Selects the full tier, accumulating @p taaSamples jittered frames (1 disables TAA). */
    static void setFullTierTaa (SceneRenderer& renderer, int taaSamples)
    {
        auto settings = renderer.getRenderSettings();
        settings.tier = RenderSettings::TierPreference::full;
        settings.taaSamples = taaSamples;
        renderer.setRenderSettings (settings);
    }

    /** A 16x16 grid of white quads, 0.45 apart and 0.24 wide, each lit by its own red point light
        0.15 in front of its center with a range of 0.4: the nearest other light is 0.47 away from a
        quad center, out of range. The camera looks at the grid from 9 units, so at 256x256 the quad
        centers are the pixels of getLightGridCenter(). The quads are one primitive when @p oneDraw
        is set, and one primitive each otherwise.
    */
    static void buildLightGridScene (Scene& scene, bool oneDraw)
    {
        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;

        auto mesh = Mesh::Ptr (new Mesh ("grid"));
        std::vector<Mesh::Vertex> vertices;
        std::vector<uint32> indices;

        for (int row = 0; row < 16; ++row)
        {
            for (int column = 0; column < 16; ++column)
            {
                const auto center = getLightGridPosition (column, row);
                const auto first = static_cast<uint32> (vertices.size());

                for (const auto& corner : { Vector3<float> (-0.12f, -0.12f, 0.0f), Vector3<float> (0.12f, -0.12f, 0.0f),
                                            Vector3<float> (0.12f, 0.12f, 0.0f), Vector3<float> (-0.12f, 0.12f, 0.0f) })
                {
                    auto& vertex = vertices.emplace_back();
                    vertex.position = center + corner;
                    vertex.normal = { 0.0f, 0.0f, 1.0f };
                }

                for (const auto index : { 0u, 1u, 2u, 0u, 2u, 3u })
                    indices.push_back (first + index);

                if (! oneDraw)
                {
                    mesh->addPrimitive (std::move (vertices), std::move (indices), material);
                    vertices.clear();
                    indices.clear();
                }

                auto light = EntityNode::Ptr (new EntityNode ("light"));
                auto& node = light->attach<LightNode> (LightNode::Type::point);
                node.color = { 1.0f, 0.0f, 0.0f };
                node.range = 0.4f;
                light->setPosition (center + Vector3<float> (0.0f, 0.0f, 0.15f));
                scene.getRoot()->addChild (light);
            }
        }

        if (oneDraw)
            mesh->addPrimitive (std::move (vertices), std::move (indices), material);

        auto grid = EntityNode::Ptr (new EntityNode ("grid"));
        grid->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (grid);

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        camera->attach<CameraNode>();
        camera->setPosition ({ 0.0f, 0.0f, 9.0f });
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        scene.setAmbientColor (Colors::black);
    }

    /** The world position of a quad center of buildLightGridScene(). */
    static Vector3<float> getLightGridPosition (int column, int row)
    {
        return { 0.45f * (static_cast<float> (column) - 7.5f), 0.45f * (static_cast<float> (row) - 7.5f), 0.0f };
    }

    /** The pixel of a quad center of buildLightGridScene() at 256x256: the camera sees 2 tan (0.4) * 9
        world units. The grid is symmetric, so this is the same quad center whichever way rows are read.
    */
    static Point<int> getLightGridCenter (int column, int row)
    {
        const auto pixelsPerUnit = 256.0f / (18.0f * std::tan (0.4f));
        const auto position = getLightGridPosition (column, row);
        return { static_cast<int> (128.0f + position.getX() * pixelsPerUnit), static_cast<int> (128.0f - position.getY() * pixelsPerUnit) };
    }

    /** The quad centers of buildLightGridScene() with a red level of at least 100 at 256x256. */
    static int countLitGridQuads (const Image& image)
    {
        int lit = 0;
        for (int row = 0; row < 16; ++row)
        {
            for (int column = 0; column < 16; ++column)
            {
                const auto pixel = getLightGridCenter (column, row);
                lit += image.getPixelColor (pixel.getX(), pixel.getY()).getRed() >= 100 ? 1 : 0;
            }
        }

        return lit;
    }

    /** The quad centers of buildLightGridScene() countLitGridQuads() misses, as column,row=red, for failure messages. */
    static String describeDarkGridQuads (const Image& image)
    {
        StringArray dark;
        for (int row = 0; row < 16; ++row)
        {
            for (int column = 0; column < 16; ++column)
            {
                const auto pixel = getLightGridCenter (column, row);
                const auto red = static_cast<int> (image.getPixelColor (pixel.getX(), pixel.getY()).getRed());
                if (red < 100)
                    dark.add (String (column) + "," + String (row) + "=" + String (red));
            }
        }

        return "dark quads: " + dark.joinIntoString (" ");
    }

    /** More than eight lights shade one scene on @p tier. */
    void checkShadesMoreThanEightLightsPerScene (RenderSettings::TierPreference tier)
    {
        // A 4x4 grid of small white quads, 0.6 apart, each lit by its own red point light 0.3 in front
        // of it with a range of 0.6: a neighbour's light is 0.67 away from a quad center, out of range
        auto material = Material::Ptr (new Material());
        material->metallicFactor = 0.0f;

        Scene scene;
        scene.setAmbientColor (Colors::black);

        std::vector<Vector3<float>> centers;
        for (int row = 0; row < 4; ++row)
            for (int column = 0; column < 4; ++column)
                centers.push_back ({ 0.6f * static_cast<float> (column) - 0.9f, 0.6f * static_cast<float> (row) - 0.9f, 0.0f });

        for (const auto& center : centers)
        {
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = center + Vector3<float> (-0.2f, -0.2f, 0.0f);
            vertices[1].position = center + Vector3<float> (0.2f, -0.2f, 0.0f);
            vertices[2].position = center + Vector3<float> (0.2f, 0.2f, 0.0f);
            vertices[3].position = center + Vector3<float> (-0.2f, 0.2f, 0.0f);

            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            auto mesh = Mesh::Ptr (new Mesh ("quad"));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

            auto quad = EntityNode::Ptr (new EntityNode ("quad"));
            quad->attach<MeshNode> (mesh);
            scene.getRoot()->addChild (quad);

            auto light = EntityNode::Ptr (new EntityNode ("light"));
            auto& node = light->attach<LightNode> (LightNode::Type::point);
            node.color = { 1.0f, 0.0f, 0.0f };
            node.range = 0.6f;
            light->setPosition (center + Vector3<float> (0.0f, 0.0f, 0.3f));
            scene.getRoot()->addChild (light);
        }

        auto& renderer = sharedRenderer();
        setTier (renderer, tier);
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        // The default camera sees 2 tan (0.4) * 5 world units over the 128 pixels; the grid is
        // symmetric, so the centers are the same pixels whichever way the rows are read back
        const auto pixelsPerUnit = 128.0f / (10.0f * std::tan (0.4f));
        int lit = 0;

        for (const auto& center : centers)
        {
            const auto x = static_cast<int> (64.0f + center.getX() * pixelsPerUnit);
            const auto y = static_cast<int> (64.0f - center.getY() * pixelsPerUnit);
            const auto red = static_cast<int> (image.getPixelColor (x, y).getRed());

            EXPECT_GE (red, 100) << "quad at " << center.getX() << ", " << center.getY();
            lit += red >= 100 ? 1 : 0;
        }

        RecordProperty ("litQuads", lit);
    }

    /** Renders @p scene at @p size x @p size, on the full tier until the accumulation converged, and reads it back. */
    Image renderConverged (SceneRenderer& renderer, Scene& scene, int size)
    {
        for (int frame = 0; frame < 64; ++frame)
            if (renderer.render (device, scene, size, size) == nullptr || renderer.isConverged())
                break;

        return renderToImage (renderer, scene, size);
    }

    /** buildLongGroundScene() on @p tier at 256 x 256, converged, with the sun casting shadows or
        not and an angular radius of @p sunRadius, and the cube drawn with @p casterMaterial. */
    Image renderLongGroundScene (RenderSettings::TierPreference tier, float casterDistance, bool castsShadows,
                                 float sunRadius = 0.0f, Material::Ptr casterMaterial = nullptr)
    {
        Scene scene;
        auto& sun = buildLongGroundScene (scene, casterDistance, std::move (casterMaterial));
        sun.castsShadows = castsShadows;
        sun.radius = sunRadius;

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto image = renderConverged (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    }

    /** The sun shadows a cube standing near the camera and one 60 m away on @p tier: beside each
        cube the shadow darkens the ground by more than 35%. */
    void checkSunShadowReachesNearAndFarCasters (RenderSettings::TierPreference tier)
    {
        for (const auto distance : { 2.0f, 60.0f })
        {
            const auto shadowed = renderLongGroundScene (tier, distance, true);
            const auto unshadowed = renderLongGroundScene (tier, distance, false);
            ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

            const auto contrast = shadowContrast (unshadowed, shadowed);
            RecordProperty ((String ("contrastAt") + String (static_cast<int> (distance))).toStdString(), String (contrast, 3).toStdString());
            EXPECT_GT (contrast, 0.35) << "caster at " << distance << " m";
        }
    }

    /** A sun with an angular radius, shaded through the area light variant, still casts its shadow on @p tier. */
    void checkAreaSunCastsAShadow (RenderSettings::TierPreference tier)
    {
        const auto shadowed = renderLongGroundScene (tier, 2.0f, true, 0.05f);
        const auto unshadowed = renderLongGroundScene (tier, 2.0f, false, 0.05f);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

        const auto contrast = shadowContrast (unshadowed, shadowed);
        RecordProperty ("contrast", String (contrast, 3).toStdString());
        EXPECT_GT (contrast, 0.35);
    }

    /** An alpha masked cube casts its shadow on @p tier only where its material keeps the texels:
        cut out entirely, it casts none. */
    void checkMaskedCasterShadowsOnlyWhatItKeeps (RenderSettings::TierPreference tier)
    {
        const auto contrastWithAlpha = [&] (float alpha)
        {
            const auto material = [alpha]
            {
                auto masked = Material::Ptr (new Material());
                masked->baseColorFactor = { 0.8f, 0.8f, 0.8f, alpha };
                masked->metallicFactor = 0.0f;
                masked->alphaMode = Material::AlphaMode::mask;
                return masked;
            };

            const auto shadowed = renderLongGroundScene (tier, 2.0f, true, 0.0f, material());
            const auto unshadowed = renderLongGroundScene (tier, 2.0f, false, 0.0f, material());
            return shadowed.isValid() && unshadowed.isValid() ? shadowContrast (unshadowed, shadowed) : -1.0;
        };

        const auto kept = contrastWithAlpha (0.8f);
        const auto cutOut = contrastWithAlpha (0.2f);
        RecordProperty ("keptContrast", String (kept, 3).toStdString());
        RecordProperty ("cutOutContrast", String (cutOut, 3).toStdString());

        EXPECT_GT (kept, 0.35);
        EXPECT_GE (cutOut, 0.0);
        EXPECT_LT (cutOut, 0.02);
    }

    /** The long ground seen from 1.6 m above it towards the horizon, with nothing on it, is lit
        on @p tier as if it cast no shadow: down five columns of the frame, from the far edge of the
        ground to the bottom, no cascade hands over to the next with a darker band, and no acne
        darkens the ground. The same view without shadows is the reference.
    */
    void checkCascadeTransitionHasNoSeam (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool castsShadows, Color& background)
        {
            Scene scene;
            addLongGround (scene).castsShadows = castsShadows;
            background = scene.getBackgroundColor();

            auto camera = EntityNode::Ptr (new EntityNode ("camera"));
            camera->attach<CameraNode>();
            aimCamera (*camera, { 0.0f, 1.6f, 4.0f }, { 0.0f, 0.0f, -1000.0f });
            scene.getRoot()->addChild (camera);
            scene.setActiveCamera (camera.get());

            SceneRenderer renderer;
            setTier (renderer, tier);
            const auto image = renderConverged (renderer, scene, 256);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        Color background;
        const auto shadowed = render (true, background);
        const auto unshadowed = render (false, background);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

        auto worst = 1.0f;
        for (const auto x : { 64, 96, 128, 160, 192 })
        {
            // Antialiasing blends the far edge of the ground with the background: its first rows are left out
            auto lit = luminanceColumn (unshadowed, x, background);
            const auto column = luminanceColumn (shadowed, x, background);
            const auto firstGround = std::find_if (lit.begin(), lit.end(), [] (float value) { return value > 0.0f; });
            std::fill (firstGround, firstGround + std::min<std::ptrdiff_t> (4, std::distance (firstGround, lit.end())), 0.0f);

            const auto groundRows = std::count_if (lit.begin(), lit.end(), [] (float value) { return value > 0.0f; });
            EXPECT_GE (groundRows, 100) << "column " << x;

            for (size_t row = 0; row < column.size(); ++row)
            {
                if (lit[row] <= 0.0f)
                    continue;

                worst = jmin (worst, column[row] / lit[row]);

                if (column[row] < 0.95f * lit[row])
                {
                    ADD_FAILURE() << "column " << x << ", row " << row << ": " << column[row] << " against " << lit[row];
                    break;
                }
            }
        }

        RecordProperty ("darkestRelative", String (worst, 3).toStdString());
    }

    /** A 0.3 m cube, the entity "blocker", centered on @p position. */
    static void addBlocker (Scene& scene, Vector3<float> position)
    {
        auto blocker = EntityNode::Ptr (new EntityNode ("blocker"));
        blocker->attach<MeshNode> (makeBox (0.15f, makeWallMaterial()));
        blocker->setPosition (position);
        scene.getRoot()->addChild (blocker);
    }

    /** A closed 4 x 4 x 4 room around the origin, its walls facing inwards, lit by a shadow casting
        point light at the origin (intensity 20, no range), with a 0.3 m blocker cube 1 m from the
        light on each of the six axes when @p withBlockers is set: each blocker shadows the middle of
        the wall behind it, and the wall around that shadow stays lit. Returns the light.
    */
    static LightNode& buildPointLightRoom (Scene& scene, bool withBlockers = true)
    {
        auto room = EntityNode::Ptr (new EntityNode ("room"));
        room->attach<MeshNode> (makeBox (2.0f, makeWallMaterial(), true));
        scene.getRoot()->addChild (room);

        if (withBlockers)
            for (const auto& axis : getCubeAxes())
                addBlocker (scene, axis);

        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& light = entity->attach<LightNode> (LightNode::Type::point);
        light.intensity = 20.0f;
        light.castsShadows = true;
        scene.getRoot()->addChild (entity);
        return light;
    }

    /** The six unit axes, in the order of the faces of a point light shadow: +X, -X, +Y, -Y, +Z, -Z. */
    static std::array<Vector3<float>, 6> getCubeAxes()
    {
        return { Vector3<float> (1.0f, 0.0f, 0.0f), Vector3<float> (-1.0f, 0.0f, 0.0f), Vector3<float> (0.0f, 1.0f, 0.0f),
                 Vector3<float> (0.0f, -1.0f, 0.0f), Vector3<float> (0.0f, 0.0f, 1.0f), Vector3<float> (0.0f, 0.0f, -1.0f) };
    }

    /** Adds a shadow casting point light of intensity 1 at @p position. */
    static void addShadowedPointLight (Scene& scene, Vector3<float> position)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("shadowedPoint"));
        entity->attach<LightNode> (LightNode::Type::point).castsShadows = true;
        entity->setPosition (position);
        scene.getRoot()->addChild (entity);
    }

    /** Adds a camera at @p eye looking at @p target, and makes it the active one. */
    static void addCamera (Scene& scene, Vector3<float> eye, Vector3<float> target, Vector3<float> up = { 0.0f, 1.0f, 0.0f })
    {
        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        camera->attach<CameraNode>();
        aimCamera (*camera, eye, target, up);
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());
    }

    /** A 10 x 10 ground at y = 0 under a shadow casting spot light at (0, 3, 0) shining down
        (intensity 20, outer cone 45 degrees), with a 0.3 m blocker cube at @p blocker. The camera
        looks at the foot of the spot from (0, 3, 4), so the default blocker at (0, 1, 0) hides a
        patch of ground behind its shadow, not the shadow. Returns the spot.
    */
    static LightNode& buildSpotOverGround (Scene& scene, Vector3<float> blocker = { 0.0f, 1.0f, 0.0f })
    {
        addGround (scene, 5.0f);
        addBlocker (scene, blocker);

        auto entity = EntityNode::Ptr (new EntityNode ("spot"));
        auto& spot = entity->attach<LightNode> (LightNode::Type::spot);
        spot.intensity = 20.0f;
        spot.castsShadows = true;
        aimCamera (*entity, { 0.0f, 3.0f, 0.0f }, {}, { 0.0f, 0.0f, -1.0f });
        scene.getRoot()->addChild (entity);

        addCamera (scene, { 0.0f, 3.0f, 4.0f }, {});
        return spot;
    }

    /** The point light at the center of buildPointLightRoom() shadows the wall behind a single
        blocker on each of the six axes on @p tier: seen from beside the light, the shadow darkens the
        wall by more than half, against the same view without shadows. One blocker at a time, so the
        shadow can only come from the face of that axis.
    */
    void checkPointLightShadowsOnAllSixFaces (RenderSettings::TierPreference tier)
    {
        const auto axes = getCubeAxes();
        for (size_t face = 0; face < axes.size(); ++face)
        {
            const auto axis = axes[face];
            const auto side = axes[(face + 2) % 6] + axes[(face + 4) % 6];

            const auto render = [&] (bool castsShadows)
            {
                Scene scene;
                buildPointLightRoom (scene, false).castsShadows = castsShadows;
                addBlocker (scene, axis);
                addCamera (scene, side * 0.9f - axis * 0.5f, axis * 2.0f, axes[(face + 2) % 6]);

                SceneRenderer renderer;
                setTier (renderer, tier);
                const auto image = renderConverged (renderer, scene, 128);
                EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                return image;
            };

            const auto shadowed = render (true);
            const auto unshadowed = render (false);
            ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

            const auto contrast = shadowContrast (unshadowed, shadowed);
            RecordProperty ((String ("contrastFace") + String (static_cast<int> (face))).toStdString(), String (contrast, 3).toStdString());
            EXPECT_GT (contrast, 0.5) << "face " << axis.getX() << "," << axis.getY() << "," << axis.getZ();
        }
    }

    /** The empty room of buildPointLightRoom(), its light moved off the center so that every face
        of its shadow sees the walls differently and the face edges cross the walls away from their
        corners, is lit on @p tier as if the light cast no shadow, seen from the light along the
        diagonal where three faces meet: no pixel darkens by more than 5% on raster, 2% on full, so
        no face edge leaves a crack and nothing acnes. The full tier filters with a smaller kernel
        than the raster tent, so a crack darkens less there: its threshold is tighter. Checked at the
        default shadow resolution, and at the smallest, whose coarse texels widen a crack on screen.
        A @p radius above 0 checks the soft shadows of a light of that size instead, whose wide
        kernels must not shadow the walls with themselves either.
    */
    void checkPointShadowHasNoFaceSeams (RenderSettings::TierPreference tier, float radius = 0.0f)
    {
        const Vector3<float> light (0.5f, -0.3f, 0.4f);
        const auto threshold = tier == RenderSettings::TierPreference::full ? 0.98 : 0.95;

        for (const auto resolution : { RenderSettings().shadowResolution, RenderCapabilities::minShadowResolution })
        {
            const auto render = [&] (bool castsShadows)
            {
                Scene scene;
                auto& pointLight = buildPointLightRoom (scene, false);
                pointLight.castsShadows = castsShadows;
                pointLight.radius = radius;
                scene.getRoot()->findChild ("point")->setPosition (light);
                addCamera (scene, light, light + Vector3<float> (1.0f, 1.0f, 1.0f), { 0.0f, 0.0f, 1.0f });

                SceneRenderer renderer;
                auto settings = renderer.getRenderSettings();
                settings.tier = tier;
                settings.shadowResolution = resolution;
                if (radius > 0.0f)
                    settings.softShadows = true;

                renderer.setRenderSettings (settings);

                const auto image = renderConverged (renderer, scene, 256);
                EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                return image;
            };

            const auto shadowed = render (true);
            const auto unshadowed = render (false);
            ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

            auto worst = 1.0;
            auto darkened = 0;
            for (int y = 0; y < shadowed.getHeight(); ++y)
            {
                for (int x = 0; x < shadowed.getWidth(); ++x)
                {
                    const auto lit = meanLuminance (unshadowed, { x, y, 1, 1 });
                    if (lit <= 10.0)
                        continue;

                    const auto relative = meanLuminance (shadowed, { x, y, 1, 1 }) / lit;
                    worst = jmin (worst, relative);
                    darkened += relative < threshold ? 1 : 0;
                }
            }

            const auto suffix = String (resolution).toStdString();
            RecordProperty ("darkestRelative" + suffix, String (worst, 3).toStdString());
            RecordProperty ("darkenedPixels" + suffix, darkened);
            EXPECT_EQ (darkened, 0) << "resolution " << resolution << ", darkest " << worst;
        }
    }

    /** The spot of buildSpotOverGround() with a radius of @p radius casts the blocker's shadow on
        the ground on @p tier: the shadow darkens the ground by more than 35%. */
    void checkSpotCastsAShadow (RenderSettings::TierPreference tier, float radius)
    {
        const auto render = [&] (bool castsShadows)
        {
            Scene scene;
            auto& spot = buildSpotOverGround (scene);
            spot.castsShadows = castsShadows;
            spot.radius = radius;

            SceneRenderer renderer;
            setTier (renderer, tier);
            const auto image = renderConverged (renderer, scene, 128);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        const auto shadowed = render (true);
        const auto unshadowed = render (false);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

        const auto contrast = shadowContrast (unshadowed, shadowed);
        RecordProperty ("contrast", String (contrast, 3).toStdString());
        EXPECT_GT (contrast, 0.35);
    }

    /** The centroid on the ground y = 0 of the pixels of @p shadowed darker than 70% of
        @p unshadowed, among those brighter than 10 levels in @p unshadowed, both seen through
        @p viewProjection: each pixel is traced back to the ground and weighs the ground area it
        covers, so the perspective of the camera does not move the centroid. Returns nothing when
        no pixel is shadowed.
    */
    static std::optional<Vector3<float>> shadowCentroidOnGround (const Image& unshadowed, const Image& shadowed, const Matrix4& viewProjection)
    {
        const auto inverse = viewProjection.inverted();
        const auto width = static_cast<float> (unshadowed.getWidth());
        const auto height = static_cast<float> (unshadowed.getHeight());

        // Where the ray through the point (x, y) of the image, in pixels from the top left, meets the ground
        const auto toGround = [&] (float x, float y)
        {
            const auto ndcX = x / width * 2.0f - 1.0f;
            const auto ndcY = 1.0f - y / height * 2.0f;
            const auto nearPoint = inverse.transformPoint ({ ndcX, ndcY, 0.0f });
            const auto farPoint = inverse.transformPoint ({ ndcX, ndcY, 0.5f });
            return nearPoint + (farPoint - nearPoint) * (nearPoint.getY() / (nearPoint.getY() - farPoint.getY()));
        };

        Vector3<float> sum;
        auto area = 0.0f;

        for (int y = 0; y < unshadowed.getHeight(); ++y)
        {
            for (int x = 0; x < unshadowed.getWidth(); ++x)
            {
                const auto lit = meanLuminance (unshadowed, { x, y, 1, 1 });
                if (lit <= 10.0 || meanLuminance (shadowed, { x, y, 1, 1 }) >= 0.7 * lit)
                    continue;

                // Each pixel weighs the ground area it covers, which shrinks with the distance
                const auto corner = toGround (static_cast<float> (x), static_cast<float> (y));
                const auto right = toGround (static_cast<float> (x + 1), static_cast<float> (y)) - corner;
                const auto down = toGround (static_cast<float> (x), static_cast<float> (y + 1)) - corner;
                const auto weight = std::abs (right.getX() * down.getZ() - right.getZ() * down.getX());

                sum = sum + toGround (static_cast<float> (x) + 0.5f, static_cast<float> (y) + 0.5f) * weight;
                area += weight;
            }
        }

        if (! (area > 0.0f))
            return std::nullopt;

        return sum * (1.0f / area);
    }

    /** A spot of radius 0.3 shadows on @p tier from its center. A blocker 1 m under it and off its
        axis, on the diagonal x = z, is symmetric about the vertical plane x = z holding the spot's
        center, so its shadow centers on the ground line x = z, where the line from the center of
        the spot through the blocker meets the ground: the shadow's centroid lies within 5 cm of that
        line, and within 2 cm of the shadow of the same spot without a radius. Shadowed from a point
        a radius away from the center along x, it would center about 0.4 m off the line.
    */
    void checkAreaSpotCastsAShadowTowardsItsCenter (RenderSettings::TierPreference tier)
    {
        const Vector3<float> blocker (0.35f, 2.0f, 0.35f);
        constexpr int size = 256;

        const auto render = [&] (bool castsShadows, float radius)
        {
            Scene scene;
            auto& spot = buildSpotOverGround (scene, blocker);
            spot.castsShadows = castsShadows;
            spot.radius = radius;

            SceneRenderer renderer;
            setTier (renderer, tier);

            // The direction of the lookup is under test, not the filter: both radii filter alike
            auto settings = renderer.getRenderSettings();
            settings.softShadows = false;
            renderer.setRenderSettings (settings);

            const auto image = renderConverged (renderer, scene, size);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        const auto unshadowed = render (false, 0.3f);
        const auto area = render (true, 0.3f);
        const auto punctual = render (true, 0.0f);
        ASSERT_TRUE (unshadowed.isValid() && area.isValid() && punctual.isValid());

        // The camera of buildSpotOverGround()
        const auto camera = Matrix4::lookAt ({ 0.0f, 3.0f, 4.0f }, {}, { 0.0f, 1.0f, 0.0f }).followedBy (Matrix4::perspective (CameraNode().yFov, 1.0f, 0.1f, 1000.0f));
        const auto areaCentroid = shadowCentroidOnGround (unshadowed, area, camera);
        const auto punctualCentroid = shadowCentroidOnGround (unshadowed, punctual, camera);
        ASSERT_TRUE (areaCentroid.has_value() && punctualCentroid.has_value());

        const auto offLine = std::abs (areaCentroid->getX() - areaCentroid->getZ()) / std::sqrt (2.0f);
        const auto toPunctual = (*areaCentroid - *punctualCentroid).length();
        RecordProperty ("areaOffTheLine", String (offLine, 3).toStdString());
        RecordProperty ("areaToPunctual", String (toPunctual, 3).toStdString());

        EXPECT_LT (offLine, 0.05f) << areaCentroid->getX() << ", " << areaCentroid->getZ();
        EXPECT_LT (toPunctual, 0.02f) << areaCentroid->getX() << ", " << areaCentroid->getZ();
    }

    /** buildPointLightRoom() with 40 more shadow casting point lights renders on @p tier without an
        error, recording whole lights within the shadow record budget of the tier, and stays finite in
        raw radiance (no temporal anti-aliasing on full, the rgba16float bloom path on raster).
    */
    void checkManyShadowedLightsRenderWithoutError (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        Scene scene;
        buildPointLightRoom (scene);
        for (int i = 0; i < 40; ++i)
            addShadowedPointLight (scene, { std::sin (static_cast<float> (i)) * 1.5f, std::cos (static_cast<float> (i) * 0.7f) * 1.5f, 0.5f });

        SceneRenderer renderer;
        if (full)
            setFullTierTaa (renderer, 1);
        else
        {
            setTier (renderer, RenderSettings::TierPreference::raster);
            setBloom (renderer, true);
        }

        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        ASSERT_TRUE (image.isValid());

        // 41 point lights of 6 faces each, without a sun: 2 fit the 16 local records of raster, 10 the 64 of full
        EXPECT_EQ (shadows (renderer).getNumRecords(), full ? 60 : 12);

#if YUP_ENABLE_SHADER_TRANSPILER
        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
#endif
    }

    /** Sets the tier of @p renderer for an exact comparison of frames: raster, or full without temporal
        anti-aliasing (no jitter and a still shadow noise).
    */
    static void setTierForExactFrames (SceneRenderer& renderer, RenderSettings::TierPreference tier)
    {
        if (tier == RenderSettings::TierPreference::full)
            setFullTierTaa (renderer, 1);
        else
            setTier (renderer, RenderSettings::TierPreference::raster);
    }

    /** Sets castsShadows on every light of @p scene. */
    static void setAllLightsCastShadows (Scene& scene, bool castsShadows)
    {
        for (const auto& child : scene.getRoot()->getChildren())
            if (auto* light = child->getNode<LightNode>())
                light->castsShadows = castsShadows;
    }

    /** A device unable to create the comparison sampler cannot read shadow maps: on @p tier it
        renders every frame, without the shadows, as if the sun cast none.
    */
    void checkRendersWithoutTheShadowCompareSampler (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool withSampler, bool castsShadows)
        {
            Scene scene;
            buildShadowsScene (scene);
            scene.getRoot()->findChild ("sun")->getNode<LightNode>()->castsShadows = castsShadows;

            SceneRenderer renderer;
            setTierForExactFrames (renderer, tier);
            if (! withSampler)
                renderer.failShadowCompareSamplerForTesting();

            // The failure is remembered, so the second frame renders like the first
            Image image;
            for (int frame = 0; frame < 2; ++frame)
            {
                image = renderToImage (renderer, scene, 128);
                EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            }

            return image;
        };

        const auto shadowed = render (true, true);
        const auto withoutSampler = render (false, true);
        const auto unshadowed = render (true, false);
        ASSERT_TRUE (shadowed.isValid() && withoutSampler.isValid() && unshadowed.isValid());

        EXPECT_EQ (countDifferences (withoutSampler, unshadowed), 0);
        EXPECT_GT (countDifferences (shadowed, unshadowed), 0);
    }

    /** Shadow pipelines the device cannot compile leave every frame of @p tier unshadowed, not
        failed: the sun scene and the spot scene render as without castsShadows, frame after frame.
    */
    void checkRendersWithoutTheShadowPipelines (RenderSettings::TierPreference tier)
    {
        for (const auto spotScene : { false, true })
        {
            const auto render = [&] (bool failPipelines, bool castsShadows)
            {
                Scene scene;
                if (spotScene)
                    buildSpotOverGround (scene);
                else
                    buildShadowsScene (scene);

                setAllLightsCastShadows (scene, castsShadows);

                SceneRenderer renderer;
                setTierForExactFrames (renderer, tier);
                if (failPipelines)
                    renderer.failShadowPipelinesForTesting();

                Image image;
                for (int frame = 0; frame < 2; ++frame)
                {
                    image = renderToImage (renderer, scene, 128);
                    EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                    if (failPipelines)
                        EXPECT_EQ (shadows (renderer).getNumRecords(), 0);
                }

                return image;
            };

            const auto shadowed = render (false, true);
            const auto withoutPipelines = render (true, true);
            const auto unshadowed = render (false, false);
            ASSERT_TRUE (shadowed.isValid() && withoutPipelines.isValid() && unshadowed.isValid());

            EXPECT_EQ (countDifferences (withoutPipelines, unshadowed), 0) << (spotScene ? "spot" : "sun");
            EXPECT_GT (countDifferences (shadowed, unshadowed), 0) << (spotScene ? "spot" : "sun");
        }
    }

    /** Shadow maps the device cannot create (out of memory) leave the lights needing them
        unshadowed on @p tier, not the frame failed. The same size is not asked for again, so the
        next frame stays unshadowed without retrying it; a new shadowResolution asks again, and the
        shadows come back.
    */
    void checkShadowMapsTheDeviceCannotCreateLeaveTheLightsUnshadowed (RenderSettings::TierPreference tier)
    {
        for (const auto spotScene : { false, true })
        {
            const auto build = [&] (Scene& scene, bool castsShadows)
            {
                if (spotScene)
                    buildSpotOverGround (scene);
                else
                    buildShadowsScene (scene);

                setAllLightsCastShadows (scene, castsShadows);
            };

            Scene unshadowedScene;
            build (unshadowedScene, false);
            SceneRenderer unshadowedRenderer;
            setTierForExactFrames (unshadowedRenderer, tier);

            Scene scene;
            build (scene, true);
            SceneRenderer renderer;
            setTierForExactFrames (renderer, tier);
            renderer.failShadowMapsForTesting();

            // The failing frame, then one that would succeed if it asked again
            for (int frame = 0; frame < 2; ++frame)
            {
                const auto image = renderToImage (renderer, scene, 128);
                const auto unshadowed = renderToImage (unshadowedRenderer, unshadowedScene, 128);
                ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                ASSERT_TRUE (image.isValid() && unshadowed.isValid());

                EXPECT_EQ (countDifferences (image, unshadowed), 0) << (spotScene ? "spot" : "sun") << ", frame " << frame;
                EXPECT_EQ (shadows (renderer).getNumRecords(), 0);
                EXPECT_EQ (shadows (renderer).getAtlasSize(), 0);
            }

            auto settings = renderer.getRenderSettings();
            settings.shadowResolution = 1024;
            renderer.setRenderSettings (settings);

            const auto image = renderToImage (renderer, scene, 128);
            const auto unshadowed = renderToImage (unshadowedRenderer, unshadowedScene, 128);
            ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_GT (shadows (renderer).getNumRecords(), 0);
            EXPECT_GT (countDifferences (image, unshadowed), 0) << (spotScene ? "spot" : "sun");
        }
    }

    /** On @p tier, a shadow map the device could not create does not cost the other one its cache:
        with the sun's cascades drawn, an atlas that fails leaves a still scene drawing no shadow map
        frame after frame, and so do failed cascades next to a drawn atlas.
    */
    void checkFailedShadowMapKeepsTheOtherOneCached (RenderSettings::TierPreference tier)
    {
        for (const auto atlasFails : { true, false })
        {
            Scene scene;
            auto& spot = buildSpotOverGround (scene);
            auto sunEntity = EntityNode::Ptr (new EntityNode ("sun"));
            auto& sun = sunEntity->attach<LightNode>();
            sun.intensity = 2.0f;
            aimCamera (*sunEntity, { 2.0f, 6.0f, 3.0f }, {}, { 0.0f, 1.0f, 0.0f });
            scene.getRoot()->addChild (sunEntity);

            // The map that survives exists first; the other one fails when it is first needed
            sun.castsShadows = atlasFails;
            spot.castsShadows = ! atlasFails;

            SceneRenderer renderer;
            setTier (renderer, tier);
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

            renderer.failShadowMapsForTesting();
            sun.castsShadows = true;
            spot.castsShadows = true;
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_EQ (shadows (renderer).getAtlasSize() == 0, atlasFails);

            const auto renders = shadows (renderer).getNumRenderedFrames();
            const auto cascadeRenders = shadows (renderer).getNumCascadeRenders (0);
            const auto draws = shadows (renderer).getNumCasterDraws();

            for (int frame = 0; frame < 3; ++frame)
                ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

            EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), renders) << (atlasFails ? "atlas fails" : "cascades fail");
            EXPECT_EQ (shadows (renderer).getNumCascadeRenders (0), cascadeRenders) << (atlasFails ? "atlas fails" : "cascades fail");
            EXPECT_EQ (shadows (renderer).getNumCasterDraws(), draws) << (atlasFails ? "atlas fails" : "cascades fail");
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
        }
    }

    /** A shadowed point light whose range reaches no receiver takes no tile on @p tier, and the frame
        is the one it gives without shadows; brought within reach of the ground it takes its six faces.
    */
    void checkShadowedLightOutOfReachAllocatesNoAtlas (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (Vector3<float> position, bool castsShadows, int& records, int& atlasSize)
        {
            Scene scene;
            buildSpotOverGround (scene).castsShadows = false;

            auto entity = EntityNode::Ptr (new EntityNode ("point"));
            auto& light = entity->attach<LightNode> (LightNode::Type::point);
            light.range = 1.0f;
            light.castsShadows = castsShadows;
            entity->setPosition (position);
            scene.getRoot()->addChild (entity);

            SceneRenderer renderer;
            setTier (renderer, tier);
            const auto image = renderConverged (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            records = shadows (renderer).getNumRecords();
            atlasSize = shadows (renderer).getAtlasSize();
            return image;
        };

        int records = -1, atlasSize = -1;
        const auto far = render ({ 0.0f, 50.0f, 0.0f }, true, records, atlasSize);
        EXPECT_EQ (records, 0);
        EXPECT_EQ (atlasSize, 0);

        const auto unshadowed = render ({ 0.0f, 50.0f, 0.0f }, false, records, atlasSize);
        ASSERT_TRUE (far.isValid() && unshadowed.isValid());
        EXPECT_EQ (countDifferences (far, unshadowed), 0);

        render ({ 2.0f, 0.5f, 0.0f }, true, records, atlasSize);
        EXPECT_EQ (records, 6);
        EXPECT_GT (atlasSize, 0);
    }

    /** One renderer on @p tier releases the atlas when its only shadowed spot stops casting
        shadows, and creates it again when it casts them again.
    */
    void checkAtlasIsReleasedWhenNoLightNeedsIt (RenderSettings::TierPreference tier)
    {
        Scene scene;
        auto& spot = buildSpotOverGround (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        for (const auto castsShadows : { true, false, true })
        {
            spot.castsShadows = castsShadows;
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

            if (castsShadows)
                EXPECT_GT (shadows (renderer).getAtlasSize(), 0);
            else
                EXPECT_EQ (shadows (renderer).getAtlasSize(), 0);
        }

        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** The spot of buildSpotOverGround() and a shadowed point light of 1.2 m range at (4, 1, 0), its
        own blocker under it at (4, 0.4, 0), out of the spot's cone, plus a third blocker at (-4, 0.3,
        -4) which no light reaches, seen from above both lights. Returns the point light's blocker,
        the spot's blocker and the third one.
    */
    static std::array<EntityNode*, 3> buildSpotAndPointOverGround (Scene& scene)
    {
        buildSpotOverGround (scene);
        auto* spotBlocker = scene.getRoot()->findChild ("blocker");

        auto entity = EntityNode::Ptr (new EntityNode ("point"));
        auto& point = entity->attach<LightNode> (LightNode::Type::point);
        point.intensity = 4.0f;
        point.range = 1.2f;
        point.castsShadows = true;
        entity->setPosition ({ 4.0f, 1.0f, 0.0f });
        scene.getRoot()->addChild (entity);

        addBlocker (scene, { 4.0f, 0.4f, 0.0f });
        auto* pointBlocker = scene.getRoot()->getChildren().back().get();

        addBlocker (scene, { -4.0f, 0.3f, -4.0f });
        auto* unlitBlocker = scene.getRoot()->getChildren().back().get();

        addCamera (scene, { 2.0f, 6.0f, 6.0f }, { 2.0f, 0.0f, 0.0f });
        return { pointBlocker, spotBlocker, unlitBlocker };
    }

    /** On @p tier, a caster moving outside a light's reach does not draw that light's atlas views
        again: moving the point light's blocker costs the draws it costs without the spot's shadow,
        and moving a blocker no light reaches draws no shadow map at all.
    */
    void checkAtlasRedrawsOnlyTheViewsACasterReaches (RenderSettings::TierPreference tier)
    {
        const auto drawsOfAMove = [&] (bool spotCastsShadows)
        {
            Scene scene;
            const auto blockers = buildSpotAndPointOverGround (scene);
            scene.getRoot()->findChild ("spot")->getNode<LightNode>()->castsShadows = spotCastsShadows;

            SceneRenderer renderer;
            setTier (renderer, tier);
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            const auto drawsBefore = shadows (renderer).getNumCasterDraws();

            blockers[0]->setPosition (blockers[0]->getPosition() + Vector3<float> (0.1f, 0.0f, 0.0f));
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            return shadows (renderer).getNumCasterDraws() - drawsBefore;
        };

        const auto draws = drawsOfAMove (true);
        EXPECT_GT (draws, 0);
        EXPECT_EQ (draws, drawsOfAMove (false));

        Scene scene;
        const auto blockers = buildSpotAndPointOverGround (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);

        blockers[2]->setPosition (blockers[2]->getPosition() + Vector3<float> (0.1f, 0.0f, 0.0f));
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), 1);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, an atlas whose views were drawn again one light at a time, each cleared alone
        while the other tiles were kept, holds what a fresh renderer draws at once: the frames match
        exactly.
    */
    void checkPartiallyRedrawnAtlasMatchesAFreshOne (RenderSettings::TierPreference tier)
    {
        Scene scene;
        const auto blockers = buildSpotAndPointOverGround (scene);

        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        ASSERT_TRUE (renderToImage (renderer, scene, 128).isValid());

        // Only the point light's views, then only the spot's
        for (auto* blocker : { blockers[0], blockers[1] })
        {
            const auto drawsBefore = shadows (renderer).getNumCasterDraws();
            blocker->setPosition (blocker->getPosition() + Vector3<float> (0.1f, 0.0f, 0.05f));
            ASSERT_TRUE (renderToImage (renderer, scene, 128).isValid());
            ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_GT (shadows (renderer).getNumCasterDraws(), drawsBefore);
        }

        const auto redrawn = renderToImage (renderer, scene, 128);

        SceneRenderer fresh;
        setTierForExactFrames (fresh, tier);
        const auto drawnAtOnce = renderToImage (fresh, scene, 128);
        ASSERT_TRUE (redrawn.isValid() && drawnAtOnce.isValid());
        EXPECT_EQ (countDifferences (redrawn, drawnAtOnce), 0);

        // The spot's view and the point light's six
        EXPECT_EQ (shadows (renderer).getNumRecords(), 7);
    }

    /** buildSpotOverGround() plus @p extraSpots more shadow casting spots like it, 2 m from it on the
        diagonals, each over its own blocker, set 15 cm further off its axis than the previous one so
        no two spots draw the same depth: the camera is inside the reach of every spot, so each asks
        for the largest tile. Returns the extra spots. */
    static std::vector<LightNode*> buildSpotsOverGround (Scene& scene, int extraSpots)
    {
        buildSpotOverGround (scene);

        std::vector<LightNode*> spots;
        for (int i = 0; i < extraSpots; ++i)
        {
            const Vector3<float> foot ((i % 2 == 0 ? 2.0f : -2.0f), 0.0f, (i / 2 % 2 == 0 ? 2.0f : -2.0f));
            addBlocker (scene, foot + Vector3<float> (0.15f * static_cast<float> (i + 1), 1.0f, 0.0f));

            auto entity = EntityNode::Ptr (new EntityNode ("spot" + String (i + 1)));
            auto& spot = entity->attach<LightNode> (LightNode::Type::spot);
            spot.intensity = 20.0f;
            spot.castsShadows = true;
            aimCamera (*entity, foot + Vector3<float> (0.0f, 3.0f, 0.0f), foot, { 0.0f, 0.0f, -1.0f });
            scene.getRoot()->addChild (entity);
            spots.push_back (&spot);
        }

        return spots;
    }

    /** The tiles of the point and spot light views of @p renderer's last frame, in atlas texels:
        the records with a map after its @p numCascades cascades. */
    static std::vector<Rectangle<int>> getLocalViewTiles (const SceneRenderer& renderer, int numCascades)
    {
        const auto data = shadows (renderer).getShadowData();
        const auto atlasSize = static_cast<float> (shadows (renderer).getAtlasSize());
        std::vector<Rectangle<int>> tiles;

        for (int r = numCascades; r < shadows (renderer).getNumRecords(); ++r)
        {
            const auto& rect = data.records[r].atlasRect;
            if (rect[2] > 0.0f)
                tiles.emplace_back (roundToInt (rect[0] * atlasSize), roundToInt (rect[1] * atlasSize), roundToInt (rect[2] * atlasSize), roundToInt (rect[2] * atlasSize));
        }

        return tiles;
    }

    /** On @p tier, a single shadow casting spot light near the camera gets a tile of half the atlas side. */
    void checkSingleSpotTakesHalfTheAtlas (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildSpotOverGround (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());

        ASSERT_GT (shadows (renderer).getAtlasSize(), 0);
        EXPECT_EQ (shadows (renderer).getTileSize (0), shadows (renderer).getAtlasSize() / 2);
    }

    /** On @p tier, spot lights each asking for the largest tile keep half the atlas side while all of
        them fit at that size (4 of them), and shrink back to a quarter of it, the cap before it was
        raised, once one more would be left without a tile: every spot keeps a tile inside the atlas,
        and no two tiles overlap. */
    void checkLocalViewsShrinkToFitTheAtlas (RenderSettings::TierPreference tier)
    {
        for (const auto& [extraSpots, divisor] : { std::pair { 3, 2 }, std::pair { 4, 4 }, std::pair { 7, 4 } })
        {
            Scene scene;
            buildSpotsOverGround (scene, extraSpots);

            SceneRenderer renderer;
            setTier (renderer, tier);
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());

            const auto atlasSize = shadows (renderer).getAtlasSize();
            ASSERT_GT (atlasSize, 0);

            for (int light = 0; light <= extraSpots; ++light)
                EXPECT_EQ (shadows (renderer).getTileSize (light), atlasSize / divisor) << extraSpots + 1 << " spots, light " << light;

            const auto tiles = getLocalViewTiles (renderer, 0);
            ASSERT_EQ (static_cast<int> (tiles.size()), extraSpots + 1);

            for (size_t i = 0; i < tiles.size(); ++i)
            {
                EXPECT_TRUE (Rectangle<int> (0, 0, atlasSize, atlasSize).contains (tiles[i]));

                for (size_t j = i + 1; j < tiles.size(); ++j)
                    EXPECT_FALSE (tiles[i].intersection (tiles[j]).area() > 0) << extraSpots + 1 << " spots, views " << i << " and " << j;
            }
        }
    }

    /** On @p tier, three spot lights asking for half the atlas side do not squeeze a shadowed point
        light after them below the tile it gets with tiles of at most a quarter: on half tiles the
        spots would take three quarters of the atlas and leave the point light's six views an eighth
        of the side, so every light keeps a quarter, without overlaps. */
    void checkLocalLightsKeepTheirQuarterTiles (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildSpotsOverGround (scene, 2);
        addShadowedPointLight (scene, { 0.0f, 1.5f, 2.0f });

        SceneRenderer renderer;
        setTier (renderer, tier);
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());

        const auto atlasSize = shadows (renderer).getAtlasSize();
        ASSERT_GT (atlasSize, 0);

        for (int light = 0; light < 4; ++light)
            EXPECT_EQ (shadows (renderer).getTileSize (light), atlasSize / 4) << "light " << light;

        const auto tiles = getLocalViewTiles (renderer, 0);
        ASSERT_EQ (tiles.size(), 9u);

        for (size_t i = 0; i < tiles.size(); ++i)
            for (size_t j = i + 1; j < tiles.size(); ++j)
                EXPECT_FALSE (tiles[i].intersection (tiles[j]).area() > 0) << "views " << i << " and " << j;
    }

    /** On @p tier, the atlas layout follows the spot lights casting shadows, and each frame draws the
        views whose tile moved again (one more shadow map render) and matches, exactly, a fresh
        renderer drawing the same scene at once: four spots on half the atlas side, then eight
        shrinking to a quarter, then seven, which moves the views after the first one to the tiles
        before them at the same size, then the four again. */
    void checkAtlasRelayoutRedrawsTheMovedTiles (RenderSettings::TierPreference tier)
    {
        Scene scene;
        auto spots = buildSpotsOverGround (scene, 7);
        spots.insert (spots.begin(), scene.getRoot()->findChild ("spot")->getNode<LightNode>());

        const auto castShadows = [&] (int first, int last)
        {
            for (int i = 0; i < static_cast<int> (spots.size()); ++i)
                spots[static_cast<size_t> (i)]->castsShadows = i >= first && i < last;
        };

        castShadows (0, 4);
        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        ASSERT_TRUE (renderToImage (renderer, scene, 128).isValid());

        const auto atlasSize = shadows (renderer).getAtlasSize();
        EXPECT_EQ (shadows (renderer).getTileSize (1), atlasSize / 2);

        const struct
        {
            const char* name;
            int first, last, divisor;
        } steps[] = {
            { "eight spots", 0, 8, 4 },
            { "seven spots", 1, 8, 4 },
            { "four spots", 0, 4, 2 },
        };

        for (const auto& step : steps)
        {
            const auto renders = shadows (renderer).getNumRenderedFrames();
            castShadows (step.first, step.last);

            const auto relaidOut = renderToImage (renderer, scene, 128);
            ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (shadows (renderer).getNumRenderedFrames(), renders + 1) << step.name;
            EXPECT_EQ (shadows (renderer).getTileSize (1), atlasSize / step.divisor) << step.name;

            SceneRenderer fresh;
            setTierForExactFrames (fresh, tier);
            const auto drawnAtOnce = renderToImage (fresh, scene, 128);
            ASSERT_TRUE (relaidOut.isValid() && drawnAtOnce.isValid());
            EXPECT_EQ (countDifferences (relaidOut, drawnAtOnce), 0) << step.name;
        }
    }

    /** The scene of the soft shadow tests: a wall 1.2 m high and 2 cm thin across the 4 m ground,
        from x = -2 to its end at x = 0 on z = 0, lit from behind (-z) by a shadow casting sun 45
        degrees high with an angular radius of @p sunRadius. Its shadow covers x < 0 from z = 0 to
        z = 1.2, so the side of the shadow cast by the wall's end runs along z at x = 0, ever further
        below the edge casting it. An orthographic camera looks straight down at x in [-0.85, 0.85]
        and z in [0.05, 1.75]: the rows of the frame run across that side, which crosses the center
        column, and the shadow touches the edge of the frame at the wall. Returns the sun.
    */
    static LightNode& buildPenumbraScene (Scene& scene, float sunRadius)
    {
        addGround (scene, 2.0f);

        auto wall = EntityNode::Ptr (new EntityNode ("wall"));
        wall->attach<MeshNode> (makeBox (0.5f, makeWallMaterial()));
        wall->setScale ({ 2.0f, 1.2f, 0.02f });
        wall->setPosition ({ -1.0f, 0.6f, -0.01f });
        scene.getRoot()->addChild (wall);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        auto& light = sun->attach<LightNode>();
        light.castsShadows = true;
        light.radius = sunRadius;
        aimCamera (*sun, { 0.0f, 7.0f, -7.0f }, {});
        scene.getRoot()->addChild (sun);

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        auto& lens = camera->attach<CameraNode>();
        lens.projection = CameraNode::Projection::orthographic;
        lens.xMag = 0.85f;
        lens.yMag = 0.85f;
        aimCamera (*camera, { 0.0f, 5.0f, 0.9f }, { 0.0f, 0.0f, 0.9f }, { 0.0f, 0.0f, -1.0f });
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        scene.setAmbientColor (Color (0xff3f3f3f)); // 0.05 once linear
        return light;
    }

    /** buildPenumbraScene() on @p tier at 256 x 256, converged, with soft shadows set to @p softShadows.
        A @p phase above 0 renders that many frames first and restarts the accumulation, so the
        converged frames run through other jitter offsets and noise rotations: the frame index,
        which drives both, keeps counting across a restart. */
    Image renderPenumbraScene (RenderSettings::TierPreference tier, float sunRadius, bool softShadows, int phase = 0)
    {
        Scene scene;
        buildPenumbraScene (scene, sunRadius);

        SceneRenderer renderer;
        setTier (renderer, tier);

        auto settings = renderer.getRenderSettings();
        settings.softShadows = softShadows;
        renderer.setRenderSettings (settings);

        for (int frame = 0; frame < phase; ++frame)
            EXPECT_NE (renderer.render (device, scene, 256, 256), nullptr) << renderer.getLastError();

        if (phase > 0)
            renderer.restartAccumulation();

        const auto image = renderConverged (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    }

    /** The luma of every pixel of row @p y, from the left. */
    static std::vector<double> luminanceRow (const Image& image, int y)
    {
        std::vector<double> row;
        for (int x = 0; x < image.getWidth(); ++x)
            row.push_back (meanLuminance (image, { x, y, 1, 1 }));

        return row;
    }

    /** The pixels of row @p y between 10% and 90% of the way from its darkest to its brightest level. */
    static int penumbraWidthAtRow (const Image& image, int y)
    {
        const auto row = luminanceRow (image, y);
        const auto [darkest, brightest] = std::minmax_element (row.begin(), row.end());
        const auto low = *darkest + 0.1 * (*brightest - *darkest);
        const auto high = *darkest + 0.9 * (*brightest - *darkest);

        return static_cast<int> (std::count_if (row.begin(), row.end(), [&] (double level) { return level > low && level < high; }));
    }

    /** penumbraWidthAtRow() along the center row. */
    static int penumbraWidth (const Image& image)
    {
        return penumbraWidthAtRow (image, image.getHeight() / 2);
    }

    /** The rows where the wall's shadow of buildPenumbraScene() starts and ends, found down a column
        inside it: first the end at the wall, which touches the edge of the frame, then the far end. */
    static std::pair<int, int> findWallShadowRows (const Image& image)
    {
        const auto x = image.getWidth() / 4;
        std::vector<double> column;
        for (int y = 0; y < image.getHeight(); ++y)
            column.push_back (meanLuminance (image, { x, y, 1, 1 }));

        const auto [darkest, brightest] = std::minmax_element (column.begin(), column.end());
        const auto middle = 0.5 * (*darkest + *brightest);
        const auto isShadowed = [middle] (double level) { return level < middle; };

        const auto first = static_cast<int> (std::find_if (column.begin(), column.end(), isShadowed) - column.begin());
        const auto last = static_cast<int> (column.rend() - std::find_if (column.rbegin(), column.rend(), isShadowed)) - 1;

        return first == 0 ? std::pair { first, last } : std::pair { last, first };
    }

    /** A row a quarter of the way along the wall's shadow from the wall. */
    static int nearRow (const Image& image)
    {
        const auto [wall, end] = findWallShadowRows (image);
        return wall + (end - wall) / 4;
    }

    /** A row a quarter of the way along the wall's shadow from its far end. */
    static int farRow (const Image& image)
    {
        const auto [wall, end] = findWallShadowRows (image);
        return end - (end - wall) / 4;
    }

    /** With soft shadows on @p tier, the side of the wall's shadow is wider under a larger sun. */
    void checkPcssPenumbraWidensWithLightRadius (RenderSettings::TierPreference tier)
    {
        const auto sharpImage = renderPenumbraScene (tier, 0.002f, true);
        const auto softImage = renderPenumbraScene (tier, 0.05f, true);
        ASSERT_TRUE (sharpImage.isValid() && softImage.isValid());

        const auto sharp = penumbraWidth (sharpImage);
        const auto soft = penumbraWidth (softImage);
        RecordProperty ("sharp", sharp);
        RecordProperty ("soft", soft);
        EXPECT_GT (soft, sharp + 4);
    }

    /** With soft shadows on @p tier, the side of the wall's shadow widens away from the wall, as the
        edge casting it lies further above the ground. */
    void checkPenumbraWidensWithDistanceFromTheBlocker (RenderSettings::TierPreference tier)
    {
        const auto image = renderPenumbraScene (tier, 0.05f, true);
        ASSERT_TRUE (image.isValid());

        const auto near = penumbraWidthAtRow (image, nearRow (image));
        const auto far = penumbraWidthAtRow (image, farRow (image));
        RecordProperty ("near", near);
        RecordProperty ("far", far);
        EXPECT_GT (far, near);
    }

    /** Adds to buildPenumbraScene() a shadow casting spot light of radius @p radius, behind the wall
        at (0, 2.4, -2.4) and aimed at the middle of the frame: the side of the wall's shadow still
        runs along x = 0, across the rows of the frame. */
    static void addPenumbraSpot (Scene& scene, float radius)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("spot"));
        auto& spot = entity->attach<LightNode> (LightNode::Type::spot);
        spot.intensity = 40.0f;
        spot.outerConeAngle = 0.8f;
        spot.innerConeAngle = 0.7f;
        spot.radius = radius;
        spot.castsShadows = true;
        aimCamera (*entity, { 0.0f, 2.4f, -2.4f }, { 0.0f, 0.0f, 0.9f });
        scene.getRoot()->addChild (entity);
    }

    /** buildPenumbraScene() lit by addPenumbraSpot() of radius @p radius instead of the sun, on
        @p tier at 256 x 256, converged, with soft shadows set to @p softShadows. */
    Image renderSpotPenumbraScene (RenderSettings::TierPreference tier, float radius, bool softShadows)
    {
        Scene scene;
        buildPenumbraScene (scene, 0.0f).intensity = 0.0f;
        addPenumbraSpot (scene, radius);

        SceneRenderer renderer;
        setTier (renderer, tier);

        auto settings = renderer.getRenderSettings();
        settings.softShadows = softShadows;
        renderer.setRenderSettings (settings);

        const auto image = renderConverged (renderer, scene, 256);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    }

    /** With soft shadows on @p tier, the side of the wall's shadow under a spot light is wider when
        the light is larger, and only through soft shadows: the same large light filtered hard
        stays as narrow as the small one. */
    void checkSpotPenumbraWidensWithLightRadius (RenderSettings::TierPreference tier)
    {
        const auto sharpImage = renderSpotPenumbraScene (tier, 0.002f, true);
        const auto softImage = renderSpotPenumbraScene (tier, 0.2f, true);
        const auto hardImage = renderSpotPenumbraScene (tier, 0.2f, false);
        ASSERT_TRUE (sharpImage.isValid() && softImage.isValid() && hardImage.isValid());

        const auto sharp = penumbraWidth (sharpImage);
        const auto soft = penumbraWidth (softImage);
        const auto hard = penumbraWidth (hardImage);
        RecordProperty ("sharp", sharp);
        RecordProperty ("soft", soft);
        RecordProperty ("hard", hard);
        EXPECT_GT (soft, sharp + 4);
        EXPECT_LE (hard, sharp + 2);
    }

    /** The width in texels of shadow map @p record of the side of the wall's shadow of
        buildPenumbraScene() at 256 x 256, between 10% and 90% of the way from the shadow to the
        light: the linear radiance (reinhardRadiance()) of the rows around the middle of the frame,
        averaged per column, crossing the levels 60 columns (0.4 m) on either side of the edge,
        interpolated. The record's texels are
        measured across the edge, along x, at the middle of the frame. */
    static double getPenumbraTexels (const Image& image, const GpuShadowRecord& record)
    {
        std::vector<double> profile;
        for (int x = 0; x < image.getWidth(); ++x)
        {
            auto radiance = 0.0;
            for (int y = 96; y < 160; ++y)
                radiance += reinhardRadiance (roundToInt (meanLuminance (image, { x, y, 1, 1 })));

            profile.push_back (radiance / 64.0);
        }

        const auto center = image.getWidth() / 2;
        const auto dark = std::min (profile[static_cast<size_t> (center - 60)], profile[static_cast<size_t> (center + 60)]);
        const auto lit = std::max (profile[static_cast<size_t> (center - 60)], profile[static_cast<size_t> (center + 60)]);
        const auto step = profile[static_cast<size_t> (center - 60)] < profile[static_cast<size_t> (center + 60)] ? 1 : -1;

        // The first column from the shadow's side reaching a level, with its fraction
        const auto crossing = [&] (double level)
        {
            for (auto x = center - 60 * step; x != center + 60 * step; x += step)
            {
                const auto a = profile[static_cast<size_t> (x)];
                const auto b = profile[static_cast<size_t> (x + step)];
                if (a < level && b >= level)
                    return static_cast<double> (x) + static_cast<double> (step) * (level - a) / (b - a);
            }

            return static_cast<double> (center + 60 * step);
        };

        const auto pixels = std::abs (crossing (dark + 0.9 * (lit - dark)) - crossing (dark + 0.1 * (lit - dark)));

        std::array<float, 16> values;
        std::copy_n (record.viewProjection, 16, values.begin());
        const Matrix4 viewProjection (values);
        const auto u = [&] (float x)
        {
            const auto clip = viewProjection.transformPoint4 (x, 0.0f, 0.9f, 1.0f);
            return clip[0] / clip[3] * 0.5f * record.atlasRect[2];
        };

        // The orthographic camera of buildPenumbraScene() shows 1.7 m across the frame
        const auto texelsPerMeter = std::abs (u (0.01f) - u (0.0f)) / record.atlasRect[3] / 0.01f;
        const auto pixelsPerMeter = static_cast<double> (image.getWidth()) / 1.7;
        return pixels / pixelsPerMeter * texelsPerMeter;
    }

    /** With soft shadows on @p tier, a sun and a spot light of almost no size still filter their
        shadows over a disk of at least 2 texels of their map, where the shadow starts at its caster:
        the side of the wall's shadow spans at least 2.5 texels between 10% and 90% (a disk of 2
        texels spans 2.75, one of 1 texel 1.4). The maps are coarse (shadowResolution 256), so a
        texel spans several pixels. */
    void checkSoftShadowsFilterAtLeastTwoTexels (RenderSettings::TierPreference tier)
    {
        for (const auto spot : { false, true })
        {
            Scene scene;
            auto& sun = buildPenumbraScene (scene, 0.002f);
            if (spot)
            {
                sun.intensity = 0.0f;
                addPenumbraSpot (scene, 0.002f);
            }

            SceneRenderer renderer;
            auto settings = renderer.getRenderSettings();
            settings.tier = tier;
            settings.softShadows = true;
            settings.shadowResolution = 256;
            renderer.setRenderSettings (settings);

            const auto image = renderConverged (renderer, scene, 256);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
            ASSERT_EQ (shadows (renderer).getNumRecords(), 1);

            const auto texels = getPenumbraTexels (image, shadows (renderer).getShadowData().records[0]);
            RecordProperty (spot ? "spotTexels" : "sunTexels", String (texels, 2).toStdString());
            EXPECT_GE (texels, 2.5) << (spot ? "spot" : "sun");
        }
    }

    /** Raster keeps the hardware filtered shadow by default, and opts in to soft shadows. */
    void checkRasterSoftShadowsOptIn()
    {
        Scene scene;
        buildPenumbraScene (scene, 0.05f);

        SceneRenderer hard;
        setTier (hard, RenderSettings::TierPreference::raster);
        const auto hardWidth = penumbraWidth (renderToImage (hard, scene, 256));
        EXPECT_LE (hardWidth, 4); // hardware PCF: today's narrow tent

        auto settings = hard.getRenderSettings();
        settings.softShadows = true;

        SceneRenderer soft;
        soft.setRenderSettings (settings);
        const auto softImage = renderToImage (soft, scene, 256);
        ASSERT_TRUE (soft.getLastError().isEmpty()) << soft.getLastError();

        const auto softWidth = penumbraWidth (softImage);
        RecordProperty ("hard", hardWidth);
        RecordProperty ("soft", softWidth);
        EXPECT_GT (softWidth, hardWidth + 4);
    }

    /** Raster shades with its soft shadow variant only in the frames that soften a shadow: turning
        soft shadows on and off again within one renderer comes back to the first frame, the soft
        frame matches a renderer drawing it from the start, and a sun without a size casts the same
        shadow with soft shadows on as off. */
    void checkRasterSoftShadowVariantFollowsTheSettings()
    {
        Scene scene;
        auto& sun = buildPenumbraScene (scene, 0.05f);

        const auto setSoftShadows = [] (SceneRenderer& target, bool soft)
        {
            auto settings = target.getRenderSettings();
            settings.tier = RenderSettings::TierPreference::raster;
            settings.softShadows = soft;
            target.setRenderSettings (settings);
        };

        SceneRenderer renderer;
        std::array<Image, 3> frames;
        const std::array<bool, 3> soft { false, true, false };

        for (size_t i = 0; i < frames.size(); ++i)
        {
            setSoftShadows (renderer, soft[i]);
            frames[i] = renderToImage (renderer, scene, 128);
            ASSERT_TRUE (frames[i].isValid()) << i << ": " << renderer.getLastError();
        }

        // The canvas read back dithers the frames by a level, within what countDifferences() ignores
        EXPECT_EQ (countDifferences (frames[0], frames[2]), 0);
        EXPECT_GT (countDifferences (frames[1], frames[0]), 0);

        setSoftShadows (renderer, true);
        SceneRenderer fresh;
        setSoftShadows (fresh, true);
        const auto softFrame = renderToImage (fresh, scene, 128);
        ASSERT_TRUE (softFrame.isValid()) << fresh.getLastError();
        EXPECT_EQ (countDifferences (frames[1], softFrame), 0);

        sun.radius = 0.0f;
        const auto pointSun = renderToImage (renderer, scene, 128);
        setSoftShadows (renderer, false);
        const auto pointSunHard = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (pointSun.isValid() && pointSunHard.isValid()) << renderer.getLastError();
        EXPECT_EQ (countDifferences (pointSun, pointSunHard), 0);
    }

    /** A large sun (0.2 rad) 15 degrees above an empty ground casts no more shadow on it with soft
        shadows than with the hard filter on @p tier: no pixel darkens by more than 5% on raster, 2%
        on full, and more than the level the canvas read back dithers by. The ground falls steeply
        away in the sun's depth, so its own depth must not shadow it across the wide soft shadow
        kernels. */
    void checkSoftSunLeavesAGrazedGroundLit (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool softShadows)
        {
            Scene scene;
            addGround (scene, 2.0f);
            scene.setAmbientColor (Color (0xff3f3f3f));

            auto sun = EntityNode::Ptr (new EntityNode ("sun"));
            auto& light = sun->attach<LightNode>();
            light.castsShadows = true;
            light.radius = 0.2f;
            aimCamera (*sun, { -10.0f * std::cos (0.26f), 10.0f * std::sin (0.26f), 0.0f }, {});
            scene.getRoot()->addChild (sun);

            addCamera (scene, { 0.0f, 3.0f, 3.0f }, {});

            SceneRenderer renderer;
            setTier (renderer, tier);

            auto settings = renderer.getRenderSettings();
            settings.softShadows = softShadows;
            renderer.setRenderSettings (settings);

            const auto image = renderConverged (renderer, scene, 256);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        const auto shadowed = render (true);
        const auto unshadowed = render (false);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());

        const auto threshold = tier == RenderSettings::TierPreference::full ? 0.98 : 0.95;
        auto worst = 1.0;
        auto darkened = 0;
        for (int y = 0; y < shadowed.getHeight(); ++y)
        {
            for (int x = 0; x < shadowed.getWidth(); ++x)
            {
                const auto lit = meanLuminance (unshadowed, { x, y, 1, 1 });
                if (lit <= 10.0)
                    continue;

                const auto level = meanLuminance (shadowed, { x, y, 1, 1 });
                if (lit - level <= 2.0)
                    continue;

                const auto relative = level / lit;
                worst = jmin (worst, relative);
                darkened += relative < threshold ? 1 : 0;
            }
        }

        RecordProperty ("darkestRelative", String (worst, 3).toStdString());
        RecordProperty ("darkenedPixels", darkened);
        EXPECT_EQ (darkened, 0) << "darkest " << worst;
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
                                .getChildFile ("modules/yup_renderer/shaders/yup_Fullscreen.vert")
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
        EXPECT_TRUE (frame.waitForGPU().wasOk());

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

    /** Writes @p expression, a vec4 computed from the texel t of @p texture, into an rgba8 image of
        its size. @p functions are GLSL declared before main(). */
    Image mapTexels (const GpuTexture::Ptr& texture, const String& expression, const String& functions = {})
    {
        return runFullscreen ("#version 450\n" + functions + R"(
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(location = 0) out vec4 color;
void main()
{
    vec4 t = texelFetch(sampler2D(source, sourceSampler), ivec2(gl_FragCoord.xy), 0);
    color = )" + expression + R"(;
})",
                              texture);
    }

    /** The source of lib/octahedral.glsl, for the test shaders decoding a G-buffer RT1. */
    static String loadOctahedralLibrary()
    {
        return File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
            .getChildFile ("modules/yup_renderer/shaders/lib/octahedral.glsl")
            .loadFileAsString();
    }

    /** The shading normal of a G-buffer RT1 decoded through lib/octahedral.glsl, as n * 0.5 + 0.5 in rgb. */
    Image readGBufferNormal (const GpuTexture::Ptr& normalRoughness)
    {
        return mapTexels (normalRoughness, "vec4(octahedralDecode(t.xy) * 0.5 + 0.5, 1.0)", loadOctahedralLibrary());
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

    /** Renders @p scene at @p size x @p size with raw radiance in the scene color (no temporal
        anti-aliasing on full, the rgba16float bloom path on raster), and returns the exposed luma of
        every pixel, row by row from the top, or nothing if the frame failed.
    */
    std::vector<double> renderLuma (Scene& scene, int size, RenderSettings::TierPreference tier)
    {
        SceneRenderer renderer;
        if (tier == RenderSettings::TierPreference::full)
            setFullTierTaa (renderer, 1);
        else
        {
            setTier (renderer, RenderSettings::TierPreference::raster);
            setBloom (renderer, true);
        }

        const auto sceneColor = renderer.render (device, scene, size, size) != nullptr ? renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor) : nullptr;
        if (sceneColor == nullptr)
        {
            ADD_FAILURE() << renderer.getLastError();
            return {};
        }

        // Raster scene color is reversibly tone mapped and premultiplied by the MSAA coverage. The
        // luma goes out as 16 bits of its log2 over [-20, 20]: the high byte in red, the low in green
        const auto color = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory()
                               .getChildFile ("modules/yup_renderer/shaders/lib/color.glsl")
                               .loadFileAsString();

        const auto radiance = tier == RenderSettings::TierPreference::full ? "t.rgb" : "yupUnpremultiplyInverse(t)";
        const auto image = mapTexels (sceneColor, String ("encodeLuma(") + radiance + ")", color + R"(
vec4 encodeLuma(vec3 c)
{
    float e = clamp((log2(max(yupLuma(c), 1.0e-6)) + 20.0) / 40.0, 0.0, 1.0) * 255.0;
    return vec4(floor(e) / 255.0, fract(e), 0.0, 1.0);
})");

        if (! image.isValid())
        {
            ADD_FAILURE() << "Unable to read the scene color back";
            return {};
        }

        std::vector<double> luma;
        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const auto texel = image.getPixelColor (x, y);
                const auto e = (texel.getRed() + texel.getGreen() / 255.0) / 255.0;
                luma.push_back (e > 0.0 ? std::exp2 (e * 40.0 - 20.0) : 0.0);
            }
        }

        return luma;
    }

    /** The mean of @p luma, an image @p size pixels wide from renderLuma(), within @p area. */
    static double meanLuma (const std::vector<double>& luma, int size, Rectangle<int> area)
    {
        if (luma.empty())
            return -1.0;

        auto sum = 0.0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
            for (int x = area.getX(); x < area.getRight(); ++x)
                sum += luma[static_cast<size_t> (y * size + x)];

        return sum / static_cast<double> (area.getWidth() * area.getHeight());
    }

    /** The 256 light grid keeps every quad lit and the scene color finite on @p tier, through the rgba16float path on raster. */
    void checkSceneColorHasNoNaN (RenderSettings::TierPreference tier)
    {
        // The 256 light grid, one draw per quad, plus a zero-area triangle (which draws no fragments
        // and only exercises the bounds, the light selection and the clusters) and a light at the
        // camera position, the real shading stress
        Scene scene;
        buildLightGridScene (scene, false);

        std::vector<Mesh::Vertex> vertices (3);
        for (auto& vertex : vertices)
        {
            vertex.position = { 0.1f, 0.1f, 0.2f };
            vertex.normal = { 0.0f, 0.0f, 1.0f };
        }

        auto mesh = Mesh::Ptr (new Mesh ("degenerate"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2 });
        auto degenerate = EntityNode::Ptr (new EntityNode ("degenerate"));
        degenerate->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (degenerate);

        auto atCamera = EntityNode::Ptr (new EntityNode ("atCamera"));
        atCamera->attach<LightNode> (LightNode::Type::point).intensity = 10.0f;
        atCamera->setPosition ({ 0.0f, 0.0f, 9.0f });
        scene.getRoot()->addChild (atCamera);

        // Scene color holds raw radiance on full without temporal anti-aliasing. Raster rgba8 hides
        // non-finite values, but a NaN pixel reads black there: every quad center must stay lit
        const auto full = tier == RenderSettings::TierPreference::full;
        auto& renderer = sharedRenderer();
        if (full)
            setFullTierTaa (renderer, 1);
        else
            setTier (renderer, RenderSettings::TierPreference::raster);

        const auto image = renderToImage (renderer, scene, 256);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto lit = countLitGridQuads (image);
        RecordProperty ("litQuads", lit);
        EXPECT_EQ (lit, 256) << describeDarkGridQuads (image);

        // Raw radiance: the full tier's scene color, and the raster rgba16float bloom path
        if (! full)
        {
            setBloom (renderer, true);
            ASSERT_NE (renderer.render (device, scene, 256, 256), nullptr) << renderer.getLastError();
        }

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
    }

    /** A point light of tiny radius shades like the punctual one on @p tier. */
    void checkTinyRadiusMatchesPunctual (RenderSettings::TierPreference tier)
    {
        // A point light of radius 0.001 one unit over a rough plate shades through the area path: its
        // sphere radiance I / (pi r^2) over the solid angle it subtends converges to the inverse square law
        const auto meanAt = [this, tier] (float radius)
        {
            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
            material->metallicFactor = 0.0f;
            material->roughnessFactor = 0.5f;

            Scene scene;
            addPlate (scene, material);
            scene.setAmbientColor (Colors::black);

            auto light = EntityNode::Ptr (new EntityNode ("point"));
            light->attach<LightNode> (LightNode::Type::point).radius = radius;
            light->setPosition ({ 0.0f, 0.0f, 1.0f });
            scene.getRoot()->addChild (light);

            return meanLuma (renderLuma (scene, 128, tier), 128, { 40, 40, 48, 48 });
        };

        const auto punctual = meanAt (0.0f);
        const auto area = meanAt (0.001f);
        RecordProperty ("punctual", String (punctual, 4).toStdString());
        RecordProperty ("area", String (area, 4).toStdString());

        ASSERT_GT (punctual, 0.05);
        EXPECT_NEAR (area / punctual, 1.0, 0.02);
    }

    /** On @p tier, the clearcoat and the sheen reflect a point light of radius 0.001 through their
        area light path (lightsEvaluateLayersArea: the clearcoat's LTC, the sheen's form factor) as
        they reflect the punctual one, within 2%, the tolerance of checkTinyRadiusMatchesPunctual. The
        base reflects nothing (black, no specular), so the layers alone light the plate.
    */
    void checkLayersUnderATinyAreaLightMatchPunctual (RenderSettings::TierPreference tier)
    {
        const auto meanAt = [this, tier] (Material::Ptr material, float radius)
        {
            Scene scene;
            addPlate (scene, material);
            scene.setAmbientColor (Colors::black);

            auto light = EntityNode::Ptr (new EntityNode ("point"));
            light->attach<LightNode> (LightNode::Type::point).radius = radius;
            light->setPosition ({ 0.6f, 0.0f, 0.8f });
            scene.getRoot()->addChild (light);

            return meanLuma (renderLuma (scene, 128, tier), 128, { 40, 40, 48, 48 });
        };

        const std::pair<const char*, Material::Ptr> layers[] = {
            { "clearcoat", makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, 1.0f, 0.4f, {}, 0.0f, 0.0f) },
            { "sheen", makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 0.5f, 0.0f) },
        };

        for (const auto& [name, material] : layers)
        {
            const auto punctual = meanAt (material, 0.0f);
            const auto area = meanAt (material, 0.001f);
            RecordProperty ((String (name) + "Punctual").toStdString(), String (punctual, 4).toStdString());
            RecordProperty ((String (name) + "Area").toStdString(), String (area, 4).toStdString());

            ASSERT_GT (punctual, 0.002) << name;
            EXPECT_NEAR (area / punctual, 1.0, 0.02) << name;
        }
    }

    /** A light radius widens the highlight without changing its energy on @p tier. */
    void checkRadiusWidensTheHighlight (RenderSettings::TierPreference tier)
    {
        // A smooth dark plate seen at 45 degrees mirrors a point light towards the camera. A radius of
        // 0.3 spreads the highlight over more bright pixels (luma above 1.4, a level of 200 once Reinhard
        // tone mapped) while the energy it reflects stays within 10%
        const auto highlightAt = [this, tier] (float radius)
        {
            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.05f, 0.05f, 0.05f, 1.0f };
            material->metallicFactor = 0.0f;
            material->roughnessFactor = 0.1f;

            Scene scene;
            auto* plate = addPlate (scene, material);
            scene.setAmbientColor (Colors::black);

            const auto tilt = Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, -MathConstants<float>::pi * 0.25f);
            plate->setRotation (tilt);

            // The camera direction from the plate center, mirrored about its normal
            const auto normal = tilt.rotate ({ 0.0f, 0.0f, 1.0f });
            const Vector3<float> toCamera { 0.0f, 0.0f, 1.0f };
            const auto mirrored = normal * (2.0f * normal.dotProduct (toCamera)) - toCamera;

            auto light = EntityNode::Ptr (new EntityNode ("point"));
            auto& node = light->attach<LightNode> (LightNode::Type::point);
            node.intensity = 20.0f;
            node.radius = radius;
            light->setPosition (mirrored * 1.5f);
            scene.getRoot()->addChild (light);

            const auto luma = renderLuma (scene, 256, tier);
            const auto bright = std::count_if (luma.begin(), luma.end(), [] (double value) { return value > 1.4; });
            return std::pair { static_cast<int> (bright), std::accumulate (luma.begin(), luma.end(), 0.0) };
        };

        const auto [sharpPixels, sharpEnergy] = highlightAt (0.0f);
        const auto [widePixels, wideEnergy] = highlightAt (0.3f);
        RecordProperty ("sharpPixels", sharpPixels);
        RecordProperty ("widePixels", widePixels);
        RecordProperty ("sharpEnergy", String (sharpEnergy, 1).toStdString());
        RecordProperty ("wideEnergy", String (wideEnergy, 1).toStdString());

        EXPECT_GT (sharpPixels, 0);
        EXPECT_GT (widePixels, sharpPixels);
        ASSERT_GT (sharpEnergy, 0.0);
        EXPECT_NEAR (wideEnergy / sharpEnergy, 1.0, 0.1);
    }

    /** Area lights stay finite on @p tier, through the rgba16float path on raster, over a material
        with a smooth clearcoat and a smooth white sheen when @p layers is set. */
    void checkAreaLightsStayFinite (RenderSettings::TierPreference tier, bool layers = false)
    {
        // A smooth plate inside a sphere light, touching another, under a spot disk and a grazing sun
        // disk, over a gray ambient light so every plate pixel stays lit
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.0f;

        if (layers)
        {
            material->clearcoatFactor = 1.0f;
            material->sheenColorFactor = { 1.0f, 1.0f, 1.0f };
        }

        Scene scene;
        addPlate (scene, material);
        scene.setAmbientColor (Color (0xff404040));

        const auto addLight = [&] (LightNode::Type type, float radius, Vector3<float> position, const Quaternion& rotation)
        {
            auto light = EntityNode::Ptr (new EntityNode ("light"));
            auto& node = light->attach<LightNode> (type);
            node.intensity = 3.0f;
            node.radius = radius;
            light->setPosition (position);
            light->setRotation (rotation);
            scene.getRoot()->addChild (light);
        };

        addLight (LightNode::Type::point, 0.3f, { 0.0f, 0.0f, 0.05f }, Quaternion::identity());
        addLight (LightNode::Type::point, 0.1f, { 0.5f, -0.5f, 0.0f }, Quaternion::identity());
        addLight (LightNode::Type::spot, 0.2f, { -0.5f, 0.5f, 0.3f }, Quaternion::identity());
        addLight (LightNode::Type::directional, 0.3f, {}, Quaternion::fromAxisAngle ({ 1.0f, 0.0f, 0.0f }, 1.55f));

        // Scene color holds raw radiance on full without temporal anti-aliasing. Raster rgba8 hides
        // non-finite values, but a NaN pixel reads black there
        const auto full = tier == RenderSettings::TierPreference::full;
        auto& renderer = sharedRenderer();
        if (full)
            setFullTierTaa (renderer, 1);
        else
            setTier (renderer, RenderSettings::TierPreference::raster);

        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        auto darkest = 255.0;
        for (int y = 40; y < 88; ++y)
            for (int x = 40; x < 88; ++x)
                darkest = jmin (darkest, meanLuminance (image, { x, y, 1, 1 }));

        RecordProperty ("darkest", String (darkest, 1).toStdString());
        EXPECT_GT (darkest, 10.0);

        // Raw radiance: the full tier's scene color, and the raster rgba16float bloom path. On Metal the
        // raster path stores a NaN as about 1 (min (NaN, 65504) then the reversible tone map), so there
        // the black rgba8 pixel above is what catches it
        if (! full)
        {
            setBloom (renderer, true);
            ASSERT_NE (renderer.render (device, scene, 128, 128), nullptr) << renderer.getLastError();
        }

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
    }

    /** The ground in the shadow of the near cube of buildLongGroundScene() stays finite on @p tier,
        in raw radiance: no temporal anti-aliasing on full, the rgba16float bloom path on raster. */
    void checkShadowedRegionIsFinite (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        const auto render = [&] (bool castsShadows, GpuTexture::Ptr& sceneColor)
        {
            Scene scene;
            buildLongGroundScene (scene, 2.0f).castsShadows = castsShadows;

            SceneRenderer renderer;
            if (full)
                setFullTierTaa (renderer, 1);
            else
            {
                setTier (renderer, RenderSettings::TierPreference::raster);
                setBloom (renderer, true);
            }

            const auto image = renderToImage (renderer, scene, 256);
            sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            return image;
        };

        GpuTexture::Ptr sceneColor, unshadowedColor;
        const auto shadowed = render (true, sceneColor);
        const auto unshadowed = render (false, unshadowedColor);
        ASSERT_TRUE (shadowed.isValid() && unshadowed.isValid());
        ASSERT_NE (sceneColor, nullptr);

        // The checked frame does hold the shadow
        EXPECT_GT (shadowContrast (unshadowed, shadowed), 0.35);

        EXPECT_EQ (sceneColor->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
    }

    /** With ambient occlusion on, the raster depth and normal prepass of the plates scene holds the
        G-buffer RT1 encoding: the plates' +Z decodes through lib/octahedral.glsl within 0.02, and only
        drawn texels have a roughness. The full tier reads its G-buffer instead and draws no prepass.
    */
    void checkPrepassMatchesTheGBufferEncoding (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildShadowsScene (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);

        auto settings = renderer.getRenderSettings();
        settings.ambientOcclusion = true;
        renderer.setRenderSettings (settings);

        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        ASSERT_TRUE (renderer.waitForGPU().wasOk());

        const auto prepass = renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal);
        if (tier == RenderSettings::TierPreference::full)
        {
            EXPECT_EQ (prepass, nullptr);
            return;
        }

        ASSERT_NE (prepass, nullptr);
        EXPECT_EQ (prepass->getFormat(), GpuTextureFormat::rgba16float);
        EXPECT_EQ (prepass->getSampleCount(), 1u);

        // Red: the distance of the decoded normal from +Z, times 10, so 0.02 reads 51 levels. Green: the roughness
        const auto decoded = mapTexels (prepass, "vec4(length(octahedralDecode(t.xy) - vec3(0.0, 0.0, 1.0)) * 10.0, t.z, 0.0, 1.0)", loadOctahedralLibrary());
        ASSERT_TRUE (decoded.isValid());

        // A cleared texel decodes to +Z as well, so the roughness proves the center was drawn
        EXPECT_LT (decoded.getPixelColor (32, 32).getRed(), 51);
        EXPECT_GT (decoded.getPixelColor (32, 32).getGreen(), 64);
        EXPECT_EQ (decoded.getPixelColor (1, 1).getGreen(), 0);
    }
#endif

    /** A 5 cm high step, 1 x 1 m, on a 50 x 50 m ground, its +x face standing on x = 0, lit by a
        shadow casting sun 15 degrees above the horizon from -x: the step shadows the ground over the
        0.19 m beyond that face, the crease. With setCreaseShadowMap(), the sun gets a single 256 x 256 map, fitted to the
        ground the camera sees (texels of about 0.12 m). Its lookups move each receiver along its
        normal by 1.5 texels times (1 - n.l), about 0.13 m under this grazing sun, which lifts the
        whole crease above the step: the map cannot show that shadow, only contact shadows can.
        The camera looks at the crease from +x, above it. The sun asks for contact shadows when
        @p contact is set. Returns the sun.
    */
    static LightNode& buildCreaseScene (Scene& scene, bool contact)
    {
        addGround (scene, 25.0f);

        auto step = EntityNode::Ptr (new EntityNode ("step"));
        step->attach<MeshNode> (makeBox (0.5f, makeWallMaterial()));
        step->setScale ({ 1.0f, 0.05f, 1.0f });
        step->setPosition ({ -0.5f, 0.025f, 0.0f });
        scene.getRoot()->addChild (step);

        auto entity = EntityNode::Ptr (new EntityNode ("sun"));
        auto& sun = entity->attach<LightNode>();
        sun.castsShadows = true;
        sun.contactShadows = contact;
        const auto elevation = degreesToRadians (15.0f);
        aimCamera (*entity, Vector3<float> (-std::cos (elevation), std::sin (elevation), 0.0f) * 10.0f, {});
        scene.getRoot()->addChild (entity);

        addCamera (scene, { 0.55f, 0.35f, 0.0f }, { 0.05f, 0.0f, 0.0f });
        scene.setAmbientColor (Color (0xff3f3f3f)); // 0.05 once linear
        return sun;
    }

    /** The sun shadow settings of buildCreaseScene(): a single 256 x 256 map. */
    static void setCreaseShadowMap (SceneRenderer& renderer)
    {
        auto settings = renderer.getRenderSettings();
        settings.sunCascades = 1;
        settings.shadowResolution = 256;
        renderer.setRenderSettings (settings);
    }

    /** The ground of the crease of buildCreaseScene() in a render of @p size x @p size. */
    static Rectangle<int> creaseRect (int size = 256)
    {
        return { 64 * size / 256, 116 * size / 256, 128 * size / 256, 16 * size / 256 };
    }

    /** Open ground of buildCreaseScene(), lit and far from the step, in a render of @p size x @p size. */
    static Rectangle<int> openGroundRect (int size = 256)
    {
        return { 64 * size / 256, 200 * size / 256, 128 * size / 256, 32 * size / 256 };
    }

    /** buildCreaseScene() on @p tier at @p size x @p size, converged, with or without contact
        shadows, the sun casting shadows or not. @p texel receives the world size of a texel of the
        sun map, 0 without one. */
    Image renderCreaseScene (RenderSettings::TierPreference tier, int size, bool contact, bool castsShadows, float& texel)
    {
        Scene scene;
        buildCreaseScene (scene, contact).castsShadows = castsShadows;

        SceneRenderer renderer;
        setCreaseShadowMap (renderer);
        setTier (renderer, tier);
        const auto image = renderConverged (renderer, scene, size);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

        texel = shadows (renderer).getShadowData().cascadeTexel[0];
        return image;
    }

    /** On @p tier, which has no contact shadows, buildCreaseScene() renders exactly the same with
        and without them. */
    void checkContactShadowsIgnoredOnRaster (RenderSettings::TierPreference tier)
    {
        Scene withContact, without;
        buildCreaseScene (withContact, true);
        buildCreaseScene (without, false);

        const auto setUp = [&] (SceneRenderer& renderer)
        {
            setCreaseShadowMap (renderer);
            setTier (renderer, tier);
        };

        // Known, not caused by contact shadows: the first frame a fresh Metal device renders differs
        // from the later ones by one level in a few hundred pixels, with or without shadows (a Phase 3
        // follow-up, see the Task 8 report). Both compared frames come after a warm-up frame
        SceneRenderer warmUp;
        setUp (warmUp);
        ASSERT_TRUE (renderToImage (warmUp, without, 128).isValid());

        SceneRenderer a, b;
        setUp (a);
        setUp (b);
        const auto imageA = renderToImage (a, withContact, 128);
        const auto imageB = renderToImage (b, without, 128);
        ASSERT_TRUE (a.getLastError().isEmpty());
        ASSERT_TRUE (imageA.isValid() && imageB.isValid());
        EXPECT_NEAR (meanLuminance (imageA, creaseRect (128)), meanLuminance (imageB, creaseRect (128)), 1.0f / 255.0f);

        auto differing = 0;
        for (int y = 0; y < imageA.getHeight(); ++y)
            for (int x = 0; x < imageA.getWidth(); ++x)
                differing += imageA.getPixelColor (x, y) != imageB.getPixelColor (x, y) ? 1 : 0;

        EXPECT_EQ (differing, 0);
    }

    /** The contact shadow settings of each light reach the shaders on @p tier through its first
        shadow record: contact[0] the march distance (0 when off), contact[1] the thickness, with
        filterInfo.w flag 1 telling the tier marches them. A light with contact shadows but no
        shadow map takes one record without a map. The raster tier has no contact shadows: no flag,
        no distance and no extra record.
    */
    void checkContactShadowSettingsLiveInTheFirstShadowRecord (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        for (const auto enabled : { true, false })
        {
            Scene scene;
            buildCreaseScene (scene, true);

            const auto addPoint = [&] (Vector3<float> position, bool castsShadows, float distance, float thickness)
            {
                auto entity = EntityNode::Ptr (new EntityNode ("point"));
                auto& light = entity->attach<LightNode> (LightNode::Type::point);
                light.castsShadows = castsShadows;
                light.contactShadows = true;
                light.contactShadowDistance = distance;
                light.contactShadowThickness = thickness;
                entity->setPosition (position);
                scene.getRoot()->addChild (entity);
            };

            addPoint ({ 0.5f, 1.0f, 0.5f }, true, 0.4f, 0.02f);
            addPoint ({ -0.5f, 1.0f, -0.5f }, false, 0.3f, 0.1f);

            SceneRenderer renderer;
            setCreaseShadowMap (renderer);
            setTier (renderer, tier);

            auto settings = renderer.getRenderSettings();
            settings.contactShadows = enabled;
            renderer.setRenderSettings (settings);

            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());

            const auto marches = full && enabled;
            const auto data = shadows (renderer).getShadowData();
            const auto cascades = static_cast<int> (data.sunInfo[0]);
            ASSERT_GE (cascades, 1);

            const auto flags = static_cast<int> (data.filterInfo[3]);
            EXPECT_EQ ((flags & 1) != 0, marches) << flags;

            // The sun, then the six faces of the shadowed point light, which keep their view count
            EXPECT_FLOAT_EQ (data.records[0].contact[0], marches ? 0.2f : 0.0f);
            EXPECT_FLOAT_EQ (data.records[cascades].contact[0], marches ? 0.4f : 0.0f);
            if (marches)
                EXPECT_FLOAT_EQ (data.records[cascades].contact[1], 0.02f);

            for (int face = 0; face < 6; ++face)
                EXPECT_FLOAT_EQ (data.records[cascades + face].contact[2], 6.0f) << face;

            // Contact shadows without a shadow map: one record without a map
            ASSERT_EQ (shadows (renderer).getNumRecords(), cascades + 6 + (marches ? 1 : 0));
            if (marches)
            {
                const auto& contactOnly = data.records[cascades + 6];
                EXPECT_EQ (contactOnly.atlasRect[2], 0.0f);
                EXPECT_FLOAT_EQ (contactOnly.contact[0], 0.3f);
                EXPECT_FLOAT_EQ (contactOnly.contact[1], 0.1f);
            }
        }
    }

    /** One shadow filter variant holds soft shadows and, on full, contact shadows: on @p tier a
        frame using neither shades with the default variants even with soft shadows on, a frame
        using either or both with the filter variant. Raster has no contact shadows: contact alone
        keeps its default.
    */
    void checkShadowFilterVariantFollowsSoftAndContactShadows (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        const struct
        {
            const char* name;
            float sunRadius;
            bool contact;
            bool filters;
        } cases[] = {
            { "neither", 0.0f, false, false },
            { "soft only", 0.05f, false, true },
            { "contact only", 0.0f, true, full },
            { "both", 0.05f, true, true },
        };

        for (const auto& c : cases)
        {
            Scene scene;
            buildCreaseScene (scene, c.contact).radius = c.sunRadius;

            SceneRenderer renderer;
            setCreaseShadowMap (renderer);
            setTier (renderer, tier);

            auto settings = renderer.getRenderSettings();
            settings.softShadows = true;
            renderer.setRenderSettings (settings);

            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << c.name << ": " << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk()) << c.name;
            EXPECT_EQ (shadows (renderer).needsShadowFilters(), c.filters) << c.name;
        }
    }

    /** The depth and normal prepass exists only on raster with ambient occlusion on: none at the
        default settings, one once ambient occlusion is turned on, released when it is turned off
        again. The full tier never draws one, and a raster device that cannot draw it renders without.
    */
    void checkPrepassIsNotCreatedByDefault (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildShadowsScene (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto drawsPrepass = [&] (std::optional<bool> ambientOcclusion)
        {
            auto settings = renderer.getRenderSettings();
            settings.ambientOcclusion = ambientOcclusion;
            renderer.setRenderSettings (settings);

            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            return renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal) != nullptr;
        };

        EXPECT_FALSE (drawsPrepass (std::nullopt));
        EXPECT_EQ (drawsPrepass (true), tier == RenderSettings::TierPreference::raster);
        EXPECT_FALSE (drawsPrepass (false));

        renderer.setDeviceFactsOverride ([] (RenderCapabilities::DeviceFacts& facts)
        {
            facts.depthNormalPrepass = false;
        });
        EXPECT_FALSE (drawsPrepass (true));
    }

    /** A prepass that fails to compile leaves the frames without it, with no error. On raster,
        where ambient occlusion reads the prepass, they render like frames without ambient
        occlusion; the full tier never draws the prepass, so its frames are left as they are.
    */
    void checkRendersWithoutTheDepthNormalPrepass (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool ambientOcclusion, bool failPrepass)
        {
            Scene scene;
            buildShadowsScene (scene);

            SceneRenderer renderer;
            setTier (renderer, tier);

            auto settings = renderer.getRenderSettings();
            settings.ambientOcclusion = ambientOcclusion;
            renderer.setRenderSettings (settings);

            if (failPrepass)
                renderer.failDepthNormalPrepassForTesting();

            Image image;
            for (int frame = 0; frame < 2; ++frame)
            {
                image = renderToImage (renderer, scene, 64);
                EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << frame;
            }

            return image;
        };

        const auto withoutPrepass = render (true, true);
        const auto reference = render (tier == RenderSettings::TierPreference::full, false);
        ASSERT_TRUE (withoutPrepass.isValid() && reference.isValid());
        EXPECT_EQ (countDifferences (withoutPrepass, reference), 0);
    }

    /** A 0.6 m box, the entity "box", resting on a 6 x 6 m ground, seen from 0.9 m in front of the
        foot of its +z face and 30 degrees above it: the crease where that face meets the ground
        runs across the middle of the frame, and the ground at the bottom of the frame lies 0.4 m
        and more in front of the face, beyond the default aoDistance of 0.2 m. Without a light, it
        is lit by the flat ambient light alone (0.5 once linear), the indirect light ambient
        occlusion darkens.
    */
    static void buildContactCreaseScene (Scene& scene)
    {
        addGround (scene, 3.0f);

        auto box = EntityNode::Ptr (new EntityNode ("box"));
        box->attach<MeshNode> (makeBox (0.3f, makeWallMaterial()));
        box->setPosition ({ 0.0f, 0.3f, 0.0f });
        scene.getRoot()->addChild (box);

        const Vector3<float> foot (0.0f, 0.0f, 0.3f);
        const auto elevation = degreesToRadians (30.0f);
        addCamera (scene, foot + Vector3<float> (0.0f, std::sin (elevation), std::cos (elevation)) * 0.9f, foot);

        scene.setUsingDefaultLight (false);
        scene.setAmbientColor (Color (0xffbcbcbc));
    }

    /** The ground just in front of the box face of buildContactCreaseScene(), in a texture or image
        of @p width x @p height. */
    static Rectangle<int> contactCreaseRect (int width, int height)
    {
        return { width * 35 / 100, height * 50 / 100, width * 30 / 100, jmax (1, height * 4 / 100) };
    }

    /** Open ground of buildContactCreaseScene(), at least 0.4 m in front of the box, in a texture or
        image of @p width x @p height. */
    static Rectangle<int> openFloorRect (int width, int height)
    {
        return { width * 35 / 100, height * 90 / 100, width * 30 / 100, height * 7 / 100 };
    }

    /** Sets RenderSettings::ambientOcclusion of a scene. */
    static void setAmbientOcclusion (SceneRenderer& renderer, std::optional<bool> ambientOcclusion)
    {
        auto settings = renderer.getRenderSettings();
        settings.ambientOcclusion = ambientOcclusion;
        renderer.setRenderSettings (settings);
    }

    /** The pixels of @p image more than one level away from @p reference in any channel. */
    static int countDifferencesAboveOneLevel (const Image& image, const Image& reference)
    {
        int differing = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto a = image.getPixelColor (x, y);
                const auto b = reference.getPixelColor (x, y);
                if (std::abs (a.getRed() - b.getRed()) > 1 || std::abs (a.getGreen() - b.getGreen()) > 1 || std::abs (a.getBlue() - b.getBlue()) > 1)
                    ++differing;
            }
        }

        return differing;
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** On @p tier, ambient occlusion darkens the crease where a box meets the ground and leaves the
        open ground nearly unoccluded. The pass output is rgba8, full size on full and half size on
        raster, and holds the view-space bent normal, which on open ground is the ground's normal.
        The shaded crease is darker than without ambient occlusion, the open ground is not.
    */
    void checkAmbientOcclusionDarkensTheContactCrease (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        Scene scene;
        buildContactCreaseScene (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        setAmbientOcclusion (renderer, true);
        const auto image = renderToImage (renderer, scene, 128);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto ao = renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion);
        ASSERT_NE (ao, nullptr);
        EXPECT_EQ (ao->getFormat(), GpuTextureFormat::rgba8unorm);
        EXPECT_EQ (ao->getWidth(), 64u);
        EXPECT_EQ (ao->getHeight(), 64u);

        const auto size = static_cast<int> (ao->getWidth());
        const auto visibility = mapTexels (ao, "vec4(t.aaa, 1.0)");
        ASSERT_TRUE (visibility.isValid());

        const auto crease = meanLuminance (visibility, contactCreaseRect (size, size));
        const auto open = meanLuminance (visibility, openFloorRect (size, size));
        RecordProperty ("creaseVisibility", String (crease, 1).toStdString());
        RecordProperty ("openVisibility", String (open, 1).toStdString());
        EXPECT_LT (crease, open - 30.0);
        EXPECT_GT (open, 220.0); // An open floor is barely occluded

        // Seen 30 degrees from above, the ground's normal in view space is (0, cos 30, sin 30)
        const auto bentNormal = mapTexels (ao, "vec4(t.rgb, 1.0)");
        ASSERT_TRUE (bentNormal.isValid());

        const auto openArea = openFloorRect (size, size);
        const auto bent = bentNormal.getPixelColor (openArea.getCenterX(), openArea.getCenterY());
        EXPECT_NEAR (bent.getRed(), 128, 16);
        EXPECT_NEAR (bent.getGreen(), 238, 16);
        EXPECT_NEAR (bent.getBlue(), 191, 16);

        Scene without;
        buildContactCreaseScene (without);

        SceneRenderer withoutRenderer;
        setTier (withoutRenderer, tier);
        setAmbientOcclusion (withoutRenderer, false);
        const auto reference = renderToImage (withoutRenderer, without, 128);
        ASSERT_TRUE (reference.isValid()) << withoutRenderer.getLastError();

        const auto shadedCrease = meanLuminance (image, contactCreaseRect (128, 128));
        const auto referenceCrease = meanLuminance (reference, contactCreaseRect (128, 128));
        RecordProperty ("shadedCrease", String (shadedCrease, 1).toStdString());
        RecordProperty ("shadedCreaseWithout", String (referenceCrease, 1).toStdString());
        EXPECT_LT (shadedCrease, referenceCrease - 10.0);
        EXPECT_NEAR (meanLuminance (image, openFloorRect (128, 128)), meanLuminance (reference, openFloorRect (128, 128)), 4.0);
    }
#endif

    /** Ambient occlusion off allocates nothing on @p tier: no prepass on raster and no ambient
        occlusion output on either tier. Raster defaults to off, full to on. A pass created by an
        earlier frame keeps no render target on the frames without it, and an aoDistance of 0 turns
        ambient occlusion off.
    */
    void checkAmbientOcclusionOffAllocatesNothing (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;

        Scene scene;
        buildContactCreaseScene (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto render = [&] (std::optional<bool> ambientOcclusion)
        {
            setAmbientOcclusion (renderer, ambientOcclusion);
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            return renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr;
        };

        EXPECT_EQ (render (std::nullopt), full);
        EXPECT_FALSE (render (false));
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
        EXPECT_TRUE (render (true));

        const auto* pass = renderer.getAmbientOcclusionPass();
        ASSERT_NE (pass, nullptr);
        EXPECT_GT (pass->getNumCachedTargets(), 0u);

        EXPECT_FALSE (render (false));
        EXPECT_EQ (renderer.getAmbientOcclusionPass(), pass);
        EXPECT_EQ (pass->getNumCachedTargets(), 0u);
        EXPECT_EQ (pass->getOutput(), nullptr);

        auto settings = renderer.getRenderSettings();
        settings.aoDistance = 0.0f;
        renderer.setRenderSettings (settings);
        EXPECT_FALSE (render (true));
        EXPECT_EQ (pass->getNumCachedTargets(), 0u);
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
    }

    /** On a raster device that cannot draw the depth and normal prepass, ambient occlusion is
        unavailable: turning it on renders the same image as leaving it off, without an error and
        without an output. The full tier reads the G-buffer instead, so the fact leaves it on.
    */
    void checkAmbientOcclusionUnavailableOnRasterRendersFallback (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool ambientOcclusion, bool& hasOutput)
        {
            Scene scene;
            buildContactCreaseScene (scene);

            SceneRenderer renderer;
            setTier (renderer, tier);
            setAmbientOcclusion (renderer, ambientOcclusion);
            renderer.setDeviceFactsOverride ([] (RenderCapabilities::DeviceFacts& facts)
            {
                facts.depthNormalPrepass = false;
            });

            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            hasOutput = renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr;
            return image;
        };

        auto hasOutput = false, hasOutputWithout = false;
        const auto withAmbientOcclusion = render (true, hasOutput);
        const auto without = render (false, hasOutputWithout);
        ASSERT_TRUE (withAmbientOcclusion.isValid() && without.isValid());
        EXPECT_FALSE (hasOutputWithout);

        if (tier == RenderSettings::TierPreference::full)
        {
            EXPECT_TRUE (hasOutput);
            return;
        }

        EXPECT_FALSE (hasOutput);
        EXPECT_EQ (countDifferencesAboveOneLevel (withAmbientOcclusion, without), 0);
    }

    /** On @p tier, ambient occlusion never darkens direct light: lit by a sun alone, without
        ambient light, the crease scene renders the same with and without it.
    */
    void checkAmbientOcclusionLeavesTheDirectLightAlone (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool ambientOcclusion)
        {
            Scene scene;
            buildContactCreaseScene (scene);
            scene.setAmbientColor (Colors::black);

            auto sun = EntityNode::Ptr (new EntityNode ("sun"));
            sun->attach<LightNode>();
            aimCamera (*sun, { -3.0f, 8.0f, 4.0f }, {});
            scene.getRoot()->addChild (sun);

            SceneRenderer renderer;
            setTier (renderer, tier);
            setAmbientOcclusion (renderer, ambientOcclusion);
            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr, ambientOcclusion);
            return image;
        };

        // A fresh Metal device's first frame can differ by a level in a few pixels, see checkContactShadowsIgnoredOnRaster()
        ASSERT_TRUE (render (false).isValid());

        const auto withAmbientOcclusion = render (true);
        const auto without = render (false);
        ASSERT_TRUE (withAmbientOcclusion.isValid() && without.isValid());
        EXPECT_EQ (countDifferencesAboveOneLevel (withAmbientOcclusion, without), 0);
    }

    /** Ambient occlusion keeps the frames deterministic on @p tier (preflight R13): its noise
        rotates only under temporal anti-aliasing, whose running mean freezes once converged, so
        two renderers converging on the same scene end on the same image.
    */
    void checkConvergedAmbientOcclusionIsDeterministic (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildContactCreaseScene (scene);

        SceneRenderer warmUp;
        setTier (warmUp, tier);
        setAmbientOcclusion (warmUp, true);
        ASSERT_TRUE (renderToImage (warmUp, scene, 64).isValid());

        SceneRenderer a, b;
        a.setRenderSettings (warmUp.getRenderSettings());
        b.setRenderSettings (warmUp.getRenderSettings());
        const auto imageA = renderConverged (a, scene, 64);
        const auto imageB = renderConverged (b, scene, 64);
        ASSERT_TRUE (imageA.isValid() && imageB.isValid());
        EXPECT_TRUE (a.isConverged() && b.isConverged());
        EXPECT_NE (a.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion), nullptr);

        auto differing = 0;
        for (int y = 0; y < imageA.getHeight(); ++y)
            for (int x = 0; x < imageA.getWidth(); ++x)
                differing += imageA.getPixelColor (x, y) != imageB.getPixelColor (x, y) ? 1 : 0;

        EXPECT_EQ (differing, 0);
    }

    /** On @p tier, blended surfaces skip ambient occlusion: the prepass and the G-buffer hold the
        opaque surfaces only, so their occlusion belongs to what lies behind. A blended quad of alpha
        1 hiding the crease of buildContactCreaseScene() renders the same with and without it.
    */
    void checkBlendedSurfacesSkipAmbientOcclusion (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool ambientOcclusion)
        {
            Scene scene;
            buildContactCreaseScene (scene);

            // Facing the camera halfway to the crease, 0.2 m wide: it covers the crease rectangle
            const Vector3<float> foot (0.0f, 0.0f, 0.3f);
            const auto elevation = degreesToRadians (30.0f);
            const Vector3<float> toCamera (0.0f, std::sin (elevation), std::cos (elevation));
            const Vector3<float> up (0.0f, std::cos (elevation), -std::sin (elevation));
            const Vector3<float> right (1.0f, 0.0f, 0.0f);
            const auto center = foot + toCamera * 0.45f;

            std::vector<Mesh::Vertex> vertices (4);
            const std::array<std::pair<float, float>, 4> corners { { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f } } };
            for (size_t i = 0; i < 4; ++i)
            {
                vertices[i].position = center + right * (0.1f * corners[i].first) + up * (0.1f * corners[i].second);
                vertices[i].normal = toCamera;
            }

            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
            material->metallicFactor = 0.0f;
            material->roughnessFactor = 0.5f;
            material->alphaMode = Material::AlphaMode::blend;

            auto mesh = Mesh::Ptr (new Mesh ("pane"));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

            auto pane = EntityNode::Ptr (new EntityNode ("pane"));
            pane->attach<MeshNode> (mesh);
            scene.getRoot()->addChild (pane);

            SceneRenderer renderer;
            setTier (renderer, tier);
            setAmbientOcclusion (renderer, ambientOcclusion);
            const auto image = renderConverged (renderer, scene, 128);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr, ambientOcclusion);
            return image;
        };

        const auto withAmbientOcclusion = render (true);
        const auto without = render (false);
        ASSERT_TRUE (withAmbientOcclusion.isValid() && without.isValid());

        const auto crease = contactCreaseRect (128, 128);
        RecordProperty ("paneWith", String (meanLuminance (withAmbientOcclusion, crease), 1).toStdString());
        RecordProperty ("paneWithout", String (meanLuminance (without, crease), 1).toStdString());
        EXPECT_NEAR (meanLuminance (withAmbientOcclusion, crease), meanLuminance (without, crease), 1.0);
    }

    /** On @p tier, the bent normals turn the diffuse light of the environment towards the open side:
        under a sky bright above and dark below, the foot of the box's face, whose bent normals lean
        up away from the ground, gets more of the sky than under the same sky turned upside down. The
        face is vertical and has no specular, so without bent normals both skies light it the same.
    */
    void checkBentNormalsTurnTheDiffuseLightToTheOpenSide (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool skyAbove)
        {
            Scene scene;
            buildContactCreaseScene (scene);
            scene.setAmbientColor (Colors::black);

            // Diffuse only: no dielectric specular, so only the irradiance tells the skies apart
            auto material = Material::Ptr (new Material());
            material->baseColorFactor = { 0.8f, 0.8f, 0.8f, 1.0f };
            material->metallicFactor = 0.0f;
            material->roughnessFactor = 1.0f;
            material->specularFactor = 0.0f;
            auto* box = scene.getRoot()->findChild ("box");
            box->getNode<MeshNode>()->mesh = makeBox (0.3f, material);

            scene.setEnvironment (new EnvironmentMap ([skyAbove] (const Vector3<float>& direction)
            {
                const auto up = jlimit (0.0f, 1.0f, (skyAbove ? direction.getY() : -direction.getY()) * 0.5f + 0.5f);
                return Vector3<float> (1.0f, 1.0f, 1.0f) * (0.05f + 2.0f * up * up);
            }, 32, 3));

            SceneRenderer renderer;
            setTier (renderer, tier);
            setAmbientOcclusion (renderer, true);
            const auto image = renderConverged (renderer, scene, 128);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        const auto skyAbove = render (true);
        const auto skyBelow = render (false);
        ASSERT_TRUE (skyAbove.isValid() && skyBelow.isValid());

        // The foot of the face, just above the crease, and the top of the face, open to the sky
        const Rectangle<int> foot (45, 58, 38, 4);
        const Rectangle<int> top (45, 30, 38, 4);
        const auto footAbove = meanLuminance (skyAbove, foot), footBelow = meanLuminance (skyBelow, foot);
        RecordProperty ("footSkyAbove", String (footAbove, 1).toStdString());
        RecordProperty ("footSkyBelow", String (footBelow, 1).toStdString());
        RecordProperty ("topSkyAbove", String (meanLuminance (skyAbove, top), 1).toStdString());
        RecordProperty ("topSkyBelow", String (meanLuminance (skyBelow, top), 1).toStdString());
        EXPECT_GT (footAbove, footBelow + 3.0);
    }

    /** Raster: ambient occlusion turned on, then off again, leaves the frame exactly as it was before,
        the variant without ambient occlusion shading it again.
    */
    void checkRasterAmbientOcclusionOffAgainRestoresTheFrame()
    {
        Scene scene;
        buildContactCreaseScene (scene);

        // A fresh Metal device's frames can differ by a level in a few hundred pixels (see
        // checkContactShadowsIgnoredOnRaster()): the frames compare within one level, while the
        // ambient occlusion darkens the crease by about ten
        SceneRenderer renderer;
        setTier (renderer, RenderSettings::TierPreference::raster);
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid());
        const auto before = renderToImage (renderer, scene, 64);

        setAmbientOcclusion (renderer, true);
        const auto during = renderToImage (renderer, scene, 64);

        setAmbientOcclusion (renderer, false);
        const auto after = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (before.isValid() && during.isValid() && after.isValid()) << renderer.getLastError();

        EXPECT_GT (countDifferencesAboveOneLevel (during, before), 0);
        EXPECT_EQ (countDifferencesAboveOneLevel (after, before), 0);
    }

    /** A mirror floor (white metal, roughness 0.02), 12 x 12 at y = 0, the entity "floor", under a
        large green emitter 2 m up facing down, the entity "emitter". The camera looks down at the
        floor from (0, 1.5, 2.5) at 31 degrees, so the top edge of its frustum points 8 degrees below
        the horizon and only the floor's reflection can show the emitter. No environment and no
        default light: without a reflection probe the floor reflects black, and shows the flat
        ambient light only. @p withEmitter false leaves the emitter out.
    */
    /** A horizontal quad at height @p y over @p area (x, z), facing up or down, the entity @p name. */
    static EntityNode::Ptr makeHorizontalQuad (const char* name, float y, Rectangle<float> area, bool facingUp, Material::Ptr material)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { area.getX(), y, area.getBottom() };
        vertices[1].position = { area.getRight(), y, area.getBottom() };
        vertices[2].position = { area.getRight(), y, area.getY() };
        vertices[3].position = { area.getX(), y, area.getY() };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, facingUp ? 1.0f : -1.0f, 0.0f };

        // Counterclockwise seen from the side the normal points to
        auto mesh = Mesh::Ptr (new Mesh (name));
        mesh->addPrimitive (std::move (vertices), facingUp ? std::vector<uint32> { 0, 1, 2, 0, 2, 3 } : std::vector<uint32> { 0, 2, 1, 0, 3, 2 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode (name));
        entity->attach<MeshNode> (mesh);
        return entity;
    }

    /** A black material emitting @p emission. */
    static Material::Ptr makeEmitterMaterial (std::array<float, 3> emission)
    {
        auto emitter = Material::Ptr (new Material());
        emitter->baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        emitter->emissiveFactor = emission;
        return emitter;
    }

    static void buildMirrorFloorScene (Scene& scene, bool withEmitter = true)
    {
        auto mirror = Material::Ptr (new Material());
        mirror->baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        mirror->metallicFactor = 1.0f;
        mirror->roughnessFactor = 0.02f;
        scene.getRoot()->addChild (makeHorizontalQuad ("floor", 0.0f, { -6.0f, -6.0f, 12.0f, 12.0f }, true, mirror));

        if (withEmitter)
            scene.getRoot()->addChild (makeHorizontalQuad ("emitter", 2.0f, { -30.0f, -40.0f, 60.0f, 41.5f }, false, makeEmitterMaterial ({ 0.0f, 1.0f, 0.0f })));

        addCamera (scene, { 0.0f, 1.5f, 2.5f }, {});
        scene.setUsingDefaultLight (false);
    }

    /** The middle of the floor of buildMirrorFloorScene() in a frame of @p size x @p size. */
    static Rectangle<int> floorRegion (int size = 64)
    {
        return { size / 4, size * 3 / 8, size / 2, size / 2 };
    }

    /** The mean color of the 8 bit pixels within @p area. */
    static Color meanColor (const Image& image, Rectangle<int> area)
    {
        double sum[3] {};
        for (int y = area.getY(); y < area.getBottom(); ++y)
        {
            for (int x = area.getX(); x < area.getRight(); ++x)
            {
                const auto color = image.getPixelColor (x, y);
                sum[0] += color.getRed();
                sum[1] += color.getGreen();
                sum[2] += color.getBlue();
            }
        }

        const auto count = static_cast<double> (area.getWidth() * area.getHeight());
        return Color (static_cast<uint8> (std::lround (sum[0] / count)),
                      static_cast<uint8> (std::lround (sum[1] / count)),
                      static_cast<uint8> (std::lround (sum[2] / count)));
    }

    /** Sets the emission of the emitter of buildMirrorFloorScene(). */
    static void setEmitterColor (Scene& scene, std::array<float, 3> color)
    {
        auto* emitter = scene.getRoot()->findChild ("emitter");
        ASSERT_NE (emitter, nullptr);
        emitter->getNode<MeshNode>()->mesh->getPrimitive (0).material->emissiveFactor = color;
    }

    /** Adds a spherical reflection probe at @p position, the entity "probe", and returns its part. */
    static ReflectionProbeNode& addReflectionProbe (Scene& scene, Vector3<float> position, float influenceRadius, const char* name = "probe")
    {
        auto probe = EntityNode::Ptr (new EntityNode (name));
        auto& node = probe->attach<ReflectionProbeNode>();
        node.influenceRadius = influenceRadius;
        probe->setPosition (position);
        scene.getRoot()->addChild (probe);
        return node;
    }

    /** The number of probes the renderer baked, or -1 before it had a baker. */
    static int getNumBakes (const SceneRenderer& renderer)
    {
        const auto* baker = renderer.getProbeBaker();
        return baker != nullptr ? baker->getNumBakes() : -1;
    }

    /** On @p tier, a reflection probe shows the floor what the camera cannot see: the emitter of
        buildMirrorFloorScene() above the frustum, which the floor reflects only through the probe.
    */
    void checkReflectionProbeShowsWhatTheCameraCannotSee (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto before = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (before.isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getProbeBaker(), nullptr); // Nothing allocated without a probe

        const auto floorBefore = meanColor (before, floorRegion());
        EXPECT_NEAR (floorBefore.getGreen(), floorBefore.getRed(), 2); // The camera does not see the emitter

        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        const auto after = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        ASSERT_TRUE (after.isValid());
        EXPECT_EQ (getNumBakes (renderer), 1);

        const auto floorAfter = meanColor (after, floorRegion());
        RecordProperty ("floorGreenBefore", floorBefore.getGreen());
        RecordProperty ("floorGreenAfter", floorAfter.getGreen());
        EXPECT_GT (floorAfter.getGreen(), floorBefore.getGreen() + 40);
        EXPECT_NEAR (floorAfter.getRed(), floorBefore.getRed(), 2);
    }

    /** On @p tier, a probe part replacing another on the same entity is a new probe: it bakes, and
        does not reuse what the old one captured.
    */
    void checkReplacedProbeIsBakedAgain (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        setTier (renderer, tier);
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();

        // Turn the green emitter red, then replace the probe: the new part must bake, not reuse the old tile
        setEmitterColor (scene, { 1.0f, 0.0f, 0.0f });
        auto* entity = scene.getRoot()->findChild ("probe");
        ASSERT_TRUE (entity->detach<ReflectionProbeNode>());
        entity->attach<ReflectionProbeNode>().influenceRadius = 20.0f;

        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 2);
        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_EQ (renderer.getProbeBaker()->getNumBakedProbes(), 1);

        const auto floor = meanColor (image, floorRegion());
        EXPECT_GT (floor.getRed(), floor.getGreen() + 30);
    }

    /** On @p tier, a probe bakes again when it moves or one of its own fields changes, never because
        the rest of the scene changed, until requestBake() asks for it.
    */
    void checkMovingAProbeBakesItAgain (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        auto& probe = addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        setTier (renderer, tier);
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        // Unchanged, it keeps its bake
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        scene.getRoot()->findChild ("probe")->setPosition ({ 0.5f, 1.0f, -1.0f });
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 2);

        probe.clipNear = 0.2f;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 3);

        // The emitter turns red: the probe keeps showing what it captured
        setEmitterColor (scene, { 1.0f, 0.0f, 0.0f });
        const auto stale = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (stale.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 3);
        EXPECT_GT (meanColor (stale, floorRegion()).getGreen(), meanColor (stale, floorRegion()).getRed() + 30);

        probe.requestBake();
        const auto rebaked = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (rebaked.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 4);
        EXPECT_GT (meanColor (rebaked, floorRegion()).getRed(), meanColor (rebaked, floorRegion()).getGreen() + 30);
    }

    /** On @p tier, a probe whose falloff crosses the floor, in a scene without an environment and
        with a black emitter, blends to finite values everywhere: the weights are never normalized by
        their sum. The floor shows the ambient light only, so it is even; a non-finite blend would
        show as a block of pixels unlike their neighbors (Metal clamps NaN to the other operand).
    */
    void checkProbeBlendWithoutEnvironmentIsFinite (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setEmitterColor (scene, { 0.0f, 0.0f, 0.0f });

        // Fading out from 1 m to 3 m around a point 1 m over the floor's center
        auto& probe = addReflectionProbe (scene, { 0.0f, 1.0f, 0.0f }, 3.0f);
        probe.falloff = 2.0f;

        SceneRenderer renderer;
        if (tier == RenderSettings::TierPreference::full)
            setFullTierTaa (renderer, 1);
        else
            setTier (renderer, tier);

        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        const auto floor = floorRegion();
        auto uneven = 0;
        for (int y = floor.getY(); y < floor.getBottom() - 1; ++y)
        {
            for (int x = floor.getX(); x < floor.getRight() - 1; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                for (const auto& neighbor : { image.getPixelColor (x + 1, y), image.getPixelColor (x, y + 1) })
                    if (std::abs (color.getRed() - neighbor.getRed()) > 2 || std::abs (color.getGreen() - neighbor.getGreen()) > 2 || std::abs (color.getBlue() - neighbor.getBlue()) > 2)
                        ++uneven;
            }
        }

        EXPECT_EQ (uneven, 0);
        EXPECT_GT (meanColor (image, floor).getGreen(), 10); // Not black: the ambient light shows

#if YUP_ENABLE_SHADER_TRANSPILER
        if (tier == RenderSettings::TierPreference::full)
        {
            const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
            ASSERT_NE (sceneColor, nullptr);
            EXPECT_TRUE (isFiniteTexture (sceneColor));
            EXPECT_FALSE (isSaturatedTexture (sceneColor));
        }
#endif
    }

    /** On @p tier, a box probe enclosing the scene, its reflections projected onto the box, shows
        the floor the emitter too.
    */
    void checkBoxProbeWithParallaxReflects (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto before = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (before.isValid()) << renderer.getLastError();

        auto& probe = addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);
        probe.shape = ReflectionProbeNode::Shape::box;
        probe.influenceExtents = { 40.0f, 5.0f, 45.0f };
        ASSERT_TRUE (probe.parallaxCorrection);

        const auto after = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (after.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        RecordProperty ("floorGreenBefore", meanColor (before, floorRegion()).getGreen());
        RecordProperty ("floorGreenAfter", meanColor (after, floorRegion()).getGreen());
        EXPECT_GT (meanColor (after, floorRegion()).getGreen(), meanColor (before, floorRegion()).getGreen() + 40);
    }

    /** On @p tier, bakeProbes() bakes the probes never baked or asked to bake again, and returns
        how many; render() then has nothing left to bake.
    */
    void checkBakeProbesReportsPendingCount (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        auto& first = addReflectionProbe (scene, { -1.0f, 1.0f, -1.0f }, 20.0f, "first");
        addReflectionProbe (scene, { 1.0f, 1.0f, -1.0f }, 5.0f, "second");

        SceneRenderer renderer;
        setTier (renderer, tier);
        EXPECT_EQ (renderer.bakeProbes (device, scene), 2);
        EXPECT_EQ (renderer.bakeProbes (device, scene), 0);

        first.requestBake();
        EXPECT_EQ (renderer.bakeProbes (device, scene), 1);
        EXPECT_EQ (getNumBakes (renderer), 3);

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 3);
        EXPECT_GT (meanColor (renderToImage (renderer, scene, 64), floorRegion()).getGreen(), 100);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, the probe resources live only as long as probes need them: the capture shadow
        maps and scratch targets until the next call that bakes nothing, the atlas until no probe is
        left. The capture shadow renderer itself, with its pipelines, stays.
    */
    void checkRemovedProbesReleaseTheAtlas (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        setTier (renderer, tier);
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        const auto* baker = renderer.getProbeBaker();
        ASSERT_NE (baker, nullptr);
        EXPECT_NE (baker->getReflectionAtlas(), nullptr);
        EXPECT_TRUE (baker->holdsCaptureTargets());

        const auto* captureShadows = baker->getCaptureShadowRenderer();
        ASSERT_NE (captureShadows, nullptr);
        EXPECT_NE (captureShadows->getCascadeTexture(), nullptr);

        // The next call bakes nothing: the capture textures go, the pipelines stay for the frames in flight
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (baker->getCaptureShadowRenderer(), captureShadows);
        EXPECT_EQ (captureShadows->getCascadeTexture(), nullptr);
        EXPECT_EQ (captureShadows->getAtlasTexture(), nullptr);
        EXPECT_FALSE (baker->holdsCaptureTargets());
        EXPECT_NE (baker->getReflectionAtlas(), nullptr);

        scene.getRoot()->removeChild (scene.getRoot()->findChild ("probe"));
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getProbeBaker(), baker);
        EXPECT_EQ (baker->getNumBakedProbes(), 0);
        EXPECT_EQ (baker->getReflectionAtlas(), nullptr);
    }

    /** On @p tier, a probe over an open sky reflects what the environment does: its capture faces
        keep the orientation of the cube. The sky's color tells directions apart (red grows towards
        +x, blue towards -z, green upwards), so a face mirrored or turned shows on the floor.
    */
    void checkProbeOfAnOpenSkyMatchesTheEnvironment (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool withProbe)
        {
            Scene scene;
            buildMirrorFloorScene (scene, false);
            scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>& d)
            {
                return Vector3<float> (0.6f + 0.55f * jlimit (-1.0f, 1.0f, 2.5f * d.getX()),
                                       0.3f + 0.3f * d.getY(),
                                       0.6f - 0.55f * jlimit (-1.0f, 1.0f, 2.5f * d.getZ()));
            }, 256, 7));

            // Without parallax the probe reads its capture along the reflected direction, as the environment does
            if (withProbe)
                addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f).parallaxCorrection = false;

            SceneRenderer renderer;
            setTier (renderer, tier);
            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (getNumBakes (renderer), withProbe ? 1 : -1);
            return image;
        };

        const auto environment = render (false);
        const auto probe = render (true);
        ASSERT_TRUE (environment.isValid() && probe.isValid());

        // Left and right, near and far parts of the floor reflect different parts of the sky
        const Rectangle<int> regions[] = { { 8, 30, 12, 10 }, { 44, 30, 12, 10 }, { 8, 50, 12, 10 }, { 44, 50, 12, 10 }, { 26, 26, 12, 8 } };
        const auto left = meanColor (environment, regions[0]), right = meanColor (environment, regions[1]);
        EXPECT_GT (std::abs (left.getRed() - right.getRed()), 20); // The check cannot pass on a uniform sky

        for (const auto& region : regions)
        {
            const auto expected = meanColor (environment, region);
            const auto actual = meanColor (probe, region);
            RecordProperty (("environment" + region.toString()).toStdString(), expected.toString().toStdString());
            RecordProperty (("probe" + region.toString()).toStdString(), actual.toString().toStdString());
            EXPECT_NEAR (actual.getRed(), expected.getRed(), 8) << region.toString();
            EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 8) << region.toString();
            EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 8) << region.toString();
        }
    }

    /** On @p tier, below the float floor the probes hold RGBM, which the passes decode: the
        reflection matches the half float one, channel by channel. The mirror floor reflects a blue
        emitter of radiance 3 on the left, a channel no tone map of the capture may saturate, and a
        dim gray one of 0.006 on the right, which 8 bit linear steps would round by a third.
    */
    void checkRgbmProbeShowsLikeTheFloatOne (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool rgbm)
        {
            Scene scene;
            buildMirrorFloorScene (scene, false);
            scene.setAmbientColor (Colors::black);
            scene.getRoot()->addChild (makeHorizontalQuad ("blue", 2.0f, { -30.0f, -40.0f, 30.0f, 41.5f }, false, makeEmitterMaterial ({ 0.0f, 0.0f, 3.0f })));
            scene.getRoot()->addChild (makeHorizontalQuad ("dim", 2.0f, { 0.0f, -40.0f, 30.0f, 41.5f }, false, makeEmitterMaterial ({ 0.006f, 0.006f, 0.006f })));
            addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

            SceneRenderer renderer;
            setTier (renderer, tier);
            if (rgbm)
                renderer.forceRgbmRadianceForTesting();

            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

            const auto* baker = renderer.getProbeBaker();
            const auto atlas = baker != nullptr ? baker->getReflectionAtlas() : nullptr;
            EXPECT_TRUE (atlas != nullptr && atlas->getFormat() == (rgbm ? GpuTextureFormat::rgba8unorm : GpuTextureFormat::rgba16float));
            return image;
        };

        const auto floatImage = render (false);
        const auto rgbmImage = render (true);
        ASSERT_TRUE (floatImage.isValid() && rgbmImage.isValid());

        const Rectangle<int> blueRegion (6, 28, 14, 24), dimRegion (44, 28, 14, 24);
        const auto expectedBlue = meanColor (floatImage, blueRegion), actualBlue = meanColor (rgbmImage, blueRegion);
        const auto expectedDim = meanColor (floatImage, dimRegion), actualDim = meanColor (rgbmImage, dimRegion);
        RecordProperty ("blueFloat", expectedBlue.toString().toStdString());
        RecordProperty ("blueRgbm", actualBlue.toString().toStdString());
        RecordProperty ("dimFloat", expectedDim.toString().toStdString());
        RecordProperty ("dimRgbm", actualDim.toString().toStdString());

        // Not vacuous: the blue is well above what a saturated capture would give, the dim region above black
        EXPECT_GT (expectedBlue.getBlue(), 210);
        EXPECT_GT (expectedDim.getGreen(), 10);

        for (const auto& [expected, actual, tolerance] : { std::tuple { expectedBlue, actualBlue, 3 }, std::tuple { expectedDim, actualDim, 1 } })
        {
            EXPECT_NEAR (actual.getRed(), expected.getRed(), tolerance);
            EXPECT_NEAR (actual.getGreen(), expected.getGreen(), tolerance);
            EXPECT_NEAR (actual.getBlue(), expected.getBlue(), tolerance);
        }
    }

    /** On @p tier, the reflection probes keep the multiple scattering compensation of rough metals
        (Fdez-Aguera 2019, preflight R23): a white metal of roughness 1 facing the view, inside a room
        whose walls all emit 0.5, lit only by a probe that captured the room, reflects all of it, as
        under a uniform environment (RoughWhiteMetalPassesTheWhiteFurnaceTest). Single scattering
        alone would reflect 1 - ln 2 of it. The tolerance is the environment furnace's 0.02 plus a
        level for the probe's own capture and filtering.
    */
    void checkProbeKeepsTheMultipleScatteringOfRoughMetals (RenderSettings::TierPreference tier)
    {
        Scene scene;
        auto metal = Material::Ptr (new Material());
        metal->roughnessFactor = 1.0f;
        addPlate (scene, metal);

        auto room = EntityNode::Ptr (new EntityNode ("room"));
        room->attach<MeshNode> (makeBox (10.0f, makeEmitterMaterial ({ 0.5f, 0.5f, 0.5f }), true));
        scene.getRoot()->addChild (room);

        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        addReflectionProbe (scene, { 0.0f, 0.0f, 2.0f }, 30.0f).parallaxCorrection = false;

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        std::array<double, 3> radiance {};
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

        RecordProperty ("probeFurnaceBrightness", String (radiance[1] / 0.5, 4).toStdString());

        for (const auto channel : radiance)
            EXPECT_NEAR (channel, 0.5, 0.025);
    }

    /** On @p tier, parallax correction puts a box probe's reflections where the walls are: a mirror
        floor inside a 6 x 6 m room, the box probe filling it, captured from its center 1 m up,
        reflects a small green patch high on the far wall at its true mirror position. Read along the
        reflected direction alone, as without the correction, the patch lands elsewhere.
    */
    void checkBoxProbeParallaxPlacesTheReflectionOnTheWalls (RenderSettings::TierPreference tier)
    {
        constexpr int size = 128;
        const Vector3<float> eye (0.0f, 1.5f, 2.5f);
        const Vector3<float> patchCenter (1.1f, 1.6f, -2.95f);

        const auto render = [&] (bool parallax)
        {
            Scene scene;
            scene.setAmbientColor (Colors::black);
            scene.setUsingDefaultLight (false);

            auto mirror = Material::Ptr (new Material());
            mirror->metallicFactor = 1.0f;
            mirror->roughnessFactor = 0.02f;
            scene.getRoot()->addChild (makeHorizontalQuad ("floor", 0.0f, { -3.0f, -3.0f, 6.0f, 6.0f }, true, mirror));

            // On the far wall, facing into the room
            std::vector<Mesh::Vertex> vertices (4);
            vertices[0].position = patchCenter + Vector3<float> (-0.5f, -0.3f, 0.0f);
            vertices[1].position = patchCenter + Vector3<float> (0.5f, -0.3f, 0.0f);
            vertices[2].position = patchCenter + Vector3<float> (0.5f, 0.3f, 0.0f);
            vertices[3].position = patchCenter + Vector3<float> (-0.5f, 0.3f, 0.0f);
            for (auto& vertex : vertices)
                vertex.normal = { 0.0f, 0.0f, 1.0f };

            auto mesh = Mesh::Ptr (new Mesh ("patch"));
            mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeEmitterMaterial ({ 0.0f, 4.0f, 0.0f }));
            auto patch = EntityNode::Ptr (new EntityNode ("patch"));
            patch->attach<MeshNode> (mesh);
            scene.getRoot()->addChild (patch);

            addCamera (scene, eye, {});

            auto& probe = addReflectionProbe (scene, { 0.0f, 1.0f, 0.0f }, 1.0f);
            probe.shape = ReflectionProbeNode::Shape::box;
            probe.influenceExtents = { 3.0f, 1.2f, 3.0f };
            probe.falloff = 0.0f;
            probe.parallaxCorrection = parallax;

            SceneRenderer renderer;
            setTier (renderer, tier);
            const auto image = renderToImage (renderer, scene, size);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (getNumBakes (renderer), 1);
            return image;
        };

        // The green pixels of the floor's reflection: the patch itself lies above the frustum
        const auto centroid = [] (const Image& image) -> std::optional<Point<float>>
        {
            Point<float> sum;
            auto count = 0;
            for (int y = 0; y < image.getHeight(); ++y)
            {
                for (int x = 0; x < image.getWidth(); ++x)
                {
                    const auto color = image.getPixelColor (x, y);
                    if (color.getGreen() > 60 && color.getGreen() > color.getRed() + 40)
                    {
                        sum += Point<float> (static_cast<float> (x) + 0.5f, static_cast<float> (y) + 0.5f);
                        ++count;
                    }
                }
            }

            return count > 0 ? std::optional<Point<float>> (sum / static_cast<float> (count)) : std::nullopt;
        };

        // Where the mirror image of the patch's center, under the floor, shows on screen
        CameraNode camera;
        const auto viewProjection = Matrix4::lookAt (eye, {}, { 0.0f, 1.0f, 0.0f }).followedBy (camera.getProjectionMatrix (1.0f));
        const auto ndc = viewProjection.transformPoint ({ patchCenter.getX(), -patchCenter.getY(), patchCenter.getZ() });
        const Point<float> expected ((ndc.getX() * 0.5f + 0.5f) * size, (0.5f - ndc.getY() * 0.5f) * size);

        const auto corrected = centroid (render (true));
        const auto uncorrected = centroid (render (false));
        ASSERT_TRUE (corrected.has_value() && uncorrected.has_value());

        RecordProperty ("expected", expected.toString().toStdString());
        RecordProperty ("corrected", corrected->toString().toStdString());
        RecordProperty ("uncorrected", uncorrected->toString().toStdString());

        EXPECT_LT (corrected->distanceTo (expected), 3.0f);
        EXPECT_GT (uncorrected->distanceTo (expected), corrected->distanceTo (expected) + 5.0f);
    }

    /** On @p tier, probe captures that fail to record never fail the frame: it renders without the
        probe, retries once, then stops baking until bakeProbes() asks again.
    */
    void checkFailedProbeCapturesLeaveTheFrameWithoutProbes (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);

        SceneRenderer renderer;
        setTier (renderer, tier);
        renderer.failProbeCapturesForTesting();

        const auto first = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getLastError().isEmpty());
        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_EQ (getNumBakes (renderer), 0);
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUp());

        // Without the probe the floor shows no green
        const auto floor = meanColor (first, floorRegion());
        EXPECT_NEAR (floor.getGreen(), floor.getRed(), 2);

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getProbeBaker()->hasGivenUp());

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 0);

        // An explicit bake tries again, and fails once more
        EXPECT_EQ (renderer.bakeProbes (device, scene), 0);
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUp());
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** buildMirrorFloorScene() without its emitter, plus a red emitter standing on the floor in
        front of the camera, 1.2 wide and 0.8 tall at z = -0.5, facing it: the camera sees both the
        emitter (rows 8 to 26 at 64 x 64) and its reflection in the floor (rows 26 to 40), which only
        screen-space reflections show, since the scene has no environment and no probe.
    */
    static void buildOnScreenEmitterScene (Scene& scene, float emission = 1.0f)
    {
        buildMirrorFloorScene (scene, false);

        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -0.6f, 0.0f, -0.5f };
        vertices[1].position = { 0.6f, 0.0f, -0.5f };
        vertices[2].position = { 0.6f, 0.8f, -0.5f };
        vertices[3].position = { -0.6f, 0.8f, -0.5f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("emitter"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeEmitterMaterial ({ emission, 0.0f, 0.0f }));

        auto emitter = EntityNode::Ptr (new EntityNode ("emitter"));
        emitter->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (emitter);
    }

    /** The floor of buildOnScreenEmitterScene() reflecting the emitter, at 64 x 64: between the
        emitter's foot (row 26) and the bottom of its reflection (row 40), within its columns.
    */
    static Rectangle<int> reflectedEmitterRegion()
    {
        return { 22, 28, 20, 10 };
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** The mean linear red of @p texture over @p area, below 1, read back in 16 bits: its 8 high
        bits in red, the next 8 in green. */
    double meanLinearRed (const GpuTexture::Ptr& texture, Rectangle<int> area)
    {
        const auto linear = mapTexels (texture, "vec4(floor(t.r * 255.0) / 255.0, fract(t.r * 255.0), 0.0, 1.0)");
        if (! linear.isValid())
            return -1.0;

        auto sum = 0.0;
        for (int y = area.getY(); y < area.getBottom(); ++y)
        {
            for (int x = area.getX(); x < area.getRight(); ++x)
            {
                const auto color = linear.getPixelColor (x, y);
                sum += (color.getRed() + color.getGreen() / 255.0) / 255.0;
            }
        }

        return sum / (area.getWidth() * area.getHeight());
    }
#endif

    /** Switches screen-space reflections on or off, or back to the tier default. */
    static void setScreenSpaceReflections (SceneRenderer& renderer, std::optional<bool> screenSpaceReflections)
    {
        auto settings = renderer.getRenderSettings();
        settings.screenSpaceReflections = screenSpaceReflections;
        renderer.setRenderSettings (settings);
    }

    /** On a raster device, screen-space reflections are unavailable: turning them on renders the
        same image as leaving them off, without an error and without an output.
    */
    void checkScreenSpaceReflectionsUnavailableOnRasterRendersFallback (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool screenSpaceReflections)
        {
            Scene scene;
            buildOnScreenEmitterScene (scene);

            SceneRenderer renderer;
            setTier (renderer, tier);
            setScreenSpaceReflections (renderer, screenSpaceReflections);
            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::reflections), nullptr);
            EXPECT_EQ (renderer.getReflectionPass(), nullptr);

            const auto* capabilities = renderer.getCapabilities();
            EXPECT_TRUE (capabilities != nullptr && ! capabilities->isAvailable (RenderCapabilities::Feature::screenSpaceReflections));
            return image;
        };

        // A fresh Metal device's first frame can differ by a level in a few pixels, see checkContactShadowsIgnoredOnRaster()
        ASSERT_TRUE (render (false).isValid());

        const auto withReflections = render (true);
        const auto without = render (false);
        ASSERT_TRUE (withReflections.isValid() && without.isValid());
        EXPECT_EQ (countDifferencesAboveOneLevel (withReflections, without), 0);
    }

    /** A vertical quad in the plane x = @p x over @p area (z, y), facing +X or -X, the entity @p name. */
    static EntityNode::Ptr makeWallQuad (const char* name, float x, Rectangle<float> area, bool facingPositiveX, Material::Ptr material)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { x, area.getY(), area.getBottom() };
        vertices[1].position = { x, area.getY(), area.getX() };
        vertices[2].position = { x, area.getBottom(), area.getX() };
        vertices[3].position = { x, area.getBottom(), area.getBottom() };

        for (auto& vertex : vertices)
            vertex.normal = { facingPositiveX ? 1.0f : -1.0f, 0.0f, 0.0f };

        // Counterclockwise seen from the side the normal points to
        auto mesh = Mesh::Ptr (new Mesh (name));
        mesh->addPrimitive (std::move (vertices), facingPositiveX ? std::vector<uint32> { 0, 1, 2, 0, 2, 3 } : std::vector<uint32> { 0, 2, 1, 0, 3, 2 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode (name));
        entity->attach<MeshNode> (mesh);
        return entity;
    }

    /** A white floor 8 x 6 at y = 0, from x = -4 to 4, lit by a constant gray environment of 0.5
        and no light, with no ambient light. A red emissive wall, the entity "red wall", double-sided
        and 3 m high, emitting 6, stands across it at x = -1.5. With @p withDivider an opaque gray wall, the entity
        "divider", double-sided and as high, splits the floor at x = 0: the floor between the two
        walls sees the red wall, the floor right of the divider does not. The volume probes next to
        the divider on its left (addIrradianceVolume()) see a lot of red, so trilinear weights alone
        would leak it to the right. The camera looks straight down from 11 m, +Z towards the bottom
        of the frame, so at 64 x 64 a floor point (x, z) shows at (32 + 6.88 x, 32 + 6.88 z).
    */
    static void buildRedRoomScene (Scene& scene, bool withDivider = true)
    {
        auto white = Material::Ptr (new Material());
        white->baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        white->metallicFactor = 0.0f;
        white->roughnessFactor = 1.0f;
        scene.getRoot()->addChild (makeHorizontalQuad ("floor", 0.0f, { -4.0f, -3.0f, 8.0f, 6.0f }, true, white));

        auto red = makeEmitterMaterial ({ 6.0f, 0.0f, 0.0f });
        red->doubleSided = true;
        scene.getRoot()->addChild (makeWallQuad ("red wall", -1.5f, { -3.0f, 0.0f, 6.0f, 3.0f }, true, red));

        if (withDivider)
        {
            auto gray = makeWallMaterial();
            gray->doubleSided = true;
            scene.getRoot()->addChild (makeWallQuad ("divider", 0.0f, { -3.0f, 0.0f, 6.0f, 3.0f }, true, gray));
        }

        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        addCamera (scene, { 0.0f, 11.0f, 0.0f }, {}, { 0.0f, 0.0f, -1.0f });
    }

    /** The floor between the two walls of buildRedRoomScene() at 64 x 64, from x = -1.3 to -0.15 and z = -2 to 2. */
    static Rectangle<int> leftFloorRegion() { return { 23, 19, 8, 26 }; }

    /** The floor right of the divider of buildRedRoomScene() at 64 x 64, next to it: x = 0.15 to 0.58, z = -2 to 2. */
    static Rectangle<int> rightFloorRegion() { return { 33, 19, 3, 26 }; }

    /** Adds an irradiance volume of @p nx x @p ny x @p nz probes enclosing the room of
        buildRedRoomScene(), the entity "volume", and returns its part. With 2 rows they lie at
        y = 0.5 and 2.5, above the floor, and with 6 columns no probe lies in a wall: the cells are
        1.5 m wide along X, centered at x = -3.75, -2.25, -0.75, 0.75, 2.25 and 3.75. The floor lies
        0.5 m inside the box, beyond the falloff of 0.25.
    */
    static IrradianceVolumeNode& addIrradianceVolume (Scene& scene, int nx, int ny, int nz)
    {
        auto entity = EntityNode::Ptr (new EntityNode ("volume"));
        auto& volume = entity->attach<IrradianceVolumeNode>();
        volume.extents = { 4.5f, 2.0f, 3.5f };
        volume.resolutionX = nx;
        volume.resolutionY = ny;
        volume.resolutionZ = nz;
        volume.falloff = 0.25f;
        entity->setPosition ({ 0.0f, 1.5f, 0.0f });
        scene.getRoot()->addChild (entity);
        return volume;
    }

    /** Adds @p count small gray quads lying on the right of the floor of buildRedRoomScene(), each
        its own entity and draw, for scenes with many draws. */
    static void addSmallCasters (Scene& scene, int count)
    {
        for (int i = 0; i < count; ++i)
        {
            const auto x = 0.5f + 0.3f * static_cast<float> (i % 10);
            const auto z = -2.5f + 0.5f * static_cast<float> (i / 10);
            scene.getRoot()->addChild (makeHorizontalQuad ("caster", 0.05f, { x, z, 0.1f, 0.1f }, true, makeWallMaterial()));
        }
    }

    /** How much redder than green @p color is. */
    static float redOverGreen (Color color)
    {
        return static_cast<float> (color.getRed()) / static_cast<float> (jmax (1, static_cast<int> (color.getGreen())));
    }

    /** buildRedRoomScene() without the divider, seen from (1.5, 2.5, 0) towards (-1, 0.3, 0): the
        red wall faces the camera across the floor, which giveRedWallFloorRegion() finds. Seen from
        above, as in the other red room tests, the wall would cover about a texel within the reach of
        the horizon search, which screen-space GI cannot gather from. Render it with setScreenSpaceGI().
    */
    static void buildGIRedRoomScene (Scene& scene)
    {
        buildRedRoomScene (scene, false);
        scene.getRoot()->removeChild (scene.getRoot()->findChild ("camera"));
        addCamera (scene, { 1.5f, 2.5f, 0.0f }, { -1.0f, 0.3f, 0.0f });
    }

    /** Selects the full tier with 8 TAA samples and screen-space diffuse GI on or off, gathering
        within an ambient occlusion distance of 1 m: the settings of buildGIRedRoomScene() and
        buildFacingAwayWallScene().
    */
    static void setScreenSpaceGI (SceneRenderer& renderer, bool screenSpaceGI)
    {
        setFullTierTaa (renderer, 8);

        auto settings = renderer.getRenderSettings();
        settings.aoDistance = 1.0f;
        settings.screenSpaceGI = screenSpaceGI;
        renderer.setRenderSettings (settings);
    }

    /** The first and last rows of the middle column of a 64 x 64 @p image where an emissive red wall
        saturates the red and not the green, or -1 for none. */
    static std::pair<int, int> findRedWallRows (const Image& image)
    {
        auto top = -1, bottom = -1;
        for (int y = 0; image.isValid() && y < 64; ++y)
        {
            const auto color = image.getPixelColor (32, y);
            if (color.getRed() > 200 && color.getGreen() < 100)
            {
                top = top < 0 ? y : top;
                bottom = y;
            }
        }

        return { top, bottom };
    }

    /** A white floor 8 x 8 at y = 0 under a constant gray environment of 0.5, and a red emissive
        wall 4 m wide and 0.5 m high across it at z = 0, single-sided and facing the camera, which
        looks down at it from (0, 4, 2.5). The floor in front of the wall sees its emitting side; the
        floor behind it, which shows above its top edge from 0.3 m behind it, only its back. Render
        it with setScreenSpaceGI().
    */
    static void buildFacingAwayWallScene (Scene& scene)
    {
        auto white = Material::Ptr (new Material());
        white->baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        white->metallicFactor = 0.0f;
        white->roughnessFactor = 1.0f;
        scene.getRoot()->addChild (makeHorizontalQuad ("floor", 0.0f, { -4.0f, -4.0f, 8.0f, 8.0f }, true, white));

        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { -2.0f, 0.0f, 0.0f };
        vertices[1].position = { 2.0f, 0.0f, 0.0f };
        vertices[2].position = { 2.0f, 0.5f, 0.0f };
        vertices[3].position = { -2.0f, 0.5f, 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto wall = Mesh::Ptr (new Mesh ("red wall"));
        wall->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, makeEmitterMaterial ({ 6.0f, 0.0f, 0.0f }));

        auto entity = EntityNode::Ptr (new EntityNode ("red wall"));
        entity->attach<MeshNode> (wall);
        scene.getRoot()->addChild (entity);

        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        addCamera (scene, { 0.0f, 4.0f, 2.5f }, { 0.0f, 0.0f, -0.5f });
    }

    /** The number of irradiance volumes the renderer baked, or -1 before it had a baker. */
    static int getNumVolumeBakes (const SceneRenderer& renderer)
    {
        const auto* baker = renderer.getProbeBaker();
        return baker != nullptr ? baker->getNumVolumeBakes() : -1;
    }

    /** On @p tier, an irradiance volume tints the floor next to the red wall red: without it the
        floor shows the gray environment only.
    */
    void checkIrradianceVolumeTintsTheFloorNearTheRedWall (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene);

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto without = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_EQ (renderer.getProbeBaker(), nullptr); // Nothing allocated without a volume

        addIrradianceVolume (scene, 6, 2, 4);
        const auto with = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        ASSERT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);

        RecordProperty ("leftWithout", without.toString().toStdString());
        RecordProperty ("leftWith", with.toString().toStdString());
        EXPECT_NEAR (redOverGreen (without), 1.0f, 0.05f);
        EXPECT_GT (redOverGreen (with), redOverGreen (without) + 0.2f);
    }

    /** On @p tier, the volume's visibility keeps the red light of the probes on the left of the
        divider off the floor right of it, which those probes cannot see: their trilinear weights
        alone would let it through. Below the float floor visibility is skipped, so the check needs
        rg16float render targets.
    */
    void checkVisibilityStopsLightLeakingThroughTheWall (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene);
        addIrradianceVolume (scene, 6, 2, 4);

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);

        if (! device->isFormatRenderable (GpuTextureFormat::rg16float))
            GTEST_SKIP() << "Visibility is skipped below the float floor";

        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        ASSERT_NE (renderer.getProbeBaker()->getVisibilityAtlas(), nullptr);

        const auto left = meanColor (image, leftFloorRegion());
        const auto right = meanColor (image, rightFloorRegion());
        RecordProperty ("left", left.toString().toStdString());
        RecordProperty ("right", right.toString().toStdString());

        EXPECT_GT (redOverGreen (left), 1.2f); // Not vacuous: the red light is there
        EXPECT_LT (redOverGreen (right), 1.1f);
    }

    /** On @p tier, a volume bakes again when it moves or one of its own fields changes, never because
        the rest of the scene changed, until requestBake() asks for it.
    */
    void checkMovingAVolumeBakesItAgain (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        auto& volume = addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        setTier (renderer, tier);
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);

        // Unchanged, it keeps its bake
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);

        scene.getRoot()->findChild ("volume")->setPosition ({ 0.0f, 1.6f, 0.0f });
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 2);

        volume.normalBias = 0.1f;
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 3);

        // The wall turns green: the volume keeps showing what it captured
        scene.getRoot()->findChild ("red wall")->getNode<MeshNode>()->mesh->getPrimitive (0).material->emissiveFactor = { 0.0f, 6.0f, 0.0f };
        const auto stale = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        EXPECT_EQ (getNumVolumeBakes (renderer), 3);
        EXPECT_GT (stale.getRed(), stale.getGreen() + 10);

        volume.requestBake();
        const auto rebaked = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        EXPECT_EQ (getNumVolumeBakes (renderer), 4);
        EXPECT_GT (rebaked.getGreen(), rebaked.getRed() + 10);
    }

    /** On @p tier, bakeProbes() bakes the volumes too, and returns how many probes and volumes it
        baked; render() then has nothing left to bake.
    */
    void checkBakeProbesBakesTheVolumes (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        auto& volume = addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        setTier (renderer, tier);
        EXPECT_EQ (renderer.bakeProbes (device, scene), 1);
        EXPECT_EQ (renderer.bakeProbes (device, scene), 0);

        volume.requestBake();
        EXPECT_EQ (renderer.bakeProbes (device, scene), 1);
        EXPECT_EQ (getNumVolumeBakes (renderer), 2);

        const auto floor = meanColor (renderToImage (renderer, scene, 64), leftFloorRegion());
        EXPECT_EQ (getNumVolumeBakes (renderer), 2);
        EXPECT_GT (redOverGreen (floor), 1.2f);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, the atlases of the volumes live as long as a volume needs them. */
    void checkRemovedVolumesReleaseTheAtlases (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        setTier (renderer, tier);
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        const auto* baker = renderer.getProbeBaker();
        ASSERT_NE (baker, nullptr);
        EXPECT_NE (baker->getIrradianceAtlas(), nullptr);
        EXPECT_EQ (baker->getVisibilityAtlas() != nullptr, device->isFormatRenderable (GpuTextureFormat::rg16float));
        EXPECT_EQ (baker->getNumBakedVolumes(), 1);

        // The next call bakes nothing and keeps the atlases
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_FALSE (baker->holdsCaptureTargets());
        EXPECT_NE (baker->getIrradianceAtlas(), nullptr);

        scene.getRoot()->removeChild (scene.getRoot()->findChild ("volume"));
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (renderer.getProbeBaker(), baker);
        EXPECT_EQ (baker->getNumBakedVolumes(), 0);
        EXPECT_EQ (baker->getIrradianceAtlas(), nullptr);
        EXPECT_EQ (baker->getVisibilityAtlas(), nullptr);
    }

    /** On @p tier, below the float floor the volume stores its first band in RGBM and the second as
        ratios to it, and lights the floor as the float volume does.
    */
    void checkRgbmVolumeLightsLikeTheFloatOne (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool rgbm)
        {
            Scene scene;
            buildRedRoomScene (scene, false);
            addIrradianceVolume (scene, 4, 2, 2);

            SceneRenderer renderer;
            setTier (renderer, tier);
            if (rgbm)
                renderer.forceRgbmRadianceForTesting();

            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

            const auto* baker = renderer.getProbeBaker();
            const auto atlas = baker != nullptr ? baker->getIrradianceAtlas() : nullptr;
            EXPECT_TRUE (atlas != nullptr && atlas->getFormat() == (rgbm ? GpuTextureFormat::rgba8unorm : GpuTextureFormat::rgba16float));
            return image;
        };

        const auto floatImage = render (false);
        const auto rgbmImage = render (true);
        ASSERT_TRUE (floatImage.isValid() && rgbmImage.isValid());

        // The floor behind the red wall, in front of it and across the room
        for (const auto& region : { Rectangle<int> (6, 19, 8, 26), Rectangle<int> (23, 19, 8, 26), Rectangle<int> (40, 19, 16, 26) })
        {
            const auto expected = meanColor (floatImage, region), actual = meanColor (rgbmImage, region);
            RecordProperty (("float" + region.toString()).toStdString(), expected.toString().toStdString());
            RecordProperty (("rgbm" + region.toString()).toStdString(), actual.toString().toStdString());

            EXPECT_NEAR (actual.getRed(), expected.getRed(), 3) << region.toString();
            EXPECT_NEAR (actual.getGreen(), expected.getGreen(), 3) << region.toString();
            EXPECT_NEAR (actual.getBlue(), expected.getBlue(), 3) << region.toString();
        }

        // Not vacuous: the volume tints the floor, more next to the wall than across the room
        EXPECT_GT (redOverGreen (meanColor (floatImage, { 23, 19, 8, 26 })), redOverGreen (meanColor (floatImage, { 40, 19, 16, 26 })) + 0.1f);
    }

    /** On @p tier, a volume with more probe positions than one bake records (preflight R28) bakes
        over several frames, and weighs nothing until all of them are captured.
    */
    void checkProgressiveVolumeBakeShadesOnceComplete (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        addIrradianceVolume (scene, 7, 7, 7);

        const auto positions = 7 * 7 * 7;
        ASSERT_GT (positions, ProbeBaker::maxVolumeProbesPerBake);
        const auto expectedFrames = (positions + ProbeBaker::maxVolumeProbesPerBake - 1) / ProbeBaker::maxVolumeProbesPerBake;

        SceneRenderer renderer;
        setTier (renderer, tier);
        const auto first = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 0);
        EXPECT_NEAR (redOverGreen (meanColor (first, leftFloorRegion())), 1.0f, 0.05f); // Weighs nothing yet

        auto frames = 1;
        Image last;
        while (getNumVolumeBakes (renderer) < 1 && frames < 4 * expectedFrames)
        {
            last = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (last.isValid()) << renderer.getLastError();
            ++frames;
        }

        RecordProperty ("framesToBake", frames);
        EXPECT_EQ (frames, expectedFrames);
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);
        EXPECT_GT (redOverGreen (meanColor (last, leftFloorRegion())), 1.2f);

        // Baked, it records nothing more
        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);
    }

    /** On @p tier, a volume whose captures fail leaves the reflection probes baking and reflecting:
        each feature has its own failure latch.
    */
    void checkFailedVolumeCapturesLeaveTheProbesReflecting (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        setTier (renderer, tier);
        renderer.failVolumeCapturesForTesting();

        Image image;
        for (int i = 0; i < 3; ++i)
        {
            image = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        }

        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_TRUE (renderer.getProbeBaker()->hasGivenUpVolumes());
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUpProbes());
        EXPECT_EQ (getNumVolumeBakes (renderer), 0);
        EXPECT_EQ (getNumBakes (renderer), 1);

        const auto floor = meanColor (image, floorRegion());
        EXPECT_GT (floor.getGreen(), floor.getRed() + 40); // The probe still reflects the emitter
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, a volume whose resolution changes gives its blocks back and bakes again into new ones. */
    void checkResizingAVolumeBakesItAgain (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        auto& volume = addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        setTier (renderer, tier);
        auto bakes = 0;
        for (const auto resolutionX : { 4, 6, 3, 6 })
        {
            volume.resolutionX = resolutionX;
            const auto image = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
            EXPECT_EQ (getNumVolumeBakes (renderer), ++bakes) << resolutionX;
            EXPECT_GT (redOverGreen (meanColor (image, leftFloorRegion())), 1.2f) << resolutionX;
        }

        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_EQ (renderer.getProbeBaker()->getNumBakedVolumes(), 1);
    }

    /** On @p tier, the volume lights without an environment, and keeps its own intensity when the
        environment's changes: its diffuse light is outside the environment's intensity, and the
        indirect branch opens for it alone (lightsIndirect's probeInfo.z gate). The volume here covers
        the left of the room only, so the floor on the right shows the environment.
    */
    void checkVolumeLightsWithoutTheEnvironmentAndKeepsItsIntensity (RenderSettings::TierPreference tier)
    {
        const auto addLeftVolume = [] (Scene& scene)
        {
            auto& volume = addIrradianceVolume (scene, 4, 2, 2);
            volume.extents = { 2.75f, 2.0f, 3.5f };
            scene.getRoot()->findChild ("volume")->setPosition ({ -1.75f, 1.5f, 0.0f });
        };

        const Rectangle<int> outsideRegion (46, 19, 10, 26); // x = 2 to 3.5, beyond the volume's border at x = 1
        {
            Scene scene;
            buildRedRoomScene (scene, false);
            scene.setEnvironment (nullptr);
            addLeftVolume (scene);

            SceneRenderer renderer;
            setTierForExactFrames (renderer, tier);
            const auto image = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
            EXPECT_EQ (getNumVolumeBakes (renderer), 1);

            const auto inside = meanColor (image, leftFloorRegion());
            RecordProperty ("insideWithoutEnvironment", inside.toString().toStdString());
            EXPECT_GT (inside.getRed(), 60);
            EXPECT_GT (redOverGreen (inside), 1.5f);

#if YUP_ENABLE_SHADER_TRANSPILER
            if (tier == RenderSettings::TierPreference::full)
            {
                const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
                ASSERT_NE (sceneColor, nullptr);
                EXPECT_TRUE (isFiniteTexture (sceneColor));
                EXPECT_FALSE (isSaturatedTexture (sceneColor));
            }
#endif
        }

        Scene scene;
        buildRedRoomScene (scene, false);
        addLeftVolume (scene);

        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        const auto full = renderToImage (renderer, scene, 64);
        scene.setEnvironmentIntensity (0.25f);
        const auto quarter = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (full.isValid() && quarter.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 1); // An environment change never bakes it again

        const auto insideFull = meanColor (full, leftFloorRegion()), insideQuarter = meanColor (quarter, leftFloorRegion());
        const auto outsideFull = meanColor (full, outsideRegion), outsideQuarter = meanColor (quarter, outsideRegion);
        RecordProperty ("insideFull", insideFull.toString().toStdString());
        RecordProperty ("insideQuarter", insideQuarter.toString().toStdString());
        RecordProperty ("outsideFull", outsideFull.toString().toStdString());
        RecordProperty ("outsideQuarter", outsideQuarter.toString().toStdString());

        // The floor inside keeps the volume's light, the environment's specular share of a rough
        // dielectric aside (a few levels); the floor outside darkens with the environment
        EXPECT_NEAR (insideQuarter.getRed(), insideFull.getRed(), 6);
        EXPECT_NEAR (insideQuarter.getGreen(), insideFull.getGreen(), 6);
        EXPECT_LT (outsideQuarter.getGreen(), outsideFull.getGreen() - 30);
    }

    /** On @p tier, a volume that changes after its bake completed bakes again from its first
        position, weighs nothing until all of them are captured again, and is reported pending
        meanwhile, however many calls that takes (preflight R28).
    */
    void checkChangedVolumeBakesAgainWhole (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        addIrradianceVolume (scene, 7, 7, 7);

        const auto callsPerBake = (7 * 7 * 7 + ProbeBaker::maxVolumeProbesPerBake - 1) / ProbeBaker::maxVolumeProbesPerBake;

        SceneRenderer renderer;
        setTier (renderer, tier);
        for (int i = 0; i < 4 * callsPerBake && getNumVolumeBakes (renderer) < 1; ++i)
            ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();

        ASSERT_EQ (getNumVolumeBakes (renderer), 1);
        const auto* baker = renderer.getProbeBaker();
        ASSERT_NE (baker, nullptr);

        // Moved: it bakes again, and weighs nothing until that bake completes
        scene.getRoot()->findChild ("volume")->setPosition ({ 0.0f, 1.6f, 0.0f });

        SceneDrawList list;
        auto calls = 0;
        while (calls < 4 * callsPerBake)
        {
            if (renderer.bakeProbes (device, scene) == 0)
                break;

            ++calls;
            list.build (*scene.getRoot(), Matrix4());

            if (getNumVolumeBakes (renderer) < 2)
            {
                EXPECT_EQ (baker->getNumPending (list), 1) << calls;
                EXPECT_TRUE (baker->packIrradianceVolumesByPriority (list).empty()) << calls;

                const auto packed = baker->packIrradianceVolumes (list);
                const GpuIrradianceVolume empty {};
                EXPECT_TRUE (packed.size() == 1 && std::memcmp (&packed[0], &empty, sizeof (empty)) == 0) << calls;
            }
        }

        EXPECT_EQ (calls, callsPerBake);
        EXPECT_EQ (getNumVolumeBakes (renderer), 2);
        EXPECT_EQ (baker->getNumPending (list), 0);

        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        EXPECT_GT (redOverGreen (meanColor (image, leftFloorRegion())), 1.2f);
    }

    /** On @p tier, reflection probe captures that fail leave the irradiance volumes baking and lighting. */
    void checkFailedProbeCapturesLeaveTheVolumesLighting (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        addIrradianceVolume (scene, 4, 2, 2);
        addReflectionProbe (scene, { 2.0f, 1.0f, 0.0f }, 1.0f);

        SceneRenderer renderer;
        setTier (renderer, tier);
        renderer.failProbeCapturesForTesting();

        Image image;
        for (int i = 0; i < 3; ++i)
        {
            image = renderToImage (renderer, scene, 64);
            ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        }

        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_TRUE (renderer.getProbeBaker()->hasGivenUpProbes());
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUpVolumes());
        EXPECT_EQ (getNumBakes (renderer), 0);
        EXPECT_EQ (getNumVolumeBakes (renderer), 1);
        EXPECT_GT (redOverGreen (meanColor (image, leftFloorRegion())), 1.2f); // The volume still lights the floor
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, a volume whose capture fails leaves the frame rendering without it, and the
        baker gives up after two failures in a row, until bakeProbes() asks again.
    */
    void checkFailedVolumeCapturesLeaveTheFrameWithoutVolumes (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildRedRoomScene (scene, false);
        addIrradianceVolume (scene, 4, 2, 2);

        SceneRenderer renderer;
        setTier (renderer, tier);
        renderer.failVolumeCapturesForTesting();

        const auto first = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (first.isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getLastError().isEmpty());
        ASSERT_NE (renderer.getProbeBaker(), nullptr);
        EXPECT_EQ (getNumVolumeBakes (renderer), 0);
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUp());

        // Without the volume the floor shows the gray environment only
        EXPECT_NEAR (redOverGreen (meanColor (first, leftFloorRegion())), 1.0f, 0.05f);

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_TRUE (renderer.getProbeBaker()->hasGivenUp());

        ASSERT_TRUE (renderToImage (renderer, scene, 64).isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumVolumeBakes (renderer), 0);

        // An explicit bake tries again, and fails once more
        EXPECT_EQ (renderer.bakeProbes (device, scene), 0);
        EXPECT_FALSE (renderer.getProbeBaker()->hasGivenUp());
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    //==============================================================================
    /** Adds a UV sphere of @p radius at the origin, 64 segments by 32 rings, drawn with @p material. */
    static EntityNode* addSphere (Scene& scene, float radius, Material::Ptr material)
    {
        constexpr int segments = 64, rings = 32;
        const auto pi = MathConstants<float>::pi;

        std::vector<Mesh::Vertex> vertices;
        for (int ring = 0; ring <= rings; ++ring)
        {
            const auto theta = pi * static_cast<float> (ring) / static_cast<float> (rings);
            for (int segment = 0; segment <= segments; ++segment)
            {
                // Segment 0 faces the default camera, on +z, and the segments turn towards +x
                const auto phi = 2.0f * pi * static_cast<float> (segment) / static_cast<float> (segments);
                const Vector3<float> normal (std::sin (theta) * std::sin (phi), std::cos (theta), std::sin (theta) * std::cos (phi));

                auto& vertex = vertices.emplace_back();
                vertex.position = normal * radius;
                vertex.normal = normal;
                vertex.uv = { static_cast<float> (segment) / static_cast<float> (segments), static_cast<float> (ring) / static_cast<float> (rings) };
            }
        }

        // Counter-clockwise seen from outside: upper left, lower left, lower right, then upper right
        std::vector<uint32> indices;
        for (int ring = 0; ring < rings; ++ring)
        {
            for (int segment = 0; segment < segments; ++segment)
            {
                const auto upperLeft = static_cast<uint32> (ring * (segments + 1) + segment);
                const auto lowerLeft = upperLeft + static_cast<uint32> (segments + 1);
                indices.insert (indices.end(), { upperLeft, lowerLeft, lowerLeft + 1, upperLeft, lowerLeft + 1, upperLeft + 1 });
            }
        }

        auto mesh = Mesh::Ptr (new Mesh ("sphere"));
        mesh->addPrimitive (std::move (vertices), std::move (indices), std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode ("sphere"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
        return entity.get();
    }

    /** The radius in pixels of a sphere of @p radius at the origin seen by the default camera, from
        (0, 0, 5) with a vertical field of view of 0.8, in a square image of @p size pixels. */
    static float sphereRadiusInPixels (float radius, int size)
    {
        return static_cast<float> (size) * 0.5f * (radius / std::sqrt (25.0f - radius * radius)) / std::tan (0.4f);
    }

    /** The mean luminance of the pixels of a square image whose centers lie between @p inner and
        @p outer pixels from its center. */
    static double annulusLuminance (const Image& image, float inner, float outer)
    {
        const auto center = static_cast<float> (image.getWidth()) * 0.5f;
        auto sum = 0.0;
        auto count = 0;

        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto distance = std::hypot (static_cast<float> (x) + 0.5f - center, static_cast<float> (y) + 0.5f - center);
                if (distance >= inner && distance < outer)
                {
                    sum += meanLuminance (image, { x, y, 1, 1 });
                    ++count;
                }
            }
        }

        return count > 0 ? sum / count : -1.0;
    }

    /** The luminance of the brightest pixel of @p image. */
    static double brightestLuminance (const Image& image)
    {
        auto brightest = 0.0;
        for (int y = 0; y < image.getHeight(); ++y)
            for (int x = 0; x < image.getWidth(); ++x)
                brightest = jmax (brightest, meanLuminance (image, { x, y, 1, 1 }));

        return brightest;
    }

    /** A dielectric of base color @p base and roughness 1 with the given clearcoat and sheen: the
        base reflects no specular light when @p specularFactor is 0. */
    static Material::Ptr makeLayeredMaterial (std::array<float, 3> base, float clearcoat, float clearcoatRoughness,
                                              std::array<float, 3> sheen, float sheenRoughness, float specularFactor = 1.0f)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { base[0], base[1], base[2], 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 1.0f;
        material->specularFactor = specularFactor;
        material->clearcoatFactor = clearcoat;
        material->clearcoatRoughnessFactor = clearcoatRoughness;
        material->sheenColorFactor = sheen;
        material->sheenRoughnessFactor = sheenRoughness;
        return material;
    }

    /** A sphere of radius @p radius drawn with @p material, lit head-on by a white sun of intensity 3
        alone, rendered on @p tier at 128 x 128 without temporal anti-aliasing. */
    Image renderLitSphere (Material::Ptr material, RenderSettings::TierPreference tier, float radius = 1.5f)
    {
        Scene scene;
        addSphere (scene, radius, std::move (material));
        scene.setAmbientColor (Colors::black);

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = 3.0f;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        const auto image = renderToImage (renderer, scene, 128);
        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        return image;
    }

    /** On @p tier, a clearcoat (KHR_materials_clearcoat) over a rough red base adds the sharp
        highlight of its own smooth lobe: the head-on sun's mirror point at the center of the sphere
        brightens by more than 40 levels of luminance, within a few pixels.
    */
    void checkClearcoatAddsASharpHighlightOnARoughBase (RenderSettings::TierPreference tier)
    {
        const auto matte = renderLitSphere (makeLayeredMaterial ({ 0.8f, 0.05f, 0.05f }, 0.0f, 0.05f, {}, 0.0f), tier);
        const auto coated = renderLitSphere (makeLayeredMaterial ({ 0.8f, 0.05f, 0.05f }, 1.0f, 0.05f, {}, 0.0f), tier);
        ASSERT_TRUE (matte.isValid() && coated.isValid());

        const auto matteBrightest = brightestLuminance (matte);
        const auto coatedBrightest = brightestLuminance (coated);
        RecordProperty ("matteBrightest", String (matteBrightest, 1).toStdString());
        RecordProperty ("coatedBrightest", String (coatedBrightest, 1).toStdString());
        EXPECT_GT (coatedBrightest, matteBrightest + 40.0);

        // Sharp: the pixels the highlight brightens by more than 20 levels lie at the center, within
        // a few pixels, where the rough base spreads over the whole sphere
        auto brightened = 0;
        for (int y = 0; y < 128; ++y)
        {
            for (int x = 0; x < 128; ++x)
            {
                if (meanLuminance (coated, { x, y, 1, 1 }) > meanLuminance (matte, { x, y, 1, 1 }) + 20.0)
                {
                    ++brightened;
                    EXPECT_LT (std::hypot (x + 0.5 - 64.0, y + 0.5 - 64.0), 8.0) << x << ", " << y;
                }
            }
        }

        RecordProperty ("brightenedPixels", brightened);
        EXPECT_GT (brightened, 0);
    }

    /** On @p tier, the clearcoat is widened where its normal varies within a pixel, as the base is
        (specular antialiasing): on a sphere about 4 pixels wide, so curved that the widening
        dominates, a mirror coat of roughness 0.04 peaks about as one of 0.3, while on the large
        sphere, where the widening is negligible, the mirror's peak stands far above.
    */
    void checkClearcoatIsWidenedOverCurvedPixels (RenderSettings::TierPreference tier)
    {
        const auto peak = [&] (float radius, float clearcoatRoughness)
        {
            const auto image = renderLitSphere (makeLayeredMaterial ({ 0.05f, 0.05f, 0.05f }, 1.0f, clearcoatRoughness, {}, 0.0f, 0.0f), tier, radius);
            EXPECT_TRUE (image.isValid());
            return brightestLuminance (image);
        };

        const auto smallMirror = peak (0.15f, 0.04f);
        const auto smallRough = peak (0.15f, 0.3f);
        const auto largeMirror = peak (1.5f, 0.04f);
        const auto largeRough = peak (1.5f, 0.3f);
        RecordProperty ("smallMirror", String (smallMirror, 1).toStdString());
        RecordProperty ("smallRough", String (smallRough, 1).toStdString());
        RecordProperty ("largeMirror", String (largeMirror, 1).toStdString());
        RecordProperty ("largeRough", String (largeRough, 1).toStdString());

        // On Apple M2 Max: 101 against 100 small, 247 against 194 large; without the widening the
        // small mirror peaks at 59, the pixel missing most of its highlight, and the large one at 179
        EXPECT_GT (largeMirror, largeRough + 40.0);
        EXPECT_LT (std::abs (smallMirror - smallRough), 15.0);
    }

    /** On @p tier, a white sheen (KHR_materials_sheen) over a black base without specular light
        brightens the grazing edges of the sphere, lit head-on: its Charlie lobe vanishes where the
        normal faces the view and the light, at the center, and peaks towards the silhouette.
    */
    void checkSheenBrightensGrazingEdges (RenderSettings::TierPreference tier)
    {
        const auto image = renderLitSphere (makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 0.5f, 0.0f), tier);
        ASSERT_TRUE (image.isValid());

        const auto radius = sphereRadiusInPixels (1.5f, 128);
        const auto center = annulusLuminance (image, 0.0f, 0.3f * radius);
        const auto ring = annulusLuminance (image, 0.85f * radius, 0.95f * radius);
        RecordProperty ("center", String (center, 1).toStdString());
        RecordProperty ("ring", String (ring, 1).toStdString());

        EXPECT_GT (ring, center + 20.0);
    }

    /** On @p tier, a white rough sheen over a black base without specular light, facing the view
        under a uniform environment of radiance 4 and no light, reflects the sheen albedo of the
        tables: 4 E (n.v = 1, roughness 1), 0.566. Within 2.5%: the half float table and its read
        0.2 texel off its first column (under 0.3%), and the rounding of the Reinhard-mapped image
        around level 160, up to half an 8-bit step there (about 1.1%). The scene color stays finite
        and unsaturated.
    */
    void checkSheenPassesTheWhiteFurnaceTest (RenderSettings::TierPreference tier)
    {
        Scene scene;
        addPlate (scene, makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 1.0f, 0.0f));
        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (4.0f, 4.0f, 4.0f); }, 32, 3));

        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        std::array<double, 3> radiance {};
        for (int y = 28; y < 36; ++y)
        {
            for (int x = 28; x < 36; ++x)
            {
                const auto color = image.getPixelColor (x, y);
                EXPECT_LT (color.getGreen(), 250) << x << ", " << y;
                radiance[0] += reinhardRadiance (color.getRed()) / 64.0;
                radiance[1] += reinhardRadiance (color.getGreen()) / 64.0;
                radiance[2] += reinhardRadiance (color.getBlue()) / 64.0;
            }
        }

        const auto expected = 4.0 * LookupTables::computeSheenAlbedo()[static_cast<size_t> ((LookupTables::size - 1) * LookupTables::size)];
        RecordProperty ("sheenFurnace", String (radiance[1] / expected, 4).toStdString());

        for (const auto channel : radiance)
            EXPECT_NEAR (channel, expected, 0.025 * expected);

#if YUP_ENABLE_SHADER_TRANSPILER
        // Raw radiance: the full tier's scene color, and the raster rgba16float bloom path
        if (tier != RenderSettings::TierPreference::full)
        {
            setBloom (renderer, true);
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        }

        const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        ASSERT_NE (sceneColor, nullptr);
        EXPECT_TRUE (isFiniteTexture (sceneColor));
        EXPECT_FALSE (isSaturatedTexture (sceneColor));
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
#endif
    }

    /** On @p tier, the frames drawing a material with a clearcoat or a sheen shade with the layers
        variants (preflight R29), local indirect ones that alone compile the layers; frames with
        probes only take the plain local indirect variant, the others the default one. On raster
        the forward pass selects them for any item, on full the lighting pass for its opaque items
        (the full forward pass, for blended items, always compiles the layers).
    */
    void checkLayeredVariantFollowsTheMaterials (RenderSettings::TierPreference tier)
    {
        const auto full = tier == RenderSettings::TierPreference::full;
        auto material = makeLayeredMaterial ({ 0.5f, 0.5f, 0.5f }, 0.0f, 0.0f, {}, 0.0f);

        Scene scene;
        addPlate (scene, material);

        // The variant of the last frame: 0 default, 1 local indirect, 2 local indirect with the layers
        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        const auto variant = [&]
        {
            EXPECT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            if (full)
            {
                const auto* pass = renderer.getLightingPass();
                return pass == nullptr ? -1 : (pass->usesMaterialLayers() ? 2 : (pass->usesLocalIndirect() ? 1 : 0));
            }

            const auto* pass = renderer.getForwardPass();
            return pass == nullptr ? -1 : (pass->usesMaterialLayers() ? 2 : (pass->usesLocalIndirect() ? 1 : 0));
        };

        EXPECT_EQ (variant(), 0);

        material->clearcoatFactor = 0.5f;
        EXPECT_EQ (variant(), 2);

        material->clearcoatFactor = 0.0f;
        material->clearcoatRoughnessFactor = 0.5f;
        material->sheenRoughnessFactor = 0.5f;
        EXPECT_EQ (variant(), 0);

        material->sheenColorFactor = { 0.0f, 0.0f, 0.25f };
        EXPECT_EQ (variant(), 2);

        // A blended layered item: on full the forward pass shades it, the lighting pass stays default
        material->alphaMode = Material::AlphaMode::blend;
        EXPECT_EQ (variant(), full ? 0 : 2);

        // A probe without layered materials takes the local indirect variant without the layers
        material->alphaMode = Material::AlphaMode::opaque;
        material->sheenColorFactor = { 0.0f, 0.0f, 0.0f };
        addReflectionProbe (scene, { 0.0f, 0.0f, 2.0f }, 20.0f);
        EXPECT_EQ (variant(), 1);

        material->clearcoatFactor = 1.0f;
        EXPECT_EQ (variant(), 2);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
    }

    /** On @p tier, a layers variant the device cannot compile is logged once and leaves the frames
        rendering with the plain local indirect variant, without the layers: the clearcoat's
        highlight is gone, and the frame is otherwise the uncoated one.
    */
    void checkFailedLayersVariantShadesWithoutTheLayers (RenderSettings::TierPreference tier)
    {
        const auto render = [this, tier] (float clearcoat, bool fail)
        {
            Scene scene;
            addSphere (scene, 1.5f, makeLayeredMaterial ({ 0.8f, 0.05f, 0.05f }, clearcoat, 0.05f, {}, 0.0f));
            scene.setAmbientColor (Colors::black);

            auto sun = EntityNode::Ptr (new EntityNode ("sun"));
            sun->attach<LightNode>().intensity = 3.0f;
            scene.getRoot()->addChild (sun);

            SceneRenderer renderer;
            setTierForExactFrames (renderer, tier);
            if (fail)
                renderer.failMaterialLayersForTesting();

            auto image = renderToImage (renderer, scene, 128);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();

            const auto full = tier == RenderSettings::TierPreference::full;
            const auto* lightingPass = renderer.getLightingPass();
            const auto* forwardPass = renderer.getForwardPass();
            const auto layers = full ? lightingPass != nullptr && lightingPass->usesMaterialLayers() : forwardPass != nullptr && forwardPass->usesMaterialLayers();
            const auto localIndirect = full ? lightingPass != nullptr && lightingPass->usesLocalIndirect() : forwardPass != nullptr && forwardPass->usesLocalIndirect();
            EXPECT_EQ (layers, clearcoat > 0.0f && ! fail);
            EXPECT_EQ (localIndirect, clearcoat > 0.0f);
            return image;
        };

        const auto uncoated = render (0.0f, false);
        const auto failed = render (1.0f, true);
        const auto coated = render (1.0f, false);
        ASSERT_TRUE (uncoated.isValid() && failed.isValid() && coated.isValid());

        RecordProperty ("failedBrightest", String (brightestLuminance (failed), 1).toStdString());
        RecordProperty ("uncoatedBrightest", String (brightestLuminance (uncoated), 1).toStdString());
        EXPECT_NEAR (brightestLuminance (failed), brightestLuminance (uncoated), 1.0);
        EXPECT_GT (brightestLuminance (coated), brightestLuminance (uncoated) + 40.0); // Not vacuous
    }

    /** The mean linear radiance of the 8x8 pixels at the center of a 64 x 64 Reinhard-mapped image. */
    static std::array<double, 3> centerRadiance (const Image& image)
    {
        std::array<double, 3> radiance {};
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

    /** On @p tier, a white sheen over a white base keeps the energy of a white furnace: under a
        uniform environment of radiance 1 and no light, the base without specular (specularFactor 0)
        reflects 1 - E (n.v) of it once the sheen dims it, and the sheen E (n.v), so the plate reads
        exactly 1, whatever the albedo E of the tables. Within 2.5%: around level 188 an 8-bit step of
        the Reinhard-mapped image is about 2.4%, so its rounding errs by up to 1.2%, and 1 reads
        1.0116. Without the base scaling it reads
        1 + E, 1.14 at roughness 1 seen head-on.
    */
    void checkSheenOverAWhiteBaseKeepsTheEnergy (RenderSettings::TierPreference tier)
    {
        Scene scene;
        addPlate (scene, makeLayeredMaterial ({ 1.0f, 1.0f, 1.0f }, 0.0f, 0.0f, { 1.0f, 1.0f, 1.0f }, 1.0f, 0.0f));
        scene.setAmbientColor (Colors::black);
        scene.setUsingDefaultLight (false);
        scene.setToneMapping (Scene::ToneMapping::reinhard);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (1.0f, 1.0f, 1.0f); }, 32, 3));

        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        const auto image = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();

        const auto radiance = centerRadiance (image);
        RecordProperty ("whiteSheenFurnace", String (radiance[1], 4).toStdString());

        for (const auto channel : radiance)
            EXPECT_NEAR (channel, 1.0, 0.025);
    }

    /** On @p tier, a clearcoat of factor 1 dims the emission under it by 1 - Fc (n.v), Fc the
        Schlick Fresnel of f0 = 0.04: a black emitter without specular, turned 70 degrees away from
        the view, alone in the scene (the clearcoat reflects nothing there), is dimmed to 0.841 of
        its uncoated radiance at the center of the frame. Within 0.025: a few 8-bit steps of the
        Reinhard-mapped image, and the view angle varying by under a degree over the 8x8 pixels.
    */
    void checkClearcoatDimsTheEmission (RenderSettings::TierPreference tier)
    {
        const auto render = [this, tier] (float clearcoat)
        {
            auto material = makeLayeredMaterial ({ 0.0f, 0.0f, 0.0f }, clearcoat, 0.3f, {}, 0.0f, 0.0f);
            material->emissiveFactor = { 0.5f, 0.5f, 0.5f };

            Scene scene;
            addPlate (scene, material)->setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, degreesToRadians (70.0f)));
            scene.setAmbientColor (Colors::black);
            scene.setUsingDefaultLight (false);
            scene.setToneMapping (Scene::ToneMapping::reinhard);

            auto& renderer = sharedRenderer();
            setTierForExactFrames (renderer, tier);
            const auto image = renderToImage (renderer, scene, 64);
            EXPECT_TRUE (image.isValid()) << renderer.getLastError();
            return image.isValid() ? centerRadiance (image) : std::array<double, 3> {};
        };

        const auto bare = render (0.0f);
        const auto coated = render (1.0f);
        ASSERT_GT (bare[1], 0.2);

        const auto nDotV = std::cos (degreesToRadians (70.0));
        const auto expected = 1.0 - (0.04 + 0.96 * std::pow (1.0 - nDotV, 5.0));
        RecordProperty ("emissionUnderClearcoat", String (coated[1] / bare[1], 4).toStdString());

        for (size_t i = 0; i < 3; ++i)
            EXPECT_NEAR (coated[i] / bare[i], expected, 0.025) << i;
    }

    /** On @p tier, a clearcoat reflects a reflection probe and leaves the environment only the share
        the probe leaves: a clearcoated floor whose base reflects nothing (black, no specular), in
        buildMirrorFloorScene() under a red sky, shows the red sky in its coat without a probe. With
        a probe it shows the green emitter above the frustum, which only the probe sees, instead.
    */
    void checkClearcoatReflectsTheProbe (RenderSettings::TierPreference tier)
    {
        Scene scene;
        buildMirrorFloorScene (scene);
        setEmitterColor (scene, { 0.0f, 8.0f, 0.0f });
        auto& floor = *scene.getRoot()->findChild ("floor")->getNode<MeshNode>()->mesh->getPrimitive (0).material;
        floor.baseColorFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        floor.metallicFactor = 0.0f;
        floor.roughnessFactor = 1.0f;
        floor.specularFactor = 0.0f;
        floor.clearcoatFactor = 1.0f;
        floor.clearcoatRoughnessFactor = 0.04f;
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (4.0f, 0.0f, 0.0f); }, 32, 3));
        scene.setAmbientColor (Colors::black);

        SceneRenderer renderer;
        setTierForExactFrames (renderer, tier);
        const auto before = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (before.isValid()) << renderer.getLastError();

        addReflectionProbe (scene, { 0.0f, 1.0f, -1.0f }, 20.0f);
        const auto after = renderToImage (renderer, scene, 64);
        ASSERT_TRUE (after.isValid()) << renderer.getLastError();
        EXPECT_EQ (getNumBakes (renderer), 1);

        const auto floorBefore = meanColor (before, floorRegion());
        const auto floorAfter = meanColor (after, floorRegion());
        RecordProperty ("floorBefore", floorBefore.toString().toStdString());
        RecordProperty ("floorAfter", floorAfter.toString().toStdString());

        EXPECT_GT (floorBefore.getRed(), floorBefore.getGreen() + 20); // The coat reflects the red sky
        EXPECT_GT (floorAfter.getGreen(), floorBefore.getGreen() + 20); // then the emitter through the probe
        EXPECT_LT (floorAfter.getRed(), floorBefore.getRed() - 20);     // and the sky no more where the probe weighs 1
    }

    /** The largest difference of any channel of any pixel between two images, in 8 bit levels. */
    static int maxChannelDifference (const Image& image, const Image& reference)
    {
        int difference = 0;
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto a = image.getPixelColor (x, y);
                const auto b = reference.getPixelColor (x, y);
                difference = std::max ({ difference, std::abs (a.getRed() - b.getRed()), std::abs (a.getGreen() - b.getGreen()), std::abs (a.getBlue() - b.getBlue()) });
            }
        }

        return difference;
    }

    //==============================================================================
    /** The size of the depth of field scenes, rendered with the camera at the origin looking down -Z
        (yFov 0.8: 15.1 px per unit at a distance of 10, a focal length of 28.4 mm). The circles of
        confusion of each scene are checked on the CPU first, through CameraNode::getCircleOfConfusion(),
        so the thresholds stand on known blur sizes.
    */
    static constexpr int depthOfFieldSize = 128;

    /** Selects @p tier with depth of field on, the full tier accumulating @p taaSamples. */
    static void setDepthOfFieldTier (SceneRenderer& renderer, RenderSettings::TierPreference tier, int taaSamples = 1)
    {
        if (tier == RenderSettings::TierPreference::full)
            setFullTierTaa (renderer, taaSamples);
        else
            setTier (renderer, tier);

        setDepthOfField (renderer, true);
    }

    /** A black background and ambient light, the camera at the origin looking down -Z at f/1.4,
        focused at @p focusDistance. Render it with setDepthOfFieldTier().
    */
    static CameraNode& setUpDepthOfFieldScene (Scene& scene, float focusDistance)
    {
        scene.setBackgroundColor (Colors::black);
        scene.setAmbientColor (Colors::black);
        addCamera (scene, {}, { 0.0f, 0.0f, -1.0f });

        auto& camera = *scene.getActiveCamera();
        camera.fStop = 1.4f;
        camera.focusDistance = focusDistance;
        return camera;
    }

    static void setDepthOfField (SceneRenderer& renderer, bool depthOfField, float maxSize = 32.0f)
    {
        auto settings = renderer.getRenderSettings();
        settings.depthOfField = depthOfField;
        settings.depthOfFieldMaxSize = maxSize;
        renderer.setRenderSettings (settings);
    }

    /** An emissive rectangle facing the camera at @p distance down -Z, @p area in world units,
        blended with @p alpha below 1.
    */
    static void addEmitter (Scene& scene, Rectangle<float> area, float distance, float emission, float alpha = 1.0f)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { area.getX(), area.getY(), -distance };
        vertices[1].position = { area.getRight(), area.getY(), -distance };
        vertices[2].position = { area.getRight(), area.getBottom(), -distance };
        vertices[3].position = { area.getX(), area.getBottom(), -distance };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("emitter"));
        auto material = makeEmitterMaterial ({ emission, emission, emission });
        if (alpha < 1.0f)
        {
            material->alphaMode = Material::AlphaMode::blend;
            material->baseColorFactor[3] = alpha;
        }

        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode ("emitter"));
        entity->attach<MeshNode> (mesh);
        scene.getRoot()->addChild (entity);
    }

    /** Three vertical white emissive bars at @p barDistance, 0.15 of the distance wide with gaps as
        wide (23 px at 128 x 128), the middle one centered: its left edge is at x = 52.7 px. With an
        @p occluderDistance, a brighter square in front, 0.2 of that distance wide (30 px), centered.
    */
    static void buildBarsScene (Scene& scene, float barDistance, float occluderDistance = 0.0f)
    {
        const auto width = 0.15f * barDistance;
        for (int bar = -1; bar <= 1; ++bar)
            addEmitter (scene, { (2.0f * static_cast<float> (bar) - 0.5f) * width, -barDistance, width, 2.0f * barDistance }, barDistance, 2.0f);

        if (occluderDistance > 0.0f)
            addEmitter (scene, { -0.1f * occluderDistance, -0.1f * occluderDistance, 0.2f * occluderDistance, 0.2f * occluderDistance }, occluderDistance, 8.0f);
    }

    /** The pixels between 10% and 90% of the luminance range along the row segment [xFrom, xTo). */
    static int edgeWidth (const Image& image, int row, int xFrom, int xTo)
    {
        std::vector<double> luma;
        for (int x = xFrom; x < xTo; ++x)
            luma.push_back (meanLuminance (image, { x, row, 1, 1 }));

        const auto [low, high] = std::minmax_element (luma.begin(), luma.end());
        const auto range = *high - *low;

        return static_cast<int> (std::count_if (luma.begin(), luma.end(), [&] (double l)
        {
            return l > *low + 0.1 * range && l < *low + 0.9 * range;
        }));
    }

    /** The mean luma of the ring from 2 to 5 px around the occluder of buildBarsScene(), 30 px wide at the center. */
    static double occluderHaloLuminance (const Image& image)
    {
        const auto occluder = Rectangle<int> (49, 49, 30, 30);
        const auto outer = occluder.enlarged (5);
        const auto inner = occluder.enlarged (2);
        const auto outerArea = static_cast<double> (outer.getWidth() * outer.getHeight());
        const auto innerArea = static_cast<double> (inner.getWidth() * inner.getHeight());

        return (meanLuminance (image, outer) * outerArea - meanLuminance (image, inner) * innerArea) / (outerArea - innerArea);
    }

    /** The CoC diameter in pixels of the depth of field frame of a point at @p distance. */
    static float circleOfConfusion (const Scene& scene, float distance)
    {
        return scene.getActiveCamera()->getCircleOfConfusion (distance, static_cast<float> (depthOfFieldSize));
    }

    /** True when @p feature can run on @p tier on this device, at the default sample count. */
    bool isFeatureAvailable (RenderCapabilities::Feature feature, RenderSettings::TierPreference tier) const
    {
        RenderSettings settings;
        settings.tier = tier;
        return RenderCapabilities (RenderCapabilities::queryDevice (*device), settings, 4).isAvailable (feature);
    }

    /** On @p tier, bars in focus stay as sharp as without depth of field, and blur out of focus. The
        scene color, before the blur, stays sharp.
    */
    void checkDepthOfFieldBlursOutOfFocusBarsOnly (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        auto& camera = setUpDepthOfFieldScene (scene, 10.0f);
        buildBarsScene (scene, 10.0f);

        SceneRenderer renderer;
        setDepthOfFieldTier (renderer, tier);
        const auto sharp = renderToImage (renderer, scene, size);
        ASSERT_TRUE (sharp.isValid()) << renderer.getLastError();

        // Focused on the bars, nothing blurs by a texel: the frame is the one without depth of field
        setDepthOfField (renderer, false);
        const auto off = renderToImage (renderer, scene, size);
        ASSERT_TRUE (off.isValid()) << renderer.getLastError();
        RecordProperty ("focusedDifference", maxChannelDifference (sharp, off));
        EXPECT_EQ (countDifferences (sharp, off), 0);

        setDepthOfField (renderer, true);
        camera.focusDistance = 0.2f;
        ASSERT_GT (circleOfConfusion (scene, 10.0f), 16.0f);

        const auto blurred = renderToImage (renderer, scene, size);
        ASSERT_TRUE (blurred.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);

        // The left edge of the middle bar, at x = 52.7: a step in focus, a ramp out of focus. The scene
        // color of the frame, the output of TAA on full and the resolved color on raster, stays sharp: the blur runs after it
        const auto sharpWidth = edgeWidth (sharp, size / 2, 40, 66);
        const auto blurredWidth = edgeWidth (blurred, size / 2, 40, 66);
        EXPECT_LE (edgeWidth (readBack (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)), size / 2, 40, 66), 2);
        RecordProperty ("sharpEdgeWidth", sharpWidth);
        RecordProperty ("blurredEdgeWidth", blurredWidth);

        EXPECT_LE (sharpWidth, 2);
        EXPECT_GE (blurredWidth, 6);
    }

    /** On @p tier, bars in focus look the same with depth of field on and off, over the background
        color and over a visible black environment: the composite blurs nothing there, so tone
        mapping keeps the blend of the frames without it, their partly covered antialiased edges
        included (preflight R21). The full tier accumulates 8 jittered frames, which leave the edges
        partly covered, and converges first. Over the environment, the raster reference renders in
        half float too (bloom at intensity 0): the resolve of that path alone moves the edges against
        the default frame.
    */
    void checkInFocusEdgesMatchWithoutDepthOfField (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;
        const auto full = tier == RenderSettings::TierPreference::full;

        for (const auto overEnvironment : { false, true })
        {
            const auto name = overEnvironment ? "over the environment" : "over the background color";

            Scene scene;
            setUpDepthOfFieldScene (scene, 10.0f);
            buildBarsScene (scene, 10.0f);
            if (overEnvironment)
            {
                scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
                scene.setEnvironmentVisible (true);
            }

            SceneRenderer renderer;
            setDepthOfFieldTier (renderer, tier, 8);
            const auto on = full ? renderConverged (renderer, scene, size) : renderToImage (renderer, scene, size);
            ASSERT_TRUE (on.isValid()) << name << ": " << renderer.getLastError();
            ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr) << name;

            auto settings = renderer.getRenderSettings();
            settings.depthOfField = false;
            settings.bloom = overEnvironment && ! full;
            settings.bloomIntensity = 0.0f;
            renderer.setRenderSettings (settings);

            const auto off = renderToImage (renderer, scene, size);
            ASSERT_TRUE (off.isValid()) << name << ": " << renderer.getLastError();
            EXPECT_TRUE (! full || renderer.isConverged()) << name;

            // Not vacuous: the edges are partly covered
            const auto interior = static_cast<int> (off.getPixelColor (size / 2, size / 2).getGreen());
            int partial = 0;
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                    partial += off.getPixelColor (x, y).getGreen() > 2 && off.getPixelColor (x, y).getGreen() < interior - 2 ? 1 : 0;

            RecordProperty (String (name).replace (" ", "_").toStdString() + "_partialPixels", partial);
            RecordProperty (String (name).replace (" ", "_").toStdString() + "_largestDifference", maxChannelDifference (on, off));
            EXPECT_GT (partial, size) << name;
            EXPECT_EQ (countDifferences (on, off), 0) << name;
        }
    }

    /** Raster depth of field that gave up stops paying for its route: the frames after it no longer
        draw the depth and normal prepass nor render in half float.
    */
    void checkRasterDepthOfFieldThatGaveUpStopsItsRoute()
    {
        Scene scene;
        setUpDepthOfFieldScene (scene, 0.2f);
        buildBarsScene (scene, 10.0f);

        SceneRenderer renderer;
        setDepthOfFieldTier (renderer, RenderSettings::TierPreference::raster);
        renderer.failDepthOfFieldForTesting (2);

        for (int frame = 0; frame < 3; ++frame)
        {
            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());

            // The two failing frames try, so they draw the route; the third finds it given up
            const auto tries = frame < 2;
            ASSERT_NE (renderer.getDepthOfField(), nullptr) << frame;
            EXPECT_EQ (renderer.getDepthOfField()->hasGivenUp(), frame >= 1) << frame;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal) != nullptr, tries) << frame;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat() == GpuTextureFormat::rgba16float, tries) << frame;
        }
    }

    /** On @p tier, a near occluder out of focus spreads beyond its silhouette onto the sharp
        background. On full, with temporal anti-aliasing, so the blur runs after it as in the default
        chain, and the same renderer gives the image without it.
    */
    void checkNearFieldBleedsOverTheFocusedBackground (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        setUpDepthOfFieldScene (scene, 10.0f);
        buildBarsScene (scene, 10.0f, 0.3f);
        ASSERT_LT (circleOfConfusion (scene, 0.3f), -8.0f);

        SceneRenderer renderer;
        setDepthOfFieldTier (renderer, tier, 16);
        const auto image = renderToImage (renderer, scene, size);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);

        setDepthOfField (renderer, false);
        const auto without = renderToImage (renderer, scene, size);
        ASSERT_TRUE (without.isValid()) << renderer.getLastError();

        const auto halo = occluderHaloLuminance (image);
        const auto haloWithout = occluderHaloLuminance (without);
        RecordProperty ("halo", String (halo, 2).toStdString());
        RecordProperty ("haloWithout", String (haloWithout, 2).toStdString());

        EXPECT_GT (halo, haloWithout + 10.0);

        // The halo over the background color is spread light, composited in radiance as over a
        // visible black environment (preflight R20, R21), not dimmed by its partial coverage
        setDepthOfField (renderer, true);
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
        scene.setEnvironmentVisible (true);
        const auto overEnvironment = renderToImage (renderer, scene, size);
        ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

        const auto haloOverEnvironment = occluderHaloLuminance (overEnvironment);
        RecordProperty ("haloOverEnvironment", String (haloOverEnvironment, 2).toStdString());
        EXPECT_NEAR (halo, haloOverEnvironment, 0.05 * haloOverEnvironment);
    }

    /** On @p tier, with depth of field off, moving the focus changes nothing. */
    void checkDepthOfFieldOffChangesNothing (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        auto& camera = setUpDepthOfFieldScene (scene, 10.0f);
        buildBarsScene (scene, 10.0f, 0.3f);

        SceneRenderer renderer;
        setDepthOfFieldTier (renderer, tier);
        setDepthOfField (renderer, false);
        const auto focusedFar = renderToImage (renderer, scene, size);
        ASSERT_TRUE (focusedFar.isValid()) << renderer.getLastError();

        camera.focusDistance = 1.0f;
        const auto focusedNear = renderToImage (renderer, scene, size);
        ASSERT_TRUE (focusedNear.isValid()) << renderer.getLastError();

        EXPECT_LE (maxChannelDifference (focusedFar, focusedNear), 1);
        EXPECT_EQ (renderer.getDepthOfField(), nullptr);
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
    }

    /** On @p tier, a white square out of focus over a red background color, without environment:
        across its edge the blur fades white into red through pink, never through a darker ring (R5).
    */
    void checkDepthOfFieldKeepsTheSilhouetteBlendOverTheBackground (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        setUpDepthOfFieldScene (scene, 0.2f);
        scene.setBackgroundColor (Colors::red);
        addEmitter (scene, { -1.5f, -1.5f, 3.0f, 3.0f }, 10.0f, 8.0f);

        auto& renderer = sharedRenderer();
        setDepthOfFieldTier (renderer, tier);
        const auto image = renderToImage (renderer, scene, size);
        ASSERT_TRUE (image.isValid()) << renderer.getLastError();
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);

        // The square spans x 41.4 to 86.6: across its left edge along the middle row
        int blended = 0;
        int darkest = 255;
        for (int x = 20; x < 64; ++x)
        {
            const auto color = image.getPixelColor (x, size / 2);
            darkest = jmin (darkest, static_cast<int> (color.getRed()));
            blended += color.getGreen() > 25 && color.getGreen() < 230 ? 1 : 0;
        }

        RecordProperty ("blendedPixels", blended);
        RecordProperty ("darkestRed", darkest);

        EXPECT_GE (blended, 6);
        EXPECT_GE (darkest, 230);
    }

    /** On @p tier, a bright square far out of focus over the black background color, without an
        environment, against the same square over a visible black environment: the blur spreads the
        same light over both, so the bokeh is as bright, not capped by its coverage (preflight R20).
    */
    void checkBlurredHighlightsKeepTheirBrightnessOverTheBackground (RenderSettings::TierPreference tier)
    {
        constexpr auto size = depthOfFieldSize;

        Scene scene;
        auto& camera = setUpDepthOfFieldScene (scene, 0.1f);
        camera.fStop = 0.7f;
        addEmitter (scene, { -0.2f, -0.2f, 0.4f, 0.4f }, 10.0f, 200.0f);

        SceneRenderer renderer;
        setDepthOfFieldTier (renderer, tier);
        setDepthOfField (renderer, true, 64.0f);
        const auto overBackground = renderToImage (renderer, scene, size);
        ASSERT_TRUE (overBackground.isValid()) << renderer.getLastError();

        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float>(); }, 32, 3));
        scene.setEnvironmentVisible (true);
        const auto overEnvironment = renderToImage (renderer, scene, size);
        ASSERT_TRUE (overEnvironment.isValid()) << renderer.getLastError();

        const auto background = meanLuminance (overBackground, { 0, 0, size, size });
        const auto environment = meanLuminance (overEnvironment, { 0, 0, size, size });
        RecordProperty ("overBackground", String (background, 2).toStdString());
        RecordProperty ("overEnvironment", String (environment, 2).toStdString());

        ASSERT_GT (environment, 20.0);
        // Measured 54.43 against 54.52 on full: both tone map the same spread radiance over black. The
        // display space blend gave 0.4, as each pixel kept only its coverage share of the tone mapped highlight
        EXPECT_NEAR (background, environment, 0.05 * environment);
    }

    /** Raster depth of field alone draws the depth and normal prepass for its depth, but creates no
        ambient occlusion pass, and the forward pass shades with its default variant, not the local
        indirect one ambient occlusion selects. With ambient occlusion on too, both run.
    */
    void checkRasterDepthOfFieldDrawsThePrepassWithoutAmbientOcclusion()
    {
        Scene scene;
        setUpDepthOfFieldScene (scene, 0.2f);
        buildBarsScene (scene, 10.0f);

        SceneRenderer renderer;
        setDepthOfFieldTier (renderer, RenderSettings::TierPreference::raster);
        const auto render = [&] (bool ambientOcclusion)
        {
            auto settings = renderer.getRenderSettings();
            settings.ambientOcclusion = ambientOcclusion;
            renderer.setRenderSettings (settings);

            ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
            EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float);
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr, ambientOcclusion);
            ASSERT_NE (renderer.getForwardPass(), nullptr);
            EXPECT_EQ (renderer.getForwardPass()->usesLocalIndirect(), ambientOcclusion);
        };

        render (false);
        EXPECT_EQ (renderer.getAmbientOcclusionPass(), nullptr);

        // Not vacuous: ambient occlusion takes the variant reading it
        render (true);
        EXPECT_NE (renderer.getAmbientOcclusionPass(), nullptr);
    }

    /** Raster depth of field where the device cannot run it, without the prepass or with too few
        multisampled half-float samples: the frames are the ones without depth of field, with no
        prepass, no HDR scene color and no depth of field object.
    */
    void checkRasterDepthOfFieldUnavailableRecordsNothing()
    {
        Scene scene;
        setUpDepthOfFieldScene (scene, 0.2f);
        buildBarsScene (scene, 10.0f);

        SceneRenderer reference;
        setDepthOfFieldTier (reference, RenderSettings::TierPreference::raster);
        setDepthOfField (reference, false);
        const auto off = renderToImage (reference, scene, 64);
        ASSERT_TRUE (off.isValid()) << reference.getLastError();

        const std::pair<const char*, std::function<void (RenderCapabilities::DeviceFacts&)>> devices[] = {
            { "no prepass", [] (RenderCapabilities::DeviceFacts& facts) { facts.depthNormalPrepass = false; } },
            { "one half float sample", [] (RenderCapabilities::DeviceFacts& facts) { facts.rgba16floatSamples = 1; } },
        };

        for (const auto& [name, edit] : devices)
        {
            SceneRenderer renderer;
            setDepthOfFieldTier (renderer, RenderSettings::TierPreference::raster);
            renderer.setDeviceFactsOverride (edit);

            for (int frame = 0; frame < 2; ++frame)
            {
                const auto image = renderToImage (renderer, scene, 64);
                ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();
                EXPECT_LE (maxChannelDifference (image, off), 1) << name;
            }

            EXPECT_FALSE (renderer.getCapabilities()->isAvailable (RenderCapabilities::Feature::depthOfField)) << name;
            EXPECT_EQ (renderer.getDepthOfField(), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
            EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float) << name;
        }
    }

    /** Raster depth of field skips the frames whose prepass gave no depth, without counting them as
        failures: once prepass targets that could not be created come back, so does the blur. A
        prepass that cannot compile takes the depth of field off the route: its frames create no
        depth of field and, without bloom, keep the inline frame.
    */
    void checkRasterDepthOfFieldSkipsFramesWithoutThePrepassDepth()
    {
        Scene scene;
        setUpDepthOfFieldScene (scene, 0.2f);
        buildBarsScene (scene, 10.0f);

        for (const auto bloom : { false, true })
        {
            const auto name = bloom ? "prepass not compiled, bloom" : "prepass not compiled";

            SceneRenderer renderer;
            setDepthOfFieldTier (renderer, RenderSettings::TierPreference::raster);
            setBloom (renderer, bloom);
            renderer.failDepthNormalPrepassForTesting();

            for (int frame = 0; frame < 3; ++frame)
            {
                ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << name << ": " << renderer.getLastError();
                EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
                EXPECT_EQ (renderer.getDepthOfField(), nullptr) << name;

                const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
                ASSERT_NE (sceneColor, nullptr) << name;
                EXPECT_EQ (sceneColor->getFormat() == GpuTextureFormat::rgba16float, bloom) << name;
            }
        }

        // More frames than the two failures in a row that give up
        const auto expectSkipped = [&] (SceneRenderer& renderer, const char* name)
        {
            for (int frame = 0; frame < 3; ++frame)
            {
                ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << name << ": " << renderer.getLastError();
                EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr) << name;
                ASSERT_NE (renderer.getDepthOfField(), nullptr) << name;
                EXPECT_FALSE (renderer.getDepthOfField()->hasGivenUp()) << name;
                EXPECT_EQ (renderer.getDepthOfField()->getNumCachedTargets(), 0u) << name;
            }
        };

        SceneRenderer renderer;
        setDepthOfFieldTier (renderer, RenderSettings::TierPreference::raster);
        setBloom (renderer, false);
        renderer.failDepthNormalPrepassTargetsForTesting (3);
        expectSkipped (renderer, "prepass without targets");

        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);
    }

    //==============================================================================
    /** The size of the motion blur scenes, and the distance of their bar from the camera at the
        origin looking down -Z (yFov 0.8: 75.7 px per unit there at 128 x 128).
    */
    static constexpr int motionBlurSize = 128;
    static constexpr float motionBarDistance = 2.0f;

    /** An emissive quad facing the camera of buildMovingBarScene(), @p area in world units around
        the center of its entity, which sits motionBarDistance down the view axis, blended with
        @p alpha below 1. Returns the entity, which tests move with moveBar().
    */
    static EntityNode* addMotionQuad (Scene& scene, Rectangle<float> area, float emission, float alpha = 1.0f)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { area.getX(), area.getY(), 0.0f };
        vertices[1].position = { area.getRight(), area.getY(), 0.0f };
        vertices[2].position = { area.getRight(), area.getBottom(), 0.0f };
        vertices[3].position = { area.getX(), area.getBottom(), 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto material = makeEmitterMaterial ({ emission, emission, emission });
        if (alpha < 1.0f)
        {
            material->alphaMode = Material::AlphaMode::blend;
            material->baseColorFactor[3] = alpha;
        }

        auto mesh = Mesh::Ptr (new Mesh ("quad"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, std::move (material));

        auto entity = EntityNode::Ptr (new EntityNode ("quad"));
        entity->attach<MeshNode> (mesh);
        entity->setPosition ({ 0.0f, 0.0f, -motionBarDistance });
        scene.getRoot()->addChild (entity);
        return entity.get();
    }

    /** One white emissive bar, 0.2 x 1.0 units (about 15 x 76 px at 128 x 128), at the center of
        the view of a camera at the origin looking down -Z, over the black background color, with
        black ambient light. Returns the bar, which tests move with moveBar().
    */
    static EntityNode* buildMovingBarScene (Scene& scene, float emission = 2.0f)
    {
        scene.setBackgroundColor (Colors::black);
        scene.setAmbientColor (Colors::black);
        addCamera (scene, {}, { 0.0f, 0.0f, -1.0f });
        return addMotionQuad (scene, { -0.1f, -0.5f, 0.2f, 1.0f }, emission);
    }

    /** Moves an entity of addMotionQuad() to @p x units right of the view axis. */
    static void moveBar (EntityNode& bar, float x)
    {
        bar.setPosition ({ x, 0.0f, -motionBarDistance });
    }

    /** The pixels per unit at motionBarDistance in the view of buildMovingBarScene(). */
    static float motionPixelsPerUnit (const Scene& scene)
    {
        return static_cast<float> (motionBlurSize) / (2.0f * motionBarDistance * std::tan (scene.getActiveCamera()->yFov * 0.5f));
    }

    static void setMotionBlur (SceneRenderer& renderer, bool motionBlur, float shutter = 1.0f)
    {
        auto settings = renderer.getRenderSettings();
        settings.motionBlur = motionBlur;
        settings.motionBlurShutter = shutter;
        renderer.setRenderSettings (settings);
    }

    /** Motion blur on the raster tier, which has no screen-space motion: after a move, the frame
        is the one without it, without an error and without a motion blur object, on the default
        route and on the half float route of bloom, where the post chain runs.
    */
    void checkMotionBlurOnRasterRendersTheUnblurredFallback()
    {
        for (const auto bloom : { false, true })
        {
            const auto name = bloom ? "bloom" : "default";

            Scene scene;
            auto* bar = buildMovingBarScene (scene);
            SceneRenderer renderer;
            auto settings = renderer.getRenderSettings();
            settings.tier = RenderSettings::TierPreference::raster;
            settings.bloom = bloom;
            renderer.setRenderSettings (settings);
            setMotionBlur (renderer, true);
            ASSERT_TRUE (renderToImage (renderer, scene, motionBlurSize).isValid()) << name << ": " << renderer.getLastError();

            moveBar (*bar, 0.1f);
            const auto on = renderToImage (renderer, scene, motionBlurSize);
            ASSERT_TRUE (on.isValid()) << name << ": " << renderer.getLastError();
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << name;
            EXPECT_FALSE (renderer.getCapabilities()->isAvailable (RenderCapabilities::Feature::motionBlur)) << name;
            EXPECT_FALSE (renderer.getCapabilities()->isEnabled (RenderCapabilities::Feature::motionBlur, renderer.getRenderSettings())) << name;
            EXPECT_EQ (renderer.getMotionBlur(), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::motionBlur), nullptr) << name;

            setMotionBlur (renderer, false);
            const auto off = renderToImage (renderer, scene, motionBlurSize);
            ASSERT_TRUE (off.isValid()) << name << ": " << renderer.getLastError();
            EXPECT_LE (maxChannelDifference (on, off), 1) << name;
        }
    }

    //==============================================================================
    /** The size of the world fog scenes. */
    static constexpr int fogSize = 64;

    /** Enabled white fog of @p density, thinning upwards from y = 0 by @p heightFalloff. */
    static WorldFog makeFog (float density, float heightFalloff = 0.0f)
    {
        WorldFog fog;
        fog.enabled = true;
        fog.density = density;
        fog.heightFalloff = heightFalloff;
        return fog;
    }

    /** Selects @p tier for the fog scenes: the full tier without TAA, so one frame is final. */
    static void setFogTier (SceneRenderer& renderer, RenderSettings::TierPreference tier)
    {
        if (tier == RenderSettings::TierPreference::full)
            setFullTierTaa (renderer, 1);
        else
            setTier (renderer, tier);
    }

    /** A camera at @p eye looking towards @p target (yFov 0.8) over a black background color,
        with black ambient light and no default light: only what a test adds lights the fog.
    */
    static CameraNode& addFogCamera (Scene& scene, Vector3<float> target = { 0.0f, 0.0f, -1.0f }, Vector3<float> eye = {})
    {
        scene.setUsingDefaultLight (false);
        scene.setBackgroundColor (Colors::black);
        scene.setAmbientColor (Colors::black);
        addCamera (scene, eye, target);
        return *scene.getActiveCamera();
    }

    /** The ambient color that is 0.2 once linear, lighting the fog of the tests. */
    static Color fogAmbientColor() { return Color (0xff7c7c7c); }

    /** The linear radiance of fogAmbientColor(), as LightingInputs converts it. */
    static float fogAmbientRadiance()
    {
        const auto encoded = 0x7c / 255.0f;
        return std::pow ((encoded + 0.055f) / 1.055f, 2.4f);
    }

    /** Two white emissive walls facing a camera at the origin looking down -Z (addFogCamera()),
        across the middle half of the rows: a near one 5 units away on the left half of the view and a
        far one 20 units away on the right half, under an ambient light of 0.2 (linear), no other light
        and a black background color. With @p withFog, uniform white fog of density 0.08.
    */
    static void buildFogWallsScene (Scene& scene, bool withFog)
    {
        addFogCamera (scene);
        scene.setAmbientColor (fogAmbientColor());
        addEmitter (scene, { -3.0f, -1.0f, 3.0f, 2.0f }, 5.0f, 1.0f);
        addEmitter (scene, { 0.0f, -4.0f, 12.0f, 8.0f }, 20.0f, 1.0f);

        if (withFog)
            scene.setWorldFog (makeFog (0.08f));
    }

    /** The pixels of the near wall, the far wall and the background of buildFogWallsScene() at fogSize. */
    static Rectangle<int> fogNearWallArea() { return { 6, 26, 20, 12 }; }
    static Rectangle<int> fogFarWallArea() { return { 38, 26, 20, 12 }; }
    static Rectangle<int> fogBackgroundArea() { return { 4, 2, 56, 10 }; }

    /** The unit world direction through the center of pixel (@p x, @p y) of a square frame of @p size
        pixels, row 0 at the top, seen by the perspective @p camera.
    */
    static Vector3<float> pixelDirection (const CameraNode& camera, int x, int y, int size)
    {
        const auto scale = std::tan (camera.yFov * 0.5f);
        const auto ndcX = 2.0f * (static_cast<float> (x) + 0.5f) / static_cast<float> (size) - 1.0f;
        const auto ndcY = 1.0f - 2.0f * (static_cast<float> (y) + 0.5f) / static_cast<float> (size);
        return camera.getEntity()->getWorldMatrix().transformVector ({ ndcX * scale, ndcY * scale, -1.0f }).normalized();
    }

    /** One channel ("r", "g", "b" or "a") of every texel of a float texture, clamped to [0, 1) and
        read back to about 1.5e-5. The read back through a canvas can be a level off, so the value is
        written as two base-16 digits, each in the middle of its 16 levels (red, green), then the rest
        in blue: only the rest takes that level, 1 / 255 of 1 / 256. Indexed y * width + x, row 0 at the top.
    */
    std::vector<float> readUnitChannel (const GpuTexture::Ptr& texture, const char* channel)
    {
        const auto image = mapTexels (texture, "encodeUnit(t." + String (channel) + ")", R"(
vec4 encodeUnit(float value)
{
    float scaled = clamp(value, 0.0, 0.99999) * 16.0;
    float first = floor(scaled);
    float rest = fract(scaled) * 16.0;
    float second = floor(rest);
    return vec4((first * 16.0 + 8.0) / 255.0, (second * 16.0 + 8.0) / 255.0, fract(rest), 1.0);
})");

        std::vector<float> values;
        for (int y = 0; image.isValid() && y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto pixel = image.getPixelColor (x, y);
                const auto first = static_cast<float> (pixel.getRed() / 16);
                const auto second = static_cast<float> (pixel.getGreen() / 16);
                values.push_back ((first + (second + static_cast<float> (pixel.getBlue()) / 255.0f) / 16.0f) / 16.0f);
            }
        }

        return values;
    }

    /** Renders one frame of @p scene at fogSize and returns its scene color: on full the HDR color
        before the post effects, on raster the fogged linear color of a frame drawing the fog.
    */
    GpuTexture::Ptr renderFogSceneColor (SceneRenderer& renderer, Scene& scene)
    {
        const auto texture = renderer.render (device, scene, fogSize, fogSize);
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        EXPECT_NE (texture, nullptr) << renderer.getLastError();

        auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
        EXPECT_TRUE (sceneColor != nullptr && sceneColor->getFormat() == GpuTextureFormat::rgba16float);
        return sceneColor;
    }

    /** On @p tier, the fog dims the far wall of buildFogWallsScene() more than the near one, and
        brightens the background with the ambient light it scatters.
    */
    void checkFogAttenuatesWithDistance (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (bool withFog)
        {
            Scene scene;
            buildFogWallsScene (scene, withFog);

            SceneRenderer renderer;
            setFogTier (renderer, tier);
            auto image = renderToImage (renderer, scene, fogSize);
            EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
            return image;
        };

        const auto clear = render (false);
        const auto foggy = render (true);
        ASSERT_TRUE (clear.isValid() && foggy.isValid());

        const auto clearNear = meanLuminance (clear, fogNearWallArea());
        const auto clearFar = meanLuminance (clear, fogFarWallArea());
        const auto foggyNear = meanLuminance (foggy, fogNearWallArea());
        const auto foggyFar = meanLuminance (foggy, fogFarWallArea());
        const auto nearLoss = (clearNear - foggyNear) / clearNear;
        const auto farLoss = (clearFar - foggyFar) / clearFar;

        RecordProperty ("nearLoss", String (nearLoss, 3).toStdString());
        RecordProperty ("farLoss", String (farLoss, 3).toStdString());
        RecordProperty ("background", String (meanLuminance (foggy, fogBackgroundArea()), 1).toStdString());

        EXPECT_GT (foggyNear, 20.0); // Not a black frame
        EXPECT_GT (farLoss, nearLoss + 0.1);
        EXPECT_GT (meanLuminance (foggy, fogBackgroundArea()), meanLuminance (clear, fogBackgroundArea()) + 20.0);
    }

    /** On @p tier, the transmittance the fog leaves at pixels of three white walls 2, 6 and 12 units
        away matches exp (-WorldFog::getOpticalDepth()) from the camera to the wall, under black fog (it
        scatters no light, so each pixel keeps T of its emission of 0.5):
        - thinning upwards from y = -1, the camera at the origin;
        - with the camera far below the base (y -22, falloff 2), where the density exponent passes the
          clamp at 40: the rows level with the camera stay past it, the top rows of the far wall cross it.
    */
    void checkFogTransmittanceMatchesTheCpu (RenderSettings::TierPreference tier)
    {
        const std::array<float, 3> distances { 2.0f, 6.0f, 12.0f };

        auto thinning = makeFog (0.12f, 0.4f);
        thinning.baseHeight = -1.0f;

        auto belowTheClamp = makeFog (1.0e-18f, 2.0f);

        const struct
        {
            const char* name;
            Vector3<float> eye;
            WorldFog fog;
        } cases[] = {
            { "thinning", {}, thinning },
            { "below the clamp", { 0.0f, -22.0f, 0.0f }, belowTheClamp },
        };

        for (auto [name, eye, fog] : cases)
        {
            fog.color = { 0.0f, 0.0f, 0.0f };

            Scene scene;
            auto& camera = addFogCamera (scene, eye + Vector3<float> (0.0f, 0.0f, -1.0f), eye);

            // Each wall fills a third of the columns, and every row
            for (size_t wall = 0; wall < distances.size(); ++wall)
            {
                const auto halfSize = distances[wall] * std::tan (camera.yFov * 0.5f);
                const auto left = (-1.0f + 2.0f * static_cast<float> (wall) / 3.0f) * halfSize;
                addEmitter (scene, { left, eye.getY() - 1.2f * halfSize, 2.0f / 3.0f * halfSize, 2.4f * halfSize }, distances[wall], 0.5f);
            }

            // The reference takes the same route with fog too faint to change anything
            const auto read = [&] (const WorldFog& sceneFog)
            {
                scene.setWorldFog (sceneFog);
                SceneRenderer renderer;
                setFogTier (renderer, tier);
                const auto sceneColor = renderFogSceneColor (renderer, scene);
                return sceneColor != nullptr ? readUnitChannel (sceneColor, "r") : std::vector<float>();
            };

            auto faint = fog;
            faint.density *= 1.0e-9f;
            const auto reference = read (faint);
            const auto fogged = read (fog);
            ASSERT_EQ (reference.size(), static_cast<size_t> (fogSize * fogSize)) << name;
            ASSERT_EQ (fogged.size(), reference.size()) << name;

            auto lowest = 1.0f, highest = 0.0f, largestError = 0.0f;
            int crossing = 0, pastTheClamp = 0;
            for (size_t wall = 0; wall < distances.size(); ++wall)
            {
                const auto x = static_cast<int> ((2 * wall + 1) * fogSize / 6);
                for (const auto y : { 6, 32, 58 })
                {
                    const auto direction = pixelDirection (camera, x, y, fogSize);
                    const auto point = eye + direction * (distances[wall] / -direction.getZ());
                    const auto expected = std::exp (-fog.getOpticalDepth (eye, point));
                    const auto index = static_cast<size_t> (y * fogSize + x);

                    // The reference is the emission itself, linear on raster too: the reversible map was inverted
                    EXPECT_NEAR (reference[index], 0.5f, 2.0e-3f) << name << ", wall " << wall << ", row " << y;

                    // Absolute on T. Half floats round both colors by half an ulp, at most 1.2e-4 in [0.25, 0.5),
                    // so the ratio by about 5e-4; the read back adds 1.5e-5
                    EXPECT_NEAR (fogged[index] / reference[index], expected, 2.0e-3f) << name << ", wall " << wall << ", row " << y;
                    largestError = jmax (largestError, std::abs (fogged[index] / reference[index] - expected));
                    lowest = jmin (lowest, expected);
                    highest = jmax (highest, expected);

                    // The exponent at the wall, against 40 at the camera's side of the clamp
                    const auto exponent = -fog.heightFalloff * (point.getY() - fog.baseHeight);
                    crossing += exponent < 40.0f && -fog.heightFalloff * (eye.getY() - fog.baseHeight) > 40.0f ? 1 : 0;
                    pastTheClamp += exponent >= 40.0f ? 1 : 0;
                }
            }

            RecordProperty (String (name).replace (" ", "_").toStdString() + "_largestError", String (largestError, 6).toStdString());
            RecordProperty (String (name).replace (" ", "_").toStdString() + "_range", (String (lowest, 3) + " to " + String (highest, 3)).toStdString());

            // Not vacuous: the pixels span thin and thick fog, and below the clamp both of its branches
            EXPECT_LT (lowest, 0.3f) << name;
            EXPECT_GT (highest, 0.55f) << name;

            if (eye.getY() < 0.0f)
            {
                EXPECT_GT (crossing, 0) << name;
                EXPECT_GT (pastTheClamp, 0) << name;
            }
        }
    }

    /** On @p tier, with an infinite far plane, background pixels:
        - in uniform fog, seen level by a camera whose far plane lies within its near plane (which
          projects to infinity too), are fully fogged, with the ambient radiance;
        - climbing out of fog thinning upwards are partly fogged, by the limit of the closed form at
          infinity, from the origin and from far below the base, past the clamp of the exponent;
        - looking down from there, into ever denser fog, are fully fogged.
        Every texel stays finite.
    */
    void checkInfiniteFarPlaneFogStaysFinite (RenderSettings::TierPreference tier)
    {
        {
            Scene scene;
            auto& camera = addFogCamera (scene);
            camera.zFar = camera.zNear * 0.5f;
            scene.setAmbientColor (fogAmbientColor());
            scene.setWorldFog (makeFog (0.08f));

            SceneRenderer renderer;
            setFogTier (renderer, tier);
            const auto sceneColor = renderFogSceneColor (renderer, scene);
            ASSERT_NE (sceneColor, nullptr);
            EXPECT_TRUE (isFiniteTexture (sceneColor));

            const auto alpha = readUnitChannel (sceneColor, "a");
            const auto red = readUnitChannel (sceneColor, "r");
            ASSERT_EQ (alpha.size(), static_cast<size_t> (fogSize * fogSize));
            ASSERT_EQ (red.size(), alpha.size());

            auto largestError = 0.0f;
            for (size_t i = 0; i < alpha.size(); ++i)
            {
                EXPECT_GE (alpha[i], 0.999f) << i;
                EXPECT_NEAR (red[i], fogAmbientRadiance(), 2.0e-3f) << i;
                largestError = jmax (largestError, std::abs (red[i] - fogAmbientRadiance()));
            }

            RecordProperty ("horizontalLargestError", String (largestError, 6).toStdString());
        }

        const auto renderAlpha = [&] (Vector3<float> eye, Vector3<float> direction, const WorldFog& fog, const char* name, std::vector<Vector3<float>>& directions)
        {
            Scene scene;
            auto& camera = addFogCamera (scene, eye + direction, eye);
            camera.zFar = 0.0f;
            scene.setWorldFog (fog);

            for (const auto y : { 2, 32, 61 })
                directions.push_back (pixelDirection (camera, 32, y, fogSize));

            SceneRenderer renderer;
            setFogTier (renderer, tier);
            const auto sceneColor = renderFogSceneColor (renderer, scene);
            EXPECT_TRUE (sceneColor != nullptr && isFiniteTexture (sceneColor)) << name;
            return sceneColor != nullptr ? readUnitChannel (sceneColor, "a") : std::vector<float>();
        };

        const auto belowTheClamp = makeFog (1.0e-18f, 2.0f);
        const struct
        {
            const char* name;
            Vector3<float> eye, direction;
            WorldFog fog;
        } climbing[] = {
            { "upwards", {}, { 0.0f, 1.0f, -1.0f }, makeFog (0.08f, 0.5f) },
            { "upwards below the clamp", { 0.0f, -22.0f, 0.0f }, { 0.0f, 0.5774f, -1.0f }, belowTheClamp },
        };

        for (const auto& [name, eye, direction, fog] : climbing)
        {
            std::vector<Vector3<float>> directions;
            const auto alpha = renderAlpha (eye, direction, fog, name, directions);
            ASSERT_EQ (alpha.size(), static_cast<size_t> (fogSize * fogSize)) << name;

            for (size_t row = 0; row < directions.size(); ++row)
            {
                // The far end 10000 units away, where the fog has thinned to nothing
                const auto y = std::array<int, 3> { 2, 32, 61 }[row];
                const auto transmittance = std::exp (-fog.getOpticalDepth (eye, eye + directions[row] * 1.0e4f));
                EXPECT_GT (transmittance, 0.01f) << name << ", row " << y;
                EXPECT_LT (transmittance, 0.95f) << name << ", row " << y;
                EXPECT_NEAR (alpha[static_cast<size_t> (y * fogSize + 32)], 1.0f - transmittance, 2.0e-3f) << name << ", row " << y;
                RecordProperty (String (name).replace (" ", "_").toStdString() + std::to_string (y), (String (alpha[static_cast<size_t> (y * fogSize + 32)], 5) + " vs " + String (1.0f - transmittance, 5)).toStdString());
            }
        }

        // Looking down from below the clamp, into ever denser fog: no light gets through
        std::vector<Vector3<float>> directions;
        const auto belowAlpha = renderAlpha ({ 0.0f, -22.0f, 0.0f }, { 0.0f, -1.0f, -1.0f }, belowTheClamp, "downwards", directions);
        ASSERT_EQ (belowAlpha.size(), static_cast<size_t> (fogSize * fogSize));

        for (size_t i = 0; i < belowAlpha.size(); ++i)
            EXPECT_GE (belowAlpha[i], 0.999f) << i;
    }

    /** On @p tier, fog the camera sees no end of scatters the ambient color plus the mean radiance of
        the environment (the L0 of its spherical harmonics) times its intensity, exposed.
    */
    void checkFogIsLitByTheAmbientLightAndTheEnvironment (RenderSettings::TierPreference tier)
    {
        Scene scene;
        addFogCamera (scene);
        scene.setAmbientColor (fogAmbientColor());
        scene.setEnvironment (new EnvironmentMap ([] (const Vector3<float>&) { return Vector3<float> (0.5f, 0.5f, 0.5f); }, 32, 3));
        scene.setEnvironmentIntensity (0.4f);
        scene.setExposure (2.0f);
        scene.setWorldFog (makeFog (0.08f));

        SceneRenderer renderer;
        setFogTier (renderer, tier);
        const auto sceneColor = renderFogSceneColor (renderer, scene);
        ASSERT_NE (sceneColor, nullptr);

        // The far plane 1000 units away: no light gets through the fog
        const auto red = readUnitChannel (sceneColor, "r");
        ASSERT_EQ (red.size(), static_cast<size_t> (fogSize * fogSize));

        // L0 of a uniform environment is its radiance
        const auto meanRadiance = scene.getEnvironment()->getIrradianceCoefficients()[0].getX();
        EXPECT_NEAR (meanRadiance, 0.5f, 1.0e-3f);
        const auto expected = (fogAmbientRadiance() + meanRadiance * 0.4f) * 2.0f;
        RecordProperty ("measured", String (red[static_cast<size_t> (32 * fogSize + 32)], 5).toStdString());
        RecordProperty ("expected", String (expected, 5).toStdString());

        for (size_t i = 0; i < red.size(); ++i)
            EXPECT_NEAR (red[i], expected, 2.0e-3f) << i;
    }

    /** On @p tier, isotropic fog lit by the sun alone, which no ray leaves, scatters E / (4 pi) of the
        sun's irradiance E towards the camera (preflight R25): the phase function is normalized over the
        sphere. A dark directional light listed before the sun lights nothing, the fog included.
    */
    void checkFogScattersTheSunPhysically (RenderSettings::TierPreference tier)
    {
        Scene scene;
        addFogCamera (scene).zFar = 0.0f;
        scene.setWorldFog (makeFog (0.08f));

        auto dark = EntityNode::Ptr (new EntityNode ("dark"));
        dark->attach<LightNode>().intensity = 0.0f;
        scene.getRoot()->addChild (dark);

        constexpr auto irradiance = 2.0f;
        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = irradiance;
        aimCamera (*sun, { 3.0f, 10.0f, 2.0f }, {});
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        setFogTier (renderer, tier);
        const auto sceneColor = renderFogSceneColor (renderer, scene);
        ASSERT_NE (sceneColor, nullptr);

        const auto red = readUnitChannel (sceneColor, "r");
        ASSERT_EQ (red.size(), static_cast<size_t> (fogSize * fogSize));

        // One half float step on store (1.22e-4 in [0.125, 0.25), and this GPU may truncate rather than
        // round) plus 1.5e-5 of read back: 1e-3 is about 8 steps
        const auto expected = irradiance / (4.0f * MathConstants<float>::pi);
        RecordProperty ("measured", String (red[static_cast<size_t> (32 * fogSize + 32)], 5).toStdString());
        RecordProperty ("expected", String (expected, 5).toStdString());

        for (size_t i = 0; i < red.size(); ++i)
            EXPECT_NEAR (red[i], expected, 1.0e-3f) << i;
    }

    /** On @p tier, fog over the background color keeps its display-space blend on frames the depth of
        field blurs (preflight R24): blurring a uniform coverage moves none of it, so the background,
        far out of focus, stays within 2 levels of the frame without depth of field.
    */
    void checkFogOverTheBackgroundKeepsItsBlendWithDepthOfField (RenderSettings::TierPreference tier)
    {
        Scene scene;
        auto& camera = addFogCamera (scene);
        camera.zFar = 10.0f;
        camera.fStop = 1.4f;
        camera.focusDistance = 0.2f;
        scene.setAmbientColor (fogAmbientColor());
        scene.setWorldFog (makeFog (0.08f));

        const auto render = [&] (bool depthOfField)
        {
            SceneRenderer renderer;
            setFogTier (renderer, tier);
            setDepthOfField (renderer, depthOfField);
            auto image = renderToImage (renderer, scene, fogSize);
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField) != nullptr, depthOfField);
            return image;
        };

        const auto off = render (false);
        const auto on = render (true);
        ASSERT_TRUE (off.isValid() && on.isValid());

        RecordProperty ("largestDifference", maxChannelDifference (on, off));
        RecordProperty ("background", String (meanLuminance (off, { 0, 0, fogSize, fogSize }), 1).toStdString());

        // Not vacuous: the background shows the fog's partial coverage
        EXPECT_GT (meanLuminance (off, { 0, 0, fogSize, fogSize }), 30.0);
        EXPECT_LE (maxChannelDifference (on, off), 2);
    }

    /** On @p tier, bloom after the fog reads the fog's linear color, not the reversible one: fog lit by
        nothing only absorbs, so the emissive quad of buildGlowScene() seen through it glows like the
        same quad dimmed by the fog's transmittance, without fog and without depth of field.
    */
    void checkFogBeforeBloomGlowsLikeADimmerEmitter (RenderSettings::TierPreference tier)
    {
        constexpr auto density = 0.3f;
        constexpr auto distance = 2.0f;

        const auto render = [&] (float emission, bool withFog)
        {
            Scene scene;
            buildGlowScene (scene, emission);
            addFogCamera (scene, {}, { 0.0f, 0.0f, distance });

            if (withFog)
                scene.setWorldFog (makeFog (density));

            SceneRenderer renderer;
            setFogTier (renderer, tier);
            setBloom (renderer, true);
            auto image = renderToImage (renderer, scene, 128);
            EXPECT_TRUE (image.isValid()) << renderer.getLastError();
            EXPECT_EQ (renderer.getFogApply() != nullptr, withFog);
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField), nullptr);
            return image;
        };

        const auto foggy = render (20.0f, true);
        const auto dimmer = render (20.0f * std::exp (-density * distance), false);
        ASSERT_TRUE (foggy.isValid() && dimmer.isValid());

        const auto quad = findBrightBounds (dimmer);
        const auto foggyRing = ringLuminance (foggy, quad);
        const auto dimmerRing = ringLuminance (dimmer, quad);
        RecordProperty ("foggyRing", String (foggyRing, 2).toStdString());
        RecordProperty ("dimmerRing", String (dimmerRing, 2).toStdString());

        // Not vacuous: the dimmed quad still glows
        ASSERT_FALSE (quad.isEmpty());
        EXPECT_GT (dimmerRing, 5.0);
        EXPECT_NEAR (foggyRing, dimmerRing, 0.05 * dimmerRing);
    }

    /** On @p tier, pixels of the background color blend towards the fog by its coverage: with the far
        plane 10 units away, the background color shows through by the transmittance T to the far plane.
    */
    void checkBackgroundColorLerpsTowardsTheFog (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (Color background, bool withFog)
        {
            Scene scene;
            addFogCamera (scene).zFar = 10.0f;
            scene.setAmbientColor (fogAmbientColor());
            scene.setBackgroundColor (background);

            if (withFog)
                scene.setWorldFog (makeFog (0.08f));

            SceneRenderer renderer;
            setFogTier (renderer, tier);
            const auto image = renderToImage (renderer, scene, fogSize);
            EXPECT_TRUE (image.isValid()) << renderer.getLastError();
            return image.isValid() ? static_cast<int> (image.getPixelColor (32, 32).getRed()) : -1;
        };

        const auto gray = Color (0xff646464);
        EXPECT_NEAR (render (gray, false), 100, 1);

        const auto overBlack = render (Colors::black, true);
        const auto overGray = render (gray, true);

        Scene scene;
        const auto& camera = addFogCamera (scene);
        const auto direction = pixelDirection (camera, 32, 32, fogSize);
        const auto transmittance = std::exp (-0.08f * 10.0f / -direction.getZ());

        RecordProperty ("overBlack", overBlack);
        RecordProperty ("overGray", overGray);

        EXPECT_GT (overBlack, 30);                                        // The fog over black
        EXPECT_NEAR (overGray - overBlack, transmittance * 100.0f, 3.0f); // The background color by T
    }

    /** On @p tier, fog with a forward scattering anisotropy (g = 0.8) is brighter looking towards the
        sun than with the sun behind the camera.
    */
    void checkFogScattersForwardTowardsTheSun (RenderSettings::TierPreference tier)
    {
        const auto render = [&] (Vector3<float> sunPosition)
        {
            Scene scene;
            addFogCamera (scene);

            auto fog = makeFog (0.08f);
            fog.anisotropy = 0.8f;
            scene.setWorldFog (fog);

            auto sun = EntityNode::Ptr (new EntityNode ("sun"));
            sun->attach<LightNode>().intensity = 0.1f;
            aimCamera (*sun, sunPosition, {});
            scene.getRoot()->addChild (sun);

            SceneRenderer renderer;
            setFogTier (renderer, tier);
            const auto image = renderToImage (renderer, scene, fogSize);
            EXPECT_TRUE (image.isValid()) << renderer.getLastError();
            return image.isValid() ? meanLuminance (image, { 28, 28, 8, 8 }) : 0.0;
        };

        // A sun ahead of the camera shines towards it
        const auto towards = render ({ 0.0f, 0.0f, -10.0f });
        const auto away = render ({ 0.0f, 0.0f, 10.0f });

        RecordProperty ("towards", String (towards, 1).toStdString());
        RecordProperty ("away", String (away, 1).toStdString());

        EXPECT_GT (towards, away + 100.0);
    }

    /** Raster fog where the device cannot run it, without the prepass or with too few multisampled
        half-float samples: the frames are the ones without fog, with no prepass, no half float scene
        color and no fog pass.
    */
    void checkRasterWorldFogUnavailableRecordsNothing()
    {
        Scene scene;
        buildFogWallsScene (scene, false);

        SceneRenderer reference;
        setFogTier (reference, RenderSettings::TierPreference::raster);
        const auto off = renderToImage (reference, scene, fogSize);
        ASSERT_TRUE (off.isValid()) << reference.getLastError();
        scene.setWorldFog (makeFog (0.08f));

        const std::pair<const char*, std::function<void (RenderCapabilities::DeviceFacts&)>> devices[] = {
            { "no prepass", [] (RenderCapabilities::DeviceFacts& facts) { facts.depthNormalPrepass = false; } },
            { "one half float sample", [] (RenderCapabilities::DeviceFacts& facts) { facts.rgba16floatSamples = 1; } },
        };

        for (const auto& [name, edit] : devices)
        {
            SceneRenderer renderer;
            setFogTier (renderer, RenderSettings::TierPreference::raster);
            renderer.setDeviceFactsOverride (edit);

            for (int frame = 0; frame < 2; ++frame)
            {
                const auto image = renderToImage (renderer, scene, fogSize);
                ASSERT_TRUE (image.isValid()) << name << ": " << renderer.getLastError();
                EXPECT_LE (maxChannelDifference (image, off), 1) << name;
            }

            EXPECT_FALSE (renderer.getCapabilities()->isAvailable (RenderCapabilities::Feature::worldFog)) << name;
            EXPECT_EQ (renderer.getFogApply(), nullptr) << name;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
            ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr) << name;
            EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float) << name;
        }
    }

    /** A raster frame with fog only draws the prepass for its depth and takes the half float route,
        without the ambient occlusion: no ambient occlusion pass, and the forward pass shades with its
        default variant. Turning ambient occlusion on takes its variant, so the check is not vacuous.
    */
    void checkRasterWorldFogDrawsThePrepassWithoutAmbientOcclusion()
    {
        Scene scene;
        buildFogWallsScene (scene, true);

        SceneRenderer renderer;
        setFogTier (renderer, RenderSettings::TierPreference::raster);
        const auto render = [&] (bool ambientOcclusion)
        {
            auto settings = renderer.getRenderSettings();
            settings.ambientOcclusion = ambientOcclusion;
            renderer.setRenderSettings (settings);

            ASSERT_NE (renderer.render (device, scene, fogSize, fogSize), nullptr) << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
            ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float);
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::ambientOcclusion) != nullptr, ambientOcclusion);
            ASSERT_NE (renderer.getFogApply(), nullptr);
            EXPECT_GT (renderer.getFogApply()->getNumCachedTargets(), 0u);
            EXPECT_EQ (renderer.getDepthOfField(), nullptr);
            ASSERT_NE (renderer.getForwardPass(), nullptr);
            EXPECT_EQ (renderer.getForwardPass()->usesLocalIndirect(), ambientOcclusion);
        };

        render (false);
        EXPECT_EQ (renderer.getAmbientOcclusionPass(), nullptr);

        // Not vacuous: ambient occlusion takes the variant reading it
        render (true);
        EXPECT_NE (renderer.getAmbientOcclusionPass(), nullptr);

        // Turning the fog off releases its target and leaves the half float route
        auto settings = renderer.getRenderSettings();
        settings.ambientOcclusion = false;
        renderer.setRenderSettings (settings);
        auto fog = scene.getWorldFog();
        fog.enabled = false;
        scene.setWorldFog (fog);

        ASSERT_NE (renderer.render (device, scene, fogSize, fogSize), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        ASSERT_NE (renderer.getFogApply(), nullptr);
        EXPECT_EQ (renderer.getFogApply()->getNumCachedTargets(), 0u);
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
        ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr);
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat(), GpuTextureFormat::rgba16float);
    }

    /** On raster the fog is the first post pass: the depth of field blurs the fogged color, whose
        background the fog covers, and both outputs stay finite.
    */
    void checkRasterWorldFogRunsBeforeTheDepthOfField()
    {
        Scene scene;
        setUpDepthOfFieldScene (scene, 0.2f);
        buildBarsScene (scene, 10.0f);
        scene.setAmbientColor (fogAmbientColor());

        SceneRenderer renderer;
        setDepthOfFieldTier (renderer, RenderSettings::TierPreference::raster);
        const auto meanBlurredCoverage = [&]
        {
            const auto texture = renderer.render (device, scene, fogSize, fogSize);
            EXPECT_TRUE (renderer.waitForGPU().wasOk());
            EXPECT_NE (texture, nullptr) << renderer.getLastError();

            const auto blurred = renderer.getPassOutput (SceneRenderer::PassOutput::depthOfField);
            EXPECT_NE (blurred, nullptr);
            if (blurred == nullptr)
                return 0.0f;

            EXPECT_TRUE (isFiniteTexture (blurred));
            const auto alpha = readUnitChannel (blurred, "a");
            return alpha.empty() ? 0.0f : std::accumulate (alpha.begin(), alpha.end(), 0.0f) / static_cast<float> (alpha.size());
        };

        const auto clear = meanBlurredCoverage();

        scene.setWorldFog (makeFog (0.08f));
        const auto foggy = meanBlurredCoverage();
        EXPECT_TRUE (isFiniteTexture (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)));

        RecordProperty ("clearCoverage", String (clear, 3).toStdString());
        RecordProperty ("foggyCoverage", String (foggy, 3).toStdString());

        // The bars cover about half the frame; the fog covers the background they leave
        EXPECT_LT (clear, 0.7f);
        EXPECT_GT (foggy, clear + 0.3f);
    }

    /** Raster fog skips the frames whose prepass gave no depth, without counting them as failures:
        once prepass targets that could not be created come back, so does the fog. A prepass that
        cannot compile takes the fog off the route: its frames create no fog pass and, without
        bloom, keep the inline frame.
    */
    void checkRasterWorldFogSkipsFramesWithoutThePrepassDepth()
    {
        Scene scene;
        buildFogWallsScene (scene, true);

        for (const auto bloom : { false, true })
        {
            const auto name = bloom ? "prepass not compiled, bloom" : "prepass not compiled";

            SceneRenderer renderer;
            setFogTier (renderer, RenderSettings::TierPreference::raster);
            setBloom (renderer, bloom);
            renderer.failDepthNormalPrepassForTesting();

            for (int frame = 0; frame < 3; ++frame)
            {
                ASSERT_NE (renderer.render (device, scene, fogSize, fogSize), nullptr) << name << ": " << renderer.getLastError();
                EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
                EXPECT_EQ (renderer.getFogApply(), nullptr) << name;

                const auto sceneColor = renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor);
                ASSERT_NE (sceneColor, nullptr) << name;
                EXPECT_EQ (sceneColor->getFormat() == GpuTextureFormat::rgba16float, bloom) << name;
            }
        }

        // More frames than the two failures in a row that give up
        const auto expectSkipped = [&] (SceneRenderer& renderer, const char* name)
        {
            for (int frame = 0; frame < 3; ++frame)
            {
                ASSERT_NE (renderer.render (device, scene, fogSize, fogSize), nullptr) << name << ": " << renderer.getLastError();
                EXPECT_TRUE (renderer.waitForGPU().wasOk()) << name;
                EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr) << name;
                ASSERT_NE (renderer.getFogApply(), nullptr) << name;
                EXPECT_FALSE (renderer.getFogApply()->hasGivenUp()) << name;
                EXPECT_EQ (renderer.getFogApply()->getNumCachedTargets(), 0u) << name;
            }
        };

        SceneRenderer renderer;
        setFogTier (renderer, RenderSettings::TierPreference::raster);
        setBloom (renderer, false);
        renderer.failDepthNormalPrepassTargetsForTesting (3);
        expectSkipped (renderer, "prepass without targets");

        ASSERT_NE (renderer.render (device, scene, fogSize, fogSize), nullptr) << renderer.getLastError();
        EXPECT_TRUE (renderer.waitForGPU().wasOk());
        EXPECT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal), nullptr);
        ASSERT_NE (renderer.getFogApply(), nullptr);
        EXPECT_GT (renderer.getFogApply()->getNumCachedTargets(), 0u);
    }

    /** Raster fog that gave up after two failures in a row leaves the route it took: the frames after
        it draw no prepass and no half float scene color.
    */
    void checkRasterWorldFogThatGaveUpStopsItsRoute()
    {
        Scene scene;
        buildFogWallsScene (scene, true);

        SceneRenderer renderer;
        setFogTier (renderer, RenderSettings::TierPreference::raster);
        renderer.failWorldFogForTesting (2);

        for (int frame = 0; frame < 3; ++frame)
        {
            ASSERT_NE (renderer.render (device, scene, fogSize, fogSize), nullptr) << frame << ": " << renderer.getLastError();
            EXPECT_TRUE (renderer.waitForGPU().wasOk()) << frame;
            ASSERT_NE (renderer.getFogApply(), nullptr) << frame;
            EXPECT_EQ (renderer.getFogApply()->hasGivenUp(), frame >= 1) << frame;

            // The two failing frames took the route; the one after the fog gave up does not
            const auto routed = frame < 2;
            ASSERT_NE (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor), nullptr) << frame;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::prepassNormal) != nullptr, routed) << frame;
            EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::sceneColor)->getFormat() == GpuTextureFormat::rgba16float, routed) << frame;
        }
    }

    /** A renderer shared by the tests of a suite on the same device, so that its pipelines compile
        once, returned with the default settings, a restarted accumulation and the default options.
        It keeps the rest of its history (the jitter phase, the previous view, pass objects, probe
        bakes, counters and failures): tests that depend on any of it use their own renderer.
    */
    SceneRenderer& sharedRenderer()
    {
        if (suiteRenderer == nullptr || suiteRendererDevice != device)
        {
            suiteRenderer.reset();
            suiteRenderer = std::make_unique<SceneRenderer>();
            suiteRendererDevice = device;
        }

        suiteRenderer->setRenderSettings (RenderSettings {});
        suiteRenderer->restartAccumulation();
        suiteRenderer->setFrameStatsEnabled (false);
        suiteRenderer->setDeviceFactsOverride ({});
        suiteRenderer->setSampleCount (4);
        return *suiteRenderer;
    }

    /** Releases the shared renderer, before the device it renders on goes away. */
    static void releaseSharedRenderer()
    {
        suiteRenderer.reset();
        suiteRendererDevice = nullptr;
    }

    static void TearDownTestSuite()
    {
        releaseSharedRenderer();
    }

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;

private:
    static inline std::unique_ptr<SceneRenderer> suiteRenderer;
    static inline GpuDevice::Ptr suiteRendererDevice;
};
#endif
