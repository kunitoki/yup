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
#include <pluginterfaces/vst/ivstchannelcontextinfo.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>
#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <pluginterfaces/vst/ivstevents.h>
#include <pluginterfaces/vst/ivstremapparamid.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivstmessage.h>
#include <pluginterfaces/vst/ivstplugview.h>
#include <pluginterfaces/vst/vstpresetkeys.h>

#include <atomic>
#include <limits>
#include <memory>
#include <string_view>
#include <vector>

//==============================================================================

extern "C" yup::AudioProcessor* createPluginProcessor();

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

String toString (Vst::TChar* source)
{
    return CharPointer_UTF16 (reinterpret_cast<CharPointer_UTF16::CharType*> (source));
}

//==============================================================================

void toString128 (const String& source, Vst::String128 destination)
{
    if (source.isEmpty())
    {
        destination[0] = 0;
        return;
    }

    // Convert UTF-8 to UTF-16
    CharPointer_UTF16 utf16 (source.toUTF16());
    const size_t length = std::min (static_cast<size_t> (sizeof (Vst::String128) - 1), utf16.length());
    std::memcpy (destination, utf16.getAddress(), length * sizeof (Vst::TChar));
    destination[length] = 0;
}

Vst::ParamID getVST3ParameterID (const AudioParameter::Ptr& parameter)
{
    return static_cast<Vst::ParamID> (parameter->getHostParameterID());
}

Vst::ParamID getVST3BypassParameterID (const AudioProcessor& processor)
{
    return static_cast<Vst::ParamID> (getBypassHostParameterID (processor));
}

constexpr int vst3WrapperStateMagic = 0x33535659; // "YVS3"
constexpr int vst3WrapperStateVersion = 1;

struct VST3WrapperState
{
    bool hasWrapperState = false;
    bool isBypassed = false;
    bool hasProcessorState = false;
    MemoryBlock processorState;
};

MemoryBlock readVST3StreamData (IBStream& stream)
{
    MemoryBlock data;
    char buffer[4096];
    int32 bytesRead = 0;

    while (stream.read (buffer, sizeof (buffer), &bytesRead) == kResultOk && bytesRead > 0)
        data.append (buffer, static_cast<size_t> (bytesRead));

    return data;
}

void writeVST3WrapperState (MemoryBlock& data,
                            bool isBypassed,
                            const MemoryBlock& processorState,
                            bool hasProcessorState)
{
    MemoryOutputStream output (data, false);
    output.writeInt (vst3WrapperStateMagic);
    output.writeInt (vst3WrapperStateVersion);
    output.writeBool (isBypassed);
    output.writeBool (hasProcessorState);
    output.writeInt64 (hasProcessorState ? static_cast<int64> (processorState.getSize()) : 0);

    if (hasProcessorState && ! processorState.isEmpty())
        output.write (processorState.getData(), processorState.getSize());

    output.flush();
}

VST3WrapperState readVST3WrapperState (const MemoryBlock& data)
{
    VST3WrapperState result;

    MemoryInputStream input (data, false);

    if (input.readInt() != vst3WrapperStateMagic
        || input.readInt() != vst3WrapperStateVersion)
    {
        result.processorState = data;
        return result;
    }

    result.hasWrapperState = true;
    result.isBypassed = input.readBool();
    result.hasProcessorState = input.readBool();

    const auto processorStateSize = input.readInt64();
    if (processorStateSize < 0
        || processorStateSize > input.getNumBytesRemaining()
        || processorStateSize > static_cast<int64> (std::numeric_limits<int>::max()))
    {
        result.hasWrapperState = false;
        result.hasProcessorState = false;
        result.processorState = data;
        return result;
    }

    result.processorState.setSize (static_cast<size_t> (processorStateSize));

    const auto bytesToRead = static_cast<int> (processorStateSize);
    if (bytesToRead > 0
        && input.read (result.processorState.getData(), bytesToRead) != bytesToRead)
    {
        result.hasWrapperState = false;
        result.hasProcessorState = false;
        result.processorState = data;
    }

    return result;
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

static Vst::SpeakerArrangement speakerArrForChannels (int channels)
{
    switch (channels)
    {
        case 1:
            return Vst::SpeakerArr::kMono;
        case 6:
            return Vst::SpeakerArr::k51;
        case 8:
            return Vst::SpeakerArr::k71CineFullFront;
        default:
            return Vst::SpeakerArr::kStereo;
    }
}

//==============================================================================

class AudioPluginEditorViewVST3
    : public Component
    , public Vst::EditorView
{
public:
    AudioPluginEditorViewVST3 (AudioProcessor* processor, Vst::EditController* controller, ViewRect* size = nullptr)
        : Vst::EditorView (controller, size)
        , processor (processor)
    {
        jassert (processor != nullptr);
        if (processor == nullptr || ! processor->hasEditor())
            return;

        editor.reset (processor->createEditor());
        if (editor == nullptr)
            return;

        addAndMakeVisible (editor.get());

        if (size != nullptr)
        {
            setSize ({ static_cast<float> (size->getWidth()),
                       static_cast<float> (size->getHeight()) });
        }
        else
        {
            const auto preferredSize = editor->getPreferredSize();
            setSize ({ static_cast<float> (preferredSize.getWidth()),
                       static_cast<float> (preferredSize.getHeight()) });
        }
    }

    ~AudioPluginEditorViewVST3()
    {
        if (editor != nullptr)
        {
            endActiveParameterGestures (processor);
            setVisible (false);
            removeFromDesktop();

            removeChildComponent (editor.get());
            editor.reset();
        }
    }

    void resized() override
    {
        editor->setBounds (getLocalBounds());

        if (plugFrame != nullptr && ! hostTriggeredResizing)
        {
            ViewRect viewRect;
            viewRect.left = 0;
            viewRect.top = 0;
            viewRect.right = getWidth();
            viewRect.bottom = getHeight();

            plugFrame->resizeView (this, std::addressof (viewRect));
        }
    }

    tresult PLUGIN_API attached (void* parent, FIDString type) override
    {
        auto result = Vst::EditorView::attached (parent, type);
        if (result != kResultOk)
            return result;

        if (editor == nullptr)
            return kInternalError;

        ComponentNative::Flags flags = ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow;

        if (editor->shouldRenderContinuous())
            flags.set (ComponentNative::renderContinuous);

        auto options = ComponentNative::Options()
                           .withFlags (flags)
                           .withResizableWindow (editor->isResizable());

        addToDesktop (options, parent);
        setVisible (true);

        editor->attachedToNative();

        return kResultTrue;
    }

    tresult PLUGIN_API removed() override
    {
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
        if (editor != nullptr && editor->isResizable())
            return kResultTrue;

        return kResultFalse;
    }

    tresult PLUGIN_API checkSizeConstraint (ViewRect* rect) override
    {
        if (editor == nullptr)
            return kInternalError;

        // TODO

        return kResultTrue;
    }

    tresult PLUGIN_API onSize (ViewRect* newSize) override
    {
        if (editor == nullptr)
            return kInternalError;

        if (newSize != nullptr)
        {
            const auto preferredSize = editor->getPreferredSize();

            if (! editor->isResizable())
            {
                newSize->right = newSize->left + preferredSize.getWidth();
                newSize->bottom = newSize->top + preferredSize.getHeight();
            }
            else if (editor->shouldPreserveAspectRatio())
            {
                const auto width = newSize->getWidth();
                const auto height = newSize->getHeight();

                if (preferredSize.getWidth() > preferredSize.getHeight())
                    newSize->bottom = newSize->top + static_cast<int32> (width * (preferredSize.getHeight() / static_cast<float> (preferredSize.getWidth())));
                else
                    newSize->right = newSize->left + static_cast<int32> (height * (preferredSize.getWidth() / static_cast<float> (preferredSize.getHeight())));
            }

            rect = *newSize;

            const auto scoped = ScopedValueSetter<bool> (hostTriggeredResizing, true);

            setSize ({ static_cast<float> (rect.getWidth()),
                       static_cast<float> (rect.getHeight()) });
        }

        return kResultTrue;
    }

    tresult PLUGIN_API getSize (ViewRect* size) override
    {
        if (editor == nullptr)
            return kInternalError;

        if (size == nullptr)
            return kInvalidArgument;

        if (editor->isResizable() && editor->getWidth() != 0 && editor->getHeight() != 0)
        {
            size->left = 0;
            size->top = 0;
            size->right = getWidth();
            size->bottom = getHeight();
        }
        else
        {
            const auto preferredSize = editor->getPreferredSize();
            size->left = 0;
            size->top = 0;
            size->right = preferredSize.getWidth();
            size->bottom = preferredSize.getHeight();
        }

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

    tresult PLUGIN_API isPlatformTypeSupported (FIDString type) override
    {
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
    ScopedYupInitialiser_Windowing scopeInitialiser;
    AudioProcessor* processor = nullptr;
    std::unique_ptr<AudioProcessorEditor> editor;
    bool hostTriggeredResizing = false;
};

//==============================================================================

class AudioPluginControllerVST3
    : public Vst::EditController
    , public Vst::IMidiMapping
    , public Vst::IUnitInfo
    , public Vst::IRemapParamID
    , public Vst::ChannelContext::IInfoListener
    , private AudioParameter::Listener
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
    DEF_INTERFACE (Vst::IRemapParamID)
    DEF_INTERFACE (Vst::ChannelContext::IInfoListener)
    END_DEFINE_INTERFACES (Vst::EditController)

    //==============================================================================

    static FUnknown* createInstance ([[maybe_unused]] void* context)
    {
        return (Vst::IEditController*) new AudioPluginControllerVST3;
    }

    //==============================================================================

    AudioPluginControllerVST3()
    {
    }

    ~AudioPluginControllerVST3()
    {
        removeParameterListeners();
    }

    //==============================================================================

    tresult PLUGIN_API initialize (FUnknown* context) override
    {
        auto result = Vst::EditController::initialize (context);
        if (result != kResultOk)
            return result;

        return kResultOk;
    }

    tresult PLUGIN_API terminate() override
    {
        removeParameterListeners();
        processor = nullptr;

        return Vst::EditController::terminate();
    }

    //==============================================================================

    tresult PLUGIN_API connect (Vst::IConnectionPoint* other) override
    {
        return Vst::EditController::connect (other);
    }

    tresult PLUGIN_API disconnect (Vst::IConnectionPoint* other) override
    {
        removeParameterListeners();
        processor = nullptr;
        return Vst::EditController::disconnect (other);
    }

    tresult PLUGIN_API notify (Vst::IMessage* message) override
    {
        if (message == nullptr)
            return kInvalidArgument;

        auto msgID = message->getMessageID();
        if (std::string_view (msgID) != "processor")
            return kResultFalse;

        if (auto attributes = message->getAttributes())
        {
            const void* msgData;
            uint32 msgSize;

            auto result = attributes->getBinary ("data", msgData, msgSize);
            if (result == kResultTrue && msgSize == sizeof (void*))
            {
                processor = static_cast<AudioProcessor*> (*reinterpret_cast<void* const*> (msgData));

                setupParameters();

                return result;
            }
        }

        return kResultFalse;
    }

    //==============================================================================

    tresult PLUGIN_API setState (IBStream* state) override
    {
        if (state == nullptr)
            return kInvalidArgument;

        return kResultOk;
    }

    tresult PLUGIN_API getState (IBStream* state) override
    {
        if (state == nullptr)
            return kInvalidArgument;

        return kResultOk;
    }

    tresult PLUGIN_API setComponentState (IBStream* state) override
    {
        if (processor == nullptr || state == nullptr)
            return kResultFalse;

        const auto wrapperState = readVST3WrapperState (readVST3StreamData (*state));
        if (! wrapperState.hasWrapperState)
        {
            syncProcessorParametersToController();
            return kResultOk;
        }

        Vst::EditController::setParamNormalized (getVST3BypassParameterID (*processor),
                                                 wrapperState.isBypassed ? 1.0 : 0.0);

        syncProcessorParametersToController();

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

        const auto bypassParameterID = getVST3BypassParameterID (*processor);

        if (tag == bypassParameterID)
        {
            // Bypass parameter
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

        const auto bypassParameterID = getVST3BypassParameterID (*processor);

        if (tag == bypassParameterID)
        {
            // Bypass parameter
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

        if (tag == getVST3BypassParameterID (*processor))
            return valueNormalized;

        if (auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag)))
            return parameter->convertToDenormalizedValue (valueNormalized);

        return valueNormalized;
    }

    Vst::ParamValue PLUGIN_API plainParamToNormalized (Vst::ParamID tag, Vst::ParamValue plainValue) override
    {
        if (processor == nullptr)
            return plainValue;

        if (tag == getVST3BypassParameterID (*processor))
            return plainValue;

        if (auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag)))
            return parameter->convertToNormalizedValue (plainValue);

        return plainValue;
    }

    Vst::ParamValue PLUGIN_API getParamNormalized (Vst::ParamID tag) override
    {
        if (processor == nullptr)
            return 0.0;

        if (tag == getVST3BypassParameterID (*processor))
            return Vst::EditController::getParamNormalized (tag);

        if (auto parameter = processor->getParameterByHostID (static_cast<uint32> (tag)))
            return parameter->getNormalizedValue();

        return 0.0;
    }

    tresult PLUGIN_API setParamNormalized (Vst::ParamID tag, Vst::ParamValue value) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (tag == getVST3BypassParameterID (*processor))
            return Vst::EditController::setParamNormalized (tag, value);

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
            || oldParamID == getVST3BypassParameterID (*processor))
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
        if (processor == nullptr)
            return kResultFalse;

        const auto parameters = processor->getParameters();
        if (midiControllerNumber < static_cast<int32> (parameters.size()))
        {
            id = getVST3ParameterID (parameters[static_cast<int> (midiControllerNumber)]);
            return kResultOk;
        }

        return kResultFalse;
    }

    //==============================================================================

    tresult PLUGIN_API getUnitInfo (int32 unitIndex, Vst::UnitInfo& info) override
    {
        if (unitIndex == 0)
        {
            info.id = Vst::kRootUnitId;
            info.parentUnitId = Vst::kNoParentUnitId;
            info.programListId = Vst::kNoProgramListId;
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
        return kNotImplemented;
    }

    //==============================================================================

    tresult PLUGIN_API getProgramListInfo (int32 listIndex, Vst::ProgramListInfo& info) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (listIndex == 0)
        {
            toString128 ("Default Programs", info.name);
            info.id = 0;
            info.programCount = static_cast<int32> (processor->getNumPresets());
            return kResultOk;
        }

        return kNotImplemented;
    }

    int32 PLUGIN_API getProgramListCount() override
    {
        return 1;
    }

    tresult PLUGIN_API getProgramName (Vst::ProgramListID listId,
                                       int32 programIndex,
                                       Vst::String128 name) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (listId != Vst::kNoProgramListId)
            return kResultFalse;

        if (isPositiveAndBelow (programIndex, processor->getNumPresets()))
        {
            toString128 (processor->getPresetName (programIndex), name);
            return kResultOk;
        }

        return kResultFalse;
    }

    tresult PLUGIN_API getProgramInfo (Vst::ProgramListID listId,
                                       int32 programIndex,
                                       Vst::CString attributeId,
                                       Vst::String128 attributeValue) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (listId != Vst::kNoProgramListId)
            return kResultFalse;

        if (std::string_view (attributeId) != Vst::PresetAttributes::kName)
            return kResultFalse;

        if (isPositiveAndBelow (programIndex, processor->getNumPresets()))
        {
            toString128 (processor->getPresetName (programIndex), attributeValue);
            return kResultOk;
        }

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
        return kNotImplemented;
    }

    //==============================================================================

    IPlugView* PLUGIN_API createView (FIDString name) override
    {
        if (std::string_view (name) == Vst::ViewType::kEditor)
            return new AudioPluginEditorViewVST3 (processor, this);

        return nullptr;
    }

private:
    void setupParameters()
    {
        removeParameterListeners();
        parameters.removeAll();

        if (processor == nullptr)
            return;

        for (size_t parameterIndex = 0; parameterIndex < processor->getParameters().size(); ++parameterIndex)
        {
            const auto parameter = processor->getParameters()[parameterIndex];
            const auto flags = (parameter->isAutomatable() && ! parameter->isReadOnly())
                                 ? Vst::ParameterInfo::kCanAutomate
                                 : 0;

            parameters.addParameter (
                reinterpret_cast<const Vst::TChar*> (parameter->getName().toUTF16().getAddress()),
                nullptr,                         // units
                parameter->getNumSteps(),        // step count
                parameter->getNormalizedValue(), // normalized value
                flags,                           // flags
                getVST3ParameterID (parameter),  // tag
                Vst::kRootUnitId,                // unit
                nullptr);                        // short title

            parameter->addListener (this);
            listenedParameters.push_back (parameter);
        }

        // VST3 bypass parameter (always the last parameter)
        parameters.addParameter (
            STR16 ("Bypass"),
            nullptr,
            1, // step count 1 = toggle
            0, // default: not bypassed
            Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
            getVST3BypassParameterID (*processor),
            Vst::kRootUnitId,
            nullptr);
    }

    void removeParameterListeners()
    {
        for (auto& parameter : listenedParameters)
            parameter->removeListener (this);

        listenedParameters.clear();
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
    }

    bool isValidProcessorParameterIndex (int indexInContainer) const
    {
        return processor != nullptr
            && isPositiveAndBelow (indexInContainer, static_cast<int> (processor->getParameters().size()));
    }

    AudioProcessor* processor = nullptr;
    std::vector<AudioParameter::Ptr> listenedParameters;
};

//==============================================================================

class AudioPluginProcessorVST3 : public Vst::AudioEffect
{
public:
    //==============================================================================

    AudioPluginProcessorVST3()
    {
        processor.reset (::createPluginProcessor());

        setControllerClass (YupPlugin_Controller_UID);
    }

    virtual ~AudioPluginProcessorVST3()
    {
        endActiveParameterGestures (processor.get());
        processor.reset();
    }

    //==============================================================================

    static FUnknown* createInstance ([[maybe_unused]] void* context)
    {
        return (Vst::IAudioProcessor*) new AudioPluginProcessorVST3();
    }

    //==============================================================================

    tresult PLUGIN_API initialize (FUnknown* context) override
    {
        auto result = AudioEffect::initialize (context);
        if (result != kResultOk || processor == nullptr)
            return result;

        for (const auto& inputBus : processor->getBusLayout().getInputBuses())
        {
            const auto nameUTF16 = inputBus.getName().toUTF16();

            if (inputBus.getType() == AudioBus::Type::Audio)
                addAudioInput (toTChar (nameUTF16), speakerArrForChannels (inputBus.getNumChannels()));
            else if (inputBus.getType() == AudioBus::Type::MIDI)
                addEventInput (toTChar (nameUTF16));
        }

        for (const auto& outputBus : processor->getBusLayout().getOutputBuses())
        {
            const auto nameUTF16 = outputBus.getName().toUTF16();

            if (outputBus.getType() == AudioBus::Type::Audio)
                addAudioOutput (toTChar (nameUTF16), speakerArrForChannels (outputBus.getNumChannels()));
            else if (outputBus.getType() == AudioBus::Type::MIDI)
                addEventOutput (toTChar (nameUTF16));
        }

        // Fallback: synths without an explicit MIDI input bus always get one
#if YupPlugin_IsSynth
        if (getBusCount (Vst::kEvent, Vst::kInput) == 0)
            addEventInput (STR16 ("Midi In"));
#endif

        return kResultOk;
    }

    tresult PLUGIN_API terminate() override
    {
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
        if (processor == nullptr)
            return kResultFalse;

        const auto& busLayout = processor->getBusLayout();

        int32 audioInputCount = 0;
        int32 audioOutputCount = 0;

        for (const auto& bus : busLayout.getInputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                ++audioInputCount;

        for (const auto& bus : busLayout.getOutputBuses())
            if (bus.getType() == AudioBus::Type::Audio)
                ++audioOutputCount;

        if (numIns != audioInputCount || numOuts != audioOutputCount)
            return kResultFalse;

        int32 idx = 0;
        for (const auto& bus : busLayout.getInputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;
            if (Vst::SpeakerArr::getChannelCount (inputs[idx]) != bus.getNumChannels())
                return kResultFalse;
            ++idx;
        }

        idx = 0;
        for (const auto& bus : busLayout.getOutputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;
            if (Vst::SpeakerArr::getChannelCount (outputs[idx]) != bus.getNumChannels())
                return kResultFalse;
            ++idx;
        }

        return kResultOk;
    }

    //==============================================================================

    tresult PLUGIN_API setActive (TBool state) override
    {
        if (processor != nullptr)
        {
            if (state)
                processor->setPlaybackConfiguration (processSetup.sampleRate, processSetup.maxSamplesPerBlock);
            else
                processor->releaseResources();
        }

        return AudioEffect::setActive (state);
    }

    //==============================================================================

    tresult PLUGIN_API getState (IBStream* stream) override
    {
        if (processor == nullptr || stream == nullptr)
            return kResultFalse;

        MemoryBlock processorState;
        const auto hasProcessorState = processor->saveStateIntoMemory (processorState).wasOk();

        MemoryBlock data;
        writeVST3WrapperState (data, isBypassed, processorState, hasProcessorState);

        int32 written = 0;
        return stream->write (data.getData(), static_cast<int32> (data.getSize()), &written) == kResultOk
                    && written == static_cast<int32> (data.getSize())
                 ? kResultOk
                 : kResultFalse;
    }

    tresult PLUGIN_API setState (IBStream* stream) override
    {
        if (processor == nullptr || stream == nullptr)
            return kResultFalse;

        const auto data = readVST3StreamData (*stream);
        if (data.isEmpty())
            return kResultFalse;

        const auto wrapperState = readVST3WrapperState (data);
        if (! wrapperState.hasWrapperState)
            return processor->loadStateFromMemory (wrapperState.processorState).wasOk() ? kResultOk : kResultFalse;

        isBypassed = wrapperState.isBypassed;

        if (! wrapperState.hasProcessorState)
            return kResultOk;

        return processor->loadStateFromMemory (wrapperState.processorState).wasOk() ? kResultOk : kResultFalse;
    }

    //==============================================================================

    tresult PLUGIN_API setupProcessing (Vst::ProcessSetup& setup) override
    {
        if (processor == nullptr)
            return kResultFalse;

        processSetup = setup;

        processor->setPlaybackConfiguration (setup.sampleRate, setup.maxSamplesPerBlock);
        processor->setOfflineProcessing (setup.processMode == Vst::kOffline);

        midiBuffer.ensureSize (4096);
        midiBuffer.clear();

        paramChangeBuffer.reserve (static_cast<int> (processor->getParameters().size()) * 4 + 32);

        return kResultOk;
    }

    //==============================================================================

    tresult PLUGIN_API process (Vst::ProcessData& data) override
    {
        if (data.processContext != nullptr)
        {
            processContext = *data.processContext;
            processor->setOfflineProcessing ((processContext.state & Vst::kOfflineProcessing) != 0);
        }

        // --- Process Parameters ---
        bool bypassed = isBypassed;
        paramChangeBuffer.clear();

        if (data.inputParameterChanges)
        {
            const auto bypassTag = getVST3BypassParameterID (*processor);

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
                        isBypassed = bypassed;
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
                                                                 sampleOffset);
                    }
                }
            }

            paramChangeBuffer.sort();
            applyParameterChangesToProcessor (*processor, paramChangeBuffer);
        }

        // --- Process Events ---
        midiBuffer.clear();

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
                        midiBuffer.addEvent (MidiMessage::noteOn (e.noteOn.channel + 1,
                                                                  e.noteOn.pitch,
                                                                  static_cast<uint8> (e.noteOn.velocity * 127.0f)),
                                             e.sampleOffset);
                        break;

                    case Vst::Event::kNoteOffEvent:
                        midiBuffer.addEvent (MidiMessage::noteOff (e.noteOff.channel + 1,
                                                                   e.noteOff.pitch,
                                                                   static_cast<uint8> (e.noteOff.velocity * 127.0f)),
                                             e.sampleOffset);
                        break;

                    case Vst::Event::kPolyPressureEvent:
                        midiBuffer.addEvent (MidiMessage::aftertouchChange (e.polyPressure.channel + 1,
                                                                            e.polyPressure.pitch,
                                                                            static_cast<int> (e.polyPressure.pressure * 127.0f)),
                                             e.sampleOffset);
                        break;

                    case Vst::Event::kLegacyMIDICCOutEvent:
                        midiBuffer.addEvent (MidiMessage::controllerEvent (e.midiCCOut.channel + 1,
                                                                           e.midiCCOut.controlNumber,
                                                                           e.midiCCOut.value),
                                             e.sampleOffset);
                        break;

                    case Vst::Event::kDataEvent:
                        if (e.data.type == Vst::DataEvent::kMidiSysEx)
                            midiBuffer.addEvent (e.data.bytes, static_cast<int> (e.data.size), e.sampleOffset);
                        break;

                    default:
                        break;
                }
            }
        }

        // --- Process Audio ---
        if (data.numSamples > 0 && data.outputs != nullptr)
        {
            // Copy input audio into output buffers for effects
            if (data.inputs != nullptr)
            {
                for (int32 busIdx = 0; busIdx < std::min (data.numInputs, data.numOutputs); ++busIdx)
                {
                    auto& inBus = data.inputs[busIdx];
                    auto& outBus = data.outputs[busIdx];

                    for (int32 ch = 0; ch < std::min (inBus.numChannels, outBus.numChannels); ++ch)
                    {
                        auto* in = reinterpret_cast<const float*> (inBus.channelBuffers32[ch]);
                        auto* out = reinterpret_cast<float*> (outBus.channelBuffers32[ch]);
                        if (in != out)
                            std::memcpy (out, in, static_cast<size_t> (data.numSamples) * sizeof (float));
                    }
                }
            }

            // Build a flat channel pointer array across all output buses
            std::vector<float*> outputChannels;
            for (int32 busIdx = 0; busIdx < data.numOutputs; ++busIdx)
                for (int32 ch = 0; ch < data.outputs[busIdx].numChannels; ++ch)
                    outputChannels.push_back (reinterpret_cast<float*> (data.outputs[busIdx].channelBuffers32[ch]));

            AudioPluginPlayHeadVST3 playHead (data.processContext);
            auto* const playHeadPtr = data.processContext != nullptr ? &playHead : nullptr;

            if (processSetup.symbolicSampleSize == Vst::kSample64 && processor->supportsDoublePrecisionProcessing())
            {
                std::vector<double*> outputChannels64;
                for (int32 busIdx = 0; busIdx < data.numOutputs; ++busIdx)
                    for (int32 ch = 0; ch < data.outputs[busIdx].numChannels; ++ch)
                        outputChannels64.push_back (reinterpret_cast<double*> (data.outputs[busIdx].channelBuffers64[ch]));

                AudioBuffer<double> audioBuffer (outputChannels64.data(),
                                                 static_cast<int> (outputChannels64.size()),
                                                 0,
                                                 data.numSamples);

                AudioProcessContext<double> doubleCtx { audioBuffer, midiBuffer, paramChangeBuffer, playHeadPtr };

                if (bypassed)
                    processor->processBlockBypassed (doubleCtx);
                else
                    processor->processBlock (doubleCtx);
            }
            else
            {
                AudioSampleBuffer audioBuffer (outputChannels.data(),
                                               static_cast<int> (outputChannels.size()),
                                               0,
                                               data.numSamples);

                AudioProcessContext<float> context { audioBuffer, midiBuffer, paramChangeBuffer, playHeadPtr };

                if (bypassed)
                    processor->processBlockBypassed (context);
                else
                    processor->processBlock (context);
            }
        }

        return kResultOk;
    }

private:
    ScopedYupInitialiser_GUI scopeInitialiser;

    std::unique_ptr<AudioProcessor> processor;

    Vst::ProcessContext processContext;
    Vst::ProcessSetup processSetup;

    MidiBuffer midiBuffer;
    ParameterChangeBuffer paramChangeBuffer;
    bool isBypassed = false;
};

#if YupPlugin_IsSynth
const auto YupPlugin_Category = Vst::PlugType::kInstrumentSynth;
#else
const auto YupPlugin_Category = Vst::PlugType::kFx;
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
    Vst::kDistributable,        // Distribution status
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
