# Audio buffers

`AudioBuffer` is the container every part of the audio stack passes around: device
callbacks, audio sources, processors, the audio graph and the DSP classes all read
and write blocks of samples through it. This page covers creating and sizing
buffers, the operations they offer and which of them are safe on the audio thread,
then the related types: `AudioChannelSet` for speaker layouts, `AudioData` for
converting interleaved and integer sample formats, and `SpectralBuffer` for
frequency-domain data.

**Module:** `yup_audio_basics`

## AudioBuffer and AudioSampleBuffer

`AudioBuffer<Type>` holds a number of channels, each a contiguous array of the same
number of samples (non-interleaved). `Type` is `float` or `double`.
`AudioSampleBuffer` is an alias of `AudioBuffer<float>`, kept for code written
against the older fixed-float class.

```cpp
yup::AudioBuffer<float> buffer (2, 512);    // 2 channels, 512 samples each
buffer.clear();                             // the initial content is undefined

const int numChannels = buffer.getNumChannels();
const int numSamples = buffer.getNumSamples();
```

The default constructor creates an empty buffer with no channels and no samples,
and does not allocate. Buffers are movable without allocating, and copyable: a copy
allocates its own memory and copies the samples, unless the source refers to
external memory (see [Referencing external memory](#referencing-external-memory)),
in which case both buffers point at the same data. `operator==` compares the
channel count and the samples of every channel.

## Sizing a buffer

`setSize()` changes the channel count and length:

```cpp
void setSize (int newNumChannels,
              int newNumSamples,
              bool keepExistingContent = false,
              bool clearExtraSpace = false,
              bool avoidReallocating = false);
```

| Flag | Effect |
| --- | --- |
| `keepExistingContent` | Preserves as much of the old samples as fit in the new size. Otherwise the content is undefined after a resize. |
| `clearExtraSpace` | Zeroes the channels and samples that were added. Otherwise they are left uninitialized. |
| `avoidReallocating` | Never shrinks the allocation: a smaller size reuses the existing memory. A larger size still allocates. |

Calling `setSize()` with the current size does nothing. When both
`keepExistingContent` and `avoidReallocating` are true, the buffer avoids
reallocating only if neither the channel count nor the length grows. Without
`keepExistingContent`, `avoidReallocating` reuses the memory whenever the existing
allocation is big enough for the new size.

The usual pattern is to allocate for the largest block you expect while preparing,
then shrink to the actual block size on the audio thread without allocating:

```cpp
// Preparing (message thread or audioDeviceAboutToStart)
scratch.setSize (2, maximumBlockSize);

// Audio thread: never allocates, because the memory is already big enough
scratch.setSize (2, numSamples, false, false, true);
```

`makeCopyOf (other, avoidReallocating)` resizes a buffer to match another one and
copies its content. The source can hold a different sample type, so it converts
between `float` and `double` buffers:

```cpp
yup::AudioBuffer<double> precise;
precise.makeCopyOf (floatBuffer);    // allocates unless the size already matches
```

## Reading and writing samples

Channels are accessed through pointers. Use `getReadPointer()` when you only read,
and `getWritePointer()` when you modify the data:

```cpp
for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
{
    const float* in = input.getReadPointer (channel);
    float* out = buffer.getWritePointer (channel);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
        out[i] = in[i] * 0.5f;
}
```

Both have an overload taking a sample index, which returns a pointer to that
sample. `getArrayOfReadPointers()` and `getArrayOfWritePointers()` return the
whole channel pointer array, which is what C-style APIs and the
[sample format converters](#converting-sample-formats) expect. The pointers become
invalid when the buffer is resized.

For single samples, `getSample (channel, index)`, `setSample (channel, index, value)`
and `addSample (channel, index, value)` are available, but a pointer loop is much
faster for whole blocks.

```{warning}
Channel and sample indices are only checked by assertions in debug builds. An
out-of-range index in a release build is undefined behavior. Never `const_cast`
a read pointer to write through it: the buffer would not know its content changed
(see the next section).
```

## Clearing and the cleared flag

`clear()` zeroes every sample, `clear (startSample, numSamples)` a region of all
channels, and `clear (channel, startSample, numSamples)` a region of one channel.
`fill (value)` and its region overloads set samples to a constant.

The buffer keeps a cleared flag, read with `hasBeenCleared()`, so that work on
silence can be skipped: clearing a buffer that is already flagged as clear does
nothing, gain changes, `findMinMax()`, `getMagnitude()` and `getRMSLevel()` return
immediately, and copying or adding from a cleared buffer is cheap. The flag is set
by `clear()` (and by clearing or applying a zero gain to the whole buffer), and
reset by anything that writes: `getWritePointer()`, `getArrayOfWritePointers()`,
`setSample()`, `copyFrom()`, `addFrom()` and `fill()`.

The flag is not a measurement of the content. Because it is reset when you ask for
a write pointer, keeping a write pointer across a `clear()` and writing through it
afterwards leaves the buffer flagged as clear while it holds data, and the next
`clear()` will do nothing. Ask for the write pointer again after clearing, or call
`setNotClear()` to reset the flag yourself.

## Copying, mixing and gain

All of these operate on regions of channels and never allocate:

| Method | Does |
| --- | --- |
| `copyFrom (destChannel, destStart, source, sourceChannel, sourceStart, numSamples)` | Copies from another buffer. |
| `copyFrom (destChannel, destStart, const Type* source, numSamples [, gain])` | Copies from a raw array, optionally scaled. |
| `copyFromWithRamp (destChannel, destStart, source, numSamples, startGain, endGain)` | Copies with a linear gain ramp. |
| `addFrom (destChannel, destStart, source, sourceChannel, sourceStart, numSamples, gain = 1)` | Mixes another buffer in, optionally scaled. |
| `addFrom (destChannel, destStart, const Type* source, numSamples, gain = 1)` | Mixes a raw array in. |
| `addFromWithRamp (destChannel, destStart, source, numSamples, startGain, endGain)` | Mixes a raw array in with a linear gain ramp. |
| `applyGain (gain)`, `applyGain (start, num, gain)`, `applyGain (channel, start, num, gain)` | Multiplies the whole buffer, a region of all channels, or a region of one channel. |
| `applyGainRamp (start, num, startGain, endGain)`, `applyGainRamp (channel, start, num, startGain, endGain)` | Multiplies by a gain going linearly from `startGain` on the first sample to `endGain` on the last one. |
| `reverse (start, num)`, `reverse (channel, start, num)` | Reverses samples in place. |

Copying or adding between overlapping regions of the same channel is not
supported, and asserts in debug builds.

A gain ramp is the simplest way to avoid clicks when a gain changes between
blocks. Ramp from the previous block's gain to the new one:

```cpp
void process (yup::AudioBuffer<float>& buffer)
{
    const float newGain = targetGain.load();   // std::atomic<float> set by the UI

    buffer.applyGainRamp (0, buffer.getNumSamples(), lastGain, newGain);
    lastGain = newGain;
}
```

For parameters that change often, a [`SmoothedValue`](utilities.md#smoothedvalue)
gives you control over the ramp length.

Mixing several sources into one output:

```cpp
output.clear();

for (auto& voice : voices)
{
    voice.render (voiceBuffer);

    for (int channel = 0; channel < output.getNumChannels(); ++channel)
        output.addFrom (channel, 0, voiceBuffer, channel, 0, output.getNumSamples(), voice.level);
}
```

## Measuring levels

| Method | Returns |
| --- | --- |
| `findMinMax (channel, start, num)` | The lowest and highest sample as a `Range<Type>`. |
| `getMagnitude (channel, start, num)` | The highest absolute sample value of a region of one channel. |
| `getMagnitude (start, num)` | The highest absolute sample value across all channels. |
| `getRMSLevel (channel, start, num)` | The root mean square level of a region of one channel. |

They return zero for a buffer flagged as clear. Convert the result to decibels with
[`Decibels::gainToDecibels()`](utilities.md#decibels):

```cpp
const float peak = buffer.getMagnitude (0, buffer.getNumSamples());
const float peakDb = yup::Decibels::gainToDecibels (peak);
```

## Referencing external memory

A buffer can wrap channel arrays that you own, without copying them. Pass an array
of channel pointers to the constructor, or to `setDataToReferTo()` to retarget an
existing buffer:

```cpp
float* channels[] = { leftData, rightData };

yup::AudioBuffer<float> view (channels, 2, numSamples);          // from sample 0
yup::AudioBuffer<float> tail (channels, 2, 128, numSamples - 128); // with a start offset
```

The buffer never frees memory it refers to. If you resize a referencing buffer
it allocates its own memory, copies the data there, and stops referring to yours.
`hasBeenCleared()` returns false after wrapping external memory.

This is how you treat raw pointers, such as those handed to a device callback, as
an `AudioBuffer`. Wrapping fewer than 32 channels does not allocate, so it is safe
on the audio thread. With 32 or more channels the pointer array itself is
allocated. `setDataToReferTo()` also frees the buffer's own memory if it had any,
so call it on a buffer that already refers to external memory, or on an empty one,
when you are on the audio thread.

## Float and double precision

`AudioBuffer<float>` is the default throughout YUP. `AudioBuffer<double>` offers
the same interface, and `makeCopyOf()` converts between the two. Use double buffers
when your processing benefits from the extra precision, for example long feedback
paths or offline rendering, and only if the code you pass them to accepts them:
devices, audio sources and synthesizers work with `float` buffers, while
[audio processors](processors.md) can opt in to double-precision processing.

Templated code can read the sample type of a buffer through
`AudioBuffer<Type>::SampleType`.

## Buffers in audio callbacks

You rarely create the buffer the audio thread works on; it is handed to you:

- An [`AudioIODeviceCallback`](devices.md) receives raw channel pointers. Wrap
  them in an `AudioBuffer` that refers to them, as above.
- An [`AudioSource`](sources.md) fills the region `startSample` to
  `startSample + numSamples` of the `AudioBuffer<float>*` held by the
  `AudioSourceChannelInfo` passed to `getNextAudioBlock()`.
- An [`AudioProcessor`](processors.md) processes the `audio` buffer of its
  `AudioProcessContext` in place.

```cpp
void audioDeviceIOCallbackWithContext (const float* const* inputs,
                                       int numInputs,
                                       float* const* outputs,
                                       int numOutputs,
                                       int numSamples,
                                       const yup::AudioIODeviceCallbackContext& context) override
{
    yup::AudioBuffer<float> output (outputs, numOutputs, numSamples);   // no copy, no allocation
    output.clear();

    renderInto (output);
}
```

Any other buffer the callback needs, such as scratch space or a delay line, must be
allocated before the callback runs, for example in `audioDeviceAboutToStart()` or
the processor's prepare step.

## Realtime safety

| Operation | Audio thread |
| --- | --- |
| Default constructor, move constructor and move assignment | Safe |
| Constructor with a channel and sample count | Allocates |
| Constructor wrapping external memory, `setDataToReferTo()` | Safe below 32 channels (see above) |
| `setSize()`, `makeCopyOf()` | Allocate, unless the size is unchanged or `avoidReallocating` can reuse the memory |
| Copy constructor and copy assignment | Allocate, unless the destination already has the same size (assignment) or the source refers to external memory (constructor) |
| Pointer access, `getSample()`, `setSample()`, `addSample()` | Safe |
| `clear()`, `fill()`, `copyFrom()`, `addFrom()`, the ramp variants, `applyGain()`, `applyGainRamp()`, `reverse()` | Safe |
| `findMinMax()`, `getMagnitude()`, `getRMSLevel()`, `operator==` | Safe |

A buffer is not thread-safe: don't resize it on one thread while another reads or
writes it. See [the audio thread](../core/multithreading/index.md#the-audio-thread)
for the general rules.

## AudioChannelSet

`AudioChannelSet` describes what each channel of a bus or file means: which
speaker it feeds, or which ambisonic component it carries. It is a set of
`AudioChannelSet::ChannelType` values, each present at most once.

You use channel sets when declaring the buses of a processor (`AudioBus` takes one
as its speaker layout, see [processors](processors.md)), when hosts and plugin
formats negotiate layouts with your plugin, and when reading multichannel files,
where `AudioFormatReader::getChannelLayout()` reports the layout of the stream (see
[audio files](audio-files.md)).

### Named layouts

Static factory functions create the standard layouts:

| Factory | Channels | Description |
| --- | --- | --- |
| `disabled()` | 0 | A disabled bus |
| `mono()` | 1 | A single center channel |
| `stereo()` | 2 | Left, right |
| `createLCR()`, `createLRS()` | 3 | Left, right, center (LCR) or surround (LRS) |
| `createLCRS()`, `quadraphonic()` | 4 | |
| `create5point0()`, `pentagonal()` | 5 | |
| `create5point1()`, `create6point0()`, `create6point0Music()`, `hexagonal()` | 6 | |
| `create6point1()`, `create6point1Music()`, `create7point0()`, `create7point0SDDS()` | 7 | |
| `create7point1()`, `create7point1SDDS()`, `octagonal()` | 8 | |
| `create5point0point2()` ... `create9point1point6()` | 7 to 16 | Immersive layouts with height channels (`5.1.2`, `7.1.4`, `9.1.6`, ...) |
| `ambisonic (order = 1)` | (order + 1)² | ACN ordered, SN3D normalized ambisonics |
| `discreteChannels (n)` | n | Channels with no speaker assignment |

The header documents the equivalent VST3, AAX and CoreAudio layout for each factory.

Three helpers pick a layout from a channel count:

- `canonicalChannelSet (n)` returns mono, stereo, LCR, quadraphonic, 5.0, 5.1,
  7.0 or 7.1 for 1 to 8 channels, and `discreteChannels (n)` otherwise.
- `namedChannelSet (n)` returns the same layouts for 1 to 8 channels, and an empty
  set otherwise.
- `channelSetsWithNumberOfChannels (n)` returns every known layout with `n`
  channels, including the discrete one.

### Channel types and order

The channels of a set are ordered by their `ChannelType` value, not by the order
you added them in. For example `create5point1()` is
`left, right, centre, LFE, leftSurround, rightSurround`. Ask the set where a
speaker is instead of assuming an index:

```cpp
const auto layout = yup::AudioChannelSet::create5point1();

const int lfeIndex = layout.getChannelIndexForType (yup::AudioChannelSet::LFE);   // 3, or -1 if absent
const auto type = layout.getTypeOfChannel (0);                                     // left

layout.size();                            // 6
layout.getDescription();                  // "5.1 Surround"
layout.getSpeakerArrangementAsString();   // "L R C Lfe Ls Rs"
```

`ChannelType` covers the surround speakers (`left`, `right`, `centre`, `LFE`,
`leftSurround`, ...), height and bottom speakers (`topFrontLeft`,
`bottomFrontCentre`, ...), the ambisonic components `ambisonicACN0` to
`ambisonicACN63` (with the aliases `ambisonicW`, `ambisonicX`, `ambisonicY`,
`ambisonicZ`), and untyped channels counting up from `discreteChannel0`.
`getChannelTypeName()` and `getAbbreviatedChannelTypeName()` give display names.

### Building and converting sets

```cpp
yup::AudioChannelSet custom;
custom.addChannel (yup::AudioChannelSet::left);
custom.addChannel (yup::AudioChannelSet::right);
custom.addChannel (yup::AudioChannelSet::LFE);

auto same = yup::AudioChannelSet::channelSetWithChannels ({ yup::AudioChannelSet::left,
                                                             yup::AudioChannelSet::right,
                                                             yup::AudioChannelSet::LFE });

auto parsed = yup::AudioChannelSet::fromAbbreviatedString ("L R C Lfe Ls Rs");   // 5.1
```

| Method | Does |
| --- | --- |
| `addChannel()`, `removeChannel()` | Edit the set. |
| `intersect (other)` | Keeps only the channels present in both sets. |
| `isDisabled()`, `isDiscreteLayout()`, `getAmbisonicOrder()` | Inspect the set; `getAmbisonicOrder()` returns -1 for non-ambisonic sets. |
| `getChannelTypes()` | All the channel types, in channel order. |
| `getSpeakerArrangementAsString()`, `fromAbbreviatedString()` | Round-trip through a string such as `"L R C"`; unknown abbreviations are ignored. |
| `fromWaveChannelMask()`, `getWaveChannelMask()` | Convert to and from the `WAVEFORMATEXTENSIBLE` channel mask of WAV files; `getWaveChannelMask()` returns -1 when the set cannot be expressed as a mask. |
| `getAmbisonicOrderForNumChannels (n)` | The ambisonic order with exactly `n` channels, or -1. |

Channel sets support `==`, `!=` and `<`, so they can be stored in sorted
containers. Treat them as configuration: build and compare them while setting up
buses and layouts, not in the audio callback.

## Converting sample formats

Audio devices, files and plugin APIs often deliver samples interleaved
(`L R L R ...`), as integers, or in a specific byte order. `AudioData` converts
between those encodings and the non-interleaved floating point channels of an
`AudioBuffer`. All conversions are `noexcept`, never allocate, and are safe on the
audio thread.

A format is described by a sample type and an endianness:

| Sample type | Encoding | Raw pointer type for the helpers |
| --- | --- | --- |
| `AudioData::Int8` | 8-bit signed | `int8*` |
| `AudioData::UInt8` | 8-bit unsigned | `uint8*` |
| `AudioData::Int16` | 16-bit signed | `uint16*` |
| `AudioData::Int24` | 24-bit signed, packed in 3 bytes | `char*` |
| `AudioData::Int24in32` | 24-bit signed in the low bits of 32 | `uint32*` |
| `AudioData::Int32` | 32-bit signed | `uint32*` |
| `AudioData::Float32` | 32-bit float | `float*` |
| `AudioData::Float64` | 64-bit float | `double*` |

The endianness is `AudioData::LittleEndian`, `AudioData::BigEndian` or
`AudioData::NativeEndian`. Integer samples map to the range -1 to 1 when read as
floats, and floats outside that range are clipped when written to an integer
format.

### Interleaving and deinterleaving

`AudioData::Format<SampleType, Endianness>` names a format, and
`interleaveSamples()` / `deinterleaveSamples()` convert a whole block of channels
at once. This reads interleaved little-endian 16-bit samples into a float buffer
and writes them back:

```cpp
using Int16LE = yup::AudioData::Format<yup::AudioData::Int16, yup::AudioData::LittleEndian>;
using NativeFloat = yup::AudioData::Format<yup::AudioData::Float32, yup::AudioData::NativeEndian>;

void readFrom (const int16_t* interleaved, yup::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();

    yup::AudioData::deinterleaveSamples (
        yup::AudioData::InterleavedSource<Int16LE> { reinterpret_cast<const yup::uint16*> (interleaved), numChannels },
        yup::AudioData::NonInterleavedDest<NativeFloat> { buffer.getArrayOfWritePointers(), numChannels },
        buffer.getNumSamples());
}

void writeTo (const yup::AudioBuffer<float>& buffer, int16_t* interleaved)
{
    const int numChannels = buffer.getNumChannels();

    yup::AudioData::interleaveSamples (
        yup::AudioData::NonInterleavedSource<NativeFloat> { buffer.getArrayOfReadPointers(), numChannels },
        yup::AudioData::InterleavedDest<Int16LE> { reinterpret_cast<yup::uint16*> (interleaved), numChannels },
        buffer.getNumSamples());
}
```

The channel counts of source and destination may differ: destination channels with
no matching source channel are filled with silence, and when deinterleaving, null
destination channel pointers are skipped.

### Sample pointers

`AudioData::Pointer` walks a single channel of encoded samples. Its four template
parameters are the sample type, the endianness, `AudioData::NonInterleaved` or
`AudioData::Interleaved`, and `AudioData::Const` or `AudioData::NonConst`:

```cpp
using Source = yup::AudioData::Pointer<yup::AudioData::Int24,
                                       yup::AudioData::BigEndian,
                                       yup::AudioData::Interleaved,
                                       yup::AudioData::Const>;

using Dest = yup::AudioData::Pointer<yup::AudioData::Float32,
                                     yup::AudioData::NativeEndian,
                                     yup::AudioData::NonInterleaved,
                                     yup::AudioData::NonConst>;

// rawBytes is a const char* to big-endian 24-bit stereo frames. Interleaved
// pointers need the channel count; start 3 bytes in to read channel 1.
Source right (rawBytes + 3, 2);

Dest (buffer.getWritePointer (1)).convertSamples (right, numFrames);

const float first = right.getAsFloat();  // read one sample
```

A pointer reads and writes the sample it points at with `getAsFloat()`,
`setAsFloat()`, `getAsInt32()` and `setAsInt32()`, and moves with `++`, `--`,
`+=` and `+`. On an interleaved pointer these move by whole frames, so the
pointer stays on the same channel. It also offers `convertSamples (source, numSamples)`,
`clearSamples()` and `findMinAndMax()` for blocks. Writing through a `Const`
pointer is a compile error.

When the formats are only known at runtime, for example when they depend on a
file's header, store an `AudioData::ConverterInstance<SourcePointer, DestPointer>`
behind the abstract `AudioData::Converter` interface. Its `convertSamples()`
overloads take untyped `void*` data and, for interleaved data, the sub-channel to
read or write. Create the converter while preparing, since that allocates; calling
it is realtime-safe.

```{note}
To read and write audio files you normally don't need any of this: the readers
and writers of [`yup_audio_formats`](audio-files.md) convert to and from
`AudioBuffer` for you.
```

## SpectralBuffer

`SpectralBuffer<Type>` is the frequency-domain counterpart of `AudioBuffer`, used
by FFT-based processing. `AudioSpectralBuffer` is an alias of
`SpectralBuffer<float>`. Each channel holds a number of complex bins stored
interleaved as `real, imag, real, imag, ...`. Processors built on the
[spectral processing](spectral.md) framework receive one in their process context.

```cpp
yup::AudioSpectralBuffer bins (2, 1025);    // 2 channels, 1025 bins: allocates
bins.clear();

auto bin = bins.getBinRef (0, 10);
bin.set (0.5f, 0.0f);
const float magnitude = bin.magnitude();
const float phase = bin.phase();
std::complex<float> value = bin;

float* raw = bins.getWritePointer (0);      // 2 * getNumBins() interleaved values
```

| Method | Audio thread |
| --- | --- |
| Constructor with sizes, `setSize()`, `makeCopyOf()` | Allocate, with the same flags as `AudioBuffer::setSize()` |
| `getNumChannels()`, `getNumBins()`, `getReadPointer()`, `getWritePointer()`, `getBinRef()` | Safe |
| `clear()`, `fill()`, `copyFrom()` | Safe |

`BinRef::magnitude()` and `BinRef::phase()` call `std::sqrt` and `std::atan2`;
they don't allocate, but computing them for every bin of a large spectrum in each
block has a cost. `hasBeenCleared()` and `setNotClear()` behave as in
`AudioBuffer`.

## Related pages

- [Audio utilities](utilities.md) - decibels, smoothing, envelopes and the load
  measurer, which all work on buffers.
- [Audio devices](devices.md) - the device callback that hands you buffers.
- [Audio sources](sources.md) - pull-model producers of `AudioBuffer` blocks.
- [Audio processors](processors.md) - buses, channel layouts and the process
  context.
- [Audio files](audio-files.md) - reading and writing buffers to files.
- [Spectral processing](spectral.md) - the processors that use `SpectralBuffer`.
- [DSP](dsp/index.md) - filters, FFTs and other processing on buffers.
- [Multithreading](../core/multithreading/index.md#the-audio-thread) - the rules of
  the audio thread.
