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
/** Config: YUP_ENABLE_FFTW3

    Enable FFTW3 backend.
*/
#ifndef YUP_ENABLE_FFTW3
#define YUP_ENABLE_FFTW3 0
#endif

/** Config: YUP_USE_INTEL_IPP

    Use Intel IPP backend.
*/
#ifndef YUP_ENABLE_INTEL_IPP
#define YUP_ENABLE_INTEL_IPP 0
#endif

/** Config: YUP_ENABLE_VDSP

    Enable Apple's vDSP backend.
*/
#ifndef YUP_ENABLE_VDSP
#if (YUP_MAC || YUP_IOS)
#define YUP_ENABLE_VDSP 1
#else
#define YUP_ENABLE_VDSP 0
#endif
#endif

/** Config: YUP_ENABLE_PFFFT

    Enable PFFFT backend.
*/
#ifndef YUP_ENABLE_PFFFT
#define YUP_ENABLE_PFFFT 1
#endif

/** Config: YUP_ENABLE_OOURA

    Enable OOURA backend.
*/
#ifndef YUP_ENABLE_OOURA
#define YUP_ENABLE_OOURA 1
#endif

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

// Frequency domain functions
#include "frequency/yup_FFTProcessor.h"
#include "frequency/yup_SpectrumAnalyzerState.h"

// Base filter interfaces and common structures
#include "base/yup_FilterBase.h"
#include "base/yup_FirstOrderCoefficients.h"
#include "base/yup_BiquadCoefficients.h"
#include "base/yup_StateVariableCoefficients.h"
#include "base/yup_FirstOrder.h"
#include "base/yup_Biquad.h"
#include "base/yup_BiquadCascade.h"
#include "base/yup_FilterCharacteristics.h"

// Filter designers and coefficient calculators
#include "designers/yup_FilterDesigner.h"

// Filter implementations
#include "filters/yup_FirstOrderFilter.h"
#include "filters/yup_BiquadFilter.h"
#include "filters/yup_RbjFilter.h"
#include "filters/yup_ZoelzerFilter.h"
#include "filters/yup_StateVariableFilter.h"
#include "filters/yup_ButterworthFilter.h"

//==============================================================================
