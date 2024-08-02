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

/*******************************************************************************

  BEGIN_JUCE_MODULE_DECLARATION

    ID:                 yup_data_model
    vendor:             yup
    version:            1.0.0
    name:               YUP Data Model
    description:        The essential set of basic YUP data model classes.
    website:            https://github.com/kunitoki/yup
    license:            ISC
    minimumCppStandard: 17

    dependencies:       juce_events
    enableARC:          1

  END_JUCE_MODULE_DECLARATION

*******************************************************************************/

#pragma once
#define YUP_DATA_MODEL_H_INCLUDED

#include <juce_events/juce_events.h>

//==============================================================================
#include "undo/yup_UndoableAction.h"
#include "undo/yup_UndoManager.h"
