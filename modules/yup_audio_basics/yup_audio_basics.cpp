/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#ifdef YUP_AUDIO_BASICS_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of JUCE cpp file"
#endif

#include "yup_audio_basics.h"

#if YUP_MAC || YUP_IOS
#include "native/yup_AudioWorkgroup_apple.h"
#endif

#include "buffers/yup_FloatVectorOperations.cpp"
#include "buffers/yup_AudioChannelSet.cpp"
#include "buffers/yup_AudioProcessLoadMeasurer.cpp"
#include "utilities/yup_IIRFilter.cpp"
#include "utilities/yup_LagrangeInterpolator.cpp"
#include "utilities/yup_WindowedSincInterpolator.cpp"
#include "midi/yup_MidiBuffer.cpp"
#include "midi/yup_MidiFile.cpp"
#include "midi/yup_MidiKeyboardState.cpp"
#include "midi/yup_MidiMessage.cpp"
#include "midi/yup_MidiMessageSequence.cpp"
#include "midi/yup_MidiRPN.cpp"
#include "mpe/yup_MPEValue.cpp"
#include "mpe/yup_MPENote.cpp"
#include "mpe/yup_MPEZoneLayout.cpp"
#include "mpe/yup_MPEInstrument.cpp"
#include "mpe/yup_MPEMessages.cpp"
#include "mpe/yup_MPESynthesiserBase.cpp"
#include "mpe/yup_MPESynthesiserVoice.cpp"
#include "mpe/yup_MPESynthesiser.cpp"
#include "mpe/yup_MPEUtils.cpp"
#include "sources/yup_BufferingAudioSource.cpp"
#include "sources/yup_ChannelRemappingAudioSource.cpp"
#include "sources/yup_IIRFilterAudioSource.cpp"
#include "sources/yup_MemoryAudioSource.cpp"
#include "sources/yup_MixerAudioSource.cpp"
#include "sources/yup_ResamplingAudioSource.cpp"
#include "sources/yup_ReverbAudioSource.cpp"
#include "sources/yup_ToneGeneratorAudioSource.cpp"
#include "sources/yup_PositionableAudioSource.cpp"
#include "synthesisers/yup_Synthesiser.cpp"
#include "audio_play_head/yup_AudioPlayHead.cpp"
#include "midi/ump/yup_UMPUtils.cpp"
#include "midi/ump/yup_UMPView.cpp"
#include "midi/ump/yup_UMPSysEx7.cpp"
#include "midi/ump/yup_UMPMidi1ToMidi2DefaultTranslator.cpp"
#include "midi/ump/yup_UMPIterator.cpp"
#include "utilities/yup_AudioWorkgroup.cpp"
