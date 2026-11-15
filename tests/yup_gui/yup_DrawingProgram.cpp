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

TEST_F (DrawingProgramTest, ComponentsResolvingNamesDifferentlyDontShareACompilation)
{
    Label label ("label");
    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    const auto drawing = parseDrawing ("<drawing><text bounds=\"bounds\" text=\"text\"/></drawing>");

    auto forLabel = DrawingProgram::create (label, drawing);
    ASSERT_TRUE (forLabel.wasOk()) << forLabel.getErrorMessage();

    EXPECT_TRUE (DrawingProgram::create (slider, drawing).failed());
}

TEST_F (DrawingProgramTest, EditedDrawingTreesAreCompiledAgain)
{
    Slider first (Slider::RotaryVerticalDrag);
    Slider second (Slider::RotaryVerticalDrag);
    configure (first);
    configure (second);

    auto drawing = parseDrawing ("<drawing><rect bounds=\"bounds\" fill=\"trackColor\"/></drawing>");

    auto program = DrawingProgram::create (first, drawing);
    ASSERT_TRUE (program.wasOk()) << program.getErrorMessage();

    drawing.getChild (0).beginTransaction().setProperty ("fill", "missingColor");

    const auto edited = DrawingProgram::create (second, drawing);
    ASSERT_TRUE (edited.failed());
    EXPECT_TRUE (edited.getErrorMessage().contains ("missingColor"));
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

TEST_F (DrawingProgramTest, ReportsLetErrorsBeforeTheErrorsOfLaterElements)
{
    EXPECT_TRUE (createError ("<drawing><let name=\"r\" value=\"nope\"/><circle center=\"bounds.center\" radius=\"r\"/></drawing>").startsWith ("drawing/let[0].value: "));
    EXPECT_TRUE (createError ("<drawing><let name=\"r\" value=\"nope\"/><circle center=\"bounds.center\" radius=\"4\" bogus=\"1\"/></drawing>").startsWith ("drawing/let[0].value: "));
    EXPECT_TRUE (createError ("<drawing><let name=\"r\" value=\"nope\"/><group><circle center=\"bounds.center\" radius=\"4\" fill=\"bounds\"/></group></drawing>").startsWith ("drawing/let[0].value: "));
    EXPECT_TRUE (createError ("<drawing><let name=\"a\" value=\"2\"/><let name=\"b\" value=\"a * 2\"/><circle center=\"bounds.center\" radius=\"b\" fill=\"bounds\"/></drawing>").startsWith ("drawing/circle[0].fill: "));
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

TEST_F (DrawingProgramTest, DeclaresItsProperties)
{
    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    const auto parsed = YuiParser::parse (R"yui(
        drawing {
            let r: bounds.width * bezel;
            property bezel: number = 0.8;
            property ring: color = palette.accent;
            property ticks: integer;
            property origin: point = bounds.center;
            property lanes: number[];
            circle { center: origin; radius: r; stroke: ring; }
            repeat i in 0 .. ticks { line { from: bounds.center; to: polar (bounds.center, r, i); } }
            repeat i in 0 .. len (lanes) { line { from: origin; to: polar (origin, r * lanes[i], i); } }
        })yui");

    ASSERT_TRUE (parsed.wasOk()) << parsed.diagnostics.front().toString();

    auto program = DrawingProgram::create (slider, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (program.wasOk()) << program.getErrorMessage();

    const auto& properties = program.getReference()->getProperties();
    ASSERT_EQ (properties.size(), 5u);
    EXPECT_EQ (properties[0].name, Identifier ("bezel"));
    EXPECT_EQ (properties[0].type, ComponentProperty::Type::number);
    EXPECT_EQ (properties[1].type, ComponentProperty::Type::color);
    EXPECT_EQ (properties[2].type, ComponentProperty::Type::integer);
    EXPECT_EQ (properties[3].type, ComponentProperty::Type::point);
    EXPECT_EQ (properties[4].type, ComponentProperty::Type::array);
    EXPECT_EQ (properties[4].elementType, ComponentProperty::Type::number);

    EXPECT_NO_THROW (drawHeadless (*program.getReference(), slider));

    slider.setProperty (Identifier ("ticks"), 4);
    slider.setProperty (Identifier ("ring"), VariantConverter<Color>::toVar (Colors::red));
    slider.setProperty (Identifier ("origin"), VariantConverter<Point<float>>::toVar ({ 10.0f, 20.0f }));
    slider.setProperty (Identifier ("lanes"), var (Array<var> { 0.25, 0.5, 0.75 }));
    EXPECT_NO_THROW (drawHeadless (*program.getReference(), slider));
}

TEST_F (DrawingProgramTest, ReportsPropertyErrors)
{
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"font\"/></drawing>"), "drawing/property[0].type: 'font' can't be used in a drawing, use number, integer, boolean, string, color, point, rectangle or an array of those");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"number[]\" value=\"1\"/></drawing>"), "drawing/property[0].value: an array property can't have a default");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"color\" value=\"true\"/></drawing>"), "drawing/property[0].value: the type 'bool' doesn't match the property type 'color'");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"integer\" value=\"0.5f\"/></drawing>"), "drawing/property[0].value: the type 'float' doesn't match the property type 'integer'");
    EXPECT_TRUE (createError ("<drawing><property name=\"a\" type=\"number\" value=\"nope\"/></drawing>").startsWith ("drawing/property[0].value: "));
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"number\" bogus=\"1\"/></drawing>"), "drawing/property[0]: unknown property 'bogus'");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"number\"/><let name=\"a\" value=\"1\"/></drawing>"), "drawing/let[0].name: 'a' is already defined");
    EXPECT_EQ (createError ("<drawing><layer><property name=\"a\" type=\"number\"/></layer></drawing>"), "drawing/layer[0]/property[0]: a 'property' can only be declared at the top of a drawing");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"number\" value=\"1\"/><circle center=\"bounds.center\" radius=\"a\"/></drawing>"), String());
}

TEST_F (DrawingProgramTest, RejectsConstantRepeatExceedingMaximumCount)
{
    const auto error = createError ("<drawing><repeat name=\"i\" from=\"0\" to=\"5000\"><circle center=\"bounds.center\" radius=\"1\"/></repeat></drawing>");
    EXPECT_FALSE (error.isEmpty());
    EXPECT_TRUE (error.contains ("4096")) << error;
}

//==============================================================================
class DrawingProgramGpuTest : public DrawingProgramTest
{
protected:
    // Paints itself with a drawing program, so the pixels show what the drawing evaluated
    struct DrawnComponent : Component
    {
        explicit DrawnComponent (const DataTree& drawing)
        {
            setOpaque (false);
            setBounds (0.0f, 0.0f, 16.0f, 16.0f);

            auto created = DrawingProgram::create (*this, drawing);
            EXPECT_TRUE (created.wasOk()) << created.getErrorMessage();

            if (created.wasOk())
                program = std::move (created.getReference());
        }

        void paint (Graphics& g) override
        {
            if (program != nullptr)
                program->draw (g, *ApplicationTheme::getGlobalTheme(), *this);
        }

        std::unique_ptr<DrawingProgram> program;
    };

    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});
        if (gpuContext == nullptr)
            return;

        if (GpuCanvas::create (*gpuContext, 64, 64) == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        DrawingProgramTest::SetUp();

        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";
    }

    static DataTree parseYui (const String& text)
    {
        const auto parsed = YuiParser::parse (text);
        EXPECT_TRUE (parsed.wasOk()) << parsed.diagnostics.front().toString();
        return parsed.document.getChildWithName ("drawing");
    }

    static Color centerColor (Component& component)
    {
        const auto image = component.snapshotToImage (*gpuContext);
        return image.isValid() ? image.getPixelColor (8, 8) : Color();
    }

    static std::unique_ptr<GraphicsContext> gpuContext;
};

std::unique_ptr<GraphicsContext> DrawingProgramGpuTest::gpuContext;

TEST_F (DrawingProgramGpuTest, PropertiesReadTheComponentAndItsParentsOrTheirDefault)
{
    const auto drawing = parseYui (R"yui(
        drawing {
            property shade: color = #ff0000;
            property alpha: number = 1;
            property shown: boolean = true;
            rect { bounds: bounds; fill: shade.withAlpha (alpha); visible: shown; }
        })yui");

    Component parent;
    DrawnComponent first (drawing);
    DrawnComponent second (drawing);
    parent.addChildComponent (first);

    EXPECT_EQ (centerColor (first), Color (0xffff0000));

    parent.setProperty (Identifier ("shade"), VariantConverter<Color>::toVar (Color (0xff00ff00)));
    EXPECT_EQ (centerColor (first), Color (0xff00ff00));

    first.setProperty (Identifier ("shade"), VariantConverter<Color>::toVar (Color (0xff0000ff)));
    EXPECT_EQ (centerColor (first), Color (0xff0000ff));
    EXPECT_EQ (centerColor (second), Color (0xffff0000));

    first.setProperty (Identifier ("alpha"), 0.0);
    EXPECT_EQ (centerColor (first).getAlpha(), 0);

    second.setProperty (Identifier ("shown"), false);
    EXPECT_EQ (centerColor (second).getAlpha(), 0);
}
