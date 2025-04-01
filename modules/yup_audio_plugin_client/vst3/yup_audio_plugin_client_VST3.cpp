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

#include <atomic>

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

//==============================================================================

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

namespace
{

//==============================================================================

Steinberg::FUID stringToFUID (const String& source)
{
    const auto uid = juce::Uuid::fromSHA1 (juce::SHA1 (source.toUTF8()));
    return { uid.getPart (0), uid.getPart (1), uid.getPart (2), uid.getPart (3) };
}

//==============================================================================

void copyStringToVST3 (const String& source, Steinberg::Vst::String128 destination)
{
    if (source.isEmpty())
    {
        destination[0] = 0;
        return;
    }

    // Convert UTF-8 to UTF-16
    CharPointer_UTF16 utf16 (source.toUTF16());
    const size_t length = std::min (static_cast<size_t> (sizeof (Steinberg::Vst::String128) - 1), utf16.length());
    std::memcpy (destination, utf16.getAddress(), length * sizeof (Steinberg::Vst::TChar));
    destination[length] = 0;
}

//==============================================================================

static std::atomic_int numScopedInitInstancesGui = 0;

struct ScopedYupInitialiser_GUI
{
    ScopedYupInitialiser_GUI()
    {
        if (numScopedInitInstancesGui.fetch_add (1) == 0)
        {
            yup::initialiseJuce_GUI();
            yup::initialiseYup_Windowing();
        }
    }

    ~ScopedYupInitialiser_GUI()
    {
        if (numScopedInitInstancesGui.fetch_add (-1) == 1)
        {
            yup::shutdownYup_Windowing();
            yup::shutdownJuce_GUI();
        }
    }
};

} // namespace

//==============================================================================

static const auto YupPlugin_Processor_UID = stringToFUID (YupPlugin_Id);
static const auto YupPlugin_Controller_UID = stringToFUID (YupPlugin_Id ".controller");

//==============================================================================

class AudioPluginEditorViewVST3
    : public Component
    , public Steinberg::Vst::EditorView
{
public:
    AudioPluginEditorViewVST3 (AudioProcessor* processor, Steinberg::Vst::EditController* controller, Steinberg::ViewRect* size = nullptr)
        : Steinberg::Vst::EditorView (controller, size)
        , processor (processor)
    {
        jassert (processor != nullptr);
        if (! processor->hasEditor())
            return;

        editor.reset (processor->createEditor());
        if (editor == nullptr)
            return;

        addAndMakeVisible (editor.get());

        if (size != nullptr)
        {
            setSize ({ static_cast<float> (size->getWidth()), static_cast<float> (size->getHeight()) });
        }
        else
        {
            const auto preferredSize = editor->getPreferredSize();
            setSize ({ static_cast<float> (preferredSize.getWidth()), static_cast<float> (preferredSize.getHeight()) });
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
        if (plugFrame != nullptr)
        {
            Steinberg::ViewRect viewRect;
            viewRect.left = getX();
            viewRect.top = getY();
            viewRect.right = viewRect.left + getWidth();
            viewRect.bottom = viewRect.top + getHeight();

            plugFrame->resizeView (this, std::addressof (viewRect));
        }

        editor->setBounds (getLocalBounds());
    }

    Steinberg::tresult PLUGIN_API attached (void* parent, Steinberg::FIDString type) override
    {
        if (editor == nullptr)
            return Steinberg::kResultFalse;

        ComponentNative::Flags flags = ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow;

        if (editor->shouldRenderContinuous())
            flags.set (ComponentNative::renderContinuous);

        auto options = ComponentNative::Options()
                           .withFlags (flags)
                           .withResizableWindow (editor->isResizable());

        addToDesktop (options, parent);
        setVisible (true);

        editor->attachedToNative();

        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API removed() override
    {
        if (editor != nullptr)
        {
            setVisible (false);
            removeFromDesktop();
        }

        return Steinberg::CPluginView::removed();
    }

    Steinberg::tresult PLUGIN_API canResize() override
    {
        if (editor != nullptr && editor->isResizable())
            return Steinberg::kResultTrue;

        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API checkSizeConstraint (Steinberg::ViewRect* rect) override
    {
        if (editor == nullptr)
            return Steinberg::kResultFalse;

        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API onSize (Steinberg::ViewRect* newSize) override
    {
        if (editor == nullptr || newSize == nullptr)
            return Steinberg::kResultFalse;

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
                newSize->bottom = newSize->top + static_cast<Steinberg::int32> (width * (preferredSize.getHeight() / static_cast<float> (preferredSize.getWidth())));
            else
                newSize->right = newSize->left + static_cast<Steinberg::int32> (height * (preferredSize.getWidth() / static_cast<float> (preferredSize.getHeight())));
        }

        rect = *newSize;

        setBounds ({ static_cast<float> (rect.left),
                     static_cast<float> (rect.top),
                     static_cast<float> (rect.getWidth()),
                     static_cast<float> (rect.getHeight()) });

        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API getSize (Steinberg::ViewRect* size) override
    {
        if (editor == nullptr || size == nullptr)
            return Steinberg::kResultFalse;

        if (editor->isResizable() && editor->getWidth() != 0)
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

        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API onFocus (Steinberg::TBool state) override
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API setFrame (Steinberg::IPlugFrame* frame) override
    {
        plugFrame = frame;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API isPlatformTypeSupported (Steinberg::FIDString type) override
    {
#if JUCE_WINDOWS
        if (std::strcmp (type, Steinberg::kPlatformTypeHWND) == 0)
            return Steinberg::kResultTrue;
#elif JUCE_MAC
        if (std::strcmp (type, Steinberg::kPlatformTypeNSView) == 0)
            return Steinberg::kResultTrue;
        else if (std::strcmp (type, Steinberg::kPlatformTypeHIView) == 0)
            return Steinberg::kResultFalse;
#endif

        return Steinberg::kResultFalse;
    }

private:
    AudioProcessor* processor = nullptr;
    std::unique_ptr<AudioProcessorEditor> editor;
};

//==============================================================================

class AudioPluginControllerVST3
    : public Steinberg::Vst::EditController
    , public Steinberg::Vst::IMidiMapping
    , public Steinberg::Vst::IUnitInfo
    , public Steinberg::Vst::IRemapParamID
    , public Steinberg::Vst::ChannelContext::IInfoListener
{
public:
    OBJ_METHODS (AudioPluginControllerVST3, Steinberg::Vst::EditController)
    REFCOUNT_METHODS (Steinberg::Vst::EditController)

    DEFINE_INTERFACES
    DEF_INTERFACE (Steinberg::Vst::IEditController)
    DEF_INTERFACE (Steinberg::Vst::IMidiMapping)
    DEF_INTERFACE (Steinberg::Vst::IUnitInfo)
    DEF_INTERFACE (Steinberg::Vst::IRemapParamID)
    DEF_INTERFACE (Steinberg::Vst::ChannelContext::IInfoListener)
    END_DEFINE_INTERFACES (Steinberg::Vst::EditController)

    AudioPluginControllerVST3()
    {
    }

    ~AudioPluginControllerVST3()
    {
    }

    static Steinberg::FUnknown* createInstance ([[maybe_unused]] void* context)
    {
        return (Steinberg::Vst::IEditController*) new AudioPluginControllerVST3;
    }

    Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) override
    {
        if (std::strcmp (name, Steinberg::Vst::ViewType::kEditor) == 0)
            return new AudioPluginEditorViewVST3 (processor, this);

        return nullptr;
    }

    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) override
    {
        auto result = Steinberg::Vst::EditController::initialize (context);
        if (result != Steinberg::kResultOk)
            return result;

        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API terminate() override
    {
        return Steinberg::Vst::EditController::terminate();
    }

    Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) override
    {
        if (processor == nullptr)
            return Steinberg::kResultFalse;

        Steinberg::tresult result = EditController::setComponentState (state);
        if (result != Steinberg::kResultOk)
            return result;

        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API getMidiControllerAssignment (Steinberg::int32 busIndex,
                                                               Steinberg::int16 channel,
                                                               Steinberg::Vst::CtrlNumber midiControllerNumber,
                                                               Steinberg::Vst::ParamID& id) override
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getUnitInfo (Steinberg::int32 unitIndex, Steinberg::Vst::UnitInfo& info) override
    {
        if (unitIndex == 0)
        {
            info.id = Steinberg::Vst::kRootUnitId;
            info.parentUnitId = Steinberg::Vst::kNoParentUnitId;
            info.programListId = Steinberg::Vst::kNoProgramListId;
            copyStringToVST3 ("root", info.name);
            return Steinberg::kResultOk;
        }

        return Steinberg::kResultFalse;
    }

    Steinberg::Vst::UnitID PLUGIN_API getSelectedUnit() override
    {
        return {};
    }

    Steinberg::int32 PLUGIN_API getUnitCount() override
    {
        return 1;
    }

    Steinberg::tresult PLUGIN_API selectUnit (Steinberg::Vst::UnitID unitId) override
    {
        if (unitId == Steinberg::Vst::kRootUnitId)
            return Steinberg::kResultOk;

        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getUnitByBus (Steinberg::Vst::MediaType type,
                                                Steinberg::Vst::BusDirection dir,
                                                Steinberg::int32 busIndex,
                                                Steinberg::int32 channel,
                                                Steinberg::Vst::UnitID& unitId) override
    {
        if (type == Steinberg::Vst::kAudio && dir == Steinberg::Vst::kInput && busIndex == 0)
        {
            unitId = Steinberg::Vst::kRootUnitId;
            return Steinberg::kResultOk;
        }

        if (type == Steinberg::Vst::kAudio && dir == Steinberg::Vst::kOutput && busIndex == 0)
        {
            unitId = Steinberg::Vst::kRootUnitId;
            return Steinberg::kResultOk;
        }

        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API setUnitProgramData (Steinberg::int32 listOrUnitId, Steinberg::int32 programIndex, Steinberg::IBStream* data) override
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getProgramListInfo (Steinberg::int32 listIndex, Steinberg::Vst::ProgramListInfo& info) override
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::int32 PLUGIN_API getProgramListCount() override
    {
        return 1;
    }

    Steinberg::tresult PLUGIN_API getProgramName (Steinberg::Vst::ProgramListID listId,
                                                  Steinberg::int32 programIndex,
                                                  Steinberg::Vst::String128 name) override
    {
        if (listId == Steinberg::Vst::kNoProgramListId && programIndex == 0)
        {
            copyStringToVST3 ("Default", name);
            return Steinberg::kResultOk;
        }

        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getProgramInfo (Steinberg::Vst::ProgramListID listId,
                                                  Steinberg::int32 programIndex,
                                                  Steinberg::Vst::CString attributeId,
                                                  Steinberg::Vst::String128 attributeValue) override
    {
        if (listId == Steinberg::Vst::kNoProgramListId && programIndex == 0)
        {
            if (std::strcmp (attributeId, Steinberg::Vst::PresetAttributes::kName) == 0)
            {
                copyStringToVST3 ("Default", attributeValue);
                return Steinberg::kResultOk;
            }
        }

        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API hasProgramPitchNames (Steinberg::Vst::ProgramListID listId, Steinberg::int32 programIndex) override
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getProgramPitchName (Steinberg::Vst::ProgramListID listId,
                                                       Steinberg::int32 programIndex,
                                                       Steinberg::int16 midiPitch,
                                                       Steinberg::Vst::String128 name) override
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API setChannelContextInfos (Steinberg::Vst::IAttributeList* list) override
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getCompatibleParamID (const Steinberg::TUID pluginToReplaceUID,
                                                        Steinberg::Vst::ParamID oldParamID,
                                                        Steinberg::Vst::ParamID& newParamID) override
    {
        if (processor == nullptr)
            return Steinberg::kResultFalse;

        const auto numParams = static_cast<int> (processor->getParameters().size());
        if (oldParamID >= 0 && oldParamID < numParams)
        {
            newParamID = oldParamID;
            return Steinberg::kResultOk;
        }

        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API connect (Steinberg::Vst::IConnectionPoint* other) override
    {
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API disconnect (Steinberg::Vst::IConnectionPoint* other) override
    {
        processor = nullptr;
        return Steinberg::kResultTrue;
    }

    Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) override
    {
        if (message == nullptr)
            return Steinberg::kResultFalse;

        auto msgID = message->getMessageID();
        if (std::strcmp (msgID, "processor") != 0)
            return Steinberg::kResultFalse;

        if (auto attributes = message->getAttributes())
        {
            const void* msgData;
            uint32 msgSize;

            auto result = attributes->getBinary ("data", msgData, msgSize);
            if (result == Steinberg::kResultTrue && msgSize == sizeof (void*))
            {
                void* ptrValue = *reinterpret_cast<void* const*> (msgData);
                processor = static_cast<AudioProcessor*> (ptrValue);

#if 0
                if (processor != nullptr)
                {
                    for (int i = 0; i < processor->getNumParameters(); ++i)
                    {
                        auto& param = processor->getParameter(i);
                        parameters.addParameter(
                            param.getName().toRawUTF8(),
                            nullptr,
                            0,
                            param.getValue(),
                            Steinberg::Vst::ParameterInfo::kCanAutomate,
                            i,
                            Steinberg::Vst::ParameterInfo::kNoFlags,
                            nullptr,
                            Steinberg::Vst::kRootUnitId,
                            nullptr);
                    }
                }
#endif

                return result;
            }
        }

        return Steinberg::kResultFalse;
    }

private:
    AudioProcessor* processor = nullptr;
};

//==============================================================================

class AudioPluginProcessorVST3 : public Steinberg::Vst::AudioEffect
{
public:
    AudioPluginProcessorVST3()
    {
        processor.reset (::createPluginProcessor());

        setControllerClass (YupPlugin_Controller_UID);
    }

    virtual ~AudioPluginProcessorVST3()
    {
        processor.reset();
    }

    static Steinberg::FUnknown* createInstance ([[maybe_unused]] void* context)
    {
        return (Steinberg::Vst::IAudioProcessor*) new AudioPluginProcessorVST3();
    }

    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) override
    {
        auto result = AudioEffect::initialize (context);
        if (result != Steinberg::kResultOk || processor == nullptr)
            return result;

        addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
        addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

#if YupPlugin_IsSynth
        addEventInput (STR16 ("Midi In"));
#endif

        // Add parameter definitions
        for (auto parameter : processor->getParameters())
        {
            /*
            auto& param = processor->getParameter (i);

            parameters.addParameter (
                param.getName().toRawUTF8(),
                nullptr,
                0,
                param.getValue(),
                Steinberg::Vst::ParameterInfo::kCanAutomate | Steinberg::Vst::ParameterInfo::kIsWritable,
                i,
                Steinberg::Vst::ParameterInfo::kNoFlags,
                nullptr,
                Steinberg::Vst::kRootUnitId,
                nullptr);
            */
        }

        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API terminate() override
    {
        if (processor != nullptr)
            processor->releaseResources();

        return AudioEffect::terminate();
    }

    Steinberg::tresult PLUGIN_API setBusArrangements (
        Steinberg::Vst::SpeakerArrangement* inputs,
        int32 numIns,
        Steinberg::Vst::SpeakerArrangement* outputs,
        int32 numOuts) override
    {
        if (numIns == 1
            && numOuts == 1
            && inputs[0] == Steinberg::Vst::SpeakerArr::kStereo
            && outputs[0] == Steinberg::Vst::SpeakerArr::kStereo)
        {
            return Steinberg::kResultOk;
        }

        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) override
    {
        if (state)
        {
            processor->prepareToPlay (processSetup.sampleRate, processSetup.maxSamplesPerBlock);
        }
        else
        {
            processor->releaseResources();
        }

        return AudioEffect::setActive (state);
    }

    Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& setup) override
    {
        if (processor == nullptr)
            return Steinberg::kResultFalse;

        processSetup = setup;

        processor->releaseResources();
        processor->prepareToPlay (setup.sampleRate, setup.maxSamplesPerBlock);

        midiBuffer.ensureSize (4096);
        midiBuffer.clear();

        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) override
    {
        if (data.processContext != nullptr)
            processContext = *data.processContext;

        if (data.inputParameterChanges)
        {
            int32 numParams = data.inputParameterChanges->getParameterCount();
            for (int32 i = 0; i < numParams; i++)
            {
                Steinberg::Vst::IParamValueQueue* queue = data.inputParameterChanges->getParameterData (i);
                if (queue == nullptr)
                    continue;

                int32 numPoints = queue->getPointCount();
                if (numPoints <= 0)
                    continue;

                int32 sampleOffset;
                Steinberg::Vst::ParamValue value;
                if (queue->getPoint (numPoints - 1, sampleOffset, value) == Steinberg::kResultOk)
                    processor->getParameters()[i]->setNormalizedValue (static_cast<float> (value));
            }
        }

        midiBuffer.clear();

        if (data.inputEvents)
        {
            int32 numEvents = data.inputEvents->getEventCount();

            for (int32 i = 0; i < numEvents; ++i)
            {
                Steinberg::Vst::Event e;
                if (data.inputEvents->getEvent (i, e) == Steinberg::kResultOk)
                {
                    switch (e.type)
                    {
                        case Steinberg::Vst::Event::kNoteOnEvent:
                            midiBuffer.addEvent (MidiMessage::noteOn (e.noteOn.channel + 1, e.noteOn.pitch, e.noteOn.velocity), e.sampleOffset);
                            break;

                        case Steinberg::Vst::Event::kNoteOffEvent:
                            midiBuffer.addEvent (MidiMessage::noteOff (e.noteOff.channel + 1, e.noteOff.pitch, e.noteOff.velocity), e.sampleOffset);
                            break;

                        case Steinberg::Vst::Event::kPolyPressureEvent:
                            // handle poly pressure if needed
                            break;

                        case Steinberg::Vst::Event::kDataEvent:
                            // optional: handle MIDI SysEx or other custom events
                            break;

                        case Steinberg::Vst::Event::kLegacyMIDICCOutEvent:
                            // handle legacy CC output
                            break;

                        default:
                            break;
                    }
                }
            }
        }

        // --- Process Audio ---
        if (data.numSamples > 0 && data.inputs && data.outputs)
        {
            Steinberg::Vst::AudioBusBuffers& inBus = data.inputs[0];
            Steinberg::Vst::AudioBusBuffers& outBus = data.outputs[0];

            AudioSampleBuffer audioBuffer (
                reinterpret_cast<float**> (outBus.channelBuffers32),
                outBus.numChannels,
                data.numSamples);

            processor->processBlock (audioBuffer, midiBuffer);
        }

        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API connect (Steinberg::Vst::IConnectionPoint* other) override
    {
        auto result = AudioEffect::connect (other);

        if (Steinberg::IPtr<Steinberg::Vst::IMessage> message = owned (allocateMessage()))
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

    Steinberg::tresult PLUGIN_API disconnect (Steinberg::Vst::IConnectionPoint* other) override
    {
        return AudioEffect::disconnect (other);
    }

    Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) override
    {
        return AudioEffect::notify (message);
    }

private:
    ScopedYupInitialiser_GUI scopeInitialiser;

    std::unique_ptr<AudioProcessor> processor;

    Steinberg::Vst::ProcessContext processContext;
    Steinberg::Vst::ProcessSetup processSetup;

    MidiBuffer midiBuffer;
};

#if YupPlugin_IsSynth
const auto YupPlugin_Category = Steinberg::Vst::PlugType::kInstrument;
#else
const auto YupPlugin_Category = Steinberg::Vst::PlugType::kFx;
#endif

} // namespace yup

//==============================================================================

BEGIN_FACTORY_DEF (
    YupPlugin_Vendor,
    YupPlugin_URL,
    "mailto:" YupPlugin_Email)

DEF_CLASS2 (
    INLINE_UID_FROM_FUID (yup::YupPlugin_Processor_UID),
    Steinberg::PClassInfo::kManyInstances, // Supports multiple instances
    kVstAudioEffectClass,                  // Component category (do not change this)
    YupPlugin_Name,                        // Plugin name
    Steinberg::Vst::kDistributable,        // Distribution status
    yup::YupPlugin_Category,               // Subcategory (effect)
    YupPlugin_Version,                     // Plugin version
    kVstVersionString,                     // The VST 3 SDK version (do not change this, always use this define)
    yup::AudioPluginProcessorVST3::createInstance)

DEF_CLASS2 (
    INLINE_UID_FROM_FUID (yup::YupPlugin_Controller_UID),
    Steinberg::PClassInfo::kManyInstances, // Supports multiple instances
    kVstComponentControllerClass,          // Controller category (do not change this)
    YupPlugin_Name "Controller",           // Controller name (can be the same as the component name)
    0,                                     // Not used here
    "",                                    // Not used here
    YupPlugin_Version,                     // Plug-in version
    kVstVersionString,                     // The VST 3 SDK version (do not change this, always use this define)
    yup::AudioPluginControllerVST3::createInstance)

END_FACTORY
