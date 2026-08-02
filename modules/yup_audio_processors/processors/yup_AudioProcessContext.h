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
    All inputs available to an AudioProcessor for a single processing block.

    AudioProcessContext<FloatType> is passed to AudioProcessor::processBlock() and bundles:
      - the audio I/O buffer (flat in-place processing model, single or double precision),
      - per-bus input and output views (for multi-bus and sidechain processing),
      - sample-accurate MIDI events,
      - sample-accurate parameter automation events,
      - host play-head information, when available.

    Use AudioProcessContext<float> for the primary single-precision processing path.
    Use AudioProcessContext<double> for double-precision processing in processors that
    override processBlock(AudioProcessContext<double>&) and return true from
    supportsDoublePrecisionProcessing().

    Processors that only need audio and MIDI can ignore the @c params,
    and @c playHead fields. Processors that implement sample-accurate
    automation should use AudioParameterHandle::prepareBlock() and
    AudioParameterHandle::advanceToSample() together with the @c params buffer.
    Processors that need tempo, transport, or timeline information should use
    @c playHead; a null pointer means no audio position information is available
    for this block.

    @see AudioProcessor, AudioPlayHead, ParameterChangeBuffer, AudioParameterHandle, MidiBuffer
*/
template <typename FloatType>
struct AudioProcessContext
{
    /** Audio I/O buffer. Process in-place: read and write the same channels.
        This is the flat concatenation of all output bus channels for backward
        compatibility. For per-bus access, use @c inputs and @c outputs. */
    AudioBuffer<FloatType>& audio;

    /** MIDI events for this block, sorted by samplePosition in [0, blockSize). */
    MidiBuffer& midi;

    /** Parameter automation events for this block, sorted by sampleOffset in [0, blockSize). */
    ParameterChangeBuffer& params;

    /** Optional play-head for this block. A null pointer means position information is unavailable. */
    AudioPlayHead* playHead = nullptr;

    /** Per-bus read-only views of the input audio buses.
        Indexed by audio-bus index (excluding MIDI buses).
        Empty when there are no audio inputs. */
    Span<const AudioBusBufferView<const FloatType>> inputs;

    /** Per-bus read-write views of the output audio buses.
        Indexed by audio-bus index (excluding MIDI buses).
        Empty when there are no audio outputs. */
    Span<AudioBusBufferView<FloatType>> outputs;

    //==============================================================================
    /** @name Convenience accessors for common bus layouts.
        These helpers return the first bus matching the requested role and direction.
        They return an empty view when no matching bus exists, so callers should
        check @c getNumChannels() before dereferencing.
    */
    ///@{

    /** Returns a view of the first main input bus, or an empty view. */
    const AudioBusBufferView<const FloatType>& getMainInput() const noexcept
    {
        return getInputByRole (AudioBus::Role::Main);
    }

    /** Returns a view of the first main output bus, or an empty view. */
    AudioBusBufferView<FloatType>& getMainOutput() noexcept
    {
        return getOutputByRole (AudioBus::Role::Main);
    }

    /** Returns a view of the @p index -th auxiliary input bus, or an empty view. */
    const AudioBusBufferView<const FloatType>& getAuxiliaryInput (int index) const noexcept
    {
        return getInputByRole (AudioBus::Role::Auxiliary, index);
    }

    ///@}

private:
    // Shared fallback views returned when no bus matches the requested role.
    // They are mutable statics so getOutputByRole can bind them, but callers
    // must treat them as read-only (they always report zero channels).
    inline static AudioBusBufferView<const FloatType> emptyInputView;
    inline static AudioBusBufferView<FloatType> emptyOutputView;

    const AudioBusBufferView<const FloatType>& getInputByRole (AudioBus::Role role, int skip = 0) const noexcept
    {
        skip = jmax (0, skip);
        for (const auto& input : inputs)
        {
            if (input.getRole() == role)
            {
                if (skip == 0)
                    return input;
                --skip;
            }
        }
        return emptyInputView;
    }

    AudioBusBufferView<FloatType>& getOutputByRole (AudioBus::Role role, int skip = 0) noexcept
    {
        skip = jmax (0, skip);
        for (auto& output : outputs)
        {
            if (output.getRole() == role)
            {
                if (skip == 0)
                    return output;
                --skip;
            }
        }
        return emptyOutputView;
    }
};

} // namespace yup
