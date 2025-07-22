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

namespace yup
{

//==============================================================================

template <typename CoeffType>
BiquadCoefficients<CoeffType> FilterDesigner<CoeffType>::designRbjImpl (
    FilterMode filterMode,
    CoeffType frequency,
    CoeffType q,
    CoeffType gain,
    double sampleRate) noexcept
{
    const auto omega = DspMath::frequencyToAngular (frequency, static_cast<CoeffType> (sampleRate));
    const auto cosOmega = std::cos (omega);
    const auto sinOmega = std::sin (omega);
    const auto alpha = sinOmega / (static_cast<CoeffType> (2.0) * q);
    const auto A = std::pow (static_cast<CoeffType> (10.0), gain / static_cast<CoeffType> (40.0));

    BiquadCoefficients<CoeffType> coeffs;

    switch (filterMode)
    {
        case FilterMode::lowpass:
            coeffs.b0 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
            coeffs.b1 = static_cast<CoeffType> (1.0) - cosOmega;
            coeffs.b2 = (static_cast<CoeffType> (1.0) - cosOmega) / static_cast<CoeffType> (2.0);
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
            break;

        case FilterMode::highpass:
            coeffs.b0 = (static_cast<CoeffType> (1.0) + cosOmega) / static_cast<CoeffType> (2.0);
            coeffs.b1 = -(static_cast<CoeffType> (1.0) + cosOmega);
            coeffs.b2 = (static_cast<CoeffType> (1.0) + cosOmega) / static_cast<CoeffType> (2.0);
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
            break;

        case FilterMode::bandpass:
            coeffs.b0 = alpha;
            coeffs.b1 = static_cast<CoeffType> (0.0);
            coeffs.b2 = -alpha;
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
            break;

        case FilterMode::bandstop:
            coeffs.b0 = static_cast<CoeffType> (1.0);
            coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.b2 = static_cast<CoeffType> (1.0);
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
            break;

        case FilterMode::peak:
            coeffs.b0 = static_cast<CoeffType> (1.0) + alpha * A;
            coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.b2 = static_cast<CoeffType> (1.0) - alpha * A;
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha / A;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha / A;
            break;

        case FilterMode::lowshelf:
        {
            const auto S = static_cast<CoeffType> (1.0);
            const auto beta = std::sqrt (A) / q;

            coeffs.b0 = A * ((A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega);
            coeffs.b1 = static_cast<CoeffType> (2.0) * A * ((A - static_cast<CoeffType> (1.0)) - (A + static_cast<CoeffType> (1.0)) * cosOmega);
            coeffs.b2 = A * ((A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega);
            coeffs.a0 = (A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * ((A - static_cast<CoeffType> (1.0)) + (A + static_cast<CoeffType> (1.0)) * cosOmega);
            coeffs.a2 = (A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega;
        }
        break;

        case FilterMode::highshelf:
        {
            const auto S = static_cast<CoeffType> (1.0);
            const auto beta = std::sqrt (A) / q;

            coeffs.b0 = A * ((A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega);
            coeffs.b1 = static_cast<CoeffType> (-2.0) * A * ((A - static_cast<CoeffType> (1.0)) + (A + static_cast<CoeffType> (1.0)) * cosOmega);
            coeffs.b2 = A * ((A + static_cast<CoeffType> (1.0)) + (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega);
            coeffs.a0 = (A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega + beta * sinOmega;
            coeffs.a1 = static_cast<CoeffType> (2.0) * ((A - static_cast<CoeffType> (1.0)) - (A + static_cast<CoeffType> (1.0)) * cosOmega);
            coeffs.a2 = (A + static_cast<CoeffType> (1.0)) - (A - static_cast<CoeffType> (1.0)) * cosOmega - beta * sinOmega;
        }
        break;

        case FilterMode::allpass:
            coeffs.b0 = static_cast<CoeffType> (1.0) - alpha;
            coeffs.b1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.b2 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a0 = static_cast<CoeffType> (1.0) + alpha;
            coeffs.a1 = static_cast<CoeffType> (-2.0) * cosOmega;
            coeffs.a2 = static_cast<CoeffType> (1.0) - alpha;
            break;

        default:
            break;
    }

    coeffs.normalize();
    return coeffs;
}

//==============================================================================

// Explicit instantiations for FilterDesigner
template class FilterDesigner<float>;
template class FilterDesigner<double>;

} // namespace yup
