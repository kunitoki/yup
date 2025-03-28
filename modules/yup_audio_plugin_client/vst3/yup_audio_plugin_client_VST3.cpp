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
#include <pluginterfaces/vst/ivstremapparamid.h>
#include <pluginterfaces/vst/ivstcomponent.h>
#include <pluginterfaces/vst/ivstplugview.h>

//==============================================================================

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

//==============================================================================

class AudioPluginEditorVST3
/*
    : public Steinberg::Vst::EditController
    , public Steinberg::Vst::IMidiMapping
    , public Steinberg::Vst::IUnitInfo
    , public Steinberg::Vst::IRemapParamID
    , public Steinberg::Vst::ChannelContext::IInfoListener
*/
{
public:
    /*
    AudioPluginEditorVST3()
    {
        processor = createPluginProcessor();
        if (processor != nullptr)
        {
#if 0
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
#endif
        }
    }

    ~AudioPluginEditorVST3()
    {
        if (editor != nullptr)
        {
            if (editor->isOnDesktop())
                editor->removeFromDesktop();

            delete editor;
            editor = nullptr;
        }

        if (processor != nullptr)
        {
            delete processor;
            processor = nullptr;
        }
    }

    static Steinberg::FUnknown* createInstance ([[maybe_unused]] void* context)
    {
        return (Steinberg::Vst::IEditController*) new AudioPluginEditorVST3;
    }

    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID iid, void** obj) SMTG_OVERRIDE
    {
        QUERY_INTERFACE (iid, obj, Steinberg::Vst::IEditController::iid, Steinberg::Vst::IEditController)
        QUERY_INTERFACE (iid, obj, Steinberg::Vst::IMidiMapping::iid, Steinberg::Vst::IMidiMapping)
        QUERY_INTERFACE (iid, obj, Steinberg::Vst::IUnitInfo::iid, Steinberg::Vst::IUnitInfo)
        QUERY_INTERFACE (iid, obj, Steinberg::Vst::IRemapParamID::iid, Steinberg::Vst::IRemapParamID)
        QUERY_INTERFACE (iid, obj, Steinberg::Vst::ChannelContext::IInfoListener::iid, Steinberg::Vst::ChannelContext::IInfoListener)
        return Steinberg::kNoInterface;
    }

    Steinberg::uint32 PLUGIN_API addRef() SMTG_OVERRIDE
    {
        return 1;
    }

    Steinberg::uint32 PLUGIN_API release() SMTG_OVERRIDE
    {
        return 1;
    }

    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE
    {
        auto result = EditController::initialize (context);
        if (result != Steinberg::kResultOk)
            return result;

        return Steinberg::kResultOk;
    }

    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE
    {
        return EditController::terminate();
    }

    Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE
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
                                                               Steinberg::Vst::ParamID& id) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getUnitInfo (Steinberg::int32 unitIndex, Steinberg::Vst::UnitInfo& info) SMTG_OVERRIDE
    {
        if (unitIndex == 0)
        {
            info.id = Steinberg::Vst::kRootUnitId;
            info.parentUnitId = Steinberg::Vst::kNoParentUnitId;
            info.programListId = Steinberg::Vst::kNoProgramListId;
            info.name[0] = 0;
            return Steinberg::kResultOk;
        }
        return Steinberg::kResultFalse;
    }

    Steinberg::Vst::UnitID PLUGIN_API getSelectedUnit() SMTG_OVERRIDE
    {
        return {};
    }

    Steinberg::int32 PLUGIN_API getUnitCount() SMTG_OVERRIDE
    {
        return 1;
    }

    Steinberg::tresult PLUGIN_API setUnitProgramData (Steinberg::int32 listOrUnitId, Steinberg::int32 programIndex, Steinberg::IBStream* data) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getProgramListInfo (Steinberg::int32 listIndex, Steinberg::Vst::ProgramListInfo& info) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::int32 PLUGIN_API getProgramListCount() SMTG_OVERRIDE
    {
        return 1;
    }

    Steinberg::tresult PLUGIN_API getProgramName (Steinberg::Vst::ProgramListID listId,
                                                  Steinberg::int32 programIndex,
                                                  Steinberg::Vst::String128 name) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getProgramInfo (Steinberg::Vst::ProgramListID listId,
                                                  Steinberg::int32 programIndex,
                                                  Steinberg::Vst::CString attributeId,
                                                  Steinberg::Vst::String128 attributeValue) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API hasProgramPitchNames (Steinberg::Vst::ProgramListID listId, Steinberg::int32 programIndex) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getProgramPitchName (Steinberg::Vst::ProgramListID listId,
                                                       Steinberg::int32 programIndex,
                                                       Steinberg::int16 midiPitch,
                                                       Steinberg::Vst::String128 name) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API selectUnit (Steinberg::Vst::UnitID unitId) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API getUnitByBus (Steinberg::Vst::MediaType type,
                                                Steinberg::Vst::BusDirection dir,
                                                Steinberg::int32 busIndex,
                                                Steinberg::int32 channel,
                                                Steinberg::Vst::UnitID& unitId) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    Steinberg::tresult PLUGIN_API setChannelContextInfos (Steinberg::Vst::IAttributeList* list) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

    //Steinberg::tresult PLUGIN_API remapParamID (Steinberg::Vst::ParamID id, Steinberg::Vst::UnitID fromUnit, Steinberg::Vst::UnitID toUnit) SMTG_OVERRIDE
    //{
    //    return Steinberg::kResultFalse;
    //}

    Steinberg::tresult PLUGIN_API getCompatibleParamID (const Steinberg::TUID pluginToReplaceUID,
                                                        Steinberg::Vst::ParamID oldParamID,
                                                        Steinberg::Vst::ParamID& newParamID) SMTG_OVERRIDE
    {
        return Steinberg::kResultFalse;
    }

private:
    AudioProcessor* processor = nullptr;
    AudioProcessorEditor* editor = nullptr;
*/
};

//==============================================================================

class AudioPluginWrapperVST3 : public Steinberg::Vst::AudioEffect
{
public:
    AudioPluginWrapperVST3()
    {
        yup::initialiseJuce_GUI();
        yup::initialiseYup_Windowing();

        processor = createPluginProcessor();
    }

    virtual ~AudioPluginWrapperVST3()
    {
        if (processor != nullptr)
        {
            delete processor;
            processor = nullptr;
        }

        yup::shutdownYup_Windowing();
        yup::shutdownJuce_GUI();
    }

    static Steinberg::FUnknown* createInstance ([[maybe_unused]] void* context)
    {
        return (Steinberg::Vst::IAudioProcessor*) new AudioPluginWrapperVST3();
    }

    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE
    {
        auto result = AudioEffect::initialize (context);
        if (result != Steinberg::kResultOk)
            return result;

        addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
        addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

        // Add parameter definitions
        for (int i = 0; i < processor->getNumParameters(); ++i)
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

    Steinberg::tresult PLUGIN_API terminate() SMTG_OVERRIDE
    {
        processor->releaseResources();

        return AudioEffect::terminate();
    }

    Steinberg::tresult PLUGIN_API setBusArrangements (
        Steinberg::Vst::SpeakerArrangement* inputs,
        int32 numIns,
        Steinberg::Vst::SpeakerArrangement* outputs,
        int32 numOuts) SMTG_OVERRIDE
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

    Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE
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

    Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE
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

    Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE
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
                    processor->getParameter (i).setValue (static_cast<float> (value));
            }
        }

        // --- Process Audio ---
        if (data.numSamples > 0 && data.inputs && data.outputs)
        {
            Steinberg::Vst::AudioBusBuffers& inBus = data.inputs[0];
            Steinberg::Vst::AudioBusBuffers& outBus = data.outputs[0];

            AudioSampleBuffer audioBuffer (
                reinterpret_cast<float**> (outBus.channelBuffers32),
                data.numOutputs,
                data.numSamples);

            processor->processBlock (audioBuffer, midiBuffer);
        }

        return Steinberg::kResultOk;
    }

private:
    AudioProcessor* processor = nullptr;

    Steinberg::Vst::ProcessContext processContext;
    Steinberg::Vst::ProcessSetup processSetup;

    MidiBuffer midiBuffer;
};

//==============================================================================

// Unique identifier for our processor class (example GUID)
// {D1F1C1B8-9F3D-4E0B-80A9-ABCD12345678}

// static auto ProcessorUUID = juce::Uuid::fromSHA1 (juce::SHA1 (CharPointer_UTF8 (YupPlugin_Id)));

static const Steinberg::FUID YupPlugin_Processor_UID (0xc9a84cd4, 0xc7c34936, 0xbf0b3315, 0x2de1ef58);
static const Steinberg::FUID YupPlugin_Controller_UID (0xa7c40810, 0xbf604829, 0xab2b5329, 0xb3f3a131);

} // namespace yup

//==============================================================================

BEGIN_FACTORY_DEF (
    YupPlugin_Vendor,
    YupPlugin_URL,
    "mailto:" YupPlugin_Email)

DEF_CLASS2 (
    INLINE_UID_FROM_FUID (yup::YupPlugin_Processor_UID),
    PClassInfo::kManyInstances,    // Supports multiple instances
    kVstAudioEffectClass,          // Component category
    YupPlugin_Name,                // Plugin name
    Vst::kDistributable,           // Distribution status
    Steinberg::Vst::PlugType::kFx, // Subcategory (effect)
    YupPlugin_Version,             // Plugin version
    kVstVersionString,             // The VST 3 SDK version (do not change this, always use this define)
    yup::AudioPluginWrapperVST3::createInstance)

/*
DEF_CLASS2 (
    INLINE_UID_FROM_FUID (yup::YupPlugin_Controller_UID),
    PClassInfo::kManyInstances,   // Supports multiple instances
    kVstComponentControllerClass, // Controller category (do not change this)
    YupPlugin_Name "Controller",  // Controller name (can be the same as the component name)
    0,                            // Not used here
    "",                           // Not used here
    YupPlugin_Version,            // Plug-in version (to be changed)
    kVstVersionString,            // The VST 3 SDK version (do not change this, always use this define)
    yup::AudioPluginEditorVST3::createInstance)
*/

END_FACTORY
