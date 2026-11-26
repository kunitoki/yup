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

//==============================================================================
/** Widgets painted by the drawings of the documentation: what a drawing reads and places of them is what they tell. */
class GuiMarkupModuleIntegrationTests : public ::testing::Test
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

    /** Returns the text of a figure of the documentation, empty where the sources are not at hand. */
    static String readFigure (const String& name)
    {
        const auto file = File (__FILE__)
                              .getParentDirectory()
                              .getParentDirectory()
                              .getParentDirectory()
                              .getChildFile ("examples/docsimg/figures/ui/yui")
                              .getChildFile (name);

        return file.existsAsFile() ? file.loadFileAsString() : String();
    }

    /** Loads a figure in a document of a size, and styles it. Returns false when there is no figure to load. */
    bool loadFigure (YuiDocument& document, const String& name, float width, float height)
    {
        const auto text = readFigure (name);
        if (text.isEmpty())
            return false;

        const auto loaded = document.loadFromString (text);
        EXPECT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

        if (auto* root = document.getRoot())
        {
            root->setBounds (0.0f, 0.0f, width, height);
            root->getStyleContext()->restyleNow();
        }

        EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
        return loaded.wasOk() && document.getRoot() != nullptr;
    }

    /** Returns the drawing a component is painted by, having painted it once. */
    DrawingStyle* paintWithItsDrawing (Component* component)
    {
        if (component == nullptr)
        {
            ADD_FAILURE() << "The figure has no such component";
            return nullptr;
        }

        auto* style = dynamic_cast<DrawingStyle*> (component->getStyle().get());
        if (style == nullptr)
        {
            ADD_FAILURE() << "The component is not painted by a drawing";
            return nullptr;
        }

        auto context = GraphicsContext::createContext (GpuPlatform::Headless, {});
        auto renderer = context->makeRenderer (jmax (1, roundToInt (component->getWidth())), jmax (1, roundToInt (component->getHeight())));
        Graphics g (*context, *renderer);

        style->paint (g, *theme, *component);
        return style;
    }

    /** Returns where the element of an id lands, as a drawing measures it for a component. */
    static DrawingStyle::ElementBounds measure (DrawingStyle& style, const Component& component, const String& id)
    {
        for (const auto& element : style.measureElements (component))
            if (element.id == id)
                return element;

        ADD_FAILURE() << "No element '" << id << "' was measured";
        return {};
    }

    /** Expects an element where a widget tells one of its areas is, to less than what shows. */
    static void expectSameArea (const Rectangle<float>& measured, const Rectangle<float>& expected)
    {
        EXPECT_NEAR (measured.getX(), expected.getX(), 0.01f);
        EXPECT_NEAR (measured.getY(), expected.getY(), 0.01f);
        EXPECT_NEAR (measured.getWidth(), expected.getWidth(), 0.01f);
        EXPECT_NEAR (measured.getHeight(), expected.getHeight(), 0.01f);
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

//==============================================================================
TEST_F (GuiMarkupModuleIntegrationTests, ADrawingJoinsTheTurnsOfARepeatIntoCurves)
{
    YuiDocument document;
    if (! loadFigure (document, "drawing_vertex.yui", 560.0f, 150.0f))
        GTEST_SKIP() << "The figures of the documentation are not available";

    auto* curves = document.findById ("curves");
    auto* style = paintWithItsDrawing (curves);
    ASSERT_NE (style, nullptr);

    // One vertex for each turn, joined into an area reaching its base, a band and a line
    const auto area = measure (*style, *curves, "area");
    EXPECT_EQ (area.type, String ("vertex"));
    EXPECT_EQ (area.numRows, 57);
    EXPECT_FLOAT_EQ (area.bounds.getX(), 0.0f);
    EXPECT_NEAR (area.bounds.getRight(), 560.0f, 0.01f);
    EXPECT_NEAR (area.bounds.getBottom(), 150.0f, 0.01f);

    const auto band = measure (*style, *curves, "band");
    const auto line = measure (*style, *curves, "line");
    EXPECT_GT (band.bounds.getHeight(), 16.0f);
    EXPECT_TRUE (band.bounds.enlarged (1.0f).contains (line.bounds));
}

TEST_F (GuiMarkupModuleIntegrationTests, ADrawingPlacesThePartsOfTheWidgetsItPaints)
{
    YuiDocument document;
    if (! loadFigure (document, "drawing_parts.yui", 560.0f, 72.0f))
        GTEST_SKIP() << "The figures of the documentation are not available";

    auto* field = document.findById<TextEditor> ("field");
    auto* apply = document.findById<TextButton> ("apply");

    auto* fieldStyle = paintWithItsDrawing (field);
    auto* applyStyle = paintWithItsDrawing (apply);
    ASSERT_NE (fieldStyle, nullptr);
    ASSERT_NE (applyStyle, nullptr);

    // The text is where each widget laid it out
    const auto fieldText = measure (*fieldStyle, *field, "label");
    EXPECT_EQ (fieldText.type, String ("part"));
    expectSameArea (fieldText.bounds, field->getTextBounds());

    expectSameArea (measure (*applyStyle, *apply, "label").bounds, apply->getTextBounds());
    expectSameArea (measure (*applyStyle, *apply, "frame").bounds, apply->getLocalBounds().reduced (1.0f));
}

TEST_F (GuiMarkupModuleIntegrationTests, DrawingsPaintTheControlsFromTheGeometryTheyTell)
{
    YuiDocument document;
    if (! loadFigure (document, "widgets_controls.yui", 560.0f, 64.0f))
        GTEST_SKIP() << "The figures of the documentation are not available";

    auto* fader = document.findById<Slider> ("fader");
    auto* stepper = document.findById<Slider> ("stepper");
    auto* toggle = document.findById<SwitchButton> ("toggle");
    auto* scroller = document.findById<ScrollBar> ("scroller");

    auto* faderStyle = paintWithItsDrawing (fader);
    auto* stepperStyle = paintWithItsDrawing (stepper);
    auto* toggleStyle = paintWithItsDrawing (toggle);
    auto* scrollerStyle = paintWithItsDrawing (scroller);
    ASSERT_NE (faderStyle, nullptr);
    ASSERT_NE (stepperStyle, nullptr);
    ASSERT_NE (toggleStyle, nullptr);
    ASSERT_NE (scrollerStyle, nullptr);

    // The thumb of the fader is along the track the slider tells, where its value is
    const auto track = fader->getSliderBounds();
    EXPECT_NEAR (measure (*faderStyle, *fader, "thumb").bounds.getCenterX(), track.getX() + 0.6f * track.getWidth(), 0.01f);

    fader->setValue (0.25, dontSendNotification);
    EXPECT_NEAR (measure (*faderStyle, *fader, "thumb").bounds.getCenterX(), track.getX() + 0.25f * track.getWidth(), 0.01f);

    // The buttons of the stepper, the handle of the switch and the thumb of the scroll bar are where the widgets put them
    EXPECT_FALSE (stepper->getDecrementButtonBounds().isEmpty());
    expectSameArea (measure (*stepperStyle, *stepper, "minus").bounds, stepper->getDecrementButtonBounds().reduced (1.0f));
    expectSameArea (measure (*stepperStyle, *stepper, "plus").bounds, stepper->getIncrementButtonBounds().reduced (1.0f));
    expectSameArea (measure (*toggleStyle, *toggle, "handle").bounds, toggle->getSwitchCircleBounds().reduced (4.0f));
    expectSameArea (measure (*scrollerStyle, *scroller, "grip").bounds, scroller->getThumbBoundsForRendering().reduced (3.0f));
}

//==============================================================================
/** The pages of the YUI graphics example, each a document validating one area: widgets, styles, drawings, series, parts. */
class YuiExamplePagesTests : public ::testing::Test
{
protected:
    static constexpr int pageWidth = 640;
    static constexpr int pageHeight = 480;

    /** A page loaded as the example loads it, with the model the Filter is wired to: the model outlives the document. */
    struct Page
    {
        static DataTree makeParams()
        {
            DataTree tree ("params");

            auto transaction = tree.beginTransaction();
            transaction.setProperty ("mode", 0);
            transaction.setProperty ("cutoff", 0.65);
            transaction.setProperty ("resonance", 0.3);
            transaction.setProperty ("drive", 0.2);
            transaction.setProperty ("mix", 0.5);

            return tree;
        }

        DataTree params = makeParams();
        DataTreeBindingProvider provider { params };
        YuiDocument document;
        YuiLoader loader { document };
    };

    static void SetUpTestSuite()
    {
        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, {});
        if (gpuContext == nullptr)
            return;

        if (GpuCanvas::create (*gpuContext, pageWidth, pageHeight) == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

    void SetUp() override
    {
        oldTheme = ApplicationTheme::getGlobalTheme();
        ApplicationTheme::setGlobalTheme (createThemeVersion1());
    }

    void TearDown() override
    {
        ApplicationTheme::setGlobalTheme (oldTheme.get());
        oldTheme = nullptr;
    }

    static File getPagesFolder()
    {
        return File (__FILE__)
            .getParentDirectory()
            .getParentDirectory()
            .getParentDirectory()
            .getChildFile ("examples/graphics/data/yui");
    }

    /** The pages the modules at hand can show: the audio widgets and the compiler of shaders are optional. */
    static StringArray getPages()
    {
        StringArray pages { "filter/ui.yui", "widgets.yui", "lists.yui", "layout.yui", "styles.yui", "bindings.yui", "drawing.yui", "series.yui", "parts.yui", "controls.yui" };

#if YUP_MODULE_AVAILABLE_yup_audio_gui
        pages.add ("audio.yui");

#if YUP_ENABLE_SHADER_TRANSPILER
        pages.add ("shaders.yui");
#endif
#endif

        return pages;
    }

    /** Loads a page from its file, lays it out and styles it. */
    static void loadPage (Page& page, const String& name)
    {
        page.document.setModel ("params", page.provider);

        const auto loaded = page.loader.loadFile (getPagesFolder().getChildFile (name));
        ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();

        auto* root = page.document.getRoot();
        ASSERT_NE (root, nullptr);

        root->setBounds (0.0f, 0.0f, static_cast<float> (pageWidth), static_cast<float> (pageHeight));
        page.document.applyPendingChanges();

        if (auto* styleContext = root->getStyleContext())
            styleContext->restyleNow();
    }

    ApplicationTheme::Ptr oldTheme;
    static std::unique_ptr<GraphicsContext> gpuContext;
};

std::unique_ptr<GraphicsContext> YuiExamplePagesTests::gpuContext;

TEST_F (YuiExamplePagesTests, EveryPageLoadsWithoutAProblem)
{
    if (! getPagesFolder().isDirectory())
        GTEST_SKIP() << "The pages of the example are not available";

    for (const auto& name : getPages())
    {
        SCOPED_TRACE (name.toStdString());

        Page page;
        loadPage (page, name);

        if (HasFatalFailure())
            return;

        EXPECT_TRUE (page.document.getDiagnostics().empty()) << DataTreeDiagnostic::join (page.document.getDiagnostics());
    }
}

TEST_F (YuiExamplePagesTests, EveryPagePaintsAPicture)
{
    if (! getPagesFolder().isDirectory())
        GTEST_SKIP() << "The pages of the example are not available";

    if (gpuContext == nullptr)
        GTEST_SKIP() << "No Metal GPU context available";

    for (const auto& name : getPages())
    {
        SCOPED_TRACE (name.toStdString());

        Page page;
        loadPage (page, name);

        if (HasFatalFailure())
            return;

        // The whole tree is painted: the theme, the drawings, and the parts and the series they read of the widgets
        const auto image = page.document.getRoot()->snapshotToImage (*gpuContext);
        ASSERT_TRUE (image.isValid());

        int numPainted = 0;

        for (int y = 0; y < image.getHeight(); y += 8)
            for (int x = 0; x < image.getWidth(); x += 8)
                numPainted += image.getPixelColor (x, y).getAlpha() > 0 ? 1 : 0;

        EXPECT_GT (numPainted, 50);

        // Nor did painting meet a problem that loading could not tell
        EXPECT_TRUE (page.document.getDiagnostics().empty()) << DataTreeDiagnostic::join (page.document.getDiagnostics());
    }
}
