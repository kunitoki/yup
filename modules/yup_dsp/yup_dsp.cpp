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

#ifdef YUP_DSP_H_INCLUDED
/* When you add this cpp file to your project, you mustn't include it in a file where you've
   already included any other headers - just put it inside a file on its own, possibly with your config
   flags preceding it, but don't include anything else. That also includes avoiding any automatic prefix
   header files that the compiler may be using.
*/
#error "Incorrect use of YUP cpp file"
#endif

#include "yup_dsp.h"

//==============================================================================

#include <atomic>
#include <thread>

//==============================================================================

#if YUP_ENABLE_BUNGEE
#include <bungee_library/bungee_library.h>
#endif

//==============================================================================

#if ! YUP_FFT_FOUND_BACKEND && YUP_ENABLE_VDSP && (YUP_MAC || YUP_IOS)
#define YUP_FFT_USING_VDSP 1
#define YUP_FFT_FOUND_BACKEND 1
#include <Accelerate/Accelerate.h>
#endif

#if ! YUP_FFT_FOUND_BACKEND && YUP_ENABLE_INTEL_IPP && __has_include(<ipp.h>)
#include <ipp.h>
#define YUP_FFT_USING_IPP 1
#define YUP_FFT_FOUND_BACKEND 1
#endif

#if ! YUP_FFT_FOUND_BACKEND && YUP_ENABLE_FFTW3 && __has_include(<fftw3.h>)
#include <fftw3.h>
#define YUP_FFT_USING_FFTW3 1
#define YUP_FFT_FOUND_BACKEND 1
#endif

#if ! YUP_FFT_FOUND_BACKEND && YUP_ENABLE_PFFFT && YUP_MODULE_AVAILABLE_pffft_library
#include <pffft_library/pffft_library.h>
#define YUP_FFT_USING_PFFFT 1
#define YUP_FFT_FOUND_BACKEND 1
#endif

#if ! YUP_FFT_FOUND_BACKEND && YUP_ENABLE_OOURA
#include "frequency/yup_OouraFFT8g.h"
#define YUP_FFT_USING_OOURA 1
#define YUP_FFT_FOUND_BACKEND 1
#endif

#if ! defined(YUP_FFT_FOUND_BACKEND)
#error "Unable to find a proper FFT backend !"
#endif

//==============================================================================

#include "frequency/yup_FFTProcessor.cpp"
#include "frequency/yup_SpectrumAnalyzerState.cpp"
#include "onsets/yup_Spectrogram.cpp"
#include "onsets/yup_FilterBank.cpp"
#include "onsets/yup_OnsetDetectionFunction.cpp"
#include "onsets/yup_OnsetPeakPicker.cpp"
#include "onsets/yup_OnsetDetector.cpp"
#include "metering/yup_LevelProcessor.cpp"
#include "metering/yup_LoudnessFilter.cpp"
#include "metering/yup_KMeterState.cpp"
#include "designers/yup_FilterDesigner.cpp"
#include "designers/yup_AnalogFilterDesigner.cpp"
#include "convolution/yup_PartitionedConvolver.cpp"
#include "stretching/yup_TimeStretchProcessorEngine.h"
#include "stretching/yup_TimeStretchTimeDomainBackend.h"
#include "stretching/yup_TimeStretchBungeeBackend.h"
#include "stretching/yup_TimeStretchProcessor.cpp"
#include "utilities/yup_DspMath.cpp"

//==============================================================================

#if YUP_ENABLE_OOURA && YUP_FFT_USING_OOURA
#include "frequency/yup_OouraFFT8g.cpp"
#endif
