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

#endif
