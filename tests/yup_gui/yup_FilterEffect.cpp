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

using namespace yup;

class FilterEffectTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        theme = new ApplicationTheme();
        ApplicationTheme::setGlobalTheme (theme);
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        theme = nullptr;
        oldTheme = nullptr;
    }

    static StyleSheet::Ptr buildStyle (const String& yui)
    {
        auto sheet = YuiParser::parseStyleSheet (yui);
        EXPECT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
        return sheet.wasOk() ? sheet.getReference() : nullptr;
    }

    static void attachStyle (Component& root, const String& yui)
    {
        root.setStyleSheet (buildStyle (yui));

        if (auto* context = root.getStyleContext())
            context->restyleNow();
    }

    static void restyle (Component& root)
    {
        if (auto* context = root.getStyleContext())
            context->restyleNow();
    }

    static void advance (Component& root, double elapsedSeconds)
    {
        if (auto* context = root.getStyleContext())
            context->refresh (elapsedSeconds);
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (FilterEffectTests, ParsesStandardCssFilterFunctionsAndDefaults)
{
    const auto noneResult = FilterEffect::parse ("none");
    ASSERT_TRUE (noneResult.wasOk()) << noneResult.getErrorMessage();
    EXPECT_TRUE (noneResult.getReference().empty());

    const auto defaultsResult = FilterEffect::parse ("blur() brightness() contrast() grayscale() hue-rotate() invert() opacity() saturate() sepia()");
    ASSERT_TRUE (defaultsResult.wasOk()) << defaultsResult.getErrorMessage();

    const auto& defaults = defaultsResult.getReference();
    ASSERT_EQ (defaults.size(), 9u);
    EXPECT_EQ (defaults[0].type, FilterEffect::Type::blur);
    EXPECT_FLOAT_EQ (defaults[0].amount, 0.0f);
    EXPECT_EQ (defaults[1].type, FilterEffect::Type::brightness);
    EXPECT_FLOAT_EQ (defaults[1].amount, 1.0f);
    EXPECT_EQ (defaults[2].type, FilterEffect::Type::contrast);
    EXPECT_FLOAT_EQ (defaults[2].amount, 1.0f);
    EXPECT_EQ (defaults[3].type, FilterEffect::Type::grayscale);
    EXPECT_FLOAT_EQ (defaults[3].amount, 1.0f);
    EXPECT_EQ (defaults[4].type, FilterEffect::Type::hueRotate);
    EXPECT_FLOAT_EQ (defaults[4].amount, 0.0f);
    EXPECT_EQ (defaults[5].type, FilterEffect::Type::invert);
    EXPECT_FLOAT_EQ (defaults[5].amount, 1.0f);
    EXPECT_EQ (defaults[6].type, FilterEffect::Type::opacity);
    EXPECT_FLOAT_EQ (defaults[6].amount, 1.0f);
    EXPECT_EQ (defaults[7].type, FilterEffect::Type::saturate);
    EXPECT_FLOAT_EQ (defaults[7].amount, 1.0f);
    EXPECT_EQ (defaults[8].type, FilterEffect::Type::sepia);
    EXPECT_FLOAT_EQ (defaults[8].amount, 1.0f);
}

TEST_F (FilterEffectTests, ParsesUnitsPercentagesClampingAndDropShadows)
{
    const auto chainResult = FilterEffect::parse (
        "blur(6px) brightness(125%) contrast(0.8) grayscale(150%) "
        "hue-rotate(90deg) hue-rotate(0.25turn) hue-rotate(100grad) hue-rotate(1.5rad) hue-rotate(0) "
        "invert(200%) opacity(40%) saturate(250%) sepia(180%) "
        "drop-shadow(2px -4px 8px #ff000080) drop-shadow(#00ff00ff 3 5)");
    ASSERT_TRUE (chainResult.wasOk()) << chainResult.getErrorMessage();

    const auto& ops = chainResult.getReference();
    ASSERT_EQ (ops.size(), 15u);

    EXPECT_FLOAT_EQ (ops[0].amount, 6.0f);
    EXPECT_FLOAT_EQ (ops[1].amount, 1.25f);
    EXPECT_FLOAT_EQ (ops[2].amount, 0.8f);
    EXPECT_FLOAT_EQ (ops[3].amount, 1.0f);
    EXPECT_NEAR (ops[4].amount, MathConstants<float>::halfPi, 1.0e-5f);
    EXPECT_NEAR (ops[5].amount, MathConstants<float>::halfPi, 1.0e-5f);
    EXPECT_NEAR (ops[6].amount, MathConstants<float>::halfPi, 1.0e-5f);
    EXPECT_FLOAT_EQ (ops[7].amount, 1.5f);
    EXPECT_FLOAT_EQ (ops[8].amount, 0.0f);
    EXPECT_FLOAT_EQ (ops[9].amount, 1.0f);
    EXPECT_FLOAT_EQ (ops[10].amount, 0.4f);
    EXPECT_FLOAT_EQ (ops[11].amount, 2.5f);
    EXPECT_FLOAT_EQ (ops[12].amount, 1.0f);

    EXPECT_EQ (ops[13].type, FilterEffect::Type::dropShadow);
    EXPECT_EQ (ops[13].offset, Point<float> (2.0f, -4.0f));
    EXPECT_FLOAT_EQ (ops[13].amount, 8.0f);
    EXPECT_EQ (ops[13].color, Color (0x80ff0000));

    EXPECT_EQ (ops[14].type, FilterEffect::Type::dropShadow);
    EXPECT_EQ (ops[14].offset, Point<float> (3.0f, 5.0f));
    EXPECT_FLOAT_EQ (ops[14].amount, 0.0f);
    EXPECT_EQ (ops[14].color, Color (0xff00ff00));
}

TEST_F (FilterEffectTests, RejectsInvalidCssFilterSyntax)
{
    EXPECT_TRUE (FilterEffect::parse ("sharpen(2)").failed());
    EXPECT_TRUE (FilterEffect::parse ("blur(-2px)").failed());
    EXPECT_TRUE (FilterEffect::parse ("blur(50%)").failed());
    EXPECT_TRUE (FilterEffect::parse ("brightness(-0.5)").failed());
    EXPECT_TRUE (FilterEffect::parse ("contrast(-10%)").failed());
    EXPECT_TRUE (FilterEffect::parse ("grayscale(-1)").failed());
    EXPECT_TRUE (FilterEffect::parse ("saturate(-1)").failed());
    EXPECT_TRUE (FilterEffect::parse ("hue-rotate(90)").failed());
    EXPECT_TRUE (FilterEffect::parse ("drop-shadow(4px)").failed());
    EXPECT_TRUE (FilterEffect::parse ("drop-shadow(0 4 -2 #000000)").failed());
    EXPECT_TRUE (FilterEffect::parse ("drop-shadow(0 4 8 2 #000000)").failed());
    EXPECT_TRUE (FilterEffect::parse ("blur(4").failed());
    EXPECT_TRUE (FilterEffect::parse ("blur(4) trailing").failed());
}

TEST_F (FilterEffectTests, BuiltInEffectClassesConstructAndConfigureDirectly)
{
    BlurEffect blur (10.0f);
    EXPECT_FLOAT_EQ (blur.getRadius(), 10.0f);
    blur.setRadius (3.5f);
    EXPECT_FLOAT_EQ (blur.getRadius(), 3.5f);

    DropShadowEffect dropShadow (Color (0xff112233), 5.0f, { 1.0f, 4.0f });
    EXPECT_EQ (dropShadow.getColor(), Color (0xff112233));
    EXPECT_FLOAT_EQ (dropShadow.getRadius(), 5.0f);
    EXPECT_EQ (dropShadow.getOffset(), Point<float> (1.0f, 4.0f));

    GlowEffect glow (8.0f, 0.5f, Color (0xffff8800));
    EXPECT_FLOAT_EQ (glow.getRadius(), 8.0f);
    EXPECT_FLOAT_EQ (glow.getIntensity(), 0.5f);
    EXPECT_EQ (glow.getColor(), Color (0xffff8800));

    const auto hueRotated = ColorMatrixEffect::hueRotate (MathConstants<float>::pi);
    EXPECT_NE (dynamic_cast<ColorMatrixEffect*> (hueRotated.get()), nullptr);

    const auto faded = ColorMatrixEffect::opacity (0.25f);
    EXPECT_NE (dynamic_cast<ColorMatrixEffect*> (faded.get()), nullptr);
}

TEST_F (FilterEffectTests, ADropShadowAsksForTheRoomItsBlurSpreadsOver)
{
    FilterEffect effect;

    // The blur spreads three deviations on every side, and the offset moves the shadow further
    effect.setOperations ({ FilterEffect::Operation::dropShadow ({ 2.0f, 6.0f }, 4.0f, Colors::black) });
    EXPECT_EQ (effect.getRequiredMargin(), EdgeSpacing<float> (12.0f, 12.0f, 14.0f, 18.0f));

    effect.setOperations ({ FilterEffect::Operation::dropShadow ({ -3.0f, 0.0f }, 0.0f, Colors::black) });
    EXPECT_EQ (effect.getRequiredMargin(), EdgeSpacing<float> (3.0f, 0.0f, 0.0f, 0.0f));

    // A shadow that paints nothing asks for none
    effect.setOperations ({ FilterEffect::Operation::dropShadow ({ 2.0f, 6.0f }, 4.0f, Colors::transparentBlack) });
    EXPECT_EQ (effect.getRequiredMargin(), EdgeSpacing<float>());
}

TEST_F (FilterEffectTests, ABlurAsksForTheRoomItSpreadsOver)
{
    FilterEffect effect;

    effect.setOperations ({ FilterEffect::Operation::blur (4.0f) });
    EXPECT_EQ (effect.getRequiredMargin(), EdgeSpacing<float> (12.0f));

    // In a chain an operation spreads what the ones before it left: 6 for the blur, then 3 and the offset
    effect.setOperations ({ FilterEffect::Operation::blur (2.0f), FilterEffect::Operation::dropShadow ({ 0.0f, 4.0f }, 1.0f, Colors::black) });
    EXPECT_EQ (effect.getRequiredMargin(), EdgeSpacing<float> (9.0f, 9.0f, 9.0f, 13.0f));

    // The operations that leave every pixel where it is ask for none
    effect.setOperations ({ FilterEffect::Operation::hueRotate (1.0f), FilterEffect::Operation::opacity (0.5f), FilterEffect::Operation::blur (0.0f) });
    EXPECT_EQ (effect.getRequiredMargin(), EdgeSpacing<float>());
}

TEST_F (FilterEffectTests, StyleContextAppliesFilterWithoutOverwritingCustomComponentEffect)
{
    struct CustomUserEffect : ComponentEffect
    {
        void apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds) override
        {
            if (inputTexture != nullptr)
                g.drawTexture (inputTexture, bounds);
        }
    };

    Component root ("root");
    const ComponentEffect::Ptr userEffect (new CustomUserEffect());
    root.setComponentEffect (userEffect);

    attachStyle (root, R"yui(
        palette Dark { colors: "#101010-#eeeeee"; mode: dark; accent: #3388ff; }
        palette Warm { colors: "#f0f0f0-#202020"; mode: light; accent: #ff8833; }
        style {
            #root { palette: Dark; }
            #root.filtered {
                filter: blur(4px) saturate(120%) drop-shadow(2 6 4 palette.accent.withAlpha (0.5));
            }
            #root.warm { palette: Warm; }
            #root.cleared { filter: none; }
        }
    )yui");

    EXPECT_EQ (root.getComponentEffect(), userEffect);
    EXPECT_EQ (root.getStyleState()->getFilterEffect(), nullptr);

    root.addStyleClass (Identifier ("filtered"));
    restyle (root);

    EXPECT_EQ (root.getComponentEffect(), userEffect);
    const auto filterPtr = root.getStyleState()->getFilterEffect();
    ASSERT_NE (filterPtr, nullptr);

    const auto ops = filterPtr->getOperations();
    ASSERT_EQ (ops.size(), 3u);
    EXPECT_EQ (ops[0].type, FilterEffect::Type::blur);
    EXPECT_FLOAT_EQ (ops[0].amount, 4.0f);
    EXPECT_EQ (ops[1].type, FilterEffect::Type::saturate);
    EXPECT_FLOAT_EQ (ops[1].amount, 1.2f);
    EXPECT_EQ (ops[2].type, FilterEffect::Type::dropShadow);
    EXPECT_EQ (ops[2].color.withAlpha (static_cast<uint8> (255)), Color (0xff3388ff));
    EXPECT_GT (root.getPaintMargin().getBottom(), 0.0f);

    // Switching palettes updates palette-dependent drop-shadow color in place while reusing the FilterEffect instance
    root.addStyleClass (Identifier ("warm"));
    restyle (root);
    ASSERT_EQ (root.getStyleState()->getFilterEffect(), filterPtr);
    EXPECT_EQ (filterPtr->getOperations()[2].color.withAlpha (static_cast<uint8> (255)), Color (0xffff8833));

    root.addStyleClass (Identifier ("cleared"));
    restyle (root);
    EXPECT_EQ (root.getStyleState()->getFilterEffect(), nullptr);
    EXPECT_EQ (root.getComponentEffect(), userEffect);
    EXPECT_EQ (root.getPaintMargin(), EdgeSpacing<float>());
}

TEST_F (FilterEffectTests, StyleContextReportsInvalidFilterDiagnostics)
{
    Component root ("root");
    attachStyle (root, "style { #root { filter: unknownFilter(2); } }");

    ASSERT_NE (root.getStyleContext(), nullptr);
    ASSERT_FALSE (root.getStyleContext()->getDiagnostics().empty());
    EXPECT_TRUE (root.getStyleContext()->getDiagnostics().front().message.contains ("filter"));
}

TEST_F (FilterEffectTests, StripsCommentsInDirectAndStylesheetFilters)
{
    const auto direct = FilterEffect::parse ("blur(4px) /* saturate(200%) */ // trailing comment\n hue-rotate(90deg)");
    ASSERT_TRUE (direct.wasOk()) << direct.getErrorMessage();
    ASSERT_EQ (direct.getReference().size(), 2u);
    EXPECT_EQ (direct.getReference()[0].type, FilterEffect::Type::blur);
    EXPECT_FLOAT_EQ (direct.getReference()[0].amount, 4.0f);
    EXPECT_EQ (direct.getReference()[1].type, FilterEffect::Type::hueRotate);
    EXPECT_NEAR (direct.getReference()[1].amount, MathConstants<float>::halfPi, 1.0e-5f);

    EXPECT_TRUE (FilterEffect::parse ("blur(4px) /* unclosed").failed());

    Slider knob (Slider::Rotary, "knob");
    knob.addStyleClass (Identifier ("knob"));
    knob.setRange (0.0, 1.0);
    knob.setValue (0.5, dontSendNotification);

    attachStyle (knob, R"yui(
        palette Dark { colors: "#101010-#eeeeee"; mode: dark; accent: #3388ff; }
        style {
            Slider.knob {
                palette: Dark;
                filter: hue-rotate(value * 180deg)
                        /*brightness(1 + value * 0.2)
                        drop-shadow(0 4px 8px palette.accent.withAlpha(value * 0.6))*/;
            }
        }
    )yui");

    ASSERT_NE (knob.getStyleContext(), nullptr);
    EXPECT_TRUE (knob.getStyleContext()->getDiagnostics().empty())
        << DataTreeDiagnostic::join (knob.getStyleContext()->getDiagnostics());

    ASSERT_NE (knob.getStyleState(), nullptr);
    ASSERT_NE (knob.getStyleState()->getFilterEffect(), nullptr);
    const auto opsAtHalf = knob.getStyleState()->getFilterEffect()->getOperations();
    ASSERT_EQ (opsAtHalf.size(), 1u);
    EXPECT_EQ (opsAtHalf[0].type, FilterEffect::Type::hueRotate);
    EXPECT_NEAR (opsAtHalf[0].amount, MathConstants<float>::halfPi, 1.0e-4f);

    knob.setValue (1.0, dontSendNotification);
    restyle (knob);
    const auto opsAtFull = knob.getStyleState()->getFilterEffect()->getOperations();
    ASSERT_EQ (opsAtFull.size(), 1u);
    EXPECT_NEAR (opsAtFull[0].amount, MathConstants<float>::pi, 1.0e-4f);
}

TEST_F (FilterEffectTests, InterpolatesFilterChainsAndFallsBackHalfwayOnMismatchedTypes)
{
    using Op = FilterEffect::Operation;

    const std::vector<Op> empty;
    const std::vector<Op> target {
        Op::hueRotate (MathConstants<float>::halfPi),
        Op::dropShadow ({ 0.0f, 8.0f }, 4.0f, Color (0x80ff0000))
    };

    const auto midFromEmpty = FilterEffect::interpolate (empty, target, 0.5f);
    ASSERT_EQ (midFromEmpty.size(), 2u);
    EXPECT_EQ (midFromEmpty[0].type, FilterEffect::Type::hueRotate);
    EXPECT_NEAR (midFromEmpty[0].amount, MathConstants<float>::halfPi * 0.5f, 1.0e-5f);
    EXPECT_EQ (midFromEmpty[1].type, FilterEffect::Type::dropShadow);
    EXPECT_EQ (midFromEmpty[1].offset, Point<float> (0.0f, 4.0f));
    EXPECT_FLOAT_EQ (midFromEmpty[1].amount, 2.0f);
    EXPECT_GT (midFromEmpty[1].color.getAlpha(), 0);
    EXPECT_LT (midFromEmpty[1].color.getAlpha(), 0x80);

    const std::vector<Op> prefix { Op::blur (2.0f) };
    const std::vector<Op> extended { Op::blur (10.0f), Op::saturate (2.0f) };
    const auto midExtended = FilterEffect::interpolate (prefix, extended, 0.5f);
    ASSERT_EQ (midExtended.size(), 2u);
    EXPECT_FLOAT_EQ (midExtended[0].amount, 6.0f);
    EXPECT_FLOAT_EQ (midExtended[1].amount, 1.5f);

    const std::vector<Op> mismatched { Op::invert (1.0f) };
    EXPECT_EQ (FilterEffect::interpolate (prefix, mismatched, 0.25f), prefix);
    EXPECT_EQ (FilterEffect::interpolate (prefix, mismatched, 0.75f), mismatched);
}

TEST_F (FilterEffectTests, StyleContextTransitionsFilterBothWaysAndReversesMidFlight)
{
    Component knob ("knob");
    knob.addStyleClass (Identifier ("knob"));

    attachStyle (knob, R"yui(
        style {
            .knob {
                transition: filter 2000ms cubicBezier (0, 0, 1, 1);
            }
            .knob.hovered {
                filter: hue-rotate(90deg) drop-shadow(0 8px 4px #ff000080);
            }
        }
    )yui");

    EXPECT_TRUE (knob.getStyleState() == nullptr || knob.getStyleState()->getFilterEffect() == nullptr);

    // Entering .hovered starts from identity (no immediate jump at t = 0)
    knob.addStyleClass (Identifier ("hovered"));
    advance (knob, 0.0);
    EXPECT_EQ (knob.getStyleState()->getFilterEffect(), nullptr);

    // Halfway through 2000ms: hue-rotate is 45deg and drop-shadow is halfway
    advance (knob, 1.0);
    ASSERT_NE (knob.getStyleState()->getFilterEffect(), nullptr);
    {
        const auto ops = knob.getStyleState()->getFilterEffect()->getOperations();
        ASSERT_EQ (ops.size(), 2u);
        EXPECT_NEAR (ops[0].amount, MathConstants<float>::halfPi * 0.5f, 1.0e-4f);
        EXPECT_NEAR (ops[1].offset.getY(), 4.0f, 1.0e-4f);
        EXPECT_NEAR (ops[1].amount, 2.0f, 1.0e-4f);
        EXPECT_GT (knob.getPaintMargin().getBottom(), 0.0f);
    }

    // Leaving .hovered mid-flight reverses smoothly from 45deg back to 0deg
    knob.removeStyleClass (Identifier ("hovered"));
    advance (knob, 0.0);
    ASSERT_NE (knob.getStyleState()->getFilterEffect(), nullptr);
    EXPECT_NEAR (knob.getStyleState()->getFilterEffect()->getOperations()[0].amount,
                 MathConstants<float>::halfPi * 0.5f,
                 1.0e-4f);

    advance (knob, 1.0);
    ASSERT_NE (knob.getStyleState()->getFilterEffect(), nullptr);
    EXPECT_NEAR (knob.getStyleState()->getFilterEffect()->getOperations()[0].amount,
                 MathConstants<float>::halfPi * 0.25f,
                 1.0e-4f);

    // Finishing the return transition removes FilterEffect and restores paint margins
    advance (knob, 1.1);
    EXPECT_EQ (knob.getStyleState()->getFilterEffect(), nullptr);
    EXPECT_EQ (knob.getPaintMargin(), EdgeSpacing<float>());
}

TEST_F (FilterEffectTests, StyleContextAnimatesFilterKeyframes)
{
    Component panel ("panel");
    attachStyle (panel, R"yui(
        style {
            .pulsing {
                animation: hueSweep 2s cubicBezier (0, 0, 1, 1);
            }
            @keyframes hueSweep {
                from { filter: hue-rotate(0deg); }
                to   { filter: hue-rotate(180deg); }
            }
        }
    )yui");

    panel.addStyleClass (Identifier ("pulsing"));
    advance (panel, 0.0);
    ASSERT_NE (panel.getStyleState()->getFilterEffect(), nullptr);
    EXPECT_NEAR (panel.getStyleState()->getFilterEffect()->getOperations()[0].amount, 0.0f, 1.0e-4f);

    advance (panel, 0.5);
    ASSERT_NE (panel.getStyleState()->getFilterEffect(), nullptr);
    EXPECT_NEAR (panel.getStyleState()->getFilterEffect()->getOperations()[0].amount,
                 MathConstants<float>::pi * 0.25f,
                 1.0e-4f);

    advance (panel, 2.0);
    EXPECT_EQ (panel.getStyleState()->getFilterEffect(), nullptr);
}

//==============================================================================
class FilterEffectGpuTests : public FilterEffectTests
{
protected:
    struct SolidPanel : Component
    {
        explicit SolidPanel (Color fillToUse)
            : fillColor (fillToUse)
        {
            setBounds (0.0f, 0.0f, 16.0f, 16.0f);
        }

        void paint (Graphics& g) override
        {
            g.setFillColor (fillColor);
            g.fillAll();
        }

        Color fillColor;
    };

    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});
        if (gpuContext == nullptr)
            return;

        if (GpuCanvas::create (*gpuContext, 32, 32) == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        FilterEffectTests::SetUp();

        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";
    }

    static std::unique_ptr<GraphicsContext> gpuContext;
};

std::unique_ptr<GraphicsContext> FilterEffectGpuTests::gpuContext;

TEST_F (FilterEffectGpuTests, RendersFilteredPixelsThroughStyleFilterLayer)
{
    SolidPanel panel (Color (0xffff0000));
    panel.setComponentID ("panel");

    attachStyle (panel, "style { #panel { filter: invert(100%); } }");
    const auto inverted = panel.snapshotToImage (*gpuContext, true);
    ASSERT_TRUE (inverted.isValid());

    const auto invertedPixel = inverted.getPixelColor (8, 8);
    EXPECT_LT (invertedPixel.getRed(), 5);
    EXPECT_GT (invertedPixel.getGreen(), 250);
    EXPECT_GT (invertedPixel.getBlue(), 250);

    attachStyle (panel, "style { #panel { filter: grayscale(100%) opacity(50%); } }");
    const auto fadedGray = panel.snapshotToImage (*gpuContext, true);
    ASSERT_TRUE (fadedGray.isValid());

    const auto grayPixel = fadedGray.getPixelColor (8, 8);
    EXPECT_NEAR (static_cast<int> (grayPixel.getRed()), static_cast<int> (grayPixel.getGreen()), 3);
    EXPECT_NEAR (static_cast<int> (grayPixel.getGreen()), static_cast<int> (grayPixel.getBlue()), 3);
    EXPECT_NEAR (static_cast<int> (grayPixel.getAlpha()), 128, 4);
}
