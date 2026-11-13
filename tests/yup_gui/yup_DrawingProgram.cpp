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

class DrawingProgramTest : public ::testing::Test
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

    static void configure (Slider& slider)
    {
        slider.setRange (0.0, 1.0, 0.0);
        slider.setValue (0.5, dontSendNotification);
        slider.setBounds (0.0f, 0.0f, 48.0f, 48.0f);
    }

    static DataTree parseDrawing (const String& xml)
    {
        auto element = XmlDocument::parse (xml);
        return element != nullptr ? DataTree::fromXml (*element) : DataTree();
    }

    static String createError (const String& xml)
    {
        Slider slider (Slider::RotaryVerticalDrag);
        configure (slider);

        auto program = DrawingProgram::create (slider, parseDrawing (xml));
        return program.failed() ? program.getErrorMessage() : String();
    }

    void drawHeadless (DrawingProgram& program, const Component& component)
    {
        auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        auto renderer = context->makeRenderer (48, 48);
        Graphics g (*context, *renderer);

        program.draw (g, *theme, component);
    }

    // A knob made of independent shapes: a filled well, a translucent layer holding the
    // track, the value arc and repeated ticks, a rotated pointer and the value as text.
    static constexpr const char* knobDrawing = R"yui(
        drawing Knob {
            let r: bounds.width / 2 - 4;
            let a0: 135deg;
            let a1: 405deg;
            let av: lerp (a0, a1, normalized);

            circle { id: well; center: bounds.center; radius: r; fill: backgroundColor; }

            layer {
                opacity: 0.8;
                arc { id: track; center: bounds.center; radius: r - 2; from: a0; to: a1; stroke: trackColor; strokeWidth: 3; cap: round; }
                arc { id: value; center: bounds.center; radius: r - 2; from: a0; to: av; stroke: thumbColor; strokeWidth: 3; cap: round; }

                repeat i in 0 .. 11 {
                    line { from: polar (bounds.center, r * 0.6, lerp (a0, a1, i / 10.0));
                           to: polar (bounds.center, r * 0.7, lerp (a0, a1, i / 10.0));
                           stroke: trackColor; strokeWidth: 1; }
                }
            }

            group {
                transform: transform.rotation (av + 90deg, bounds.center.x, bounds.center.y);
                rect { bounds: rect (bounds.center.x - 1, 4, 2, r * 0.5); fill: thumbColor; cornerRadius: 1; }
            }

            text { bounds: bounds.reduced (r * 0.5); text: format ("%.2f", value); fill: thumbColor; justification: center; }
        })yui";

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

TEST_F (DrawingProgramTest, CompilesAndDrawsKnob)
{
    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    const auto parsed = YuiParser::parse (knobDrawing);
    ASSERT_TRUE (parsed.wasOk()) << parsed.diagnostics.front().toString();

    auto program = DrawingProgram::create (slider, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (program.wasOk()) << program.getErrorMessage();

    EXPECT_NO_THROW (drawHeadless (*program.getReference(), slider));

    slider.setValue (1.0, dontSendNotification);
    EXPECT_NO_THROW (drawHeadless (*program.getReference(), slider));
}

TEST_F (DrawingProgramTest, TextSizesTheDefaultFont)
{
    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    auto program = DrawingProgram::create (slider, parseDrawing ("<drawing><text bounds=\"bounds\" text=\"format (&quot;%.2f&quot;, value)\" font=\"defaultFont.withHeight (bounds.height * 0.3)\"/></drawing>"));
    ASSERT_TRUE (program.wasOk()) << program.getErrorMessage();

    EXPECT_NO_THROW (drawHeadless (*program.getReference(), slider));
}

TEST_F (DrawingProgramTest, FailsForNonDrawingRoot)
{
    EXPECT_TRUE (createError ("<layer/>").contains ("must be a 'drawing'"));
}

TEST_F (DrawingProgramTest, ReportsUnknownElementWithPath)
{
    EXPECT_EQ (createError ("<drawing><layer><blob/></layer></drawing>"), "drawing/layer[0]/blob[0]: unknown element 'blob'");
}

TEST_F (DrawingProgramTest, UsesRootNameAndIdsInPaths)
{
    EXPECT_TRUE (createError ("<drawing name=\"Knob\"><blob/></drawing>").startsWith ("Knob/blob[0]: "));
    EXPECT_TRUE (createError ("<drawing><arc id=\"track\" center=\"bounds.center\" radius=\"4\" from=\"0\" to=\"nope\"/></drawing>").startsWith ("drawing/arc#track.to: "));
}

TEST_F (DrawingProgramTest, ReportsUnknownProperty)
{
    EXPECT_EQ (createError ("<drawing><circle center=\"bounds.center\" radius=\"4\" fil=\"#ff0000\"/></drawing>"), "drawing/circle[0]: unknown property 'fil'");
    EXPECT_EQ (createError ("<drawing><layer transform=\"transform()\"/></drawing>"), "drawing/layer[0]: unknown property 'transform'");
}

TEST_F (DrawingProgramTest, ReportsMissingRequiredProperty)
{
    EXPECT_EQ (createError ("<drawing><circle center=\"bounds.center\"/></drawing>"), "drawing/circle[0]: missing the required property 'radius'");
}

TEST_F (DrawingProgramTest, ReportsExpressionErrorsAtTheProperty)
{
    EXPECT_TRUE (createError ("<drawing><rect bounds=\"bounds\" fill=\"bounds\"/></drawing>").startsWith ("drawing/rect[0].fill: "));
    EXPECT_TRUE (createError ("<drawing><let name=\"r\" value=\"nope\"/></drawing>").startsWith ("drawing/let[0].value: "));
}

TEST_F (DrawingProgramTest, RejectsTrailingTextInValues)
{
    EXPECT_EQ (createError ("<drawing><circle center=\"bounds.center\" radius=\"4; rect = bounds\"/></drawing>"), "drawing/circle[0].radius: unexpected text after the expression");
}

TEST_F (DrawingProgramTest, RejectsReservedLetNames)
{
    EXPECT_TRUE (createError ("<drawing><let name=\"_r\" value=\"4\"/></drawing>").startsWith ("drawing/let[0].name: "));
}

TEST_F (DrawingProgramTest, LetsAreVisibleOnlyAfterTheirDeclarationInTheirScope)
{
    EXPECT_TRUE (createError ("<drawing><circle center=\"bounds.center\" radius=\"r\"/><let name=\"r\" value=\"4\"/></drawing>").startsWith ("drawing/circle[0].radius: "));
    EXPECT_TRUE (createError ("<drawing><group><let name=\"r\" value=\"4\"/></group><circle center=\"bounds.center\" radius=\"r\"/></drawing>").startsWith ("drawing/circle[0].radius: "));
    EXPECT_TRUE (createError ("<drawing><repeat name=\"i\" from=\"0\" to=\"3\"/><circle center=\"bounds.center\" radius=\"i\"/></drawing>").startsWith ("drawing/circle[0].radius: "));
    EXPECT_EQ (createError ("<drawing><let name=\"r\" value=\"4\"/><group><layer><circle center=\"bounds.center\" radius=\"r\"/></layer></group></drawing>"), String());
}

TEST_F (DrawingProgramTest, LetsCanShadowOutputsAndOuterLets)
{
    EXPECT_EQ (createError ("<drawing><let name=\"radius\" value=\"4\"/><circle center=\"bounds.center\" radius=\"radius\" fill=\"trackColor\"/></drawing>"), String());
    EXPECT_EQ (createError ("<drawing><let name=\"r\" value=\"1\"/><group><let name=\"r\" value=\"r + 1\"/><circle center=\"bounds.center\" radius=\"r\"/></group></drawing>"), String());
    EXPECT_EQ (createError ("<drawing><let name=\"r\" value=\"1\"/><let name=\"r\" value=\"2\"/></drawing>"), "drawing/let[1].name: 'r' is already defined");
}

TEST_F (DrawingProgramTest, RejectsContainersInsideRepeat)
{
    EXPECT_EQ (createError ("<drawing><repeat name=\"i\" from=\"0\" to=\"3\"><layer/></repeat></drawing>"), "drawing/repeat[0]/layer[0]: a 'layer' can't be nested inside a repeat");
}
