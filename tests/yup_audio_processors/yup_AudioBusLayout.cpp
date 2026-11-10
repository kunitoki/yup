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

#include <yup_audio_processors/yup_audio_processors.h>

using namespace yup;

//==============================================================================

TEST (AudioBusLayoutTests, GetAudioBusRoleReturnsMainForFirstAudioBus)
{
    AudioBusLayout layout (
        { AudioBus ("Main", AudioBus::Type::Audio, AudioBus::Direction::Input, 2),
          AudioBus ("Sidechain", AudioBus::Type::Audio, AudioBus::Direction::Input, 2, AudioBus::Role::Auxiliary) },
        { AudioBus ("Main Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });

    EXPECT_EQ (AudioBus::Role::Main, layout.getAudioBusRole (0, true));
}

TEST (AudioBusLayoutTests, GetAudioBusRoleReturnsAuxiliaryForSecondAudioBus)
{
    AudioBusLayout layout (
        { AudioBus ("Main", AudioBus::Type::Audio, AudioBus::Direction::Input, 2),
          AudioBus ("Sidechain", AudioBus::Type::Audio, AudioBus::Direction::Input, 2, AudioBus::Role::Auxiliary) },
        {});

    EXPECT_EQ (AudioBus::Role::Auxiliary, layout.getAudioBusRole (1, true));
}

TEST (AudioBusLayoutTests, GetAudioBusRoleSkipsMidiBuses)
{
    AudioBusLayout layout (
        { AudioBus ("Main", AudioBus::Type::Audio, AudioBus::Direction::Input, 2),
          AudioBus ("MIDI In", AudioBus::Type::Midi, AudioBus::Direction::Input, 0),
          AudioBus ("Sidechain", AudioBus::Type::Audio, AudioBus::Direction::Input, 2, AudioBus::Role::Auxiliary) },
        {});

    // MIDI buses do not consume an audio-bus index
    EXPECT_EQ (AudioBus::Role::Main, layout.getAudioBusRole (0, true));
    EXPECT_EQ (AudioBus::Role::Auxiliary, layout.getAudioBusRole (1, true));
}

TEST (AudioBusLayoutTests, GetAudioBusRoleReturnsMainForOutOfRange)
{
    AudioBusLayout layout (
        { AudioBus ("Main", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
        {});

    EXPECT_EQ (AudioBus::Role::Main, layout.getAudioBusRole (5, true));
    EXPECT_EQ (AudioBus::Role::Main, layout.getAudioBusRole (-1, true));
}

TEST (AudioBusLayoutTests, GetAudioBusRoleReturnsMainForEmptyLayout)
{
    AudioBusLayout layout;

    EXPECT_EQ (AudioBus::Role::Main, layout.getAudioBusRole (0, true));
    EXPECT_EQ (AudioBus::Role::Main, layout.getAudioBusRole (0, false));
}

TEST (AudioBusLayoutTests, GetAudioBusRoleForOutputBuses)
{
    AudioBusLayout layout (
        {},
        { AudioBus ("Main Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2),
          AudioBus ("Aux Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2, AudioBus::Role::Auxiliary) });

    EXPECT_EQ (AudioBus::Role::Main, layout.getAudioBusRole (0, false));
    EXPECT_EQ (AudioBus::Role::Auxiliary, layout.getAudioBusRole (1, false));
    EXPECT_EQ (AudioBus::Role::Main, layout.getAudioBusRole (0, true));
}

TEST (AudioBusLayoutTests, CopyAssignmentAndEqualityCompareAllBuses)
{
    const AudioBusLayout stereo ({ AudioBus ("In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                 { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });

    AudioBusLayout copy;
    EXPECT_FALSE (copy == stereo);

    copy = stereo;
    EXPECT_TRUE (copy == stereo);
    EXPECT_FALSE (copy.withAudioBusNumChannels (0, false, 1) == stereo);
}

TEST (AudioBusLayoutTests, HasSameBusStructureIgnoresChannelCountsAndNames)
{
    const AudioBusLayout layout ({ AudioBus ("In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                 { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });

    const AudioBusLayout renamed ({ AudioBus ("Main In", AudioBus::Type::Audio, AudioBus::Direction::Input, 1) },
                                  { AudioBus ("Main Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 6) });

    const AudioBusLayout auxiliaryInput ({ AudioBus ("In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2, AudioBus::Role::Auxiliary) },
                                         { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });

    const AudioBusLayout midiInput ({ AudioBus ("In", AudioBus::Type::Midi, AudioBus::Direction::Input, 0) },
                                    { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });

    const AudioBusLayout extraOutput ({ AudioBus ("In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                      { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2),
                                        AudioBus ("Aux", AudioBus::Type::Audio, AudioBus::Direction::Output, 2, AudioBus::Role::Auxiliary) });

    EXPECT_TRUE (layout.hasSameBusStructure (renamed));
    EXPECT_FALSE (layout.hasSameBusStructure (auxiliaryInput));
    EXPECT_FALSE (layout.hasSameBusStructure (midiInput));
    EXPECT_FALSE (layout.hasSameBusStructure (extraOutput));
    EXPECT_FALSE (layout.hasSameBusStructure (AudioBusLayout()));
}

TEST (AudioBusLayoutTests, WithAudioBusChannelSetReplacesOnlyTheTargetAudioBus)
{
    const AudioBusLayout layout ({ AudioBus ("MIDI In", AudioBus::Type::Midi, AudioBus::Direction::Input, 0),
                                   AudioBus ("In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                 { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });

    const auto surround = layout.withAudioBusChannelSet (0, false, AudioChannelSet::create5point1());

    EXPECT_EQ (6, surround.getAudioBusNumChannels (0, false));
    EXPECT_EQ (AudioChannelSet::create5point1(), surround.getAudioBusChannelSet (0, false));
    EXPECT_EQ (2, surround.getAudioBusNumChannels (0, true));
    EXPECT_TRUE (surround.hasSameBusStructure (layout));
}

TEST (AudioBusLayoutTests, OutOfRangeAudioBusQueriesAndEditsAreHarmless)
{
    const AudioBusLayout layout ({ AudioBus ("In", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                                 { AudioBus ("Out", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });

    EXPECT_TRUE (layout.withAudioBusChannelSet (3, true, AudioChannelSet::mono()) == layout);
    EXPECT_TRUE (layout.withAudioBusNumChannels (3, false, 1) == layout);
    EXPECT_EQ (0, layout.getAudioBusNumChannels (5, true));
    EXPECT_EQ (AudioChannelSet::disabled(), layout.getAudioBusChannelSet (5, true));
}
