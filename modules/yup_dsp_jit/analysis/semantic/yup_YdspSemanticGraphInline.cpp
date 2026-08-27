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
// Subgraph selection and ordering

int YdspSemanticAnalyzer::findGraphIndex (const YdspProgram& program, const String& name)
{
    // A processor wins a name clash, so that adding a graph can never silently
    // repoint an existing `node x = Foo;` at something else.
    for (const auto& processor : program.processors)
        if (processor.name == name)
            return -1;

    for (size_t i = 0; i < program.graphs.size(); ++i)
        if (program.graphs[i].name == name)
            return static_cast<int> (i);

    return -1;
}

int YdspSemanticAnalyzer::selectMainGraph (const YdspProgram& program)
{
    // Only a graph declared in this program can be its entry point; an
    // imported one keeps whatever [[ main ]] it carries for its own file.
    std::vector<int> candidates;
    std::vector<int> marked;

    for (size_t i = 0; i < program.graphs.size(); ++i)
    {
        const auto& graph = program.graphs[i];

        for (const auto& [key, value] : graph.annotations)
            if (key != "main")
                error (graph.location, "Unknown graph annotation '" + key + "' on graph '" + graph.name + "' (expected 'main')");

        if (graph.isImported)
            continue;

        candidates.push_back (static_cast<int> (i));

        for (const auto& [key, value] : graph.annotations)
            if (key == "main")
                marked.push_back (static_cast<int> (i));
    }

    const auto names = [&program] (const std::vector<int>& indices)
    {
        StringArray list;

        for (const int i : indices)
            list.add ("'" + program.graphs[static_cast<size_t> (i)].name + "'");

        return list.joinIntoString (", ");
    };

    if (marked.size() > 1)
    {
        error (program.graphs[static_cast<size_t> (marked[1])].location,
               "Only one graph can be annotated [[ main ]], but " + names (marked) + " all are");
        return -1;
    }

    if (marked.size() == 1)
        return marked[0];

    if (candidates.size() == 1)
        return candidates.front();

    if (candidates.empty())
    {
        diagnostics.addError (0, 0, "The program must define at least one graph of its own");
        return -1;
    }

    error (program.graphs[static_cast<size_t> (candidates.front())].location,
           "The program defines several graphs (" + names (candidates) + ") and none is annotated [[ main ]]: mark the entry point with 'graph Name [[ main ]] { ... }'");
    return -1;
}

bool YdspSemanticAnalyzer::orderGraphsByDependency (const YdspProgram& program, int mainIndex, std::vector<int>& order)
{
    enum class Mark
    {
        unvisited,
        onStack,
        done
    };

    std::vector<Mark> marks (program.graphs.size(), Mark::unvisited);
    std::vector<int> stack;
    bool acyclic = true;

    std::function<void (int)> visit = [&] (int index)
    {
        marks[static_cast<size_t> (index)] = Mark::onStack;
        stack.push_back (index);

        for (const auto& node : program.graphs[static_cast<size_t> (index)].nodes)
        {
            const int target = findGraphIndex (program, node.processorName);

            if (target < 0)
                continue;

            if (marks[static_cast<size_t> (target)] == Mark::onStack)
            {
                StringArray loop;

                for (size_t i = 0; i < stack.size(); ++i)
                    if (! loop.isEmpty() || stack[i] == target)
                        loop.add (program.graphs[static_cast<size_t> (stack[i])].name);

                loop.add (program.graphs[static_cast<size_t> (target)].name);

                error (node.location, "Graph '" + program.graphs[static_cast<size_t> (target)].name + "' instantiates itself: " + loop.joinIntoString (" -> "));
                acyclic = false;
                continue;
            }

            if (marks[static_cast<size_t> (target)] == Mark::unvisited)
                visit (target);
        }

        stack.pop_back();
        marks[static_cast<size_t> (index)] = Mark::done;
        order.push_back (index);
    };

    visit (mainIndex);

    return acyclic;
}

//==============================================================================
// Inlining

void YdspSemanticAnalyzer::inlineSubgraphs (YdspAnalyzedGraph& graph,
                                            const YdspGraphDecl& decl,
                                            const std::vector<YdspAnalyzedGraph>& analyzedGraphs)
{
    bool inlinedAny = false;

    // One end of a spliced wire, with the delay accumulated onto it so far.
    struct Endpoint
    {
        int node = -1;
        int stream = 0;
        int delay = 0;
    };

    std::vector<Endpoint> sources;
    std::vector<Endpoint> dests;

    // Every subgraph is already flat by the time it is spliced (graphs are
    // analyzed in dependency order), so no placeholder can appear among the
    // nodes this loop appends and a single forward sweep is enough.
    std::vector<bool> dead (graph.nodes.size(), false);

    for (size_t k = 0; k < graph.nodes.size(); ++k)
    {
        const int subgraphIndex = graph.nodes[k].subgraphIndex;

        if (subgraphIndex < 0)
            continue;

        const auto& sub = analyzedGraphs[static_cast<size_t> (subgraphIndex)];
        const auto placeholder = static_cast<int> (k);

        const int base = static_cast<int> (graph.nodes.size());
        const String prefix = graph.nodes[k].instanceName + ".";
        const auto placeholderParams = graph.nodes[k].paramDefaults;

        // ---- Nodes ----
        for (const auto& node : sub.nodes)
        {
            auto copy = node;
            copy.instanceName = prefix + node.instanceName;
            graph.nodes.push_back (std::move (copy));
            dead.push_back (false);
        }

        // ---- Streams ----
        // A boundary port may carry any number of edges on either side: the
        // parent may fan several sources into `sub.in`, or read `sub.out` from
        // several places, and the subgraph may do the same internally. So each
        // side is a *list* of parent edges, not one.
        std::vector<std::vector<int>> feeders (sub.inputStreams.size());
        std::vector<std::vector<int>> consumers (sub.outputStreams.size());

        for (size_t e = 0; e < graph.edges.size(); ++e)
        {
            const auto& edge = graph.edges[e];

            if (edge.dstNode == placeholder && static_cast<size_t> (edge.dstStream) < feeders.size())
                feeders[static_cast<size_t> (edge.dstStream)].push_back (static_cast<int> (e));

            if (edge.srcNode == placeholder && static_cast<size_t> (edge.srcStream) < consumers.size())
                consumers[static_cast<size_t> (edge.srcStream)].push_back (static_cast<int> (e));
        }

        std::vector<YdspAnalyzedEdge> splicedEdges;

        // One internal edge can stand for several real wires: the cross product
        // of the sources reaching its producer's boundary and the destinations
        // leaving its consumer's boundary. The nested loop is *required* - a
        // pass-through edge inside the subgraph (graph input straight to graph
        // output) touches both boundaries at once, which two independent `if`s
        // only ever handled for the 1x1 case.
        //
        // Each resulting edge carries the sum of the delays on the wire it
        // stands for: the internal edge's own, plus the parent edge it is
        // spliced onto at each end. Those parent delays must be added *per
        // resulting edge* - hoisting them out and adding once would give a 2x2
        // splice four wires all carrying the same wrong delay.
        for (const auto& edge : sub.edges)
        {
            sources.clear();
            dests.clear();

            if (edge.srcNode < 0)
            {
                for (const int e : feeders[static_cast<size_t> (edge.srcStream)])
                {
                    const auto& parent = graph.edges[static_cast<size_t> (e)];
                    sources.push_back ({ parent.srcNode, parent.srcStream, parent.delaySamples });
                }
            }
            else
            {
                sources.push_back ({ edge.srcNode + base, edge.srcStream, 0 });
            }

            if (edge.dstNode < 0)
            {
                for (const int e : consumers[static_cast<size_t> (edge.dstStream)])
                {
                    const auto& parent = graph.edges[static_cast<size_t> (e)];
                    dests.push_back ({ parent.dstNode, parent.dstStream, parent.delaySamples });
                }
            }
            else
            {
                dests.push_back ({ edge.dstNode + base, edge.dstStream, 0 });
            }

            // An unwired boundary port leaves its list empty and the edge
            // vanishes, as it did before. The parent's connectivity check has
            // already rejected that, so this only unwinds an already-reported
            // error.
            for (const auto& source : sources)
            {
                for (const auto& dest : dests)
                {
                    YdspAnalyzedEdge spliced;
                    spliced.srcNode = source.node;
                    spliced.srcStream = source.stream;
                    spliced.dstNode = dest.node;
                    spliced.dstStream = dest.stream;
                    spliced.delaySamples = edge.delaySamples + source.delay + dest.delay;
                    splicedEdges.push_back (spliced);
                }
            }
        }

        std::vector<YdspAnalyzedEdge> edges;

        for (const auto& edge : graph.edges)
            if (edge.srcNode != placeholder && edge.dstNode != placeholder)
                edges.push_back (edge);

        edges.insert (edges.end(), splicedEdges.begin(), splicedEdges.end());
        graph.edges = std::move (edges);

        // ---- Parameters ----
        // A subgraph parameter is authoritative for the node it drives: its
        // value - the placeholder's override, or the subgraph's own declared
        // default - replaces that node's default, and a parent alias onto the
        // placeholder is re-pointed at the node.
        std::vector<int> paramAlias (sub.inputValues.size(), -1);

        for (const auto& valueEdge : graph.valueEdges)
            if (valueEdge.dstNode == placeholder && static_cast<size_t> (valueEdge.dstParam) < paramAlias.size())
                paramAlias[static_cast<size_t> (valueEdge.dstParam)] = valueEdge.srcParam;

        std::vector<bool> paramUsed (paramAlias.size(), false);
        std::vector<YdspAnalyzedValueEdge> splicedValueEdges;

        for (const auto& valueEdge : sub.valueEdges)
        {
            if (valueEdge.srcParam < 0 || static_cast<size_t> (valueEdge.srcParam) >= paramAlias.size())
                continue;

            paramUsed[static_cast<size_t> (valueEdge.srcParam)] = true;

            const int target = valueEdge.dstNode + base;
            auto& defaults = graph.nodes[static_cast<size_t> (target)].paramDefaults;

            if (static_cast<size_t> (valueEdge.dstParam) < defaults.size())
                defaults[static_cast<size_t> (valueEdge.dstParam)] = placeholderParams[static_cast<size_t> (valueEdge.srcParam)];

            if (paramAlias[static_cast<size_t> (valueEdge.srcParam)] >= 0)
            {
                YdspAnalyzedValueEdge spliced;
                spliced.srcParam = paramAlias[static_cast<size_t> (valueEdge.srcParam)];
                spliced.dstNode = target;
                spliced.dstParam = valueEdge.dstParam;
                splicedValueEdges.push_back (spliced);
            }
        }

        for (size_t p = 0; p < paramUsed.size(); ++p)
        {
            if (paramUsed[p])
                continue;

            const auto* endpoint = sub.inputValues[p];

            diagnostics.addWarning (endpoint->location.line,
                                    endpoint->location.column,
                                    "Parameter '" + endpoint->name + "' of graph '" + graph.nodes[static_cast<size_t> (placeholder)].targetName()
                                        + "' drives no node inside it, so setting it on node '" + graph.nodes[static_cast<size_t> (placeholder)].instanceName + "' has no effect");
        }

        std::vector<YdspAnalyzedValueEdge> valueEdges;

        for (const auto& valueEdge : graph.valueEdges)
            if (valueEdge.dstNode != placeholder)
                valueEdges.push_back (valueEdge);

        valueEdges.insert (valueEdges.end(), splicedValueEdges.begin(), splicedValueEdges.end());
        graph.valueEdges = std::move (valueEdges);

        // ---- Meters ----
        std::vector<int> meterAlias (sub.outputValues.size(), -1);

        for (const auto& meterEdge : graph.meterEdges)
            if (meterEdge.srcNode == placeholder && static_cast<size_t> (meterEdge.srcMeter) < meterAlias.size())
                meterAlias[static_cast<size_t> (meterEdge.srcMeter)] = meterEdge.dstMeter;

        std::vector<YdspAnalyzedMeterEdge> meterEdges;

        for (const auto& meterEdge : graph.meterEdges)
            if (meterEdge.srcNode != placeholder)
                meterEdges.push_back (meterEdge);

        for (const auto& meterEdge : sub.meterEdges)
        {
            if (meterEdge.dstMeter < 0 || static_cast<size_t> (meterEdge.dstMeter) >= meterAlias.size())
                continue;

            if (meterAlias[static_cast<size_t> (meterEdge.dstMeter)] < 0)
                continue;

            YdspAnalyzedMeterEdge spliced;
            spliced.srcNode = meterEdge.srcNode + base;
            spliced.srcMeter = meterEdge.srcMeter;
            spliced.dstMeter = meterAlias[static_cast<size_t> (meterEdge.dstMeter)];
            meterEdges.push_back (spliced);
        }

        graph.meterEdges = std::move (meterEdges);

        // ---- Events ----
        // Mirrors the stream splice above: an internal event edge can touch
        // either boundary (the child's own `input event`, its own `output
        // event`, or neither), so each boundary side is spliced against every
        // parent wire reaching it, not just one. A wire touching both
        // boundaries at once (a pure passthrough) cannot occur - the analyzer
        // already rejects a graph input event wired directly to a graph
        // output event, in every graph, subgraphs included.
        struct EventEndpoint
        {
            int node = -1;
            int endpoint = 0;
            int compensationSamples = -1; // -1 = "use the internal edge's own value"
        };

        std::vector<std::vector<int>> eventFeeders (sub.inputEvents.size());
        std::vector<std::vector<int>> eventConsumers (sub.outputEvents.size());

        for (size_t e = 0; e < graph.eventEdges.size(); ++e)
        {
            const auto& edge = graph.eventEdges[e];

            if (edge.dstNode == placeholder && static_cast<size_t> (edge.dstEndpoint) < eventFeeders.size())
                eventFeeders[static_cast<size_t> (edge.dstEndpoint)].push_back (static_cast<int> (e));

            if (edge.srcNode == placeholder && static_cast<size_t> (edge.srcEndpoint) < eventConsumers.size())
                eventConsumers[static_cast<size_t> (edge.srcEndpoint)].push_back (static_cast<int> (e));
        }

        std::vector<YdspAnalyzedEventEdge> splicedEventEdges;
        std::vector<EventEndpoint> eventSources;
        std::vector<EventEndpoint> eventDests;

        for (const auto& edge : sub.eventEdges)
        {
            eventSources.clear();
            eventDests.clear();

            if (edge.srcNode < 0)
            {
                for (const int e : eventFeeders[static_cast<size_t> (edge.srcEndpoint)])
                {
                    const auto& parent = graph.eventEdges[static_cast<size_t> (e)];
                    eventSources.push_back ({ parent.srcNode, parent.srcEndpoint, parent.compensationSamples });
                }
            }
            else
            {
                eventSources.push_back ({ edge.srcNode + base, edge.srcEndpoint, -1 });
            }

            if (edge.dstNode < 0)
            {
                for (const int e : eventConsumers[static_cast<size_t> (edge.dstEndpoint)])
                {
                    const auto& parent = graph.eventEdges[static_cast<size_t> (e)];
                    eventDests.push_back ({ parent.dstNode, parent.dstEndpoint, parent.compensationSamples });
                }
            }
            else
            {
                eventDests.push_back ({ edge.dstNode + base, edge.dstEndpoint, -1 });
            }

            for (const auto& source : eventSources)
            {
                for (const auto& dest : eventDests)
                {
                    YdspAnalyzedEventEdge spliced;
                    spliced.srcNode = source.node;
                    spliced.srcEndpoint = source.endpoint;
                    spliced.dstNode = dest.node;
                    spliced.dstEndpoint = dest.endpoint;
                    spliced.compensationSamples = source.compensationSamples >= 0 ? source.compensationSamples
                                                 : dest.compensationSamples >= 0   ? dest.compensationSamples
                                                                                    : edge.compensationSamples;
                    splicedEventEdges.push_back (spliced);
                }
            }
        }

        std::vector<YdspAnalyzedEventEdge> eventEdges;

        for (const auto& edge : graph.eventEdges)
            if (edge.srcNode != placeholder && edge.dstNode != placeholder)
                eventEdges.push_back (edge);

        eventEdges.insert (eventEdges.end(), splicedEventEdges.begin(), splicedEventEdges.end());
        graph.eventEdges = std::move (eventEdges);

        dead[k] = true;
        inlinedAny = true;
    }

    if (! inlinedAny)
        return;

    // ---- Compaction ----
    // Placeholders are dropped in one pass at the end: erasing them as they are
    // spliced would shift every index above them mid-flight.
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

    // Every surviving reference must name a live node: an edge onto a
    // placeholder was either rewritten or removed when that placeholder was
    // spliced.
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

    // validateConnectivity is deliberately *not* re-run here. Splicing cannot
    // create a zero-use port: every port it touches had at least one edge on
    // each side before, and the cross product of two non-empty lists is
    // non-empty. Nor can it smuggle a non-float stream onto a summing
    // destination: the strict type match on every edge means the parent edge,
    // the boundary port and the internal edge form an equality chain, so a
    // spliced edge's type is the type the destination was already checked with.
    rebuildTopoOrder (graph, decl.location);
}

} // namespace yup
