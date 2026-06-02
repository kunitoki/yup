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

#include <memory>
#include <vector>

#include "NodeViewHelpers.h"

//==============================================================================
struct SubgraphConfig
{
    enum Preset
    {
        mono = 1,
        stereo,
        monoMidi,
        stereoMidi
    };

    int audioChannels = 2;
    bool midiEnabled = false;

    static SubgraphConfig fromPreset (int presetID)
    {
        switch (presetID)
        {
            case mono:
                return { 1, false };
            case stereo:
                return { 2, false };
            case monoMidi:
                return { 1, true };
            case stereoMidi:
                return { 2, true };
            default:
                return {};
        }
    }

    int getPresetID() const noexcept
    {
        if (audioChannels == 1)
            return midiEnabled ? monoMidi : mono;

        return midiEnabled ? stereoMidi : stereo;
    }

    yup::String getDisplayName() const
    {
        return audioChannels == 1 ? "Mono Subgraph" : "Stereo Subgraph";
    }

    yup::String getSubtitle() const
    {
        auto text = audioChannels == 1 ? yup::String ("mono") : yup::String ("stereo");

        if (midiEnabled)
            text += " + MIDI";

        return text;
    }

    static yup::MemoryBlock toCreationData (const SubgraphConfig& config)
    {
        yup::XmlElement xml ("SubgraphConfig");
        xml.setAttribute ("audioChannels", yup::jlimit (1, 2, config.audioChannels));
        xml.setAttribute ("midiEnabled", config.midiEnabled ? 1 : 0);

        yup::MemoryBlock block;
        yup::MemoryOutputStream stream (block, false);
        xml.writeTo (stream);
        stream.flush();
        return block;
    }

    static SubgraphConfig fromCreationData (const yup::MemoryBlock& block)
    {
        if (block.getSize() == 0)
            return {};

        yup::MemoryInputStream stream (block, false);
        auto xml = yup::parseXML (stream.readEntireStreamAsString());

        if (xml == nullptr || ! xml->hasTagName ("SubgraphConfig"))
            return {};

        SubgraphConfig config;
        config.audioChannels = yup::jlimit (1, 2, xml->getIntAttribute ("audioChannels", 2));
        config.midiEnabled = xml->getBoolAttribute ("midiEnabled", false);
        return config;
    }
};

//==============================================================================
class SubgraphProcessor final : public yup::AudioProcessor
{
public:
    explicit SubgraphProcessor (SubgraphConfig configIn = {})
        : AudioProcessor ("Subgraph", createBusLayout (configIn))
        , config (configIn)
        , model (std::make_shared<yup::AudioGraphModel>())
        , graph (std::make_shared<yup::AudioGraphProcessor> (model, createBusLayout (configIn)))
    {
    }

    const SubgraphConfig& getConfig() const noexcept { return config; }

    std::shared_ptr<yup::AudioGraphProcessor> getGraph() const noexcept { return graph; }

    std::shared_ptr<yup::AudioGraphModel> getModel() const noexcept { return model; }

    void setNodeFactory (yup::AudioGraphModel::NodeFactory factory)
    {
        model->setNodeFactory (std::move (factory));
    }

    void prepareToPlay (float sampleRate, int maxBlockSize) override
    {
        graph->prepareToPlay (sampleRate, maxBlockSize);
    }

    void releaseResources() override
    {
        graph->releaseResources();
    }

    void processBlock (yup::AudioProcessContext<float>& context) override
    {
        graph->processBlock (context);
    }

    void flush() override
    {
        graph->flush();
    }

    int getLatencySamples() override
    {
        return graph->getLatencySamples();
    }

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    yup::String getPresetName (int) const override { return {}; }

    void setPresetName (int, yup::StringRef) override {}

    yup::Result loadStateFromMemory (const yup::MemoryBlock& data) override
    {
        if (data.getSize() == 0)
            return yup::Result::ok();

        yup::MemoryInputStream stream (data, false);
        auto xml = yup::parseXML (stream.readEntireStreamAsString());

        if (xml == nullptr || ! xml->hasTagName ("SubgraphState"))
            return yup::Result::fail ("Invalid subgraph state");

        if (xml->getIntAttribute ("version", 0) != 1)
            return yup::Result::fail ("Unsupported subgraph state version");

        auto* graphState = xml->getChildByName ("graphState");
        if (graphState == nullptr || graphState->getStringAttribute ("encoding") != "base64")
            return yup::Result::fail ("Subgraph state is missing graph data");

        yup::MemoryBlock graphBlock;
        yup::MemoryOutputStream output (graphBlock, false);
        const auto base64Text = graphState->getAllSubText().removeCharacters (" \t\r\n");

        if (! yup::Base64::convertFromBase64 (output, base64Text))
            return yup::Result::fail ("Subgraph state has invalid graph data");

        output.flush();

        SubgraphConfig savedConfig;
        savedConfig.audioChannels = yup::jlimit (1, 2, xml->getIntAttribute ("audioChannels", config.audioChannels));
        savedConfig.midiEnabled = xml->getBoolAttribute ("midiEnabled", config.midiEnabled);

        return loadPrunedGraphState (graphBlock, savedConfig);
    }

    yup::Result saveStateIntoMemory (yup::MemoryBlock& data) override
    {
        yup::MemoryBlock graphBlock;
        const auto result = graph->saveStateIntoMemory (graphBlock);

        if (result.failed())
            return result;

        yup::XmlElement xml ("SubgraphState");
        xml.setAttribute ("version", 1);
        xml.setAttribute ("audioChannels", yup::jlimit (1, 2, config.audioChannels));
        xml.setAttribute ("midiEnabled", config.midiEnabled ? 1 : 0);

        auto* graphState = new yup::XmlElement ("graphState");
        graphState->setAttribute ("encoding", "base64");
        graphState->addTextElement (yup::Base64::toBase64 (graphBlock.getData(), graphBlock.getSize()));
        xml.addChildElement (graphState);

        yup::MemoryOutputStream stream (data, false);
        xml.writeTo (stream);
        stream.flush();
        return yup::Result::ok();
    }

    bool hasEditor() const override { return false; }

    yup::AudioProcessorEditor* createEditor() override { return nullptr; }

    static yup::AudioBusLayout createBusLayout (const SubgraphConfig& config)
    {
        std::vector<yup::AudioBus> inputs;
        std::vector<yup::AudioBus> outputs;

        const auto channels = yup::jlimit (1, 2, config.audioChannels);
        inputs.emplace_back ("Audio In", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Input, channels);
        outputs.emplace_back ("Audio Out", yup::AudioBus::Type::Audio, yup::AudioBus::Direction::Output, channels);

        if (config.midiEnabled)
        {
            inputs.emplace_back ("MIDI In", yup::AudioBus::Type::MIDI, yup::AudioBus::Direction::Input, 0);
            outputs.emplace_back ("MIDI Out", yup::AudioBus::Type::MIDI, yup::AudioBus::Direction::Output, 0);
        }

        return yup::AudioBusLayout (std::move (inputs), std::move (outputs));
    }

private:
    yup::Result loadPrunedGraphState (const yup::MemoryBlock& graphBlock, const SubgraphConfig& savedConfig)
    {
        yup::MemoryInputStream stream (graphBlock, false);
        auto xml = yup::parseXML (stream.readEntireStreamAsString());

        if (xml == nullptr)
            return yup::Result::fail ("Invalid nested graph state");

        pruneInvalidGraphEndpointConnections (*xml, savedConfig);

        yup::MemoryBlock prunedBlock;
        yup::MemoryOutputStream output (prunedBlock, false);
        xml->writeTo (output);
        output.flush();

        return graph->loadStateFromMemory (prunedBlock);
    }

    void pruneInvalidGraphEndpointConnections (yup::XmlElement& graphXml, const SubgraphConfig& savedConfig) const
    {
        auto* connections = graphXml.getChildByName ("connections");
        if (connections == nullptr)
            return;

        std::vector<yup::XmlElement*> toRemove;

        for (auto* connection : connections->getChildWithTagNameIterator ("connection"))
        {
            auto* source = connection->getChildByName ("source");
            auto* destination = connection->getChildByName ("destination");

            if ((source != nullptr && ! graphEndpointIsValid (*source, savedConfig))
                || (destination != nullptr && ! graphEndpointIsValid (*destination, savedConfig)))
                toRemove.push_back (connection);
        }

        for (auto* connection : toRemove)
            connections->removeChildElement (connection, true);
    }

    bool graphEndpointIsValid (const yup::XmlElement& endpoint, const SubgraphConfig& savedConfig) const
    {
        const auto kind = endpoint.getStringAttribute ("kind");

        if (kind != "graphInput" && kind != "graphOutput")
            return true;

        const int busIndex = endpoint.getIntAttribute ("busIndex", -1);
        const auto previousLayout = createBusLayout (savedConfig);
        const auto previousBuses = kind == "graphInput" ? previousLayout.getInputBuses()
                                                        : previousLayout.getOutputBuses();
        const auto currentBuses = kind == "graphInput" ? getBusLayout().getInputBuses()
                                                       : getBusLayout().getOutputBuses();

        if (busIndex < 0
            || busIndex >= static_cast<int> (previousBuses.size())
            || busIndex >= static_cast<int> (currentBuses.size()))
            return false;

        const auto& previousBus = previousBuses[static_cast<size_t> (busIndex)];
        const auto& currentBus = currentBuses[static_cast<size_t> (busIndex)];

        return previousBus.getType() == currentBus.getType()
            && previousBus.getNumChannels() == currentBus.getNumChannels();
    }

    SubgraphConfig config;
    std::shared_ptr<yup::AudioGraphModel> model;
    std::shared_ptr<yup::AudioGraphProcessor> graph;
};

//==============================================================================
class SubgraphNodeView final : public yup::AudioGraphNodeView
{
public:
    SubgraphNodeView (yup::AudioGraphNodeID nodeID, SubgraphProcessor& processorIn)
        : AudioGraphNodeView (nodeID)
        , processor (processorIn)
    {
    }

    yup::String getNodeTitle() const override { return "SUBGRAPH"; }

    yup::String getNodeSubtitle() const override { return processor.getConfig().getSubtitle(); }

    int getNumInputPorts() const override
    {
        return static_cast<int> (processor.getBusLayout().getInputBuses().size());
    }

    int getNumOutputPorts() const override
    {
        return static_cast<int> (processor.getBusLayout().getOutputBuses().size());
    }

    int getPreferredWidth() const override { return 230; }

    yup::Color getNodeColor() const override { return yup::Color (0xff8b5cf6); }

    PortInfo getInputPortInfo (int busIndex) const override
    {
        return getPortInfo (processor.getBusLayout().getInputBuses(), busIndex);
    }

    PortInfo getOutputPortInfo (int busIndex) const override
    {
        return getPortInfo (processor.getBusLayout().getOutputBuses(), busIndex);
    }

private:
    static PortInfo getPortInfo (yup::Span<const yup::AudioBus> buses, int busIndex)
    {
        if (busIndex < 0 || busIndex >= static_cast<int> (buses.size()))
            return { "?", getPortKindColor (PortKind::audio), PortKind::audio };

        const auto& bus = buses[static_cast<size_t> (busIndex)];
        const auto kind = bus.getType() == yup::AudioBus::Type::Audio ? PortKind::audio
                                                                      : PortKind::midi;

        return { bus.getName(), getPortKindColor (kind), kind };
    }

    SubgraphProcessor& processor;
};
