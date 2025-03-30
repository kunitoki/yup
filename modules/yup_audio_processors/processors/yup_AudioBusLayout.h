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
        : inputBuses (std::move (inputs)), outputBuses (std::move (outputs))
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

private:
    std::vector<AudioBus> inputBuses;
    std::vector<AudioBus> outputBuses;
};

} // namespace yup
