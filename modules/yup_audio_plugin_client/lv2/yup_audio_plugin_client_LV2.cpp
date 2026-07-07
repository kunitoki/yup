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

#include "../common/yup_AudioPluginEntryPoint.h"
#include "../common/yup_AudioPluginUtilities.h"

#if ! defined(YUP_AUDIO_PLUGIN_ENABLE_LV2)
#error "YUP_AUDIO_PLUGIN_ENABLE_LV2 must be defined"
#endif

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>
#include <lv2/state/state.h>
#include <lv2/options/options.h>
#include <lv2/parameters/parameters.h>
#include <lv2/patch/patch.h>
#include <lv2/time/time.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/worker/worker.h>
#include <lv2/urid/urid.h>
#include <lv2/ui/ui.h>

#include <atomic>
#include <cstring>
#include <map>
#include <vector>

//==============================================================================

extern "C" yup::AudioProcessor* createPluginProcessor();

namespace yup
{

//==============================================================================

static constexpr int lv2WrapperStateMagic = 0x564c5932; // "YLV2"
static constexpr int lv2WrapperStateVersion = 1;

//==============================================================================

struct UridMap
{
    LV2_URID_Map map;
    LV2_URID_Unmap unmap;
    std::map<String, LV2_URID> uriToUrid;
    std::map<LV2_URID, String> uridToUri;

    UridMap()
    {
        map.handle = this;
        map.map = [] (LV2_URID_Map_Handle handle, const char* uri) -> LV2_URID
        {
            auto* self = static_cast<UridMap*> (handle);
            return self->mapUri (uri);
        };

        unmap.handle = this;
        unmap.unmap = [] (LV2_URID_Unmap_Handle handle, LV2_URID urid) -> const char*
        {
            auto* self = static_cast<UridMap*> (handle);
            return self->unmapUri (urid);
        };
    }

    LV2_URID mapUri (const char* uri)
    {
        if (uri == nullptr)
            return 0;

        String uriStr (uri);

        auto it = uriToUrid.find (uriStr);
        if (it != uriToUrid.end())
            return it->second;

        LV2_URID urid = static_cast<LV2_URID> (uriToUrid.size() + 1);
        uriToUrid[uriStr] = urid;
        uridToUri[urid] = uriStr;
        return urid;
    }

    const char* unmapUri (LV2_URID urid)
    {
        auto it = uridToUri.find (urid);
        if (it != uridToUri.end())
            return it->second.toRawUTF8();

        return nullptr;
    }

    LV2_URID operator() (const char* uri) { return mapUri (uri); }

#define URID(name, uri) const LV2_URID name = mapUri (uri)

    URID (atom_Float, LV2_ATOM__Float);
    URID (atom_Int, LV2_ATOM__Int);
    URID (atom_Object, LV2_ATOM__Object);
    URID (atom_Blank, LV2_ATOM__Blank);
    URID (atom_Sequence, LV2_ATOM__Sequence);
    URID (atom_String, LV2_ATOM__String);
    URID (atom_eventTransfer, LV2_ATOM__eventTransfer);
    URID (midi_MidiEvent, LV2_MIDI__MidiEvent);
    URID (patch_Set, LV2_PATCH__Set);
    URID (patch_Get, LV2_PATCH__Get);
    URID (patch_property, LV2_PATCH__property);
    URID (patch_value, LV2_PATCH__value);
    URID (time_Position, LV2_TIME__Position);
    URID (time_frame, LV2_TIME__frame);
    URID (time_speed, LV2_TIME__speed);
    URID (time_bar, LV2_TIME__bar);
    URID (time_beat, LV2_TIME__beat);
    URID (time_beatUnit, LV2_TIME__beatUnit);
    URID (time_beatsPerBar, LV2_TIME__beatsPerBar);
    URID (time_beatsPerMinute, LV2_TIME__beatsPerMinute);
    URID (bufs_maxBlockLength, LV2_BUF_SIZE__maxBlockLength);
    URID (bufs_minBlockLength, LV2_BUF_SIZE__minBlockLength);
    URID (bufs_sequenceSize, LV2_BUF_SIZE__sequenceSize);
    URID (state_StateChanged, LV2_STATE__StateChanged);
    URID (state_threadSafeRestore, LV2_STATE__threadSafeRestore);

#undef URID
};

//==============================================================================

enum class NonAudioPort
{
    seqInput,
    seqOutput,
    latencyOutput,
    freeWheelingInput,
    enabledInput,
    count
};

static constexpr int kNumNonAudioPorts = static_cast<int> (NonAudioPort::count);

//==============================================================================

struct PortIndices
{
    int numInputChannels = 0;
    int numOutputChannels = 0;

    int getAudioInputPort (int index) const noexcept { return index; }

    int getAudioOutputPort (int index) const noexcept { return index + numInputChannels; }

    int getMaxAudioPortIndex() const noexcept { return numInputChannels + numOutputChannels; }

    int getPortIndexFor (NonAudioPort p) const noexcept { return getMaxAudioPortIndex() + static_cast<int> (p); }
};

//==============================================================================

static void midiBufferToAtomSequence (const MidiBuffer& midi, LV2_Atom_Forge* forge, LV2_URID midiEventUrid)
{
    for (const auto& meta : midi)
    {
        const auto& msg = meta.getMessage();
        const auto data = msg.getRawData();
        const auto size = msg.getRawDataSize();

        lv2_atom_forge_frame_time (forge, static_cast<int64_t> (meta.samplePosition));
        lv2_atom_forge_atom (forge, static_cast<uint32_t> (size), midiEventUrid);
        lv2_atom_forge_write (forge, data, static_cast<uint32_t> (size));
    }
}

//==============================================================================

class AudioPluginProcessorLV2 : private AudioProcessorBase::Listener
{
public:
    AudioPluginProcessorLV2 (double sampleRate,
                             const char* bundlePath,
                             const LV2_Feature* const* features)
        : sampleRate (sampleRate)
    {
        ignoreUnused (bundlePath);

        processFeatures (features);

        processor.reset (createPluginProcessor());
        jassert (processor != nullptr);

        if (processor == nullptr)
            return;

        numInputChannels = processor->getBusLayout().getNumAudioInputChannels();
        numOutputChannels = processor->getBusLayout().getNumAudioOutputChannels();

        // Build parameter Urid maps
        const auto params = processor->getParameters();
        lastSentValues.resize (params.size(), -1.0f);

        for (std::size_t i = 0; i < params.size(); ++i)
        {
            const auto hostId = params[i]->getHostParameterID();
            const auto uri = String (YupPlugin_LV2URI) + ":param_" + String (static_cast<int> (hostId));
            const auto urid = urids.mapUri (uri.toRawUTF8());
            paramUridToIndex[urid] = i;
            indexToParamUrid[static_cast<int> (i)] = urid;
        }

        // Setup state URIs
        stateUri = String (YupPlugin_LV2URI) + ":state";
        stateUrid = urids (stateUri.toRawUTF8());

        programUri = String (YupPlugin_LV2URI) + ":program";
        programUrid = urids (programUri.toRawUTF8());

        // Setup state interface
        stateInterface.save = stateSave;
        stateInterface.restore = stateRestore;

        prepare();

        processor->addListener (this);
    }

    ~AudioPluginProcessorLV2() override
    {
        if (processor != nullptr)
        {
            processor->removeListener (this);
            processor.reset();
        }
    }

    void connectPort (uint32_t port, void* data)
    {
        PortIndices indices { numInputChannels, numOutputChannels };

        if (port == static_cast<uint32_t> (indices.getPortIndexFor (NonAudioPort::seqInput)))
        {
            inputSequence = static_cast<const LV2_Atom_Sequence*> (data);
        }
        else if (port == static_cast<uint32_t> (indices.getPortIndexFor (NonAudioPort::seqOutput)))
        {
            outputSequence = static_cast<LV2_Atom_Sequence*> (data);
        }
        else if (port == static_cast<uint32_t> (indices.getPortIndexFor (NonAudioPort::latencyOutput)))
        {
            latencyPort = static_cast<float*> (data);
        }
        else if (port == static_cast<uint32_t> (indices.getPortIndexFor (NonAudioPort::freeWheelingInput)))
        {
            freeWheelingPort = static_cast<const float*> (data);
        }
        else if (port == static_cast<uint32_t> (indices.getPortIndexFor (NonAudioPort::enabledInput)))
        {
            enabledPort = static_cast<const float*> (data);
        }
        else if (port < static_cast<uint32_t> (indices.getMaxAudioPortIndex()))
        {
            audioPorts.resize (static_cast<std::size_t> (indices.getMaxAudioPortIndex()));
            audioPorts[port] = static_cast<float*> (data);
        }
    }

    void activate()
    {
        if (processor != nullptr)
            processor->setPlaybackConfiguration (static_cast<float> (sampleRate), maxBlockSize);
    }

    void run (uint32_t numSamples)
    {
        if (processor == nullptr)
            return;

        midiEvents.clear();
        playHead.invalidate();
        parameterChanges.clear();
        parameterChanges.reserve (getDefaultParameterChangeCapacity (*processor));

        const int numSamplesInt = static_cast<int> (numSamples);
        audioBuffer.setSize (jmax (numInputChannels, numOutputChannels), numSamplesInt, false, true, true);
        audioBuffer.clear();

        // Parse input atom sequence
        parseInputSequence (numSamplesInt);

        // Copy audio inputs
        PortIndices indices { numInputChannels, numOutputChannels };
        for (int ch = 0; ch < numInputChannels; ++ch)
        {
            const auto* src = audioPorts[static_cast<std::size_t> (indices.getAudioInputPort (ch))];
            if (src != nullptr)
                audioBuffer.copyFrom (ch, 0, src, numSamplesInt);
        }

        // Apply offline processing state
        if (freeWheelingPort != nullptr)
            processor->setOfflineProcessing (*freeWheelingPort > 0.5f);

        const bool isEnabled = (enabledPort == nullptr) || (*enabledPort > 0.5f);
        isBypassed = ! isEnabled;

        // Build process context and process
        AudioProcessContext<float> context { audioBuffer, midiEvents, parameterChanges, &playHead };

        {
            const ScopedLock lock (processor->getProcessLock());

            if (processor->isSuspended())
            {
                for (int ch = 0; ch < numOutputChannels; ++ch)
                {
                    auto* dst = audioPorts[static_cast<std::size_t> (indices.getAudioOutputPort (ch))];
                    if (dst != nullptr)
                        std::fill_n (dst, numSamplesInt, 0.0f);
                }
            }
            else
            {
                processAudioBlock (*processor, context, isBypassed);
            }
        }

        // Copy audio outputs
        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            auto* dst = audioPorts[static_cast<std::size_t> (indices.getAudioOutputPort (ch))];
            if (dst != nullptr)
            {
                const auto* src = audioBuffer.getReadPointer (ch);
                std::copy_n (src, numSamplesInt, dst);
            }
        }

        // Write output atom sequence
        writeOutputSequence (numSamples);

        // Write latency
        if (latencyPort != nullptr)
            *latencyPort = static_cast<float> (processor->getLatencySamples());
    }

    void deactivate()
    {
    }

    const void* extensionData (const char* uri) const
    {
        if (std::strcmp (uri, LV2_STATE__interface) == 0)
            return &stateInterface;

        return nullptr;
    }

private:
    void processFeatures (const LV2_Feature* const* features)
    {
        for (auto* feature = features; feature != nullptr && *feature != nullptr; ++feature)
        {
            if (std::strcmp ((*feature)->URI, LV2_URID__map) == 0)
            {
                auto* hostMap = static_cast<LV2_URID_Map*> ((*feature)->data);
                if (hostMap != nullptr)
                    urids.map = *hostMap;
            }
            else if (std::strcmp ((*feature)->URI, LV2_URID__unmap) == 0)
            {
                auto* hostUnmap = static_cast<LV2_URID_Unmap*> ((*feature)->data);
                if (hostUnmap != nullptr)
                    urids.unmap = *hostUnmap;
            }
            else if (std::strcmp ((*feature)->URI, LV2_WORKER__schedule) == 0)
            {
                workerSchedule = static_cast<LV2_Worker_Schedule*> ((*feature)->data);
            }
        }

        // Process options
        for (auto* feature = features; feature != nullptr && *feature != nullptr; ++feature)
        {
            if (std::strcmp ((*feature)->URI, LV2_OPTIONS__options) == 0)
            {
                auto* options = static_cast<const LV2_Options_Option*> ((*feature)->data);
                for (auto* opt = options; opt->key != 0 && opt->value != nullptr; ++opt)
                {
                    if (opt->key == urids.bufs_maxBlockLength && opt->type == urids.atom_Int)
                        maxBlockSize = static_cast<int> (*static_cast<const int32_t*> (opt->value));
                }
            }
        }
    }

    void prepare()
    {
        if (processor == nullptr)
            return;

        const auto blockSize = jmax (maxBlockSize, 256);
        processor->setPlaybackConfiguration (static_cast<float> (sampleRate), blockSize);
    }

    void parseInputSequence (int numSamplesInt)
    {
        if (inputSequence == nullptr)
            return;

        const LV2_Atom_Event* iter = lv2_atom_sequence_begin (&inputSequence->body);
        while (! lv2_atom_sequence_is_end (&inputSequence->body, inputSequence->atom.size, iter))
        {
            const auto* body = &iter->body;
            const auto sampleOffset = static_cast<int> (iter->time.frames);

            // MIDI events
            if (body->type == urids.midi_MidiEvent)
            {
                const auto* data = reinterpret_cast<const uint8_t*> (iter + 1);
                const auto size = static_cast<int> (body->size);

                if (size > 0 && size <= 3)
                    midiEvents.addEvent (data, size, sampleOffset);
                else if (size > 3)
                    midiEvents.addEvent (MidiMessage (data, size), sampleOffset);
            }
            // Patch Set / Get messages
            else if (body->type == urids.atom_Object || body->type == urids.atom_Blank)
            {
                const auto* obj = reinterpret_cast<const LV2_Atom_Object*> (body);

                if (obj->body.otype == urids.patch_Set)
                {
                    const LV2_Atom *propertyAtom = nullptr, *valueAtom = nullptr;
                    LV2_Atom_Object_Query query[] = {
                        { urids.patch_property, &propertyAtom },
                        { urids.patch_value, &valueAtom },
                        LV2_ATOM_OBJECT_QUERY_END
                    };
                    lv2_atom_object_query (obj, query);

                    if (propertyAtom != nullptr && propertyAtom->type == urids.atom_Int)
                    {
                        // Deprecated patch:Set with URID
                        const auto urid = reinterpret_cast<const LV2_Atom_Int*> (propertyAtom)->body;
                        handleParameterSet (urid, valueAtom, 0);
                    }
                    else if (propertyAtom != nullptr && propertyAtom->type == urids.mapUri (LV2_ATOM__URID))
                    {
                        const auto urid = reinterpret_cast<const LV2_Atom_Int*> (propertyAtom)->body;
                        handleParameterSet (urid, valueAtom, sampleOffset);
                    }
                }
                // Time:Position
                else if (obj->body.otype == urids.time_Position)
                {
                    parsePosition (obj, numSamplesInt);
                }
            }

            iter = lv2_atom_sequence_next (iter);
        }
    }

    void handleParameterSet (LV2_URID paramUrid, const LV2_Atom* valueAtom, int sampleOffset)
    {
        auto it = paramUridToIndex.find (paramUrid);
        if (it == paramUridToIndex.end())
            return;

        const auto paramIndex = static_cast<int> (it->second);
        const auto& params = processor->getParameters();

        if (! isPositiveAndBelow (paramIndex, static_cast<int> (params.size())))
            return;

        if (params[paramIndex]->isReadOnly())
            return;

        float value = 0.0f;

        if (valueAtom != nullptr && valueAtom->type == urids.atom_Float)
        {
            value = reinterpret_cast<const LV2_Atom_Float*> (valueAtom)->body;
        }

        params[paramIndex]->setValueNotifyingHost (value);
    }

    void parsePosition (const LV2_Atom_Object* obj, int numSamples)
    {
        const LV2_Atom *frame = nullptr, *speed = nullptr, *bar = nullptr, *beat = nullptr;
        const LV2_Atom *beatUnit = nullptr, *beatsPerBar = nullptr, *beatsPerMinute = nullptr;

        LV2_Atom_Object_Query query[] = {
            { urids.time_frame, &frame },
            { urids.time_speed, &speed },
            { urids.time_bar, &bar },
            { urids.time_beat, &beat },
            { urids.time_beatUnit, &beatUnit },
            { urids.time_beatsPerBar, &beatsPerBar },
            { urids.time_beatsPerMinute, &beatsPerMinute },
            LV2_ATOM_OBJECT_QUERY_END
        };
        lv2_atom_object_query (obj, query);

        AudioPlayHead::PositionInfo info;

        if (beatsPerBar != nullptr && beatsPerBar->type == urids.atom_Float
            && beatUnit != nullptr && beatUnit->type == urids.atom_Int)
        {
            const auto num = static_cast<int> (reinterpret_cast<const LV2_Atom_Float*> (beatsPerBar)->body);
            const auto den = static_cast<int> (reinterpret_cast<const LV2_Atom_Int*> (beatUnit)->body);
            info.setTimeSignature (AudioPlayHead::TimeSignature { num, den });
        }

        if (beatsPerMinute != nullptr && beatsPerMinute->type == urids.atom_Float)
            info.setBpm (reinterpret_cast<const LV2_Atom_Float*> (beatsPerMinute)->body);

        if (beat != nullptr && beat->type == urids.atom_Float)
            info.setPpqPosition (static_cast<double> (reinterpret_cast<const LV2_Atom_Float*> (beat)->body));

        if (speed != nullptr && speed->type == urids.atom_Float)
            info.setIsPlaying (reinterpret_cast<const LV2_Atom_Float*> (speed)->body != 0.0f);

        if (bar != nullptr && bar->type == urids.atom_Int)
            info.setBarCount (reinterpret_cast<const LV2_Atom_Int*> (bar)->body);

        if (frame != nullptr && frame->type == urids.atom_Int)
        {
            const auto frameVal = reinterpret_cast<const LV2_Atom_Int*> (frame)->body;
            info.setTimeInSamples (frameVal);
            if (sampleRate > 0.0)
                info.setTimeInSeconds (static_cast<double> (frameVal) / sampleRate);
        }

        lastPosition = info;
        playHead.setPosition (lastPosition);
    }

    void writeOutputSequence (uint32_t numSamples)
    {
        if (outputSequence == nullptr)
            return;

        LV2_Atom_Forge forge;
        lv2_atom_forge_init (&forge, &urids.map);
        lv2_atom_forge_set_buffer (&forge,
                                   reinterpret_cast<uint8_t*> (outputSequence),
                                   outputSequence->atom.size);

        LV2_Atom_Forge_Frame seqFrame;
        lv2_atom_forge_sequence_head (&forge, &seqFrame, 0);

        // Write parameter change events
        const auto& params = processor->getParameters();
        for (std::size_t i = 0; i < params.size(); ++i)
        {
            const auto& param = params[i];
            const auto currentValue = param->getValue();

            if (currentValue != lastSentValues[static_cast<int> (i)])
            {
                lastSentValues[static_cast<int> (i)] = currentValue;

                lv2_atom_forge_frame_time (&forge, 0);

                LV2_Atom_Forge_Frame objFrame;
                lv2_atom_forge_object (&forge, &objFrame, 0, urids.patch_Set);

                lv2_atom_forge_key (&forge, urids.patch_property);
                lv2_atom_forge_int (&forge, indexToParamUrid[static_cast<int> (i)]);

                lv2_atom_forge_key (&forge, urids.patch_value);
                lv2_atom_forge_float (&forge, currentValue);

                lv2_atom_forge_pop (&forge, &objFrame);
            }
        }

        // Write MIDI output events
        midiBufferToAtomSequence (midiOutputEvents, &forge, urids.midi_MidiEvent);

        // Write state change notification
        if (shouldSendStateChange.exchange (false))
        {
            lv2_atom_forge_frame_time (&forge, 0);

            LV2_Atom_Forge_Frame objFrame;
            lv2_atom_forge_object (&forge, &objFrame, 0, urids.state_StateChanged);
            lv2_atom_forge_pop (&forge, &objFrame);
        }

        lv2_atom_forge_pop (&forge, &seqFrame);
    }

    void audioProcessorChanged (AudioProcessorBase* processor, const AudioProcessor::ChangeDetails& details) override
    {
        if (details.nonParameterStateChanged)
            shouldSendStateChange.store (true);
    }

public:
    UridMap urids;
    double sampleRate = 44100.0;
    int maxBlockSize = 1024;
    int numInputChannels = 0;
    int numOutputChannels = 0;

    std::unique_ptr<AudioProcessor> processor;
    MidiBuffer midiEvents;
    MidiBuffer midiOutputEvents;
    AudioBuffer<float> audioBuffer { 0, 0 };
    ParameterChangeBuffer parameterChanges;

    std::vector<float*> audioPorts;
    const LV2_Atom_Sequence* inputSequence = nullptr;
    LV2_Atom_Sequence* outputSequence = nullptr;
    float* latencyPort = nullptr;
    const float* freeWheelingPort = nullptr;
    const float* enabledPort = nullptr;

    LV2_Worker_Schedule* workerSchedule = nullptr;

    std::map<LV2_URID, std::size_t> paramUridToIndex;
    std::map<int, LV2_URID> indexToParamUrid;
    std::vector<float> lastSentValues;

    String stateUri;
    LV2_URID stateUrid = 0;
    String programUri;
    LV2_URID programUrid = 0;

    bool isBypassed = false;
    std::atomic<bool> shouldSendStateChange { false };
    AudioPlayHead::PositionInfo lastPosition;

    class AudioPluginPlayHeadLV2 final : public AudioPlayHead
    {
    public:
        AudioPluginPlayHeadLV2() = default;

        void invalidate() { currentPosition.reset(); }

        void setPosition (const PositionInfo& info) { currentPosition = info; }

        std::optional<PositionInfo> getPosition() const override
        {
            return currentPosition;
        }

    private:
        std::optional<PositionInfo> currentPosition;
    };

    AudioPluginPlayHeadLV2 playHead;

    // State interface
    static LV2_State_Status stateSave (LV2_State_Handle instance,
                                       LV2_State_Store_Function store,
                                       LV2_State_Handle handle,
                                       uint32_t flags,
                                       const LV2_Feature* const* features)
    {
        auto* self = static_cast<AudioPluginProcessorLV2*> (instance);
        if (self == nullptr || self->processor == nullptr)
            return LV2_STATE_ERR_UNKNOWN;

        MemoryBlock processorState;
        const auto stateResult = self->processor->saveStateIntoMemory (processorState);
        const bool hasProcessorState = stateResult.wasOk() && ! processorState.isEmpty();

        const auto data = writeWrapperBypassState (lv2WrapperStateMagic,
                                                   lv2WrapperStateVersion,
                                                   self->isBypassed,
                                                   processorState,
                                                   hasProcessorState);

        const auto base64 = data.toBase64Encoding();

        store (handle,
               self->stateUrid,
               base64.toRawUTF8(),
               base64.getNumBytesAsUTF8() + 1,
               self->urids.atom_String,
               LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

        return LV2_STATE_SUCCESS;
    }

    static LV2_State_Status stateRestore (LV2_State_Handle instance,
                                          LV2_State_Retrieve_Function retrieve,
                                          LV2_State_Handle handle,
                                          uint32_t flags,
                                          const LV2_Feature* const* features)
    {
        auto* self = static_cast<AudioPluginProcessorLV2*> (instance);
        if (self == nullptr || self->processor == nullptr)
            return LV2_STATE_ERR_UNKNOWN;

        size_t size = 0;
        uint32_t type = 0;
        uint32_t dataFlags = 0;

        // Try program preset first
        const auto* programData = retrieve (handle, self->programUrid, &size, &type, &dataFlags);

        if (programData != nullptr && type == self->urids.atom_Int && size == sizeof (int32_t))
        {
            const auto programIndex = *static_cast<const int32_t*> (programData);
            self->processor->setCurrentPreset (programIndex);
            return LV2_STATE_SUCCESS;
        }

        // Try full state
        const auto* data = retrieve (handle, self->stateUrid, &size, &type, &dataFlags);

        if (data == nullptr)
            return LV2_STATE_ERR_NO_PROPERTY;

        if (type != self->urids.atom_String)
            return LV2_STATE_ERR_BAD_TYPE;

        String text (static_cast<const char*> (data), size);
        MemoryBlock block;
        if (! block.fromBase64Encoding (text))
            return LV2_STATE_ERR_BAD_TYPE;

        const auto wrapperState = readWrapperBypassState (block, lv2WrapperStateMagic, lv2WrapperStateVersion);

        if (wrapperState.hasWrapperState)
        {
            self->isBypassed = wrapperState.isBypassed;

            if (wrapperState.hasProcessorState && ! wrapperState.processorState.isEmpty())
                self->processor->loadStateFromMemory (wrapperState.processorState);
        }
        else
        {
            self->processor->loadStateFromMemory (block);
        }

        return LV2_STATE_SUCCESS;
    }

    LV2_State_Interface stateInterface = { stateSave, stateRestore };
};

//==============================================================================
// LV2 Descriptor
//==============================================================================

static LV2_Handle lv2Instantiate (const LV2_Descriptor* descriptor,
                                  double sampleRate,
                                  const char* bundlePath,
                                  const LV2_Feature* const* features)
{
    auto* instance = new AudioPluginProcessorLV2 (sampleRate, bundlePath, features);
    return static_cast<LV2_Handle> (instance);
}

static void lv2ConnectPort (LV2_Handle instance, uint32_t port, void* data)
{
    if (auto* self = static_cast<AudioPluginProcessorLV2*> (instance))
        self->connectPort (port, data);
}

static void lv2Activate (LV2_Handle instance)
{
    if (auto* self = static_cast<AudioPluginProcessorLV2*> (instance))
        self->activate();
}

static void lv2Run (LV2_Handle instance, uint32_t sampleCount)
{
    if (auto* self = static_cast<AudioPluginProcessorLV2*> (instance))
        self->run (sampleCount);
}

static void lv2Deactivate (LV2_Handle instance)
{
    if (auto* self = static_cast<AudioPluginProcessorLV2*> (instance))
        self->deactivate();
}

static void lv2Cleanup (LV2_Handle instance)
{
    delete static_cast<AudioPluginProcessorLV2*> (instance);
}

static const void* lv2ExtensionData (const char* uri)
{
    if (std::strcmp (uri, LV2_STATE__interface) == 0)
    {
        static const LV2_State_Interface stateInterface = {
            AudioPluginProcessorLV2::stateSave,
            AudioPluginProcessorLV2::stateRestore
        };
        return &stateInterface;
    }

    return nullptr;
}

static const LV2_Descriptor lv2Descriptor = {
    YupPlugin_LV2URI,
    lv2Instantiate,
    lv2ConnectPort,
    lv2Activate,
    lv2Run,
    lv2Deactivate,
    lv2Cleanup,
    lv2ExtensionData
};

} // namespace yup

//==============================================================================
// Exported entry points
//==============================================================================

extern "C" YUP_PLUGIN_ENTRY_POINT const LV2_Descriptor* lv2_descriptor (uint32_t index)
{
    return (index == 0) ? &yup::lv2Descriptor : nullptr;
}
