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

SharedMemory::SharedMemory() = default;

SharedMemory::~SharedMemory()
{
    close();
}

SharedMemory::SharedMemory (SharedMemory&& other) noexcept
    : data (std::exchange (other.data, nullptr))
    , size (std::exchange (other.size, 0))
    , name (std::move (other.name))
    , owner (std::exchange (other.owner, false))
    , handle (std::exchange (other.handle, 0))
{
}

SharedMemory& SharedMemory::operator= (SharedMemory&& other) noexcept
{
    if (this != &other)
    {
        close();

        data = std::exchange (other.data, nullptr);
        size = std::exchange (other.size, 0);
        name = std::move (other.name);
        owner = std::exchange (other.owner, false);
        handle = std::exchange (other.handle, 0);
    }

    return *this;
}

String SharedMemory::createUniqueName (const String& prefix)
{
    jassert (prefix.length() <= 8);

    return prefix.substring (0, 8) + String::toHexString (Random().nextInt64()).paddedLeft ('0', 16);
}

} // namespace yup
