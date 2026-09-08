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

#include <yup_audio_basics/yup_audio_basics.h>

#include <vector>

using namespace yup;

class TuningMapTests : public ::testing::Test
{
protected:
    void TearDown() override
    {
        for (auto& file : tempFiles)
            file.deleteFile();
    }

    File writeTempFile (const String& content)
    {
        auto file = File::createTempFile ("tuning_map_test");
        EXPECT_TRUE (file.replaceWithText (content));
        tempFiles.push_back (file);
        return file;
    }

    TuningMap map;
    std::vector<File> tempFiles;
};

TEST_F (TuningMapTests, DefaultTuningIsTwelveToneEqualTemperament)
{
    EXPECT_NEAR (map.noteToPitch (69), 440.0, 1e-6);
    EXPECT_NEAR (map.noteToPitch (60), 440.0 * std::pow (2.0, -9.0 / 12.0), 1e-6);
    EXPECT_NEAR (map.noteToPitch (81), 880.0, 1e-6);
    EXPECT_NEAR (map.noteToPitch (0), 440.0 * std::pow (2.0, -69.0 / 12.0), 1e-6);
    EXPECT_NEAR (map.noteToPitch (127), 440.0 * std::pow (2.0, 58.0 / 12.0), 1e-6);

    for (int note = 0; note < 128; ++note)
    {
        EXPECT_TRUE (map.isNoteMapped (note));
        EXPECT_TRUE (map.isNoteActive (note));
    }

    EXPECT_EQ (map.getZeroNote(), 0);
    EXPECT_EQ (map.getReferenceNote(), 69);
    EXPECT_EQ (map.getReferencePitch(), 440.0);
    EXPECT_EQ (map.getKeyMapRepeatIncrement(), 1);
    EXPECT_EQ (map.getNumberOfScaleDegrees(), 12);
    EXPECT_TRUE (map.getScaleFile().isEmpty());
    EXPECT_TRUE (map.getKeyMapFile().isEmpty());
}

TEST_F (TuningMapTests, DefaultTuningIsSemitoneConsecutive)
{
    for (int note = 1; note < 128; ++note)
        EXPECT_NEAR (map.noteToPitch (note) / map.noteToPitch (note - 1), std::pow (2.0, 1.0 / 12.0), 1e-9);
}

TEST_F (TuningMapTests, LoadingCentsScaleProducesEqualTemperament)
{
    auto file = writeTempFile (
        "! 12 tone equal temperament\n"
        "12-EDO\n"
        "12\n"
        "100.0\n"
        "200.0\n"
        "300.0\n"
        "400.0\n"
        "500.0\n"
        "600.0\n"
        "700.0\n"
        "800.0\n"
        "900.0\n"
        "1000.0\n"
        "1100.0\n"
        "1200.0\n");

    EXPECT_TRUE (map.loadScale (file).wasOk());
    EXPECT_EQ (map.getScaleFile(), file.getFullPathName());
    EXPECT_EQ (map.getNumberOfScaleDegrees(), 12);

    for (int note = 0; note < 128; ++note)
        EXPECT_NEAR (map.noteToPitch (note), 440.0 * std::pow (2.0, (note - 69) / 12.0), 1e-6);
}

TEST_F (TuningMapTests, LoadingRatioScaleMapsScaleDegreesToRatios)
{
    auto scaleFile = writeTempFile (
        "! A just intonation major pentatonic\n"
        "major pentatonic\n"
        "5\n"
        "9/8\n"
        "5/4\n"
        "3/2\n"
        "5/3\n"
        "2/1\n");

    auto keyMapFile = writeTempFile (
        "! chromatic key map over the pentatonic scale\n"
        "5\n"
        "0\n"
        "127\n"
        "0\n"
        "60\n"
        "440\n"
        "5\n"
        "0\n"
        "1\n"
        "2\n"
        "3\n"
        "4\n");

    EXPECT_TRUE (map.loadScale (scaleFile).wasOk());
    EXPECT_TRUE (map.loadKeyMap (keyMapFile).wasOk());

    EXPECT_EQ (map.getKeyMapFile(), keyMapFile.getFullPathName());
    EXPECT_EQ (map.getZeroNote(), 0);
    EXPECT_EQ (map.getReferenceNote(), 60);
    EXPECT_EQ (map.getReferencePitch(), 440.0);
    EXPECT_EQ (map.getKeyMapRepeatIncrement(), 5);
    EXPECT_EQ (map.getNumberOfScaleDegrees(), 5);

    // The tonic (note 60) carries the reference pitch, and each of the next
    // keys sounds the consecutive scale interval above it.
    EXPECT_NEAR (map.noteToPitch (60), 440.0, 1e-6);
    EXPECT_NEAR (map.noteToPitch (61), 440.0 * 9.0 / 8.0, 1e-6);
    EXPECT_NEAR (map.noteToPitch (62), 440.0 * 5.0 / 4.0, 1e-6);
    EXPECT_NEAR (map.noteToPitch (63), 440.0 * 3.0 / 2.0, 1e-6);
    EXPECT_NEAR (map.noteToPitch (64), 440.0 * 5.0 / 3.0, 1e-6);

    // Advancing one full pass through the key map (five keys) raises the
    // pitch by one octave.
    EXPECT_NEAR (map.noteToPitch (65), 880.0, 1e-6);
    EXPECT_NEAR (map.noteToPitch (59), 440.0 * 5.0 / 6.0, 1e-6);
}

TEST_F (TuningMapTests, AutomaticKeyMapMapsKeysToConsecutiveDegrees)
{
    auto file = writeTempFile (
        "! automatic linear key map\n"
        "0\n"
        "0\n"
        "127\n"
        "0\n"
        "69\n"
        "440\n"
        "0\n");

    EXPECT_TRUE (map.loadKeyMap (file).wasOk());

    EXPECT_EQ (map.getKeyMapRepeatIncrement(), 1);
    EXPECT_EQ (map.getZeroNote(), 0);

    for (int note = 0; note < 128; ++note)
        EXPECT_NEAR (map.noteToPitch (note), 440.0 * std::pow (2.0, (note - 69) / 12.0), 1e-6);
}

TEST_F (TuningMapTests, UnmappedKeysAreNotAudible)
{
    auto file = writeTempFile (
        "! key map with an unmapped key\n"
        "3\n"
        "0\n"
        "127\n"
        "60\n"
        "60\n"
        "440\n"
        "0\n"
        "0\n"
        "x\n"
        "2\n");

    EXPECT_TRUE (map.loadKeyMap (file).wasOk());

    EXPECT_TRUE (map.isNoteMapped (60));
    EXPECT_FALSE (map.isNoteMapped (61));
    EXPECT_TRUE (map.isNoteMapped (62));

    EXPECT_NEAR (map.noteToPitch (60), 440.0, 1e-6);
    EXPECT_LT (map.noteToPitch (61), 0.0);
    EXPECT_GE (map.noteToPitch (62), 0.0);

    // A repeat increment of zero selects the key map size.
    EXPECT_EQ (map.getKeyMapRepeatIncrement(), 3);
}

TEST_F (TuningMapTests, ActiveRangeReflectsDeclaredRanges)
{
    auto file = writeTempFile (
        "! key map declaring a playable range\n"
        "< 60 72\n"
        "12\n"
        "0\n"
        "127\n"
        "60\n"
        "69\n"
        "440\n"
        "12\n"
        "0\n"
        "1\n"
        "2\n"
        "3\n"
        "4\n"
        "5\n"
        "6\n"
        "7\n"
        "8\n"
        "9\n"
        "10\n"
        "11\n");

    EXPECT_TRUE (map.loadKeyMap (file).wasOk());

    EXPECT_EQ (map.getZeroNote(), 60);
    EXPECT_EQ (map.getReferenceNote(), 69);
    EXPECT_EQ (map.getKeyMapRepeatIncrement(), 12);

    EXPECT_TRUE (map.isNoteActive (60));
    EXPECT_TRUE (map.isNoteActive (72));
    EXPECT_FALSE (map.isNoteActive (59));
    EXPECT_FALSE (map.isNoteActive (73));
    EXPECT_FALSE (map.isNoteActive (0));

    // A note outside the declared range can still be mapped.
    EXPECT_TRUE (map.isNoteMapped (59));
    EXPECT_FALSE (map.isNoteActive (59));
}

TEST_F (TuningMapTests, MissingAndExtraKeyMapEntriesAreLenient)
{
    // Extra entries beyond the declared map size are dropped.
    auto withExtras = writeTempFile (
        "! key map with extra entries\n"
        "2\n"
        "0\n"
        "127\n"
        "60\n"
        "60\n"
        "440\n"
        "1\n"
        "0\n"
        "1\n"
        "2\n");

    EXPECT_TRUE (map.loadKeyMap (withExtras).wasOk());

    EXPECT_NEAR (map.noteToPitch (60), 440.0, 1e-6);
    EXPECT_NEAR (map.noteToPitch (61), 440.0 * std::pow (2.0, 1.0 / 12.0), 1e-6);
    EXPECT_NEAR (map.noteToPitch (62), map.noteToPitch (61), 1e-9);

    // Keys that are never listed after the header fields stay unmapped.
    auto withMissing = writeTempFile (
        "! key map with a missing entry\n"
        "2\n"
        "0\n"
        "127\n"
        "60\n"
        "60\n"
        "440\n"
        "1\n"
        "0\n");

    EXPECT_TRUE (map.loadKeyMap (withMissing).wasOk());

    EXPECT_NEAR (map.noteToPitch (60), 440.0, 1e-6);
    EXPECT_FALSE (map.isNoteMapped (61));
    EXPECT_LT (map.noteToPitch (61), 0.0);
}

TEST_F (TuningMapTests, OversizedKeyMapIsRejected)
{
    auto file = writeTempFile (
        "! key map spanning more notes than the MIDI range\n"
        "129\n"
        "0\n"
        "127\n"
        "0\n"
        "69\n"
        "440\n"
        "1\n");

    EXPECT_TRUE (map.loadKeyMap (file).failed());
    EXPECT_NEAR (map.noteToPitch (69), 440.0, 1e-6);
}

TEST_F (TuningMapTests, FailedScaleLoadKeepsPreviousTuning)
{
    auto scaleFile = writeTempFile (
        "major pentatonic\n"
        "5\n"
        "9/8\n"
        "5/4\n"
        "3/2\n"
        "5/3\n"
        "2/1\n");

    EXPECT_TRUE (map.loadScale (scaleFile).wasOk());
    EXPECT_EQ (map.getNumberOfScaleDegrees(), 5);

    auto badFile = writeTempFile (
        "broken scale\n"
        "5\n"
        "9/8\n"
        "5/4\n");

    EXPECT_TRUE (map.loadScale (badFile).failed());

    EXPECT_EQ (map.getNumberOfScaleDegrees(), 5);
    EXPECT_NEAR (map.noteToPitch (60), 440.0, 1e-6);
    EXPECT_NEAR (map.noteToPitch (61), 440.0 * 9.0 / 8.0, 1e-6);
    EXPECT_EQ (map.getScaleFile(), scaleFile.getFullPathName());
}

TEST_F (TuningMapTests, FailedKeyMapLoadKeepsPreviousTuning)
{
    auto keyMapFile = writeTempFile (
        "! valid key map\n"
        "1\n"
        "0\n"
        "127\n"
        "60\n"
        "60\n"
        "440\n"
        "1\n"
        "0\n");

    EXPECT_TRUE (map.loadKeyMap (keyMapFile).wasOk());
    EXPECT_NEAR (map.noteToPitch (60), 440.0, 1e-6);

    auto badFile = writeTempFile (
        "! key map leaving the reference note unmapped\n"
        "1\n"
        "0\n"
        "127\n"
        "60\n"
        "60\n"
        "440\n"
        "1\n"
        "x\n");

    EXPECT_TRUE (map.loadKeyMap (badFile).failed());
    EXPECT_TRUE (map.loadKeyMap (File::getCurrentWorkingDirectory().getChildFile ("missing_key_map.kbm")).failed());

    EXPECT_NEAR (map.noteToPitch (60), 440.0, 1e-6);
    EXPECT_EQ (map.getKeyMapFile(), keyMapFile.getFullPathName());
}

TEST_F (TuningMapTests, InvalidScaleAndKeyMapFilesAreRejected)
{
    EXPECT_TRUE (map.loadScale (File::getCurrentWorkingDirectory().getChildFile ("missing_scale.scl")).failed());
    EXPECT_TRUE (map.loadScale (writeTempFile ("just a description\n")).failed());

    auto invalidInterval = writeTempFile (
        "bad interval\n"
        "1\n"
        "banana\n");
    EXPECT_TRUE (map.loadScale (invalidInterval).failed());

    auto invalidRange = writeTempFile (
        "! invalid active range\n"
        "< 200 300\n"
        "1\n"
        "0\n"
        "127\n"
        "0\n"
        "69\n"
        "440\n"
        "1\n"
        "0\n");
    EXPECT_TRUE (map.loadKeyMap (invalidRange).failed());

    auto truncated = writeTempFile (
        "! truncated key map\n"
        "2\n"
        "0\n"
        "127\n");
    EXPECT_TRUE (map.loadKeyMap (truncated).failed());

    EXPECT_NEAR (map.noteToPitch (69), 440.0, 1e-6);
}
