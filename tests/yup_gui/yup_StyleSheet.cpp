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

    EXPECT_TRUE (StyleSheet::fromTree (DataTree ("style", { DataTree ("palette", { { "name", "Bad" }, { "colors", "#100b00-#85cb33-#efffc8-#a5cbc3-#3b341f" }, { "notARole", "#ff0000" } }) })).failed());
}
