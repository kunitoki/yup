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

#include <yup_gui_markup/yup_gui_markup.h>

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

TEST_F (YuiParserTest, ParsesNamedBlocksInsideDrawings)
{
    const auto result = YuiParser::parse (R"yui(
        drawing Icon {
            viewBox: rect (0, 0, 24, 24);
            drawing Leaf { path { data: "M 5 0 L 10 10 L 0 10 Z"; } }
            pattern Dots { tile: rect (0, 0, 4, 4); circle { center: point (2, 2); radius: 1; } }
            marker Arrow { size: size (6, 6); orient: auto; }
            group {
                fill: none;
                use Leaf { bounds: rect (2, 2, 8, 8); fill: #00ff00; }
                clip { clipPath { circle { center: bounds.center; radius: 8; } } }
            }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto drawing = result.document.getChild (0);
    EXPECT_EQ (drawing.getProperty ("viewBox").toString(), "rect (0, 0, 24, 24)");
    ASSERT_EQ (drawing.getNumChildren(), 4);

    const auto leaf = drawing.getChild (0);
    EXPECT_EQ (leaf.getType(), Identifier ("drawing"));
    EXPECT_EQ (leaf.getProperty ("name").toString(), "Leaf");
    EXPECT_EQ (leaf.getNumChildren(), 1);

    const auto dots = drawing.getChild (1);
    EXPECT_EQ (dots.getType(), Identifier ("pattern"));
    EXPECT_EQ (dots.getProperty ("name").toString(), "Dots");
    EXPECT_EQ (dots.getProperty ("tile").toString(), "rect (0, 0, 4, 4)");

    const auto arrow = drawing.getChild (2);
    EXPECT_EQ (arrow.getType(), Identifier ("marker"));
    EXPECT_EQ (arrow.getProperty ("name").toString(), "Arrow");
    EXPECT_EQ (arrow.getProperty ("orient").toString(), "auto");

    const auto group = drawing.getChild (3);
    EXPECT_EQ (group.getProperty ("fill").toString(), "none");
    ASSERT_EQ (group.getNumChildren(), 2);

    const auto use = group.getChild (0);
    EXPECT_EQ (use.getType(), Identifier ("use"));
    EXPECT_EQ (use.getProperty ("drawing").toString(), "Leaf");
    EXPECT_EQ (use.getProperty ("fill").toString(), "#00ff00");
    EXPECT_EQ (group.getChild (1).getChild (0).getType(), Identifier ("clipPath"));

    // Outside of a drawing these are ordinary names, and a name is expected after them inside one
    EXPECT_TRUE (parseError ("Column { use Leaf { } }").isNotEmpty());
    EXPECT_TRUE (parseError ("drawing { pattern { tile: bounds; } }").isEmpty());
    EXPECT_TRUE (parseError ("drawing { use: 1; }").isEmpty());
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
    EXPECT_EQ (parseError ("widget { }"), "1:1: Unknown block 'widget', expected 'import', 'drawing', 'shader', 'palette', 'style', 'tokens', 'component' or a component instance");
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

TEST_F (YuiParserTest, ParsesShaderBlocks)
{
    const auto result = YuiParser::parse (R"yui(
        shader Crt {
            fragment: "fx/crt.frag";
            property intensity: number = 0.5;
            property tint: color = palette.accent;
            property mask: point;
        }

        shader Hologram {
            material: "fx/hologram.glsl";
            property rim: color = #00ffff;
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();
    ASSERT_EQ (result.document.getNumChildren(), 2);

    const auto crt = result.document.getChild (0);
    EXPECT_EQ (crt.getType(), Identifier ("shader"));
    EXPECT_EQ (crt.getNumProperties(), 2);
    EXPECT_EQ (crt.getProperty ("name").toString(), "Crt");
    EXPECT_EQ (crt.getProperty ("fragment").toString(), "fx/crt.frag");
    ASSERT_EQ (crt.getNumChildren(), 3);

    const auto intensity = crt.getChild (0);
    EXPECT_EQ (intensity.getType().toString(), "property");
    EXPECT_EQ (intensity.getProperty ("name").toString(), "intensity");
    EXPECT_EQ (intensity.getProperty ("type").toString(), "number");
    EXPECT_EQ (intensity.getProperty ("value").toString(), "0.5");

    EXPECT_EQ (crt.getChild (1).getProperty ("type").toString(), "color");
    EXPECT_EQ (crt.getChild (1).getProperty ("value").toString(), "palette.accent");
    EXPECT_EQ (crt.getChild (2).getProperty ("type").toString(), "point");
    EXPECT_FALSE (crt.getChild (2).hasProperty ("value"));

    const auto hologram = result.document.getChild (1);
    EXPECT_EQ (hologram.getNumProperties(), 2);
    EXPECT_EQ (hologram.getProperty ("name").toString(), "Hologram");
    EXPECT_EQ (hologram.getProperty ("material").toString(), "fx/hologram.glsl");
    ASSERT_EQ (hologram.getNumChildren(), 1);
    EXPECT_EQ (hologram.getChild (0).getProperty ("value").toString(), "#00ffff");

    // Any member is taken as it is written, the stylesheet checks what it means
    const auto loose = YuiParser::parse ("shader S { vertex: fx/crt.vert; property: 1; }");
    ASSERT_TRUE (loose.wasOk()) << loose.diagnostics.front().toString();
    EXPECT_EQ (loose.document.getChild (0).getProperty ("vertex").toString(), "fx/crt.vert");
    EXPECT_EQ (loose.document.getChild (0).getProperty ("property").toString(), "1");
    EXPECT_EQ (loose.document.getChild (0).getNumChildren(), 0);

    const auto located = YuiParser::parse ("shader Crt {\n  fragment: \"a.frag\";\n  property gain: number;\n}");
    ASSERT_TRUE (located.wasOk()) << located.diagnostics.front().toString();

    const auto shader = located.document.getChild (0);
    EXPECT_EQ (locate (located, shader), "1:1: here");
    EXPECT_EQ (locate (located, shader, "fragment"), "2:3: here");
    EXPECT_EQ (locate (located, shader.getChild (0)), "3:12: here");
}

TEST_F (YuiParserTest, ReportsShaderErrors)
{
    EXPECT_EQ (parseError ("shader { }"), "1:8: Expected the name of the shader");
    EXPECT_EQ (parseError ("shader S { fragment \"a\"; }"), "1:21: Expected ':'");
    EXPECT_EQ (parseError ("shader S { fragment: \"a\"; fragment: \"b\"; }"), "1:27: The property 'fragment' is already set");
    EXPECT_EQ (parseError ("shader S {\n  fragment: \"a\";\n  property a: number;\n  property a: color;\n}"), "4:12: 'a' is already declared");

    // The name is the one written after the keyword
    EXPECT_EQ (parseError ("shader S { name: T; }"), "1:12: The property 'name' is already set");

    // A shader is a block of the document, not an element of a drawing
    EXPECT_EQ (parseError ("drawing { shader S { fragment: \"a\"; } }"), "1:18: Expected ':'");
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

TEST_F (YuiParserTest, ParsesArrayTypesAndLiterals)
{
    const auto result = YuiParser::parse (R"yui(
        component Picker : Column {
            property items: string[] = ["a", "b"];
            property lanes: number[];
            property offset: transform = transform.translation (4, 8);
            property steps: transform[];
            property titleFont: font;
            property fonts: font[];
            property badge: drawing;
            property payload: any;

            Column {
                values: [];
                mixed: [1, model.gain, [2, 3]];
            }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto component = result.document.getChildWithName ("component");

    const auto items = component.getChild (0);
    EXPECT_EQ (items.getProperty ("type").toString(), "string[]");
    EXPECT_EQ (items.getProperty ("value").toString(), "[\"a\", \"b\"]");
    EXPECT_EQ (component.getChild (1).getProperty ("type").toString(), "number[]");
    EXPECT_EQ (component.getChild (2).getProperty ("type").toString(), "transform");
    EXPECT_EQ (component.getChild (2).getProperty ("value").toString(), "transform.translation (4, 8)");
    EXPECT_EQ (component.getChild (3).getProperty ("type").toString(), "transform[]");
    EXPECT_EQ (component.getChild (4).getProperty ("type").toString(), "font");
    EXPECT_EQ (component.getChild (5).getProperty ("type").toString(), "font[]");
    EXPECT_EQ (component.getChild (6).getProperty ("type").toString(), "drawing");
    EXPECT_EQ (component.getChild (7).getProperty ("type").toString(), "any");

    const auto body = component.getChildWithName ("Column").getChildWithName ("Column");
    EXPECT_EQ (body.getProperty ("values").toString(), "[]");
    EXPECT_EQ (body.getProperty ("mixed").toString(), "[1, model.gain, [2, 3]]");

    EXPECT_EQ (parseError ("tokens { property a: drawing[] = 1; }"), "1:22: 'drawing' has no array type");
    EXPECT_EQ (parseError ("tokens { property a: any[] = 1; }"), "1:22: 'any' has no array type");
    EXPECT_EQ (parseError ("tokens { property a: number[ = 1; }"), "1:30: Expected ']'");
    EXPECT_EQ (parseError ("Component { items: [1, ]; }"), "1:24: Unexpected character ']'");
    EXPECT_EQ (parseError ("Component { items: [1 2]; }"), "1:23: Expected ',' or ']'");
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

TEST_F (YuiParserTest, ParsesATwoWayBindingToAnElementOfAList)
{
    const auto result = YuiParser::parse ("Row { ToggleButton { checked <=> seq.steps[i + 1]; } }");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto binding = result.document.getChildWithName ("Row").getChild (0).getChild (0);
    EXPECT_EQ (binding.getProperty ("property").toString(), "checked");
    EXPECT_EQ (binding.getProperty ("target").toString(), "seq.steps[i + 1]");

    EXPECT_TRUE (parseError ("Row { ToggleButton { checked <=> seq.steps[2; } }").contains ("Expected ']'"));
}

TEST_F (YuiParserTest, ParsesARepeatOfComponents)
{
    const auto result = YuiParser::parse ("Row {\n"
                                          "    repeat i in 0 .. len (seq.steps) {\n"
                                          "        ToggleButton { text: i; }\n"
                                          "        repeat j in 0 .. i { Label { } }\n"
                                          "    }\n"
                                          "}");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto repeat = result.document.getChildWithName ("Row").getChild (0);
    EXPECT_EQ (repeat.getType().toString(), "repeat");
    EXPECT_EQ (repeat.getProperty ("name").toString(), "i");
    EXPECT_EQ (repeat.getProperty ("from").toString(), "0");
    EXPECT_EQ (repeat.getProperty ("to").toString(), "len (seq.steps)");
    ASSERT_EQ (repeat.getNumChildren(), 2);
    EXPECT_EQ (repeat.getChild (0).getType().toString(), "ToggleButton");
    EXPECT_EQ (repeat.getChild (1).getChild (0).getType().toString(), "Label");
}

TEST_F (YuiParserTest, ParsesHandlerArgumentsBlocksAndChangedHandlers)
{
    const auto result = YuiParser::parse ("Column {\n"
                                          "    Slider { onValueChanged: (v) => action (\"set\", v); changed value: { action (\"a\"); root.moved (); } }\n"
                                          "    Label { changed: 1; onClick: () => { }; }\n"
                                          "}");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto column = result.document.getChildWithName ("Column");
    const auto slider = column.getChild (0);
    EXPECT_EQ (slider.getProperty ("onValueChanged").toString(), "(v) => action (\"set\", v)");
    ASSERT_EQ (slider.getNumChildren(), 1);

    const auto changed = slider.getChild (0);
    EXPECT_EQ (changed.getType().toString(), "changed");
    EXPECT_EQ (changed.getProperty ("property").toString(), "value");
    EXPECT_EQ (changed.getProperty ("handler").toString(), "{ action (\"a\"); root.moved (); }");
    EXPECT_EQ (locate (result, slider, "onValueChanged"), "2:14: here");
    EXPECT_EQ (locate (result, changed, "handler"), "2:56: here");

    // A property named changed is still a property
    const auto label = column.getChild (1);
    EXPECT_EQ (label.getProperty ("changed").toString(), "1");
    EXPECT_EQ (label.getProperty ("onClick").toString(), "() => { }");

    // Parentheses that aren't argument names are part of the value
    const auto values = YuiParser::parse ("Column { a: (b) * 2; c: (d + e); f: (1); }");
    ASSERT_TRUE (values.wasOk()) << values.diagnostics.front().toString();

    const auto valuesColumn = values.document.getChildWithName ("Column");
    EXPECT_EQ (valuesColumn.getProperty ("a").toString(), "(b) * 2");
    EXPECT_EQ (valuesColumn.getProperty ("c").toString(), "(d + e)");
    EXPECT_EQ (valuesColumn.getProperty ("f").toString(), "(1)");

    EXPECT_EQ (parseError ("Column { changed value: x; changed value: y; }"), "1:36: A changed handler for 'value' is already set");
    EXPECT_EQ (parseError ("Column { onClick: { action (\"a\") }; }"), "1:34: Expected ';'");
    EXPECT_EQ (parseError ("Column { onClick: { action (\"a\");"), "1:34: Expected '}'");
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
    EXPECT_EQ (parseError ("tokens { property glow: size = 1; }"), "1:25: Unknown type 'size', expected one of number, integer, boolean, string, color, point, rectangle, transform, font, drawing, any");
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

TEST_F (YuiParserTest, BuildsTheStyleSheetOfATextOrTellsItsFirstProblem)
{
    auto sheet = YuiParser::parseStyleSheet ("drawing Knob { circle { center: bounds.center; radius: 4; } }\nstyle { Slider { drawing: Knob; } }");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    EXPECT_TRUE (sheet.getReference()->findDrawing ("Knob").isValid());
    EXPECT_EQ (sheet.getReference()->getRules().size(), 1u);

    // A text that can't be parsed fails with where, one whose sheet can't be built with why
    EXPECT_EQ (YuiParser::parseStyleSheet ("style {").getErrorMessage(), "1:8: Expected '}'");

    const auto unsupported = YuiParser::parseStyleSheet ("style { Slider::before { opacity: 1; } }");
    EXPECT_TRUE (unsupported.failed());
    EXPECT_TRUE (unsupported.getErrorMessage().contains ("Pseudo-elements are not supported")) << unsupported.getErrorMessage();
}

TEST_F (YuiParserTest, ReturnsNoDocumentOnErrors)
{
    const auto result = YuiParser::parse ("drawing { rect { bounds: ; } }");

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.document.isValid());
    EXPECT_EQ (result.diagnostics.size(), 1u);
}

TEST_F (YuiParserTest, StripsCommentsInsideRawStyleValuesSelectorsAndExpressions)
{
    const auto result = YuiParser::parse (R"yui(
        style {
            Slider/*attach*/.knob:hover {
                filter: hue-rotate(value * 180deg)
                        /*brightness(1 + value * 0.2);
                        drop-shadow(0 4px 8px palette.accent.withAlpha(value * 0.6))*/;
                label: "http://example/*not-a-comment*/;ok"; // end of line ; }
            }
        })yui");

    ASSERT_TRUE (result.wasOk()) << result.diagnostics.front().toString();

    const auto rule = result.document.getChildWithName ("style").getChild (0);
    EXPECT_EQ (rule.getProperty ("selector").toString(), "Slider.knob:hover");
    ASSERT_EQ (rule.getNumChildren(), 2);
    EXPECT_EQ (rule.getChild (0).getProperty ("value").toString(), "hue-rotate(value * 180deg)");
    EXPECT_EQ (rule.getChild (1).getProperty ("value").toString(), "\"http://example/*not-a-comment*/;ok\"");

    EXPECT_EQ (parseError ("style { Slider { filter: blur(4px) /* unclosed } }"), "1:36: Unterminated comment");

    Component comp;
    auto scalarExpr = PropertyExpression::compile (comp, "2 + 3 // trailing comment", ComponentProperty::Type::number);
    ASSERT_TRUE (scalarExpr.wasOk()) << scalarExpr.getErrorMessage();
    EXPECT_DOUBLE_EQ (static_cast<double> (scalarExpr.getReference()->evaluate()), 5.0);

    auto arrayExpr = PropertyExpression::compile (comp, "[1, /* mid */ 2, // line\n 3]", ComponentProperty::Type::array);
    ASSERT_TRUE (arrayExpr.wasOk()) << arrayExpr.getErrorMessage();
    const auto evaluated = arrayExpr.getReference()->evaluate();
    const auto* evaluatedArray = evaluated.getArray();
    ASSERT_NE (evaluatedArray, nullptr);
    ASSERT_EQ (evaluatedArray->size(), 3);
    EXPECT_DOUBLE_EQ (static_cast<double> ((*evaluatedArray)[0]), 1.0);
    EXPECT_DOUBLE_EQ (static_cast<double> ((*evaluatedArray)[1]), 2.0);
    EXPECT_DOUBLE_EQ (static_cast<double> ((*evaluatedArray)[2]), 3.0);

    auto unclosedArray = PropertyExpression::compile (comp, "[1, /* unclosed]", ComponentProperty::Type::array);
    EXPECT_TRUE (unclosedArray.failed());
}

TEST_F (YuiParserTest, ReportsElementsNestedTooDeeply)
{
    const auto nested = [] (int depth)
    {
        return String::repeatedString ("Component { ", depth) + String::repeatedString ("}", depth);
    };

    EXPECT_TRUE (YuiParser::parse (nested (64)).wasOk());

    const auto parsed = YuiParser::parse (nested (200));
    ASSERT_FALSE (parsed.wasOk());
    EXPECT_TRUE (parsed.diagnostics.front().message.contains ("nested")) << parsed.diagnostics.front().message;
}

