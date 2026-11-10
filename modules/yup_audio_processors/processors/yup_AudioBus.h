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
    Describes a single audio, MIDI, or CV bus for plugin I/O.

    Each AudioBus is unidirectional and carries one kind of signal.
*/
class AudioBus
{
public:
    /** The type of signal the bus carries. */
    enum Type
    {
        Audio,
        Midi
    };

    /** The direction of the bus. */
    enum Direction
    {
        Input,
        Output
    };

    /** The role of the bus within a multi-bus layout. */
    enum class Role
    {
        /** Primary signal path (e.g. main input, main output). */
        Main,
        /** Auxiliary signal path (e.g. sidechain input, reference input). */
        Auxiliary
    };

    /**
        Constructs an AudioBus.

        @param name            A user-friendly name for the bus.
        @param type            Signal type.
        @param direction       Input or output.
        @param channels        Number of channels (e.g., stereo = 2).
        @param role            The role of the bus in the layout.
        @param isDefaultActive Whether the bus is active by default.
    */
    AudioBus (StringRef name,
              Type type,
              Direction direction,
              int channels,
              Role role = Role::Main,
              bool isDefaultActive = true)
        : name (name)
        , type (type)
        , direction (direction)
        , numChannels (channels)
        , role (role)
        , defaultActive (isDefaultActive)
    {
    }

    /**
        Constructs an audio bus with an explicit speaker layout.

        @param name            A user-friendly name for the bus.
        @param direction       Input or output.
        @param channelSet      The speaker layout, which also defines the channel count.
        @param role            The role of the bus in the layout.
        @param isDefaultActive Whether the bus is active by default.
    */
    AudioBus (StringRef name,
              Direction direction,
              const AudioChannelSet& channelSet,
              Role role = Role::Main,
              bool isDefaultActive = true)
        : AudioBus (name, Type::Audio, direction, channelSet.size(), role, isDefaultActive)
    {
        layout = channelSet;
    }

    /** Returns the name of the bus. */
    const String& getName() const noexcept { return name; }

    /** Returns the type of the bus. */
    Type getType() const noexcept { return type; }

    /** Returns the direction of the bus. */
    Direction getDirection() const noexcept { return direction; }

    /** Returns the number of channels on the bus. */
    int getNumChannels() const noexcept { return numChannels; }

    /**
        Returns the speaker layout of the bus.

        When no explicit layout was given, or the channel count changed since, this
        returns the canonical layout for the channel count (mono, stereo, LCR, quad,
        5.0, 5.1, 7.0, 7.1, otherwise discrete channels).
    */
    AudioChannelSet getChannelSet() const
    {
        if (layout.size() == numChannels)
            return layout;

        return AudioChannelSet::canonicalChannelSet (numChannels);
    }

    /** Returns the role of the bus within the layout. */
    Role getRole() const noexcept { return role; }

    /** Returns true if the bus is active by default. */
    bool isDefaultActive() const noexcept { return defaultActive; }

    /** Returns true if the bus is mono. */
    bool isMono() const noexcept { return numChannels == 1; }

    /** Returns true if the bus is stereo. */
    bool isStereo() const noexcept { return numChannels == 2; }

    /** Returns a copy of this bus with a different channel count. */
    AudioBus withNumChannels (int newNumChannels) const
    {
        auto copy = *this;
        copy.numChannels = jmax (0, newNumChannels);

        if (copy.layout.size() != copy.numChannels)
            copy.layout = {};

        return copy;
    }

    /** Returns a copy of this bus with a different speaker layout and matching channel count. */
    AudioBus withChannelSet (const AudioChannelSet& newChannelSet) const
    {
        auto copy = *this;
        copy.numChannels = newChannelSet.size();
        copy.layout = newChannelSet;
        return copy;
    }

    /** Compares every property of two buses, using the effective speaker layout. */
    bool operator== (const AudioBus& other) const
    {
        return name == other.name
            && type == other.type
            && direction == other.direction
            && numChannels == other.numChannels
            && role == other.role
            && defaultActive == other.defaultActive
            && getChannelSet() == other.getChannelSet();
    }

private:
    String name;
    Type type = Type::Audio;
    Direction direction = Direction::Output;
    int numChannels = 0;
    AudioChannelSet layout;
    Role role = Role::Main;
    bool defaultActive = true;
};

} // namespace yup
