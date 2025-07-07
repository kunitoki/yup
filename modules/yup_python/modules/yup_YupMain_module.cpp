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
#include "../utilities/yup_CrashHandling.h"

#include "../bindings/yup_YupCore_bindings.h"

/*
#if YUP_MODULE_AVAILABLE_yup_events
#include "yup_YupEvents_bindings.h"
#endif

#if YUP_MODULE_AVAILABLE_yup_data_model
#include "yup_YupDataModel_bindings.h"
#endif

#if YUP_MODULE_AVAILABLE_yup_graphics
#include "yup_YupGraphics_bindings.h"
#endif

#if YUP_MODULE_AVAILABLE_yup_gui
#include "yup_YupGui_bindings.h"
#include "yup_YupGuiEntryPoints_bindings.h"
#endif

#if YUP_MODULE_AVAILABLE_yup_audio_basics
#include "yup_YupAudioBasics_bindings.h"
#endif

#if YUP_MODULE_AVAILABLE_yup_audio_devices
#include "yup_YupAudioDevices_bindings.h"
#endif

#if YUP_MODULE_AVAILABLE_yup_audio_processors
#include "yup_YupAudioProcessors_bindings.h"
#endif
*/

//==============================================================================

#if YUP_PYTHON_EMBEDDED_INTERPRETER
PYBIND11_EMBEDDED_MODULE (YUP_PYTHON_MODULE_NAME, m)
#else
PYBIND11_MODULE (YUP_PYTHON_MODULE_NAME, m)
#endif
{
#if ! YUP_PYTHON_EMBEDDED_INTERPRETER
    yup::SystemStats::setApplicationCrashHandler (Helpers::applicationCrashHandler);
#endif

#if YUP_MAC
    yup::Process::setDockIconVisible (false);
#endif

    yup::Bindings::registerYupCoreBindings (m);

/*
#if YUP_MODULE_AVAILABLE_yup_events
    yup::Bindings::registerYupEventsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_yup_data_model
    yup::Bindings::registerYupDataModelBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_yup_graphics
    yup::Bindings::registerYupGraphicsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_yup_gui
    yup::Bindings::registerYupGuiBindings (m);
    yup::Bindings::registerYupGuiEntryPointsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_yup_audio_basics
    yup::Bindings::registerYupAudioBasicsBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_yup_audio_devices
    yup::Bindings::registerYupAudioDevicesBindings (m);
#endif

#if YUP_MODULE_AVAILABLE_yup_audio_processors
    yup::Bindings::registerYupAudioProcessorsBindings (m);
#endif
*/
}
