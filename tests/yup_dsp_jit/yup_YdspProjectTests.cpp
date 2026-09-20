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

#include <yup_dsp_jit/yup_dsp_jit.h>
#include <gtest/gtest.h>

using namespace yup;

class YdspProjectTests : public ::testing::Test
{
protected:
    File directory;
    File manifest;

    void SetUp() override
    {
        directory = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup-project-" + Uuid().toString());
        ASSERT_TRUE (directory.createDirectory().wasOk());
        manifest = directory.getChildFile ("test.ydsp-project");
    }

    void TearDown() override
    {
        directory.deleteRecursively();
    }

    void write (StringRef path, StringRef source)
    {
        const auto file = directory.getChildFile (String (path));
        ASSERT_TRUE (file.getParentDirectory().createDirectory().wasOk());
        ASSERT_TRUE (file.replaceWithText (String (source)));
    }

    void project (StringRef main, StringRef sources)
    {
        write ("test.ydsp-project", "formatVersion: 1\nname: Test patch\nversion: \"1.0\"\nmain: "
                                      + String (main) + "\nsources: " + String (sources) + "\n");
    }

    static void expectOutput (YdspAudioGraph& graph, float expected)
    {
        graph.prepare (48000.0, 8);
        std::array<float, 8> input;
        input.fill (0.25f);
        std::array<float, 8> output {};
        const YdspInputBuffer inputs[] { Span<const float> (input.data(), input.size()) };
        YdspOutputBuffer outputs[] { Span<float> (output.data(), output.size()) };
        ASSERT_EQ (YdspProcessResult::ok, graph.process (YdspProcessRequest { inputs, outputs, 8 }));
        for (const auto value : output)
            EXPECT_FLOAT_EQ (expected, value);
    }
};

TEST_F (YdspProjectTests, ParsesYamlMetadataAndSourceList)
{
    YdspDiagnostics diagnostics;
    auto parsed = YdspProject::parse (
        "formatVersion: 1\nmain: Main\nsources:\n  - Main.ydsp\n  - lib/Math.ydsp\n"
        "id: org.yup.test\nname: 'Patch: test'\nversion: \"1.0\"\nisInstrument: false\n"
        "description: |\n  A reusable patch.\n  With explicit imports.\n",
        manifest, diagnostics);
    ASSERT_TRUE (parsed.wasOk()) << diagnostics.toString();
    EXPECT_EQ ("Main", parsed.getReference().getMain());
    EXPECT_EQ (2, parsed.getReference().getSources().size());
    EXPECT_EQ ("lib/Math.ydsp", parsed.getReference().getSources()[1]);
    EXPECT_EQ ("Patch: test", parsed.getReference().getMetadata()["name"].toString());
    EXPECT_TRUE (parsed.getReference().getMetadata()["description"].toString().contains ("explicit imports"));
    EXPECT_EQ (manifest, parsed.getReference().getFile());
}

TEST_F (YdspProjectTests, RejectsInvalidManifestFields)
{
    for (const auto* text : {
             "formatVersion: 2\nmain: P\nsources: [P.ydsp]",
             "formatVersion: true\nmain: P\nsources: [P.ydsp]",
             "formatVersion: 1\nsources: [P.ydsp]",
             "formatVersion: 1\nmain: P\nsources: []",
             "formatVersion: 1\nmain: P\nsources: P.ydsp",
             "formatVersion: 1\nmain: P\nsources: [3]",
             "formatVersion: 1\nmain: P\nsources: [P.txt]",
             "formatVersion: 1\nmain: P\nsources: [P.ydsp, ./P.ydsp]",
             "formatVersion: 1\nmain: P\nsources: [P.ydsp]\nversion: 1.0",
             "formatVersion: 1\nmain: P\nsources: [P.ydsp]\nisInstrument: \"yes\"",
             "formatVersion: 1\nmain: P\nsources: [P.ydsp]\nmisspelled: value",
             "- P.ydsp" })
    {
        SCOPED_TRACE (text);
        YdspDiagnostics diagnostics;
        EXPECT_FALSE (YdspProject::parse (text, manifest, diagnostics).wasOk());
        EXPECT_TRUE (diagnostics.hasErrors());
        EXPECT_TRUE (diagnostics.toString().contains (manifest.getFullPathName() + ":"));
        EXPECT_TRUE (diagnostics.toString().contains ("^"));
    }
}

TEST_F (YdspProjectTests, AcceptsYamlBooleanSpellings)
{
    for (const auto* value : { "true", "yes", "false", "no" })
    {
        SCOPED_TRACE (value);
        YdspDiagnostics diagnostics;
        auto parsed = YdspProject::parse (
            String ("formatVersion: 1\nmain: P\nsources: [P.ydsp]\nisInstrument: ") + value,
            manifest, diagnostics);
        ASSERT_TRUE (parsed.wasOk()) << diagnostics.toString();
        EXPECT_EQ (String (value) == "true" || String (value) == "yes",
                   static_cast<bool> (parsed.getReference().getMetadata()["isInstrument"]));
    }
}

TEST_F (YdspProjectTests, ReportsYamlSyntaxErrorWithSourceExcerpt)
{
    YdspDiagnostics diagnostics;
    EXPECT_FALSE (YdspProject::parse ("formatVersion: 1\nmain: P\nsources: [P.ydsp", manifest, diagnostics).wasOk());
    EXPECT_TRUE (diagnostics.toString().contains ("sources: [P.ydsp"));
    EXPECT_TRUE (diagnostics.toString().contains ("^"));
}

TEST_F (YdspProjectTests, CompilesProcessorEntryWithExplicitFunctionImport)
{
    project ("Main", "[lib/Math.ydsp, Main.ydsp]");
    write ("lib/Math.ydsp", "func twice (x: float) : float { return x * 2.0; }");
    write ("Main.ydsp", "import lib.Math as math; processor Main { input stream in; output stream out; process { out = math.twice (in); } }");

    for (const auto tier : { YdspOptimizationTier::baseline, YdspOptimizationTier::automatic })
    {
        YdspCompileOptions options;
        options.optimizationTier = tier;
        YdspCompiler compiler;
        auto compiled = compiler.compileProject (manifest, options);
        ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
        expectOutput (compiled.getReference(), 0.5f);
    }
}

TEST_F (YdspProjectTests, ListedFilesDoNotShareDefinitionsWithoutImports)
{
    project ("Main", "[Main.ydsp, Library.ydsp]");
    write ("Library.ydsp", "func twice (x: float) : float { return x * 2.0; }");
    write ("Main.ydsp", "processor Main { input stream in; output stream out; process { out = twice (in); } }");
    YdspCompiler compiler;
    EXPECT_FALSE (compiler.compileProject (manifest).wasOk());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("Unknown function 'twice'"));
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains (directory.getChildFile ("Main.ydsp").getFullPathName()));
}

TEST_F (YdspProjectTests, HostOverrideSelectsAnotherProcessorWithoutChangingManifest)
{
    project ("First", "[First.ydsp, Second.ydsp]");
    write ("First.ydsp", "processor First { input stream in; output stream out; process { out = in; } }");
    write ("Second.ydsp", "processor Second { input stream in; output stream out; process { out = in * 3.0; } }");
    YdspCompiler compiler;
    auto first = compiler.compileProject (manifest);
    ASSERT_TRUE (first.wasOk()) << compiler.getDiagnostics().toString();
    expectOutput (first.getReference(), 0.25f);
    auto second = compiler.compileProject (manifest, {}, "Second");
    ASSERT_TRUE (second.wasOk()) << compiler.getDiagnostics().toString();
    expectOutput (second.getReference(), 0.75f);
    EXPECT_TRUE (manifest.loadFileAsString().contains ("main: First"));
    EXPECT_FALSE (compiler.compileProject (manifest, {}, "Missing").wasOk());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("Unknown project main"));
}

TEST_F (YdspProjectTests, GraphSelectionOverridesMainAnnotationAndResolvesNestedImports)
{
    project ("Chosen", "[src/Main.ydsp, src/lib/Gain.ydsp, src/lib/Math.ydsp]");
    write ("src/lib/Math.ydsp", "func twice (x: float) : float { return x * 2.0; }");
    write ("src/lib/Gain.ydsp", "import Math as math; processor Gain { input stream in; output stream out; process { out = math.twice (in); } }");
    write ("src/Main.ydsp",
           "import lib.Gain as lib; "
           "graph Other [[ main ]] { input stream in; output stream out; node p = lib.Gain; connection { in -> p.in; p.out -> out; } } "
           "graph Chosen { input stream in; output stream out; node p = lib.Gain; connection { in -> p.in; p.out -> out; } }");
    ThreadPool pool (2);
    YdspCompiler compiler;
    auto compiled = compiler.compileProject (manifest, {}, {}, &pool);
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    expectOutput (compiled.getReference(), 0.5f);
}

TEST_F (YdspProjectTests, RejectsAmbiguousEntryNamesAcrossIndependentFiles)
{
    project ("Main", "[A.ydsp, B.ydsp]");
    for (const auto* path : { "A.ydsp", "B.ydsp" })
        write (path, "processor Main { input stream in; output stream out; process { out = in; } }");
    YdspCompiler compiler;
    EXPECT_FALSE (compiler.compileProject (manifest).wasOk());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("Ambiguous project main"));
}

TEST_F (YdspProjectTests, ReportsMissingManifestSourceAndUnlistedImport)
{
    YdspCompiler compiler;
    EXPECT_FALSE (compiler.compileProject (manifest).wasOk());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("Cannot read project manifest"));
    project ("Main", "[Main.ydsp]");
    EXPECT_FALSE (compiler.compileProject (manifest).wasOk());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("Cannot read project source"));
    write ("Main.ydsp", "import Library; processor Main { input stream in; output stream out; process { out = in; } }");
    write ("Library.ydsp", "func twice (x: float) : float { return x * 2.0; }");
    EXPECT_FALSE (compiler.compileProject (manifest).wasOk());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("not listed in project 'sources'"));
}

TEST_F (YdspProjectTests, ImportedSemanticErrorsRetainFileAndRange)
{
    project ("Main", "[Main.ydsp, Library.ydsp]");
    write ("Main.ydsp", "import Library; graph Main { input stream in; output stream out; node p = Library.P; connection { in -> p.in; p.out -> out; } }");
    write ("Library.ydsp", "processor P { input stream in; output stream out; process { out = missing; } }");
    YdspCompiler compiler;
    EXPECT_FALSE (compiler.compileProject (manifest).wasOk());
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains (directory.getChildFile ("Library.ydsp").getFullPathName() + ":1:"));
    EXPECT_TRUE (compiler.getDiagnostics().toString().contains ("^~~~~~~"));
}

TEST_F (YdspProjectTests, ProcessorEntryExposesParameterDefaults)
{
    project ("Main", "[Main.ydsp]");
    write ("Main.ydsp", "processor Main { input stream in; output stream out; input parameter float gain = 2.0; process { out = in * gain; } }");
    YdspCompiler compiler;
    auto compiled = compiler.compileProject (manifest);
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    expectOutput (compiled.getReference(), 0.5f);
}


TEST_F (YdspProjectTests, ProcessorEntryExposesMetersAndEventInputs)
{
    project ("Main", "[Main.ydsp]");
    write ("Main.ydsp", "processor Main { input stream in; output stream out; input event midi; "
                       "output parameter float level; process { out = in; level = in; } }");
    YdspCompiler compiler;
    auto compiled = compiler.compileProject (manifest);
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    auto& graph = compiled.getReference();
    EXPECT_EQ (1, graph.getEventInputCount());
    EXPECT_EQ ("midi", graph.getEventInputName (0));
    expectOutput (graph, 0.25f);
    EXPECT_FLOAT_EQ (0.25f, graph.getOutputValue ("level"));
}


TEST_F (YdspProjectTests, NestedLibraryFunctionsGainOuterPrefixWithoutCapturingLocalFunctions)
{
    project ("Main", "[Main.ydsp, lib/Outer.ydsp, lib/Inner.ydsp]");
    write ("lib/Inner.ydsp", "func twice (x: float) : float { return x * 2.0; } "
                           "func wrap (x: float) : float { return twice (x); }");
    write ("lib/Outer.ydsp", "import Inner as inner; "
                           "func wrap (x: float) : float { return inner.wrap (x); } "
                           "processor Gain { input stream in; output stream out; "
                           "func wrap (x: float) : float { return x * 3.0; } "
                           "process { out = inner.wrap (in) + wrap (in); } }");
    const auto source = String ("import lib.Outer as outer; "
                                "processor Main { input stream in; output stream out; "
                                "process { out = outer.wrap (in); } } "
                                "graph Alternative { input stream in; output stream out; node p = outer.Gain; "
                                "connection { in -> p.in; p.out -> out; } }");
    write ("Main.ydsp", source);
    YdspCompiler compiler;
    auto compiled = compiler.compileProject (manifest);
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    expectOutput (compiled.getReference(), 0.5f);
    auto alternate = compiler.compileProject (manifest, {}, "Alternative");
    ASSERT_TRUE (alternate.wasOk()) << compiler.getDiagnostics().toString();
    expectOutput (alternate.getReference(), 1.25f);
    auto direct = compiler.compile (source, directory.getChildFile ("Main.ydsp").getFullPathName());
    ASSERT_TRUE (direct.wasOk()) << compiler.getDiagnostics().toString();
    expectOutput (direct.getReference(), 1.25f);
}
