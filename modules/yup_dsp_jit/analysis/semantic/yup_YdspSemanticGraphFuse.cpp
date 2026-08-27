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
using YdspRenameMap = std::unordered_map<String, String>;

String renamedTo (const YdspRenameMap& renames, const String& name)
{
    const auto entry = renames.find (name);
    return entry == renames.end() ? name : entry->second;
}

YdspExprPtr cloneExpr (const YdspExpr& source, const YdspRenameMap& renames);
YdspStmtPtr cloneStmt (const YdspStmt& source, const YdspRenameMap& renames);

YdspExprPtr cloneExpr (const YdspExpr& source, const YdspRenameMap& renames)
{
    auto copy = std::make_unique<YdspExpr>();

    copy->kind = source.kind;
    copy->location = source.location;
    copy->number = source.number;
    copy->flag = source.flag;
    copy->op = source.op;

    copy->text = source.kind == YdspExprKind::member ? source.text
                                                     : renamedTo (renames, source.text);

    for (const auto& child : source.children)
        if (child != nullptr)
            copy->children.push_back (cloneExpr (*child, renames));

    for (const auto& override : source.overrides)
        copy->overrides.emplace_back (override.first,
                                      override.second != nullptr ? cloneExpr (*override.second, renames)
                                                                 : YdspExprPtr {});

    return copy;
}

YdspStmtPtr cloneStmt (const YdspStmt& source, const YdspRenameMap& renames)
{
    auto copy = std::make_unique<YdspStmt>();

    copy->kind = source.kind;
    copy->location = source.location;
    copy->isLet = source.isLet;
    copy->hasDeclType = source.hasDeclType;
    copy->declType = source.declType;

    copy->name = renamedTo (renames, source.name);

    const auto expr = [&renames] (const YdspExprPtr& e)
    {
        return e != nullptr ? cloneExpr (*e, renames) : YdspExprPtr {};
    };

    const auto stmt = [&renames] (const YdspStmtPtr& s)
    {
        return s != nullptr ? cloneStmt (*s, renames) : YdspStmtPtr {};
    };

    copy->cond = expr (source.cond);
    copy->thenStmt = stmt (source.thenStmt);
    copy->elseStmt = stmt (source.elseStmt);
    copy->startExpr = expr (source.startExpr);
    copy->endExpr = expr (source.endExpr);
    copy->body = stmt (source.body);
    copy->target = expr (source.target);
    copy->value = expr (source.value);
    copy->returnExpr = expr (source.returnExpr);

    for (const auto& child : source.children)
        if (child != nullptr)
            copy->children.push_back (cloneStmt (*child, renames));

    return copy;
}

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

//==============================================================================
struct FusionMember
{
    int nodeIndex = -1;
    const YdspAnalyzedProcessor* analyzed = nullptr;
    const YdspProcessorDecl* decl = nullptr;
    YdspRenameMap renames;
    String prefix;
};

bool buildRenameMap (FusionMember& member, const String& inputName, const String& outputName)
{
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

    add (member.analyzed->inputStreams[0]->name, inputName);
    add (member.analyzed->outputStreams[0]->name, outputName);

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

} // namespace

//==============================================================================

void YdspSemanticAnalyzer::fuseNodeChains (YdspAnalyzedProgram& program)
{
    auto& graph = program.graph;

    const auto nodeCount = static_cast<int> (graph.nodes.size());

    if (nodeCount < 2)
        return;

    const auto isFusable = [&program, &graph] (int index)
    {
        const auto& node = graph.nodes[static_cast<size_t> (index)];

        if (node.processor == nullptr || node.subgraphIndex >= 0 || node.isEventDriven)
            return false;

        if (node.voiceCount != 1 || node.rateMultiplier != 1 || node.rateDivider != 1)
            return false;

        const auto* analyzed = program.findProcessor (node.processor);

        if (analyzed == nullptr || analyzed->mode != YdspProcessMode::sample)
            return false;

        if (analyzed->inputStreams.size() != 1 || analyzed->outputStreams.size() != 1)
            return false;

        if (! analyzed->inputEvents.empty())
            return false;

        if (! analyzed->outputEvents.empty())
            return false;

        if (! analyzed->eventHandlers.empty() || analyzed->activityState != nullptr)
            return false;

        return true;
    };

    std::vector<int> fanIn (static_cast<size_t> (nodeCount), 0);
    std::vector<int> fanOut (static_cast<size_t> (nodeCount), 0);

    for (const auto& edge : graph.edges)
    {
        if (edge.srcNode >= 0)
            ++fanOut[static_cast<size_t> (edge.srcNode)];

        if (edge.dstNode >= 0)
            ++fanIn[static_cast<size_t> (edge.dstNode)];
    }

    std::vector<int> nextInChain (static_cast<size_t> (nodeCount), -1);
    std::vector<int> prevInChain (static_cast<size_t> (nodeCount), -1);

    for (const auto& edge : graph.edges)
    {
        if (edge.srcNode < 0 || edge.dstNode < 0 || edge.delaySamples != 0)
            continue;

        if (fanOut[static_cast<size_t> (edge.srcNode)] != 1 || fanIn[static_cast<size_t> (edge.dstNode)] != 1)
            continue;

        if (! isFusable (edge.srcNode) || ! isFusable (edge.dstNode))
            continue;

        nextInChain[static_cast<size_t> (edge.srcNode)] = edge.dstNode;
        prevInChain[static_cast<size_t> (edge.dstNode)] = edge.srcNode;
    }

    std::vector<std::vector<int>> chains;

    for (int n = 0; n < nodeCount; ++n)
    {
        if (prevInChain[static_cast<size_t> (n)] != -1 || nextInChain[static_cast<size_t> (n)] == -1)
            continue;

        std::vector<int> chain;

        for (int m = n; m != -1; m = nextInChain[static_cast<size_t> (m)])
            chain.push_back (m);

        chains.push_back (std::move (chain));
    }

    if (chains.empty())
        return;

    std::vector<bool> dead (graph.nodes.size(), false);
    std::vector<const YdspProcessorDecl*> fusedAwayDecls;
    bool fusedAny = false;

    for (const auto& chain : chains)
    {
        const auto memberCount = chain.size();

        // ---- Members and their rename maps ----
        std::vector<FusionMember> members (memberCount);
        std::vector<String> junctions; // one local per internal junction

        for (size_t i = 0; i + 1 < memberCount; ++i)
            junctions.push_back (String ("_fuse") + String (static_cast<int> (i)));

        bool renamesOk = true;

        for (size_t i = 0; i < memberCount && renamesOk; ++i)
        {
            auto& member = members[i];

            member.nodeIndex = chain[i];
            member.decl = graph.nodes[static_cast<size_t> (chain[i])].processor;
            member.analyzed = program.findProcessor (member.decl);
            member.prefix = graph.nodes[static_cast<size_t> (chain[i])].instanceName.replaceCharacter ('.', '_') + "_";

            const auto inputName = i == 0 ? String ("in") : junctions[i - 1];
            const auto outputName = i + 1 == memberCount ? String ("out") : junctions[i];

            renamesOk = buildRenameMap (member, inputName, outputName);
        }

        if (! renamesOk)
            continue;

        // ---- Synthesize the fused processor ----
        auto fused = std::make_unique<YdspProcessorDecl>();

        String fusedName;

        for (size_t i = 0; i < memberCount; ++i)
        {
            if (i != 0)
                fusedName += "+";

            fusedName += graph.nodes[static_cast<size_t> (chain[i])].instanceName;
        }

        fused->name = String ("fused(") + fusedName + ")";
        fused->location = members.front().decl->location;

        const auto streamType = [] (const YdspEndpointDecl* endpoint)
        {
            return endpoint->type;
        };

        {
            YdspEndpointDecl endpoint;
            endpoint.kind = YdspEndpointKind::inputStream;
            endpoint.type = streamType (members.front().analyzed->inputStreams[0]);
            endpoint.name = "in";
            endpoint.location = fused->location;
            fused->endpoints.push_back (std::move (endpoint));
        }

        {
            YdspEndpointDecl endpoint;
            endpoint.kind = YdspEndpointKind::outputStream;
            endpoint.type = streamType (members.back().analyzed->outputStreams[0]);
            endpoint.name = "out";
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
                endpoint.kind = YdspEndpointKind::inputValue;
                endpoint.type = source->type;
                endpoint.name = renamedTo (member.renames, source->name);
                endpoint.location = source->location;
                endpoint.annotations = source->annotations;

                if (source->defaultValue != nullptr)
                    endpoint.defaultValue = cloneExpr (*source->defaultValue, member.renames);

                fused->endpoints.push_back (std::move (endpoint));

                paramPublicNames.push_back (node.instanceName + "." + source->name);
                paramDefaults.push_back (p < node.paramDefaults.size() ? node.paramDefaults[p] : YdspConstValue {});
                paramOrigin.emplace_back (static_cast<int> (i), static_cast<int> (p));
            }

            for (size_t m = 0; m < member.analyzed->outputValues.size(); ++m)
            {
                const auto* source = member.analyzed->outputValues[m];

                YdspEndpointDecl endpoint;
                endpoint.kind = YdspEndpointKind::outputValue;
                endpoint.type = source->type;
                endpoint.name = renamedTo (member.renames, source->name);
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
                copy.name = renamedTo (member.renames, structDecl.name);
                copy.location = structDecl.location;
                copy.fields = structDecl.fields; // field names belong to the type
                fused->structs.push_back (std::move (copy));
            }

            for (const auto* state : member.analyzed->states)
            {
                YdspStateDecl copy;
                copy.type = state->type;
                copy.name = renamedTo (member.renames, state->name);
                copy.arraySize = state->arraySize;
                copy.arraySizeName = state->arraySizeName; // a program constant, not ours
                copy.structName = renamedTo (member.renames, state->structName);
                copy.location = state->location;
                copy.annotations = state->annotations;

                for (const auto& initialiser : state->initialisers)
                    copy.initialisers.push_back (initialiser != nullptr ? cloneExpr (*initialiser, member.renames)
                                                                        : YdspExprPtr {});

                fused->states.push_back (std::move (copy));
            }

            for (const auto& function : member.decl->functions)
            {
                auto bodyRenames = member.renames;

                for (const auto& param : function.params)
                    bodyRenames.erase (param.first);

                YdspFuncDecl copy;
                copy.name = renamedTo (member.renames, function.name);
                copy.location = function.location;
                copy.params = function.params;
                copy.returnType = function.returnType;
                copy.hasReturnType = function.hasReturnType;

                for (const auto& statement : function.body)
                    copy.body.push_back (cloneStmt (*statement, bodyRenames));

                fused->functions.push_back (std::move (copy));
            }
        }

        fused->process = std::make_unique<YdspProcessDecl>();
        fused->process->mode = YdspProcessMode::sample;
        fused->process->location = fused->location;

        for (size_t i = 0; i < junctions.size(); ++i)
        {
            auto declaration = std::make_unique<YdspStmt>();
            declaration->kind = YdspStmtKind::localDecl;
            declaration->location = fused->location;
            declaration->name = junctions[i];
            declaration->isLet = false;
            declaration->hasDeclType = true;
            declaration->declType = streamType (members[i].analyzed->outputStreams[0]);
            declaration->value = makeZero (fused->location, declaration->declType);

            fused->process->body.push_back (std::move (declaration));
        }

        for (const auto& member : members)
            if (member.decl->process != nullptr)
                for (const auto& statement : member.decl->process->body)
                    fused->process->body.push_back (cloneStmt (*statement, member.renames));

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
                fused->init->body.push_back (cloneStmt (*statement, member.renames));
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

        YdspAnalyzedNode fusedNode;
        fusedNode.instanceName = fusedName;
        fusedNode.processor = fusedDecl;
        fusedNode.paramDefaults = std::move (paramDefaults);
        fusedNode.paramPublicNames = std::move (paramPublicNames);
        fusedNode.meterPublicNames = std::move (meterPublicNames);

        for (const auto memberIndex : chain)
            fusedNode.latencySamples += graph.nodes[static_cast<size_t> (memberIndex)].latencySamples;

        graph.nodes.push_back (std::move (fusedNode));
        dead.push_back (false);

        const auto head = chain.front();
        const auto tail = chain.back();

        std::vector<YdspAnalyzedEdge> edges;

        for (const auto& edge : graph.edges)
        {
            auto spliced = edge;

            const bool srcInChain = spliced.srcNode >= 0 && std::find (chain.begin(), chain.end(), spliced.srcNode) != chain.end();
            const bool dstInChain = spliced.dstNode >= 0 && std::find (chain.begin(), chain.end(), spliced.dstNode) != chain.end();

            if (srcInChain && dstInChain)
                continue; // an internal junction, now a register

            if (srcInChain)
            {
                if (spliced.srcNode != tail)
                    continue;

                spliced.srcNode = fusedIndex;
                spliced.srcStream = 0;
            }

            if (dstInChain)
            {
                if (spliced.dstNode != head)
                    continue;

                spliced.dstNode = fusedIndex;
                spliced.dstStream = 0;
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

        for (const auto memberIndex : chain)
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
