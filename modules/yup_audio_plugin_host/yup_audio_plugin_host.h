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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                 yup_audio_plugin_host
    vendor:             yup
    version:            2.0.0
    name:               YUP Audio Plugin Host
    description:        In-process hosting of VST3, CLAP, LV2, and AU (v2 and v3) audio plugins.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_audio_processors
    searchpaths:        native

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AUDIO_PLUGIN_HOST_H_INCLUDED

//==============================================================================
/** Config: YUP_ENABLE_PLUGIN_HOST_AU_LOGGING

    Enable debug logging for AU plugin scanning and loading.
*/
#ifndef YUP_ENABLE_PLUGIN_HOST_AU_LOGGING
#define YUP_ENABLE_PLUGIN_HOST_AU_LOGGING 0
#endif

/** Config: YUP_ENABLE_PLUGIN_HOST_CLAP_LOGGING

    Enable debug logging for CLAP plugin scanning and loading.
*/
#ifndef YUP_ENABLE_PLUGIN_HOST_CLAP_LOGGING
#define YUP_ENABLE_PLUGIN_HOST_CLAP_LOGGING 0
#endif

/** Config: YUP_ENABLE_PLUGIN_HOST_VST3_LOGGING

    Enable debug logging for VST3 plugin scanning and loading.
*/
#ifndef YUP_ENABLE_PLUGIN_HOST_VST3_LOGGING
#define YUP_ENABLE_PLUGIN_HOST_VST3_LOGGING 0
#endif

/** Config: YUP_ENABLE_PLUGIN_HOST_LV2_LOGGING

    Enable debug logging for LV2 plugin scanning and loading.
*/
#ifndef YUP_ENABLE_PLUGIN_HOST_LV2_LOGGING
#define YUP_ENABLE_PLUGIN_HOST_LV2_LOGGING 0
#endif

//==============================================================================
#include <yup_audio_processors/yup_audio_processors.h>

//==============================================================================
#include "host/yup_AudioPluginFormatType.h"
#include "host/yup_AudioPluginDescription.h"
#include "host/yup_AudioPluginHostContext.h"
#include "host/yup_AudioPluginFormat.h"
#include "host/yup_AudioPluginScanner.h"
#include "host/yup_AudioPluginInstance.h"

//==============================================================================
#include "native/yup_AudioPluginInstance_VST3.h"
#include "native/yup_AudioPluginInstance_CLAP.h"
#include "native/yup_AudioPluginInstance_AU.h"
#include "native/yup_AudioPluginInstance_LV2.h"
