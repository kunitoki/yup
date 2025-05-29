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

  BEGIN_JUCE_MODULE_DECLARATION

    ID:                 yup_audio_processors
    vendor:             yup
    version:            1.0.0
    name:               YUP Audio Processors
    description:        The essential set of basic YUP audio processing classes.
    website:            https://github.com/kunitoki/yup
    license:            ISC
    minimumCppStandard: 17

    dependencies:       juce_audio_basics yup_gui
    enableARC:          1

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AUDIO_PROCESSORS_H_INCLUDED

#include <juce_audio_basics/juce_audio_basics.h>

#include <yup_gui/yup_gui.h>

//==============================================================================
#include "processors/yup_AudioBus.h"
#include "processors/yup_AudioBusLayout.h"
#include "processors/yup_AudioParameter.h"
#include "processors/yup_AudioParameterBuilder.h"
#include "processors/yup_AudioParameterHandle.h"
#include "processors/yup_AudioProcessor.h"
#include "processors/yup_AudioProcessorEditor.h"
