/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <algorithm>

namespace yup
{

/** Hash generator template.

    This template provides a mechanism to generate hash values for different types of data.
    It uses a multiplier to compute the hash value based on the input data.

    @tparam Type The type of the hash value to be generated (e.g., uint32, uint64).
*/
template <typename Type>
struct HashGenerator
{
    /** Calculates the hash value for the given character pointer.
    
        @tparam CharPointer The type of the character pointer (e.g., const char*, const wchar_t*).

        @param t The character pointer to be hashed.

        @return The computed hash value of the input data.
    */
    template <typename CharPointer>
    static Type calculate (CharPointer t) noexcept
    {
        Type result = {};

        while (! t.isEmpty())
            result = ((Type) multiplier) * result + (Type) t.getAndAdvance();

        return result;
    }

    /** The multiplier used in the hash calculation. */
    enum
    {
        multiplier = sizeof (Type) > 4 ? 101 : 31
    };
};

} // namespace yup
