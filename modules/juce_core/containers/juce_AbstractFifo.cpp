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

AbstractFifo::AbstractFifo (int capacity) noexcept
    : bufferSize (capacity)
{
    jassert (bufferSize > 0);
}

int AbstractFifo::getTotalSize() const noexcept { return bufferSize; }

int AbstractFifo::getFreeSpace() const noexcept { return bufferSize - getNumReady() - 1; }

int AbstractFifo::getNumReady() const noexcept
{
    auto vs = validStart.get();
    auto ve = validEnd.get();
    return ve >= vs ? (ve - vs) : (bufferSize - (vs - ve));
}

void AbstractFifo::reset() noexcept
{
    validEnd = 0;
    validStart = 0;
}

void AbstractFifo::setTotalSize (int newSize) noexcept
{
    jassert (newSize > 0);
    reset();
    bufferSize = newSize;
}

//==============================================================================
void AbstractFifo::prepareToWrite (int numToWrite, int& startIndex1, int& blockSize1, int& startIndex2, int& blockSize2) const noexcept
{
    auto vs = validStart.get();
    auto ve = validEnd.get();

    auto freeSpace = ve >= vs ? (bufferSize - (ve - vs)) : (vs - ve);
    numToWrite = jmin (numToWrite, freeSpace - 1);

    if (numToWrite <= 0)
    {
        startIndex1 = 0;
        startIndex2 = 0;
        blockSize1 = 0;
        blockSize2 = 0;
    }
    else
    {
        startIndex1 = ve;
        startIndex2 = 0;
        blockSize1 = jmin (bufferSize - ve, numToWrite);
        numToWrite -= blockSize1;
        blockSize2 = numToWrite <= 0 ? 0 : jmin (numToWrite, vs);
    }
}

void AbstractFifo::finishedWrite (int numWritten) noexcept
{
    jassert (numWritten >= 0 && numWritten < bufferSize);

    auto newEnd = validEnd.get() + numWritten;

    if (newEnd >= bufferSize)
        newEnd -= bufferSize;

    validEnd = newEnd;
}

void AbstractFifo::prepareToRead (int numWanted, int& startIndex1, int& blockSize1, int& startIndex2, int& blockSize2) const noexcept
{
    auto vs = validStart.get();
    auto ve = validEnd.get();

    auto numReady = ve >= vs ? (ve - vs) : (bufferSize - (vs - ve));
    numWanted = jmin (numWanted, numReady);

    if (numWanted <= 0)
    {
        startIndex1 = 0;
        startIndex2 = 0;
        blockSize1 = 0;
        blockSize2 = 0;
    }
    else
    {
        startIndex1 = vs;
        startIndex2 = 0;
        blockSize1 = jmin (bufferSize - vs, numWanted);
        numWanted -= blockSize1;
        blockSize2 = numWanted <= 0 ? 0 : jmin (numWanted, ve);
    }
}

void AbstractFifo::finishedRead (int numRead) noexcept
{
    jassert (numRead >= 0 && numRead <= bufferSize);

    auto newStart = validStart.get() + numRead;

    if (newStart >= bufferSize)
        newStart -= bufferSize;

    validStart = newStart;
}

//==============================================================================
template <AbstractFifo::ReadOrWrite mode>
AbstractFifo::ScopedReadWrite<mode>::ScopedReadWrite (ScopedReadWrite&& other) noexcept
    : startIndex1 (other.startIndex1)
    , blockSize1 (other.blockSize1)
    , startIndex2 (other.startIndex2)
    , blockSize2 (other.blockSize2)
{
    swap (other);
}

template <AbstractFifo::ReadOrWrite mode>
AbstractFifo::ScopedReadWrite<mode>&
    AbstractFifo::ScopedReadWrite<mode>::operator= (ScopedReadWrite&& other) noexcept
{
    swap (other);
    return *this;
}

template <AbstractFifo::ReadOrWrite mode>
void AbstractFifo::ScopedReadWrite<mode>::swap (ScopedReadWrite& other) noexcept
{
    std::swap (other.fifo, fifo);
    std::swap (other.startIndex1, startIndex1);
    std::swap (other.blockSize1, blockSize1);
    std::swap (other.startIndex2, startIndex2);
    std::swap (other.blockSize2, blockSize2);
}

template class AbstractFifo::ScopedReadWrite<AbstractFifo::ReadOrWrite::read>;
template class AbstractFifo::ScopedReadWrite<AbstractFifo::ReadOrWrite::write>;

AbstractFifo::ScopedRead AbstractFifo::read (int numToRead) noexcept { return { *this, numToRead }; }

AbstractFifo::ScopedWrite AbstractFifo::write (int numToWrite) noexcept { return { *this, numToWrite }; }

} // namespace yup
