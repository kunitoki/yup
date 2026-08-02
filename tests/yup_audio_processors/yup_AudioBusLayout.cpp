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
