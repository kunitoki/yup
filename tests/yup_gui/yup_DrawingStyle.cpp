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

class DrawingStyleTest : public ::testing::Test
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

        auto style = DrawingStyle::create (slider, parseDrawing (xml));
        return style.failed() ? style.getErrorMessage() : String();
    }

    void paintHeadless (DrawingStyle& style, const Component& component)
    {
        auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        auto renderer = context->makeRenderer (48, 48);
        Graphics g (*context, *renderer);

        style.paint (g, *theme, component);
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

TEST_F (DrawingStyleTest, CreatesStyleForValidDrawing)
{
    Slider slider (Slider::LinearHorizontal);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseDrawing ("<drawing><rect bounds=\"bounds\" fill=\"trackColor\"/></drawing>"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();
    EXPECT_NE (style.getReference().get(), nullptr);
}

TEST_F (DrawingStyleTest, FailsForInvalidDrawing)
{
    Slider slider (Slider::LinearHorizontal);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseDrawing ("<drawing><blob/></drawing>"));
    EXPECT_TRUE (style.failed());
}

TEST_F (DrawingStyleTest, SetStyleIsReturnedByComponentStyleLookup)
{
    Slider slider (Slider::LinearHorizontal);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseDrawing ("<drawing><rect bounds=\"bounds\" fill=\"trackColor\"/></drawing>"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    slider.setStyle (style.getReference());

    EXPECT_EQ (ApplicationTheme::findComponentStyle (slider).get(), style.getReference().get());
}

TEST_F (DrawingStyleTest, PaintsThroughComponentStyleWithoutThrowing)
{
    Slider slider (Slider::LinearHorizontal);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseDrawing ("<drawing><circle center=\"bounds.center\" radius=\"normalized * 20\" fill=\"trackColor\"/></drawing>"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    slider.setStyle (style.getReference());

    auto resolved = ApplicationTheme::findComponentStyle (slider);
    ASSERT_NE (resolved.get(), nullptr);

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (48, 48);
    Graphics g (*context, *renderer);

    EXPECT_NO_THROW (resolved->paint (g, *theme, slider));
}

TEST_F (DrawingStyleTest, CompilesAndDrawsKnob)
{
    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    const auto parsed = YuiParser::parse (knobDrawing);
    ASSERT_TRUE (parsed.wasOk()) << parsed.diagnostics.front().toString();

    auto style = DrawingStyle::create (slider, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    EXPECT_NO_THROW (paintHeadless (*style.getReference(), slider));

    slider.setValue (1.0, dontSendNotification);
    EXPECT_NO_THROW (paintHeadless (*style.getReference(), slider));
}

TEST_F (DrawingStyleTest, TextSizesTheDefaultFont)
{
    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseDrawing ("<drawing><text bounds=\"bounds\" text=\"format (&quot;%.2f&quot;, value)\" font=\"defaultFont.withHeight (bounds.height * 0.3)\"/></drawing>"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    EXPECT_NO_THROW (paintHeadless (*style.getReference(), slider));
}

TEST_F (DrawingStyleTest, ComponentsResolvingNamesDifferentlyDontShareACompilation)
{
    Label label ("label");
    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    const auto drawing = parseDrawing ("<drawing><text bounds=\"bounds\" text=\"text\"/></drawing>");

    auto forLabel = DrawingStyle::create (label, drawing);
    ASSERT_TRUE (forLabel.wasOk()) << forLabel.getErrorMessage();

    EXPECT_TRUE (DrawingStyle::create (slider, drawing).failed());
}

TEST_F (DrawingStyleTest, EditedDrawingTreesAreCompiledAgain)
{
    Slider first (Slider::RotaryVerticalDrag);
    Slider second (Slider::RotaryVerticalDrag);
    configure (first);
    configure (second);

    auto drawing = parseDrawing ("<drawing><rect bounds=\"bounds\" fill=\"trackColor\"/></drawing>");

    auto style = DrawingStyle::create (first, drawing);
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    drawing.getChild (0).beginTransaction().setProperty ("fill", "missingColor");

    const auto edited = DrawingStyle::create (second, drawing);
    ASSERT_TRUE (edited.failed());
    EXPECT_TRUE (edited.getErrorMessage().contains ("missingColor"));
}

TEST_F (DrawingStyleTest, FailsForNonDrawingRoot)
{
    EXPECT_TRUE (createError ("<layer/>").contains ("must be a 'drawing'"));
}

TEST_F (DrawingStyleTest, ReportsUnknownElementWithPath)
{
    EXPECT_EQ (createError ("<drawing><layer><blob/></layer></drawing>"), "drawing/layer[0]/blob[0]: unknown element 'blob'");
}

TEST_F (DrawingStyleTest, UsesRootNameAndIdsInPaths)
{
    EXPECT_TRUE (createError ("<drawing name=\"Knob\"><blob/></drawing>").startsWith ("Knob/blob[0]: "));
    EXPECT_TRUE (createError ("<drawing><arc id=\"track\" center=\"bounds.center\" radius=\"4\" from=\"0\" to=\"nope\"/></drawing>").startsWith ("drawing/arc#track.to: "));
}

TEST_F (DrawingStyleTest, ReportsUnknownProperty)
{
    EXPECT_EQ (createError ("<drawing><circle center=\"bounds.center\" radius=\"4\" fil=\"#ff0000\"/></drawing>"), "drawing/circle[0]: unknown property 'fil'");
    EXPECT_EQ (createError ("<drawing><layer transform=\"transform()\"/></drawing>"), "drawing/layer[0]: unknown property 'transform'");
}

TEST_F (DrawingStyleTest, ReportsMissingRequiredProperty)
{
    EXPECT_EQ (createError ("<drawing><circle center=\"bounds.center\"/></drawing>"), "drawing/circle[0]: missing the required property 'radius'");
}

TEST_F (DrawingStyleTest, ReportsExpressionErrorsAtTheProperty)
{
    EXPECT_TRUE (createError ("<drawing><rect bounds=\"bounds\" fill=\"bounds\"/></drawing>").startsWith ("drawing/rect[0].fill: "));
    EXPECT_TRUE (createError ("<drawing><let name=\"r\" value=\"nope\"/></drawing>").startsWith ("drawing/let[0].value: "));
}

TEST_F (DrawingStyleTest, ReportsLetErrorsBeforeTheErrorsOfLaterElements)
{
    EXPECT_TRUE (createError ("<drawing><let name=\"r\" value=\"nope\"/><circle center=\"bounds.center\" radius=\"r\"/></drawing>").startsWith ("drawing/let[0].value: "));
    EXPECT_TRUE (createError ("<drawing><let name=\"r\" value=\"nope\"/><circle center=\"bounds.center\" radius=\"4\" bogus=\"1\"/></drawing>").startsWith ("drawing/let[0].value: "));
    EXPECT_TRUE (createError ("<drawing><let name=\"r\" value=\"nope\"/><group><circle center=\"bounds.center\" radius=\"4\" fill=\"bounds\"/></group></drawing>").startsWith ("drawing/let[0].value: "));
    EXPECT_TRUE (createError ("<drawing><let name=\"a\" value=\"2\"/><let name=\"b\" value=\"a * 2\"/><circle center=\"bounds.center\" radius=\"b\" fill=\"bounds\"/></drawing>").startsWith ("drawing/circle[0].fill: "));
}

TEST_F (DrawingStyleTest, RejectsTrailingTextInValues)
{
    EXPECT_EQ (createError ("<drawing><circle center=\"bounds.center\" radius=\"4; rect = bounds\"/></drawing>"), "drawing/circle[0].radius: unexpected text after the expression");
}

TEST_F (DrawingStyleTest, RejectsReservedLetNames)
{
    EXPECT_TRUE (createError ("<drawing><let name=\"_r\" value=\"4\"/></drawing>").startsWith ("drawing/let[0].name: "));
}

TEST_F (DrawingStyleTest, LetsAreVisibleOnlyAfterTheirDeclarationInTheirScope)
{
    EXPECT_TRUE (createError ("<drawing><circle center=\"bounds.center\" radius=\"r\"/><let name=\"r\" value=\"4\"/></drawing>").startsWith ("drawing/circle[0].radius: "));
    EXPECT_TRUE (createError ("<drawing><group><let name=\"r\" value=\"4\"/></group><circle center=\"bounds.center\" radius=\"r\"/></drawing>").startsWith ("drawing/circle[0].radius: "));
    EXPECT_TRUE (createError ("<drawing><repeat name=\"i\" from=\"0\" to=\"3\"/><circle center=\"bounds.center\" radius=\"i\"/></drawing>").startsWith ("drawing/circle[0].radius: "));
    EXPECT_EQ (createError ("<drawing><let name=\"r\" value=\"4\"/><group><layer><circle center=\"bounds.center\" radius=\"r\"/></layer></group></drawing>"), String());
}

TEST_F (DrawingStyleTest, LetsCanShadowOutputsAndOuterLets)
{
    EXPECT_EQ (createError ("<drawing><let name=\"radius\" value=\"4\"/><circle center=\"bounds.center\" radius=\"radius\" fill=\"trackColor\"/></drawing>"), String());
    EXPECT_EQ (createError ("<drawing><let name=\"r\" value=\"1\"/><group><let name=\"r\" value=\"r + 1\"/><circle center=\"bounds.center\" radius=\"r\"/></group></drawing>"), String());
    EXPECT_EQ (createError ("<drawing><let name=\"r\" value=\"1\"/><let name=\"r\" value=\"2\"/></drawing>"), "drawing/let[1].name: 'r' is already defined");
}

TEST_F (DrawingStyleTest, RejectsLayersInsideRepeat)
{
    EXPECT_EQ (createError ("<drawing><repeat name=\"i\" from=\"0\" to=\"3\"><layer/></repeat></drawing>"), "drawing/repeat[0]/layer[0]: a 'layer' can't be nested inside a repeat");
}

TEST_F (DrawingStyleTest, DeclaresItsProperties)
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

    auto style = DrawingStyle::create (slider, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    const auto& properties = style.getReference()->getProperties();
    ASSERT_EQ (properties.size(), 5u);
    EXPECT_EQ (properties[0].name, Identifier ("bezel"));
    EXPECT_EQ (properties[0].type, ComponentProperty::Type::number);
    EXPECT_EQ (properties[1].type, ComponentProperty::Type::color);
    EXPECT_EQ (properties[2].type, ComponentProperty::Type::integer);
    EXPECT_EQ (properties[3].type, ComponentProperty::Type::point);
    EXPECT_EQ (properties[4].type, ComponentProperty::Type::array);
    EXPECT_EQ (properties[4].elementType, ComponentProperty::Type::number);

    EXPECT_NO_THROW (paintHeadless (*style.getReference(), slider));

    slider.setProperty (Identifier ("ticks"), 4);
    slider.setProperty (Identifier ("ring"), VariantConverter<Color>::toVar (Colors::red));
    slider.setProperty (Identifier ("origin"), VariantConverter<Point<float>>::toVar ({ 10.0f, 20.0f }));
    slider.setProperty (Identifier ("lanes"), var (Array<var> { 0.25, 0.5, 0.75 }));
    EXPECT_NO_THROW (paintHeadless (*style.getReference(), slider));
}

TEST_F (DrawingStyleTest, ReportsPropertyErrors)
{
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"font\"/></drawing>"), String());
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"transform\"/></drawing>"), String());
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"font[]\"/></drawing>"), String());
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"any\"/></drawing>"), "drawing/property[0].type: 'any' can't be used in a drawing, use number, integer, boolean, string, color, point, rectangle, transform, font or an array of scalar types");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"number[]\" value=\"1\"/></drawing>"), "drawing/property[0].value: an array property can't have a default");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"color\" value=\"true\"/></drawing>"), "drawing/property[0].value: the type 'bool' doesn't match the property type 'color'");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"integer\" value=\"0.5f\"/></drawing>"), "drawing/property[0].value: the type 'float' doesn't match the property type 'integer'");
    EXPECT_TRUE (createError ("<drawing><property name=\"a\" type=\"number\" value=\"nope\"/></drawing>").startsWith ("drawing/property[0].value: "));
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"number\" bogus=\"1\"/></drawing>"), "drawing/property[0]: unknown property 'bogus'");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"number\"/><let name=\"a\" value=\"1\"/></drawing>"), "drawing/let[0].name: 'a' is already defined");
    EXPECT_EQ (createError ("<drawing><layer><property name=\"a\" type=\"number\"/></layer></drawing>"), "drawing/layer[0]/property[0]: a 'property' can only be declared at the top of a drawing");
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"number\" value=\"1\"/><circle center=\"bounds.center\" radius=\"a\"/></drawing>"), String());
}

TEST_F (DrawingStyleTest, CompilesAndPaintsDashedStrokesClipLayerMasksImagesAndSvgs)
{
    // 1x1 red RGBA PNG encoded as a data URI
    static constexpr const char* redPixelPng =
        "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";

    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    const auto parsed = YuiParser::parse (String (R"yui(
        drawing Complete {
            property customDashes: number[];
            line { from: point (0, 4); to: point (48, 4); stroke: #ffffff; strokeWidth: 2; dash: dash (6, 3); dashOffset: 2; }
            line { from: point (0, 10); to: point (48, 10); stroke: #ffffff; strokeWidth: 2; dash: "4, 2"; }
            line { from: point (0, 16); to: point (48, 16); stroke: #ffffff; strokeWidth: 2; dash: 3; }
            line { from: point (0, 22); to: point (48, 22); stroke: #ffffff; strokeWidth: 2; dash: customDashes; }
            path { data: "M 0 28 L 48 28"; stroke: #ffffff; strokeWidth: 1.5; dash: "5, 3"; }
            clip {
                bounds: bounds.reduced (4);
                rect { bounds: bounds; fill: #224466; }
            }
            layer {
                opacity: 0.9;
                blend: screen;
                mask: rect (4, 4, 40, 40);
                maskMode: alpha;
                rect { bounds: bounds; fill: #ff4422; }
                mask {
                    mode: luminance;
                    circle { center: bounds.center; radius: 16; fill: #ffffff; }
                }
            }
            image {
                bounds: rect (2, 2, 12, 12);
                source: ")yui") + redPixelPng + R"yui(";
                fitting: scaleToFit;
                justification: center;
                opacity: 0.85;
            }
            svg {
                bounds: rect (16, 16, 16, 16);
                data: "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'><circle cx='8' cy='8' r='6' fill='#00ff88'/></svg>";
                fitting: scaleToFit;
            }
        }
    )yui");
    ASSERT_TRUE (parsed.wasOk()) << YuiDiagnostic::join (parsed.diagnostics);

    auto style = DrawingStyle::create (slider, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    slider.setProperty (Identifier ("customDashes"), var (Array<var> { 5.0, 2.0 }));
    EXPECT_NO_THROW (paintHeadless (*style.getReference(), slider));

    EXPECT_EQ (createError ("<drawing><mask/></drawing>"), "drawing/mask[0]: a 'mask' can only appear inside a 'layer'");
    EXPECT_EQ (createError ("<drawing><image/></drawing>"), "drawing/image[0]: missing the required property 'source'");
    EXPECT_EQ (createError ("<drawing><svg/></drawing>"), "drawing/svg[0]: missing the required property 'source'");
    EXPECT_EQ (createError ("<drawing><svg source=\"a.svg\" data=\"&lt;svg/&gt;\"/></drawing>"), "drawing/svg[0]: specify either 'source' or 'data', not both");
}

TEST_F (DrawingStyleTest, ReportsStructuredChildElementAndPropertyLocations)
{
    const auto parsed = YuiParser::parse (
        "drawing Broken {\n"
        "    rect {\n"
        "        bounds: bounds;\n"
        "        fill: unknownSymbol + 1;\n"
        "    }\n"
        "}\n");
    ASSERT_TRUE (parsed.wasOk()) << YuiDiagnostic::join (parsed.diagnostics);

    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    YuiDiagnostic failure;
    auto created = DrawingStyle::create (slider, parsed.document.getChildWithName ("drawing"), nullptr, &failure);
    EXPECT_TRUE (created.failed());
    EXPECT_TRUE (failure.element.isValid());
    EXPECT_EQ (failure.element.getType(), Identifier ("rect"));
    EXPECT_EQ (failure.property, Identifier ("fill"));

    const auto located = parsed.sourceMap.locate (failure);
    EXPECT_EQ (located.line, 4);
    EXPECT_GT (located.column, 1);
}

TEST_F (DrawingStyleTest, RejectsConstantRepeatExceedingMaximumCount)
{
    const auto error = createError ("<drawing><repeat name=\"i\" from=\"0\" to=\"5000\"><circle center=\"bounds.center\" radius=\"1\"/></repeat></drawing>");
    EXPECT_FALSE (error.isEmpty());
    EXPECT_TRUE (error.contains ("4096")) << error;
}

//==============================================================================
class DrawingStyleLanguageTest : public DrawingStyleTest
{
protected:
    static DataTree parseYuiDrawing (const String& text, const String& name = {})
    {
        const auto parsed = YuiParser::parse (text);
        EXPECT_TRUE (parsed.wasOk()) << YuiDiagnostic::join (parsed.diagnostics);

        for (const auto& block : parsed.document)
            if (block.getType() == Identifier ("drawing") && (name.isEmpty() || block.getProperty ("name").toString() == name))
                return block;

        return {};
    }

    static String createYuiError (const String& text, const DrawingStyle::DrawingResolver& drawingResolver = nullptr)
    {
        Slider slider (Slider::RotaryVerticalDrag);
        configure (slider);

        auto style = DrawingStyle::create (slider, parseYuiDrawing (text), nullptr, nullptr, drawingResolver);
        return style.failed() ? style.getErrorMessage() : String();
    }
};

TEST_F (DrawingStyleLanguageTest, ARepeatHoldsContainersButNoLayerOrDefinition)
{
    // Groups, clips, viewports, uses and other repeats, reading the index of the repeats around them
    EXPECT_EQ (createYuiError (R"yui(
        drawing {
            drawing Leaf { rect { bounds: bounds; fill: thumbColor; } }

            repeat i in 0 .. 2 {
                group { transform: transform.translation (i, 0); rect { bounds: bounds; fill: thumbColor; } }
                clip { bounds: rect (i, 0, 4, 4); rect { bounds: bounds; fill: thumbColor; } }
                viewport { bounds: rect (i, 0, 4, 4); }
                use Leaf { bounds: rect (i, 0, 4, 4); }
                repeat j in 0 .. i + 1 { group { rect { bounds: rect (i, j, 1, 1); fill: thumbColor; } } }
            }
        })yui"),
               String());

    // Layers and definitions stay out of a repeat, however deep
    EXPECT_EQ (createYuiError ("drawing { repeat i in 0 .. 2 { group { layer { } } } }"), "drawing/repeat[0]/group[0]/layer[0]: a 'layer' can't be nested inside a repeat");
    EXPECT_EQ (createYuiError ("drawing { repeat i in 0 .. 2 { pattern Dots { tile: rect (0, 0, 4, 4); } } }"), "drawing/repeat[0]/pattern[0]: a 'pattern' can't be nested inside a repeat");
    EXPECT_EQ (createYuiError ("drawing { repeat i in 0 .. 2 { drawing Leaf { } } }"), "drawing/repeat[0]/drawing[0]: a 'drawing' can't be nested inside a repeat");
    EXPECT_EQ (createYuiError ("drawing { drawing Glow { layer { } } repeat i in 0 .. 2 { use Glow { } } }"), "drawing/repeat[0]/use[0]/Glow/layer[0]: a 'layer' can't be nested inside a repeat");

    // The shapes of a clip path are still all a repeat holds there
    EXPECT_EQ (createYuiError ("drawing { clip { clipPath { repeat i in 0 .. 2 { group { } } } } }"), "drawing/clip[0]/clipPath[0]/repeat[0]/group[0]: a 'group' can't be nested inside a repeat");

    EXPECT_TRUE (createYuiError ("drawing { repeat i in 0 .. 5000 { group { } } }").contains ("4096"));
}

TEST_F (DrawingStyleLanguageTest, MapsAViewBoxToTheBoundsAndToViewports)
{
    EXPECT_EQ (createYuiError (R"yui(
        drawing {
            viewBox: rect (0, 0, 24, 24);
            fitting: scaleToFill;
            justification: topLeft;
            circle { center: bounds.center; radius: bounds.width / 2; fill: trackColor; }
            viewport { bounds: rect (0, 0, 8, 8); viewBox: rect (0, 0, 100, 100); fitting: fill;
                       rect { bounds: bounds.reduced (10); fill: thumbColor; } }
            viewport { rect { bounds: bounds; fill: thumbColor; } }
        })yui"),
               String());

    EXPECT_EQ (createYuiError ("drawing { fitting: fill; }"), "drawing.fitting: needs a 'viewBox' to fit in the bounds");
    EXPECT_TRUE (createYuiError ("drawing { viewBox: 4; }").startsWith ("drawing.viewBox: "));
}

TEST_F (DrawingStyleLanguageTest, GroupsSetThePaintOfTheShapesInsideThem)
{
    EXPECT_EQ (createYuiError (R"yui(
        drawing {
            let accent: trackColor;
            group {
                color: accent; fill: currentColor; fillOpacity: 0.5; stroke: thumbColor; strokeWidth: 2; dash: "4, 2"; cap: round;
                opacity: 0.8; blend: screen; font: defaultFont.withHeight (10); textAnchor: middle; letterSpacing: 1;

                rect { bounds: bounds; }
                circle { center: bounds.center; radius: 4; fill: none; }
                group { stroke: none; fill: none; ellipse { bounds: bounds; } line { from: bounds.topLeft; to: bounds.bottomRight; } }
                text { anchor: bounds.center; text: "a"; }
                text { bounds: bounds; text: "b"; stroke: none; }
                repeat i in 0 .. 3 { rect { bounds: rect (i * 4, 0, 2, 2); fillGradient: linearGradient (#f00, point (0, 0), #00f, point (1, 0)).mappedTo (objectBounds); } }
            }
        })yui"),
               String());

    EXPECT_TRUE (createYuiError ("drawing { group { fill: bounds; rect { bounds: bounds; } } }").startsWith ("drawing/group[0].fill: "));
    EXPECT_TRUE (createYuiError ("drawing { group { color: bounds; } }").startsWith ("drawing/group[0].color: "));
    EXPECT_TRUE (createYuiError ("drawing { rect { bounds: bounds; fill: currentColor; } }").startsWith ("drawing/rect[0].fill: "));
    EXPECT_EQ (createYuiError ("drawing { layer { fill: #f00; } }"), "drawing/layer[0]: unknown property 'fill'");
}

TEST_F (DrawingStyleLanguageTest, ObjectBoundsIsOnlyReadByThePaintOfShapes)
{
    EXPECT_EQ (createYuiError (R"yui(
        drawing {
            let sky: linearGradient (#48f, point (0, 0), #fff, point (0, 1)).withSpread (reflect);
            group {
                fillGradient: sky.mappedTo (objectBounds);
                rect { bounds: bounds; }
                circle { center: bounds.center; radius: 4; }
                arc { center: bounds.center; radius: 4; from: 0; to: pi; stroke: #fff; }
                line { from: bounds.topLeft; to: bounds.bottomRight; }
                path { data: "M 0 0 L 8 8"; fill: objectBounds.width > 4 ? #f00 : #00f; }
                star { center: bounds.center; points: 5; innerRadius: 2; outerRadius: 4; }
            }
        })yui"),
               String());

    const String message ("'objectBounds' can only be read by the paint properties of a shape, or of the groups around it");

    EXPECT_EQ (createYuiError ("drawing { let size: objectBounds; }"), "drawing/let[0].value: " + message);
    EXPECT_EQ (createYuiError ("drawing { rect { bounds: objectBounds; } }"), "drawing/rect[0].bounds: " + message);
    EXPECT_EQ (createYuiError ("drawing { layer { opacity: objectBounds.width; } }"), "drawing/layer[0].opacity: " + message);
    EXPECT_EQ (createYuiError ("drawing { text { anchor: bounds.center; text: \"a\"; fill: objectBounds.width > 4 ? #f00 : #00f; } }"),
               "drawing/text[0]: 'objectBounds' is not available to a text placed from an 'anchor'");
}

TEST_F (DrawingStyleLanguageTest, UsePaintsDrawingsDefinedInsideOrFoundByName)
{
    const String text (R"yui(
        drawing Icon {
            let accent: trackColor;
            drawing Leaf {
                property tone: color = accent;
                property weight: number = 2;
                viewBox: rect (0, 0, 10, 10);
                path { data: "M 5 0 L 10 10 L 0 10 Z"; fill: tone; strokeWidth: weight; }
            }
            use Leaf { bounds: rect (0, 0, 8, 8); }
            group { fill: #000; use Leaf { bounds: bounds.reduced (4); tone: thumbColor; weight: 3; transform: transform.rotation (pi / 2); opacity: 0.5; color: accent; } }
            use Badge { bounds: rect (0, 0, 4, 4); }
        }
        drawing Badge { rect { bounds: bounds; fill: trackColor; } }
        drawing Loop { use Loop {} }
        drawing Lanes { property lanes: number[]; rect { bounds: bounds; } })yui");

    const auto resolver = [&text] (const String& name)
    {
        return parseYuiDrawing (text, name);
    };

    EXPECT_EQ (createYuiError (text, resolver), String());
    EXPECT_EQ (createYuiError (text), "Icon/use[1]: unknown drawing 'Badge'");

    EXPECT_EQ (createYuiError ("drawing { use Leaf {} drawing Leaf {} }"), "drawing/use[0]: unknown drawing 'Leaf'");
    EXPECT_EQ (createYuiError ("drawing { drawing Leaf {} use Leaf { nope: 1; } }"), "drawing/use[0]: unknown property 'nope'");
    EXPECT_EQ (createYuiError ("drawing { drawing Leaf {} drawing Leaf {} }"), "drawing/drawing[1].name: the drawing 'Leaf' is already defined");
    EXPECT_TRUE (createYuiError ("drawing { drawing Leaf { property tone: color; } use Leaf { tone: bounds; } }").startsWith ("drawing/use[0].tone: "));
    EXPECT_TRUE (createYuiError ("drawing { drawing Leaf { rect { bounds: bounds; fill: bounds; } } use Leaf {} }").startsWith ("drawing/use[0]/Leaf/rect[0].fill: "));

    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    const auto loop = DrawingStyle::create (slider, parseYuiDrawing (text, "Loop"), nullptr, nullptr, resolver);
    ASSERT_TRUE (loop.failed());
    EXPECT_TRUE (loop.getErrorMessage().endsWith ("the drawing 'Loop' ends up using itself")) << loop.getErrorMessage();

    const auto lanes = DrawingStyle::create (slider, parseYuiDrawing ("drawing { use Lanes {} }"), nullptr, nullptr, resolver);
    ASSERT_TRUE (lanes.failed());
    EXPECT_EQ (lanes.getErrorMessage(), "drawing/use[0]: the array property 'lanes' of the drawing can't be set by a 'use'");
}

TEST_F (DrawingStyleLanguageTest, DrawingsFoundByNameAreCompiledAgainWhenTheyChange)
{
    Slider first (Slider::RotaryVerticalDrag);
    Slider second (Slider::RotaryVerticalDrag);
    configure (first);
    configure (second);

    const auto user = parseYuiDrawing ("drawing { use Badge {} }");
    auto badge = parseYuiDrawing ("drawing Badge { rect { bounds: bounds; fill: trackColor; } }");

    const auto resolver = [&badge] (const String& name)
    {
        return name == "Badge" ? badge : DataTree();
    };

    ASSERT_TRUE (DrawingStyle::create (first, user, nullptr, nullptr, resolver).wasOk());

    badge.getChild (0).beginTransaction().setProperty ("fill", "missingColor");

    const auto edited = DrawingStyle::create (second, user, nullptr, nullptr, resolver);
    ASSERT_TRUE (edited.failed());
    EXPECT_TRUE (edited.getErrorMessage().contains ("missingColor"));
}

TEST_F (DrawingStyleLanguageTest, PatternsAndMarkersAreReferredToByName)
{
    EXPECT_EQ (createYuiError (R"yui(
        drawing {
            pattern Dots { tile: rect (0, 0, 4, 4); viewBox: rect (0, 0, 8, 8); fitting: fill; transform: transform.rotation (pi / 4);
                           circle { center: bounds.center; radius: 2; fill: trackColor; } }
            marker Arrow { size: size (6, 6); ref: point (3, 3); viewBox: rect (0, 0, 6, 6); orient: autoStartReverse; units: userSpace;
                           path { data: "M 0 0 L 6 3 L 0 6 Z"; fill: thumbColor; } }
            marker Dot { orient: 45deg; circle { center: bounds.center; radius: 1; } }

            rect { bounds: bounds; fillPattern: Dots; strokePattern: Dots; strokeWidth: 2; }
            path { data: "M 2 2 L 20 2 L 20 20"; fill: none; stroke: thumbColor; markerStart: Arrow; markerMid: Dot; markerEnd: Arrow; }
            group { fillPattern: Dots; markerEnd: Arrow; stroke: thumbColor;
                    line { from: bounds.topLeft; to: bounds.center; }
                    line { from: bounds.center; to: bounds.bottomRight; markerEnd: none; }
                    repeat i in 0 .. 3 { line { from: point (i, 0); to: point (i, 8); } } }
        })yui"),
               String());

    EXPECT_EQ (createYuiError ("drawing { rect { bounds: bounds; fillPattern: Dots; } }"), "drawing/rect[0].fillPattern: unknown pattern 'Dots'");
    EXPECT_EQ (createYuiError ("drawing { group { strokePattern: Dots; } }"), "drawing/group[0].strokePattern: unknown pattern 'Dots'");
    EXPECT_EQ (createYuiError ("drawing { line { from: bounds.topLeft; to: bounds.center; markerEnd: Arrow; } }"), "drawing/line[0].markerEnd: unknown marker 'Arrow'");
    EXPECT_EQ (createYuiError ("drawing { rect { bounds: bounds; markerEnd: Arrow; } }"), "drawing/rect[0]: unknown property 'markerEnd'");
    EXPECT_EQ (createYuiError ("drawing { pattern Dots {} }"), "drawing/pattern[0]: missing the required property 'tile'");
    EXPECT_EQ (createYuiError ("drawing { pattern Dots { tile: bounds; } pattern Dots { tile: bounds; } }"), "drawing/pattern[1].name: the pattern 'Dots' is already defined");
    EXPECT_EQ (createYuiError ("drawing { marker Arrow { units: pixels; } }"), "drawing/marker[0].units: expected 'strokeWidth' or 'userSpace'");
    EXPECT_EQ (createYuiError ("drawing { marker Arrow {} repeat i in 0 .. 2 { line { from: point (i, 0); to: point (i, 8); markerEnd: Arrow; } } }"),
               "drawing/repeat[0]/line[0].markerEnd: a shape inside a repeat can't have markers");
}

TEST_F (DrawingStyleLanguageTest, ClipsToTheShapesOfAClipPath)
{
    EXPECT_EQ (createYuiError (R"yui(
        drawing {
            clip {
                clipPath { circle { center: bounds.center; radius: 8; } rect { bounds: rect (0, 0, 12, 24); transform: transform.translation (2, 0); }
                           repeat i in 0 .. 3 { ellipse { bounds: rect (i * 4, 0, 3, 3); } } }
                clipPath { path { data: "M 0 0 L 24 0 L 24 24 Z"; } }
                rect { bounds: bounds; fill: trackColor; }
            }
            layer { opacity: 0.5; clipPath { star { center: bounds.center; points: 5; innerRadius: 4; outerRadius: 8; } } rect { bounds: bounds; fill: thumbColor; } }
        })yui"),
               String());

    EXPECT_EQ (createYuiError ("drawing { clipPath { rect { bounds: bounds; } } }"), "drawing/clipPath[0]: a 'clipPath' can only appear inside a 'clip' or a 'layer'");
    EXPECT_EQ (createYuiError ("drawing { clip { clipPath { text { bounds: bounds; text: \"a\"; } } } }"), "drawing/clip[0]/clipPath[0]/text[0]: a 'text' has no outline to clip to");
    EXPECT_EQ (createYuiError ("drawing { clip { clipPath { group {} } } }"), "drawing/clip[0]/clipPath[0]/group[0]: a 'group' has no outline to clip to");
    EXPECT_EQ (createYuiError ("drawing { clip { clipPath { repeat i in 0 .. 2 { text { bounds: bounds; text: \"a\"; } } } } }"),
               "drawing/clip[0]/clipPath[0]/repeat[0]/text[0]: a 'text' has no outline to clip to");
}

TEST_F (DrawingStyleLanguageTest, DrawsLayersThroughTheFilterTheyHold)
{
    EXPECT_TRUE (createYuiError (R"yui(
        drawing {
            property amount: number = 1;

            layer {
                opacity: 0.5;
                filter {
                    bounds: rect (0, 0, 20, 20);
                    linearColors: true;

                    morphology { in: sourceAlpha; radius: 2; dilate: true; result: fat; }
                    blur { deviation: 2 * amount; }
                    offset { offset: point (1, amount); result: moved; }
                    flood { color: palette.accent; result: tinted; }
                    composite { in: tinted; in2: moved; operator: arithmetic; k1: 1; k2: 0.5; }
                    blend { in: source; in2: fat; mode: multiply; }
                    colorMatrix { saturate: amount; }
                    colorMatrix { matrix: "1 0 0 0 0  0 1 0 0 0  0 0 1 0 0  0 0 0 1 0"; }
                    lighting { scale: 2; constant: 1; specular: true; exponent: 4; color: #ffffff; position: point (10, 10); height: 5; }
                    displacement { in2: moved; scale: 3; xChannel: red; yChannel: green; }
                    dropShadow { offset: point (2, 2); deviation: 1; color: #000000; }
                }
                rect { bounds: rect (4, 4, 12, 12); fill: #ff0000; }
            }
        })yui").isEmpty());

    EXPECT_TRUE (createYuiError ("drawing { filter { blur { deviation: 2; } } }").contains ("can only appear inside a 'layer'"));
    EXPECT_TRUE (createYuiError ("drawing { layer { filter { } } }").contains ("needs at least one primitive"));
    EXPECT_TRUE (createYuiError ("drawing { layer { filter { rect { bounds: bounds; } } } }").contains ("is made of blur"));
    EXPECT_TRUE (createYuiError ("drawing { layer { filter { blur { radius: 2; } } } }").contains ("unknown property 'radius'"));
    EXPECT_TRUE (createYuiError ("drawing { layer { filter { offset { offset: 2; } } } }").contains ("expected a point"));
    EXPECT_TRUE (createYuiError ("drawing { layer { filter { composite { operator: under; } } } }").contains ("expected over, in, out"));
    EXPECT_TRUE (createYuiError ("drawing { layer { filter { morphology { radius: 1; dilate: maybe; } } } }").contains ("expected true or false"));
    EXPECT_TRUE (createYuiError ("drawing { layer { filter { colorMatrix { matrix: \"1 0 0\"; } } } }").contains ("20 numbers"));
    EXPECT_TRUE (createYuiError ("drawing { layer { filter { blur { deviation: 1; } } filter { blur { deviation: 1; } } } }").contains ("a single 'filter'"));
    EXPECT_TRUE (createYuiError ("drawing { layer { filter { blur { deviation: 1; } } mask { rect { bounds: bounds; fill: #ffffff; } } } }").contains ("can't be masked"));
}

TEST_F (DrawingStyleLanguageTest, PlacesTextFromAnAnchorAndRoundsRectanglesElliptically)
{
    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseYuiDrawing (R"yui(
        drawing {
            rect { bounds: bounds; cornerSize: size (12, 4); fill: trackColor; }
            text { anchor: point (24, 30); text: format ("%.2f", value); textAnchor: middle; letterSpacing: 0.5; font: defaultFont.withHeight (12); fill: thumbColor; stroke: trackColor; }
            text { anchor: bounds.center; text: "end"; textAnchor: end; }
        })yui"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    EXPECT_NO_THROW (paintHeadless (*style.getReference(), slider));

    EXPECT_EQ (createYuiError ("drawing { text { bounds: bounds; anchor: bounds.center; text: \"a\"; } }"), "drawing/text[0]: specify either 'bounds' or 'anchor', not both");
    EXPECT_EQ (createYuiError ("drawing { text { text: \"a\"; } }"), "drawing/text[0]: missing the required property 'bounds'");
    EXPECT_EQ (createYuiError ("drawing { text { bounds: bounds; text: \"a\"; textAnchor: middle; } }"), "drawing/text[0]: unknown property 'textAnchor'");
    EXPECT_EQ (createYuiError ("drawing { text { anchor: bounds.center; text: \"a\"; justification: center; } }"), "drawing/text[0]: unknown property 'justification'");
}

TEST_F (DrawingStyleLanguageTest, PaintsEveryNewElementHeadless)
{
    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    auto style = DrawingStyle::create (slider, parseYuiDrawing (R"yui(
        drawing {
            viewBox: rect (0, 0, 24, 24);

            drawing Leaf { property tone: color = #00ff00; viewBox: rect (0, 0, 10, 10); path { data: "M 5 0 L 10 10 L 0 10 Z"; fill: tone; } }
            pattern Dots { tile: rect (0, 0, 4, 4); circle { center: bounds.center; radius: 1; fill: trackColor; } }
            pattern Live { tile: rect (0, 0, 4, 4); rect { bounds: bounds.reduced (normalized); fill: thumbColor; } }
            marker Arrow { size: size (3, 3); ref: point (1.5, 1.5); viewBox: rect (0, 0, 6, 6); path { data: "M 0 0 L 6 3 L 0 6 Z"; } }

            group {
                fill: #000; color: trackColor; strokeWidth: 2;

                rect { bounds: rect (2, 2, 20, 20); cornerSize: size (4, 2); fillPattern: Dots; stroke: currentColor; strokePattern: Live; }
                path { data: "M 2 12 L 12 12 L 22 20"; fill: none; stroke: currentColor; markerStart: Arrow; markerMid: Arrow; markerEnd: Arrow; transform: transform.translation (1, 0); opacity: 0.5; }
                use Leaf { bounds: rect (14, 2, 8, 8); tone: thumbColor; }

                clip {
                    clipPath { circle { center: point (12, 12); radius: 8; } rect { bounds: rect (0, 0, 12, 24); } }
                    layer { opacity: 0.5; mask: rect (0, 0, 12, 12); rect { bounds: bounds; fillGradient: linearGradient (#f00, point (0, 0), #00f, point (0.25, 0)).withSpread (repeat).mappedTo (objectBounds); } }
                }

                viewport { bounds: rect (0, 0, 8, 8); viewBox: rect (0, 0, 100, 100); fitting: scaleToFill; circle { center: bounds.center; radius: 40; } }
                text { anchor: point (12, 20); textAnchor: middle; text: "yup"; }
            }
        })yui"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    EXPECT_NO_THROW (paintHeadless (*style.getReference(), slider));

    slider.setValue (1.0, dontSendNotification);
    EXPECT_NO_THROW (paintHeadless (*style.getReference(), slider));
}

//==============================================================================
class DrawingStyleGpuTest : public DrawingStyleTest
{
protected:
    struct DrawnComponent : Component
    {
        explicit DrawnComponent (const DataTree& drawing, const DrawingStyle::DrawingResolver& drawingResolver = nullptr)
        {
            setOpaque (false);
            setBounds (0.0f, 0.0f, 16.0f, 16.0f);

            auto created = DrawingStyle::create (*this, drawing, nullptr, nullptr, drawingResolver);
            EXPECT_TRUE (created.wasOk()) << created.getErrorMessage();

            if (created.wasOk())
                setStyle (created.getReference());
        }

        void paint (Graphics& g) override
        {
            if (auto currentStyle = getStyle())
                currentStyle->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
        }
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
        DrawingStyleTest::SetUp();

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

    /** Draws a 16 x 16 component with a drawing and returns what it painted. */
    static Image render (const String& text, const DrawingStyle::DrawingResolver& drawingResolver = nullptr)
    {
        DrawnComponent component (parseYui (text), drawingResolver);
        return component.snapshotToImage (*gpuContext);
    }

    static std::unique_ptr<GraphicsContext> gpuContext;
};

std::unique_ptr<GraphicsContext> DrawingStyleGpuTest::gpuContext;

TEST_F (DrawingStyleGpuTest, PropertiesReadTheComponentAndItsParentsOrTheirDefault)
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

TEST_F (DrawingStyleGpuTest, ACachedDrawingIsPaintedAgainWhenWhatItReadsChanges)
{
    const auto drawing = parseYui (R"yui(
        drawing {
            property shade: color = #ff0000;
            rect { bounds: bounds; fill: enabled ? shade : #0000ff; }
        })yui");

    Component parent;
    parent.setOpaque (false);
    parent.setBounds (0.0f, 0.0f, 16.0f, 16.0f);

    DrawnComponent child (drawing);
    child.setProperty (Identifier ("cached"), true);
    parent.addAndMakeVisible (child);

    ASSERT_TRUE (child.isCachedToTexture());
    EXPECT_EQ (centerColor (parent), Color (0xffff0000));

    child.setProperty (Identifier ("shade"), VariantConverter<Color>::toVar (Color (0xff00ff00)));
    EXPECT_EQ (centerColor (parent), Color (0xff00ff00));

    // The enabled state comes from the parent, which is the only one repainted
    parent.setEnabled (false);
    EXPECT_EQ (centerColor (parent), Color (0xff0000ff));
}

TEST_F (DrawingStyleGpuTest, ClipsShapesMasksLayersAndRendersInlineSvg)
{
    const auto clipped = parseYui (R"yui(
        drawing {
            clip {
                bounds: rect (6, 6, 4, 4);
                rect { bounds: bounds; fill: #ff0000; }
            }
        })yui");

    DrawnComponent clippedComp (clipped);
    const auto clippedImage = clippedComp.snapshotToImage (*gpuContext);
    ASSERT_TRUE (clippedImage.isValid());
    EXPECT_EQ (clippedImage.getPixelColor (8, 8), Color (0xffff0000));
    EXPECT_EQ (clippedImage.getPixelColor (1, 1).getAlpha(), 0);

    const auto masked = parseYui (R"yui(
        drawing {
            layer {
                rect { bounds: bounds; fill: #ff0000; }
                mask {
                    mode: alpha;
                    rect { bounds: rect (6, 6, 4, 4); fill: #ffffff; }
                }
            }
        })yui");

    DrawnComponent maskedComp (masked);
    const auto maskedImage = maskedComp.snapshotToImage (*gpuContext);
    ASSERT_TRUE (maskedImage.isValid());
    EXPECT_EQ (maskedImage.getPixelColor (8, 8), Color (0xffff0000));
    EXPECT_EQ (maskedImage.getPixelColor (1, 1).getAlpha(), 0);

    const auto inlineSvg = parseYui (R"yui(
        drawing {
            svg {
                bounds: bounds;
                data: "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'><rect width='16' height='16' fill='#00ff00'/></svg>";
            }
        })yui");

    DrawnComponent svgComp (inlineSvg);
    EXPECT_EQ (centerColor (svgComp), Color (0xff00ff00));

    const auto builtPath = parseYui (R"yui(
        drawing {
            let area: path().addRectangle (bounds);
            path { data: area; fill: #0000ff; }
        })yui");

    DrawnComponent pathComp (builtPath);
    EXPECT_EQ (centerColor (pathComp), Color (0xff0000ff));
}

TEST_F (DrawingStyleGpuTest, PaintsPlainAndCompositeComponentsWithoutCustomPaintOverride)
{
    const auto solid = parseYui (R"yui(
        drawing {
            rect { bounds: bounds; fill: #ff0000; }
        })yui");

    Component plain;
    plain.setOpaque (false);
    plain.setBounds (0.0f, 0.0f, 16.0f, 16.0f);

    auto created = DrawingStyle::create (plain, solid);
    ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();
    plain.setStyle (created.getReference());
    EXPECT_EQ (centerColor (plain), Color (0xffff0000));

    YuiDocument document;
    ASSERT_TRUE (document.loadFromString (R"yui(
        component ScopeBox : Component {
            property shade: color = #00ff00;
            class: scope;
        }
        drawing ScopeDrawing {
            rect { bounds: bounds; fill: shade; }
        }
        style {
            .scope { drawing: ScopeDrawing; }
        }
        Row {
            ScopeBox { id: scope; flexGrow: 1; }
        })yui")
                     .wasOk());

    auto* root = document.getRoot();
    ASSERT_NE (root, nullptr);
    root->setBounds (0.0f, 0.0f, 16.0f, 16.0f);

    auto* scope = document.findById ("scope");
    ASSERT_NE (scope, nullptr);
    EXPECT_EQ (centerColor (*scope), Color (0xff00ff00));
}

TEST_F (DrawingStyleGpuTest, ViewBoxMapsTheDrawingAndViewportsClipTheirChildren)
{
    // Four units across the 16 pixels of the component: the square covers the pixels 4 to 12
    const auto mapped = render (R"yui(
        drawing {
            viewBox: rect (0, 0, 4, 4);
            rect { bounds: rect (1, 1, 2, 2); fill: #ff0000; }
        })yui");
    ASSERT_TRUE (mapped.isValid());
    EXPECT_EQ (mapped.getPixelColor (8, 8), Color (0xffff0000));
    EXPECT_EQ (mapped.getPixelColor (2, 2).getAlpha(), 0);
    EXPECT_EQ (mapped.getPixelColor (14, 8).getAlpha(), 0);

    // The view box keeps its proportions: a 4 x 8 box is centered in the middle half of the component
    const auto fitted = render (R"yui(
        drawing {
            viewBox: rect (0, 0, 4, 8);
            rect { bounds: bounds; fill: #00ff00; }
        })yui");
    ASSERT_TRUE (fitted.isValid());
    EXPECT_EQ (fitted.getPixelColor (8, 8), Color (0xff00ff00));
    EXPECT_EQ (fitted.getPixelColor (2, 8).getAlpha(), 0);

    // A viewport maps its own view box into its bounds, and clips what overflows them
    const auto clipped = render (R"yui(
        drawing {
            viewport {
                bounds: rect (8, 0, 8, 16); viewBox: rect (10, 10, 2, 4);
                rect { bounds: rect (0, 0, 100, 100); fill: #0000ff; }
            }
        })yui");
    ASSERT_TRUE (clipped.isValid());
    EXPECT_EQ (clipped.getPixelColor (12, 8), Color (0xff0000ff));
    EXPECT_EQ (clipped.getPixelColor (4, 8).getAlpha(), 0);
}

TEST_F (DrawingStyleGpuTest, LayersKeepTheirPlaceUnderAViewBox)
{
    const auto image = render (R"yui(
        drawing {
            viewBox: rect (10, 10, 4, 4);
            layer {
                opacity: 0.5;
                rect { bounds: rect (12, 10, 2, 4); fill: #ff0000; }
            }
        })yui");
    ASSERT_TRUE (image.isValid());

    // The right half of the view box, half transparent
    EXPECT_NEAR (image.getPixelColor (12, 8).getAlpha(), 128, 8);
    EXPECT_EQ (image.getPixelColor (4, 8).getAlpha(), 0);
}

TEST_F (DrawingStyleGpuTest, GroupsHandTheirPaintDownToTheirShapes)
{
    const auto image = render (R"yui(
        drawing {
            group {
                fill: #00ff00; color: #0000ff;
                rect { bounds: rect (0, 0, 4, 16); }
                rect { bounds: rect (4, 0, 4, 16); fill: currentColor; }
                rect { bounds: rect (8, 0, 4, 16); fill: none; }
                group { fill: #ff0000; rect { bounds: rect (12, 0, 4, 16); } }
            }
        })yui");
    ASSERT_TRUE (image.isValid());

    EXPECT_EQ (image.getPixelColor (2, 8), Color (0xff00ff00));
    EXPECT_EQ (image.getPixelColor (6, 8), Color (0xff0000ff));
    EXPECT_EQ (image.getPixelColor (10, 8).getAlpha(), 0);
    EXPECT_EQ (image.getPixelColor (14, 8), Color (0xffff0000));

    const auto faded = render (R"yui(
        drawing {
            group { fill: #ff0000; fillOpacity: 0.5; rect { bounds: bounds; } }
        })yui");
    ASSERT_TRUE (faded.isValid());
    EXPECT_NEAR (faded.getPixelColor (8, 8).getAlpha(), 128, 8);
}

TEST_F (DrawingStyleGpuTest, GradientsFollowTheBoundsOfTheShapeAndRepeat)
{
    // Red to blue over a quarter of each shape, repeated: every shape starts red and ends blue
    const auto image = render (R"yui(
        drawing {
            group {
                fillGradient: linearGradient (#ff0000, point (0, 0), #0000ff, point (0.5, 0)).withSpread (repeat).mappedTo (objectBounds);
                rect { bounds: rect (0, 0, 16, 8); }
                rect { bounds: rect (8, 8, 8, 8); }
            }
        })yui");
    ASSERT_TRUE (image.isValid());

    const auto expectRed = [&image] (int x, int y)
    {
        EXPECT_GT (image.getPixelColor (x, y).getRed(), 180) << "at " << x << ", " << y;
        EXPECT_LT (image.getPixelColor (x, y).getBlue(), 80) << "at " << x << ", " << y;
    };

    const auto expectBlue = [&image] (int x, int y)
    {
        EXPECT_GT (image.getPixelColor (x, y).getBlue(), 180) << "at " << x << ", " << y;
        EXPECT_LT (image.getPixelColor (x, y).getRed(), 80) << "at " << x << ", " << y;
    };

    // The wide shape repeats every 8 pixels, the small one every 4
    expectRed (0, 4);
    expectBlue (7, 4);
    expectRed (8, 4);
    expectBlue (15, 4);
    expectRed (8, 12);
    expectBlue (11, 12);
    expectRed (12, 12);
}

TEST_F (DrawingStyleGpuTest, UsePaintsADrawingInItsBoundsWithItsProperties)
{
    const String text (R"yui(
        drawing {
            drawing Swatch {
                property tone: color = #ff0000;
                viewBox: rect (0, 0, 2, 2);
                rect { bounds: bounds; fill: tone; }
            }

            use Swatch { bounds: rect (0, 0, 8, 8); }
            use Swatch { bounds: rect (8, 0, 8, 8); tone: #0000ff; }
            group { fill: #00ff00; color: #ff00ff; use Plain { bounds: rect (0, 8, 8, 8); } }
            use Plain { bounds: rect (0, 0, 8, 8); transform: transform.translation (8, 8); fill: #ffffff; color: #ffff00; }
        }
        drawing Plain { rect { bounds: bounds; } rect { bounds: rect (bounds.x, bounds.y, 2, 2); fill: currentColor; } })yui");

    const auto resolver = [&text] (const String& name)
    {
        const auto parsed = YuiParser::parse (text);

        for (const auto& block : parsed.document)
            if (block.getProperty ("name").toString() == name)
                return DataTree (block);

        return DataTree();
    };

    DrawnComponent component (parseYui (text), resolver);
    component.setProperty (Identifier ("tone"), VariantConverter<Color>::toVar (Color (0xffffffff)));

    const auto image = component.snapshotToImage (*gpuContext);
    ASSERT_TRUE (image.isValid());

    // The default of the property rather than the value of the component, then the value set by the use
    EXPECT_EQ (image.getPixelColor (4, 4), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (12, 4), Color (0xff0000ff));

    // The fill and the color of the group around the use, then the ones of the use itself, moved by its transform
    EXPECT_EQ (image.getPixelColor (4, 12), Color (0xff00ff00));
    EXPECT_EQ (image.getPixelColor (1, 9), Color (0xffff00ff));
    EXPECT_EQ (image.getPixelColor (12, 12), Color (0xffffffff));
    EXPECT_EQ (image.getPixelColor (9, 9), Color (0xffffff00));
}

TEST_F (DrawingStyleGpuTest, PatternsTileTheShapesTheyPaint)
{
    const String stripes (R"yui(
        drawing {
            property shade: color = #ff0000;
            pattern Stripes { tile: rect (2, 0, 8, 8); rect { bounds: rect (0, 0, 4, 8); fill: shade; } }
            rect { bounds: rect (0, 0, 16, 8); fillPattern: Stripes; }
            line { from: point (0, 12); to: point (16, 12); strokePattern: Stripes; strokeWidth: 4; }
        })yui");

    DrawnComponent component (parseYui (stripes));

    // The tiles start two pixels in: four painted pixels, then four empty ones
    const auto image = component.snapshotToImage (*gpuContext);
    ASSERT_TRUE (image.isValid());

    for (const auto y : { 4, 12 })
    {
        EXPECT_EQ (image.getPixelColor (4, y), Color (0xffff0000)) << "at y = " << y;
        EXPECT_EQ (image.getPixelColor (8, y).getAlpha(), 0) << "at y = " << y;
        EXPECT_EQ (image.getPixelColor (12, y), Color (0xffff0000)) << "at y = " << y;
        EXPECT_EQ (image.getPixelColor (0, y).getAlpha(), 0) << "at y = " << y;
    }

    // The tile follows what its elements read
    component.setProperty (Identifier ("shade"), VariantConverter<Color>::toVar (Color (0xff0000ff)));
    EXPECT_EQ (component.snapshotToImage (*gpuContext).getPixelColor (4, 4), Color (0xff0000ff));
}

TEST_F (DrawingStyleGpuTest, MarkersAreDrawnAtTheVerticesOfShapes)
{
    const auto image = render (R"yui(
        drawing {
            marker Square { size: size (4, 4); ref: point (2, 2); units: userSpace; rect { bounds: bounds; fill: #ff0000; } }
            marker Bar { size: size (2, 6); ref: point (1, 3); units: userSpace; orient: 0; rect { bounds: bounds; fill: #00ff00; } }

            path { data: "M 3 3 L 3 12 L 12 12"; fill: none; stroke: #0000ff; strokeWidth: 2; markerStart: Square; markerMid: Bar; markerEnd: Square; }
        })yui");
    ASSERT_TRUE (image.isValid());

    // A square on the first and on the last vertex, an upright bar on the corner, the stroke in between
    EXPECT_EQ (image.getPixelColor (3, 3), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (12, 12), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (3, 12), Color (0xff00ff00));
    EXPECT_EQ (image.getPixelColor (3, 7), Color (0xff0000ff));
    EXPECT_EQ (image.getPixelColor (8, 12), Color (0xff0000ff));
    EXPECT_EQ (image.getPixelColor (10, 4).getAlpha(), 0);
}

TEST_F (DrawingStyleGpuTest, MarkersFollowTheDirectionAndTheStrokeWidthOfThePath)
{
    // A bar across the end of a vertical line: one unit long and three wide, scaled by the stroke width of two
    const auto image = render (R"yui(
        drawing {
            marker Bar { size: size (1, 3); ref: point (0.5, 1.5); rect { bounds: bounds; fill: #ff0000; } }
            line { from: point (8, 2); to: point (8, 8); stroke: #0000ff; strokeWidth: 2; markerEnd: Bar; }
        })yui");
    ASSERT_TRUE (image.isValid());

    EXPECT_EQ (image.getPixelColor (6, 8), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (10, 8), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (8, 4), Color (0xff0000ff));
    EXPECT_EQ (image.getPixelColor (8, 13).getAlpha(), 0);
    EXPECT_EQ (image.getPixelColor (3, 8).getAlpha(), 0);
}

TEST_F (DrawingStyleGpuTest, ClipPathsClipToAllTheirShapes)
{
    const auto image = render (R"yui(
        drawing {
            clip {
                clipPath {
                    rect { bounds: rect (0, 0, 4, 16); }
                    repeat i in 0 .. 2 { rect { bounds: rect (8 + i * 4, 0, 2, 16); } }
                }
                clipPath { rect { bounds: rect (0, 4, 16, 8); } }
                rect { bounds: bounds; fill: #ff0000; }
            }
        })yui");
    ASSERT_TRUE (image.isValid());

    // The shapes of a clip path add up, and the clip paths intersect
    EXPECT_EQ (image.getPixelColor (2, 8), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (9, 8), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (13, 8), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (6, 8).getAlpha(), 0);
    EXPECT_EQ (image.getPixelColor (11, 8).getAlpha(), 0);
    EXPECT_EQ (image.getPixelColor (2, 1).getAlpha(), 0);
}

TEST_F (DrawingStyleGpuTest, ARepeatHoldingContainersDrawsThemOncePerTurn)
{
    const auto image = render (R"yui(
        drawing {
            drawing Dot {
                property tone: color = #ff0000;
                rect { bounds: bounds; fill: tone; }
            }

            repeat i in 0 .. 3 {
                group {
                    transform: transform.translation (i * 5, 0);
                    rect { bounds: rect (0, 0, 3, 4); fill: #ff0000; }
                }

                use Dot { bounds: rect (i * 5, 6, 3, 4); tone: i == 1 ? #00ff00 : #0000ff; }
            }

            repeat row in 0 .. 2 {
                repeat column in 0 .. row + 1 { rect { bounds: rect (column * 5, 12 + row * 2, 3, 2); fill: #ffffff; } }
            }
        })yui");
    ASSERT_TRUE (image.isValid());

    for (int turn = 0; turn < 3; ++turn)
    {
        // The group is moved by the index of its turn
        EXPECT_EQ (image.getPixelColor (turn * 5 + 1, 2), Color (0xffff0000)) << turn;
        EXPECT_EQ (image.getPixelColor (turn * 5 + 4, 2).getAlpha(), 0) << turn;

        // A drawing is painted with what its turn sets
        EXPECT_EQ (image.getPixelColor (turn * 5 + 1, 8), turn == 1 ? Color (0xff00ff00) : Color (0xff0000ff)) << turn;
    }

    // A repeat inside a repeat reads the index around it: one rectangle on the first row, two on the second
    EXPECT_EQ (image.getPixelColor (1, 12), Color (0xffffffff));
    EXPECT_EQ (image.getPixelColor (6, 12).getAlpha(), 0);
    EXPECT_EQ (image.getPixelColor (6, 15), Color (0xffffffff));
    EXPECT_EQ (image.getPixelColor (11, 15).getAlpha(), 0);
}

TEST_F (DrawingStyleGpuTest, TheTurnsOfARepeatHoldingContainersAreDrawnOneAfterTheOther)
{
    // Each turn draws a red bar, then a blue one over its right end: the red of the second turn covers the blue of the first
    const auto image = render (R"yui(
        drawing {
            repeat i in 0 .. 2 {
                group { rect { bounds: rect (i * 6, 0, 10, 16); fill: #ff0000; } }
                rect { bounds: rect (i * 6 + 6, 0, 4, 16); fill: #0000ff; }
            }
        })yui");
    ASSERT_TRUE (image.isValid());

    EXPECT_EQ (image.getPixelColor (2, 8), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (8, 8), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (14, 8), Color (0xff0000ff));
}

TEST_F (DrawingStyleGpuTest, LayersAreDrawnThroughTheirFilter)
{
    // The red square is moved four pixels to the right, and what leaves the bounds of the filter is lost
    const auto moved = render (R"yui(
        drawing {
            layer {
                filter { bounds: rect (0, 0, 12, 16); offset { offset: point (4, 0); } }
                rect { bounds: rect (2, 4, 8, 8); fill: #ff0000; }
            }
        })yui");
    ASSERT_TRUE (moved.isValid());
    EXPECT_EQ (moved.getPixelColor (3, 8).getAlpha(), 0);
    EXPECT_EQ (moved.getPixelColor (8, 8), Color (0xffff0000));
    EXPECT_EQ (moved.getPixelColor (13, 8).getAlpha(), 0);

    // Primitives read the layer and each other by name, and the result gets the opacity of the layer
    const auto tinted = render (R"yui(
        drawing {
            group {
                fill: #ff0000;

                layer {
                    opacity: 0.5;
                    filter {
                        flood { color: #0000ff; result: blue; }
                        composite { in: blue; in2: sourceAlpha; operator: in; }
                    }
                    rect { bounds: rect (4, 4, 8, 8); }
                }
            }
        })yui");
    ASSERT_TRUE (tinted.isValid());
    EXPECT_EQ (tinted.getPixelColor (1, 1).getAlpha(), 0);
    EXPECT_EQ (tinted.getPixelColor (8, 8).getRed(), 0);
    EXPECT_NEAR (tinted.getPixelColor (8, 8).getAlpha(), 128, 4);
}

TEST_F (DrawingStyleGpuTest, FiltersFollowThePropertiesTheyRead)
{
    DrawnComponent component (parseYui (R"yui(
        drawing {
            property shift: number = 0;

            layer {
                filter { offset { offset: point (shift, 0); } }
                rect { bounds: rect (0, 4, 4, 8); fill: #00ff00; }
            }
        })yui"));

    EXPECT_EQ (component.snapshotToImage (*gpuContext).getPixelColor (2, 8), Color (0xff00ff00));

    component.setProperty ("shift", 8.0);

    const auto image = component.snapshotToImage (*gpuContext);
    EXPECT_EQ (image.getPixelColor (2, 8).getAlpha(), 0);
    EXPECT_EQ (image.getPixelColor (10, 8), Color (0xff00ff00));
}

TEST_F (DrawingStyleGpuTest, ImagesComeFromTheResolverBeforeTheFiles)
{
    DrawnComponent component (parseYui (R"yui(
        drawing {
            image { bounds: bounds; source: "memory:blue"; fitting: fill; }
        })yui"));

    // Nothing is called that, among the files
    EXPECT_EQ (centerColor (component).getAlpha(), 0);

    auto* style = dynamic_cast<DrawingStyle*> (component.getStyle().get());
    ASSERT_NE (style, nullptr);

    StringArray asked;
    style->setImageResolver ([&asked] (const String& source) -> std::optional<Image>
    {
        asked.add (source);

        Image image (4, 4);
        image.fillColor (Color (0xff0000ff));
        return image;
    });

    for (int pass = 0; pass < 2; ++pass)
    {
        const auto painted = centerColor (component);
        EXPECT_GT (painted.getBlue(), 200);
        EXPECT_LT (painted.getRed(), 50);
        EXPECT_GT (painted.getAlpha(), 200);
    }

    // Asked once for the source, what it returned is kept
    EXPECT_EQ (asked, StringArray ({ "memory:blue" }));

    style->setImageResolver (nullptr);
    EXPECT_EQ (centerColor (component).getAlpha(), 0);
}

TEST_F (DrawingStyleGpuTest, CornerSizeRoundsRectanglesElliptically)
{
    const auto image = render (R"yui(
        drawing {
            rect { bounds: bounds; cornerSize: size (8, 2); fill: #ff0000; }
        })yui");
    ASSERT_TRUE (image.isValid());

    // The corners are eight pixels wide and two tall
    EXPECT_EQ (image.getPixelColor (8, 1), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (0, 8), Color (0xffff0000));
    EXPECT_EQ (image.getPixelColor (2, 2), Color (0xffff0000));
    EXPECT_LT (image.getPixelColor (0, 0).getAlpha(), 64);
}

TEST_F (DrawingStyleTest, ListsTheElementsADrawingIsMadeOf)
{
    const auto elements = DrawingStyle::getElementDescriptions();

    const auto find = [&elements] (const String& type, const String& firstGeometry = {})
    {
        const auto found = std::find_if (elements.begin(), elements.end(), [&] (const DrawingStyle::ElementDescription& element)
        {
            return element.type == type && (firstGeometry.isEmpty() || element.geometry.contains (firstGeometry));
        });

        return found != elements.end() ? *found : DrawingStyle::ElementDescription();
    };

    EXPECT_EQ (find ("rect").role, String ("shape"));
    EXPECT_EQ (find ("rect").geometry, StringArray ({ "bounds" }));
    EXPECT_TRUE (find ("rect").properties.contains ("cornerRadius"));
    EXPECT_FALSE (find ("rect").properties.contains ("markerEnd"));
    EXPECT_TRUE (find ("line").properties.contains ("markerEnd"));
    EXPECT_TRUE (find ("star").geometry.contains ("angle=0"));

    // A text is fitted in its bounds or placed at an anchor, with other properties for each
    EXPECT_TRUE (find ("text", "bounds").properties.contains ("justification"));
    EXPECT_TRUE (find ("text", "anchor").properties.contains ("textAnchor"));

    EXPECT_EQ (find ("group").role, String ("container"));
    EXPECT_TRUE (find ("group").properties.contains ("transform"));
    EXPECT_TRUE (find ("group").properties.contains ("fill"));
    EXPECT_FALSE (find ("viewport").properties.contains ("fill"));
    EXPECT_TRUE (find ("svg").properties.contains ("data"));
    EXPECT_FALSE (find ("image").properties.contains ("data"));

    EXPECT_EQ (find ("blur").role, String ("filterPrimitive"));
    EXPECT_TRUE (find ("blur").properties.contains ("deviation"));
    EXPECT_TRUE (find ("blur").properties.contains ("result"));

    EXPECT_TRUE (DrawingStyle::getShapePaintProperties().contains ("strokeWidth"));
    EXPECT_TRUE (DrawingStyle::getShapePaintProperties().contains ("dash"));
}

TEST_F (DrawingStyleTest, MeasuresWhatARepeatOfContainersDraws)
{
    const auto parsed = YuiParser::parse (R"yui(
        drawing Panel {
            repeat i in 0 .. 3 {
                group {
                    id: cell;
                    transform: transform.translation (i * 10, 0);
                    rect { id: step; bounds: rect (0, 40, 4, 4); fill: #ffffff; }
                }
            }

            repeat row in 0 .. 100 {
                repeat column in 0 .. 100 { group { rect { id: many; bounds: rect (column, row, 1, 1); fill: #ffffff; } } }
            }
        })yui");
    ASSERT_TRUE (parsed.wasOk()) << parsed.diagnostics.front().toString();

    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    auto style = DrawingStyle::create (slider, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    const auto measured = style.getReference()->measureElements (slider);

    const auto find = [&measured] (const String& id)
    {
        const auto found = std::find_if (measured.begin(), measured.end(), [&id] (const DrawingStyle::ElementBounds& element)
        {
            return element.id == id;
        });

        return found != measured.end() ? *found : DrawingStyle::ElementBounds();
    };

    // An element is told once, covering what all the turns draw
    EXPECT_EQ (find ("step").bounds, Rectangle<float> (0.0f, 40.0f, 24.0f, 4.0f));
    EXPECT_EQ (find ("step").numRows, 3);
    EXPECT_EQ (find ("cell").bounds, Rectangle<float> (0.0f, 40.0f, 24.0f, 4.0f));
    EXPECT_EQ (find ("cell").numRows, 3);

    EXPECT_EQ (std::count_if (measured.begin(), measured.end(), [] (const DrawingStyle::ElementBounds& element) { return element.id == "step"; }), 1);

    // The turns of repeats inside one another add up to no more than a repeat runs
    EXPECT_GT (find ("many").numRows, 100);
    EXPECT_LE (find ("many").numRows, 4096);
}

TEST_F (DrawingStyleTest, MeasuresWhereTheElementsLand)
{
    const auto parsed = YuiParser::parse (R"yui(
        drawing Panel {
            rect { id: frame; bounds: rect (2, 4, 20, 10); fill: #ff0000; }

            group {
                id: shifted;
                transform: transform.translation (10, 20);
                circle { id: dot; center: point (5, 5); radius: 3; fill: #00ff00; }
            }

            viewport {
                id: scaled;
                bounds: rect (0, 24, 48, 24);
                viewBox: rect (0, 0, 24, 12);
                rect { id: half; bounds: rect (0, 0, 12, 12); fill: #0000ff; }
            }

            repeat i in 0 .. 3 {
                rect { id: step; bounds: rect (i * 10, 40, 4, 4); fill: #ffffff; }
            }

            group {
                visible: false;
                rect { id: hidden; bounds: rect (0, 0, 4, 4); fill: #ffffff; }
            }

            text { id: caption; bounds: rect (0, 0, 48, 20); text: "Hi"; fill: #ffffff; justification: topLeft; }
        })yui");
    ASSERT_TRUE (parsed.wasOk()) << parsed.diagnostics.front().toString();

    // The glyphs of a text are measured with the font of the theme
    const auto font = Font::loadFontFromFile (File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("data/fonts/Linefont-VariableFont_wdth,wght.ttf"));
    ASSERT_TRUE (font.wasOk()) << font.getErrorMessage();
    theme->setDefaultFont (font.getValue().withHeight (10.0f));

    Slider slider (Slider::RotaryVerticalDrag);
    configure (slider);

    auto style = DrawingStyle::create (slider, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    const auto measured = style.getReference()->measureElements (slider);

    const auto find = [&measured] (const String& id)
    {
        const auto found = std::find_if (measured.begin(), measured.end(), [&id] (const DrawingStyle::ElementBounds& element)
        {
            return element.id == id;
        });

        return found != measured.end() ? *found : DrawingStyle::ElementBounds();
    };

    EXPECT_EQ (find ("frame").bounds, Rectangle<float> (2.0f, 4.0f, 20.0f, 10.0f));
    EXPECT_EQ (find ("frame").type, String ("rect"));
    EXPECT_EQ (find ("frame").path, String ("Panel/rect#frame"));
    EXPECT_EQ (find ("frame").numRows, 1);

    // The transforms of the groups and the mapping of the viewports around an element place it
    EXPECT_EQ (find ("dot").bounds, Rectangle<float> (12.0f, 22.0f, 6.0f, 6.0f));
    EXPECT_EQ (find ("dot").path, String ("Panel/group#shifted/circle#dot"));
    EXPECT_EQ (find ("half").bounds, Rectangle<float> (0.0f, 24.0f, 24.0f, 24.0f));

    // A container covers what it holds, and what is repeated all its copies
    EXPECT_EQ (find ("shifted").bounds, Rectangle<float> (12.0f, 22.0f, 6.0f, 6.0f));
    EXPECT_EQ (find ("step").bounds, Rectangle<float> (0.0f, 40.0f, 24.0f, 4.0f));
    EXPECT_EQ (find ("step").numRows, 3);

    // What is not shown is not measured
    EXPECT_TRUE (find ("hidden").path.isEmpty());

    // A text is where it is fitted, and its glyphs in the part of it its justification gives them
    EXPECT_EQ (find ("caption").bounds, Rectangle<float> (0.0f, 0.0f, 48.0f, 20.0f));
    EXPECT_FLOAT_EQ (find ("caption").textBounds.getX(), 0.0f);
    EXPECT_GT (find ("caption").textBounds.getWidth(), 0.0f);
    EXPECT_LT (find ("caption").textBounds.getWidth(), 48.0f);

    // Elements are told in the order they are painted, a container before what it holds
    EXPECT_EQ (measured.front().id, String ("frame"));
    EXPECT_EQ (measured[1].id, String ("shifted"));
    EXPECT_EQ (measured[2].id, String ("dot"));
}

TEST_F (DrawingStyleTest, MeasuringFollowsTheStateOfTheComponent)
{
    const auto parsed = YuiParser::parse (R"yui(
        drawing Bar {
            rect { id: level; bounds: rect (0, 0, bounds.width * normalized, 8); fill: #ffffff; }
        })yui");
    ASSERT_TRUE (parsed.wasOk()) << parsed.diagnostics.front().toString();

    Slider slider (Slider::LinearHorizontal);
    configure (slider);

    auto style = DrawingStyle::create (slider, parsed.document.getChildWithName ("drawing"));
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage();

    EXPECT_EQ (style.getReference()->measureElements (slider).front().bounds, Rectangle<float> (0.0f, 0.0f, 24.0f, 8.0f));

    slider.setValue (1.0, dontSendNotification);
    EXPECT_EQ (style.getReference()->measureElements (slider).front().bounds, Rectangle<float> (0.0f, 0.0f, 48.0f, 8.0f));
}
