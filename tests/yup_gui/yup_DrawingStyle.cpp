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

TEST_F (DrawingStyleTest, RejectsContainersInsideRepeat)
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
    EXPECT_EQ (createError ("<drawing><property name=\"a\" type=\"font[]\"/></drawing>"), "drawing/property[0].type: 'font[]' can't be used in a drawing, use number, integer, boolean, string, color, point, rectangle, transform, font or an array of scalar types");
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
    EXPECT_EQ (createError ("<drawing><repeat name=\"i\" from=\"0\" to=\"2\"><clip/></repeat></drawing>"), "drawing/repeat[0]/clip[0]: a 'clip' can't be nested inside a repeat");
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
class DrawingStyleGpuTest : public DrawingStyleTest
{
protected:
    struct DrawnComponent : Component
    {
        explicit DrawnComponent (const DataTree& drawing)
        {
            setOpaque (false);
            setBounds (0.0f, 0.0f, 16.0f, 16.0f);

            auto created = DrawingStyle::create (*this, drawing);
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
}

