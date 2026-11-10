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

#include "../yup_audio_plugin_client.h"

#include "../common/yup_AudioPluginUtilities.h"

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_VST3)
#error "YUP_AUDIO_PLUGIN_ENABLE_VST3 must be defined"
#endif

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/main/pluginfactory.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/gui/iplugview.h>
#include <pluginterfaces/gui/iplugviewcontentscalesupport.h>
#include <pluginterfaces/vst/ivstchannelcontextinfo.h>
#include <pluginterfaces/vst/ivstcontextmenu.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstmidicontrollers.h>
#include <pluginterfaces/vst/ivstnoteexpression.h>
#include <pluginterfaces/vst/ivstremapparamid.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/ivstplugview.h>
#include <pluginterfaces/vst/vstpresetkeys.h>

#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

//==============================================================================

extern "C" yup::AudioProcessor* YUP_AUDIO_PLUGIN_CREATE_FUNCTION();

namespace yup
{

using namespace Steinberg;

namespace
{

FUID toFUID (const String& source)
{
    const auto uid = Uuid::fromSHA1 (SHA1 (source.toUTF8()));
    return { uid.getPart (0), uid.getPart (1), uid.getPart (2), uid.getPart (3) };
}

//==============================================================================

const Vst::TChar* toTChar (const CharPointer_UTF16& source)
{
    return reinterpret_cast<const Vst::TChar*> (source.getAddress());
}

String toString (const Vst::TChar* source)
{
    return CharPointer_UTF16 (reinterpret_cast<const CharPointer_UTF16::CharType*> (source));
}

/** Declares the host application from the name the host context reports. */
void updatePluginHostTypeFromContext (FUnknown* context)
{
    FUnknownPtr<Vst::IHostApplication> hostApplication (context);
    if (hostApplication == nullptr)
        return;

    Vst::String128 name {};
    if (hostApplication->getName (name) == kResultOk)
        updatePluginHostType (toString (name));
}

//==============================================================================

void toString128 (const String& source, Vst::String128 destination)
{
    if (source.isEmpty())
    {
        destination[0] = 0;
        return;
    }

    constexpr size_t capacity = sizeof (Vst::String128) / sizeof (Vst::TChar) - 1;

    const CharPointer_UTF16 utf16 (source.toUTF16());
    const auto* units = utf16.getAddress();

    size_t length = 0;
    while (length < capacity && units[length] != 0)
        ++length;

    if (length == capacity && units[length] != 0 && (units[length - 1] & 0xfc00) == 0xd800)
        --length;

    std::memcpy (destination, units, length * sizeof (Vst::TChar));
    destination[length] = 0;
}

//==============================================================================

struct VST3NoteExpressionType
{
    Vst::NoteExpressionTypeID typeId;
    NoteExpression::Type type;
    const char* title;
    const char* shortTitle;
    double defaultValue;
    int32 flags;
};

// The first entries have MIDI 1.0 equivalents and are offered to every processor
constexpr VST3NoteExpressionType vst3NoteExpressionTypes[] = {
    { Vst::kTuningTypeID, NoteExpression::Type::tuning, "Tuning", "Tune", 0.5, Vst::NoteExpressionTypeInfo::kIsBipolar },
    { Vst::kBrightnessTypeID, NoteExpression::Type::brightness, "Brightness", "Bright", 0.0, 0 },
    { Vst::kVolumeTypeID, NoteExpression::Type::volume, "Volume", "Vol", 0.25, 0 },
    { Vst::kPanTypeID, NoteExpression::Type::pan, "Pan", "Pan", 0.5, Vst::NoteExpressionTypeInfo::kIsBipolar },
    { Vst::kVibratoTypeID, NoteExpression::Type::vibrato, "Vibrato", "Vib", 0.0, 0 },
    { Vst::kExpressionTypeID, NoteExpression::Type::expression, "Expression", "Expr", 0.0, 0 }
};

constexpr int numMidiConvertibleNoteExpressionTypes = 2;

int getNumVST3NoteExpressionTypes (const AudioProcessor& processor)
{
    return processor.supportsNoteExpression() ? static_cast<int> (std::size (vst3NoteExpressionTypes)) : numMidiConvertibleNoteExpressionTypes;
}

const VST3NoteExpressionType* findVST3NoteExpressionType (const AudioProcessor& processor, Vst::NoteExpressionTypeID typeId)
{
    for (int index = 0; index < getNumVST3NoteExpressionTypes (processor); ++index)
        if (vst3NoteExpressionTypes[index].typeId == typeId)
            return vst3NoteExpressionTypes + index;

    return nullptr;
}

/** Converts a normalized VST3 note expression value to the plain units of NoteExpression. */
float vst3NoteExpressionToPlain (NoteExpression::Type type, double normalized)
{
    switch (type)
    {
        case NoteExpression::Type::volume:
            return static_cast<float> (normalized * 4.0);
        case NoteExpression::Type::tuning:
            return static_cast<float> ((normalized - 0.5) * 240.0);
        default:
            return static_cast<float> (normalized);
    }
}

double plainToVST3NoteExpression (NoteExpression::Type type, float plain)
{
    switch (type)
    {
        case NoteExpression::Type::volume:
            return jlimit (0.0, 1.0, plain / 4.0);
        case NoteExpression::Type::tuning:
            return jlimit (0.0, 1.0, plain / 240.0 + 0.5);
        default:
            return jlimit (0.0, 1.0, static_cast<double> (plain));
    }
}

//==============================================================================

Vst::ParamID getVST3ParameterID (const AudioParameter::Ptr& parameter)
{
    return static_cast<Vst::ParamID> (parameter->getHostParameterID());
}

constexpr int vst3WrapperStateMagic = 0x33535659; // "YVS3"
constexpr int vst3WrapperStateVersion = 1;

MemoryBlock readVST3StreamData (IBStream& stream)
{
    MemoryBlock data;
    char buffer[4096];

    // Some hosts report failure on the read that reaches the end of the stream, keep its bytes
    for (;;)
    {
        int32 bytesRead = 0;
        const auto result = stream.read (buffer, sizeof (buffer), &bytesRead);

        if (bytesRead > 0)
            data.append (buffer, static_cast<size_t> (jmin (bytesRead, static_cast<int32> (sizeof (buffer)))));

        if (result != kResultOk || bytesRead <= 0)
            break;
    }

    return data;
}

bool writeVST3StreamData (IBStream& stream, const MemoryBlock& data)
{
    // Some host streams accept partial writes, keep writing until everything is stored
    auto* bytes = static_cast<const char*> (data.getData());
    auto remaining = static_cast<int64> (data.getSize());

    while (remaining > 0)
    {
        int32 written = 0;
        const auto chunkSize = static_cast<int32> (jmin (remaining, static_cast<int64> (std::numeric_limits<int32>::max())));
        if (stream.write (const_cast<char*> (bytes), chunkSize, &written) != kResultOk || written <= 0)
            return false;

        bytes += written;
        remaining -= written;
    }

    return true;
}

class AudioPluginPlayHeadVST3 final : public AudioPlayHead
{
public:
    explicit AudioPluginPlayHeadVST3 (const Vst::ProcessContext* processContext)
        : processContext (processContext)
    {
    }

    bool canControlTransport() override
    {
        return false;
    }

    std::optional<PositionInfo> getPosition() const override
    {
        if (processContext == nullptr)
            return {};

        PositionInfo result;

        result.setTimeInSamples (static_cast<int64_t> (processContext->projectTimeSamples));

        if (processContext->sampleRate > 0.0)
            result.setTimeInSeconds (static_cast<double> (processContext->projectTimeSamples) / processContext->sampleRate);

        if ((processContext->state & Vst::ProcessContext::kTempoValid) != 0)
            result.setBpm (processContext->tempo);

        if ((processContext->state & Vst::ProcessContext::kTimeSigValid) != 0)
            result.setTimeSignature (TimeSignature { processContext->timeSigNumerator, processContext->timeSigDenominator });

        if ((processContext->state & Vst::ProcessContext::kProjectTimeMusicValid) != 0)
            result.setPpqPosition (processContext->projectTimeMusic);

        if ((processContext->state & Vst::ProcessContext::kBarPositionValid) != 0)
            result.setPpqPositionOfLastBarStart (processContext->barPositionMusic);

        if ((processContext->state & Vst::ProcessContext::kCycleValid) != 0)
            result.setLoopPoints (LoopPoints { processContext->cycleStartMusic, processContext->cycleEndMusic });

        result.setIsPlaying ((processContext->state & Vst::ProcessContext::kPlaying) != 0);
        result.setIsRecording ((processContext->state & Vst::ProcessContext::kRecording) != 0);
        result.setIsLooping ((processContext->state & Vst::ProcessContext::kCycleActive) != 0);

        return result;
    }

private:
    const Vst::ProcessContext* processContext = nullptr;
};

} // namespace

//==============================================================================

static const auto YupPlugin_Processor_UID = toFUID (YupPlugin_Id);
static const auto YupPlugin_Controller_UID = toFUID (YupPlugin_Id ".controller");

//==============================================================================

struct VST3SpeakerMapping
{
    Vst::Speaker speaker;
    AudioChannelSet::ChannelType type;
};

static constexpr VST3SpeakerMapping vst3SpeakerMappings[] = {
        { Vst::kSpeakerL, AudioChannelSet::left },
        { Vst::kSpeakerR, AudioChannelSet::right },
        { Vst::kSpeakerC, AudioChannelSet::centre },
        { Vst::kSpeakerLfe, AudioChannelSet::LFE },
        { Vst::kSpeakerLs, AudioChannelSet::leftSurround },
        { Vst::kSpeakerRs, AudioChannelSet::rightSurround },
        { Vst::kSpeakerLc, AudioChannelSet::leftCentre },
        { Vst::kSpeakerRc, AudioChannelSet::rightCentre },
        { Vst::kSpeakerCs, AudioChannelSet::centreSurround },
        { Vst::kSpeakerSl, AudioChannelSet::leftSurroundSide },
        { Vst::kSpeakerSr, AudioChannelSet::rightSurroundSide },
        { Vst::kSpeakerTc, AudioChannelSet::topMiddle },
        { Vst::kSpeakerTfl, AudioChannelSet::topFrontLeft },
        { Vst::kSpeakerTfc, AudioChannelSet::topFrontCentre },
        { Vst::kSpeakerTfr, AudioChannelSet::topFrontRight },
        { Vst::kSpeakerTrl, AudioChannelSet::topRearLeft },
        { Vst::kSpeakerTrc, AudioChannelSet::topRearCentre },
        { Vst::kSpeakerTrr, AudioChannelSet::topRearRight },
        { Vst::kSpeakerLfe2, AudioChannelSet::LFE2 },
        { Vst::kSpeakerM, AudioChannelSet::centre },
        { Vst::kSpeakerACN0, AudioChannelSet::ambisonicACN0 },
        { Vst::kSpeakerACN1, AudioChannelSet::ambisonicACN1 },
        { Vst::kSpeakerACN2, AudioChannelSet::ambisonicACN2 },
        { Vst::kSpeakerACN3, AudioChannelSet::ambisonicACN3 },
        { Vst::kSpeakerACN4, AudioChannelSet::ambisonicACN4 },
        { Vst::kSpeakerACN5, AudioChannelSet::ambisonicACN5 },
        { Vst::kSpeakerACN6, AudioChannelSet::ambisonicACN6 },
        { Vst::kSpeakerACN7, AudioChannelSet::ambisonicACN7 },
        { Vst::kSpeakerACN8, AudioChannelSet::ambisonicACN8 },
        { Vst::kSpeakerACN9, AudioChannelSet::ambisonicACN9 },
        { Vst::kSpeakerACN10, AudioChannelSet::ambisonicACN10 },
        { Vst::kSpeakerACN11, AudioChannelSet::ambisonicACN11 },
        { Vst::kSpeakerACN12, AudioChannelSet::ambisonicACN12 },
        { Vst::kSpeakerACN13, AudioChannelSet::ambisonicACN13 },
        { Vst::kSpeakerACN14, AudioChannelSet::ambisonicACN14 },
        { Vst::kSpeakerACN15, AudioChannelSet::ambisonicACN15 },
        { Vst::kSpeakerACN16, AudioChannelSet::ambisonicACN16 },
        { Vst::kSpeakerACN17, AudioChannelSet::ambisonicACN17 },
        { Vst::kSpeakerACN18, AudioChannelSet::ambisonicACN18 },
        { Vst::kSpeakerACN19, AudioChannelSet::ambisonicACN19 },
        { Vst::kSpeakerACN20, AudioChannelSet::ambisonicACN20 },
        { Vst::kSpeakerACN21, AudioChannelSet::ambisonicACN21 },
        { Vst::kSpeakerACN22, AudioChannelSet::ambisonicACN22 },
        { Vst::kSpeakerACN23, AudioChannelSet::ambisonicACN23 },
        { Vst::kSpeakerACN24, AudioChannelSet::ambisonicACN24 },
        { Vst::kSpeakerTsl, AudioChannelSet::topSideLeft },
        { Vst::kSpeakerTsr, AudioChannelSet::topSideRight },
        { Vst::kSpeakerLcs, AudioChannelSet::leftSurroundRear },
        { Vst::kSpeakerRcs, AudioChannelSet::rightSurroundRear },
        { Vst::kSpeakerBfl, AudioChannelSet::bottomFrontLeft },
        { Vst::kSpeakerBfc, AudioChannelSet::bottomFrontCentre },
        { Vst::kSpeakerBfr, AudioChannelSet::bottomFrontRight },
        { Vst::kSpeakerPl, AudioChannelSet::proximityLeft },
        { Vst::kSpeakerPr, AudioChannelSet::proximityRight },
        { Vst::kSpeakerBsl, AudioChannelSet::bottomSideLeft },
        { Vst::kSpeakerBsr, AudioChannelSet::bottomSideRight },
        { Vst::kSpeakerBrl, AudioChannelSet::bottomRearLeft },
        { Vst::kSpeakerBrc, AudioChannelSet::bottomRearCentre },
        { Vst::kSpeakerBrr, AudioChannelSet::bottomRearRight },
        { Vst::kSpeakerLw, AudioChannelSet::wideLeft },
        { Vst::kSpeakerRw, AudioChannelSet::wideRight }
};

static std::optional<Vst::Speaker> speakerForChannelType (AudioChannelSet::ChannelType type)
{
    for (const auto& mapping : vst3SpeakerMappings)
        if (mapping.type == type)
            return mapping.speaker;

    return std::nullopt;
}

static std::optional<AudioChannelSet::ChannelType> channelTypeForSpeaker (Vst::Speaker speaker)
{
    for (const auto& mapping : vst3SpeakerMappings)
        if (mapping.speaker == speaker)
            return mapping.type;

    return std::nullopt;
}

static Vst::SpeakerArrangement speakerArrForChannels (int channels)
{
    switch (channels)
    {
        case 0:
            return Vst::SpeakerArr::kEmpty;
        case 1:
            return Vst::SpeakerArr::kMono;
        case 2:
            return Vst::SpeakerArr::kStereo;
        case 4:
            return Vst::SpeakerArr::k40Music;
        case 5:
            return Vst::SpeakerArr::k50;
        case 6:
            return Vst::SpeakerArr::k51;
        case 8:
            return Vst::SpeakerArr::k71CineFullFront;
        default:
            // Any arrangement with the right number of speakers
            return channels < 64 ? (Vst::SpeakerArrangement (1) << channels) - 1 : Vst::SpeakerArr::kStereo;
    }
}

/** Converts a channel set to a VST3 speaker arrangement, falling back to one with the same channel count. */
static Vst::SpeakerArrangement speakerArrForChannelSet (const AudioChannelSet& set)
{
    if (set == AudioChannelSet::mono())
        return Vst::SpeakerArr::kMono;

    Vst::SpeakerArrangement arrangement = 0;

    for (const auto type : set.getChannelTypes())
    {
        const auto speaker = speakerForChannelType (type);
        if (! speaker.has_value())
            return speakerArrForChannels (set.size());

        arrangement |= *speaker;
    }

    return arrangement;
}

/** Converts a VST3 speaker arrangement to a channel set, falling back to the canonical set for its channel count. */
static AudioChannelSet channelSetForSpeakerArr (Vst::SpeakerArrangement arrangement)
{
    const auto numChannels = Vst::SpeakerArr::getChannelCount (arrangement);
    if (arrangement == Vst::SpeakerArr::kMono)
        return AudioChannelSet::mono();

    AudioChannelSet set;

    for (int bit = 0; bit < 64; ++bit)
    {
        const auto speaker = Vst::Speaker (1) << bit;
        if ((arrangement & speaker) == 0)
            continue;

        const auto type = channelTypeForSpeaker (speaker);
        if (! type.has_value() || set.getChannelIndexForType (*type) >= 0)
            return AudioChannelSet::canonicalChannelSet (numChannels);

        set.addChannel (*type);
    }

    return set;
}

/** Returns, for each channel of the set, its index in the VST3 buffers of the arrangement.
    VST3 orders channels by speaker bit, AudioChannelSet by channel type. Empty means identity. */
static std::vector<int> channelOrderForSpeakerArr (const AudioChannelSet& set, Vst::SpeakerArrangement arrangement)
{
    if (set.size() != Vst::SpeakerArr::getChannelCount (arrangement) || channelSetForSpeakerArr (arrangement) != set)
        return {};

    std::vector<int> order;
    bool isIdentity = true;

    for (int channel = 0; channel < set.size(); ++channel)
    {
        const auto speaker = speakerForChannelType (set.getTypeOfChannel (channel));
        if (! speaker.has_value() || arrangement == Vst::SpeakerArr::kMono)
            return {};

        const auto index = Vst::SpeakerArr::getSpeakerIndex (*speaker, arrangement);
        isIdentity = isIdentity && index == channel;
        order.push_back (index);
    }

    return isIdentity ? std::vector<int> {} : order;
}

static void writeMidiBufferToVST3EventList (const MidiBuffer& midiBuffer,
                                            Vst::IEventList& outputEvents,
                                            int numSamples)
{
    for (const MidiMessageMetadata metadata : midiBuffer)
    {
        const auto& message = metadata.getMessage();

        Vst::Event event {};
        event.busIndex = 0;
        event.sampleOffset = jlimit (0, jmax (0, numSamples - 1), metadata.samplePosition);
        event.ppqPosition = 0.0;
        event.flags = 0;

        if (message.isNoteOn())
        {
            event.type = Vst::Event::kNoteOnEvent;
            event.noteOn.channel = static_cast<int16> (message.getChannel() - 1);
            event.noteOn.pitch = static_cast<int16> (message.getNoteNumber());
            event.noteOn.tuning = 0.0f;
            event.noteOn.velocity = message.getFloatVelocity();
            event.noteOn.length = 0;
            event.noteOn.noteId = -1;
        }
        else if (message.isNoteOff())
        {
            event.type = Vst::Event::kNoteOffEvent;
            event.noteOff.channel = static_cast<int16> (message.getChannel() - 1);
            event.noteOff.pitch = static_cast<int16> (message.getNoteNumber());
            event.noteOff.velocity = message.getFloatVelocity();
            event.noteOff.noteId = -1;
            event.noteOff.tuning = 0.0f;
        }
        else if (message.isAftertouch())
        {
            event.type = Vst::Event::kPolyPressureEvent;
            event.polyPressure.channel = static_cast<int16> (message.getChannel() - 1);
            event.polyPressure.pitch = static_cast<int16> (message.getNoteNumber());
            event.polyPressure.pressure = static_cast<float> (message.getAfterTouchValue()) / 127.0f;
            event.polyPressure.noteId = -1;
        }
        else if (message.isController())
        {
            event.type = Vst::Event::kLegacyMIDICCOutEvent;
            event.midiCCOut.channel = static_cast<int8> (jlimit (0, 15, message.getChannel() - 1));
            event.midiCCOut.controlNumber = static_cast<uint8> (message.getControllerNumber());
            event.midiCCOut.value = static_cast<int8> (message.getControllerValue());
            event.midiCCOut.value2 = 0;
        }
        else if (message.isProgramChange())
        {
            event.type = Vst::Event::kLegacyMIDICCOutEvent;
            event.midiCCOut.channel = static_cast<int8> (jlimit (0, 15, message.getChannel() - 1));
            event.midiCCOut.controlNumber = static_cast<uint8> (Vst::kCtrlProgramChange);
            event.midiCCOut.value = static_cast<int8> (message.getProgramChangeNumber());
            event.midiCCOut.value2 = 0;
        }
        else if (message.isPitchWheel())
        {
            const auto value = jlimit (0, 0x3fff, message.getPitchWheelValue());
            event.type = Vst::Event::kLegacyMIDICCOutEvent;
            event.midiCCOut.channel = static_cast<int8> (jlimit (0, 15, message.getChannel() - 1));
            event.midiCCOut.controlNumber = static_cast<uint8> (Vst::kPitchBend);
            event.midiCCOut.value = static_cast<int8> (value & 0x7f);
            event.midiCCOut.value2 = static_cast<int8> ((value >> 7) & 0x7f);
        }
        else if (message.isChannelPressure())
        {
            event.type = Vst::Event::kLegacyMIDICCOutEvent;
            event.midiCCOut.channel = static_cast<int8> (jlimit (0, 15, message.getChannel() - 1));
            event.midiCCOut.controlNumber = static_cast<uint8> (Vst::kAfterTouch);
            event.midiCCOut.value = static_cast<int8> (message.getChannelPressureValue());
            event.midiCCOut.value2 = 0;
        }
        else if (message.isSysEx())
        {
            event.type = Vst::Event::kDataEvent;
            event.data.size = static_cast<uint32> (message.getSysExDataSize());
            event.data.type = Vst::DataEvent::kMidiSysEx;
            event.data.bytes = message.getSysExData();
        }
        else
        {
            continue;
        }

        outputEvents.addEvent (event);
    }
}

//==============================================================================

#if YUP_LINUX
/** Services the YUP message loop from the host run loop, shared by every view and controller of the module. */
class HostRunLoopVST3 final
    : public FObject
    , public Linux::ITimerHandler
{
public:
    OBJ_METHODS (HostRunLoopVST3, FObject)
    REFCOUNT_METHODS (FObject)

    DEFINE_INTERFACES
    DEF_INTERFACE (Linux::ITimerHandler)
    END_DEFINE_INTERFACES (FObject)

    static void attach (Linux::IRunLoop* loop)
    {
        if (loop == nullptr)
            return;

        auto& instance = getInstance();
        if (instance == nullptr)
            instance = owned (new HostRunLoopVST3());

        instance->loops.push_back (loop);

        if (instance->loops.size() == 1)
            loop->registerTimer (instance, hostMessageTimerIntervalMs);
    }

    static void detach (Linux::IRunLoop* loop)
    {
        auto& instance = getInstance();
        if (instance == nullptr || loop == nullptr)
            return;

        auto& loops = instance->loops;
        const auto it = std::find (loops.begin(), loops.end(), loop);
        if (it == loops.end())
            return;

        // The timer lives on the first loop, move it if that loop goes away
        const auto wasTimerLoop = it == loops.begin();
        if (wasTimerLoop)
            loop->unregisterTimer (instance);

        loops.erase (it);

        if (loops.empty())
            instance = nullptr;
        else if (wasTimerLoop)
            loops.front()->registerTimer (instance, hostMessageTimerIntervalMs);
    }

    void PLUGIN_API onTimer() override
    {
        dispatchPendingMessagesFromHostTimer();
    }

private:
    HostRunLoopVST3() = default;

    static IPtr<HostRunLoopVST3>& getInstance()
    {
        static IPtr<HostRunLoopVST3> instance;
        return instance;
    }

    std::vector<Linux::IRunLoop*> loops;
};
#endif

//==============================================================================

class AudioPluginEditorViewVST3
    : public Component
    , public Vst::EditorView
    , public IPlugViewContentScaleSupport
    , public Vst::IParameterFinder
    , private AudioProcessorEditor::HostContext
    , private ComponentListener
{
public:
    OBJ_METHODS (AudioPluginEditorViewVST3, Vst::EditorView)
    REFCOUNT_METHODS (Vst::EditorView)

    DEFINE_INTERFACES
    DEF_INTERFACE (IPlugViewContentScaleSupport)
    DEF_INTERFACE (Vst::IParameterFinder)
    END_DEFINE_INTERFACES (Vst::EditorView)

    AudioPluginEditorViewVST3 (AudioProcessor* processor, Vst::EditController* controller, ViewRect* size = nullptr)
        : Vst::EditorView (controller, size)
        , processor (processor)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "creating editor view: view=" << describePointer (this) << ", processor=" << describePointer (processor) << ", controller=" << describePointer (controller));

        jassert (processor != nullptr);
        if (processor == nullptr || ! processor->hasEditor())
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "processor has no editor: processor=" << describePointer (processor));
            return;
        }

        editor.reset (processor->createEditor());
        if (editor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "processor returned null editor");
            return;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "created editor: preferredSize=" << editor->getPreferredSize().toString() << ", resizable=" << String (editor->isResizable() ? "true" : "false") << ", editor=" << describePointer (editor.get()));

        addAndMakeVisible (editor.get());

        if (size != nullptr)
            setSize (viewScale.fromHost ({ size->getWidth(), size->getHeight() }, *this).to<float>());
        else
        {
            const auto initialSize = editor->getConstrainedSize (editor->getInitialSize());
            setSize ({ static_cast<float> (initialSize.getWidth()),
                       static_cast<float> (initialSize.getHeight()) });
        }

        if (editor->shouldShowResizableCorner())
        {
            resizableCorner = std::make_unique<ResizableCorner>();
            resizableCorner->constrainSize = [this] (Size<float> size)
            {
                return editor->getConstrainedSize (size.to<int>()).to<float>();
            };
            addAndMakeVisible (*resizableCorner);
        }

        editor->addComponentListener (this);
        editor->setHostContext (this);
    }

    ~AudioPluginEditorViewVST3()
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "destroying editor view: onDesktop=" << String (isOnDesktop() ? "true" : "false"));

#if YUP_LINUX
        detachHostRunLoop();
#endif

        if (editor != nullptr)
        {
            endActiveParameterGestures (processor);
            setVisible (false);
            removeFromDesktop();

            editor->removeComponentListener (this);
            editor->setHostContext (nullptr);
            resizableCorner.reset();
            removeChildComponent (editor.get());
            editor.reset();
        }
    }

    void resized() override
    {
        fitEditorInsideBounds();

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "resized: bounds=" << getBounds().toString() << ", hostTriggered=" << String (hostTriggeredResizing ? "true" : "false"));

        if (! hostTriggeredResizing)
            requestHostResize();
    }

    void paint (Graphics& g) override
    {
        // Only the area left uncovered by a fitted editor needs painting
        if (editor == nullptr || editor->getBounds() == getLocalBounds())
            return;

        g.setFillColor (Colors::black);
        g.fillAll();
    }

    tresult PLUGIN_API attached (void* parent, FIDString type) override
    {
        auto result = Vst::EditorView::attached (parent, type);

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "attached: parent=" << describePointer (parent) << ", type=" << String (type) << ", result=" << static_cast<int> (result) << ", bounds=" << getBounds().toString());

        if (result != kResultOk)
            return result;

        if (editor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "attached failed: editor is null");
            return kInternalError;
        }

        ComponentNative::Flags flags = ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow;

        if (editor->shouldRenderContinuous())
            flags.set (ComponentNative::renderContinuous);

        auto options = ComponentNative::Options()
                           .withFlags (flags)
                           .withResizableWindow (editor->isResizable());

        const auto scaleBeforeAttach = viewScale.get (*this);

        addToDesktop (options, parent);
        setVisible (true);

        editor->attachedToNative();

#if YUP_LINUX
        attachHostRunLoop();
#endif

        // Sizes reported before attaching used an estimated scale, the editor window may use another one
        if (viewScale.get (*this) != scaleBeforeAttach)
            requestHostResize();

        return kResultTrue;
    }

    tresult PLUGIN_API removed() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "removed: onDesktop=" << String (isOnDesktop() ? "true" : "false"));

#if YUP_LINUX
        detachHostRunLoop();
#endif

        if (editor != nullptr)
        {
            endActiveParameterGestures (processor);
            setVisible (false);
            removeFromDesktop();
        }

        return CPluginView::removed();
    }

    tresult PLUGIN_API canResize() override
    {
        const auto resizable = editor != nullptr && editor->isResizable();

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "canResize: editor=" << describePointer (editor.get()) << ", result=" << String (resizable ? "true" : "false"));

        if (resizable)
            return kResultTrue;

        return kResultFalse;
    }

    tresult PLUGIN_API checkSizeConstraint (ViewRect* rect) override
    {
        if (editor == nullptr || rect == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "checkSizeConstraint failed: editor=" << describePointer (editor.get()) << ", rect=" << describePointer (rect));
        }

        if (editor == nullptr)
            return kInternalError;

        if (rect == nullptr)
            return kInvalidArgument;

        const auto requestedWidth = rect->getWidth();
        const auto requestedHeight = rect->getHeight();

        // Resizable editors accept any host size while dragging, they are fitted inside and snapped afterwards
        if (! editor->isResizable())
            constrainViewRect (*rect);

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "checkSizeConstraint: " << requestedWidth << "x" << requestedHeight << " -> " << rect->getWidth() << "x" << rect->getHeight());
        ignoreUnused (requestedWidth, requestedHeight);

        return kResultTrue;
    }

    tresult PLUGIN_API onSize (ViewRect* newSize) override
    {
        if (editor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "onSize failed: editor is null");
            return kInternalError;
        }

        if (newSize != nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "onSize: left=" << newSize->left << ", top=" << newSize->top << ", size=" << newSize->getWidth() << "x" << newSize->getHeight() << ", current=" << getBounds().toString());

            if (! editor->isResizable())
            {
                const auto preferredSize = viewScale.toHost (editor->getPreferredSize(), *this);
                newSize->right = newSize->left + preferredSize.getWidth();
                newSize->bottom = newSize->top + preferredSize.getHeight();
            }

            rect = *newSize;

            const auto scoped = ScopedValueSetter<bool> (hostTriggeredResizing, true);

            setSize (viewScale.fromHost ({ rect.getWidth(), rect.getHeight() }, *this).to<float>());

            if (editor->isResizable())
                hostResizeSnapper.hostResized();
        }

        return kResultTrue;
    }

    tresult PLUGIN_API getSize (ViewRect* size) override
    {
        if (editor == nullptr || size == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getSize failed: editor=" << describePointer (editor.get()) << ", size=" << describePointer (size));
        }

        if (editor == nullptr)
            return kInternalError;

        if (size == nullptr)
            return kInvalidArgument;

        const auto editorSize = editor->isResizable() && editor->getWidth() != 0 && editor->getHeight() != 0
                                  ? getBounds().getSize().to<int>()
                                  : editor->getConstrainedSize (editor->getInitialSize());

        const auto hostSize = viewScale.toHost (editorSize, *this);
        size->left = 0;
        size->top = 0;
        size->right = hostSize.getWidth();
        size->bottom = hostSize.getHeight();

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getSize: " << size->getWidth() << "x" << size->getHeight());

        return kResultTrue;
    }

    tresult PLUGIN_API onFocus (TBool state) override
    {
        if (editor == nullptr)
            return kInternalError;

        if (state)
            editor->takeKeyboardFocus();
        else
            editor->leaveKeyboardFocus();

        return kResultTrue;
    }

    tresult PLUGIN_API findParameter (int32 xPos, int32 yPos, Vst::ParamID& resultTag) override
    {
        if (editor == nullptr)
            return kResultFalse;

        const auto scale = viewScale.get (*this);
        const auto position = Point<float> (static_cast<float> (xPos), static_cast<float> (yPos)) / scale;

        if (const auto parameter = editor->getParameterAt (position - editor->getPosition()))
        {
            resultTag = getVST3ParameterID (parameter);
            return kResultTrue;
        }

        return kResultFalse;
    }

    bool showParameterContextMenu (const AudioParameter::Ptr& parameter, Point<float> position) override
    {
        auto* controller = getController();
        if (parameter == nullptr || controller == nullptr)
            return false;

        FUnknownPtr<Vst::IComponentHandler3> handler (controller->getComponentHandler());
        if (handler == nullptr)
            return false;

        const auto tag = getVST3ParameterID (parameter);
        IPtr<Vst::IContextMenu> menu (handler->createContextMenu (this, &tag), false);
        if (menu == nullptr)
            return false;

        const auto hostPosition = (position + editor->getPosition()) * viewScale.get (*this);
        return menu->popup (roundToInt (hostPosition.getX()), roundToInt (hostPosition.getY())) == kResultOk;
    }

    tresult PLUGIN_API setContentScaleFactor (ScaleFactor factor) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setContentScaleFactor: factor=" << String (factor));

#if YUP_MAC
        // Cocoa views are sized in points and the OS provides the backing scale
        ignoreUnused (factor);
        return kResultFalse;
#else
        // Once attached the editor window follows its display scale, view sizes are converted with it
        if (viewScale.setHostScale (static_cast<float> (factor)) && ! isOnDesktop())
            requestHostResize();

        return kResultTrue;
#endif
    }

    tresult PLUGIN_API isPlatformTypeSupported (FIDString type) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "isPlatformTypeSupported requested: type=" << String (type));

#if YUP_WINDOWS
        if (std::string_view (type) == kPlatformTypeHWND)
            return kResultTrue;
#elif YUP_MAC
        if (std::string_view (type) == kPlatformTypeNSView)
            return kResultTrue;
        else if (std::string_view (type) == kPlatformTypeHIView)
            return kResultFalse;
#elif YUP_LINUX
        if (std::string_view (type) == kPlatformTypeX11EmbedWindowID)
            return kResultTrue;
#endif

        return kResultFalse;
    }

private:
    void componentResized (Component& component) override
    {
        if (fittingEditor)
            return;

        const auto editorSize = component.getBounds().getSize();
        if (editorSize != getBounds().getSize())
            setSize (editorSize);
    }

    void fitEditorInsideBounds()
    {
        const auto size = editor->getConstrainedSizeToFit (getBounds().getSize().to<int>());

        const auto scoped = ScopedValueSetter<bool> (fittingEditor, true);
        editor->setBounds ({ 0.0f, 0.0f, size.to<float>() });
    }

    void snapToConstrainedSize()
    {
        if (editor == nullptr)
            return;

        const auto size = editor->getConstrainedSizeToFit (getBounds().getSize().to<int>()).to<float>();

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "snapping to constrained size: " << getBounds().getSize().toString() << " -> " << size.toString());

        if (size != getBounds().getSize())
            setSize (size);
    }

    void constrainViewRect (ViewRect& viewRect) const
    {
        const auto size = editor->getConstrainedSizeToFit (viewScale.fromHost ({ viewRect.getWidth(), viewRect.getHeight() }, *this));
        const auto hostSize = viewScale.toHost (size, *this);

        viewRect.right = viewRect.left + hostSize.getWidth();
        viewRect.bottom = viewRect.top + hostSize.getHeight();
    }

    void requestHostResize()
    {
        if (plugFrame == nullptr)
            return;

        const auto hostSize = viewScale.toHost (getBounds().getSize().to<int>(), *this);

        ViewRect viewRect (0, 0, hostSize.getWidth(), hostSize.getHeight());
        const auto result = plugFrame->resizeView (this, std::addressof (viewRect));

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "requested host resizeView: " << viewRect.getWidth() << "x" << viewRect.getHeight() << ", result=" << static_cast<int> (result));
        ignoreUnused (result);
    }

#if YUP_LINUX
    void attachHostRunLoop()
    {
        if (plugFrame != nullptr && plugFrame->queryInterface (Linux::IRunLoop::iid, reinterpret_cast<void**> (&hostRunLoop)) != kResultOk)
            hostRunLoop = nullptr;

        HostRunLoopVST3::attach (hostRunLoop);
    }

    void detachHostRunLoop()
    {
        HostRunLoopVST3::detach (hostRunLoop);

        if (hostRunLoop != nullptr)
            hostRunLoop->release();

        hostRunLoop = nullptr;
    }

    Linux::IRunLoop* hostRunLoop = nullptr;
#endif

    ScopedYupInitialiser_Windowing scopeInitialiser;
    WrapperViewScale viewScale;
    AudioProcessor* processor = nullptr;
    std::unique_ptr<AudioProcessorEditor> editor;
    std::unique_ptr<ResizableCorner> resizableCorner;
    bool hostTriggeredResizing = false;
    bool fittingEditor = false;
    HostResizeSnapper hostResizeSnapper { [this]
                                          {
                                              snapToConstrainedSize();
                                          } };
};

//==============================================================================

class AudioPluginControllerVST3
    : public Vst::EditController
    , public Vst::IMidiMapping
    , public Vst::IUnitInfo
    , public Vst::INoteExpressionController
    , public Vst::IRemapParamID
    , public Vst::ChannelContext::IInfoListener
    , private AudioParameter::Listener
    , private AudioProcessor::Listener
{
public:
    //==============================================================================

    OBJ_METHODS (AudioPluginControllerVST3, Vst::EditController)
    REFCOUNT_METHODS (Vst::EditController)

    //==============================================================================

    DEFINE_INTERFACES
    DEF_INTERFACE (Vst::IEditController)
    DEF_INTERFACE (Vst::IMidiMapping)
    DEF_INTERFACE (Vst::IUnitInfo)
    DEF_INTERFACE (Vst::INoteExpressionController)
    DEF_INTERFACE (Vst::IRemapParamID)
    DEF_INTERFACE (Vst::ChannelContext::IInfoListener)
    END_DEFINE_INTERFACES (Vst::EditController)

    //==============================================================================

    static FUnknown* createInstance ([[maybe_unused]] void* context)
    {
        auto* instance = new AudioPluginControllerVST3;
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "created controller instance: controller=" << describePointer (instance));
        return (Vst::IEditController*) instance;
    }

    //==============================================================================

    AudioPluginControllerVST3()
    {
    }

    ~AudioPluginControllerVST3()
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "destroying controller instance: controller=" << describePointer (this) << ", processor=" << describePointer (processor));

        removeParameterListeners();
        removeProcessorListener();
    }

    //==============================================================================

    tresult PLUGIN_API initialize (FUnknown* context) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller initialize requested: context=" << describePointer (context));

        auto result = Vst::EditController::initialize (context);
        updatePluginHostTypeFromContext (context);

        if (result != kResultOk)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller initialize failed: base result=" << static_cast<int> (result));
            return result;
        }

#if YUP_LINUX
        // Some hosts expose their run loop on the context, it services messages while no editor is open
        if (context == nullptr || context->queryInterface (Linux::IRunLoop::iid, reinterpret_cast<void**> (&hostRunLoop)) != kResultOk)
            hostRunLoop = nullptr;

        HostRunLoopVST3::attach (hostRunLoop);
#endif

        return kResultOk;
    }

    tresult PLUGIN_API terminate() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller terminate requested: controller=" << describePointer (this) << ", processor=" << describePointer (processor));

#if YUP_LINUX
        HostRunLoopVST3::detach (hostRunLoop);

        if (hostRunLoop != nullptr)
            hostRunLoop->release();

        hostRunLoop = nullptr;
#endif

        removeParameterListeners();
        removeProcessorListener();
        processor = nullptr;

        return Vst::EditController::terminate();
    }

    //==============================================================================

    tresult PLUGIN_API connect (Vst::IConnectionPoint* other) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller connect requested: other=" << describePointer (other));

        return Vst::EditController::connect (other);
    }

    tresult PLUGIN_API disconnect (Vst::IConnectionPoint* other) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller disconnect requested: other=" << describePointer (other));

        removeParameterListeners();
        removeProcessorListener();
        processor = nullptr;
        return Vst::EditController::disconnect (other);
    }

    tresult PLUGIN_API notify (Vst::IMessage* message) override
    {
        if (message == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller notify failed: message is null");
            return kInvalidArgument;
        }

        auto msgID = message->getMessageID();
        if (std::string_view (msgID) != "processor")
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller notify ignored: messageID=" << String (msgID));
            return kResultFalse;
        }

        if (auto attributes = message->getAttributes())
        {
            const void* msgData;
            uint32 msgSize;

            auto result = attributes->getBinary ("data", msgData, msgSize);
            if (result == kResultTrue && msgSize == sizeof (void*))
            {
                removeProcessorListener();
                processor = static_cast<AudioProcessor*> (*reinterpret_cast<void* const*> (msgData));

                setupParameters();
                addProcessorListener();

                YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller connected to processor: processor=" << describePointer (processor) << ", parameters=" << String (static_cast<int> (parameters.getParameterCount())));

                return result;
            }

            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller notify failed: invalid processor data, getBinary result=" << static_cast<int> (result));
            return kResultFalse;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller notify failed: message has no attributes");
        return kResultFalse;
    }

    //==============================================================================

    tresult PLUGIN_API setState (IBStream* state) override
    {
        if (state == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller setState failed: state is null");
            return kInvalidArgument;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller setState completed: nothing to restore");
        return kResultOk;
    }

    tresult PLUGIN_API getState (IBStream* state) override
    {
        if (state == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller getState failed: state is null");
            return kInvalidArgument;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "controller getState completed: nothing to save");
        return kResultOk;
    }

    tresult PLUGIN_API setComponentState (IBStream* state) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setComponentState requested");

        if (processor == nullptr || state == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setComponentState failed: processor=" << describePointer (processor) << ", state=" << describePointer (state));
            return kResultFalse;
        }

        const auto wrapperState = readWrapperBypassState (readVST3StreamData (*state),
                                                          vst3WrapperStateMagic,
                                                          vst3WrapperStateVersion);
        if (! wrapperState.hasWrapperState)
        {
            syncProcessorParametersToController();
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setComponentState completed without wrapper state");
            return kResultOk;
        }

        Vst::EditController::setParamNormalized (bypass.getHostParameterID(),
                                                 wrapperState.isBypassed ? 1.0 : 0.0);

        syncProcessorParametersToController();

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setComponentState completed: bypassed=" << String (wrapperState.isBypassed ? "true" : "false"));

        return kResultOk;
    }

    //==============================================================================

    tresult PLUGIN_API getParameterInfo (int32 paramIndex, Vst::ParameterInfo& info) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (auto parameter = parameters.getParameterByIndex (paramIndex))
        {
            info = parameter->getInfo();
            return kResultOk;
        }

        return kResultFalse;
    }

    tresult PLUGIN_API getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (isProgramParameter (tag))
            return Vst::EditController::getParamStringByValue (tag, valueNormalized, string);

        if (bypass.isSyntheticParameter (tag))
        {
            toString128 (valueNormalized >= 0.5 ? "On" : "Off", string);
            return kResultOk;
        }

        if (auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag)))
        {
            toString128 (parameter->convertToString (parameter->convertToDenormalizedValue (valueNormalized)), string);
            return kResultOk;
        }

        return kResultFalse;
    }

    tresult PLUGIN_API getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (isProgramParameter (tag))
            return Vst::EditController::getParamValueByString (tag, string, valueNormalized);

        if (bypass.isSyntheticParameter (tag))
        {
            const auto str = toString (string);
            valueNormalized = (str == "On" || str == "1") ? 1.0 : 0.0;
            return kResultOk;
        }

        if (auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag)))
        {
            valueNormalized = parameter->convertToNormalizedValue (parameter->convertFromString (toString (string)));
            return kResultOk;
        }

        return kResultFalse;
    }

    Vst::ParamValue PLUGIN_API normalizedParamToPlain (Vst::ParamID tag, Vst::ParamValue valueNormalized) override
    {
        if (processor == nullptr)
            return valueNormalized;

        if (isProgramParameter (tag))
            return Vst::EditController::normalizedParamToPlain (tag, valueNormalized);

        if (bypass.isSyntheticParameter (tag))
            return valueNormalized;

        if (auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag)))
            return parameter->convertToDenormalizedValue (valueNormalized);

        return valueNormalized;
    }

    Vst::ParamValue PLUGIN_API plainParamToNormalized (Vst::ParamID tag, Vst::ParamValue plainValue) override
    {
        if (processor == nullptr)
            return plainValue;

        if (isProgramParameter (tag))
            return Vst::EditController::plainParamToNormalized (tag, plainValue);

        if (bypass.isSyntheticParameter (tag))
            return plainValue;

        if (auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag)))
            return parameter->convertToNormalizedValue (plainValue);

        return plainValue;
    }

    Vst::ParamValue PLUGIN_API getParamNormalized (Vst::ParamID tag) override
    {
        if (processor == nullptr)
            return 0.0;

        if (bypass.isSyntheticParameter (tag) || isProgramParameter (tag))
            return Vst::EditController::getParamNormalized (tag);

        if (auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag)))
            return parameter->getNormalizedValue();

        return 0.0;
    }

    tresult PLUGIN_API setParamNormalized (Vst::ParamID tag, Vst::ParamValue value) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (bypass.isSyntheticParameter (tag))
            return Vst::EditController::setParamNormalized (tag, value);

        if (isProgramParameter (tag))
        {
            const auto result = Vst::EditController::setParamNormalized (tag, value);
            const auto index = static_cast<int> (Vst::EditController::normalizedParamToPlain (tag, value));
            if (index != processor->getCurrentPreset())
                processor->setCurrentPreset (index);

            return result;
        }

        if (auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag)))
        {
            parameter->setNormalizedValue (static_cast<float> (value));
            Vst::EditController::setParamNormalized (tag, value);
            return kResultOk;
        }

        return kResultFalse;
    }

    tresult PLUGIN_API getCompatibleParamID (const TUID pluginToReplaceUID,
                                             Vst::ParamID oldParamID,
                                             Vst::ParamID& newParamID) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (processor->getParameterByHostID (static_cast<uint32> (oldParamID)) != nullptr
            || bypass.isSyntheticParameter (oldParamID)
            || isProgramParameter (oldParamID))
        {
            newParamID = oldParamID;
            return kResultOk;
        }

        return kResultFalse;
    }

    //==============================================================================

    tresult PLUGIN_API getMidiControllerAssignment (int32 busIndex,
                                                    int16 channel,
                                                    Vst::CtrlNumber midiControllerNumber,
                                                    Vst::ParamID& id) override
    {
        if (processor == nullptr || busIndex != 0)
            return kResultFalse;

        // VST3 controller numbers use 128 for channel pressure and 129 for pitch bend, like AudioProcessor
        const auto parameterIndex = processor->getParameterIndexForMidiController (channel + 1, static_cast<int> (midiControllerNumber));
        const auto parameters = processor->getParameters();
        if (! isPositiveAndBelow (parameterIndex, static_cast<int> (parameters.size())))
            return kResultFalse;

        id = getVST3ParameterID (parameters[parameterIndex]);
        return kResultOk;
    }

    //==============================================================================

    int32 PLUGIN_API getNoteExpressionCount (int32 busIndex, int16 channel) override
    {
        ignoreUnused (channel);

        if (processor == nullptr || busIndex != 0 || ! processor->acceptsMidi())
            return 0;

        return getNumVST3NoteExpressionTypes (*processor);
    }

    tresult PLUGIN_API getNoteExpressionInfo (int32 busIndex, int16 channel, int32 noteExpressionIndex, Vst::NoteExpressionTypeInfo& info) override
    {
        if (! isPositiveAndBelow (noteExpressionIndex, getNoteExpressionCount (busIndex, channel)))
            return kInvalidArgument;

        const auto& type = vst3NoteExpressionTypes[noteExpressionIndex];

        info = {};
        info.typeId = type.typeId;
        toString128 (type.title, info.title);
        toString128 (type.shortTitle, info.shortTitle);
        toString128 (type.type == NoteExpression::Type::tuning ? "st" : "", info.units);
        info.unitId = Vst::kRootUnitId;
        info.valueDesc.minimum = 0.0;
        info.valueDesc.maximum = 1.0;
        info.valueDesc.defaultValue = type.defaultValue;
        info.associatedParameterId = Vst::kNoParamId;
        info.flags = type.flags;
        return kResultOk;
    }

    tresult PLUGIN_API getNoteExpressionStringByValue (int32 busIndex, int16 channel, Vst::NoteExpressionTypeID id, Vst::NoteExpressionValue valueNormalized, Vst::String128 string) override
    {
        ignoreUnused (busIndex, channel);

        const auto* type = processor != nullptr ? findVST3NoteExpressionType (*processor, id) : nullptr;
        if (type == nullptr)
            return kInvalidArgument;

        toString128 (String (vst3NoteExpressionToPlain (type->type, valueNormalized), 2), string);
        return kResultOk;
    }

    tresult PLUGIN_API getNoteExpressionValueByString (int32 busIndex, int16 channel, Vst::NoteExpressionTypeID id, const Vst::TChar* string, Vst::NoteExpressionValue& valueNormalized) override
    {
        ignoreUnused (busIndex, channel);

        const auto* type = processor != nullptr ? findVST3NoteExpressionType (*processor, id) : nullptr;
        if (type == nullptr || string == nullptr)
            return kInvalidArgument;

        valueNormalized = plainToVST3NoteExpression (type->type, toString (string).getFloatValue());
        return kResultOk;
    }

    //==============================================================================

    tresult PLUGIN_API getUnitInfo (int32 unitIndex, Vst::UnitInfo& info) override
    {
        if (unitIndex == 0)
        {
            info.id = Vst::kRootUnitId;
            info.parentUnitId = Vst::kNoParentUnitId;
            info.programListId = hasProgramList() ? programListID : Vst::kNoProgramListId;
            toString128 ("root", info.name);
            return kResultOk;
        }

        return kResultFalse;
    }

    Vst::UnitID PLUGIN_API getSelectedUnit() override
    {
        return Vst::kRootUnitId;
    }

    int32 PLUGIN_API getUnitCount() override
    {
        return 1;
    }

    tresult PLUGIN_API selectUnit (Vst::UnitID unitId) override
    {
        if (unitId == Vst::kRootUnitId)
            return kResultOk;

        return kResultFalse;
    }

    tresult PLUGIN_API getUnitByBus (Vst::MediaType type,
                                     Vst::BusDirection dir,
                                     int32 busIndex,
                                     int32 channel,
                                     Vst::UnitID& unitId) override
    {
        if (type == Vst::kAudio && dir == Vst::kInput && busIndex == 0)
        {
            unitId = Vst::kRootUnitId;
            return kResultOk;
        }

        if (type == Vst::kAudio && dir == Vst::kOutput && busIndex == 0)
        {
            unitId = Vst::kRootUnitId;
            return kResultOk;
        }

        return kResultFalse;
    }

    tresult PLUGIN_API setUnitProgramData (int32 listOrUnitId, int32 programIndex, IBStream* data) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setUnitProgramData not implemented: listOrUnitId=" << String (listOrUnitId) << ", program=" << String (programIndex));

        return kNotImplemented;
    }

    //==============================================================================

    tresult PLUGIN_API getProgramListInfo (int32 listIndex, Vst::ProgramListInfo& info) override
    {
        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramListInfo failed: processor is null");
            return kInternalError;
        }

        if (listIndex == 0 && hasProgramList())
        {
            toString128 ("Default Programs", info.name);
            info.id = programListID;
            info.programCount = static_cast<int32> (processor->getNumPresets());
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramListInfo returned " << String (static_cast<int> (info.programCount)) << " programs");
            return kResultOk;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramListInfo failed: list out of range, listIndex=" << String (listIndex));
        return kNotImplemented;
    }

    int32 PLUGIN_API getProgramListCount() override
    {
        return hasProgramList() ? 1 : 0;
    }

    tresult PLUGIN_API getProgramName (Vst::ProgramListID listId,
                                       int32 programIndex,
                                       Vst::String128 name) override
    {
        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramName failed: processor is null");
            return kInternalError;
        }

        if (listId != programListID)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramName failed: unknown listId=" << String (static_cast<int> (listId)));
            return kResultFalse;
        }

        if (isPositiveAndBelow (programIndex, processor->getNumPresets()))
        {
            toString128 (processor->getPresetName (programIndex), name);
            return kResultOk;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramName failed: program out of range, program=" << String (programIndex));
        return kResultFalse;
    }

    tresult PLUGIN_API getProgramInfo (Vst::ProgramListID listId,
                                       int32 programIndex,
                                       Vst::CString attributeId,
                                       Vst::String128 attributeValue) override
    {
        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramInfo failed: processor is null");
            return kInternalError;
        }

        if (listId != programListID)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramInfo failed: unknown listId=" << String (static_cast<int> (listId)));
            return kResultFalse;
        }

        if (std::string_view (attributeId) != Vst::PresetAttributes::kName)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramInfo failed: unsupported attribute=" << String (attributeId));
            return kResultFalse;
        }

        if (isPositiveAndBelow (programIndex, processor->getNumPresets()))
        {
            toString128 (processor->getPresetName (programIndex), attributeValue);
            return kResultOk;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getProgramInfo failed: program out of range, program=" << String (programIndex));
        return kResultFalse;
    }

    tresult PLUGIN_API hasProgramPitchNames (Vst::ProgramListID listId, int32 programIndex) override
    {
        return kResultFalse;
    }

    tresult PLUGIN_API getProgramPitchName (Vst::ProgramListID listId,
                                            int32 programIndex,
                                            int16 midiPitch,
                                            Vst::String128 name) override
    {
        return kResultFalse;
    }

    //==============================================================================

    tresult PLUGIN_API setChannelContextInfos (Vst::IAttributeList* list) override
    {
        if (list == nullptr || processor == nullptr)
            return kInvalidArgument;

        AudioProcessorBase::HostInfo info;

        Vst::String128 name {};
        if (list->getString (Vst::ChannelContext::kChannelNameKey, name, sizeof (name)) == kResultTrue)
            info.trackName = toString (name);

        int64 color = 0;
        if (list->getInt (Vst::ChannelContext::kChannelColorKey, color) == kResultTrue)
            info.trackColor = static_cast<uint32> (color);

        processor->setHostInfo (info);
        return kResultTrue;
    }

    //==============================================================================

    IPlugView* PLUGIN_API createView (FIDString name) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "createView requested: name=" << String (name) << ", processor=" << describePointer (processor));

        if (std::string_view (name) == Vst::ViewType::kEditor)
        {
            windowing.ensureInitialised();
            return new AudioPluginEditorViewVST3 (processor, this);
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "createView failed: unsupported view type");
        return nullptr;
    }

private:
    void setupParameters()
    {
        removeParameterListeners();
        parameters.removeAll();

        if (processor == nullptr)
            return;

        bypass.attach (*processor);

        for (size_t parameterIndex = 0; parameterIndex < processor->getParameters().size(); ++parameterIndex)
        {
            const auto parameter = processor->getParameters()[parameterIndex];
            auto flags = (parameter->isAutomatable() && ! parameter->isReadOnly())
                           ? Vst::ParameterInfo::kCanAutomate
                           : 0;

            if (parameter->isBypass())
                flags |= Vst::ParameterInfo::kIsBypass;

            const auto unitShortName = String (AudioParameter::getParameterUnitShortName (parameter->getUnit(), parameter->getUnitName()));

            parameters.addParameter (
                toTChar (parameter->getName().toUTF16()),
                unitShortName.isEmpty() ? nullptr : toTChar (unitShortName.toUTF16()),
                parameter->getNumSteps(),        // step count
                parameter->getNormalizedValue(), // normalized value
                flags,                           // flags
                getVST3ParameterID (parameter),  // tag
                Vst::kRootUnitId,                // unit
                nullptr);                        // short title

            parameter->addListener (this);
            listenedParameters.push_back (parameter);
        }

        addProgramParameter();

        if (bypass.isProcessorOwned())
            return;

        // VST3 bypass parameter (always the last parameter)
        parameters.addParameter (
            STR16 ("Bypass"),
            nullptr,
            1, // step count 1 = toggle
            0, // default: not bypassed
            Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
            static_cast<Vst::ParamID> (bypass.getHostParameterID()),
            Vst::kRootUnitId,
            nullptr);
    }

    // Exposes the processor presets as the VST3 program list, selected by a program change parameter
    void addProgramParameter()
    {
        programParameterID = Vst::kNoParamId;

        if (! hasProgramList())
            return;

        programParameterID = static_cast<Vst::ParamID> (findFirstUnusedHostParameterID (*processor, bypass.getHostParameterID() + 1));

        auto* programParameter = new Vst::StringListParameter (STR16 ("Program"),
                                                              programParameterID,
                                                              nullptr,
                                                              Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsList | Vst::ParameterInfo::kIsProgramChange,
                                                              Vst::kRootUnitId);

        for (int index = 0; index < processor->getNumPresets(); ++index)
            programParameter->appendString (toTChar (processor->getPresetName (index).toUTF16()));

        programParameter->setNormalized (programParameter->toNormalized (jmax (0, processor->getCurrentPreset())));
        parameters.addParameter (programParameter);
    }

    bool hasProgramList() const
    {
        return processor != nullptr && processor->getNumPresets() > 1;
    }

    bool isProgramParameter (Vst::ParamID tag) const noexcept
    {
        return programParameterID != Vst::kNoParamId && tag == programParameterID;
    }

    void syncProgramParameterToController()
    {
        if (! isProgramParameter (programParameterID) || processor == nullptr)
            return;

        const auto value = Vst::EditController::plainParamToNormalized (programParameterID, jmax (0, processor->getCurrentPreset()));
        if (value != Vst::EditController::getParamNormalized (programParameterID))
            Vst::EditController::setParamNormalized (programParameterID, value);
    }

    void removeParameterListeners()
    {
        for (auto& parameter : listenedParameters)
            parameter->removeListener (this);

        listenedParameters.clear();
    }

    void addProcessorListener()
    {
        if (processor != nullptr)
            processor->addListener (this);
    }

    void removeProcessorListener()
    {
        if (processor != nullptr)
            processor->removeListener (this);
    }

    void parameterValueChanged (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        if (! isValidProcessorParameterIndex (indexInContainer))
            return;

        const auto tag = getVST3ParameterID (parameter);
        const auto normalizedValue = static_cast<Vst::ParamValue> (parameter->getNormalizedValue());

        if (parameter->isReadOnly())
            return;

        Vst::EditController::setParamNormalized (tag, normalizedValue);

        if (parameter->isAutomatable())
            Vst::EditController::performEdit (tag, normalizedValue);
    }

    void parameterGestureBegin (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        if (isValidProcessorParameterIndex (indexInContainer)
            && parameter->isAutomatable()
            && ! parameter->isReadOnly())
        {
            Vst::EditController::beginEdit (getVST3ParameterID (processor->getParameters()[indexInContainer]));
        }
    }

    void parameterGestureEnd (const AudioParameter::Ptr& parameter, int indexInContainer) override
    {
        if (isValidProcessorParameterIndex (indexInContainer)
            && parameter->isAutomatable()
            && ! parameter->isReadOnly())
        {
            Vst::EditController::endEdit (getVST3ParameterID (processor->getParameters()[indexInContainer]));
        }
    }

    void syncProcessorParametersToController()
    {
        if (processor == nullptr)
            return;

        for (const auto& parameter : processor->getParameters())
        {
            Vst::EditController::setParamNormalized (
                getVST3ParameterID (parameter),
                static_cast<Vst::ParamValue> (parameter->getNormalizedValue()));
        }

        syncProgramParameterToController();
    }

    void audioProcessorChanged (AudioProcessorBase* processor, const AudioProcessor::ChangeDetails& details) override
    {
        ignoreUnused (processor);

        int32 restartFlags = 0;

        if (details.latencyChanged)
            restartFlags |= Vst::kLatencyChanged;

        if (details.parameterValuesChanged)
            restartFlags |= Vst::kParamValuesChanged;

        if (details.parameterInfoChanged)
            restartFlags |= Vst::kParamValuesChanged
                          | Vst::kParamTitlesChanged
                          | Vst::kMidiCCAssignmentChanged;

        if (details.programChanged)
        {
            syncProgramParameterToController();
            restartFlags |= Vst::kParamValuesChanged;
        }

        if (restartFlags != 0)
            if (auto* handler = getComponentHandler())
                handler->restartComponent (restartFlags);

        if (details.nonParameterStateChanged)
            setDirty (true);
    }

    bool isValidProcessorParameterIndex (int indexInContainer) const
    {
        return processor != nullptr
            && isPositiveAndBelow (indexInContainer, static_cast<int> (processor->getParameters().size()));
    }

    WrapperWindowing windowing;
    AudioProcessor* processor = nullptr;
    WrapperBypass bypass;
    std::vector<AudioParameter::Ptr> listenedParameters;
    Vst::ParamID programParameterID = Vst::kNoParamId;
    static constexpr Vst::ProgramListID programListID = 1;
#if YUP_LINUX
    Linux::IRunLoop* hostRunLoop = nullptr;
#endif
};

//==============================================================================

class AudioPluginProcessorVST3 : public Vst::AudioEffect
{
public:
    //==============================================================================

    AudioPluginProcessorVST3()
    {
        processor.reset (yup::createPluginProcessorForWrapper (yup::AudioPluginWrapperType::vst3, ::YUP_AUDIO_PLUGIN_CREATE_FUNCTION));

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "created processor instance: component=" << describePointer (this) << ", processor=" << describePointer (processor.get()));

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "createPluginProcessor returned null");
        }
        else
        {
            bypass.attach (*processor);
        }

        setControllerClass (YupPlugin_Controller_UID);
    }

    virtual ~AudioPluginProcessorVST3()
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "destroying processor instance: component=" << describePointer (this) << ", processor=" << describePointer (processor.get()));

        endActiveParameterGestures (processor.get());
        processor.reset();
    }

    //==============================================================================

    AudioProcessor* getProcessor() const noexcept { return processor.get(); }

    //==============================================================================

    static FUnknown* createInstance ([[maybe_unused]] void* context)
    {
        auto* instance = new AudioPluginProcessorVST3();
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "created component instance: component=" << describePointer (instance));
        return (Vst::IAudioProcessor*) instance;
    }

    //==============================================================================

    tresult PLUGIN_API initialize (FUnknown* context) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component initialize requested: context=" << describePointer (context));

        auto result = AudioEffect::initialize (context);
        updatePluginHostTypeFromContext (context);

        if (result != kResultOk || processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component initialize failed: base result=" << static_cast<int> (result) << ", processor=" << describePointer (processor.get()));
            return result;
        }

        for (const auto& inputBus : processor->getBusLayout().getInputBuses())
        {
            const auto nameUTF16 = inputBus.getName().toUTF16();

            if (inputBus.getType() == AudioBus::Type::Audio)
            {
                const auto busType = inputBus.getRole() == AudioBus::Role::Auxiliary
                                       ? Steinberg::Vst::kAux
                                       : Steinberg::Vst::kMain;
                const auto busFlags = inputBus.isDefaultActive()
                                        ? Steinberg::Vst::BusInfo::kDefaultActive
                                        : 0;

                addAudioInput (toTChar (nameUTF16), speakerArrForChannelSet (inputBus.getChannelSet()), busType, busFlags);
            }
            else if (inputBus.getType() == AudioBus::Type::Midi)
            {
                addEventInput (toTChar (nameUTF16));
            }
        }

        for (const auto& outputBus : processor->getBusLayout().getOutputBuses())
        {
            const auto nameUTF16 = outputBus.getName().toUTF16();

            if (outputBus.getType() == AudioBus::Type::Audio)
            {
                const auto busType = outputBus.getRole() == AudioBus::Role::Auxiliary
                                       ? Steinberg::Vst::kAux
                                       : Steinberg::Vst::kMain;
                const auto busFlags = outputBus.isDefaultActive()
                                        ? Steinberg::Vst::BusInfo::kDefaultActive
                                        : 0;

                addAudioOutput (toTChar (nameUTF16), speakerArrForChannelSet (outputBus.getChannelSet()), busType, busFlags);
            }
            else if (outputBus.getType() == AudioBus::Type::Midi)
            {
                addEventOutput (toTChar (nameUTF16));
            }
        }

        // Fallback: synths without an explicit MIDI input bus always get one
#if YupPlugin_IsSynth
        if (getBusCount (Vst::kEvent, Vst::kInput) == 0)
            addEventInput (STR16 ("Midi In"));
#endif

        updateChannelOrders();

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component initialize completed: audioInputs=" << String (static_cast<int> (getBusCount (Vst::kAudio, Vst::kInput))) << ", audioOutputs=" << String (static_cast<int> (getBusCount (Vst::kAudio, Vst::kOutput))) << ", eventInputs=" << String (static_cast<int> (getBusCount (Vst::kEvent, Vst::kInput))) << ", eventOutputs=" << String (static_cast<int> (getBusCount (Vst::kEvent, Vst::kOutput))));

        return kResultOk;
    }

    tresult PLUGIN_API terminate() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component terminate requested: component=" << describePointer (this) << ", processor=" << describePointer (processor.get()));

        if (processor != nullptr)
        {
            endActiveParameterGestures (processor.get());
            processor->releaseResources();
        }

        return AudioEffect::terminate();
    }

    //==============================================================================

    tresult PLUGIN_API connect (Vst::IConnectionPoint* other) override
    {
        auto result = AudioEffect::connect (other);

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component connect: other=" << describePointer (other) << ", result=" << static_cast<int> (result) << ", processor=" << describePointer (processor.get()));

        if (IPtr<Vst::IMessage> message = owned (allocateMessage()))
        {
            message->setMessageID ("processor");

            if (auto attributes = message->getAttributes())
            {
                void* ptrValue = static_cast<void*> (processor.get());
                attributes->setBinary ("data", std::addressof (ptrValue), sizeof (ptrValue));
            }

            sendMessage (message);
        }

        return result;
    }

    tresult PLUGIN_API disconnect (Vst::IConnectionPoint* other) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component disconnect requested: other=" << describePointer (other));

        return AudioEffect::disconnect (other);
    }

    tresult PLUGIN_API notify (Vst::IMessage* message) override
    {
        return AudioEffect::notify (message);
    }

    //==============================================================================

    tresult PLUGIN_API setBusArrangements (
        Vst::SpeakerArrangement* inputs,
        int32 numIns,
        Vst::SpeakerArrangement* outputs,
        int32 numOuts) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setBusArrangements requested: numIns=" << String (numIns) << ", numOuts=" << String (numOuts));

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setBusArrangements failed: processor is null");
            return kResultFalse;
        }

        const auto& busLayout = processor->getBusLayout();

        if (numIns != busLayout.getNumAudioBuses (true) || numOuts != busLayout.getNumAudioBuses (false))
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setBusArrangements rejected: bus count mismatch, expectedIns=" << String (busLayout.getNumAudioBuses (true)) << ", expectedOuts=" << String (busLayout.getNumAudioBuses (false)));
            return kResultFalse;
        }

        auto requestedLayout = busLayout;

        for (int32 idx = 0; idx < numIns; ++idx)
            requestedLayout = requestedLayout.withAudioBusChannelSet (idx, true, channelSetForSpeakerArr (inputs[idx]));

        for (int32 idx = 0; idx < numOuts; ++idx)
            requestedLayout = requestedLayout.withAudioBusChannelSet (idx, false, channelSetForSpeakerArr (outputs[idx]));

        if (requestedLayout != busLayout)
        {
            if (isActive)
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setBusArrangements rejected: component is active");
                return kResultFalse;
            }

            if (const auto result = processor->setBusLayout (requestedLayout); result.failed())
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setBusArrangements rejected: " << result.getErrorMessage());
                return kResultFalse;
            }
        }

        for (int32 idx = 0; idx < numIns; ++idx)
            if (auto* bus = FCast<Vst::AudioBus> (audioInputs.at (static_cast<size_t> (idx))))
                bus->setArrangement (inputs[idx]);

        for (int32 idx = 0; idx < numOuts; ++idx)
            if (auto* bus = FCast<Vst::AudioBus> (audioOutputs.at (static_cast<size_t> (idx))))
                bus->setArrangement (outputs[idx]);

        updateChannelOrders();

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setBusArrangements accepted");

        return kResultOk;
    }

    //==============================================================================

    tresult PLUGIN_API setActive (TBool state) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setActive requested: state=" << String (state ? "true" : "false") << ", sampleRate=" << String (processSetup.sampleRate) << ", maxSamplesPerBlock=" << String (static_cast<int> (processSetup.maxSamplesPerBlock)) << ", processor=" << describePointer (processor.get()));

        std::optional<ScopedLock> activationGuard;
        if (HostQuirks::forCurrentHost().processMayOverlapActivation)
            activationGuard.emplace (activationLock);

        if (processor != nullptr)
        {
            if (state)
                processor->setPlaybackConfiguration (processSetup.sampleRate, processSetup.maxSamplesPerBlock);
            else
                processor->releaseResources();
        }

        isActive = state != 0;
        activeNoteIds.clear();
        noteExpressions.reset();

        return AudioEffect::setActive (state);
    }

    tresult PLUGIN_API getBusArrangement (Vst::BusDirection dir, int32 index, Vst::SpeakerArrangement& arr) override
    {
        const auto result = AudioEffect::getBusArrangement (dir, index, arr);

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "getBusArrangement: direction=" << String (dir == Vst::kInput ? "input" : "output") << ", index=" << String (index) << ", channels=" << String (result == kResultOk ? Vst::SpeakerArr::getChannelCount (arr) : 0) << ", result=" << static_cast<int> (result));

        return result;
    }

    tresult PLUGIN_API activateBus (Vst::MediaType type, Vst::BusDirection dir, int32 index, TBool state) override
    {
        const auto result = AudioEffect::activateBus (type, dir, index, state);

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "activateBus: type=" << String (type == Vst::kAudio ? "audio" : "event") << ", direction=" << String (dir == Vst::kInput ? "input" : "output") << ", index=" << String (index) << ", state=" << String (state ? "true" : "false") << ", result=" << static_cast<int> (result));

        return result;
    }

    //==============================================================================

    tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) override
    {
        if (symbolicSampleSize == Vst::kSample32)
            return kResultTrue;

        if (symbolicSampleSize == Vst::kSample64
            && processor != nullptr
            && processor->supportsDoublePrecisionProcessing())
        {
            return kResultTrue;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "canProcessSampleSize rejected: symbolicSampleSize=" << String (symbolicSampleSize));
        return kResultFalse;
    }

    uint32 PLUGIN_API getLatencySamples() override
    {
        return processor != nullptr ? static_cast<uint32> (jmax (0, processor->getLatencySamples()))
                                    : 0u;
    }

    uint32 PLUGIN_API getTailSamples() override
    {
        if (processor == nullptr)
            return Vst::kNoTail;

        const auto tailSamples = processor->getTailSamples();
        return tailSamples > 0 ? static_cast<uint32> (tailSamples) : Vst::kNoTail;
    }

    tresult PLUGIN_API setProcessing (TBool state) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setProcessing requested: state=" << String (state ? "true" : "false"));

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setProcessing failed: processor is null");
            return kResultFalse;
        }

        processor->suspendProcessing (! state);

        if (! state)
            processor->flush();

        return kResultOk;
    }

    //==============================================================================

    tresult PLUGIN_API getState (IBStream* stream) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component getState requested");

        if (processor == nullptr || stream == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component getState failed: processor=" << describePointer (processor.get()) << ", stream=" << describePointer (stream));
            return kResultFalse;
        }

        MemoryBlock processorState;
        const auto saveResult = processor->saveStateIntoMemory (processorState);
        const auto hasProcessorState = saveResult.wasOk();

        if (! hasProcessorState)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component getState without processor state: " << saveResult.getErrorMessage());
        }

        auto data = writeWrapperBypassState (vst3WrapperStateMagic,
                                             vst3WrapperStateVersion,
                                             bypass.isBypassed(),
                                             processorState,
                                             hasProcessorState);

        const auto ok = writeVST3StreamData (*stream, data);

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component getState " << String (ok ? "completed" : "failed") << ": bytes=" << String (static_cast<int64> (data.getSize())) << ", processorBytes=" << String (static_cast<int64> (processorState.getSize())));

        return ok ? kResultOk : kResultFalse;
    }

    tresult PLUGIN_API setState (IBStream* stream) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component setState requested");

        if (processor == nullptr || stream == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component setState failed: processor=" << describePointer (processor.get()) << ", stream=" << describePointer (stream));
            return kResultFalse;
        }

        const auto data = readVST3StreamData (*stream);
        if (HostQuirks::forCurrentHost().isForeignStateChunk (data))
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component setState ignored: host state chunk");
            return kResultFalse;
        }

        if (data.isEmpty())
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component setState failed: stream is empty");
            return kResultFalse;
        }

        const auto wrapperState = readWrapperBypassState (data, vst3WrapperStateMagic, vst3WrapperStateVersion);
        if (! wrapperState.hasWrapperState)
        {
            const auto result = processor->loadStateFromMemory (wrapperState.processorState);
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component setState " << String (result.wasOk() ? "completed" : "failed") << " without wrapper state: bytes=" << String (static_cast<int64> (wrapperState.processorState.getSize())) << (result.wasOk() ? String() : ", error=" + result.getErrorMessage()));
            return result.wasOk() ? kResultOk : kResultFalse;
        }

        if (! wrapperState.hasProcessorState)
        {
            bypass.setBypassed (wrapperState.isBypassed);
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component setState completed without processor state: bypassed=" << String (wrapperState.isBypassed ? "true" : "false"));
            return kResultOk;
        }

        const auto result = processor->loadStateFromMemory (wrapperState.processorState);
        bypass.setBypassed (wrapperState.isBypassed);
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "component setState " << String (result.wasOk() ? "completed" : "failed") << ": bytes=" << String (static_cast<int64> (wrapperState.processorState.getSize())) << ", bypassed=" << String (wrapperState.isBypassed ? "true" : "false") << (result.wasOk() ? String() : ", error=" + result.getErrorMessage()));
        return result.wasOk() ? kResultOk : kResultFalse;
    }

    //==============================================================================

    tresult PLUGIN_API setupProcessing (Vst::ProcessSetup& setup) override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setupProcessing requested: sampleRate=" << String (setup.sampleRate) << ", maxSamplesPerBlock=" << String (static_cast<int> (setup.maxSamplesPerBlock)) << ", processMode=" << String (static_cast<int> (setup.processMode)) << ", offline=" << String (setup.processMode == Vst::kOffline ? "true" : "false") << ", symbolicSampleSize=" << String (static_cast<int> (setup.symbolicSampleSize)));

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setupProcessing failed: processor is null");
            return kResultFalse;
        }

        processSetup = setup;

        processor->setPlaybackConfiguration (setup.sampleRate, setup.maxSamplesPerBlock);
        processor->setOfflineProcessing (setup.processMode == Vst::kOffline);

        midiBuffer.ensureSize (4096);
        midiBuffer.clear();

        noteExpressions.prepare (*processor);
        activeNoteIds.reserve (maxActiveNoteIds);

        paramChangeBuffer.reserve (getDefaultParameterChangeCapacity (*processor));
        const auto totalOutputChannels = getTotalAudioOutputChannels (*processor);
        outputChannelsFloat.reserve (static_cast<size_t> (totalOutputChannels));
        outputChannelsDouble.reserve (static_cast<size_t> (totalOutputChannels));

        // Pre-allocate per-bus view storage
        {
            const auto numAudioInputs = static_cast<size_t> (processor->getNumAudioInputs());
            const auto numAudioOutputs = static_cast<size_t> (processor->getNumAudioOutputs());

            inputBusViewsFloat.reserve (numAudioInputs);
            outputBusViewsFloat.reserve (numAudioOutputs);
            inputBusViewsDouble.reserve (numAudioInputs);
            outputBusViewsDouble.reserve (numAudioOutputs);
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_VST3, "setupProcessing completed: outputChannels=" << String (totalOutputChannels) << ", audioInputs=" << String (processor->getNumAudioInputs()) << ", audioOutputs=" << String (processor->getNumAudioOutputs()));

        return kResultOk;
    }

    //==============================================================================

    tresult PLUGIN_API process (Vst::ProcessData& data) override
    {
        std::optional<ScopedLock> activationGuard;
        if (HostQuirks::forCurrentHost().processMayOverlapActivation)
            activationGuard.emplace (activationLock);

        if (! isActive)
            return kResultFalse;

        if (data.processContext != nullptr)
        {
            processContext = *data.processContext;
            processor->setOfflineProcessing ((processContext.state & Vst::kOfflineProcessing) != 0);
        }

        const int32 lastSampleOffset = jmax (0, data.numSamples - 1);

        // --- Process Parameters ---
        bool bypassed = bypass.isBypassed();
        paramChangeBuffer.clear();

        if (data.inputParameterChanges)
        {
            const auto bypassTag = static_cast<Vst::ParamID> (bypass.getHostParameterID());

            const int32 numParams = data.inputParameterChanges->getParameterCount();
            for (int32 i = 0; i < numParams; ++i)
            {
                Vst::IParamValueQueue* queue = data.inputParameterChanges->getParameterData (i);
                if (queue == nullptr)
                    continue;

                const int32 numPoints = queue->getPointCount();
                if (numPoints <= 0)
                    continue;

                const auto tag = queue->getParameterId();

                if (tag == bypassTag)
                {
                    int32 sampleOffset;
                    Vst::ParamValue value;
                    if (queue->getPoint (numPoints - 1, sampleOffset, value) == kResultOk)
                    {
                        bypassed = (value >= 0.5);
                        bypass.setBypassed (bypassed);
                    }
                }
                else
                {
                    const auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag));
                    if (parameter == nullptr
                        || parameter->isReadOnly()
                        || parameter->isPerformingChangeGesture())
                    {
                        continue;
                    }

                    for (int32 p = 0; p < numPoints; ++p)
                    {
                        int32 sampleOffset;
                        Vst::ParamValue value;
                        if (queue->getPoint (p, sampleOffset, value) == kResultOk)
                            addParameterChangeByHostParameterID (*processor,
                                                                 paramChangeBuffer,
                                                                 static_cast<uint32> (tag),
                                                                 static_cast<float> (value),
                                                                 jlimit (0, lastSampleOffset, sampleOffset));
                    }
                }
            }

            paramChangeBuffer.sort();
            applyParameterChangesToProcessor (*processor, paramChangeBuffer);
        }

        // --- Process Events ---
        midiBuffer.clear();
        noteExpressions.clear();

        if (data.inputEvents)
        {
            int32 numEvents = data.inputEvents->getEventCount();
            for (int32 i = 0; i < numEvents; ++i)
            {
                Vst::Event e;
                if (data.inputEvents->getEvent (i, e) != kResultOk)
                    continue;

                switch (e.type)
                {
                    case Vst::Event::kNoteOnEvent:
                        trackNoteId (e.noteOn.noteId, e.noteOn.channel, e.noteOn.pitch);
                        midiBuffer.addEvent (MidiMessage::noteOn (e.noteOn.channel + 1,
                                                                  e.noteOn.pitch,
                                                                  static_cast<uint8> (e.noteOn.velocity * 127.0f)),
                                             jlimit (0, lastSampleOffset, e.sampleOffset));
                        break;

                    case Vst::Event::kNoteOffEvent:
                        releaseNoteId (e.noteOff.noteId);
                        midiBuffer.addEvent (MidiMessage::noteOff (e.noteOff.channel + 1,
                                                                   e.noteOff.pitch,
                                                                   static_cast<uint8> (e.noteOff.velocity * 127.0f)),
                                             jlimit (0, lastSampleOffset, e.sampleOffset));
                        break;

                    case Vst::Event::kPolyPressureEvent:
                        midiBuffer.addEvent (MidiMessage::aftertouchChange (e.polyPressure.channel + 1,
                                                                            e.polyPressure.pitch,
                                                                            static_cast<int> (e.polyPressure.pressure * 127.0f)),
                                             jlimit (0, lastSampleOffset, e.sampleOffset));
                        break;

                    case Vst::Event::kLegacyMIDICCOutEvent:
                        midiBuffer.addEvent (MidiMessage::controllerEvent (e.midiCCOut.channel + 1,
                                                                           e.midiCCOut.controlNumber,
                                                                           e.midiCCOut.value),
                                             jlimit (0, lastSampleOffset, e.sampleOffset));
                        break;

                    case Vst::Event::kDataEvent:
                        if (e.data.type == Vst::DataEvent::kMidiSysEx)
                            midiBuffer.addEvent (e.data.bytes, static_cast<int> (e.data.size), jlimit (0, lastSampleOffset, e.sampleOffset));
                        break;

                    case Vst::Event::kNoteExpressionValueEvent:
                        addNoteExpression (e.noteExpressionValue, jlimit (0, lastSampleOffset, e.sampleOffset));
                        break;

                    default:
                        break;
                }
            }
        }

        const auto* noteExpressionBuffer = noteExpressions.apply (midiBuffer);

        // --- Process Audio ---
        if (data.numSamples > 0 && data.outputs != nullptr)
        {
            const bool useDoublePrecision = processSetup.symbolicSampleSize == Vst::kSample64
                                         && processor->supportsDoublePrecisionProcessing();

            // Copy main input audio into matching main output buffers for effects.
            // Auxiliary (sidechain) inputs are NOT copied to outputs.
            if (data.inputs != nullptr)
            {
                for (int32 busIdx = 0; busIdx < std::min (data.numInputs, data.numOutputs); ++busIdx)
                {
                    auto& inBus = data.inputs[busIdx];
                    auto& outBus = data.outputs[busIdx];

                    // Only copy when the bus role is Main (skip Auxiliary/sidechain)
                    if (! isVST3AudioBusMain (busIdx, true))
                        continue;

                    for (int32 ch = 0; ch < std::min (inBus.numChannels, outBus.numChannels); ++ch)
                    {
                        if (useDoublePrecision)
                        {
                            auto* in = reinterpret_cast<const double*> (inBus.channelBuffers64[ch]);
                            auto* out = reinterpret_cast<double*> (outBus.channelBuffers64[ch]);
                            if (in != out)
                                std::memcpy (out, in, static_cast<size_t> (data.numSamples) * sizeof (double));
                        }
                        else
                        {
                            auto* in = reinterpret_cast<const float*> (inBus.channelBuffers32[ch]);
                            auto* out = reinterpret_cast<float*> (outBus.channelBuffers32[ch]);
                            if (in != out)
                                std::memcpy (out, in, static_cast<size_t> (data.numSamples) * sizeof (float));
                        }
                    }
                }
            }

            AudioPluginPlayHeadVST3 playHead (data.processContext);
            auto* const playHeadPtr = data.processContext != nullptr ? &playHead : nullptr;

            if (useDoublePrecision)
            {
                // Build output channel pointers (flat buffer, backward compat)
                outputChannelsDouble.clear();
                for (int32 busIdx = 0; busIdx < data.numOutputs; ++busIdx)
                    for (int32 ch = 0; ch < data.outputs[busIdx].numChannels; ++ch)
                        outputChannelsDouble.push_back (reinterpret_cast<double*> (data.outputs[busIdx].channelBuffers64[mapOutputChannel (data, busIdx, ch)]));

                AudioBuffer<double> audioBuffer (outputChannelsDouble.data(),
                                                 static_cast<int> (outputChannelsDouble.size()),
                                                 0,
                                                 data.numSamples);

                // Build per-bus input views (all input buses)
                inputBusViewsDouble.clear();
                for (int32 busIdx = 0; busIdx < data.numInputs; ++busIdx)
                    inputBusViewsDouble.push_back (buildVST3InputBusView<double> (data, busIdx));

                // Build per-bus output views (all output buses)
                outputBusViewsDouble.clear();
                for (int32 busIdx = 0; busIdx < data.numOutputs; ++busIdx)
                    outputBusViewsDouble.push_back (buildVST3OutputBusView<double> (data, busIdx));

                AudioProcessContext<double> doubleCtx {
                    audioBuffer,
                    midiBuffer,
                    paramChangeBuffer,
                    playHeadPtr,
                    { inputBusViewsDouble.data(), static_cast<size_t> (inputBusViewsDouble.size()) },
                    { outputBusViewsDouble.data(), static_cast<size_t> (outputBusViewsDouble.size()) }
                };
                doubleCtx.noteExpressions = noteExpressionBuffer;

                processAudioBlock (*processor, doubleCtx, bypassed);
            }
            else
            {
                // Build output channel pointers (flat buffer, backward compat)
                outputChannelsFloat.clear();
                for (int32 busIdx = 0; busIdx < data.numOutputs; ++busIdx)
                    for (int32 ch = 0; ch < data.outputs[busIdx].numChannels; ++ch)
                        outputChannelsFloat.push_back (reinterpret_cast<float*> (data.outputs[busIdx].channelBuffers32[mapOutputChannel (data, busIdx, ch)]));

                AudioSampleBuffer audioBuffer (outputChannelsFloat.data(),
                                               static_cast<int> (outputChannelsFloat.size()),
                                               0,
                                               data.numSamples);

                // Build per-bus input views (all input buses)
                inputBusViewsFloat.clear();
                for (int32 busIdx = 0; busIdx < data.numInputs; ++busIdx)
                    inputBusViewsFloat.push_back (buildVST3InputBusView<float> (data, busIdx));

                // Build per-bus output views (all output buses)
                outputBusViewsFloat.clear();
                for (int32 busIdx = 0; busIdx < data.numOutputs; ++busIdx)
                    outputBusViewsFloat.push_back (buildVST3OutputBusView<float> (data, busIdx));

                AudioProcessContext<float> context {
                    audioBuffer,
                    midiBuffer,
                    paramChangeBuffer,
                    playHeadPtr,
                    { inputBusViewsFloat.data(), static_cast<size_t> (inputBusViewsFloat.size()) },
                    { outputBusViewsFloat.data(), static_cast<size_t> (outputBusViewsFloat.size()) }
                };
                context.noteExpressions = noteExpressionBuffer;

                processAudioBlock (*processor, context, bypassed);
            }
        }

        if (data.outputEvents != nullptr)
            writeMidiBufferToVST3EventList (midiBuffer, *data.outputEvents, data.numSamples);

        return kResultOk;
    }

private:
    ScopedYupInitialiser_GUI scopeInitialiser;

    std::unique_ptr<AudioProcessor> processor;

    Vst::ProcessContext processContext;
    Vst::ProcessSetup processSetup;

    MidiBuffer midiBuffer;
    ParameterChangeBuffer paramChangeBuffer;
    std::vector<float*> outputChannelsFloat;
    std::vector<double*> outputChannelsDouble;

    std::vector<AudioBusBufferView<const float>> inputBusViewsFloat;
    std::vector<AudioBusBufferView<float>> outputBusViewsFloat;
    std::vector<AudioBusBufferView<const double>> inputBusViewsDouble;
    std::vector<AudioBusBufferView<double>> outputBusViewsDouble;

    /** The processor-to-VST3 channel order of an audio bus, with scratch storage for remapped pointers. */
    struct BusChannelOrder
    {
        std::vector<int> order;
        std::vector<void*> pointers;
    };

    std::vector<BusChannelOrder> inputChannelOrders;
    std::vector<BusChannelOrder> outputChannelOrders;

    WrapperBypass bypass;
    std::atomic<bool> isActive { false };
    CriticalSection activationLock;

    struct ActiveNoteId
    {
        int32 noteId;
        int16 channel;
        int16 pitch;
    };

    static constexpr size_t maxActiveNoteIds = 256;
    std::vector<ActiveNoteId> activeNoteIds;
    WrapperNoteExpressions noteExpressions;

    //==============================================================================
    void trackNoteId (int32 noteId, int16 channel, int16 pitch) noexcept
    {
        if (noteId == -1)
            return;

        releaseNoteId (noteId);

        if (activeNoteIds.size() < activeNoteIds.capacity())
            activeNoteIds.push_back ({ noteId, channel, pitch });
    }

    void releaseNoteId (int32 noteId) noexcept
    {
        if (noteId == -1)
            return;

        activeNoteIds.erase (std::remove_if (activeNoteIds.begin(), activeNoteIds.end(), [noteId] (const ActiveNoteId& note)
        {
            return note.noteId == noteId;
        }),
                             activeNoteIds.end());
    }

    void addNoteExpression (const Vst::NoteExpressionValueEvent& event, int sampleOffset) noexcept
    {
        const auto* type = findVST3NoteExpressionType (*processor, event.typeId);
        if (type == nullptr)
            return;

        const auto note = std::find_if (activeNoteIds.begin(), activeNoteIds.end(), [&] (const ActiveNoteId& active)
        {
            return active.noteId == event.noteId;
        });

        if (note == activeNoteIds.end())
            return;

        NoteExpression expression;
        expression.sampleOffset = sampleOffset;
        expression.noteId = event.noteId;
        expression.channel = static_cast<int8> (note->channel + 1);
        expression.key = static_cast<int8> (note->pitch);
        expression.type = type->type;
        expression.value = vst3NoteExpressionToPlain (type->type, event.value);
        noteExpressions.add (expression);
    }

    //==============================================================================
    /** Looks up the role of a VST3 audio bus by its audio-bus index.
        Returns true when the bus role is Main. */
    bool isVST3AudioBusMain (int32 vst3AudioBusIndex, bool isInput) const noexcept
    {
        return processor->getBusLayout().getAudioBusRole (vst3AudioBusIndex, isInput) == AudioBus::Role::Main;
    }

    void updateChannelOrders()
    {
        const auto update = [this] (Vst::BusList& buses, bool isInput, std::vector<BusChannelOrder>& orders)
        {
            orders.clear();
            orders.resize (buses.size());

            for (size_t idx = 0; idx < buses.size(); ++idx)
            {
                if (auto* bus = FCast<Vst::AudioBus> (buses.at (idx)))
                {
                    const auto set = processor->getBusLayout().getAudioBusChannelSet (static_cast<int> (idx), isInput);
                    orders[idx].order = channelOrderForSpeakerArr (set, bus->getArrangement());
                    orders[idx].pointers.resize (orders[idx].order.size());
                }
            }
        };

        update (audioInputs, true, inputChannelOrders);
        update (audioOutputs, false, outputChannelOrders);
    }

    /** Returns the channel pointers in processor order, reordering them when the VST3 order differs. */
    template <typename SampleType>
    SampleType** remapChannels (SampleType** channels, int32 numChannels, std::vector<BusChannelOrder>& orders, int32 busIdx) noexcept
    {
        if (channels == nullptr || ! isPositiveAndBelow (busIdx, static_cast<int32> (orders.size())))
            return channels;

        auto& bus = orders[static_cast<size_t> (busIdx)];
        if (bus.order.empty() || static_cast<int32> (bus.order.size()) != numChannels)
            return channels;

        for (size_t ch = 0; ch < bus.order.size(); ++ch)
            bus.pointers[ch] = channels[bus.order[ch]];

        return reinterpret_cast<SampleType**> (bus.pointers.data());
    }

    int32 mapOutputChannel (const Vst::ProcessData& data, int32 busIdx, int32 channel) const noexcept
    {
        if (! isPositiveAndBelow (busIdx, static_cast<int32> (outputChannelOrders.size())))
            return channel;

        const auto& order = outputChannelOrders[static_cast<size_t> (busIdx)].order;
        if (static_cast<int32> (order.size()) != data.outputs[busIdx].numChannels)
            return channel;

        return order[static_cast<size_t> (channel)];
    }

    /** Builds an AudioBusBufferView for a VST3 input bus. */
    template <typename FloatType>
    AudioBusBufferView<const FloatType> buildVST3InputBusView (const Vst::ProcessData& data, int32 busIdx)
    {
        const auto& inBus = data.inputs[busIdx];

        if constexpr (std::is_same_v<FloatType, double>)
        {
            return { reinterpret_cast<const FloatType* const*> (remapChannels (inBus.channelBuffers64, inBus.numChannels, inputChannelOrders, busIdx)),
                     static_cast<int> (inBus.numChannels),
                     processor->getBusLayout().getAudioBusRole (busIdx, true) };
        }
        else
        {
            // Silence flag: when the host reports silence, provide null pointers so
            // the processor can detect inactive buses via getReadPointer returning null.
            const bool isSilent = (inBus.silenceFlags != 0);
            if (isSilent)
            {
                // All channels are silent — return a view with null data pointers
                return { nullptr, static_cast<int> (inBus.numChannels), processor->getBusLayout().getAudioBusRole (busIdx, true) };
            }

            return { reinterpret_cast<const FloatType* const*> (remapChannels (inBus.channelBuffers32, inBus.numChannels, inputChannelOrders, busIdx)),
                     static_cast<int> (inBus.numChannels),
                     processor->getBusLayout().getAudioBusRole (busIdx, true) };
        }
    }

    /** Builds an AudioBusBufferView for a VST3 output bus. */
    template <typename FloatType>
    AudioBusBufferView<FloatType> buildVST3OutputBusView (const Vst::ProcessData& data, int32 busIdx)
    {
        const auto& outBus = data.outputs[busIdx];

        if constexpr (std::is_same_v<FloatType, double>)
        {
            return { reinterpret_cast<FloatType* const*> (remapChannels (outBus.channelBuffers64, outBus.numChannels, outputChannelOrders, busIdx)),
                     static_cast<int> (outBus.numChannels),
                     processor->getBusLayout().getAudioBusRole (busIdx, false) };
        }
        else
        {
            return { reinterpret_cast<FloatType* const*> (remapChannels (outBus.channelBuffers32, outBus.numChannels, outputChannelOrders, busIdx)),
                     static_cast<int> (outBus.numChannels),
                     processor->getBusLayout().getAudioBusRole (busIdx, false) };
        }
    }
};

#ifdef YupPlugin_VST3_Categories
constexpr auto YupPlugin_Category = YupPlugin_VST3_Categories;
#elif YupPlugin_IsSynth
constexpr auto YupPlugin_Category = Vst::PlugType::kInstrumentSynth;
#else
constexpr auto YupPlugin_Category = Vst::PlugType::kFx;
#endif

} // namespace yup

//==============================================================================

BEGIN_FACTORY_DEF (
    YupPlugin_Vendor,
    YupPlugin_URL,
    "mailto:" YupPlugin_Email)

DEF_CLASS2 (
    INLINE_UID_FROM_FUID (yup::YupPlugin_Processor_UID),
    PClassInfo::kManyInstances, // Supports multiple instances
    kVstAudioEffectClass,       // Component category (do not change this)
    YupPlugin_Name,             // Plugin name
    0,                          // Not distributable: controller shares the processor instance
    yup::YupPlugin_Category,    // Subcategory
    YupPlugin_Version,          // Plugin version
    kVstVersionString,          // The VST 3 SDK version (do not change this, always use this define)
    yup::AudioPluginProcessorVST3::createInstance)

DEF_CLASS2 (
    INLINE_UID_FROM_FUID (yup::YupPlugin_Controller_UID),
    PClassInfo::kManyInstances,   // Supports multiple instances
    kVstComponentControllerClass, // Controller category (do not change this)
    YupPlugin_Name "Controller",  // Controller name (can be the same as the component name)
    0,                            // Not used here
    "",                           // Not used here
    YupPlugin_Version,            // Plug-in version
    kVstVersionString,            // The VST 3 SDK version (do not change this, always use this define)
    yup::AudioPluginControllerVST3::createInstance)

END_FACTORY
