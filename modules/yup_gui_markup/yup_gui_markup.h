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

    ID:                   yup_gui_markup
    vendor:               yup
    version:              2.0.1
    name:                 YUP GUI Markup
    description:          YUI, the declarative language of .yui files building component trees with their styles, drawings and bindings.
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_gui
    optionalDeps:         yup_renderer

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_GUI_MARKUP_H_INCLUDED

#include <yup_gui/yup_gui.h>

//==============================================================================

#include "markup/yup_YuiParser.h"
#include "markup/yup_YuiWriter.h"
#include "markup/yup_YuiSvgImporter.h"
#include "markup/yup_YuiTreeDiff.h"
#include "markup/yup_YuiBuilder.h"
#include "markup/yup_YuiDocument.h"
#include "markup/yup_YuiLoader.h"
#include "markup/yup_YuiView.h"
