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

#ifdef YUP_AUDIO_DEVICES_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#define YUP_CORE_INCLUDE_OBJC_HELPERS 1
#define YUP_CORE_INCLUDE_COM_SMART_PTR 1
#define YUP_CORE_INCLUDE_JNI_HELPERS 1
#define YUP_CORE_INCLUDE_NATIVE_HEADERS 1
#define YUP_EVENTS_INCLUDE_WIN32_MESSAGE_WINDOW 1

#ifndef YUP_USE_WINRT_MIDI
#define YUP_USE_WINRT_MIDI 0
#endif

#if YUP_USE_WINRT_MIDI
#define YUP_EVENTS_INCLUDE_WINRT_WRAPPER 1
#endif

#include "yup_audio_devices.h"

#include "audio_io/yup_SampleRateHelpers.cpp"
#include "midi_io/yup_MidiDevices.cpp"

//==============================================================================
#if YUP_MAC || YUP_IOS
#include <yup_audio_basics/native/yup_CoreAudioTimeConversions_apple.h>
#include <yup_audio_basics/native/yup_AudioWorkgroup_apple.h>

#include "midi_io/ump/yup_UMPBytestreamInputHandler.h"
#include "midi_io/ump/yup_UMPU32InputHandler.h"
#endif

#if YUP_MAC
#define Point CarbonDummyPointName
#define Component CarbonDummyCompName
#import <CoreAudio/AudioHardware.h>
#import <CoreMIDI/MIDIServices.h>
#import <AudioToolbox/AudioServices.h>
#undef Point
#undef Component

#include "native/yup_CoreAudio_mac.cpp"
#include "native/yup_CoreMidi_apple.mm"

#elif YUP_IOS
#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMIDI/MIDIServices.h>

#if TARGET_OS_SIMULATOR
#import <CoreMIDI/MIDINetworkSession.h>
#endif

#if 0 && YUP_MODULE_AVAILABLE_yup_graphics
#include <yup_graphics/native/yup_CoreGraphicsHelpers_mac.h>
#endif

#include "native/yup_Audio_ios.cpp"
#include "native/yup_CoreMidi_apple.mm"

//==============================================================================
#elif YUP_WINDOWS
#if YUP_WASAPI
#include <mmreg.h>
#include "native/yup_WASAPI_windows.cpp"
#endif

#if YUP_DIRECTSOUND
#include "native/yup_DirectSound_windows.cpp"
#endif

// clang-format off
#if YUP_USE_WINRT_MIDI && (YUP_MSVC || YUP_CLANG)
/* If you cannot find any of the header files below then you are probably
   attempting to use the Windows 10 Bluetooth Low Energy API. For this to work you
   need to install version 10.0.14393.0 of the Windows Standalone SDK and you may
   need to add the path to the WinRT headers to your build system. This path should
   have the form "C:\Program Files (x86)\Windows Kits\10\Include\10.0.14393.0\winrt".

   Also please note that Microsoft's Bluetooth MIDI stack has multiple issues, so
   this API is EXPERIMENTAL - use at your own risk!
*/
#include <windows.devices.h>
#include <windows.devices.midi.h>
#include <windows.devices.enumeration.h>

YUP_BEGIN_IGNORE_WARNINGS_MSVC (4265)
#include <wrl/event.h>
YUP_END_IGNORE_WARNINGS_MSVC

YUP_BEGIN_IGNORE_WARNINGS_MSVC (4467)
#include <robuffer.h>
YUP_END_IGNORE_WARNINGS_MSVC
#endif
// clang-format on

#include "native/yup_Midi_windows.cpp"

// clang-format off
#if YUP_ASIO
/* This is very frustrating - we only need to use a handful of definitions from
   a couple of the header files in Steinberg's ASIO SDK, and it'd be easy to copy
   about 30 lines of code into this cpp file to create a fully stand-alone ASIO
   implementation...

   ..unfortunately that would break Steinberg's license agreement for use of
   their SDK, so I'm not allowed to do this.

   This means that anyone who wants to use YUP's ASIO abilities will have to:

   1) Agree to Steinberg's licensing terms and download the ASIO SDK
       (see http://www.steinberg.net/en/company/developers.html).

   2) Enable this code with a global definition #define YUP_ASIO 1.

   3) Make sure that your header search path contains the iasiodrv.h file that
      comes with the SDK. (Only about a handful of the SDK header files are actually
      needed - so to simplify things, you could just copy these into your YUP directory).
*/
#include <iasiodrv.h>
#include "native/yup_ASIO_windows.cpp"
#endif
// clang-format oon

//==============================================================================
#elif YUP_LINUX || YUP_BSD
// clang-format off
#if YUP_ALSA
/* Got an include error here? If so, you've either not got ALSA installed, or you've
   not got your paths set up correctly to find its header files.

   The package you need to install to get ASLA support is "libasound2-dev".

   If you don't have the ALSA library and don't want to build YUP with audio support,
   just set the YUP_ALSA flag to 0.
*/
YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wzero-length-array")
#include <alsa/asoundlib.h>
YUP_END_IGNORE_WARNINGS_GCC_LIKE
#include "native/yup_ALSA_linux.cpp"
#endif

#if (YUP_LINUX && YUP_BELA)
/* Got an include error here? If so, you've either not got the bela headers
   installed, or you've not got your paths set up correctly to find its header
   files.
*/
#include <Bela.h>
#include <Midi.h>
#include "native/yup_Bela_linux.cpp"
#endif
// clang-format on

#undef SIZEOF

#if ! YUP_BELA
#include "native/yup_Midi_linux.cpp"
#endif

//==============================================================================
#elif YUP_ANDROID

namespace yup
{
using RealtimeThreadFactory = pthread_t (*) (void* (*) (void*), void*);
RealtimeThreadFactory getAndroidRealtimeThreadFactory();
} // namespace yup

#include "native/yup_Audio_android.cpp"

#include "native/yup_Midi_android.cpp"

#if YUP_USE_ANDROID_OPENSLES || YUP_USE_ANDROID_OBOE
#include "native/yup_HighPerformanceAudioHelpers_android.h"

#if YUP_USE_ANDROID_OPENSLES
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include "native/yup_OpenSL_android.cpp"
#endif

#if YUP_USE_ANDROID_OBOE
#if YUP_USE_ANDROID_OPENSLES
#error "Oboe cannot be enabled at the same time as openSL! Please disable YUP_USE_ANDROID_OPENSLES"
#endif

YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wunused-parameter",
                                    "-Wzero-as-null-pointer-constant",
                                    "-Winconsistent-missing-destructor-override",
                                    "-Wshadow-field-in-constructor",
                                    "-Wshadow-field",
                                    "-Wsign-conversion",
                                    "-Wswitch-enum")
#include <oboe_library/oboe_library.h>
YUP_END_IGNORE_WARNINGS_GCC_LIKE

#include "native/yup_Oboe_android.cpp"
#endif
#else
// No audio library, so no way to create realtime threads.
namespace yup
{
RealtimeThreadFactory getAndroidRealtimeThreadFactory() { return nullptr; }
} // namespace yup
#endif

#elif YUP_WASM
#if YUP_EMSCRIPTEN
#include <emscripten/webaudio.h>
#include <emscripten/em_math.h>

#include "native/yup_AudioWorklet_emscripten.cpp"
#endif

#include "native/yup_Midi_wasm.cpp"

#endif

#if (YUP_LINUX || YUP_BSD || YUP_MAC || YUP_WINDOWS) && YUP_JACK
/* Got an include error here? If so, you've either not got jack-audio-connection-kit
   installed, or you've not got your paths set up correctly to find its header files.
   Linux: The package you need to install to get JACK support is libjack-dev.
   macOS: The package you need to install to get JACK support is jack, which you can
   install using Homebrew.
   Windows: The package you need to install to get JACK support is available from the
   JACK Audio website. Download and run the installer for Windows.
   If you don't have the jack-audio-connection-kit library and don't want to build
   YUP with low latency audio support, just set the YUP_JACK flag to 0.
*/
#include <jack/jack.h>

#include "native/yup_JackAudio.cpp"
#endif

#if ! YUP_SYSTEMAUDIOVOL_IMPLEMENTED
namespace yup
{

// None of these methods are available. (On Windows you might need to enable WASAPI for this)
float YUP_CALLTYPE SystemAudioVolume::getGain()
{
    jassertfalse;
    return 0.0f;
}

bool YUP_CALLTYPE SystemAudioVolume::setGain (float)
{
    jassertfalse;
    return false;
}

bool YUP_CALLTYPE SystemAudioVolume::isMuted()
{
    jassertfalse;
    return false;
}

bool YUP_CALLTYPE SystemAudioVolume::setMuted (bool)
{
    jassertfalse;
    return false;
}

} // namespace yup
#endif

#include "audio_io/yup_AudioDeviceManager.cpp"
#include "audio_io/yup_AudioIODevice.cpp"
#include "audio_io/yup_AudioIODeviceType.cpp"
#include "midi_io/yup_MidiMessageCollector.cpp"
#include "sources/yup_AudioSourcePlayer.cpp"
#include "sources/yup_AudioTransportSource.cpp"
