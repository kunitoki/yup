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
#include <AAX_TransportTypes.h>
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
#include <numeric>
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

extern "C" yup::AudioProcessor* YUP_AUDIO_PLUGIN_CREATE_FUNCTION();

namespace yup
{

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
    int32_t* sideChainBuffers = nullptr;
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
    fieldSideChain = AAX_FIELD_INDEX (YupAlgorithmContext, sideChainBuffers),
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
// AAX stem formats
//==============================================================================

struct AAXStemLayout
{
    AAX_EStemFormat format;
    Array<AudioChannelSet::ChannelType> channels; // In AAX stream order
};

static const std::vector<AAXStemLayout>& getAAXStemLayouts()
{
    using C = AudioChannelSet;

    // Ordered so that the first entry with a given channel count is the
    // canonical layout used for discrete channel sets.
    static const std::vector<AAXStemLayout> layouts {
        { AAX_eStemFormat_Mono, { C::centre } },
        { AAX_eStemFormat_Stereo, { C::left, C::right } },
        { AAX_eStemFormat_LCR, { C::left, C::centre, C::right } },
        { AAX_eStemFormat_Quad, { C::left, C::right, C::leftSurround, C::rightSurround } },
        { AAX_eStemFormat_LCRS, { C::left, C::centre, C::right, C::surround } },
        { AAX_eStemFormat_5_0, { C::left, C::centre, C::right, C::leftSurround, C::rightSurround } },
        { AAX_eStemFormat_5_1, { C::left, C::centre, C::right, C::leftSurround, C::rightSurround, C::LFE } },
        { AAX_eStemFormat_6_0, { C::left, C::centre, C::right, C::leftSurround, C::centreSurround, C::rightSurround } },
        { AAX_eStemFormat_7_0_DTS, { C::left, C::centre, C::right, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear } },
        { AAX_eStemFormat_6_1, { C::left, C::centre, C::right, C::leftSurround, C::centreSurround, C::rightSurround, C::LFE } },
        { AAX_eStemFormat_7_0_SDDS, { C::left, C::leftCentre, C::centre, C::rightCentre, C::right, C::leftSurround, C::rightSurround } },
        { AAX_eStemFormat_5_0_2, { C::left, C::centre, C::right, C::leftSurround, C::rightSurround, C::topSideLeft, C::topSideRight } },
        { AAX_eStemFormat_7_1_DTS, { C::left, C::centre, C::right, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::LFE } },
        { AAX_eStemFormat_7_1_SDDS, { C::left, C::leftCentre, C::centre, C::rightCentre, C::right, C::leftSurround, C::rightSurround, C::LFE } },
        { AAX_eStemFormat_5_1_2, { C::left, C::centre, C::right, C::leftSurround, C::rightSurround, C::LFE, C::topSideLeft, C::topSideRight } },
        { AAX_eStemFormat_7_0_2, { C::left, C::centre, C::right, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::topSideLeft, C::topSideRight } },
        { AAX_eStemFormat_5_0_4, { C::left, C::centre, C::right, C::leftSurround, C::rightSurround, C::topFrontLeft, C::topFrontRight, C::topRearLeft, C::topRearRight } },
        { AAX_eStemFormat_7_1_2, { C::left, C::centre, C::right, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::LFE, C::topSideLeft, C::topSideRight } },
        { AAX_eStemFormat_5_1_4, { C::left, C::centre, C::right, C::leftSurround, C::rightSurround, C::LFE, C::topFrontLeft, C::topFrontRight, C::topRearLeft, C::topRearRight } },
        { AAX_eStemFormat_7_0_4, { C::left, C::centre, C::right, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::topFrontLeft, C::topFrontRight, C::topRearLeft, C::topRearRight } },
        { AAX_eStemFormat_7_1_4, { C::left, C::centre, C::right, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::LFE, C::topFrontLeft, C::topFrontRight, C::topRearLeft, C::topRearRight } },
        { AAX_eStemFormat_9_0_4, { C::left, C::centre, C::right, C::wideLeft, C::wideRight, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::topFrontLeft, C::topFrontRight, C::topRearLeft, C::topRearRight } },
        { AAX_eStemFormat_7_0_6, { C::left, C::centre, C::right, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::topFrontLeft, C::topFrontRight, C::topSideLeft, C::topSideRight, C::topRearLeft, C::topRearRight } },
        { AAX_eStemFormat_9_1_4, { C::left, C::centre, C::right, C::wideLeft, C::wideRight, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::LFE, C::topFrontLeft, C::topFrontRight, C::topRearLeft, C::topRearRight } },
        { AAX_eStemFormat_7_1_6, { C::left, C::centre, C::right, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::LFE, C::topFrontLeft, C::topFrontRight, C::topSideLeft, C::topSideRight, C::topRearLeft, C::topRearRight } },
        { AAX_eStemFormat_9_0_6, { C::left, C::centre, C::right, C::wideLeft, C::wideRight, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::topFrontLeft, C::topFrontRight, C::topSideLeft, C::topSideRight, C::topRearLeft, C::topRearRight } },
        { AAX_eStemFormat_9_1_6, { C::left, C::centre, C::right, C::wideLeft, C::wideRight, C::leftSurroundSide, C::rightSurroundSide, C::leftSurroundRear, C::rightSurroundRear, C::LFE, C::topFrontLeft, C::topFrontRight, C::topSideLeft, C::topSideRight, C::topRearLeft, C::topRearRight } },
    };

    return layouts;
}

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

// Picks the stem with exactly the speakers of the set. Discrete sets fall back
// to the first stem with the same channel count and are passed in stream order.
static AAX_EStemFormat stemFormatForChannelSet (const AudioChannelSet& set)
{
    if (set.isDisabled())
        return AAX_eStemFormat_None;

    if (const auto order = set.getAmbisonicOrder(); order >= 0)
        return stemFormatForAmbisonicOrder (order);

    const auto& layouts = getAAXStemLayouts();

    for (const auto& layout : layouts)
        if (AudioChannelSet::channelSetWithChannels (layout.channels) == set)
            return layout.format;

    for (const auto& layout : layouts)
        if (layout.channels.size() == set.size())
            return layout.format;

    if (const auto order = AudioChannelSet::getAmbisonicOrderForNumChannels (set.size()); order != -1)
        return stemFormatForAmbisonicOrder (order);

    return AAX_eStemFormat_INT32_MAX;
}

// Returns, for each processor channel, the AAX stream channel it maps to.
// Formats without a speaker mapping (ambisonics, discrete sets) map one to one.
static std::vector<int> getStreamChannelOrder (AAX_EStemFormat format, const AudioChannelSet& set)
{
    std::vector<int> order (static_cast<size_t> (set.size()));
    std::iota (order.begin(), order.end(), 0);

    const auto& layouts = getAAXStemLayouts();
    const auto found = std::find_if (layouts.begin(), layouts.end(), [format] (const auto& l)
    {
        return l.format == format;
    });

    if (found == layouts.end() || found->channels.size() != set.size())
        return order;

    std::vector<int> mapped (order.size(), -1);

    for (int stream = 0; stream < set.size(); ++stream)
    {
        const auto index = set.getChannelIndexForType (found->channels.getUnchecked (stream));
        if (index < 0 || mapped[static_cast<size_t> (index)] >= 0)
            return order;

        mapped[static_cast<size_t> (index)] = stream;
    }

    return mapped;
}

// The first main audio bus carries the AAX stem, returns a disabled set if missing.
static AudioChannelSet getMainAudioChannelSet (Span<const AudioBus> buses)
{
    for (const auto& bus : buses)
        if (bus.getType() == AudioBus::Type::Audio && bus.getRole() == AudioBus::Role::Main)
            return bus.getChannelSet();

    return AudioChannelSet::disabled();
}

//==============================================================================
// Meter type mapping
//==============================================================================

static AAX_EMeterType getMeterTypeFromParam (const AudioParameter& param)
{
    switch (param.getCategory())
    {
        case AudioParameter::Category::InputMeter:
            return AAX_eMeterType_Input;
        case AudioParameter::Category::OutputMeter:
            return AAX_eMeterType_Output;
        case AudioParameter::Category::CompressorLimiterGainReductionMeter:
            return AAX_eMeterType_CLGain;
        case AudioParameter::Category::ExpanderGateGainReductionMeter:
            return AAX_eMeterType_EGGain;
        case AudioParameter::Category::AnalysisMeter:
            return AAX_eMeterType_Analysis;
        default:
            return AAX_eMeterType_Other;
    }
}

//==============================================================================
// Transport play head
//==============================================================================

class YupAAXPlayHead final : public AudioPlayHead
{
public:
    void prepare (const AAX_ITransport* newTransport, double newSampleRate) noexcept
    {
        transport = newTransport;
        sampleRate = newSampleRate;
    }

    void setRecording (bool shouldBeRecording) noexcept { recording.store (shouldBeRecording); }

    std::optional<PositionInfo> getPosition() const override
    {
        if (transport == nullptr)
            return {};

        PositionInfo info;

        if (double bpm = 0.0; transport->GetCurrentTempo (&bpm) == AAX_SUCCESS)
            info.setBpm (bpm);

        std::optional<TimeSignature> signature;
        if (int32_t num = 4, den = 4; transport->GetCurrentMeter (&num, &den) == AAX_SUCCESS)
            signature = TimeSignature { static_cast<int> (num), static_cast<int> (den) };

        info.setTimeSignature (signature);

        bool isPlaying = false;
        info.setIsPlaying (transport->IsTransportPlaying (&isPlaying) == AAX_SUCCESS && isPlaying);
        info.setIsRecording (recording.load());

        std::optional<int64> timeInSamples;
        if (int64_t samples = 0; (! info.getIsPlaying() && transport->GetTimelineSelectionStartPosition (&samples) == AAX_SUCCESS)
                                 || transport->GetCurrentNativeSampleLocation (&samples) == AAX_SUCCESS)
            timeInSamples = static_cast<int64> (samples);

        info.setTimeInSamples (timeInSamples);

        if (timeInSamples.has_value() && sampleRate > 0.0)
            info.setTimeInSeconds (static_cast<double> (*timeInSamples) / sampleRate);

        std::optional<int64> ticks;
        if (int64_t t = 0; (info.getIsPlaying() && transport->GetCustomTickPosition (&t, timeInSamples.value_or (0)) == AAX_SUCCESS)
                           || transport->GetCurrentTickPosition (&t) == AAX_SUCCESS)
            ticks = static_cast<int64> (t);

        if (ticks.has_value())
            info.setPpqPosition (static_cast<double> (*ticks) / ticksPerQuarter);

        bool isLooping = false;
        int64_t loopStart = 0, loopEnd = 0;
        if (transport->GetCurrentLoopPosition (&isLooping, &loopStart, &loopEnd) == AAX_SUCCESS)
        {
            info.setIsLooping (isLooping);
            info.setLoopPoints (LoopPoints { static_cast<double> (loopStart) / ticksPerQuarter,
                                             static_cast<double> (loopEnd) / ticksPerQuarter });
        }

        const auto frameRate = getFrameRate();
        if (frameRate.has_value())
        {
            info.setFrameRate (frameRate->first);

            const auto effectiveRate = frameRate->first.getEffectiveRate();
            const auto offset = static_cast<double> (frameRate->second);
            info.setEditOriginTime (effectiveRate != 0.0 ? offset / effectiveRate : offset);
        }

        int32_t bars = 0, beats = 0;
        int64_t displayTicks = 0;
        if (timeInSamples.has_value()
            && transport->GetBarBeatPosition (&bars, &beats, &displayTicks, *timeInSamples) == AAX_SUCCESS)
        {
            info.setBarCount (bars);

            if (signature.has_value() && ticks.has_value() && signature->denominator > 0)
            {
                const auto ticksSinceBar = static_cast<int64> (((beats - 1) * 4 * static_cast<int64> (ticksPerQuarter)) / signature->denominator) + displayTicks;

                if (ticksSinceBar <= *ticks)
                    info.setPpqPositionOfLastBarStart (static_cast<double> (*ticks - ticksSinceBar) / ticksPerQuarter);
            }
        }

        return info;
    }

private:
    std::optional<std::pair<FrameRate, int64>> getFrameRate() const
    {
        AAX_EFrameRate rate {};
        int64_t offset = 0;

        if (transport->GetHDTimeCodeInfo (&rate, &offset) != AAX_SUCCESS)
        {
            int32_t offset32 = 0;
            if (transport->GetTimeCodeInfo (&rate, &offset32) != AAX_SUCCESS)
                return {};

            offset = offset32;
        }

        // Raw values, because not all AAX SDK versions declare every enumerator
        const auto base = FrameRate();
        std::optional<FrameRate> result;

        switch (static_cast<int> (rate))
        {
            case 1: result = base.withBaseRate (24); break;
            case 7: result = base.withBaseRate (24).withPullDown(); break;
            case 2: result = base.withBaseRate (25); break;
            case 5: result = base.withBaseRate (30); break;
            case 6: result = base.withBaseRate (30).withDrop(); break;
            case 3: result = base.withBaseRate (30).withPullDown(); break;
            case 4: result = base.withBaseRate (30).withPullDown().withDrop(); break;
            case 9: result = base.withBaseRate (48); break;
            case 8: result = base.withBaseRate (48).withPullDown(); break;
            case 10: result = base.withBaseRate (50); break;
            case 13: result = base.withBaseRate (60); break;
            case 14: result = base.withBaseRate (60).withDrop(); break;
            case 11: result = base.withBaseRate (60).withPullDown(); break;
            case 12: result = base.withBaseRate (60).withPullDown().withDrop(); break;
            case 15: result = base.withBaseRate (100); break;
            case 18: result = base.withBaseRate (120); break;
            case 19: result = base.withBaseRate (120).withDrop(); break;
            case 16: result = base.withBaseRate (120).withPullDown(); break;
            case 17: result = base.withBaseRate (120).withPullDown().withDrop(); break;
            default: break;
        }

        if (! result.has_value())
            return {};

        return std::make_pair (*result, static_cast<int64> (offset));
    }

    static constexpr double ticksPerQuarter = 960000.0;

    const AAX_ITransport* transport = nullptr;
    double sampleRate = 44100.0;
    std::atomic<bool> recording { false };
};

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
        processor.reset (yup::createPluginProcessorForWrapper (yup::AudioPluginWrapperType::aax, ::YUP_AUDIO_PLUGIN_CREATE_FUNCTION));

        if (processor == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "createPluginProcessorForWrapper returned null");
            return;
        }

        processor->addListener (this);
        bypass.attach (*processor);

        AAX_CEffectParameters::GetNumberOfChunks (&yupChunkIndex);
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "created effect parameters: wrapper=" << describePointer (this) << ", processor=" << describePointer (processor.get()) << ", chunkIndex=" << String (yupChunkIndex));
    }

    ~YupAAX_Processor() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "destroying effect parameters: wrapper=" << describePointer (this) << ", processor=" << describePointer (processor.get()));

        if (processor == nullptr)
            return;

        processor->removeListener (this);
        endActiveParameterGestures (processor.get());
    }

    //==========================================================================
    // AAX_CEffectParameters - Initialization
    //==========================================================================

    AAX_Result Uninitialize() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "Uninitialize requested: wrapper=" << describePointer (this) << ", isPrepared=" << String (isPrepared ? "true" : "false"));

        if (isPrepared && processor != nullptr)
        {
            isPrepared = false;
            processor->releaseResources();
        }

        return AAX_CEffectParameters::Uninitialize();
    }

    AAX_Result EffectInit() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "EffectInit requested: wrapper=" << describePointer (this));

        auto* ctrl = Controller();
        if (ctrl == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "EffectInit failed: controller is null");
            return AAX_ERROR_NULL_OBJECT;
        }

        AAX_CSampleRate sampleRate = 0;
        ctrl->GetSampleRate (&sampleRate);

        AAX_EStemFormat inputFormat = AAX_eStemFormat_None;
        AAX_EStemFormat outputFormat = AAX_eStemFormat_None;
        ctrl->GetInputStemFormat (&inputFormat);
        ctrl->GetOutputStemFormat (&outputFormat);

        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "EffectInit host config: sampleRate=" << String (static_cast<double> (sampleRate)) << ", inputStemFormat=" << String (static_cast<int> (inputFormat)) << ", outputStemFormat=" << String (static_cast<int> (outputFormat)));

        auto err = preparePlugin (static_cast<float> (sampleRate), inputFormat, outputFormat);
        if (err != AAX_SUCCESS)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "EffectInit failed: preparePlugin result=" << String (static_cast<int> (err)));
            return err;
        }

        addAudioProcessorParameters();

        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "EffectInit completed");
        return AAX_SUCCESS;
    }

    //==========================================================================
    // AAX_CEffectParameters - Parameters
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
    // AAX_CEffectParameters - State reset
    //==========================================================================

    AAX_Result ResetFieldData (AAX_CFieldIndex fieldIndex, void* data, uint32_t dataSize) const override
    {
        switch (fieldIndex)
        {
            case fieldPluginInfo:
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "ResetFieldData plugin info: wrapper=" << describePointer (this) << ", data=" << describePointer (data) << ", dataSize=" << String (static_cast<int> (dataSize)));

                const auto numObjects = dataSize / sizeof (YupPluginInstanceInfo);

                auto* objects = static_cast<YupPluginInstanceInfo*> (data);
                for (size_t i = 0; i < numObjects; ++i)
                    new (objects + i) YupPluginInstanceInfo (const_cast<YupAAX_Processor&> (*this));

                break;
            }

            case fieldPreparedFlag:
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "ResetFieldData prepared flag: wrapper=" << describePointer (this) << ", dataSize=" << String (static_cast<int> (dataSize)));

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
    // AAX_CEffectParameters - Chunks
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
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetChunkSize requested");

            MemoryBlock processorState;
            const auto result = processor->saveStateIntoMemory (processorState);

            if (result.failed())
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetChunkSize saving processor state failed: " << result.getErrorMessage());
            }

            auto& tls = chunkData.get();
            tls.data = writeWrapperBypassState (
                YupPlugin_AAX_ChunkMagic,
                YupPlugin_AAX_ChunkVersion,
                bypass.isBypassed(),
                processorState,
                ! processorState.isEmpty());
            tls.isValid = true;

            *size = static_cast<uint32_t> (tls.data.getSize());

            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetChunkSize completed: bytes=" << String (static_cast<int64> (*size)) << ", processorBytes=" << String (static_cast<int64> (processorState.getSize())) << ", bypassed=" << String (bypass.isBypassed() ? "true" : "false"));
            return AAX_SUCCESS;
        }

        return AAX_CEffectParameters::GetChunkSize (chunkID, size);
    }

    AAX_Result GetChunk (AAX_CTypeID chunkID, AAX_SPlugInChunk* chunk) const override
    {
        if (chunkID == YupPlugin_AAX_ChunkID)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetChunk requested: chunk=" << describePointer (chunk));

            auto& tls = chunkData.get();
            if (! tls.isValid)
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetChunk failed: GetChunkSize was not called on this thread");
                return 20700; // AAX_ERROR_PLUGIN_API_INVALID_THREAD
            }

            // The host allocates the chunk using the size returned by
            // GetChunkSize, fData is a variable length trailing array
            const auto size = static_cast<int32_t> (tls.data.getSize());
            chunk->fSize = size;

            if (size > 0 && tls.data.getData() != nullptr)
                tls.data.copyTo (chunk->fData, 0, static_cast<size_t> (size));

            tls.isValid = false;

            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetChunk completed: bytes=" << String (size));
            return AAX_SUCCESS;
        }

        return AAX_CEffectParameters::GetChunk (chunkID, chunk);
    }

    AAX_Result SetChunk (AAX_CTypeID chunkID, const AAX_SPlugInChunk* chunk) override
    {
        if (chunkID == YupPlugin_AAX_ChunkID)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "SetChunk requested: bytes=" << String (static_cast<int> (chunk->fSize)));

            MemoryBlock rawData (chunk->fData, static_cast<size_t> (chunk->fSize));
            auto state = readWrapperBypassState (rawData, YupPlugin_AAX_ChunkMagic, YupPlugin_AAX_ChunkVersion);

            if (state.hasWrapperState)
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "SetChunk wrapper state: bypassed=" << String (state.isBypassed ? "true" : "false") << ", hasProcessorState=" << String (state.hasProcessorState ? "true" : "false") << ", processorBytes=" << String (static_cast<int64> (state.processorState.getSize())));

                setBypassed (state.isBypassed);
                SetParameterNormalizedValue (cDefaultMasterBypassID, state.isBypassed ? 1.0 : 0.0);

                if (state.hasProcessorState && ! state.processorState.isEmpty())
                {
                    const auto result = processor->loadStateFromMemory (state.processorState);
                    ignoreUnused (result);

                    YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "SetChunk processor state " << (result.wasOk() ? String ("loaded") : "failed: " + result.getErrorMessage()));
                }
            }
            else
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "SetChunk without wrapper state, loading raw processor state");

                const auto result = processor->loadStateFromMemory (rawData);
                ignoreUnused (result);

                YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "SetChunk processor state " << (result.wasOk() ? String ("loaded") : "failed: " + result.getErrorMessage()));
            }

            resyncParameterValues();

            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "SetChunk completed");
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
    // AAX_CEffectParameters - Notifications
    //==========================================================================

    AAX_Result NotificationReceived (AAX_CTypeID notificationType, const void* notificationData, uint32_t notificationDataSize) override
    {
        if (notificationType == AAX_eNotificationEvent_EnteringOfflineMode)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "entering offline mode");
            processor->setOfflineProcessing (true);
        }
        else if (notificationType == AAX_eNotificationEvent_ExitingOfflineMode)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "exiting offline mode");
            processor->setOfflineProcessing (false);
        }
        else if (notificationType == AAX_eNotificationEvent_TransportStateChanged
                 && notificationData != nullptr
                 && notificationDataSize >= sizeof (AAX_TransportStateInfo_V1))
        {
            playHead.setRecording (static_cast<const AAX_TransportStateInfo_V1*> (notificationData)->mIsRecording);
        }

        return AAX_CEffectParameters::NotificationReceived (notificationType, notificationData, notificationDataSize);
    }

    //==========================================================================
    // Audio processing
    //==========================================================================

    void buildBusViews (float* const* inputChannelData, float* const* outputChannelData, int sideChainIndex, int bufferSize)
    {
        inputBusViews.clear();
        outputBusViews.clear();

        const auto& layout = processor->getBusLayout();
        const float* const* mainInputs = nullptr;
        int numMainInputs = 0;
        bool sideChainAssigned = false;
        int offset = 0;

        for (const auto& bus : layout.getInputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            const int numCh = bus.getNumChannels();
            auto* chPtrs = inputChannelPtrStorage.data() + offset;
            const bool isMain = bus.getRole() == AudioBus::Role::Main && mainInputs == nullptr;
            const bool isSideChain = bus.getRole() == AudioBus::Role::Auxiliary && ! sideChainAssigned;

            for (int ch = 0; ch < numCh; ++ch)
            {
                const float* channel = nullptr;

                if (inputChannelData != nullptr && isMain)
                    channel = ch < static_cast<int> (inputStreamOrder.size()) ? inputChannelData[inputStreamOrder[static_cast<size_t> (ch)]] : nullptr;
                else if (inputChannelData != nullptr && isSideChain && sideChainIndex > 0)
                    channel = inputChannelData[sideChainIndex];

                chPtrs[ch] = channel;
            }

            if (isMain)
            {
                mainInputs = chPtrs;
                numMainInputs = numCh;
            }

            sideChainAssigned = sideChainAssigned || isSideChain;
            inputBusViews.emplace_back (chPtrs, numCh, bus.getRole());
            offset += numCh;
        }

        bool mainOutputAssigned = false;
        int scratchChannel = 0;
        offset = 0;

        for (const auto& bus : layout.getOutputBuses())
        {
            if (bus.getType() != AudioBus::Type::Audio)
                continue;

            const int numCh = bus.getNumChannels();
            auto* chPtrs = outputChannelPtrStorage.data() + offset;
            const bool isMain = bus.getRole() == AudioBus::Role::Main && ! mainOutputAssigned;

            for (int ch = 0; ch < numCh; ++ch)
            {
                float* channel = isMain && ch < static_cast<int> (outputStreamOrder.size())
                                   ? outputChannelData[outputStreamOrder[static_cast<size_t> (ch)]]
                                   : scratchOutputs.getWritePointer (scratchChannel++);

                if (isMain && ch < numMainInputs && mainInputs[ch] != nullptr)
                {
                    if (channel != mainInputs[ch])
                        std::memcpy (channel, mainInputs[ch], static_cast<size_t> (bufferSize) * sizeof (float));
                }
                else
                {
                    std::memset (channel, 0, static_cast<size_t> (bufferSize) * sizeof (float));
                }

                chPtrs[ch] = channel;
            }

            mainOutputAssigned = mainOutputAssigned || isMain;
            outputBusViews.emplace_back (chPtrs, numCh, bus.getRole());
            offset += numCh;
        }
    }

    void process (float* const* inputChannelData,
                  float* const* outputChannelData,
                  int sideChainIndex,
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

        buildBusViews (inputChannelData, outputChannelData, sideChainIndex, bufferSize);

        AudioBuffer<float> audioBuffer (outputChannelPtrStorage.data(),
                                        static_cast<int> (outputChannelPtrStorage.size()),
                                        bufferSize);

        paramChangeBuffer.clear();
        playHead.prepare (Transport(), lastSampleRate);

        AudioProcessContext<float> context {
            audioBuffer,
            midiBuffer,
            paramChangeBuffer,
            &playHead,
            { inputBusViews.data(), inputBusViews.size() },
            { outputBusViews.data(), outputBusViews.size() }
        };

        processAudioBlock (*processor, context, currentlyBypassed);

        if (processor->producesMidi() && midiNodeOut != nullptr)
            fillAaxMidiNodeFromBuffer (*midiNodeOut, midiBuffer);

        if (! aaxMeters.empty() && meterTapBuffers != nullptr)
            extractMeterValues (*meterTapBuffers);
    }

    AudioProcessor& getAudioProcessor() noexcept { return *processor; }

    void ensureWindowingInitialised() { windowing.ensureInitialised(); }

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

        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "preparing processor: sampleRate=" << String (static_cast<double> (sampleRate)) << ", maxBlockSize=" << String (maxAaxBufferSize) << ", inputStemFormat=" << String (static_cast<int> (inputFormat)) << ", outputStemFormat=" << String (static_cast<int> (outputFormat)) << ", inCh=" << String (processor->getBusLayout().getNumAudioInputChannels()) << ", outCh=" << String (processor->getBusLayout().getNumAudioOutputChannels()));

        processor->setPlaybackConfiguration (sampleRate, maxAaxBufferSize);
        isPrepared = true;

        paramChangeBuffer.reserve (getDefaultParameterChangeCapacity (*processor));

        // Pre-allocate per-bus view storage so the algorithm callback never allocates
        inputBusViews.reserve (static_cast<size_t> (processor->getNumAudioInputs()));
        outputBusViews.reserve (static_cast<size_t> (processor->getNumAudioOutputs()));
        inputChannelPtrStorage.resize (static_cast<size_t> (processor->getBusLayout().getNumAudioInputChannels()));
        outputChannelPtrStorage.resize (static_cast<size_t> (processor->getBusLayout().getNumAudioOutputChannels()));

        const auto& layout = processor->getBusLayout();
        const auto mainOutputSet = getMainAudioChannelSet (layout.getOutputBuses());
        inputStreamOrder = getStreamChannelOrder (inputFormat, getMainAudioChannelSet (layout.getInputBuses()));
        outputStreamOrder = getStreamChannelOrder (outputFormat, mainOutputSet);
        scratchOutputs.setSize (jmax (0, layout.getNumAudioOutputChannels() - mainOutputSet.size()), maxAaxBufferSize);

        if (auto* ctrl = Controller())
            ctrl->SetSignalLatency (processor->getLatencySamples());

        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "processor prepared: latency=" << String (processor->getLatencySamples()));

        return AAX_SUCCESS;
    }

    void addAudioProcessorParameters()
    {
        paramMap.clear();
        aaxMeters.clear();

        addMasterBypassParameter();

        const auto parameters = processor->getParameters();
        const auto bypassParameter = processor->getBypassParameter();

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            auto* param = parameters[i].get();

            if (param == nullptr)
                continue;

            // The processor bypass parameter is driven by the host master bypass
            if (param == bypassParameter.get())
            {
                paramMap[getAaxParamHash (cDefaultMasterBypassID)] = param;
                continue;
            }

            if (param->isMeter())
            {
                aaxMeters.push_back (param);
                continue;
            }

            const auto aaxParamID = getAaxParamIDFromIndex (static_cast<int> (i));
            addAaxParameter (*param, aaxParamID);

            const auto hash = getAaxParamHash (aaxParamID.toRawUTF8());
            paramMap[hash] = param;
        }

        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "registered parameters: processorParameters=" << String (static_cast<int> (parameters.size())) << ", aaxParameters=" << String (static_cast<int> (paramMap.size())) << ", meters=" << String (static_cast<int> (aaxMeters.size())));
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
        const auto bypassParameter = processor->getBypassParameter();

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            auto* param = parameters[i].get();

            if (param == nullptr || param == bypassParameter.get() || param->isMeter())
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
        const auto bypassParameter = processor->getBypassParameter();

        for (size_t i = 0; i < parameters.size(); ++i)
        {
            if (parameters[i] == bypassParameter)
                continue;

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
        bypass.setBypassed (shouldBypass);
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
    WrapperWindowing windowing;

    std::unique_ptr<AudioProcessor> processor;
    int32_t yupChunkIndex = 0;
    float lastSampleRate = 44100.0f;
    YupAAXPlayHead playHead;
    AAX_EStemFormat lastInputFormat = AAX_eStemFormat_None;
    AAX_EStemFormat lastOutputFormat = AAX_eStemFormat_None;
    bool isPrepared = false;
    WrapperBypass bypass;
    mutable std::atomic<int> numSetDirtyCalls { 0 };

    MidiBuffer midiBuffer;
    ParameterChangeBuffer paramChangeBuffer;

    // Per-bus view state, pre-allocated in preparePlugin so the real-time
    // algorithm callback only clears and refills the vectors without allocating
    std::vector<AudioBusBufferView<const float>> inputBusViews;
    std::vector<AudioBusBufferView<float>> outputBusViews;
    std::vector<const float*> inputChannelPtrStorage;
    std::vector<float*> outputChannelPtrStorage;
    AudioBuffer<float> scratchOutputs;
    std::vector<int> inputStreamOrder;
    std::vector<int> outputStreamOrder;

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

    YupAAX_GUI()
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "created effect GUI: gui=" << describePointer (this));
    }

    ~YupAAX_GUI() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "destroying effect GUI: gui=" << describePointer (this) << ", editor=" << describePointer (editorComponent.get()));
        DeleteViewContainer();
    }

    //==========================================================================
    // AAX_CEffectGUI overrides (all return void!)
    //==========================================================================

    void CreateViewContents() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "CreateViewContents requested: gui=" << describePointer (this));

        auto* params = GetEffectParameters();
        if (params == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "CreateViewContents failed: effect parameters are null");
            return;
        }

        auto* yupParams = dynamic_cast<YupAAX_Processor*> (params);
        if (yupParams == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "CreateViewContents failed: effect parameters are not a YUP processor");
            return;
        }

        if (editorComponent != nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "CreateViewContents skipped: editor already exists");
            return;
        }

        yupParams->ensureWindowingInitialised();
        auto& audioProcessor = yupParams->getAudioProcessor();

        if (audioProcessor.hasEditor())
        {
            editorComponent.reset (audioProcessor.createEditor());

            if (editorComponent == nullptr)
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "processor returned null editor");
            }
            else
            {
                YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "created editor: preferredSize=" << editorComponent->getPreferredSize().toString() << ", editor=" << describePointer (editorComponent.get()));
            }
        }
        else
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "processor has no editor");
        }
    }

    void CreateViewContainer() override
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "CreateViewContainer requested: gui=" << describePointer (this) << ", editor=" << describePointer (editorComponent.get()));

        if (editorComponent == nullptr)
            CreateViewContents();

        if (editorComponent == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "CreateViewContainer failed: editor is null");
            return;
        }

        auto* viewContainer = GetViewContainer();
        if (viewContainer == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "CreateViewContainer failed: view container is null");
            return;
        }

        auto* nativeView = viewContainer->GetPtr();
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "view container: type=" << String (static_cast<int> (viewContainer->GetType())) << ", nativeView=" << describePointer (nativeView));

        if (nativeView == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "CreateViewContainer failed: native view is null");
            return;
        }

        ComponentNative::Flags flags =
            ComponentNative::defaultFlags & ~ComponentNative::decoratedWindow;

        if (editorComponent->shouldRenderContinuous())
            flags.set (ComponentNative::renderContinuous);

        auto options = ComponentNative::Options()
                           .withFlags (flags)
                           .withResizableWindow (editorComponent->isResizable());

        const auto initialSize = editorComponent->getConstrainedSize (editorComponent->getInitialSize());
        editorComponent->setSize ({ static_cast<float> (initialSize.getWidth()),
                                    static_cast<float> (initialSize.getHeight()) });

        editorComponent->addToDesktop (options, nativeView);
        editorComponent->setVisible (true);
        editorComponent->attachedToNative();

        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "editor attached to native view: bounds=" << editorComponent->getBounds().toString() << ", resizable=" << String (editorComponent->isResizable() ? "true" : "false") << ", renderContinuous=" << String (editorComponent->shouldRenderContinuous() ? "true" : "false") << ", isOnDesktop=" << String (editorComponent->isOnDesktop() ? "true" : "false"));
    }

    void DeleteViewContainer() override
    {
        if (editorComponent != nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "DeleteViewContainer destroying editor: editor=" << describePointer (editorComponent.get()));

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
                const auto initialSize = editorComponent->getConstrainedSize (editorComponent->getInitialSize());
                size->horz = static_cast<float> (initialSize.getWidth());
                size->vert = static_cast<float> (initialSize.getHeight());
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
            ctx->sideChainBuffers != nullptr ? static_cast<int> (*ctx->sideChainBuffers) : -1,
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
    std::unique_ptr<AudioProcessor> plugin (yup::createPluginProcessorForWrapper (yup::AudioPluginWrapperType::aax, ::YUP_AUDIO_PLUGIN_CREATE_FUNCTION));

    YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "describing plugin: processor=" << describePointer (plugin.get()) << ", hasEditor=" << String (plugin != nullptr && plugin->hasEditor() ? "true" : "false"));

    if (plugin == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "createPluginProcessorForWrapper returned null");
        return;
    }

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
        if (param == nullptr || ! param->isMeter())
            continue;

        auto* meterProps = descriptor.NewPropertyMap();
        if (meterProps == nullptr)
        {
            YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "skipping meter: property map is null, name=" << param->getName());
            continue;
        }

        meterProps->AddProperty (AAX_eProperty_Meter_Type, getMeterTypeFromParam (*param));
        meterProps->AddProperty (AAX_eProperty_Meter_Orientation, AAX_eMeterOrientation_TopRight);

        const auto meterID = static_cast<AAX_CTypeID> ('Mtr0' + static_cast<AAX_CTypeID> (meterIDs.size()));

        descriptor.AddMeterDescription (meterID, param->getName().toRawUTF8(), meterProps);

        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "registered meter: id=" << String (static_cast<int64> (meterID)) << ", name=" << param->getName());

        meterIDs.push_back (meterID);
    }

    // YUP processors have a fixed bus layout, so a single component matching
    // the main buses is registered. Instruments with no audio input still
    // process in place on the track stem. Other auxiliary buses map to the
    // mono sidechain input, remaining outputs are rendered but not routed.
    const auto& busLayout = plugin->getBusLayout();
    const auto mainInputSet = getMainAudioChannelSet (busLayout.getInputBuses());
    const auto mainOutputSet = getMainAudioChannelSet (busLayout.getOutputBuses());
    const auto numInCh = mainInputSet.size();
    const auto numOutCh = mainOutputSet.size();

    const auto outputFormat = stemFormatForChannelSet (mainOutputSet);
    const auto inputFormat = numInCh > 0
                               ? stemFormatForChannelSet (mainInputSet)
                               : outputFormat;

    const auto hasSideChain = std::any_of (busLayout.getInputBuses().begin(), busLayout.getInputBuses().end(), [] (const AudioBus& bus)
    {
        return bus.getType() == AudioBus::Type::Audio && bus.getRole() == AudioBus::Role::Auxiliary && bus.getNumChannels() > 0;
    });

    jassert (inputFormat != AAX_eStemFormat_INT32_MAX);
    jassert (outputFormat != AAX_eStemFormat_INT32_MAX);

    YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "stem formats: inCh=" << String (numInCh) << ", outCh=" << String (numOutCh) << ", inputStemFormat=" << String (static_cast<int> (inputFormat)) << ", outputStemFormat=" << String (static_cast<int> (outputFormat)) << ", acceptsMidi=" << String (plugin->acceptsMidi() ? "true" : "false") << ", producesMidi=" << String (plugin->producesMidi() ? "true" : "false"));

    if (inputFormat == AAX_eStemFormat_INT32_MAX || outputFormat == AAX_eStemFormat_INT32_MAX)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "unsupported channel count for AAX stem format");
    }

    auto* compDesc = descriptor.NewComponentDescriptor();
    if (compDesc == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "describing plugin failed: component descriptor is null");
        return;
    }

    aaxCheck (compDesc->AddAudioIn (fieldAudioIn));
    aaxCheck (compDesc->AddAudioOut (fieldAudioOut));
    aaxCheck (compDesc->AddAudioBufferLength (fieldBufferSize));
    aaxCheck (compDesc->AddDataInPort (fieldBypass, sizeof (int32_t)));

    if (hasSideChain)
        aaxCheck (compDesc->AddSideChainIn (fieldSideChain));

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
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "describing plugin failed: component property map is null");
        return;
    }

    YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "plugin ids: manufacturer=" << String (static_cast<int64> (YupPlugin_AAX_ManufacturerID)) << ", product=" << String (static_cast<int64> (YupPlugin_AAX_ProductID)) << ", native=" << String (static_cast<int64> (YupPlugin_AAX_PlugInID_Native)) << ", audioSuite=" << String (static_cast<int64> (YupPlugin_AAX_PlugInID_AudioSuite)));

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

    YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "described plugin: meters=" << String (static_cast<int> (meterIDs.size())) << ", usesClientGUI=" << String (plugin->hasEditor() ? "false" : "true"));
}

} // namespace yup

//==============================================================================
// DLL export
//==============================================================================

AAX_Result GetEffectDescriptions (AAX_ICollection* collection)
{
    YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetEffectDescriptions requested: collection=" << yup::describePointer (collection));

    if (collection == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetEffectDescriptions failed: collection is null");
        return AAX_ERROR_NULL_OBJECT;
    }

    AAX_IEffectDescriptor* descriptor = collection->NewDescriptor();
    if (descriptor == nullptr)
    {
        YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetEffectDescriptions failed: descriptor is null");
        return AAX_ERROR_NULL_OBJECT;
    }
    yup::getPlugInDescription (*descriptor);

    collection->AddEffect (YupPlugin_Id, descriptor);

    collection->SetManufacturerName (YupPlugin_Vendor);

    // At least one package name of 16 characters or less is required
    const yup::String pluginName (YupPlugin_Name);
    collection->AddPackageName (pluginName.toRawUTF8());
    if (pluginName.length() > 16)
        collection->AddPackageName (pluginName.substring (0, 16).toRawUTF8());

    collection->SetPackageVersion (1);

    YUP_MODULE_DBG (PLUGIN_CLIENT_AAX, "GetEffectDescriptions completed: id=" << yup::String (YupPlugin_Id) << ", vendor=" << yup::String (YupPlugin_Vendor) << ", package=" << pluginName);

    return AAX_SUCCESS;
}
