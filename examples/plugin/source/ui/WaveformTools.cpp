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

#include "WaveformTools.h"

#include <vector>

namespace
{
/** Mirrors the reconstructed cycle and analyzes it back, since a mirror has no exact
    counterpart on the coefficients. Allocates, so it runs on the message thread. */
void mirrorPartials (SynthPartials::Snapshot& partials)
{
    constexpr int resolution = 512;
    constexpr int harmonics = SynthExample::editableHarmonics;

    yup::FourierSeries<double> series (harmonics);

    for (int harmonic = 1; harmonic <= harmonics; ++harmonic)
    {
        const auto index = static_cast<std::size_t> (harmonic - 1);
        series.setHarmonic (harmonic, partials.cosines[index], partials.sines[index]);
    }

    std::vector<float> cycle (resolution);

    for (int sample = 0; sample <= resolution / 2; ++sample)
        cycle[static_cast<std::size_t> (sample)] = evaluateFourierSeries (series, static_cast<double> (sample) / resolution, harmonics);

    for (int sample = resolution / 2 + 1; sample < resolution; ++sample)
        cycle[static_cast<std::size_t> (sample)] = cycle[static_cast<std::size_t> (resolution - sample)];

    series.setFromCycle (yup::Span<const float> (cycle));

    for (int harmonic = 1; harmonic <= harmonics; ++harmonic)
    {
        const auto index = static_cast<std::size_t> (harmonic - 1);
        partials.cosines[index] = static_cast<float> (series.getCosine (harmonic));
        partials.sines[index] = static_cast<float> (series.getSine (harmonic));
    }
}
} // namespace

void stampWaveformShape (yup::Span<float> cycle,
                         WaveformDrawTool tool,
                         int fromIndex,
                         float fromValue,
                         int toIndex,
                         float toValue,
                         int gridDivisions,
                         yup::int64 seed) noexcept
{
    jassert (tool != WaveformDrawTool::brush);

    const auto size = static_cast<int> (cycle.size());

    if (size <= 0)
        return;

    fromIndex = yup::jlimit (0, size - 1, fromIndex);
    toIndex = yup::jlimit (0, size - 1, toIndex);

    auto first = yup::jmin (fromIndex, toIndex);
    auto last = yup::jmax (fromIndex, toIndex);
    auto cells = 1;
    const auto cellSize = gridDivisions > 0 ? static_cast<double> (size) / static_cast<double> (gridDivisions) : 0.0;

    if (gridDivisions > 0)
    {
        const auto firstCell = static_cast<int> (first / cellSize);
        const auto lastCell = static_cast<int> (last / cellSize);

        first = yup::roundToInt (firstCell * cellSize);
        last = yup::jmin (size - 1, yup::roundToInt ((lastCell + 1) * cellSize) - 1);
        cells = lastCell - firstCell + 1;
    }

    const auto length = last - first + 1;
    const auto forward = toIndex >= fromIndex;

    // Hashed per sample, or per grid cell, so the noise already drawn stays put as the span grows.
    // A hash rather than a Random per key: neighboring seeds give nearly equal first values.
    const auto noiseAt = [seed] (int key)
    {
        auto hash = static_cast<yup::uint64> (seed) + static_cast<yup::uint64> (key) * 0x9e3779b97f4a7c15ull;
        hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ull;
        hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebull;
        hash ^= hash >> 31;

        return static_cast<float> (hash >> 40) / static_cast<float> (1 << 24) * 2.0f - 1.0f;
    };

    for (int index = first; index <= last; ++index)
    {
        // How far along the span from the press toward the pointer, 0 to 1
        const auto along = static_cast<float> (forward ? index - first : last - index);
        const auto scaled = (along + 0.5f) / static_cast<float> (length) * static_cast<float> (cells);
        const auto cell = yup::jmin (cells - 1, static_cast<int> (scaled));
        const auto phase = scaled - static_cast<float> (cell);

        auto shape = 1.0f;

        switch (tool)
        {
            case WaveformDrawTool::line:
                shape = length > 1 ? along / static_cast<float> (length - 1) : 1.0f;
                break;

            case WaveformDrawTool::arc:
                shape = std::sin (yup::MathConstants<float>::pi * phase);
                break;

            case WaveformDrawTool::zigzag:
                shape = 1.0f - std::abs (2.0f * phase - 1.0f);
                break;

            case WaveformDrawTool::steps:
                shape = static_cast<float> (cell + 1) / static_cast<float> (cells);
                break;

            case WaveformDrawTool::noise:
                shape = noiseAt (gridDivisions > 0 ? static_cast<int> (index / cellSize) : index);
                break;

            case WaveformDrawTool::brush:
                break;
        }

        cycle[static_cast<std::size_t> (index)] = yup::jlimit (-1.0f, 1.0f, fromValue + (toValue - fromValue) * shape);
    }
}

void applyPartialsOperation (SynthPartials::Snapshot& partials, PartialsOperation operation, yup::Random& random)
{
    if (operation == PartialsOperation::mirror)
    {
        mirrorPartials (partials);
        return;
    }

    auto& cosines = partials.cosines;
    auto& sines = partials.sines;

    // Index 0 is the fundamental, so the odd harmonics sit at the even indices.
    for (std::size_t index = 0; index < cosines.size(); ++index)
    {
        const auto isOddHarmonic = index % 2 == 0;

        switch (operation)
        {
            case PartialsOperation::invert:
                cosines[index] = -cosines[index];
                sines[index] = -sines[index];
                break;

            case PartialsOperation::reverse:
                sines[index] = -sines[index];
                break;

            case PartialsOperation::mirror:
                break;

            case PartialsOperation::oddOnly:
            case PartialsOperation::evenOnly:
                if (isOddHarmonic != (operation == PartialsOperation::oddOnly))
                    cosines[index] = sines[index] = 0.0f;
                break;

            case PartialsOperation::randomizePhases:
            {
                const auto magnitude = std::hypot (cosines[index], sines[index]);
                const auto phase = random.nextFloat() * yup::MathConstants<float>::twoPi;

                cosines[index] = magnitude * std::sin (phase);
                sines[index] = magnitude * std::cos (phase);
                break;
            }
        }
    }
}
