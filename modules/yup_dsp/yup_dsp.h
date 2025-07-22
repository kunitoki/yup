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

    ID:                   yup_dsp
    vendor:               yup
    version:              1.0.0
    name:                 YUP DSP
    description:          The essential set of basic YUP DSP.
    website:              https://github.com/kunitoki/yup
    license:              ISC

    dependencies:         yup_core yup_audio_basics
    appleFrameworks:      Accelerate

  END_YUP_MODULE_DECLARATION

  ==============================================================================
*/

#pragma once
#define YUP_DSP_H_INCLUDED

#include <yup_core/yup_core.h>
#include <yup_audio_basics/yup_audio_basics.h>

//==============================================================================

#include <array>
#include <cmath>
#include <complex>
#include <memory>
#include <vector>

//==============================================================================

// DSP utilities and mathematical functions
#include "utilities/yup_DspMath.h"

// Windowing functions
#include "windowing/yup_WindowFunctions.h"

// Base filter interfaces and common structures
#include "base/yup_FilterBase.h"

// Filter designers and coefficient calculators
#include "designers/yup_FilterDesigner.h"

// Core filter implementations
#include "filters/yup_Biquad.h"
#include "filters/yup_RbjFilter.h"
#include "filters/yup_StateVariableFilter.h"

//==============================================================================
