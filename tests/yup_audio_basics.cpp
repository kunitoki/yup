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

#include "yup_audio_basics/yup_ADSR.cpp"
#include "yup_audio_basics/yup_AudioChannelSet.cpp"
#include "yup_audio_basics/yup_AudioDataConverters.cpp"
#include "yup_audio_basics/yup_AudioPlayHead.cpp"
#include "yup_audio_basics/yup_AudioProcessLoadMeasurer.cpp"
#include "yup_audio_basics/yup_AudioSampleBuffer.cpp"
#include "yup_audio_basics/yup_AudioSpectralBuffer.cpp"
#include "yup_audio_basics/yup_BufferingAudioSource.cpp"
#include "yup_audio_basics/yup_ChannelRemappingAudioSource.cpp"
#include "yup_audio_basics/yup_Decibels.cpp"
#include "yup_audio_basics/yup_IIRFilter.cpp"
#include "yup_audio_basics/yup_Interpolators.cpp"
#include "yup_audio_basics/yup_MemoryAudioSource.cpp"
#include "yup_audio_basics/yup_MidiBuffer.cpp"
#include "yup_audio_basics/yup_MidiDataConcatenator.cpp"
#include "yup_audio_basics/yup_MidiFile.cpp"
#include "yup_audio_basics/yup_MidiKeyboardState.cpp"
#include "yup_audio_basics/yup_MidiMessage.cpp"
#include "yup_audio_basics/yup_MidiMessageSequence.cpp"
#include "yup_audio_basics/yup_MidiRPN.cpp"
#include "yup_audio_basics/yup_MixerAudioSource.cpp"
#include "yup_audio_basics/yup_MPEInstrument.cpp"
#include "yup_audio_basics/yup_MPEMessages.cpp"
#include "yup_audio_basics/yup_MPENote.cpp"
#include "yup_audio_basics/yup_MPESynthesiserBase.cpp"
#include "yup_audio_basics/yup_MPEUtils.cpp"
#include "yup_audio_basics/yup_MPEValue.cpp"
#include "yup_audio_basics/yup_MPEZoneLayout.cpp"
#include "yup_audio_basics/yup_ResamplingAudioSource.cpp"
#include "yup_audio_basics/yup_Reverb.cpp"
#include "yup_audio_basics/yup_ReverbAudioSource.cpp"
#include "yup_audio_basics/yup_SmoothedValue.cpp"
#include "yup_audio_basics/yup_Synthesiser.cpp"
#include "yup_audio_basics/yup_ToneGeneratorAudioSource.cpp"
#include "yup_audio_basics/yup_UMP.cpp"
#include "yup_audio_basics/yup_UMPCapabilityInquiry.cpp"
#include "yup_audio_basics/yup_UMPChannelVoice.cpp"
#include "yup_audio_basics/yup_UMPDataMessages.cpp"
#include "yup_audio_basics/yup_UMPExtendedDataMessages.cpp"
#include "yup_audio_basics/yup_UMPFlexDataMessages.cpp"
#include "yup_audio_basics/yup_UMPJitterReductionTimestamps.cpp"
#include "yup_audio_basics/yup_UMPKeyboardState.cpp"
#include "yup_audio_basics/yup_UMPMessages.cpp"
#include "yup_audio_basics/yup_UMPMidi1ByteStream.cpp"
#include "yup_audio_basics/yup_UMPMidi1ChannelVoiceMessage.cpp"
#include "yup_audio_basics/yup_UMPMidi2ChannelVoiceMessage.cpp"
#include "yup_audio_basics/yup_UMPPacketBuffer.cpp"
#include "yup_audio_basics/yup_UMPStreamMessages.cpp"
#include "yup_audio_basics/yup_UMPSysExCollectors.cpp"
#include "yup_audio_basics/yup_UMPTypes.cpp"
#include "yup_audio_basics/yup_UMPUniversalPacket.cpp"
#include "yup_audio_basics/yup_UMPUniversalSysEx.cpp"
