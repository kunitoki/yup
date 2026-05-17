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
    Opaque identifier for a processor node owned by an AudioGraphProcessor.

    Node identifiers remain stable until the node is removed. The invalid identifier
    is used by graph input and graph output endpoints, and by failed addNode calls.
*/
class AudioGraphNodeID
{
public:
    /** Creates an invalid node identifier. */
    constexpr AudioGraphNodeID() noexcept = default;

    /** Returns an invalid node identifier. */
    static constexpr AudioGraphNodeID invalid() noexcept { return {}; }

    /** Returns true when this identifier names a graph node. */
    constexpr bool isValid() const noexcept { return value != 0; }

    /** Returns the raw integer identifier. */
    constexpr uint64_t getRawID() const noexcept { return value; }

    /** Creates an identifier from a raw value. Prefer IDs returned by addNode(). */
    explicit constexpr AudioGraphNodeID (uint64_t rawValue) noexcept
        : value (rawValue)
    {
    }

    constexpr bool operator== (AudioGraphNodeID other) const noexcept { return value == other.value; }

    constexpr bool operator!= (AudioGraphNodeID other) const noexcept { return value != other.value; }

    constexpr bool operator< (AudioGraphNodeID other) const noexcept { return value < other.value; }

private:
    uint64_t value = 0;
};

//==============================================================================
/**
    Describes one routable audio or MIDI endpoint in an AudioGraphProcessor.

    Source endpoints are graphInput() and nodeOutput(). Destination endpoints are
    graphOutput() and nodeInput(). The bus index refers to the AudioBusLayout of the
    graph or the addressed processor node.
*/
class AudioGraphEndpoint
{
public:
    /** The endpoint owner and direction. */
    enum class Kind
    {
        graphInput,
        graphOutput,
        nodeInput,
        nodeOutput
    };

    /** Creates an invalid graph input endpoint. */
    AudioGraphEndpoint() = default;

    /** Creates a source endpoint for one graph input bus. */
    static AudioGraphEndpoint graphInput (int busIndex) noexcept
    {
        return AudioGraphEndpoint (Kind::graphInput, AudioGraphNodeID::invalid(), busIndex);
    }

    /** Creates a destination endpoint for one graph output bus. */
    static AudioGraphEndpoint graphOutput (int busIndex) noexcept
    {
        return AudioGraphEndpoint (Kind::graphOutput, AudioGraphNodeID::invalid(), busIndex);
    }

    /** Creates a destination endpoint for one node input bus. */
    static AudioGraphEndpoint nodeInput (AudioGraphNodeID nodeID, int busIndex) noexcept
    {
        return AudioGraphEndpoint (Kind::nodeInput, nodeID, busIndex);
    }

    /** Creates a source endpoint for one node output bus. */
    static AudioGraphEndpoint nodeOutput (AudioGraphNodeID nodeID, int busIndex) noexcept
    {
        return AudioGraphEndpoint (Kind::nodeOutput, nodeID, busIndex);
    }

    /** Returns the endpoint kind. */
    Kind getKind() const noexcept { return kind; }

    /** Returns the addressed node, or an invalid ID for graph endpoints. */
    AudioGraphNodeID getNodeID() const noexcept { return nodeID; }

    /** Returns the bus index on the graph or processor layout. */
    int getBusIndex() const noexcept { return busIndex; }

    /** Returns true when this endpoint can appear as a connection source. */
    bool isSource() const noexcept { return kind == Kind::graphInput || kind == Kind::nodeOutput; }

    /** Returns true when this endpoint can appear as a connection destination. */
    bool isDestination() const noexcept { return kind == Kind::graphOutput || kind == Kind::nodeInput; }

    bool operator== (const AudioGraphEndpoint& other) const noexcept
    {
        return kind == other.kind && nodeID == other.nodeID && busIndex == other.busIndex;
    }

    bool operator!= (const AudioGraphEndpoint& other) const noexcept { return ! (*this == other); }

private:
    AudioGraphEndpoint (Kind endpointKind, AudioGraphNodeID endpointNodeID, int endpointBusIndex) noexcept
        : kind (endpointKind)
        , nodeID (endpointNodeID)
        , busIndex (endpointBusIndex)
    {
    }

    Kind kind = Kind::graphInput;
    AudioGraphNodeID nodeID;
    int busIndex = -1;
};

//==============================================================================
/**
    A directed connection between two graph endpoints.
*/
class AudioGraphConnection
{
public:
    AudioGraphConnection() = default;

    /** Creates a connection from sourceEndpoint to destinationEndpoint. */
    AudioGraphConnection (AudioGraphEndpoint sourceEndpoint, AudioGraphEndpoint destinationEndpoint) noexcept
        : source (sourceEndpoint)
        , destination (destinationEndpoint)
    {
    }

    bool operator== (const AudioGraphConnection& other) const noexcept
    {
        return source == other.source && destination == other.destination;
    }

    bool operator!= (const AudioGraphConnection& other) const noexcept { return ! (*this == other); }

    /** Source endpoint. */
    AudioGraphEndpoint source;

    /** Destination endpoint. */
    AudioGraphEndpoint destination;
};

//==============================================================================
/**
    Lightweight diagnostics describing the most recently compiled graph plan.
*/
struct AudioGraphAllocationStats
{
    /** Number of preallocated node scratch audio buffers. */
    int scratchAudioBuffers = 0;

    /** Number of preallocated node MIDI buffers. */
    int midiBuffers = 0;

    /** Number of per-connection delay lines. */
    int delayLines = 0;

    /** Maximum latency compensation inserted on any graph output path. */
    int totalCompensationSamples = 0;

    /** Maximum preallocated audio channel count used by any node or graph endpoint. */
    int maxPreallocatedChannels = 0;

    /** Maximum preallocated block size for the compiled plan. */
    int maxPreallocatedBlockSize = 0;
};

//==============================================================================
/**
    An AudioProcessor that owns and executes an acyclic graph of AudioProcessor nodes.

    Edits are made to a control-thread graph model. commitChanges() validates the
    model, prepares newly compiled nodes for the current playback configuration, and
    publishes an immutable processing plan. processBlock() only swaps pending plans at
    block boundaries and keeps retired plans alive until a later control-thread commit
    or destruction.
*/
class YUP_API AudioGraphProcessor final : public AudioProcessor
{
public:
    /** Creates the default stereo graph bus layout. */
    static AudioBusLayout createDefaultBusLayout();

    /** Constructs an audio graph with the supplied graph input/output bus layout. */
    explicit AudioGraphProcessor (AudioBusLayout busLayout = createDefaultBusLayout());

    /** Destructs the graph and releases all owned processors. */
    ~AudioGraphProcessor() override;

    /** Adds a processor node and returns its stable node identifier. */
    AudioGraphNodeID addNode (std::unique_ptr<AudioProcessor> processor);

    /** Removes a processor node and all connections that mention it. */
    bool removeNode (AudioGraphNodeID nodeID);

    /** Adds a connection to the control-thread graph model. */
    Result addConnection (const AudioGraphConnection& connection);

    /** Removes a connection from the control-thread graph model. */
    bool removeConnection (const AudioGraphConnection& connection);

    /** Returns a snapshot of the current control-thread graph model connections. */
    std::vector<AudioGraphConnection> getConnections() const;

    /** Removes all graph nodes and connections. */
    void clear();

    /** Validates, compiles, and publishes the current graph model. */
    Result commitChanges();

    /** Sets the desired worker thread count for future processing blocks. */
    void setNumWorkerThreads (int numThreads);

    /** Returns the desired worker thread count. */
    int getNumWorkerThreads() const noexcept;

    /** Supplies the audio workgroup that worker threads should join when supported. */
    void setAudioWorkgroup (AudioWorkgroup workgroup);

    /** Returns diagnostics for the last successfully compiled graph. */
    AudioGraphAllocationStats getAllocationStats() const noexcept;

    /** Returns true when graph edits have not yet been committed. */
    bool hasUncommittedChanges() const noexcept;

    /** Returns the processor for a node, or nullptr when the node is not present. */
    AudioProcessor* getNodeProcessor (AudioGraphNodeID nodeID) const noexcept;

    void prepareToPlay (float sampleRate, int maxBlockSize) override;
    void releaseResources() override;
    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer) override;
    void flush() override;

    int getLatencySamples() override;

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock&) override { return Result::ok(); }

    Result saveStateIntoMemory (MemoryBlock&) override { return Result::ok(); }

    bool hasEditor() const override { return false; }

    AudioProcessorEditor* createEditor() override { return nullptr; }

private:
    class Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioGraphProcessor)
};

} // namespace yup
