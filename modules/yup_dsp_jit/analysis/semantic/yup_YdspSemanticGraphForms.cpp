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

namespace
{

const YdspEndpointDecl* findEndpoint (const YdspAnalyzedNode& node, const String& name)
{
    for (const auto& endpoint : node.endpoints())
        if (endpoint.name == name)
            return &endpoint;

    return nullptr;
}

} // namespace

void YdspSemanticAnalyzer::analyzeConnectionsForm (const YdspGraphDecl& decl, YdspAnalyzedGraph& graph, const std::unordered_map<String, int>& nodeIndexByName)
{
    for (const auto& connection : decl.connections)
    {
        const auto& sourcePath = connection.sourcePath;
        const auto& destPath = connection.destPath;

        // ---- Resolve source ----
        const YdspEndpointDecl* sourceEndpoint = nullptr;
        int sourceNode = -1;

        if (const auto dot = sourcePath.indexOfChar ('.'); dot >= 0)
        {
            const auto nodeName = sourcePath.substring (0, dot);
            const auto endpointName = sourcePath.substring (dot + 1);

            const auto it = nodeIndexByName.find (nodeName);

            if (it == nodeIndexByName.end())
            {
                error (connection.location, "Unknown node '" + nodeName + "'");
                continue;
            }

            sourceNode = it->second;
            sourceEndpoint = findEndpoint (graph.nodes[static_cast<size_t> (sourceNode)], endpointName);

            if (sourceEndpoint == nullptr)
            {
                error (connection.location, "Node '" + nodeName + "' has no endpoint '" + endpointName + "'");
                continue;
            }
        }
        else
        {
            for (const auto* endpoint : graph.inputStreams)
            {
                if (endpoint->name == sourcePath)
                {
                    sourceEndpoint = endpoint;
                    break;
                }
            }

            if (sourceEndpoint == nullptr)
            {
                for (const auto* endpoint : graph.inputValues)
                {
                    if (endpoint->name == sourcePath)
                    {
                        sourceEndpoint = endpoint;
                        break;
                    }
                }
            }

            if (sourceEndpoint == nullptr)
            {
                for (const auto* endpoint : graph.inputEvents)
                {
                    if (endpoint->name == sourcePath)
                    {
                        sourceEndpoint = endpoint;
                        break;
                    }
                }
            }

            if (sourceEndpoint == nullptr)
            {
                error (connection.location, "Unknown connection source '" + sourcePath + "'");
                continue;
            }
        }

        // ---- Resolve destination ----
        const YdspEndpointDecl* destEndpoint = nullptr;
        int destNode = -1;

        if (const auto dot = destPath.indexOfChar ('.'); dot >= 0)
        {
            const auto nodeName = destPath.substring (0, dot);
            const auto endpointName = destPath.substring (dot + 1);

            const auto it = nodeIndexByName.find (nodeName);

            if (it == nodeIndexByName.end())
            {
                error (connection.location, "Unknown node '" + nodeName + "'");
                continue;
            }

            destNode = it->second;
            destEndpoint = findEndpoint (graph.nodes[static_cast<size_t> (destNode)], endpointName);

            if (destEndpoint == nullptr)
            {
                error (connection.location, "Node '" + nodeName + "' has no endpoint '" + endpointName + "'");
                continue;
            }
        }
        else
        {
            for (const auto* endpoint : graph.outputStreams)
            {
                if (endpoint->name == destPath)
                {
                    destEndpoint = endpoint;
                    break;
                }
            }

            if (destEndpoint == nullptr)
            {
                for (const auto* endpoint : graph.outputValues)
                {
                    if (endpoint->name == destPath)
                    {
                        destEndpoint = endpoint;
                        break;
                    }
                }
            }

            if (destEndpoint == nullptr)
            {
                for (const auto* endpoint : graph.outputEvents)
                {
                    if (endpoint->name == destPath)
                    {
                        destEndpoint = endpoint;
                        break;
                    }
                }
            }

            if (destEndpoint == nullptr)
            {
                error (connection.location, "Unknown connection destination '" + destPath + "'");
                continue;
            }
        }

        // ---- Classify and append ----
        const auto sourceKind = sourceEndpoint->kind;
        const auto destKind = destEndpoint->kind;

        auto indexOf = [] (const std::vector<const YdspEndpointDecl*>& list, const YdspEndpointDecl* endpoint)
        {
            for (size_t i = 0; i < list.size(); ++i)
                if (list[i] == endpoint)
                    return static_cast<int> (i);

            return -1;
        };

        const bool sourceIsEvent = (sourceKind == YdspEndpointKind::outputEvent || sourceKind == YdspEndpointKind::inputEvent);
        const bool destIsEvent = (destKind == YdspEndpointKind::outputEvent || destKind == YdspEndpointKind::inputEvent);

        if (sourceIsEvent || destIsEvent)
        {
            const bool sourceIsNodeOutputEvent = (sourceNode >= 0 && sourceKind == YdspEndpointKind::outputEvent);
            const bool sourceIsGraphInputEvent = (sourceNode < 0 && sourceKind == YdspEndpointKind::inputEvent);

            if (! destIsEvent || (! sourceIsNodeOutputEvent && ! sourceIsGraphInputEvent))
            {
                error (connection.location, "Incompatible connection kinds: '" + sourcePath + "' -> '" + destPath + "'");
                continue;
            }

            if (sourceIsGraphInputEvent && destKind == YdspEndpointKind::outputEvent && destNode < 0)
            {
                error (connection.location, "A graph input event cannot be wired directly to a graph output event");
                continue;
            }

            if (connection.delaySamples > 0)
            {
                error (connection.location, "An inline delay is not supported on an event connection");
                continue;
            }

            int srcEndpoint = 0;

            if (sourceIsNodeOutputEvent)
            {
                for (const auto& endpoint : graph.nodes[static_cast<size_t> (sourceNode)].endpoints())
                {
                    if (endpoint.kind == YdspEndpointKind::outputEvent)
                    {
                        if (&endpoint == sourceEndpoint)
                            break;
                        ++srcEndpoint;
                    }
                }
            }
            else
            {
                srcEndpoint = indexOf (graph.inputEvents, sourceEndpoint);
            }

            if (destKind == YdspEndpointKind::inputEvent && destNode >= 0)
            {
                YdspAnalyzedEventEdge edge;
                edge.srcNode = sourceNode;
                edge.srcEndpoint = srcEndpoint;
                edge.dstNode = destNode;

                int count = 0;
                for (const auto& endpoint : graph.nodes[static_cast<size_t> (destNode)].endpoints())
                {
                    if (endpoint.kind == YdspEndpointKind::inputEvent)
                    {
                        if (&endpoint == destEndpoint)
                        {
                            edge.dstEndpoint = count;
                            break;
                        }
                        ++count;
                    }
                }

                graph.eventEdges.push_back (edge);
                continue;
            }

            if (destKind == YdspEndpointKind::outputEvent && destNode < 0)
            {
                YdspAnalyzedEventEdge edge;
                edge.srcNode = sourceNode;
                edge.srcEndpoint = srcEndpoint;
                edge.dstNode = -1;
                edge.dstEndpoint = indexOf (graph.outputEvents, destEndpoint);

                graph.eventEdges.push_back (edge);
                continue;
            }

            error (connection.location, "Incompatible connection kinds: '" + sourcePath + "' -> '" + destPath + "'");
            continue;
        }

        if (sourceEndpoint->type != destEndpoint->type)
        {
            error (connection.location,
                   "Cannot connect '" + sourcePath + "' (" + yup::toString (sourceEndpoint->type) + ") to '"
                       + destPath + "' (" + yup::toString (destEndpoint->type) + "): types must match (use an explicit conversion in the process body)");
            continue;
        }

        if (connection.delaySamples > 0 && sourceEndpoint->type != YdspPrimitiveType::float32Type)
        {
            error (connection.location,
                   "An inline delay is only supported on a float32 stream connection, but '" + sourcePath + "' is "
                       + yup::toString (sourceEndpoint->type));
            continue;
        }

        if (sourceKind == YdspEndpointKind::inputStream && sourceNode >= 0)
        {
            error (connection.location, "A node input stream cannot be used as a connection source");
            continue;
        }

        if (sourceKind == YdspEndpointKind::inputValue && sourceNode >= 0)
        {
            error (connection.location, "A node parameter cannot be used as a connection source (only graph parameters can)");
            continue;
        }

        if (sourceKind == YdspEndpointKind::outputValue && sourceNode < 0)
        {
            error (connection.location, "Only a node meter can be used as a meter connection source");
            continue;
        }

        if (destKind == YdspEndpointKind::outputStream && destNode >= 0)
        {
            error (connection.location, "A node output stream cannot be used as a connection destination");
            continue;
        }

        if (destKind == YdspEndpointKind::outputValue && destNode >= 0)
        {
            error (connection.location, "Only a graph meter can be a meter connection destination");
            continue;
        }

        if (destKind == YdspEndpointKind::inputValue && destNode < 0)
        {
            error (connection.location, "A node parameter must be the destination of a parameter connection");
            continue;
        }

        if (sourceKind == YdspEndpointKind::inputStream && destKind == YdspEndpointKind::inputStream)
        {
            YdspAnalyzedEdge edge;
            edge.srcNode = -1;
            edge.srcStream = indexOf (graph.inputStreams, sourceEndpoint);
            edge.dstNode = destNode;

            int count = 0;
            for (const auto& endpoint : graph.nodes[static_cast<size_t> (destNode)].endpoints())
            {
                if (endpoint.kind == YdspEndpointKind::inputStream)
                {
                    if (&endpoint == destEndpoint)
                    {
                        edge.dstStream = count;
                        break;
                    }
                    ++count;
                }
            }

            edge.delaySamples = connection.delaySamples;
            graph.edges.push_back (edge);
            continue;
        }

        if (sourceKind == YdspEndpointKind::outputStream && destKind == YdspEndpointKind::outputStream)
        {
            YdspAnalyzedEdge edge;
            edge.srcNode = sourceNode;
            edge.dstNode = -1;
            edge.dstStream = indexOf (graph.outputStreams, destEndpoint);

            int count = 0;
            for (const auto& endpoint : graph.nodes[static_cast<size_t> (sourceNode)].endpoints())
            {
                if (endpoint.kind == YdspEndpointKind::outputStream)
                {
                    if (&endpoint == sourceEndpoint)
                    {
                        edge.srcStream = count;
                        break;
                    }
                    ++count;
                }
            }

            edge.delaySamples = connection.delaySamples;
            graph.edges.push_back (edge);
            continue;
        }

        if (sourceKind == YdspEndpointKind::outputStream && destKind == YdspEndpointKind::inputStream)
        {
            YdspAnalyzedEdge edge;
            edge.srcNode = sourceNode;
            edge.dstNode = destNode;

            int count = 0;
            for (const auto& endpoint : graph.nodes[static_cast<size_t> (sourceNode)].endpoints())
            {
                if (endpoint.kind == YdspEndpointKind::outputStream)
                {
                    if (&endpoint == sourceEndpoint)
                    {
                        edge.srcStream = count;
                        break;
                    }
                    ++count;
                }
            }

            count = 0;
            for (const auto& endpoint : graph.nodes[static_cast<size_t> (destNode)].endpoints())
            {
                if (endpoint.kind == YdspEndpointKind::inputStream)
                {
                    if (&endpoint == destEndpoint)
                    {
                        edge.dstStream = count;
                        break;
                    }
                    ++count;
                }
            }

            edge.delaySamples = connection.delaySamples;
            graph.edges.push_back (edge);
            continue;
        }

        if (sourceKind == YdspEndpointKind::inputStream && destKind == YdspEndpointKind::outputStream)
        {
            YdspAnalyzedEdge edge;
            edge.srcNode = -1;
            edge.srcStream = indexOf (graph.inputStreams, sourceEndpoint);
            edge.dstNode = -1;
            edge.dstStream = indexOf (graph.outputStreams, destEndpoint);
            edge.delaySamples = connection.delaySamples;
            graph.edges.push_back (edge);
            continue;
        }

        if (sourceKind == YdspEndpointKind::inputValue && destKind == YdspEndpointKind::inputValue)
        {
            YdspAnalyzedValueEdge edge;
            edge.srcParam = indexOf (graph.inputValues, sourceEndpoint);
            edge.dstNode = destNode;

            int count = 0;
            for (const auto& endpoint : graph.nodes[static_cast<size_t> (destNode)].endpoints())
            {
                if (endpoint.kind == YdspEndpointKind::inputValue)
                {
                    if (&endpoint == destEndpoint)
                    {
                        edge.dstParam = count;
                        break;
                    }
                    ++count;
                }
            }

            graph.valueEdges.push_back (edge);
            continue;
        }

        if (sourceKind == YdspEndpointKind::outputValue && destKind == YdspEndpointKind::outputValue)
        {
            YdspAnalyzedMeterEdge edge;
            edge.srcNode = sourceNode;
            edge.dstMeter = indexOf (graph.outputValues, destEndpoint);

            int count = 0;
            for (const auto& endpoint : graph.nodes[static_cast<size_t> (sourceNode)].endpoints())
            {
                if (endpoint.kind == YdspEndpointKind::outputValue)
                {
                    if (&endpoint == sourceEndpoint)
                    {
                        edge.srcMeter = count;
                        break;
                    }
                    ++count;
                }
            }

            graph.meterEdges.push_back (edge);
            continue;
        }

        error (connection.location, "Incompatible connection kinds: '" + sourcePath + "' -> '" + destPath + "'");
    }

    // ---- Exactly-once validation ----
    validateConnectivity (decl, graph);
}

//==============================================================================

namespace
{

struct AlgebraTerminal
{
    enum class Kind
    {
        graphInput,
        graphOutput,
        nodeInput,
        nodeOutput
    };

    Kind kind = Kind::graphInput;
    int nodeIndex = -1;
    int streamIndex = -1;
};

struct AlgebraWire
{
    AlgebraTerminal source;
    AlgebraTerminal dest;
};

using AlgebraBundle = std::vector<AlgebraTerminal>;

struct AlgebraValue
{
    bool isIdentity = false;
    int inArity = -1;
    int outArity = -1;

    YdspLocation location;

    std::vector<AlgebraBundle> inPorts;
    std::vector<AlgebraBundle> outPorts;
    std::vector<AlgebraWire> wires;
};

AlgebraTerminal makeGraphInputTerminal (int index) { return { AlgebraTerminal::Kind::graphInput, -1, index }; }

AlgebraTerminal makeGraphOutputTerminal (int index) { return { AlgebraTerminal::Kind::graphOutput, -1, index }; }

AlgebraTerminal makeNodeInputTerminal (int n, int s) { return { AlgebraTerminal::Kind::nodeInput, n, s }; }

AlgebraTerminal makeNodeOutputTerminal (int n, int s) { return { AlgebraTerminal::Kind::nodeOutput, n, s }; }

} // namespace

void YdspSemanticAnalyzer::analyzeAlgebraForm (const YdspGraphDecl& decl,
                                               YdspAnalyzedGraph& graph,
                                               const std::unordered_map<String, int>& nodeIndexByName,
                                               const YdspAnalyzedProgram& program)
{
    if (decl.algebraRoot == nullptr)
    {
        error (decl.location, "The graph has no 'process =' expression");
        return;
    }

    std::unordered_map<String, int> graphInputByName;
    std::unordered_map<String, int> graphOutputByName;

    for (size_t i = 0; i < graph.inputStreams.size(); ++i)
        graphInputByName[graph.inputStreams[i]->name] = static_cast<int> (i);

    for (size_t i = 0; i < graph.outputStreams.size(); ++i)
        graphOutputByName[graph.outputStreams[i]->name] = static_cast<int> (i);

    // ---- Resolve one leaf ----
    auto resolveLeaf = [&] (const YdspExpr& expr) -> AlgebraValue
    {
        AlgebraValue value;
        value.location = expr.location;

        if (expr.text == "_")
        {
            value.isIdentity = true;
            return value;
        }

        // 1. declared node instance
        if (const auto it = nodeIndexByName.find (expr.text); it != nodeIndexByName.end())
        {
            const auto& node = graph.nodes[static_cast<size_t> (it->second)];

            int inCount = 0;
            int outCount = 0;

            for (const auto& endpoint : node.endpoints())
            {
                if (endpoint.kind == YdspEndpointKind::inputStream)
                    value.inPorts.push_back ({ makeNodeInputTerminal (it->second, inCount++) });

                if (endpoint.kind == YdspEndpointKind::outputStream)
                    value.outPorts.push_back ({ makeNodeOutputTerminal (it->second, outCount++) });
            }

            value.inArity = inCount;
            value.outArity = outCount;
            return value;
        }

        // 2. inline processor instance (each occurrence creates a new node)
        for (const auto& processor : program.processors)
        {
            if (processor.decl->name != expr.text)
                continue;

            for (const auto& [paramName, valueExpr] : expr.overrides)
            {
                bool found = false;

                for (const auto& endpoint : processor.decl->endpoints)
                {
                    if (endpoint.name != paramName)
                        continue;

                    found = true;

                    if (endpoint.kind != YdspEndpointKind::inputValue)
                        error (valueExpr->location, "'" + paramName + "' is not a parameter of processor '" + expr.text + "'");
                }

                if (! found)
                    error (expr.location, "Processor '" + expr.text + "' has no parameter '" + paramName + "'");
            }

            YdspAnalyzedNode node;
            node.instanceName = expr.text + "#" + String (static_cast<int> (graph.nodes.size()));
            node.processor = processor.decl;
            node.rateMultiplier = 1;
            node.rateDivider = 1;

            for (const auto* paramEndpoint : processor.inputValues)
            {
                const YdspExpr* defaultExpr = nullptr;

                for (const auto& [paramName, valueExpr] : expr.overrides)
                {
                    if (paramName == paramEndpoint->name)
                    {
                        defaultExpr = valueExpr.get();
                        break;
                    }
                }

                if (defaultExpr == nullptr)
                    defaultExpr = paramEndpoint->defaultValue.get();

                node.paramDefaults.push_back (constEvalDefault (defaultExpr, paramEndpoint->type));
            }

            const int nodeIndex = static_cast<int> (graph.nodes.size());
            graph.nodes.push_back (std::move (node));

            int inCount = 0;
            int outCount = 0;

            for (const auto& endpoint : processor.decl->endpoints)
            {
                if (endpoint.kind == YdspEndpointKind::inputStream)
                    value.inPorts.push_back ({ makeNodeInputTerminal (nodeIndex, inCount++) });

                if (endpoint.kind == YdspEndpointKind::outputStream)
                    value.outPorts.push_back ({ makeNodeOutputTerminal (nodeIndex, outCount++) });
            }

            value.inArity = inCount;
            value.outArity = outCount;
            return value;
        }

        // 3. graph input stream
        if (const auto it = graphInputByName.find (expr.text); it != graphInputByName.end())
        {
            value.outPorts.push_back ({ makeGraphInputTerminal (it->second) });
            value.inArity = 1;
            value.outArity = 1;
            return value;
        }

        // 4. graph output stream
        if (const auto it = graphOutputByName.find (expr.text); it != graphOutputByName.end())
        {
            value.inPorts.push_back ({ makeGraphOutputTerminal (it->second) });
            value.inArity = 1;
            value.outArity = 1;
            return value;
        }

        error (expr.location, "Unknown identifier in graph expression: '" + expr.text + "'");
        return value;
    };

    const auto operatorName = [] (YdspOperator op)
    {
        if (op == YdspOperator::split)
            return "<:";

        if (op == YdspOperator::merge)
            return ":>";

        return ":";
    };

    auto composeFanned = [&] (AlgebraValue& a, AlgebraValue& b, YdspOperator op) -> AlgebraValue
    {
        const auto name = String (operatorName (op));

        if (a.outArity == -1)
        {
            if (op == YdspOperator::split)
                a.inArity = a.outArity = 1;
            else
                a.inArity = a.outArity = (b.inArity != -1 ? b.inArity : 1);
        }

        if (b.inArity == -1)
        {
            if (op == YdspOperator::merge)
                b.inArity = b.outArity = 1;
            else
                b.inArity = b.outArity = (a.outArity != -1 ? a.outArity : 1);
        }

        if (op == YdspOperator::split)
        {
            const bool ok = a.outArity == 0 ? b.inArity == 0 : (b.inArity % a.outArity) == 0;

            if (! ok)
            {
                error (a.location,
                       "Arity mismatch in '<:' : the left side has " + String (a.outArity)
                           + " outputs, which does not divide the right side's " + String (b.inArity) + " inputs");
                return a;
            }
        }
        else if (op == YdspOperator::merge)
        {
            const bool ok = b.inArity == 0 ? a.outArity == 0 : (a.outArity % b.inArity) == 0;

            if (! ok)
            {
                error (a.location,
                       "Arity mismatch in ':>' : the right side has " + String (b.inArity)
                           + " inputs, which does not divide the left side's " + String (a.outArity) + " outputs");
                return a;
            }
        }
        else if (a.outArity != b.inArity)
        {
            error (a.location, "Arity mismatch in ':' : the left side has " + String (a.outArity) + " outputs but the right side has " + String (b.inArity) + " inputs");
            return a;
        }

        std::vector<std::pair<int, int>> pairs;

        if (op == YdspOperator::merge)
        {
            for (int i = 0; i < a.outArity; ++i)
                pairs.emplace_back (i, i % b.inArity);
        }
        else
        {
            for (int j = 0; j < b.inArity; ++j)
                pairs.emplace_back (j % a.outArity, j);
        }

        if (a.isIdentity && b.isIdentity)
        {
            AlgebraValue result;
            result.isIdentity = true;
            result.inArity = a.inArity;
            result.outArity = b.outArity;
            return result;
        }

        const auto requireOutPorts = [&]
        {
            if (static_cast<int> (a.outPorts.size()) >= a.outArity)
                return true;

            error (a.location, "The left side of '" + name + "' has no output to connect from: a graph output stream cannot feed another operand");
            return false;
        };

        const auto requireInPorts = [&]
        {
            if (static_cast<int> (b.inPorts.size()) >= b.inArity)
                return true;

            error (b.location, "The right side of '" + name + "' has no input to connect to: a graph input stream cannot be fed by another operand");
            return false;
        };

        AlgebraValue result;
        result.inArity = a.inArity;
        result.outArity = b.outArity;
        result.wires = std::move (a.wires);

        for (auto& wire : b.wires)
            result.wires.push_back (std::move (wire));

        if (a.isIdentity)
        {
            if (! requireInPorts())
                return b;

            result.inPorts.assign (static_cast<size_t> (a.inArity), AlgebraBundle {});
            result.outPorts = std::move (b.outPorts);

            for (const auto& [outIdx, inIdx] : pairs)
            {
                auto& target = result.inPorts[static_cast<size_t> (outIdx)];
                const auto& consumers = b.inPorts[static_cast<size_t> (inIdx)];
                target.insert (target.end(), consumers.begin(), consumers.end());
            }

            return result;
        }

        if (b.isIdentity)
        {
            if (! requireOutPorts())
                return a;

            result.inPorts = std::move (a.inPorts);
            result.outPorts.assign (static_cast<size_t> (b.outArity), AlgebraBundle {});

            for (const auto& [outIdx, inIdx] : pairs)
            {
                auto& target = result.outPorts[static_cast<size_t> (inIdx)];
                const auto& producers = a.outPorts[static_cast<size_t> (outIdx)];
                target.insert (target.end(), producers.begin(), producers.end());
            }

            return result;
        }

        if (! requireOutPorts())
            return a;

        if (! requireInPorts())
            return b;

        for (const auto& [outIdx, inIdx] : pairs)
        {
            for (const auto& source : a.outPorts[static_cast<size_t> (outIdx)])
            {
                for (const auto& dest : b.inPorts[static_cast<size_t> (inIdx)])
                {
                    AlgebraWire wire;
                    wire.source = source;
                    wire.dest = dest;
                    result.wires.push_back (wire);
                }
            }
        }

        result.inPorts = std::move (a.inPorts);
        result.outPorts = std::move (b.outPorts);
        return result;
    };

    // ---- Recursive descent over the algebra tree ----
    std::function<AlgebraValue (const YdspExpr&)> resolve;

    resolve = [&] (const YdspExpr& expr) -> AlgebraValue
    {
        if (expr.kind == YdspExprKind::graphLeaf)
            return resolveLeaf (expr);

        if (expr.kind != YdspExprKind::graphOp || expr.children.size() != 2)
        {
            error (expr.location, "Invalid graph expression");
            return {};
        }

        auto a = resolve (*expr.children[0]);
        auto b = resolve (*expr.children[1]);

        a.location = expr.children[0]->location;
        b.location = expr.children[1]->location;

        switch (expr.op)
        {
            case YdspOperator::par:
            {
                if (a.outArity == -1)
                    a.inArity = a.outArity = 1;
                if (b.outArity == -1)
                    b.inArity = b.outArity = 1;

                if (a.isIdentity != b.isIdentity)
                {
                    error (expr.location,
                           "'_' cannot be combined with another operand inside ',' in this version: the identity contributes no ports of its own, "
                           "so the result would claim an arity nothing backs. Route the pass-through with an explicit 'connection' block instead");

                    AlgebraValue result;
                    result.isIdentity = true;
                    result.inArity = a.inArity + b.inArity;
                    result.outArity = a.outArity + b.outArity;
                    return result;
                }

                AlgebraValue result;
                result.isIdentity = a.isIdentity && b.isIdentity;
                result.inArity = a.inArity + b.inArity;
                result.outArity = a.outArity + b.outArity;

                for (auto& port : a.inPorts)
                    result.inPorts.push_back (std::move (port));
                for (auto& port : b.inPorts)
                    result.inPorts.push_back (std::move (port));
                for (auto& port : a.outPorts)
                    result.outPorts.push_back (std::move (port));
                for (auto& port : b.outPorts)
                    result.outPorts.push_back (std::move (port));

                for (auto& wire : a.wires)
                    result.wires.push_back (std::move (wire));
                for (auto& wire : b.wires)
                    result.wires.push_back (std::move (wire));

                return result;
            }

            case YdspOperator::seq:
            case YdspOperator::split:
            case YdspOperator::merge:
                return composeFanned (a, b, expr.op);

            case YdspOperator::recurse:
            {
                if (b.inArity == -1)
                    b.inArity = b.outArity + 1;
                if (b.outArity == -1)
                    b.outArity = b.inArity - 1;

                if (b.inArity != b.outArity + 1)
                {
                    error (expr.location, "Arity mismatch in '~' : the right side must have exactly one more input than output");
                    return a;
                }

                error (expr.location, "The recursion operator '~' is parsed and validated but not yet supported (requires fused code generation, planned)");
                return a;
            }

            default:
                error (expr.location, "Invalid graph operator");
                return {};
        }
    };

    auto root = resolve (*decl.algebraRoot);

    if (root.isIdentity)
    {
        if (root.inArity == -1)
            root.inArity = root.outArity = 1;

        const int numInputs = static_cast<int> (graph.inputStreams.size());
        const int numOutputs = static_cast<int> (graph.outputStreams.size());

        if (root.inArity != numInputs || root.outArity != numOutputs)
        {
            error (decl.location, "The graph expression arity (" + String (root.inArity) + " in, " + String (root.outArity) + " out) does not match the graph inputs/outputs (" + String (numInputs) + " in, " + String (numOutputs) + " out)");
            return;
        }

        for (int i = 0; i < numInputs; ++i)
        {
            YdspAnalyzedEdge edge;
            edge.srcNode = -1;
            edge.srcStream = i;
            edge.dstNode = -1;
            edge.dstStream = i;
            graph.edges.push_back (edge);
        }

        validateConnectivity (decl, graph);
        return;
    }

    auto streamType = [&graph] (int node, int stream, bool isOutput) -> const YdspEndpointDecl*
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

    for (const auto& wire : root.wires)
    {
        YdspAnalyzedEdge edge;

        if (wire.source.kind == AlgebraTerminal::Kind::graphInput)
        {
            edge.srcNode = -1;
            edge.srcStream = wire.source.streamIndex;
        }
        else if (wire.source.kind == AlgebraTerminal::Kind::nodeOutput)
        {
            edge.srcNode = wire.source.nodeIndex;
            edge.srcStream = wire.source.streamIndex;
        }
        else
        {
            error (decl.location, "Unresolved wire source in the graph expression");
            continue;
        }

        if (wire.dest.kind == AlgebraTerminal::Kind::nodeInput)
        {
            edge.dstNode = wire.dest.nodeIndex;
            edge.dstStream = wire.dest.streamIndex;
        }
        else if (wire.dest.kind == AlgebraTerminal::Kind::graphOutput)
        {
            edge.dstNode = -1;
            edge.dstStream = wire.dest.streamIndex;
        }
        else
        {
            error (decl.location, "Unresolved wire destination in the graph expression");
            continue;
        }

        const auto* srcEndpoint = streamType (edge.srcNode, edge.srcStream, true);
        const auto* dstEndpoint = streamType (edge.dstNode, edge.dstStream, false);

        if (srcEndpoint != nullptr && dstEndpoint != nullptr && srcEndpoint->type != dstEndpoint->type)
        {
            error (decl.location,
                   "Cannot connect a '" + yup::toString (srcEndpoint->type) + "' stream to a '"
                       + yup::toString (dstEndpoint->type) + "' stream: types must match");
            continue;
        }

        graph.edges.push_back (edge);
    }

    validateConnectivity (decl, graph);
}

} // namespace yup
