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
#include <public.sdk/source/main/pluginfactory.h>
#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/base/funknown.h>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <pluginterfaces/vst/ivstparameterchanges.h>

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

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
        if (processor)
        {
            delete processor;
            processor = nullptr;
        }

        yup::shutdownYup_Windowing();
        yup::shutdownJuce_GUI();
    }

    static FUnknown* createInstance ([[maybe_unused]] void* context)
    {
        return (IAudioProcessor*) new AudioPluginWrapperVST3();
    }

    Steinberg::tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE
    {
        auto result = AudioEffect::initialize (context);
        if (result != Steinberg::kResultOk)
            return result;

        addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
        addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

        // Optionally add parameter definitions and presets here.

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
            // Optionally initialize your AudioProcessor with the current sample rate/block size.

            //virtual void prepareToPlay (float sampleRate, int maxBlockSize) = 0;
            //virtual void releaseResources() = 0;
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
                {
                    // Map the parameter change to your AudioProcessor
                    // processor->setParameter(i, static_cast<float>(value));
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
static const Steinberg::FUID ProcessorUID (0xD1F1C1B8, 0x9F3D4E0B, 0x80A9ABCD, 0x12345678);

} // namespace yup

//==============================================================================

BEGIN_FACTORY_DEF (
    "My Company",
    "http://www.mycompany.com",
    "mailto:info@mycompany.com")

// Register the VST3 processor component.
DEF_CLASS2 (
    INLINE_UID_FROM_FUID (yup::ProcessorUID),
    PClassInfo::kManyInstances, // Supports multiple instances
#if YupPlugin_IsSynth           // Component category
    kVstPluginCategorySynth,
#else
    kVstAudioEffectClass,
#endif
    YupPlugin_Name,      // Plugin name
    Vst::kDistributable, // Distribution status
    "Fx",                // Subcategory (effect)
    YupPlugin_Version,   // Plugin version
    kVstVersionString,
    yup::AudioPluginWrapperVST3::createInstance)

END_FACTORY
