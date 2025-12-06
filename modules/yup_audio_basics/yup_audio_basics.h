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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                 yup_audio_basics
    vendor:             yup
    version:            1.0.0
    name:               YUP audio and MIDI data classes
    description:        Classes for audio buffer manipulation, midi message handling, synthesis, etc.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_core
    appleFrameworks:    Accelerate

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AUDIO_BASICS_H_INCLUDED

#include <yup_core/yup_core.h>

//==============================================================================
#undef Complex // apparently some C libraries actually define these symbols (!)
#undef Factor

//==============================================================================
#ifndef YUP_USE_SSE_INTRINSICS
#if defined (__SSE__)
#define YUP_USE_SSE_INTRINSICS 1
#endif
#endif

#ifndef YUP_USE_AVX_INTRINSICS
#if defined (__AVX2__)
#define YUP_USE_AVX_INTRINSICS 1
#endif
#endif

#ifndef YUP_USE_FMA_INTRINSICS
#if defined (__FMA__)
#define YUP_USE_FMA_INTRINSICS 1
#endif
#endif

#if ! YUP_INTEL
#undef YUP_USE_SSE_INTRINSICS
#undef YUP_USE_AVX_INTRINSICS
#undef YUP_USE_FMA_INTRINSICS
#endif

#if __ARM_NEON__ && ! (YUP_USE_VDSP_FRAMEWORK || defined(YUP_USE_ARM_NEON))
#define YUP_USE_ARM_NEON 1
#endif

#if TARGET_IPHONE_SIMULATOR
#ifdef YUP_USE_ARM_NEON
#undef YUP_USE_ARM_NEON
#endif
#define YUP_USE_ARM_NEON 0
#endif

//==============================================================================
#if YUP_USE_AVX_INTRINSICS || YUP_USE_FMA_INTRINSICS
#include <immintrin.h>
#endif

#if YUP_USE_SSE_INTRINSICS
#include <emmintrin.h>
#endif

#if YUP_USE_ARM_NEON
#if JUCE_64BIT && JUCE_WINDOWS
#include <arm64_neon.h>
#else
#include <arm_neon.h>
#endif
#endif

#if (YUP_MAC || YUP_IOS) && __has_include(<Accelerate/Accelerate.h>)
#ifndef YUP_USE_VDSP_FRAMEWORK
#define YUP_USE_VDSP_FRAMEWORK 1
#endif

#if YUP_USE_VDSP_FRAMEWORK
#include <Accelerate/Accelerate.h>
#endif

#elif YUP_USE_VDSP_FRAMEWORK
#undef YUP_USE_VDSP_FRAMEWORK
#endif

//==============================================================================
#include "buffers/yup_AudioDataConverters.h"
YUP_BEGIN_IGNORE_WARNINGS_MSVC (4661)
#include "buffers/yup_FloatVectorOperations.h"
YUP_END_IGNORE_WARNINGS_MSVC
#include "buffers/yup_AudioSampleBuffer.h"
#include "buffers/yup_AudioChannelSet.h"
#include "buffers/yup_AudioProcessLoadMeasurer.h"
#include "utilities/yup_Decibels.h"
#include "utilities/yup_IIRFilter.h"
#include "utilities/yup_GenericInterpolator.h"
#include "utilities/yup_Interpolators.h"
#include "utilities/yup_SmoothedValue.h"
#include "utilities/yup_Reverb.h"
#include "utilities/yup_ADSR.h"
#include "midi/yup_MidiMessage.h"
#include "midi/yup_MidiBuffer.h"
#include "midi/yup_MidiMessageSequence.h"
#include "midi/yup_MidiFile.h"
#include "midi/yup_MidiKeyboardState.h"
#include "midi/yup_MidiRPN.h"
#include "midi/yup_MidiDataConcatenator.h"
#include "mpe/yup_MPEValue.h"
#include "mpe/yup_MPENote.h"
#include "mpe/yup_MPEZoneLayout.h"
#include "mpe/yup_MPEInstrument.h"
#include "mpe/yup_MPEMessages.h"
#include "mpe/yup_MPESynthesiserBase.h"
#include "mpe/yup_MPESynthesiserVoice.h"
#include "mpe/yup_MPESynthesiser.h"
#include "mpe/yup_MPEUtils.h"
#include "sources/yup_AudioSource.h"
#include "sources/yup_PositionableAudioSource.h"
#include "sources/yup_BufferingAudioSource.h"
#include "sources/yup_ChannelRemappingAudioSource.h"
#include "sources/yup_IIRFilterAudioSource.h"
#include "sources/yup_MemoryAudioSource.h"
#include "sources/yup_MixerAudioSource.h"
#include "sources/yup_ResamplingAudioSource.h"
#include "sources/yup_ReverbAudioSource.h"
#include "sources/yup_ToneGeneratorAudioSource.h"
#include "synthesisers/yup_Synthesiser.h"
#include "audio_play_head/yup_AudioPlayHead.h"
#include "utilities/yup_AudioWorkgroup.h"
#include "midi/ump/yup_UMPBytesOnGroup.h"
#include "midi/ump/yup_UMPDeviceInfo.h"
#include "midi/ump/yup_UMP.h"

namespace yup
{
namespace ump = universal_midi_packets;
} // namespace yup
