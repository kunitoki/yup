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
enum class GraphSignalType
{
    audio,
    midi
};

struct BusDescriptor
{
    GraphSignalType type = GraphSignalType::audio;
    int channels = 0;
    int offset = 0;
};

int getTotalAudioChannels (Span<const AudioBus> buses)
{
    int result = 0;

    for (const auto& bus : buses)
        if (bus.getType() == AudioBus::Type::Audio)
            result += bus.getNumChannels();

    return result;
}

std::vector<int> getAudioBusOffsets (Span<const AudioBus> buses)
{
    std::vector<int> offsets (static_cast<size_t> (buses.size()), 0);
    int offset = 0;

    for (int i = 0; i < static_cast<int> (buses.size()); ++i)
    {
        offsets[static_cast<size_t> (i)] = offset;

        if (buses[static_cast<size_t> (i)].getType() == AudioBus::Type::Audio)
            offset += buses[static_cast<size_t> (i)].getNumChannels();
    }

    return offsets;
}

GraphSignalType toSignalType (AudioBus::Type busType)
{
    return busType == AudioBus::Type::Audio ? GraphSignalType::audio : GraphSignalType::midi;
}

bool isValidBusIndex (Span<const AudioBus> buses, int busIndex)
{
    return busIndex >= 0 && busIndex < static_cast<int> (buses.size());
}
} // namespace

//==============================================================================
class AudioGraphProcessor::Pimpl
{
public:
    explicit Pimpl (AudioGraphProcessor& ownerIn)
        : owner (ownerIn)
    {
    }

    ~Pimpl()
    {
        resizeWorkers (0);
        delete pendingPlan.exchange (nullptr);
        delete currentPlan;
        deleteRetiredPlans();
    }

    struct ModelNode
    {
        AudioGraphNodeID id;
        std::shared_ptr<AudioProcessor> processor;
        float preparedSampleRate = 0.0f;
        int preparedBlockSize = 0;
        bool prepared = false;
    };

    struct DelayLine
    {
        void initialise (GraphSignalType signalTypeIn, int numChannels, int delaySamplesIn, int maxBlockSize, size_t midiReserveBytes = 4096)
        {
            signalType = signalTypeIn;
            delaySamples = delaySamplesIn;
            channels = numChannels;
            writePosition = 0;

            if (signalType == GraphSignalType::audio && delaySamples > 0)
            {
                audio.setSize (channels, delaySamples + maxBlockSize + 1, false, true, false);
                audio.clear();
            }

            pendingMidi.ensureSize (midiReserveBytes);
            nextPendingMidi.ensureSize (midiReserveBytes);
        }

        void reset()
        {
            writePosition = 0;
            audio.clear();
            pendingMidi.clear();
            nextPendingMidi.clear();
        }

        GraphSignalType signalType = GraphSignalType::audio;
        int delaySamples = 0;
        int channels = 0;
        int writePosition = 0;
        AudioBuffer<float> audio;
        MidiBuffer pendingMidi;
        MidiBuffer nextPendingMidi;
    };

    struct CompiledConnection
    {
        AudioGraphConnection connection;
        GraphSignalType type = GraphSignalType::audio;
        int sourceNodeIndex = -1;
        int destinationNodeIndex = -1;
        int sourceOffset = 0;
        int destinationOffset = 0;
        int channels = 0;
        int delayLineIndex = -1;
        int delaySamples = 0;
    };

    struct NodeRuntime
    {
        AudioGraphNodeID id;
        std::shared_ptr<AudioProcessor> processor;
        int inputChannels = 0;
        int outputChannels = 0;
        int workChannels = 0;
        int inputLatencySamples = 0;
        int outputLatencySamples = 0;
        std::vector<int> incomingConnections;
        AudioBuffer<float> audioBuffer;
        MidiBuffer midiBuffer;
    };

    struct CompiledGraph
    {
        CompiledGraph* nextRetired = nullptr;

        int maxBlockSize = 0;
        int graphLatencySamples = 0;
        int graphInputChannels = 0;
        int graphOutputChannels = 0;
        std::vector<int> graphInputBusOffsets;
        std::vector<int> graphOutputBusOffsets;
        std::vector<NodeRuntime> nodes;
        std::vector<int> topologicalOrder;
        std::vector<std::vector<int>> executionLevels;
        std::vector<CompiledConnection> connections;
        std::vector<int> graphOutputConnections;
        std::vector<DelayLine> delayLines;
        AudioBuffer<float> graphInputAudio;
        MidiBuffer graphInputMidi;
        AudioGraphAllocationStats stats;
    };

    struct ResolvedEndpoint
    {
        GraphSignalType type = GraphSignalType::audio;
        int channels = 0;
        int offset = 0;
        int nodeIndex = -1;
    };

    class WorkerThread final : public Thread
    {
    public:
        WorkerThread (Pimpl& ownerIn, int index);
        ~WorkerThread() override;

        void run() override;

    private:
        Pimpl& owner;
    };

    AudioGraphNodeID addNode (std::unique_ptr<AudioProcessor> processor)
    {
        if (processor == nullptr)
            return AudioGraphNodeID::invalid();

        const std::lock_guard<std::mutex> lock (modelMutex);

        const auto id = AudioGraphNodeID (++nextNodeID);
        modelNodes.push_back ({ id, std::shared_ptr<AudioProcessor> (std::move (processor)) });
        dirty.store (true);
        return id;
    }

    bool removeNode (AudioGraphNodeID nodeID)
    {
        if (! nodeID.isValid())
            return false;

        const std::lock_guard<std::mutex> lock (modelMutex);

        const auto nodeIterator = std::find_if (modelNodes.begin(), modelNodes.end(), [nodeID] (const ModelNode& node)
        {
            return node.id == nodeID;
        });

        if (nodeIterator == modelNodes.end())
            return false;

        modelNodes.erase (nodeIterator);

        modelConnections.erase (std::remove_if (modelConnections.begin(), modelConnections.end(), [nodeID] (const AudioGraphConnection& connection)
        {
            return connection.source.getNodeID() == nodeID || connection.destination.getNodeID() == nodeID;
        }),
                                modelConnections.end());

        dirty.store (true);
        return true;
    }

    Result addConnection (const AudioGraphConnection& connection)
    {
        if (! connection.source.isSource())
            return Result::fail ("Audio graph connection source is not a source endpoint");

        if (! connection.destination.isDestination())
            return Result::fail ("Audio graph connection destination is not a destination endpoint");

        const std::lock_guard<std::mutex> lock (modelMutex);

        if (std::find (modelConnections.begin(), modelConnections.end(), connection) != modelConnections.end())
            return Result::fail ("Audio graph connection already exists");

        modelConnections.push_back (connection);
        dirty.store (true);
        return Result::ok();
    }

    bool removeConnection (const AudioGraphConnection& connection)
    {
        const std::lock_guard<std::mutex> lock (modelMutex);
        const auto oldSize = modelConnections.size();

        modelConnections.erase (std::remove (modelConnections.begin(), modelConnections.end(), connection), modelConnections.end());

        const bool removed = modelConnections.size() != oldSize;
        if (removed)
            dirty.store (true);

        return removed;
    }

    std::vector<AudioGraphConnection> getConnections() const
    {
        const std::lock_guard<std::mutex> lock (modelMutex);
        return modelConnections;
    }

    void clear()
    {
        const std::lock_guard<std::mutex> lock (modelMutex);

        modelConnections.clear();
        modelNodes.clear();
        dirty.store (true);
    }

    Result commitChanges()
    {
        std::vector<ModelNode> nodesSnapshot;
        std::vector<AudioGraphConnection> connectionsSnapshot;

        {
            const std::lock_guard<std::mutex> lock (modelMutex);
            nodesSnapshot = modelNodes;
            connectionsSnapshot = modelConnections;
        }

        for (auto& node : nodesSnapshot)
        {
            if (node.processor == nullptr)
                return Result::fail ("Audio graph contains an empty node");

            if (! node.prepared || node.preparedSampleRate != sampleRate || node.preparedBlockSize != maxBlockSize)
            {
                node.processor->setPlayHead (owner.getPlayHead());
                node.processor->setPlaybackConfiguration (sampleRate, maxBlockSize);
            }
        }

        auto compiled = std::make_unique<CompiledGraph>();
        const auto result = compileGraph (*compiled, nodesSnapshot, connectionsSnapshot);

        if (! result)
            return result;

        {
            const std::lock_guard<std::mutex> lock (modelMutex);

            for (auto& modelNode : modelNodes)
            {
                const auto preparedIterator = std::find_if (nodesSnapshot.begin(), nodesSnapshot.end(), [&modelNode] (const ModelNode& preparedNode)
                {
                    return preparedNode.id == modelNode.id;
                });

                if (preparedIterator != nodesSnapshot.end())
                {
                    modelNode.prepared = true;
                    modelNode.preparedSampleRate = sampleRate;
                    modelNode.preparedBlockSize = maxBlockSize;
                }
            }
        }

        storeStats (compiled->stats);
        latestLatencySamples.store (compiled->graphLatencySamples);
        dirty.store (false);

        delete pendingPlan.exchange (compiled.release());
        deleteRetiredPlans();

        return Result::ok();
    }

    void prepareToPlay (float newSampleRate, int newMaxBlockSize)
    {
        sampleRate = newSampleRate;
        maxBlockSize = std::max (1, newMaxBlockSize);

        const auto result = commitChanges();
        jassert (result.wasOk());
    }

    void releaseResources()
    {
        const std::lock_guard<std::mutex> lock (modelMutex);

        for (auto& node : modelNodes)
        {
            if (node.processor != nullptr && node.prepared)
                node.processor->releaseResources();

            node.prepared = false;
            node.preparedSampleRate = 0.0f;
            node.preparedBlockSize = 0;
        }
    }

    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer)
    {
        ScopedNoDenormals noDenormals;
        swapPendingPlan();

        auto* graph = currentPlan;
        if (graph == nullptr || owner.isSuspended())
        {
            audioBuffer.clear();
            midiBuffer.clear();
            return;
        }

        const int numSamples = std::min (audioBuffer.getNumSamples(), graph->maxBlockSize);
        if (numSamples <= 0)
        {
            audioBuffer.clear();
            midiBuffer.clear();
            return;
        }

        captureGraphInput (*graph, audioBuffer, midiBuffer, numSamples);

        audioBuffer.clear();
        midiBuffer.clear();

        if (desiredWorkerThreads.load() > 0)
        {
            processLevels (*graph, numSamples);
        }
        else
        {
            for (const auto nodeIndex : graph->topologicalOrder)
                processNode (*graph, nodeIndex, numSamples);
        }

        for (const auto connectionIndex : graph->graphOutputConnections)
            routeConnection (*graph, graph->connections[static_cast<size_t> (connectionIndex)], audioBuffer, midiBuffer, numSamples);
    }

    void flush()
    {
        if (auto* graph = currentPlan)
        {
            for (auto& node : graph->nodes)
            {
                node.processor->flush();
                node.midiBuffer.clear();
                node.audioBuffer.clear();
            }

            for (auto& delayLine : graph->delayLines)
                delayLine.reset();
        }
    }

    void setNumWorkerThreads (int numThreads)
    {
        const int newNumThreads = std::max (0, numThreads);
        desiredWorkerThreads.store (newNumThreads);
        resizeWorkers (newNumThreads);
    }

    void setAudioWorkgroup (AudioWorkgroup newWorkgroup)
    {
        const std::lock_guard<std::mutex> lock (workgroupMutex);
        workgroup = std::move (newWorkgroup);
    }

    void joinWorkgroup (WorkgroupToken& token)
    {
        AudioWorkgroup currentWorkgroup;

        {
            const std::lock_guard<std::mutex> lock (workgroupMutex);
            currentWorkgroup = workgroup;
        }

        if (currentWorkgroup)
            currentWorkgroup.join (token);
        else
            token.reset();
    }

    AudioGraphAllocationStats getAllocationStats() const noexcept
    {
        AudioGraphAllocationStats stats;
        stats.scratchAudioBuffers = latestScratchAudioBuffers.load();
        stats.midiBuffers = latestMidiBuffers.load();
        stats.delayLines = latestDelayLines.load();
        stats.totalCompensationSamples = latestTotalCompensationSamples.load();
        stats.maxPreallocatedChannels = latestMaxPreallocatedChannels.load();
        stats.maxPreallocatedBlockSize = latestMaxPreallocatedBlockSize.load();
        return stats;
    }

    AudioProcessor* getNodeProcessor (AudioGraphNodeID nodeID) const noexcept
    {
        const std::lock_guard<std::mutex> lock (modelMutex);

        const auto iterator = std::find_if (modelNodes.begin(), modelNodes.end(), [nodeID] (const ModelNode& node)
        {
            return node.id == nodeID;
        });

        return iterator != modelNodes.end() ? iterator->processor.get() : nullptr;
    }

    Result compileGraph (CompiledGraph& graph,
                         const std::vector<ModelNode>& nodesSnapshot,
                         const std::vector<AudioGraphConnection>& connectionsSnapshot)
    {
        graph.maxBlockSize = maxBlockSize;
        graph.graphInputChannels = getTotalAudioChannels (owner.getBusLayout().getInputBuses());
        graph.graphOutputChannels = getTotalAudioChannels (owner.getBusLayout().getOutputBuses());
        graph.graphInputBusOffsets = getAudioBusOffsets (owner.getBusLayout().getInputBuses());
        graph.graphOutputBusOffsets = getAudioBusOffsets (owner.getBusLayout().getOutputBuses());
        graph.graphInputAudio.setSize (graph.graphInputChannels, maxBlockSize, false, true, false);
        graph.graphInputAudio.clear();

        std::unordered_map<uint64_t, int> nodeIndexByID;

        for (int i = 0; i < static_cast<int> (nodesSnapshot.size()); ++i)
        {
            const auto& modelNode = nodesSnapshot[static_cast<size_t> (i)];

            if (modelNode.processor == nullptr)
                return Result::fail ("Audio graph contains an empty node");

            if (! nodeIndexByID.emplace (modelNode.id.getRawID(), i).second)
                return Result::fail ("Audio graph contains duplicate node IDs");

            NodeRuntime runtime;
            runtime.id = modelNode.id;
            runtime.processor = modelNode.processor;
            runtime.inputChannels = getTotalAudioChannels (modelNode.processor->getBusLayout().getInputBuses());
            runtime.outputChannels = getTotalAudioChannels (modelNode.processor->getBusLayout().getOutputBuses());
            runtime.workChannels = std::max (runtime.inputChannels, runtime.outputChannels);
            runtime.audioBuffer.setSize (runtime.workChannels, maxBlockSize, false, true, false);
            runtime.audioBuffer.clear();
            graph.nodes.push_back (std::move (runtime));
        }

        std::vector<std::vector<int>> adjacency (nodesSnapshot.size());
        std::vector<int> indegrees (nodesSnapshot.size(), 0);

        for (const auto& modelConnection : connectionsSnapshot)
        {
            if (std::count (connectionsSnapshot.begin(), connectionsSnapshot.end(), modelConnection) > 1)
                return Result::fail ("Audio graph contains duplicate connections");

            CompiledConnection connection;
            connection.connection = modelConnection;

            const auto source = resolveEndpoint (modelConnection.source, true, nodesSnapshot, nodeIndexByID);
            if (! source)
                return Result::fail (source.getErrorMessage());

            const auto destination = resolveEndpoint (modelConnection.destination, false, nodesSnapshot, nodeIndexByID);
            if (! destination)
                return Result::fail (destination.getErrorMessage());

            const auto& sourceEndpoint = sourceEndpointScratch;
            const auto& destinationEndpoint = destinationEndpointScratch;

            if (sourceEndpoint.type != destinationEndpoint.type)
                return Result::fail ("Audio graph connection mixes audio and MIDI endpoints");

            if (sourceEndpoint.type == GraphSignalType::audio && sourceEndpoint.channels != destinationEndpoint.channels)
                return Result::fail ("Audio graph connection has incompatible channel counts");

            connection.type = sourceEndpoint.type;
            connection.sourceNodeIndex = sourceEndpoint.nodeIndex;
            connection.destinationNodeIndex = destinationEndpoint.nodeIndex;
            connection.sourceOffset = sourceEndpoint.offset;
            connection.destinationOffset = destinationEndpoint.offset;
            connection.channels = sourceEndpoint.channels;

            const int connectionIndex = static_cast<int> (graph.connections.size());
            graph.connections.push_back (connection);

            if (connection.destinationNodeIndex >= 0)
            {
                graph.nodes[static_cast<size_t> (connection.destinationNodeIndex)].incomingConnections.push_back (connectionIndex);

                if (connection.sourceNodeIndex >= 0)
                {
                    adjacency[static_cast<size_t> (connection.sourceNodeIndex)].push_back (connection.destinationNodeIndex);
                    ++indegrees[static_cast<size_t> (connection.destinationNodeIndex)];
                }
            }
            else
            {
                graph.graphOutputConnections.push_back (connectionIndex);
            }
        }

        int numMidiConnections = 0;
        for (const auto& conn : graph.connections)
            if (conn.type == GraphSignalType::midi)
                ++numMidiConnections;

        const size_t midiReserveBytes = static_cast<size_t> (std::max (1, numMidiConnections + 1)) * 4096;

        graph.graphInputMidi.ensureSize (midiReserveBytes);

        for (auto& node : graph.nodes)
            node.midiBuffer.ensureSize (midiReserveBytes);

        auto topologicalResult = buildTopologicalOrder (graph, adjacency, indegrees);
        if (! topologicalResult)
            return topologicalResult;

        buildExecutionLevels (graph);
        computeLatenciesAndDelays (graph, midiReserveBytes);
        fillStats (graph);

        return Result::ok();
    }

    Result resolveEndpoint (const AudioGraphEndpoint& endpoint,
                            bool source,
                            const std::vector<ModelNode>& nodesSnapshot,
                            const std::unordered_map<uint64_t, int>& nodeIndexByID)
    {
        auto& result = source ? sourceEndpointScratch : destinationEndpointScratch;
        result = {};

        if (source && ! endpoint.isSource())
            return Result::fail ("Audio graph endpoint is not a source");

        if (! source && ! endpoint.isDestination())
            return Result::fail ("Audio graph endpoint is not a destination");

        Span<const AudioBus> buses;
        std::vector<int> offsets;

        switch (endpoint.getKind())
        {
            case AudioGraphEndpoint::Kind::graphInput:
                buses = owner.getBusLayout().getInputBuses();
                offsets = getAudioBusOffsets (buses);
                break;

            case AudioGraphEndpoint::Kind::graphOutput:
                buses = owner.getBusLayout().getOutputBuses();
                offsets = getAudioBusOffsets (buses);
                break;

            case AudioGraphEndpoint::Kind::nodeInput:
            case AudioGraphEndpoint::Kind::nodeOutput:
            {
                const auto iterator = nodeIndexByID.find (endpoint.getNodeID().getRawID());
                if (iterator == nodeIndexByID.end())
                    return Result::fail ("Audio graph connection references a missing node");

                result.nodeIndex = iterator->second;
                const auto& processor = *nodesSnapshot[static_cast<size_t> (result.nodeIndex)].processor;

                buses = endpoint.getKind() == AudioGraphEndpoint::Kind::nodeInput
                          ? processor.getBusLayout().getInputBuses()
                          : processor.getBusLayout().getOutputBuses();
                offsets = getAudioBusOffsets (buses);
                break;
            }
        }

        if (! isValidBusIndex (buses, endpoint.getBusIndex()))
            return Result::fail ("Audio graph connection references an invalid bus index");

        const auto& bus = buses[static_cast<size_t> (endpoint.getBusIndex())];
        result.type = toSignalType (bus.getType());
        result.channels = bus.getType() == AudioBus::Type::Audio ? bus.getNumChannels() : 0;
        result.offset = bus.getType() == AudioBus::Type::Audio ? offsets[static_cast<size_t> (endpoint.getBusIndex())] : 0;
        return Result::ok();
    }

    Result buildTopologicalOrder (CompiledGraph& graph,
                                  const std::vector<std::vector<int>>& adjacency,
                                  std::vector<int> indegrees)
    {
        std::vector<int> ready;

        for (int i = 0; i < static_cast<int> (indegrees.size()); ++i)
            if (indegrees[static_cast<size_t> (i)] == 0)
                ready.push_back (i);

        while (! ready.empty())
        {
            const int nodeIndex = ready.front();
            ready.erase (ready.begin());
            graph.topologicalOrder.push_back (nodeIndex);

            for (const auto destination : adjacency[static_cast<size_t> (nodeIndex)])
            {
                auto& indegree = indegrees[static_cast<size_t> (destination)];
                --indegree;

                if (indegree == 0)
                    ready.push_back (destination);
            }
        }

        if (graph.topologicalOrder.size() != graph.nodes.size())
            return Result::fail ("Audio graph contains a cycle");

        return Result::ok();
    }

    void buildExecutionLevels (CompiledGraph& graph)
    {
        std::vector<int> nodeLevels (graph.nodes.size(), 0);

        for (const auto nodeIndex : graph.topologicalOrder)
        {
            int level = 0;
            const auto& node = graph.nodes[static_cast<size_t> (nodeIndex)];

            for (const auto connectionIndex : node.incomingConnections)
            {
                const auto& connection = graph.connections[static_cast<size_t> (connectionIndex)];
                if (connection.sourceNodeIndex >= 0)
                    level = std::max (level, nodeLevels[static_cast<size_t> (connection.sourceNodeIndex)] + 1);
            }

            nodeLevels[static_cast<size_t> (nodeIndex)] = level;

            if (level >= static_cast<int> (graph.executionLevels.size()))
                graph.executionLevels.resize (static_cast<size_t> (level + 1));

            graph.executionLevels[static_cast<size_t> (level)].push_back (nodeIndex);
        }
    }

    int getSourceLatency (const CompiledGraph& graph, const CompiledConnection& connection) const
    {
        if (connection.sourceNodeIndex < 0)
            return 0;

        return graph.nodes[static_cast<size_t> (connection.sourceNodeIndex)].outputLatencySamples;
    }

    void computeLatenciesAndDelays (CompiledGraph& graph, size_t midiReserveBytes)
    {
        for (const auto nodeIndex : graph.topologicalOrder)
        {
            auto& node = graph.nodes[static_cast<size_t> (nodeIndex)];

            for (const auto connectionIndex : node.incomingConnections)
                node.inputLatencySamples = std::max (node.inputLatencySamples,
                                                     getSourceLatency (graph, graph.connections[static_cast<size_t> (connectionIndex)]));

            node.outputLatencySamples = node.inputLatencySamples + std::max (0, node.processor->getLatencySamples());
        }

        for (const auto connectionIndex : graph.graphOutputConnections)
            graph.graphLatencySamples = std::max (graph.graphLatencySamples,
                                                  getSourceLatency (graph, graph.connections[static_cast<size_t> (connectionIndex)]));

        for (auto& connection : graph.connections)
        {
            const int destinationLatency = connection.destinationNodeIndex >= 0
                                             ? graph.nodes[static_cast<size_t> (connection.destinationNodeIndex)].inputLatencySamples
                                             : graph.graphLatencySamples;

            connection.delaySamples = std::max (0, destinationLatency - getSourceLatency (graph, connection));

            if (connection.delaySamples > 0)
            {
                DelayLine delayLine;
                delayLine.initialise (connection.type, connection.channels, connection.delaySamples, graph.maxBlockSize, midiReserveBytes);
                connection.delayLineIndex = static_cast<int> (graph.delayLines.size());
                graph.delayLines.push_back (std::move (delayLine));
            }
        }
    }

    void fillStats (CompiledGraph& graph)
    {
        graph.stats.scratchAudioBuffers = static_cast<int> (graph.nodes.size());
        graph.stats.midiBuffers = static_cast<int> (graph.nodes.size());
        graph.stats.delayLines = static_cast<int> (graph.delayLines.size());
        graph.stats.totalCompensationSamples = graph.graphLatencySamples;
        graph.stats.maxPreallocatedBlockSize = graph.maxBlockSize;
        graph.stats.maxPreallocatedChannels = std::max (graph.graphInputChannels, graph.graphOutputChannels);

        for (const auto& node : graph.nodes)
            graph.stats.maxPreallocatedChannels = std::max (graph.stats.maxPreallocatedChannels, node.workChannels);
    }

    void storeStats (const AudioGraphAllocationStats& stats) noexcept
    {
        latestScratchAudioBuffers.store (stats.scratchAudioBuffers);
        latestMidiBuffers.store (stats.midiBuffers);
        latestDelayLines.store (stats.delayLines);
        latestTotalCompensationSamples.store (stats.totalCompensationSamples);
        latestMaxPreallocatedChannels.store (stats.maxPreallocatedChannels);
        latestMaxPreallocatedBlockSize.store (stats.maxPreallocatedBlockSize);
    }

    void captureGraphInput (CompiledGraph& graph,
                            AudioBuffer<float>& audioBuffer,
                            MidiBuffer& midiBuffer,
                            int numSamples)
    {
        graph.graphInputAudio.clear (0, numSamples);

        const int numChannels = std::min (graph.graphInputChannels, audioBuffer.getNumChannels());
        for (int channel = 0; channel < numChannels; ++channel)
            graph.graphInputAudio.copyFrom (channel, 0, audioBuffer, channel, 0, numSamples);

        graph.graphInputMidi.clear();
        graph.graphInputMidi.addEvents (midiBuffer, 0, numSamples, 0);
    }

    void processNode (CompiledGraph& graph, int nodeIndex, int numSamples)
    {
        auto& node = graph.nodes[static_cast<size_t> (nodeIndex)];

        node.audioBuffer.setSize (node.workChannels, numSamples, false, false, true);
        node.audioBuffer.clear (0, numSamples);
        node.midiBuffer.clear();

        for (const auto connectionIndex : node.incomingConnections)
            routeConnection (graph, graph.connections[static_cast<size_t> (connectionIndex)], node.audioBuffer, node.midiBuffer, numSamples);

        node.processor->processBlock (node.audioBuffer, node.midiBuffer);
    }

    void processLevels (CompiledGraph& graph, int numSamples)
    {
        for (auto& level : graph.executionLevels)
        {
            if (level.empty())
                continue;

            const auto generation = workGeneration.load (std::memory_order_relaxed) + 1;

            activeGraph.store (&graph, std::memory_order_relaxed);
            activeLevel.store (&level, std::memory_order_relaxed);
            activeNumSamples.store (numSamples, std::memory_order_relaxed);
            nextJobIndex.store (0, std::memory_order_relaxed);
            remainingJobs.store (static_cast<int> (level.size()), std::memory_order_release);
            activeGeneration.store (generation, std::memory_order_release);
            workGeneration.store (generation, std::memory_order_release);
            workerReadyEvent.reset();
            workerReadyEvent.signal();

            drainActiveJobs (generation);

            while (remainingJobs.load (std::memory_order_acquire) > 0)
                ;
        }

        activeGraph.store (nullptr, std::memory_order_relaxed);
        activeLevel.store (nullptr, std::memory_order_relaxed);
        activeNumSamples.store (0, std::memory_order_relaxed);
        activeGeneration.store (0, std::memory_order_release);
    }

    void drainActiveJobs (int generation)
    {
        if (activeGeneration.load (std::memory_order_acquire) != generation)
            return;

        auto* graph = activeGraph.load (std::memory_order_relaxed);
        auto* level = activeLevel.load (std::memory_order_relaxed);

        if (graph == nullptr || level == nullptr)
            return;

        const int numSamples = activeNumSamples.load (std::memory_order_relaxed);

        for (;;)
        {
            const int jobIndex = nextJobIndex.fetch_add (1);

            if (jobIndex >= static_cast<int> (level->size()))
                break;

            processNode (*graph, (*level)[static_cast<size_t> (jobIndex)], numSamples);

            remainingJobs.fetch_sub (1, std::memory_order_acq_rel);
        }
    }

    void routeConnection (CompiledGraph& graph,
                          CompiledConnection& connection,
                          AudioBuffer<float>& destinationAudio,
                          MidiBuffer& destinationMidi,
                          int numSamples)
    {
        if (connection.type == GraphSignalType::audio)
        {
            const auto& sourceAudio = getSourceAudioBuffer (graph, connection);

            if (connection.delayLineIndex >= 0)
                routeDelayedAudio (graph.delayLines[static_cast<size_t> (connection.delayLineIndex)], sourceAudio, connection, destinationAudio, numSamples);
            else
                routeAudio (sourceAudio, connection, destinationAudio, numSamples);

            return;
        }

        const auto& sourceMidi = getSourceMidiBuffer (graph, connection);

        if (connection.delayLineIndex >= 0)
            routeDelayedMidi (graph.delayLines[static_cast<size_t> (connection.delayLineIndex)], sourceMidi, destinationMidi, numSamples);
        else
            destinationMidi.addEvents (sourceMidi, 0, numSamples, 0);
    }

    const AudioBuffer<float>& getSourceAudioBuffer (CompiledGraph& graph, const CompiledConnection& connection) const
    {
        if (connection.sourceNodeIndex < 0)
            return graph.graphInputAudio;

        return graph.nodes[static_cast<size_t> (connection.sourceNodeIndex)].audioBuffer;
    }

    const MidiBuffer& getSourceMidiBuffer (CompiledGraph& graph, const CompiledConnection& connection) const
    {
        if (connection.sourceNodeIndex < 0)
            return graph.graphInputMidi;

        return graph.nodes[static_cast<size_t> (connection.sourceNodeIndex)].midiBuffer;
    }

    void routeAudio (const AudioBuffer<float>& sourceAudio,
                     const CompiledConnection& connection,
                     AudioBuffer<float>& destinationAudio,
                     int numSamples)
    {
        const int channels = std::min ({ connection.channels,
                                         std::max (0, sourceAudio.getNumChannels() - connection.sourceOffset),
                                         std::max (0, destinationAudio.getNumChannels() - connection.destinationOffset) });

        for (int channel = 0; channel < channels; ++channel)
            destinationAudio.addFrom (connection.destinationOffset + channel,
                                      0,
                                      sourceAudio,
                                      connection.sourceOffset + channel,
                                      0,
                                      numSamples);
    }

    void routeDelayedAudio (DelayLine& delayLine,
                            const AudioBuffer<float>& sourceAudio,
                            const CompiledConnection& connection,
                            AudioBuffer<float>& destinationAudio,
                            int numSamples)
    {
        if (delayLine.delaySamples <= 0)
        {
            routeAudio (sourceAudio, connection, destinationAudio, numSamples);
            return;
        }

        const int channels = std::min ({ connection.channels,
                                         std::max (0, sourceAudio.getNumChannels() - connection.sourceOffset),
                                         std::max (0, destinationAudio.getNumChannels() - connection.destinationOffset),
                                         delayLine.audio.getNumChannels() });
        const int ringSize = delayLine.audio.getNumSamples();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const int readPosition = (delayLine.writePosition + ringSize - delayLine.delaySamples) % ringSize;

            for (int channel = 0; channel < channels; ++channel)
            {
                const float delayedSample = delayLine.audio.getReadPointer (channel)[readPosition];
                destinationAudio.addFrom (connection.destinationOffset + channel, sample, &delayedSample, 1);

                delayLine.audio.getWritePointer (channel)[delayLine.writePosition] =
                    sourceAudio.getReadPointer (connection.sourceOffset + channel)[sample];
            }

            delayLine.writePosition = (delayLine.writePosition + 1) % ringSize;
        }
    }

    void routeDelayedMidi (DelayLine& delayLine,
                           const MidiBuffer& sourceMidi,
                           MidiBuffer& destinationMidi,
                           int numSamples)
    {
        delayLine.nextPendingMidi.clear();

        for (const auto metadata : delayLine.pendingMidi)
        {
            if (metadata.samplePosition < numSamples)
                destinationMidi.addEvent (metadata.data, metadata.numBytes, metadata.samplePosition);
            else
                delayLine.nextPendingMidi.addEvent (metadata.data, metadata.numBytes, metadata.samplePosition - numSamples);
        }

        for (const auto metadata : sourceMidi)
        {
            if (metadata.samplePosition < 0 || metadata.samplePosition >= numSamples)
                continue;

            const int delayedPosition = metadata.samplePosition + delayLine.delaySamples;

            if (delayedPosition < numSamples)
                destinationMidi.addEvent (metadata.data, metadata.numBytes, delayedPosition);
            else
                delayLine.nextPendingMidi.addEvent (metadata.data, metadata.numBytes, delayedPosition - numSamples);
        }

        delayLine.pendingMidi.swapWith (delayLine.nextPendingMidi);
    }

    void swapPendingPlan()
    {
        if (auto* nextPlan = pendingPlan.exchange (nullptr))
        {
            retirePlan (currentPlan);
            currentPlan = nextPlan;
        }
    }

    void retirePlan (CompiledGraph* plan) noexcept
    {
        if (plan == nullptr)
            return;

        auto* head = retiredPlans.load();

        do
        {
            plan->nextRetired = head;
        } while (! retiredPlans.compare_exchange_weak (head, plan));
    }

    void deleteRetiredPlans()
    {
        auto* plan = retiredPlans.exchange (nullptr);

        while (plan != nullptr)
        {
            auto* next = plan->nextRetired;
            delete plan;
            plan = next;
        }
    }

    void resizeWorkers (int newNumThreads)
    {
        while (static_cast<int> (workers.size()) > newNumThreads)
        {
            workers.back()->signalThreadShouldExit();
            workerReadyEvent.signal();
            workers.pop_back();
        }

        if (! workers.empty())
            workerReadyEvent.reset();

        while (static_cast<int> (workers.size()) < newNumThreads)
        {
            auto worker = std::make_unique<WorkerThread> (*this, static_cast<int> (workers.size()));
            worker->startThread (Thread::Priority::highest);
            workers.push_back (std::move (worker));
        }
    }

    AudioGraphProcessor& owner;
    mutable std::mutex modelMutex;
    mutable std::mutex workgroupMutex;
    std::vector<ModelNode> modelNodes;
    std::vector<AudioGraphConnection> modelConnections;
    uint64_t nextNodeID = 0;
    std::atomic<bool> dirty { true };
    std::atomic<int> desiredWorkerThreads { 0 };
    std::atomic<int> latestLatencySamples { 0 };
    std::atomic<int> latestScratchAudioBuffers { 0 };
    std::atomic<int> latestMidiBuffers { 0 };
    std::atomic<int> latestDelayLines { 0 };
    std::atomic<int> latestTotalCompensationSamples { 0 };
    std::atomic<int> latestMaxPreallocatedChannels { 0 };
    std::atomic<int> latestMaxPreallocatedBlockSize { 0 };
    AudioWorkgroup workgroup;
    float sampleRate = 44100.0f;
    int maxBlockSize = 1024;
    std::atomic<CompiledGraph*> pendingPlan { nullptr };
    std::atomic<CompiledGraph*> retiredPlans { nullptr };
    CompiledGraph* currentPlan = nullptr;
    ResolvedEndpoint sourceEndpointScratch;
    ResolvedEndpoint destinationEndpointScratch;
    std::vector<std::unique_ptr<WorkerThread>> workers;
    WaitableEvent workerReadyEvent { true };
    std::atomic<int> workGeneration { 0 };
    std::atomic<int> nextJobIndex { 0 };
    std::atomic<int> remainingJobs { 0 };
    std::atomic<CompiledGraph*> activeGraph { nullptr };
    std::atomic<std::vector<int>*> activeLevel { nullptr };
    std::atomic<int> activeNumSamples { 0 };
    std::atomic<int> activeGeneration { 0 };
};

//==============================================================================
AudioGraphProcessor::Pimpl::WorkerThread::WorkerThread (Pimpl& ownerIn, int index)
    : Thread ("Audio Graph Worker " + String (index + 1))
    , owner (ownerIn)
{
}

AudioGraphProcessor::Pimpl::WorkerThread::~WorkerThread()
{
    stopThread (2000);
}

void AudioGraphProcessor::Pimpl::WorkerThread::run()
{
    int lastGeneration = 0;
    WorkgroupToken workgroupToken;

    while (! threadShouldExit())
    {
        owner.workerReadyEvent.wait (-1.0);

        if (threadShouldExit())
            break;

        const int generation = owner.workGeneration.load (std::memory_order_acquire);
        if (generation == lastGeneration)
            continue;

        lastGeneration = generation;
        ScopedNoDenormals noDenormals;
        owner.joinWorkgroup (workgroupToken);
        owner.drainActiveJobs (generation);
    }
}

//==============================================================================
AudioBusLayout AudioGraphProcessor::createDefaultBusLayout()
{
    return AudioBusLayout ({ AudioBus ("Input", AudioBus::Type::Audio, AudioBus::Direction::Input, 2) },
                           { AudioBus ("Output", AudioBus::Type::Audio, AudioBus::Direction::Output, 2) });
}

AudioGraphProcessor::AudioGraphProcessor (AudioBusLayout busLayout)
    : AudioProcessor ("Audio Graph", std::move (busLayout))
    , pimpl (std::make_unique<Pimpl> (*this))
{
}

AudioGraphProcessor::~AudioGraphProcessor() = default;

AudioGraphNodeID AudioGraphProcessor::addNode (std::unique_ptr<AudioProcessor> processor)
{
    return pimpl->addNode (std::move (processor));
}

bool AudioGraphProcessor::removeNode (AudioGraphNodeID nodeID)
{
    return pimpl->removeNode (nodeID);
}

Result AudioGraphProcessor::addConnection (const AudioGraphConnection& connection)
{
    return pimpl->addConnection (connection);
}

bool AudioGraphProcessor::removeConnection (const AudioGraphConnection& connection)
{
    return pimpl->removeConnection (connection);
}

std::vector<AudioGraphConnection> AudioGraphProcessor::getConnections() const
{
    return pimpl->getConnections();
}

void AudioGraphProcessor::clear()
{
    pimpl->clear();
}

Result AudioGraphProcessor::commitChanges()
{
    return pimpl->commitChanges();
}

void AudioGraphProcessor::setNumWorkerThreads (int numThreads)
{
    pimpl->setNumWorkerThreads (numThreads);
}

int AudioGraphProcessor::getNumWorkerThreads() const noexcept
{
    return pimpl->desiredWorkerThreads.load();
}

void AudioGraphProcessor::setAudioWorkgroup (AudioWorkgroup workgroup)
{
    pimpl->setAudioWorkgroup (std::move (workgroup));
}

AudioGraphAllocationStats AudioGraphProcessor::getAllocationStats() const noexcept
{
    return pimpl->getAllocationStats();
}

bool AudioGraphProcessor::hasUncommittedChanges() const noexcept
{
    return pimpl->dirty.load();
}

AudioProcessor* AudioGraphProcessor::getNodeProcessor (AudioGraphNodeID nodeID) const noexcept
{
    return pimpl->getNodeProcessor (nodeID);
}

void AudioGraphProcessor::prepareToPlay (float sampleRate, int maxBlockSize)
{
    pimpl->prepareToPlay (sampleRate, maxBlockSize);
}

void AudioGraphProcessor::releaseResources()
{
    pimpl->releaseResources();
}

void AudioGraphProcessor::processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer)
{
    pimpl->processBlock (audioBuffer, midiBuffer);
}

void AudioGraphProcessor::flush()
{
    pimpl->flush();
}

int AudioGraphProcessor::getLatencySamples()
{
    return pimpl->latestLatencySamples.load();
}

} // namespace yup
