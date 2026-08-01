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
    A lightweight non-owning view over the channels of a single audio bus.

    AudioBusBufferView wraps a pointer array and channel count to provide
    per-bus access within AudioProcessContext, enabling processors to read
    individual input buses (main, sidechain) and write individual output
    buses independently of the flat channel buffer.

    The const-qualified variant (@c AudioBusBufferView<const SampleType>)
    provides read-only access for input buses; the mutable variant
    (@c AudioBusBufferView<SampleType>) provides read-write access for
    output buses.

    @see AudioProcessContext, AudioBus, AudioBusLayout
*/
template <typename SampleType>
struct AudioBusBufferView
{
    /** The underlying sample type (float or double). */
    using Type = SampleType;

    /** Default-constructs an empty view with no channels. */
    AudioBusBufferView() = default;

    /**
        Constructs a view over @p numCh channels pointed to by @p channelPtrs.

        @param channelPtrs  Array of @p numCh sample pointers, one per channel.
                           Pass nullptr to create a view without channel data
                           (e.g. for an inactive or silent bus); all channel
                           accessors will then return nullptr.
        @param numCh        Number of channels in this bus.
        @param busRole      The role of this bus in the layout (Main or Auxiliary).
    */
    AudioBusBufferView (SampleType* const* channelPtrs, int numCh, AudioBus::Role busRole = AudioBus::Role::Main) noexcept
        : channels (channelPtrs)
        , numChannels (numCh)
        , role (busRole)
    {
    }

    /** Returns the number of channels in this bus. */
    int getNumChannels() const noexcept { return numChannels; }

    /** Returns the role of this bus within the layout. */
    AudioBus::Role getRole() const noexcept { return role; }

    /** Returns a pointer to the channel data array.
        The array has getNumChannels() elements. */
    SampleType* const* getChannels() const noexcept { return channels; }

    /** Returns a read-only pointer to the sample data for channel @p index.
        Returns nullptr if @p index is out of range, or when the view has no
        channel data (e.g. an inactive or silent bus). */
    const SampleType* getReadPointer (int index) const noexcept
    {
        return (channels != nullptr && isPositiveAndBelow (index, numChannels)) ? channels[index] : nullptr;
    }

    /** Returns a mutable pointer to the sample data for channel @p index.
        Returns nullptr if @p index is out of range, or when the view has no
        channel data (e.g. an inactive or silent bus). */
    SampleType* getWritePointer (int index) const noexcept
    {
        return (channels != nullptr && isPositiveAndBelow (index, numChannels)) ? const_cast<SampleType*> (channels[index]) : nullptr;
    }

private:
    SampleType* const* channels = nullptr;
    int numChannels = 0;
    AudioBus::Role role = AudioBus::Role::Main;
};

} // namespace yup
