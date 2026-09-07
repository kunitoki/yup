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

void YdspSemanticAnalyzer::computeLatencyAndCompensate (YdspAnalyzedGraph& graph, const YdspLocation& location)
{
    const auto nodeCount = graph.nodes.size();

    std::vector<int> artifactIn (nodeCount, 0);
    std::vector<int> artifactOut (nodeCount, 0);

    for (const int index : graph.topoOrder)
    {
        int latest = 0;

        for (const auto& edge : graph.edges)
            if (edge.dstNode == index && edge.srcNode >= 0)
                latest = std::max (latest, artifactOut[static_cast<size_t> (edge.srcNode)]);

        artifactIn[static_cast<size_t> (index)] = latest;
        artifactOut[static_cast<size_t> (index)] = latest + graph.nodes[static_cast<size_t> (index)].latencySamples;
    }

    int graphLatency = 0;

    for (const auto& edge : graph.edges)
        if (edge.dstNode < 0)
            graphLatency = std::max (graphLatency, edge.srcNode >= 0 ? artifactOut[static_cast<size_t> (edge.srcNode)] : 0);

    graph.latencySamples = graphLatency;

    const auto streamEndpoint = [&graph] (int node, int stream, bool isOutput) -> const YdspEndpointDecl*
    {
        if (node < 0)
        {
            const auto& list = isOutput ? graph.inputStreams : graph.outputStreams;

            if (stream >= 0 && static_cast<size_t> (stream) < list.size())
                return list[static_cast<size_t> (stream)];

            return nullptr;
        }

        int count = 0;

        for (const auto& endpoint : graph.nodes[static_cast<size_t> (node)].endpoints())
        {
            const bool matches = isOutput ? (endpoint.kind == YdspEndpointKind::outputStream)
                                          : (endpoint.kind == YdspEndpointKind::inputStream);

            if (matches)
            {
                if (count == stream)
                    return &endpoint;

                ++count;
            }
        }

        return nullptr;
    };

    for (auto& edge : graph.edges)
    {
        const auto upstream = edge.srcNode >= 0 ? artifactOut[static_cast<size_t> (edge.srcNode)] : 0;
        const auto needed = edge.dstNode >= 0 ? artifactIn[static_cast<size_t> (edge.dstNode)] : graphLatency;

        edge.compensationSamples = needed - upstream;

        jassert (edge.compensationSamples >= 0);

        if (edge.compensationSamples <= 0)
            continue;

        const auto* source = streamEndpoint (edge.srcNode, edge.srcStream, true);

        if (source != nullptr && source->type != YdspPrimitiveType::float32Type)
        {
            error (location,
                   "Latency compensation needs to delay a " + String (yup::toString (source->type))
                       + " stream by " + String (edge.compensationSamples)
                       + " samples, but the runtime's delay ring is float32-only in this version. Either make the stream float32, "
                         "or equalise the branches by hand with an explicit '-> [N] ->' and remove the '[[ latency ]]' declaration");
        }
    }

    for (auto& edge : graph.eventEdges)
    {
        // A graph input event carries no producer of its own - it reaches its
        // destination exactly as an uncompensated host-originated event does today.
        edge.compensationSamples = (edge.srcNode >= 0) ? artifactOut[static_cast<size_t> (edge.srcNode)] : 0;

        if (edge.dstNode < 0)
            edge.compensationSamples = graphLatency;

        jassert (edge.compensationSamples >= 0);
    }
}

} // namespace yup
