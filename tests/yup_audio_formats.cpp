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

#if YUP_MODULE_AVAILABLE_dr_libs && (YUP_AUDIO_FORMAT_WAVE || YUP_AUDIO_FORMAT_MP3)
#include <dr_libs/dr_libs.h>
#endif

#if YUP_MODULE_AVAILABLE_opus_library && YUP_AUDIO_FORMAT_OPUS
#include <opus_library/opus_library.h>
#endif

#if YUP_MODULE_AVAILABLE_flac_library && YUP_AUDIO_FORMAT_FLAC
#include <flac_library/flac_library.h>
#endif

#include "yup_audio_formats/yup_AudioFormatManager.cpp"
#include "yup_audio_formats/yup_AudioFormatReader.cpp"
#include "yup_audio_formats/yup_AudioFormatWriter.cpp"

#if YUP_MODULE_AVAILABLE_dr_libs && YUP_AUDIO_FORMAT_WAVE
#include "yup_audio_formats/yup_WaveAudioFormat.cpp"
#endif

#if YUP_MODULE_AVAILABLE_dr_libs && YUP_AUDIO_FORMAT_MP3
#include "yup_audio_formats/yup_Mp3AudioFormat.cpp"
#endif

#if YUP_MODULE_AVAILABLE_opus_library && YUP_AUDIO_FORMAT_OPUS
#include "yup_audio_formats/yup_OpusAudioFormat.cpp"
#endif

#if YUP_MODULE_AVAILABLE_flac_library && YUP_AUDIO_FORMAT_FLAC
#include "yup_audio_formats/yup_FlacAudioFormat.cpp"
#endif

#if YUP_AUDIO_FORMAT_COREAUDIO
#include "yup_audio_formats/yup_AppleCoreAudioFormat.cpp"
#endif
