# UMP and MIDI 2.0

MIDI 2.0 carries its messages as Universal MIDI Packets (UMP): small packets
of one to four 32-bit words instead of a stream of bytes. A packet can hold a
classic MIDI 1.0 message or a MIDI 2.0 one, with 16-bit velocities, 32-bit
controllers, per-note controllers and pitch bend, and it addresses 16 groups
of 16 channels. YUP keeps both worlds working together: the `yup::ump`
namespace builds, stores and parses packets, and converters translate between
bytestream MIDI 1.0, MIDI 1.0 in UMP and MIDI 2.0 in UMP. For the classic
`MidiMessage` API see [MIDI](midi.md).

**Module:** `yup_audio_basics`

## Packet types

The top four bits of the first word give the message type, which also fixes
the packet size:

| Type | Content | Words |
|------|---------|-------|
| `0x0` | Utility (no-op, jitter reduction clock and timestamps) | 1 |
| `0x1` | System real time and system common | 1 |
| `0x2` | MIDI 1.0 channel voice | 1 |
| `0x3` | Data (7-bit SysEx) | 2 |
| `0x4` | MIDI 2.0 channel voice | 2 |
| `0x5` | Extended data (8-bit SysEx, mixed data sets) | 4 |
| `0xD` | Flex data (tempo, time and key signatures, text) | 4 |
| `0xF` | UMP stream (endpoint and function block discovery) | 4 |

The next four bits are the group (0 to 15). For channel voice messages the
status and channel follow; **UMP channels are numbered 0 to 15**, unlike the
1 to 16 of `MidiMessage`.

`ump::Utils` decodes a first word: `getMessageType()`, `getGroup()`,
`getStatus()`, `getChannel()` and `getNumWordsForMessageType()`.

`PacketProtocol` (`MIDI_1_0` or `MIDI_2_0`) selects which kind of channel
voice messages a converter, device or keyboard state produces.

## Building messages

There are two ways to build packets. The typed builders return a
`UniversalPacket` (four words, of which only the first `getSize()` are used)
and take value types that convert between resolutions for you:

```cpp
using namespace yup::ump;

const auto on   = makeMidi2NoteOnMessage (0, 0, 60, Velocity { 0.8f });
const auto off  = makeMidi2NoteOffMessage (0, 0, 60, Velocity { 0.0f });
const auto cc   = makeMidi2ControlChangeMessage (0, 0, 74, ControllerValue { 0.5f });
const auto bend = makeMidi2PitchBendMessage (0, 0, PitchBend { 0.25f }); // -1..1

const auto legacyOn = makeMidi1NoteOnMessage (0, 0, 60, Velocity { uint7_t { 100 } });
```

The arguments are group, channel, note or controller, then the value.
`Velocity`, `ControllerValue` and `PitchBend` accept floats (0..1, or -1..1
for pitch bend) or integers of 7, 14, 16 or 32 bits, and read back with
`asFloat()`, `asUInt7()` and similar. MIDI 2.0 adds
`makePerNotePitchBendMessage()`, `makeRegisteredPerNoteControllerMessage()`
and `makePerNoteManagementMessage()`, and a note-on overload taking a
`Pitch7_9` sets an exact pitch for the note.

`ump::Factory` offers the same messages with raw integers, returning a
fixed-size `PacketX1`, `PacketX2` or `PacketX4`:

```cpp
const auto noteOn = yup::ump::Factory::makeNoteOnV2 (0, 0, 60, yup::ump::Factory::NoteAttributeKind::none, 0xc000, 0);
const auto clock  = yup::ump::Factory::makeTimingClock (0);
```

Its functions are named after the message with a `V1` or `V2` suffix for the
protocol, such as `makeControlChangeV1()`, `makeControlChangeV2()` and
`makePitchBendV2()`, plus the system messages (`makeStart()`, `makeStop()`,
`makeSongPositionPointer()`, ...).

Flex data and stream messages have builders too, for example
`makeSetTempoMessage()`, `makeSetTimeSignatureMessage()`,
`makeEndpointDiscoveryMessage()` and `makeStreamConfigurationRequest()`.

## Storing and reading packets

A `ump::View` is a non-owning pointer to one packet. It knows its size from
the message type, and is how packets are passed around:

```cpp
const auto message = yup::ump::makeMidi2NoteOnMessage (0, 0, 60, yup::ump::Velocity { 0.8f });
const yup::ump::View view (message.data);       // from a UniversalPacket

const auto packet = yup::ump::Factory::makeStart (0);
const yup::ump::View startView (packet.data()); // from a Factory packet
```

`ump::Packets` owns a sequence of packets without timestamps. Add packets or
views with `add()`, and iterate to get a `View` for each one:

```cpp
yup::ump::Packets packets;
packets.add (yup::ump::View (message.data));
packets.add (yup::ump::Factory::makeStop (0));

for (const auto& view : packets)
    handlePacket (view, 0); // view.size() words, view[0] is the first
```

`UMPPacketBuffer` (in the `yup` namespace) is the UMP version of `MidiBuffer`:
each packet has a sample position inside an audio block.

```cpp
yup::UMPPacketBuffer buffer;
buffer.ensureSize (4096); // reserve outside the audio thread
buffer.addEvent (yup::ump::View (message.data), 32);

for (const auto metadata : buffer)
    handlePacket (metadata.getView(), metadata.samplePosition);
```

It offers the same operations as `MidiBuffer`: `clear()`, `addEvents()`,
`findNextSamplePosition()`, `getFirstEventTime()`, `getLastEventTime()` and
`swapWith()`.

To inspect a packet with the typed helpers, copy it into a `UniversalPacket`.
The accessors work for both MIDI 1.0 and MIDI 2.0 channel voice messages:

```cpp
void handlePacket (const yup::ump::View& view, int samplePosition)
{
    yup::ump::UniversalPacket packet;
    std::copy (view.begin(), view.end(), packet.data);

    if (yup::ump::isNoteOnMessage (packet))
        startNote (packet.getChannel(), yup::ump::getNoteNumber (packet), yup::ump::getNoteVelocity (packet).asFloat());
    else if (yup::ump::isNoteOffMessage (packet))
        stopNote (packet.getChannel(), yup::ump::getNoteNumber (packet));
    else if (yup::ump::isControlChangeMessage (packet))
        setController (yup::ump::getControllerNumber (packet), yup::ump::getControllerValue (packet).asFloat());
}
```

`isChannelPitchBendMessage()` with `getChannelPitchBendValue()`,
`isPerNotePitchBendMessage()` with `getPerNotePitchBendValue()`,
`isChannelPressureMessage()` and `isProgramChangeMessage()` complete the set.
A MIDI 1.0 note-on with velocity 0 counts as a note-off.

## Converting between formats

The converters call a callback with each produced packet or message, since one
input can produce several outputs (or none, until a multi-part message is
complete).

**MidiMessage to MIDI 1.0 UMP:**

```cpp
yup::ump::Packets packets;
const auto message = yup::MidiMessage::noteOn (1, 60, 0.8f);

yup::ump::Conversion::toMidi1 (yup::ump::BytestreamMidiView (&message), [&] (const yup::ump::View& view)
{
    packets.add (view);
});
```

`BytestreamMidiView` also wraps the `MidiMessageMetadata` of a `MidiBuffer`.
For a raw stream of bytes, `BytestreamToUMPDispatcher (protocol, storageSize)`
parses running status and SysEx and produces packets in the protocol you ask
for.

**MIDI 1.0 UMP to MIDI 2.0 UMP:** `Midi1ToMidi2DefaultTranslator` widens the
values and combines RPN/NRPN controller sequences into single MIDI 2.0
messages, so it keeps state between calls:

```cpp
yup::ump::Midi1ToMidi2DefaultTranslator translator;

for (const auto& view : midi1Packets)
    translator.dispatch (view, [&] (const yup::ump::View& midi2) { midi2Packets.add (midi2); });
```

**MIDI 2.0 UMP to MIDI 1.0 UMP:**
`Conversion::midi2ToMidi1DefaultTranslation (view, callback)` narrows the
values. MIDI 2.0 messages without a MIDI 1.0 equivalent are dropped.

**UMP to MidiMessage:** `Midi1ToBytestreamTranslator (sysExBufferSize)`
turns MIDI 1.0 packets back into messages, joining SysEx packets.
`ToBytestreamDispatcher` does the whole job from a raw array of words in
either protocol:

```cpp
yup::ump::ToBytestreamDispatcher toBytes (1024);

toBytes.dispatch (words, words + numWords, timeStamp, [&] (const yup::MidiMessage& message)
{
    midiBuffer.addEvent (message, samplePosition);
});
```

`GenericUMPConverter (PacketProtocol)` accepts either a `BytestreamMidiView`
or a `View` and always outputs the selected protocol. `Conversion` also
exposes the scaling helpers (`scaleTo7()`, `scaleTo14()`, `scaleTo16()`,
`scaleTo32()`) used by all of these.

```{note}
The stateful converters (the translators and dispatchers) must be fed one
stream each and `reset()` when that stream restarts. Create them outside the
audio thread, and give the ones taking a storage size enough room for the
largest SysEx message you expect.
```

## Receiving and sending with devices

MIDI inputs can deliver UMP directly. Pass the protocol you want and a
`ump::Receiver` when opening the device; packets from MIDI 1.0 devices are
converted for you:

```cpp
struct MyReceiver : yup::ump::Receiver
{
    void packetReceived (const yup::ump::View& packet, double time) override
    {
        // called on the MIDI thread, time in seconds
    }
};

MyReceiver receiver;
auto input = yup::MidiInput::openDevice (info.identifier, yup::ump::PacketProtocol::MIDI_2_0, &receiver);
input->start();
```

```{warning}
`packetReceived()` runs on a high-priority system thread. Don't block or touch
the UI from it; queue the packets for the audio or message thread instead.
`UMPPacketCollector` (module `yup_audio_devices`) is a ready-made `Receiver`
that turns incoming packets into per-block `UMPPacketBuffer`s.
```

`MidiOutput::sendMessageNow()` accepts a `ump::View` or a whole `ump::Packets`
as well as a `MidiMessage`. `MidiDeviceInfo::protocol` and `supportsMidi2`
tell what the operating system reports for each device. Opening devices,
choosing them and handling disconnection are covered in
[Devices](devices.md).

## Keyboard state

`UMPKeyboardState` is the UMP counterpart of `MidiKeyboardState`: it tracks
held notes from packets of the configured group and can inject notes played
from code.

```cpp
yup::UMPKeyboardState keyboard (yup::ump::PacketProtocol::MIDI_2_0);
keyboard.setGroup (0);

keyboard.processNextUMPPacket (yup::ump::View (message.data));
const bool held = keyboard.isNoteOn (1, 60); // channels 1..16 here
```

`processNextUMPBuffer (buffer, startSample, numSamples, true)` updates the
state from a `UMPPacketBuffer` and adds the notes queued with `noteOn()` and
`noteOff()`, in the protocol set with `setProtocol()`. Its `Listener` gets
`handleNoteOn()` and `handleNoteOff()` synchronously on the calling thread,
with the same rules as for `MidiKeyboardState` (see [MIDI](midi.md)).

## System exclusive

`ump::SysEx7` holds a complete 7-bit SysEx message: a `manufacturerId` and
the `data` bytes. A message longer than one packet is split across several,
so collect them with `SysEx7Collector`:

```cpp
yup::ump::SysEx7Collector collector ([] (const yup::ump::SysEx7& sysex)
{
    handleSysEx (sysex.manufacturerId, sysex.data);
});

collector.setMaxSysExDataSize (4096); // optional limit, preallocated

yup::ump::UniversalPacket packet;
std::copy (view.begin(), view.end(), packet.data);
collector.feed (packet); // ignores anything that is not a SysEx7 packet
```

To send one, `asSysEx7Packets (sysex, group)` splits it into packets, which
can be added to a `ump::Packets` with `View (packet.data)`. `SysEx8`,
`SysEx8Collector` and `asSysEx8Packets()` do the same for 8-bit SysEx, with
a stream id.

`Midi1ByteStreamParser` goes straight from MIDI 1.0 bytes to
`UniversalPacket`s, optionally delivering SysEx as whole `SysEx7` messages
through a second callback.

## MIDI-CI

MIDI Capability Inquiry messages are SysEx7 messages, so they travel through
the collectors above. `isCapabilityInquiryMessage (sysex)` recognizes them,
and the `ump::ci` namespace has a `make...()` builder and a `...View` reader
for each message:

```cpp
if (yup::ump::isCapabilityInquiryMessage (sysex))
{
    if (const auto reply = yup::ump::ci::as<yup::ump::ci::DiscoveryReplyView> (sysex))
        rememberDevice (reply->getSourceMuid());
}
```

Builders such as `ci::makeDiscoveryInquiry()`, `ci::makeEndpointInformationInquiry()`
and `ci::makeProfileInquiryMessage()` return a `ci::Message`, which is a
`SysEx7` ready for `asSysEx7Packets()`. YUP provides the message formats; the
discovery and negotiation logic is up to the application.

## Related pages

- [MIDI](midi.md) - `MidiMessage`, `MidiBuffer` and keyboard state.
- [Devices](devices.md) - opening MIDI inputs and outputs, protocols and disconnection.
- [MPE](mpe.md) - per-note expression on MIDI 1.0 channels.
- [Synthesizers](synthesizers.md) - voices driven by MIDI.
- [Web](web.md) - audio and MIDI in the browser.
