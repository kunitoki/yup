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
    Represents the full input/output bus layout of a plugin processor.

    This layout is immutable after construction and supports multiple
    types of buses, grouped by input/output.
*/
class AudioBusLayout
{
public:
    /** Constructs an empty AudioBusLayout. */
    AudioBusLayout() = default;

    /** Constructs an AudioBusLayout from lists of buses. */
    AudioBusLayout (std::vector<AudioBus> inputs, std::vector<AudioBus> outputs)
        : inputBuses (std::move (inputs))
        , outputBuses (std::move (outputs))
    {
    }

    /** Copy constructor. */
    AudioBusLayout (const AudioBusLayout& other) = default;

    /** Assignment operator. */
    AudioBusLayout& operator= (const AudioBusLayout& other) = default;

    /** Move constructor. */
    AudioBusLayout (AudioBusLayout&& other) noexcept = default;

    /** Move assignment operator. */
    AudioBusLayout& operator= (AudioBusLayout&& other) noexcept = default;

    /** Destructor. */
    ~AudioBusLayout() = default;

    /** Returns all input buses. */
    Span<const AudioBus> getInputBuses() const noexcept { return inputBuses; }

    /** Returns all output buses. */
    Span<const AudioBus> getOutputBuses() const noexcept { return outputBuses; }

    /** Returns the total number of audio input channels across all input buses. */
    int getNumAudioInputChannels() const { return getNumAudioChannels (inputBuses); }

    /** Returns the total number of audio output channels across all output buses. */
    int getNumAudioOutputChannels() const { return getNumAudioChannels (outputBuses); }

    /** Returns the total number of audio channels across the given buses. */
    int getNumAudioChannels (Span<const AudioBus> buses) const
    {
        int numChannels = 0;

        for (const auto& bus : buses)
            if (bus.getType() == AudioBus::Type::Audio)
                numChannels += bus.getNumChannels();

        return numChannels;
    }

    /** Returns the role of the audio bus at the given audio-only index.

        Audio-bus indexes count only Audio buses (MIDI buses are skipped), which
        matches the audio-bus indexing convention used by plugin hosts (e.g. the
        VST3 audio-bus index, the CLAP audio-port index, or the AU audio-element
        index). Returns Role::Main when @p audioBusIndex is out of range, so
        wrapper code can safely fall back to the primary signal path.
    */
    AudioBus::Role getAudioBusRole (int audioBusIndex, bool isInput) const noexcept
    {
        const auto& buses = isInput ? inputBuses : outputBuses;
        int audioIdx = 0;

        for (const auto& bus : buses)
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            if (audioIdx == audioBusIndex)
                return bus.getRole();

            ++audioIdx;
        }

        return AudioBus::Role::Main;
    }

    /** Returns the channel count of the audio bus at the given audio-only index, or 0 when out of range. */
    int getAudioBusNumChannels (int audioBusIndex, bool isInput) const noexcept
    {
        const auto* bus = findAudioBus (isInput ? inputBuses : outputBuses, audioBusIndex);
        return bus != nullptr ? bus->getNumChannels() : 0;
    }

    /** Returns the number of audio buses (MIDI buses excluded) in the given direction. */
    int getNumAudioBuses (bool isInput) const noexcept
    {
        int count = 0;

        for (const auto& bus : isInput ? inputBuses : outputBuses)
            if (bus.getType() == AudioBus::Type::Audio)
                ++count;

        return count;
    }

    /** Returns a copy of this layout where the audio bus at the given audio-only index has
        @p numChannels channels. Returns an unchanged copy when the index is out of range.
    */
    AudioBusLayout withAudioBusNumChannels (int audioBusIndex, bool isInput, int numChannels) const
    {
        auto copy = *this;
        auto& buses = isInput ? copy.inputBuses : copy.outputBuses;

        if (auto* bus = findAudioBus (buses, audioBusIndex))
            *bus = bus->withNumChannels (numChannels);

        return copy;
    }

    /** Returns true if both layouts have the same buses in the same order. */
    bool operator== (const AudioBusLayout& other) const noexcept = default;

    /** Returns true if both layouts have the same buses, types, roles and directions in the same
        order, ignoring channel counts and names. A layout negotiation can only change channel counts.
    */
    bool hasSameBusStructure (const AudioBusLayout& other) const noexcept
    {
        const auto sameStructure = [] (const std::vector<AudioBus>& a, const std::vector<AudioBus>& b)
        {
            return std::equal (a.begin(), a.end(), b.begin(), b.end(), [] (const AudioBus& x, const AudioBus& y)
            {
                return x.getType() == y.getType() && x.getDirection() == y.getDirection() && x.getRole() == y.getRole();
            });
        };

        return sameStructure (inputBuses, other.inputBuses) && sameStructure (outputBuses, other.outputBuses);
    }

private:
    template <typename BusVector>
    static auto findAudioBus (BusVector& buses, int audioBusIndex) noexcept -> decltype (buses.data())
    {
        int audioIdx = 0;

        for (auto& bus : buses)
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            if (audioIdx++ == audioBusIndex)
                return &bus;
        }

        return nullptr;
    }

    std::vector<AudioBus> inputBuses;
    std::vector<AudioBus> outputBuses;
};

} // namespace yup
