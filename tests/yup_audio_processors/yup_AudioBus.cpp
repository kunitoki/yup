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

TEST (AudioBusTests, DefaultRoleIsMain)
{
    AudioBus bus ("Main Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2);
    EXPECT_EQ (AudioBus::Role::Main, bus.getRole());
}

TEST (AudioBusTests, DefaultIsActive)
{
    AudioBus bus ("Main Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2);
    EXPECT_TRUE (bus.isDefaultActive());
}

TEST (AudioBusTests, ExplicitRoleAuxiliary)
{
    AudioBus bus ("Sidechain", AudioBus::Type::Audio, AudioBus::Direction::Input, 2, AudioBus::Role::Auxiliary);
    EXPECT_EQ (AudioBus::Role::Auxiliary, bus.getRole());
}

TEST (AudioBusTests, ExplicitNotDefaultActive)
{
    AudioBus bus ("Sidechain", AudioBus::Type::Audio, AudioBus::Direction::Input, 2, AudioBus::Role::Auxiliary, false);
    EXPECT_FALSE (bus.isDefaultActive());
}

TEST (AudioBusTests, RoleDoesNotAffectOtherProperties)
{
    AudioBus bus ("Sidechain", AudioBus::Type::Audio, AudioBus::Direction::Input, 2, AudioBus::Role::Auxiliary, false);
    EXPECT_EQ ("Sidechain", bus.getName());
    EXPECT_EQ (AudioBus::Type::Audio, bus.getType());
    EXPECT_EQ (AudioBus::Direction::Input, bus.getDirection());
    EXPECT_EQ (2, bus.getNumChannels());
    EXPECT_TRUE (bus.isStereo());
}

TEST (AudioBusTests, BackwardCompatibleConstructors)
{
    // Old-style constructors (no Role, no isDefaultActive) should compile and default correctly
    AudioBus bus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 1);
    EXPECT_EQ (AudioBus::Role::Main, bus.getRole());
    EXPECT_TRUE (bus.isDefaultActive());
    EXPECT_TRUE (bus.isMono());
}

TEST (AudioBusTests, MainOutputBus)
{
    AudioBus bus ("Main Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2);
    EXPECT_EQ (AudioBus::Role::Main, bus.getRole());
    EXPECT_EQ (AudioBus::Direction::Output, bus.getDirection());
}

TEST (AudioBusTests, MidiBusRoleIsMainByDefault)
{
    AudioBus bus ("MIDI In", AudioBus::Type::Midi, AudioBus::Direction::Input, 0);
    EXPECT_EQ (AudioBus::Role::Main, bus.getRole());
    EXPECT_TRUE (bus.isDefaultActive());
}
