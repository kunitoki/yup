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

#include <algorithm>
#include <cmath>
#include <vector>

namespace yup::test
{

namespace
{

/*
  The demo patches under examples/graphics/data/synths/ are the corpus that
  exercises the language's surface - one patch per feature cluster - and until
  now nothing but a human clicking through the graphics demo's combo box ever
  compiled them. Every other reference to them in this suite is a hand-copied
  excerpt, which cannot go stale loudly.

  So this fixture walks the shipped folder and compiles every file in it. It
  reaches outside tests/ on purpose: the point is to compile the files that ship,
  not a copy of them.
*/
File exampleSynthsFolder()
{
    return File (__FILE__)
        .getParentDirectory() // tests/yup_dsp_jit
        .getParentDirectory() // tests
        .getParentDirectory() // repository root
        .getChildFile ("examples")
        .getChildFile ("graphics")
        .getChildFile ("data")
        .getChildFile ("synths");
}

} // namespace

//==============================================================================

class YdspExamplePatchTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // On wasm the tests run against a preloaded virtual filesystem that
        // carries tests/data only, so the example folder is not reachable.
        if (! exampleSynthsFolder().isDirectory())
            GTEST_SKIP() << "example synth folder not available on this platform";
    }

    // Compiles one patch with its own path as the import base, which is what the
    // demo app does, so a patch's relative `import fx.Delay` resolves.
    DspJitGraph compilePatch (const File& patchFile, DspJitCompiler& compiler)
    {
        auto result = compiler.compile (patchFile.loadFileAsString(), patchFile.getFullPathName());

        EXPECT_TRUE (result.wasOk())
            << patchFile.getFileName() << ":\n"
            << compiler.getDiagnostics().toString();

        if (! result.wasOk())
            return DspJitGraph {};

        return std::move (result).getValue();
    }

    std::vector<File> patchFiles() const
    {
        std::vector<File> files;

        for (DirectoryEntry entry : RangedDirectoryIterator (exampleSynthsFolder(), false, "*.ydsp", File::findFiles))
            if (! entry.isDirectory() && ! entry.isHidden())
                files.push_back (entry.getFile());

        std::sort (files.begin(), files.end(), [] (const File& a, const File& b)
        {
            return a.getFileName().toStdString() < b.getFileName().toStdString();
        });

        return files;
    }
};

//==============================================================================

TEST_F (YdspExamplePatchTests, EveryPatchCompilesAndExposesParamsAndOutputsAndRendersFine)
{
    EXPECT_GE (patchFiles().size(), 10u);

    constexpr int blockSize = 128;
    constexpr double sampleRate = 48000.0;

    for (const auto& patchFile : patchFiles())
    {
        DspJitCompiler compiler;
        auto graph = compilePatch (patchFile, compiler);

        EXPECT_TRUE (graph.isValid()) << patchFile.getFileName();

        if (! graph.isValid())
            continue;

        EXPECT_GT (graph.getParameterCount(), 0) << patchFile.getFileName();
        EXPECT_EQ (graph.getInputStreamCount(), 0) << patchFile.getFileName();

        const auto isMidiOnlyPatch = patchFile.getFileName() == "ArpTranspose.ydsp";

        if (! isMidiOnlyPatch)
            EXPECT_GE (graph.getOutputStreamCount(), 1) << patchFile.getFileName();

        EXPECT_LE (graph.getOutputStreamCount(), 2) << patchFile.getFileName();

        graph.prepare (sampleRate, blockSize);

        const auto numOutputs = graph.getOutputStreamCount();

        std::vector<float> left (blockSize, 0.0f);
        std::vector<float> right (blockSize, 0.0f);

        DspJitOutputBuffer outputs[] = {
            Span<float> (left.data(), left.size()),
            Span<float> (right.data(), right.size())
        };

        MidiBuffer midi;
        midi.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

        int nonFinite = 0;

        for (int block = 0; block < 8; ++block)
        {
            graph.process ({},
                           Span<DspJitOutputBuffer> (outputs, static_cast<size_t> (numOutputs)),
                           blockSize,
                           &midi,
                           nullptr,
                           0);

            midi.clear();

            for (int channel = 0; channel < numOutputs; ++channel)
            {
                const auto& buffer = channel == 0 ? left : right;

                for (int i = 0; i < blockSize; ++i)
                    if (! std::isfinite (buffer[static_cast<size_t> (i)]))
                        ++nonFinite;
            }
        }

        EXPECT_EQ (nonFinite, 0) << patchFile.getFileName() << " produced non-finite samples";
    }
}

} // namespace yup::test
