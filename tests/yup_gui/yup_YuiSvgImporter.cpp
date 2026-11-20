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

//==============================================================================
class YuiSvgImporterTests : public ::testing::Test
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

    static String makeSvg (const String& body, const String& attributes = "viewBox=\"0 0 100 100\"")
    {
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" " + attributes + ">" + body + "</svg>";
    }

    static YuiSvgImporter::ImportResult importBody (const String& body, const String& attributes = "viewBox=\"0 0 100 100\"")
    {
        return YuiSvgImporter::importSvg (StringRef (makeSvg (body, attributes)));
    }

    /** Returns the nth element of a type inside a tree, depth first, or an invalid tree. */
    static DataTree find (const DataTree& tree, const String& type, int index = 0)
    {
        int remaining = index;
        return findNext (tree, type, remaining);
    }

    static DataTree findNext (const DataTree& tree, const String& type, int& remaining)
    {
        for (const auto& child : tree)
        {
            if (child.getType().toString() == type && remaining-- == 0)
                return child;

            if (auto found = findNext (child, type, remaining); found.isValid())
                return found;
        }

        return {};
    }

    static DataTree findLet (const DataTree& drawing, const String& name)
    {
        for (const auto& child : drawing)
            if (child.getType().toString() == "let" && child.getProperty ("name").toString() == name)
                return child;

        return {};
    }

    static int count (const DataTree& tree, const String& type)
    {
        int total = 0;

        for (const auto& child : tree)
            total += (child.getType().toString() == type ? 1 : 0) + count (child, type);

        return total;
    }

    static String get (const DataTree& element, const char* property)
    {
        return element.getProperty (property).toString();
    }

    /** Returns the error of compiling a drawing, or an empty text when it compiles. */
    static String compileError (const DataTree& drawing)
    {
        Component component;
        component.setBounds (0.0f, 0.0f, 48.0f, 48.0f);

        const auto style = DrawingStyle::create (component, drawing);
        return style.failed() ? style.getErrorMessage() : String();
    }

    static File getSvgDirectory()
    {
        return File (__FILE__)
            .getParentDirectory()
            .getParentDirectory()
            .getParentDirectory()
            .getChildFile ("examples")
            .getChildFile ("graphics")
            .getChildFile ("data")
            .getChildFile ("svg");
    }

    // One of everything the importer writes
    static constexpr const char* everything = R"svg(
        <defs>
            <linearGradient id="sky"><stop offset="0" stop-color="#ff0000"/><stop offset="1" stop-color="#0000ff"/></linearGradient>
            <linearGradient id="user" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="50" y2="50" spreadMethod="reflect">
                <stop offset="0" stop-color="#ff0000"/><stop offset="0.5" stop-color="#00ff00"/><stop offset="1" stop-color="#0000ff"/>
            </linearGradient>
            <radialGradient id="glow" gradientTransform="scale(1 0.5)"><stop offset="0" stop-color="#ffffff"/><stop offset="1" stop-color="#000000" stop-opacity="0.5"/></radialGradient>
            <pattern id="dots" width="10" height="10" patternUnits="userSpaceOnUse"><circle cx="5" cy="5" r="3" fill="#00ff00"/></pattern>
            <pattern id="boxed" width="0.5" height="0.5" viewBox="0 0 4 4"><rect width="2" height="2"/></pattern>
            <marker id="arrow" markerWidth="6" markerHeight="6" refX="3" refY="3" orient="auto-start-reverse"><path d="M0 0 L6 3 L0 6 Z"/></marker>
            <marker id="dot" markerUnits="userSpaceOnUse" orient="45" viewBox="0 0 2 2"><circle cx="1" cy="1" r="1" fill="currentColor"/></marker>
            <clipPath id="window"><circle cx="50" cy="50" r="40"/><rect width="20" height="20" transform="translate(5 5)"/></clipPath>
            <clipPath id="box" clipPathUnits="objectBoundingBox" clip-path="url(#window)"><rect width="0.5" height="1"/></clipPath>
            <mask id="fade"><rect x="0.1" y="0.1" width="0.8" height="0.8"/></mask>
            <filter id="soft"><feGaussianBlur stdDeviation="2"/><feBlend mode="screen"/></filter>
            <symbol id="badge" viewBox="0 0 10 10"><circle cx="5" cy="5" r="4"/><path d="M2 5 L8 5" stroke="currentColor" marker-end="url(#dot)"/></symbol>
            <path id="tri" d="M0 0 L10 0 L5 10 Z"/>
            <g id="pair"><rect width="4" height="4"/><use href="#tri" x="6"/></g>
        </defs>
        <g id="layer-1" fill="url(#sky)" stroke="#0000ff" stroke-width="2" stroke-dasharray="4 2 1" stroke-linecap="round" stroke-linejoin="bevel" color="#123456">
            <rect x="1" y="2" width="30" height="20" rx="4" ry="2"/>
            <circle cx="50" cy="50" r="10" fill="url(#glow)" stroke="url(#user)"/>
            <ellipse cx="50" cy="50" rx="20" ry="10" fill="url(#dots)" stroke="none" opacity="0.5"/>
            <line x1="0" y1="0" x2="10" y2="20" marker-start="url(#arrow)" marker-end="url(#arrow)"/>
            <polyline points="0,0 10,0 10,10" fill="none" marker-mid="url(#dot)" stroke-dasharray="none"/>
            <path d="M0 0 L10 0 L10 10 Z M2 2 L8 2 L8 8 Z" fill="currentColor" fill-opacity="0.5" fill-rule="evenodd" stroke="currentColor"/>
            <text x="10" y="20" font-size="12" font-family="Arial" text-anchor="end" letter-spacing="1">Hello "you"<tspan dx="4" fill="#ff0000">there</tspan></text>
        </g>
        <rect width="100" height="100" fill="url(#boxed)" clip-path="url(#box)" mask="url(#fade)" filter="url(#soft)" transform="rotate(10 50 50)"/>
        <use href="#badge" x="10" y="20" width="20" height="20" fill="#ff0000" color="#00ff00"/>
        <use href="#badge" x="40" y="20" width="20" height="20" fill="none" stroke="#000000"/>
        <use id="first" href="#tri" x="5"/>
        <use href="#tri" x="25" fill="#00ff00" stroke="#000000"/>
        <use href="#pair" transform="translate(60 60) scale(2)" fill="#0000ff"/>
        <svg x="10" y="10" width="40" height="20" viewBox="0 0 80 40" preserveAspectRatio="xMaxYMin slice"><rect width="80" height="40"/></svg>
        <image href="missing.png" x="0" y="0" width="10" height="10" preserveAspectRatio="none"/>
    )svg";

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

//==============================================================================
TEST_F (YuiSvgImporterTests, FailsForTextThatIsNotAnSvg)
{
    const auto imported = YuiSvgImporter::importSvg (StringRef ("<html></html>"));

    EXPECT_FALSE (imported.wasOk());
    EXPECT_FALSE (imported.drawing.isValid());
    EXPECT_TRUE (imported.toText().isEmpty());
}

TEST_F (YuiSvgImporterTests, WritesTheViewBoxAndThePaintOfTheRoot)
{
    const auto imported = importBody ("<rect width=\"10\" height=\"10\"/>");
    ASSERT_TRUE (imported.wasOk());
    EXPECT_TRUE (imported.warnings.isEmpty());

    EXPECT_EQ (imported.drawing.getType().toString(), "drawing");
    EXPECT_EQ (get (imported.drawing, "name"), "Imported");
    EXPECT_EQ (get (imported.drawing, "viewBox"), "rect (0, 0, 100, 100)");
    EXPECT_FALSE (imported.drawing.hasProperty ("fitting"));

    // Shapes are black unless they say otherwise, which the group around them all sets
    const auto group = find (imported.drawing, "group");
    ASSERT_TRUE (group.isValid());
    EXPECT_EQ (get (group, "fill"), "#000000");

    const auto rect = find (group, "rect");
    ASSERT_TRUE (rect.isValid());
    EXPECT_EQ (get (rect, "bounds"), "rect (0, 0, 10, 10)");
    EXPECT_FALSE (rect.hasProperty ("fill"));
    EXPECT_FALSE (rect.hasProperty ("stroke"));
}

TEST_F (YuiSvgImporterTests, WritesTheNameTheFittingAndARootWithoutFill)
{
    YuiSvgImporter::Options options;
    options.name = "My logo";

    const auto imported = YuiSvgImporter::importSvg (StringRef (makeSvg ("<rect width=\"10\" height=\"10\" stroke=\"#ff0000\"/>",
                                                                          "viewBox=\"0 0 20 10\" preserveAspectRatio=\"xMinYMax slice\" fill=\"none\"")),
                                                     options);
    ASSERT_TRUE (imported.wasOk());

    EXPECT_EQ (get (imported.drawing, "name"), "My_logo");
    EXPECT_EQ (get (imported.drawing, "viewBox"), "rect (0, 0, 20, 10)");
    EXPECT_EQ (get (imported.drawing, "fitting"), "scaleToFill");
    EXPECT_EQ (get (imported.drawing, "justification"), "bottomLeft");

    EXPECT_EQ (get (find (imported.drawing, "group"), "fill"), "none");

    const auto rect = find (imported.drawing, "rect");
    EXPECT_FALSE (rect.hasProperty ("fill"));
    EXPECT_EQ (get (rect, "stroke"), "#ff0000");
    EXPECT_EQ (get (rect, "strokeWidth"), "1");
}

TEST_F (YuiSvgImporterTests, KeepsTheShapesAsTheyAreDeclared)
{
    const auto imported = importBody (R"svg(
        <rect id="card" x="1" y="2" width="30" height="20" rx="4" ry="2"/>
        <circle cx="50" cy="50" r="10"/>
        <ellipse cx="50" cy="50" rx="20" ry="10"/>
        <line x1="0" y1="0" x2="10" y2="20" stroke="#ff0000"/>
        <line x1="0" y1="0" x2="10" y2="20"/>
        <polygon points="0,0 10,0 10,10"/>
        <path d="M0 0 L10 0 L10 10 Z" fill-rule="evenodd" transform="matrix(1 2 3 4 5 6)"/>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    const auto rect = find (imported.drawing, "rect");
    EXPECT_EQ (get (rect, "id"), "card");
    EXPECT_EQ (get (rect, "bounds"), "rect (1, 2, 30, 20)");
    EXPECT_EQ (get (rect, "cornerSize"), "size (4, 2)");

    const auto circle = find (imported.drawing, "circle");
    EXPECT_EQ (get (circle, "center"), "point (50, 50)");
    EXPECT_EQ (get (circle, "radius"), "10");

    EXPECT_EQ (get (find (imported.drawing, "ellipse"), "bounds"), "rect (30, 40, 40, 20)");

    // A line is only stroked, so it has no fill left when it has no stroke
    const auto stroked = find (imported.drawing, "line", 0);
    EXPECT_EQ (get (stroked, "from"), "point (0, 0)");
    EXPECT_EQ (get (stroked, "to"), "point (10, 20)");
    EXPECT_EQ (get (stroked, "stroke"), "#ff0000");
    EXPECT_FALSE (stroked.hasProperty ("fill"));
    EXPECT_EQ (get (find (imported.drawing, "line", 1), "fill"), "none");

    const auto polygon = find (imported.drawing, "path", 0);
    EXPECT_TRUE (get (polygon, "data").startsWith ("\"M 0 0 L 10 0 L 10 10")) << get (polygon, "data");

    const auto path = find (imported.drawing, "path", 1);
    EXPECT_TRUE (get (path, "data").startsWith ("path (\"M 0 0 L 10 0 L 10 10")) << get (path, "data");
    EXPECT_TRUE (get (path, "data").endsWith (".withNonZeroWinding (false)")) << get (path, "data");
    EXPECT_EQ (get (path, "transform"), "transform (1, 2, 3, 4, 5, 6)");
}

TEST_F (YuiSvgImporterTests, GroupsSetThePaintTheirShapesShare)
{
    const auto imported = importBody (R"svg(
        <g id="layer" fill="#ff0000" stroke="#0000ff" stroke-width="2" opacity="0.5" transform="translate(10 20)">
            <rect width="10" height="10"/>
            <rect width="10" height="10" fill="none"/>
            <rect width="10" height="10" stroke="none" fill-opacity="0.5"/>
        </g>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    const auto group = find (find (imported.drawing, "group"), "group");
    ASSERT_TRUE (group.isValid());
    EXPECT_EQ (get (group, "id"), "layer");
    EXPECT_EQ (get (group, "fill"), "#ff0000");
    EXPECT_EQ (get (group, "stroke"), "#0000ff");
    EXPECT_EQ (get (group, "strokeWidth"), "2");
    EXPECT_EQ (get (group, "opacity"), "0.5");
    EXPECT_EQ (get (group, "transform"), "transform.translation (10, 20)");
    ASSERT_EQ (group.getNumChildren(), 3);

    // Only what differs from the group is written on its shapes
    EXPECT_EQ (group.getChild (0).getNumProperties(), 1);

    EXPECT_EQ (get (group.getChild (1), "fill"), "none");
    EXPECT_FALSE (group.getChild (1).hasProperty ("stroke"));

    EXPECT_EQ (get (group.getChild (2), "stroke"), "none");
    EXPECT_TRUE (get (group.getChild (2), "fill").startsWith ("#ff0000"));
    EXPECT_EQ (get (group.getChild (2), "fill").length(), 9);
}

TEST_F (YuiSvgImporterTests, FillOpacityAppliesToTheDefaultFill)
{
    const auto imported = importBody (R"svg(
        <path d="M0 0h50v50h-50z" fill-opacity="0"/>
        <g fill-opacity="0.5"><rect width="10" height="10"/></g>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    EXPECT_EQ (get (find (imported.drawing, "path"), "fill"), "#00000000");
    EXPECT_EQ (get (find (find (imported.drawing, "group"), "group"), "fill"), "#00000080");
    EXPECT_FALSE (find (imported.drawing, "rect").hasProperty ("fill"));
    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, CurrentColorIsWrittenWithTheColorItStandsFor)
{
    const auto imported = importBody (R"svg(
        <g color="#00ff00">
            <rect width="10" height="10" fill="currentColor"/>
            <rect width="10" height="10" fill="currentColor" fill-opacity="0.5"/>
        </g>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    const auto first = find (imported.drawing, "rect", 0);
    EXPECT_EQ (get (first, "color"), "#00ff00");
    EXPECT_EQ (get (first, "fill"), "currentColor");

    EXPECT_EQ (get (find (imported.drawing, "rect", 1), "fill"), "currentColor.withMultipliedAlpha (0.5)");
    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, GradientsAreSharedOrPlacedInTheShapesTheyFill)
{
    const auto imported = importBody (R"svg(
        <defs>
            <linearGradient id="sky"><stop offset="0" stop-color="#ff0000"/><stop offset="1" stop-color="#0000ff"/></linearGradient>
            <linearGradient id="user" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="50" y2="50" spreadMethod="reflect">
                <stop offset="0" stop-color="#ff0000"/><stop offset="0.5" stop-color="#00ff00"/><stop offset="1" stop-color="#0000ff"/>
            </linearGradient>
            <radialGradient id="glow"><stop offset="0" stop-color="#ffffff"/><stop offset="1" stop-color="#000000"/></radialGradient>
        </defs>
        <rect width="40" height="20" fill="url(#sky)"/>
        <rect width="40" height="20" fill="url(#user)"/>
        <rect x="10" y="10" width="40" height="20" fill="url(#glow)"/>
        <g fill="url(#sky)"><rect width="10" height="10"/><rect width="20" height="20"/></g>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    // Along an axis of the bounding box, one gradient is mapped to every shape
    EXPECT_EQ (get (findLet (imported.drawing, "skyGradient"), "value"), "linearGradient (#ff0000, point (0, 0), #0000ff, point (1, 0))");
    EXPECT_EQ (get (find (imported.drawing, "rect", 0), "fillGradient"), "skyGradient.mappedTo (objectBounds)");

    // In user space, the gradient is the same for every shape
    EXPECT_EQ (get (findLet (imported.drawing, "userGradient"), "value"),
               "linearGradient (#ff0000, point (0, 0), #0000ff, point (50, 50)).withStop (#00ff00, 0.5).withSpread (reflect)");
    EXPECT_EQ (get (find (imported.drawing, "rect", 1), "fillGradient"), "userGradient");

    // Otherwise it is placed in its shape the way Drawable does
    EXPECT_EQ (get (find (imported.drawing, "rect", 2), "fillGradient"), "radialGradient (#ffffff, point (30, 20), #000000, point (50, 20))");

    // A group shares the gradient its shapes are mapped to
    const auto group = find (find (imported.drawing, "group"), "group");
    EXPECT_EQ (get (group, "fillGradient"), "skyGradient.mappedTo (objectBounds)");
    EXPECT_FALSE (group.getChild (0).hasProperty ("fillGradient"));
    EXPECT_FALSE (group.getChild (1).hasProperty ("fillGradient"));

    EXPECT_FALSE (find (imported.drawing, "rect", 0).hasProperty ("fill"));
    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, GradientStopsSharingAnOffsetKeepTheirOrder)
{
    const auto imported = importBody (R"svg(
        <defs>
            <linearGradient id="edge" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="10" y2="0">
                <stop offset="0" stop-color="#ff0000"/><stop offset="0.5" stop-color="#ff0000"/><stop offset="0.5" stop-color="#0000ff"/>
                <stop offset="1" stop-color="#0000ff"/><stop offset="1" stop-color="#00ff00"/>
            </linearGradient>
        </defs>
        <rect width="10" height="10" fill="url(#edge)"/>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    // The gradient ends at the first of the stops placed at its end, the last one is added after it
    EXPECT_EQ (get (findLet (imported.drawing, "edgeGradient"), "value"),
               "linearGradient (#ff0000, point (0, 0), #0000ff, point (10, 0)).withStop (#ff0000, 0.5).withStop (#0000ff, 0.5).withStop (#00ff00, 1)");
}

TEST_F (YuiSvgImporterTests, UsedElementsBecomeDrawingsAndSharedPaths)
{
    const auto imported = importBody (R"svg(
        <defs>
            <symbol id="dot" viewBox="0 0 10 10"><circle cx="5" cy="5" r="4"/></symbol>
            <path id="tri" d="M0 0 L10 0 L5 10 Z"/>
        </defs>
        <use href="#dot" x="10" y="20" width="20" height="20" fill="#ff0000"/>
        <use href="#dot" x="40" y="20" width="20" height="20" fill="#00ff00"/>
        <use href="#tri" x="5"/>
        <use href="#tri" x="25" fill="none" stroke="#ff0000"/>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    // A symbol is written once, and painted with the paint of each use
    ASSERT_EQ (count (imported.drawing, "drawing"), 1);

    const auto symbol = find (imported.drawing, "drawing");
    EXPECT_EQ (get (symbol, "name"), "dot");
    EXPECT_EQ (get (symbol, "viewBox"), "rect (0, 0, 10, 10)");
    EXPECT_EQ (find (symbol, "circle").getNumProperties(), 2);

    const auto first = find (imported.drawing, "use", 0);
    EXPECT_EQ (get (first, "drawing"), "dot");
    EXPECT_EQ (get (first, "bounds"), "rect (0, 0, 20, 20)");
    EXPECT_EQ (get (first, "transform"), "transform.translation (10, 20)");
    EXPECT_EQ (get (first, "fill"), "#ff0000");

    const auto second = find (imported.drawing, "use", 1);
    EXPECT_EQ (get (second, "drawing"), "dot");
    EXPECT_EQ (get (second, "fill"), "#00ff00");

    // The geometry of a shape used more than once is a let
    EXPECT_TRUE (get (findLet (imported.drawing, "triPath"), "value").startsWith ("path (\"M 0 0 L 10 0 L 5 10"));

    const auto group = find (imported.drawing, "group");
    const auto filled = find (group, "path", 0);
    EXPECT_EQ (get (filled, "data"), "triPath");
    EXPECT_EQ (get (filled, "transform"), "transform.translation (5, 0)");
    EXPECT_FALSE (filled.hasProperty ("fill"));

    const auto stroked = find (group, "path", 1);
    EXPECT_EQ (get (stroked, "data"), "triPath");
    EXPECT_EQ (get (stroked, "fill"), "none");
    EXPECT_EQ (get (stroked, "stroke"), "#ff0000");

    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, ReferencesToThemselvesAreLeftOut)
{
    const auto imported = importBody (R"svg(
        <g id="loop"><rect width="10" height="10"/><use href="#loop" x="10"/></g>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    // The group is painted once more by its use, which doesn't use it again
    EXPECT_EQ (count (imported.drawing, "drawing"), 1);
    EXPECT_EQ (count (find (imported.drawing, "drawing"), "use"), 0);
    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, ClipPathsAndMasksBecomeClips)
{
    const auto imported = importBody (R"svg(
        <defs>
            <clipPath id="window"><circle cx="50" cy="50" r="40"/><rect width="20" height="20" transform="translate(5 5)"/></clipPath>
            <clipPath id="nothing"></clipPath>
            <mask id="half" maskUnits="userSpaceOnUse"><rect width="50" height="100"/></mask>
        </defs>
        <rect width="100" height="100" clip-path="url(#window)" mask="url(#half)" fill="#ff0000" transform="translate(1 2)"/>
        <rect width="100" height="100" clip-path="url(#nothing)"/>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    // The clips are inside the transform of the element, around what it draws
    const auto group = find (find (imported.drawing, "group"), "group");
    ASSERT_TRUE (group.isValid());
    EXPECT_EQ (get (group, "transform"), "transform.translation (1, 2)");

    const auto clip = find (group, "clip");
    ASSERT_TRUE (clip.isValid());
    ASSERT_EQ (clip.getNumChildren(), 3);

    const auto window = clip.getChild (0);
    EXPECT_EQ (window.getType().toString(), "clipPath");
    ASSERT_EQ (window.getNumChildren(), 2);
    EXPECT_EQ (window.getChild (0).getType().toString(), "path");
    EXPECT_EQ (get (window.getChild (1), "transform"), "transform.translation (5, 5)");

    EXPECT_EQ (clip.getChild (1).getType().toString(), "clipPath");
    EXPECT_EQ (clip.getChild (1).getNumChildren(), 1);

    EXPECT_EQ (clip.getChild (2).getType().toString(), "rect");
    EXPECT_EQ (get (clip.getChild (2), "fill"), "#ff0000");

    // A clip path without shapes doesn't clip
    EXPECT_EQ (count (imported.drawing, "clip"), 1);
    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, MarkersArePaintedWithThePaintOfTheirShape)
{
    const auto imported = importBody (R"svg(
        <defs>
            <marker id="arrow" markerWidth="6" markerHeight="6" refX="3" refY="3" orient="auto"><path d="M0 0 L6 3 L0 6 Z"/></marker>
        </defs>
        <path d="M10 50 L90 50" fill="none" stroke="#0000ff" stroke-width="2" marker-end="url(#arrow)"/>
        <path d="M10 70 L90 70" fill="#ff0000" marker-start="url(#arrow)"/>
        <rect width="10" height="10" marker-start="url(#arrow)"/>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    // Drawable paints a marker with the fill left by its shape, so there is one for each fill
    ASSERT_EQ (count (imported.drawing, "marker"), 2);

    const auto black = find (imported.drawing, "marker", 0);
    EXPECT_EQ (get (black, "name"), "arrow");
    EXPECT_EQ (get (black, "size"), "size (6, 6)");
    EXPECT_EQ (get (black, "ref"), "point (3, 3)");
    EXPECT_FALSE (black.hasProperty ("orient"));
    EXPECT_EQ (get (find (black, "path"), "fill"), "#000000");

    const auto red = find (imported.drawing, "marker", 1);
    EXPECT_EQ (get (red, "name"), "arrow2");
    EXPECT_EQ (get (find (red, "path"), "fill"), "#ff0000");

    const auto group = find (imported.drawing, "group");

    const auto stroked = find (group, "path", 0);
    EXPECT_EQ (get (stroked, "markerEnd"), "arrow");
    EXPECT_EQ (get (stroked, "strokeWidth"), "2");

    // Markers are scaled by a stroke width of 1 on a shape that isn't stroked
    const auto filled = find (group, "path", 1);
    EXPECT_EQ (get (filled, "markerStart"), "arrow2");
    EXPECT_EQ (get (filled, "strokeWidth"), "1");

    // Only paths and lines have markers, so a rectangle with markers is written as a path
    EXPECT_EQ (count (group, "rect"), 0);
    EXPECT_EQ (get (find (group, "path", 2), "markerStart"), "arrow");

    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, PatternsBecomePatternPaints)
{
    const auto imported = importBody (R"svg(
        <defs>
            <pattern id="dots" width="10" height="10" patternUnits="userSpaceOnUse"><circle cx="5" cy="5" r="3" fill="#00ff00"/></pattern>
            <pattern id="boxed" width="0.5" height="0.25" viewBox="0 0 4 4"><rect width="2" height="2"/></pattern>
        </defs>
        <rect width="50" height="50" fill="url(#dots)"/>
        <rect x="10" y="20" width="40" height="80" fill="url(#boxed)"/>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    const auto dots = find (imported.drawing, "pattern", 0);
    EXPECT_EQ (get (dots, "name"), "dots");
    EXPECT_EQ (get (dots, "tile"), "rect (0, 0, 10, 10)");
    EXPECT_EQ (get (find (dots, "circle"), "fill"), "#00ff00");

    // A tile in the units of the bounding box is placed in the shape it fills
    const auto boxed = find (imported.drawing, "pattern", 1);
    EXPECT_EQ (get (boxed, "tile"), "rect (10, 20, 20, 20)");
    EXPECT_EQ (get (boxed, "viewBox"), "rect (0, 0, 4, 4)");
    EXPECT_EQ (get (boxed, "fitting"), "fill");
    EXPECT_EQ (get (find (boxed, "rect"), "fill"), "#000000");

    const auto group = find (imported.drawing, "group");
    EXPECT_EQ (get (find (group, "rect", 0), "fillPattern"), "dots");
    EXPECT_FALSE (find (group, "rect", 0).hasProperty ("fill"));
    EXPECT_EQ (get (find (group, "rect", 1), "fillPattern"), "boxed");

    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, TextIsPlacedFromItsAnchor)
{
    const auto body = "<text x=\"10\" y=\"20\" font-size=\"12\" font-family=\"Arial\" font-weight=\"bold\" text-anchor=\"middle\" fill=\"#ff0000\" stroke=\"#0000ff\">Hello \"you\"</text>";

    const auto imported = importBody (body);
    ASSERT_TRUE (imported.wasOk());

    const auto text = find (imported.drawing, "text");
    ASSERT_TRUE (text.isValid());
    EXPECT_EQ (get (text, "anchor"), "point (10, 20)");
    EXPECT_EQ (get (text, "text"), "\"Hello \\\"you\\\"\"");
    EXPECT_EQ (get (text, "font"), "defaultFont.withHeight (12)");
    EXPECT_EQ (get (text, "textAnchor"), "middle");
    EXPECT_EQ (get (text, "fill"), "#ff0000");
    EXPECT_FALSE (text.hasProperty ("letterSpacing"));

    // Drawable only fills text
    EXPECT_FALSE (text.hasProperty ("stroke"));

    ASSERT_EQ (imported.warnings.size(), 1);
    EXPECT_TRUE (imported.warnings[0].contains ("defaultFont"));
    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);

    YuiSvgImporter::Options options;
    options.fontExpression = [] (StringRef family, float size, int weight, bool italic)
    {
        return String (family) + (weight >= 700 ? "Bold" : "") + (italic ? "Italic" : "") + ".withHeight (" + String (roundToInt (size)) + ")";
    };

    const auto styled = YuiSvgImporter::importSvg (StringRef (makeSvg (body)), options);
    ASSERT_TRUE (styled.wasOk());
    EXPECT_EQ (get (find (styled.drawing, "text"), "font"), "ArialBold.withHeight (12)");
    EXPECT_TRUE (styled.warnings.isEmpty());
}

TEST_F (YuiSvgImporterTests, NestedSvgsBecomeViewports)
{
    const auto imported = importBody (R"svg(
        <svg x="10" y="10" width="40" height="20" viewBox="0 0 80 40" preserveAspectRatio="none"><rect width="80" height="40"/></svg>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    const auto viewport = find (imported.drawing, "viewport");
    ASSERT_TRUE (viewport.isValid());
    EXPECT_EQ (get (viewport, "bounds"), "rect (10, 10, 40, 20)");
    EXPECT_EQ (get (viewport, "viewBox"), "rect (0, 0, 80, 40)");
    EXPECT_EQ (get (viewport, "fitting"), "fill");
    EXPECT_FALSE (viewport.hasProperty ("justification"));
    EXPECT_EQ (get (find (viewport, "rect"), "bounds"), "rect (0, 0, 80, 40)");

    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, FiltersBecomeLayersDrawnThroughAFilter)
{
    const auto imported = importBody (R"svg(
        <defs>
            <filter id="soft" color-interpolation-filters="sRGB"><feGaussianBlur stdDeviation="3"/></filter>
            <filter id="ring" x="0" y="0" width="1" height="1">
                <feMorphology in="SourceAlpha" radius="2" result="thin-1"/>
                <feComposite in="SourceGraphic" in2="thin-1" operator="out"/>
                <feFlood flood-color="#00ff00" flood-opacity="0.5" result="green"/>
                <feMerge><feMergeNode in="green"/><feMergeNode in="SourceGraphic"/><feMergeNode in="thin-1"/></feMerge>
                <feColorMatrix type="saturate" values="0"/>
                <feDiffuseLighting surfaceScale="2"><feDistantLight azimuth="90" elevation="45"/></feDiffuseLighting>
                <feDisplacementMap in2="green" scale="4" xChannelSelector="R"/>
                <feBlend in2="SourceGraphic" mode="multiply"/>
                <feOffset dx="1" dy="2"/>
            </filter>
        </defs>
        <rect x="10" y="10" width="20" height="10" filter="url(#soft)" opacity="0.5" mix-blend-mode="multiply"/>
        <g filter="url(#ring)"><rect width="10" height="10"/><rect x="10" width="10" height="20"/></g>
        <circle cx="50" cy="50" r="10" style="filter: drop-shadow(2px 3px 1px rgba(0, 0, 0, 0.5))"/>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    const auto expectBounds = [] (const DataTree& filter, float x, float y, float width, float height)
    {
        const auto text = get (filter, "bounds");
        const auto numbers = StringArray::fromTokens (text.fromFirstOccurrenceOf ("(", false, false).upToLastOccurrenceOf (")", false, false), ",", "");
        ASSERT_EQ (numbers.size(), 4) << text;

        EXPECT_NEAR (numbers[0].getFloatValue(), x, 0.01f) << text;
        EXPECT_NEAR (numbers[1].getFloatValue(), y, 0.01f) << text;
        EXPECT_NEAR (numbers[2].getFloatValue(), width, 0.01f) << text;
        EXPECT_NEAR (numbers[3].getFloatValue(), height, 0.01f) << text;
    };

    // What the filter makes of the element gets its opacity and its blend mode, from the group around the layer
    const auto blurred = find (imported.drawing, "layer", 0);
    ASSERT_TRUE (blurred.isValid()) << imported.toText();
    EXPECT_EQ (blurred.getParent().getType().toString(), "group");
    EXPECT_EQ (get (blurred.getParent(), "opacity"), "0.5");
    EXPECT_EQ (get (blurred.getParent(), "blend"), "multiply");
    EXPECT_FALSE (blurred.hasProperty ("opacity"));
    EXPECT_FALSE (find (blurred, "rect").hasProperty ("feather"));

    // A tenth of the bounding box around it, unless the filter says where it is rendered
    const auto soft = blurred.getChildWithName ("filter");
    ASSERT_TRUE (soft.isValid());
    expectBounds (soft, 8.0f, 9.0f, 24.0f, 12.0f);
    EXPECT_FALSE (soft.hasProperty ("linearColors"));
    ASSERT_EQ (soft.getNumChildren(), 1);
    EXPECT_EQ (soft.getChild (0).getType().toString(), "blur");
    EXPECT_EQ (get (soft.getChild (0), "deviation"), "3");

    // The filter of a group is given everything in it, over the box of it all
    const auto ring = find (imported.drawing, "layer", 1).getChildWithName ("filter");
    ASSERT_TRUE (ring.isValid()) << imported.toText();
    expectBounds (ring, 0.0f, 0.0f, 20.0f, 20.0f);
    EXPECT_EQ (get (ring, "linearColors"), "true");
    EXPECT_EQ (count (find (imported.drawing, "layer", 1), "rect"), 2);

    StringArray types;
    for (const auto& primitive : ring)
        types.add (primitive.getType().toString());

    EXPECT_EQ (types.joinIntoString (" "), "morphology composite flood composite composite colorMatrix lighting displacement blend offset");

    EXPECT_EQ (get (ring.getChild (0), "in"), "sourceAlpha");
    EXPECT_EQ (get (ring.getChild (0), "radius"), "2");
    EXPECT_EQ (get (ring.getChild (0), "result"), "thin_1");
    EXPECT_EQ (get (ring.getChild (1), "in"), "source");
    EXPECT_EQ (get (ring.getChild (1), "in2"), "thin_1");
    EXPECT_EQ (get (ring.getChild (1), "operator"), "out");
    EXPECT_TRUE (get (ring.getChild (2), "color").startsWith ("#00ff00"));
    EXPECT_EQ (get (ring.getChild (2), "color").length(), 9);

    // A merge puts its pictures over each other, one at a time
    EXPECT_EQ (get (ring.getChild (3), "in"), "source");
    EXPECT_EQ (get (ring.getChild (3), "in2"), "green");
    EXPECT_EQ (get (ring.getChild (4), "in"), "thin_1");
    EXPECT_FALSE (ring.getChild (4).hasProperty ("in2"));

    EXPECT_TRUE (get (ring.getChild (5), "matrix").startsWith ("\"0.213 0.715 0.072 0 0 "));
    EXPECT_EQ (get (ring.getChild (6), "scale"), "2");
    EXPECT_TRUE (ring.getChild (6).hasProperty ("azimuth"));
    EXPECT_EQ (get (ring.getChild (7), "xChannel"), "red");
    EXPECT_EQ (get (ring.getChild (7), "yChannel"), "alpha");
    EXPECT_EQ (get (ring.getChild (8), "mode"), "multiply");
    EXPECT_EQ (get (ring.getChild (9), "offset"), "point (1, 2)");

    // The functions of CSS have the room they need around the element
    const auto shadow = find (imported.drawing, "layer", 2).getChildWithName ("filter");
    ASSERT_TRUE (shadow.isValid()) << imported.toText();
    expectBounds (shadow, 34.0f, 34.0f, 32.0f, 32.0f);
    ASSERT_EQ (shadow.getNumChildren(), 1);
    EXPECT_EQ (shadow.getChild (0).getType().toString(), "dropShadow");
    EXPECT_EQ (get (shadow.getChild (0), "offset"), "point (2, 3)");
    EXPECT_EQ (get (shadow.getChild (0), "deviation"), "1");
    EXPECT_TRUE (get (shadow.getChild (0), "color").startsWith ("#000000"));
    EXPECT_EQ (get (shadow.getChild (0), "color").length(), 9);

    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing) << "\n"
                                                           << imported.toText();
}

TEST_F (YuiSvgImporterTests, FiltersBlendingWithTheBackgroundSetTheBlendOfTheElement)
{
    const auto imported = importBody (R"svg(
        <defs>
            <filter id="multiply"><feBlend mode="multiply" in="SourceGraphic" in2="BackgroundImage"/></filter>
            <filter id="soft"><feGaussianBlur stdDeviation="2"/><feBlend mode="screen" in2="BackgroundImage"/></filter>
        </defs>
        <text x="10" y="20" font-size="12" filter="url(#multiply)">Multiply</text>
        <rect width="10" height="10" filter="url(#soft)"/>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    // The blend is set by a group around what it applies to
    const auto blendAround = [] (DataTree element)
    {
        for (; element.isValid(); element = element.getParent())
            if (element.hasProperty ("blend"))
                return get (element, "blend");

        return String();
    };

    // Nothing is left for a filter to do: the text is drawn as it is, blended
    const auto text = find (imported.drawing, "text");
    ASSERT_TRUE (text.isValid()) << imported.toText();
    EXPECT_EQ (blendAround (text), "multiply") << imported.toText();
    EXPECT_EQ (count (imported.drawing, "layer"), 1) << imported.toText();

    // What the filter makes before the blend is blended too
    const auto layer = find (imported.drawing, "layer");
    ASSERT_TRUE (layer.isValid());
    EXPECT_EQ (blendAround (layer), "screen") << imported.toText();

    const auto filter = layer.getChildWithName ("filter");
    ASSERT_EQ (filter.getNumChildren(), 1);
    EXPECT_EQ (filter.getChild (0).getType().toString(), "blur");

    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing) << "\n"
                                                           << imported.toText();
}

TEST_F (YuiSvgImporterTests, WarnsAboutImagesADrawingCanNotLoad)
{
    const auto href = "https://example.com/pictures/" + String::repeatedString ("a", 80) + ".png";

    SVGDocument::ParseOptions parseOptions;
    parseOptions.imageResolver = [] (StringRef, const File&) -> std::optional<Image>
    {
        Image image (2, 2);
        image.fillColor (Colors::red);
        return image;
    };

    const auto body = "<image href=\"" + href + "\" width=\"10\" height=\"10\"/>";

    // The SVG has it, but the drawing won't find it as a file
    const auto resolved = YuiSvgImporter::importSvg (StringRef (makeSvg (body)), {}, parseOptions);
    ASSERT_TRUE (resolved.wasOk());
    EXPECT_EQ (get (find (resolved.drawing, "image"), "source"), "\"" + href + "\"");
    ASSERT_EQ (resolved.warnings.size(), 1);
    EXPECT_TRUE (resolved.warnings[0].contains ("DrawingStyle::setImageResolver")) << resolved.warnings[0];

    // The message tells which image, in full unless it goes on for too long
    const auto missing = importBody (body);
    ASSERT_TRUE (missing.wasOk());
    ASSERT_EQ (missing.warnings.size(), 1);
    EXPECT_TRUE (missing.warnings[0].contains ("could not be loaded")) << missing.warnings[0];
    EXPECT_TRUE (missing.warnings[0].contains ("https://example.com/pictures/aaaa")) << missing.warnings[0];
    EXPECT_TRUE (missing.warnings[0].contains ("...'")) << missing.warnings[0];
    EXPECT_FALSE (missing.warnings[0].contains (".png")) << missing.warnings[0];

    const auto shortBody = String ("<image href=\"https://example.com/rock.jpg\" width=\"10\" height=\"10\"/>");
    const auto shortName = importBody (shortBody);
    ASSERT_TRUE (shortName.wasOk());
    ASSERT_EQ (shortName.warnings.size(), 1);
    EXPECT_TRUE (shortName.warnings[0].contains ("'https://example.com/rock.jpg'")) << shortName.warnings[0];
}

TEST_F (YuiSvgImporterTests, StrokesKeepTheirDashesCapsAndJoins)
{
    const auto imported = importBody (R"svg(
        <path d="M0 0 L50 0" fill="none" stroke="#000000" stroke-dasharray="5 3 2" stroke-dashoffset="1" stroke-linecap="round" stroke-linejoin="bevel" stroke-miterlimit="10"/>
        <path d="M0 10 L50 10" fill="none" stroke="#000000" stroke-dasharray="4 2"/>
    )svg");
    ASSERT_TRUE (imported.wasOk());

    const auto first = find (imported.drawing, "path", 0);
    EXPECT_EQ (get (first, "fill"), "none");
    EXPECT_EQ (get (first, "stroke"), "#000000");
    EXPECT_EQ (get (first, "strokeWidth"), "1");
    EXPECT_EQ (get (first, "dash"), "\"5 3 2\"");
    EXPECT_EQ (get (first, "dashOffset"), "1");
    EXPECT_EQ (get (first, "cap"), "round");
    EXPECT_EQ (get (first, "join"), "bevel");
    EXPECT_EQ (get (first, "strokeMiterLimit"), "10");

    const auto second = find (imported.drawing, "path", 1);
    EXPECT_EQ (get (second, "dash"), "dash (4, 2)");
    EXPECT_FALSE (second.hasProperty ("dashOffset"));
    EXPECT_FALSE (second.hasProperty ("strokeMiterLimit"));

    EXPECT_TRUE (compileError (imported.drawing).isEmpty()) << compileError (imported.drawing);
}

TEST_F (YuiSvgImporterTests, EverythingItWritesCompilesAndIsReadBackFromItsText)
{
    const auto imported = importBody (everything);
    ASSERT_TRUE (imported.wasOk());

    Component component;
    component.setBounds (0.0f, 0.0f, 48.0f, 48.0f);

    auto style = DrawingStyle::create (component, imported.drawing);
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage() << "\n"
                                << imported.toText();

    auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
    auto renderer = context->makeRenderer (48, 48);
    Graphics g (*context, *renderer);
    EXPECT_NO_THROW (style.getReference()->paint (g, *theme, component));

    // The text of the drawing is the drawing again
    const auto parsed = YuiParser::parse (imported.toText());
    ASSERT_TRUE (parsed.wasOk()) << YuiDiagnostic::join (parsed.diagnostics) << "\n"
                                 << imported.toText();

    const auto drawing = parsed.document.getChildWithName ("drawing");
    ASSERT_TRUE (drawing.isValid());
    EXPECT_TRUE (drawing.isEquivalentTo (imported.drawing)) << imported.toText();
    EXPECT_EQ (YuiWriter::write (drawing), imported.toText());
}

TEST_F (YuiSvgImporterTests, ColorsBecomePropertiesWhenAsked)
{
    YuiSvgImporter::Options options;
    options.colorProperties = true;

    const auto imported = YuiSvgImporter::importSvg (StringRef (makeSvg (R"svg(
        <defs>
            <linearGradient id="sky" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="10" y2="0">
                <stop offset="0" stop-color="#ff0000"/><stop offset="1" stop-color="#0000ff"/>
            </linearGradient>
        </defs>
        <rect width="10" height="10" fill="#ff0000"/>
        <rect width="10" height="10" fill="url(#sky)" stroke="#00ff00"/>
        <text x="1" y="12" font-size="10">#ff0000</text>
    )svg")),
                                                     options);
    ASSERT_TRUE (imported.wasOk());

    // One property for each color, in the order they are met, before everything else
    const StringArray colors { "#ff0000", "#0000ff", "#000000", "#00ff00" };
    ASSERT_EQ (count (imported.drawing, "property"), colors.size()) << imported.toText();

    for (int i = 0; i < colors.size(); ++i)
    {
        const auto property = imported.drawing.getChild (i);
        EXPECT_EQ (property.getType().toString(), "property");
        EXPECT_EQ (get (property, "name"), "color" + String (i + 1));
        EXPECT_EQ (get (property, "type"), "color");
        EXPECT_EQ (get (property, "value"), colors[i]);
    }

    EXPECT_EQ (get (findLet (imported.drawing, "skyGradient"), "value"), "linearGradient (color1, point (0, 0), color2, point (10, 0))");
    EXPECT_EQ (get (find (imported.drawing, "group"), "fill"), "color3");
    EXPECT_EQ (get (find (imported.drawing, "rect", 0), "fill"), "color1");
    EXPECT_EQ (get (find (imported.drawing, "rect", 1), "stroke"), "color4");

    // What a text says is not a color
    EXPECT_EQ (get (find (imported.drawing, "text"), "text"), "\"#ff0000\"");

    Component component;
    component.setBounds (0.0f, 0.0f, 48.0f, 48.0f);

    const auto style = DrawingStyle::create (component, imported.drawing);
    ASSERT_TRUE (style.wasOk()) << style.getErrorMessage() << "\n"
                                << imported.toText();

    const auto& properties = style.getReference()->getProperties();
    ASSERT_EQ (static_cast<int> (properties.size()), colors.size());
    EXPECT_EQ (properties[0].name.toString(), "color1");
    EXPECT_TRUE (properties[0].type == ComponentProperty::Type::color);

    const auto parsed = YuiParser::parse (imported.toText());
    ASSERT_TRUE (parsed.wasOk()) << YuiDiagnostic::join (parsed.diagnostics) << "\n"
                                 << imported.toText();
    EXPECT_TRUE (parsed.document.getChildWithName ("drawing").isEquivalentTo (imported.drawing)) << imported.toText();
}

TEST_F (YuiSvgImporterTests, ColorPropertiesStandForEveryColorItWrites)
{
    YuiSvgImporter::Options options;
    options.colorProperties = true;

    const auto imported = YuiSvgImporter::importSvg (StringRef (makeSvg (everything)), options);
    ASSERT_TRUE (imported.wasOk());
    EXPECT_GT (count (imported.drawing, "property"), 0);

    for (const auto& line : StringArray::fromLines (imported.toText()))
        if (! line.trimStart().startsWith ("property "))
            EXPECT_FALSE (line.containsChar ('#')) << line;

    // Drawings, patterns and markers read the properties of the drawing they are in
    const auto error = compileError (imported.drawing);
    EXPECT_TRUE (error.isEmpty()) << error << "\n"
                                  << imported.toText();
}

TEST_F (YuiSvgImporterTests, TheExampleSvgsCompile)
{
    const auto directory = getSvgDirectory();
    if (! directory.isDirectory())
        GTEST_SKIP() << "The example SVGs are not available";

    int numImported = 0;

    for (const auto& file : directory.findChildFiles (File::findFiles, false, "*.svg"))
    {
        // The largest ones only take longer
        if (file.getSize() > 100 * 1024)
            continue;

        // What the parser can't read is not for the importer to write
        const auto imported = YuiSvgImporter::importSvg (file);
        if (! imported.wasOk())
            continue;

        const auto error = compileError (imported.drawing);
        EXPECT_TRUE (error.isEmpty()) << file.getFileName() << ": " << error;

        ++numImported;
    }

    EXPECT_GT (numImported, 0);
}

//==============================================================================
class YuiSvgImporterParityTests : public YuiSvgImporterTests
{
protected:
    static constexpr int size = 64;

    /** Paints an SVG through a Drawable. */
    struct PaintedSvg : Component
    {
        PaintedSvg()
        {
            setOpaque (false);
            setBounds (0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size));
        }

        void paint (Graphics& g) override
        {
            drawable.paint (g, getLocalBounds(), Fitting::scaleToFit, Justification::center);
        }

        Drawable drawable;
    };

    /** Paints the drawing an SVG was imported as. */
    struct PaintedDrawing : Component
    {
        explicit PaintedDrawing (const DataTree& drawing)
        {
            setOpaque (false);
            setBounds (0.0f, 0.0f, static_cast<float> (size), static_cast<float> (size));

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

        if (GpuCanvas::create (*gpuContext, size, size) == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        YuiSvgImporterTests::SetUp();

        if (gpuContext == nullptr)
            GTEST_SKIP() << "No Metal GPU context available";
    }

    /** Text is drawn with the default font of the theme on both sides. */
    static SVGDocument::ParseOptions makeParseOptions (const File& baseDirectory = {})
    {
        SVGDocument::ParseOptions options;
        options.baseDirectory = baseDirectory;
        options.fontResolver = [] (StringRef, float, int, bool) -> std::optional<Font>
        {
            return ApplicationTheme::getGlobalTheme()->getDefaultFont();
        };

        return options;
    }

    /** Expects the drawing an SVG is imported as to paint what a Drawable paints for it. */
    static void expectSamePicture (const String& svgText, const String& name, const File& baseDirectory = {})
    {
        const auto options = makeParseOptions (baseDirectory);

        PaintedSvg expected;
        ASSERT_TRUE (expected.drawable.parseSVG (StringRef (svgText), options)) << name;

        const auto imported = YuiSvgImporter::importSvg (StringRef (svgText), {}, options);
        ASSERT_TRUE (imported.wasOk()) << name;

        PaintedDrawing actual (imported.drawing);

        const auto expectedImage = expected.snapshotToImage (*gpuContext);
        const auto actualImage = actual.snapshotToImage (*gpuContext);
        ASSERT_TRUE (expectedImage.isValid() && actualImage.isValid()) << name;

        int numDifferent = 0;
        int numPainted = 0;
        int largest = 0;

        for (int y = 0; y < size; ++y)
        {
            for (int x = 0; x < size; ++x)
            {
                const auto a = expectedImage.getPixelColor (x, y);
                const auto b = actualImage.getPixelColor (x, y);

                const auto difference = jmax (std::abs (a.getRed() - b.getRed()),
                                              std::abs (a.getGreen() - b.getGreen()),
                                              std::abs (a.getBlue() - b.getBlue()),
                                              std::abs (a.getAlpha() - b.getAlpha()));

                largest = jmax (largest, difference);
                numDifferent += difference > tolerance ? 1 : 0;
                numPainted += a.getAlpha() > 0 ? 1 : 0;
            }
        }

        // Anything painted at all, and the same but for the edges of a few shapes
        EXPECT_GT (numPainted, 0) << name << " paints nothing";
        EXPECT_LE (numDifferent, size * size / 100) << name << ": " << numDifferent << " pixels differ, by up to " << largest << "\n"
                                                    << imported.toText();
    }

    static void expectSamePictureForBody (const String& body, const String& name)
    {
        expectSamePicture (makeSvg (body), name);
    }

    static constexpr int tolerance = 24;
    static std::unique_ptr<GraphicsContext> gpuContext;
};

std::unique_ptr<GraphicsContext> YuiSvgImporterParityTests::gpuContext;

TEST_F (YuiSvgImporterParityTests, ShapesAndGroupsPaintWhatDrawablePaints)
{
    expectSamePictureForBody (R"svg(
        <rect x="5" y="5" width="40" height="30" rx="8" ry="4"/>
        <g fill="#ff0000" stroke="#0000ff" stroke-width="3" transform="translate(10 20) rotate(15)">
            <circle cx="50" cy="40" r="15"/>
            <ellipse cx="40" cy="60" rx="20" ry="8" fill="none" stroke-dasharray="6 3" stroke-linecap="round"/>
            <polygon points="60,10 80,10 70,30" stroke="none" fill-opacity="0.5"/>
        </g>
        <line x1="5" y1="95" x2="95" y2="80" stroke="#00aa00" stroke-width="4"/>
        <path d="M60 50 L95 50 L95 75 L60 75 Z M68 56 L88 56 L88 70 L68 70 Z" fill-rule="evenodd" fill="#884400" opacity="0.6"/>
    )svg", "shapes");
}

TEST_F (YuiSvgImporterParityTests, GradientsPaintWhatDrawablePaints)
{
    expectSamePictureForBody (R"svg(
        <defs>
            <linearGradient id="a"><stop offset="0" stop-color="#ff0000"/><stop offset="1" stop-color="#0000ff"/></linearGradient>
            <linearGradient id="b" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ffff00"/><stop offset="0.5" stop-color="#00ff00"/><stop offset="1" stop-color="#000000"/></linearGradient>
            <linearGradient id="c" gradientUnits="userSpaceOnUse" x1="50" y1="50" x2="60" y2="50" spreadMethod="reflect"><stop offset="0" stop-color="#ffffff"/><stop offset="1" stop-color="#ff00ff"/></linearGradient>
            <radialGradient id="d" fx="0.3" fy="0.3"><stop offset="0" stop-color="#ffffff"/><stop offset="1" stop-color="#004488"/></radialGradient>
            <radialGradient id="e" gradientTransform="scale(1 0.5)" spreadMethod="repeat" r="0.2"><stop offset="0" stop-color="#ff8800"/><stop offset="1" stop-color="#002200"/></radialGradient>
        </defs>
        <rect x="0" y="0" width="50" height="30" fill="url(#a)"/>
        <rect x="50" y="0" width="50" height="30" fill="url(#b)"/>
        <rect x="0" y="30" width="100" height="20" fill="url(#c)"/>
        <rect x="0" y="50" width="50" height="50" fill="url(#d)" stroke="url(#a)" stroke-width="6"/>
        <rect x="50" y="50" width="50" height="50" fill="url(#e)"/>
    )svg", "gradients");
}

TEST_F (YuiSvgImporterParityTests, UsedElementsPaintWhatDrawablePaints)
{
    expectSamePictureForBody (R"svg(
        <defs>
            <symbol id="badge" viewBox="0 0 10 10"><circle cx="5" cy="5" r="4"/><rect x="4" y="0" width="2" height="10" fill="#ffffff"/></symbol>
            <path id="tri" d="M0 0 L20 0 L10 20 Z" fill="#0000ff"/>
            <g id="pair"><rect width="10" height="10"/><circle cx="20" cy="5" r="5" stroke="#000000"/></g>
        </defs>
        <use href="#badge" x="5" y="5" width="30" height="30" fill="#ff0000"/>
        <use href="#badge" x="40" y="5" width="20" height="40" fill="#00aa00"/>
        <use href="#tri" x="70" y="5"/>
        <use href="#tri" x="70" y="35" fill="none" stroke="#ff0000" stroke-width="2"/>
        <use href="#pair" transform="translate(10 60) scale(2)" fill="#884400"/>
    )svg", "use");
}

TEST_F (YuiSvgImporterParityTests, ClipsMarkersAndViewportsPaintWhatDrawablePaints)
{
    expectSamePictureForBody (R"svg(
        <defs>
            <clipPath id="window"><circle cx="25" cy="25" r="20"/><rect x="25" y="25" width="25" height="25"/></clipPath>
            <mask id="half" maskUnits="userSpaceOnUse"><rect x="50" y="0" width="25" height="50"/></mask>
            <marker id="arrow" markerWidth="4" markerHeight="4" refX="2" refY="2" orient="auto" viewBox="0 0 4 4"><path d="M0 0 L4 2 L0 4 Z"/></marker>
        </defs>
        <rect width="50" height="50" fill="#ff0000" clip-path="url(#window)"/>
        <rect x="50" width="50" height="50" fill="#0000ff" mask="url(#half)"/>
        <path d="M10 70 L50 60 L90 80" fill="none" stroke="#008800" stroke-width="3" marker-start="url(#arrow)" marker-mid="url(#arrow)" marker-end="url(#arrow)"/>
        <svg x="60" y="55" width="35" height="15" viewBox="0 0 10 10" preserveAspectRatio="xMinYMid slice"><circle cx="5" cy="5" r="5" fill="#aa00aa"/></svg>
    )svg", "clips");
}

TEST_F (YuiSvgImporterParityTests, FiltersPaintWhatDrawablePaints)
{
    expectSamePictureForBody (R"svg(
        <defs>
            <filter id="soft" x="-0.5" y="-0.5" width="2" height="2"><feGaussianBlur stdDeviation="3"/></filter>
            <filter id="edge" color-interpolation-filters="sRGB">
                <feMorphology in="SourceAlpha" radius="3" result="thin"/>
                <feComposite in="SourceGraphic" in2="thin" operator="out" result="ring"/>
                <feFlood flood-color="#0000ff"/>
                <feComposite in2="ring" operator="in"/>
                <feBlend in2="SourceGraphic" mode="screen"/>
            </filter>
        </defs>
        <rect x="8" y="8" width="30" height="30" fill="#ff0000" filter="url(#soft)" opacity="0.8"/>
        <g filter="url(#edge)" transform="translate(50 10)"><circle cx="20" cy="20" r="18" fill="#00aa00"/><rect x="10" y="30" width="20" height="20" fill="#884400"/></g>
        <circle cx="30" cy="70" r="15" fill="#ffcc00" style="filter: drop-shadow(6px 6px 2px rgba(0, 0, 0, 0.6))"/>
    )svg", "filters");
}

TEST_F (YuiSvgImporterParityTests, TheExampleSvgsPaintWhatDrawablePaints)
{
    const auto directory = getSvgDirectory();
    if (! directory.isDirectory())
        GTEST_SKIP() << "The example SVGs are not available";

    for (const auto* name : { "hearth_use.svg", "clippath.svg", "lineargradient1.svg", "lineargradient3.svg", "radialgradient1.svg", "radialgradient2.svg",
                              "shapes-polygon.svg", "shapes-polyline.svg", "circles1.svg", "opacity_blend.svg", "preserveAspectRatio.svg",
                              "vector_example.svg", "patch.svg", "android.svg", "python.svg", "tiger.svg" })
    {
        const auto file = directory.getChildFile (name);
        ASSERT_TRUE (file.existsAsFile()) << name;

        expectSamePicture (file.loadFileAsString(), name, directory);
    }
}

TEST_F (YuiSvgImporterParityTests, ColorPropertiesRecolorTheDrawing)
{
    YuiSvgImporter::Options options;
    options.colorProperties = true;

    const auto imported = YuiSvgImporter::importSvg (StringRef (makeSvg ("<rect width=\"100\" height=\"100\" fill=\"#ff0000\"/>")), options);
    ASSERT_TRUE (imported.wasOk());

    Identifier red;

    for (const auto& child : imported.drawing)
        if (child.getType().toString() == "property" && get (child, "value") == "#ff0000")
            red = Identifier (get (child, "name"));

    ASSERT_TRUE (red.isValid()) << imported.toText();

    PaintedDrawing drawing (imported.drawing);

    const auto before = drawing.snapshotToImage (*gpuContext).getPixelColor (size / 2, size / 2);
    EXPECT_GT (before.getRed(), 200);
    EXPECT_LT (before.getBlue(), 50);

    drawing.setProperty (red, VariantConverter<Color>::toVar (Color (0xff0000ff)));

    const auto after = drawing.snapshotToImage (*gpuContext).getPixelColor (size / 2, size / 2);
    EXPECT_LT (after.getRed(), 50);
    EXPECT_GT (after.getBlue(), 200);
}
