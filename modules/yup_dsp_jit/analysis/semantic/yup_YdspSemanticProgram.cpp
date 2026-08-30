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

bool constantExprMentionsFloat (const YdspExpr& expr)
{
    if (expr.kind == YdspExprKind::floatLiteral)
        return true;

    if (expr.kind == YdspExprKind::identifier && (expr.text == "pi" || expr.text == "e" || expr.text == "inf"))
        return true;

    for (const auto& child : expr.children)
        if (child != nullptr && constantExprMentionsFloat (*child))
            return true;

    return false;
}

template <typename Decl>
const String* findAnnotation (const Decl& decl, StringRef key)
{
    for (const auto& [k, v] : decl.annotations)
        if (k == key)
            return &v;

    return nullptr;
}

void renameIdentifier (YdspExpr& expr, const String& from, const String& to)
{
    if (expr.kind == YdspExprKind::identifier && expr.text == from)
        expr.text = to;

    for (auto& child : expr.children)
        if (child != nullptr)
            renameIdentifier (*child, from, to);
}

void renameIdentifier (YdspStmt& stmt, const String& from, const String& to)
{
    for (auto* expr : { stmt.cond.get(), stmt.startExpr.get(), stmt.endExpr.get(), stmt.target.get(), stmt.value.get(), stmt.returnExpr.get() })
        if (expr != nullptr)
            renameIdentifier (*expr, from, to);

    for (auto* child : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
        if (child != nullptr)
            renameIdentifier (*child, from, to);

    for (auto& child : stmt.children)
        if (child != nullptr)
            renameIdentifier (*child, from, to);
}

void validateDiscreteValuesAnnotation (const YdspEndpointDecl& endpoint, YdspDiagnostics& diagnostics)
{
    const auto* annotation = findAnnotation (endpoint, "values");

    if (annotation == nullptr)
        return;

    if (endpoint.kind != YdspEndpointKind::inputValue)
    {
        diagnostics.addError (endpoint.location.line, endpoint.location.column, "[[ values ]] is only valid on an 'input value' parameter");
        return;
    }

    int meaningful = 0;

    for (const auto& token : StringArray::fromTokens (*annotation, ",", ""))
        if (token.trim().isNotEmpty())
            ++meaningful;

    if (meaningful < 2)
        diagnostics.addError (endpoint.location.line, endpoint.location.column, "[[ values ]] requires a list of at least two labels, but is '{ " + *annotation + " }'");
}

void validateStreamChannelCount (const YdspEndpointDecl& endpoint, YdspDiagnostics& diagnostics)
{
    if ((endpoint.kind != YdspEndpointKind::inputStream && endpoint.kind != YdspEndpointKind::outputStream)
        || endpoint.channelCount == 1)
        return;

    diagnostics.addError (endpoint.location.line,
                          endpoint.location.column,
                          "Stream '" + endpoint.name + "' declares " + String (endpoint.channelCount)
                              + " channels, but YDSP streams are mono-only (see spec section 8)");
}

void validateEndpointAnnotationKeys (const YdspEndpointDecl& endpoint, YdspDiagnostics& diagnostics)
{
    static constexpr std::array<const char*, 11> allowedEndpointKeys = {
        "name", "min", "max", "values", "init", "smoothing", "unit", "step", "style", "mid", "bipolar"
    };

    for (const auto& [key, value] : endpoint.annotations)
    {
        const auto isAllowed = std::any_of (allowedEndpointKeys.begin(), allowedEndpointKeys.end(), [&] (const char* allowedKey)
        {
            return key == allowedKey;
        });

        if (! isAllowed)
            diagnostics.addWarning (endpoint.location.line,
                                    endpoint.location.column,
                                    "Unknown endpoint annotation '" + key + "' on '" + endpoint.name
                                        + "' (expected 'name', 'min', 'max', 'values', 'init', 'smoothing', 'unit', 'step', 'style', 'mid' or 'bipolar')");
    }
}

} // namespace

//==============================================================================

void YdspSemanticAnalyzer::preprocessProgram (YdspProgram& program)
{
    resolveProgramConstants (program);

    if (! programConstants.empty())
    {
        rejectConstantShadowing (program);

        if (diagnostics.hasErrors())
            return;

        for (auto& processor : program.processors)
        {
            for (auto& endpoint : processor.endpoints)
                if (endpoint.defaultValue != nullptr)
                    substituteConstants (*endpoint.defaultValue);

            for (auto& state : processor.states)
                for (auto& initialiser : state.initialisers)
                    substituteConstants (*initialiser);

            for (auto& function : processor.functions)
                substituteConstants (function.body);

            if (processor.process != nullptr)
                substituteConstants (processor.process->body);

            if (processor.init != nullptr)
                substituteConstants (processor.init->body);

            for (auto& handler : processor.eventHandlers)
                substituteConstants (handler.body);
        }

        for (auto& function : program.functions)
            substituteConstants (function.body);

        for (auto& graph : program.graphs)
        {
            for (auto& endpoint : graph.endpoints)
                if (endpoint.defaultValue != nullptr)
                    substituteConstants (*endpoint.defaultValue);

            for (auto& node : graph.nodes)
                for (auto& [name, valueExpr] : node.overrides)
                    if (valueExpr != nullptr)
                        substituteConstants (*valueExpr);
        }
    }

    for (auto& processor : program.processors)
    {
        resolveStateArraySizes (processor);
        applyInitAnnotationDefaults (processor.endpoints);
        applySmoothingAnnotations (processor);

        for (const auto& endpoint : processor.endpoints)
        {
            validateDiscreteValuesAnnotation (endpoint, diagnostics);
            validateStreamChannelCount (endpoint, diagnostics);
            validateEndpointAnnotationKeys (endpoint, diagnostics);
        }
    }

    for (auto& graph : program.graphs)
    {
        applyInitAnnotationDefaults (graph.endpoints);

        for (const auto& endpoint : graph.endpoints)
        {
            if (findAnnotation (endpoint, "smoothing") != nullptr)
                error (endpoint.location, "[[ smoothing ]] is not available on a graph endpoint (annotate the processor's parameter instead)");

            validateDiscreteValuesAnnotation (endpoint, diagnostics);
            validateStreamChannelCount (endpoint, diagnostics);
            validateEndpointAnnotationKeys (endpoint, diagnostics);
        }
    }

    if (diagnostics.hasErrors())
        return;

    for (auto& processor : program.processors)
        lowerStateInitialisers (processor);
}

//==============================================================================

void YdspSemanticAnalyzer::resolveProgramConstants (YdspProgram& program)
{
    for (auto& constant : program.constants)
    {
        if (programConstants.find (constant.name) != programConstants.end())
        {
            error (constant.location, "Duplicate program constant '" + constant.name + "'");
            continue;
        }

        if (constant.value == nullptr)
            continue;

        substituteConstants (*constant.value);

        double folded = 0.0;

        if (! tryConstantFold (*constant.value, folded))
        {
            error (constant.location,
                   "The value of program constant '" + constant.name + "' is not a compile-time constant");
            continue;
        }

        YdspConstValue value;

        const bool isInteger = ! constantExprMentionsFloat (*constant.value)
                            && folded == std::floor (folded)
                            && std::abs (folded) < 9.0e15;
        if (isInteger)
        {
            value.type = YdspValueType::int32Type;
            value.asInt = static_cast<int64_t> (folded);
        }
        else
        {
            value.type = YdspValueType::float32Type;
            value.asDouble = folded;
        }

        programConstants[constant.name] = value;
    }
}

//==============================================================================

void YdspSemanticAnalyzer::rejectConstantShadowing (const YdspProgram& program)
{
    const auto check = [this] (const String& name, const YdspLocation& location, StringRef what)
    {
        if (programConstants.find (name) != programConstants.end())
            error (location, String (what) + " '" + name + "' redeclares the program constant '" + name + "'");
    };

    std::function<void (const YdspStmt&)> checkStatement;
    checkStatement = [&] (const YdspStmt& stmt)
    {
        if (stmt.kind == YdspStmtKind::localDecl)
            check (stmt.name, stmt.location, "Local");
        else if (stmt.kind == YdspStmtKind::forStmt)
            check (stmt.name, stmt.location, "Loop variable");

        for (const auto& child : stmt.children)
            if (child != nullptr)
                checkStatement (*child);

        for (auto* nested : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
            if (nested != nullptr)
                checkStatement (*nested);
    };

    const auto checkBody = [&] (const std::vector<YdspStmtPtr>& body)
    {
        for (const auto& stmt : body)
            if (stmt != nullptr)
                checkStatement (*stmt);
    };

    for (const auto& processor : program.processors)
    {
        for (const auto& endpoint : processor.endpoints)
            check (endpoint.name, endpoint.location, "Endpoint");

        for (const auto& state : processor.states)
            check (state.name, state.location, "State");

        for (const auto& function : processor.functions)
        {
            check (function.name, function.location, "Function");

            for (const auto& [paramName, paramType] : function.params)
                check (paramName, function.location, "Function parameter");

            checkBody (function.body);
        }

        if (processor.process != nullptr)
            checkBody (processor.process->body);

        if (processor.init != nullptr)
            checkBody (processor.init->body);

        for (const auto& handler : processor.eventHandlers)
        {
            check (handler.paramName, handler.location, "Event parameter");
            checkBody (handler.body);
        }
    }

    for (const auto& graph : program.graphs)
    {
        for (const auto& endpoint : graph.endpoints)
            check (endpoint.name, endpoint.location, "Endpoint");

        for (const auto& node : graph.nodes)
            check (node.instanceName, node.location, "Node");
    }
}

//==============================================================================

String YdspSemanticAnalyzer::dottedName (const YdspExpr& expr)
{
    if (expr.kind == YdspExprKind::identifier)
        return expr.text;

    if (expr.kind == YdspExprKind::member && expr.children.size() == 1 && expr.children[0] != nullptr)
    {
        const auto base = dottedName (*expr.children[0]);

        if (base.isNotEmpty())
            return base + "." + expr.text;
    }

    return {};
}

void YdspSemanticAnalyzer::substituteConstants (YdspExpr& expr) const
{
    if (expr.kind == YdspExprKind::identifier || expr.kind == YdspExprKind::member)
    {
        const auto name = dottedName (expr);

        if (name.isNotEmpty())
        {
            const auto it = programConstants.find (name);

            if (it != programConstants.end())
            {
                const auto& value = it->second;

                expr.kind = value.type == YdspValueType::int32Type ? YdspExprKind::intLiteral : YdspExprKind::floatLiteral;
                expr.number = value.type == YdspValueType::int32Type ? static_cast<double> (value.asInt) : value.asDouble;
                expr.text.clear();
                expr.op = YdspOperator::none;
                expr.children.clear();
                expr.overrides.clear();
                return;
            }
        }
    }

    for (auto& child : expr.children)
        if (child != nullptr)
            substituteConstants (*child);

    for (auto& [name, valueExpr] : expr.overrides)
        if (valueExpr != nullptr)
            substituteConstants (*valueExpr);
}

void YdspSemanticAnalyzer::substituteConstants (YdspStmt& stmt) const
{
    for (auto* subExpr : { stmt.cond.get(), stmt.startExpr.get(), stmt.endExpr.get(), stmt.target.get(), stmt.value.get(), stmt.returnExpr.get() })
        if (subExpr != nullptr)
            substituteConstants (*subExpr);

    for (auto* subStmt : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
        if (subStmt != nullptr)
            substituteConstants (*subStmt);

    for (auto& child : stmt.children)
        if (child != nullptr)
            substituteConstants (*child);
}

void YdspSemanticAnalyzer::substituteConstants (const std::vector<YdspStmtPtr>& body) const
{
    for (const auto& stmt : body)
        if (stmt != nullptr)
            substituteConstants (*stmt);
}

//==============================================================================

void YdspSemanticAnalyzer::resolveStateArraySizes (YdspProcessorDecl& processor)
{
    for (auto& state : processor.states)
    {
        if (state.arraySizeName.isNotEmpty())
        {
            const auto it = programConstants.find (state.arraySizeName);

            if (it == programConstants.end())
            {
                error (state.location, "Unknown program constant '" + state.arraySizeName + "' used as the size of state '" + state.name + "'");
                continue;
            }

            const auto size = it->second.type == YdspValueType::int32Type
                                ? it->second.asInt
                                : static_cast<int64_t> (it->second.asDouble);

            if (size <= 0 || size > 1'000'000)
            {
                error (state.location, "The size of state '" + state.name + "' must be a positive integer (got " + String (static_cast<int> (size)) + ")");
                continue;
            }

            state.arraySize = static_cast<int> (size);
        }

        if (state.initialisers.empty())
            continue;

        if (! state.structName.isEmpty())
        {
            error (state.location, "Struct-typed state '" + state.name + "' cannot have an initialiser");
            state.initialisers.clear();
            continue;
        }

        const auto expected = state.arraySize > 0 ? static_cast<size_t> (state.arraySize) : size_t (1);

        if (state.initialisers.size() > expected)
        {
            error (state.location,
                   "State '" + state.name + "' has " + String (static_cast<int> (state.initialisers.size()))
                       + " initialisers but holds only " + String (static_cast<int> (expected)));
            state.initialisers.resize (expected);
        }
    }
}

//==============================================================================

void YdspSemanticAnalyzer::lowerStateInitialisers (YdspProcessorDecl& processor)
{
    std::vector<YdspStmtPtr> assignments;

    for (auto& state : processor.states)
    {
        if (! state.initialisers.empty())
        {
            const auto* role = findAnnotation (state, "role");

            if (role != nullptr && *role == "voiceActivity")
                error (state.location, "A 'voiceActivity' state must not have an initialiser (it starts at 0, i.e. asleep)");
        }

        for (size_t i = 0; i < state.initialisers.size(); ++i)
        {
            if (state.initialisers[i] == nullptr)
                continue;

            auto target = YdspExprFactory::makeIdentifier (state.location, state.name);

            if (state.arraySize > 0)
            {
                auto element = std::make_unique<YdspExpr>();
                element->kind = YdspExprKind::index;
                element->location = state.location;
                element->children.push_back (std::move (target));
                element->children.push_back (YdspExprFactory::makeInt (state.location, static_cast<long long> (i)));
                target = std::move (element);
            }

            auto assign = std::make_unique<YdspStmt>();
            assign->kind = YdspStmtKind::assign;
            assign->location = state.location;
            assign->target = std::move (target);
            assign->value = std::move (state.initialisers[i]);

            assignments.push_back (std::move (assign));
        }

        state.initialisers.clear();
    }

    if (assignments.empty())
        return;

    if (processor.init == nullptr)
    {
        processor.init = std::make_unique<YdspProcessDecl>();
        processor.init->location = processor.location;
    }

    auto& body = processor.init->body;

    for (auto& stmt : body)
        assignments.push_back (std::move (stmt));

    body = std::move (assignments);
}

//==============================================================================

void YdspSemanticAnalyzer::applyInitAnnotationDefaults (std::vector<YdspEndpointDecl>& endpoints)
{
    for (auto& endpoint : endpoints)
    {
        if (endpoint.kind != YdspEndpointKind::inputValue || endpoint.defaultValue != nullptr)
            continue;

        for (const auto& [key, value] : endpoint.annotations)
        {
            if (key != "init")
                continue;

            const auto isIntegral = endpoint.type == YdspPrimitiveType::int32Type
                                 || endpoint.type == YdspPrimitiveType::int64Type
                                 || endpoint.type == YdspPrimitiveType::boolType;

            endpoint.defaultValue = isIntegral
                                      ? YdspExprFactory::makeInt (endpoint.location, value.getLargeIntValue())
                                      : YdspExprFactory::makeFloat (endpoint.location, value.getDoubleValue());
            break;
        }
    }
}

//==============================================================================

void YdspSemanticAnalyzer::applySmoothingAnnotations (YdspProcessorDecl& processor)
{
    std::vector<YdspStmtPtr> smoothers;

    for (const auto& endpoint : processor.endpoints)
    {
        const auto* annotation = findAnnotation (endpoint, "smoothing");

        if (annotation == nullptr)
            continue;

        if (endpoint.kind != YdspEndpointKind::inputValue)
        {
            error (endpoint.location, "[[ smoothing ]] is only valid on an 'input value' parameter");
            continue;
        }

        if (endpoint.type != YdspPrimitiveType::float32Type)
        {
            error (endpoint.location, "[[ smoothing ]] requires a float32 parameter");
            continue;
        }

        const auto seconds = annotation->getDoubleValue();

        if (! (seconds > 0.0))
        {
            error (endpoint.location, "[[ smoothing ]] requires a positive time constant in seconds");
            continue;
        }

        if (processor.process == nullptr || processor.process->mode != YdspProcessMode::sample)
        {
            error (endpoint.location, "[[ smoothing ]] requires a per-sample 'process { }' body");
            continue;
        }

        const auto smoothedName = endpoint.name + "__s";

        // Only the per-sample body is rewritten: event handlers and function
        // bodies deliberately keep reading the raw target.
        for (auto& statement : processor.process->body)
            if (statement != nullptr)
                renameIdentifier (*statement, endpoint.name, smoothedName);

        std::vector<YdspExprPtr> arguments;
        arguments.push_back (YdspExprFactory::makeIdentifier (endpoint.location, endpoint.name));
        arguments.push_back (YdspExprFactory::makeFloat (endpoint.location, seconds));

        auto declaration = std::make_unique<YdspStmt>();
        declaration->kind = YdspStmtKind::localDecl;
        declaration->location = endpoint.location;
        declaration->hasDeclType = true;
        declaration->declType = YdspPrimitiveType::float32Type;
        declaration->name = smoothedName;
        declaration->value = YdspExprFactory::makeCall (endpoint.location, "smooth", std::move (arguments));

        smoothers.push_back (std::move (declaration));
    }

    if (smoothers.empty())
        return;

    auto& body = processor.process->body;

    for (auto& statement : body)
        smoothers.push_back (std::move (statement));

    body = std::move (smoothers);
}

} // namespace yup
