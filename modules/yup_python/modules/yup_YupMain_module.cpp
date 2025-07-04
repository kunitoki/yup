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

#include "../utilities/yup_PyBind11Includes.h"
#include "../bindings/yup_YupCore_bindings.h"

#if YUP_MODULE_AVAILABLE_juce_events
#include "ScriptJuceEventsBindings.h"
#endif

#if YUP_MODULE_AVAILABLE_juce_data_structures
#include "ScriptJuceDataStructuresBindings.h"
#endif

#if YUP_MODULE_AVAILABLE_juce_graphics
#include "ScriptJuceGraphicsBindings.h"
#endif

#if YUP_MODULE_AVAILABLE_juce_gui_basics
#include "ScriptJuceGuiBasicsBindings.h"
#include "ScriptJuceGuiEntryPointsBindings.h"
#endif

#if YUP_MODULE_AVAILABLE_juce_gui_extra
#include "ScriptJuceGuiExtraBindings.h"
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_basics
#include "ScriptJuceAudioBasicsBindings.h"
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_devices
#include "ScriptJuceAudioDevicesBindings.h"
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_processors
#include "ScriptJuceAudioProcessorsBindings.h"
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_formats
#include "ScriptJuceAudioFormatsBindings.h"
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_utils
#include "ScriptJuceAudioUtilsBindings.h"
#endif

//==============================================================================

#if YUP_PYTHON_EMBEDDED_INTERPRETER
PYBIND11_EMBEDDED_MODULE (YUP_PYTHON_MODULE_NAME, m)
#else
PYBIND11_MODULE (YUP_PYTHON_MODULE_NAME, m)
#endif
{
#if YUP_MAC
    yup::Process::setDockIconVisible (false);
#endif

    yup::Bindings::registerYupCoreBindings (m);

#if YUP_MODULE_AVAILABLE_juce_events
    yup::Bindings::registerJuceEventsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_juce_data_structures
    yup::Bindings::registerJuceDataStructuresBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_juce_graphics
    yup::Bindings::registerJuceGraphicsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_juce_gui_basics
    yup::Bindings::registerJuceGuiBasicsBindings (m);
    yup::Bindings::registerJuceGuiEntryPointsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_juce_gui_extra
    yup::Bindings::registerJuceGuiExtraBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_basics
    yup::Bindings::registerJuceAudioBasicsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_devices
    yup::Bindings::registerJuceAudioDevicesBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_processors
    yup::Bindings::registerJuceAudioProcessorsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_formats
    yup::Bindings::registerJuceAudioFormatsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_juce_audio_utils
    yup::Bindings::registerJuceAudioUtilsBindings (m);
#endif
}
