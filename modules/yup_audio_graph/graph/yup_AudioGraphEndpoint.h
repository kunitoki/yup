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

} // namespace yup
