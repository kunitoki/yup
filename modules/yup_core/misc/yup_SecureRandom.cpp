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

namespace yup
{

bool SecureRandom::fillBytes (void* destination, size_t numBytes) noexcept
{
#if YUP_WINDOWS
    return BCRYPT_SUCCESS (BCryptGenRandom (nullptr, static_cast<PUCHAR> (destination), (ULONG) numBytes, BCRYPT_USE_SYSTEM_PREFERRED_RNG));
#else
    // getentropy accepts at most 256 bytes per call
    auto* bytes = static_cast<uint8*> (destination);

    while (numBytes > 0)
    {
        const auto chunk = jmin (numBytes, (size_t) 256);

        if (getentropy (bytes, chunk) != 0)
            return false;

        bytes += chunk;
        numBytes -= chunk;
    }

    return true;
#endif
}

} // namespace yup
