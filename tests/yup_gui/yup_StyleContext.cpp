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

    EXPECT_FALSE (root.getStyleContext()->getDiagnostics().isEmpty());
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

    EXPECT_TRUE (root.getStyleContext()->getDiagnostics().contains ("Unknown drawing 'Missing'"));
}
