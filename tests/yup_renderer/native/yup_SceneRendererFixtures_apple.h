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

using namespace yup;

class SceneRendererMetalTests : public SceneRendererBaselineScenes
{
protected:
    /** One device per test suite, so that the suite's shared renderer keeps its pipelines. It
        compiles Rive's shaders synchronously: a pooled 2D context would otherwise read a texture back
        through Rive's fallback shader on its first frame only, a level off the specialized one, so
        exact comparisons would depend on the order the tests run in.
    */
    static void TearDownTestSuite()
    {
        SceneRendererBaselineScenes::TearDownTestSuite();
        suiteDevice = nullptr;
    }

    void SetUp() override
    {
        if (suiteDevice == nullptr)
        {
            GpuDevice::Options options;
            options.synchronousShaderCompilations = true;
            suiteDevice = GpuDevice::create (GpuPlatform::Metal, options);
        }

        device = suiteDevice;
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";
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
    Color renderEnvironmentCorner (float radiance, ToneMappingOperator toneMapping)
    {
        Scene scene;
        buildShadowsScene (scene);
        scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([radiance] (const Vector3<float>&) { return Vector3<float> (radiance, radiance, radiance); }, 32, 3));
        scene.getRoot()->attach<ToneMappingNode> (toneMapping);

        SceneRenderer renderer;
        const auto image = renderToImage (renderer, scene, 64);

        EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
        EXPECT_TRUE (image.isValid());
        return image.isValid() ? image.getPixelColor (1, 1) : Color();
    }

    /** A rough white metal plate and a smoother dielectric plate in front of it, lit by a sphere
        point light, a spot disk and a sun disk the front plate shadows. The back plate is larger
        than the frame region DeferredMatchesForwardForOpaque compares, so its silhouettes, where
        MSAA and TAA differ, stay out of it. */
    static void buildAreaLightsScene (Scene& scene)
    {
        const auto addScaledPlate = [&] (Material::Ptr material, float scale, Vector3<float> position)
        {
            auto* plate = addPlate (scene, std::move (material));
            plate->setScale ({ scale, scale, 1.0f });
            plate->setPosition (position);
        };

        auto metal = Material::Ptr (new Material());
        metal->roughnessFactor = 0.6f;

        auto dielectric = Material::Ptr (new Material());
        dielectric->baseColorFactor = { 0.6f, 0.3f, 0.2f, 1.0f };
        dielectric->metallicFactor = 0.0f;
        dielectric->roughnessFactor = 0.3f;

        addScaledPlate (metal, 1.3f, {});
        addScaledPlate (dielectric, 0.3f, { 0.3f, -0.3f, 0.3f });

        const auto addLight = [&] (LightNode::Type type, float intensity, float radius, Vector3<float> position, const Quaternion& rotation)
        {
            auto light = EntityNode::Ptr (new EntityNode ("light"));
            auto& node = light->attach<LightNode> (type);
            node.intensity = intensity;
            node.radius = radius;
            node.castsShadows = type == LightNode::Type::directional;
            light->setPosition (position);
            light->setRotation (rotation);
            scene.getRoot()->addChild (light);
        };

        addLight (LightNode::Type::point, 1.0f, 0.25f, { 0.4f, 0.3f, 1.0f }, Quaternion::identity());
        addLight (LightNode::Type::spot, 1.5f, 0.15f, { -0.4f, -0.3f, 1.2f }, Quaternion::identity());
        addLight (LightNode::Type::directional, 0.5f, 0.05f, {}, Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, 0.5f));
    }

    /** A rough (roughness 1) plate drawn with @p material and lit head-on by a white sun of
        intensity @p sunIntensity, under @p ambient light: the mean linear radiance of the 8x8
        pixels at the center, where the normal, the view and the light directions coincide. There
        D = 1 / pi, V = 1 / 4 and F = f0, so the sun adds 2 (f0 / 4 + diffuseColor * weight) / pi
        at the default intensity of 2. A positive @p environmentRadiance adds a uniform environment
        of that radiance, and @p failLookupTables renders without the lookup tables.
    */
    std::array<double, 3> renderHeadOnRadiance (Material::Ptr material, RenderSettings::TierPreference tier,
                                                float sunIntensity = 2.0f, Color ambient = Colors::black,
                                                float environmentRadiance = 0.0f, bool failLookupTables = false)
    {
        material->roughnessFactor = 1.0f;

        Scene scene;
        addPlate (scene, std::move (material));
        scene.getRoot()->attach<AmbientLightNode> (ambient);
        scene.getRoot()->attach<ToneMappingNode> (ToneMappingOperator::reinhard);

        if (environmentRadiance > 0.0f)
            scene.getRoot()->attach<EnvironmentNode> (new EnvironmentMap ([environmentRadiance] (const Vector3<float>&) { return Vector3<float> (environmentRadiance, environmentRadiance, environmentRadiance); }, 32, 3));

        auto sun = EntityNode::Ptr (new EntityNode ("sun"));
        sun->attach<LightNode>().intensity = sunIntensity;
        scene.getRoot()->addChild (sun);

        SceneRenderer renderer;
        setTier (renderer, tier);
        if (failLookupTables)
            renderer.failLookupTablesForTesting();

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
        scene.getRoot()->attach<BackgroundNode> (Color (static_cast<uint8> (40), static_cast<uint8> (70), static_cast<uint8> (110)));
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

    /** Nine gray pads 0.4 wide and 0.55 apart, each its own draw, facing a camera 3 units away,
        under 32 colored point lights of range 0.5 and intensity 0.5, like the light show of the
        Scene3D demo. Returns the light entities, placed by moveOrbitingLights().
    */
    static std::vector<EntityNode*> buildOrbitingLightsScene (Scene& scene)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.5f, 0.5f, 0.5f, 1.0f };
        material->metallicFactor = 0.0f;
        material->roughnessFactor = 0.7f;

        for (int row = 0; row < 3; ++row)
        {
            for (int column = 0; column < 3; ++column)
            {
                auto* pad = addPlate (scene, material);
                pad->setScale ({ 0.2f, 0.2f, 1.0f });
                pad->setPosition ({ 0.55f * static_cast<float> (column - 1), 0.55f * static_cast<float> (row - 1), 0.0f });
            }
        }

        std::vector<EntityNode*> lights;
        for (int i = 0; i < 32; ++i)
        {
            auto light = EntityNode::Ptr (new EntityNode ("showLight"));
            auto& node = light->attach<LightNode> (LightNode::Type::point);
            const auto color = Color::fromHSV (static_cast<float> (i) / 32.0f, 0.85f, 1.0f);
            node.color = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat() };
            node.intensity = 0.5f;
            node.range = 0.5f;
            scene.getRoot()->addChild (light);
            lights.push_back (light.get());
        }

        auto camera = EntityNode::Ptr (new EntityNode ("camera"));
        camera->attach<CameraNode>();
        camera->setPosition ({ 0.0f, 0.0f, 3.0f });
        scene.getRoot()->addChild (camera);
        scene.setActiveCamera (camera.get());

        scene.getRoot()->attach<AmbientLightNode> (Colors::black);
        scene.getRoot()->attach<BackgroundNode> (Colors::black);
        return lights;
    }

    /** Places the lights of buildOrbitingLightsScene() at @p time seconds: four rings of orbits, alternating in direction, as in the demo. */
    static void moveOrbitingLights (const std::vector<EntityNode*>& lights, double time)
    {
        for (size_t i = 0; i < lights.size(); ++i)
        {
            const auto ring = static_cast<float> (i % 4);
            const auto speed = (i % 2 == 0 ? 0.35f : -0.25f) * (1.0f + 0.15f * ring);
            const auto angle = static_cast<float> (i) * 2.39996f + speed * static_cast<float> (time);
            const auto radius = 0.25f + 0.2f * ring;

            lights[i]->setPosition ({ std::cos (angle) * radius, std::sin (angle) * radius, 0.1f + 0.05f * ring });
        }
    }

private:
    static inline GpuDevice::Ptr suiteDevice;
};

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
        SceneRenderer renderer;
        if (GetParam() == RenderSettings::TierPreference::full)
            setFullTierTaa (renderer, 1);
        else
        {
            setTier (renderer, RenderSettings::TierPreference::raster);
            setBloom (renderer, true);
        }

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

    /** renderLuma() on the tier of the test parameter. */
    std::vector<double> renderLuma (Scene& scene, int size)
    {
        return SceneRendererBaselineScenes::renderLuma (scene, size, GetParam());
    }
#endif
};

#if YUP_ENABLE_SHADER_TRANSPILER
/** GBufferPass on its own targets, read back before any other pass draws over the depth and
    velocity it shares with them. */
class GBufferPassMetalTests : public SceneRendererMetalTests
{
protected:
    static constexpr int size = 64;

    void SetUp() override
    {
        SceneRendererMetalTests::SetUp();
        if (! IsSkipped())
            frameResources = std::make_unique<FrameResources> (device);
    }

    /** A green alpha masked material of base color alpha @p alpha, cut out below the default 0.5 cutoff. */
    static Material::Ptr makeMasked (float alpha)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.2f, 0.8f, 0.2f, alpha };
        material->alphaMode = Material::AlphaMode::mask;
        return material;
    }

    /** A square of @p halfSize centered at (@p x, 0, 0), facing the default camera. */
    static EntityNode::Ptr makeQuad (float x, float halfSize, Material::Ptr material)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { x - halfSize, -halfSize, 0.0f };
        vertices[1].position = { x + halfSize, -halfSize, 0.0f };
        vertices[2].position = { x + halfSize, halfSize, 0.0f };
        vertices[3].position = { x - halfSize, halfSize, 0.0f };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("quad"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, material);

        auto entity = EntityNode::Ptr (new EntityNode ("quad"));
        entity->attach<MeshNode> (mesh);
        return entity;
    }

    /** Draws the opaque items of @p scene, moved by @p motion since the previous frame, into the
        targets of a frame. The previous view is the one of the previous call.
    */
    GBufferPass::Targets render (Scene& scene, Vector3<float> motion = {})
    {
        frameResources->beginFrame (size, size);

        const auto acquire = [&] (GpuTextureFormat format)
        {
            return frameResources->acquire (GpuTextureDesc (static_cast<uint32_t> (size), static_cast<uint32_t> (size), format, true));
        };

        const auto targets = GBufferPass::acquireTargets (*frameResources, acquire (GpuTextureFormat::rg16float), acquire (GpuTextureFormat::depth32float));

        view.update (scene, SceneSettings::resolve (scene), size, size, false, 1, 0);
        drawList.build (*scene.getRoot(), view.getView());

        for (const auto& item : drawList.getOpaqueItems())
            item.node->previousWorld = item.world.followedBy (Matrix4::translation ({ -motion.getX(), -motion.getY(), -motion.getZ() }));

        EXPECT_TRUE (gbufferPass.prepare (device).wasOk());

        auto frame = GpuFrame::begin (device);
        EXPECT_TRUE (gbufferPass.render (frame, view, drawList.getOpaqueItems(), materials, targets));
        frame.submit();
        EXPECT_TRUE (frame.waitForGPU().wasOk());

        frameResources->endFrame();
        return targets;
    }

    std::unique_ptr<FrameResources> frameResources;
    ViewData view;
    SceneDrawList drawList;
    GBufferPass gbufferPass;
    MaterialTable materials;
};
#endif
