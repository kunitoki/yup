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

#include "SynthModulationMatrix.h"

bool SynthModulationValues::hasSpectrumRoute (int oscillator) const noexcept
{
    for (const auto& route : routes)
    {
        if (route.depth != 0.0f && isSpectrumDestination (route.destination)
            && getDestinationOscillator (route.destination) == oscillator)
            return true;
    }

    return false;
}

float* getDestinationField (SynthPatchValues& patch, SynthModulationDestination destination) noexcept
{
    switch (destination)
    {
        case SynthModulationDestination::filterCutoff: return &patch.filter.cutoff;
        case SynthModulationDestination::filterResonance: return &patch.filter.resonance;
        case SynthModulationDestination::filterDrive: return &patch.filter.drive;
        default: break;
    }

    const auto oscillator = getDestinationOscillator (destination);

    if (oscillator < 0)
        return nullptr;

    auto& values = patch.oscillators[static_cast<std::size_t> (oscillator)];

    switch ((static_cast<int> (destination) - 1) % synthOscillatorDestinations)
    {
        case 0: return &values.level;
        case 1: return &values.detuneSemitones;
        case 2: return &values.ridgeSpacing;
        case 3: return &values.color;
        case 4: return &values.dispersion;
        case 5: return &values.squeeze;
        case 6: return &values.squash;
        case 7: return &values.tilt;
        case 8: return &values.oddEven;
        case 9: return &values.formant;
        case 10: return &values.formantPosition;
        case 11: return &values.scatter;
        case 12: return &values.syncRatio;
        case 13: return &values.unisonDetune;
        case 14: return &values.unisonSpread;
        default: return nullptr;
    }
}

void applyModulation (SynthPatchValues& patch,
                      const SynthModulationValues& modulation,
                      const std::array<float, 5>& sourceValues) noexcept
{
    for (const auto& route : modulation.routes)
    {
        const auto source = sourceValues[static_cast<std::size_t> (route.source)];

        if (route.depth == 0.0f || source == 0.0f)
            continue;

        if (auto* field = getDestinationField (patch, route.destination))
            *field = getDestinationRange (route.destination).modulate (*field, route.depth * source);
    }
}
