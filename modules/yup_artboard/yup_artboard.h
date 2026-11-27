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

    ID:                   yup_artboard
    vendor:               yup
    version:              2.0.1
    name:                 YUP Artboard
    description:          Rive artboards as components, with state machines, node access and ViewModel data binding.
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_gui rive

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_ARTBOARD_H_INCLUDED

#include <yup_gui/yup_gui.h>

#include <rive/rive.h>

namespace rive::cmd
{
class DeferredSession;
class DeferredReplayer;
} // namespace rive::cmd

//==============================================================================

#include "artboard/yup_ArtboardViewModel.h"
#include "artboard/yup_ArtboardViewModelInstance.h"
#include "artboard/yup_ArtboardFile.h"
#include "artboard/yup_ArtboardNode.h"
#include "artboard/yup_Artboard.h"
