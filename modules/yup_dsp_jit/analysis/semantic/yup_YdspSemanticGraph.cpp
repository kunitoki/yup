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
            if (nodeDecl.rateMultiplier != 1 || nodeDecl.rateDivider != 1)
                error (nodeDecl.location,
                       "Node '" + nodeDecl.instanceName + "': oversampling/undersampling (*//N) is only supported on processors, not graphs");

            if (node.voiceCount <= 0)
                error (nodeDecl.location, "The voice count of node '" + nodeDecl.instanceName + "' must be a positive integer");

            if (node.voiceCount > 256)
            {
                error (nodeDecl.location, "The voice count of node '" + nodeDecl.instanceName + "' must not exceed 256");
                node.voiceCount = 1;
            }

            if (node.voiceCount == 1)
            {
                if (! nodeDecl.annotations.empty())
                    error (nodeDecl.location,
                           "Node '" + nodeDecl.instanceName + "': voice annotations are only supported on graphs instantiated as a voice bank ([N]) in this version");

                for (const auto& endpoint : subgraph->endpoints)
                    if (endpoint.kind == YdspEndpointKind::inputEvent)
                        error (nodeDecl.location,
                               "Graph '" + nodeDecl.processorName + "' declares an event input, so it can only be a main graph and not a node");
            }
            else
            {
                analyzeNodeAnnotations (nodeDecl, node);

                if (node.voiceMode == YdspVoiceMode::mono && node.voiceCount != 1)
                    error (nodeDecl.location, "Node '" + nodeDecl.instanceName + "' is 'mode: mono' and must declare exactly one voice (found " + String (node.voiceCount) + ")");

                int eventInputCount = 0;
                int outputStreamCount = 0;
                bool outputIsFloat32 = false;

                for (const auto& endpoint : subgraph->endpoints)
                {
                    if (endpoint.kind == YdspEndpointKind::inputEvent)
                        ++eventInputCount;

                    if (endpoint.kind == YdspEndpointKind::outputStream)
                    {
                        ++outputStreamCount;
                        outputIsFloat32 = (endpoint.type == YdspPrimitiveType::float32Type);
                    }
                }

                if (eventInputCount == 0)
                    error (nodeDecl.location, "Node '" + nodeDecl.instanceName + "': a graph used as a voice bank ([N]) must declare an input event");

                if (outputStreamCount != 1 || ! outputIsFloat32)
                    error (nodeDecl.location, "Node '" + nodeDecl.instanceName + "': a graph used as a voice bank ([N]) must declare exactly one float32 output stream");

                bool hasEventHandlerMember = false;

                for (const auto& memberDecl : subgraph->nodes)
                {
                    for (const auto& candidate : program.processors)
                    {
                        if (candidate.decl != nullptr
                            && candidate.decl->name == memberDecl.processorName
                            && ! candidate.eventHandlers.empty())
                        {
                            hasEventHandlerMember = true;
                            break;
                        }
                    }

                    if (hasEventHandlerMember)
                        break;
                }

                if (! hasEventHandlerMember)
                    error (nodeDecl.location, "Node '" + nodeDecl.instanceName + "': a graph used as a voice bank ([N]) must contain at least one node that declares an event handler");
            }
        }
        else
        {
            analyzeNodeAnnotations (nodeDecl, node);

            if (node.voiceCount <= 0)
                error (nodeDecl.location, "The voice count of node '" + nodeDecl.instanceName + "' must be a positive integer");

            if (node.voiceCount > 256)
            {
                error (nodeDecl.location, "The voice count of node '" + nodeDecl.instanceName + "' must not exceed 256");
                node.voiceCount = 1;
            }

            if (node.voiceCount > 1 && ! node.isEventDriven)
                error (nodeDecl.location, "Node '" + nodeDecl.instanceName + "' declares a voice bank ([N]) but processor '" + nodeDecl.processorName + "' has no event input");

            if (node.voiceMode == YdspVoiceMode::mono && node.voiceCount != 1)
                error (nodeDecl.location, "Node '" + nodeDecl.instanceName + "' is 'mode: mono' and must declare exactly one voice (found " + String (node.voiceCount) + ")");

            // A *0 or /0 node would divide by zero below (the latency math and
            // the runtime's rate conversion); validate like the voice count.
            if (node.rateMultiplier <= 0)
            {
                error (nodeDecl.location, "The oversampling factor of node '" + nodeDecl.instanceName + "' must be a positive integer");
                node.rateMultiplier = 1;
            }

            if (node.rateDivider <= 0)
            {
                error (nodeDecl.location, "The undersampling factor of node '" + nodeDecl.instanceName + "' must be a positive integer");
                node.rateDivider = 1;
            }

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

    bool hasGroups = false;

    for (const auto& node : graph.nodes)
    {
        if (node.voiceGroupIndex >= 0)
        {
            hasGroups = true;
            break;
        }
    }

    if (! hasGroups)
    {
        // No node carries a voice group - only inlining a banked subgraph
        // stamps voiceGroupIndex - so this is exactly the plain Kahn BFS below
        // and every existing graph keeps its byte-identical execution order.
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
        return;
    }

    // Group-aware pass. A banked subgraph is inlined as several adjacent
    // nodes that must stay contiguous in the execution order: the runtime
    // walks the voice loop around the whole group, not per node. Contract each
    // group into one supernode, Kahn the quotient, then expand each supernode
    // back into its members in the group's own internal topological order. The
    // group is convex (a cycle through it would have been a cycle through its
    // placeholder and is already rejected upstream), so a supernode order
    // always exists; the leftover check below is insurance on that argument.
    const auto groupIdOf = [&graph] (int nodeIndex)
    {
        return graph.nodes[static_cast<size_t> (nodeIndex)].voiceGroupIndex;
    };

    std::unordered_map<int, int> groupClass; // voiceGroupIndex -> class id
    std::vector<int> classOf (static_cast<size_t> (numNodes), -1);
    std::vector<std::vector<int>> classMembers;
    std::vector<bool> classIsGroup;
    int classCount = 0;

    for (int i = 0; i < numNodes; ++i)
    {
        const int groupId = groupIdOf (i);

        if (groupId < 0)
        {
            classOf[static_cast<size_t> (i)] = classCount++;
            classMembers.emplace_back (1, i);
            classIsGroup.push_back (false);
            continue;
        }

        const auto found = groupClass.find (groupId);

        if (found != groupClass.end())
        {
            const int c = found->second;
            classOf[static_cast<size_t> (i)] = c;
            classMembers[static_cast<size_t> (c)].push_back (i);
            continue;
        }

        const int c = classCount++;
        groupClass[groupId] = c;
        classOf[static_cast<size_t> (i)] = c;
        classMembers.emplace_back (1, i);
        classIsGroup.push_back (true);
    }

    // Classes in ascending first-member node order, so the quotient Kahn below
    // is deterministic: it is seeded from and drains into this exact sequence.
    std::vector<int> classOrder (static_cast<size_t> (classCount));

    for (int c = 0; c < classCount; ++c)
        classOrder[static_cast<size_t> (c)] = c;

    std::sort (classOrder.begin(), classOrder.end(), [&classMembers] (int a, int b)
    {
        return classMembers[static_cast<size_t> (a)].front() < classMembers[static_cast<size_t> (b)].front();
    });

    std::vector<int> indegree (static_cast<size_t> (classCount), 0);
    std::vector<std::vector<int>> outEdges (static_cast<size_t> (classCount));

    const auto contractEdge = [&] (int srcNode, int dstNode)
    {
        const int srcClass = classOf[static_cast<size_t> (srcNode)];
        const int dstClass = classOf[static_cast<size_t> (dstNode)];

        if (srcClass == dstClass)
            return; // stays inside one group; resolved by the group's own expansion

        outEdges[static_cast<size_t> (srcClass)].push_back (dstClass);
        ++indegree[static_cast<size_t> (dstClass)];
    };

    for (const auto& edge : graph.edges)
        if (edge.srcNode >= 0 && edge.dstNode >= 0)
            contractEdge (edge.srcNode, edge.dstNode);

    for (const auto& edge : graph.eventEdges)
        if (edge.srcNode >= 0 && edge.dstNode >= 0)
            contractEdge (edge.srcNode, edge.dstNode);

    std::vector<int> ready;

    for (const int c : classOrder)
        if (indegree[static_cast<size_t> (c)] == 0)
            ready.push_back (c);

    for (size_t i = 0; i < ready.size(); ++i)
    {
        const int c = ready[i];

        for (const int successor : outEdges[static_cast<size_t> (c)])
            if (--indegree[static_cast<size_t> (successor)] == 0)
                ready.push_back (successor);
    }

    if (ready.size() != static_cast<size_t> (classCount))
    {
        std::vector<bool> emitted (static_cast<size_t> (classCount), false);

        for (const int c : ready)
            emitted[static_cast<size_t> (c)] = true;

        // Any leftover group class is a bank the cycle runs through. Such a
        // cycle should be unreachable (it would have been a cycle through the
        // placeholder upstream), but if the pass ordering ever changes, name
        // the offending wire instead of tripping an assert.
        int leftoverGroup = -1;

        for (int c = 0; c < classCount; ++c)
        {
            if (! emitted[static_cast<size_t> (c)] && classIsGroup[static_cast<size_t> (c)])
            {
                leftoverGroup = c;
                break;
            }
        }

        if (leftoverGroup >= 0)
        {
            const auto& members = classMembers[static_cast<size_t> (leftoverGroup)];
            const auto& firstMember = graph.nodes[static_cast<size_t> (members.front())].instanceName;
            const auto dot = firstMember.lastIndexOfChar ('.');

            String bankName = dot > 0 ? firstMember.substring (0, dot) : firstMember;
            String wire;

            const auto nameWire = [&] (int srcNode, int dstNode)
            {
                wire = " from '" + graph.nodes[static_cast<size_t> (srcNode)].instanceName
                     + "' to '" + graph.nodes[static_cast<size_t> (dstNode)].instanceName + "'";
            };

            for (const auto& edge : graph.edges)
            {
                if (edge.srcNode < 0 || edge.dstNode < 0)
                    continue;

                const int srcClass = classOf[static_cast<size_t> (edge.srcNode)];
                const int dstClass = classOf[static_cast<size_t> (edge.dstNode)];

                if ((srcClass == leftoverGroup && ! emitted[static_cast<size_t> (dstClass)])
                    || (dstClass == leftoverGroup && ! emitted[static_cast<size_t> (srcClass)]))
                {
                    nameWire (edge.srcNode, edge.dstNode);
                    break;
                }
            }

            if (wire.isEmpty())
            {
                for (const auto& edge : graph.eventEdges)
                {
                    if (edge.srcNode < 0 || edge.dstNode < 0)
                        continue;

                    const int srcClass = classOf[static_cast<size_t> (edge.srcNode)];
                    const int dstClass = classOf[static_cast<size_t> (edge.dstNode)];

                    if ((srcClass == leftoverGroup && ! emitted[static_cast<size_t> (dstClass)])
                        || (dstClass == leftoverGroup && ! emitted[static_cast<size_t> (srcClass)]))
                    {
                        nameWire (edge.srcNode, edge.dstNode);
                        break;
                    }
                }
            }

            String message = "The graph contains a signal that leaves and re-enters the voice bank '" + bankName + "'";

            if (wire.isNotEmpty())
                message += " through the wire" + wire;

            message += ", which is not supported in this version";
            error (location, message);
        }
        else
        {
            error (location, "The graph contains a feedback cycle, which is not supported in this version (an inline delay on an edge does not break it)");
        }
    }

    // Expand each class back into node indices. A singleton class is its own
    // node; a supernode yields its members in the group's internal topological
    // order, which always exists because the subgraph was cycle-checked on its
    // own before inlining.
    std::vector<int> expanded;
    expanded.reserve (static_cast<size_t> (numNodes));

    for (const int c : ready)
    {
        const auto& members = classMembers[static_cast<size_t> (c)];

        if (members.size() == 1)
        {
            expanded.push_back (members.front());
            continue;
        }

        std::vector<int> internalIndegree (members.size(), 0);
        std::vector<std::vector<int>> internalOutEdges (members.size());

        const auto memberPosition = [&members] (int node)
        {
            for (size_t m = 0; m < members.size(); ++m)
                if (members[m] == node)
                    return static_cast<int> (m);

            return -1;
        };

        const auto addInternalEdge = [&] (int srcNode, int dstNode)
        {
            const int src = memberPosition (srcNode);
            const int dst = memberPosition (dstNode);

            if (src < 0 || dst < 0)
                return;

            internalOutEdges[static_cast<size_t> (src)].push_back (dst);
            ++internalIndegree[static_cast<size_t> (dst)];
        };

        for (const auto& edge : graph.edges)
            addInternalEdge (edge.srcNode, edge.dstNode);

        for (const auto& edge : graph.eventEdges)
            addInternalEdge (edge.srcNode, edge.dstNode);

        std::vector<int> order;

        for (size_t m = 0; m < members.size(); ++m)
            if (internalIndegree[m] == 0)
                order.push_back (static_cast<int> (m));

        for (size_t i = 0; i < order.size(); ++i)
        {
            for (const int successor : internalOutEdges[static_cast<size_t> (order[i])])
                if (--internalIndegree[static_cast<size_t> (successor)] == 0)
                    order.push_back (successor);
        }

        if (order.size() != members.size())
        {
            const auto& firstMember = graph.nodes[static_cast<size_t> (members.front())].instanceName;
            const auto dot = firstMember.lastIndexOfChar ('.');

            error (location, "The graph contains a feedback cycle inside the voice bank '"
                       + (dot > 0 ? firstMember.substring (0, dot) : firstMember)
                       + "', which is not supported in this version");
        }

        for (const int m : order)
            expanded.push_back (members[static_cast<size_t> (m)]);
    }

    graph.topoOrder = std::move (expanded);
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

    // Exclusive prefix sums over each node's endpoint counts, so any
    // "global slot of node n, endpoint k" is a constant-time lookup.
    const auto prefix = [] (const std::vector<int>& counts)
    {
        std::vector<int> start (counts.size() + 1, 0);

        for (size_t i = 0; i < counts.size(); ++i)
            start[i + 1] = start[i] + counts[i];

        return start;
    };

    const auto nodeInputStart = prefix (nodeInputCounts);
    const auto nodeOutputStart = prefix (nodeOutputCounts);
    const auto nodeParamStart = prefix (nodeParamCounts);
    const auto nodeMeterStart = prefix (nodeMeterCounts);

    const int totalNodeInputs = nodeInputStart.back();
    const int totalNodeOutputs = nodeOutputStart.back();
    const int totalNodeParams = nodeParamStart.back();
    const int totalNodeMeters = nodeMeterStart.back();

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
            const auto offset = nodeOutputStart[static_cast<size_t> (edge.srcNode)] + edge.srcStream;
            ++nodeOutputUses[static_cast<size_t> (offset)];
        }

        if (edge.dstNode == -1)
        {
            ++graphOutputUses[static_cast<size_t> (edge.dstStream)];
        }
        else
        {
            const auto offset = nodeInputStart[static_cast<size_t> (edge.dstNode)] + edge.dstStream;
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
                const auto offset = nodeInputStart[n] + inCount;
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
                const auto offset = nodeOutputStart[n] + outCount;

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

    const auto nodeOutputEventStart = prefix (nodeOutputEventCounts);

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

    const auto nodeInputEventStart = prefix (nodeInputEventCounts);

    std::vector<int> graphOutputEventUses (static_cast<size_t> (graph.outputEvents.size()), 0);
    std::vector<int> nodeOutputEventUses (static_cast<size_t> (totalNodeOutputEvents), 0);
    std::vector<int> graphInputEventUses (static_cast<size_t> (graph.inputEvents.size()), 0);
    std::vector<int> nodeInputEventUses (static_cast<size_t> (totalNodeInputEvents), 0);

    for (const auto& edge : graph.eventEdges)
    {
        if (edge.srcNode < 0)
            ++graphInputEventUses[static_cast<size_t> (edge.srcEndpoint)];
        else
            ++nodeOutputEventUses[static_cast<size_t> (nodeOutputEventStart[static_cast<size_t> (edge.srcNode)] + edge.srcEndpoint)];

        if (edge.dstNode < 0)
            ++graphOutputEventUses[static_cast<size_t> (edge.dstEndpoint)];
        else
            ++nodeInputEventUses[static_cast<size_t> (nodeInputEventStart[static_cast<size_t> (edge.dstNode)] + edge.dstEndpoint)];
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
                const auto offset = nodeInputEventStart[n] + inCount;

                if (nodeInputEventUses[static_cast<size_t> (offset)] == 0)
                    error (endpoint.location, "Node '" + instanceName + "' input event '" + endpoint.name + "' is not connected: it must be driven by at least one source");

                ++inCount;
            }
            else if (endpoint.kind == YdspEndpointKind::outputEvent)
            {
                const auto offset = nodeOutputEventStart[n] + outCount;

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
        return nodeParamStart[static_cast<size_t> (node)] + param;
    };
    auto meterSlot = [&] (int node, int meter)
    {
        return nodeMeterStart[static_cast<size_t> (node)] + meter;
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
