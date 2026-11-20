# Audio files

`yup_audio_formats` reads and writes audio files. An `AudioFormatManager` holds
the formats your app supports and picks the right one from the file
extension; readers decode a file into floating point samples, and writers
encode samples into a file.

**Module:** `yup_audio_formats`

## Registering formats

Create an `AudioFormatManager` and register the formats you need, usually once
at startup:

```cpp
yup::AudioFormatManager formatManager;
formatManager.registerDefaultFormats();
```

`registerDefaultFormats` takes an `AudioFormatType` mask, so you can choose:

```cpp
formatManager.registerDefaultFormats (yup::AudioFormatType::wav | yup::AudioFormatType::flac);

// Everything except the Apple system codecs
formatManager.registerDefaultFormats (yup::AudioFormatType::all & ~yup::AudioFormatType::coreAudio);
```

The values are `wav`, `mp3`, `flac`, `ogg`, `opus`, `coreAudio`, `windowsMedia`
and `all`. Formats that are not compiled in are skipped. The formats are tried
in the order CoreAudio, Windows Media, Wave, FLAC, MP3, Ogg Vorbis, Opus, and
the first one that handles the file extension wins: on Apple platforms
CoreAudio decodes `.wav` and `.mp3` before the built-in decoders. Exclude
`coreAudio` or `windowsMedia` from the mask if you want the same decoders on
every platform.

To add a single format, or your own, use `registerFormat`:

```cpp
formatManager.registerFormat (std::make_unique<yup::WaveAudioFormat>());
```

## Supported formats

| Format class | Extensions | Read | Write | Bit depths for writing | Channels for writing |
| --- | --- | --- | --- | --- | --- |
| `WaveAudioFormat` | `.wav`, `.wave`, `.bwf` | yes | `.wav` | 8, 16, 24 (integer), 32 (float) | 1 to 64 |
| `FlacAudioFormat` | `.flac` | yes | yes | 8, 16, 24, 32 | 1 to 8 |
| `Mp3AudioFormat` | `.mp3` | yes | only with `hmp3_library` | 16 | 1 or 2 |
| `OggVorbisAudioFormat` | `.ogg` | yes | yes | 16 | 1 to 8 |
| `OpusAudioFormat` | `.opus` | yes | yes | 32 | 1 or 2 |
| `AppleCoreAudioFormat` (macOS, iOS) | All system types: `.aiff`, `.caf`, `.m4a`, `.aac`, `.mp3`, `.wav`, ... | yes | AAC only | - | - |
| `WindowsMediaAudioFormat` (Windows) | `.m4a`, `.mp4`, `.aac`, `.wma`, `.wm`, `.wmv`, `.asf`, `.mp3` | yes | AAC, in `.m4a`/`.mp4` or `.aac` | 32 | 1 or 2 |

Each format is enabled by a config flag, and only when the third-party library
it needs is part of the build. Add the library to your target's modules (for
example `dr_libs flac_library hmp3_library libvorbis opus_library`, as the
graphics example does) to enable the format.

| Flag | Needs | Default |
| --- | --- | --- |
| `YUP_AUDIO_FORMAT_WAVE` | `dr_libs` | on |
| `YUP_AUDIO_FORMAT_MP3` | `dr_libs` (decoding), `hmp3_library` (encoding) | on |
| `YUP_AUDIO_FORMAT_FLAC` | `flac_library` | on |
| `YUP_AUDIO_FORMAT_OGG` | `libvorbis` and `libogg` | on |
| `YUP_AUDIO_FORMAT_OPUS` | `opus_library` | on |
| `YUP_AUDIO_FORMAT_COREAUDIO` | - | on for macOS and iOS |
| `YUP_AUDIO_FORMAT_MEDIAFOUNDATION` | - | on for Windows |

To query a format at runtime, use `getPossibleBitDepths()`,
`getPossibleSampleRates()`, `canDoMono()`, `canDoStereo()`, `isCompressed()`,
`getQualityOptions()` and `getFileExtensions (AudioFormat::forReading)` or
`AudioFormat::forWriting`.

## Reading a file into memory

`createReaderFor` returns a reader, or null when no registered format can open
the file. The reader exposes the file's properties as public members:

| Member | Meaning |
| --- | --- |
| `sampleRate` | Sample rate in Hz. |
| `numChannels` | Number of channels. |
| `lengthInSamples` | Length in samples, per channel. |
| `bitsPerSample`, `usesFloatingPointData` | The stored sample format. |
| `metadataValues` | Tags read from the file. |
| `getFormatName()` | The name of the format that opened it. |

Samples are always delivered as `float` in the -1 to 1 range. To load a whole
file:

```cpp
yup::AudioBuffer<float> loadFile (yup::AudioFormatManager& formatManager, const yup::File& file, double& sampleRate)
{
    auto reader = formatManager.createReaderFor (file);
    if (reader == nullptr)
        return {};

    const auto numSamples = static_cast<int> (reader->lengthInSamples);

    yup::AudioBuffer<float> buffer (reader->numChannels, numSamples);
    reader->read (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), 0, numSamples);

    sampleRate = reader->sampleRate;
    return buffer;
}
```

`read (destChannels, numDestChannels, startSampleInSource, numSamplesToRead, fillLeftoverChannelsWithCopies)`
reads any number of channels. Destination channels beyond the file's channel
count are cleared, or filled with copies of the file channels when the last
argument is true.

The `AudioBuffer` overload is convenient for mono and stereo:

```cpp
// Reads the left and right channels; a mono file fills every buffer channel,
// a stereo file read into a mono buffer is mixed down
reader->read (&buffer, 0, numSamples, 0, true, true);
```

It reads at most the first two channels of the file. Use the pointer overload
for multichannel files.

## Streaming a file

Loading large files into memory is wasteful. To play them, read them as they
go: wrap the reader in an `AudioFormatReaderSource` and give it to an
`AudioTransportSource` with a read-ahead buffer (a full example is in
[Audio sources](sources.md)):

```cpp
auto reader = formatManager.createReaderFor (file);
const auto fileSampleRate = reader->sampleRate;

readerSource = std::make_unique<yup::AudioFormatReaderSource> (std::move (reader));
readerSource->setLooping (true);

transportSource.setSource (readerSource.get(), 32768, &readAheadThread, fileSampleRate);
```

`AudioFormatReaderSource` is a `PositionableAudioSource`. The constructor
taking a `std::unique_ptr` owns the reader; the one taking a raw pointer owns
it only when its second argument is true. `getAudioFormatReader()` returns the
reader.

A reader reads from disk and may allocate, so never call it on the audio
thread directly. Read-ahead (through `AudioTransportSource` or
`BufferingAudioSource`) moves the reading to a background thread.

## Reading from memory and streams

The manager only opens files. To decode from memory, an embedded resource or
any other `InputStream`, call the format directly. The format takes ownership
of the stream:

```cpp
yup::FlacAudioFormat flac;

auto reader = flac.createReaderFor (new yup::MemoryInputStream (data, dataSize, false));

if (reader != nullptr)
{
    // ... same API as a reader created from a file
}
```

## Metadata

Readers put the tags they find in `metadataValues`, a `StringPairArray`. Keys
are lowercase:

```cpp
const auto title = reader->metadataValues.getValue ("title", {});
const auto artist = reader->metadataValues.getValue ("artist", {});
```

| Format | Reads | Writes |
| --- | --- | --- |
| Wave | `title`, `artist`, `album`, `year`, `genre`, `comment`, `tracknumber` (RIFF INFO) | The same keys |
| FLAC | Vorbis comments | No |
| Ogg Vorbis | Vorbis comments | Yes, keys are stored uppercase |
| Opus | Opus tags | Yes |
| MP3 | No | No |
| CoreAudio | Information chunks of CAF files | No |
| Windows Media | No | No |

## Levels and silence

Readers can scan a range without loading it into a buffer:

```cpp
yup::Range<float> levels[2];
reader->readMaxLevels (0, reader->lengthInSamples, levels, 2);
const auto peak = yup::jmax (std::abs (levels[0].getStart()), std::abs (levels[0].getEnd()));

// First sample after the start where 10 consecutive samples exceed -60 dB
const auto firstSound = reader->searchForLevel (0, reader->lengthInSamples, 0.001, 1.0, 10);
```

`searchForLevel` returns -1 when nothing matches. For waveform displays and
loudness meters see [Audio GUI](audio-gui.md).

## Writing a file

`createWriterFor` picks the format from the file extension:

```cpp
bool saveWave (yup::AudioFormatManager& formatManager,
               const yup::File& file,
               const yup::AudioBuffer<float>& buffer,
               double sampleRate)
{
    file.deleteFile(); // the writer appends to an existing file

    auto writer = formatManager.createWriterFor (file,
                                                 static_cast<int> (sampleRate),
                                                 buffer.getNumChannels(),
                                                 24); // bits per sample
    if (writer == nullptr)
        return false;

    return writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples());
} // the writer finalizes the file when destroyed
```

The full signature is
`createWriterFor (file, sampleRate, numChannels, bitsPerSample, metadataValues = {}, qualityOptionIndex = 0)`.
It returns null when no format handles the extension or the parameters are not
supported, for example a bit depth the format can't write.

- **Delete first.** The output stream is opened at the end of an existing
  file, so delete the file before writing to replace it.
- **Finalize.** Headers and lengths are completed when the writer is destroyed.
  Reset the `unique_ptr` before reading the file back. `flush()` pushes
  buffered data to the stream.
- **Bit depth.** 32 means floating point for Wave (`isFloatingPoint()` returns
  true). Opus and Windows Media only accept 32, MP3 and Ogg Vorbis only 16.

Ways to feed a writer:

| Call | Source |
| --- | --- |
| `write (channels, numSamples)` | Arrays of `float` per channel. |
| `writeFromAudioSampleBuffer (buffer, startSample, numSamples)` | An `AudioBuffer<float>`. |
| `writeFromAudioReader (reader, startSample, numSamples)` | Another reader, for converting files. |
| `writeFromAudioSource (source, numSamplesToRead, samplesPerBlock)` | Renders an `AudioSource` offline. |

Converting a FLAC file to Wave:

```cpp
auto reader = formatManager.createReaderFor (flacFile);
auto writer = formatManager.createWriterFor (wavFile,
                                             static_cast<int> (reader->sampleRate),
                                             reader->numChannels,
                                             reader->bitsPerSample);

if (writer != nullptr)
    writer->writeFromAudioReader (*reader, 0, reader->lengthInSamples);
```

To write to memory or any `OutputStream`, call `format.createWriterFor (stream, ...)`
directly. On success the writer owns the stream; release your pointer only
then:

```cpp
yup::MemoryBlock block;
auto stream = std::make_unique<yup::MemoryOutputStream> (block, false);

yup::WaveAudioFormat wave;
auto writer = wave.createWriterFor (stream.get(), 48000.0, 2, 16, {}, 0);

if (writer != nullptr)
    stream.release();
```

## Quality and metadata when writing

Compressed formats list their quality settings in `getQualityOptions()`; pass
the index as `qualityOptionIndex`:

| Format | Quality options |
| --- | --- |
| FLAC | Compression levels "0 - Fastest" to "8 - Highest" ("5 - Default" is the usual choice); index 0 is the fastest |
| Ogg Vorbis | "0 - Lowest" to "10 - Highest"; index 0 is the lowest, so pass one explicitly |
| Windows Media | "Low (96 kbps)", "Medium (128 kbps)", "High (192 kbps)" |
| Opus, Wave | None |

```cpp
yup::StringPairArray metadata;
metadata.set ("title", "Take 3");
metadata.set ("artist", "YUP");

auto writer = formatManager.createWriterFor (outputFolder.getChildFile ("take3.ogg"), 48000, 2, 16, metadata, 6);
```

On Apple platforms CoreAudio always encodes AAC. Registered through the
manager it writes an ADTS stream, whatever the extension; to write a real
`.m4a` or `.caf` container, create the format with a stream kind:

```cpp
yup::AppleCoreAudioFormat m4a (yup::AppleCoreAudioFormat::StreamKind::kM4a);
auto writer = m4a.createWriterFor (stream, 44100.0, 2, 16, {}, 0);
```

Encoding is supported for `kAacAdts`, `kMp4`, `kM4a`, `kM4b`, `kCaf`, `k3gp`
and `k3gp2`.

## Recording from the audio thread

A writer encodes and writes to disk, which must not happen on the audio
thread. `AudioFormatWriter::ThreadedWriter` puts a FIFO in front of it and
writes on a `TimeSliceThread`:

```cpp
yup::TimeSliceThread writerThread { "Recorder" };
std::unique_ptr<yup::AudioFormatWriter::ThreadedWriter> recorder;

void startRecording (const yup::File& file, double sampleRate)
{
    writerThread.startThread();
    file.deleteFile();

    auto writer = formatManager.createWriterFor (file, static_cast<int> (sampleRate), 2, 24);
    if (writer == nullptr)
        return;

    auto newRecorder = std::make_unique<yup::AudioFormatWriter::ThreadedWriter> (std::move (writer), writerThread, 32768);

    const yup::AudioLockType::ScopedLockType lock (deviceManager.getAudioCallbackLock());
    recorder = std::move (newRecorder);
}

// In the audio callback
if (recorder != nullptr)
    recorder->write (inputChannelData, numSamples); // false if the FIFO is full

void stopRecording()
{
    std::unique_ptr<yup::AudioFormatWriter::ThreadedWriter> finished;

    {
        const yup::AudioLockType::ScopedLockType lock (deviceManager.getAudioCallbackLock());
        std::swap (finished, recorder);
    }

    finished.reset(); // flushes the remaining data and finalizes the file
}
```

The recorder is swapped under the device manager's callback lock (see
[Audio devices](devices.md)), so the audio thread never sees it half
constructed or half destroyed, and destroyed outside the lock.

`write` copies into the FIFO under a short lock and returns false when the
FIFO is full and the block is dropped; size the FIFO for a few hundred
milliseconds. Keep the `TimeSliceThread` running until the recorder is
destroyed, since its destructor waits for the FIFO to drain. The FIFO lock is
a `CriticalSection`, which the audio thread must not wait on in the browser
(see [Web audio](web.md)).

## Threading and memory

- Creating readers and writers opens files and allocates: do it on the
  message thread or a background thread.
- A reader is not thread-safe; use one reader per thread.
- `read` allocates a temporary buffer for each call, so keep reads out of the
  audio thread and prefer reading large blocks.
- `lengthInSamples` is an `int64`; long files at high sample rates may not fit
  in one `AudioBuffer`, so stream them instead.

## Custom formats

To support another format, derive from `AudioFormat` (name, extensions,
`createReaderFor`, `createWriterFor`, supported rates and depths), from
`AudioFormatReader` (implement `readSamples`) and, for writing, from
`AudioFormatWriter` (implement `write`). Register it with
`formatManager.registerFormat (std::make_unique<MyFormat>())`.

## Related pages

- [Audio sources](sources.md) - play files with `AudioFormatReaderSource` and `AudioTransportSource`
- [Audio devices](devices.md) - open an audio device and record from it
- [Audio buffers](buffers.md) - `AudioBuffer` and channel layouts
- [MIDI](midi.md) - reading and writing MIDI files
- [Resampling](../dsp/resampling.md) - converting the sample rate of loaded files
- [Threads](../core/multithreading/threads.md) - `TimeSliceThread` for background reading and writing
- [Audio](index.md) - overview of the audio modules
