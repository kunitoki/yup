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

/*
  ==============================================================================

  BEGIN_YUP_MODULE_DECLARATION

    ID:                   yup_audio_gui
    vendor:               yup
    version:              1.0.0
    name:                 YUP Audio GUI Components
    description:          Audio-related GUI components for the YUP library
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_audio_basics yup_dsp yup_gui

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_AUDIO_GUI_H_INCLUDED

#include <yup_audio_basics/yup_audio_basics.h>
#include <yup_dsp/yup_dsp.h>
#include <yup_gui/yup_gui.h>

//==============================================================================

#include "keyboard/yup_MidiKeyboardComponent.h"
#include "displays/yup_SpectrumAnalyzerComponent.h"
