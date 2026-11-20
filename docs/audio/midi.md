# MIDI

YUP handles classic MIDI 1.0 data with a small set of value types:
`MidiMessage` holds one event, `MidiBuffer` carries the events of an audio
block with sample positions, `MidiMessageSequence` and `MidiFile` store
timelines, and `MidiKeyboardState` tracks which keys are held. This page also
covers RPN/NRPN parsing and microtuning with `TuningMap`. MIDI 2.0 and the
Universal MIDI Packet format are described in [UMP and MIDI 2.0](ump.md).

**Module:** `yup_audio_basics`

## Creating messages

`MidiMessage` has a static factory function for every common message.
Channels are numbered 1 to 16, and velocities can be passed as a float in the
0..1 range or as a raw 7-bit `uint8`.

```cpp
const auto on   = yup::MidiMessage::noteOn (1, 60, 0.8f);        // middle C, channel 1
const auto off  = yup::MidiMessage::noteOff (1, 60);
const auto cc   = yup::MidiMessage::controllerEvent (1, 7, 100); // channel volume
const auto bend = yup::MidiMessage::pitchWheel (1, 8192);        // 0..16383, 8192 is center
const auto prog = yup::MidiMessage::programChange (10, 0);
```

Other factories cover:

- **Channel messages:** `aftertouchChange()`, `channelPressureChange()`,
  `allNotesOff()`, `allSoundOff()`, `allControllersOff()`.
- **Meta events** (used in MIDI files): `tempoMetaEvent()`,
  `timeSignatureMetaEvent()`, `keySignatureMetaEvent()`, `textMetaEvent()`,
  `midiChannelMetaEvent()`, `endOfTrack()`.
- **System and transport:** `midiStart()`, `midiStop()`, `midiContinue()`,
  `midiClock()`, `songPositionPointer()`, `quarterFrame()`, `fullFrame()`,
  `midiMachineControlCommand()`, `midiMachineControlGoto()`, `masterVolume()`.
- **SysEx:** `createSysExMessage()` takes the payload without the `F0`/`F7`
  framing bytes and adds them for you.

```cpp
const std::byte payload[] { std::byte { 0x7e }, std::byte { 0x7f }, std::byte { 0x06 }, std::byte { 0x01 } };
const auto identityRequest = yup::MidiMessage::createSysExMessage (payload);
```

A message can also be built from raw bytes with
`MidiMessage (status, data1, data2)` or `MidiMessage (data, numBytes)`.
Short messages are stored inline without allocating, while SysEx messages
allocate their data.

## Inspecting messages

Each message kind has an `is...()` test and matching getters:

```cpp
void handle (const yup::MidiMessage& message)
{
    if (message.isNoteOn())
        startNote (message.getChannel(), message.getNoteNumber(), message.getFloatVelocity());
    else if (message.isNoteOff())
        stopNote (message.getChannel(), message.getNoteNumber());
    else if (message.isController())
        setController (message.getControllerNumber(), message.getControllerValue());
    else if (message.isPitchWheel())
        setBend (message.getPitchWheelValue() - 8192);
}
```

```{note}
A note-on with velocity 0 is a note-off by MIDI convention. `isNoteOff()`
returns true for it by default, and `isNoteOn()` returns false unless you pass
`true` to `isNoteOn (returnTrueForVelocity0)`.
```

Other useful inspectors:

- `getChannel()` returns 1 to 16, or 0 for messages without a channel.
  `isForChannel()` and `setChannel()` complete the set.
- `isSustainPedalOn()`, `isSostenutoPedalOn()`, `isSoftPedalOn()` and their
  `Off` counterparts recognize the pedal controllers.
- `isMetaEvent()`, `isTempoMetaEvent()` with `getTempoSecondsPerQuarterNote()`,
  `isTimeSignatureMetaEvent()` with `getTimeSignatureInfo()`, and
  `isTextMetaEvent()` with `getTextFromTextMetaEvent()`.
- `isSysEx()` with `getSysExDataSpan()`, which returns the payload without the
  framing bytes.
- `getRawData()` and `getRawDataSize()` give the bytes as sent on the wire,
  and `getDescription()` returns a readable summary for logging.

The static helpers convert between note numbers and other units:

```cpp
const auto name = yup::MidiMessage::getMidiNoteName (61, true, true, 3); // "C#3"
const auto hz   = yup::MidiMessage::getMidiNoteInHertz (69);           // 440.0
const auto black = yup::MidiMessage::isMidiNoteBlack (61);              // true
```

`getControllerName()` and `getGMInstrumentName()` return General MIDI names,
and `pitchbendToPitchwheelPos()` turns a bend in semitones into a wheel value
for a given bend range.

## Timestamps

Every `MidiMessage` carries a `double` timestamp, read with `getTimeStamp()`
and changed with `setTimeStamp()`, `addToTimeStamp()` or `withTimeStamp()`.
Its unit depends on where the message lives:

- In a `MidiMessageSequence` or `MidiFile`, it is in ticks or seconds (see
  MIDI files below).
- In a `MidiBuffer`, the timestamp is ignored and each event has a sample
  position instead.
- Messages from a MIDI input are stamped in seconds on the
  `Time::getMillisecondCounter()` time base; see [Devices](devices.md).

## MidiBuffer

`MidiBuffer` holds the MIDI events of one audio block, each at a sample offset
from the start of the block. It is what a processor receives next to its audio
buffer.

```cpp
yup::MidiBuffer midi;
midi.addEvent (yup::MidiMessage::noteOn (1, 60, 0.8f), 0);
midi.addEvent (yup::MidiMessage::noteOff (1, 60), 256);
```

`addEvent()` keeps the events sorted by sample position; events at the same
position stay in the order they were added. Iterate with a range-for loop,
which yields lightweight `MidiMessageMetadata` views:

```cpp
for (const auto metadata : midi)
{
    const auto message = metadata.getMessage();
    const auto position = metadata.samplePosition;

    if (message.isNoteOn())
        triggerAt (position, message.getNoteNumber());
}
```

`metadata.data` and `metadata.numBytes` give the raw bytes without building a
`MidiMessage`. To process events from a given position onwards, use
`findNextSamplePosition (sample)` as the starting iterator.

Other operations:

- `clear()` empties the buffer, and `clear (start, numSamples)` removes the
  events in a range.
- `addEvents (other, startSample, numSamples, sampleDeltaToAdd)` copies a
  range of another buffer, shifting its positions.
- `getFirstEventTime()`, `getLastEventTime()` and `isEmpty()` describe the
  contents. `getNumEvents()` walks the whole buffer, so avoid it in hot code.
- `swapWith()` exchanges two buffers without allocating.

```{warning}
Adding events may allocate. On the audio thread, call `ensureSize()` up front
(for example in `prepareToPlay`) so the buffer has room for the largest block
you expect, and reuse the same buffer instead of creating new ones. See
[the audio thread](../core/multithreading/index.md#the-audio-thread).
```

## MidiMessageSequence

`MidiMessageSequence` is an editable, time-sorted list of messages, used for
MIDI file tracks and recorded takes. Each entry is a `MidiEventHolder` that
owns the message and, for note-ons, points to the matching note-off.

```cpp
yup::MidiMessageSequence sequence;
sequence.addEvent (yup::MidiMessage::noteOn (1, 60, 0.8f).withTimeStamp (0.0));
sequence.addEvent (yup::MidiMessage::noteOff (1, 60).withTimeStamp (1.0));
sequence.updateMatchedPairs();

for (auto* holder : sequence)
{
    if (holder->message.isNoteOn() && holder->noteOffObject != nullptr)
        drawNote (holder->message.getTimeStamp(), holder->noteOffObject->message.getTimeStamp());
}
```

Call `updateMatchedPairs()` after adding or deleting events so note-ons are
linked to their note-offs. `getTimeOfMatchingKeyUp (index)` and
`getIndexOfMatchingKeyUp (index)` then answer note-length queries.

- `addEvent (message, timeAdjustment)` inserts in time order;
  `addSequence (other, timeDelta)` merges another sequence.
- `deleteEvent (index, deleteMatchingNoteUp)` removes an event and,
  optionally, its note-off.
- `getStartTime()`, `getEndTime()` and `getNextIndexAtTime (time)` help with
  playback.
- `extractMidiChannelMessages()`, `extractSysExMessages()`,
  `deleteMidiChannelMessages()` and `deleteSysExMessages()` split or filter
  a sequence.
- `addTimeToMessages (delta)` shifts everything in time.
- `createControllerUpdatesForTime (channel, time, messages)` builds the
  controller and program change messages needed to restore a channel's
  state when playback starts in the middle of the sequence.

## MIDI files

`MidiFile` reads and writes Standard MIDI Files as a set of
`MidiMessageSequence` tracks.

```cpp
yup::MidiFile midiFile;
yup::FileInputStream input (file);

if (! input.openedOk() || ! midiFile.readFrom (input))
    return;

midiFile.convertTimestampTicksToSeconds();

for (int i = 0; i < midiFile.getNumTracks(); ++i)
{
    const auto* track = midiFile.getTrack (i);
    // ...
}
```

`readFrom()` returns false for invalid data. It creates missing note-offs by
default, and can report the file type (0, 1 or 2) through its optional
`int*` argument. Timestamps are read as ticks;
`convertTimestampTicksToSeconds()` applies the tempo map to turn them into
seconds.

The time format describes how ticks relate to time:

- `getTimeFormat()` returns a positive value for ticks per quarter note, or a
  negative value for SMPTE frames.
- `setTicksPerQuarterNote (960)` selects musical time.
- `setSmpteTimeFormat (framesPerSecond, subframeResolution)` selects SMPTE
  time (24, 25, 29 or 30 fps).

To write a file, add tracks whose timestamps are in ticks and call
`writeTo()`:

```cpp
yup::MidiMessageSequence track;
track.addEvent (yup::MidiMessage::tempoMetaEvent (500000)); // 120 BPM
track.addEvent (yup::MidiMessage::noteOn (1, 60, 0.8f).withTimeStamp (0.0));
track.addEvent (yup::MidiMessage::noteOff (1, 60).withTimeStamp (960.0));
track.updateMatchedPairs();

yup::MidiFile midiFile;
midiFile.setTicksPerQuarterNote (960);
midiFile.addTrack (track);

file.deleteFile();
yup::FileOutputStream output (file);

if (output.openedOk())
    midiFile.writeTo (output);
```

`findAllTempoEvents()`, `findAllTimeSigEvents()` and `findAllKeySigEvents()`
collect the meta events of every track into one sequence, and
`getLastTimestamp()` returns the end of the longest track.

## Tracking key state

`MidiKeyboardState` keeps track of which notes are held on each channel. It
is shared by the code that plays notes (a `MidiKeyboardComponent`, a MIDI
input, the audio callback) and the code that displays them.

```cpp
class MyProcessor : public yup::AudioProcessor
{
    // ...
    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        keyboardState.processNextMidiBuffer (context.midi, 0, context.audio.getNumSamples(), true);
        // context.midi now also holds the notes played on the on-screen keyboard
    }

    yup::MidiKeyboardState keyboardState;
};
```

`processNextMidiBuffer()` updates the state from the incoming events. With
`injectIndirectEvents` set to true it also adds the notes played through
`noteOn()` and `noteOff()` since the last block, spread across the block.
Queued notes older than 500 ms are dropped, so a keyboard played while audio
is stopped doesn't burst out later.

- `isNoteOn (channel, note)` and `isNoteOnForChannels (mask, note)` query
  the state.
- `noteOn()`, `noteOff()` and `allNotesOff (channel)` change it; pass 0 to
  `allNotesOff()` for every channel. `reset()` clears the state without
  sending note-offs.
- `getPitchWheelPosition (channel)` and `getControllerValue (channel, cc)`
  return the last values seen (-1 for a controller never received).

A `MidiKeyboardState::Listener` is told about every change:

```cpp
struct NoteLogger : yup::MidiKeyboardState::Listener
{
    void handleNoteOn (yup::MidiKeyboardState*, int channel, int note, float velocity) override {}
    void handleNoteOff (yup::MidiKeyboardState*, int channel, int note, float velocity) override {}
};
```

`handlePitchWheelMoved()` and `handleControllerMoved()` are optional.

```{warning}
Listener callbacks are synchronous: they run on whichever thread changed the
state, which is often the audio thread inside `processNextMidiBuffer()`. Keep
them short, never block, and never touch components from them. A UI that
needs to react should hand over to the message thread, for example with
`triggerAsyncUpdate()` on an `AsyncUpdater` and then reading `isNoteOn()`,
which is what `MidiKeyboardComponent` does.
```

The on-screen keyboard that plays into a `MidiKeyboardState` is described in
[MIDI Keyboard](../ui/midi-keyboard.md).

<p align="center">
  <img class="light-only rounded" src="../_static/images/ui/midi-keyboard_keyboard_light.jpg#gh-light-mode-only" alt="A keyboard of three octaves holding a C major chord" width="560">
  <img class="dark-only rounded" src="../_static/images/ui/midi-keyboard_keyboard_dark.jpg#gh-dark-mode-only" alt="A keyboard of three octaves holding a C major chord" width="560">
</p>

## RPN and NRPN

Registered and non-registered parameter numbers are sent as a series of
controller messages. `MidiRPNDetector` reassembles them:

```cpp
yup::MidiRPNDetector detector;

for (const auto metadata : midi)
{
    const auto message = metadata.getMessage();

    if (! message.isController())
        continue;

    if (const auto rpn = detector.tryParse (message.getChannel(), message.getControllerNumber(), message.getControllerValue()))
    {
        // RPN 0 is the pitch bend range, in semitones on the MSB
        if (! rpn->isNRPN && rpn->parameterNumber == 0)
            setPitchBendRange (rpn->channel, rpn->is14BitValue ? rpn->value >> 7 : rpn->value);
    }
}
```

`tryParse()` returns a `MidiRPNMessage` when a value arrives: a 7-bit value
on the data entry MSB, then a 14-bit value (`is14BitValue`) on each LSB that
follows. Use one detector per input stream and call `reset()` when the stream
restarts.

`MidiRPNGenerator::generate()` does the reverse and returns a `MidiBuffer`
with the controller messages to send:

```cpp
// Set the pitch bend range of channel 1 to 12 semitones (RPN 0, 7-bit value)
const auto messages = yup::MidiRPNGenerator::generate (1, 0, 12 << 7, false, true);
```

## Microtuning

`TuningMap` maps MIDI notes to frequencies under any tuning, ready to feed a
synthesizer voice with `noteToPitch (note)`. It starts as 12 tone equal
temperament with A4 = 440 Hz, and reads the common tuning file formats:

- **Scala** `.scl` scales with `loadScale()`, and `.kbm` key maps, which lay
  the scale out on the keys and set the reference pitch, with `loadKeyMap()`.
- **AnaMark** `.tun` / `.msf` tunings with `loadTuning()`. All format
  versions are read, including the version 2 `[Functional Tuning]` formulas
  and `[Mapping]`. A file holding several scales gives the one assigned to
  the MIDI channel passed in.

An AnaMark tuning sets every note's frequency and replaces the scale and key
map until the next `loadScale()` or `loadKeyMap()`. Each loader has a
`FromString` variant for tunings kept in memory, e.g. inside a preset. A load
that fails returns a failed `Result` and leaves the current tuning in place.

```cpp
yup::TuningMap tuning;

if (auto result = tuning.loadTuning (file); result.failed())
    showError (result.getErrorMessage());

const auto frequency = tuning.noteToPitch (60); // negative if the key is unmapped
```

`isNoteMapped (note)` tells whether a key plays at all, and `getName()`,
`getNumberOfScaleDegrees()` and `getReferencePitch()` describe the loaded
tuning.

```{warning}
Loading allocates, so load on the message thread and hand the audio thread a
copy of the `TuningMap` (it is copyable) rather than calling the loaders
while it plays. `noteToPitch()` itself does not allocate.
```

## Related pages

- [UMP and MIDI 2.0](ump.md) - Universal MIDI Packets and protocol conversion.
- [MPE](mpe.md) - per-note expression with zones and `MPEInstrument`.
- [Synthesizers](synthesizers.md) - turning MIDI into sound with voices.
- [Devices](devices.md) - opening MIDI inputs and outputs.
- [Processors](processors.md) - receiving MIDI in an `AudioProcessor`.
- [MIDI Keyboard](../ui/midi-keyboard.md) - the on-screen keyboard component.
