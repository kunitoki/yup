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

std::unique_ptr<YdspAnalyzedProcessor> YdspSemanticAnalyzer::analyzeProcessor (const YdspProcessorDecl& decl)
{
    auto processor = std::make_unique<YdspAnalyzedProcessor>();
    processor->decl = &decl;

    for (const auto& [key, value] : decl.annotations)
    {
        if (key != "latency")
        {
            error (decl.location, "Unknown processor annotation '" + key + "' on processor '" + decl.name + "' (expected 'latency')");
            continue;
        }

        if (! value.containsOnly ("0123456789") || value.isEmpty())
        {
            error (decl.location, "The '[[ latency ]]' annotation on processor '" + decl.name + "' must be a non-negative integer number of samples, but is '" + value + "'");
            continue;
        }

        processor->declaredLatencySamples = value.getIntValue();
    }

    symbols.clear();
    localScopes.clear();
    structDecls.clear();
    procStates.clear();
    loopDepth = 0;
    hiddenStateCount = 0;
    isInitMode = false;
    isBlockMode = false;

    for (const auto& structDecl : decl.structs)
        structDecls[structDecl.name] = &structDecl;

    for (const auto& endpoint : decl.endpoints)
    {
        if (endpoint.kind == YdspEndpointKind::inputEvent)
        {
            bool duplicate = false;

            for (const auto* existing : processor->inputEvents)
                if (existing->name == endpoint.name)
                {
                    duplicate = true;
                    break;
                }

            if (duplicate)
                error (endpoint.location, "Duplicate event input '" + endpoint.name + "'");
            else
                processor->inputEvents.push_back (&endpoint);

            continue;
        }

        if (endpoint.kind == YdspEndpointKind::outputEvent)
        {
            bool duplicate = false;

            for (const auto* existing : processor->outputEvents)
                if (existing->name == endpoint.name)
                {
                    duplicate = true;
                    break;
                }

            if (duplicate)
                error (endpoint.location, "Duplicate event output '" + endpoint.name + "'");
            else
                processor->outputEvents.push_back (&endpoint);

            continue;
        }

        YdspSymbolInfo info;

        switch (endpoint.kind)
        {
            case YdspEndpointKind::inputStream:
                info.kind = YdspSymbolKind::inputStream;
                info.index = static_cast<int> (processor->inputStreams.size());
                break;

            case YdspEndpointKind::outputStream:
                info.kind = YdspSymbolKind::outputStream;
                info.index = static_cast<int> (processor->outputStreams.size());
                break;

            case YdspEndpointKind::inputValue:
                info.kind = YdspSymbolKind::inputValue;
                info.index = static_cast<int> (processor->inputValues.size());
                break;

            case YdspEndpointKind::outputValue:
                info.kind = YdspSymbolKind::outputValue;
                info.index = static_cast<int> (processor->outputValues.size());
                break;

            case YdspEndpointKind::inputEvent:
                break; // handled above

            case YdspEndpointKind::outputEvent:
                break; // handled above
        }

        info.type = toValueType (endpoint.type);

        if (! addSymbol (endpoint.name, info, endpoint.location))
            continue;

        switch (endpoint.kind)
        {
            case YdspEndpointKind::inputStream:
                processor->inputStreams.push_back (&endpoint);
                break;
            case YdspEndpointKind::outputStream:
                processor->outputStreams.push_back (&endpoint);
                break;
            case YdspEndpointKind::inputValue:
                processor->inputValues.push_back (&endpoint);
                break;
            case YdspEndpointKind::outputValue:
                processor->outputValues.push_back (&endpoint);
                break;
            case YdspEndpointKind::inputEvent:
                break; // handled above

            case YdspEndpointKind::outputEvent:
                break; // handled above
        }
    }

    for (const auto& state : decl.states)
    {
        if (! state.structName.isEmpty())
        {
            if (findStruct (state.structName) == nullptr)
            {
                error (state.location, "Unknown struct type '" + state.structName + "'");
                continue;
            }
        }

        YdspSymbolInfo info;
        info.kind = state.arraySize > 0 ? YdspSymbolKind::stateArray : YdspSymbolKind::stateScalar;
        info.type = toValueType (state.type);
        info.index = static_cast<int> (processor->states.size());
        info.arraySize = state.arraySize;
        info.isArray = state.arraySize > 0;

        if (! addSymbol (state.name, info, state.location))
            continue;

        processor->states.push_back (&state);
        procStates.push_back (&state);
    }

    resolveActivityState (decl, *processor);

    for (const auto& [name, type] : builtinConstants)
    {
        YdspSymbolInfo info;
        info.kind = YdspSymbolKind::builtinConstant;
        info.type = type;
        symbols[name] = info;
    }

    if (decl.process == nullptr)
    {
        error (decl.location, "Processor '" + decl.name + "' has no process body");
        return processor;
    }

    analyzeFunctionBodies (*processor);

    if (diagnostics.hasErrors())
        return processor;

    processor->mode = decl.process->mode;
    isBlockMode = (decl.process->mode == YdspProcessMode::block);

    for (auto& [name, info] : symbols)
    {
        if ((info.kind == YdspSymbolKind::inputStream || info.kind == YdspSymbolKind::outputStream) && isBlockMode)
        {
            info.isArray = true;
            info.arraySize = -1;
        }
    }

    for (const auto& statement : decl.process->body)
        analyzeStatement (*statement, *processor);

    if (decl.init != nullptr)
    {
        isInitMode = true;

        for (const auto& statement : decl.init->body)
            analyzeStatement (*statement, *processor);

        isInitMode = false;
    }

    processor->hiddenStateCount = hiddenStateCount;

    for (const auto& handlerDecl : decl.eventHandlers)
        analyzeEventHandler (handlerDecl, *processor);

    return processor;
}

//==============================================================================

void YdspSemanticAnalyzer::resolveActivityState (const YdspProcessorDecl& decl, YdspAnalyzedProcessor& proc)
{
    for (const auto& state : decl.states)
    {
        for (const auto& [key, value] : state.annotations)
        {
            if (key != "role")
            {
                error (state.location, "Unknown state annotation '" + key + "' (the only recognized state annotation is 'role')");
                continue;
            }

            if (value != "voiceActivity")
            {
                error (state.location, "Unknown state role '" + value + "' (expected 'voiceActivity')");
                continue;
            }

            if (decl.eventHandlers.empty())
            {
                error (state.location, "A 'voiceActivity' state requires processor '" + decl.name + "' to declare at least one event handler");
                continue;
            }

            if (state.arraySize != 0 || state.arraySizeName.isNotEmpty() || state.structName.isNotEmpty()
                || state.type != YdspPrimitiveType::int32Type)
            {
                error (state.location, "A 'voiceActivity' state must be a scalar 'int'");
                continue;
            }

            if (proc.activityState != nullptr)
            {
                error (state.location, "Processor '" + decl.name + "' declares more than one 'voiceActivity' state");
                continue;
            }

            proc.activityState = &state;
        }
    }
}

//==============================================================================

void YdspSemanticAnalyzer::analyzeEventHandler (const YdspEventHandlerDecl& decl, YdspAnalyzedProcessor& proc)
{
    const YdspEndpointDecl* endpoint = nullptr;

    for (const auto* candidate : proc.inputEvents)
        if (candidate->name == decl.endpointName)
        {
            endpoint = candidate;
            break;
        }

    if (endpoint == nullptr)
    {
        error (decl.location, "Event handler references unknown event endpoint '" + decl.endpointName + "'");
        return;
    }

    const auto* shapeDesc = findEventShape (decl.shapeName);

    if (shapeDesc == nullptr)
    {
        error (decl.location, "Unknown event shape '" + decl.shapeName + "' (expected one of " + eventShapeNameList() + ")");
        return;
    }

    for (const auto& existing : proc.eventHandlers)
        if (existing.decl->endpointName == decl.endpointName && existing.shape == shapeDesc->shape)
        {
            error (decl.location, "Duplicate event handler for shape '" + decl.shapeName + "' on input '" + decl.endpointName + "'");
            return;
        }

    YdspAnalyzedEventHandler analyzed;
    analyzed.decl = &decl;
    analyzed.shape = shapeDesc->shape;

    symbols.clear();
    localScopes.clear();
    procStates.clear();

    for (const auto* state : proc.states)
    {
        YdspSymbolInfo info;
        info.kind = state->arraySize > 0 ? YdspSymbolKind::stateArray : YdspSymbolKind::stateScalar;
        info.type = toValueType (state->type);
        info.index = static_cast<int> (procStates.size());
        info.arraySize = state->arraySize;
        info.isArray = state->arraySize > 0;

        symbols[state->name] = info;
        procStates.push_back (state);
    }

    for (const auto* inputValue : proc.inputValues)
    {
        YdspSymbolInfo info;
        info.kind = YdspSymbolKind::inputValue;
        info.type = toValueType (inputValue->type);
        info.index = -1;

        symbols[inputValue->name] = info;
    }

    for (const auto& [name, type] : builtinConstants)
    {
        if (StringRef (name) == StringRef ("blockSize"))
            continue; // a handler has no sample loop to measure

        YdspSymbolInfo info;
        info.kind = YdspSymbolKind::builtinConstant;
        info.type = type;
        symbols[name] = info;
    }

    const auto shadowIt = symbols.find (decl.paramName);

    if (shadowIt != symbols.end() && shadowIt->second.kind != YdspSymbolKind::builtinConstant)
    {
        error (decl.location, "Event parameter '" + decl.paramName + "' shadows an existing symbol in the event handler");
        return;
    }

    YdspSymbolInfo eventInfo;
    eventInfo.kind = YdspSymbolKind::eventParam;
    eventInfo.type = YdspValueType::float32Type;
    eventInfo.index = -1;
    symbols[decl.paramName] = eventInfo;

    const auto savedHiddenStateCount = hiddenStateCount;
    const auto savedLoopDepth = loopDepth;
    const auto savedIsBlockMode = isBlockMode;
    loopDepth = 0;
    isBlockMode = false;

    isEventHandlerMode = true;
    currentEventShape = shapeDesc;

    for (const auto& statement : decl.body)
        analyzeStatement (*statement, proc);

    currentEventShape = nullptr;
    isEventHandlerMode = false;

    hiddenStateCount = savedHiddenStateCount;
    loopDepth = savedLoopDepth;
    isBlockMode = savedIsBlockMode;

    proc.eventHandlers.push_back (std::move (analyzed));
}

//==============================================================================

const YdspStructDecl* YdspSemanticAnalyzer::findStruct (const String& name) const
{
    const auto it = structDecls.find (name);
    return it == structDecls.end() ? nullptr : it->second;
}

bool YdspSemanticAnalyzer::resolveStructField (const YdspExpr& base, const String& fieldName, const YdspStateDecl*& outState, const YdspStructField*& outField)
{
    outState = nullptr;
    outField = nullptr;

    const YdspExpr* nameExpr = &base;

    if (nameExpr->kind == YdspExprKind::index)
    {
        if (nameExpr->children.empty() || nameExpr->children[0]->kind != YdspExprKind::identifier)
            return false;

        nameExpr = nameExpr->children[0].get();
    }

    if (nameExpr->kind != YdspExprKind::identifier)
        return false;

    YdspSymbolInfo info;

    if (! findSymbol (nameExpr->text, info))
        return false;

    if (info.index < 0 || info.index >= static_cast<int> (procStates.size()))
        return false;

    const auto* state = procStates[static_cast<size_t> (info.index)];

    if (state->structName.isEmpty())
    {
        error (base.location, "'" + nameExpr->text + "' is not a struct-typed state");
        return false;
    }

    const auto* structDecl = findStruct (state->structName);

    if (structDecl == nullptr)
        return false;

    for (const auto& field : structDecl->fields)
    {
        if (field.name == fieldName)
        {
            outState = state;
            outField = &field;
            return true;
        }
    }

    error (base.location, "Struct '" + state->structName + "' has no field '" + fieldName + "'");
    return false;
}

} // namespace yup
