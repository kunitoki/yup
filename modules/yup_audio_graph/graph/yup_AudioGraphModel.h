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

//==============================================================================
/**
    Editable control-thread topology for an AudioGraphProcessor.

    AudioGraphModel owns the graph nodes, persistent node metadata, connections,
    node factory, and XML state serialization. AudioGraphProcessor consumes
    immutable model snapshots when compiling a realtime processing plan.

    The model does not own an UndoManager. User and editor code should perform
    undoable commands above this layer and call AudioGraphProcessor::commitChanges()
    after accepted topology edits. Metadata edits such as node positions and
    properties are serialized with the model but do not invalidate the processor's
    compiled processing plan.

    @see AudioGraphProcessor, AudioGraphComponent
*/
class YUP_API AudioGraphModel final
{
public:
    /** Creates a processor for a saved node. */
    using NodeFactory = std::function<ResultValue<std::unique_ptr<AudioProcessor>> (const AudioGraphNodeProperties&)>;

    /** Immutable node entry returned by createSnapshot(). */
    struct NodeSnapshot
    {
        /** Stable node identifier. */
        AudioGraphNodeID id;

        /** Processor owned by the model and shared with compiled graph plans. */
        std::shared_ptr<AudioProcessor> processor;

        /** Persistent node metadata. */
        AudioGraphNodeProperties properties;
    };

    /** Immutable model state for compilation or rollback. */
    struct Snapshot
    {
        /** Nodes in model order. */
        std::vector<NodeSnapshot> nodes;

        /** Connections in model order. */
        std::vector<AudioGraphConnection> connections;

        /** Next raw node ID seed. */
        uint64_t nextNodeID = 0;

        /** Model revision at the time the snapshot was taken. */
        uint64_t revision = 0;

        /** Topology revision at the time the snapshot was taken. */
        uint64_t topologyRevision = 0;
    };

    //==============================================================================
    /** Constructs an empty graph model. */
    AudioGraphModel();

    /** Destructs the graph model. */
    ~AudioGraphModel();

    //==============================================================================
    /** Adds a processor node and returns its stable node identifier. */
    AudioGraphNodeID addNode (std::unique_ptr<AudioProcessor> processor);

    /** Adds a processor node with persistent metadata and returns its stable node identifier. */
    AudioGraphNodeID addNode (std::unique_ptr<AudioProcessor> processor,
                              AudioGraphNodeProperties properties);

    /** Removes a processor node and all connections that mention it. */
    bool removeNode (AudioGraphNodeID nodeID);

    /**
        Replaces a processor node while preserving its node identifier.

        Connections that reference an input or output bus whose type or channel
        count no longer matches the replacement processor are removed from the
        control-thread graph model.
    */
    Result replaceNode (AudioGraphNodeID nodeID,
                        std::unique_ptr<AudioProcessor> processor,
                        AudioGraphNodeProperties properties);

    /** Adds a connection to the model. */
    Result addConnection (const AudioGraphConnection& connection);

    /** Removes a connection from the model. */
    bool removeConnection (const AudioGraphConnection& connection);

    /** Returns a snapshot of the current model connections. */
    std::vector<AudioGraphConnection> getConnections() const;

    /** Removes all graph nodes and connections. */
    void clear();

    //==============================================================================
    /** Returns the processor for a node, or nullptr when the node is not present. */
    AudioProcessor* getNodeProcessor (AudioGraphNodeID nodeID) const noexcept;

    /** Updates the saved canvas position for a node. */
    bool setNodePosition (AudioGraphNodeID nodeID, float positionX, float positionY);

    /** Updates the persistent metadata for a node. */
    bool setNodeProperties (AudioGraphNodeID nodeID, AudioGraphNodeProperties properties);

    /** Returns persistent metadata for a node, or nullopt when the node is not present. */
    std::optional<AudioGraphNodeProperties> getNodeProperties (AudioGraphNodeID nodeID) const;

    /** Returns the identifiers of all nodes currently in the model. */
    std::vector<AudioGraphNodeID> getNodeIDs() const;

    /** Sets the factory used to recreate processor nodes during state loading. */
    void setNodeFactory (NodeFactory factory);

    //==============================================================================
    /** Creates an XML representation of the current graph, including node state. */
    ResultValue<std::unique_ptr<XmlElement>> createXml() const;

    /** Restores the model from XML previously created by createXml(). */
    Result restoreFromXml (const XmlElement& xml);

    //==============================================================================
    /** Returns an immutable snapshot of the current graph model. */
    Snapshot createSnapshot() const;

    /** Restores the model to a previously captured snapshot. */
    void restoreSnapshot (Snapshot snapshot);

    /** Returns the current model revision. */
    uint64_t getRevision() const noexcept;

    /** Returns the current topology revision. */
    uint64_t getTopologyRevision() const noexcept;

private:
    struct ModelNode
    {
        AudioGraphNodeID id;
        std::shared_ptr<AudioProcessor> processor;
        AudioGraphNodeProperties properties;
    };

    struct SavedNodeState
    {
        AudioGraphNodeID id;
        AudioGraphNodeProperties properties;
        MemoryBlock state;
    };

    Result restoreModel (uint64_t savedNextNodeID,
                         std::vector<SavedNodeState> savedNodes,
                         std::vector<AudioGraphConnection> savedConnections);

    ResultValue<std::unique_ptr<AudioProcessor>> createProcessorForSavedNode (const AudioGraphNodeProperties& properties);
    Result validateConnectionLocked (const AudioGraphConnection& connection) const;

    void markTopologyChanged();
    void markMetadataChanged();
    void rebuildDataTree();

    mutable std::mutex mutex;
    mutable std::mutex factoryMutex;
    std::vector<ModelNode> nodes;
    std::vector<AudioGraphConnection> connections;
    DataTree data;
    uint64_t nextNodeID = 0;
    uint64_t revision = 0;
    uint64_t topologyRevision = 0;
    NodeFactory nodeFactory;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioGraphModel)
};

} // namespace yup
