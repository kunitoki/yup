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
#include <yup_gui_markup/yup_gui_markup.h>

#include <gtest/gtest.h>

#include <cstdlib>

using namespace yup;

#if YUP_MAC

/** What painting, restyling and rebuilding an interface cost, measured by hand: disabled in the
    normal suites. Run them with "just perf_2d", which also records their baseline and compares
    with it, or one of them with "just perf_2d run Release PaintBenchmarkTests.DISABLED_<name>".

    Each row runs 30 warm-up iterations, then times 120, twice, the rounds of the rows of a test
    interleaved (A/A). An iteration of a painting row paints a component into a canvas at 2 pixels a
    unit, as a window paints a frame of it, and lasts until the GPU finished the frame: `paint` is
    the part of it spent painting on the message thread.

    Prints a table per row, writes the rows as JSON to the absolute path in the environment variable
    YUP_PERF_OUTPUT when set, and compares their medians with the JSON of an earlier run named by
    YUP_PERF_BASELINE when set: it reports, it never fails on a regression. The first test of a
    process to write starts the file, and the ones after it add their rows to it.
*/
class PaintBenchmarkTests : public ::testing::Test
{
protected:
    static constexpr int numWarmUps = 30;
    static constexpr int numTimed = 120;
    static constexpr float pixelsPerUnit = 2.0f;

    /** The shaders of Rive are compiled synchronously: its fallback shaders, which draw until the
        specialized ones are compiled in the background, would be timed in their place. */
    static void SetUpTestSuite()
    {
        GraphicsContext::Options options;
        options.synchronousShaderCompilations = true;

        gpuContext = GraphicsContext::createContext (GpuPlatform::Metal, options);
        if (gpuContext != nullptr && GpuCanvas::create (*gpuContext, 64, 64) == nullptr)
            gpuContext.reset();
    }

    static void TearDownTestSuite()
    {
        gpuContext.reset();
    }

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

    //==============================================================================
    /** A component painted by the drawing set as its style, keeping the levels a drawing reads as a series. */
    struct Drawn : Component
    {
        Drawn (float initialWidth, float initialHeight)
        {
            setOpaque (false);
            setBounds (0.0f, 0.0f, initialWidth, initialHeight);
            setVisible (true);
        }

        ~Drawn() override
        {
            setStyle (nullptr);
        }

        const ComponentTypeInfo& getDefaultTypeInfo() const override
        {
            static const ComponentTypeInfo info = ComponentTypeBuilder<Drawn> ("PaintBenchmarkDrawn")
                .series ("levels", &Drawn::getLevels)
                .build();
            return info;
        }

        Span<const float> getLevels() const noexcept { return levels; }

        std::vector<float> levels;
    };

    /** Fills itself with a color. */
    struct Filled : Component
    {
        explicit Filled (Color colorToFill)
            : fill (colorToFill)
        {
        }

        void paint (Graphics& g) override
        {
            g.setFillColor (fill);
            g.fillAll();
        }

        Color fill;
    };

    /** A filled panel holding a grid of filled cells. */
    struct Panel : Filled
    {
        Panel (int columns, int rows, float cellSize)
            : Filled (Color (0xff203040))
        {
            setBounds (0.0f, 0.0f, static_cast<float> (columns) * cellSize, static_cast<float> (rows) * cellSize);

            for (int i = 0; i < columns * rows; ++i)
            {
                auto& cell = *cells.emplace_back (std::make_unique<Filled> (Color::fromHSV (static_cast<float> (i) / static_cast<float> (columns * rows), 0.6f, 0.9f)));
                cell.setBounds (static_cast<float> (i % columns) * cellSize + 2.0f, static_cast<float> (i / columns) * cellSize + 2.0f, cellSize - 4.0f, cellSize - 4.0f);
                addAndMakeVisible (cell);
            }
        }

        std::vector<std::unique_ptr<Filled>> cells;
    };

    /** 344 components under a root: 8 panels of 6 rows of 6 cells, each with the class of what it is and the class
        of its index among its siblings. */
    struct StyledTree
    {
        StyledTree()
        {
            for (int p = 0; p < 8; ++p)
            {
                auto& panel = add (root, "panel", p);

                for (int r = 0; r < 6; ++r)
                {
                    auto& row = add (panel, "row", r);

                    for (int c = 0; c < 6; ++c)
                        add (row, "cell", c);
                }
            }
        }

        Component& add (Component& parent, const String& styleClass, int index)
        {
            auto& component = *components.emplace_back (std::make_unique<Component> (styleClass + String (index)));
            component.addStyleClass (Identifier (styleClass));
            component.addStyleClass (Identifier ("kind" + String (index)));
            parent.addAndMakeVisible (component);
            return component;
        }

        Component root { "root" };
        std::vector<std::unique_ptr<Component>> components;
    };

    /** Sets the drawing of a `.yui` text as the style of a component. */
    static void setDrawing (Component& component, const String& text)
    {
        const auto parsed = YuiParser::parse (text);
        ASSERT_TRUE (parsed.wasOk()) << DataTreeDiagnostic::join (parsed.diagnostics);

        const auto created = DrawingStyle::create (component, parsed.document.getChildWithName ("drawing"));
        ASSERT_TRUE (created.wasOk()) << created.getErrorMessage();
        component.setStyle (created.getReference());
    }

    /** A canvas holding a component at pixelsPerUnit, or nullptr when it can't be created. */
    static GpuCanvas::Ptr makeCanvas (const Component& component)
    {
        return GpuCanvas::create (*gpuContext, roundToInt (component.getWidth() * pixelsPerUnit), roundToInt (component.getHeight() * pixelsPerUnit));
    }

    /** Paints a component into a canvas as a window paints a frame of it, then waits for the GPU to
        finish the frame by reading it back. */
    void paintFrame (Component& component, GpuCanvas& canvas)
    {
        const auto start = Time::getMillisecondCounterHiRes();

        auto& g = canvas.beginDraw ({}, pixelsPerUnit);
        yup::ComponentTestHelper<Component>::triggerPaint (component, g, component.getLocalBounds());
        canvas.commit();

        paintMilliseconds = Time::getMillisecondCounterHiRes() - start;

        pixels.resize (static_cast<size_t> (canvas.getWidth()) * static_cast<size_t> (canvas.getHeight()) * 4);
        EXPECT_TRUE (canvas.readPixels (pixels.data(), pixels.size()));
    }

    //==============================================================================
    /** A row of a benchmark: what a test times, under the name its baseline is kept with. */
    struct Row
    {
        String name;
        std::function<void (int)> run;              // an iteration, given its number: what is timed
        std::function<void (int)> change = nullptr; // run before each iteration and not timed, when set
    };

    /** The timed iterations of a row in one of its two rounds, in milliseconds. */
    struct Round
    {
        std::vector<double> total; // each iteration as a whole
        std::vector<double> paint; // the part of it paintFrame() spent painting, empty for a row painting nothing
        double load = 0.0;
    };

    /** What compareWithBaseline() decides for a row. */
    enum class Verdict
    {
        pass,      ///< Within 3% of the baseline, within the A/A spread of the run, or faster.
        regressed, ///< More than 3% slower than the baseline, and slower by more than the A/A spread.
        newRow,    ///< The baseline has no such row.
        invalid    ///< A time is not a positive finite number, or the spread is negative or not finite.
    };

    /** Whether a row regressed against its baseline, by the rule of the 3D target benchmarks
        (SceneRendererBenchmarkTests::compareWithBaseline).

        @param baseline  The baseline median of the row in milliseconds, empty when the baseline lacks the row.
        @param current   The median of the row in this run, in milliseconds.
        @param spread    The A/A spread of the row in this run, |a - b| of its two rounds, in milliseconds.
    */
    static Verdict compareWithBaseline (std::optional<double> baseline, double current, double spread)
    {
        if (! std::isfinite (current) || current <= 0.0 || ! std::isfinite (spread) || spread < 0.0)
            return Verdict::invalid;

        if (! baseline.has_value())
            return Verdict::newRow;

        if (! std::isfinite (*baseline) || *baseline <= 0.0)
            return Verdict::invalid;

        return current > *baseline * 1.03 && current - *baseline > spread ? Verdict::regressed : Verdict::pass;
    }

    /** The value below which @p fraction of @p values lie, 0.5 for the median; 0 without values. */
    static double percentile (std::vector<double> values, double fraction)
    {
        if (values.empty())
            return 0.0;

        std::sort (values.begin(), values.end());
        return values[jmin (values.size() - 1, static_cast<size_t> (fraction * static_cast<double> (values.size())))];
    }

    /** Times the rows in two interleaved rounds, then prints them, writes them and compares them
        with their baseline (see the class). A failure of an iteration leaves the rows unreported. */
    void timeRows (const std::vector<Row>& rows)
    {
        double load[3] = {};
        getloadavg (load, 3);

        std::vector<std::vector<Round>> results (rows.size());

        for (int round = 0; round < 2; ++round)
        {
            for (size_t i = 0; i < rows.size(); ++i)
            {
                Round result;
                double roundLoad[3] = {};
                getloadavg (roundLoad, 3);
                result.load = roundLoad[0];

                for (int iteration = 0; iteration < numWarmUps + numTimed; ++iteration)
                {
                    if (rows[i].change != nullptr)
                        rows[i].change (iteration);

                    paintMilliseconds = -1.0;
                    const auto start = Time::getMillisecondCounterHiRes();
                    rows[i].run (iteration);
                    const auto elapsed = Time::getMillisecondCounterHiRes() - start;

                    if (iteration < numWarmUps)
                        continue;

                    result.total.push_back (elapsed);
                    if (paintMilliseconds >= 0.0)
                        result.paint.push_back (paintMilliseconds);
                }

                results[i].push_back (std::move (result));
            }
        }

        if (! HasFailure())
            reportRows (rows, results, load);
    }

    /** Prints a table per row, writes the rows and compares them with their baseline (see the class).
        @p load is the load average when the test started. */
    void reportRows (const std::vector<Row>& rows, const std::vector<std::vector<Round>>& results, const double* load)
    {
        const auto gpuName = SystemStats::getEnvironmentVariable ("YUP_PERF_GPU", "unknown");
#if YUP_DEBUG
        const String config ("Debug");
#else
        const String config ("Release");
#endif

        var json (new DynamicObject());
        json.getDynamicObject()->setProperty ("gpu", gpuName);
        json.getDynamicObject()->setProperty ("config", config);
        json.getDynamicObject()->setProperty ("date", Time::getCurrentTime().toISO8601 (true));
        json.getDynamicObject()->setProperty ("load", Array<var> { load[0], load[1], load[2] });

        Array<var> rowsJson;
        std::vector<double> medians, spreads;

        for (size_t i = 0; i < rows.size(); ++i)
        {
            const auto& rounds = results[i];
            const auto a = percentile (rounds[0].total, 0.5);
            const auto b = percentile (rounds[1].total, 0.5);
            medians.push_back ((a + b) * 0.5);
            spreads.push_back (std::abs (a - b));

            std::cout << "\n"
                      << String::formatted ("%s: median %.3f ms (rounds %.3f and %.3f, A/A spread %.1f%%)",
                                            rows[i].name.toRawUTF8(), medians.back(), a, b, spreads.back() / medians.back() * 100.0)
                      << "\n"
                      << String::formatted ("  %5s %8s %8s %8s %8s %6s", "round", "samples", "median", "p95", "paint", "load")
                      << std::endl;

            Array<var> roundsJson;
            for (size_t r = 0; r < rounds.size(); ++r)
            {
                const auto& round = rounds[r];
                const auto median = percentile (round.total, 0.5);
                const auto p95 = percentile (round.total, 0.95);
                const auto paintMedian = percentile (round.paint, 0.5);

                std::cout << String::formatted ("  %5d %8d %8.3f %8.3f %8.3f %6.2f", static_cast<int> (r + 1), static_cast<int> (round.total.size()), median, p95, paintMedian, round.load)
                          << std::endl;

                var roundJson (new DynamicObject());
                auto& properties = *roundJson.getDynamicObject();
                properties.setProperty ("median", median);
                properties.setProperty ("p95", p95);
                properties.setProperty ("paintMedian", paintMedian);
                properties.setProperty ("samples", static_cast<int> (round.total.size()));
                properties.setProperty ("load", round.load);
                roundsJson.add (roundJson);
            }

            if (spreads.back() > medians.back() * 0.03)
                std::cout << String::formatted ("  Warning: the A/A spread of this row is %.1f%%, above 3%%: the machine was not idle enough to trust it", spreads.back() / medians.back() * 100.0) << std::endl;

            var rowJson (new DynamicObject());
            rowJson.getDynamicObject()->setProperty ("name", rows[i].name);
            rowJson.getDynamicObject()->setProperty ("median", medians.back());
            rowJson.getDynamicObject()->setProperty ("spread", spreads.back() / medians.back());
            rowJson.getDynamicObject()->setProperty ("spreadMilliseconds", spreads.back());
            rowJson.getDynamicObject()->setProperty ("rounds", roundsJson);
            rowsJson.add (rowJson);
        }

        const auto outputPath = SystemStats::getEnvironmentVariable ("YUP_PERF_OUTPUT", {});

        // The rows the tests run before this one wrote stay in the file, but for the ones timed again
        Array<var> allRows;
        if (std::exchange (outputStarted, true) && File::isAbsolutePath (outputPath))
        {
            const auto written = JSON::parse (File (outputPath));
            if (const auto* writtenRows = written["rows"].getArray(); writtenRows != nullptr)
                for (const auto& writtenRow : *writtenRows)
                    if (std::none_of (rows.begin(), rows.end(), [&writtenRow] (const Row& row) { return writtenRow["name"].toString() == row.name; }))
                        allRows.add (writtenRow);
        }

        allRows.addArray (rowsJson);
        json.getDynamicObject()->setProperty ("rows", allRows);

        if (outputPath.isNotEmpty())
        {
            if (! File::isAbsolutePath (outputPath))
                ADD_FAILURE() << "YUP_PERF_OUTPUT is not an absolute path, nothing written: " << outputPath;
            else if (File (outputPath).getParentDirectory().createDirectory().wasOk() && File (outputPath).replaceWithText (JSON::toString (json)))
                std::cout << "\nWrote " << outputPath << std::endl;
            else
                ADD_FAILURE() << "Could not write " << outputPath;
        }

        const auto baselinePath = SystemStats::getEnvironmentVariable ("YUP_PERF_BASELINE", {});
        if (baselinePath.isEmpty())
            return;

        if (! File::isAbsolutePath (baselinePath) || ! File (baselinePath).existsAsFile())
        {
            std::cout << "\nNo baseline to compare with at " << baselinePath << std::endl;
            return;
        }

        const auto baseline = JSON::parse (File (baselinePath));
        if (! baseline.isObject())
        {
            std::cout << "\nThe baseline is not a JSON object, not compared: " << baselinePath << std::endl;
            return;
        }

        if (baseline["gpu"].toString() != gpuName || baseline["config"].toString() != config)
        {
            std::cout << "\nWarning: the baseline was recorded on " << baseline["gpu"].toString() << " in " << baseline["config"].toString()
                      << ", this run is on " << gpuName << " in " << config << ": not compared" << std::endl;
            return;
        }

        std::cout << "\nCompared with " << baselinePath << " (" << baseline["date"].toString() << "):" << std::endl;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            std::optional<double> baselineMedian;
            if (const auto* baselineRows = baseline["rows"].getArray(); baselineRows != nullptr)
                for (const auto& baselineRow : *baselineRows)
                    if (baselineRow["name"].toString() == rows[i].name)
                        baselineMedian = static_cast<double> (baselineRow["median"]);

            const auto current = medians[i];
            const auto verdict = compareWithBaseline (baselineMedian, current, spreads[i]);
            if (verdict == Verdict::newRow)
            {
                std::cout << String::formatted ("  new        %s: %.3f ms, not in the baseline", rows[i].name.toRawUTF8(), current) << std::endl;
                continue;
            }

            const auto before = baselineMedian.value_or (0.0);
            std::cout << String::formatted ("  %-10s %s: %.3f ms, baseline %.3f ms, %+.3f ms (%+.1f%%), A/A spread %.3f ms",
                                            verdict == Verdict::pass ? "PASS" : (verdict == Verdict::regressed ? "REGRESSED" : "INVALID"),
                                            rows[i].name.toRawUTF8(), current, before, current - before, before > 0.0 ? (current / before - 1.0) * 100.0 : 0.0, spreads[i])
                      << std::endl;
        }
    }

#if YUP_ENABLE_SHADER_TRANSPILER
    /** The picture of the component, pulsing with the time. */
    static constexpr const char* pulseFragment = R"(#version 450
layout(set = 0, binding = 0) uniform Parameters { float time; } params;
layout(set = 0, binding = 1) uniform texture2D source;
layout(set = 0, binding = 2) uniform sampler sourceSampler;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() { color = texture (sampler2D (source, sourceSampler), uv) * (0.75 + 0.25 * sin (params.time)); })";
#endif

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
    std::vector<uint8> pixels;
    double paintMilliseconds = -1.0; // what the last paintFrame() spent painting, negative when the iteration painted nothing

    static inline std::unique_ptr<GraphicsContext> gpuContext;
    static inline bool outputStarted = false; // a test of this process wrote its rows to YUP_PERF_OUTPUT
};

/** A drawing whose shapes are in a blurred layer, painted again and again with nothing changed:
    the shapes read the bounds of the component and roles of the palette. */
TEST_F (PaintBenchmarkTests, DISABLED_FilteredLayerPaintTimes)
{
    if (gpuContext == nullptr)
        GTEST_SKIP() << "No Metal GPU context available";

    Drawn component (256.0f, 256.0f);
    ASSERT_NO_FATAL_FAILURE (setDrawing (component, R"yui(
        drawing Glow {
            layer {
                filter { blur { deviation: 6; } }
                rect { bounds: bounds.reduced (48); fill: palette.accent; cornerRadius: 16; }
                circle { center: bounds.center; radius: bounds.width / 4; fill: palette.text; }
            }
        })yui"));

    const auto canvas = makeCanvas (component);
    ASSERT_NE (canvas, nullptr);

    timeRows ({ { "filtered layer 256x256", [&] (int) { paintFrame (component, *canvas); } } });
}

/** An area under a polyline of 2048 points, a `repeat` of `vertex` elements with a `base` reading a
    series of the component, painted while the series changes before every paint. */
TEST_F (PaintBenchmarkTests, DISABLED_SeriesPolylinePaintTimes)
{
    if (gpuContext == nullptr)
        GTEST_SKIP() << "No Metal GPU context available";

    Drawn component (512.0f, 256.0f);
    component.levels.resize (2048);

    // More points than the pixels they are drawn across: 4096 across the 512 pixels of 256 units
    Drawn dense (256.0f, 128.0f);
    dense.levels.resize (4096);

    for (auto* drawn : { &component, &dense })
    {
        ASSERT_NO_FATAL_FAILURE (setDrawing (*drawn, R"yui(
            drawing Scope {
                let count: len (levels);
                let step: bounds.width / (count - 1.0);

                repeat i in 0 .. count {
                    vertex { at: point (i * step, bounds.bottom - levels[i] * bounds.height); base: point (i * step, bounds.bottom); fill: palette.accent; }
                }
            })yui"));
    }

    const auto canvas = makeCanvas (component);
    const auto denseCanvas = makeCanvas (dense);
    ASSERT_NE (canvas, nullptr);
    ASSERT_NE (denseCanvas, nullptr);

    const auto moveLevels = [] (Drawn& drawn, int iteration)
    {
        for (size_t i = 0; i < drawn.levels.size(); ++i)
            drawn.levels[i] = 0.5f + 0.4f * std::sin (0.02f * static_cast<float> (i) + 0.1f * static_cast<float> (iteration));
    };

    timeRows ({ { "series polyline 2048 points 512x256", [&] (int) { paintFrame (component, *canvas); }, [&] (int iteration) { moveLevels (component, iteration); } },
                { "series polyline 4096 points 256x128", [&] (int) { paintFrame (dense, *denseCanvas); }, [&] (int iteration) { moveLevels (dense, iteration); } } });
}

/** 200 plain shapes, each reading the size and the states of the component, two properties its
    drawing declares and roles of the palette, painted again and again with nothing changed: what
    feeding the inputs of a drawing costs. */
TEST_F (PaintBenchmarkTests, DISABLED_ManyShapesPaintTimes)
{
    if (gpuContext == nullptr)
        GTEST_SKIP() << "No Metal GPU context available";

    String text ("drawing Cells {\n    property shade: color = #ff8040;\n    property gain: number = 1;\n");

    for (int i = 0; i < 200; ++i)
        text << "    rect { bounds: rect (" << (i % 20) << " * width / 20, " << (i / 20) << " * height / 10, width / 20 - 2, height / 10 - 2);"
             << " fill: enabled ? shade.withAlpha (gain) : palette.surface;"
             << " stroke: focused ? palette.accent : palette.outline; strokeWidth: hovered ? 2 : 1; }\n";

    text << "}\n";

    Drawn component (320.0f, 160.0f);
    ASSERT_NO_FATAL_FAILURE (setDrawing (component, text));

    const auto canvas = makeCanvas (component);
    ASSERT_NE (canvas, nullptr);

    timeRows ({ { "200 shapes 320x160", [&] (int) { paintFrame (component, *canvas); } } });
}

/** A drawing of 20 pictures of one data URI as long as the one of an icon, painted again and again with nothing
    changed: what finding the picture kept for a source costs. */
TEST_F (PaintBenchmarkTests, DISABLED_PicturesPaintTimes)
{
    if (gpuContext == nullptr)
        GTEST_SKIP() << "No Metal GPU context available";

    // A 1x1 PNG behind a name that makes its source 4096 characters long
    const String pixel ("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==");
    const String prefix ("data:image/png;name=");
    const String suffix (";base64,");
    const auto source = prefix + String::repeatedString ("x", 4096 - prefix.length() - suffix.length() - pixel.length()) + suffix + pixel;

    String text ("drawing Pictures {\n");

    for (int i = 0; i < 20; ++i)
        text << "    image { source: \"" << source << "\"; bounds: rect (" << (i % 10) * 32 + 4 << ", " << (i / 10) * 80 + 4 << ", 24, 24); }\n";

    text << "}\n";

    Drawn component (320.0f, 160.0f);
    ASSERT_NO_FATAL_FAILURE (setDrawing (component, text));

    const auto canvas = makeCanvas (component);
    ASSERT_NE (canvas, nullptr);

    timeRows ({ { "20 pictures of a 4096 character data URI 320x160", [&] (int) { paintFrame (component, *canvas); } } });
}

/** The widgets that show a text for each of their rows or tabs, painted again and again with nothing changed, and
    a tab bar laid out again: what the texts of a list, of a tree and of tabs cost at each paint and each layout. */
TEST_F (PaintBenchmarkTests, DISABLED_WidgetTextPaintTimes)
{
    if (gpuContext == nullptr)
        GTEST_SKIP() << "No Metal GPU context available";

    // The theme that paints the widgets, which the fixture leaves out
    theme = createThemeVersion1();
    ApplicationTheme::setGlobalTheme (theme);

    /** An item of a tree showing a text. */
    struct TextItem final : TreeViewItem
    {
        explicit TextItem (String textToShow)
            : text (std::move (textToShow))
        {
        }

        String getItemText() const override { return text; }

        String text;
    };

    StringArray rows;

    for (int row = 0; row < 20; ++row)
        rows.add ("List row number " + String (row + 1));

    ListBox list;
    list.setBounds (0.0f, 0.0f, 240.0f, 480.0f);
    list.setVisible (true);
    list.setItems (rows);

    // 4 open folders of 4 items
    TreeView tree;
    auto root = std::make_unique<TextItem> (String());
    TreeViewItem* folder = root.get();

    for (int row = 0; row < 20; ++row)
    {
        auto item = std::make_unique<TextItem> (row % 5 == 0 ? "Folder " + String (row / 5 + 1) : "Tree item " + String (row));

        if (row % 5 != 0)
        {
            folder->addSubItem (std::move (item));
            continue;
        }

        item->setOpen (true);
        folder = &root->addSubItem (std::move (item));
    }

    tree.setRootItem (std::move (root));
    tree.setRootItemVisible (false);
    tree.setBounds (0.0f, 0.0f, 240.0f, 480.0f);
    tree.setVisible (true);

    TabBar tabs;
    int numTabs = 0;

    for (const auto* name : { "Overview", "Mixer", "Effects", "Routing", "Presets", "Settings", "Devices", "About" })
        tabs.addTab (Identifier ("tab" + String (numTabs++)), name);

    tabs.setBounds (0.0f, 0.0f, 800.0f, 36.0f);
    tabs.setVisible (true);

    const auto listCanvas = makeCanvas (list);
    const auto tabsCanvas = makeCanvas (tabs);
    ASSERT_NE (listCanvas, nullptr);
    ASSERT_NE (tabsCanvas, nullptr);

    timeRows ({ { "list of 20 rows 240x480", [&] (int) { paintFrame (list, *listCanvas); } },
                { "tree of 20 rows 240x480", [&] (int) { paintFrame (tree, *listCanvas); } },
                { "tab bar of 8 tabs 800x36", [&] (int) { paintFrame (tabs, *tabsCanvas); } },
                { "tab bar of 8 tabs laid out", [&] (int iteration) { tabs.setBounds (0.0f, 0.0f, 800.0f, iteration % 2 == 0 ? 37.0f : 36.0f); } } });
}

#if YUP_ENABLE_SHADER_TRANSPILER

/** A fragment shader made from its source, as the `shader` block of a stylesheet is when it is loaded and each
    time its file changes: the source is another one at each iteration, as an edited file is. For every graphics
    API, as a tool checks it, and for the one a window draws with, as an interface loads it. */
TEST_F (PaintBenchmarkTests, DISABLED_ShaderLoadTimes)
{
    ASSERT_TRUE (GpuImageShader::createFromGlsl (pulseFragment).wasOk());

    const auto makeSource = [] (int iteration)
    {
        return String (pulseFragment) + "\n// " + String (iteration) + "\n";
    };

    const auto load = [&makeSource] (int iteration)
    {
        const auto shader = GpuImageShader::createFromGlsl (makeSource (iteration));
        ignoreUnused (shader);
    };

    const auto loadForOneApi = [&makeSource] (int iteration)
    {
        const auto shader = GpuImageShader::createFromGlsl (makeSource (iteration), ComponentNative::getDefaultGraphicsApi());
        ignoreUnused (shader);
    };

    timeRows ({ { "image shader made from its source", load },
                { "image shader made from its source for one graphics API", loadForOneApi } });
}

/** A panel of 48 children drawn through a ShaderEffect whose shader is given the time: every frame
    of its window advances the time, which repaints the panel, then the window is painted. */
TEST_F (PaintBenchmarkTests, DISABLED_ShaderEffectSubtreePaintTimes)
{
    if (gpuContext == nullptr)
        GTEST_SKIP() << "No Metal GPU context available";

    const auto shader = GpuImageShader::createFromGlsl (pulseFragment);
    ASSERT_TRUE (shader.wasOk()) << shader.getErrorMessage();

    Filled window (Color (0xff101010));
    auto* native = new StubComponentNative (window, ComponentNative::defaultFlags);
    yup::ComponentTestHelper<Component>::attachNative (window, native);
    window.setBounds (0.0f, 0.0f, 384.0f, 256.0f);
    window.setVisible (true);

    Panel panel (8, 6, 32.0f);
    window.addAndMakeVisible (panel);
    panel.setComponentEffect (new ShaderEffect (panel, shader.getValue()));

    const auto canvas = makeCanvas (window);
    ASSERT_NE (canvas, nullptr);

    const auto advanceFrame = [native] (int)
    {
        native->repaintAreas.clearQuick();
        native->callFrameListeners (1.0 / 60.0);
    };

    timeRows ({ { "shader effect over 48 children 384x256", [&] (int) { paintFrame (window, *canvas); }, advanceFrame } });
}

#endif // YUP_ENABLE_SHADER_TRANSPILER

/** 344 components in 8 panels of 6 rows of 6 cells, under a stylesheet of 39 rules: the restyle
    after a class of the root changed, which the rules reading it take to every cell. */
TEST_F (PaintBenchmarkTests, DISABLED_RestyleTimes)
{
    StyledTree tree;
    auto& root = tree.root;
    auto& components = tree.components;

    String text ("style {\n"
                 "    .panel { trackWidth: 1; }\n"
                 "    .row { trackWidth: 2; }\n"
                 "    .cell { trackWidth: 3; opacity: 0.9; }\n"
                 "    .panel > .row { trackWidth: 4; }\n"
                 "    .panel .cell { trackWidth: 5; }\n"
                 "    .active .cell { trackWidth: 6; }\n"
                 "    .active .row > .cell { trackWidth: 7; }\n");

    for (int kind = 0; kind < 8; ++kind)
        text << "    .kind" << kind << " { trackWidth: " << (10 + kind) << "; }\n"
             << "    .row > .kind" << kind << " { trackWidth: " << (20 + kind) << "; }\n"
             << "    .active .kind" << kind << " { trackWidth: " << (30 + kind) << "; }\n"
             << "    #panel" << kind << " .kind" << kind << " { trackWidth: " << (40 + kind) << "; }\n";

    text << "}\n";

    const auto sheet = YuiParser::parseStyleSheet (text);
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    root.setStyleSheet (sheet.getReference());

    auto* context = root.getStyleContext();
    ASSERT_NE (context, nullptr);
    context->restyleNow();

    // The class of the root has to reach the cells, or the rows below time a restyle of nothing
    const auto getLastCellWidth = [&components] { return static_cast<double> (components.back()->findProperty (Identifier ("trackWidth"))); };
    const auto idleWidth = getLastCellWidth();
    root.addStyleClass (Identifier ("active"));
    context->restyleNow();
    ASSERT_NE (getLastCellWidth(), idleWidth);
    root.removeStyleClass (Identifier ("active"));

    const auto toggleClass = [&root] (int iteration)
    {
        if (iteration % 2 == 0)
            root.addStyleClass (Identifier ("active"));
        else
            root.removeStyleClass (Identifier ("active"));
    };

    timeRows ({ { "restyle of 344 components under 39 rules", [context] (int) { context->restyleNow(); }, toggleClass } });
}

/** The same 344 components under rules reading the hover of the component they match: the restyle after the pointer
    came over a cell or left it, which the cell and each of its parents up to the root are told. */
TEST_F (PaintBenchmarkTests, DISABLED_HoverRestyleTimes)
{
    StyledTree tree;

    const auto sheet = YuiParser::parseStyleSheet ("style {\n"
                                                   "    .panel { trackWidth: 1; }\n"
                                                   "    .row { trackWidth: 2; }\n"
                                                   "    .row:hover { opacity: 0.9; }\n"
                                                   "    .cell { trackWidth: 3; }\n"
                                                   "    .cell:hover { trackWidth: 6; }\n"
                                                   "}\n");
    ASSERT_TRUE (sheet.wasOk()) << sheet.getErrorMessage();
    tree.root.setStyleSheet (sheet.getReference());

    auto* context = tree.root.getStyleContext();
    ASSERT_NE (context, nullptr);
    context->restyleNow();

    // The hover has to restyle the cell, or the row below times a restyle of nothing
    auto& cell = *tree.components.back();
    const auto getCellWidth = [&cell] { return static_cast<double> (cell.findProperty (Identifier ("trackWidth"))); };
    const auto idleWidth = getCellWidth();
    cell.setHovered (true);
    context->restyleNow();
    ASSERT_NE (getCellWidth(), idleWidth);
    cell.setHovered (false);
    context->restyleNow();

    const auto toggleHover = [&cell] (int iteration) { cell.setHovered (iteration % 2 == 0); };

    timeRows ({ { "restyle of a hovered cell among 344 components", [context] (int) { context->restyleNow(); }, toggleHover } });
}

/** A document whose `repeat` makes 256 labels over a range bound to a model: making its components
    again after the range changed by one, and after a property of one element of its live tree
    changed. */
TEST_F (PaintBenchmarkTests, DISABLED_DocumentRebuildTimes)
{
    DataTree model ("sequencer");
    model.beginTransaction().setProperty ("length", 256);

    DataTreeBindingProvider sequencer (model, nullptr);

    YuiDocument document;
    document.setModel ("seq", sequencer);

    const auto loaded = document.loadFromString (R"yui(
        style {
            .step { opacity: 0.5; }
        }

        Column {
            Label { id: title; text: "Steps"; }
            Row { id: row; repeat i in 0 .. seq.length { Label { class: step; text: format ("%d", i); } } }
        })yui");

    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    ASSERT_NE (document.findById ("row"), nullptr);
    ASSERT_EQ (document.findById ("row")->getNumChildComponents(), 256);

    auto title = document.getTree().getChild (1).getChild (0);
    ASSERT_EQ (title.getProperty ("id").toString(), "title");

    const auto changeRange = [&model] (int iteration) { model.beginTransaction().setProperty ("length", 255 + iteration % 2); };
    const auto applyChanges = [&document] (int) { document.applyPendingChanges(); };

    // Whether the document builds again as its tree is mutated or once the change is over, the row ends with the build done
    const auto renameTitle = [&] (int iteration)
    {
        title.beginTransaction().setProperty ("text", iteration % 2 == 0 ? "\"Steps\"" : "\"Beats\"");
        document.applyPendingChanges();
    };

    timeRows ({ { "document range change of 256 labels", applyChanges, changeRange },
                { "document live mutation with 256 labels", renameTitle } });

    EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
}

/** A document of 256 labels written one by one, each with the plain values an interface is mostly made of, a text, a
    number and two booleans: loading it into a new document, and releasing that document. */
TEST_F (PaintBenchmarkTests, DISABLED_DocumentLoadTimes)
{
    String text ("style {\n    .step { visible: true; }\n}\n\nColumn {\n");

    for (int i = 0; i < 256; ++i)
        text << "    Label { id: step" << i << "; class: step; text: \"Step " << i << "\"; opacity: 0.75; enabled: true; cached: false; }\n";

    text << "}\n";

    {
        YuiDocument document;
        const auto loaded = document.loadFromString (text);

        ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
        ASSERT_NE (document.findById ("step255"), nullptr);
        EXPECT_TRUE (document.getDiagnostics().empty()) << DataTreeDiagnostic::join (document.getDiagnostics());
    }

    const auto load = [&text] (int)
    {
        YuiDocument document;
        document.loadFromString (text);
    };

    timeRows ({ { "document first load of 256 labels", load } });
}

#endif // YUP_MAC
