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
    EXPECT_EQ (parseError ("style { }"), "1:1: Unknown block 'style', expected 'drawing'");
    EXPECT_EQ (parseError ("drawing { rect { fill: a; fill: b; } }"), "1:27: The property 'fill' is already set");
    EXPECT_EQ (parseError ("drawing { repeat i 0 .. 3 { } }"), "1:20: Expected 'in'");
    EXPECT_EQ (parseError ("drawing { let r 4; }"), "1:17: Expected ':'");
    EXPECT_EQ (parseError ("drawing { /* }"), "1:11: Unterminated comment");
}

TEST_F (YuiParserTest, CountsColumnsInCharacters)
{
    EXPECT_EQ (parseError (String::fromUTF8 ("drawing { text { text: \"\xc3\xa9\"; fill: ; } }")), "1:35: Unexpected character ';'");
}

TEST_F (YuiParserTest, ReturnsNoDocumentOnErrors)
{
    const auto result = YuiParser::parse ("drawing { rect { bounds: ; } }");

    EXPECT_FALSE (result.wasOk());
    EXPECT_FALSE (result.document.isValid());
    EXPECT_EQ (result.diagnostics.size(), 1u);
}
