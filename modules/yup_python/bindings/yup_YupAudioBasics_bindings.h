/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

#if ! YUP_MODULE_AVAILABLE_yup_audio_basics
#error This binding file requires adding the yup_audio_basics module in the project
#else
#include <yup_audio_basics/yup_audio_basics.h>
#endif

#include "yup_YupCore_bindings.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_STL
#include "../utilities/yup_PyBind11Includes.h"

namespace yup::Bindings
{

//==============================================================================

void registerYupAudioBasicsBindings (pybind11::module_& m);

//==============================================================================

template <class Base = AudioSource>
struct PyAudioSource : Base
{
    using Base::Base;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, prepareToPlay, samplesPerBlockExpected, sampleRate);
    }

    void releaseResources() override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, releaseResources);
    }

    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, getNextAudioBlock, bufferToFill);
    }
};

//==============================================================================

template <class Base = PositionableAudioSource>
struct PyPositionableAudioSource : PyAudioSource<Base>
{
    using PyAudioSource<Base>::PyAudioSource;

    void setNextReadPosition (int64 newPosition) override
    {
        PYBIND11_OVERRIDE_PURE (void, Base, setNextReadPosition, newPosition);
    }

    int64 getNextReadPosition() const override
    {
        PYBIND11_OVERRIDE_PURE (int64, Base, getNextReadPosition);
    }

    int64 getTotalLength() const override
    {
        PYBIND11_OVERRIDE_PURE (int64, Base, getTotalLength);
    }

    bool isLooping() const override
    {
        PYBIND11_OVERRIDE_PURE (bool, Base, isLooping);
    }

    void setLooping (bool shouldLoop) override
    {
        PYBIND11_OVERRIDE (void, Base, setLooping, shouldLoop);
    }
};

//==============================================================================

struct PySynthesiserSound : SynthesiserSound
{
    bool appliesToNote (int midiNoteNumber) override
    {
        PYBIND11_OVERRIDE_PURE (bool, SynthesiserSound, appliesToNote, midiNoteNumber);
    }

    bool appliesToChannel (int midiChannel) override
    {
        PYBIND11_OVERRIDE_PURE (bool, SynthesiserSound, appliesToChannel, midiChannel);
    }
};

//==============================================================================

struct PySynthesiserVoice : SynthesiserVoice
{
    bool canPlaySound (SynthesiserSound* sound) override
    {
        PYBIND11_OVERRIDE_PURE (bool, SynthesiserVoice, canPlaySound, sound);
    }

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition) override
    {
        PYBIND11_OVERRIDE_PURE (void, SynthesiserVoice, startNote, midiNoteNumber, velocity, sound, currentPitchWheelPosition);
    }

    void stopNote (float velocity, bool allowTailOff) override
    {
        PYBIND11_OVERRIDE_PURE (void, SynthesiserVoice, stopNote, velocity, allowTailOff);
    }

    void pitchWheelMoved (int newPitchWheelValue) override
    {
        PYBIND11_OVERRIDE_PURE (void, SynthesiserVoice, pitchWheelMoved, newPitchWheelValue);
    }

    void controllerMoved (int controllerNumber, int newControllerValue) override
    {
        PYBIND11_OVERRIDE_PURE (void, SynthesiserVoice, controllerMoved, controllerNumber, newControllerValue);
    }

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        PYBIND11_OVERRIDE_PURE (void, SynthesiserVoice, renderNextBlock, outputBuffer, startSample, numSamples);
    }

    void setCurrentPlaybackSampleRate (double newRate) override
    {
        PYBIND11_OVERRIDE (void, SynthesiserVoice, setCurrentPlaybackSampleRate, newRate);
    }

    bool isVoiceActive() const override
    {
        PYBIND11_OVERRIDE (bool, SynthesiserVoice, isVoiceActive);
    }
};

//==============================================================================
struct PySynthesiser : Synthesiser
{
    void noteOn (int midiChannel, int midiNoteNumber, float velocity) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, noteOn, midiChannel, midiNoteNumber, velocity);
    }

    void noteOff (int midiChannel, int midiNoteNumber, float velocity, bool allowTailOff) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, noteOff, midiChannel, midiNoteNumber, velocity, allowTailOff);
    }

    void allNotesOff (int midiChannel, bool allowTailOff) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, allNotesOff, midiChannel, allowTailOff);
    }

    void handlePitchWheel (int midiChannel, int wheelValue) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, handlePitchWheel, midiChannel, wheelValue);
    }

    void handleController (int midiChannel, int controllerNumber, int controllerValue) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, handleController, midiChannel, controllerNumber, controllerValue);
    }

    void handleAftertouch (int midiChannel, int midiNoteNumber, int aftertouchValue) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, handleAftertouch, midiChannel, midiNoteNumber, aftertouchValue);
    }

    void handleChannelPressure (int midiChannel, int channelPressureValue) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, handleChannelPressure, midiChannel, channelPressureValue);
    }

    void handleSustainPedal (int midiChannel, bool isDown) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, handleSustainPedal, midiChannel, isDown);
    }

    void handleSostenutoPedal (int midiChannel, bool isDown) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, handleSostenutoPedal, midiChannel, isDown);
    }

    void handleSoftPedal (int midiChannel, bool isDown) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, handleSoftPedal, midiChannel, isDown);
    }

    void handleProgramChange (int midiChannel, int programNumber) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, handleProgramChange, midiChannel, programNumber);
    }

    void setCurrentPlaybackSampleRate (double sampleRate) override
    {
        PYBIND11_OVERRIDE (void, Synthesiser, setCurrentPlaybackSampleRate, sampleRate);
    }
};

//==============================================================================

struct PyAudioPlayHeadPositionInfo : AudioPlayHead::PositionInfo
{
    using AudioPlayHead::PositionInfo::PositionInfo;
};

} // namespace yup::Bindings
