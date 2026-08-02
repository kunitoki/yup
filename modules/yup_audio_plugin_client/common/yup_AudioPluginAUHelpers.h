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

#if YUP_MAC

#import <Foundation/Foundation.h>

#include <AudioUnit/AudioUnit.h>

#include <cstdint>
#include <vector>

//==============================================================================
namespace yup
{

/** Returns a hex string describing a pointer, for debug logging. */
inline String describePointer (const void* value)
{
    return "0x" + String::toHexString (static_cast<int64> (reinterpret_cast<uintptr_t> (value)));
}

/** Returns the dictionary key used to store the YUP processor state
    alongside the host's own AU state (shared by the AUv2 and AUv3 clients). */
inline CFStringRef getAUProcessorStateKey()
{
    return CFSTR ("YUPProcessorState");
}

/** Maps a ParameterUnit to the corresponding AudioUnitParameterUnit constant. */
inline AudioUnitParameterUnit makeAUUnit (const AudioParameter& param)
{
    switch (param.getUnit())
    {
        case AudioParameter::ParameterUnit::Generic:
            return kAudioUnitParameterUnit_Generic;
        case AudioParameter::ParameterUnit::Percent:
            return kAudioUnitParameterUnit_Percent;
        case AudioParameter::ParameterUnit::Decibels:
            return kAudioUnitParameterUnit_Decibels;
        case AudioParameter::ParameterUnit::Hertz:
            return kAudioUnitParameterUnit_Hertz;
        case AudioParameter::ParameterUnit::Milliseconds:
            return kAudioUnitParameterUnit_Milliseconds;
        case AudioParameter::ParameterUnit::Seconds:
            return kAudioUnitParameterUnit_Seconds;
        case AudioParameter::ParameterUnit::Degrees:
            return kAudioUnitParameterUnit_Degrees;
        case AudioParameter::ParameterUnit::Cents:
            return kAudioUnitParameterUnit_Cents;
        case AudioParameter::ParameterUnit::Semitones:
            return kAudioUnitParameterUnit_RelativeSemiTones;
        case AudioParameter::ParameterUnit::Octaves:
            return kAudioUnitParameterUnit_Octaves;
        case AudioParameter::ParameterUnit::BPM:
            return kAudioUnitParameterUnit_BPM;
        case AudioParameter::ParameterUnit::Beats:
            return kAudioUnitParameterUnit_Beats;
        case AudioParameter::ParameterUnit::Ratio:
            return kAudioUnitParameterUnit_Ratio;
        case AudioParameter::ParameterUnit::LinearGain:
            return kAudioUnitParameterUnit_LinearGain;
        case AudioParameter::ParameterUnit::Pan:
            return kAudioUnitParameterUnit_Pan;
        case AudioParameter::ParameterUnit::MIDINoteNumber:
            return kAudioUnitParameterUnit_MIDINoteNumber;
        case AudioParameter::ParameterUnit::Custom:
            return kAudioUnitParameterUnit_CustomUnit;
    }

    return kAudioUnitParameterUnit_Generic;
}

/** Returns the base AudioUnitParameterOptions shared by the AUv2 and AUv3
    clients: readable, plus writable when the parameter is not read-only.

    AUv2 callers must OR in kAudioUnitParameterFlag_HasCFNameString.
*/
inline AudioUnitParameterOptions makeAUParameterFlags (const AudioParameter& param)
{
    AudioUnitParameterOptions flags = kAudioUnitParameterFlag_IsReadable;

    if (! param.isReadOnly())
        flags |= kAudioUnitParameterFlag_IsWritable;

    return flags;
}

/** Returns the AudioUnitParameterOptions used by the AUv3 client.

    Extends makeAUParameterFlags with value-string and ramp capabilities.
    This must not be used by the AUv2 client, which implements no string
    properties and cannot schedule parameters.
*/
inline AudioUnitParameterOptions makeAUv3ParameterFlags (const AudioParameter& param)
{
    AudioUnitParameterOptions flags = makeAUParameterFlags (param)
                                    | kAudioUnitParameterFlag_ValuesHaveStrings;

    if (! param.isStepped())
        flags |= kAudioUnitParameterFlag_CanRamp;

    return flags;
}

//==============================================================================

/** Per-bus render view storage shared by the AUv2 and AUv3 clients.

    Pre-allocated once at initialization so the render callback only clears
    and refills the vectors without allocating.
*/
struct AudioPluginAURenderViews
{
    std::vector<AudioBusBufferView<const float>> inputBusViews;
    std::vector<AudioBusBufferView<float>> outputBusViews;
    std::vector<const float*> inputChannelPtrStorage;
    std::vector<float*> outputChannelPtrStorage;

    /** Reserves and sizes the storage for the processor's bus layout. */
    void prepare (const AudioProcessor& processor, int totalInputChannels, int totalOutputChannels)
    {
        inputBusViews.reserve (static_cast<size_t> (processor.getNumAudioInputs()));
        outputBusViews.reserve (static_cast<size_t> (processor.getNumAudioOutputs()));
        inputChannelPtrStorage.resize (static_cast<size_t> (totalInputChannels));
        outputChannelPtrStorage.resize (static_cast<size_t> (totalOutputChannels));
    }
};

} // namespace yup

#endif // YUP_MAC
