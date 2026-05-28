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
*/

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                 yup_audio_plugin_client
    vendor:             yup
    version:            1.0.0
    name:               YUP Audio Plugin Client
    description:        The essential set of basic YUP audio plugin clients.
    website:            https://github.com/kunitoki/yup
    license:            ISC

    dependencies:       yup_audio_processors yup_gui

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AUDIO_PLUGIN_CLIENT_H_INCLUDED

//==============================================================================
/** Config: YUP_ENABLE_PLUGIN_CLIENT_AU_LOGGING

    Enable debug logging for AUv2 plugin client.
*/
#ifndef YUP_ENABLE_PLUGIN_CLIENT_AU_LOGGING
#define YUP_ENABLE_PLUGIN_CLIENT_AU_LOGGING 0
#endif

/** Config: YUP_ENABLE_PLUGIN_CLIENT_CLAP_LOGGING

    Enable debug logging for CLAP plugin client.
*/
#ifndef YUP_ENABLE_PLUGIN_CLIENT_CLAP_LOGGING
#define YUP_ENABLE_PLUGIN_CLIENT_CLAP_LOGGING 0
#endif

/** Config: YUP_ENABLE_PLUGIN_CLIENT_VST3_LOGGING

    Enable debug logging for VST3 plugin client.
*/
#ifndef YUP_ENABLE_PLUGIN_CLIENT_VST3_LOGGING
#define YUP_ENABLE_PLUGIN_CLIENT_VST3_LOGGING 0
#endif

//==============================================================================

#include <yup_audio_processors/yup_audio_processors.h>
