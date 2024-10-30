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

    ID:                 yup_gui
    vendor:             yup
    version:            1.0.0
    name:               YUP Graphical User Interface
    description:        The essential set of basic YUP user interface.
    website:            https://github.com/kunitoki/yup
    license:            ISC
    minimumCppStandard: 17

    dependencies:       juce_events yup_graphics rive
    osxFrameworks:      Metal
    iosFrameworks:      Metal
    enableARC:          1

  END_JUCE_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_GUI_H_INCLUDED

#include <juce_events/juce_events.h>

#include <yup_graphics/yup_graphics.h>

//==============================================================================

#include <rive/artboard.hpp>
#include <rive/file.hpp>
#include <rive/static_scene.hpp>

//==============================================================================

#include "application/yup_Application.h"
#include "keyboard/yup_KeyModifiers.h"
#include "keyboard/yup_KeyPress.h"
#include "mouse/yup_MouseEvent.h"
#include "mouse/yup_MouseWheelData.h"
#include "desktop/yup_Display.h"
#include "desktop/yup_Desktop.h"
#include "component/yup_ComponentNative.h"
#include "component/yup_Component.h"
#include "widgets/yup_Button.h"
#include "widgets/yup_TextButton.h"
#include "widgets/yup_Slider.h"
#include "artboard/yup_Artboard.h"
#include "windowing/yup_DocumentWindow.h"
