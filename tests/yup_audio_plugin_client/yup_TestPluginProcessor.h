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

#pragma once

#include <yup_audio_processors/yup_audio_processors.h>

//==============================================================================
/** Shared test processor used by all plugin wrapper test TUs.

    Provides a known set of parameters so that per-format tests can verify
    correct parameter mapping, state serialization, and audio processing
    through the wrapper code.

    The bus layout is configurable — each test TU can supply its own layout
    (e.g. 1-in/1-out for AU safety, 2-in/2-out for VST3 multi-channel tests).
*/
class TestPluginProcessor final : public yup::AudioProcessor
{
public:
    /** Constructs with a custom bus layout. */
    explicit TestPluginProcessor (yup::AudioBusLayout layout)
        : AudioProcessor ("TestPlugin", std::move (layout))
    {
        // Float parameter, automatable, modulatable — host ID 100
        auto floatMeta = yup::AudioParameter::Metadata {};
        floatMeta.name = "Gain";
        floatMeta.hostParameterID = 100;
        floatMeta.valueRange = { 0.0f, 1.0f };
        floatMeta.defaultValue = 0.5f;
        floatMeta.setModulatable (true);
        addParameter (new yup::AudioParameter ("gain", floatMeta));

        // Stepped integer parameter, enumerated — host ID 200
        auto steppedMeta = yup::AudioParameter::Metadata {};
        steppedMeta.name = "Mode";
        steppedMeta.hostParameterID = 200;
        steppedMeta.valueRange = { 0.0f, 4.0f, 1.0f };
        steppedMeta.defaultValue = 0.0f;
        steppedMeta.setStepped (true);
        steppedMeta.setEnum (true);
        addParameter (new yup::AudioParameter ("mode", steppedMeta));

        // Read-only parameter (meter) — host ID 300
        auto readOnlyMeta = yup::AudioParameter::Metadata {};
        readOnlyMeta.name = "Meter";
        readOnlyMeta.hostParameterID = 300;
        readOnlyMeta.valueRange = { -60.0f, 0.0f };
        readOnlyMeta.defaultValue = -60.0f;
        readOnlyMeta.setReadOnly (true);
        readOnlyMeta.setAutomatable (false);
        addParameter (new yup::AudioParameter ("meter", readOnlyMeta));

        // Non-automatable parameter — host ID 400
        auto nonAutoMeta = yup::AudioParameter::Metadata {};
        nonAutoMeta.name = "Internal";
        nonAutoMeta.hostParameterID = 400;
        nonAutoMeta.valueRange = { 0.0f, 100.0f };
        nonAutoMeta.defaultValue = 50.0f;
        nonAutoMeta.setAutomatable (false);
        addParameter (new yup::AudioParameter ("internal", nonAutoMeta));
    }

    void prepareToPlay (const yup::AudioSpec&) override { prepared = true; }

    void releaseResources() override { prepared = false; }

    void processBlock (yup::AudioProcessContext<float>&) override { ++processCallCount; }

    void processBlockBypassed (yup::AudioProcessContext<float>&) override { ++bypassCallCount; }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    yup::Result loadStateFromMemory (const yup::MemoryBlock& block) override
    {
        lastLoadedState = block;
        return yup::Result::ok();
    }

    yup::Result saveStateIntoMemory (yup::MemoryBlock& block) override
    {
        block = lastSavedState;
        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    bool supportsDoublePrecisionProcessing() const override { return supportsDouble; }

    bool prepared = false;
    bool supportsDouble = false;
    int processCallCount = 0;
    int bypassCallCount = 0;
    yup::MemoryBlock lastLoadedState;
    yup::MemoryBlock lastSavedState;
};

//==============================================================================
/** Convenience: a 1-in/1-out layout safe for AU wrappers. */
inline yup::AudioBusLayout testPluginBusLayoutMono()
{
    return yup::AudioBusLayout (
        { yup::AudioBus ("Input", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Input, 1) },
        { yup::AudioBus ("Output", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Output, 1) });
}

/** Convenience: a 2-in/2-out layout for multi-channel tests. */
inline yup::AudioBusLayout testPluginBusLayoutStereo()
{
    return yup::AudioBusLayout (
        { yup::AudioBus ("Input", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Input, 2) },
        { yup::AudioBus ("Output", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Output, 2) });
}

/** Convenience: a sidechain layout with 2 main inputs, 1 sidechain input, 2 main outputs.
    The sidechain bus is auxiliary and active by default. */
inline yup::AudioBusLayout testPluginBusLayoutWithSidechain()
{
    return yup::AudioBusLayout (
        { yup::AudioBus ("Main Input", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Input, 2),
          yup::AudioBus ("Sidechain Input", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Input, 1, yup::AudioBus::Role::Auxiliary) },
        { yup::AudioBus ("Main Output", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Output, 2) });
}

/** Convenience: a sidechain layout where the auxiliary bus is NOT active by default. */
inline yup::AudioBusLayout testPluginBusLayoutWithInactiveSidechain()
{
    return yup::AudioBusLayout (
        { yup::AudioBus ("Main Input", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Input, 2),
          yup::AudioBus ("Sidechain Input", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Input, 1, yup::AudioBus::Role::Auxiliary, false) },
        { yup::AudioBus ("Main Output", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Output, 2) });
}

/** Convenience: a layout with an auxiliary output bus. */
inline yup::AudioBusLayout testPluginBusLayoutWithAuxOutput()
{
    return yup::AudioBusLayout (
        { yup::AudioBus ("Main Input", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Input, 2) },
        { yup::AudioBus ("Main Output", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Output, 2),
          yup::AudioBus ("Aux Output", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Output, 2, yup::AudioBus::Role::Auxiliary) });
}
