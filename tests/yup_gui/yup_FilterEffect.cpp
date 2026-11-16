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

#include <gtest/gtest.h>

using namespace yup;

class FilterEffectTests : public ::testing::Test
{
protected:
    static StyleSheet::Ptr buildStyle (const String& yui)
    {
        const auto parsed = YuiParser::parse (yui);
        EXPECT_TRUE (parsed.wasOk()) << YuiDiagnostic::join (parsed.diagnostics);

        auto sheet = StyleSheet::create (parsed.document);
        EXPECT_TRUE (sheet.wasOk()) << sheet.diagnostic.toString();
        return sheet.styleSheet;
    }

    static void attachStyle (Component& root, const String& yui)
    {
        root.setStyleSheet (buildStyle (yui));

        if (auto* context = root.getStyleContext())
            context->restyleDirty();
    }

    static void restyle (Component& root)
    {
        if (auto* context = root.getStyleContext())
            context->restyleDirty();
    }
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
        palette Dark { accent: #3388ff; dark: true; }
        palette Warm { accent: #ff8833; dark: false; }
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
    EXPECT_TRUE (root.getStyleContext()->getDiagnostics().front().message.contains ("filter:"));
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
        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";
    }

    static std::unique_ptr<GraphicsContext> gpuContext;
};

std::unique_ptr<GraphicsContext> FilterEffectGpuTests::gpuContext;

TEST_F (FilterEffectGpuTests, RendersFilteredPixelsThroughStyleFilterLayer)
{
    SolidPanel panel (Color (0xffff0000));
    panel.setComponentName ("panel");

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
