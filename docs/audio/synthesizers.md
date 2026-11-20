# Synthesizers

`Synthesiser` turns a `MidiBuffer` into audio by handing each note to one of
a pool of voices. You write two small classes: a `SynthesiserSound` that says
which notes and channels it covers, and a `SynthesiserVoice` that renders one
note at a time. The synthesizer does the rest: voice allocation and stealing,
sample-accurate MIDI timing, pitch wheel, sustain and sostenuto pedals. For
per-note expression with MPE controllers see [MPE](mpe.md).

**Module:** `yup_audio_basics`

## Structure

- **`SynthesiserSound`** describes something that can be played, such as a
  sample set or a patch. It answers `appliesToNote (note)` and
  `appliesToChannel (channel)`, and is otherwise just data shared by the
  voices.
- **`SynthesiserVoice`** plays one note of a sound. It is told when a note
  starts and stops and when controllers move, and adds its output to the
  buffer in `renderNextBlock()`.
- **`Synthesiser`** owns the voices and sounds, reads the MIDI, picks a free
  voice that `canPlaySound()` for each note-on, and renders all voices.

Sounds and voices are reference counted (`SynthesiserSound::Ptr`,
`SynthesiserVoice::Ptr`), so the synthesizer keeps them alive for you. The
number of voices is the polyphony.

## A minimal sine synth

```cpp
struct SineSound : public yup::SynthesiserSound
{
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class SineVoice : public yup::SynthesiserVoice
{
public:
    bool canPlaySound (yup::SynthesiserSound* sound) override
    {
        return dynamic_cast<SineSound*> (sound) != nullptr;
    }

    void startNote (int note, float velocity, yup::SynthesiserSound*, int /*pitchWheel*/) override
    {
        const auto frequency = yup::MidiMessage::getMidiNoteInHertz (note);
        phaseDelta = yup::MathConstants<double>::twoPi * frequency / getSampleRate();
        phase = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            if (tailOff == 0.0)
                tailOff = 1.0;

            return;
        }

        clearCurrentNote();
        phaseDelta = 0.0;
    }

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (yup::AudioBuffer<float>& output, int startSample, int numSamples) override
    {
        if (phaseDelta == 0.0)
            return;

        for (int i = startSample; i < startSample + numSamples; ++i)
        {
            auto sample = std::sin (phase) * level;

            if (tailOff > 0.0)
            {
                sample *= tailOff;
                tailOff *= 0.999;

                if (tailOff < 0.005)
                {
                    clearCurrentNote();
                    phaseDelta = 0.0;
                    break;
                }
            }

            for (int channel = 0; channel < output.getNumChannels(); ++channel)
                output.addSample (channel, i, static_cast<float> (sample));

            phase += phaseDelta;
        }
    }

private:
    double phase = 0.0, phaseDelta = 0.0, level = 0.0, tailOff = 0.0;
};
```

Set it up once, then render it from the audio callback:

```cpp
yup::Synthesiser synth;

for (int i = 0; i < 8; ++i)
    synth.addVoice (new SineVoice());

synth.addSound (new SineSound());

synth.setCurrentPlaybackSampleRate (48000.0);

// for each audio block
buffer.clear();
synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
```

For anything beyond a test tone, build the voice from the oscillators and
envelopes of `yup_dsp` instead of hand-written loops; see
[Oscillators](dsp/oscillators.md) and [Envelopes](dsp/envelopes.md).

## The rendering contract

`Synthesiser::renderNextBlock (buffer, midi, startSample, numSamples)` splits
the block at each MIDI event, handles the event, and calls every voice's
`renderNextBlock()` for the samples in between. A voice must follow these
rules:

- **Add, don't overwrite.** Several voices write into the same buffer, so use
  `addSample()` or `addFrom()`. The synthesizer adds to the buffer too, so
  clear it first if it holds input you don't want.
- **Stay in range.** Only touch samples from `startSample` to
  `startSample + numSamples`. Sub-blocks can be as short as one sample.
- **Return early when silent.** Voices without a note are still called.
- **Free the voice.** Call `clearCurrentNote()` when the sound has ended, for
  example at the end of a release. When `stopNote()` is called with
  `allowTailOff` false, stop at once and call `clearCurrentNote()` before
  returning.

MIDI events outside the rendered range are ignored. `startNote()`,
`stopNote()` and the controller callbacks run between sub-blocks on the
audio thread, so they must be fast and must not allocate or lock.

A double precision `renderNextBlock (AudioBuffer<double>&, ...)` exists on
both classes. Voices that don't override it are rendered in single precision
and converted, which uses an internal buffer that may allocate.

## Sample rate

Call `setCurrentPlaybackSampleRate()` before rendering, typically from
`prepareToPlay()`. It passes the rate on to every voice, which reads it with
`getSampleRate()`, and changing it stops all playing notes. Override
`SynthesiserVoice::setCurrentPlaybackSampleRate()` (calling the base
version) to recompute rate-dependent coefficients.

## Voice allocation and stealing

On a note-on the synthesizer looks for a free voice that can play the sound.
If a voice is still sounding the same note on the same channel (for example
held by the sustain pedal), it is released first with a tail-off. When all
voices are busy, note stealing reuses the oldest voice, preferring one already
playing the same note, then voices in their release phase, while protecting
the lowest and highest held notes. The stolen voice receives
`stopNote (0.0f, false)` and must stop immediately.

`setNoteStealingEnabled (false)` drops notes that find no free voice instead.
For a different policy, override `findFreeVoice()` or `findVoiceToSteal()` in
a subclass of `Synthesiser`.

Voices and sounds are managed with `addVoice()`, `removeVoice()`,
`clearVoices()`, `getNumVoices()`, `getVoice()`, and the matching `...Sound()`
functions. They take an internal lock shared with the rendering, so changing
them while playing briefly blocks the audio thread; prefer doing it before
playback starts.

## Pedals and controllers

The synthesizer handles the sustain (CC64) and sostenuto (CC66) pedals per
channel: a released key keeps sounding while its pedal is down, and stops
when the pedal is lifted. Voices can check `isKeyDown()`,
`isSustainPedalDown()`, `isSostenutoPedalDown()` and `isPlayingButReleased()`.

Other events reach the voices playing on that channel:

- Pitch wheel calls `pitchWheelMoved()`, and `startNote()` receives the
  current wheel position for new notes.
- Controllers call `controllerMoved()`.
- Polyphonic aftertouch and channel pressure call the optional
  `aftertouchChanged()` and `channelPressureChanged()`.

`handleSoftPedal()` and `handleProgramChange()` do nothing by default; derive
from `Synthesiser` and override them, or any of the other `handle...()`
functions, to change the behavior. `allNotesOff (channel, allowTailOff)` with
channel 0 is a panic button for all channels.

## Rendering subdivision

Splitting the block at every MIDI event keeps timing sample-accurate, but a
dense stream of controller messages would render many tiny sub-blocks.
`setMinimumRenderingSubdivisionSize (numSamples, shouldBeStrict)` sets the
smallest sub-block, 32 samples by default. Events closer together than that
are handled together. Unless `shouldBeStrict` is true, the first event in each
block stays sample-accurate.

## Using it in a processor

A synthesizer usually lives inside an `AudioProcessor`, which receives the
MIDI next to its output buffer. Pairing it with a `MidiKeyboardState` lets an
on-screen keyboard play it as well:

```cpp
class SineSynthProcessor : public yup::AudioProcessor
{
public:
    SineSynthProcessor()
        : yup::AudioProcessor ("SineSynth", yup::AudioBusLayout ({}, { yup::AudioBus ("main", yup::AudioBus::Audio, yup::AudioBus::Output, 2) }))
    {
        for (int i = 0; i < 8; ++i)
            synth.addVoice (new SineVoice());

        synth.addSound (new SineSound());
    }

    bool acceptsMidi() const noexcept override { return true; }

    void prepareToPlay (const yup::AudioSpec& spec) override
    {
        synth.setCurrentPlaybackSampleRate (spec.sampleRate);
        keyboardState.reset();
    }

    void releaseResources() override {}

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        const int numSamples = context.audio.getNumSamples();

        keyboardState.processNextMidiBuffer (context.midi, 0, numSamples, true);

        context.audio.clear();
        synth.renderNextBlock (context.audio, context.midi, 0, numSamples);
    }

    yup::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }

    // hasEditor(), createEditor() and the other overrides omitted

private:
    yup::Synthesiser synth;
    yup::MidiKeyboardState keyboardState;
};
```

`processNextMidiBuffer()` with `true` merges the notes played on a
`MidiKeyboardComponent` into `context.midi`, so the synthesizer sees them as
ordinary MIDI. See [Processors](processors.md) for the rest of the processor
API and [MIDI](midi.md) for `MidiKeyboardState`.

## Alternative tunings

`getMidiNoteInHertz()` assumes 12 tone equal temperament. To play scales
loaded from Scala or AnaMark files, look the frequency up in a `TuningMap`
instead:

```cpp
void startNote (int note, float velocity, yup::SynthesiserSound*, int) override
{
    const auto frequency = tuning.noteToPitch (note); // negative if unmapped
    phaseDelta = frequency > 0.0 ? yup::MathConstants<double>::twoPi * frequency / getSampleRate() : 0.0;
    // ...
}
```

`noteToPitch()` doesn't allocate, but loading a tuning does. Load it on the
message thread and give the audio thread its own copy, rather than loading
into the map the voices are reading. Returning `false` from
`SynthesiserSound::appliesToNote()` for keys where `isNoteMapped()` is false
keeps unmapped keys from taking a voice. See the microtuning section of
[MIDI](midi.md) for the file formats.

## Related pages

- [MIDI](midi.md) - messages, `MidiBuffer`, keyboard state and tunings.
- [MPE](mpe.md) - per-note expression with `MPESynthesiser`.
- [Processors](processors.md) - hosting a synthesizer in an `AudioProcessor`.
- [MIDI Keyboard](../ui/midi-keyboard.md) - playing the synthesizer from the screen.
- [Oscillators](dsp/oscillators.md) - building blocks for voices.
- [Envelopes](dsp/envelopes.md) - amplitude and filter envelopes for voices.
