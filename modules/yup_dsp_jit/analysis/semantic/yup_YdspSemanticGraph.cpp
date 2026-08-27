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

void YdspSemanticAnalyzer::analyzeNodeAnnotations (const YdspNodeDecl& decl, YdspAnalyzedNode& node)
{
    for (const auto& [key, value] : decl.annotations)
    {
        if (key == "mode")
        {
            if (value == "poly")
                node.voiceMode = YdspVoiceMode::poly;
            else if (value == "mono")
                node.voiceMode = YdspVoiceMode::mono;
            else
                error (decl.location, "Unknown voice mode '" + value + "' on node '" + decl.instanceName + "' (expected 'poly' or 'mono')");
        }
        else if (key == "stealing")
        {
            if (value == "oldest")
                node.stealing = YdspVoiceStealing::oldest;
            else if (value == "newest")
                node.stealing = YdspVoiceStealing::newest;
            else if (value == "none")
                node.stealing = YdspVoiceStealing::none;
            else
                error (decl.location, "Unknown stealing policy '" + value + "' on node '" + decl.instanceName + "' (expected 'oldest', 'newest' or 'none')");
        }
        else if (key == "priority")
        {
            if (value == "last")
                node.monoPriority = YdspMonoPriority::last;
            else if (value == "low")
                node.monoPriority = YdspMonoPriority::low;
            else if (value == "high")
                node.monoPriority = YdspMonoPriority::high;
            else
                error (decl.location, "Unknown note priority '" + value + "' on node '" + decl.instanceName + "' (expected 'last', 'low' or 'high')");
        }
        else
        {
            error (decl.location, "Unknown node annotation '" + key + "' on node '" + decl.instanceName + "' (expected 'mode', 'stealing' or 'priority')");
        }
    }
}

//==============================================================================

std::unique_ptr<YdspAnalyzedGraph> YdspSemanticAnalyzer::analyzeGraph (const YdspGraphDecl& decl, const YdspAnalyzedProgram& program)
{
    auto graph = std::make_unique<YdspAnalyzedGraph>();

    for (const auto& endpoint : decl.endpoints)
    {
        switch (endpoint.kind)
        {
            case YdspEndpointKind::inputStream:
                graph->inputStreams.push_back (&endpoint);
                break;
            case YdspEndpointKind::outputStream:
                graph->outputStreams.push_back (&endpoint);
                break;
            case YdspEndpointKind::inputValue:
                graph->inputValues.push_back (&endpoint);
                graph->inputValueDefaults.push_back (constEvalDefault (endpoint.defaultValue.get(), endpoint.type));
                break;

            case YdspEndpointKind::outputValue:
                graph->outputValues.push_back (&endpoint);
                break;

            case YdspEndpointKind::inputEvent:
            {
                bool duplicate = false;

                for (const auto* existing : graph->inputEvents)
                    if (existing->name == endpoint.name)
                    {
                        duplicate = true;
                        break;
                    }

                if (duplicate)
                    error (endpoint.location, "Duplicate event input '" + endpoint.name + "'");
                else
                    graph->inputEvents.push_back (&endpoint);

                break;
            }

            case YdspEndpointKind::outputEvent:
            {
                bool duplicate = false;

                for (const auto* existing : graph->outputEvents)
                    if (existing->name == endpoint.name)
                    {
                        duplicate = true;
                        break;
                    }

                if (duplicate)
                    error (endpoint.location, "Duplicate event output '" + endpoint.name + "'");
                else
                    graph->outputEvents.push_back (&endpoint);

                break;
            }
        }
    }

    std::unordered_map<String, int> nodeIndexByName;

    for (const auto& nodeDecl : decl.nodes)
    {
        if (nodeIndexByName.find (nodeDecl.instanceName) != nodeIndexByName.end())
        {
            error (nodeDecl.location, "Duplicate node instance name '" + nodeDecl.instanceName + "'");
            continue;
        }

        const YdspAnalyzedProcessor* processor = nullptr;
        const YdspGraphDecl* subgraph = nullptr;
        int subgraphIndex = -1;

        for (const auto& candidate : program.processors)
            if (candidate.decl != nullptr && candidate.decl->name == nodeDecl.processorName)
                processor = &candidate;

        if (processor == nullptr)
        {
            subgraphIndex = findGraphIndex (*program.ast, nodeDecl.processorName);

            if (subgraphIndex >= 0)
                subgraph = &program.ast->graphs[static_cast<size_t> (subgraphIndex)];
        }

        if (processor == nullptr && subgraph == nullptr)
        {
            error (nodeDecl.location, "Unknown processor or graph '" + nodeDecl.processorName + "'");
            continue;
        }

        YdspAnalyzedNode node;
        node.instanceName = nodeDecl.instanceName;
        node.processor = (processor != nullptr ? processor->decl : nullptr);
        node.subgraph = subgraph;
        node.subgraphIndex = subgraphIndex;
        node.rateMultiplier = nodeDecl.rateMultiplier;
        node.rateDivider = nodeDecl.rateDivider;
        node.voiceCount = nodeDecl.voiceCount;
        node.isEventDriven = (processor != nullptr && ! processor->eventHandlers.empty());

        const auto what = String (subgraph != nullptr ? "Graph '" : "Processor '") + nodeDecl.processorName + "'";

        for (const auto& [paramName, valueExpr] : nodeDecl.overrides)
        {
            bool found = false;

            for (const auto& endpoint : node.endpoints())
            {
                if (endpoint.name != paramName)
                    continue;

                found = true;

                if (endpoint.kind != YdspEndpointKind::inputValue)
                    error (valueExpr->location, "'" + paramName + "' is not a parameter of " + what);
            }

            if (! found)
                error (nodeDecl.location, what + " has no parameter '" + paramName + "'");
        }

        if (subgraph != nullptr)
        {
            if (nodeDecl.voiceCount != 1)
                error (nodeDecl.location,
                       "Node '" + nodeDecl.instanceName + "': voice banks ([N]) are only supported on processors, not graphs, "
                                                          "because the runtime's per-voice summing path requires one processor with one float32 output stream");

            if (nodeDecl.rateMultiplier != 1 || nodeDecl.rateDivider != 1)
                error (nodeDecl.location,
                       "Node '" + nodeDecl.instanceName + "': oversampling/undersampling (*//N) is only supported on processors, not graphs");

            if (! nodeDecl.annotations.empty())
                error (nodeDecl.location,
                       "Node '" + nodeDecl.instanceName + "': voice annotations are only supported on processors, not graphs");

            for (const auto& endpoint : subgraph->endpoints)
                if (endpoint.kind == YdspEndpointKind::inputEvent)
                    error (nodeDecl.location,
                           "Graph '" + nodeDecl.processorName + "' declares an event input, so it can only be a main graph and not a node");
        }
        else
        {
            analyzeNodeAnnotations (nodeDecl, node);

            if (node.voiceCount <= 0)
                error (nodeDecl.location, "The voice count of node '" + nodeDecl.instanceName + "' must be a positive integer");

            if (node.voiceCount > 1 && ! node.isEventDriven)
                error (nodeDecl.location, "Node '" + nodeDecl.instanceName + "' declares a voice bank ([N]) but processor '" + nodeDecl.processorName + "' has no event input");

            if (node.voiceMode == YdspVoiceMode::mono && node.voiceCount != 1)
                error (nodeDecl.location, "Node '" + nodeDecl.instanceName + "' is 'mode: mono' and must declare exactly one voice (found " + String (node.voiceCount) + ")");

            if (node.isEventDriven && (node.rateMultiplier > 1 || node.rateDivider > 1))
                error (nodeDecl.location, "An event-driven node cannot use oversampling/undersampling (*//N) in this version");

            if (node.rateDivider > 1 && node.rateDivider != 2 && node.rateDivider != 4 && node.rateDivider != 8)
                error (nodeDecl.location,
                       "Node '" + nodeDecl.instanceName + "': undersampling (/N) supports a factor of 2, 4 or 8, but this is /" + String (node.rateDivider));

            if (node.rateMultiplier > 1 || node.rateDivider > 1)
            {
                for (const auto& endpoint : node.endpoints())
                {
                    if (endpoint.kind != YdspEndpointKind::inputStream && endpoint.kind != YdspEndpointKind::outputStream)
                        continue;

                    if (endpoint.type == YdspPrimitiveType::float32Type)
                        continue;

                    error (nodeDecl.location,
                           "Node '" + nodeDecl.instanceName + "': a rate change (*N or /N) is only supported on float32 streams, but " + what
                               + " declares stream '" + endpoint.name + "' as " + yup::toString (endpoint.type));
                }
            }

            {
                const auto declared = processor->declaredLatencySamples;

                if (declared % node.rateMultiplier != 0)
                    error (nodeDecl.location,
                           "Node '" + nodeDecl.instanceName + "': " + what + " declares '[[ latency: " + String (declared)
                               + " ]]' in its own sample domain, which is not divisible by this instance's oversampling factor of "
                               + String (node.rateMultiplier));

                if (node.rateDivider > 1)
                {
                    node.latencySamples = ydspOversamplerLatencySamples * node.rateDivider
                                        + (node.rateDivider - 1)
                                        + declared * node.rateDivider;
                }
                else
                {
                    node.latencySamples = (node.rateMultiplier > 1 ? ydspOversamplerLatencySamples : 0)
                                        + declared / node.rateMultiplier;
                }
            }

            if (node.isEventDriven)
            {
                const int inputStreamCount = static_cast<int> (processor->inputStreams.size());
                int outputStreamCount = 0;
                bool outputIsFloat32 = false;

                for (const auto* endpoint : processor->outputStreams)
                {
                    ++outputStreamCount;
                    outputIsFloat32 = (endpoint->type == YdspPrimitiveType::float32Type);
                }

                const bool isMidiOnly = (inputStreamCount == 0 && outputStreamCount == 0);
                node.isMidiOnly = isMidiOnly;

                if (! isMidiOnly && outputStreamCount != 1)
                    error (nodeDecl.location, "Event-driven processor '" + nodeDecl.processorName + "' must declare exactly one output stream in this version");

                if (outputStreamCount == 1 && ! outputIsFloat32)
                    error (nodeDecl.location, "Event-driven processor '" + nodeDecl.processorName + "' must declare a float32 output stream in this version");
            }
        }

        for (const auto& paramEndpoint : node.endpoints())
        {
            if (paramEndpoint.kind != YdspEndpointKind::inputValue)
                continue;

            const YdspExpr* defaultExpr = nullptr;

            for (const auto& [paramName, valueExpr] : nodeDecl.overrides)
            {
                if (paramName == paramEndpoint.name)
                {
                    defaultExpr = valueExpr.get();
                    break;
                }
            }

            if (defaultExpr == nullptr)
                defaultExpr = paramEndpoint.defaultValue.get();

            node.paramDefaults.push_back (constEvalDefault (defaultExpr, paramEndpoint.type));
        }

        nodeIndexByName[nodeDecl.instanceName] = static_cast<int> (graph->nodes.size());
        graph->nodes.push_back (std::move (node));
    }

    switch (decl.bodyKind)
    {
        case YdspGraphBodyKind::connections:
            analyzeConnectionsForm (decl, *graph, nodeIndexByName);
            break;

        case YdspGraphBodyKind::algebra:
            analyzeAlgebraForm (decl, *graph, nodeIndexByName, program);
            break;

        default:
            error (decl.location, "The graph must contain a 'connection' block or a 'process =' definition");
            break;
    }

    rebuildTopoOrder (*graph, decl.location);

    return graph;
}

//==============================================================================

void YdspSemanticAnalyzer::rebuildTopoOrder (YdspAnalyzedGraph& graph, const YdspLocation& location)
{
    const int numNodes = static_cast<int> (graph.nodes.size());
    std::vector<int> indegree (static_cast<size_t> (numNodes), 0);
    std::vector<std::vector<int>> outEdges (static_cast<size_t> (numNodes));

    for (const auto& edge : graph.edges)
    {
        if (edge.srcNode >= 0 && edge.dstNode >= 0)
        {
            outEdges[static_cast<size_t> (edge.srcNode)].push_back (edge.dstNode);
            ++indegree[static_cast<size_t> (edge.dstNode)];
        }
    }

    for (const auto& edge : graph.eventEdges)
    {
        if (edge.srcNode >= 0 && edge.dstNode >= 0)
        {
            outEdges[static_cast<size_t> (edge.srcNode)].push_back (edge.dstNode);
            ++indegree[static_cast<size_t> (edge.dstNode)];
        }
    }

    std::vector<int> ready;

    for (int i = 0; i < numNodes; ++i)
        if (indegree[static_cast<size_t> (i)] == 0)
            ready.push_back (i);

    for (size_t i = 0; i < ready.size(); ++i)
    {
        const int node = ready[i];

        for (const int successor : outEdges[static_cast<size_t> (node)])
            if (--indegree[static_cast<size_t> (successor)] == 0)
                ready.push_back (successor);
    }

    if (ready.size() != static_cast<size_t> (numNodes))
        error (location, "The graph contains a feedback cycle, which is not supported in this version (an inline delay on an edge does not break it)");

    graph.topoOrder = std::move (ready);
}

//==============================================================================

void YdspSemanticAnalyzer::validateConnectivity (const YdspGraphDecl& decl, YdspAnalyzedGraph& graph)
{
    (void) decl;

    const int numGraphInputs = static_cast<int> (graph.inputStreams.size());
    const int numGraphOutputs = static_cast<int> (graph.outputStreams.size());

    std::vector<int> nodeInputCounts;
    std::vector<int> nodeOutputCounts;
    std::vector<int> nodeParamCounts;
    std::vector<int> nodeMeterCounts;

    for (const auto& node : graph.nodes)
    {
        int inCount = 0;
        int outCount = 0;
        int paramCount = 0;
        int meterCount = 0;

        for (const auto& endpoint : node.endpoints())
        {
            if (endpoint.kind == YdspEndpointKind::inputStream)
                ++inCount;
            if (endpoint.kind == YdspEndpointKind::outputStream)
                ++outCount;
            if (endpoint.kind == YdspEndpointKind::inputValue)
                ++paramCount;
            if (endpoint.kind == YdspEndpointKind::outputValue)
                ++meterCount;
        }

        nodeInputCounts.push_back (inCount);
        nodeOutputCounts.push_back (outCount);
        nodeParamCounts.push_back (paramCount);
        nodeMeterCounts.push_back (meterCount);
    }

    auto nodeInputStart = [&] (int node)
    {
        int offset = 0;
        for (int i = 0; i < node; ++i)
            offset += nodeInputCounts[static_cast<size_t> (i)];

        return offset;
    };

    auto nodeOutputStart = [&] (int node)
    {
        int offset = 0;
        for (int i = 0; i < node; ++i)
            offset += nodeOutputCounts[static_cast<size_t> (i)];

        return offset;
    };

    auto nodeParamStart = [&] (int node)
    {
        int offset = 0;
        for (int i = 0; i < node; ++i)
            offset += nodeParamCounts[static_cast<size_t> (i)];

        return offset;
    };

    auto nodeMeterStart = [&] (int node)
    {
        int offset = 0;
        for (int i = 0; i < node; ++i)
            offset += nodeMeterCounts[static_cast<size_t> (i)];

        return offset;
    };

    const int totalNodeInputs = nodeInputStart (static_cast<int> (graph.nodes.size()));
    const int totalNodeOutputs = nodeOutputStart (static_cast<int> (graph.nodes.size()));
    const int totalNodeParams = nodeParamStart (static_cast<int> (graph.nodes.size()));
    const int totalNodeMeters = nodeMeterStart (static_cast<int> (graph.nodes.size()));

    std::vector<int> graphInputUses (static_cast<size_t> (numGraphInputs), 0);
    std::vector<int> graphOutputUses (static_cast<size_t> (numGraphOutputs), 0);
    std::vector<int> nodeInputUses (static_cast<size_t> (totalNodeInputs), 0);
    std::vector<int> nodeOutputUses (static_cast<size_t> (totalNodeOutputs), 0);

    for (const auto& edge : graph.edges)
    {
        if (edge.srcNode == -1)
        {
            ++graphInputUses[static_cast<size_t> (edge.srcStream)];
        }
        else
        {
            const auto offset = nodeOutputStart (edge.srcNode) + edge.srcStream;
            ++nodeOutputUses[static_cast<size_t> (offset)];
        }

        if (edge.dstNode == -1)
        {
            ++graphOutputUses[static_cast<size_t> (edge.dstStream)];
        }
        else
        {
            const auto offset = nodeInputStart (edge.dstNode) + edge.dstStream;
            ++nodeInputUses[static_cast<size_t> (offset)];
        }
    }

    const auto rejectNonFloatFanIn = [this] (const YdspEndpointDecl& endpoint, int uses, const String& prefix)
    {
        if (uses < 2)
            return;

        if (endpoint.type == YdspPrimitiveType::float32Type || endpoint.type == YdspPrimitiveType::float64Type)
            return;

        error (endpoint.location,
               prefix + " '" + endpoint.name + "' is driven by " + String (uses)
                   + " sources, but implicit summing is only supported on float32 and float64 streams (this one is "
                   + yup::toString (endpoint.type) + ")");
    };

    for (int i = 0; i < numGraphInputs; ++i)
        if (graphInputUses[static_cast<size_t> (i)] == 0)
            error (graph.inputStreams[static_cast<size_t> (i)]->location,
                   "Graph input '" + graph.inputStreams[static_cast<size_t> (i)]->name + "' is not connected: it must feed at least one destination");

    for (int i = 0; i < numGraphOutputs; ++i)
    {
        const auto* endpoint = graph.outputStreams[static_cast<size_t> (i)];
        const auto uses = graphOutputUses[static_cast<size_t> (i)];

        if (uses == 0)
            error (endpoint->location, "Graph output '" + endpoint->name + "' is not connected: it must be driven by at least one source");
        else
            rejectNonFloatFanIn (*endpoint, uses, "Graph output");
    }

    for (size_t n = 0; n < graph.nodes.size(); ++n)
    {
        const auto& instanceName = graph.nodes[n].instanceName;

        int inCount = 0;
        for (const auto& endpoint : graph.nodes[n].endpoints())
        {
            if (endpoint.kind == YdspEndpointKind::inputStream)
            {
                const auto offset = nodeInputStart (static_cast<int> (n)) + inCount;
                const auto uses = nodeInputUses[static_cast<size_t> (offset)];

                if (uses == 0)
                    error (endpoint.location, "Node '" + instanceName + "' input '" + endpoint.name + "' is not connected: it must be driven by at least one source");
                else
                    rejectNonFloatFanIn (endpoint, uses, "Node '" + instanceName + "' input");

                ++inCount;
            }
        }

        int outCount = 0;
        for (const auto& endpoint : graph.nodes[n].endpoints())
        {
            if (endpoint.kind == YdspEndpointKind::outputStream)
            {
                const auto offset = nodeOutputStart (static_cast<int> (n)) + outCount;

                if (nodeOutputUses[static_cast<size_t> (offset)] == 0)
                    error (endpoint.location, "Node '" + instanceName + "' output '" + endpoint.name + "' is not connected: it must feed at least one destination");

                ++outCount;
            }
        }
    }

    int totalNodeOutputEvents = 0;
    std::vector<int> nodeOutputEventCounts;

    for (const auto& node : graph.nodes)
    {
        int count = 0;

        for (const auto& endpoint : node.endpoints())
            if (endpoint.kind == YdspEndpointKind::outputEvent)
                ++count;

        nodeOutputEventCounts.push_back (count);
        totalNodeOutputEvents += count;
    }

    auto nodeOutputEventStart = [&] (int node)
    {
        int offset = 0;
        for (int i = 0; i < node; ++i)
            offset += nodeOutputEventCounts[static_cast<size_t> (i)];

        return offset;
    };

    int totalNodeInputEvents = 0;
    std::vector<int> nodeInputEventCounts;

    for (const auto& node : graph.nodes)
    {
        int count = 0;

        for (const auto& endpoint : node.endpoints())
            if (endpoint.kind == YdspEndpointKind::inputEvent)
                ++count;

        nodeInputEventCounts.push_back (count);
        totalNodeInputEvents += count;
    }

    auto nodeInputEventStart = [&] (int node)
    {
        int offset = 0;
        for (int i = 0; i < node; ++i)
            offset += nodeInputEventCounts[static_cast<size_t> (i)];

        return offset;
    };

    std::vector<int> graphOutputEventUses (static_cast<size_t> (graph.outputEvents.size()), 0);
    std::vector<int> nodeOutputEventUses (static_cast<size_t> (totalNodeOutputEvents), 0);
    std::vector<int> graphInputEventUses (static_cast<size_t> (graph.inputEvents.size()), 0);
    std::vector<int> nodeInputEventUses (static_cast<size_t> (totalNodeInputEvents), 0);

    for (const auto& edge : graph.eventEdges)
    {
        if (edge.srcNode < 0)
            ++graphInputEventUses[static_cast<size_t> (edge.srcEndpoint)];
        else
            ++nodeOutputEventUses[static_cast<size_t> (nodeOutputEventStart (edge.srcNode) + edge.srcEndpoint)];

        if (edge.dstNode < 0)
            ++graphOutputEventUses[static_cast<size_t> (edge.dstEndpoint)];
        else
            ++nodeInputEventUses[static_cast<size_t> (nodeInputEventStart (edge.dstNode) + edge.dstEndpoint)];
    }

    for (size_t i = 0; i < graph.inputEvents.size(); ++i)
        if (graphInputEventUses[i] == 0)
            error (graph.inputEvents[i]->location, "Graph input event '" + graph.inputEvents[i]->name + "' is not connected: it must feed at least one destination");

    for (size_t i = 0; i < graph.outputEvents.size(); ++i)
        if (graphOutputEventUses[i] == 0)
            error (graph.outputEvents[i]->location, "Graph output event '" + graph.outputEvents[i]->name + "' is not connected: it must be driven by at least one source");

    for (size_t n = 0; n < graph.nodes.size(); ++n)
    {
        const auto& instanceName = graph.nodes[n].instanceName;

        int inCount = 0;
        int outCount = 0;

        for (const auto& endpoint : graph.nodes[n].endpoints())
        {
            if (endpoint.kind == YdspEndpointKind::inputEvent)
            {
                const auto offset = nodeInputEventStart (static_cast<int> (n)) + inCount;

                if (nodeInputEventUses[static_cast<size_t> (offset)] == 0)
                    error (endpoint.location, "Node '" + instanceName + "' input event '" + endpoint.name + "' is not connected: it must be driven by at least one source");

                ++inCount;
            }
            else if (endpoint.kind == YdspEndpointKind::outputEvent)
            {
                const auto offset = nodeOutputEventStart (static_cast<int> (n)) + outCount;

                if (nodeOutputEventUses[static_cast<size_t> (offset)] == 0)
                    error (endpoint.location, "Node '" + instanceName + "' output event '" + endpoint.name + "' is not connected: it must feed at least one destination");

                ++outCount;
            }
        }
    }

    std::vector<int> nodeParamUses (static_cast<size_t> (totalNodeParams), 0);
    std::vector<int> nodeMeterUses (static_cast<size_t> (totalNodeMeters), 0);
    std::vector<int> graphMeterUses (static_cast<size_t> (graph.outputValues.size()), 0);

    auto paramSlot = [&] (int node, int param)
    {
        return nodeParamStart (node) + param;
    };
    auto meterSlot = [&] (int node, int meter)
    {
        return nodeMeterStart (node) + meter;
    };

    for (const auto& edge : graph.valueEdges)
        ++nodeParamUses[static_cast<size_t> (paramSlot (edge.dstNode, edge.dstParam))];

    for (const auto& edge : graph.meterEdges)
    {
        ++nodeMeterUses[static_cast<size_t> (meterSlot (edge.srcNode, edge.srcMeter))];
        ++graphMeterUses[static_cast<size_t> (edge.dstMeter)];
    }

    for (size_t n = 0; n < graph.nodes.size(); ++n)
    {
        int paramCount = 0;
        for (const auto& endpoint : graph.nodes[n].endpoints())
        {
            if (endpoint.kind == YdspEndpointKind::inputValue)
            {
                if (nodeParamUses[static_cast<size_t> (paramSlot (static_cast<int> (n), paramCount))] > 1)
                    error (endpoint.location, "Node '" + graph.nodes[n].instanceName + "' parameter '" + endpoint.name + "' can be connected at most once");

                ++paramCount;
            }
        }

        int meterCount = 0;
        for (const auto& endpoint : graph.nodes[n].endpoints())
        {
            if (endpoint.kind == YdspEndpointKind::outputValue)
            {
                if (nodeMeterUses[static_cast<size_t> (meterSlot (static_cast<int> (n), meterCount))] > 1)
                    error (endpoint.location, "Node '" + graph.nodes[n].instanceName + "' meter '" + endpoint.name + "' can be connected at most once");

                ++meterCount;
            }
        }
    }

    for (size_t i = 0; i < graph.outputValues.size(); ++i)
        if (graphMeterUses[i] != 1)
            error (graph.outputValues[i]->location, "Graph meter '" + graph.outputValues[i]->name + "' must be connected exactly once");
}

} // namespace yup
