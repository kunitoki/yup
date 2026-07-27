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

/** A class that generates pink noise. */
class PinkNoise
{
public:
    /** Constructor. */
    PinkNoise()
    {
        for (int i = 0; i < 7; ++i)
            pinkFilters[i] = 0.0;
    }

    /** Constructor. */
    PinkNoise (int64 seed)
        : whiteNoise (seed)
    {
        for (int i = 0; i < 7; ++i)
            pinkFilters[i] = 0.0;
    }

    /** Set the seed for the random number generator. */
    void setSeed (int64 seed) noexcept
    {
        whiteNoise.setSeed (seed);
    }

    /** Get the next sample of pink noise. */
    float getNextSample() noexcept
    {
        // Paul Kellett's refined method for pink noise
        float white = whiteNoise.getNextSample();

        pinkFilters[0] = 0.99886f * pinkFilters[0] + white * 0.0555179f;
        pinkFilters[1] = 0.99332f * pinkFilters[1] + white * 0.0750759f;
        pinkFilters[2] = 0.96900f * pinkFilters[2] + white * 0.1538520f;
        pinkFilters[3] = 0.86650f * pinkFilters[3] + white * 0.3104856f;
        pinkFilters[4] = 0.55000f * pinkFilters[4] + white * 0.5329522f;
        pinkFilters[5] = -0.7616f * pinkFilters[5] - white * 0.0168980f;

        float pink = pinkFilters[0] + pinkFilters[1] + pinkFilters[2] + pinkFilters[3] + pinkFilters[4] + pinkFilters[5] + pinkFilters[6] + white * 0.5362f;
        pinkFilters[6] = white * 0.115926f;

        return pink * 0.11f; // Scale down
    }

    /** Get the next sample of pink noise. */
    float operator()() noexcept
    {
        return getNextSample();
    }

private:
    WhiteNoise whiteNoise;
    double pinkFilters[7] = { 0.0 };
};

} // namespace yup
