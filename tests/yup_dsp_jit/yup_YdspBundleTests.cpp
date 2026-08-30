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

#include <gtest/gtest.h>

#include <yup_dsp_jit/yup_dsp_jit.h>

using namespace yup;

namespace
{

const char* simplePatch = R"YDSP(
    processor Passthrough
    {
        input stream in;
        output stream out;
        process { out = in; }
    }

    graph Main
    {
        input stream in;
        output stream out;
        node p = Passthrough;
        connection
        {
            in -> p.in;
            p.out -> out;
        }
    }
)YDSP";

} // namespace

TEST (YdspBundleTests, CompileAndLoadMemoryRoundTrip)
{
    YdspCompiler compiler;
    YdspBundleCompileOptions options;
    options.nativeTargets.push_back ({ YdspTargetOperatingSystem::macos, YdspTargetArchitecture::arm64 });

    auto compiled = compiler.compileBundle (simplePatch, options);
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());

    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_EQ (1u, loaded.getReference().getSources().size());
    ASSERT_EQ (1, loaded.getReference().getNativeTargets().size());
    EXPECT_EQ ("macos-arm64", loaded.getReference().getNativeTargets()[0]);

    auto graph = loaded.getReference().instantiate();
    ASSERT_TRUE (graph.wasOk()) << graph.getErrorMessage();
    EXPECT_TRUE (graph.getReference().isValid());
}

TEST (YdspBundleTests, FileRoundTripPreservesBundle)
{
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (simplePatch, {});
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();

    const auto file = File::createTempFile ("ydsp-bundle.ydsb");
    ASSERT_TRUE (compiled.getReference().saveToFile (file).wasOk());

    auto loaded = YdspBundle::loadFromFile (file);
    EXPECT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    if (loaded.wasOk())
        EXPECT_EQ (compiled.getReference().getSources()[0].source,
                   loaded.getReference().getSources()[0].source);
    file.deleteFile();
}

TEST (YdspBundleTests, RejectsCorruptHeader)
{
    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (simplePatch, {});
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    static_cast<uint8_t*> (bytes.getData())[0] ^= 0xff;

    EXPECT_TRUE (YdspBundle::loadFromMemoryBlock (bytes).failed());
}

TEST (YdspBundleTests, StoresImportedSourceClosure)
{
    const auto directory = File::getSpecialLocation (File::tempDirectory).getChildFile ("yup_ydsp_bundle_import_test");
    directory.deleteRecursively();
    ASSERT_TRUE (directory.getChildFile ("fx").createDirectory());

    directory.getChildFile ("fx/Gain.ydsp").replaceWithText (
        "processor Gain { input stream in; output stream out; process { out = in; } }\n");

    const auto source = R"YDSP(
        import fx.Gain as fx;
        graph Main {
            input stream in;
            output stream out;
            node gain = fx.Gain;
            connection { in -> gain.in; gain.out -> out; }
        }
    )YDSP";

    YdspCompiler compiler;
    auto compiled = compiler.compileBundle (source, {}, directory.getChildFile ("Main.ydsp").getFullPathName());
    ASSERT_TRUE (compiled.wasOk()) << compiler.getDiagnostics().toString();
    ASSERT_EQ (2u, compiled.getReference().getSources().size());
    EXPECT_EQ ("source-0", compiled.getReference().getSources()[0].id);
    EXPECT_TRUE (compiled.getReference().getSources()[0].isRoot);
    EXPECT_EQ ("source-1", compiled.getReference().getSources()[1].id);
    EXPECT_FALSE (compiled.getReference().getSources()[1].isRoot);

    MemoryBlock bytes;
    ASSERT_TRUE (compiled.getReference().saveToMemoryBlock (bytes).wasOk());
    auto loaded = YdspBundle::loadFromMemoryBlock (bytes);
    ASSERT_TRUE (loaded.wasOk()) << loaded.getErrorMessage();
    EXPECT_EQ (2u, loaded.getReference().getSources().size());
    directory.deleteRecursively();
}
