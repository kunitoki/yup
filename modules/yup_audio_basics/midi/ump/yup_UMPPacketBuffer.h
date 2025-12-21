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
/**
    A view of UMP packet data stored in a contiguous buffer.

    Instances of this class do *not* own the UMP data words that they point to.
    Instead, they expect the UMP data to live in a separate buffer that outlives
    the UMPPacketMetadata instance.

    @tags{Audio}
*/
struct UMPPacketMetadata
{
    UMPPacketMetadata() noexcept = default;

    UMPPacketMetadata (const uint32_t* dataIn, uint16_t numWordsIn, int positionIn) noexcept
        : data (dataIn)
        , numWords (numWordsIn)
        , samplePosition (positionIn)
    {
    }

    /** Constructs a UMP view from the data that this object is viewing. */
    ump::View getView() const { return ump::View (data); }

    /** Pointer to the first word of a UMP packet. */
    const uint32_t* data = nullptr;

    /** The number of 32-bit words in the UMP packet. */
    uint16_t numWords = 0;

    /** The packet's timestamp. */
    int samplePosition = 0;
};

//==============================================================================
/**
    An iterator to move over contiguous UMP data, which allows iterating
    over a UMPPacketBuffer using C++11 range-for syntax.

    @tags{Audio}
*/
class YUP_API UMPPacketBufferIterator
{
    using Ptr = const uint32_t*;

public:
    UMPPacketBufferIterator() = default;

    /** Constructs an iterator pointing at the packet starting at the word `dataIn`.
        `dataIn` must point to the start of a valid packet entry.
    */
    explicit UMPPacketBufferIterator (const uint32_t* dataIn) noexcept
        : data (dataIn)
    {
    }

    using difference_type = std::iterator_traits<Ptr>::difference_type;
    using value_type = UMPPacketMetadata;
    using reference = UMPPacketMetadata;
    using pointer = void;
    using iterator_category = std::input_iterator_tag;

    /** Make this iterator point to the next packet in the buffer. */
    UMPPacketBufferIterator& operator++() noexcept;

    /** Create a copy of this object, make this iterator point to the next packet in
        the buffer, then return the copy.
    */
    UMPPacketBufferIterator operator++ (int) noexcept;

    /** Return true if this iterator points to the same packet as another
        iterator instance, otherwise return false.
    */
    bool operator== (const UMPPacketBufferIterator& other) const noexcept { return data == other.data; }

    /** Return false if this iterator points to the same packet as another
        iterator instance, otherwise returns true.
    */
    bool operator!= (const UMPPacketBufferIterator& other) const noexcept { return ! operator== (other); }

    /** Return an instance of UMPPacketMetadata which describes the packet to which
        the iterator is currently pointing.
    */
    reference operator*() const noexcept;

private:
    Ptr data = nullptr;
};

//==============================================================================
/**
    Holds a sequence of time-stamped UMP packets.

    Analogous to the MidiBuffer, this holds a set of UMP packets with
    integer time-stamps. The buffer is kept sorted in order of the time-stamps.

    @tags{Audio}
*/
class YUP_API UMPPacketBuffer
{
public:
    //==============================================================================
    /** Creates an empty UMPPacketBuffer. */
    UMPPacketBuffer() noexcept = default;

    /** Creates a UMPPacketBuffer containing a single packet. */
    explicit UMPPacketBuffer (const ump::View& packet) noexcept;

    //==============================================================================
    /** Removes all events from the buffer. */
    void clear() noexcept;

    /** Removes all events between two times from the buffer. */
    void clear (int startSample, int numSamples);

    /** Returns true if the buffer is empty. */
    bool isEmpty() const noexcept;

    /** Counts the number of events in the buffer. */
    int getNumEvents() const noexcept;

    /** Adds an event to the buffer. */
    bool addEvent (const ump::View& packet, int sampleNumber);

    /** Adds an event to the buffer from raw UMP data. */
    bool addEvent (const uint32_t* data, uint16_t numWords, int sampleNumber);

    /** Adds the events from another buffer. */
    void addEvents (const UMPPacketBuffer& otherBuffer,
                    int startSample,
                    int numSamples,
                    int sampleDeltaToAdd);

    /** Returns the sample number of the first event in the buffer.
        If the buffer's empty, this will just return 0.
    */
    int getFirstEventTime() const noexcept;

    /** Returns the sample number of the last event in the buffer.
        If the buffer's empty, this will just return 0.
    */
    int getLastEventTime() const noexcept;

    //==============================================================================
    /** Exchanges the contents of this buffer with another one. */
    void swapWith (UMPPacketBuffer&) noexcept;

    /** Preallocates some memory for the buffer to use. */
    void ensureSize (size_t minimumNumBytes);

    /** Get a read-only iterator pointing to the beginning of this buffer. */
    UMPPacketBufferIterator begin() const noexcept { return cbegin(); }

    /** Get a read-only iterator pointing one past the end of this buffer. */
    UMPPacketBufferIterator end() const noexcept { return cend(); }

    /** Get a read-only iterator pointing to the beginning of this buffer. */
    UMPPacketBufferIterator cbegin() const noexcept { return UMPPacketBufferIterator (data.begin()); }

    /** Get a read-only iterator pointing one past the end of this buffer. */
    UMPPacketBufferIterator cend() const noexcept { return UMPPacketBufferIterator (data.end()); }

    /** Get an iterator pointing to the first event with a timestamp greater-than or
        equal-to `samplePosition`.
    */
    UMPPacketBufferIterator findNextSamplePosition (int samplePosition) const noexcept;

    /** The raw data holding this buffer. */
    Array<uint32_t> data;

private:
    YUP_LEAK_DETECTOR (UMPPacketBuffer)
};

} // namespace yup
