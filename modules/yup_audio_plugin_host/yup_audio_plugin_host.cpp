/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#if YUP_MAC
#define YUP_CORE_INCLUDE_OBJC_HELPERS 1
#endif

#include "yup_audio_plugin_host.h"

#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

//==============================================================================
#include "host/yup_AudioPluginScanner.cpp"
#include "host/yup_AudioPluginInstance.cpp"

//==============================================================================
#if YUP_AUDIO_PLUGIN_HOST_ENABLE_VST3
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/base/ipluginbase.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivstaudioprocessor.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivsthostapplication.h>
#include <pluginterfaces/vst/vstpresetkeys.h>
#include <pluginterfaces/gui/iplugview.h>
#include <public.sdk/source/common/memorystream.h>
#include <public.sdk/source/vst/hosting/eventlist.h>
#include <public.sdk/source/vst/hosting/parameterchanges.h>

#if YUP_MAC
#import <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "native/yup_AudioPluginInstance_VST3.cpp"
#endif

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_CLAP
#include <clap/clap.h>

#if YUP_MAC
#import <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

#if YUP_WINDOWS
#include <windows.h>
using CLAPModuleHandle = HMODULE;
#define clapLoadModule(p) LoadLibraryA (p)
#define clapGetAddress GetProcAddress
#define clapUnloadModule FreeLibrary
#else
#include <dlfcn.h>
using CLAPModuleHandle = void*;
#define clapLoadModule(p) dlopen (p, RTLD_LAZY | RTLD_LOCAL)
#define clapGetAddress dlsym
#define clapUnloadModule dlclose
#endif

#include "native/yup_AudioPluginInstance_CLAP.cpp"
#endif

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_AU && YUP_MAC
#import <AppKit/AppKit.h>
#import <AudioUnit/AudioUnit.h>
#import <AudioUnit/AUCocoaUIView.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioToolbox/AudioUnitUtilities.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreMIDI/CoreMIDI.h>

#include "native/yup_AudioPluginInstance_AU.mm"
#endif
