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

namespace yup
{

//==============================================================================
/**
    Supplies bytes from the operating system's cryptographically secure generator.

    Unlike Random, whose output can be predicted from a few samples, these bytes are
    unpredictable. Use it for identifiers, temporary file names and anything else an
    attacker must not be able to guess.

    @see Random, Uuid
*/
class YUP_API SecureRandom
{
public:
    //==============================================================================
    /** Fills a buffer with secure random bytes.

        @returns true if the operating system supplied all the bytes. On false the
                 contents of the buffer are unspecified.
    */
    static bool fillBytes (void* destination, size_t numBytes) noexcept;

private:
    SecureRandom() = delete;
};

} // namespace yup
