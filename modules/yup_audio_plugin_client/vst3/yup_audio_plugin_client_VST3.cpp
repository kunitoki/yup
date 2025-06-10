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

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_VST3)
#error "YUP_AUDIO_PLUGIN_ENABLE_VST3 must be defined"
#endif

#include <public.sdk/source/vst/vstaudioeffect.h>
#include <public.sdk/source/vst/vsteditcontroller.h>
#include <public.sdk/source/main/pluginfactory.h>
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
#include <string_view>

//==============================================================================

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

using namespace Steinberg;

namespace
{

//==============================================================================

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

//==============================================================================

static std::atomic_int numScopedInitInstancesGui = 0;

struct VST3ScopedYupInitialiser
{
    VST3ScopedYupInitialiser()
    {
        if (numScopedInitInstancesGui.fetch_add (1) == 0)
        {
            initialiseYup_GUI();
            initialiseYup_Windowing();
        }
    }

    ~VST3ScopedYupInitialiser()
    {
        if (numScopedInitInstancesGui.fetch_add (-1) == 1)
        {
            shutdownYup_Windowing();
            shutdownYup_GUI();
        }
    }
};

} // namespace

//==============================================================================

static const auto YupPlugin_Processor_UID = toFUID (YupPlugin_Id);
static const auto YupPlugin_Controller_UID = toFUID (YupPlugin_Id ".controller");

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
            setBounds ({ static_cast<float> (size->left),
                         static_cast<float> (size->top),
                         static_cast<float> (size->getWidth()),
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
            viewRect.left = getX();
            viewRect.top = getY();
            viewRect.right = viewRect.left + getWidth();
            viewRect.bottom = viewRect.top + getHeight();

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

            setBounds ({ static_cast<float> (rect.left),
                         static_cast<float> (rect.top),
                         static_cast<float> (rect.getWidth()),
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
            size->left = getX();
            size->top = getY();
            size->right = size->left + getWidth();
            size->bottom = size->top + getHeight();
        }
        else
        {
            const auto preferredSize = editor->getPreferredSize();
            size->left = getX();
            size->top = getY();
            size->right = size->left + preferredSize.getWidth();
            size->bottom = size->top + preferredSize.getHeight();
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
        return Vst::EditController::terminate();
    }

    //==============================================================================

    tresult PLUGIN_API connect (Vst::IConnectionPoint* other) override
    {
        return kResultTrue;
    }

    tresult PLUGIN_API disconnect (Vst::IConnectionPoint* other) override
    {
        processor = nullptr;
        return kResultTrue;
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
        return kResultFalse;
    }

    tresult PLUGIN_API getState (IBStream* state) override
    {
        return kResultFalse;
    }

    //==============================================================================

    int32 PLUGIN_API getParameterCount() override
    {
        if (processor == nullptr)
            return 0;

        return static_cast<int32> (processor->getParameters().size());
    }

    tresult PLUGIN_API getParameterInfo (int32 paramIndex, Vst::ParameterInfo& info) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (! isPositiveAndBelow (paramIndex, getParameterCount()))
            return kInvalidArgument;

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

        if (! isPositiveAndBelow (static_cast<int32> (tag), getParameterCount()))
            return kInvalidArgument;

        if (auto parameter = processor->getParameters()[tag])
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

        if (! isPositiveAndBelow (static_cast<int32> (tag), getParameterCount()))
            return kInvalidArgument;

        if (auto parameter = processor->getParameters()[tag])
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

        if (! isPositiveAndBelow (static_cast<int32> (tag), getParameterCount()))
            return valueNormalized;

        if (auto parameter = processor->getParameters()[tag])
            return parameter->convertToDenormalizedValue (valueNormalized);

        return valueNormalized;
    }

    Vst::ParamValue PLUGIN_API plainParamToNormalized (Vst::ParamID tag, Vst::ParamValue plainValue) override
    {
        if (processor == nullptr)
            return plainValue;

        if (! isPositiveAndBelow (static_cast<int32> (tag), getParameterCount()))
            return plainValue;

        if (auto parameter = processor->getParameters()[tag])
            return parameter->convertToNormalizedValue (plainValue);

        return plainValue;
    }

    Vst::ParamValue PLUGIN_API getParamNormalized (Vst::ParamID tag) override
    {
        if (processor == nullptr)
            return 0.0;

        if (! isPositiveAndBelow (static_cast<int32> (tag), getParameterCount()))
            return 0.0;

        if (auto parameter = processor->getParameters()[tag])
            return parameter->getNormalizedValue();

        return 0.0;
    }

    tresult PLUGIN_API setParamNormalized (Vst::ParamID tag, Vst::ParamValue value) override
    {
        if (processor == nullptr)
            return kInternalError;

        if (! isPositiveAndBelow (static_cast<int32> (tag), getParameterCount()))
            return kInvalidArgument;

        if (auto parameter = processor->getParameters()[tag])
        {
            parameter->setNormalizedValue (value);
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

        const auto numParams = static_cast<int> (processor->getParameters().size());
        if (oldParamID >= 0 && oldParamID < numParams)
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
        return kNotImplemented;
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
        if (processor == nullptr)
            return;

        for (size_t parameterIndex = 0; parameterIndex < processor->getParameters().size(); ++parameterIndex)
        {
            const auto parameter = processor->getParameters()[parameterIndex];

            parameters.addParameter (
                reinterpret_cast<const Vst::TChar*> (parameter->getName().toUTF16().getAddress()),
                nullptr,                           // units
                0,                                 // step count
                parameter->getNormalizedValue(),   // normalized value
                Vst::ParameterInfo::kCanAutomate,  // flags (Vst::ParameterInfo::kNoFlags)
                static_cast<int> (parameterIndex), // tag
                Vst::kRootUnitId,                  // unit
                nullptr);                          // short title
        }
    }

    AudioProcessor* processor = nullptr;
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
            addAudioInput (toTChar (nameUTF16), Vst::SpeakerArr::kStereo);
        }

        for (const auto& outputBus : processor->getBusLayout().getOutputBuses())
        {
            const auto nameUTF16 = outputBus.getName().toUTF16();
            addAudioOutput (toTChar (nameUTF16), Vst::SpeakerArr::kStereo);
        }

#if YupPlugin_IsSynth
        addEventInput (STR16 ("Midi In"));
#endif

        return kResultOk;
    }

    tresult PLUGIN_API terminate() override
    {
        if (processor != nullptr)
            processor->releaseResources();

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

        // TODO - check compatibility of bus arrangement

        if (numIns == 1
            && numOuts == 1
            && inputs[0] == Vst::SpeakerArr::kStereo
            && outputs[0] == Vst::SpeakerArr::kStereo)
        {
            return kResultOk;
        }

        return kResultFalse;
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

    tresult PLUGIN_API setupProcessing (Vst::ProcessSetup& setup) override
    {
        if (processor == nullptr)
            return kResultFalse;

        processSetup = setup;

        processor->setPlaybackConfiguration (setup.sampleRate, setup.maxSamplesPerBlock);

        /*
        if (setup.processMode != Vst::kOffline)
            processor->setIsRealtime (true);
        else
            processor->setIsRealtime (false);
        */

        midiBuffer.ensureSize (4096);
        midiBuffer.clear();

        return kResultOk;
    }

    //==============================================================================

    tresult PLUGIN_API process (Vst::ProcessData& data) override
    {
        if (data.processContext != nullptr)
            processContext = *data.processContext;

        // --- Process Parameters ---
        if (data.inputParameterChanges)
        {
            int32 numParams = data.inputParameterChanges->getParameterCount();
            for (int32 i = 0; i < numParams; i++)
            {
                Vst::IParamValueQueue* queue = data.inputParameterChanges->getParameterData (i);
                if (queue == nullptr)
                    continue;

                int32 numPoints = queue->getPointCount();
                if (numPoints <= 0)
                    continue;

                int32 sampleOffset;
                Vst::ParamValue value;
                if (queue->getPoint (numPoints - 1, sampleOffset, value) == kResultOk)
                    processor->getParameters()[i]->setNormalizedValue (static_cast<float> (value));
            }
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
                        midiBuffer.addEvent (MidiMessage::noteOn (e.noteOn.channel + 1, e.noteOn.pitch, e.noteOn.velocity), e.sampleOffset);
                        break;

                    case Vst::Event::kNoteOffEvent:
                        midiBuffer.addEvent (MidiMessage::noteOff (e.noteOff.channel + 1, e.noteOff.pitch, e.noteOff.velocity), e.sampleOffset);
                        break;

                    case Vst::Event::kPolyPressureEvent:
                        // handle poly pressure if needed
                        break;

                    case Vst::Event::kDataEvent:
                        // optional: handle MIDI SysEx or other custom events
                        break;

                    case Vst::Event::kLegacyMIDICCOutEvent:
                        // handle legacy CC output
                        break;

                    default:
                        break;
                }
            }
        }

        // --- Process Audio ---
        if (data.numSamples > 0 && data.outputs != nullptr)
        {
            Vst::AudioBusBuffers& outBus = data.outputs[0];

            AudioSampleBuffer audioBuffer (
                reinterpret_cast<float**> (outBus.channelBuffers32),
                outBus.numChannels,
                data.numSamples);

            processor->processBlock (audioBuffer, midiBuffer);
        }

        return kResultOk;
    }

private:
    VST3ScopedYupInitialiser scopeInitialiser;

    std::unique_ptr<AudioProcessor> processor;

    Vst::ProcessContext processContext;
    Vst::ProcessSetup processSetup;

    MidiBuffer midiBuffer;
};

#if YupPlugin_IsSynth
const auto YupPlugin_Category = Vst::PlugType::kInstrument;
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
    yup::YupPlugin_Category,    // Subcategory (effect)
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
