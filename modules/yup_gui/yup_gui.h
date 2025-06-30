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

    ID:                   yup_gui
    vendor:               yup
    version:              1.0.0
    name:                 YUP Graphical User Interface
    description:          The essential set of basic YUP user interface.
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_events yup_data_model yup_graphics rive
    appleFrameworks:      Metal
    iosWeakFrameworks:    UniformTypeIdentifiers
    iosSimWeakFrameworks: UniformTypeIdentifiers
    enableARC:            1

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_GUI_H_INCLUDED

#include <yup_events/yup_events.h>
#include <yup_data_model/yup_data_model.h>
#include <yup_graphics/yup_graphics.h>

#include <rive/rive.h>

//==============================================================================
/** Config: YUP_ENABLE_COMPONENT_REPAINT_DEBUGGING

    Enable repaint debugging for components.
*/
#ifndef YUP_ENABLE_COMPONENT_REPAINT_DEBUGGING
#define YUP_ENABLE_COMPONENT_REPAINT_DEBUGGING 0
#endif

//==============================================================================

#include <tuple>
#include <unordered_map>

//==============================================================================

#include "application/yup_Application.h"
#include "keyboard/yup_KeyModifiers.h"
#include "keyboard/yup_KeyPress.h"
#include "mouse/yup_MouseEvent.h"
#include "mouse/yup_MouseCursor.h"
#include "mouse/yup_MouseWheelData.h"
#include "mouse/yup_MouseListener.h"
#include "clipboard/yup_SystemClipboard.h"
#include "desktop/yup_Screen.h"
#include "desktop/yup_Desktop.h"
#include "component/yup_ComponentNative.h"
#include "component/yup_ComponentStyle.h"
#include "component/yup_Component.h"
#include "menus/yup_PopupMenu.h"
#include "buttons/yup_Button.h"
#include "buttons/yup_TextButton.h"
#include "buttons/yup_ToggleButton.h"
#include "buttons/yup_SwitchButton.h"
#include "buttons/yup_ImageButton.h"
#include "widgets/yup_TextEditor.h"
#include "widgets/yup_Label.h"
#include "widgets/yup_Slider.h"
#include "widgets/yup_ComboBox.h"
#include "artboard/yup_ArtboardFile.h"
#include "artboard/yup_Artboard.h"
#include "windowing/yup_DocumentWindow.h"
#include "dialogs/yup_FileChooser.h"

//==============================================================================

#include "native/yup_WindowingHelpers.h"

//==============================================================================

#include "themes/yup_ApplicationTheme.h"
#include "themes/theme_v1/yup_ThemeVersion1.h"
