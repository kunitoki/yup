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

/** A class that generates white noise. */
class WhiteNoise
{
public:
    /** Constructor. */
    WhiteNoise()
        : random (static_cast<int64> (std::chrono::steady_clock::now().time_since_epoch().count()))
    {
    }

    /** Constructor. */
    WhiteNoise (int64 seed)
        : random (seed)
    {
    }

    /** Set the seed for the random number generator. */
    void setSeed (int64 seed) noexcept
    {
        random.setSeed (seed);
    }

    /** Get the next sample of white noise. */
    float getNextSample() noexcept
    {
        return random.nextFloat() * 2.0f - 1.0f;
    }

    /** Get the next sample of white noise. */
    float operator()() noexcept
    {
        return random.nextFloat() * 2.0f - 1.0f;
    }

private:
    yup::Random random;
};

} // namespace yup
