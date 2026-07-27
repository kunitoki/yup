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

namespace yup
{

namespace UMPPacketBufferHelpers
{
inline int getEventTime (const uint32_t* d) noexcept
{
    return static_cast<int32> (d[0]);
}

inline uint16_t getEventWordCount (const uint32_t* d) noexcept
{
    return static_cast<uint16_t> (d[1] & 0xffffu);
}

inline uint32_t getEventTotalWords (const uint32_t* d) noexcept
{
    return 2u + getEventWordCount (d);
}

static const uint32_t* findEventAfter (const uint32_t* d,
                                       const uint32_t* endData,
                                       int samplePosition) noexcept
{
    while (d < endData && getEventTime (d) <= samplePosition)
        d += getEventTotalWords (d);

    return d;
}
} // namespace UMPPacketBufferHelpers

//==============================================================================
UMPPacketBufferIterator& UMPPacketBufferIterator::operator++() noexcept
{
    data += UMPPacketBufferHelpers::getEventTotalWords (data);
    return *this;
}

UMPPacketBufferIterator UMPPacketBufferIterator::operator++ (int) noexcept
{
    auto copy = *this;
    ++(*this);
    return copy;
}

UMPPacketBufferIterator::reference UMPPacketBufferIterator::operator*() const noexcept
{
    return { data + 2, UMPPacketBufferHelpers::getEventWordCount (data), UMPPacketBufferHelpers::getEventTime (data) };
}

//==============================================================================
UMPPacketBuffer::UMPPacketBuffer (const ump::View& packet) noexcept
{
    addEvent (packet, 0);
}

void UMPPacketBuffer::swapWith (UMPPacketBuffer& other) noexcept { data.swapWith (other.data); }

void UMPPacketBuffer::clear() noexcept { data.clearQuick(); }

void UMPPacketBuffer::ensureSize (size_t minimumNumBytes)
{
    const auto minimumNumWords = (minimumNumBytes + sizeof (uint32_t) - 1) / sizeof (uint32_t);
    data.ensureStorageAllocated ((int) minimumNumWords);
}

bool UMPPacketBuffer::isEmpty() const noexcept { return data.size() == 0; }

void UMPPacketBuffer::clear (int startSample, int numSamples)
{
    auto start = UMPPacketBufferHelpers::findEventAfter (data.begin(), data.end(), startSample - 1);
    auto end = UMPPacketBufferHelpers::findEventAfter (start, data.end(), startSample + numSamples - 1);

    data.removeRange ((int) (start - data.begin()), (int) (end - start));
}

bool UMPPacketBuffer::addEvent (const ump::View& packet, int sampleNumber)
{
    return addEvent (packet.data(), static_cast<uint16_t> (packet.size()), sampleNumber);
}

bool UMPPacketBuffer::addEvent (const uint32_t* packetData, uint16_t numWords, int sampleNumber)
{
    if (packetData == nullptr || numWords == 0 || numWords > 4)
        return false;

    const auto newItemSize = (int) (2u + numWords);
    const auto offset = (int) (UMPPacketBufferHelpers::findEventAfter (data.begin(), data.end(), sampleNumber) - data.begin());

    data.insertMultiple (offset, 0u, newItemSize);

    auto* d = data.begin() + offset;
    d[0] = static_cast<uint32_t> (sampleNumber);
    d[1] = static_cast<uint32_t> (numWords);
    std::copy (packetData, packetData + numWords, d + 2);

    return true;
}

void UMPPacketBuffer::addEvents (const UMPPacketBuffer& otherBuffer,
                                 int startSample,
                                 int numSamples,
                                 int sampleDeltaToAdd)
{
    for (auto i = otherBuffer.findNextSamplePosition (startSample); i != otherBuffer.cend(); ++i)
    {
        const auto metadata = *i;

        if (metadata.samplePosition >= startSample + numSamples && numSamples >= 0)
            break;

        addEvent (metadata.data, metadata.numWords, metadata.samplePosition + sampleDeltaToAdd);
    }
}

int UMPPacketBuffer::getNumEvents() const noexcept
{
    int n = 0;
    auto end = data.end();

    for (auto d = data.begin(); d < end; ++n)
        d += UMPPacketBufferHelpers::getEventTotalWords (d);

    return n;
}

int UMPPacketBuffer::getFirstEventTime() const noexcept
{
    return data.size() > 0 ? UMPPacketBufferHelpers::getEventTime (data.begin()) : 0;
}

int UMPPacketBuffer::getLastEventTime() const noexcept
{
    if (data.size() == 0)
        return 0;

    auto endData = data.end();

    for (auto d = data.begin();;)
    {
        auto nextOne = d + UMPPacketBufferHelpers::getEventTotalWords (d);

        if (nextOne >= endData)
            return UMPPacketBufferHelpers::getEventTime (d);

        d = nextOne;
    }
}

UMPPacketBufferIterator UMPPacketBuffer::findNextSamplePosition (int samplePosition) const noexcept
{
    return UMPPacketBufferIterator (UMPPacketBufferHelpers::findEventAfter (data.begin(), data.end(), samplePosition - 1));
}

} // namespace yup
