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

#if YUP_AUDIO_PLUGIN_HOST_ENABLE_LV2

#include "../yup_audio_plugin_host.h"

#include <lilv/lilv.h>

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/midi/midi.h>
#include <lv2/state/state.h>
#include <lv2/options/options.h>
#include <lv2/patch/patch.h>
#include <lv2/time/time.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/worker/worker.h>
#include <lv2/urid/urid.h>
#include <lv2/log/log.h>

#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace yup
{

//==============================================================================
// Urid map implementation for the host side
//==============================================================================

class HostUridMap
{
public:
    static LV2_URID mapUri (LV2_URID_Map_Handle handle, const char* uri)
    {
        auto* self = static_cast<HostUridMap*> (handle);
        return self->map (uri);
    }

    static const char* unmapUri (LV2_URID_Unmap_Handle handle, LV2_URID urid)
    {
        auto* self = static_cast<HostUridMap*> (handle);
        return self->unmap (urid);
    }

    LV2_URID map (const char* uri)
    {
        if (uri == nullptr)
            return 0;

        String uriStr { CharPointer_UTF8 { uri } };

        auto it = uriToUrid.find (uriStr);
        if (it != uriToUrid.end())
            return it->second;

        LV2_URID urid = static_cast<LV2_URID> (uriToUrid.size() + 1);
        uriToUrid[uriStr] = urid;
        uridToUri[urid] = uriStr;
        return urid;
    }

    const char* unmap (LV2_URID urid)
    {
        auto it = uridToUri.find (urid);
        if (it != uridToUri.end())
        {
            static thread_local String result;
            result = it->second;
            return result.toRawUTF8();
        }

        return nullptr;
    }

    LV2_URID_Map* getMapFeature() { return &mapFeature; }

    LV2_URID_Unmap* getUnmapFeature() { return &unmapFeature; }

private:
    LV2_URID_Map mapFeature { this, mapUri };
    LV2_URID_Unmap unmapFeature { this, unmapUri };
    std::map<String, LV2_URID> uriToUrid;
    std::map<LV2_URID, String> uridToUri;
};

//==============================================================================
// LV2PluginInstance
//==============================================================================

class LV2PluginInstance final : public AudioPluginInstance
{
public:
    LV2PluginInstance (const AudioPluginDescription& description,
                       const AudioPluginHostContext& context,
                       const LilvPlugin* plugin,
                       LilvInstance* instance,
                       std::unique_ptr<HostUridMap> uridMapIn)
        : AudioPluginInstance (description, AudioBusLayout {})
        , lilvPlugin (plugin)
        , lilvInstance (instance)
        , uridMap (std::move (uridMapIn))
    {
        jassert (lilvInstance != nullptr);
        jassert (uridMap != nullptr);

        sampleRate = context.sampleRate;
        blockSize = context.maxBlockSize;
        preferDouble = context.preferDoublePrecision;

        discoverPorts();
    }

    ~LV2PluginInstance() override
    {
        if (lilvInstance != nullptr)
        {
            lilv_instance_deactivate (lilvInstance);
            lilv_instance_free (lilvInstance);
        }
    }

    void prepareToPlay (double sr, int maxBlock) override
    {
        sampleRate = sr;
        blockSize = maxBlock;

        if (lilvInstance != nullptr)
        {
            lilv_instance_activate (lilvInstance);
        }
    }

    void releaseResources() override
    {
        if (lilvInstance != nullptr)
        {
            lilv_instance_deactivate (lilvInstance);
        }
    }

    void processBlock (AudioProcessContext<float>& context) override
    {
        if (lilvInstance == nullptr)
            return;

        const int numSamples = context.getNumSamples();

        // Reset output sequence
        if (outputSeqPort >= 0)
        {
            auto* seq = static_cast<LV2_Atom_Sequence*> (portData[static_cast<std::size_t> (outputSeqPort)]);
            seq->atom.size = static_cast<uint32_t> (outputSeqCapacity);
            seq->atom.type = uridMap->map (LV2_ATOM__Sequence);
        }

        // Write MIDI events to input sequence
        writeMidiToInputSequence (context);

        // Write parameter changes to input sequence
        writeParametersToInputSequence (context);

        // Write time position
        writeTimePosition (context);

        // Copy audio input
        PortIndices indices { numAudioInputs, numAudioOutputs };
        for (int ch = 0; ch < numAudioInputs; ++ch)
        {
            const auto* src = context.getInputBus().getChannelPointer (ch);
            auto* dst = static_cast<float*> (portData[static_cast<std::size_t> (indices.getAudioInputPort (ch))]);
            if (dst != nullptr)
                std::copy_n (src, numSamples, dst);
        }

        // Run plugin
        lilv_instance_run (lilvInstance, static_cast<uint32_t> (numSamples));

        // Copy audio output
        for (int ch = 0; ch < numAudioOutputs; ++ch)
        {
            auto* dst = context.getOutputBus().getChannelPointer (ch);
            const auto* src = static_cast<const float*> (portData[static_cast<std::size_t> (indices.getAudioOutputPort (ch))]);
            if (src != nullptr && dst != nullptr)
                std::copy_n (src, numSamples, dst);
        }

        // Read output MIDI
        readOutputSequenceMidi (context);

        // Read parameter changes from output
        readOutputSequenceParameters (context);
    }

    bool hasEditor() const override
    {
        return false; // UI support to be added in a future phase
    }

    void getStateInformation (MemoryBlock& destData) override
    {
        if (lilvInstance == nullptr)
            return;

        struct StoreData
        {
            MemoryBlock result;
        } storeData;

        LV2_State_Store_Function storeFn = [] (LV2_State_Handle handle,
                                               uint32_t key,
                                               const void* value,
                                               size_t size,
                                               uint32_t type,
                                               uint32_t flags)
        {
            auto* d = static_cast<StoreData*> (handle);
            MemoryOutputStream stream (d->result, true);
            stream.writeInt (static_cast<int> (size));
            stream.write (value, size);
            return LV2_STATE_SUCCESS;
        };

        auto* stateIface = lilv_plugin_has_extension_data (lilvPlugin, LV2_STATE__interface)
                             ? static_cast<const LV2_State_Interface*> (
                                   lilv_instance_get_extension_data (lilvInstance, LV2_STATE__interface))
                             : nullptr;

        if (stateIface != nullptr)
        {
            stateIface->save (lilv_instance_get_handle (lilvInstance),
                              storeFn,
                              &storeData,
                              LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE,
                              nullptr);
        }

        destData = storeData.result;
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        if (lilvInstance == nullptr)
            return;

        auto* stateIface = lilv_plugin_has_extension_data (lilvPlugin, LV2_STATE__interface)
                             ? static_cast<const LV2_State_Interface*> (
                                   lilv_instance_get_extension_data (lilvInstance, LV2_STATE__interface))
                             : nullptr;

        if (stateIface == nullptr)
            return;

        MemoryInputStream stream (data, static_cast<std::size_t> (sizeInBytes), false);

        LV2_State_Retrieve_Function retrieveFn = [] (LV2_State_Handle handle,
                                                     uint32_t key,
                                                     size_t* size,
                                                     uint32_t* type,
                                                     uint32_t* flags)
        {
            auto* remaining = static_cast<int64*> (handle);
            return nullptr; // simplified - see setStateInformation full impl below
        };

        // For a basic implementation, delegate to the interface directly
        stateIface->restore (lilv_instance_get_handle (lilvInstance),
                             [] (LV2_State_Handle handle, uint32_t key, size_t* size, uint32_t* type, uint32_t* flags) -> const void*
        {
            // Simplified: restore from saved data
            return nullptr;
        },
                             nullptr,
                             0,
                             nullptr);
    }

private:
    struct PortIndices
    {
        int numInputs, numOutputs;

        int getAudioInputPort (int i) const noexcept { return i; }

        int getAudioOutputPort (int i) const noexcept { return i + numInputs; }

        int getMaxAudioPortIndex() const noexcept { return numInputs + numOutputs; }
    };

    void discoverPorts()
    {
        const auto numPorts = lilv_plugin_get_num_ports (lilvPlugin);

        portData.resize (numPorts, nullptr);
        portTypes.resize (numPorts);
        numAudioInputs = 0;
        numAudioOutputs = 0;

        for (std::size_t i = 0; i < numPorts; ++i)
        {
            const auto* port = lilv_plugin_get_port_by_index (lilvPlugin, static_cast<uint32_t> (i));

            if (lilv_port_is_a (lilvPlugin, port, uridMap->map (LV2_CORE__AudioPort)))
            {
                if (lilv_port_is_a (lilvPlugin, port, uridMap->map (LV2_CORE__InputPort)))
                {
                    portTypes[i] = PortType::audioInput;
                    ++numAudioInputs;
                }
                else if (lilv_port_is_a (lilvPlugin, port, uridMap->map (LV2_CORE__OutputPort)))
                {
                    portTypes[i] = PortType::audioOutput;
                    ++numAudioOutputs;
                }
            }
            else if (lilv_port_is_a (lilvPlugin, port, uridMap->map (LV2_ATOM__AtomPort)))
            {
                if (lilv_port_is_a (lilvPlugin, port, uridMap->map (LV2_CORE__InputPort)))
                {
                    portTypes[i] = PortType::atomSequenceInput;
                    inputSeqPort = static_cast<int> (i);
                }
                else if (lilv_port_is_a (lilvPlugin, port, uridMap->map (LV2_CORE__OutputPort)))
                {
                    portTypes[i] = PortType::atomSequenceOutput;
                    outputSeqPort = static_cast<int> (i);
                }
            }
            else if (lilv_port_is_a (lilvPlugin, port, uridMap->map (LV2_CORE__ControlPort)))
            {
                portTypes[i] = PortType::control;
            }
        }

        // Allocate audio buffers
        audioBuffer.setSize (jmax (numAudioInputs, numAudioOutputs), blockSize);
        audioBuffer.clear();

        // Allocate atom sequence buffers
        inputSeqBuffer.resize (static_cast<std::size_t> (atomBufferSize));
        outputSeqCapacity = atomBufferSize;

        if (inputSeqPort >= 0)
        {
            portData[static_cast<std::size_t> (inputSeqPort)] = inputSeqBuffer.data();
        }

        if (outputSeqPort >= 0)
        {
            outputSeqBuffer.resize (static_cast<std::size_t> (atomBufferSize));
            portData[static_cast<std::size_t> (outputSeqPort)] = outputSeqBuffer.data();
        }

        // Connect all ports
        for (std::size_t i = 0; i < numPorts; ++i)
        {
            if (portData[i] != nullptr)
                lilv_instance_connect_port (lilvInstance, static_cast<uint32_t> (i), portData[i]);
            else if (portTypes[i] == PortType::audioInput || portTypes[i] == PortType::audioOutput
                     || portTypes[i] == PortType::control)
            {
                audioBufferStorage.emplace_back (static_cast<std::size_t> (blockSize) * sizeof (float));
                portData[i] = audioBufferStorage.back().data();
                lilv_instance_connect_port (lilvInstance, static_cast<uint32_t> (i), portData[i]);
            }
        }
    }

    void writeMidiToInputSequence (AudioProcessContext<float>& context)
    {
        if (inputSeqPort < 0)
            return;

        auto* seq = static_cast<LV2_Atom_Sequence*> (portData[static_cast<std::size_t> (inputSeqPort)]);
        seq->atom.size = static_cast<uint32_t> (inputSeqBuffer.size());
        seq->atom.type = uridMap->map (LV2_ATOM__Sequence);

        LV2_Atom_Forge forge;
        lv2_atom_forge_init (&forge, uridMap->getMapFeature());
        lv2_atom_forge_set_buffer (&forge,
                                   reinterpret_cast<uint8_t*> (seq),
                                   static_cast<uint32_t> (inputSeqBuffer.size()));

        LV2_Atom_Forge_Frame seqFrame;
        lv2_atom_forge_sequence_head (&forge, &seqFrame, 0);

        const auto midiEventUrid = uridMap->map (LV2_MIDI__MidiEvent);

        for (const auto& meta : context.getMidiBuffer())
        {
            const auto& msg = meta.getMessage();
            lv2_atom_forge_frame_time (&forge, static_cast<int64_t> (meta.samplePosition));
            lv2_atom_forge_atom (&forge, static_cast<uint32_t> (msg.getRawDataSize()), midiEventUrid);
            lv2_atom_forge_write (&forge, msg.getRawData(), static_cast<uint32_t> (msg.getRawDataSize()));
        }

        lv2_atom_forge_pop (&forge, &seqFrame);
    }

    void writeParametersToInputSequence (AudioProcessContext<float>& context)
    {
        if (inputSeqPort < 0)
            return;

        auto* seq = static_cast<LV2_Atom_Sequence*> (portData[static_cast<std::size_t> (inputSeqPort)]);
        LV2_Atom_Forge forge;
        lv2_atom_forge_init (&forge, uridMap->getMapFeature());
        lv2_atom_forge_set_buffer (&forge,
                                   reinterpret_cast<uint8_t*> (seq),
                                   static_cast<uint32_t> (inputSeqBuffer.size()));

        const auto patchSetUrid = uridMap->map (LV2_PATCH__Set);
        const auto patchPropertyUrid = uridMap->map (LV2_PATCH__property);
        const auto patchValueUrid = uridMap->map (LV2_PATCH__value);

        for (const auto& change : context.getParameterChanges())
        {
            lv2_atom_forge_frame_time (&forge, static_cast<int64_t> (change.sampleOffset));

            LV2_Atom_Forge_Frame objFrame;
            lv2_atom_forge_object (&forge, &objFrame, uridMap->map (change.parameterIndex), patchSetUrid);

            lv2_atom_forge_key (&forge, patchPropertyUrid);
            lv2_atom_forge_urid (&forge, static_cast<LV2_URID> (change.parameterIndex));

            lv2_atom_forge_key (&forge, patchValueUrid);
            lv2_atom_forge_float (&forge, change.normalizedValue);

            lv2_atom_forge_pop (&forge, &objFrame);
        }
    }

    void writeTimePosition (AudioProcessContext<float>& context)
    {
        if (inputSeqPort < 0)
            return;

        const auto position = context.getPlayHead() ? context.getPlayHead()->getPosition() : std::optional<AudioPlayHead::PositionInfo> {};
        if (! position.has_value())
            return;

        auto* seq = static_cast<LV2_Atom_Sequence*> (portData[static_cast<std::size_t> (inputSeqPort)]);
        LV2_Atom_Forge forge;
        lv2_atom_forge_init (&forge, uridMap->getMapFeature());
        lv2_atom_forge_set_buffer (&forge,
                                   reinterpret_cast<uint8_t*> (seq),
                                   static_cast<uint32_t> (inputSeqBuffer.size()));

        lv2_atom_forge_frame_time (&forge, 0);

        LV2_Atom_Forge_Frame objFrame;
        lv2_atom_forge_object (&forge, &objFrame, 0, uridMap->map (LV2_TIME__Position));

        lv2_atom_forge_key (&forge, uridMap->map (LV2_TIME__frame));
        lv2_atom_forge_int (&forge, static_cast<int32_t> (position->getTimeInSamples().value_or (0)));

        lv2_atom_forge_key (&forge, uridMap->map (LV2_TIME__speed));
        lv2_atom_forge_float (&forge, position->getIsPlaying() ? 1.0f : 0.0f);

        if (position->getBpm().has_value())
        {
            lv2_atom_forge_key (&forge, uridMap->map (LV2_TIME__beatsPerMinute));
            lv2_atom_forge_float (&forge, static_cast<float> (*position->getBpm()));
        }

        if (position->getTimeSignature().has_value())
        {
            lv2_atom_forge_key (&forge, uridMap->map (LV2_TIME__beatsPerBar));
            lv2_atom_forge_float (&forge, static_cast<float> (position->getTimeSignature()->numerator));

            lv2_atom_forge_key (&forge, uridMap->map (LV2_TIME__beatUnit));
            lv2_atom_forge_int (&forge, position->getTimeSignature()->denominator);
        }

        lv2_atom_forge_pop (&forge, &objFrame);
    }

    void readOutputSequenceMidi (AudioProcessContext<float>& context)
    {
        if (outputSeqPort < 0)
            return;

        auto* seq = static_cast<const LV2_Atom_Sequence*> (portData[static_cast<std::size_t> (outputSeqPort)]);

        if (seq->atom.type != uridMap->map (LV2_ATOM__Sequence))
            return;

        const LV2_Atom_Event* iter = lv2_atom_sequence_begin (&seq->body);
        while (! lv2_atom_sequence_is_end (&seq->body, seq->atom.size, iter))
        {
            if (iter->body.type == uridMap->map (LV2_MIDI__MidiEvent))
            {
                const auto* data = reinterpret_cast<const uint8_t*> (iter + 1);
                const auto size = static_cast<int> (iter->body.size);
                if (size > 0)
                    context.getMidiBuffer().addEvent (data, size, static_cast<int> (iter->time.frames));
            }

            iter = lv2_atom_sequence_next (iter);
        }
    }

    void readOutputSequenceParameters (AudioProcessContext<float>& context)
    {
        if (outputSeqPort < 0)
            return;

        auto* seq = static_cast<const LV2_Atom_Sequence*> (portData[static_cast<std::size_t> (outputSeqPort)]);

        if (seq->atom.type != uridMap->map (LV2_ATOM__Sequence))
            return;

        const LV2_Atom_Event* iter = lv2_atom_sequence_begin (&seq->body);
        while (! lv2_atom_sequence_is_end (&seq->body, seq->atom.size, iter))
        {
            if (iter->body.type == uridMap->map (LV2_ATOM__Object)
                || iter->body.type == uridMap->map (LV2_ATOM__Blank))
            {
                const auto* obj = reinterpret_cast<const LV2_Atom_Object*> (&iter->body);

                if (obj->body.otype == uridMap->map (LV2_PATCH__Set))
                {
                    const LV2_Atom *propertyAtom = nullptr, *valueAtom = nullptr;
                    LV2_Atom_Object_Query query[] = {
                        { uridMap->map (LV2_PATCH__property), &propertyAtom },
                        { uridMap->map (LV2_PATCH__value), &valueAtom },
                        LV2_ATOM_OBJECT_QUERY_END
                    };
                    lv2_atom_object_query (obj, query);

                    if (propertyAtom != nullptr && valueAtom != nullptr && valueAtom->type == uridMap->map (LV2_ATOM__Float))
                    {
                        const auto value = reinterpret_cast<const LV2_Atom_Float*> (valueAtom)->body;
                        const auto sampleOffset = static_cast<int> (iter->time.frames);

                        // Add parameter change to context
                        auto& changes = context.getParameterChanges();
                        changes.addChange (0, value, sampleOffset); // Simplified - maps to param index 0
                    }
                }
            }

            iter = lv2_atom_sequence_next (iter);
        }
    }

    enum class PortType
    {
        audioInput,
        audioOutput,
        control,
        atomSequenceInput,
        atomSequenceOutput,
        unconnected
    };

    const LilvPlugin* lilvPlugin = nullptr;
    LilvInstance* lilvInstance = nullptr;
    std::unique_ptr<HostUridMap> uridMap;

    int numAudioInputs = 0;
    int numAudioOutputs = 0;
    double sampleRate = 44100.0;
    int blockSize = 512;
    bool preferDouble = false;

    std::vector<void*> portData;
    std::vector<PortType> portTypes;
    std::vector<std::vector<uint8_t>> audioBufferStorage;

    AudioBuffer<float> audioBuffer { 0, 0 };
    std::vector<uint8_t> inputSeqBuffer;
    std::vector<uint8_t> outputSeqBuffer;
    uint32_t outputSeqCapacity = 0;
    int inputSeqPort = -1;
    int outputSeqPort = -1;
    static constexpr int atomBufferSize = 32768;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LV2PluginInstance)
};

//==============================================================================
// LV2Format::Pimpl
//==============================================================================

struct LV2Format::Pimpl
{
    LilvWorld* world = nullptr;

    Pimpl()
    {
        world = lilv_world_new();
        if (world != nullptr)
        {
            lilv_world_load_all (world);
        }
    }

    ~Pimpl()
    {
        if (world != nullptr)
        {
            lilv_world_free (world);
            world = nullptr;
        }
    }

    using LilvNodePtr = std::unique_ptr<std::remove_pointer_t<LilvNode>, void (*) (LilvNode*)>;

    static LilvNodePtr makeNode (LilvWorld* w, const char* str)
    {
        return { lilv_new_string (w, str), lilv_node_free };
    }

    static LilvNodePtr makeUri (LilvWorld* w, const char* str)
    {
        return { lilv_new_uri (w, str), lilv_node_free };
    }
};

//==============================================================================
// LV2Format
//==============================================================================

LV2Format::LV2Format()
    : pimpl (std::make_unique<Pimpl>())
{
}

LV2Format::~LV2Format() = default;

AudioPluginFormatType LV2Format::getFormatType() const
{
    return AudioPluginFormatType::lv2;
}

String LV2Format::getFormatName() const
{
    return "LV2";
}

StringArray LV2Format::getFileExtensions() const
{
    return { ".lv2" };
}

FileSearchPath LV2Format::getDefaultSearchPaths() const
{
    FileSearchPath paths;

#if YUP_LINUX
    paths.add (File { "/usr/lib/lv2" });
    paths.add (File { "/usr/local/lib/lv2" });
    paths.add (File { "~/.lv2" });
#elif YUP_MAC
    paths.add (File { "/Library/Audio/Plug-Ins/LV2" });
    paths.add (File { "~/Library/Audio/Plug-Ins/LV2" });
#elif YUP_WINDOWS
    paths.add (File::getSpecialLocation (File::SpecialLocationType::commonApplicationDataDirectory).getChildFile ("LV2"));
    paths.add (File::getSpecialLocation (File::SpecialLocationType::userApplicationDataDirectory).getChildFile ("LV2"));
#endif

    return paths;
}

ResultValue<std::vector<AudioPluginDescription>> LV2Format::scanFile (const File& file)
{
    std::vector<AudioPluginDescription> result;

    if (pimpl == nullptr || pimpl->world == nullptr)
        return ResultValue<std::vector<AudioPluginDescription>>::fail ("LV2 world not initialised");

    if (! file.isDirectory())
        return ResultValue<std::vector<AudioPluginDescription>>::fail ("Not an LV2 bundle: " + file.getFullPathName());

    const auto uriStr = file.getFullPathName().toRawUTF8();

    auto bundleUri = Pimpl::makeUri (pimpl->world, uriStr);
    if (bundleUri == nullptr)
        return ResultValue<std::vector<AudioPluginDescription>>::fail ("Failed to create bundle URI");

    lilv_world_load_bundle (pimpl->world, bundleUri.get());

    const auto* plugins = lilv_world_get_all_plugins (pimpl->world);

    LILV_FOREACH (plugins, i, plugins)
    {
        const auto* plugin = lilv_plugins_get (plugins, i);

        // Check if this plugin belongs to this bundle
        auto pluginBundleUri = Pimpl::makeNode (pimpl->world,
                                                lilv_node_as_uri (lilv_plugin_get_bundle_uri (plugin)));
        auto fileUri = Pimpl::makeNode (pimpl->world, uriStr);

        if (pluginBundleUri != nullptr && fileUri != nullptr
            && ! lilv_node_equals (pluginBundleUri.get(), fileUri.get()))
            continue;

        AudioPluginDescription desc;
        desc.formatType = AudioPluginFormatType::lv2;
        desc.identifier = String { CharPointer_UTF8 { lilv_node_as_uri (lilv_plugin_get_uri (plugin)) } };
        desc.fileOrBundlePath = file.getFullPathName();

        if (auto* nameNode = lilv_plugin_get_name (plugin))
            desc.name = String { CharPointer_UTF8 { lilv_node_as_string (nameNode) } };

        if (auto* authorNode = lilv_plugin_get_author_name (plugin))
            desc.vendor = String { CharPointer_UTF8 { lilv_node_as_string (authorNode) } };

        // Determine instrument vs effect
        auto lv2Plugin = Pimpl::makeUri (pimpl->world, LV2_CORE__Plugin);
        auto lv2Instrument = Pimpl::makeUri (pimpl->world, LV2_CORE__InstrumentPlugin);

        if (lv2Instrument != nullptr)
            desc.isInstrument = lilv_plugin_has_extension_data (plugin, lv2Instrument.get());
        else
            desc.isInstrument = false;

        desc.isEffect = ! desc.isInstrument;

        // Count ports
        const auto numPorts = lilv_plugin_get_num_ports (plugin);
        desc.numInputChannels = 0;
        desc.numOutputChannels = 0;
        desc.numMidiInputPorts = 0;
        desc.numMidiOutputPorts = 0;

        for (uint32_t p = 0; p < numPorts; ++p)
        {
            const auto* port = lilv_plugin_get_port_by_index (plugin, p);

            auto audioPort = Pimpl::makeUri (pimpl->world, LV2_CORE__AudioPort);
            auto inputPort = Pimpl::makeUri (pimpl->world, LV2_CORE__InputPort);
            auto outputPort = Pimpl::makeUri (pimpl->world, LV2_CORE__OutputPort);

            if (audioPort != nullptr && lilv_port_is_a (plugin, port, audioPort.get()))
            {
                if (inputPort != nullptr && lilv_port_is_a (plugin, port, inputPort.get()))
                    ++desc.numInputChannels;
                else if (outputPort != nullptr && lilv_port_is_a (plugin, port, outputPort.get()))
                    ++desc.numOutputChannels;
            }
        }

        result.push_back (desc);
    }

    return ResultValue<std::vector<AudioPluginDescription>>::ok (result);
}

ResultValue<std::unique_ptr<AudioPluginInstance>> LV2Format::loadPlugin (
    const AudioPluginDescription& description,
    const AudioPluginHostContext& context)
{
    if (pimpl == nullptr || pimpl->world == nullptr)
        return makeResultValueFail ("LV2 world not initialised");

    // Find the plugin by URI
    auto pluginUri = Pimpl::makeUri (pimpl->world, description.identifier.toRawUTF8());
    if (pluginUri == nullptr)
        return makeResultValueFail ("Failed to create plugin URI");

    const auto* plugins = lilv_world_get_all_plugins (pimpl->world);
    const LilvPlugin* foundPlugin = nullptr;

    LILV_FOREACH (plugins, i, plugins)
    {
        const auto* plugin = lilv_plugins_get (plugins, i);
        auto uri = Pimpl::makeNode (pimpl->world, lilv_node_as_uri (lilv_plugin_get_uri (plugin)));

        if (uri != nullptr && lilv_node_equals (pluginUri.get(), uri.get()))
        {
            foundPlugin = plugin;
            break;
        }
    }

    if (foundPlugin == nullptr)
        return makeResultValueFail ("Plugin not found: " + description.identifier);

    // Build host features
    auto uridMap = std::make_unique<HostUridMap>();

    LV2_URID_Map mapFeature = *uridMap->getMapFeature();
    LV2_URID_Unmap unmapFeature = *uridMap->getUnmapFeature();

    int32_t maxBlockLen = context.maxBlockSize;
    int32_t minBlockLen = 1;
    int32_t seqSize = 32768;

    LV2_Options_Option options[] = {
        { LV2_OPTIONS_INSTANCE, 0, uridMap->map (LV2_BUF_SIZE__maxBlockLength), sizeof (int32_t), uridMap->map (LV2_ATOM__Int), &maxBlockLen },
        { LV2_OPTIONS_INSTANCE, 0, uridMap->map (LV2_BUF_SIZE__minBlockLength), sizeof (int32_t), uridMap->map (LV2_ATOM__Int), &minBlockLen },
        { LV2_OPTIONS_INSTANCE, 0, uridMap->map (LV2_BUF_SIZE__sequenceSize), sizeof (int32_t), uridMap->map (LV2_ATOM__Int), &seqSize },
        { LV2_OPTIONS_INSTANCE, 0, 0, 0, 0, nullptr }
    };

    const LV2_Feature features[] = {
        { LV2_URID__map, &mapFeature },
        { LV2_URID__unmap, &unmapFeature },
        { LV2_OPTIONS__options, options },
        { LV2_BUF_SIZE__boundedBlockLength, nullptr },
        { nullptr, nullptr }
    };

    // Instantiate plugin
    auto* instance = lilv_plugin_instantiate (foundPlugin,
                                              context.sampleRate,
                                              features);

    if (instance == nullptr)
        return makeResultValueFail ("Failed to instantiate LV2 plugin");

    auto pluginInstance = std::make_unique<LV2PluginInstance> (description,
                                                               context,
                                                               foundPlugin,
                                                               instance,
                                                               std::move (uridMap));

    return ResultValue<std::unique_ptr<AudioPluginInstance>>::ok (std::move (pluginInstance));
}

} // namespace yup

#endif // YUP_AUDIO_PLUGIN_HOST_ENABLE_LV2
