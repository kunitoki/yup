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

namespace yup
{

namespace
{
constexpr int audioGraphModelStateVersion = 1;
constexpr const char* audioGraphModelStateTag = "YUPAudioGraphState";

enum class ModelSignalType
{
    audio,
    midi
};

bool modelIsValidBusIndex (Span<const AudioBus> buses, int busIndex)
{
    return busIndex >= 0 && busIndex < static_cast<int> (buses.size());
}

ModelSignalType modelToSignalType (AudioBus::Type busType)
{
    return busType == AudioBus::Type::Audio ? ModelSignalType::audio : ModelSignalType::midi;
}

struct ModelEndpointDescriptor
{
    bool valid = false;
    ModelSignalType type = ModelSignalType::audio;
    int channels = 0;
};

ModelEndpointDescriptor modelDescribeNodeEndpoint (const AudioProcessor& processor, const AudioGraphEndpoint& endpoint)
{
    const auto buses = endpoint.getKind() == AudioGraphEndpoint::Kind::nodeInput
                         ? processor.getBusLayout().getInputBuses()
                         : processor.getBusLayout().getOutputBuses();

    if (! modelIsValidBusIndex (buses, endpoint.getBusIndex()))
        return {};

    const auto& bus = buses[static_cast<size_t> (endpoint.getBusIndex())];
    return { true,
             modelToSignalType (bus.getType()),
             bus.getType() == AudioBus::Type::Audio ? bus.getNumChannels() : 0 };
}

bool modelConnectionEndpointIsStillCompatible (const AudioGraphEndpoint& endpoint,
                                               AudioGraphNodeID replacedNodeID,
                                               const AudioProcessor* oldProcessor,
                                               const AudioProcessor* newProcessor)
{
    if (endpoint.getNodeID() != replacedNodeID)
        return true;

    if (endpoint.getKind() != AudioGraphEndpoint::Kind::nodeInput
        && endpoint.getKind() != AudioGraphEndpoint::Kind::nodeOutput)
        return true;

    const auto oldDescriptor = modelDescribeNodeEndpoint (*oldProcessor, endpoint);
    const auto newDescriptor = modelDescribeNodeEndpoint (*newProcessor, endpoint);

    return oldDescriptor.valid
        && newDescriptor.valid
        && oldDescriptor.type == newDescriptor.type
        && oldDescriptor.channels == newDescriptor.channels;
}

AudioGraphNodeProperties modelMakeDefaultNodeProperties (const AudioProcessor& processor)
{
    AudioGraphNodeProperties properties;
    properties.identifier = processor.getName();
    properties.name = processor.getName();
    return properties;
}

AudioGraphNodeProperties modelMakeBoundaryNodeProperties (AudioGraphModel::NodeKind kind)
{
    AudioGraphNodeProperties properties;
    properties.identifier = kind == AudioGraphModel::NodeKind::graphInput ? "graphInput" : "graphOutput";
    properties.name = kind == AudioGraphModel::NodeKind::graphInput ? "Graph Input" : "Graph Output";
    return properties;
}

void modelWriteBase64Element (XmlElement& parent, const char* tagName, const MemoryBlock& block)
{
    auto* element = new XmlElement (tagName);
    element->setAttribute ("encoding", "base64");
    element->addTextElement (Base64::toBase64 (block.getData(), block.getSize()));
    parent.addChildElement (element);
}

Result modelReadBase64Element (const XmlElement& parent, const char* tagName, MemoryBlock& block)
{
    auto* element = parent.getChildByName (tagName);

    if (element == nullptr)
        return Result::fail (String ("Audio graph state is missing ") + tagName);

    if (element->getStringAttribute ("encoding") != "base64")
        return Result::fail (String ("Audio graph state has unsupported ") + tagName + " encoding");

    MemoryOutputStream output (block, false);
    const auto base64Text = element->getAllSubText().removeCharacters (" \t\r\n");

    if (! Base64::convertFromBase64 (output, base64Text))
        return Result::fail (String ("Audio graph state has invalid ") + tagName + " data");

    output.flush();
    return Result::ok();
}

void modelWriteDataTreeElement (XmlElement& parent, const char* tagName, const DataTree& state)
{
    auto xml = state.createXml();
    if (xml == nullptr)
        return;

    auto* element = new XmlElement (tagName);
    element->addChildElement (xml.release());
    parent.addChildElement (element);
}

Result modelReadDataTreeElement (const XmlElement& parent, const char* tagName, DataTree& state)
{
    auto* element = parent.getChildByName (tagName);

    if (element == nullptr)
        return Result::fail (String ("Audio graph state is missing ") + tagName);

    auto* stateXml = element->getFirstChildElement();
    if (stateXml == nullptr)
        return Result::fail (String ("Audio graph state has invalid ") + tagName + " data");

    state = DataTree::fromXml (*stateXml);
    if (! state.isValid())
        return Result::fail (String ("Audio graph state has invalid ") + tagName + " DataTree");

    return Result::ok();
}

void modelWriteCreationDataElement (XmlElement& parent, const MemoryBlock& block)
{
    if (block.getSize() == 0)
        return;

    MemoryInputStream stream (block, false);
    auto xml = parseXML (stream.readEntireStreamAsString());
    if (xml == nullptr)
        return;

    auto* element = new XmlElement ("creationData");
    element->addChildElement (new XmlElement (*xml));
    parent.addChildElement (element);
}

Result modelReadCreationDataElement (const XmlElement& parent, MemoryBlock& block)
{
    auto* element = parent.getChildByName ("creationData");
    if (element == nullptr)
    {
        block.reset();
        return Result::ok();
    }

    auto* creationXml = element->getFirstChildElement();
    if (creationXml == nullptr)
    {
        block.reset();
        return Result::ok();
    }

    block.reset();
    MemoryOutputStream stream (block, false);
    creationXml->writeTo (stream);
    stream.flush();
    return Result::ok();
}

void modelWriteNodeProperties (XmlElement& element, const AudioGraphNodeProperties& properties)
{
    element.setAttribute ("identifier", properties.identifier);
    element.setAttribute ("name", properties.name);
    element.setAttribute ("positionX", static_cast<double> (properties.positionX));
    element.setAttribute ("positionY", static_cast<double> (properties.positionY));
    modelWriteCreationDataElement (element, properties.creationData);
}

Result modelReadNodeProperties (const XmlElement& element, AudioGraphNodeProperties& properties)
{
    properties.identifier = element.getStringAttribute ("identifier");
    properties.name = element.getStringAttribute ("name");
    properties.positionX = static_cast<float> (element.getDoubleAttribute ("positionX"));
    properties.positionY = static_cast<float> (element.getDoubleAttribute ("positionY"));

    return modelReadCreationDataElement (element, properties.creationData);
}

String modelEndpointKindToString (AudioGraphEndpoint::Kind kind)
{
    switch (kind)
    {
        case AudioGraphEndpoint::Kind::graphInput:
            return "graphInput";

        case AudioGraphEndpoint::Kind::graphOutput:
            return "graphOutput";

        case AudioGraphEndpoint::Kind::nodeInput:
            return "nodeInput";

        case AudioGraphEndpoint::Kind::nodeOutput:
            return "nodeOutput";
    }

    return {};
}

Result modelEndpointKindFromString (const String& text, AudioGraphEndpoint::Kind& kind)
{
    if (text == "graphInput")
    {
        kind = AudioGraphEndpoint::Kind::graphInput;
        return Result::ok();
    }

    if (text == "graphOutput")
    {
        kind = AudioGraphEndpoint::Kind::graphOutput;
        return Result::ok();
    }

    if (text == "nodeInput")
    {
        kind = AudioGraphEndpoint::Kind::nodeInput;
        return Result::ok();
    }

    if (text == "nodeOutput")
    {
        kind = AudioGraphEndpoint::Kind::nodeOutput;
        return Result::ok();
    }

    return Result::fail ("Audio graph state has an invalid endpoint kind");
}

void modelWriteEndpoint (XmlElement& element, const AudioGraphEndpoint& endpoint)
{
    element.setAttribute ("kind", modelEndpointKindToString (endpoint.getKind()));
    element.setAttribute ("nodeID", String (static_cast<int64> (endpoint.getNodeID().getRawID())));
    element.setAttribute ("busIndex", endpoint.getBusIndex());
}

Result modelReadEndpoint (const XmlElement& element, AudioGraphEndpoint& endpoint)
{
    auto kind = AudioGraphEndpoint::Kind::graphInput;
    const auto result = modelEndpointKindFromString (element.getStringAttribute ("kind"), kind);

    if (result.failed())
        return result;

    const auto nodeID = AudioGraphNodeID (static_cast<uint64_t> (element.getStringAttribute ("nodeID").getLargeIntValue()));
    const int busIndex = element.getIntAttribute ("busIndex");

    switch (kind)
    {
        case AudioGraphEndpoint::Kind::graphInput:
            if (nodeID.isValid())
                return Result::fail ("Audio graph state has an invalid graph input endpoint");

            endpoint = AudioGraphEndpoint::graphInput (busIndex);
            return Result::ok();

        case AudioGraphEndpoint::Kind::graphOutput:
            if (nodeID.isValid())
                return Result::fail ("Audio graph state has an invalid graph output endpoint");

            endpoint = AudioGraphEndpoint::graphOutput (busIndex);
            return Result::ok();

        case AudioGraphEndpoint::Kind::nodeInput:
            endpoint = AudioGraphEndpoint::nodeInput (nodeID, busIndex);
            return Result::ok();

        case AudioGraphEndpoint::Kind::nodeOutput:
            endpoint = AudioGraphEndpoint::nodeOutput (nodeID, busIndex);
            return Result::ok();
    }

    return Result::fail ("Audio graph state has an invalid endpoint kind");
}

DataTree modelCreateEndpointTree (const Identifier& type, const AudioGraphEndpoint& endpoint)
{
    return DataTree (type,
                     { { "kind", modelEndpointKindToString (endpoint.getKind()) },
                       { "nodeID", static_cast<int64> (endpoint.getNodeID().getRawID()) },
                       { "busIndex", endpoint.getBusIndex() } });
}
} // namespace

//==============================================================================
AudioGraphModel::AudioGraphModel()
{
    resetBoundaryNodes();
    rebuildDataTree();
}

AudioGraphModel::~AudioGraphModel() = default;

AudioGraphNodeID AudioGraphModel::addNode (std::unique_ptr<AudioProcessor> processor)
{
    if (processor == nullptr)
        return AudioGraphNodeID::invalid();

    auto properties = modelMakeDefaultNodeProperties (*processor);
    return addNode (std::move (processor), std::move (properties));
}

AudioGraphNodeID AudioGraphModel::addNode (std::unique_ptr<AudioProcessor> processor,
                                           AudioGraphNodeProperties properties)
{
    if (processor == nullptr)
        return AudioGraphNodeID::invalid();

    const std::lock_guard<std::mutex> lock (mutex);

    const auto id = AudioGraphNodeID (++nextNodeID);
    if (properties.name.isEmpty())
        properties.name = processor->getName();

    nodes.push_back ({ NodeKind::processor, id, std::shared_ptr<AudioProcessor> (std::move (processor)), std::move (properties) });
    markTopologyChanged();
    return id;
}

bool AudioGraphModel::removeNode (AudioGraphNodeID nodeID)
{
    if (! nodeID.isValid())
        return false;

    const std::lock_guard<std::mutex> lock (mutex);

    const auto nodeIterator = std::find_if (nodes.begin(), nodes.end(), [nodeID] (const ModelNode& node)
    {
        return node.kind == NodeKind::processor && node.id == nodeID;
    });

    if (nodeIterator == nodes.end())
        return false;

    nodes.erase (nodeIterator);

    connections.erase (std::remove_if (connections.begin(), connections.end(), [nodeID] (const AudioGraphConnection& connection)
    {
        return connection.source.getNodeID() == nodeID || connection.destination.getNodeID() == nodeID;
    }),
                       connections.end());

    markTopologyChanged();
    return true;
}

Result AudioGraphModel::replaceNode (AudioGraphNodeID nodeID,
                                     std::unique_ptr<AudioProcessor> processor,
                                     AudioGraphNodeProperties properties)
{
    if (! nodeID.isValid())
        return Result::fail ("Audio graph node ID is invalid");

    if (processor == nullptr)
        return Result::fail ("Audio graph replacement processor is empty");

    const std::lock_guard<std::mutex> lock (mutex);

    const auto nodeIterator = std::find_if (nodes.begin(), nodes.end(), [nodeID] (const ModelNode& node)
    {
        return node.kind == NodeKind::processor && node.id == nodeID;
    });

    if (nodeIterator == nodes.end())
        return Result::fail ("Audio graph node does not exist");

    auto replacement = std::shared_ptr<AudioProcessor> (std::move (processor));

    if (properties.name.isEmpty())
        properties.name = replacement->getName();

    auto& node = *nodeIterator;
    auto oldProcessor = node.processor;

    connections.erase (std::remove_if (connections.begin(), connections.end(), [nodeID, &oldProcessor, &replacement] (const AudioGraphConnection& connection)
    {
        return ! modelConnectionEndpointIsStillCompatible (connection.source, nodeID, oldProcessor.get(), replacement.get())
            || ! modelConnectionEndpointIsStillCompatible (connection.destination, nodeID, oldProcessor.get(), replacement.get());
    }),
                       connections.end());

    node.processor = std::move (replacement);
    node.properties = std::move (properties);

    markTopologyChanged();
    return Result::ok();
}

Result AudioGraphModel::addConnection (const AudioGraphConnection& connection)
{
    const std::lock_guard<std::mutex> lock (mutex);

    if (const auto result = validateConnectionLocked (connection); result.failed())
        return result;

    connections.push_back (connection);
    markTopologyChanged();
    return Result::ok();
}

bool AudioGraphModel::removeConnection (const AudioGraphConnection& connection)
{
    const std::lock_guard<std::mutex> lock (mutex);
    const auto oldSize = connections.size();

    connections.erase (std::remove (connections.begin(), connections.end(), connection), connections.end());

    const bool removed = connections.size() != oldSize;
    if (removed)
        markTopologyChanged();

    return removed;
}

std::vector<AudioGraphConnection> AudioGraphModel::getConnections() const
{
    const std::lock_guard<std::mutex> lock (mutex);
    return connections;
}

void AudioGraphModel::clear()
{
    const std::lock_guard<std::mutex> lock (mutex);

    nodes.erase (std::remove_if (nodes.begin(), nodes.end(), [] (const ModelNode& node)
    {
        return node.kind == NodeKind::processor;
    }),
                 nodes.end());
    resetBoundaryNodes();
    connections.clear();
    markTopologyChanged();
}

AudioProcessor* AudioGraphModel::getNodeProcessor (AudioGraphNodeID nodeID) const noexcept
{
    const std::lock_guard<std::mutex> lock (mutex);

    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [nodeID] (const ModelNode& node)
    {
        return node.id == nodeID;
    });

    return iterator != nodes.end() ? iterator->processor.get() : nullptr;
}

bool AudioGraphModel::setNodePosition (AudioGraphNodeID nodeID, float positionX, float positionY)
{
    const std::lock_guard<std::mutex> lock (mutex);

    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [nodeID] (const ModelNode& node)
    {
        return node.id == nodeID;
    });

    if (iterator == nodes.end())
        return false;

    if (iterator->properties.positionX == positionX && iterator->properties.positionY == positionY)
        return true;

    iterator->properties.positionX = positionX;
    iterator->properties.positionY = positionY;

    markMetadataChanged();
    return true;
}

bool AudioGraphModel::setNodeProperties (AudioGraphNodeID nodeID, AudioGraphNodeProperties properties)
{
    const std::lock_guard<std::mutex> lock (mutex);

    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [nodeID] (const ModelNode& node)
    {
        return node.id == nodeID;
    });

    if (iterator == nodes.end())
        return false;

    if (properties.name.isEmpty() && iterator->processor != nullptr)
        properties.name = iterator->processor->getName();

    iterator->properties = std::move (properties);
    markMetadataChanged();
    return true;
}

std::optional<AudioGraphNodeProperties> AudioGraphModel::getNodeProperties (AudioGraphNodeID nodeID) const
{
    const std::lock_guard<std::mutex> lock (mutex);

    const auto iterator = std::find_if (nodes.begin(), nodes.end(), [nodeID] (const ModelNode& node)
    {
        return node.id == nodeID;
    });

    if (iterator == nodes.end())
        return std::nullopt;

    return iterator->properties;
}

std::vector<AudioGraphNodeID> AudioGraphModel::getNodeIDs() const
{
    const std::lock_guard<std::mutex> lock (mutex);

    std::vector<AudioGraphNodeID> result;
    result.reserve (nodes.size());

    for (const auto& node : nodes)
        if (node.kind == NodeKind::processor)
            result.push_back (node.id);

    return result;
}

void AudioGraphModel::setNodeFactory (AudioGraphModel::NodeFactory factory)
{
    const std::lock_guard<std::mutex> lock (factoryMutex);
    nodeFactory = std::move (factory);
}

ResultValue<std::unique_ptr<XmlElement>> AudioGraphModel::createXml() const
{
    const auto snapshot = createSnapshot();

    std::vector<SavedNodeState> savedNodes;
    savedNodes.reserve (snapshot.nodes.size());

    for (const auto& node : snapshot.nodes)
    {
        if (node.kind != NodeKind::processor)
            continue;

        if (node.processor == nullptr)
            return makeResultValueFail ("Audio graph contains an empty node");

        SavedNodeState savedNode;
        savedNode.id = node.id;
        savedNode.properties = node.properties;

        if (node.processor->supportsDataTreeState())
        {
            savedNode.hasStateTree = true;
            const auto result = node.processor->saveStateIntoDataTree (savedNode.stateTree);

            if (! result)
                return makeResultValueFail ("Audio graph node DataTree state save failed: " + result.getErrorMessage());

            if (! savedNode.stateTree.isValid())
                return makeResultValueFail ("Audio graph node DataTree state save failed: invalid state");
        }
        else
        {
            const auto result = node.processor->saveStateIntoMemory (savedNode.state);

            if (! result)
                return makeResultValueFail ("Audio graph node state save failed: " + result.getErrorMessage());
        }

        savedNodes.push_back (std::move (savedNode));
    }

    auto root = std::make_unique<XmlElement> (audioGraphModelStateTag);
    root->setAttribute ("version", audioGraphModelStateVersion);
    root->setAttribute ("nextNodeID", String (static_cast<int64> (snapshot.nextNodeID)));

    auto* nodesElement = new XmlElement ("nodes");
    root->addChildElement (nodesElement);

    for (const auto& node : savedNodes)
    {
        auto* nodeElement = new XmlElement ("node");
        nodesElement->addChildElement (nodeElement);

        nodeElement->setAttribute ("id", String (static_cast<int64> (node.id.getRawID())));
        modelWriteNodeProperties (*nodeElement, node.properties);

        if (node.hasStateTree)
            modelWriteDataTreeElement (*nodeElement, "stateTree", node.stateTree);
        else
            modelWriteBase64Element (*nodeElement, "state", node.state);
    }

    auto* boundaryNodesElement = new XmlElement ("boundaryNodes");
    root->addChildElement (boundaryNodesElement);

    for (const auto& node : snapshot.nodes)
    {
        const auto isGraphInput = node.kind == NodeKind::graphInput;
        const auto isGraphOutput = node.kind == NodeKind::graphOutput;

        if (! isGraphInput && ! isGraphOutput)
            continue;

        auto* nodeElement = new XmlElement (isGraphInput ? "graphInput" : "graphOutput");
        boundaryNodesElement->addChildElement (nodeElement);
        modelWriteNodeProperties (*nodeElement, node.properties);
    }

    auto* connectionsElement = new XmlElement ("connections");
    root->addChildElement (connectionsElement);

    for (const auto& connection : snapshot.connections)
    {
        auto* connectionElement = new XmlElement ("connection");
        connectionsElement->addChildElement (connectionElement);

        auto* sourceElement = new XmlElement ("source");
        connectionElement->addChildElement (sourceElement);
        modelWriteEndpoint (*sourceElement, connection.source);

        auto* destinationElement = new XmlElement ("destination");
        connectionElement->addChildElement (destinationElement);
        modelWriteEndpoint (*destinationElement, connection.destination);
    }

    return makeResultValueOk (std::move (root));
}

Result AudioGraphModel::restoreFromXml (const XmlElement& xml)
{
    if (! xml.hasTagName (audioGraphModelStateTag))
        return Result::fail ("Audio graph state has an invalid header");

    if (xml.getIntAttribute ("version", 0) != audioGraphModelStateVersion)
        return Result::fail ("Audio graph state has an unsupported version");

    const auto savedNextNodeID = static_cast<uint64_t> (xml.getStringAttribute ("nextNodeID").getLargeIntValue());

    auto* nodesElement = xml.getChildByName ("nodes");
    if (nodesElement == nullptr)
        return Result::fail ("Audio graph state is missing nodes");

    std::vector<SavedNodeState> savedNodes;
    savedNodes.reserve (static_cast<size_t> (nodesElement->getNumChildElements()));

    for (auto* nodeElement : nodesElement->getChildWithTagNameIterator ("node"))
    {
        SavedNodeState savedNode;
        savedNode.id = AudioGraphNodeID (static_cast<uint64_t> (nodeElement->getStringAttribute ("id").getLargeIntValue()));

        if (const auto result = modelReadNodeProperties (*nodeElement, savedNode.properties); result.failed())
            return result;

        if (nodeElement->getChildByName ("stateTree") != nullptr)
        {
            savedNode.hasStateTree = true;

            if (const auto result = modelReadDataTreeElement (*nodeElement, "stateTree", savedNode.stateTree); result.failed())
                return result;
        }
        else if (const auto result = modelReadBase64Element (*nodeElement, "state", savedNode.state); result.failed())
        {
            return result;
        }

        savedNodes.push_back (std::move (savedNode));
    }

    std::optional<AudioGraphNodeProperties> graphInputProperties;
    std::optional<AudioGraphNodeProperties> graphOutputProperties;

    if (auto* boundaryNodesElement = xml.getChildByName ("boundaryNodes"))
    {
        if (auto* graphInputElement = boundaryNodesElement->getChildByName ("graphInput"))
        {
            AudioGraphNodeProperties properties;
            if (const auto result = modelReadNodeProperties (*graphInputElement, properties); result.failed())
                return result;

            graphInputProperties = std::move (properties);
        }

        if (auto* graphOutputElement = boundaryNodesElement->getChildByName ("graphOutput"))
        {
            AudioGraphNodeProperties properties;
            if (const auto result = modelReadNodeProperties (*graphOutputElement, properties); result.failed())
                return result;

            graphOutputProperties = std::move (properties);
        }
    }

    auto* connectionsElement = xml.getChildByName ("connections");
    if (connectionsElement == nullptr)
        return Result::fail ("Audio graph state is missing connections");

    std::vector<AudioGraphConnection> savedConnections;
    savedConnections.reserve (static_cast<size_t> (connectionsElement->getNumChildElements()));

    for (auto* connectionElement : connectionsElement->getChildWithTagNameIterator ("connection"))
    {
        AudioGraphEndpoint source;
        AudioGraphEndpoint destination;

        auto* sourceElement = connectionElement->getChildByName ("source");
        if (sourceElement == nullptr)
            return Result::fail ("Audio graph state connection is missing source endpoint");

        auto* destinationElement = connectionElement->getChildByName ("destination");
        if (destinationElement == nullptr)
            return Result::fail ("Audio graph state connection is missing destination endpoint");

        if (const auto result = modelReadEndpoint (*sourceElement, source); result.failed())
            return result;

        if (const auto result = modelReadEndpoint (*destinationElement, destination); result.failed())
            return result;

        savedConnections.push_back ({ source, destination });
    }

    if (const auto result = restoreModel (savedNextNodeID, std::move (savedNodes), std::move (savedConnections)); result.failed())
        return result;

    if (graphInputProperties.has_value())
        setNodeProperties (getGraphInputNodeID(), std::move (*graphInputProperties));

    if (graphOutputProperties.has_value())
        setNodeProperties (getGraphOutputNodeID(), std::move (*graphOutputProperties));

    return Result::ok();
}

AudioGraphModel::Snapshot AudioGraphModel::createSnapshot() const
{
    const std::lock_guard<std::mutex> lock (mutex);

    AudioGraphModel::Snapshot snapshot;
    snapshot.nodes.reserve (nodes.size());

    for (const auto& node : nodes)
        snapshot.nodes.push_back ({ node.kind, node.id, node.processor, node.properties });

    snapshot.connections = connections;
    snapshot.nextNodeID = nextNodeID;
    snapshot.revision = revision;
    snapshot.topologyRevision = topologyRevision;
    return snapshot;
}

void AudioGraphModel::restoreSnapshot (AudioGraphModel::Snapshot snapshot)
{
    const std::lock_guard<std::mutex> lock (mutex);

    nodes.clear();
    nodes.reserve (snapshot.nodes.size());

    for (auto& node : snapshot.nodes)
        nodes.push_back ({ node.kind, node.id, std::move (node.processor), std::move (node.properties) });

    resetBoundaryNodes();

    connections = std::move (snapshot.connections);
    nextNodeID = snapshot.nextNodeID;
    revision = snapshot.revision;
    topologyRevision = snapshot.topologyRevision;
    rebuildDataTree();
}

uint64_t AudioGraphModel::getRevision() const noexcept
{
    const std::lock_guard<std::mutex> lock (mutex);
    return revision;
}

uint64_t AudioGraphModel::getTopologyRevision() const noexcept
{
    const std::lock_guard<std::mutex> lock (mutex);
    return topologyRevision;
}

Result AudioGraphModel::restoreModel (uint64_t savedNextNodeID,
                                      std::vector<SavedNodeState> savedNodes,
                                      std::vector<AudioGraphConnection> savedConnections)
{
    std::vector<ModelNode> loadedNodes;
    loadedNodes.reserve (savedNodes.size());

    for (auto& savedNode : savedNodes)
    {
        auto processorResult = createProcessorForSavedNode (savedNode.properties);

        if (! processorResult)
            return Result::fail (processorResult.getErrorMessage());

        auto processor = std::move (processorResult).getValue();

        if (processor == nullptr)
            return Result::fail ("Audio graph node factory returned an empty processor");

        const auto result = savedNode.hasStateTree
                              ? (processor->supportsDataTreeState()
                                     ? processor->loadStateFromDataTree (savedNode.stateTree)
                                     : Result::fail ("Audio graph node does not support DataTree state"))
                              : processor->loadStateFromMemory (savedNode.state);

        if (! result)
            return Result::fail ("Audio graph node state load failed: " + result.getErrorMessage());

        loadedNodes.push_back ({ NodeKind::processor,
                                 savedNode.id,
                                 std::shared_ptr<AudioProcessor> (std::move (processor)),
                                 std::move (savedNode.properties) });
    }

    uint64_t highestSavedNodeID = 0;
    for (const auto& savedNode : savedNodes)
        highestSavedNodeID = jmax (highestSavedNodeID, savedNode.id.getRawID());

    {
        const std::lock_guard<std::mutex> lock (mutex);
        nodes = std::move (loadedNodes);
        connections = std::move (savedConnections);
        resetBoundaryNodes();
        nextNodeID = jmax (savedNextNodeID, highestSavedNodeID);
        markTopologyChanged();
    }

    return Result::ok();
}

ResultValue<std::unique_ptr<AudioProcessor>> AudioGraphModel::createProcessorForSavedNode (const AudioGraphNodeProperties& properties)
{
    AudioGraphModel::NodeFactory factoryCopy;

    {
        const std::lock_guard<std::mutex> lock (factoryMutex);
        factoryCopy = nodeFactory;
    }

    if (! factoryCopy)
        return makeResultValueFail ("Audio graph node factory is not configured");

    return factoryCopy (properties);
}

void AudioGraphModel::resetBoundaryNodes()
{
    auto inputProperties = modelMakeBoundaryNodeProperties (NodeKind::graphInput);
    auto outputProperties = modelMakeBoundaryNodeProperties (NodeKind::graphOutput);

    for (const auto& node : nodes)
    {
        if (node.kind == NodeKind::graphInput)
            inputProperties = node.properties;
        else if (node.kind == NodeKind::graphOutput)
            outputProperties = node.properties;
    }

    nodes.erase (std::remove_if (nodes.begin(), nodes.end(), [] (const ModelNode& node)
    {
        return node.kind == NodeKind::graphInput || node.kind == NodeKind::graphOutput;
    }),
                 nodes.end());

    nodes.insert (nodes.begin(), { NodeKind::graphInput, getGraphInputNodeID(), nullptr, std::move (inputProperties) });
    nodes.insert (nodes.begin() + 1, { NodeKind::graphOutput, getGraphOutputNodeID(), nullptr, std::move (outputProperties) });
}

Result AudioGraphModel::validateConnectionLocked (const AudioGraphConnection& connection) const
{
    if (! connection.source.isSource())
        return Result::fail ("Audio graph connection source is not a source endpoint");

    if (! connection.destination.isDestination())
        return Result::fail ("Audio graph connection destination is not a destination endpoint");

    if (std::find (connections.begin(), connections.end(), connection) != connections.end())
        return Result::fail ("Audio graph connection already exists");

    const auto describeEndpoint = [this] (const AudioGraphEndpoint& endpoint) -> ResultValue<std::optional<ModelEndpointDescriptor>>
    {
        if (endpoint.getKind() == AudioGraphEndpoint::Kind::graphInput
            || endpoint.getKind() == AudioGraphEndpoint::Kind::graphOutput)
        {
            return makeResultValueOk (std::optional<ModelEndpointDescriptor>());
        }

        const auto nodeIterator = std::find_if (nodes.begin(), nodes.end(), [nodeID = endpoint.getNodeID()] (const ModelNode& node)
        {
            return node.id == nodeID;
        });

        if (nodeIterator == nodes.end() || nodeIterator->processor == nullptr)
            return makeResultValueFail ("Audio graph connection references a missing node");

        const auto descriptor = modelDescribeNodeEndpoint (*nodeIterator->processor, endpoint);

        if (! descriptor.valid)
            return makeResultValueFail ("Audio graph connection references an invalid bus index");

        return makeResultValueOk (std::optional<ModelEndpointDescriptor> (descriptor));
    };

    auto source = describeEndpoint (connection.source);
    if (! source)
        return Result::fail (source.getErrorMessage());

    auto destination = describeEndpoint (connection.destination);
    if (! destination)
        return Result::fail (destination.getErrorMessage());

    const auto sourceDescriptor = std::move (source).getValue();
    const auto destinationDescriptor = std::move (destination).getValue();

    if (sourceDescriptor.has_value() && destinationDescriptor.has_value())
    {
        if (sourceDescriptor->type != destinationDescriptor->type)
            return Result::fail ("Audio graph connection mixes audio and MIDI endpoints");

        if (sourceDescriptor->type == ModelSignalType::audio && sourceDescriptor->channels != destinationDescriptor->channels)
            return Result::fail ("Audio graph connection has incompatible channel counts");
    }

    return Result::ok();
}

void AudioGraphModel::markTopologyChanged()
{
    ++revision;
    ++topologyRevision;
    rebuildDataTree();
}

void AudioGraphModel::markMetadataChanged()
{
    ++revision;
    rebuildDataTree();
}

void AudioGraphModel::rebuildDataTree()
{
    std::vector<DataTree> nodeTrees;
    nodeTrees.reserve (nodes.size());

    for (const auto& node : nodes)
    {
        nodeTrees.push_back (DataTree ("node",
                                       { { "id", static_cast<int64> (node.id.getRawID()) },
                                         { "identifier", node.properties.identifier },
                                         { "name", node.properties.name },
                                         { "positionX", static_cast<double> (node.properties.positionX) },
                                         { "positionY", static_cast<double> (node.properties.positionY) } }));
    }

    std::vector<DataTree> connectionTrees;
    connectionTrees.reserve (connections.size());

    for (const auto& connection : connections)
    {
        connectionTrees.push_back (DataTree ("connection",
                                             {},
                                             { modelCreateEndpointTree ("source", connection.source),
                                               modelCreateEndpointTree ("destination", connection.destination) }));
    }

    auto nodesTree = DataTree ("nodes");
    {
        auto transaction = nodesTree.beginTransaction();
        for (const auto& node : nodeTrees)
            transaction.addChild (node);
    }

    auto connectionsTree = DataTree ("connections");
    {
        auto transaction = connectionsTree.beginTransaction();
        for (const auto& connection : connectionTrees)
            transaction.addChild (connection);
    }

    data = DataTree ("audioGraphModel",
                     { { "revision", static_cast<int64> (revision) },
                       { "topologyRevision", static_cast<int64> (topologyRevision) },
                       { "nextNodeID", static_cast<int64> (nextNodeID) } },
                     { nodesTree, connectionsTree });
}

} // namespace yup
