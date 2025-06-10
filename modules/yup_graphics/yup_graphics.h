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

    ID:                 yup_graphics
    vendor:             yup
    version:            1.0.0
    name:               YUP Graphics Classes
    description:        The essential set of basic YUP graphics classes.
    website:            https://github.com/kunitoki/yup
    license:            ISC
    minimumCppStandard: 17

    dependencies:       juce_core rive rive_renderer
    appleFrameworks:    Metal
    searchpaths:        native
    enableARC:          1

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_GRAPHICS_H_INCLUDED

#include <juce_core/juce_core.h>

#include <rive_renderer/rive_renderer.h>
#include <rive_decoders/rive_decoders.h>

//==============================================================================

YUP_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wattributes", "-Wdeprecated-declarations")
#include <rive/rive.h>
#include <rive/text/raw_text.hpp>
#include <rive/text/utf.hpp>
YUP_END_IGNORE_WARNINGS_GCC_LIKE

//==============================================================================

#include <tuple>

//==============================================================================

#include "primitives/yup_AffineTransform.h"
#include "primitives/yup_Size.h"
#include "primitives/yup_Point.h"
#include "primitives/yup_Line.h"
#include "primitives/yup_Rectangle.h"
#include "primitives/yup_RectangleList.h"
#include "primitives/yup_Path.h"
#include "fonts/yup_Font.h"
#include "fonts/yup_StyledText.h"
#include "imaging/yup_Image.h"
#include "graphics/yup_Color.h"
#include "graphics/yup_ColorGradient.h"
#include "graphics/yup_Colors.h"
#include "graphics/yup_StrokeJoin.h"
#include "graphics/yup_StrokeCap.h"
#include "graphics/yup_Graphics.h"
#include "context/yup_GraphicsContext.h"
