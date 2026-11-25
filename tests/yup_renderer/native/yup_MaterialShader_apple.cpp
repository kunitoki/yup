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

using namespace yup;

#if YUP_ENABLE_SHADER_TRANSPILER

/** What SceneRenderer draws for the meshes of a MaterialShaderNode, on both tiers, with materials
    compiled from their source. The scenes are unlit and Reinhard tone mapped, so a standard
    material shows its emission alone: a channel emitting 1 reads 188, and one emitting nothing 0.
*/
class MaterialShaderTierTests : public SceneRendererTierTests
{
protected:
    /** Compiles a material inside the shaders of the module. */
    static MaterialShader::Ptr compile (const String& source)
    {
        const auto shaders = File (__FILE__).getParentDirectory().getParentDirectory().getParentDirectory().getParentDirectory().getChildFile ("modules/yup_renderer/shaders");

        const auto created = MaterialShader::createFromSource (source, shaders);
        EXPECT_TRUE (created.wasOk()) << (created.failed() ? created.getErrorMessage() : String());
        return created.wasOk() ? created.getValue() : nullptr;
    }

    /** Turns the lights of @p scene off: no default light and a black ambient light. */
    static void makeUnlit (Scene& scene, Color background = Colors::black, ToneMappingOperator toneMapping = ToneMappingOperator::reinhard)
    {
        scene.getRoot().attach<DefaultLightNode> (false);
        scene.getRoot().attach<AmbientLightNode> (Colors::black);
        scene.getRoot().attach<BackgroundNode> (background);
        scene.getRoot().attach<ToneMappingNode> (toneMapping);
    }

    /** A material emitting @p color and reflecting nothing, of base color alpha @p alpha. */
    static Material::Ptr makeEmissive (std::array<float, 3> color, Material::AlphaMode alphaMode = Material::AlphaMode::opaque, float alpha = 1.0f)
    {
        auto material = Material::Ptr (new Material());
        material->baseColorFactor = { 0.0f, 0.0f, 0.0f, alpha };
        material->emissiveFactor = color;
        material->alphaMode = alphaMode;
        return material;
    }

    /** Adds a square 0.6 wide facing the default camera, centered at (@p x, 0, @p z): about 9 px
        wide at 64 x 64, around the pixel (32 + 15 x, 32) at z = 0, (32 + 13 x, 32) at z = -0.5. */
    static Entity addQuad (Scene& scene, float x, float z, Material::Ptr material)
    {
        std::vector<Mesh::Vertex> vertices (4);
        vertices[0].position = { x - 0.3f, -0.3f, z };
        vertices[1].position = { x + 0.3f, -0.3f, z };
        vertices[2].position = { x + 0.3f, 0.3f, z };
        vertices[3].position = { x - 0.3f, 0.3f, z };

        for (auto& vertex : vertices)
            vertex.normal = { 0.0f, 0.0f, 1.0f };

        auto mesh = Mesh::Ptr (new Mesh ("quad"));
        mesh->addPrimitive (std::move (vertices), { 0, 1, 2, 0, 2, 3 }, std::move (material));

        auto entity = scene.createEntity ("quad");
        entity.attach<MeshNode> (mesh);
        return entity;
    }

    /** Colors the meshes of @p entity with @p shader, compiled from tintSource, showing @p tint. */
    static MaterialShaderNode& attachTint (Entity entity, const MaterialShader::Ptr& shader, std::array<float, 4> tint)
    {
        auto& node = entity.attach<MaterialShaderNode> (shader);
        EXPECT_TRUE (node.getUniforms().set ("tint", Span<const float> (tint.data(), tint.size())));
        return node;
    }

    /** The channels of the pixel at (@p x, @p y) holding light, among "r", "g" and "b": the ones
        above 150. A level from 40 to 150, which no surface of these scenes shows alone, reads "?". */
    static String channels (const Image& image, int x, int y)
    {
        const auto color = image.getPixelColor (x, y);
        String lit;

        for (const auto& [level, name] : { std::pair { color.getRed(), "r" }, std::pair { color.getGreen(), "g" }, std::pair { color.getBlue(), "b" } })
            lit << (level > 150 ? name : (level < 40 ? "" : "?"));

        return lit;
    }

    /** The channel of the pixel at (@p x, @p y) leading the two others by 100 levels, "r", "g" or
        "b", or "?" without one: what an accumulated frame shows, whatever is left of its history. */
    static String leadingChannel (const Image& image, int x, int y)
    {
        const auto color = image.getPixelColor (x, y);
        const int red = color.getRed(), green = color.getGreen(), blue = color.getBlue();

        if (red >= jmax (green, blue) + 100)
            return "r";

        if (green >= jmax (red, blue) + 100)
            return "g";

        return blue >= jmax (red, green) + 100 ? "b" : "?";
    }

    /** A material showing the color and the alpha of its `tint`. */
    static constexpr const char* tintSource = R"(YUP_MATERIAL_PARAMS
{
    vec4 tint;
} params;

vec4 yupMaterial (YupSurface surface)
{
    return params.tint;
}
)";
};

TEST_P (MaterialShaderTierTests, ColorOfAUniformShowsAsTheSameEmissionOfAMaterial)
{
    const auto shader = compile (tintSource);
    ASSERT_NE (shader, nullptr);

    // A plate emitting orange, the reference, and one emitting blue
    Scene reference, scene;
    addPlate (reference, makeEmissive ({ 1.0f, 0.5f, 0.1f }));
    auto plate = addPlate (scene, makeEmissive ({ 0.1f, 0.2f, 1.0f }));

    for (auto* each : { &reference, &scene })
    {
        makeUnlit (*each, Colors::black, ToneMappingOperator::aces);
        each->getRoot().attach<ExposureNode> (2.0f);
    }

    SceneRenderer renderer;
    setTierForExactFrames (renderer, GetParam());

    const auto expected = renderToImage (renderer, reference, 64);
    ASSERT_TRUE (expected.isValid()) << renderer.getLastError();

    const auto standard = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (standard.isValid()) << renderer.getLastError();
    EXPECT_GT (countDifferences (standard, expected), 400);

    // Under a shader returning the orange, the blue plate is the orange one, exposed and tone mapped alike
    attachTint (plate, shader, { 1.0f, 0.5f, 0.1f, 1.0f });

    const auto colored = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (colored.isValid()) << renderer.getLastError();
    EXPECT_EQ (countDifferences (colored, expected), 0);
}

TEST_P (MaterialShaderTierTests, StockShaderShowsTheBaseColorOfTheMaterialUnlit)
{
    auto red = Material::Ptr (new Material());
    red->baseColorFactor = { 1.0f, 0.0f, 0.0f, 1.0f };

    Scene scene;
    makeUnlit (scene);
    auto plate = addPlate (scene, red);

    SceneRenderer renderer;
    setTierForExactFrames (renderer, GetParam());

    // Nothing lights the plate
    auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 32, 32), "");

    plate.attach<MaterialShaderNode> (MaterialShader::createDefault());

    image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 32, 32), "r");
    EXPECT_EQ (channels (image, 4, 4), "");
}

TEST_P (MaterialShaderTierTests, SurfaceDescribesTheFragmentAndTheMaterialOfTheMesh)
{
    // Red on the right half of the world, green from the material, blue where the normal faces the viewer
    const auto shader = compile (R"(vec4 yupMaterial (YupSurface surface)
{
    float right = surface.position.x > 0.0 ? 1.0 : 0.0;
    float facing = dot (surface.normal, surface.view) > 0.9 ? 1.0 : 0.0;
    return vec4 (right, surface.baseColor.g * surface.roughness * surface.color.a, facing * surface.emissive.b, 1.0);
}
)");
    ASSERT_NE (shader, nullptr);

    auto material = Material::Ptr (new Material());
    material->baseColorFactor = { 0.0f, 1.0f, 0.0f, 1.0f };
    material->emissiveFactor = { 0.0f, 0.0f, 1.0f };
    material->doubleSided = true;

    Scene scene;
    makeUnlit (scene);
    auto plate = addPlate (scene, material);
    plate.attach<MaterialShaderNode> (shader);

    SceneRenderer renderer;
    setTierForExactFrames (renderer, GetParam());

    const auto front = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (front.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (front, 24, 32), "gb");
    EXPECT_EQ (channels (front, 40, 32), "rgb");

    // Seen from behind, the normal of a double sided material still faces the viewer
    plate.getOrAttach<TransformNode>().setRotation (Quaternion::fromAxisAngle ({ 0.0f, 1.0f, 0.0f }, MathConstants<float>::pi));

    const auto back = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (back.isValid()) << renderer.getLastError();
    EXPECT_EQ (countDifferences (back, front), 0);
}

TEST_P (MaterialShaderTierTests, NewValueOfAUniformIsDrawnAndRestartsTheConvergence)
{
    const auto shader = compile (tintSource);
    ASSERT_NE (shader, nullptr);

    Scene scene;
    makeUnlit (scene);
    auto plate = addPlate (scene, makeEmissive ({ 0.0f, 1.0f, 0.0f }));
    auto& node = attachTint (plate, shader, { 1.0f, 0.0f, 0.0f, 1.0f });

    SceneRenderer renderer;
    setTier (renderer, GetParam());

    auto image = renderConverged (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    ASSERT_TRUE (renderer.isConverged());
    EXPECT_EQ (leadingChannel (image, 32, 32), "r");

    // The same shader, node and mesh: only the value changed. The full tier restarts its accumulation
    EXPECT_TRUE (node.getUniforms().set ("tint", { 0.0f, 0.0f, 1.0f, 1.0f }));
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.isConverged(), GetParam() == RenderSettings::TierPreference::raster);

    image = renderConverged (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.isConverged());
    EXPECT_EQ (leadingChannel (image, 32, 32), "b");

    // Without the node the material shades the mesh again, which restarts it too
    EXPECT_TRUE (plate.detach<MaterialShaderNode>());
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();
    EXPECT_EQ (renderer.isConverged(), GetParam() == RenderSettings::TierPreference::raster);

    image = renderConverged (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (leadingChannel (image, 32, 32), "g");
}

TEST_P (MaterialShaderTierTests, NodesSharingAShaderKeepTheirOwnValues)
{
    const auto shader = compile (tintSource);
    ASSERT_NE (shader, nullptr);

    // Two quads of the shader around one of a standard material
    Scene scene;
    makeUnlit (scene);
    attachTint (addQuad (scene, -0.6f, 0.0f, makeEmissive ({ 0.0f, 1.0f, 0.0f })), shader, { 1.0f, 0.0f, 0.0f, 1.0f });
    addQuad (scene, 0.0f, 0.0f, makeEmissive ({ 0.0f, 1.0f, 0.0f }));
    attachTint (addQuad (scene, 0.6f, 0.0f, makeEmissive ({ 0.0f, 1.0f, 0.0f })), shader, { 0.0f, 0.0f, 1.0f, 1.0f });

    SceneRenderer renderer;
    setTierForExactFrames (renderer, GetParam());

    const auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 23, 32), "r");
    EXPECT_EQ (channels (image, 32, 32), "g");
    EXPECT_EQ (channels (image, 40, 32), "b");
}

TEST_P (MaterialShaderTierTests, BlendedMeshIsBlendedByTheAlphaTheShaderReturns)
{
    const auto shader = compile (tintSource);
    ASSERT_NE (shader, nullptr);

    // A blended material of alpha 1, which covers the blue background by itself
    Scene scene;
    makeUnlit (scene, Color (0xff0000ff));
    auto plate = addPlate (scene, makeEmissive ({ 0.0f, 1.0f, 0.0f }, Material::AlphaMode::blend));
    auto& node = attachTint (plate, shader, { 1.0f, 0.0f, 0.0f, 0.5f });

    SceneRenderer renderer;
    setTierForExactFrames (renderer, GetParam());

    auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();

    // Half of the red of the shader over half of the background. The raster tier blends after tone
    // mapping and the full tier before, so the mix differs between them
    const auto mixed = image.getPixelColor (32, 32);
    EXPECT_GT (mixed.getRed(), 60);
    EXPECT_LT (mixed.getRed(), 170);
    EXPECT_GT (mixed.getBlue(), 60);
    EXPECT_LT (mixed.getGreen(), 40);
    EXPECT_EQ (channels (image, 4, 4), "b");

    // An alpha of 1 covers the background, an alpha of 0 leaves it
    EXPECT_TRUE (node.getUniforms().set ("tint", { 1.0f, 0.0f, 0.0f, 1.0f }));
    image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 32, 32), "r");

    EXPECT_TRUE (node.getUniforms().set ("tint", { 1.0f, 0.0f, 0.0f, 0.0f }));
    image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 32, 32), "b");
}

TEST_P (MaterialShaderTierTests, MaskedMeshIsCutOutByTheAlphaTheShaderReturns)
{
    // No alpha on the left half of the world
    const auto shader = compile (R"(vec4 yupMaterial (YupSurface surface)
{
    return vec4 (1.0, 0.0, 0.0, surface.position.x < 0.0 ? 0.0 : 1.0);
}
)");
    ASSERT_NE (shader, nullptr);

    // The alpha of the material, 0.2 under its cutoff of 0.5, cuts the whole plate out
    Scene scene;
    makeUnlit (scene);
    auto plate = addPlate (scene, makeEmissive ({ 0.0f, 1.0f, 0.0f }, Material::AlphaMode::mask, 0.2f));

    SceneRenderer renderer;
    setTierForExactFrames (renderer, GetParam());

    auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 24, 32), "");
    EXPECT_EQ (channels (image, 40, 32), "");

    // The alpha of the shader is the one compared with the cutoff
    plate.attach<MaterialShaderNode> (shader);

    image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 24, 32), "");
    EXPECT_EQ (channels (image, 40, 32), "r");
}

TEST_P (MaterialShaderTierTests, OpaqueMeshOfAShaderIsDepthTestedAgainstStandardOnes)
{
    const auto shader = compile (tintSource);
    ASSERT_NE (shader, nullptr);

    // A green plate of a standard material, with a red quad of the shader in front of it and another behind it
    Scene scene;
    makeUnlit (scene);
    addPlate (scene, makeEmissive ({ 0.0f, 1.0f, 0.0f }));
    attachTint (addQuad (scene, 0.0f, 0.5f, makeEmissive ({ 0.0f, 0.0f, 1.0f })), shader, { 1.0f, 0.0f, 0.0f, 1.0f });
    attachTint (addQuad (scene, -0.6f, -0.5f, makeEmissive ({ 0.0f, 0.0f, 1.0f })), shader, { 1.0f, 0.0f, 0.0f, 1.0f });

    SceneRenderer renderer;
    setTierForExactFrames (renderer, GetParam());

    const auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 32, 44), "g");
    EXPECT_EQ (channels (image, 32, 32), "r");
    EXPECT_EQ (channels (image, 24, 32), "g");
}

TEST_P (MaterialShaderTierTests, OpaqueMeshOfAShaderWritesTheDepthStandardOnesAreTestedAgainst)
{
    const auto shader = compile (tintSource);
    ASSERT_NE (shader, nullptr);

    // A red plate of the shader, with a green quad of a standard material in front of it, another
    // behind it, and a blended one, drawn after the plate, behind it too
    Scene scene;
    makeUnlit (scene);
    attachTint (addPlate (scene, makeEmissive ({ 0.0f, 0.0f, 1.0f })), shader, { 1.0f, 0.0f, 0.0f, 1.0f });
    addQuad (scene, 0.0f, 0.5f, makeEmissive ({ 0.0f, 1.0f, 0.0f }));
    addQuad (scene, -0.6f, -0.5f, makeEmissive ({ 0.0f, 1.0f, 0.0f }));
    addQuad (scene, 0.6f, -0.5f, makeEmissive ({ 0.0f, 1.0f, 0.0f }, Material::AlphaMode::blend));

    SceneRenderer renderer;
    setTierForExactFrames (renderer, GetParam());

    const auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 32, 44), "r");
    EXPECT_EQ (channels (image, 32, 32), "g");
    EXPECT_EQ (channels (image, 24, 32), "r");
    EXPECT_EQ (channels (image, 40, 32), "r");
}

TEST_P (MaterialShaderTierTests, ShaderWithoutTheVariantOfTheTierLeavesTheMeshToItsMaterial)
{
    const auto shader = compile (tintSource);
    ASSERT_NE (shader, nullptr);

    const auto full = GetParam() == RenderSettings::TierPreference::full;
    const auto ofThisTier = MaterialShader::createFromBundles (full ? nullptr : shader->getRasterBundle(), full ? shader->getFullBundle() : nullptr);
    const auto ofTheOtherTier = MaterialShader::createFromBundles (full ? shader->getRasterBundle() : nullptr, full ? nullptr : shader->getFullBundle());
    ASSERT_TRUE (ofThisTier.wasOk());
    ASSERT_TRUE (ofTheOtherTier.wasOk());

    Scene scene;
    makeUnlit (scene);
    auto plate = addPlate (scene, makeEmissive ({ 0.0f, 1.0f, 0.0f }));
    auto& node = attachTint (plate, ofThisTier.getValue(), { 1.0f, 0.0f, 0.0f, 1.0f });

    SceneRenderer renderer;
    setTierForExactFrames (renderer, GetParam());

    auto image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 32, 32), "r");

    // Not a failure: the frame is the one of the material
    node.setShader (ofTheOtherTier.getValue());
    EXPECT_TRUE (node.getUniforms().set ("tint", { 1.0f, 0.0f, 0.0f, 1.0f }));

    image = renderToImage (renderer, scene, 64);
    ASSERT_TRUE (image.isValid()) << renderer.getLastError();
    EXPECT_TRUE (renderer.getLastError().isEmpty()) << renderer.getLastError();
    EXPECT_EQ (channels (image, 32, 32), "g");
}

TEST_P (MaterialShaderTierTests, OpaqueMeshOfAShaderWritesItsMotionOnTheFullTier)
{
    const auto shader = compile (tintSource);
    ASSERT_NE (shader, nullptr);

    Scene scene;
    makeUnlit (scene);
    auto plate = addPlate (scene, makeEmissive ({ 0.0f, 1.0f, 0.0f }));
    attachTint (plate, shader, { 1.0f, 0.0f, 0.0f, 1.0f });

    SceneRenderer renderer;
    setTier (renderer, GetParam());

    for (int frame = 0; frame < 2; ++frame)
        ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    if (GetParam() == RenderSettings::TierPreference::raster)
    {
        EXPECT_EQ (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), nullptr);
        return;
    }

    const auto still = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (still.isValid());
    EXPECT_EQ (maxVelocityDeviation (still), 0);

    // Velocity is current uv minus previous uv: the plate moving right moves x up, on the plate alone
    plate.getOrAttach<TransformNode>().setPosition ({ 0.1f, 0.0f, 0.0f });
    ASSERT_NE (renderer.render (device, scene, 64, 64), nullptr) << renderer.getLastError();

    const auto moved = readVelocity (renderer.getPassOutput (SceneRenderer::PassOutput::velocity), 1000.0f);
    ASSERT_TRUE (moved.isValid());
    EXPECT_GT (moved.getPixelColor (32, 32).getRed(), 192);
    EXPECT_EQ (velocityDeviation (moved.getPixelColor (32, 32).getGreen()), 0);
    EXPECT_EQ (velocityDeviation (moved.getPixelColor (4, 4).getRed()), 0);
}

INSTANTIATE_TEST_SUITE_P (MaterialShaderTiers,
                          MaterialShaderTierTests,
                          ::testing::Values (RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full),
                          [] (const auto& info)
                          {
                              return info.param == RenderSettings::TierPreference::full ? "full" : "raster";
                          });

#endif
