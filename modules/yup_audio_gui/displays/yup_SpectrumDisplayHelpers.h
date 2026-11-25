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

#pragma once

namespace yup::detail
{

/** Adds the properties the spectrum displays share to the type of one: its window, its ranges and its sample rate.

    An end of a range written past the other one moves it out of its way, so that a document writes them in any
    order, and a value a range can't take is ignored.
*/
template <class Display>
void addSpectrumDisplayProperties (ComponentTypeBuilder<Display>& builder)
{
    using D = Display;

    builder
        .template enumeration<WindowType> (
            "windowType",
            { { "hann", WindowType::hann },
              { "rectangular", WindowType::rectangular },
              { "hamming", WindowType::hamming },
              { "blackman", WindowType::blackman },
              { "blackmanHarris", WindowType::blackmanHarris },
              { "kaiser", WindowType::kaiser },
              { "gaussian", WindowType::gaussian },
              { "tukey", WindowType::tukey },
              { "bartlett", WindowType::bartlett },
              { "welch", WindowType::welch },
              { "flattop", WindowType::flattop },
              { "cosine", WindowType::cosine },
              { "lanczos", WindowType::lanczos },
              { "nuttall", WindowType::nuttall },
              { "blackmanNuttall", WindowType::blackmanNuttall },
              { "rakshitUllah", WindowType::rakshitUllah } },
            &D::getWindowType,
            &D::setWindowType,
            "The window applied to the samples before their FFT.")
        .property ("minFrequency", &D::getMinFrequency, [] (D& d, float v)
        {
            if (std::isfinite (v) && v > 0.0f)
                d.setFrequencyRange (v, d.getMaxFrequency() > v ? d.getMaxFrequency() : v * 2.0f);
        }, 20.0f, "The lowest frequency shown, in Hz.")
        .property ("maxFrequency", &D::getMaxFrequency, [] (D& d, float v)
        {
            if (std::isfinite (v) && v > 0.0f)
                d.setFrequencyRange (d.getMinFrequency() < v ? d.getMinFrequency() : v * 0.5f, v);
        }, 20000.0f, "The highest frequency shown, in Hz.")
        .property ("minDecibels", &D::getMinDecibels, [] (D& d, float v)
        {
            if (std::isfinite (v))
                d.setDecibelRange (v, d.getMaxDecibels() > v ? d.getMaxDecibels() : v + 1.0f);
        }, -100.0f, "The level at the bottom of the scale, in dB.")
        .property ("maxDecibels", &D::getMaxDecibels, [] (D& d, float v)
        {
            if (std::isfinite (v))
                d.setDecibelRange (d.getMinDecibels() < v ? d.getMinDecibels() : v - 1.0f, v);
        }, 0.0f, "The level at the top of the scale, in dB.")
        .property ("sampleRate", &D::getSampleRate, [] (D& d, double v)
        {
            if (std::isfinite (v) && v > 0.0)
                d.setSampleRate (v);
        }, 44100.0, "The sample rate of the audio the state is given, in Hz, which places the frequencies.");
}

} // namespace yup::detail
