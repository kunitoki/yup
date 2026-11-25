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

#include <chrono>
#include <iostream>

using namespace yup;

//==============================================================================
/** Times the stages an SVG goes through from its file to the drawing painting it, for the largest
    samples of the `Svg` graphics example. The stages are the ones the example runs when a sample
    is selected, so the numbers add up to how long it blocks for.

    Disabled by default, run it with:
    GTEST_ALSO_RUN_DISABLED_TESTS=1 just test Release "*YuiSvgImporterBenchmark*" yup_gui
*/
class YuiSvgImporterBenchmarkTests : public ::testing::Test
{
protected:
    static constexpr int numRuns = 3;

    enum Stage
    {
        parseSvg,
        convert,
        parseDrawable,
        writeText,
        documentText,
        parseYui,
        compile,
        numStages
    };

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

    template <class F>
    static void measure (double& bestMilliseconds, F&& run)
    {
        const auto start = std::chrono::steady_clock::now();
        run();
        bestMilliseconds = jmin (bestMilliseconds, std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - start).count());
    }

    /* Runs the stages once. Each run names its drawing differently, so none is served a drawing compiled by a run before it. */
    static void runStages (const File& file, int run, std::array<double, numStages>& best)
    {
        YuiSvgImporter::Options options;
        options.colorProperties = true;
        options.name = "Imported" + String (run);

        SVGDocument::Ptr svg;
        measure (best[parseSvg], [&] { svg = SVGParser::parse (file); });
        ASSERT_TRUE (svg != nullptr) << file.getFileName();

        YuiSvgImporter::ImportResult imported;
        measure (best[convert], [&] { imported = YuiSvgImporter::importSvg (*svg, options); });
        ASSERT_TRUE (imported.wasOk()) << file.getFileName();

        Drawable drawable;
        measure (best[parseDrawable], [&] { EXPECT_TRUE (drawable.parseSVG (file)); });

        String text;
        measure (best[writeText], [&] { text = imported.toText(); });

        CodeDocument document;
        measure (best[documentText], [&]
        {
            document.setText (text, dontSendNotification);
            text = document.getText();
        });

        YuiParser::ParseResult parsed;
        measure (best[parseYui], [&] { parsed = YuiParser::parse (text); });
        ASSERT_TRUE (parsed.wasOk()) << file.getFileName() << ": " << YuiDiagnostic::join (parsed.diagnostics);

        Component component;
        component.setBounds (0.0f, 0.0f, 480.0f, 480.0f);

        const auto drawing = parsed.document.getChildWithName ("drawing");
        measure (best[compile], [&]
        {
            const auto style = DrawingStyle::create (component, drawing);
            EXPECT_TRUE (style.wasOk()) << file.getFileName() << ": " << (style.failed() ? style.getErrorMessage() : String());
        });
    }

    ApplicationTheme::Ptr theme;
    ApplicationTheme::Ptr oldTheme;
};

//==============================================================================
TEST_F (YuiSvgImporterBenchmarkTests, DISABLED_TimesTheStagesOfAnImport)
{
    static constexpr const char* stageNames[numStages] = {
        "SVGParser::parse",
        "YuiSvgImporter conversion",
        "Drawable::parseSVG",
        "ImportResult::toText",
        "CodeDocument set + getText",
        "YuiParser::parse",
        "DrawingStyle::create"
    };

    const auto directory = getSvgDirectory();
    if (! directory.isDirectory())
        GTEST_SKIP() << "The example SVGs are not available";

    for (const auto* name : { "car.svg", "gallardo.svg", "tommek_Car.svg" })
    {
        const auto file = directory.getChildFile (name);
        if (! file.existsAsFile())
            continue;

        std::array<double, numStages> best;
        best.fill (std::numeric_limits<double>::max());

        for (int run = 0; run < numRuns; ++run)
        {
            runStages (file, run, best);

            if (HasFatalFailure())
                return;
        }

        double total = 0.0;

        std::cout << "\n  " << name << " (" << file.getSize() / 1024 << " KB), best of " << numRuns << " runs\n";

        for (int stage = 0; stage < numStages; ++stage)
        {
            total += best[static_cast<size_t> (stage)];
            std::cout << "    " << String (stageNames[stage]).paddedRight (' ', 28)
                      << String (best[static_cast<size_t> (stage)], 2).paddedLeft (' ', 10) << " ms\n";
        }

        std::cout << "    " << String ("total").paddedRight (' ', 28) << String (total, 2).paddedLeft (' ', 10) << " ms\n";
    }
}
