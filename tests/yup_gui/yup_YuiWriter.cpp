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

class YuiWriterTests : public ::testing::Test
{
protected:
    static DataTree parse (const String& text)
    {
        const auto result = YuiParser::parse (text);
        EXPECT_TRUE (result.wasOk()) << (result.wasOk() ? String() : result.diagnostics.front().toString());
        return result.document;
    }

    static void expectRoundTrip (const String& text)
    {
        const auto document = parse (text);
        const auto written = YuiWriter::write (document);
        const auto reparsed = parse (written);

        EXPECT_TRUE (reparsed.isEquivalentTo (document)) << written;
        EXPECT_EQ (YuiWriter::write (reparsed), written);
    }
};

TEST_F (YuiWriterTests, WritesDocumentsInTheCanonicalLayout)
{
    const auto document = parse (R"yui(
        import "palette.yui"; import "style.yui";
        drawing Dot { let r: bounds.width / 2; circle { center: bounds.center; radius: r; } }
        style { .panel { padding: 8; } @keyframes pulse { from { opacity: 1; } 50% { opacity: 0.5; } } }
        Column { gap: 4; Label { text: "Hi"; } Component {} })yui");

    EXPECT_EQ (YuiWriter::write (document),
               "import \"palette.yui\";\n"
               "import \"style.yui\";\n"
               "\n"
               "drawing Dot {\n"
               "    let r: bounds.width / 2;\n"
               "    circle {\n"
               "        center: bounds.center;\n"
               "        radius: r;\n"
               "    }\n"
               "}\n"
               "\n"
               "style {\n"
               "    .panel {\n"
               "        padding: 8;\n"
               "    }\n"
               "    @keyframes pulse {\n"
               "        from {\n"
               "            opacity: 1;\n"
               "        }\n"
               "        50% {\n"
               "            opacity: 0.5;\n"
               "        }\n"
               "    }\n"
               "}\n"
               "\n"
               "Column {\n"
               "    gap: 4;\n"
               "    Label {\n"
               "        text: \"Hi\";\n"
               "    }\n"
               "    Component {}\n"
               "}\n");
}

TEST_F (YuiWriterTests, RoundTripsDrawings)
{
    expectRoundTrip (R"yui(
        drawing Knob {
            property bezel: number = 0.8;
            property ring: color;
            let r: min (bounds.width, bounds.height) / 2 - 4;
            circle { id: well; center: bounds.center; radius: r; fill: palette.surface; }
            layer {
                opacity: enabled ? 1 : 0.4;
                repeat i in 0 .. 10 {
                    line { from: polar (bounds.center, r, i / 10.0); to: bounds.center; stroke: palette.outline; }
                }
            }
        }
        drawing { rect { bounds: bounds; } }
        drawing Empty {})yui");
}

TEST_F (YuiWriterTests, RoundTripsPalettesAndStyles)
{
    expectRoundTrip (R"yui(
        palette Porcelain {
            mode: light;
            colors: "#ece9e1 #9aa3c0 #8d8b85";
            accent: #9aa3c0;
            label: "a; b";
        }
        style {
            Slider.knob, #gain:hovered {
                property glow: number = 0.5;
                Slider.thumbColor: palette.accent;
                transition: opacity 200ms ease-out;
            }
            .empty {}
            @keyframes pulse {
                from { opacity: 0; }
                12.5% { opacity: 0.2; }
                30% { opacity: 0.6; }
                to { opacity: 1; }
            }
        })yui");
}

TEST_F (YuiWriterTests, RoundTripsTokensComponentsAndInstances)
{
    expectRoundTrip (R"yui(
        tokens {
            property spacing: number = 8;
            property title: string;
        }
        component LabeledKnob : Column {
            property label: string = "Gain";
            property alias value <=> knob.value;
            signal moved;
            signal changed (value: number, final: boolean);
            gap: spacing;
            Slider { id: knob; }
            Label { text: label; }
        }
        Column {
            Slider { id: gain; value <=> model.gain; }
            Label { text: gain.pressed ? "pressed" : "idle"; }
        })yui");
}

TEST_F (YuiWriterTests, WritesSingleElements)
{
    const auto document = parse ("Row { Label { text: \"Hi\"; } }");
    ASSERT_EQ (document.getNumChildren(), 1);

    EXPECT_EQ (YuiWriter::write (document.getChild (0).getChild (0)), "Label {\n    text: \"Hi\";\n}\n");
    EXPECT_EQ (YuiWriter::write (DataTree ("yui")), "");
}

TEST_F (YuiWriterTests, RoundTripsStringsWithBothQuoteKindsAndEscapes)
{
    DataTree document ("yui");
    DataTree importNode ("import");
    importNode.beginTransaction().setProperty ("path", "dir's/\"theme\".yui");

    DataTree labelNode ("Label");
    {
        auto tx = labelNode.beginTransaction();
        tx.setProperty ("id", "quoteLabel");
        // Instance values are expression source, so a string is stored as its literal
        tx.setProperty ("text", R"("\"He said 'hi'\"\tand\\slash\nline2")");
    }

    {
        auto tx = document.beginTransaction();
        tx.addChild (importNode);
        tx.addChild (labelNode);
    }

    const auto written = YuiWriter::write (document);
    const auto reparsed = YuiParser::parse (written);
    ASSERT_TRUE (reparsed.wasOk()) << YuiDiagnostic::join (reparsed.diagnostics) << "\n" << written;
    EXPECT_TRUE (reparsed.document.isEquivalentTo (document)) << written;
    EXPECT_EQ (YuiWriter::write (reparsed.document), written);

    ApplicationTheme::Ptr oldTheme = ApplicationTheme::getGlobalTheme();
    ApplicationTheme::Ptr theme = new ApplicationTheme();
    ApplicationTheme::setGlobalTheme (theme);

    YuiDocument liveDoc;
    const auto loadResult = liveDoc.loadFromString (YuiWriter::write (labelNode));
    ASSERT_TRUE (loadResult.wasOk()) << loadResult.getErrorMessage();
    auto* builtLabel = liveDoc.findById<Label> ("quoteLabel");
    ASSERT_NE (builtLabel, nullptr);
    EXPECT_EQ (builtLabel->getText(), String ("\"He said 'hi'\"\tand\\slash\nline2"));

    liveDoc.clear();
    ApplicationTheme::setGlobalTheme (oldTheme.get());
}
