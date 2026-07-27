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

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

MemoryInputStream::MemoryInputStream (const void* sourceData, size_t sourceDataSize, bool keepCopy)
    : data (sourceData)
    , dataSize (sourceDataSize)
{
    if (keepCopy)
    {
        internalCopy = MemoryBlock (sourceData, sourceDataSize);
        data = internalCopy.getData();
    }
}

MemoryInputStream::MemoryInputStream (const MemoryBlock& sourceData, bool keepCopy)
    : data (sourceData.getData())
    , dataSize (sourceData.getSize())
{
    if (keepCopy)
    {
        internalCopy = sourceData;
        data = internalCopy.getData();
    }
}

MemoryInputStream::MemoryInputStream (StringRef stringToTake)
    : internalCopy (stringToTake.text, stringToTake.length())
{
    data = internalCopy.getData();
    dataSize = internalCopy.getSize();
}

MemoryInputStream::MemoryInputStream (MemoryBlock&& source)
    : internalCopy (std::move (source))
{
    data = internalCopy.getData();
    dataSize = internalCopy.getSize();
}

MemoryInputStream::~MemoryInputStream() = default;

int64 MemoryInputStream::getTotalLength()
{
    return (int64) dataSize;
}

int MemoryInputStream::read (void* buffer, int howMany)
{
    jassert (buffer != nullptr && howMany >= 0);

    if (howMany <= 0 || position >= dataSize)
        return 0;

    auto num = jmin ((size_t) howMany, dataSize - position);

    if (num > 0)
    {
        memcpy (buffer, addBytesToPointer (data, position), num);
        position += num;
    }

    return (int) num;
}

bool MemoryInputStream::isExhausted()
{
    return position >= dataSize;
}

bool MemoryInputStream::setPosition (const int64 pos)
{
    position = (size_t) jlimit ((int64) 0, (int64) dataSize, pos);
    return true;
}

int64 MemoryInputStream::getPosition()
{
    return (int64) position;
}

void MemoryInputStream::skipNextBytes (int64 numBytesToSkip)
{
    if (numBytesToSkip > 0)
        setPosition (getPosition() + numBytesToSkip);
}

} // namespace yup
