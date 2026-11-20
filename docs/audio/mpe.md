# MPE

MIDI Polyphonic Expression (MPE) gives every note its own MIDI channel, so
pitch bend, pressure and timbre (CC74) can change per note instead of per
channel. Controllers such as the ROLI Seaboard, LinnStrument or Osmose rely on
it. YUP's MPE classes configure zones, track the notes and their expression
with `MPEInstrument`, and render them with `MPESynthesiser`. For plain
channel-wide MIDI see [MIDI](midi.md) and [Synthesizers](synthesizers.md).

**Module:** `yup_audio_basics`

## Zones

An MPE zone is a master channel plus a block of member channels. Each new
note goes to a member channel of its own, and messages sent on the master
channel (sustain, channel-wide pitch bend) apply to every note in the zone.

- The **lower zone** has channel 1 as master and uses member channels
  upwards from 2.
- The **upper zone** has channel 16 as master and uses member channels
  downwards from 15.

`MPEZoneLayout` holds up to two zones. Each zone has a number of member
channels, a per-note pitch bend range (48 semitones by default) and a master
pitch bend range (2 semitones by default):

```cpp
yup::MPEZoneLayout layout;
layout.setLowerZone (15);       // channel 1 master, 2..16 members
// or: layout.setLowerZone (7); layout.setUpperZone (7); for two zones

const auto zone = layout.getLowerZone();
const int first = zone.getFirstMemberChannel(); // 2
const int last = zone.getLastMemberChannel();   // 16
```

`MPEZone` answers `isActive()`, `getMasterChannel()`,
`isUsingChannelAsMemberChannel (channel)` and exposes the
`numMemberChannels`, `perNotePitchbendRange` and `masterPitchbendRange`
fields. `clearAllZones()` turns MPE off.

A layout can follow the configuration messages sent by a controller: feed it
MIDI with `processNextMidiEvent()` or `processNextMidiBuffer()` and it reacts
to the MPE configuration RPN (6) and to pitch bend range changes (RPN 0).
An `MPEZoneLayout::Listener` gets `zoneLayoutChanged()` each time, on the
thread that fed the messages.

## Configuring a device

`MPEMessages` builds the RPN messages that set up zones on a device. Each
function returns a `MidiBuffer` ready to be sent:

```cpp
auto messages = yup::MPEMessages::setLowerZone (15, 48, 2);
messages.addEvents (yup::MPEMessages::setUpperZone (0), 0, -1, 0);

midiOutput->sendBlockOfMessagesNow (messages);
```

Other functions are `setZoneLayout (layout)`, `clearLowerZone()`,
`clearUpperZone()`, `clearAllZones()`, and
`setLowerZonePerNotePitchbendRange()` / `setLowerZoneMasterPitchbendRange()`
with their upper zone counterparts. See [Devices](devices.md) for opening a
`MidiOutput`.

## Values and notes

`MPEValue` stores a 14-bit expression value and converts between the
representations used by MIDI and by DSP code:

```cpp
const auto value = yup::MPEValue::from7BitInt (64);
const float bipolar  = value.asSignedFloat();   // -1..1, 0 at the center
const float unipolar = value.asUnsignedFloat(); // 0..1
```

The factories are `from7BitInt()`, `from14BitInt()`, `fromSignedFloat()`,
`fromUnsignedFloat()`, `minValue()`, `centreValue()` and `maxValue()`; the
getters add `as7BitInt()` and `as14BitInt()`.

An `MPENote` is a snapshot of one sounding note:

- `initialNote`, `midiChannel` and a unique `noteID`.
- `noteOnVelocity`, `pressure`, `timbre`, `pitchbend` and `noteOffVelocity`
  as `MPEValue`s.
- `totalPitchbendInSemitones`, combining the per-note and master pitch bend
  with the zone's ranges, and `getFrequencyInHertz()` which applies it to the
  note.
- `keyState`: `off`, `keyDown`, `sustained` or `keyDownAndSustained`.

## Tracking notes with MPEInstrument

`MPEInstrument` turns incoming MIDI into a list of `MPENote`s and reports
every change. `MPESynthesiser` uses one internally, but it is also useful on
its own, for example to drive a visualizer.

```cpp
yup::MPEInstrument instrument (yup::MPEZone (yup::MPEZone::Type::lower, 15));
instrument.reserveNotes (64);

for (const auto metadata : midi)
    instrument.processNextMidiEvent (metadata.getMessage());

for (int i = 0; i < instrument.getNumPlayingNotes(); ++i)
{
    const auto note = instrument.getNote (i);
    // note.getFrequencyInHertz(), note.pressure.asUnsignedFloat(), ...
}
```

The default constructor creates an instrument with no active zone, which
ignores MPE messages until `setZoneLayout()` or `enableLegacyMode()` is
called. Changing the layout releases all notes.

Notes are looked up with `getNote (index)`, `getNote (channel, note)`,
`getNoteWithID()` and `getMostRecentNote (channel)`, and stopped with
`releaseAllNotes()`. Pitch bend arrives as pitch wheel messages, pressure as
channel pressure (or CC70), and timbre as CC74.

When several notes share a channel, a tracking mode chooses which ones
receive the channel's expression:

```cpp
instrument.setPitchbendTrackingMode (yup::MPEInstrument::lastNotePlayedOnChannel); // default
instrument.setPressureTrackingMode (yup::MPEInstrument::allNotesOnChannel);
```

The other modes are `lowestNoteOnChannel` and `highestNoteOnChannel`, and
`setTimbreTrackingMode()` completes the set.

An `MPEInstrument::Listener` is told about each change. All methods have
empty defaults, so override only what you need:

```cpp
struct NoteWatcher : yup::MPEInstrument::Listener
{
    void noteAdded (yup::MPENote note) override {}
    void notePitchbendChanged (yup::MPENote note) override {}
    void notePressureChanged (yup::MPENote note) override {}
    void noteTimbreChanged (yup::MPENote note) override {}
    void noteKeyStateChanged (yup::MPENote note) override {}
    void noteReleased (yup::MPENote note) override {}
    void zoneLayoutChanged() override {}
};

instrument.addListener (&watcher);
```

```{warning}
Listener callbacks run synchronously on the thread that feeds MIDI into the
instrument, usually the audio or MIDI thread. Keep them short and never touch
the UI from them. Call `reserveNotes()` beforehand with the largest number of
notes you expect, so adding notes on the audio thread doesn't allocate.
```

## MPESynthesiser

`MPESynthesiser` plays an `MPEInstrument` through a set of
`MPESynthesiserVoice`s. A voice is told when its note starts, stops and
changes, and reads the current state from `currentlyPlayingNote`:

```cpp
class MpeSineVoice : public yup::MPESynthesiserVoice
{
public:
    void noteStarted() override
    {
        level = currentlyPlayingNote.noteOnVelocity.asUnsignedFloat() * 0.2;
        tailOff = 0.0;
        updateFrequency();
        notePressureChanged();
    }

    void noteStopped (bool allowTailOff) override
    {
        if (allowTailOff)
        {
            tailOff = 1.0;
            return;
        }

        clearCurrentNote();
        level = 0.0;
    }

    void notePitchbendChanged() override { updateFrequency(); }
    void notePressureChanged() override { pressure = 0.5 + 0.5 * currentlyPlayingNote.pressure.asUnsignedFloat(); }
    void noteTimbreChanged() override {}
    void noteKeyStateChanged() override {}

    void renderNextBlock (yup::AudioBuffer<float>& output, int startSample, int numSamples) override
    {
        if (! isActive())
            return;

        for (int i = startSample; i < startSample + numSamples; ++i)
        {
            auto sample = std::sin (phase) * level * pressure;

            if (tailOff > 0.0)
            {
                sample *= tailOff;
                tailOff *= 0.999;

                if (tailOff < 0.005)
                {
                    clearCurrentNote();
                    level = 0.0;
                    break;
                }
            }

            for (int channel = 0; channel < output.getNumChannels(); ++channel)
                output.addSample (channel, i, static_cast<float> (sample));

            phase += phaseDelta;
        }
    }

private:
    void updateFrequency()
    {
        const auto frequency = currentlyPlayingNote.getFrequencyInHertz();
        phaseDelta = yup::MathConstants<double>::twoPi * frequency / getSampleRate();
    }

    double phase = 0.0, phaseDelta = 0.0, level = 0.0, pressure = 1.0, tailOff = 0.0;
};
```

The voice follows the same contract as a regular synthesizer voice: add to
the buffer instead of overwriting it, touch only the requested range, and
call `clearCurrentNote()` once the sound has ended (immediately when
`noteStopped (false)` is called). See [Synthesizers](synthesizers.md) for
details.

Set up the synth once, then render it in the audio callback:

```cpp
yup::MPESynthesiser synth; // lower zone with 15 member channels by default

// setup
for (int i = 0; i < 15; ++i)
    synth.addVoice (new MpeSineVoice()); // the synth owns the voices

synth.setVoiceStealingEnabled (true);

// prepareToPlay
synth.setCurrentPlaybackSampleRate (spec.sampleRate);

// processBlock
context.audio.clear();
synth.renderNextBlock (context.audio, context.midi, 0, context.audio.getNumSamples());
```

- Voice stealing is **off** by default for `MPESynthesiser`, so notes beyond
  the number of voices are dropped until you enable it.
- `setZoneLayout()`, the tracking mode setters and `enableLegacyMode()` are
  available directly on the synth. To share an instrument, pass it to the
  `MPESynthesiser (MPEInstrument&)` constructor.
- `turnOffAllVoices (allowTailOff)` is a panic button. `reduceNumVoices()`
  shrinks the voice pool.
- Controllers that are not part of MPE and program changes reach the virtual
  `handleController()` and `handleProgramChange()`, which a subclass can
  override.
- `setMinimumRenderingSubdivisionSize()` works as for `Synthesiser`.

To build a different engine on top of the note tracking, derive from
`MPESynthesiserBase` and implement `renderNextSubBlock()`, reacting to the
`MPEInstrument::Listener` callbacks it inherits.

## Legacy mode

Some multi-channel controllers, such as MIDI guitars, send one string per
channel but no MPE configuration. `enableLegacyMode (pitchbendRange,
channelRange)` makes the instrument treat every channel in the range as a
member channel with no master channel:

```cpp
synth.enableLegacyMode (24, yup::Range<int> (1, 7)); // channels 1..6, +/- 24 semitones
```

Setting a zone layout leaves legacy mode again. `isLegacyModeEnabled()` and
the `get`/`setLegacyModeChannelRange()` and
`get`/`setLegacyModePitchbendRange()` accessors query and adjust it.

## Generating and merging MPE

`MPEChannelAssigner` chooses member channels when your code generates MPE,
for example from a sequencer or an arpeggiator. It follows the channel
allocation rules of the MPE specification:

```cpp
yup::MPEChannelAssigner assigner (layout.getLowerZone());

const int channel = assigner.findMidiChannelForNewNote (60);
output.addEvent (yup::MidiMessage::noteOn (channel, 60, 0.8f), 0);

// later
assigner.noteOff (60, channel);
output.addEvent (yup::MidiMessage::noteOff (channel, 60), 128);
```

Call `noteOff()` for every note you release so the assigner knows which
channels are free, and `allNotesOff()` to start over. A constructor taking a
`Range<int>` of channels supports legacy mode.

`MPEChannelRemapper` merges MPE streams from several sources into one zone
without channel clashes. Pass each message with an id you choose for its
source; the channel is changed in place when needed:

```cpp
yup::MPEChannelRemapper remapper (layout.getLowerZone());

auto message = metadata.getMessage();
remapper.remapMidiChannelIfNeeded (message, sourceId); // sourceId != MPEChannelRemapper::notMPE
```

`clearSource()`, `clearChannel()` and `reset()` forget previous mappings.

## Related pages

- [MIDI](midi.md) - messages, buffers and keyboard state.
- [Synthesizers](synthesizers.md) - the voice contract and channel-wide synthesis.
- [UMP and MIDI 2.0](ump.md) - per-note expression in MIDI 2.0 packets.
- [Devices](devices.md) - receiving MPE from controllers and sending configuration.
- [Processors](processors.md) - hosting a synthesizer in an `AudioProcessor`.
