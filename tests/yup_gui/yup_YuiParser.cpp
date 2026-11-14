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

class YuiParserTest : public ::testing::Test
{
protected:
    static String parseError (const String& text)
    {
        const auto result = YuiParser::parse (text);
        return result.wasOk() ? String() : result.diagnostics.front().toString();
    }

    static String locate (const YuiParser::ParseResult& result, const DataTree& element, const Identifier& property = {})
    {
        return result.sourceMap.locate ({ .message = "here", .element = element, .property = property }).toString();
    }
};

TEST_F (YuiParserTest, ParsesDrawingBlocksIntoTrees)
{
    const auto result = YuiParser::parse (R"yui(
        drawing Knob {
            let r: bounds.width / 2 - 4;
            circle { id: well; center: bounds.center; radius: r; fill: backgroundColor; }
            layer {
                opacity: enabled ? 1 : 0.4;
                repeat i in 0 .. 11 {
                    line { from: polar (bounds.center, r, i / 10.0); to: bounds.center; }
                }
            }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();
    EXPECT_EQ (result.document.getType(), Identifier ("yui"));
    ASSERT_EQ (result.document.getNumChildren(), 1);

    const auto drawing = result.document.getChild (0);
    EXPECT_EQ (drawing.getType(), Identifier ("drawing"));
    EXPECT_EQ (drawing.getProperty ("name").toString(), "Knob");
    ASSERT_EQ (drawing.getNumChildren(), 3);

    const auto let = drawing.getChild (0);
    EXPECT_EQ (let.getType(), Identifier ("let"));
    EXPECT_EQ (let.getProperty ("name").toString(), "r");
    EXPECT_EQ (let.getProperty ("value").toString(), "bounds.width / 2 - 4");

    const auto circle = drawing.getChild (1);
    EXPECT_EQ (circle.getType(), Identifier ("circle"));
    EXPECT_EQ (circle.getNumProperties(), 4);
    EXPECT_EQ (circle.getProperty ("id").toString(), "well");
    EXPECT_EQ (circle.getProperty ("radius").toString(), "r");

    const auto layer = drawing.getChild (2);
    EXPECT_EQ (layer.getProperty ("opacity").toString(), "enabled ? 1 : 0.4");
    ASSERT_EQ (layer.getNumChildren(), 1);

    const auto repeat = layer.getChild (0);
    EXPECT_EQ (repeat.getType(), Identifier ("repeat"));
    EXPECT_EQ (repeat.getProperty ("name").toString(), "i");
    EXPECT_EQ (repeat.getProperty ("from").toString(), "0");
    EXPECT_EQ (repeat.getProperty ("to").toString(), "11");
    ASSERT_EQ (repeat.getNumChildren(), 1);
    EXPECT_EQ (repeat.getChild (0).getProperty ("from").toString(), "polar (bounds.center, r, i / 10.0)");
}

TEST_F (YuiParserTest, KeepsValuesAsWritten)
{
    const auto result = YuiParser::parse ("drawing { text { text: format (\"%.2f; }\", value) ; } }");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto drawing = result.document.getChild (0);
    EXPECT_FALSE (drawing.hasProperty ("name"));
    EXPECT_EQ (drawing.getChild (0).getProperty ("text").toString(), "format (\"%.2f; }\", value)");
}

TEST_F (YuiParserTest, ParsesSeveralBlocksAndComments)
{
    const auto result = YuiParser::parse (R"yui(
        // The first drawing
        drawing A { rect { bounds: bounds; } }

        /* The second drawing,
           on two lines */
        drawing B {
            circle { radius: 4; /* inline */ center: bounds.center; } // trailing
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();
    ASSERT_EQ (result.document.getNumChildren(), 2);
    EXPECT_EQ (result.document.getChild (1).getProperty ("name").toString(), "B");
    EXPECT_EQ (result.document.getChild (1).getChild (0).getNumProperties(), 2);
}

TEST_F (YuiParserTest, AcceptsEmptyText)
{
    const auto result = YuiParser::parse (" // nothing here\n");

    ASSERT_TRUE (result.wasOk());
    EXPECT_EQ (result.document.getNumChildren(), 0);
}

TEST_F (YuiParserTest, ReportsErrorsWithLineAndColumn)
{
    EXPECT_EQ (parseError ("drawing Knob {\n    circle { radius: 4 }\n}"), "2:24: Expected ';'");
    EXPECT_EQ (parseError ("drawing {"), "1:10: Expected '}'");
    EXPECT_EQ (parseError ("drawing { rect { bounds: 1 +* 2; } }"), "1:29: Unexpected character '*'");
    EXPECT_EQ (parseError ("widget { }"), "1:1: Unknown block 'widget', expected 'import', 'drawing', 'palette', 'style', 'tokens', 'component' or a component instance");
    EXPECT_EQ (parseError ("drawing { rect { fill: a; fill: b; } }"), "1:27: The property 'fill' is already set");
    EXPECT_EQ (parseError ("drawing { repeat i 0 .. 3 { } }"), "1:20: Expected 'in'");
    EXPECT_EQ (parseError ("drawing { let r 4; }"), "1:17: Expected ':'");
    EXPECT_EQ (parseError ("drawing { /* }"), "1:11: Unterminated comment");
}

TEST_F (YuiParserTest, ParsesPaletteBlocks)
{
    const auto result = YuiParser::parse (R"yui(
        palette Ember { colors: "#100b00-#85cb33"; mode: dark; accent: #ff9a1f; })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto palette = result.document.getChildWithName ("palette");
    ASSERT_TRUE (palette.isValid());
    EXPECT_EQ (palette.getProperty ("name").toString(), "Ember");
    EXPECT_EQ (palette.getProperty ("colors").toString(), "#100b00-#85cb33");
    EXPECT_EQ (palette.getProperty ("mode").toString(), "dark");
    EXPECT_EQ (palette.getProperty ("accent").toString(), "#ff9a1f");
}

TEST_F (YuiParserTest, ParsesStyleRulesAndKeyframes)
{
    const auto result = YuiParser::parse (R"yui(
        style {
            Slider.knob:hover, #main > Label { thumbColor: palette.accent.brighter (0.2); drawing: Knob; }
            .panel { Slider.trackWidth: 4; transition: opacity 0.2s ease-out, thumbColor 0.1s; }
            @keyframes flash { from { glow: 8; } 50% { glow: 4; } to { glow: 0; } }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto style = result.document.getChildWithName ("style");
    ASSERT_EQ (style.getNumChildren(), 3);

    const auto first = style.getChild (0);
    EXPECT_EQ (first.getType().toString(), "rule");
    EXPECT_EQ (first.getProperty ("selector").toString(), "Slider.knob:hover, #main > Label");
    ASSERT_EQ (first.getNumChildren(), 2);
    EXPECT_EQ (first.getChild (0).getProperty ("name").toString(), "thumbColor");
    EXPECT_EQ (first.getChild (0).getProperty ("value").toString(), "palette.accent.brighter (0.2)");
    EXPECT_EQ (first.getChild (1).getProperty ("value").toString(), "Knob");

    const auto second = style.getChild (1);
    EXPECT_EQ (second.getChild (0).getProperty ("name").toString(), "Slider.trackWidth");
    EXPECT_EQ (second.getChild (1).getProperty ("value").toString(), "opacity 0.2s ease-out, thumbColor 0.1s");

    const auto keyframes = style.getChild (2);
    EXPECT_EQ (keyframes.getType().toString(), "keyframes");
    EXPECT_EQ (keyframes.getProperty ("name").toString(), "flash");
    ASSERT_EQ (keyframes.getNumChildren(), 3);
    EXPECT_DOUBLE_EQ (static_cast<double> (keyframes.getChild (1).getProperty ("at")), 0.5);
    EXPECT_DOUBLE_EQ (static_cast<double> (keyframes.getChild (2).getProperty ("at")), 1.0);
}

TEST_F (YuiParserTest, ReportsStyleErrors)
{
    EXPECT_EQ (parseError ("style { Slider { opacity: 1 } }"), "1:29: Expected ';'");
    EXPECT_EQ (parseError ("style { Slider { opacity: ; } }"), "1:18: The declaration 'opacity' needs a value");
    EXPECT_EQ (parseError ("style { { opacity: 1; } }"), "1:9: Expected a selector");
    EXPECT_EQ (parseError ("style { @keyframes f { half { glow: 1; } } }"), "1:24: Expected 'from', 'to' or a percentage");
    EXPECT_EQ (parseError ("style { @keyframes f { 150% { glow: 1; } } }"), "1:24: The keyframe position must be between 0% and 100%");
    EXPECT_EQ (parseError ("palette P { mode: dark; }"), "1:25: The palette needs 'colors'");
    EXPECT_EQ (parseError ("style { Label { text: \"a;b\"; } }"), String());
}

TEST_F (YuiParserTest, ParsesTheRootComponent)
{
    const auto result = YuiParser::parse (R"yui(
        Component {
            id: panel; width: 200;
            Slider { id: knob; class: "knob big"; x: parent.width / 4; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto root = result.document.getChildWithName ("Component");
    ASSERT_TRUE (root.isValid());
    EXPECT_EQ (root.getProperty ("width").toString(), "200");

    const auto knob = root.getChildWithName ("Slider");
    EXPECT_EQ (knob.getProperty ("class").toString(), "\"knob big\"");
    EXPECT_EQ (knob.getProperty ("x").toString(), "parent.width / 4");

    EXPECT_EQ (parseError ("Component { } Component { }"), "1:15: A document can only have one root component");
}

TEST_F (YuiParserTest, ParsesTokens)
{
    const auto result = YuiParser::parse (R"yui(
        tokens {
            property glowColor: color = palette.accent.withAlpha (0.4);
            property knobTrack: number = 0.075;
            property label: string;
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto tokens = result.document.getChildWithName ("tokens");
    ASSERT_EQ (tokens.getNumChildren(), 3);

    const auto glow = tokens.getChild (0);
    EXPECT_EQ (glow.getType().toString(), "property");
    EXPECT_EQ (glow.getProperty ("name").toString(), "glowColor");
    EXPECT_EQ (glow.getProperty ("type").toString(), "color");
    EXPECT_EQ (glow.getProperty ("value").toString(), "palette.accent.withAlpha (0.4)");

    EXPECT_EQ (tokens.getChild (1).getProperty ("value").toString(), "0.075");
    EXPECT_FALSE (tokens.getChild (2).hasProperty ("value"));
}

TEST_F (YuiParserTest, ParsesDrawingProperties)
{
    const auto result = YuiParser::parse (R"yui(
        drawing Knob {
            property bezel: number = 0.8;
            property ring: color;
            circle { radius: bezel; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto drawing = result.document.getChildWithName ("drawing");
    ASSERT_EQ (drawing.getNumChildren(), 3);

    const auto bezel = drawing.getChild (0);
    EXPECT_EQ (bezel.getType().toString(), "property");
    EXPECT_EQ (bezel.getProperty ("name").toString(), "bezel");
    EXPECT_EQ (bezel.getProperty ("type").toString(), "number");
    EXPECT_EQ (bezel.getProperty ("value").toString(), "0.8");
    EXPECT_FALSE (drawing.getChild (1).hasProperty ("value"));

    EXPECT_EQ (parseError ("drawing { property a: number; property a: color; }"), "1:40: 'a' is already declared");
    EXPECT_EQ (parseError ("drawing { layer { property a: number; } }"), "1:28: Expected ':'");
}

TEST_F (YuiParserTest, ParsesComponents)
{
    const auto result = YuiParser::parse (R"yui(
        component LabeledKnob : Column {
            property label: string = "";
            property alias value <=> knob.value;
            signal changed (value: number, final: boolean);
            signal reset;

            gap: 4;
            Slider { id: knob; flexGrow: 1; onValueChanged: root.changed (self.value, false); }
            Label { text: root.label; }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto component = result.document.getChildWithName ("component");
    EXPECT_EQ (component.getProperty ("name").toString(), "LabeledKnob");
    EXPECT_EQ (component.getProperty ("base").toString(), "Column");
    ASSERT_EQ (component.getNumChildren(), 5);

    const auto label = component.getChild (0);
    EXPECT_EQ (label.getType().toString(), "property");
    EXPECT_EQ (label.getProperty ("value").toString(), "\"\"");

    const auto alias = component.getChild (1);
    EXPECT_EQ (alias.getType().toString(), "alias");
    EXPECT_EQ (alias.getProperty ("name").toString(), "value");
    EXPECT_EQ (alias.getProperty ("target").toString(), "knob.value");

    const auto changed = component.getChild (2);
    EXPECT_EQ (changed.getType().toString(), "signal");
    EXPECT_EQ (changed.getProperty ("name").toString(), "changed");
    ASSERT_EQ (changed.getNumChildren(), 2);
    EXPECT_EQ (changed.getChild (1).getProperty ("name").toString(), "final");
    EXPECT_EQ (changed.getChild (1).getProperty ("type").toString(), "boolean");

    EXPECT_EQ (component.getChild (3).getNumChildren(), 0);

    const auto body = component.getChild (4);
    EXPECT_EQ (body.getType().toString(), "Column");
    EXPECT_EQ (body.getProperty ("gap").toString(), "4");
    ASSERT_EQ (body.getNumChildren(), 2);
    EXPECT_EQ (body.getChild (0).getProperty ("onValueChanged").toString(), "root.changed (self.value, false)");
}

TEST_F (YuiParserTest, ParsesTwoWayBindings)
{
    const auto result = YuiParser::parse ("Column { LabeledKnob { label: \"Cutoff\"; value <=> params.cutoff; } }");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto knob = result.document.getChildWithName ("Column").getChild (0);
    ASSERT_EQ (knob.getNumChildren(), 1);

    const auto binding = knob.getChild (0);
    EXPECT_EQ (binding.getType().toString(), "bind");
    EXPECT_EQ (binding.getProperty ("property").toString(), "value");
    EXPECT_EQ (binding.getProperty ("target").toString(), "params.cutoff");

    EXPECT_EQ (parseError ("Column { value: 1; value <=> a.b; }"), "1:20: The property 'value' is already set");
}

TEST_F (YuiParserTest, ParsesPropertyDeclarationsInStyleRules)
{
    const auto result = YuiParser::parse ("style { Slider.knob { property glow: number = 0; transition: glow 150ms; } }");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto rule = result.document.getChildWithName ("style").getChild (0);
    ASSERT_EQ (rule.getNumChildren(), 2);
    EXPECT_EQ (rule.getChild (0).getType().toString(), "property");
    EXPECT_EQ (rule.getChild (0).getProperty ("value").toString(), "0");
    EXPECT_EQ (rule.getChild (1).getType().toString(), "declaration");

    EXPECT_EQ (parseError ("style { @keyframes f { from { property glow: number = 0; } } }"), "1:40: Expected ':'");
}

TEST_F (YuiParserTest, ReportsDeclarationErrors)
{
    EXPECT_EQ (parseError ("tokens { glow: 1; }"), "1:10: Expected 'property'");
    EXPECT_EQ (parseError ("tokens { property glow: size = 1; }"), "1:25: Unknown type 'size', expected one of number, integer, boolean, string, color, point, rectangle, font, drawing, any");
    EXPECT_EQ (parseError ("tokens { property a: number; property a: color; }"), "1:39: 'a' is already declared");
    EXPECT_EQ (parseError ("component knob : Column { }"), "1:11: Type names start with an upper case letter");
    EXPECT_EQ (parseError ("component Knob { }"), "1:16: Expected ':'");
    EXPECT_EQ (parseError ("component K : Column { signal s (v: number; }"), "1:43: Expected ')'");
    EXPECT_EQ (parseError ("component K : Column { property alias v <=> ; }"), "1:45: Expected the property the alias stands for");
    EXPECT_EQ (parseError ("Component { value <=> ; }"), "1:23: Expected the property to bind to");
}

TEST_F (YuiParserTest, ResolvesImports)
{
    std::map<String, String> files {
        { "palette.yui", "palette P { colors: \"#100b00-#85cb33\"; }" },
        { "style.yui", "import \"palette.yui\"; style { Slider { palette: P; } }" },
        { "broken.yui", "\n style {" }
    };

    const auto resolver = [&] (const String& path) -> std::optional<String>
    {
        if (const auto it = files.find (path); it != files.end())
            return it->second;

        return std::nullopt;
    };

    const auto result = YuiParser::parse ("import \"style.yui\"; import \"palette.yui\"; Component { }", resolver);
    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto& document = result.document;
    ASSERT_EQ (document.getNumChildren(), 3);
    EXPECT_EQ (document.getChild (0).getType().toString(), "palette");
    EXPECT_EQ (document.getChild (1).getType().toString(), "style");
    EXPECT_EQ (document.getChild (2).getType().toString(), "Component");

    EXPECT_EQ (locate (result, document.getChild (0), "colors"), "palette.yui:1:13: here");
    EXPECT_EQ (locate (result, document.getChild (1).getChild (0)), "style.yui:1:31: here");
    EXPECT_EQ (locate (result, document.getChild (2)), "1:43: here");

    const auto missing = YuiParser::parse ("import \"missing.yui\";", resolver);
    ASSERT_FALSE (missing.wasOk());
    EXPECT_EQ (missing.diagnostics.front().toString(), "1:1: Can't import 'missing.yui'");

    files["nested.yui"] = "\n  import \"missing.yui\";";
    const auto nested = YuiParser::parse ("import \"nested.yui\";", resolver);
    ASSERT_FALSE (nested.wasOk());
    EXPECT_EQ (nested.diagnostics.front().toString(), "nested.yui:2:3: Can't import 'missing.yui'");

    const auto broken = YuiParser::parse ("import \"broken.yui\";", resolver);
    ASSERT_FALSE (broken.wasOk());
    EXPECT_EQ (broken.diagnostics.front().toString(), "broken.yui:2:9: Expected '}'");
}

TEST_F (YuiParserTest, CountsColumnsInCharacters)
{
    EXPECT_EQ (parseError (String::fromUTF8 ("drawing { text { text: \"\xc3\xa9\"; fill: ; } }")), "1:35: Unexpected character ';'");

    const auto result = YuiParser::parse (String::fromUTF8 ("drawing { text { text: \"\xc3\xa9\"; fill: red; } }"));
    EXPECT_EQ (locate (result, result.document.getChild (0).getChild (0), "fill"), "1:29: here");
}

TEST_F (YuiParserTest, RecordsWhereElementsAndPropertiesWereWritten)
{
    const auto result = YuiParser::parse (R"yui(palette P { colors: "#100b00-#85cb33"; primary: #ff0000; }
style {
  .a { opacity: 1; property glow: number = 0; }
  @keyframes f { from { glow: 0; } }
}
component K : Column {
  property alias v <=> s.value;
  signal changed;
  Slider { id: s; }
}
Column {
  gap: 4;
  value <=> m.x;
  Slider { }
}
drawing D { let r: 1; repeat i in 0 .. 2 { circle { radius: r; } } })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto& document = result.document;
    const auto palette = document.getChild (0);
    EXPECT_EQ (locate (result, palette), "1:1: here");
    EXPECT_EQ (locate (result, palette, "primary"), "1:40: here");

    const auto rule = document.getChild (1).getChild (0);
    EXPECT_EQ (locate (result, document.getChild (1)), "2:1: here");
    EXPECT_EQ (locate (result, rule), "3:3: here");
    EXPECT_EQ (locate (result, rule.getChild (0)), "3:8: here");
    EXPECT_EQ (locate (result, rule.getChild (1)), "3:29: here");

    const auto keyframes = document.getChild (1).getChild (1);
    EXPECT_EQ (locate (result, keyframes), "4:4: here");
    EXPECT_EQ (locate (result, keyframes.getChild (0)), "4:18: here");
    EXPECT_EQ (locate (result, keyframes.getChild (0).getChild (0)), "4:25: here");

    const auto component = document.getChild (2);
    EXPECT_EQ (locate (result, component), "6:1: here");
    EXPECT_EQ (locate (result, component.getChild (0)), "7:12: here");
    EXPECT_EQ (locate (result, component.getChild (1)), "8:10: here");
    EXPECT_EQ (locate (result, component.getChild (2)), "6:15: here");
    EXPECT_EQ (locate (result, component.getChild (2).getChild (0), "id"), "9:12: here");

    const auto root = document.getChild (3);
    EXPECT_EQ (locate (result, root), "11:1: here");
    EXPECT_EQ (locate (result, root, "gap"), "12:3: here");
    EXPECT_EQ (locate (result, root.getChild (0)), "13:3: here");
    EXPECT_EQ (locate (result, root.getChild (1)), "14:3: here");

    const auto drawing = document.getChild (4);
    EXPECT_EQ (locate (result, drawing), "16:1: here");
    EXPECT_EQ (locate (result, drawing.getChild (0)), "16:13: here");
    EXPECT_EQ (locate (result, drawing.getChild (1)), "16:23: here");
    EXPECT_EQ (locate (result, drawing.getChild (1).getChild (0)), "16:44: here");
    EXPECT_EQ (locate (result, drawing.getChild (1).getChild (0), "radius"), "16:53: here");
}

TEST_F (YuiParserTest, ReturnsNoDocumentOnErrors)
{
    const auto result = YuiParser::parse ("drawing { rect { bounds: ; } }");

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.document.isValid());
    EXPECT_EQ (result.diagnostics.size(), 1u);
}
