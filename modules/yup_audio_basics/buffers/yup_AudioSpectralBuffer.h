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

//==============================================================================
/**
    A multi-channel buffer for spectral (frequency-domain) data.

    Each channel stores `numBins` complex-valued spectral bins as interleaved
    `[real, imag, real, imag, ...]` pairs, backed by an AudioBuffer internally
    (2 * numBins samples per channel).

    This buffer is designed for spectral processors such as FFT-based effects,
    phase vocoders, and frequency-domain filters. Raw `Type*` access via
    `getWritePointer` / `getReadPointer` enables vectorized operations with
    ComplexVectorOperations or FloatVectorOperations, while `getBinRef` provides
    safe per-bin real/imaginary reference access.

    @tags{Audio, Spectral, Realtime}
*/
template <typename Type>
class SpectralBuffer
{
public:
    //==============================================================================
    /**
        A lightweight, realtime-safe reference to a single spectral bin's
        real and imaginary components.

        BinRef holds a pointer into the buffer's interleaved data and provides
        mutable/const reference access to the real and imaginary parts. This
        avoids copying and is safe to use on the audio thread.

        @tags{Audio, Spectral, Realtime}
    */
    class BinRef
    {
    public:
        /** Returns a mutable reference to the real component of this bin.
            This is realtime-safe as it only dereferences an existing pointer.
        */
        Type& real() noexcept { return *data; }

        /** Returns a const reference to the real component of this bin.
            This is realtime-safe as it only dereferences an existing pointer.
        */
        const Type& real() const noexcept { return *data; }

        /** Returns a mutable reference to the imaginary component of this bin.
            This is realtime-safe as it only dereferences an existing pointer.
        */
        Type& imag() noexcept { return *(data + 1); }

        /** Returns a const reference to the imaginary component of this bin.
            This is realtime-safe as it only dereferences an existing pointer.
        */
        const Type& imag() const noexcept { return *(data + 1); }

        /** Sets both real and imaginary components of this bin in a single call.
            This is realtime-safe.
        */
        void set (Type newReal, Type newImag) noexcept
        {
            *data = newReal;
            *(data + 1) = newImag;
        }

        /** Computes the magnitude of this bin on-the-fly.
            This involves a `std::sqrt` call, which should be used with caution
            on the audio thread for large numbers of bins.
        */
        Type magnitude() const noexcept { return std::sqrt (real() * real() + imag() * imag()); }

        /** Computes the phase (in radians) of this bin on-the-fly.
            This involves an `std::atan2` call, which should be used with caution
            on the audio thread for large numbers of bins.
        */
        Type phase() const noexcept { return std::atan2 (imag(), real()); }

        /** Implicit conversion to std::complex for convenience.
            This is realtime-safe (does not allocate).
        */
        operator std::complex<Type>() const noexcept { return { real(), imag() }; }

    private:
        template <class T>
        friend class SpectralBuffer;

        explicit BinRef (Type* d) noexcept
            : data (d)
        {
        }

        Type* data;
    };

    //==============================================================================
    /** This allows templated code that takes a SpectralBuffer to access its sample type. */
    using SampleType = Type;

    //==============================================================================
    /**
        Creates an empty buffer with 0 channels and 0 bins.

        This constructor is noexcept and does not allocate, making it realtime-safe.
    */
    SpectralBuffer() noexcept = default;

    //==============================================================================
    /**
        Creates a buffer with a specified number of channels and spectral bins.

        The contents of the buffer will initially be undefined - use clear() to
        zero all data.

        @warning This constructor may allocate memory internally, so it should
                 not be called on the audio thread.

        @param numChannels   The number of channels to allocate.
        @param numBins       The number of spectral bins per channel.

        @see setSize, clear
    */
    SpectralBuffer (int numChannels, int numBins)
        : numChannels (numChannels)
        , numBins (numBins)
    {
        data.setSize (numChannels, numBins * 2, false, false, false);
    }

    //==============================================================================
    /**
        Changes the buffer's size or number of channels.

        @warning This method may allocate memory internally. When called with
                 `keepExistingContent = true` and the new size is larger than the
                 current allocation, or when `avoidReallocating = false`, a new
                 allocation will occur. For realtime safety, call this method on
                 the main thread or during non-realtime setup, and ensure the
                 allocation is performed before audio processing begins.

        @param newNumChannels      The new number of channels.
        @param newNumBins          The new number of spectral bins per channel.
        @param keepExistingContent If true, preserves as much of the existing data
                                   as possible in the new buffer.
        @param clearExtraSpace     If true, any newly allocated space beyond the
                                   existing content will be zeroed.
        @param avoidReallocating   If true, the buffer will not shrink its internal
                                   allocation when the size is reduced (but will
                                   still expand if needed).

        @see AudioBuffer::setSize
    */
    void setSize (int newNumChannels,
                  int newNumBins,
                  bool keepExistingContent = false,
                  bool clearExtraSpace = false,
                  bool avoidReallocating = false)
    {
        jassert (newNumChannels >= 0);
        jassert (newNumBins >= 0);

        if (newNumChannels == numChannels && newNumBins == numBins)
            return;

        data.setSize (newNumChannels, newNumBins * 2, keepExistingContent, clearExtraSpace, avoidReallocating);

        numChannels = newNumChannels;
        numBins = newNumBins;
    }

    //==============================================================================
    /**
        Clears all spectral bins in all channels to zero.

        This method is realtime-safe and uses vectorized operations internally.
        If the buffer is already cleared (hasBeenCleared() returns true), this
        is a no-op.

        @see hasBeenCleared, setNotClear
    */
    void clear() noexcept
    {
        data.clear();
    }

    //==============================================================================
    /** Returns the number of channels in this buffer.
        This is realtime-safe.
    */
    int getNumChannels() const noexcept { return numChannels; }

    //==============================================================================
    /** Returns the number of spectral bins per channel.
        This is realtime-safe.
    */
    int getNumBins() const noexcept { return numBins; }

    //==============================================================================
    /**
        Returns a writeable pointer to the interleaved spectral data for a given channel.

        The returned pointer points to an array of `2 * numBins` values stored as
        `[real0, imag0, real1, imag1, ...]`. This is suitable for use with
        FloatVectorOperations and ComplexVectorOperations.

        For speed, this does not check whether the channel number is in range -
        use an assertion build to catch out-of-bounds access.

        This is realtime-safe (pointer arithmetic only).

        @see getReadPointer, getBinRef, ComplexVectorOperations
    */
    Type* getWritePointer (int channel) noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));

        return data.getWritePointer (channel);
    }

    /**
        Returns a writeable pointer to a specific spectral bin.
    
        This is a convenience method that calculates the correct offset for the
        specified bin index and returns a pointer to the real component of that
        bin. The imaginary component can be accessed at the next index.

        For speed, this does not check whether the channel or bin number is in
        range - use an assertion build to catch out-of-bounds access.

        This is realtime-safe (pointer arithmetic only).

        @param channel   The channel index (0-based).
        @param bin       The bin index within the channel (0-based).

        @see getReadPointer, getBinRef
    */
    Type* getWritePointer (int channel, int bin) noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (isPositiveAndBelow (bin, numBins));

        return data.getWritePointer (channel) + static_cast<size_t> (bin) * 2;
    }

    //==============================================================================
    /**
        Returns a read-only pointer to the interleaved spectral data for a given channel.

        The returned pointer points to an array of `2 * numBins` values stored as
        `[real0, imag0, real1, imag1, ...]`. This is suitable for use with
        FloatVectorOperations and ComplexVectorOperations.

        For speed, this does not check whether the channel number is in range -
        use an assertion build to catch out-of-bounds access.

        This is realtime-safe (pointer arithmetic only).

        @see getWritePointer, getBinRef, ComplexVectorOperations
    */
    const Type* getReadPointer (int channel) const noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));

        return data.getReadPointer (channel);
    }

    /**
        Returns a read-only pointer to a specific spectral bin.

        This is a convenience method that calculates the correct offset for the
        specified bin index and returns a pointer to the real component of that
        bin. The imaginary component can be accessed at the next index.

        For speed, this does not check whether the channel or bin number is in
        range - use an assertion build to catch out-of-bounds access.

        This is realtime-safe (pointer arithmetic only).

        @param channel   The channel index (0-based).
        @param bin       The bin index within the channel (0-based).

        @see getWritePointer, getBinRef
    */
    const Type* getReadPointer (int channel, int bin) const noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (isPositiveAndBelow (bin, numBins));

        return data.getReadPointer (channel) + static_cast<size_t> (bin) * 2;
    }

    //==============================================================================
    /**
        Returns a realtime-safe reference to a specific spectral bin.

        The returned BinRef provides mutable access to the real and imaginary
        components of the bin at the specified channel and index. This is
        lightweight (pointer-based) and safe for audio thread use.

        For speed, this does not check whether the channel or bin number is in
        range - use an assertion build to catch out-of-bounds access.

        @param channel   The channel index (0-based).
        @param bin       The bin index within the channel (0-based).

        @see getWritePointer, getReadPointer
    */
    BinRef getBinRef (int channel, int bin) noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (isPositiveAndBelow (bin, numBins));

        return BinRef (data.getWritePointer (channel) + static_cast<size_t> (bin) * 2);
    }

    //==============================================================================
    /**
        Copies a range of spectral bins from one channel of the source buffer
        to a channel of this buffer.

        This is realtime-safe and uses vectorized copy operations internally.
        This buffer and the source buffer must not overlap for the given channels
        and ranges.

        @param destChannel        The destination channel in this buffer.
        @param destStartBin       The first bin to write to in the destination.
        @param source             The source buffer to copy from.
        @param sourceChannel      The source channel to read from.
        @param sourceStartBin     The first bin to read from in the source.
        @param numBinsToCopy      The number of spectral bins to copy.

        @see copyFrom (simple overload)
    */
    void copyFrom (int destChannel,
                   int destStartBin,
                   const SpectralBuffer<Type>& source,
                   int sourceChannel,
                   int sourceStartBin,
                   int numBinsToCopy) noexcept
    {
        jassert (&source != this
                 || sourceChannel != destChannel
                 || sourceStartBin + numBinsToCopy <= destStartBin
                 || destStartBin + numBinsToCopy <= sourceStartBin);
        jassert (isPositiveAndBelow (destChannel, numChannels));
        jassert (destStartBin >= 0 && destStartBin + numBinsToCopy <= numBins);
        jassert (isPositiveAndBelow (sourceChannel, source.numChannels));
        jassert (sourceStartBin >= 0 && sourceStartBin + numBinsToCopy <= source.numBins);

        if (numBinsToCopy <= 0)
            return;

        const auto numFloats = numBinsToCopy * 2;
        const auto destSample = destStartBin * 2;
        const auto sourceSample = sourceStartBin * 2;

        data.copyFrom (destChannel, destSample, source.data, sourceChannel, sourceSample, numFloats);
    }

    /**
        Copies all spectral bins from one channel of the source buffer to a
        channel of this buffer.

        The number of bins in the source buffer must match the number of bins in
        this buffer. This is realtime-safe and uses vectorized copy internally.

        @param destChannel     The destination channel in this buffer.
        @param source          The source buffer to copy from.
        @param sourceChannel   The source channel to read from.

        @see copyFrom (ranged overload)
    */
    void copyFrom (int destChannel,
                   const SpectralBuffer<Type>& source,
                   int sourceChannel) noexcept
    {
        jassert (source.numBins == numBins);

        copyFrom (destChannel, 0, source, sourceChannel, 0, numBins);
    }

    //==============================================================================
    /**
        Resizes this buffer to match the given one and copies all its content.

        The source buffer can hold a different floating point type, enabling
        conversion between float and double spectral buffers.

        @warning This method may allocate memory internally. See setSize() for
                 realtime safety considerations.

        @param other              The source buffer to copy from.
        @param avoidReallocating  If true, avoids shrinking the internal allocation.

        @see setSize
    */
    template <typename OtherType>
    void makeCopyOf (const SpectralBuffer<OtherType>& other, bool avoidReallocating = false)
    {
        setSize (other.getNumChannels(), other.getNumBins(), false, false, avoidReallocating);

        if (other.hasBeenCleared())
        {
            clear();
        }
        else
        {
            for (int chan = 0; chan < numChannels; ++chan)
            {
                auto* dest = getWritePointer (chan);
                auto* src = other.getReadPointer (chan);

                for (int i = 0; i < numBins * 2; ++i)
                    dest[i] = static_cast<Type> (src[i]);
            }
        }
    }

    //==============================================================================
    /**
        Fills all bins in a single channel with a constant real/imaginary value pair.

        This is realtime-safe.

        @param channel      The channel to fill.
        @param realValue    The real component value to fill.
        @param imagValue    The imaginary component value to fill.
    */
    void fill (int channel, Type realValue, Type imagValue) noexcept
    {
        jassert (isPositiveAndBelow (channel, numChannels));

        auto* ptr = getWritePointer (channel);

        for (int i = 0; i < numBins; ++i)
        {
            *ptr++ = realValue;
            *ptr++ = imagValue;
        }

        setNotClear();
    }

    /**
        Fills all bins in all channels with a constant real/imaginary value pair.

        This is realtime-safe.

        @param realValue    The real component value to fill.
        @param imagValue    The imaginary component value to fill.
    */
    void fill (Type realValue, Type imagValue) noexcept
    {
        for (int i = 0; i < numChannels; ++i)
            fill (i, realValue, imagValue);

        if (realValue != static_cast<Type> (0) || imagValue != static_cast<Type> (0))
            setNotClear();
    }

    //==============================================================================
    /** Returns true if the buffer has been entirely cleared via clear().
        This is realtime-safe.
    */
    bool hasBeenCleared() const noexcept { return data.hasBeenCleared(); }

    /** Forces the internal cleared flag to false.
        This is realtime-safe.
    */
    void setNotClear() noexcept { data.setNotClear(); }

private:
    //==============================================================================
    AudioBuffer<Type> data;
    int numChannels = 0, numBins = 0;

    YUP_LEAK_DETECTOR (SpectralBuffer)
};

//==============================================================================
template <typename Type>
bool operator== (const SpectralBuffer<Type>& a, const SpectralBuffer<Type>& b)
{
    if (a.getNumChannels() != b.getNumChannels())
        return false;

    if (a.getNumBins() != b.getNumBins())
        return false;

    for (int c = 0; c < a.getNumChannels(); ++c)
    {
        const auto* pa = a.getReadPointer (c);
        const auto* pb = b.getReadPointer (c);

        if (! std::equal (pa, pa + a.getNumBins() * 2, pb))
            return false;
    }

    return true;
}

template <typename Type>
bool operator!= (const SpectralBuffer<Type>& a, const SpectralBuffer<Type>& b)
{
    return ! (a == b);
}

//==============================================================================
/**
    A multi-channel buffer of 32-bit floating point spectral samples.

    @see SpectralBuffer
*/
using AudioSpectralBuffer = SpectralBuffer<float>;

} // namespace yup
