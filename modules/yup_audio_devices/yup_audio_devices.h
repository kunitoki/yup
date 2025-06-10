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

    ID:                 yup_audio_devices
    vendor:             yup
    version:            7.0.12
    name:               YUP audio and MIDI I/O device classes
    description:        Classes to play and record from audio and MIDI I/O devices
    website:            https://github.com/kunitoki/yup
    license:            ISC
    minimumCppStandard: 17

    dependencies:       yup_audio_basics yup_events
    appleFrameworks:    CoreAudio CoreMIDI AudioToolbox
    iosFrameworks:      AVFoundation
    iosSimFrameworks:   AVFoundation
    linuxPackages:      alsa
    androidDeps:        oboe_library
    mingwLibs:          winmm

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AUDIO_DEVICES_H_INCLUDED

#include <yup_events/yup_events.h>
#include <yup_audio_basics/yup_audio_basics.h>

#if 0 && YUP_MODULE_AVAILABLE_yup_graphics
#include <yup_graphics/yup_graphics.h>
#endif

//==============================================================================
/** Config: YUP_USE_WINRT_MIDI
    Enables the use of the Windows Runtime API for MIDI, allowing connections
    to Bluetooth Low Energy devices on Windows 10 version 1809 (October 2018
    Update) and later. If you enable this flag then older versions of Windows
    will automatically fall back to using the regular Win32 MIDI API.

    You will need version 10.0.14393.0 of the Windows Standalone SDK to compile
    and you may need to add the path to the WinRT headers. The path to the
    headers will be something similar to
    "C:\Program Files (x86)\Windows Kits\10\Include\10.0.14393.0\winrt".
*/
#ifndef YUP_USE_WINRT_MIDI
#define YUP_USE_WINRT_MIDI 0
#endif

/** Config: YUP_ASIO
    Enables ASIO audio devices (MS Windows only).
    Turning this on means that you'll need to have the Steinberg ASIO SDK installed
    on your Windows build machine.

    See the comments in the ASIOAudioIODevice class's header file for more
    info about this.
*/
#ifndef YUP_ASIO
#define YUP_ASIO 0
#endif

/** Config: YUP_WASAPI
    Enables WASAPI audio devices (Windows Vista and above).
*/
#ifndef YUP_WASAPI
#define YUP_WASAPI 1
#endif

/** Config: YUP_DIRECTSOUND
    Enables DirectSound audio (MS Windows only).
*/
#ifndef YUP_DIRECTSOUND
#define YUP_DIRECTSOUND 1
#endif

/** Config: YUP_ALSA
    Enables ALSA audio devices (Linux only).
*/
#ifndef YUP_ALSA
#define YUP_ALSA 1
#endif

/** Config: YUP_JACK
    Enables JACK audio devices.
*/
#ifndef YUP_JACK
#define YUP_JACK 0
#endif

/** Config: YUP_BELA
    Enables Bela audio devices on Bela boards.
*/
#ifndef YUP_BELA
#define YUP_BELA 0
#endif

/** Config: YUP_USE_ANDROID_OBOE
    Enables Oboe devices (Android only).
*/
#ifndef YUP_USE_ANDROID_OBOE
#define YUP_USE_ANDROID_OBOE 1
#endif

/** Config: YUP_USE_OBOE_STABILIZED_CALLBACK
    If YUP_USE_ANDROID_OBOE is enabled, enabling this will wrap output audio
    streams in the oboe::StabilizedCallback class. This class attempts to keep
    the CPU spinning to avoid it being scaled down on certain devices.
    (Android only).
*/
#ifndef YUP_USE_ANDROID_OBOE_STABILIZED_CALLBACK
#define YUP_USE_ANDROID_OBOE_STABILIZED_CALLBACK 0
#endif

/** Config: YUP_USE_ANDROID_OPENSLES
    Enables OpenSLES devices (Android only).
*/
#ifndef YUP_USE_ANDROID_OPENSLES
#if ! YUP_USE_ANDROID_OBOE
#define YUP_USE_ANDROID_OPENSLES 1
#else
#define YUP_USE_ANDROID_OPENSLES 0
#endif
#endif

/** Config: YUP_DISABLE_AUDIO_MIXING_WITH_OTHER_APPS
    Turning this on gives your app exclusive access to the system's audio
    on platforms which support it (currently iOS only).
*/
#ifndef YUP_DISABLE_AUDIO_MIXING_WITH_OTHER_APPS
#define YUP_DISABLE_AUDIO_MIXING_WITH_OTHER_APPS 0
#endif

//==============================================================================
#include "midi_io/yup_MidiDevices.h"
#include "midi_io/yup_MidiMessageCollector.h"

namespace yup
{
/** Available modes for the WASAPI audio device.

    Pass one of these to the AudioIODeviceType::createAudioIODeviceType_WASAPI()
    method to create a WASAPI AudioIODeviceType object in this mode.
*/
enum class WASAPIDeviceMode
{
    shared,
    exclusive,
    sharedLowLatency
};
} // namespace yup

#include "audio_io/yup_AudioIODevice.h"
#include "audio_io/yup_AudioIODeviceType.h"
#include "audio_io/yup_SystemAudioVolume.h"
#include "sources/yup_AudioSourcePlayer.h"
#include "sources/yup_AudioTransportSource.h"
#include "audio_io/yup_AudioDeviceManager.h"

#if YUP_IOS
#include "native/yup_Audio_ios.h"
#endif
