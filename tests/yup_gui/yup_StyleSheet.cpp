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

class StyleSheetTests : public ::testing::Test
{
protected:
    static DataTree declaration (const String& name, const String& value)
    {
        return DataTree ("declaration", { { "name", name }, { "value", value } });
    }

    static StyleSheet::Ptr build (const DataTree& style)
    {
        auto sheet = StyleSheet::fromTree (style);
        EXPECT_FALSE (sheet.failed()) << sheet.getErrorMessage();
        return sheet.wasOk() ? sheet.getReference() : nullptr;
    }
};

TEST_F (StyleSheetTests, ReadsRulesAndDeclarationsInOrder)
{
    auto sheet = build (DataTree ("style",
                                  { DataTree ("rule", { { "selector", "Slider" } }, { declaration ("thumbColor", "#ff0000") }),
                                    DataTree ("rule", { { "selector", ".panel" } }, { declaration ("opacity", "0.5") }) }));

    ASSERT_NE (sheet, nullptr);
    ASSERT_EQ (sheet->getRules().size(), 2u);

    EXPECT_EQ (sheet->getRules()[0].selector.toString(), "Slider");
    EXPECT_EQ (sheet->getRules()[0].order, 0);
    EXPECT_EQ (sheet->getRules()[0].declarations.front().name, Identifier ("thumbColor"));
    EXPECT_EQ (sheet->getRules()[1].order, 1);
}

TEST_F (StyleSheetTests, SplitsQualifiedDeclarationNames)
{
    auto sheet = build (DataTree ("style",
                                  { DataTree ("rule", { { "selector", ".panel" } }, { declaration ("Slider.thumbColor", "#ff0000") }) }));

    ASSERT_NE (sheet, nullptr);
    const auto& first = sheet->getRules().front().declarations.front();

    EXPECT_EQ (first.qualifier, Identifier ("Slider"));
    EXPECT_EQ (first.name, Identifier ("thumbColor"));
}

TEST_F (StyleSheetTests, AppliesTheDefaultsOfDeclaredProperties)
{
    auto sheet = build (DataTree ("style",
                                  { DataTree ("rule", { { "selector", "Slider" } }, { DataTree ("property", { { "name", "glow" }, { "type", "number" }, { "value", "2" } }),
                                                                                     DataTree ("property", { { "name", "spin" }, { "type", "number" } }) }) }));

    ASSERT_NE (sheet, nullptr);
    const auto& declarations = sheet->getRules().front().declarations;

    ASSERT_EQ (declarations.size(), 1u);
    EXPECT_EQ (declarations.front().name, Identifier ("glow"));
    EXPECT_EQ (declarations.front().value, "2");
}

TEST_F (StyleSheetTests, ReadsPalettesWithRoleOverrides)
{
    auto sheet = build (DataTree ("style",
                                  { DataTree ("palette", { { "name", "Ember" }, { "colors", "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f" }, { "mode", "dark" }, { "accent", "#ff9a1f" } }) }));

    ASSERT_NE (sheet, nullptr);

    const auto* palette = sheet->findPalette ("Ember");
    ASSERT_NE (palette, nullptr);
    EXPECT_EQ (palette->getMode(), ThemePalette::Mode::dark);
    EXPECT_EQ (palette->getColor (ThemePalette::Role::accent), Color (0xffff9a1f));
    EXPECT_TRUE (sheet->getPaletteNames().contains ("Ember"));
    EXPECT_EQ (sheet->findPalette ("Missing"), nullptr);
}

TEST_F (StyleSheetTests, ReadsPalettesWithCustomColors)
{
    auto sheet = build (DataTree ("style",
                                  { DataTree ("palette", { { "name", "Ember" }, { "colors", "#100b00-#85cb33" }, { "meterHot", "#f20707" } }) }));

    ASSERT_NE (sheet, nullptr);

    const auto* palette = sheet->findPalette ("Ember");
    ASSERT_NE (palette, nullptr);
    EXPECT_EQ (palette->findCustomColor ("meterHot"), std::optional<Color> (Color (0xfff20707)));
    EXPECT_FALSE (palette->findCustomColor ("missing").has_value());
}

TEST_F (StyleSheetTests, KeepsADarkAndALightVersionOfAPalette)
{
    auto sheet = build (DataTree ("style",
                                  { DataTree ("palette", { { "name", "Skin" }, { "colors", "#100b00-#85cb33" }, { "accent", "#ff0000" } }),
                                    DataTree ("palette", { { "name", "Skin" }, { "colors", "#efffc8-#85cb33" }, { "mode", "light" }, { "accent", "#0000ff" } }),
                                    DataTree ("palette", { { "name", "Plain" }, { "colors", "#efffc8-#85cb33" }, { "mode", "light" } }) }));

    ASSERT_NE (sheet, nullptr);
    EXPECT_EQ (sheet->getPaletteNames(), StringArray ({ "Plain", "Skin" }));

    const auto* skin = sheet->findPaletteVersions ("Skin");
    ASSERT_NE (skin, nullptr);
    EXPECT_TRUE (skin->followsTheme());
    EXPECT_EQ (skin->find (ThemePalette::Mode::dark)->getColor (ThemePalette::Role::accent), Color (0xffff0000));
    EXPECT_EQ (skin->find (ThemePalette::Mode::light)->getColor (ThemePalette::Role::accent), Color (0xff0000ff));

    // A palette declared once is the one of both modes
    const auto* plain = sheet->findPaletteVersions ("Plain");
    ASSERT_NE (plain, nullptr);
    EXPECT_FALSE (plain->followsTheme());
    EXPECT_EQ (plain->find (ThemePalette::Mode::dark), plain->find (ThemePalette::Mode::light));
    EXPECT_EQ (plain->find (ThemePalette::Mode::dark)->getMode(), ThemePalette::Mode::light);
}

TEST_F (StyleSheetTests, ReadsKeyframesSortedByPosition)
{
    auto sheet = build (DataTree ("style",
                                  { DataTree ("keyframes",
                                              { { "name", "flash" } },
                                              { DataTree ("keyframe", { { "at", 1.0 } }, { declaration ("opacity", "0") }),
                                                DataTree ("keyframe", { { "at", 0.0 } }, { declaration ("opacity", "1") }) }) }));

    ASSERT_NE (sheet, nullptr);

    const auto* sequence = sheet->findKeyframes (Identifier ("flash"));
    ASSERT_NE (sequence, nullptr);
    ASSERT_EQ (sequence->frames.size(), 2u);
    EXPECT_DOUBLE_EQ (sequence->frames[0].position, 0.0);
    EXPECT_DOUBLE_EQ (sequence->frames[1].position, 1.0);
    EXPECT_EQ (sequence->frames[0].declarations.front().value, "1");
    EXPECT_EQ (sheet->findKeyframes (Identifier ("missing")), nullptr);
}

TEST_F (StyleSheetTests, CollectsWatchedProperties)
{
    auto sheet = build (DataTree ("style",
                                  { DataTree ("rule", { { "selector", "Slider[title=\"a\"]" } }, { declaration ("opacity", "1") }) }));

    ASSERT_NE (sheet, nullptr);
    EXPECT_TRUE (sheet->watchesProperty (Identifier ("title")));
    EXPECT_FALSE (sheet->watchesProperty (Identifier ("opacity")));
}

TEST_F (StyleSheetTests, ParsesWholeDocuments)
{
    auto sheet = StyleSheet::parse (R"yui(
        palette Ember { colors: "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f"; accent: #ff9a1f; }
        drawing Knob { circle { center: bounds.center; radius: 4; fill: thumbColor; } }
        style {
            .synth { palette: Ember; Slider.thumbColor: palette.accent; }
            Slider { drawing: Knob; }
        })yui");

    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();

    const auto& parsed = sheet.getReference();
    EXPECT_NE (parsed->findPalette ("Ember"), nullptr);
    EXPECT_TRUE (parsed->findDrawing ("Knob").isValid());
    EXPECT_FALSE (parsed->findDrawing ("Missing").isValid());
    ASSERT_EQ (parsed->getRules().size(), 2u);
    EXPECT_EQ (parsed->getRules()[0].declarations[1].qualifier, Identifier ("Slider"));
}

TEST_F (StyleSheetTests, FailsForInvalidInput)
{
    EXPECT_TRUE (StyleSheet::fromTree (DataTree ("other")).failed());

    EXPECT_TRUE (StyleSheet::fromTree (DataTree ("style", { DataTree ("rule", { { "selector", "" } }, { declaration ("opacity", "1") }) })).failed());

    EXPECT_TRUE (StyleSheet::fromTree (DataTree ("style", { DataTree ("rule", { { "selector", "Slider" } }, { DataTree ("declaration", { { "name", "opacity" } }) }) })).failed());

    EXPECT_TRUE (StyleSheet::fromTree (DataTree ("style", { DataTree ("palette", { { "name", "Bad" }, { "colors", "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f" }, { "dark", "#ff0000" } }) })).failed());
}

TEST_F (StyleSheetTests, ReportsTheElementOfTheFirstProblem)
{
    const auto locate = [] (const String& text)
    {
        const auto parsed = YuiParser::parse (text);
        EXPECT_TRUE (parsed.wasOk());

        YuiDiagnostic failure;
        EXPECT_TRUE (StyleSheet::fromTree (parsed.document, &failure).failed());
        return parsed.sourceMap.locate (failure).toString();
    };

    EXPECT_EQ (locate ("palette P {\n  colors: \"#100b00-#85cb33\";\n  dark: #ff0000;\n}"), "3:3: The palette 'P' can't name a color 'dark'");
    EXPECT_EQ (locate ("palette P {\n  colors: \"nope\";\n}"), "2:3: The palette 'P' is invalid: No valid colors found");
    EXPECT_EQ (locate ("palette P { colors: \"\"; }"), "1:1: The palette 'P' needs colors");
    EXPECT_EQ (locate ("palette P { colors: \"#100b00-#85cb33\"; }\npalette P { colors: \"#efffc8-#85cb33\"; }"), "2:1: The palette 'P' already has a dark version");
    EXPECT_EQ (locate ("style {\n  Slider::before { opacity: 1; }\n}"), "2:3: Pseudo-elements are not supported in selector 'Slider::before'");

    YuiDiagnostic failure;
    EXPECT_TRUE (StyleSheet::fromTree (DataTree ("style", { DataTree ("keyframes") }), &failure).failed());
    EXPECT_EQ (failure.element.getType(), Identifier ("keyframes"));

    EXPECT_TRUE (StyleSheet::fromTree (DataTree ("other"), &failure).failed());
    EXPECT_EQ (failure.toString(), "Expected a 'yui' document or a 'style' element");
    EXPECT_FALSE (failure.element.isValid());
}

TEST_F (StyleSheetTests, KeepsTheElementsOfDeclarationsInTheirDocument)
{
    auto sheet = StyleSheet::parse ("style {\n  Slider { opacity: 0.5; }\n}");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();

    const auto& element = sheet.getReference()->getRules().front().declarations.front().element;
    EXPECT_EQ (element.getType(), Identifier ("declaration"));
    EXPECT_EQ (element.getRoot().getType(), Identifier ("yui"));
}

TEST_F (StyleSheetTests, ExpandsCommaSeparatedSelectorBlocksWithPerBranchSpecificity)
{
    auto sheet = StyleSheet::parse (R"yui(
        style {
            TextButton,
            ComboBox:open {
                cursor: hand;
            }
            Slider, #special {
                opacity: 0.4;
            }
            .override {
                opacity: 0.8;
            }
        })yui");

    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();

    const auto& rules = sheet.getReference()->getRules();
    ASSERT_EQ (rules.size(), 5u);
    EXPECT_EQ (rules[0].selector.toString(), "TextButton");
    EXPECT_EQ (rules[1].selector.toString(), "ComboBox:open");
    EXPECT_TRUE (sheet.getReference()->watchesProperty (Identifier ("open")));

    Component root;
    Slider normalSlider (Slider::LinearHorizontal, "normal");
    normalSlider.addStyleClass (Identifier ("override"));
    Slider specialSlider (Slider::LinearHorizontal, "special");
    specialSlider.addStyleClass (Identifier ("override"));
    root.addChildComponent (normalSlider);
    root.addChildComponent (specialSlider);

    root.setStyleSheet (sheet.getReference());
    ASSERT_NE (root.getStyleContext(), nullptr);
    root.getStyleContext()->restyleNow();

    // `.override` (100) beats the `Slider` branch (1), while the `#special` branch (10000) beats `.override`.
    EXPECT_NEAR (normalSlider.getOpacity(), 0.8f, 1.0e-5f);
    EXPECT_NEAR (specialSlider.getOpacity(), 0.4f, 1.0e-5f);
}
