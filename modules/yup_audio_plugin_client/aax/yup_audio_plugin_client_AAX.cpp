/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_AAX)
#error "YUP_AUDIO_PLUGIN_ENABLE_AAX must be defined"
#endif

#include <AAX_Version.h>

static_assert (AAX_SDK_CURRENT_REVISION >= AAX_SDK_2p6p1_REVISION,
               "YUP requires AAX SDK version 2.6.1 or higher");

#include <AAX.h>
#include <AAX_CEffectParameters.h>
#include <AAX_CEffectGUI.h>
#include <AAX_ICollection.h>
#include <AAX_IEffectDescriptor.h>
#include <AAX_IComponentDescriptor.h>
#include <AAX_IPropertyMap.h>
#include <AAX_IController.h>
#include <AAX_IViewContainer.h>
#include <AAX_ITransport.h>
#include <AAX_IMIDINode.h>
#include <AAX_CBinaryTaperDelegate.h>
#include <AAX_CBinaryDisplayDelegate.h>
#include <AAX_CLinearTaperDelegate.h>
#include <AAX_CNumberDisplayDelegate.h>
#include <AAX_Errors.h>
#include <AAX_Assert.h>
#include <AAX_Enums.h>
#include <AAX_IDescriptionHost.h>
#include <AAX_IFeatureInfo.h>
#include <AAX_UIDs.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#if ! defined(YupPlugin_AAX_ManufacturerID) || ! defined(YupPlugin_AAX_ProductID) \
    || ! defined(YupPlugin_AAX_PlugInID_Native) || ! defined(YupPlugin_AAX_PlugInID_AudioSuite)
#error "AAX plugin clients require YupPlugin_AAX_ManufacturerID, YupPlugin_AAX_ProductID, \
YupPlugin_AAX_PlugInID_Native and YupPlugin_AAX_PlugInID_AudioSuite to be defined"
#endif

#if ! defined(YupPlugin_AAXCategory)
#if YupPlugin_IsSynth
#define YupPlugin_AAXCategory AAX_ePlugInCategory_SWGenerators
#else
#define YupPlugin_AAXCategory AAX_ePlugInCategory_Effect
#endif
#endif

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

//==============================================================================
#if YUP_ENABLE_PLUGIN_CLIENT_AAX_LOGGING
#define YUP_AAX_LOG(x) Logger::writeToLog (x)
#else
#define YUP_AAX_LOG(x)
#endif

//==============================================================================
// Forward declarations
//==============================================================================

class YupAAX_Processor;
class YupAAX_GUI;

void AAX_CALLBACK yupAAXAlgorithmCallback (void* const instancesBegin[], const void* instancesEnd);

//==============================================================================
// Algorithm context
//==============================================================================

struct YupAlgorithmContext
{
    float* const* inputChannels = nullptr;
    float* const* outputChannels = nullptr;
    int32_t* bufferSize = nullptr;
    int32_t* bypass = nullptr;
    AAX_IMIDINode* midiNodeIn = nullptr;
    AAX_IMIDINode* midiNodeOut = nullptr;
    void* pluginInfo = nullptr;
    int32_t* isPrepared = nullptr;
    float* const* meterTapBuffers = nullptr;
};

enum YupAlgorithmField
{
    fieldAudioIn = AAX_FIELD_INDEX (YupAlgorithmContext, inputChannels),
    fieldAudioOut = AAX_FIELD_INDEX (YupAlgorithmContext, outputChannels),
    fieldBufferSize = AAX_FIELD_INDEX (YupAlgorithmContext, bufferSize),
    fieldBypass = AAX_FIELD_INDEX (YupAlgorithmContext, bypass),
    fieldMidiIn = AAX_FIELD_INDEX (YupAlgorithmContext, midiNodeIn),
    fieldMidiOut = AAX_FIELD_INDEX (YupAlgorithmContext, midiNodeOut),
    fieldPluginInfo = AAX_FIELD_INDEX (YupAlgorithmContext, pluginInfo),
    fieldPreparedFlag = AAX_FIELD_INDEX (YupAlgorithmContext, isPrepared),
    fieldMeterTaps = AAX_FIELD_INDEX (YupAlgorithmContext, meterTapBuffers),
};

//==============================================================================
// Helpers
//==============================================================================

struct YupPluginInstanceInfo
{
    explicit YupPluginInstanceInfo (YupAAX_Processor& p)
        : processor (p)
    {
    }

    YupAAX_Processor& processor;
};

static void aaxCheck ([[maybe_unused]] AAX_Result result)
{
    jassert (result == AAX_SUCCESS);
}

static int32_t getAaxParamHash (const char* paramID) noexcept
{
    int32_t h = 0;
    if (paramID != nullptr)
    {
        for (auto p = paramID; *p != '\0'; ++p)
            h = 31 * h + static_cast<int32_t> (*p);
    }
    return h;
}

//==============================================================================
// Channel count to AAX stem format conversion
//==============================================================================

static AAX_EStemFormat stemFormatForAmbisonicOrder (int order)
{
    switch (order)
    {
        case 1:
            return AAX_eStemFormat_Ambi_1_ACN;
        case 2:
            return AAX_eStemFormat_Ambi_2_ACN;
        case 3:
            return AAX_eStemFormat_Ambi_3_ACN;
        case 4:
            return AAX_eStemFormat_Ambi_4_ACN;
        case 5:
            return AAX_eStemFormat_Ambi_5_ACN;
        case 6:
            return AAX_eStemFormat_Ambi_6_ACN;
        case 7:
            return AAX_eStemFormat_Ambi_7_ACN;
        default:
            return AAX_eStemFormat_INT32_MAX;
    }
}

// YUP buses carry a plain channel count, so the stem format is chosen by count
// and channels are passed through to the processor in AAX stream order.
static AAX_EStemFormat stemFormatForChannelCount (int numChannels)
{
    switch (numChannels)
    {
        case 0:
            return AAX_eStemFormat_None;
        case 1:
            return AAX_eStemFormat_Mono;
        case 2:
            return AAX_eStemFormat_Stereo;
        case 3:
            return AAX_eStemFormat_LCR;
        case 4:
            return AAX_eStemFormat_Quad;
        case 5:
            return AAX_eStemFormat_5_0;
        case 6:
            return AAX_eStemFormat_5_1;
        case 7:
            return AAX_eStemFormat_7_0_DTS;
        case 8:
            return AAX_eStemFormat_7_1_DTS;
        case 9:
            return AAX_eStemFormat_7_0_2;
        case 10:
            return AAX_eStemFormat_7_1_2;
        case 11:
            return AAX_eStemFormat_7_0_4;
        case 12:
            return AAX_eStemFormat_7_1_4;
        case 13:
            return AAX_eStemFormat_9_0_4;
        case 14:
            return AAX_eStemFormat_9_1_4;
        case 15:
            return AAX_eStemFormat_9_0_6;
        case 16:
            return AAX_eStemFormat_9_1_6;
        default:
            break;
    }

    const auto maybeOrder = AudioChannelSet::getAmbisonicOrderForNumChannels (numChannels);
    if (maybeOrder != -1)
        return stemFormatForAmbisonicOrder (maybeOrder);

    return AAX_eStemFormat_INT32_MAX;
}

//==============================================================================
// Meter type detection (name-based since YUP doesn't expose categories)
//==============================================================================

static AAX_EMeterType getMeterTypeFromParam (const AudioParameter& param)
{
    const auto name = param.getName();

    if (name.containsIgnoreCase ("input"))
        return AAX_eMeterType_Input;

    if (name.containsIgnoreCase ("output"))
        return AAX_eMeterType_Output;

    if (name.containsIgnoreCase ("gr") || name.containsIgnoreCase ("gain reduction"))
        return AAX_eMeterType_CLGain;

    return AAX_eMeterType_Other;
}

static bool isMeterParameter (const AudioParameter& param)
{
    const auto name = param.getName();
    return name.containsIgnoreCase ("meter");
}

//==============================================================================
// YupAAX_Processor
//==============================================================================

class YupAAX_Processor
    : public AAX_CEffectParameters
    , private AudioProcessorBase::Listener
{
public:
    static AAX_CEffectParameters* AAX_CALLBACK Create()
    {
        return new YupAAX_Processor();
    }

    YupAAX_Processor()
    {
        processor.reset (createPluginProcessor());
        processor->addListener (this);

        AAX_CEffectParameters::GetNumberOfChunks (&yupChunkIndex);
        YUP_AAX_LOG ("YupAAX_Processor: created");
    }

    ~YupAAX_Processor() override
    {
        processor->removeListener (this);
        endActiveParameterGestures (processor.get());
    }

    //==========================================================================
    // AAX_CEffectParameters — Initialization
    //==========================================================================

    AAX_Result Uninitialize() override
    {
        if (isPrepared && processor != nullptr)
        {
            isPrepared = false;
            processor->releaseResources();
        }

        return AAX_CEffectParameters::Uninitialize();
    }

    AAX_Result EffectInit() override
    {
        YUP_AAX_LOG ("YupAAX_Processor::EffectInit");

        auto* ctrl = Controller();
        if (ctrl == nullptr)
            return AAX_ERROR_NULL_OBJECT;

        AAX_CSampleRate sampleRate = 0;
        ctrl->GetSampleRate (&sampleRate);

        AAX_EStemFormat inputFormat = AAX_eStemFormat_None;
        AAX_EStemFormat outputFormat = AAX_eStemFormat_None;
        ctrl->GetInputStemFormat (&inputFormat);
        ctrl->GetOutputStemFormat (&outputFormat);

        auto err = preparePlugin (static_cast<float> (sampleRate), inputFormat, outputFormat);
        if (err != AAX_SUCCESS)
            return err;

        addAudioProcessorParameters();
        return AAX_SUCCESS;
    }

    //==========================================================================
    // AAX_CEffectParameters — Parameters
    //==========================================================================

    AAX_Result UpdateParameterNormalizedValue (AAX_CParamID paramID, double value, AAX_EUpdateSource source) override
    {
        const auto result = AAX_CEffectParameters::UpdateParameterNormalizedValue (paramID, value, source);

        if (result == AAX_SUCCESS && ! inParameterChangedCallback.get())
            setAudioProcessorParameter (paramID, static_cast<float> (value));

        return result;
    }

    AAX_Result GetParameterValueFromString (AAX_CParamID paramID, double* result, const AAX_IString& text) const override
    {
        if (auto* param = getParamForID (paramID))
        {
            *result = static_cast<double> (param->convertFromString (String (text.Get())));
            return AAX_SUCCESS;
        }

        return AAX_CEffectParameters::GetParameterValueFromString (paramID, result, text);
    }

    AAX_Result GetParameterStringFromValue (AAX_CParamID paramID, double value, AAX_IString* result, int32_t maxLen) const override
    {
        if (auto* param = getParamForID (paramID))
        {
            const auto str = param->convertToString (static_cast<float> (value));
            result->Set (str.substring (0, maxLen).toRawUTF8());
        }

        return AAX_SUCCESS;
    }

    AAX_Result GetParameterNumberOfSteps (AAX_CParamID paramID, int32_t* result) const override
    {
        if (auto* param = getParamForID (paramID))
        {
            const int steps = param->getNumSteps();
            *result = steps > 0 ? jmin (steps, 2048) : 0;
        }
        else
        {
            *result = 0;
        }

        return AAX_SUCCESS;
    }

    AAX_Result GetParameterNormalizedValue (AAX_CParamID paramID, double* result) const override
    {
        if (auto* param = getParamForID (paramID))
            *result = static_cast<double> (param->getNormalizedValue());
        else
            *result = 0.0;

        return AAX_SUCCESS;
    }

    AAX_Result SetParameterNormalizedValue (AAX_CParamID paramID, double newValue) override
    {
        if (auto* p = mParameterManager.GetParameterByID (paramID))
            p->SetValueWithFloat (static_cast<float> (newValue));

        setAudioProcessorParameter (paramID, static_cast<float> (newValue));

        return AAX_SUCCESS;
    }

    AAX_Result SetParameterNormalizedRelative (AAX_CParamID paramID, double newDeltaValue) override
    {
        if (auto* param = getParamForID (paramID))
        {
            float newValue = param->getNormalizedValue() + static_cast<float> (newDeltaValue);
            newValue = jlimit (0.0f, 1.0f, newValue);
            setAudioProcessorParameter (paramID, newValue);

            if (auto* p = mParameterManager.GetParameterByID (paramID))
                p->SetValueWithFloat (newValue);
        }

        return AAX_SUCCESS;
    }

    AAX_Result GetParameterNameOfLength (AAX_CParamID paramID, AAX_IString* result, int32_t maxLen) const override
    {
        if (auto* param = getParamForID (paramID))
            result->Set (param->getName().substring (0, maxLen).toRawUTF8());

        return AAX_SUCCESS;
    }

    AAX_Result GetParameterName (AAX_CParamID paramID, AAX_IString* result) const override
    {
        if (auto* param = getParamForID (paramID))
            result->Set (param->getName().substring (0, 31).toRawUTF8());

        return AAX_SUCCESS;
    }

    AAX_Result GetParameterDefaultNormalizedValue (AAX_CParamID paramID, double* result) const override
    {
        if (auto* param = getParamForID (paramID))
            *result = static_cast<double> (
                param->convertToNormalizedValue (param->getDefaultValue()));
        else
            *result = 0.0;

        return AAX_SUCCESS;
    }

    //==========================================================================
    // AAX_CEffectParameters — State reset
    //==========================================================================

    AAX_Result ResetFieldData (AAX_CFieldIndex fieldIndex, void* data, uint32_t dataSize) const override
    {
        switch (fieldIndex)
        {
            case fieldPluginInfo:
            {
                const auto numObjects = dataSize / sizeof (YupPluginInstanceInfo);

                auto* objects = static_cast<YupPluginInstanceInfo*> (data);
                for (size_t i = 0; i < numObjects; ++i)
                    new (objects + i) YupPluginInstanceInfo (const_cast<YupAAX_Processor&> (*this));

                break;
            }

            case fieldPreparedFlag:
            {
                const_cast<YupAAX_Processor*> (this)->preparePlugin (lastSampleRate, lastInputFormat, lastOutputFormat);

                const auto numObjects = dataSize / sizeof (uint32_t);

                auto* objects = static_cast<uint32_t*> (data);
                for (size_t i = 0; i < numObjects; ++i)
                    objects[i] = 1;

                break;
            }

            default:
            {
                if (data != nullptr && dataSize > 0)
                    std::memset (data, 0, dataSize);

                break;
            }
        }

        return AAX_SUCCESS;
    }

    //==========================================================================
    // AAX_CEffectParameters — Chunks
    //==========================================================================

    AAX_Result GetNumberOfChunks (int32_t* numChunks) const override
    {
        *numChunks = yupChunkIndex + 1;

        return AAX_SUCCESS;
    }

    AAX_Result GetChunkIDFromIndex (int32_t index, AAX_CTypeID* chunkID) const override
    {
        if (index == yupChunkIndex)
        {
            *chunkID = YupPlugin_AAX_ChunkID;
            return AAX_SUCCESS;
        }

        return AAX_CEffectParameters::GetChunkIDFromIndex (index, chunkID);
    }

    AAX_Result GetChunkSize (AAX_CTypeID chunkID, uint32_t* size) const override
    {
        if (chunkID == YupPlugin_AAX_ChunkID)
        {
            MemoryBlock processorState;
            processor->saveStateIntoMemory (processorState);

            auto& tls = chunkData.get();
            tls.data = writeWrapperBypassState (
                YupPlugin_AAX_ChunkMagic,
                YupPlugin_AAX_ChunkVersion,
                isBypassed.load(),
                processorState,
                ! processorState.isEmpty());
            tls.isValid = true;

            *size = static_cast<uint32_t> (tls.data.getSize());
            return AAX_SUCCESS;
        }

        return AAX_CEffectParameters::GetChunkSize (chunkID, size);
    }

    AAX_Result GetChunk (AAX_CTypeID chunkID, AAX_SPlugInChunk* chunk) const override
    {
        if (chunkID == YupPlugin_AAX_ChunkID)
        {
            auto& tls = chunkData.get();
            if (! tls.isValid)
                return 20700; // AAX_ERROR_PLUGIN_API_INVALID_THREAD

            // The host allocates the chunk using the size returned by
            // GetChunkSize, fData is a variable length trailing array
            const auto size = static_cast<int32_t> (tls.data.getSize());
            chunk->fSize = size;

            if (size > 0 && tls.data.getData() != nullptr)
                tls.data.copyTo (chunk->fData, 0, static_cast<size_t> (size));

            tls.isValid = false;
            return AAX_SUCCESS;
        }

        return AAX_CEffectParameters::GetChunk (chunkID, chunk);
    }

    AAX_Result SetChunk (AAX_CTypeID chunkID, const AAX_SPlugInChunk* chunk) override
    {
        if (chunkID == YupPlugin_AAX_ChunkID)
        {
            MemoryBlock rawData (chunk->fData, static_cast<size_t> (chunk->fSize));
            auto state = readWrapperBypassState (rawData, YupPlugin_AAX_ChunkMagic, YupPlugin_AAX_ChunkVersion);

            if (state.hasWrapperState)
            {
                setBypassed (state.isBypassed);
                SetParameterNormalizedValue (cDefaultMasterBypassID, state.isBypassed ? 1.0 : 0.0);

                if (state.hasProcessorState && ! state.processorState.isEmpty())
                    processor->loadStateFromMemory (state.processorState);
            }
            else
            {
                processor->loadStateFromMemory (rawData);
            }

            resyncParameterValues();
            return AAX_SUCCESS;
        }

        return AAX_CEffectParameters::SetChunk (chunkID, chunk);
    }

    AAX_Result GetNumberOfChanges (int32_t* numChanges) const override
    {
        const auto result = AAX_CEffectParameters::GetNumberOfChanges (numChanges);

        *numChanges += numSetDirtyCalls;

        return result;
    }

    //==========================================================================
    // AAX_CEffectParameters — Notifications
    //==========================================================================

    AAX_Result NotificationReceived (AAX_CTypeID notificationType, const void* notificationData, uint32_t notificationDataSize) override
    {
        if (notificationType == AAX_eNotificationEvent_EnteringOfflineMode)
        {
            processor->suspendProcessing (true);
        }
        else if (notificationType == AAX_eNotificationEvent_ExitingOfflineMode)
        {
            processor->suspendProcessing (false);
        }

        return AAX_CEffectParameters::NotificationReceived (notificationType, notificationData, notificationDataSize);
    }

    //==========================================================================
    // Audio processing
    //==========================================================================

    void process (float* const* inputChannelData,
                  float* const* outputChannelData,
                  int bufferSize,
                  int32_t bypassFlag,
                  AAX_IMIDINode* midiNodeIn,
                  AAX_IMIDINode* midiNodeOut,
                  float* const* meterTapBuffers)
    {
        const bool currentlyBypassed = (bypassFlag != 0);
        setBypassed (currentlyBypassed);

        // The MIDI node fields are only registered when the plugin handles
        // MIDI, otherwise their context values are meaningless
        midiBuffer.clear();
        if (processor->acceptsMidi() && midiNodeIn != nullptr)
            fillMidiBufferFromAaxNode (*midiNodeIn, midiBuffer);

        // The component is described with stem formats matching the processor
        // layout, so the host provides exactly these channel counts
        const auto& layout = processor->getBusLayout();
        const auto numIn = layout.getNumAudioInputChannels();
        const auto numOut = layout.getNumAudioOutputChannels();

        for (int ch = 0; ch < numOut; ++ch)
        {
            if (outputChannelData[ch] == nullptr)
                continue;

            if (ch < numIn && inputChannelData[ch] != nullptr)
            {
                if (outputChannelData[ch] != inputChannelData[ch])
                    std::memcpy (outputChannelData[ch], inputChannelData[ch], static_cast<size_t> (bufferSize) * sizeof (float));
            }
            else
            {
                std::memset (outputChannelData[ch], 0, static_cast<size_t> (bufferSize) * sizeof (float));
            }
        }

        AudioBuffer<float> audioBuffer (outputChannelData, numOut, bufferSize);

        paramChangeBuffer.clear();

        AudioProcessContext<float> context {
            audioBuffer, midiBuffer, paramChangeBuffer, nullptr
        };

        processAudioBlock (*processor, context, currentlyBypassed);

        if (processor->producesMidi() && midiNodeOut != nullptr)
            fillAaxMidiNodeFromBuffer (*midiNodeOut, midiBuffer);

        if (! aaxMeters.empty() && meterTapBuffers != nullptr)
            extractMeterValues (*meterTapBuffers);
    }

    AudioProcessor& getAudioProcessor() noexcept { return *processor; }

private:
    //==========================================================================
    // AudioProcessorBase::Listener
    //==========================================================================

    void audioProcessorChanged (AudioProcessorBase* p, const AudioProcessorBase::ChangeDetails& details) override
    {
        if (p != processor.get())
            return;

        if (details.parameterValuesChanged)
            resyncParameterValues();

        if (details.parameterInfoChanged)
            syncParameterAttributes();

        if (details.latencyChanged)
        {
            if (auto* ctrl = Controller())
                ctrl->SetSignalLatency (processor->getLatencySamples());
        }

        if (details.nonParameterStateChanged)
            numSetDirtyCalls.fetch_add (1);
    }

    //==========================================================================
    // Helpers
    //==========================================================================

    AAX_Result preparePlugin (float sampleRate, AAX_EStemFormat inputFormat, AAX_EStemFormat outputFormat)
    {
        lastSampleRate = sampleRate;
        lastInputFormat = inputFormat;
        lastOutputFormat = outputFormat;

        processor->setPlaybackConfiguration (sampleRate, maxAaxBufferSize);
        isPrepared = true;

        paramChangeBuffer.reserve (getDefaultParameterChangeCapacity (*processor));

        if (auto* ctrl = Controller())
            ctrl->SetSignalLatency (processor->getLatencySamples());

        return AAX_SUCCESS;
    }

    void addAudioProcessorParameters()
    {
        paramMap.clear();
        aaxMeters.clear();

        addMasterBypassParameter();

        const auto parameters = processor->getParameters();

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            auto* param = parameters[i].get();

            if (param == nullptr)
                continue;

            if (isMeterParameter (*param))
            {
                aaxMeters.push_back (param);
                continue;
            }

            const auto aaxParamID = getAaxParamIDFromIndex (static_cast<int> (i));
            addAaxParameter (*param, aaxParamID);

            const auto hash = getAaxParamHash (aaxParamID.toRawUTF8());
            paramMap[hash] = param;
        }
    }

    void addMasterBypassParameter()
    {
        auto* bypassParam = new AAX_CParameter<bool> (
            cDefaultMasterBypassID,
            AAX_CString ("Master Bypass"),
            false,
            AAX_CBinaryTaperDelegate<bool>(),
            AAX_CBinaryDisplayDelegate<bool> ("bypass", "on"),
            true);

        bypassParam->SetNumberOfSteps (2);
        bypassParam->SetType (AAX_eParameterType_Discrete);
        mParameterManager.AddParameter (bypassParam);

        mPacketDispatcher.RegisterPacket (cDefaultMasterBypassID, fieldBypass);
    }

    void addAaxParameter (AudioParameter& param, const String& aaxParamID)
    {
        const auto numSteps = param.getNumSteps();
        const auto isDiscrete = numSteps > 1 && numSteps <= 1000;

        auto* aaxParam = new AAX_CParameter<float> (
            aaxParamID.toRawUTF8(),
            AAX_CString (param.getName().substring (0, 31).toRawUTF8()),
            param.convertToNormalizedValue (param.getDefaultValue()),
            AAX_CLinearTaperDelegate<float> (0.0f, 1.0f),
            AAX_CNumberDisplayDelegate<float>(),
            param.isAutomatable());

        aaxParam->SetNumberOfSteps (static_cast<uint32_t> (numSteps > 0 ? jmin (numSteps, 2048) : 1000));

        aaxParam->SetType (isDiscrete
                               ? AAX_eParameterType_Discrete
                               : AAX_eParameterType_Continuous);

        mParameterManager.AddParameter (aaxParam);
    }

    void syncParameterAttributes()
    {
        const auto parameters = processor->getParameters();

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            auto* param = parameters[i].get();

            if (param == nullptr || isMeterParameter (*param))
                continue;

            const auto aaxParamID = getAaxParamIDFromIndex (static_cast<int> (i));

            SetParameterDefaultNormalizedValue (
                aaxParamID.toRawUTF8(),
                static_cast<double> (param->convertToNormalizedValue (param->getDefaultValue())));
        }
    }

    void setAudioProcessorParameter (const char* aaxParamID, float value)
    {
        const auto hash = getAaxParamHash (aaxParamID);
        const auto it = paramMap.find (hash);

        if (it != paramMap.end() && it->second != nullptr)
        {
            inParameterChangedCallback = true;
            it->second->setNormalizedValue (value);
            inParameterChangedCallback = false;
        }
    }

    void resyncParameterValues()
    {
        const auto parameters = processor->getParameters();

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            const auto aaxParamID = getAaxParamIDFromIndex (static_cast<int> (i));

            inParameterChangedCallback = true;
            SetParameterNormalizedValue (
                aaxParamID.toRawUTF8(),
                static_cast<double> (parameters[i]->getNormalizedValue()));
            inParameterChangedCallback = false;
        }
    }

    AudioParameter* getParamForID (const char* aaxParamID) const
    {
        const auto hash = getAaxParamHash (aaxParamID);
        const auto it = paramMap.find (hash);
        return (it != paramMap.end()) ? it->second : nullptr;
    }

    void setBypassed (bool shouldBypass)
    {
        isBypassed.store (shouldBypass);
    }

    //==========================================================================
    // MIDI
    //==========================================================================

    void fillMidiBufferFromAaxNode (AAX_IMIDINode& node, MidiBuffer& buffer)
    {
        auto* stream = node.GetNodeBuffer();
        if (stream == nullptr)
            return;

        const auto* firstPacket = stream->mBuffer;
        const auto numPackets = static_cast<int> (stream->mBufferSize);

        for (int i = 0; i < numPackets; ++i)
        {
            const auto& pkt = firstPacket[i];
            auto message = aaxMidiPacketToYup (pkt);
            if (message.has_value())
                buffer.addEvent (*message, pkt.mTimestamp);
        }
    }

    static std::optional<MidiMessage> aaxMidiPacketToYup (const AAX_CMidiPacket& pkt)
    {
        if (pkt.mLength == 0 || pkt.mLength > 4)
            return std::nullopt;

        const auto* data = pkt.mData;
        const auto status = data[0] & 0xF0;
        const auto channel = 1 + (data[0] & 0x0F);

        switch (status)
        {
            case 0x80:
                return MidiMessage::noteOff (channel, data[1], static_cast<uint8> (data[2]));
            case 0x90:
                return MidiMessage::noteOn (channel, data[1], static_cast<uint8> (data[2]));
            case 0xA0:
                return MidiMessage::aftertouchChange (channel, data[1], data[2]);
            case 0xB0:
                return MidiMessage::controllerEvent (channel, data[1], data[2]);
            case 0xC0:
                return MidiMessage::programChange (channel, data[1]);
            case 0xD0:
                return MidiMessage::channelPressureChange (channel, data[1]);
            case 0xE0:
            {
                const auto value = static_cast<int> (data[1]) | (static_cast<int> (data[2]) << 7);
                return MidiMessage::pitchWheel (channel, value);
            }
            default:
                break;
        }
        return std::nullopt;
    }

    void fillAaxMidiNodeFromBuffer (AAX_IMIDINode& node, const MidiBuffer& buffer)
    {
        for (const auto& event : buffer)
        {
            AAX_CMidiPacket pkt;
            pkt.mTimestamp = event.samplePosition;
            pkt.mIsImmediate = false;
            yupMidiMessageToAaxPacket (event.getMessage(), pkt);
            node.PostMIDIPacket (&pkt);
        }
    }

    static void yupMidiMessageToAaxPacket (const MidiMessage& msg, AAX_CMidiPacket& pkt)
    {
        const auto data = msg.getRawData();
        const auto dataSize = msg.getRawDataSize();
        const auto copySize = jmin (dataSize, static_cast<int> (sizeof (pkt.mData)));
        std::memcpy (pkt.mData, data, static_cast<size_t> (copySize));
        pkt.mLength = static_cast<uint32_t> (copySize);
    }

    //==========================================================================
    // Meters
    //==========================================================================

    void extractMeterValues (float* meterTaps)
    {
        if (meterTaps == nullptr)
            return;

        for (size_t i = 0; i < aaxMeters.size(); ++i)
        {
            const auto value = aaxMeters[i]->getNormalizedValue();
            meterTaps[i] = std::max (meterTaps[i], std::abs (value));
        }
    }

    //==========================================================================
    // Parameter IDs
    //==========================================================================

    static String getAaxParamIDFromIndex (int index)
    {
        return String::formatted ("par_%d", index);
    }

    //==========================================================================

    struct ChunkMemoryBlock
    {
        bool isValid = false;
        MemoryBlock data;
    };

    ScopedYupInitialiser_GUI scopeInitialiser;

    std::unique_ptr<AudioProcessor> processor;
    int32_t yupChunkIndex = 0;
    float lastSampleRate = 44100.0f;
    AAX_EStemFormat lastInputFormat = AAX_eStemFormat_None;
    AAX_EStemFormat lastOutputFormat = AAX_eStemFormat_None;
    bool isPrepared = false;
    std::atomic<bool> isBypassed { false };
    mutable std::atomic<int> numSetDirtyCalls { 0 };

    MidiBuffer midiBuffer;
    ParameterChangeBuffer paramChangeBuffer;

    std::unordered_map<int32_t, AudioParameter*> paramMap;
    std::vector<AudioParameter*> aaxMeters;

    mutable ThreadLocalValue<ChunkMemoryBlock> chunkData;
    ThreadLocalValue<bool> inParameterChangedCallback;

    // AAX native algorithm buffers are at most 1024 samples
    static constexpr int maxAaxBufferSize = 1024;

    static constexpr int YupPlugin_AAX_ChunkMagic = 0x59415858; // 'YAAX'
    static constexpr int YupPlugin_AAX_ChunkVersion = 1;
    static constexpr AAX_CTypeID YupPlugin_AAX_ChunkID = 'YUpS'; // YUP State
};

//==============================================================================
// YupAAX_GUI
//==============================================================================

class YupAAX_GUI : public AAX_CEffectGUI
{
public:
    static AAX_CEffectGUI* AAX_CALLBACK Create()
    {
        return new YupAAX_GUI();
    }

    YupAAX_GUI() = default;

    ~YupAAX_GUI() override { DeleteViewContainer(); }

    //==========================================================================
    // AAX_CEffectGUI overrides (all return void!)
    //==========================================================================

    void CreateViewContents() override
    {
        auto* params = GetEffectParameters();
        if (params == nullptr)
            return;

        auto* yupParams = dynamic_cast<YupAAX_Processor*> (params);
        if (yupParams == nullptr)
            return;

        if (editorComponent != nullptr)
            return;

        auto& audioProcessor = yupParams->getAudioProcessor();

        if (audioProcessor.hasEditor())
        {
            editorComponent.reset (audioProcessor.createEditor());
            YUP_AAX_LOG ("YupAAX_GUI: created editor");
        }
    }

    void CreateViewContainer() override
    {
        if (editorComponent == nullptr)
            CreateViewContents();

        if (editorComponent == nullptr)
            return;

        auto* viewContainer = GetViewContainer();
        if (viewContainer == nullptr)
            return;

        auto* nativeView = viewContainer->GetPtr();
        if (nativeView == nullptr)
            return;

        ComponentNative::Flags flags =
            ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow;

        if (editorComponent->shouldRenderContinuous())
            flags.set (ComponentNative::renderContinuous);

        auto options = ComponentNative::Options()
                           .withFlags (flags)
                           .withResizableWindow (editorComponent->isResizable());

        const auto preferredSize = editorComponent->getPreferredSize();
        editorComponent->setSize ({ static_cast<float> (preferredSize.getWidth()),
                                    static_cast<float> (preferredSize.getHeight()) });

        editorComponent->addToDesktop (options, nativeView);
        editorComponent->setVisible (true);
        editorComponent->attachedToNative();

        YUP_AAX_LOG ("YupAAX_GUI: attached to native view");
    }

    void DeleteViewContainer() override
    {
        if (editorComponent != nullptr)
        {
            editorComponent->removeFromDesktop();
            editorComponent.reset();
        }
    }

    AAX_Result GetViewSize (AAX_Point* size) const override
    {
        if (size == nullptr)
            return AAX_ERROR_NULL_OBJECT;

        size->horz = 0.0f;
        size->vert = 0.0f;

        if (editorComponent != nullptr)
        {
            if (editorComponent->isResizable() && editorComponent->getWidth() != 0)
            {
                size->horz = static_cast<float> (editorComponent->getWidth());
                size->vert = static_cast<float> (editorComponent->getHeight());
            }
            else
            {
                const auto preferredSize = editorComponent->getPreferredSize();
                size->horz = static_cast<float> (preferredSize.getWidth());
                size->vert = static_cast<float> (preferredSize.getHeight());
            }
        }

        return AAX_SUCCESS;
    }

    AAX_Result SetControlHighlightInfo (
        AAX_CParamID,
        AAX_CBoolean,
        AAX_EHighlightColor) override
    {
        return AAX_SUCCESS;
    }

private:
    ScopedYupInitialiser_Windowing scopeInitialiser;
    std::unique_ptr<AudioProcessorEditor> editorComponent;
};

//==============================================================================
// Algorithm callback
//==============================================================================

void AAX_CALLBACK yupAAXAlgorithmCallback (void* const instancesBegin[], const void* instancesEnd)
{
    auto** typedBegin = static_cast<YupAlgorithmContext**> ((void*) instancesBegin);
    auto** typedEnd = static_cast<YupAlgorithmContext**> ((void*) instancesEnd);

    for (auto** walk = typedBegin; walk < typedEnd; ++walk)
    {
        auto* ctx = *walk;
        if (ctx == nullptr || ctx->pluginInfo == nullptr)
            continue;

        auto* info = static_cast<YupPluginInstanceInfo*> (ctx->pluginInfo);

        const auto bufSize = (ctx->bufferSize != nullptr)
                               ? static_cast<int> (*ctx->bufferSize)
                               : 0;

        if (bufSize <= 0)
            continue;

        info->processor.process (
            ctx->inputChannels,
            ctx->outputChannels,
            bufSize,
            ctx->bypass != nullptr ? *ctx->bypass : 0,
            ctx->midiNodeIn,
            ctx->midiNodeOut,
            ctx->meterTapBuffers);
    }
}

//==============================================================================
// Plugin description
//==============================================================================

static void getPlugInDescription (AAX_IEffectDescriptor& descriptor)
{
    std::unique_ptr<AudioProcessor> plugin (createPluginProcessor());

    descriptor.AddName (YupPlugin_Name);
    descriptor.AddName (YupPlugin_Description);
    descriptor.AddCategory (YupPlugin_AAXCategory);

    if (plugin->hasEditor())
        aaxCheck (descriptor.AddProcPtr (reinterpret_cast<void*> (YupAAX_GUI::Create), kAAX_ProcPtrID_Create_EffectGUI));

    aaxCheck (descriptor.AddProcPtr (reinterpret_cast<void*> (YupAAX_Processor::Create), kAAX_ProcPtrID_Create_EffectParameters));

#ifdef YupPlugin_AAXPageTableFile
    descriptor.AddResourceInfo (AAX_eResourceType_PageTable, YupPlugin_AAXPageTableFile);
#endif

    // Register meters on the descriptor, the same IDs are then bound to the
    // component's meter tap field below
    std::vector<AAX_CTypeID> meterIDs;

    for (const auto& parameter : plugin->getParameters())
    {
        auto* param = parameter.get();
        if (param == nullptr || ! isMeterParameter (*param))
            continue;

        auto* meterProps = descriptor.NewPropertyMap();
        if (meterProps == nullptr)
            continue;

        meterProps->AddProperty (AAX_eProperty_Meter_Type, getMeterTypeFromParam (*param));
        meterProps->AddProperty (AAX_eProperty_Meter_Orientation, AAX_eMeterOrientation_TopRight);

        const auto meterID = static_cast<AAX_CTypeID> ('Mtr0' + static_cast<AAX_CTypeID> (meterIDs.size()));

        descriptor.AddMeterDescription (meterID, param->getName().toRawUTF8(), meterProps);

        meterIDs.push_back (meterID);
    }

    // YUP processors have a fixed bus layout, so a single component matching
    // the processor's channel configuration is registered. Instruments with no
    // audio input still process in place on the track stem.
    const auto numInCh = plugin->getBusLayout().getNumAudioInputChannels();
    const auto numOutCh = plugin->getBusLayout().getNumAudioOutputChannels();

    const auto outputFormat = stemFormatForChannelCount (numOutCh);
    const auto inputFormat = numInCh > 0
                               ? stemFormatForChannelCount (numInCh)
                               : outputFormat;

    jassert (inputFormat != AAX_eStemFormat_INT32_MAX);
    jassert (outputFormat != AAX_eStemFormat_INT32_MAX);

    auto* compDesc = descriptor.NewComponentDescriptor();
    if (compDesc == nullptr)
        return;

    aaxCheck (compDesc->AddAudioIn (fieldAudioIn));
    aaxCheck (compDesc->AddAudioOut (fieldAudioOut));
    aaxCheck (compDesc->AddAudioBufferLength (fieldBufferSize));
    aaxCheck (compDesc->AddDataInPort (fieldBypass, sizeof (int32_t)));

    if (plugin->acceptsMidi())
        aaxCheck (compDesc->AddMIDINode (fieldMidiIn, AAX_eMIDINodeType_LocalInput, "MIDI In", 0xFFFF));

    if (plugin->producesMidi())
        aaxCheck (compDesc->AddMIDINode (fieldMidiOut, AAX_eMIDINodeType_LocalOutput, "MIDI Out", 0xFFFF));

    aaxCheck (compDesc->AddPrivateData (fieldPluginInfo, sizeof (YupPluginInstanceInfo), AAX_ePrivateDataOptions_DefaultOptions));
    aaxCheck (compDesc->AddPrivateData (fieldPreparedFlag, sizeof (int32_t)));

    if (! meterIDs.empty())
        aaxCheck (compDesc->AddMeters (fieldMeterTaps, meterIDs.data(), static_cast<uint32_t> (meterIDs.size())));

    auto* properties = compDesc->NewPropertyMap();
    if (properties == nullptr)
        return;

    aaxCheck (properties->AddProperty (AAX_eProperty_ManufacturerID, static_cast<AAX_CPropertyValue> (YupPlugin_AAX_ManufacturerID)));
    aaxCheck (properties->AddProperty (AAX_eProperty_ProductID, static_cast<AAX_CPropertyValue> (YupPlugin_AAX_ProductID)));
    aaxCheck (properties->AddProperty (AAX_eProperty_PlugInID_Native, static_cast<AAX_CPropertyValue> (YupPlugin_AAX_PlugInID_Native)));
    aaxCheck (properties->AddProperty (AAX_eProperty_PlugInID_AudioSuite, static_cast<AAX_CPropertyValue> (YupPlugin_AAX_PlugInID_AudioSuite)));
    aaxCheck (properties->AddProperty (AAX_eProperty_InputStemFormat, inputFormat));
    aaxCheck (properties->AddProperty (AAX_eProperty_OutputStemFormat, outputFormat));
    aaxCheck (properties->AddProperty (AAX_eProperty_CanBypass, true));

    // Request the host-generated parameter GUI only when no editor is provided
    if (! plugin->hasEditor())
        aaxCheck (properties->AddProperty (AAX_eProperty_UsesClientGUI, true));

    aaxCheck (compDesc->AddProcessProc_Native (reinterpret_cast<AAX_CProcessProc> (yupAAXAlgorithmCallback), properties));

    aaxCheck (descriptor.AddComponent (compDesc));
}

} // namespace yup

//==============================================================================
// DLL export
//==============================================================================

AAX_Result GetEffectDescriptions (AAX_ICollection* collection)
{
    if (collection == nullptr)
        return AAX_ERROR_NULL_OBJECT;

    AAX_IEffectDescriptor* descriptor = collection->NewDescriptor();
    if (descriptor == nullptr)
        return AAX_ERROR_NULL_OBJECT;
    yup::getPlugInDescription (*descriptor);

    collection->AddEffect (YupPlugin_Id, descriptor);

    collection->SetManufacturerName (YupPlugin_Vendor);

    // At least one package name of 16 characters or less is required
    const yup::String pluginName (YupPlugin_Name);
    collection->AddPackageName (pluginName.toRawUTF8());
    if (pluginName.length() > 16)
        collection->AddPackageName (pluginName.substring (0, 16).toRawUTF8());

    collection->SetPackageVersion (1);

    return AAX_SUCCESS;
}
