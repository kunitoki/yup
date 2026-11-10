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
    A sample-accurate, per-note expression change.

    Values use plain (non-normalized) units, matching both CLAP and VST3 note
    expressions:
    - volume: linear gain in [0, 4], 1 is unity
    - pan: [0, 1], 0.5 is center
    - tuning: offset in semitones, typically [-120, 120]
    - vibrato, expression, brightness, pressure: [0, 1]

    @see NoteExpressionBuffer, AudioProcessor::supportsNoteExpression
*/
struct NoteExpression
{
    /** The kind of expression being changed. */
    enum class Type : uint8
    {
        volume,
        pan,
        tuning,
        vibrato,
        expression,
        brightness,
        pressure
    };

    /** Sample position within the current processing block, in [0, blockSize). */
    int sampleOffset = 0;

    /** Host note identifier, or -1 when the host doesn't provide one. */
    int32 noteId = -1;

    /** MIDI channel in [1, 16], or -1 to apply to all channels. */
    int8 channel = -1;

    /** MIDI key in [0, 127], or -1 to apply to all keys. */
    int8 key = -1;

    /** The expression kind. */
    Type type = Type::volume;

    /** The plain expression value, see the struct documentation for units. */
    float value = 0.0f;
};

//==============================================================================
/**
    A pre-allocated, sorted buffer of per-note expression changes.

    Like ParameterChangeBuffer, memory is reserved at prepare time so add() and
    clear() never allocate on the audio thread. Events exceeding the reserved
    capacity are dropped and fire a debug assertion.

    @see NoteExpression, AudioProcessContext
*/
class NoteExpressionBuffer
{
public:
    //==============================================================================
    /** Reserves capacity for expression events. Call at prepare time. */
    void reserve (int maxEvents)
    {
        events.reserve (static_cast<size_t> (jmax (0, maxEvents)));
    }

    /** Clears all events without releasing memory. Safe on the audio thread. */
    void clear() noexcept { events.clear(); }

    /** Returns true when the buffer contains no events. */
    bool isEmpty() const noexcept { return events.empty(); }

    /** Returns the number of events currently held. */
    int getNumEvents() const noexcept { return static_cast<int> (events.size()); }

    //==============================================================================
    /** Adds an event, returning false if it was dropped because the buffer is full. */
    bool add (const NoteExpression& expression) noexcept
    {
        if (events.size() >= events.capacity())
        {
            jassertfalse; // Increase reserved capacity at prepare time
            return false;
        }

        events.push_back (expression);
        return true;
    }

    /** Stable-sorts events by sampleOffset, preserving the order of simultaneous events. */
    void sort() noexcept
    {
        std::stable_sort (events.begin(), events.end(), [] (const NoteExpression& a, const NoteExpression& b) noexcept
        {
            return a.sampleOffset < b.sampleOffset;
        });
    }

    //==============================================================================
    /** Returns a pointer to the first event. */
    const NoteExpression* begin() const noexcept { return events.data(); }

    /** Returns a pointer one past the last event. */
    const NoteExpression* end() const noexcept { return events.data() + events.size(); }

private:
    std::vector<NoteExpression> events;
};

} // namespace yup
