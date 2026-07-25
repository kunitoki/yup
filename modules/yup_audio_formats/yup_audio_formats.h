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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                   yup_audio_formats
    vendor:               yup
    version:              2.0.0
    name:                 YUP Audio Formats
    description:          Audio formats for the YUP library
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_audio_basics yup_simd
    optionalDeps:         dr_libs flac_library hmp3_library libogg libvorbis opus_library
    appleFrameworks:      AudioToolbox CoreAudio CoreFoundation

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AUDIO_FORMATS_H_INCLUDED

#include <yup_audio_basics/yup_audio_basics.h>
#include <yup_simd/yup_simd.h>

//==============================================================================
/** Config: YUP_AUDIO_FORMAT_WAVE

    Enable Wave audio format support.
*/
#ifndef YUP_AUDIO_FORMAT_WAVE
#if YUP_MODULE_AVAILABLE_dr_libs
#define YUP_AUDIO_FORMAT_WAVE 1
#endif
#endif

//==============================================================================
/** Config: YUP_AUDIO_FORMAT_MP3

    Enable Mp3 audio format support.
*/
#ifndef YUP_AUDIO_FORMAT_MP3
#if YUP_MODULE_AVAILABLE_dr_libs
#define YUP_AUDIO_FORMAT_MP3 1
#endif
#endif

//==============================================================================
/** Config: YUP_AUDIO_FORMAT_OGG

    Enable Ogg Vorbis audio format support.
*/
#ifndef YUP_AUDIO_FORMAT_OGG
#if YUP_MODULE_AVAILABLE_libvorbis && YUP_MODULE_AVAILABLE_libogg
#define YUP_AUDIO_FORMAT_OGG 1
#endif
#endif

//==============================================================================
/** Config: YUP_AUDIO_FORMAT_OPUS

    Enable Opus audio format support.
*/
#ifndef YUP_AUDIO_FORMAT_OPUS
#if YUP_MODULE_AVAILABLE_opus_library
#define YUP_AUDIO_FORMAT_OPUS 1
#endif
#endif

//==============================================================================

/** Config: YUP_AUDIO_FORMAT_FLAC

    Enable FLAC audio format support.
*/
#ifndef YUP_AUDIO_FORMAT_FLAC
#if YUP_MODULE_AVAILABLE_flac_library
#define YUP_AUDIO_FORMAT_FLAC 1
#endif
#endif

//==============================================================================
/** Config: YUP_AUDIO_FORMAT_COREAUDIO

    Enable CoreAudio audio format support on Apple platforms.
*/
#ifndef YUP_AUDIO_FORMAT_COREAUDIO
#if YUP_MAC || YUP_IOS
#define YUP_AUDIO_FORMAT_COREAUDIO 1
#endif
#endif

//==============================================================================
/** Config: YUP_AUDIO_FORMAT_MEDIAFOUNDATION

    Enable Media Foundation audio format support on Windows.
*/
#ifndef YUP_AUDIO_FORMAT_MEDIAFOUNDATION
#if YUP_WINDOWS
#define YUP_AUDIO_FORMAT_MEDIAFOUNDATION 1
#endif
#endif

//==============================================================================

#if YUP_AUDIO_FORMAT_WAVE && ! YUP_MODULE_AVAILABLE_dr_libs
#undef YUP_AUDIO_FORMAT_WAVE
#define YUP_AUDIO_FORMAT_WAVE 0
#endif

#if YUP_AUDIO_FORMAT_MP3 && ! YUP_MODULE_AVAILABLE_dr_libs
#undef YUP_AUDIO_FORMAT_MP3
#define YUP_AUDIO_FORMAT_MP3 0
#endif

#if YUP_AUDIO_FORMAT_OGG && ! (YUP_MODULE_AVAILABLE_libvorbis && YUP_MODULE_AVAILABLE_libogg)
#undef YUP_AUDIO_FORMAT_OGG
#define YUP_AUDIO_FORMAT_OGG 0
#endif

#if YUP_AUDIO_FORMAT_OPUS && ! YUP_MODULE_AVAILABLE_opus_library
#undef YUP_AUDIO_FORMAT_OPUS
#define YUP_AUDIO_FORMAT_OPUS 0
#endif

#if YUP_AUDIO_FORMAT_FLAC && ! YUP_MODULE_AVAILABLE_flac_library
#undef YUP_AUDIO_FORMAT_FLAC
#define YUP_AUDIO_FORMAT_FLAC 0
#endif

#if YUP_AUDIO_FORMAT_COREAUDIO && ! (YUP_MAC || YUP_IOS)
#undef YUP_AUDIO_FORMAT_COREAUDIO
#define YUP_AUDIO_FORMAT_COREAUDIO 0
#endif

#if YUP_AUDIO_FORMAT_MEDIAFOUNDATION && ! YUP_WINDOWS
#undef YUP_AUDIO_FORMAT_MEDIAFOUNDATION
#define YUP_AUDIO_FORMAT_MEDIAFOUNDATION 0
#endif

//==============================================================================

#include "format/yup_AudioFormat.h"
#include "format/yup_AudioFormatReader.h"
#include "format/yup_AudioFormatWriter.h"
#include "common/yup_AudioFormatManager.h"
#include "sources/yup_AudioFormatReaderSource.h"

//==============================================================================

#if YUP_AUDIO_FORMAT_WAVE
#include "formats/yup_WaveAudioFormat.h"
#endif

#if YUP_AUDIO_FORMAT_MP3
#include "formats/yup_Mp3AudioFormat.h"
#endif

#if YUP_AUDIO_FORMAT_OGG
#include "formats/yup_OggVorbisAudioFormat.h"
#endif

#if YUP_AUDIO_FORMAT_OPUS
#include "formats/yup_OpusAudioFormat.h"
#endif

#if YUP_AUDIO_FORMAT_FLAC
#include "formats/yup_FlacAudioFormat.h"
#endif

#if YUP_AUDIO_FORMAT_COREAUDIO
#include "formats/yup_AppleCoreAudioFormat.h"
#endif

#if YUP_AUDIO_FORMAT_MEDIAFOUNDATION
#include "formats/yup_WindowsMediaAudioFormat.h"
#endif
