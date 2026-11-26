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

/** What a `Material3D` naming a shader shows, drawn by a Metal device on both tiers of the
    renderer, with the material compiled from its source.

    The scenes are 32 pixels wide, seen through the camera the renderer draws a scene without one
    with: a quad 2 wide around the origin covers their middle.
*/
class SceneMaterialShaderMetalTests : public SceneDocumentTests
{
protected:
    void SetUp() override
    {
        SceneDocumentTests::SetUp();

        device = GpuDevice::create (GpuPlatform::Metal, {});
        if (device == nullptr)
            GTEST_SKIP() << "No Metal device available";

        context = GraphicsContext::createContext (GpuPlatform::Metal, {}, device);
        if (context == nullptr || ! context->isGpuAvailable())
            GTEST_SKIP() << "No Metal graphics context available";

        // A material showing the color and the alpha of its `tint`
        files["fx/tint.glsl"] = bytesOf (R"(YUP_MATERIAL_PARAMS
{
    vec4 tint;
} params;

vec4 yupMaterial (YupSurface surface)
{
    return params.tint;
}
)");
    }

    /** Runs the display frame of a scene, then paints it over a canvas of its size and reads the canvas back. */
    Image paintFrame (SceneComponent& view)
    {
        view.refreshDisplay (0.0);

        auto canvas = GpuCanvas::create (*context, 32, 32);
        if (canvas == nullptr)
            return {};

        view.paint (canvas->beginDraw());

        EXPECT_TRUE (view.getRenderer().getLastError().isEmpty()) << view.getRenderer().getLastError();
        EXPECT_TRUE (view.getRenderer().waitForGPU().wasOk());
        return canvas->asImage();
    }

    /** The channel of a pixel leading the two others by 100 levels, "r", "g" or "b", or "?" without
        one: what a frame shows whatever the tone mapping, and what is left of the frames before it.
    */
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

    GpuDevice::Ptr device;
    std::unique_ptr<GraphicsContext> context;
};

TEST_F (SceneMaterialShaderMetalTests, AMaterialShowsTheColorItsPropertyGivesItsShader)
{
    for (const auto tier : { RenderSettings::TierPreference::raster, RenderSettings::TierPreference::full })
    {
        SCOPED_TRACE (tier == RenderSettings::TierPreference::full ? "full" : "raster");

        YuiDocument document;
        serve (document);
        load (document, "shader Tint { material: \"fx/tint.glsl\"; property tint: color = #ff0000; }\n"
                        "Scene3D { id: view; Mesh3D { "
                            + quadGeometry()
                            + " Material3D { id: paint; shader: Tint; } } }");

        auto* view = document.findById<SceneComponent> ("view");
        auto* material = document.findById<SceneMaterialComponent> ("paint");

        ASSERT_NE (view, nullptr);
        ASSERT_NE (material, nullptr);

        auto settings = view->getRenderer().getRenderSettings();
        settings.tier = tier;
        view->getRenderer().setRenderSettings (settings);
        view->setVisible (true);
        view->setBounds (0.0f, 0.0f, 32.0f, 32.0f);

        // The frame of the scene compiles the material and gives it the default of the block
        auto image = paintFrame (*view);
        ASSERT_TRUE (image.isValid());
        ASSERT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
        EXPECT_EQ (leadingChannel (image, 16, 16), "r");

        // Another color shows by the frames that follow, which start accumulating over
        material->setProperty (Identifier ("tint"), VariantConverter<Color>::toVar (Color (0xff0000ff)));

        for (int frame = 0; frame < 4; ++frame)
            image = paintFrame (*view);

        ASSERT_TRUE (image.isValid());
        EXPECT_EQ (leadingChannel (image, 16, 16), "b");
    }
}

TEST_F (SceneMaterialShaderMetalTests, AMaterialTheRendererCanNotDrawWithIsReportedOnceAtItsBlock)
{
    // What a build embedding the variant of one tier leaves: the material compiled for the raster tier alone
    const auto compiled = MaterialShader::createFromSource (files["fx/tint.glsl"].toString());
    ASSERT_TRUE (compiled.wasOk()) << compiled.getErrorMessage();
    ASSERT_NE (compiled.getReference()->getRasterBundle(), nullptr);

    MemoryBlock raster;
    ASSERT_TRUE (compiled.getReference()->getRasterBundle()->saveToMemoryBlock (raster, ShaderBundle::Compression::none).wasOk());
    files["fx/tint.glsl.raster.ysl"] = raster;

    const auto text = "shader Tint {\n"
                      "    material: \"fx/tint.glsl\";\n"
                      "    property tint: color = #ff0000;\n"
                      "}\n"
                      "Scene3D {\n"
                      "    id: view;\n"
                      "    Mesh3D { "
                    + quadGeometry()
                    + " Material3D { shader: Tint; } }\n"
                      "    Mesh3D { position: [0, 0, -1]; "
                    + quadGeometry()
                    + " Material3D { shader: Tint; } }\n"
                      "}";

    YuiDocument document;
    serve (document);

    int numTold = 0;
    DiagnosticListener listener (document, [&numTold] (const DataTreeDiagnostic&)
    {
        ++numTold;
    });

    ASSERT_TRUE (document.loadFromString (text).wasOk());
    ASSERT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());

    auto* view = document.findById<SceneComponent> ("view");
    ASSERT_NE (view, nullptr);

    auto settings = view->getRenderer().getRenderSettings();
    settings.tier = RenderSettings::TierPreference::full;
    view->getRenderer().setRenderSettings (settings);
    view->setVisible (true);
    view->setBounds (0.0f, 0.0f, 32.0f, 32.0f);

    // The painting of the scene finds that the material has nothing to draw with on the full tier, and leaves
    // the meshes to their materials
    ASSERT_TRUE (paintFrame (*view).isValid());

    const auto* capabilities = view->getRenderer().getCapabilities();
    ASSERT_NE (capabilities, nullptr);

    if (capabilities->getTier() != RenderCapabilities::Tier::full)
        GTEST_SKIP() << "No full tier on this device";

    EXPECT_EQ (numTold, 0) << DataTreeDiagnostic::join (document.getDiagnostics());

    // The frame that follows tells it where the block names the file: once, for the two materials naming the shader
    ASSERT_TRUE (paintFrame (*view).isValid());

    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());

    const auto problem = document.getDiagnostics().front();
    EXPECT_EQ (problem.message, "shader Tint: the renderer can't draw with it, which leaves the meshes to their material: it has no full tier variant");
    EXPECT_EQ (problem.line, 2);
    EXPECT_EQ (problem.property.toString(), "material");
    EXPECT_TRUE (problem.isError());
    EXPECT_EQ (numTold, 1);

    // The frames that follow have nothing to add
    ASSERT_TRUE (paintFrame (*view).isValid());
    EXPECT_EQ (numTold, 1);

    // It stands: a document forgets what it was told when it loads, and is told again
    ASSERT_TRUE (document.loadFromString (text).wasOk());
    ASSERT_EQ (document.findById<SceneComponent> ("view"), view);
    ASSERT_TRUE (paintFrame (*view).isValid());

    ASSERT_EQ (document.getDiagnostics().size(), 1u) << DataTreeDiagnostic::join (document.getDiagnostics());
    EXPECT_EQ (document.getDiagnostics().front().message, problem.message);
    EXPECT_EQ (document.getDiagnostics().front().line, 2);
}

#endif
