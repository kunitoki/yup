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

void YdspSemanticAnalyzer::analyzeStatement (const YdspStmt& stmt, YdspAnalyzedProcessor& proc)
{
    YdspRecursionGuard guard (recursionDepth);

    if (guard.exceeded())
    {
        error (stmt.location, "Statement nested too deeply");
        return;
    }

    switch (stmt.kind)
    {
        case YdspStmtKind::block:
        {
            pushLocalScope();

            for (const auto& child : stmt.children)
                analyzeStatement (*child, proc);

            popLocalScope();

            break;
        }

        case YdspStmtKind::localDecl:
        {
            YdspValueType type = YdspValueType::float32Type;

            if (stmt.value != nullptr)
            {
                const auto valueType = analyzeExpr (*stmt.value, proc);

                if (stmt.hasDeclType)
                {
                    type = toValueType (stmt.declType);

                    if (! canCoerce (valueType, type) && ! isAdaptableTo (*stmt.value, type) && ! isImplicitlyConvertibleTo (valueType, type))
                        error (stmt.value->location, "Cannot initialize a '" + yup::toString (stmt.declType) + "' local with a '" + typeName (valueType) + "' value (use an explicit cast)");
                }
                else
                {
                    type = valueType; // 'let' infers its type
                }
            }
            else if (stmt.hasDeclType)
            {
                type = toValueType (stmt.declType);
            }
            else
            {
                error (stmt.location, "A local declaration needs a type or an initializer");
            }

            YdspSymbolInfo info;
            info.kind = YdspSymbolKind::local;
            info.type = type;
            info.isLet = stmt.isLet;

            addSymbol (stmt.name, info, stmt.location);

            break;
        }

        case YdspStmtKind::assign:
        {
            const auto targetType = analyzeLvalue (*stmt.target, proc);

            if (stmt.value != nullptr)
            {
                const auto valueType = analyzeExpr (*stmt.value, proc);

                if (targetType.has_value() && ! canCoerce (valueType, *targetType) && ! isAdaptableTo (*stmt.value, *targetType) && ! isImplicitlyConvertibleTo (valueType, *targetType))
                    error (stmt.value->location, "Cannot assign a '" + typeName (valueType) + "' value to a '" + typeName (*targetType) + "' target (use an explicit cast)");
            }

            break;
        }

        case YdspStmtKind::ifStmt:
        {
            if (stmt.cond != nullptr)
            {
                const auto condType = analyzeExpr (*stmt.cond, proc);

                if (! canCoerce (condType, YdspValueType::boolType))
                    error (stmt.cond->location, "The if condition must be a boolean expression");
            }

            if (stmt.thenStmt != nullptr)
            {
                pushLocalScope();
                analyzeStatement (*stmt.thenStmt, proc);
                popLocalScope();
            }

            if (stmt.elseStmt != nullptr)
            {
                pushLocalScope();
                analyzeStatement (*stmt.elseStmt, proc);
                popLocalScope();
            }

            break;
        }

        case YdspStmtKind::forStmt:
        {
            const auto shadowed = symbols.find (stmt.name);
            const auto hadShadowed = shadowed != symbols.end();
            const auto shadowedInfo = hadShadowed ? shadowed->second : YdspSymbolInfo {};

            if (hadShadowed && shadowedInfo.kind != YdspSymbolKind::local && shadowedInfo.kind != YdspSymbolKind::builtinConstant)
                error (stmt.location, "Loop variable '" + stmt.name + "' shadows an existing symbol");

            YdspSymbolInfo loopVar;
            loopVar.kind = YdspSymbolKind::local;
            loopVar.type = YdspValueType::int32Type;
            loopVar.isLet = true;

            symbols[stmt.name] = loopVar;

            YdspLoopAnalysis loop;
            loop.depth = loopDepth;

            if (stmt.startExpr != nullptr)
            {
                const auto startType = analyzeExpr (*stmt.startExpr, proc);

                if (! canCoerce (startType, YdspValueType::int32Type) && ! isAdaptableTo (*stmt.startExpr, YdspValueType::int32Type))
                    error (stmt.startExpr->location, "The loop start bound must be an int32 (use int32(...) to convert)");
            }

            if (stmt.endExpr != nullptr)
            {
                const auto endType = analyzeExpr (*stmt.endExpr, proc);

                if (! canCoerce (endType, YdspValueType::int32Type) && ! isAdaptableTo (*stmt.endExpr, YdspValueType::int32Type))
                    error (stmt.endExpr->location, "The loop end bound must be an int32 (use int32(...) to convert)");

                if (! resolveLoopBound (*stmt.endExpr, loop.bound))
                {
                    error (stmt.endExpr->location,
                           "Loop bound must be statically provable: an integer literal, blockSize, or blockSize +/- an integer literal");
                    proc.provenRealtimeSafe = false;
                }
                else if (isEventHandlerMode && loop.bound.kind != YdspLoopBoundKind::constant)
                {
                    error (stmt.endExpr->location, "Loop bounds in an event handler must be compile-time constants");
                }
            }

            ++loopDepth;
            proc.loops.push_back (loop);

            if (stmt.body != nullptr)
                analyzeStatement (*stmt.body, proc);

            --loopDepth;

            if (hadShadowed)
                symbols[stmt.name] = shadowedInfo;
            else
                symbols.erase (stmt.name);

            break;
        }

        case YdspStmtKind::emitStmt:
        {
            if (isInitMode || isBlockMode)
            {
                error (stmt.location, "emit is not allowed in init or block-mode process (only in the per-sample process body or an event handler)");
                break;
            }

            const auto* shapeDesc = findEventShape (stmt.shapeName);

            if (shapeDesc == nullptr)
            {
                error (stmt.location, "Unknown event shape '" + stmt.shapeName + "' (expected one of " + eventShapeNameList() + ")");
                break;
            }

            for (const auto& [fieldName, valueExpr] : stmt.emitFields)
            {
                const auto* field = findEventField (*shapeDesc, fieldName);

                if (field == nullptr)
                {
                    error (valueExpr->location, "Event '" + stmt.shapeName + "' has no field '" + fieldName + "' (expected " + eventShapeFieldList (*shapeDesc) + ")");
                    continue;
                }

                const auto valueType = analyzeExpr (*valueExpr, proc);

                if (! canCoerce (valueType, field->type) && ! isAdaptableTo (*valueExpr, field->type))
                    error (valueExpr->location, "Cannot pass a '" + typeName (valueType) + "' value for emit field '" + fieldName + "' of type '" + typeName (field->type) + "' (use an explicit cast)");
            }

            const YdspEndpointDecl* endpoint = nullptr;

            for (const auto* candidate : proc.outputEvents)
                if (candidate->name == stmt.endpointName)
                {
                    endpoint = candidate;
                    break;
                }

            if (endpoint == nullptr)
                error (stmt.location, "Unknown emit target '" + stmt.endpointName + "'");

            break;
        }

        default:
            break;
    }
}

//==============================================================================

YdspValueType YdspSemanticAnalyzer::analyzeExpr (const YdspExpr& expr, YdspAnalyzedProcessor& proc)
{
    YdspRecursionGuard guard (recursionDepth);

    if (guard.exceeded())
    {
        error (expr.location, "Expression nested too deeply");
        return YdspValueType::float32Type;
    }

    switch (expr.kind)
    {
        case YdspExprKind::intLiteral:
            return YdspValueType::int32Type;

        case YdspExprKind::floatLiteral:
            return YdspValueType::float32Type;

        case YdspExprKind::boolLiteral:
            return YdspValueType::boolType;

        case YdspExprKind::identifier:
        {
            YdspSymbolInfo info;

            if (! findSymbol (expr.text, info))
            {
                error (expr.location, "Unknown symbol '" + expr.text + "'");
                return YdspValueType::float32Type;
            }

            if (isInitMode && (info.kind == YdspSymbolKind::inputStream || info.kind == YdspSymbolKind::outputStream))
            {
                error (expr.location, "Streams are not accessible during init");
                return YdspValueType::float32Type;
            }

            if (info.kind == YdspSymbolKind::stateScalar
                && info.index >= 0 && info.index < static_cast<int> (procStates.size())
                && ! procStates[static_cast<size_t> (info.index)]->structName.isEmpty())
            {
                error (expr.location, "Struct state '" + expr.text + "' must be accessed via a member ('.')");
                return YdspValueType::float32Type;
            }

            if (info.isArray)
                error (expr.location, "Array '" + expr.text + "' must be indexed");

            if (info.kind == YdspSymbolKind::eventParam && currentEventShape != nullptr)
            {
                error (expr.location, "Event value '" + expr.text + "' must be accessed via a member (" + eventShapeFieldList (*currentEventShape) + ")");
                return YdspValueType::float32Type;
            }

            return info.type;
        }

        case YdspExprKind::member:
        {
            if (! expr.children.empty() && expr.children[0]->kind == YdspExprKind::index
                && ! expr.children[0]->children.empty())
                analyzeExpr (*expr.children[0]->children[1], proc);

            if (isEventHandlerMode && currentEventShape != nullptr
                && ! expr.children.empty() && expr.children[0]->kind == YdspExprKind::identifier)
            {
                YdspSymbolInfo info;

                if (findSymbol (expr.children[0]->text, info) && info.kind == YdspSymbolKind::eventParam)
                {
                    if (const auto* field = findEventField (*currentEventShape, expr.text))
                        return field->type;

                    error (expr.location, "Event '" + String (currentEventShape->name) + "' has no field '" + expr.text + "' (expected " + eventShapeFieldList (*currentEventShape) + ")");
                    return YdspValueType::float32Type;
                }
            }

            if (expr.children.empty())
                return YdspValueType::float32Type;

            const YdspStateDecl* state = nullptr;
            const YdspStructField* field = nullptr;

            if (! resolveStructField (*expr.children[0], expr.text, state, field))
                return YdspValueType::float32Type;

            if (field->arraySize > 0)
            {
                error (expr.location, "Array field '" + expr.text + "' must be indexed");
                return YdspValueType::float32Type;
            }

            return toValueType (field->type);
        }

        case YdspExprKind::unary:
        {
            const auto operandType = analyzeExpr (*expr.children[0], proc);

            if (expr.op == YdspOperator::notL)
            {
                if (! canCoerce (operandType, YdspValueType::boolType))
                    error (expr.location, "'!' requires a boolean operand");

                return YdspValueType::boolType;
            }

            if (expr.op == YdspOperator::notI)
            {
                if (! isIntValueType (operandType))
                    error (expr.location, "'~' requires an integer operand");

                return operandType;
            }

            return operandType; // unary minus preserves the operand type
        }

        case YdspExprKind::binary:
        {
            const auto lhsType = analyzeExpr (*expr.children[0], proc);
            const auto rhsType = analyzeExpr (*expr.children[1], proc);

            switch (expr.op)
            {
                case YdspOperator::add:
                case YdspOperator::sub:
                case YdspOperator::mul:
                case YdspOperator::div:
                case YdspOperator::mod:
                {
                    const auto result = unifyTypes (*expr.children[0], lhsType, *expr.children[1], rhsType);

                    if (! result.has_value())
                        error (expr.location, "Cannot mix '" + typeName (lhsType) + "' and '" + typeName (rhsType) + "' operands (use an explicit cast)");

                    return result.value_or (lhsType);
                }

                case YdspOperator::lt:
                case YdspOperator::le:
                case YdspOperator::gt:
                case YdspOperator::ge:
                case YdspOperator::eq:
                case YdspOperator::ne:
                {
                    if (! unifyTypes (*expr.children[0], lhsType, *expr.children[1], rhsType).has_value())
                        error (expr.location, "Cannot compare '" + typeName (lhsType) + "' with '" + typeName (rhsType) + "' (use an explicit cast)");

                    return YdspValueType::boolType;
                }

                case YdspOperator::bitAnd:
                case YdspOperator::bitOr:
                case YdspOperator::bitXor:
                case YdspOperator::shl:
                case YdspOperator::shr:
                {
                    const auto result = unifyTypes (*expr.children[0], lhsType, *expr.children[1], rhsType);

                    if (! result.has_value())
                        error (expr.location, "Cannot mix '" + typeName (lhsType) + "' and '" + typeName (rhsType) + "' operands (use an explicit cast)");

                    const auto unifiedType = result.value_or (lhsType);

                    if (! isIntValueType (unifiedType))
                        error (expr.location, "Bitwise operators require integer operands");

                    return unifiedType;
                }

                case YdspOperator::andL:
                case YdspOperator::orL:
                {
                    if (! canCoerce (lhsType, YdspValueType::boolType) || ! canCoerce (rhsType, YdspValueType::boolType))
                        error (expr.location, "'&&' and '||' require boolean operands");

                    return YdspValueType::boolType;
                }

                default:
                    return YdspValueType::float32Type;
            }
        }

        case YdspExprKind::ternary:
        {
            const auto condType = analyzeExpr (*expr.children[0], proc);

            if (! canCoerce (condType, YdspValueType::boolType))
                error (expr.location, "The ternary condition must be a boolean expression");

            const auto thenType = analyzeExpr (*expr.children[1], proc);
            const auto elseType = analyzeExpr (*expr.children[2], proc);

            const auto result = unifyTypes (*expr.children[1], thenType, *expr.children[2], elseType);

            if (! result.has_value())
                error (expr.location, "The ternary branches must have compatible types (use an explicit cast)");

            return result.value_or (thenType);
        }

        case YdspExprKind::call:
        {
            const auto* intrinsic = findIntrinsic (expr.text);

            if (intrinsic == nullptr)
            {
                YdspValueType returnType;
                if (resolveFunctionCall (expr.text, expr.children, expr.location, returnType))
                    return returnType;

                error (expr.location, "Unknown function '" + expr.text + "'");
                return YdspValueType::float32Type;
            }

            const auto numArgs = static_cast<int> (expr.children.size());

            if (numArgs < intrinsic->minArgs || numArgs > intrinsic->maxArgs)
            {
                error (expr.location, "Function '" + expr.text + "' expects " + String (intrinsic->minArgs) + (intrinsic->maxArgs > intrinsic->minArgs ? " to " : " ") + String (intrinsic->maxArgs) + " arguments, got " + String (numArgs));
                return YdspValueType::float32Type;
            }

            if (expr.text == "mem")
            {
                if (isBlockMode || isInitMode || isEventHandlerMode || loopDepth > 0)
                {
                    if (isEventHandlerMode)
                        error (expr.location, "Delay primitives are not available inside event handlers");
                    else
                        error (expr.location, "Delay primitives (mem, ', @) are only allowed in the per-sample body, outside loops");
                    return YdspValueType::float32Type;
                }

                ++hiddenStateCount;

                const auto operandType = analyzeExpr (*expr.children[0], proc);

                if (! canCoerce (operandType, YdspValueType::float32Type) && ! isAdaptableTo (*expr.children[0], YdspValueType::float32Type))
                    error (expr.location, "Delay primitives (mem, ', @) require float32 operands (use float32(...) to convert)");

                return YdspValueType::float32Type;
            }

            if (expr.text == "smooth")
            {
                if (isBlockMode || isInitMode || isEventHandlerMode || loopDepth > 0)
                {
                    if (isEventHandlerMode)
                        error (expr.location, "smooth() is not available inside event handlers");
                    else
                        error (expr.location, "smooth() is only allowed in the per-sample body, outside loops");

                    return YdspValueType::float32Type;
                }

                ++hiddenStateCount;

                for (const auto& argument : expr.children)
                {
                    const auto argumentType = analyzeExpr (*argument, proc);

                    if (! canCoerce (argumentType, YdspValueType::float32Type) && ! isAdaptableTo (*argument, YdspValueType::float32Type))
                        error (expr.location, "smooth() requires float32 operands (use float32(...) to convert)");
                }

                return YdspValueType::float32Type;
            }

            if (expr.text == "fma")
            {
                for (const auto& argument : expr.children)
                {
                    const auto argumentType = analyzeExpr (*argument, proc);

                    if (argumentType == YdspValueType::float64Type)
                    {
                        error (argument->location, "fma() is float32 only, because no backend-independent float64 form exists (compute it as a * b + c instead)");
                        break;
                    }

                    if (! canCoerce (argumentType, YdspValueType::float32Type) && ! isAdaptableTo (*argument, YdspValueType::float32Type))
                        error (argument->location, "fma() requires float32 operands (use float32(...) to convert)");
                }

                return YdspValueType::float32Type;
            }

            if (expr.text == "select")
            {
                const auto condType = analyzeExpr (*expr.children[0], proc);

                if (! canCoerce (condType, YdspValueType::boolType))
                    error (expr.location, "select() requires a boolean first argument");

                const auto aType = analyzeExpr (*expr.children[1], proc);
                const auto bType = analyzeExpr (*expr.children[2], proc);

                const auto result = unifyTypes (*expr.children[1], aType, *expr.children[2], bType);

                if (! result.has_value())
                    error (expr.location, "select() branches must have compatible types (use an explicit cast)");

                return result.value_or (aType);
            }

            if (expr.text == "int" || expr.text == "int32")
            {
                analyzeExpr (*expr.children[0], proc);
                return YdspValueType::int32Type;
            }

            if (expr.text == "int64")
            {
                analyzeExpr (*expr.children[0], proc);
                return YdspValueType::int64Type;
            }

            if (expr.text == "float" || expr.text == "float32")
            {
                analyzeExpr (*expr.children[0], proc);
                return YdspValueType::float32Type;
            }

            if (expr.text == "float64")
            {
                analyzeExpr (*expr.children[0], proc);
                return YdspValueType::float64Type;
            }

            if (expr.text == "min" || expr.text == "max" || expr.text == "clamp" || expr.text == "abs" || expr.text == "sign")
            {
                std::vector<YdspValueType> argTypes;
                argTypes.reserve (expr.children.size());

                for (const auto& arg : expr.children)
                    argTypes.push_back (analyzeExpr (*arg, proc));

                bool sawInt = false;
                bool sawFloat = false;

                for (size_t i = 0; i < argTypes.size(); ++i)
                {
                    if (isAdaptableLiteral (*expr.children[i]))
                        continue;

                    if (isIntType (argTypes[i]))
                        sawInt = true;
                    else if (isFloatType (argTypes[i]))
                        sawFloat = true;
                }

                if (sawInt && ! sawFloat)
                {
                    YdspValueType resultType = YdspValueType::int32Type;
                    bool haveResultType = false;

                    for (size_t i = 0; i < argTypes.size(); ++i)
                    {
                        if (isAdaptableLiteral (*expr.children[i]))
                            continue;

                        if (haveResultType && argTypes[i] != resultType)
                            error (expr.children[i]->location, "Intrinsic '" + expr.text + "' requires operands of the same type (use an explicit cast)");
                        else
                        {
                            resultType = argTypes[i];
                            haveResultType = true;
                        }
                    }

                    return resultType;
                }

                YdspValueType resultType = YdspValueType::float32Type;
                bool haveResultType = false;

                for (size_t i = 0; i < argTypes.size(); ++i)
                {
                    if (isFloatType (argTypes[i]))
                    {
                        if (haveResultType && argTypes[i] != resultType)
                            error (expr.children[i]->location, "Intrinsic '" + expr.text + "' requires operands of the same type (use an explicit cast)");
                        else
                        {
                            resultType = argTypes[i];
                            haveResultType = true;
                        }
                    }
                    else if (! isAdaptableLiteral (*expr.children[i]))
                    {
                        error (expr.children[i]->location, "Intrinsic '" + expr.text + "' requires float operands (use an explicit float32(...)/float64(...) cast)");
                    }
                }

                return resultType;
            }

            YdspValueType resultType = YdspValueType::float32Type;
            bool haveResultType = false;

            for (const auto& arg : expr.children)
            {
                const auto argType = analyzeExpr (*arg, proc);

                if (isFloatType (argType))
                {
                    if (haveResultType && argType != resultType)
                        error (arg->location, "Intrinsic '" + expr.text + "' requires operands of the same type (use an explicit cast)");
                    else
                    {
                        resultType = argType;
                        haveResultType = true;
                    }
                }
                else if (! isAdaptableLiteral (*arg))
                {
                    error (arg->location, "Intrinsic '" + expr.text + "' requires float operands (use an explicit float32(...)/float64(...) cast)");
                }
            }

            return resultType;
        }

        case YdspExprKind::index:
        {
            const auto indexType = analyzeExpr (*expr.children[1], proc);

            if (! canCoerce (indexType, YdspValueType::int32Type) && ! isAdaptableTo (*expr.children[1], YdspValueType::int32Type))
                error (expr.location, "Array indices must be int32 (use int32(...) to convert)");

            if (expr.children[0]->kind == YdspExprKind::identifier)
            {
                YdspSymbolInfo info;

                if (findSymbol (expr.children[0]->text, info))
                {
                    if (! info.isArray)
                        error (expr.location, "Cannot index non-array '" + expr.children[0]->text + "'");

                    if (info.index >= 0 && info.index < static_cast<int> (procStates.size())
                        && ! procStates[static_cast<size_t> (info.index)]->structName.isEmpty())
                        error (expr.location, "Struct array element '" + expr.children[0]->text + "' must be accessed via a member ('.')");

                    return info.type;
                }
            }
            else if (expr.children[0]->kind == YdspExprKind::member)
            {
                const YdspStateDecl* state = nullptr;
                const YdspStructField* field = nullptr;

                if (resolveStructField (*expr.children[0]->children[0], expr.children[0]->text, state, field))
                {
                    if (field->arraySize <= 0)
                        error (expr.location, "Cannot index the scalar field '" + expr.children[0]->text + "'");

                    return toValueType (field->type);
                }
            }

            error (expr.location, "Invalid array index target");
            return YdspValueType::float32Type;
        }

        case YdspExprKind::prev:
        {
            if (isBlockMode || isInitMode || isEventHandlerMode || loopDepth > 0)
            {
                if (isEventHandlerMode)
                    error (expr.location, "Delay primitives are not available inside event handlers");
                else
                    error (expr.location, "Delay primitives (mem, ', @) are only allowed in the per-sample body, outside loops");
                return YdspValueType::float32Type;
            }

            ++hiddenStateCount;

            const auto operandType = analyzeExpr (*expr.children[0], proc);

            if (! canCoerce (operandType, YdspValueType::float32Type) && ! isAdaptableTo (*expr.children[0], YdspValueType::float32Type))
                error (expr.location, "Delay primitives (mem, ', @) require float32 operands (use float32(...) to convert)");

            return YdspValueType::float32Type;
        }

        case YdspExprKind::delay:
        {
            if (isBlockMode || isInitMode || isEventHandlerMode || loopDepth > 0)
            {
                if (isEventHandlerMode)
                    error (expr.location, "Delay primitives are not available inside event handlers");
                else
                    error (expr.location, "Delay primitives (mem, ', @) are only allowed in the per-sample body, outside loops");
                return YdspValueType::float32Type;
            }

            const auto signalType = analyzeExpr (*expr.children[0], proc);

            if (! canCoerce (signalType, YdspValueType::float32Type) && ! isAdaptableTo (*expr.children[0], YdspValueType::float32Type))
                error (expr.location, "Delay primitives (mem, ', @) require float32 operands (use float32(...) to convert)");

            if (expr.children[1]->kind != YdspExprKind::intLiteral)
            {
                error (expr.location, "The '@' delay amount must be a non-negative integer literal");
                return signalType;
            }

            if (expr.children[1]->number < 0)
                error (expr.location, "The '@' delay amount must be non-negative");

            const auto delaySamples = static_cast<int> (expr.children[1]->number);
            if (delaySamples > YdspAnalyzedProcessor::maxDelay)
                error (expr.location, "The '@' delay amount exceeds the maximum of " + String (YdspAnalyzedProcessor::maxDelay) + " samples");

            if (expr.children[1]->number > 0)
                ++hiddenStateCount;

            return signalType;
        }

        default:
            error (expr.location, "Invalid expression");
            return YdspValueType::float32Type;
    }
}

std::optional<YdspValueType> YdspSemanticAnalyzer::analyzeLvalue (const YdspExpr& expr, YdspAnalyzedProcessor& proc)
{
    if (expr.kind == YdspExprKind::identifier)
    {
        YdspSymbolInfo info;

        if (! findSymbol (expr.text, info))
        {
            error (expr.location, "Unknown symbol '" + expr.text + "'");
            return std::nullopt;
        }

        switch (info.kind)
        {
            case YdspSymbolKind::local:
                if (info.isLet)
                    error (expr.location, "Cannot assign to the immutable local '" + expr.text + "'");

                return info.type;

            case YdspSymbolKind::stateScalar:
                if (info.index >= 0 && info.index < static_cast<int> (procStates.size())
                    && ! procStates[static_cast<size_t> (info.index)]->structName.isEmpty())
                {
                    error (expr.location, "Struct state '" + expr.text + "' must be assigned via a member ('.')");
                    return std::nullopt;
                }

                return info.type;

            case YdspSymbolKind::stateArray:
                error (expr.location, "State array '" + expr.text + "' must be indexed when assigned");
                return std::nullopt;

            case YdspSymbolKind::outputStream:
                if (isInitMode)
                    error (expr.location, "Cannot write to an output stream during init");

                if (isBlockMode)
                    error (expr.location, "Output stream '" + expr.text + "' must be indexed in block mode");

                return info.type;

            case YdspSymbolKind::inputStream:
                error (expr.location, "Cannot write to the input stream '" + expr.text + "'");
                return std::nullopt;

            case YdspSymbolKind::inputValue:
                if (! isBlockMode)
                {
                    if (info.type != YdspValueType::float64Type && info.type != YdspValueType::int64Type)
                    {
                        error (expr.location, "Parameters can only be written in block mode (per-block automation)");
                        return std::nullopt;
                    }
                }

                return info.type;

            case YdspSymbolKind::outputValue:
                return info.type;

            case YdspSymbolKind::eventParam:
                error (expr.location, "Cannot assign to the event value '" + expr.text + "'");
                return std::nullopt;

            case YdspSymbolKind::builtinConstant:
                error (expr.location, "Cannot assign to the built-in constant '" + expr.text + "'");
                return std::nullopt;
        }

        return std::nullopt;
    }

    if (expr.kind == YdspExprKind::index)
    {
        analyzeExpr (*expr.children[1], proc); // type-check the index

        if (expr.children[0]->kind == YdspExprKind::identifier)
        {
            YdspSymbolInfo info;

            if (findSymbol (expr.children[0]->text, info))
            {
                switch (info.kind)
                {
                    case YdspSymbolKind::stateArray:
                        if (info.index >= 0 && info.index < static_cast<int> (procStates.size())
                            && ! procStates[static_cast<size_t> (info.index)]->structName.isEmpty())
                            error (expr.location, "Struct array element '" + expr.children[0]->text + "' must be assigned via a member ('.')");

                        return info.type;

                    case YdspSymbolKind::outputStream:
                        if (! isBlockMode)
                            error (expr.location, "Output stream '" + expr.children[0]->text + "' can only be indexed in block mode");

                        return info.type;

                    case YdspSymbolKind::inputStream:
                        error (expr.location, "Cannot write to the input stream '" + expr.children[0]->text + "'");
                        return std::nullopt;

                    default:
                        error (expr.location, "Cannot index '" + expr.children[0]->text + "' as an assignment target");
                        return std::nullopt;
                }
            }
        }
        else if (expr.children[0]->kind == YdspExprKind::member)
        {
            const YdspStateDecl* state = nullptr;
            const YdspStructField* field = nullptr;

            if (resolveStructField (*expr.children[0]->children[0], expr.children[0]->text, state, field))
            {
                if (field->arraySize <= 0)
                {
                    error (expr.location, "Cannot index the scalar field '" + expr.children[0]->text + "'");
                    return std::nullopt;
                }

                return toValueType (field->type);
            }
        }

        error (expr.location, "Invalid assignment target");
        return std::nullopt;
    }

    if (expr.kind == YdspExprKind::member)
    {
        if (! expr.children.empty() && expr.children[0]->kind == YdspExprKind::index
            && ! expr.children[0]->children.empty())
            analyzeExpr (*expr.children[0]->children[1], proc); // instance index

        if (isEventHandlerMode && ! expr.children.empty() && expr.children[0]->kind == YdspExprKind::identifier)
        {
            YdspSymbolInfo info;

            if (findSymbol (expr.children[0]->text, info) && info.kind == YdspSymbolKind::eventParam)
            {
                error (expr.location, "Cannot assign to the event value '" + expr.children[0]->text + "'");
                return std::nullopt;
            }
        }

        if (expr.children.empty())
            return std::nullopt;

        const YdspStateDecl* state = nullptr;
        const YdspStructField* field = nullptr;

        if (resolveStructField (*expr.children[0], expr.text, state, field))
        {
            if (field->arraySize > 0)
            {
                error (expr.location, "Array field '" + expr.text + "' must be indexed when assigned");
                return std::nullopt;
            }

            return toValueType (field->type);
        }

        return std::nullopt;
    }

    error (expr.location, "Invalid assignment target");
    return std::nullopt;
}

} // namespace yup
