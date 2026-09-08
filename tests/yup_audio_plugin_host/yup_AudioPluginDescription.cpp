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

#include <yup_audio_plugin_host/yup_audio_plugin_host.h>

using namespace yup;

TEST (AudioPluginDescriptionTests, DefaultFormatTypeIsUnknown)
{
    AudioPluginDescription desc;
    EXPECT_EQ (AudioPluginFormatType::unknown, desc.formatType);
}

TEST (AudioPluginDescriptionTests, IsInstrumentDefaultsFalse)
{
    AudioPluginDescription desc;
    EXPECT_FALSE (desc.isInstrument);
}

TEST (AudioPluginDescriptionTests, MidiPortCountsDefaultToZero)
{
    AudioPluginDescription desc;
    EXPECT_EQ (0, desc.numMidiInputPorts);
    EXPECT_EQ (0, desc.numMidiOutputPorts);
}

TEST (AudioPluginDescriptionTests, EqualityMatchesAllFields)
{
    AudioPluginDescription a;
    a.name = "Synth";
    a.identifier = "com.vendor.synth";
    a.formatType = AudioPluginFormatType::vst3;

    AudioPluginDescription b = a;
    EXPECT_EQ (a, b);

    b.version = "2.0";
    EXPECT_NE (a, b);
}

TEST (AudioPluginHostContextTests, DefaultSampleRate)
{
    AudioPluginHostContext ctx;
    EXPECT_FLOAT_EQ (44100.0f, ctx.sampleRate);
}

TEST (AudioPluginHostContextTests, DefaultMaxBlockSize)
{
    AudioPluginHostContext ctx;
    EXPECT_EQ (512, ctx.maxBlockSize);
}

TEST (AudioPluginHostContextTests, DefaultsToRealtimeRendering)
{
    AudioPluginHostContext ctx;
    EXPECT_FALSE (ctx.isNonRealtime);
}

TEST (AudioPluginDescriptionTests, DefaultStringsAreEmpty)
{
    AudioPluginDescription desc;
    EXPECT_TRUE (desc.name.isEmpty());
    EXPECT_TRUE (desc.identifier.isEmpty());
    EXPECT_TRUE (desc.vendor.isEmpty());
    EXPECT_TRUE (desc.version.isEmpty());
    EXPECT_TRUE (desc.category.isEmpty());
    EXPECT_TRUE (desc.fileOrBundlePath.isEmpty());
}

TEST (AudioPluginDescriptionTests, DefaultChannelCountsAreZero)
{
    AudioPluginDescription desc;
    EXPECT_EQ (0, desc.numInputChannels);
    EXPECT_EQ (0, desc.numOutputChannels);
}

TEST (AudioPluginDescriptionTests, IsEffectDefaultsFalse)
{
    AudioPluginDescription desc;
    EXPECT_FALSE (desc.isEffect);
}

TEST (AudioPluginDescriptionTests, EqualityRequiresMatchingFormatType)
{
    AudioPluginDescription a, b;
    a.name = "Plugin";
    a.identifier = "com.vendor.plugin";
    a.formatType = AudioPluginFormatType::vst3;

    b = a;
    b.formatType = AudioPluginFormatType::unknown;
    EXPECT_NE (a, b);
}

TEST (AudioPluginDescriptionTests, CopyConstructorPreservesAllFields)
{
    AudioPluginDescription a;
    a.name = "Piano";
    a.vendor = "Vendor";
    a.version = "1.0";
    a.category = "Instrument";
    a.identifier = "com.vendor.piano";
    a.fileOrBundlePath = "/path/to/plugin.vst3";
    a.isInstrument = true;
    a.isEffect = false;
    a.numInputChannels = 2;
    a.numOutputChannels = 2;
    a.numMidiInputPorts = 1;
    a.numMidiOutputPorts = 0;
    a.formatType = AudioPluginFormatType::vst3;

    AudioPluginDescription b = a;
    EXPECT_EQ (a, b);
    EXPECT_EQ ("Piano", b.name);
    EXPECT_EQ ("Vendor", b.vendor);
    EXPECT_TRUE (b.isInstrument);
    EXPECT_EQ (2, b.numInputChannels);
}

TEST (AudioPluginHostContextTests, CustomSampleRateIsStored)
{
    AudioPluginHostContext ctx;
    ctx.sampleRate = 48000.0f;
    EXPECT_FLOAT_EQ (48000.0f, ctx.sampleRate);
}

TEST (AudioPluginHostContextTests, NonRealtimeModeCanBeSet)
{
    AudioPluginHostContext ctx;
    ctx.isNonRealtime = true;
    EXPECT_TRUE (ctx.isNonRealtime);
}
