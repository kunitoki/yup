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

#include "SynthPartials.h"

namespace
{
/** Points the level measurement sums, matching the waveform display's resolution. */
constexpr int measureResolution = 256;

const yup::Identifier customProperty ("custom");
const yup::Identifier cosinesProperty ("cosines");
const yup::Identifier sinesProperty ("sines");

yup::String joinCoefficients (const std::array<std::atomic<float>, SynthExample::editableHarmonics>& coefficients)
{
    yup::StringArray values;

    for (const auto& coefficient : coefficients)
        values.add (yup::String (static_cast<double> (coefficient.load())));

    return values.joinIntoString (" ");
}

void splitCoefficients (const yup::String& text, std::array<std::atomic<float>, SynthExample::editableHarmonics>& coefficients)
{
    const auto values = yup::StringArray::fromTokens (text, false);

    for (std::size_t index = 0; index < coefficients.size(); ++index)
        coefficients[index].store (static_cast<int> (index) < values.size() ? values[static_cast<int> (index)].getFloatValue() : 0.0f);
}
} // namespace

float evaluateFourierSeries (const yup::FourierSeries<double>& series, double phase, int maxHarmonic) noexcept
{
    const auto count = yup::jmin (maxHarmonic, series.getNumHarmonics());
    const auto theta = yup::MathConstants<double>::twoPi * phase;

    auto value = series.getDC();

    for (int harmonic = 1; harmonic <= count; ++harmonic)
    {
        const auto angle = theta * harmonic;

        value += series.getCosine (harmonic) * std::cos (angle)
               + series.getSine (harmonic) * std::sin (angle);
    }

    return static_cast<float> (value);
}

//==============================================================================
SynthPartials::SynthPartials()
{
    for (auto& cosine : harmonicCosines)
        cosine.store (0.0f);

    for (auto& sine : harmonicSines)
        sine.store (0.0f);
}

void SynthPartials::copyHarmonicsInto (yup::FourierSeries<double>& series, float scale) const noexcept
{
    series.clear();

    const auto count = yup::jmin (SynthExample::editableHarmonics, series.getNumHarmonics());

    for (int harmonic = 1; harmonic <= count; ++harmonic)
    {
        const auto index = static_cast<std::size_t> (harmonic - 1);

        series.setHarmonic (harmonic,
                            static_cast<double> (harmonicCosines[index].load() * scale),
                            static_cast<double> (harmonicSines[index].load() * scale));
    }
}

void SynthPartials::seedHarmonicsFrom (const yup::FourierSeries<double>& series) noexcept
{
    const auto count = yup::jmin (SynthExample::editableHarmonics, series.getNumHarmonics());

    for (int harmonic = 1; harmonic <= count; ++harmonic)
    {
        const auto index = static_cast<std::size_t> (harmonic - 1);

        harmonicCosines[index].store (static_cast<float> (series.getCosine (harmonic)));
        harmonicSines[index].store (static_cast<float> (series.getSine (harmonic)));
    }

    for (int harmonic = count; harmonic < SynthExample::editableHarmonics; ++harmonic)
    {
        harmonicCosines[static_cast<std::size_t> (harmonic)].store (0.0f);
        harmonicSines[static_cast<std::size_t> (harmonic)].store (0.0f);
    }
}

void SynthPartials::setHarmonicMagnitude (int index, float magnitude) noexcept
{
    jassert (yup::isPositiveAndBelow (index, SynthExample::editableHarmonics));

    auto& cosine = harmonicCosines[static_cast<std::size_t> (index)];
    auto& sine = harmonicSines[static_cast<std::size_t> (index)];

    const auto currentCosine = cosine.load();
    const auto currentSine = sine.load();
    const auto currentMagnitude = std::hypot (currentCosine, currentSine);

    if (currentMagnitude < 1.0e-6f)
    {
        cosine.store (0.0f);
        sine.store (magnitude);
        return;
    }

    const auto scale = magnitude / currentMagnitude;

    cosine.store (currentCosine * scale);
    sine.store (currentSine * scale);
}

//==============================================================================
void SynthPartials::updateHarmonicScale()
{
    if (! usesCustomSeries())
        return;

    yup::FourierSeries<double> series;
    series.resize (SynthExample::editableHarmonics);
    copyHarmonicsInto (series, 1.0f);

    auto peak = 0.0f;

    for (int index = 0; index < measureResolution; ++index)
    {
        const auto phase = static_cast<double> (index) / static_cast<double> (measureResolution - 1);

        peak = yup::jmax (peak, std::abs (evaluateFourierSeries (series, phase, SynthExample::displayHarmonics)));
    }

    if (peak > 1.0e-6f)
        harmonicScale.store (1.0f / peak);
}

void SynthPartials::fillValues (SynthOscillatorValues& values) const noexcept
{
    values.harmonicScale = harmonicScale.load();
    values.usesCustomSeries = customSeries.load();
    values.harmonicGeneration = generation.load();
}

//==============================================================================
void SynthPartials::writeTo (yup::DataTree& tree) const
{
    auto transaction = tree.beginTransaction();
    transaction.setProperty (customProperty, usesCustomSeries());
    transaction.setProperty (cosinesProperty, joinCoefficients (harmonicCosines));
    transaction.setProperty (sinesProperty, joinCoefficients (harmonicSines));
}

void SynthPartials::readFrom (const yup::DataTree& tree)
{
    splitCoefficients (tree.getProperty (cosinesProperty).toString(), harmonicCosines);
    splitCoefficients (tree.getProperty (sinesProperty).toString(), harmonicSines);
    setUsesCustomSeries (static_cast<bool> (tree.getProperty (customProperty, false)));

    updateHarmonicScale();
}
