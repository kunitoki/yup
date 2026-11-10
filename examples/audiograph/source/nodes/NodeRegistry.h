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

#pragma once

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "AnalyzerNodes.h"
#include "DistortionNodes.h"
#include "FractionalDelayNode.h"
#include "GainNode.h"
#include "LatencyNode.h"
#include "StateVariableFilterNode.h"
#include "OscillatorNode.h"
#include "PluginNodeView.h"
#include "RecorderNode.h"
#include "SamplePlayerNode.h"
#include "SpectralPassthroughNode.h"
#include "SubgraphNode.h"

//==============================================================================
/**
    Maps stable string identifiers to processor and view factories.

    NodeRegistry drives both the AudioGraphModel::NodeFactory (used for
    graph state reconstruction after loading) and menu-based node creation in
    the editor. Register all internal node types via registerInternalNodes()
    and, on desktop builds, plugin format entries via registerPluginFormats().
*/
class NodeRegistry
{
public:
    //==============================================================================
    /** Stable factory key for the built-in oscillator node. */
    static constexpr const char* oscillatorIdentifier = "internal.oscillator";

    /** Stable factory key for the built-in gain node. */
    static constexpr const char* gainIdentifier = "internal.gain";

    /** Stable factory key for the built-in latency node. */
    static constexpr const char* latencyIdentifier = "internal.latency";

    /** Stable factory key for the built-in fractional delay node. */
    static constexpr const char* fractionalDelayIdentifier = "internal.fractionalDelay";

    /** Stable factory key for the built-in state variable filter node. */
    static constexpr const char* svfIdentifier = "internal.svf";

    /** Stable factory key for the built-in oscilloscope node. */
    static constexpr const char* oscilloscopeIdentifier = "internal.oscilloscope";

    /** Stable factory key for the built-in spectrum analyzer node. */
    static constexpr const char* spectrumAnalyzerIdentifier = "internal.spectrumAnalyzer";

    /** Stable factory key for the built-in tanh distortion node. */
    static constexpr const char* tanhDistortionIdentifier = "internal.tanhDistortion";

    /** Stable factory key for the built-in Blunter soft clipper node. */
    static constexpr const char* blunterSoftClipperIdentifier = "internal.blunterSoftClipper";

    /** Stable factory key for the built-in AA-IIR antialiased hard clipper node. */
    static constexpr const char* aaIirHardClipperIdentifier = "internal.aaIirHardClipper";

    /** Stable factory key for the built-in looping sample player node. */
    static constexpr const char* samplePlayerIdentifier = "internal.samplePlayer";

    /** Stable factory key for the built-in recorder node. */
    static constexpr const char* recorderIdentifier = "internal.recorder";

    /** Stable factory key for the built-in spectral passthrough node. */
    static constexpr const char* spectralPassthroughIdentifier = "internal.spectralPassthrough";

    /** Stable factory key for the built-in recursive subgraph node. */
    static constexpr const char* subgraphIdentifier = "internal.subgraph";

    /** Stable factory key for Unknown plugin nodes. */
    static constexpr const char* pluginUnknownIdentifier = "plugin.unknown";

#if YUP_DESKTOP
    /** Stable factory key for VST3 plugin nodes. */
    static constexpr const char* pluginVst3Identifier = "plugin.vst3";

    /** Stable factory key for CLAP plugin nodes. */
    static constexpr const char* pluginClapIdentifier = "plugin.clap";

    /** Stable factory key for Audio Unit plugin nodes. */
    static constexpr const char* pluginAuIdentifier = "plugin.au";
#endif

    //==============================================================================
    using ProcessorFactory = std::function<yup::ResultValue<std::unique_ptr<yup::AudioProcessor>> (const yup::AudioGraphNodeProperties&)>;
    using ViewFactory = std::function<std::unique_ptr<yup::AudioGraphNodeView> (yup::AudioGraphNodeID, yup::AudioProcessor*, yup::AudioGraphProcessor*)>;

    //==============================================================================
    struct Entry
    {
        ProcessorFactory createProcessor;
        ViewFactory createView;
    };

    //==============================================================================
    /**
        Registers the built-in internal node types.

        Each entry gets a processor factory and a matching view factory. The view
        factory dynamic_casts to the concrete processor type before constructing
        the view.
    */
    void registerInternalNodes()
    {
        entries[oscillatorIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<OscillatorProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* osc = dynamic_cast<OscillatorProcessor*> (proc);
            if (osc == nullptr)
                return nullptr;

            return std::make_unique<OscillatorNodeView> (nodeID, *osc);
        }
        };

        entries[gainIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<GainProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* gain = dynamic_cast<GainProcessor*> (proc);
            if (gain == nullptr)
                return nullptr;

            return std::make_unique<GainNodeView> (nodeID, *gain);
        }
        };

        entries[latencyIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<LatencyProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* latency = dynamic_cast<LatencyProcessor*> (proc);
            if (latency == nullptr)
                return nullptr;

            return std::make_unique<LatencyNodeView> (nodeID, *latency);
        }
        };

        entries[fractionalDelayIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<FractionalDelayProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* delay = dynamic_cast<FractionalDelayProcessor*> (proc);
            if (delay == nullptr)
                return nullptr;

            return std::make_unique<FractionalDelayNodeView> (nodeID, *delay);
        }
        };

        entries[svfIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<StateVariableFilterProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* svf = dynamic_cast<StateVariableFilterProcessor*> (proc);
            if (svf == nullptr)
                return nullptr;

            return std::make_unique<StateVariableFilterNodeView> (nodeID, *svf);
        }
        };

        entries[oscilloscopeIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<OscilloscopeProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* oscilloscope = dynamic_cast<OscilloscopeProcessor*> (proc);
            if (oscilloscope == nullptr)
                return nullptr;

            return std::make_unique<OscilloscopeNodeView> (nodeID, *oscilloscope);
        }
        };

        entries[spectrumAnalyzerIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<SpectrumAnalyzerProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* spectrumAnalyzer = dynamic_cast<SpectrumAnalyzerProcessor*> (proc);
            if (spectrumAnalyzer == nullptr)
                return nullptr;

            return std::make_unique<SpectrumAnalyzerNodeView> (nodeID, *spectrumAnalyzer);
        }
        };

        entries[tanhDistortionIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<TanhDistortionProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* distortion = dynamic_cast<TanhDistortionProcessor*> (proc);
            if (distortion == nullptr)
                return nullptr;

            return std::make_unique<TanhDistortionNodeView> (nodeID, *distortion);
        }
        };

        entries[blunterSoftClipperIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<BlunterSoftClipperProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* clipper = dynamic_cast<BlunterSoftClipperProcessor*> (proc);
            if (clipper == nullptr)
                return nullptr;

            return std::make_unique<BlunterSoftClipperNodeView> (nodeID, *clipper);
        }
        };

        entries[aaIirHardClipperIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<AaIirHardClipperProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* clipper = dynamic_cast<AaIirHardClipperProcessor*> (proc);
            if (clipper == nullptr)
                return nullptr;

            return std::make_unique<AaIirHardClipperNodeView> (nodeID, *clipper);
        }
        };

        entries[samplePlayerIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<SamplePlayerProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* samplePlayer = dynamic_cast<SamplePlayerProcessor*> (proc);
            if (samplePlayer == nullptr)
                return nullptr;

            return std::make_unique<SamplePlayerNodeView> (nodeID, *samplePlayer);
        }
        };

        entries[recorderIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<RecorderProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* recorder = dynamic_cast<RecorderProcessor*> (proc);
            if (recorder == nullptr)
                return nullptr;

            return std::make_unique<RecorderNodeView> (nodeID, *recorder);
        }
        };

        entries[spectralPassthroughIdentifier] = {
            [] (const yup::AudioGraphNodeProperties&) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            return yup::makeResultValueOk (std::make_unique<SpectralPassthroughProcessor>());
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* spectral = dynamic_cast<SpectralPassthroughProcessor*> (proc);
            if (spectral == nullptr)
                return nullptr;

            return std::make_unique<SpectralPassthroughNodeView> (nodeID, *spectral);
        }
        };

        entries[subgraphIdentifier] = {
            [this] (const yup::AudioGraphNodeProperties& props) -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            auto processor = std::make_unique<SubgraphProcessor> (SubgraphConfig::fromCreationData (props.creationData));
            processor->setNodeFactory (makeProcessorFactory());

            std::unique_ptr<yup::AudioProcessor> result = std::move (processor);
            return yup::makeResultValueOk (std::move (result));
        },
            [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*) -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* subgraph = dynamic_cast<SubgraphProcessor*> (proc);
            if (subgraph == nullptr)
                return nullptr;

            return std::make_unique<SubgraphNodeView> (nodeID, *subgraph);
        }
        };
    }

#if YUP_DESKTOP
    //==============================================================================
    /** Loads the plugin described by a node, in process or sandboxed as the host decides. */
    using PluginLoader = std::function<yup::ResultValue<std::unique_ptr<yup::AudioPluginInstance>> (const yup::AudioPluginDescription&)>;

    /**
        Registers the factory entries of every plugin format identifier.

        Plugin nodes, including the ones restored from a saved graph, are loaded with
        @p loader and shown with a PluginNodeView.
    */
    void registerPluginFormats (PluginLoader loader)
    {
        pluginLoader = std::move (loader);

        auto processorFactory = [this] (const yup::AudioGraphNodeProperties& props)
            -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            const auto desc = creationDataToDescription (props.creationData);
            if (! desc.has_value())
                return yup::makeResultValueFail ("Invalid plugin description in node: " + props.name);

            auto result = pluginLoader (*desc);
            if (result.failed())
                return yup::makeResultValueFail (result.getErrorMessage());

            std::unique_ptr<yup::AudioProcessor> processor = std::move (result.getReference());
            return yup::makeResultValueOk (std::move (processor));
        };

        auto viewFactory = [] (yup::AudioGraphNodeID nodeID, yup::AudioProcessor* proc, yup::AudioGraphProcessor*)
            -> std::unique_ptr<yup::AudioGraphNodeView>
        {
            auto* instance = dynamic_cast<yup::AudioPluginInstance*> (proc);
            if (instance == nullptr)
                return nullptr;

            const bool sandboxed = dynamic_cast<yup::RemoteAudioPluginInstance*> (proc) != nullptr;
            return std::make_unique<PluginNodeView> (nodeID, instance->getDescription(), sandboxed);
        };

        entries[pluginVst3Identifier] = { processorFactory, viewFactory };
        entries[pluginClapIdentifier] = { processorFactory, viewFactory };
        entries[pluginAuIdentifier] = { processorFactory, viewFactory };
    }

    //==============================================================================
    /** Replaces the list of discovered plugins, sorted by format, vendor and name. */
    void setDiscoveredPlugins (std::vector<yup::AudioPluginDescription> plugins)
    {
        std::sort (plugins.begin(), plugins.end(), [] (const auto& a, const auto& b)
        {
            if (a.formatType != b.formatType)
                return formatTypeToDisplayName (a.formatType) < formatTypeToDisplayName (b.formatType);

            if (const auto order = getVendorName (a).compareIgnoreCase (getVendorName (b)); order != 0)
                return order < 0;

            return a.name.compareIgnoreCase (b.name) < 0;
        });

        discoveredPlugins = std::move (plugins);
    }

    const std::vector<yup::AudioPluginDescription>& getDiscoveredPlugins() const noexcept
    {
        return discoveredPlugins;
    }

    //==============================================================================
    static yup::String formatTypeToDisplayName (yup::AudioPluginFormatType type)
    {
        switch (type)
        {
            case yup::AudioPluginFormatType::vst3:
                return "VST3";

            case yup::AudioPluginFormatType::clap:
                return "CLAP";

            case yup::AudioPluginFormatType::audioUnit:
                return "Audio Unit";

            default:
                return "Other";
        }
    }

    static yup::String getVendorName (const yup::AudioPluginDescription& desc)
    {
        return desc.vendor.trim().isEmpty() ? yup::String ("Unknown Vendor") : desc.vendor.trim();
    }

    //==============================================================================
    /**
        Returns the node registry identifier that corresponds to the given plugin description.

        @param desc  A plugin description whose formatType drives the selection.
        @returns     One of pluginVst3Identifier, pluginClapIdentifier, or pluginAuIdentifier.
    */
    static yup::String identifierForDescription (const yup::AudioPluginDescription& desc)
    {
        switch (desc.formatType)
        {
            case yup::AudioPluginFormatType::vst3:
                return pluginVst3Identifier;

            case yup::AudioPluginFormatType::clap:
                return pluginClapIdentifier;

            case yup::AudioPluginFormatType::audioUnit:
                return pluginAuIdentifier;

            default:
                return pluginUnknownIdentifier;
        }
    }

    //==============================================================================
    /** Serializes a plugin description for AudioGraphNodeProperties::creationData. */
    static yup::MemoryBlock descriptionToCreationData (const yup::AudioPluginDescription& desc)
    {
        yup::XmlElement xml ("PluginDescription");
        xml.setAttribute ("name", desc.name);
        xml.setAttribute ("vendor", desc.vendor);
        xml.setAttribute ("version", desc.version);
        xml.setAttribute ("category", desc.category);
        xml.setAttribute ("identifier", desc.identifier);
        xml.setAttribute ("fileOrBundlePath", desc.fileOrBundlePath);
        xml.setAttribute ("isV3", desc.isV3 ? 1 : 0);
        xml.setAttribute ("isInstrument", desc.isInstrument ? 1 : 0);
        xml.setAttribute ("isEffect", desc.isEffect ? 1 : 0);
        xml.setAttribute ("numInputChannels", desc.numInputChannels);
        xml.setAttribute ("numOutputChannels", desc.numOutputChannels);
        xml.setAttribute ("numMidiInputPorts", desc.numMidiInputPorts);
        xml.setAttribute ("numMidiOutputPorts", desc.numMidiOutputPorts);
        xml.setAttribute ("formatType", formatTypeToKey (desc.formatType));

        yup::MemoryBlock block;
        yup::MemoryOutputStream stream (block, false);
        xml.writeTo (stream);
        stream.flush();

        return block;
    }

    /** Decodes a plugin description written by descriptionToCreationData(). */
    static std::optional<yup::AudioPluginDescription> creationDataToDescription (const yup::MemoryBlock& creationData)
    {
        auto xml = yup::parseXML (creationData.toString());
        if (xml == nullptr || ! xml->hasTagName ("PluginDescription"))
            return std::nullopt;

        yup::AudioPluginDescription desc;
        desc.name = xml->getStringAttribute ("name");
        desc.vendor = xml->getStringAttribute ("vendor");
        desc.version = xml->getStringAttribute ("version");
        desc.category = xml->getStringAttribute ("category");
        desc.identifier = xml->getStringAttribute ("identifier");
        desc.fileOrBundlePath = xml->getStringAttribute ("fileOrBundlePath");
        desc.isV3 = xml->getBoolAttribute ("isV3", false);
        desc.isInstrument = xml->getBoolAttribute ("isInstrument", false);
        desc.isEffect = xml->getBoolAttribute ("isEffect", false);
        desc.numInputChannels = xml->getIntAttribute ("numInputChannels", 0);
        desc.numOutputChannels = xml->getIntAttribute ("numOutputChannels", 0);
        desc.numMidiInputPorts = xml->getIntAttribute ("numMidiInputPorts", desc.isInstrument ? 1 : 0);
        desc.numMidiOutputPorts = xml->getIntAttribute ("numMidiOutputPorts", 0);

        for (const auto type : { yup::AudioPluginFormatType::vst3, yup::AudioPluginFormatType::clap, yup::AudioPluginFormatType::audioUnit })
        {
            if (xml->getStringAttribute ("formatType") == formatTypeToKey (type))
                desc.formatType = type;
        }

        if (desc.formatType == yup::AudioPluginFormatType::unknown)
            return std::nullopt;

        return desc;
    }
#endif

    //==============================================================================
    /**
        Returns an AudioGraphModel::NodeFactory that dispatches to the
        registered processor factory for a given node identifier.

        If no entry is found for props.identifier, the factory returns a failure
        result.
    */
    yup::AudioGraphModel::NodeFactory makeProcessorFactory()
    {
        return [this] (const yup::AudioGraphNodeProperties& props)
                   -> yup::ResultValue<std::unique_ptr<yup::AudioProcessor>>
        {
            auto it = entries.find (props.identifier);
            if (it == entries.end())
                return yup::makeResultValueFail ("Unknown node: " + props.identifier);

            return it->second.createProcessor (props);
        };
    }

    //==============================================================================
    /**
        Creates the visual representation for an already-constructed node.

        @param nodeID      The stable graph node identifier.
        @param identifier  The registry key identifying the node type.
        @param proc        The processor that was created for this node.
        @param graph       The graph that owns the node, used by graph-aware views.
        @returns           A new view, or nullptr if the identifier is unknown.
    */
    std::unique_ptr<yup::AudioGraphNodeView> createView (yup::AudioGraphNodeID nodeID,
                                                         const yup::String& identifier,
                                                         yup::AudioProcessor* proc,
                                                         yup::AudioGraphProcessor* graph = nullptr)
    {
        auto it = entries.find (identifier);
        if (it == entries.end())
            return nullptr;

        return it->second.createView (nodeID, proc, graph);
    }

    //==============================================================================
    std::vector<yup::String> getInternalNodeIdentifiers() const
    {
        return {
            aaIirHardClipperIdentifier,
            blunterSoftClipperIdentifier,
            fractionalDelayIdentifier,
            gainIdentifier,
            latencyIdentifier,
            oscillatorIdentifier,
            oscilloscopeIdentifier,
            recorderIdentifier,
            samplePlayerIdentifier,
            spectralPassthroughIdentifier,
            spectrumAnalyzerIdentifier,
            subgraphIdentifier,
            svfIdentifier,
            tanhDistortionIdentifier,
        };
    }

    //==============================================================================
    /**
        Maps a node registry identifier to a human-readable display name.

        @param id  A registry identifier string.
        @returns   A display name such as "Oscillator", "Gain", or "Low Pass Filter",
                   or the original identifier if unrecognised.
    */
    static yup::String identifierToDisplayName (const yup::String& id)
    {
        if (id == oscillatorIdentifier)
            return "Oscillator";

        if (id == gainIdentifier)
            return "Gain";

        if (id == latencyIdentifier)
            return "Latency";

        if (id == fractionalDelayIdentifier)
            return "Fractional Delay";

        if (id == svfIdentifier)
            return "State Variable Filter";

        if (id == oscilloscopeIdentifier)
            return "Oscilloscope";

        if (id == spectrumAnalyzerIdentifier)
            return "Spectrum Analyzer";

        if (id == tanhDistortionIdentifier)
            return "Tanh Distortion";

        if (id == blunterSoftClipperIdentifier)
            return "Blunter Soft Clip";

        if (id == aaIirHardClipperIdentifier)
            return "AA-IIR Hard Clip";

        if (id == samplePlayerIdentifier)
            return "Sample Player";

        if (id == recorderIdentifier)
            return "Recorder";

        if (id == spectralPassthroughIdentifier)
            return "Spectral Passthrough";

        if (id == subgraphIdentifier)
            return "Subgraph";

        return id;
    }

private:
    //==============================================================================
    std::map<yup::String, Entry> entries;

#if YUP_DESKTOP
    PluginLoader pluginLoader;
    std::vector<yup::AudioPluginDescription> discoveredPlugins;

    static yup::String formatTypeToKey (yup::AudioPluginFormatType type)
    {
        switch (type)
        {
            case yup::AudioPluginFormatType::vst3:
                return "vst3";

            case yup::AudioPluginFormatType::clap:
                return "clap";

            case yup::AudioPluginFormatType::audioUnit:
                return "au";

            default:
                return "unknown";
        }
    }
#endif
};
