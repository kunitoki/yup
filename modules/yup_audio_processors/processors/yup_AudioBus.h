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
        MIDI
    };

    /** The direction of the bus. */
    enum Direction
    {
        Input,
        Output
    };

    /**
        Constructs an AudioBus.

        @param name        A user-friendly name for the bus.
        @param type        Signal type.
        @param direction   Input or output.
        @param channels    Number of channels (e.g., stereo = 2).
    */
    AudioBus (StringRef name,
              Type type,
              Direction direction,
              int channels)
        : name (name)
        , type (type)
        , direction (direction)
        , numChannels (channels)
    {
    }

    /** Returns the name of the bus. */
    const String& getName() const noexcept { return name; }

    /** Returns the type of the bus. */
    Type getType() const noexcept { return type; }

    /** Returns the direction of the bus. */
    Direction getDirection() const noexcept { return direction; }

    /** Returns the number of channels on the bus. */
    int getNumChannels() const noexcept { return numChannels; }

    /** Returns true if the bus is mono. */
    bool isMono() const noexcept {
        return numChannels
        ==
1; }

    /** Returns true if the bus is stereo. */
    bool isStereo() const noexcept { return numChannels == 2; }

private:
    String name;
    Type type = Type::Audio;
    Direction direction = Direction::Output;
    int numChannels = 0;
};

} // namespace yup
