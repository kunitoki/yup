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

namespace yup
{

AudioIODeviceType::AudioIODeviceType (const String& name)
    : typeName (name)
{
}

AudioIODeviceType::~AudioIODeviceType()
{
}

//==============================================================================
void AudioIODeviceType::addListener (Listener* l) { listeners.add (l); }

void AudioIODeviceType::removeListener (Listener* l) { listeners.remove (l); }

void AudioIODeviceType::callDeviceChangeListeners()
{
    listeners.call ([] (Listener& l)
    {
        l.audioDeviceListChanged();
    });
}

//==============================================================================
AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_CoreAudio()
{
#if YUP_MAC
    return new CoreAudioClasses::CoreAudioIODeviceType();
#else
    return nullptr;
#endif
}

//==============================================================================
AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_iOSAudio()
{
#if YUP_IOS
    return new iOSAudioIODeviceType();
#else
    return nullptr;
#endif
}

//==============================================================================
AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_WASAPI (WASAPIDeviceMode deviceMode)
{
#if YUP_WINDOWS && YUP_WASAPI
    auto windowsVersion = SystemStats::getOperatingSystemType();

    if (windowsVersion < SystemStats::WinVista
        || (WasapiClasses::isLowLatencyMode (deviceMode) && windowsVersion < SystemStats::Windows10))
        return nullptr;

    return new WasapiClasses::WASAPIAudioIODeviceType (deviceMode);
#else
    ignoreUnused (deviceMode);
    return nullptr;
#endif
}

AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_DirectSound()
{
#if YUP_WINDOWS && YUP_DIRECTSOUND
    return new DSoundAudioIODeviceType();
#else
    return nullptr;
#endif
}

AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_ASIO()
{
#if YUP_WINDOWS && YUP_ASIO
    return new ASIOAudioIODeviceType();
#else
    return nullptr;
#endif
}

//==============================================================================
AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_ALSA()
{
#if (YUP_LINUX || YUP_BSD) && YUP_ALSA
    return createAudioIODeviceType_ALSA_PCMDevices();
#else
    return nullptr;
#endif
}

AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_JACK()
{
#if (YUP_LINUX || YUP_BSD || YUP_MAC || YUP_WINDOWS) && YUP_JACK
    return new JackAudioIODeviceType();
#else
    return nullptr;
#endif
}

AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_Bela()
{
#if YUP_LINUX && YUP_BELA
    return new BelaAudioIODeviceType();
#else
    return nullptr;
#endif
}

//==============================================================================
AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_Android()
{
#if YUP_ANDROID
#if YUP_USE_ANDROID_OBOE
    if (isOboeAvailable())
        return nullptr;
#endif

#if YUP_USE_ANDROID_OPENSLES
    if (isOpenSLAvailable())
        return nullptr;
#endif

    return new AndroidAudioIODeviceType();
#else
    return nullptr;
#endif
}

AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_OpenSLES()
{
#if YUP_ANDROID && YUP_USE_ANDROID_OPENSLES
    return isOpenSLAvailable() ? new OpenSLAudioDeviceType() : nullptr;
#else
    return nullptr;
#endif
}

AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_Oboe()
{
#if YUP_ANDROID && YUP_USE_ANDROID_OBOE
    return isOboeAvailable() ? new OboeAudioIODeviceType() : nullptr;
#else
    return nullptr;
#endif
}

//==============================================================================
AudioIODeviceType* AudioIODeviceType::createAudioIODeviceType_AudioWorklet()
{
#if YUP_EMSCRIPTEN
    return new AudioWorkletAudioIODeviceType();
#else
    return nullptr;
#endif
}

} // namespace yup
