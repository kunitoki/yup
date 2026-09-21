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

#include "../mocks/yup_audio_basics.h"

#include "yup_ADSR.cpp"
#include "yup_AudioChannelSet.cpp"
#include "yup_AudioDataConverters.cpp"
#include "yup_AudioLockType.cpp"
#include "yup_AudioPlayHead.cpp"
#include "yup_AudioProcessLoadMeasurer.cpp"
#include "yup_AudioSampleBuffer.cpp"
#include "yup_AudioSpectralBuffer.cpp"
#include "yup_BufferingAudioSource.cpp"
#include "yup_ChannelRemappingAudioSource.cpp"
#include "yup_Decibels.cpp"
#include "yup_IIRFilter.cpp"
#include "yup_Interpolators.cpp"
#include "yup_MemoryAudioSource.cpp"
#include "yup_MidiBuffer.cpp"
#include "yup_MidiDataConcatenator.cpp"
#include "yup_MidiFile.cpp"
#include "yup_MidiKeyboardState.cpp"
#include "yup_MidiMessage.cpp"
#include "yup_MidiMessageSequence.cpp"
#include "yup_MidiRPN.cpp"
#include "yup_MixerAudioSource.cpp"
#include "yup_MPEInstrument.cpp"
#include "yup_MPEMessages.cpp"
#include "yup_MPENote.cpp"
#include "yup_MPESynthesiserBase.cpp"
#include "yup_MPEUtils.cpp"
#include "yup_MPEValue.cpp"
#include "yup_MPEZoneLayout.cpp"
#include "yup_ResamplingAudioSource.cpp"
#include "yup_Reverb.cpp"
#include "yup_ReverbAudioSource.cpp"
#include "yup_SmoothedValue.cpp"
#include "yup_Synthesiser.cpp"
#include "yup_ToneGeneratorAudioSource.cpp"
#include "yup_TuningMap.cpp"
#include "yup_UMP.cpp"
#include "yup_UMPCapabilityInquiry.cpp"
#include "yup_UMPChannelVoice.cpp"
#include "yup_UMPDataMessages.cpp"
#include "yup_UMPExtendedDataMessages.cpp"
#include "yup_UMPFlexDataMessages.cpp"
#include "yup_UMPJitterReductionTimestamps.cpp"
#include "yup_UMPKeyboardState.cpp"
#include "yup_UMPMessages.cpp"
#include "yup_UMPMidi1ByteStream.cpp"
#include "yup_UMPMidi1ChannelVoiceMessage.cpp"
#include "yup_UMPMidi2ChannelVoiceMessage.cpp"
#include "yup_UMPPacketBuffer.cpp"
#include "yup_UMPStreamMessages.cpp"
#include "yup_UMPSysExCollectors.cpp"
#include "yup_UMPTypes.cpp"
#include "yup_UMPUniversalPacket.cpp"
#include "yup_UMPUniversalSysEx.cpp"
