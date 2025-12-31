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

#ifdef YUP_AUDIO_FORMATS_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other YUP headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#include "yup_audio_formats.h"

//==============================================================================

#if YUP_AUDIO_FORMAT_WAVE || YUP_AUDIO_FORMAT_MP3
#include <dr_libs/dr_libs.h>
#endif

#if YUP_AUDIO_FORMAT_OPUS
#include <opus_library/opus_library.h>
#endif

#if YUP_AUDIO_FORMAT_FLAC
#include <flac_library/flac_library.h>
#endif

#if YUP_MAC || YUP_IOS
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>

#include <yup_audio_basics/native/yup_CoreAudioLayouts_apple.h>
#endif

#if YUP_WINDOWS
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>

#if ! YUP_DONT_AUTOLINK_TO_WIN32_LIBRARIES
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#endif
#endif

//==============================================================================

#include "format/yup_AudioFormat.cpp"
#include "format/yup_AudioFormatReader.cpp"
#include "format/yup_AudioFormatWriter.cpp"
#include "common/yup_AudioFormatManager.cpp"

//==============================================================================

#if YUP_AUDIO_FORMAT_WAVE
#include "formats/yup_WaveAudioFormat.cpp"
#endif

#if YUP_AUDIO_FORMAT_MP3
#include "formats/yup_Mp3AudioFormat.cpp"
#endif

#if YUP_AUDIO_FORMAT_OPUS
#include "formats/yup_OpusAudioFormat.cpp"
#endif

#if YUP_AUDIO_FORMAT_FLAC
#include "formats/yup_FlacAudioFormat.cpp"
#endif

#if YUP_AUDIO_FORMAT_COREAUDIO
#include "formats/yup_AppleCoreAudioFormat.cpp"
#endif

#if YUP_AUDIO_FORMAT_MEDIAFOUNDATION
#include "formats/yup_WindowsMediaAudioFormat.cpp"
#endif
