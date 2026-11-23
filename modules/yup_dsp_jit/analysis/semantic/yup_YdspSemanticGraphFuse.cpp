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

//==============================================================================

void collectDeclaredNames (const YdspStmt& stmt, std::vector<String>& out)
{
    if (stmt.kind == YdspStmtKind::localDecl || stmt.kind == YdspStmtKind::forStmt)
        if (stmt.name.isNotEmpty())
            out.push_back (stmt.name);

    for (const auto& child : stmt.children)
        if (child != nullptr)
            collectDeclaredNames (*child, out);

    if (stmt.thenStmt != nullptr)
        collectDeclaredNames (*stmt.thenStmt, out);

    if (stmt.elseStmt != nullptr)
        collectDeclaredNames (*stmt.elseStmt, out);

    if (stmt.body != nullptr)
        collectDeclaredNames (*stmt.body, out);
}

YdspExprPtr makeZero (YdspLocation location, YdspPrimitiveType type)
{
    switch (type)
    {
        case YdspPrimitiveType::boolType:
            return YdspExprFactory::makeBool (location, false);

        case YdspPrimitiveType::int32Type:
        case YdspPrimitiveType::int64Type:
            return YdspExprFactory::makeInt (location, 0);

        default:
            return YdspExprFactory::makeFloat (location, 0.0);
    }
}

YdspExprPtr makeAddChain (YdspLocation location, std::vector<YdspExprPtr> terms)
{
    jassert (! terms.empty());

    auto acc = std::move (terms[0]);

    for (size_t i = 1; i < terms.size(); ++i)
        acc = YdspExprFactory::makeBinary (location, YdspOperator::add, std::move (acc), std::move (terms[i]));

    return acc;
}

//==============================================================================
struct FusionMember
{
    int nodeIndex = -1;
    const YdspAnalyzedProcessor* analyzed = nullptr;
    const YdspProcessorDecl* decl = nullptr;
    YdspRenameMap renames;
    String prefix;
};

bool buildRenameMap (FusionMember& member,
                     const std::vector<String>& inputNames,
                     const std::vector<String>& outputNames)
{
    if (inputNames.size() != member.analyzed->inputStreams.size()
        || outputNames.size() != member.analyzed->outputStreams.size())
        return false;

    bool ok = true;

    const auto add = [&member, &ok] (const String& from, const String& to)
    {
        if (from.isEmpty())
            return;

        const auto existing = member.renames.find (from);

        if (existing != member.renames.end())
        {
            if (existing->second != to)
                ok = false;

            return;
        }

        member.renames[from] = to;
    };

    for (size_t s = 0; s < member.analyzed->inputStreams.size(); ++s)
        add (member.analyzed->inputStreams[s]->name, inputNames[s]);

    for (size_t s = 0; s < member.analyzed->outputStreams.size(); ++s)
        add (member.analyzed->outputStreams[s]->name, outputNames[s]);

    for (const auto* param : member.analyzed->inputValues)
        add (param->name, member.prefix + "v_" + param->name);

    for (const auto* meter : member.analyzed->outputValues)
        add (meter->name, member.prefix + "m_" + meter->name);

    for (const auto* state : member.analyzed->states)
        add (state->name, member.prefix + "s_" + state->name);

    for (const auto& function : member.decl->functions)
        add (function.name, member.prefix + "f_" + function.name);

    for (const auto& structDecl : member.decl->structs)
        add (structDecl.name, member.prefix + "t_" + structDecl.name);

    std::vector<String> locals;

    if (member.decl->process != nullptr)
        for (const auto& statement : member.decl->process->body)
            collectDeclaredNames (*statement, locals);

    if (member.decl->init != nullptr)
        for (const auto& statement : member.decl->init->body)
            collectDeclaredNames (*statement, locals);

    for (const auto& function : member.decl->functions)
        for (const auto& statement : function.body)
            collectDeclaredNames (*statement, locals);

    for (const auto& local : locals)
        add (local, member.prefix + "l_" + local);

    return ok;
}

YdspPrimitiveType findStreamType (const YdspAnalyzedGraph& graph,
                                  const YdspAnalyzedProgram& program,
                                  int nodeIndex,
                                  int streamIndex,
                                  bool isSource)
{
    if (nodeIndex < 0)
    {
        const auto& list = isSource ? graph.inputStreams : graph.outputStreams;

        if (streamIndex >= 0 && static_cast<size_t> (streamIndex) < list.size())
            return list[static_cast<size_t> (streamIndex)]->type;

        return YdspPrimitiveType::float32Type;
    }

    const auto& node = graph.nodes[static_cast<size_t> (nodeIndex)];

    if (const auto* analyzed = program.findProcessor (node.processor))
    {
        const auto& list = isSource ? analyzed->outputStreams : analyzed->inputStreams;

        if (streamIndex >= 0 && static_cast<size_t> (streamIndex) < list.size())
            return list[static_cast<size_t> (streamIndex)]->type;
    }

    int count = 0;

    for (const auto& endpoint : node.endpoints())
    {
        const bool matches = isSource ? (endpoint.kind == YdspEndpointKind::outputStream)
                                      : (endpoint.kind == YdspEndpointKind::inputStream);

        if (matches)
        {
            if (count == streamIndex)
                return endpoint.type;

            ++count;
        }
    }

    return YdspPrimitiveType::float32Type;
}

} // namespace

//==============================================================================

void YdspSemanticAnalyzer::fuseNodeChains (YdspAnalyzedProgram& program)
{
    auto& graph = program.graph;

    const auto initialNodeCount = static_cast<int> (graph.nodes.size());

    if (initialNodeCount < 2 || graph.topoOrder.size() != static_cast<size_t> (initialNodeCount))
        return;

    const auto isNodeFusable = [&program, &graph] (int index)
    {
        const auto& node = graph.nodes[static_cast<size_t> (index)];

        if (node.processor == nullptr || node.subgraphIndex >= 0 || node.isEventDriven)
            return false;

        if (node.rateMultiplier != 1 || node.rateDivider != 1)
            return false;

        if (node.voiceGroupIndex < 0 && node.voiceCount != 1)
            return false;

        const auto* analyzed = program.findProcessor (node.processor);

        if (analyzed == nullptr || analyzed->mode != YdspProcessMode::sample)
            return false;

        if (! analyzed->inputEvents.empty() || ! analyzed->outputEvents.empty())
            return false;

        if (! analyzed->eventHandlers.empty() || analyzed->activityState != nullptr)
            return false;

        return true;
    };

    const auto canFusePair = [&graph, &isNodeFusable] (int a, int b)
    {
        if (! isNodeFusable (a) || ! isNodeFusable (b))
            return false;

        const auto& nodeA = graph.nodes[static_cast<size_t> (a)];
        const auto& nodeB = graph.nodes[static_cast<size_t> (b)];

        return nodeA.voiceGroupIndex == nodeB.voiceGroupIndex
            && nodeA.voiceCount == nodeB.voiceCount
            && nodeA.voiceMode == nodeB.voiceMode
            && nodeA.stealing == nodeB.stealing
            && nodeA.monoPriority == nodeB.monoPriority;
    };

    // ---- Pre-fusion artifact latency & topological position ----
    std::vector<int> topoPos (static_cast<size_t> (initialNodeCount), 0);

    for (size_t i = 0; i < graph.topoOrder.size(); ++i)
        topoPos[static_cast<size_t> (graph.topoOrder[i])] = static_cast<int> (i);

    std::vector<int> nodeArtifactIn (static_cast<size_t> (initialNodeCount), 0);
    std::vector<int> nodeArtifactOut (static_cast<size_t> (initialNodeCount), 0);

    for (const int index : graph.topoOrder)
    {
        int latest = 0;

        for (const auto& edge : graph.edges)
            if (edge.dstNode == index && edge.srcNode >= 0)
                latest = std::max (latest, nodeArtifactOut[static_cast<size_t> (edge.srcNode)]);

        nodeArtifactIn[static_cast<size_t> (index)] = latest;
        nodeArtifactOut[static_cast<size_t> (index)] = latest + graph.nodes[static_cast<size_t> (index)].latencySamples;
    }

    // ---- Quotient adjacency for convexity checks ----
    std::vector<std::vector<int>> outNodes (static_cast<size_t> (initialNodeCount));

    for (const auto& edge : graph.edges)
        if (edge.srcNode >= 0 && edge.dstNode >= 0)
            outNodes[static_cast<size_t> (edge.srcNode)].push_back (edge.dstNode);

    for (const auto& edge : graph.eventEdges)
        if (edge.srcNode >= 0 && edge.dstNode >= 0)
            outNodes[static_cast<size_t> (edge.srcNode)].push_back (edge.dstNode);

    // ---- Connected convex DAG component discovery ----
    std::vector<int> compOf (static_cast<size_t> (initialNodeCount));
    std::vector<std::vector<int>> compMembers (static_cast<size_t> (initialNodeCount));

    for (int i = 0; i < initialNodeCount; ++i)
    {
        compOf[static_cast<size_t> (i)] = i;
        compMembers[static_cast<size_t> (i)].push_back (i);
    }

    const auto hasPathViaOther = [&] (int fromComp, int toComp)
    {
        std::vector<bool> visited (static_cast<size_t> (initialNodeCount), false);
        std::vector<int> queue;

        visited[static_cast<size_t> (fromComp)] = true;

        for (const int u : compMembers[static_cast<size_t> (fromComp)])
        {
            for (const int v : outNodes[static_cast<size_t> (u)])
            {
                const int nextComp = compOf[static_cast<size_t> (v)];

                if (nextComp == fromComp || nextComp == toComp || visited[static_cast<size_t> (nextComp)])
                    continue;

                visited[static_cast<size_t> (nextComp)] = true;
                queue.push_back (nextComp);
            }
        }

        for (size_t q = 0; q < queue.size(); ++q)
        {
            const int c = queue[q];

            for (const int u : compMembers[static_cast<size_t> (c)])
            {
                for (const int v : outNodes[static_cast<size_t> (u)])
                {
                    const int nextComp = compOf[static_cast<size_t> (v)];

                    if (nextComp == toComp)
                        return true;

                    if (! visited[static_cast<size_t> (nextComp)])
                    {
                        visited[static_cast<size_t> (nextComp)] = true;
                        queue.push_back (nextComp);
                    }
                }
            }
        }

        return false;
    };

    const auto tryMerge = [&] (int nodeA, int nodeB)
    {
        const int cA = compOf[static_cast<size_t> (nodeA)];
        const int cB = compOf[static_cast<size_t> (nodeB)];

        if (cA == cB)
            return false;

        if (! canFusePair (compMembers[static_cast<size_t> (cA)].front(),
                           compMembers[static_cast<size_t> (cB)].front()))
            return false;

        const auto inMerged = [&compOf, cA, cB] (int node)
        {
            return node >= 0 && (compOf[static_cast<size_t> (node)] == cA || compOf[static_cast<size_t> (node)] == cB);
        };

        // Delayed audio edges and event edges cannot become internal to a component.
        for (const auto& edge : graph.edges)
        {
            if (edge.srcNode < 0 || edge.dstNode < 0)
                continue;

            const int sc = compOf[static_cast<size_t> (edge.srcNode)];
            const int dc = compOf[static_cast<size_t> (edge.dstNode)];

            if (((sc == cA && dc == cB) || (sc == cB && dc == cA)) && edge.delaySamples != 0)
                return false;
        }

        for (const auto& edge : graph.eventEdges)
        {
            if (edge.srcNode < 0 || edge.dstNode < 0)
                continue;

            const int sc = compOf[static_cast<size_t> (edge.srcNode)];
            const int dc = compOf[static_cast<size_t> (edge.dstNode)];

            if ((sc == cA && dc == cB) || (sc == cB && dc == cA))
                return false;
        }

        // Preserve convexity so contracting the component cannot introduce a cycle.
        if (hasPathViaOther (cA, cB) || hasPathViaOther (cB, cA))
            return false;

        // Preserve exact artifact latency alignment across internal and boundary edges.
        int fusedIn = 0;

        for (const auto& edge : graph.edges)
            if (! inMerged (edge.srcNode) && inMerged (edge.dstNode) && edge.srcNode >= 0)
                fusedIn = std::max (fusedIn, nodeArtifactOut[static_cast<size_t> (edge.srcNode)]);

        int commonOut = -1;

        for (const auto& edge : graph.edges)
        {
            const bool srcIn = inMerged (edge.srcNode);
            const bool dstIn = inMerged (edge.dstNode);

            if (srcIn && dstIn)
            {
                if (nodeArtifactOut[static_cast<size_t> (edge.srcNode)] != nodeArtifactIn[static_cast<size_t> (edge.dstNode)])
                    return false;
            }
            else if (! srcIn && dstIn)
            {
                if (nodeArtifactIn[static_cast<size_t> (edge.dstNode)] != fusedIn)
                    return false;
            }
            else if (srcIn && ! dstIn)
            {
                const int outLat = nodeArtifactOut[static_cast<size_t> (edge.srcNode)];

                if (commonOut < 0)
                    commonOut = outLat;
                else if (commonOut != outLat)
                    return false;
            }
        }

        for (const int n : compMembers[static_cast<size_t> (cB)])
        {
            compOf[static_cast<size_t> (n)] = cA;
            compMembers[static_cast<size_t> (cA)].push_back (n);
        }

        compMembers[static_cast<size_t> (cB)].clear();
        return true;
    };

    // Group zero-delay fan-out siblings (sharing a source stream) and zero-delay
    // float fan-in siblings (sharing a sink stream) deterministically.
    struct PortPeers
    {
        std::pair<int, int> port;
        std::vector<int> peers;
    };

    std::vector<PortPeers> sharedSources;
    std::vector<PortPeers> sharedSinks;

    const auto appendPeer = [] (std::vector<PortPeers>& table, std::pair<int, int> port, int peer)
    {
        for (auto& entry : table)
        {
            if (entry.port == port)
            {
                if (std::find (entry.peers.begin(), entry.peers.end(), peer) == entry.peers.end())
                    entry.peers.push_back (peer);

                return;
            }
        }

        table.push_back ({ port, { peer } });
    };

    for (const auto& edge : graph.edges)
    {
        if (edge.delaySamples != 0)
            continue;

        if (edge.dstNode >= 0 && isNodeFusable (edge.dstNode))
            appendPeer (sharedSources, { edge.srcNode, edge.srcStream }, edge.dstNode);

        if (edge.srcNode >= 0 && isNodeFusable (edge.srcNode))
        {
            const auto dstType = findStreamType (graph, program, edge.dstNode, edge.dstStream, false);

            if (isFloatValueType (toValueType (dstType)))
                appendPeer (sharedSinks, { edge.dstNode, edge.dstStream }, edge.srcNode);
        }
    }

    bool merged = true;

    while (merged)
    {
        merged = false;

        for (const auto& edge : graph.edges)
            if (edge.srcNode >= 0 && edge.dstNode >= 0 && edge.delaySamples == 0)
                if (tryMerge (edge.srcNode, edge.dstNode))
                    merged = true;

        for (const auto& group : sharedSources)
            for (size_t i = 0; i < group.peers.size(); ++i)
                for (size_t j = i + 1; j < group.peers.size(); ++j)
                    if (compOf[static_cast<size_t> (group.peers[i])] != compOf[static_cast<size_t> (group.peers[j])])
                        if (tryMerge (group.peers[i], group.peers[j]))
                            merged = true;

        for (const auto& group : sharedSinks)
            for (size_t i = 0; i < group.peers.size(); ++i)
                for (size_t j = i + 1; j < group.peers.size(); ++j)
                    if (compOf[static_cast<size_t> (group.peers[i])] != compOf[static_cast<size_t> (group.peers[j])])
                        if (tryMerge (group.peers[i], group.peers[j]))
                            merged = true;
    }

    std::vector<std::vector<int>> components;

    for (int c = 0; c < initialNodeCount; ++c)
    {
        if (compMembers[static_cast<size_t> (c)].size() < 2)
            continue;

        auto membersList = compMembers[static_cast<size_t> (c)];

        std::sort (membersList.begin(), membersList.end(), [&topoPos] (int a, int b)
        {
            return topoPos[static_cast<size_t> (a)] < topoPos[static_cast<size_t> (b)];
        });

        components.push_back (std::move (membersList));
    }

    if (components.empty())
        return;

    std::sort (components.begin(), components.end(), [&topoPos] (const std::vector<int>& a, const std::vector<int>& b)
    {
        return topoPos[static_cast<size_t> (a.front())] < topoPos[static_cast<size_t> (b.front())];
    });

    std::vector<bool> dead (graph.nodes.size(), false);
    std::vector<const YdspProcessorDecl*> fusedAwayDecls;
    bool fusedAny = false;

    for (const auto& component : components)
    {
        const auto memberCount = component.size();

        std::unordered_map<int, int> memberIndexMap;
        memberIndexMap.reserve (memberCount);

        for (size_t i = 0; i < memberCount; ++i)
            memberIndexMap[component[i]] = static_cast<int> (i);

        const auto memberOf = [&memberIndexMap] (int nodeIndex) -> int
        {
            if (nodeIndex < 0)
                return -1;

            const auto it = memberIndexMap.find (nodeIndex);
            return it == memberIndexMap.end() ? -1 : it->second;
        };

        std::vector<FusionMember> members (memberCount);

        for (size_t i = 0; i < memberCount; ++i)
        {
            auto& member = members[i];
            member.nodeIndex = component[i];
            member.decl = graph.nodes[static_cast<size_t> (component[i])].processor;
            member.analyzed = program.findProcessor (member.decl);
            member.prefix = String ("_m") + String (static_cast<int> (i)) + "_";
        }

        // ---- Analyze external inputs, internal wires, and external outputs ----
        struct ExternalInput
        {
            int srcNode = -1;
            int srcStream = 0;
            int delaySamples = 0;
            YdspPrimitiveType type = YdspPrimitiveType::float32Type;
            String name;
        };

        struct InputTerm
        {
            bool isExternal = false;
            int externalInputIndex = -1;
            int srcMember = -1;
            int srcStream = 0;
        };

        struct SummedSink
        {
            int dstNode = -1;
            int dstStream = 0;
            YdspPrimitiveType type = YdspPrimitiveType::float32Type;
            std::vector<std::pair<int, int>> sources; // (srcMember, srcStream)
            int fusedOutputIndex = -1;
        };

        struct ExternalOutput
        {
            bool isSummed = false;
            int summedSinkIndex = -1;
            int srcMember = -1;
            int srcStream = 0;
            YdspPrimitiveType type = YdspPrimitiveType::float32Type;
            String name;
        };

        std::vector<ExternalInput> externalInputs;
        std::map<std::pair<int, int>, int> zeroDelayExtInputMap;

        std::vector<std::vector<std::vector<InputTerm>>> inputTerms (memberCount);
        std::vector<std::vector<bool>> outputUsedInternally (memberCount);
        std::vector<std::vector<bool>> outputUsedInSum (memberCount);
        std::vector<std::vector<int>> outputRawIndex (memberCount);
        std::vector<std::vector<String>> outputLocalName (memberCount);

        for (size_t m = 0; m < memberCount; ++m)
        {
            const auto inCount = members[m].analyzed->inputStreams.size();
            const auto outCount = members[m].analyzed->outputStreams.size();

            inputTerms[m].resize (inCount);
            outputUsedInternally[m].assign (outCount, false);
            outputUsedInSum[m].assign (outCount, false);
            outputRawIndex[m].assign (outCount, -1);
            outputLocalName[m].resize (outCount);
        }

        // Pre-scan outgoing edges to detect zero-delay float fan-in sinks.
        std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> outgoingBySink;

        for (const auto& edge : graph.edges)
        {
            const int sm = memberOf (edge.srcNode);
            const int dm = memberOf (edge.dstNode);

            if (sm >= 0 && dm < 0 && edge.delaySamples == 0)
            {
                const auto dstType = findStreamType (graph, program, edge.dstNode, edge.dstStream, false);

                if (isFloatValueType (toValueType (dstType)))
                    outgoingBySink[{ edge.dstNode, edge.dstStream }].emplace_back (sm, edge.srcStream);
            }
        }

        std::vector<SummedSink> summedSinks;
        std::map<std::pair<int, int>, int> summedSinkByDst;

        for (auto& [dst, srcs] : outgoingBySink)
        {
            if (srcs.size() < 2)
                continue;

            const int sinkIdx = static_cast<int> (summedSinks.size());
            SummedSink sink;
            sink.dstNode = dst.first;
            sink.dstStream = dst.second;
            sink.type = findStreamType (graph, program, dst.first, dst.second, false);
            sink.sources = srcs;

            for (const auto& [sm, ss] : srcs)
                outputUsedInSum[static_cast<size_t> (sm)][static_cast<size_t> (ss)] = true;

            summedSinks.push_back (std::move (sink));
            summedSinkByDst[dst] = sinkIdx;
        }

        std::vector<ExternalOutput> externalOutputs;
        std::vector<int> edgeExternalInput (graph.edges.size(), -1);
        std::vector<int> edgeExternalOutput (graph.edges.size(), -1);

        for (size_t eIdx = 0; eIdx < graph.edges.size(); ++eIdx)
        {
            const auto& edge = graph.edges[eIdx];
            const int sm = memberOf (edge.srcNode);
            const int dm = memberOf (edge.dstNode);

            if (sm >= 0 && dm >= 0)
            {
                InputTerm term;
                term.isExternal = false;
                term.srcMember = sm;
                term.srcStream = edge.srcStream;
                inputTerms[static_cast<size_t> (dm)][static_cast<size_t> (edge.dstStream)].push_back (term);
                outputUsedInternally[static_cast<size_t> (sm)][static_cast<size_t> (edge.srcStream)] = true;
            }
            else if (sm < 0 && dm >= 0)
            {
                int extInIdx = -1;

                if (edge.delaySamples == 0)
                {
                    const auto key = std::make_pair (edge.srcNode, edge.srcStream);
                    const auto it = zeroDelayExtInputMap.find (key);

                    if (it != zeroDelayExtInputMap.end())
                        extInIdx = it->second;
                }

                if (extInIdx < 0)
                {
                    extInIdx = static_cast<int> (externalInputs.size());
                    ExternalInput extIn;
                    extIn.srcNode = edge.srcNode;
                    extIn.srcStream = edge.srcStream;
                    extIn.delaySamples = edge.delaySamples;
                    extIn.type = members[static_cast<size_t> (dm)].analyzed->inputStreams[static_cast<size_t> (edge.dstStream)]->type;
                    externalInputs.push_back (std::move (extIn));

                    if (edge.delaySamples == 0)
                        zeroDelayExtInputMap[{ edge.srcNode, edge.srcStream }] = extInIdx;
                }

                edgeExternalInput[eIdx] = extInIdx;

                InputTerm term;
                term.isExternal = true;
                term.externalInputIndex = extInIdx;
                inputTerms[static_cast<size_t> (dm)][static_cast<size_t> (edge.dstStream)].push_back (term);
            }
            else if (sm >= 0 && dm < 0)
            {
                if (edge.delaySamples == 0)
                {
                    const auto sumIt = summedSinkByDst.find ({ edge.dstNode, edge.dstStream });

                    if (sumIt != summedSinkByDst.end())
                    {
                        auto& sink = summedSinks[static_cast<size_t> (sumIt->second)];

                        if (sink.fusedOutputIndex < 0)
                        {
                            sink.fusedOutputIndex = static_cast<int> (externalOutputs.size());
                            ExternalOutput extOut;
                            extOut.isSummed = true;
                            extOut.summedSinkIndex = sumIt->second;
                            extOut.type = sink.type;
                            externalOutputs.push_back (std::move (extOut));
                        }

                        edgeExternalOutput[eIdx] = sink.fusedOutputIndex;
                        continue;
                    }
                }

                int rawIdx = outputRawIndex[static_cast<size_t> (sm)][static_cast<size_t> (edge.srcStream)];

                if (rawIdx < 0)
                {
                    rawIdx = static_cast<int> (externalOutputs.size());
                    outputRawIndex[static_cast<size_t> (sm)][static_cast<size_t> (edge.srcStream)] = rawIdx;

                    ExternalOutput extOut;
                    extOut.isSummed = false;
                    extOut.srcMember = sm;
                    extOut.srcStream = edge.srcStream;
                    extOut.type = members[static_cast<size_t> (sm)].analyzed->outputStreams[static_cast<size_t> (edge.srcStream)]->type;
                    externalOutputs.push_back (std::move (extOut));
                }

                edgeExternalOutput[eIdx] = rawIdx;
            }
        }

        for (size_t i = 0; i < externalInputs.size(); ++i)
            externalInputs[i].name = externalInputs.size() == 1 ? String ("in") : String ("in") + String (static_cast<int> (i));

        for (size_t i = 0; i < externalOutputs.size(); ++i)
            externalOutputs[i].name = externalOutputs.size() == 1 ? String ("out") : String ("out") + String (static_cast<int> (i));

        // ---- Allocate local variables for internal wires, summed outputs, and fan-in inputs ----
        struct LocalDeclSpec
        {
            String name;
            YdspPrimitiveType type;
        };

        struct PendingInputSum
        {
            String targetLocal;
            std::vector<String> termNames;
        };

        std::vector<LocalDeclSpec> fuseLocals;
        std::vector<std::vector<PendingInputSum>> pendingInputSums (memberCount);
        int nextFuseLocal = 0;

        const auto allocFuseLocal = [&] (YdspPrimitiveType type) -> String
        {
            const auto name = String ("_fuse_") + String (nextFuseLocal++);
            fuseLocals.push_back ({ name, type });
            return name;
        };

        for (size_t m = 0; m < memberCount; ++m)
        {
            const auto outCount = members[m].analyzed->outputStreams.size();

            for (size_t s = 0; s < outCount; ++s)
            {
                const bool needsLocal = outputUsedInternally[m][s]
                                     || outputUsedInSum[m][s]
                                     || outputRawIndex[m][s] < 0;

                if (needsLocal)
                    outputLocalName[m][s] = allocFuseLocal (members[m].analyzed->outputStreams[s]->type);
            }
        }

        bool inputsValid = true;
        bool renamesOk = true;

        for (size_t m = 0; m < memberCount && inputsValid && renamesOk; ++m)
        {
            const auto inCount = members[m].analyzed->inputStreams.size();
            const auto outCount = members[m].analyzed->outputStreams.size();

            std::vector<String> inNames (inCount);

            for (size_t s = 0; s < inCount; ++s)
            {
                const auto& terms = inputTerms[m][s];

                if (terms.empty())
                {
                    inputsValid = false;
                    break;
                }

                const auto termName = [&] (const InputTerm& term)
                {
                    if (term.isExternal)
                        return externalInputs[static_cast<size_t> (term.externalInputIndex)].name;

                    return outputLocalName[static_cast<size_t> (term.srcMember)][static_cast<size_t> (term.srcStream)];
                };

                if (terms.size() == 1)
                {
                    inNames[s] = termName (terms[0]);
                }
                else
                {
                    const auto sumVar = allocFuseLocal (members[m].analyzed->inputStreams[s]->type);
                    inNames[s] = sumVar;

                    PendingInputSum sum;
                    sum.targetLocal = sumVar;

                    for (const auto& term : terms)
                        sum.termNames.push_back (termName (term));

                    pendingInputSums[m].push_back (std::move (sum));
                }
            }

            if (! inputsValid)
                break;

            std::vector<String> outNames (outCount);

            for (size_t s = 0; s < outCount; ++s)
            {
                if (! outputLocalName[m][s].isEmpty())
                    outNames[s] = outputLocalName[m][s];
                else
                    outNames[s] = externalOutputs[static_cast<size_t> (outputRawIndex[m][s])].name;
            }

            renamesOk = buildRenameMap (members[m], inNames, outNames);
        }

        if (! inputsValid || ! renamesOk)
            continue;

        // ---- Synthesize the fused processor ----
        auto fused = std::make_unique<YdspProcessorDecl>();

        String fusedName;

        for (size_t i = 0; i < memberCount; ++i)
        {
            if (i != 0)
                fusedName += "+";

            fusedName += graph.nodes[static_cast<size_t> (component[i])].instanceName;
        }

        fused->name = String ("fused(") + fusedName + ")";
        fused->location = members.front().decl->location;

        for (const auto& extIn : externalInputs)
        {
            YdspEndpointDecl endpoint;
            endpoint.kind = YdspEndpointKind::inputStream;
            endpoint.type = extIn.type;
            endpoint.name = extIn.name;
            endpoint.location = fused->location;
            fused->endpoints.push_back (std::move (endpoint));
        }

        for (const auto& extOut : externalOutputs)
        {
            YdspEndpointDecl endpoint;
            endpoint.kind = YdspEndpointKind::outputStream;
            endpoint.type = extOut.type;
            endpoint.name = extOut.name;
            endpoint.location = fused->location;
            fused->endpoints.push_back (std::move (endpoint));
        }

        std::vector<String> paramPublicNames;
        std::vector<YdspConstValue> paramDefaults;
        std::vector<std::pair<int, int>> paramOrigin; // (member, index within member)

        std::vector<String> meterPublicNames;
        std::vector<std::pair<int, int>> meterOrigin; // (member, index within member)

        for (size_t i = 0; i < memberCount; ++i)
        {
            const auto& member = members[i];
            const auto& node = graph.nodes[static_cast<size_t> (member.nodeIndex)];

            for (size_t p = 0; p < member.analyzed->inputValues.size(); ++p)
            {
                const auto* source = member.analyzed->inputValues[p];

                YdspEndpointDecl endpoint;
                endpoint.kind = YdspEndpointKind::inputParameter;
                endpoint.type = source->type;
                endpoint.name = ydspRenamedTo (member.renames, source->name);
                endpoint.location = source->location;
                endpoint.annotations = source->annotations;

                if (source->defaultValue != nullptr)
                    endpoint.defaultValue = ydspCloneExpr (*source->defaultValue, &member.renames);

                fused->endpoints.push_back (std::move (endpoint));

                paramPublicNames.push_back (node.instanceName + "." + source->name);
                paramDefaults.push_back (p < node.paramDefaults.size() ? node.paramDefaults[p] : YdspConstValue {});
                paramOrigin.emplace_back (static_cast<int> (i), static_cast<int> (p));
            }

            for (size_t m = 0; m < member.analyzed->outputValues.size(); ++m)
            {
                const auto* source = member.analyzed->outputValues[m];

                YdspEndpointDecl endpoint;
                endpoint.kind = YdspEndpointKind::outputParameter;
                endpoint.type = source->type;
                endpoint.name = ydspRenamedTo (member.renames, source->name);
                endpoint.location = source->location;
                endpoint.annotations = source->annotations;

                fused->endpoints.push_back (std::move (endpoint));

                meterPublicNames.push_back (node.instanceName + "." + source->name);
                meterOrigin.emplace_back (static_cast<int> (i), static_cast<int> (m));
            }
        }

        for (const auto& member : members)
        {
            for (const auto& structDecl : member.decl->structs)
            {
                YdspStructDecl copy;
                copy.name = ydspRenamedTo (member.renames, structDecl.name);
                copy.location = structDecl.location;
                copy.fields = structDecl.fields; // field names belong to the type
                fused->structs.push_back (std::move (copy));
            }

            for (const auto* state : member.analyzed->states)
            {
                YdspStateDecl copy;
                copy.type = state->type;
                copy.name = ydspRenamedTo (member.renames, state->name);
                copy.arraySize = state->arraySize;
                copy.arraySizeName = state->arraySizeName; // a program constant, not ours
                copy.structName = ydspRenamedTo (member.renames, state->structName);
                copy.location = state->location;
                copy.annotations = state->annotations;

                for (const auto& initialiser : state->initialisers)
                    copy.initialisers.push_back (initialiser != nullptr ? ydspCloneExpr (*initialiser, &member.renames)
                                                                        : YdspExprPtr {});

                fused->states.push_back (std::move (copy));
            }

            for (const auto& function : member.decl->functions)
            {
                auto bodyRenames = member.renames;

                for (const auto& param : function.params)
                    bodyRenames.erase (param.first);

                YdspFuncDecl copy;
                copy.name = ydspRenamedTo (member.renames, function.name);
                copy.location = function.location;
                copy.params = function.params;
                copy.returnType = function.returnType;
                copy.hasReturnType = function.hasReturnType;

                for (const auto& statement : function.body)
                    copy.body.push_back (ydspCloneStmt (*statement, &bodyRenames));

                fused->functions.push_back (std::move (copy));
            }
        }

        fused->process = std::make_unique<YdspProcessDecl>();
        fused->process->mode = YdspProcessMode::sample;
        fused->process->location = fused->location;

        for (const auto& localSpec : fuseLocals)
        {
            auto declaration = std::make_unique<YdspStmt>();
            declaration->kind = YdspStmtKind::localDecl;
            declaration->location = fused->location;
            declaration->name = localSpec.name;
            declaration->isLet = false;
            declaration->hasDeclType = true;
            declaration->declType = localSpec.type;
            declaration->value = makeZero (fused->location, localSpec.type);

            fused->process->body.push_back (std::move (declaration));
        }

        for (size_t m = 0; m < memberCount; ++m)
        {
            for (const auto& sum : pendingInputSums[m])
            {
                std::vector<YdspExprPtr> exprs;
                exprs.reserve (sum.termNames.size());

                for (const auto& termName : sum.termNames)
                    exprs.push_back (YdspExprFactory::makeIdentifier (fused->location, termName));

                auto assign = std::make_unique<YdspStmt>();
                assign->kind = YdspStmtKind::assign;
                assign->location = fused->location;
                assign->target = YdspExprFactory::makeIdentifier (fused->location, sum.targetLocal);
                assign->value = makeAddChain (fused->location, std::move (exprs));

                fused->process->body.push_back (std::move (assign));
            }

            if (members[m].decl->process != nullptr)
                for (const auto& statement : members[m].decl->process->body)
                    fused->process->body.push_back (ydspCloneStmt (*statement, &members[m].renames));
        }

        for (const auto& extOut : externalOutputs)
        {
            if (extOut.isSummed)
            {
                const auto& sink = summedSinks[static_cast<size_t> (extOut.summedSinkIndex)];
                std::vector<YdspExprPtr> exprs;
                exprs.reserve (sink.sources.size());

                for (const auto& [sm, ss] : sink.sources)
                    exprs.push_back (YdspExprFactory::makeIdentifier (fused->location,
                                                                      outputLocalName[static_cast<size_t> (sm)][static_cast<size_t> (ss)]));

                auto assign = std::make_unique<YdspStmt>();
                assign->kind = YdspStmtKind::assign;
                assign->location = fused->location;
                assign->target = YdspExprFactory::makeIdentifier (fused->location, extOut.name);
                assign->value = makeAddChain (fused->location, std::move (exprs));

                fused->process->body.push_back (std::move (assign));
            }
            else
            {
                const auto& localName = outputLocalName[static_cast<size_t> (extOut.srcMember)][static_cast<size_t> (extOut.srcStream)];

                if (localName.isEmpty())
                    continue; // wrote directly to the output stream

                auto assign = std::make_unique<YdspStmt>();
                assign->kind = YdspStmtKind::assign;
                assign->location = fused->location;
                assign->target = YdspExprFactory::makeIdentifier (fused->location, extOut.name);
                assign->value = YdspExprFactory::makeIdentifier (fused->location, localName);

                fused->process->body.push_back (std::move (assign));
            }
        }

        for (const auto& member : members)
        {
            if (member.decl->init == nullptr)
                continue;

            if (fused->init == nullptr)
            {
                fused->init = std::make_unique<YdspProcessDecl>();
                fused->init->mode = YdspProcessMode::sample;
                fused->init->location = fused->location;
            }

            for (const auto& statement : member.decl->init->body)
                fused->init->body.push_back (ydspCloneStmt (*statement, &member.renames));
        }

        // ---- Analyze it, and back out cleanly if it does not hold up ----
        const auto marker = diagnostics.mark();
        auto analyzedFused = analyzeProcessor (*fused);

        if (analyzedFused == nullptr || diagnostics.hasErrors())
        {
            jassertfalse;
            diagnostics.rollbackTo (marker);
            continue;
        }

        // ---- Splice the fused node in ----
        program.ast->synthesizedProcessors.push_back (std::move (fused));
        const auto* fusedDecl = program.ast->synthesizedProcessors.back().get();

        analyzedFused->decl = fusedDecl;
        program.processors.push_back (std::move (*analyzedFused));

        const auto fusedIndex = static_cast<int> (graph.nodes.size());
        const auto& headNode = graph.nodes[static_cast<size_t> (component.front())];

        int fusedInLatency = 0;
        int fusedOutLatency = -1;

        for (const auto& edge : graph.edges)
        {
            const int sm = memberOf (edge.srcNode);
            const int dm = memberOf (edge.dstNode);

            if (sm < 0 && dm >= 0 && edge.srcNode >= 0)
                fusedInLatency = std::max (fusedInLatency, nodeArtifactOut[static_cast<size_t> (edge.srcNode)]);

            if (sm >= 0 && dm < 0)
                fusedOutLatency = nodeArtifactOut[static_cast<size_t> (edge.srcNode)];
        }

        if (fusedOutLatency < 0)
            for (const int memberIndex : component)
                fusedOutLatency = std::max (fusedOutLatency, nodeArtifactOut[static_cast<size_t> (memberIndex)]);

        nodeArtifactIn.push_back (fusedInLatency);
        nodeArtifactOut.push_back (fusedOutLatency);

        YdspAnalyzedNode fusedNode;
        fusedNode.instanceName = fusedName;
        fusedNode.processor = fusedDecl;
        fusedNode.paramDefaults = std::move (paramDefaults);
        fusedNode.paramPublicNames = std::move (paramPublicNames);
        fusedNode.meterPublicNames = std::move (meterPublicNames);
        fusedNode.latencySamples = std::max (0, fusedOutLatency - fusedInLatency);
        fusedNode.voiceCount = headNode.voiceCount;
        fusedNode.voiceGroupIndex = headNode.voiceGroupIndex;
        fusedNode.voiceMode = headNode.voiceMode;
        fusedNode.stealing = headNode.stealing;
        fusedNode.monoPriority = headNode.monoPriority;

        graph.nodes.push_back (std::move (fusedNode));
        dead.push_back (false);

        std::vector<bool> emittedExternalInput (externalInputs.size(), false);
        std::vector<bool> emittedSummedOutput (summedSinks.size(), false);
        std::vector<YdspAnalyzedEdge> edges;

        for (size_t eIdx = 0; eIdx < graph.edges.size(); ++eIdx)
        {
            auto spliced = graph.edges[eIdx];
            const int sm = memberOf (spliced.srcNode);
            const int dm = memberOf (spliced.dstNode);

            if (sm >= 0 && dm >= 0)
                continue; // internal wire, now a local variable or sum

            if (sm < 0 && dm >= 0)
            {
                const int extInIdx = edgeExternalInput[eIdx];
                jassert (extInIdx >= 0);

                if (spliced.delaySamples == 0)
                {
                    if (emittedExternalInput[static_cast<size_t> (extInIdx)])
                        continue;

                    emittedExternalInput[static_cast<size_t> (extInIdx)] = true;
                }

                spliced.dstNode = fusedIndex;
                spliced.dstStream = extInIdx;
            }
            else if (sm >= 0 && dm < 0)
            {
                const int extOutIdx = edgeExternalOutput[eIdx];
                jassert (extOutIdx >= 0);

                const auto& extOut = externalOutputs[static_cast<size_t> (extOutIdx)];

                if (extOut.isSummed)
                {
                    if (emittedSummedOutput[static_cast<size_t> (extOut.summedSinkIndex)])
                        continue;

                    emittedSummedOutput[static_cast<size_t> (extOut.summedSinkIndex)] = true;
                }

                spliced.srcNode = fusedIndex;
                spliced.srcStream = extOutIdx;
            }

            edges.push_back (spliced);
        }

        graph.edges = std::move (edges);

        for (auto& valueEdge : graph.valueEdges)
        {
            for (size_t f = 0; f < paramOrigin.size(); ++f)
            {
                const auto originNode = members[static_cast<size_t> (paramOrigin[f].first)].nodeIndex;

                if (valueEdge.dstNode == originNode && valueEdge.dstParam == paramOrigin[f].second)
                {
                    valueEdge.dstNode = fusedIndex;
                    valueEdge.dstParam = static_cast<int> (f);
                    break;
                }
            }
        }

        for (auto& meterEdge : graph.meterEdges)
        {
            for (size_t f = 0; f < meterOrigin.size(); ++f)
            {
                const auto originNode = members[static_cast<size_t> (meterOrigin[f].first)].nodeIndex;

                if (meterEdge.srcNode == originNode && meterEdge.srcMeter == meterOrigin[f].second)
                {
                    meterEdge.srcNode = fusedIndex;
                    meterEdge.srcMeter = static_cast<int> (f);
                    break;
                }
            }
        }

        for (const auto memberIndex : component)
        {
            dead[static_cast<size_t> (memberIndex)] = true;
            fusedAwayDecls.push_back (graph.nodes[static_cast<size_t> (memberIndex)].processor);
        }

        fusedAny = true;
    }

    if (! fusedAny)
        return;

    for (const auto& valueEdge : graph.valueEdges)
        jassert (! dead[static_cast<size_t> (valueEdge.dstNode)]);

    for (const auto& meterEdge : graph.meterEdges)
        jassert (! dead[static_cast<size_t> (meterEdge.srcNode)]);

    for (const auto& eventEdge : graph.eventEdges)
    {
        if (eventEdge.srcNode >= 0)
            jassert (! dead[static_cast<size_t> (eventEdge.srcNode)]);

        if (eventEdge.dstNode >= 0)
            jassert (! dead[static_cast<size_t> (eventEdge.dstNode)]);
    }

    // ---- Compaction ----
    std::vector<int> remap (graph.nodes.size(), -1);
    int liveCount = 0;

    for (size_t i = 0; i < graph.nodes.size(); ++i)
    {
        if (dead[i])
            continue;

        remap[i] = liveCount;

        if (static_cast<int> (i) != liveCount)
            graph.nodes[static_cast<size_t> (liveCount)] = std::move (graph.nodes[i]);

        ++liveCount;
    }

    graph.nodes.resize (static_cast<size_t> (liveCount));

    const auto liveIndex = [&remap] (int node)
    {
        jassert (node >= 0 && remap[static_cast<size_t> (node)] >= 0);
        return remap[static_cast<size_t> (node)];
    };

    for (auto& edge : graph.edges)
    {
        if (edge.srcNode >= 0)
            edge.srcNode = liveIndex (edge.srcNode);

        if (edge.dstNode >= 0)
            edge.dstNode = liveIndex (edge.dstNode);
    }

    for (auto& valueEdge : graph.valueEdges)
        valueEdge.dstNode = liveIndex (valueEdge.dstNode);

    for (auto& meterEdge : graph.meterEdges)
        meterEdge.srcNode = liveIndex (meterEdge.srcNode);

    for (auto& eventEdge : graph.eventEdges)
    {
        if (eventEdge.srcNode >= 0)
            eventEdge.srcNode = liveIndex (eventEdge.srcNode);

        if (eventEdge.dstNode >= 0)
            eventEdge.dstNode = liveIndex (eventEdge.dstNode);
    }

    // ---- Drop the members' kernels ----
    const auto stillInstantiated = [&graph] (const YdspProcessorDecl* decl)
    {
        for (const auto& node : graph.nodes)
            if (node.processor == decl)
                return true;

        return false;
    };

    std::erase_if (program.processors, [&] (const YdspAnalyzedProcessor& processor)
    {
        return std::find (fusedAwayDecls.begin(), fusedAwayDecls.end(), processor.decl) != fusedAwayDecls.end()
            && ! stillInstantiated (processor.decl);
    });

    rebuildTopoOrder (graph, program.ast->graphs.front().location);
}

} // namespace yup
