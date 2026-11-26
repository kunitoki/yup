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

#include <yup_gui/yup_gui.h>
#include <yup_gui_markup/yup_gui_markup.h>

#include <gtest/gtest.h>

#include <cstring>

using namespace yup;

//==============================================================================
// ShaderEffect - what needs no GPU: the values it gives a shader and the frames it asks for
//==============================================================================

class ShaderEffectTests : public ::testing::Test
{
protected:
    using Type = ComponentProperty::Type;
    using EffectPtr = ReferenceCountedObjectPtr<ShaderEffect>;

    /** Puts a component in a window of its own, whose frames are the ones a test sends and whose
        repaints it keeps.
    */
    static StubComponentNative& makeWindow (Component& component)
    {
        auto* native = new StubComponentNative (component, ComponentNative::defaultFlags);
        yup::ComponentTestHelper<Component>::attachNative (component, native);
        return *native;
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    static GpuImageShader::Ptr compile (const char* fragment)
    {
        const auto created = GpuImageShader::createFromGlsl (fragment);
        EXPECT_TRUE (created.wasOk()) << created.getErrorMessage();
        return created.wasOk() ? created.getValue() : nullptr;
    }

    /** A member for each type a property fills. */
    static constexpr const char* typesFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters
{
    vec4 tint;
    vec4 area;
    vec3 shade;
    vec2 origin;
    float amount;
    int steps;
    bool inverted;
} params;
layout(location = 0) out vec4 color;
void main()
{
    vec4 sum = params.tint + params.area + vec4 (params.shade, params.amount) + vec4 (params.origin, float (params.steps), 1.0);
    color = params.inverted ? vec4 (1.0) - sum : sum;
})";

    /** An array of scalars. */
    static constexpr const char* levelsFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters { float levels[3]; } params;
layout(location = 0) out vec4 color;
void main() { color = vec4 (params.levels[0], params.levels[1], params.levels[2], 1.0); })";

    /** Multiplies the picture of the component by a color. */
    static constexpr const char* tintFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters { vec4 tint; } params;
layout(set = 0, binding = 1) uniform texture2D source;
layout(set = 0, binding = 2) uniform sampler sourceSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = texture (sampler2D (source, sourceSampler), uv) * params.tint; })";

    /** The picture of the component, where the series sampled along its width is above a half. */
    static constexpr const char* gateFragment = R"(#version 450
layout(set = 0, binding = 0) uniform texture2D source;
layout(set = 0, binding = 1) uniform sampler sourceSampler;
layout(set = 0, binding = 2) uniform texture2D gates;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    float gate = texture (sampler2D (gates, sourceSampler), vec2 (uv.x, 0.5)).r;
    color = texture (sampler2D (source, sourceSampler), uv) * step (0.5, gate);
})";

    /** Fills with its color, reading no picture. */
    static constexpr const char* fillFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters { vec4 tint; } params;
layout(location = 0) out vec4 color;
void main() { color = params.tint; })";

    /** Red once a quarter of a second went by, green once three quarters did. */
    static constexpr const char* timeFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters { float time; } params;
layout(location = 0) out vec4 color;
void main() { color = vec4 (step (0.25, params.time), step (0.75, params.time), 0.0, 1.0); })";

    /** Red for a picture of 64 by 32 pixels, green for the reciprocals of that size, blue for 2 pixels a unit. */
    static constexpr const char* resolutionFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters
{
    vec4 resolution;
    float scale;
} params;
layout(location = 0) out vec4 color;
void main()
{
    float size = (1.0 - step (0.5, abs (params.resolution.x - 64.0))) * (1.0 - step (0.5, abs (params.resolution.y - 32.0)));
    float reciprocal = (1.0 - step (0.001, abs (params.resolution.z * 64.0 - 1.0))) * (1.0 - step (0.001, abs (params.resolution.w * 32.0 - 1.0)));
    float pixels = 1.0 - step (0.01, abs (params.scale - 2.0));
    color = vec4 (size, reciprocal, pixels, 1.0);
})";
#endif
};

TEST_F (ShaderEffectTests, TellsHowManyScalarsAPropertyNeedsOfAMember)
{
    EXPECT_EQ (ShaderEffect::getNumUniformComponents (Type::number), 1);
    EXPECT_EQ (ShaderEffect::getNumUniformComponents (Type::integer), 1);
    EXPECT_EQ (ShaderEffect::getNumUniformComponents (Type::boolean), 1);
    EXPECT_EQ (ShaderEffect::getNumUniformComponents (Type::array), 1);
    EXPECT_EQ (ShaderEffect::getNumUniformComponents (Type::point), 2);
    EXPECT_EQ (ShaderEffect::getNumUniformComponents (Type::color), 3);
    EXPECT_EQ (ShaderEffect::getNumUniformComponents (Type::rectangle), 4);

    for (const auto type : { Type::string, Type::transform, Type::enumeration, Type::font, Type::drawing, Type::any })
        EXPECT_EQ (ShaderEffect::getNumUniformComponents (type), 0);
}

TEST_F (ShaderEffectTests, WithoutAGpuNothingIsDrawnAndNothingFails)
{
    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    ASSERT_NE (context, nullptr);
    auto renderer = context->makeRenderer (64, 64);
    ASSERT_NE (renderer, nullptr);
    Graphics g (*context, *renderer);

    Component screen ("screen");
    screen.setBounds (0.0f, 0.0f, 64.0f, 64.0f);

    const EffectPtr effect (new ShaderEffect (screen, nullptr, { { "tint", Type::color } }));
    screen.setComponentEffect (effect);
    EXPECT_EQ (effect->getShader(), nullptr);

    // No shader, no picture and no GPU to make one with
    effect->apply (g, nullptr, screen.getLocalBounds());
    EXPECT_FALSE (screen.snapshotToImage (*context, true).isValid());

#if YUP_ENABLE_SHADER_TRANSPILER
    const auto shader = compile (tintFragment);
    effect->setShader (shader);
    EXPECT_EQ (effect->getShader(), shader);

    effect->apply (g, nullptr, screen.getLocalBounds());
    EXPECT_FALSE (screen.snapshotToImage (*context, true).isValid());
#endif
}

#if YUP_ENABLE_SHADER_TRANSPILER

TEST_F (ShaderEffectTests, WritesAPropertyToTheMemberOfItsNameByItsType)
{
    const auto shader = compile (typesFragment);
    ASSERT_NE (shader, nullptr);

    auto uniforms = shader->createUniforms();
    auto expected = shader->createUniforms();

    // A color as it is written, not premultiplied: red, green, blue and alpha from 0 to 1
    const float tint[] = { 1.0f, 0.0f, 0.0f, 0.0f };
    EXPECT_TRUE (ShaderEffect::setUniform (uniforms, { "tint", Type::color }, VariantConverter<Color>::toVar (Color (0x00ff0000))));
    EXPECT_TRUE (expected.set ("tint", tint));

    // A vec3 takes it without its alpha
    const float shade[] = { 1.0f, 1.0f, 0.0f };
    EXPECT_TRUE (ShaderEffect::setUniform (uniforms, { "shade", Type::color }, VariantConverter<Color>::toVar (Color (0xffffff00))));
    EXPECT_TRUE (expected.set ("shade", shade));

    const float area[] = { 1.0f, 2.0f, 3.0f, 4.0f };
    EXPECT_TRUE (ShaderEffect::setUniform (uniforms, { "area", Type::rectangle }, VariantConverter<Rectangle<float>>::toVar ({ 1.0f, 2.0f, 3.0f, 4.0f })));
    EXPECT_TRUE (expected.set ("area", area));

    const float origin[] = { 5.0f, 6.0f };
    EXPECT_TRUE (ShaderEffect::setUniform (uniforms, { "origin", Type::point }, VariantConverter<Point<float>>::toVar ({ 5.0f, 6.0f })));
    EXPECT_TRUE (expected.set ("origin", origin));

    EXPECT_TRUE (ShaderEffect::setUniform (uniforms, { "amount", Type::number }, 0.5));
    EXPECT_TRUE (expected.set ("amount", 0.5f));

    // An integer is cut to a whole number, where the member alone would round it
    EXPECT_TRUE (ShaderEffect::setUniform (uniforms, { "steps", Type::integer }, 2.75));
    EXPECT_TRUE (expected.set ("steps", 2));

    EXPECT_TRUE (ShaderEffect::setUniform (uniforms, { "inverted", Type::boolean }, true));
    EXPECT_TRUE (expected.set ("inverted", true));

    EXPECT_TRUE (uniforms == expected);
    EXPECT_FALSE (uniforms == shader->createUniforms());

    // What can't be written leaves the uniforms as they are
    EXPECT_FALSE (ShaderEffect::setUniform (uniforms, { "missing", Type::number }, 1.0));
    EXPECT_FALSE (ShaderEffect::setUniform (uniforms, { "amount", Type::string }, "high"));
    EXPECT_FALSE (ShaderEffect::setUniform (uniforms, { "origin", Type::point }, "here"));
    EXPECT_TRUE (uniforms == expected);

    // A property the component has no value for is zero
    auto unset = shader->createUniforms();
    ShaderEffect::setUniform (unset, { "tint", Type::color }, var());
    ShaderEffect::setUniform (unset, { "area", Type::rectangle }, var());
    ShaderEffect::setUniform (unset, { "amount", Type::number }, var());
    ShaderEffect::setUniform (unset, { "inverted", Type::boolean }, var());
    EXPECT_TRUE (unset == shader->createUniforms());
}

TEST_F (ShaderEffectTests, WritesAnArrayOfNumbersToAnArray)
{
    const auto shader = compile (levelsFragment);
    ASSERT_NE (shader, nullptr);

    auto uniforms = shader->createUniforms();
    auto expected = shader->createUniforms();

    const float levels[] = { 0.25f, 0.5f, 1.0f };
    EXPECT_TRUE (ShaderEffect::setUniform (uniforms, { "levels", Type::array }, var (Array<var> { 0.25, 0.5, 1.0 })));
    EXPECT_TRUE (expected.set ("levels", levels));
    EXPECT_TRUE (uniforms == expected);

    EXPECT_FALSE (ShaderEffect::setUniform (uniforms, { "levels", Type::array }, var (Array<var> { "low", "high" })));
    EXPECT_TRUE (uniforms == expected);
}

TEST_F (ShaderEffectTests, AShaderGivenTheTimeRepaintsItsComponentAtEveryFrame)
{
    Component window ("window");
    auto& native = makeWindow (window);
    window.setBounds (0.0f, 0.0f, 64.0f, 64.0f);
    window.setVisible (true);

    Component screen ("screen");
    screen.setBounds (8.0f, 8.0f, 32.0f, 32.0f);
    window.addAndMakeVisible (screen);

    const auto timed = compile (timeFragment);
    const auto still = compile (fillFragment);
    ASSERT_NE (timed, nullptr);
    ASSERT_NE (still, nullptr);

    {
        const EffectPtr effect (new ShaderEffect (screen, still, { { "tint", Type::color } }));
        screen.setComponentEffect (effect);

        // A shader that is not given the time asks for no frame
        EXPECT_FALSE (native.hasFrameListeners());

        effect->setShader (timed);
        EXPECT_TRUE (native.hasFrameListeners());

        native.repaintAreas.clearQuick();
        native.callFrameListeners (0.25);
        EXPECT_GT (native.getRepaintAreas().getNumRectangles(), 0);

        // Nothing is repainted while the component is hidden
        screen.setVisible (false);
        native.repaintAreas.clearQuick();
        native.callFrameListeners (0.25);
        EXPECT_EQ (native.getRepaintAreas().getNumRectangles(), 0);

        // Nor once the component has another effect
        screen.setVisible (true);
        screen.setComponentEffect (nullptr);
        native.repaintAreas.clearQuick();
        native.callFrameListeners (0.25);
        EXPECT_EQ (native.getRepaintAreas().getNumRectangles(), 0);

        effect->setShader (still);
        EXPECT_FALSE (native.hasFrameListeners());

        effect->setShader (timed);
        EXPECT_TRUE (native.hasFrameListeners());
    }

    // The frames go with the effect
    EXPECT_FALSE (native.hasFrameListeners());

    // A property named as the time takes its place: the component sets it, and no frame is asked for
    const EffectPtr driven (new ShaderEffect (screen, timed, { { "time", Type::number } }));
    EXPECT_FALSE (native.hasFrameListeners());
}

#endif // YUP_ENABLE_SHADER_TRANSPILER

//==============================================================================
// ShaderEffect - shaders run by Metal, read back from the picture of a component
//==============================================================================

#if YUP_MAC && YUP_ENABLE_SHADER_TRANSPILER

class ShaderEffectMetalTests : public ShaderEffectTests
{
protected:
    /** Fills itself with a color. Unclipped, as the components ComponentEffectGpuTest paints. */
    class Filled : public Component
    {
    public:
        Filled (StringRef componentID, Color colorToFill)
            : Component (componentID)
            , fill (colorToFill)
        {
            setBounds (0.0f, 0.0f, 32.0f, 16.0f);
            enableRenderingUnclipped (true);
        }

        void paint (Graphics& g) override
        {
            g.setFillColor (fill);
            g.fillAll();
        }

        Color fill;
    };

    /** The shaders of Rive are compiled synchronously, as in ComponentEffectGpuTest: what its
        fallback shaders draw in the meantime is a level off.
    */
    static void SetUpTestSuite()
    {
        GraphicsContext::Options options;
        options.synchronousShaderCompilations = true;

        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, options);
        if (gpuContext == nullptr)
            return;

        auto probe = GpuCanvas::create (*gpuContext, 64, 64);
        if (probe == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";
    }

    /** Draws a component through its effect, and expects a color at a pixel of the picture. */
    static void expectPixel (Component& component, int x, int y, Color expected, float scale = 1.0f)
    {
        const auto image = component.snapshotToImage (*gpuContext, true, scale);
        ASSERT_TRUE (image.isValid());

        const auto pixel = image.getPixelColor (x, y);
        const auto where = String (x) + ", " + String (y);

        EXPECT_NEAR (pixel.getRed(), expected.getRed(), 2) << "red at " << where;
        EXPECT_NEAR (pixel.getGreen(), expected.getGreen(), 2) << "green at " << where;
        EXPECT_NEAR (pixel.getBlue(), expected.getBlue(), 2) << "blue at " << where;
        EXPECT_NEAR (pixel.getAlpha(), expected.getAlpha(), 2) << "alpha at " << where;
    }

    /** A shader that is given the time and that Metal can't draw: its bundle, made by hand, has a GLSL variant only. */
    static GpuImageShader::Ptr makeShaderWithoutAVariantForMetal()
    {
        ShaderReflection::ResourceMember time;
        time.name = "time";
        time.size = 4;
        time.baseType = ShaderReflection::BaseType::float32;

        ShaderReflection::ResourceBinding block;
        block.name = "Parameters";
        block.type = ShaderReflection::ResourceType::uniformBuffer;
        block.members = { time };
        block.blockSize = 4;

        ShaderBundle::Ptr bundle (new ShaderBundle());

        for (const auto stage : { ShaderStage::vertex, ShaderStage::fragment })
        {
            ShaderInfo info;
            info.stage = stage;
            info.source = "void main() {}";

            if (stage == ShaderStage::fragment)
                info.reflection.uniformBuffers = { block };

            bundle->addShader (std::move (info));
        }

        const auto created = GpuImageShader::createFromBundle (bundle);
        EXPECT_TRUE (created.wasOk()) << created.getErrorMessage();
        return created.wasOk() ? created.getValue() : nullptr;
    }

    static std::unique_ptr<GraphicsContext> gpuContext;
};

std::unique_ptr<GraphicsContext> ShaderEffectMetalTests::gpuContext;

TEST_F (ShaderEffectMetalTests, TintsAComponentByAColorFromARuleAnExplicitValueAndTheDefault)
{
    YuiDocument document;
    document.setAssetReader ([] (const String& path) -> std::optional<MemoryBlock>
    {
        if (path == "fx/tint.frag")
            return MemoryBlock (tintFragment, std::strlen (tintFragment));

        return std::nullopt;
    });

    const auto loaded = document.loadFromString (R"yui(
        shader Tint { fragment: "fx/tint.frag"; property tint: color = #0000ff; }

        style {
            #screen     { effect: Tint; }
            #screen.red { tint: #ff0000; }
        }

        Component { })yui");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

    // A component made in code, styled by the document it is added to, which gives it its shader
    auto* root = document.getRoot();
    Filled screen ("screen", Colors::white);
    root->addAndMakeVisible (screen);
    root->getStyleContext()->restyleNow();

    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
    ASSERT_NE (dynamic_cast<ShaderEffect*> (screen.getComponentEffect().get()), nullptr);

    // The default of the block
    expectPixel (screen, 16, 8, Color (0xff0000ff));

    // A rule
    screen.addStyleClass (Identifier ("red"));
    root->getStyleContext()->restyleNow();
    expectPixel (screen, 16, 8, Color (0xffff0000));

    // A value set on the component
    screen.setProperty (Identifier ("tint"), VariantConverter<Color>::toVar (Color (0xff00ff00)));
    expectPixel (screen, 16, 8, Color (0xff00ff00));

    // And the default again
    screen.removeProperty (Identifier ("tint"));
    screen.removeStyleClass (Identifier ("red"));
    root->getStyleContext()->restyleNow();
    expectPixel (screen, 16, 8, Color (0xff0000ff));
}

TEST_F (ShaderEffectMetalTests, GivesAShaderTheSizeAndTheScaleOfThePicture)
{
    const auto shader = compile (resolutionFragment);
    ASSERT_NE (shader, nullptr);

    Filled screen ("screen", Colors::black);
    screen.setComponentEffect (new ShaderEffect (screen, shader));

    // 32 by 16 units at 2 pixels a unit
    expectPixel (screen, 32, 16, Colors::white, 2.0f);

    // A property takes the place of the member of its name the effect fills
    screen.setComponentEffect (new ShaderEffect (screen, shader, { { "scale", Type::number } }));
    expectPixel (screen, 32, 16, Color (0xffffff00), 2.0f);

    screen.setProperty (Identifier ("scale"), 2.0);
    expectPixel (screen, 32, 16, Colors::white, 2.0f);
}

TEST_F (ShaderEffectMetalTests, AShaderReadingNoPictureFillsTheBounds)
{
    const auto shader = compile (fillFragment);
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (shader->getNumInputTextures(), 0);

    Filled screen ("screen", Color (0xffff0000));
    screen.setComponentEffect (new ShaderEffect (screen, shader, { { "tint", Type::color } }));
    screen.setProperty (Identifier ("tint"), VariantConverter<Color>::toVar (Color (0xff00ff00)));

    // What the component painted is not shown
    for (const auto point : { Point<int> (1, 1), Point<int> (16, 8), Point<int> (30, 14) })
        expectPixel (screen, point.getX(), point.getY(), Color (0xff00ff00));
}

TEST_F (ShaderEffectMetalTests, AnArrayFillsTheTextureOfItsNameBesideThePictureOfTheComponent)
{
    const auto shader = compile (gateFragment);
    ASSERT_NE (shader, nullptr);
    EXPECT_EQ (shader->getInputTextureNames(), StringArray ({ "source", "gates" }));

    // The texture an array names is no picture: the shader still reads one, the component
    const std::vector<ShaderEffect::Property> properties { { "gates", Type::array } };
    EXPECT_EQ (ShaderEffect::getNumPictures (*shader, properties), 1);
    EXPECT_EQ (ShaderEffect::getNumPictures (*shader, {}), 2);

    Filled screen ("screen", Color (0xff0000ff));
    screen.setComponentEffect (new ShaderEffect (screen, shader, properties));

    // Open on the left, closed on the right
    screen.setProperty (Identifier ("gates"), var (Array<var> { 1.0, 1.0, 0.0, 0.0 }));
    expectPixel (screen, 2, 8, Color (0xff0000ff));
    expectPixel (screen, 29, 8, Colors::transparentBlack);

    screen.setProperty (Identifier ("gates"), var (Array<var> { 0.0, 0.0, 1.0, 1.0 }));
    expectPixel (screen, 2, 8, Colors::transparentBlack);
    expectPixel (screen, 29, 8, Color (0xff0000ff));

    // Without the property the texture is a second picture, which an effect doesn't have: the component is drawn as it is
    screen.setComponentEffect (new ShaderEffect (screen, shader));
    expectPixel (screen, 2, 8, Color (0xff0000ff));
    expectPixel (screen, 29, 8, Color (0xff0000ff));
}

TEST_F (ShaderEffectMetalTests, WithoutAShaderTheComponentIsDrawnAsItIs)
{
    Filled screen ("screen", Color (0xff0000ff));
    screen.setComponentEffect (new ShaderEffect (screen, nullptr, { { "tint", Type::color } }));

    expectPixel (screen, 16, 8, Color (0xff0000ff));
}

TEST_F (ShaderEffectMetalTests, TheTimeOfAShaderGoesByWithTheFramesOfItsWindow)
{
    const auto shader = compile (timeFragment);
    ASSERT_NE (shader, nullptr);

    Component window ("window");
    auto& native = makeWindow (window);
    window.setBounds (0.0f, 0.0f, 64.0f, 64.0f);
    window.setVisible (true);

    Filled screen ("screen", Colors::white);
    window.addAndMakeVisible (screen);
    screen.setComponentEffect (new ShaderEffect (screen, shader));

    expectPixel (screen, 16, 8, Colors::black);

    native.callFrameListeners (0.5);
    expectPixel (screen, 16, 8, Color (0xffff0000));

    native.callFrameListeners (0.5);
    expectPixel (screen, 16, 8, Color (0xffffff00));

    // The time stands still while the component is hidden
    screen.setVisible (false);
    native.callFrameListeners (3600.0);
    screen.setVisible (true);
    expectPixel (screen, 16, 8, Color (0xffffff00));

    // And starts again after an hour
    native.callFrameListeners (3599.0);
    expectPixel (screen, 16, 8, Colors::black);
}

TEST_F (ShaderEffectMetalTests, AShaderThatCantBeDrawnStopsRepaintingItsComponent)
{
    const auto undrawable = makeShaderWithoutAVariantForMetal();
    const auto timed = compile (timeFragment);
    ASSERT_NE (undrawable, nullptr);
    ASSERT_NE (timed, nullptr);

    Component window ("window");
    auto& native = makeWindow (window);
    window.setBounds (0.0f, 0.0f, 64.0f, 64.0f);
    window.setVisible (true);

    Filled screen ("screen", Color (0xff0000ff));
    window.addAndMakeVisible (screen);

    const EffectPtr effect (new ShaderEffect (screen, undrawable));
    screen.setComponentEffect (effect);
    EXPECT_TRUE (native.hasFrameListeners());

    const auto countRepaintsOfAFrame = [&native]
    {
        native.repaintAreas.clearQuick();
        native.callFrameListeners (0.25);
        return native.getRepaintAreas().getNumRectangles();
    };

    // Until it was painted nothing tells that the shader can't be drawn, so the time goes by
    EXPECT_GT (countRepaintsOfAFrame(), 0);

    // The component is drawn as it is, and no frame repaints it for a shader that draws nothing
    expectPixel (screen, 16, 8, Color (0xff0000ff));
    EXPECT_EQ (countRepaintsOfAFrame(), 0);

    expectPixel (screen, 16, 8, Color (0xff0000ff));
    EXPECT_EQ (countRepaintsOfAFrame(), 0);

    // Another shader is drawn, and asks for its frames: a quarter of a second went by before, and one does now
    effect->setShader (timed);
    EXPECT_GT (countRepaintsOfAFrame(), 0);
    expectPixel (screen, 16, 8, Color (0xffff0000));

    // The shader that can't be drawn is tried again when it is set again, and forgotten again
    effect->setShader (undrawable);
    EXPECT_GT (countRepaintsOfAFrame(), 0);
    expectPixel (screen, 16, 8, Color (0xff0000ff));
    EXPECT_EQ (countRepaintsOfAFrame(), 0);
}

#endif // YUP_MAC && YUP_ENABLE_SHADER_TRANSPILER
