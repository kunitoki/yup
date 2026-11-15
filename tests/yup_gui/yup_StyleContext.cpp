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

class StyleContextTests : public ::testing::Test
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

    static DataTree declaration (const String& name, const String& value)
    {
        return DataTree ("declaration", { { "name", name }, { "value", value } });
    }

    static DataTree rule (const String& selector, std::initializer_list<DataTree> declarations)
    {
        return DataTree ("rule", { { "selector", selector } }, declarations);
    }

    static StyleSheet::Ptr build (const DataTree& style)
    {
        auto sheet = StyleSheet::fromTree (style);
        EXPECT_FALSE (sheet.failed()) << sheet.getErrorMessage();
        return sheet.wasOk() ? sheet.getReference() : nullptr;
    }

    static void restyle (Component& component)
    {
        auto* context = component.getStyleContext();
        ASSERT_NE (context, nullptr);
        context->restyleNow();
    }

    static void attach (Component& root, const String& text)
    {
        auto sheet = StyleSheet::parse (text);
        ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
        root.setStyleSheet (sheet.getReference());
        restyle (root);
    }

    static void advance (Component& root, double elapsedSeconds)
    {
        root.getStyleContext()->refresh (elapsedSeconds);
    }

    static double glow (const Component& component)
    {
        return static_cast<double> (component.findProperty (Identifier ("glow")));
    }

    static void expectDiagnostic (const Component& root, const String& prefix)
    {
        const auto& diagnostics = root.getStyleContext()->getDiagnostics();

        for (const auto& diagnostic : diagnostics)
            if (diagnostic.message.startsWith (prefix))
                return;

        ADD_FAILURE() << "No diagnostic starting with '" << prefix << "' in: " << YuiDiagnostic::join (diagnostics);
    }

    class StyleTestEffect : public ComponentEffect
    {
    public:
        void apply (Graphics&, GpuTexture::Ptr, Rectangle<float>) override {}
    };

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (StyleContextTests, ComputesCustomPropertiesForMatchedComponents)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    child.addStyleClass (Identifier ("knob"));

    root.setStyleSheet (build (DataTree ("style", { rule (".knob", { declaration ("trackWidth", "4 + 2") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (child.findProperty (Identifier ("trackWidth"))), 6.0);
    EXPECT_FALSE (root.getStyleState() != nullptr && root.getStyleState()->getComputedValues().contains (Identifier ("trackWidth")));
}

TEST_F (StyleContextTests, ComputesColorValues)
{
    Component root ("root");
    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("thumbColor", "#ff8800") }) })));
    restyle (root);

    const auto color = root.findColor (Identifier ("thumbColor"));
    ASSERT_TRUE (color.has_value());
    EXPECT_EQ (*color, Color (0xffff8800));
}

TEST_F (StyleContextTests, ExplicitValuesWinOverComputedOnes)
{
    Component root ("root");
    root.setProperty (Identifier ("trackWidth"), 12.0);

    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("trackWidth", "4") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (root.findProperty (Identifier ("trackWidth"))), 12.0);
}

TEST_F (StyleContextTests, HigherSpecificityWins)
{
    Component root ("root");
    root.addStyleClass (Identifier ("panel"));

    root.setStyleSheet (build (DataTree ("style",
                                         { rule ("#root", { declaration ("trackWidth", "1") }),
                                           rule (".panel", { declaration ("trackWidth", "2") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (root.findProperty (Identifier ("trackWidth"))), 1.0);
}

TEST_F (StyleContextTests, LaterRuleWinsAtEqualSpecificity)
{
    Component root ("root");
    root.addStyleClass (Identifier ("panel"));

    root.setStyleSheet (build (DataTree ("style",
                                         { rule (".panel", { declaration ("trackWidth", "1") }),
                                           rule (".panel", { declaration ("trackWidth", "2") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (root.findProperty (Identifier ("trackWidth"))), 2.0);
}

TEST_F (StyleContextTests, QualifiedDeclarationsCascadeToDescendants)
{
    Component root ("root");
    Slider slider (Slider::LinearHorizontal, "slider");
    Component other ("other");

    root.addChildComponent (slider);
    root.addChildComponent (other);
    root.addStyleClass (Identifier ("panel"));

    root.setStyleSheet (build (DataTree ("style", { rule (".panel", { declaration ("Slider.trackWidth", "7") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (slider.findProperty (Identifier ("trackWidth"))), 7.0);
    EXPECT_TRUE (other.getStyleState() == nullptr || other.getStyleState()->getComputedValues().isEmpty());
}

TEST_F (StyleContextTests, ChangingStyleClassesRestyles)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    root.setStyleSheet (build (DataTree ("style", { rule (".knob", { declaration ("trackWidth", "9") }) })));
    restyle (root);

    EXPECT_FALSE (child.findProperty (Identifier ("trackWidth")).isDouble());

    child.addStyleClass (Identifier ("knob"));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (child.findProperty (Identifier ("trackWidth"))), 9.0);

    child.removeStyleClass (Identifier ("knob"));
    restyle (root);

    EXPECT_TRUE (child.getStyleState() == nullptr || child.getStyleState()->getComputedValues().isEmpty());
}

TEST_F (StyleContextTests, StateBackedPropertiesAreAppliedAndRestored)
{
    Component root ("root");
    root.setOpacity (1.0f);

    root.setStyleSheet (build (DataTree ("style", { rule (".faded", { declaration ("opacity", "0.5") }) })));
    restyle (root);

    EXPECT_FLOAT_EQ (root.getOpacity(), 1.0f);

    root.addStyleClass (Identifier ("faded"));
    restyle (root);

    EXPECT_NEAR (root.getOpacity(), 0.5f, 0.01f);

    root.removeStyleClass (Identifier ("faded"));
    restyle (root);

    EXPECT_FLOAT_EQ (root.getOpacity(), 1.0f);
}

TEST_F (StyleContextTests, ScopesPaletteDeclarationsToTheSubtree)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    root.setStyleSheet (build (DataTree ("style",
                                         { DataTree ("palette", { { "name", "Ember" }, { "colors", "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f" }, { "accent", "#ff9a1f" } }),
                                           rule ("#root", { declaration ("palette", "Ember") }),
                                           rule ("#child", { declaration ("thumbColor", "palette.accent") }) })));
    restyle (root);

    ASSERT_NE (root.getStyleState(), nullptr);
    ASSERT_NE (root.getStyleState()->getStylePalette(), nullptr);
    EXPECT_EQ (child.findPalette(), root.getStyleState()->getStylePalette());

    const auto color = child.findColor (Identifier ("thumbColor"));
    ASSERT_TRUE (color.has_value());
    EXPECT_EQ (*color, Color (0xffff9a1f));
}

TEST_F (StyleContextTests, DetachingClearsComputedValues)
{
    Component root ("root");
    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("trackWidth", "3") }) })));
    restyle (root);

    EXPECT_DOUBLE_EQ (static_cast<double> (root.findProperty (Identifier ("trackWidth"))), 3.0);

    root.setStyleSheet (nullptr);

    EXPECT_EQ (root.getStyleContext(), nullptr);
    EXPECT_TRUE (root.getStyleState() == nullptr || root.getStyleState()->getComputedValues().isEmpty());
}

TEST_F (StyleContextTests, ReportsDiagnosticsForBadValues)
{
    Component root ("root");
    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("trackWidth", "notAKnownSymbol * 2") }) })));
    restyle (root);

    EXPECT_FALSE (root.getStyleContext()->getDiagnostics().empty());
}

TEST_F (StyleContextTests, InstallsAndRestoresDrawings)
{
    Component root ("root");
    Component child ("child");
    root.addChildComponent (child);

    const auto previousStyle = child.getStyle();

    auto sheet = StyleSheet::parse (R"yui(
        drawing Dot { circle { center: bounds.center; radius: 2; fill: #ff0000; } }
        style { .dot { drawing: Dot; } })yui");

    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    root.setStyleSheet (sheet.getReference());

    child.addStyleClass (Identifier ("dot"));
    restyle (root);

    const auto installed = child.getStyle();
    EXPECT_NE (installed, previousStyle);
    EXPECT_NE (dynamic_cast<DrawingStyle*> (installed.get()), nullptr);

    restyle (root);
    EXPECT_EQ (child.getStyle(), installed);

    child.removeStyleClass (Identifier ("dot"));
    restyle (root);
    EXPECT_EQ (child.getStyle(), previousStyle);
}

TEST_F (StyleContextTests, ReportsUnknownDrawings)
{
    Component root ("root");
    root.setStyleSheet (build (DataTree ("style", { rule ("#root", { declaration ("drawing", "Missing") }) })));
    restyle (root);

    expectDiagnostic (root, "Unknown drawing 'Missing'");
}

TEST_F (StyleContextTests, TypesTheDeclarationsOfDrawingProperties)
{
    Component root ("root");
    Component first ("first");
    Component second ("second");
    root.addChildComponent (first);
    root.addChildComponent (second);

    auto sheet = StyleSheet::parse (R"yui(
        drawing Dot {
            property dotRadius: number = 2;
            circle { center: bounds.center; radius: dotRadius; fill: #ff0000; }
        }
        style {
            #first { drawing: Dot; dotRadius: 1; }
            #second { drawing: Dot; dotRadius: #ff0000; }
        })yui");

    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    root.setStyleSheet (sheet.getReference());
    restyle (root);

    const auto radius = first.findProperty (Identifier ("dotRadius"));
    EXPECT_TRUE (radius.isDouble());
    EXPECT_DOUBLE_EQ (static_cast<double> (radius), 1.0);

    const auto diagnostics = root.getStyleContext()->getDiagnostics();
    ASSERT_EQ (diagnostics.size(), 1);
    EXPECT_TRUE (diagnostics[0].message.startsWith ("dotRadius: ")) << diagnostics[0].message;
}

TEST_F (StyleContextTests, TransitionsFollowStyleChanges)
{
    Component root ("root");
    root.addStyleClass (Identifier ("knob"));

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }");
    EXPECT_DOUBLE_EQ (glow (root), 0.0);

    root.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 0.0);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 5.0);

    advance (root, 0.6);
    EXPECT_DOUBLE_EQ (glow (root), 10.0);
    EXPECT_EQ (root.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);
}

TEST_F (StyleContextTests, TheFirstStyleIsNotTransitioned)
{
    Component root ("root");
    attach (root, "style { #root { glow: 10; transition: glow 1s; } }");

    EXPECT_DOUBLE_EQ (glow (root), 10.0);
    EXPECT_EQ (root.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);
}

TEST_F (StyleContextTests, TransitionsStateBackedPropertiesBothWays)
{
    Component root ("root");
    root.setOpacity (1.0f);
    root.addStyleClass (Identifier ("fade"));

    attach (root, "style { .fade { transition: opacity 1s; } .faded { opacity: 0.5; } }");

    root.addStyleClass (Identifier ("faded"));
    advance (root, 0.0);
    EXPECT_NEAR (root.getOpacity(), 1.0f, 0.01f);

    advance (root, 0.5);
    EXPECT_NEAR (root.getOpacity(), 0.75f, 0.01f);

    advance (root, 0.6);
    EXPECT_NEAR (root.getOpacity(), 0.5f, 0.01f);

    root.removeStyleClass (Identifier ("faded"));
    advance (root, 0.0);
    EXPECT_NEAR (root.getOpacity(), 0.5f, 0.01f);

    advance (root, 0.5);
    EXPECT_NEAR (root.getOpacity(), 0.75f, 0.01f);

    advance (root, 0.6);
    EXPECT_NEAR (root.getOpacity(), 1.0f, 0.01f);

    root.removeStyleClass (Identifier ("fade"));
    advance (root, 0.0);
    EXPECT_NEAR (root.getOpacity(), 1.0f, 0.01f);
}

TEST_F (StyleContextTests, RetargetsARunningTransitionFromWhereItIs)
{
    Component root ("root");
    root.addStyleClass (Identifier ("knob"));

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }");

    root.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 5.0);

    root.removeStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 5.0);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 2.5);

    advance (root, 0.6);
    EXPECT_DOUBLE_EQ (glow (root), 0.0);
}

TEST_F (StyleContextTests, TransitionsTheNewStyleDoesNotDeclareStop)
{
    Component root ("root");
    root.addStyleClass (Identifier ("knob"));

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } .instant { transition: none; } }");

    root.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 5.0);

    root.addStyleClass (Identifier ("instant"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 10.0);
    EXPECT_EQ (root.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);
}

TEST_F (StyleContextTests, ExplicitValuesWinOverAnimatedOnes)
{
    Component root ("root");
    root.addStyleClass (Identifier ("knob"));

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }");
    root.setProperty (Identifier ("glow"), 3.0);

    root.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 3.0);
}

TEST_F (StyleContextTests, PlaysKeyframeAnimations)
{
    Component root ("root");
    attach (root, "style { .pulsing { animation: pulse 1s cubicBezier (0, 0, 1, 1); } @keyframes pulse { from { glow: 0; } to { glow: 10; } } }");

    root.addStyleClass (Identifier ("pulsing"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 0.0);

    advance (root, 0.25);
    EXPECT_NEAR (glow (root), 2.5, 1.0e-4);

    advance (root, 1.0);
    EXPECT_TRUE (root.findProperty (Identifier ("glow")).isVoid());
}

TEST_F (StyleContextTests, AnimationsFillingForwardsHoldUntilRemoved)
{
    Component root ("root");
    attach (root, "style { .pulsing { animation: pulse 1s forwards; } @keyframes pulse { from { glow: 0; } to { glow: 10; } } }");

    root.addStyleClass (Identifier ("pulsing"));
    advance (root, 0.0);
    advance (root, 1.5);
    EXPECT_DOUBLE_EQ (glow (root), 10.0);

    root.removeStyleClass (Identifier ("pulsing"));
    advance (root, 0.0);
    EXPECT_TRUE (root.findProperty (Identifier ("glow")).isVoid());
}

TEST_F (StyleContextTests, MissingKeyframeEndsUseTheStyledValue)
{
    Component root ("root");
    attach (root, "style { .flashing { glow: 4; animation: flash 1s; } @keyframes flash { 50% { glow: 10; } } }");

    root.addStyleClass (Identifier ("flashing"));
    advance (root, 0.0);
    EXPECT_DOUBLE_EQ (glow (root), 4.0);

    advance (root, 0.25);
    EXPECT_DOUBLE_EQ (glow (root), 7.0);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 7.0);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (root), 4.0);
}

TEST_F (StyleContextTests, ReportsAnimationDiagnostics)
{
    Component root ("root");
    attach (root, "style { #root { transition: glow 1; animation: missing 1s; } }");

    expectDiagnostic (root, "transition: Unexpected '1'");
    expectDiagnostic (root, "Unknown keyframes 'missing'");
}

TEST_F (StyleContextTests, ReportsTheDeclarationOfEachDiagnostic)
{
    const auto parsed = YuiParser::parse (R"yui(drawing Broken { circle { radius: missingSymbol; } }
style {
    #root {
        palette: Missing;
        transition: glow 1;
        text: "x";
        glow: notAKnownSymbol * 2;
        cursor: finger;
        effect: missingEffect;
        shadow: 1 red;
        animation: missing 1s;
    }
    #a { drawing: Missing; shadow: 0 0 2 notAColor; animation: 1s; }
    #b { drawing: Broken; animation: bad 1s; }
    @keyframes bad { from { text: "y"; } to { glow: notAKnownSymbol; } }
})yui");

    ASSERT_TRUE (parsed.wasOk()) << YuiDiagnostic::join (parsed.diagnostics);

    TextButton root ("root");
    Component a ("a");
    TextButton b ("b");
    root.addChildComponent (a);
    root.addChildComponent (b);
    root.setStyleSheet (build (parsed.document));

    StringArray reported;
    root.getStyleContext()->onDiagnostic = [&] (const YuiDiagnostic& diagnostic)
    {
        reported.add (parsed.sourceMap.locate (diagnostic).toString());
    };

    restyle (root);

    EXPECT_EQ (reported.size(), static_cast<int> (root.getStyleContext()->getDiagnostics().size()));

    for (const auto* expected : { "4:9: Unknown palette 'Missing'",
                                  "5:9: transition: ",
                                  "6:9: The property 'text' can't be styled",
                                  "7:9: glow: ",
                                  "8:9: Unknown cursor 'finger'",
                                  "9:9: Unknown effect 'missingEffect'",
                                  "10:9: shadow: Expected",
                                  "11:9: Unknown keyframes 'missing'",
                                  "13:10: Unknown drawing 'Missing'",
                                  "13:28: shadow[0]: ",
                                  "13:53: animation: ",
                                  "1:1: drawing Broken: ",
                                  "15:29: The property 'text' can't be animated",
                                  "15:47: bad glow: " })
    {
        EXPECT_TRUE (std::any_of (reported.begin(), reported.end(), [&] (const String& line)
        {
            return line.startsWith (expected);
        })) << "No diagnostic starting with '" << expected << "' in:\n"
            << reported.joinIntoString ("\n");
    }
}

TEST_F (StyleContextTests, ReportsDiagnosticsOfRulesMatchingLater)
{
    Component root ("root");
    attach (root, "style { #root.late { glow: notAKnownSymbol; } }");

    std::vector<YuiDiagnostic> reported;
    root.getStyleContext()->onDiagnostic = [&] (const YuiDiagnostic& diagnostic)
    {
        reported.push_back (diagnostic);
    };

    root.addStyleClass (Identifier ("late"));
    restyle (root);

    ASSERT_EQ (reported.size(), 1u);
    EXPECT_TRUE (reported[0].message.startsWith ("glow: ")) << reported[0].message;
    EXPECT_EQ (reported[0].element.getProperty ("name").toString(), "glow");

    root.removeStyleClass (Identifier ("late"));
    restyle (root);

    EXPECT_TRUE (root.getStyleContext()->getDiagnostics().empty());
    EXPECT_EQ (reported.size(), 1u);
}

TEST_F (StyleContextTests, StylesAndRestoresTheMouseCursor)
{
    Component root ("root");
    root.setMouseCursor (MouseCursor::Crosshair);

    attach (root, "style { .link { cursor: hand; } }");
    EXPECT_EQ (root.getMouseCursor().getType(), MouseCursor::Crosshair);

    root.addStyleClass (Identifier ("link"));
    restyle (root);
    EXPECT_EQ (root.getMouseCursor().getType(), MouseCursor::Hand);

    root.removeStyleClass (Identifier ("link"));
    restyle (root);
    EXPECT_EQ (root.getMouseCursor().getType(), MouseCursor::Crosshair);
}

TEST_F (StyleContextTests, ReportsUnknownCursors)
{
    Component root ("root");
    attach (root, "style { #root { cursor: finger; } }");

    expectDiagnostic (root, "Unknown cursor 'finger'");
    EXPECT_EQ (root.getMouseCursor().getType(), MouseCursor::Default);
}

TEST_F (StyleContextTests, InstallsRegisteredEffectsAndRestoresTheOriginal)
{
    const Identifier effectName ("styleTestEffect");
    ComponentEffectRegistry::getInstance().registerEffect (effectName, []
    {
        return ComponentEffect::Ptr (new StyleTestEffect());
    });

    Component root ("root");
    const ComponentEffect::Ptr explicitEffect (new StyleTestEffect());
    root.setComponentEffect (explicitEffect);

    attach (root, "style { .neon { effect: styleTestEffect; } .flat { effect: none; } }");
    EXPECT_EQ (root.getComponentEffect(), explicitEffect);

    root.addStyleClass (Identifier ("neon"));
    restyle (root);

    const auto installed = root.getComponentEffect();
    EXPECT_NE (installed, explicitEffect);
    EXPECT_NE (dynamic_cast<StyleTestEffect*> (installed.get()), nullptr);

    root.getStyleContext()->markDirty (root);
    restyle (root);
    EXPECT_EQ (root.getComponentEffect(), installed);

    root.addStyleClass (Identifier ("flat"));
    restyle (root);
    EXPECT_EQ (root.getComponentEffect(), nullptr);

    root.removeStyleClass (Identifier ("neon"));
    root.removeStyleClass (Identifier ("flat"));
    restyle (root);
    EXPECT_EQ (root.getComponentEffect(), explicitEffect);

    ComponentEffectRegistry::getInstance().unregisterEffect (effectName);
}

TEST_F (StyleContextTests, ReportsUnknownEffects)
{
    Component root ("root");
    attach (root, "style { #root { effect: missingEffect; } }");

    expectDiagnostic (root, "Unknown effect 'missingEffect'");
    EXPECT_EQ (root.getComponentEffect(), nullptr);
}

TEST_F (StyleContextTests, ParsesShadowLayersAndWidensThePaintMargin)
{
    Component root ("root");
    attach (root, "style { #root { shadow: 0 4 12 #00000080, -2 0 3 1 #ff0000; } }");

    ASSERT_NE (root.getStyleState(), nullptr);
    const auto& shadows = root.getStyleState()->getShadows();
    ASSERT_EQ (shadows.size(), 2u);

    EXPECT_EQ (shadows[0].offset, Point<float> (0.0f, 4.0f));
    EXPECT_FLOAT_EQ (shadows[0].blur, 12.0f);
    EXPECT_FLOAT_EQ (shadows[0].spread, 0.0f);
    EXPECT_EQ (shadows[0].color, Color (0x80000000));

    EXPECT_EQ (shadows[1].offset, Point<float> (-2.0f, 0.0f));
    EXPECT_FLOAT_EQ (shadows[1].blur, 3.0f);
    EXPECT_FLOAT_EQ (shadows[1].spread, 1.0f);
    EXPECT_EQ (shadows[1].color, Color (0xffff0000));

    const auto margin = root.getPaintMargin();
    EXPECT_FLOAT_EQ (margin.getLeft(), 12.0f);
    EXPECT_FLOAT_EQ (margin.getTop(), 8.0f);
    EXPECT_FLOAT_EQ (margin.getRight(), 12.0f);
    EXPECT_FLOAT_EQ (margin.getBottom(), 16.0f);
}

TEST_F (StyleContextTests, ShadowColorsReadThePalette)
{
    Component root ("root");
    attach (root, R"yui(
        palette Ember { colors: "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f"; accent: #ff9a1f; }
        style { #root { palette: Ember; shadow: 0 0 8 palette.accent.withAlpha (0.5); } })yui");

    ASSERT_NE (root.getStyleState(), nullptr);
    const auto& shadows = root.getStyleState()->getShadows();
    ASSERT_EQ (shadows.size(), 1u);

    EXPECT_EQ (shadows[0].color, Color (0xffff9a1f).withAlpha (0.5f));
    EXPECT_TRUE (root.getStyleState()->isPaletteDependent());
}

TEST_F (StyleContextTests, RemovingShadowsRestoresThePaintMargin)
{
    Component root ("root");
    root.setPaintMargin (2.0f);

    attach (root, "style { .card { shadow: 0 0 6 #000000; } .flat { shadow: none; } }");

    root.addStyleClass (Identifier ("card"));
    restyle (root);
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 6.0f);

    root.addStyleClass (Identifier ("flat"));
    restyle (root);
    EXPECT_TRUE (root.getStyleState()->getShadows().empty());
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 2.0f);

    root.removeStyleClass (Identifier ("flat"));
    restyle (root);
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 6.0f);

    root.removeStyleClass (Identifier ("card"));
    restyle (root);
    EXPECT_TRUE (root.getStyleState()->getShadows().empty());
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 2.0f);
}

TEST_F (StyleContextTests, ReportsMalformedShadows)
{
    Component root ("root");
    attach (root, "style { #root { shadow: 0 0 4 #000000, 4 #000000; } }");

    expectDiagnostic (root, "shadow: Expected 'x y [blur [spread]] color'");
    EXPECT_TRUE (root.getStyleState()->getShadows().empty());
    EXPECT_FLOAT_EQ (root.getPaintMargin().getLeft(), 0.0f);
}

namespace yup
{

template <>
class ComponentTestHelper<StyleContext>
{
public:
    static void triggerMouseEnter (Component& component, const MouseEvent& event) { component.internalMouseEnter (event); }
    static void triggerMouseExit (Component& component, const MouseEvent& event) { component.internalMouseExit (event); }
    static void triggerMouseDown (Component& component, const MouseEvent& event) { component.internalMouseDown (event); }
    static void triggerMouseUp (Component& component, const MouseEvent& event) { component.internalMouseUp (event); }
    static void triggerFocusGained (Component& component) { component.internalFocusChanged (true); }
    static void triggerFocusLost (Component& component) { component.internalFocusChanged (false); }
};

} // namespace yup

TEST_F (StyleContextTests, AncestorStateClassAndIdChangesRestyleCombinatorDescendants)
{
    Component root ("root");
    Component row ("row");
    Component icon ("icon");
    icon.addStyleClass (Identifier ("icon"));

    root.addAndMakeVisible (row);
    row.addAndMakeVisible (icon);

    attach (root, R"yui(
        style {
            .icon { glow: 1; }
            #row:hover .icon { glow: 2; }
            #row:pressed > .icon { glow: 3; }
            #row:focused .icon { glow: 4; }
            .active .icon { glow: 5; }
            #featured > .icon { glow: 6; }
            #row:disabled .icon { glow: 7; }
        })yui");

    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    ComponentTestHelper<StyleContext>::triggerMouseEnter (row, MouseEvent {});
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 2.0);

    ComponentTestHelper<StyleContext>::triggerMouseDown (row, MouseEvent {});
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 3.0);

    ComponentTestHelper<StyleContext>::triggerMouseUp (row, MouseEvent {});
    ComponentTestHelper<StyleContext>::triggerMouseExit (row, MouseEvent {});
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    ComponentTestHelper<StyleContext>::triggerFocusGained (row);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 4.0);

    ComponentTestHelper<StyleContext>::triggerFocusLost (row);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    row.addStyleClass (Identifier ("active"));
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 5.0);

    row.removeStyleClass (Identifier ("active"));
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    row.setEnabled (false);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 7.0);

    row.setEnabled (true);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 1.0);

    row.setComponentID ("featured");
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (icon), 6.0);
}

TEST_F (StyleContextTests, DetachingAChildStopsItsAnimationsAndClearsCachedState)
{
    Component root ("root");
    Component knob ("knob");
    knob.addStyleClass (Identifier ("knob"));
    root.addAndMakeVisible (knob);

    attach (root, "style { .knob { glow: 0; transition: glow 1s; } .lit { glow: 10; } }");
    EXPECT_DOUBLE_EQ (glow (knob), 0.0);

    knob.addStyleClass (Identifier ("lit"));
    advance (root, 0.0);
    advance (root, 0.25);
    EXPECT_DOUBLE_EQ (glow (knob), 2.5);
    ASSERT_NE (knob.getStyleState(), nullptr);
    EXPECT_NE (knob.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);

    const auto detachedGlow = glow (knob);
    root.removeChildComponent (&knob);

    EXPECT_EQ (knob.getStyleState()->getAnimatedValue (Identifier ("glow")), nullptr);

    advance (root, 0.5);
    EXPECT_DOUBLE_EQ (glow (knob), detachedGlow);

    alignas (Component) unsigned char slot[sizeof (Component)];
    auto* first = new (slot) Component ("slot");
    first->addStyleClass (Identifier ("lit"));
    root.addAndMakeVisible (first);
    restyle (root);
    EXPECT_DOUBLE_EQ (glow (*first), 10.0);
    first->~Component();

    auto* second = new (slot) Component ("slot");
    root.addAndMakeVisible (second);
    restyle (root);
    EXPECT_TRUE (second->findProperty (Identifier ("glow")).isVoid());
    root.removeChildComponent (second);
    second->~Component();
}
