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
    Folds per-note expressions into a MidiBuffer for processors that don't
    support NoteExpressionBuffer directly.

    Two modes are available:
    - channelWide: expressions become pitch bend, channel pressure and CC74 on the
      note's own channel. With several notes on the same channel, they all follow.
    - mpe: every note-on is moved to its own MPE member channel (2-16, lower zone),
      so pitch bend, channel pressure and CC74 only affect that note. Non-note
      channel messages move to the master channel 1. An MPE configuration message is
      sent after reset().

    Volume, pan, vibrato and expression have no MIDI 1.0 equivalent and are dropped.

    @see NoteExpressionBuffer, AudioProcessor::supportsMpe
*/
class YUP_API NoteExpressionMidiConverter
{
public:
    //==============================================================================
    /** How expressions are translated to MIDI. */
    enum class Mode
    {
        channelWide,
        mpe
    };

    //==============================================================================
    /** Creates a converter in the given mode. */
    explicit NoteExpressionMidiConverter (Mode mode = Mode::channelWide);

    /** Changes the conversion mode and resets the note state. */
    void setMode (Mode newMode);

    /** Returns the conversion mode. */
    Mode getMode() const noexcept { return mode; }

    /** Sets the pitch bend range in semitones used to convert tuning (default: 2, or 48 in MPE mode). */
    void setPitchBendRange (float semitones) noexcept;

    /** Pre-allocates the scratch buffer. Call at prepare time with the expected MIDI bytes per block. */
    void prepare (int maxMidiBytesPerBlock);

    /** Forgets all active notes. In MPE mode the next process() emits an MPE configuration message. */
    void reset() noexcept;

    //==============================================================================
    /** Merges @p expressions into @p midi in sample order, remapping channels in MPE mode. */
    void process (MidiBuffer& midi, const NoteExpressionBuffer& expressions);

private:
    struct ActiveNote
    {
        int sourceChannel = 0;
        int key = 0;
        int memberChannel = 0;
    };

    void processMidi (const MidiMessage& message, int samplePosition);
    void processExpression (const NoteExpression& expression);
    void addExpressionMessages (const NoteExpression& expression, int channel);
    int allocateMemberChannel() noexcept;
    int findMemberChannel (int sourceChannel, int key) const noexcept;
    void releaseNote (int sourceChannel, int key) noexcept;

    Mode mode;
    float pitchBendRange = 2.0f;
    bool needsMpeConfiguration = true;
    int nextMemberChannel = 2;
    std::array<int, 17> notesPerChannel {};
    std::vector<ActiveNote> activeNotes;
    MidiBuffer scratch;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoteExpressionMidiConverter)
};

} // namespace yup
