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
    version:            1.0.0
    name:               YUP Audio Plugin Host
    description:        In-process hosting of VST3, CLAP, and AUv2 audio plugins.
    website:            https://github.com/kunitoki/yup
    license:            ISC
    minimumCppStandard: 17

    dependencies:       yup_audio_processors
    searchpaths:        native

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AUDIO_PLUGIN_HOST_H_INCLUDED

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
#include "native/yup_AudioPluginInstance_AUv2.h"
