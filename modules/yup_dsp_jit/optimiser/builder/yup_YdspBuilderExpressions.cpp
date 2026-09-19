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

template <size_t N>
const YdspIrOp* findIntrinsicOp (const std::array<std::pair<const char*, YdspIrOp>, N>& table, StringRef name) noexcept
{
    for (const auto& [intrinsicName, op] : table)
        if (name == StringRef (intrinsicName))
            return &op;

    return nullptr;
}

} // namespace

int YdspIrBuilder::lowerExpr (const YdspExpr& expr)
{
    switch (expr.kind)
    {
        case YdspExprKind::intLiteral:
            return expr.integer >= std::numeric_limits<int32_t>::min() && expr.integer <= std::numeric_limits<int32_t>::max()
                       ? emitConstI (expr.integer) : emitConstI64 (expr.integer);

        case YdspExprKind::floatLiteral:
            return emitConstF (expr.number);

        case YdspExprKind::boolLiteral:
            return emitConstB (expr.flag);

        case YdspExprKind::identifier:
            return lowerIdentifier (expr.text, expr.location);

        case YdspExprKind::unary:
        {
            const auto operand = lowerExpr (*expr.children[0]);

            if (expr.op == YdspOperator::neg)
                return emitInst ({ isFloatValue (operand) ? YdspIrOp::negF : YdspIrOp::negI,
                                   newValue (valueTypes[static_cast<size_t> (operand)]),
                                   operand });

            if (expr.op == YdspOperator::notL)
                return emitInst ({ YdspIrOp::notB, newValue (YdspValueType::boolType), operand });

            if (expr.op == YdspOperator::notI)
            {
                const auto operandType = valueTypes[static_cast<size_t> (operand)];
                const auto minusOne = emitConstIFor (-1, operandType);
                return emitInst ({ YdspIrOp::xorI, newValue (operandType), operand, minusOne });
            }

            return operand;
        }

        case YdspExprKind::binary:
            return lowerBinary (expr);

        case YdspExprKind::ternary:
            return lowerConditional (*expr.children[0], expr.children[1].get(), expr.children[2].get());

        case YdspExprKind::call:
            return lowerCall (expr);

        case YdspExprKind::index:
        {
            const auto& base = *expr.children[0];
            const auto index = lowerExpr (*expr.children[1]);

            if (base.kind == YdspExprKind::member)
            {
                int structBase = 0, stride = 0, instanceIndex = -1, fieldLength = 0, instanceCount = 0;
                YdspValueType structType = YdspValueType::float32Type;

                if (resolveStructFieldLayout (base, structBase, stride, instanceIndex, structType, fieldLength, instanceCount))
                {
                    return emitCheckedMemory (isFloatValueType (structType) ? YdspIrOp::loadStateArrayF : YdspIrOp::loadStateArrayI,
                                              structType, structBase, index, emitConstI (fieldLength), -1, instanceIndex, instanceCount, stride);
                }

                diagnostics.addError (expr.location, "Invalid struct field index expression");
                return emitConstF (0.0);
            }

            if (base.kind == YdspExprKind::identifier)
            {
                if (const auto* state = findState (base.text))
                {
                    const auto stateType = toStorageType (state->type);

                    return emitCheckedMemory (stateIsInt (state) ? YdspIrOp::loadStateArrayI : YdspIrOp::loadStateArrayF,
                                              stateType, stateArrayBase (state), index, emitConstI (state->arraySize), -1, -1, 0, 0,
                                              provenArrayAccesses != nullptr && provenArrayAccesses->count (&expr) != 0);
                }

                if (const auto* endpoint = findEndpoint (base.text);
                    endpoint != nullptr && (endpoint->kind == YdspEndpointKind::inputStream || endpoint->kind == YdspEndpointKind::outputStream))
                {
                    const auto streamIndex = endpointStreamIndex (endpoint);

                    return emitCheckedMemory (endpoint->kind == YdspEndpointKind::inputStream ? YdspIrOp::loadInput : YdspIrOp::loadOutput,
                                              toStorageType (endpoint->type), streamIndex, index, blockSizeValue);
                }
            }

            diagnostics.addError (expr.location, "Invalid array index expression");
            return emitConstF (0.0);
        }

        case YdspExprKind::member:
        {
            // `e.<field>` - a payload field of the dispatching event.
            if (isEventHandler && eventHandler != nullptr
                && ! expr.children.empty() && expr.children[0]->kind == YdspExprKind::identifier
                && expr.children[0]->text == eventHandler->decl->paramName)
            {
                return lowerEventField (expr);
            }

            int structBase = 0, stride = 0, instanceIndex = -1, fieldLength = 0, instanceCount = 0;
            YdspValueType structType = YdspValueType::float32Type;

            if (! resolveStructFieldLayout (expr, structBase, stride, instanceIndex, structType, fieldLength, instanceCount))
            {
                diagnostics.addError (expr.location, "Invalid struct member access");
                return emitConstF (0.0);
            }

            if (stride == 0)
            {
                // Scalar field of a scalar struct instance: `state.field`.
                return emitInst ({ isFloatValueType (structType) ? YdspIrOp::loadStateF : YdspIrOp::loadStateI,
                                   newValue (structType),
                                   structBase });
            }

            return emitCheckedMemory (isFloatValueType (structType) ? YdspIrOp::loadStateArrayF : YdspIrOp::loadStateArrayI,
                                      structType, structBase, instanceIndex, emitConstI (instanceCount));
        }

        case YdspExprKind::prev:
        {
            const auto slot = newHiddenFloatScalar();
            const auto previous = promoteHiddenSlot (slot, YdspValueType::float32Type);

            pendingPrevStores.emplace_back (slot, previous, expr.children[0].get());

            return previous;
        }

        case YdspExprKind::delay:
        {
            constexpr int maxDelay = 65536;
            const auto n = static_cast<int> (expr.children[1]->integer);

            if (n <= 0)
                return lowerExpr (*expr.children[0]);

            if (n > maxDelay)
            {
                diagnostics.addError (expr.location, "The '@' delay amount exceeds the maximum of " + String (maxDelay) + " samples");
                return emitConstF (0.0);
            }

            const auto signal = coerceTo (lowerExpr (*expr.children[0]), YdspValueType::float32Type);

            if (const auto tap = sharedInputDelayTaps.find (&expr); tap != sharedInputDelayTaps.end())
            {
                auto& history = sharedInputDelays[tap->second];
                const auto mask = emitConstI (history.size - 1);

                if (history.writePosition < 0)
                {
                    history.ringBase = layout.float32ArrayCursor + hiddenFloat32ArrayElements;
                    hiddenFloat32ArrayElements += history.size;
                    const auto slot = layout.int32ScalarCount + hiddenInt32Scalars++;
                    history.writePosition = promoteHiddenSlot (slot, YdspValueType::int32Type);

                    emitInst ({ YdspIrOp::storeStateArrayF, -1, history.writePosition, signal, -1, history.ringBase });
                }

                const auto offset = emitInst ({ YdspIrOp::subI, newValue (YdspValueType::int32Type), history.writePosition, emitConstI (n) });
                const auto readIndex = emitInst ({ YdspIrOp::andI, newValue (YdspValueType::int32Type), offset, mask });
                return emitInst ({ YdspIrOp::loadStateArrayF, newValue (YdspValueType::float32Type), readIndex, -1, -1, history.ringBase });
            }

            const auto wpSlot = layout.int32ScalarCount + hiddenInt32Scalars++;
            const auto ringBase = layout.float32ArrayCursor + hiddenFloat32ArrayElements;
            hiddenFloat32ArrayElements += n + 1;

            const auto wp = promoteHiddenSlot (wpSlot, YdspValueType::int32Type);
            emitInst ({ YdspIrOp::storeStateArrayF, -1, wp, signal, -1, ringBase });

            const auto wrapped = emitInst ({ YdspIrOp::advanceWrapI, wp, wp, -1, -1, -1, 0.0, n + 1 });
            if (! fn.isSampleMode)
                writeHiddenSlot (wpSlot, wp, wrapped, YdspValueType::int32Type);

            const auto read = emitInst ({ YdspIrOp::loadStateArrayF, newValue (YdspValueType::float32Type), wrapped, -1, -1, ringBase });

            return read;
        }

        default:
            diagnostics.addError (expr.location, "Invalid expression in process body");
            return emitConstF (0.0);
    }
}

//==============================================================================

int YdspIrBuilder::lowerEventField (const YdspExpr& expr)
{
    const auto* shape = findEventShape (eventHandler->shape);
    const auto* field = shape != nullptr ? findEventField (*shape, expr.text) : nullptr;

    if (field == nullptr)
    {
        diagnostics.addError (expr.location, "Unknown event field '" + expr.text + "'");
        return emitConstF (0.0);
    }

    if (isFloatValueType (field->type))
        return emitInst ({ YdspIrOp::loadEventFieldF, newValue (YdspValueType::float32Type), -1, -1, -1, field->byteOffset });

    const auto raw = emitInst ({ YdspIrOp::loadEventFieldI, newValue (YdspValueType::int32Type), -1, -1, -1, field->byteOffset });

    if (field->type != YdspValueType::boolType)
        return raw;

    const auto masked = emitBinary (YdspIrOp::andI, YdspValueType::int32Type, raw, emitConstIFor (ydspEventFlagLegato, YdspValueType::int32Type));

    return emitBinary (YdspIrOp::neI, YdspValueType::boolType, masked, emitConstIFor (0, YdspValueType::int32Type));
}

//==============================================================================

int YdspIrBuilder::lowerIdentifier (const String& name, const YdspLocation& location)
{
    if (const auto it = locals.find (name); it != locals.end())
        return it->second;

    if (name == "blockSize")
    {
        if (! isEventHandler)
            return blockSizeValue;

        return emitConstI (0);
    }

    if (name == "sampleRate")
        return emitInst ({ YdspIrOp::loadSampleRate, newValue (YdspValueType::float32Type) });

    if (name == "samplePeriod")
    {
        const auto rate = emitInst ({ YdspIrOp::loadSampleRate, newValue (YdspValueType::float32Type) });
        return emitInst ({ YdspIrOp::divF, newValue (YdspValueType::float32Type), emitConstF (1.0), rate });
    }

    if (name == "pi")
        return emitConstF (3.14159265358979323846);

    if (name == "e")
        return emitConstF (2.71828182845904523536);

    if (name == "true")
        return emitConstB (true);

    if (name == "false")
        return emitConstB (false);

    if (const auto* state = findState (name))
    {
        if (! state->structName.isEmpty())
        {
            diagnostics.addError (location, "Struct state '" + name + "' must be accessed via a member ('.')");
            return emitConstF (0.0);
        }

        if (fn.isSampleMode && state->arraySize <= 0)
        {
            if (const auto it = stateRegs.find (state); it != stateRegs.end())
                return it->second;

            return emitInst ({ stateIsInt (state) ? YdspIrOp::loadStateI : YdspIrOp::loadStateF,
                               newValue (toStorageType (state->type)),
                               stateScalarSlot (state) });
        }

        if (state->arraySize > 0)
        {
            diagnostics.addError (location, "State array '" + name + "' must be indexed");
            return emitConstF (0.0);
        }

        return emitInst ({ stateIsInt (state) ? YdspIrOp::loadStateI : YdspIrOp::loadStateF,
                           newValue (toStorageType (state->type)),
                           stateScalarSlot (state) });
    }

    if (const auto* endpoint = findEndpoint (name))
    {
        switch (endpoint->kind)
        {
            case YdspEndpointKind::inputStream:
                return emitInst ({ YdspIrOp::loadInput, newValue (toStorageType (endpoint->type)), sampleIndex, -1, -1, endpointStreamIndex (endpoint) });

            case YdspEndpointKind::outputStream:
            {
                const auto streamIndex = endpointStreamIndex (endpoint);

                if (const auto it = lastOutputValue.find (static_cast<size_t> (streamIndex)); it != lastOutputValue.end())
                    return it->second;

                return emitInst ({ YdspIrOp::loadOutput, newValue (toStorageType (endpoint->type)), sampleIndex, -1, -1, streamIndex });
            }

            case YdspEndpointKind::inputParameter:
                return paramValues[static_cast<size_t> (endpointValueIndex (endpoint))];

            case YdspEndpointKind::outputParameter:
                return emitInst ({ YdspIrOp::loadParamOut, newValue (toStorageType (endpoint->type)), endpointValueIndex (endpoint) });

            default:
                break;
        }
    }

    diagnostics.addError (location, "Unknown symbol '" + name + "'");
    return emitConstF (0.0);
}

//==============================================================================

int YdspIrBuilder::captureValue (const YdspExpr& expr)
{
    if (expr.kind == YdspExprKind::identifier)
    {
        const auto& name = expr.text;

        if (const auto it = locals.find (name); it != locals.end())
            return it->second;

        if (const auto* state = findState (name); state != nullptr && fn.isSampleMode && state->arraySize <= 0)
        {
            if (const auto it = stateRegs.find (state); it != stateRegs.end())
                return it->second;
        }

        if (const auto* endpoint = findEndpoint (name))
        {
            if (endpoint->kind == YdspEndpointKind::outputStream)
            {
                const auto streamIndex = endpointStreamIndex (endpoint);

                if (const auto it = lastOutputValue.find (static_cast<size_t> (streamIndex)); it != lastOutputValue.end())
                    return it->second;
            }

            if (endpoint->kind == YdspEndpointKind::inputStream)
            {
                diagnostics.addError (expr.location, "'" + name + "' cannot be captured: input streams in sample mode are not buffered. Use a let-binding or state variable instead.");
                return emitConstF (0.0);
            }

            if (endpoint->kind == YdspEndpointKind::inputParameter)
            {
                diagnostics.addError (expr.location, "'" + name + "' cannot be captured: parameters are block-rate values.");
                return emitConstF (0.0);
            }
        }
    }

    return lowerExpr (expr);
}

//==============================================================================

int YdspIrBuilder::emitCheckedMemory (YdspIrOp op, YdspValueType type, int base, int index, int length, int value, int instanceIndex, int instanceCount, int stride, bool proven)
{
    const auto provenInRange = [&] (int candidate, int bound)
    {
        const auto candidateConstant = intConstPayloads.find (candidate);
        const auto boundConstant = intConstPayloads.find (bound);
        if (candidateConstant != intConstPayloads.end() && boundConstant != intConstPayloads.end())
            return candidateConstant->second >= 0 && candidateConstant->second < boundConstant->second;

        const auto upper = indexUpperBounds.find (candidate);
        if (upper == indexUpperBounds.end())
            return false;
        if (upper->second == bound)
            return true;
        const auto upperConstant = intConstPayloads.find (upper->second);
        return upperConstant != intConstPayloads.end() && boundConstant != intConstPayloads.end()
            && upperConstant->second <= boundConstant->second;
    };
    if ((proven || provenInRange (index, length))
        && (instanceIndex < 0 || provenInRange (instanceIndex, emitConstI (instanceCount))))
    {
        auto element = index;
        if (instanceIndex >= 0)
        {
            const auto scaled = emitBinary (YdspIrOp::mulI, YdspValueType::int32Type, instanceIndex, emitConstI (stride));
            element = emitBinary (YdspIrOp::addI, YdspValueType::int32Type, scaled, index);
        }
        return emitInst ({ op, value < 0 ? newValue (type) : -1, element, value, -1, base });
    }

    fn.hasGuardedControlFlow = true;
    const auto inRange = [&] (int candidate, int bound)
    {
        return emitBinary (YdspIrOp::ltUI, YdspValueType::boolType, candidate, bound);
    };
    auto valid = inRange (index, length);
    const auto bound = intConstPayloads.find (length);
    if (value < 0 && instanceIndex < 0
        && (op == YdspIrOp::loadStateArrayF || op == YdspIrOp::loadStateArrayI)
        && bound != intConstPayloads.end() && bound->second > 0)
    {
        // The fallback access must itself be in bounds; selecting only the
        // loaded result would still allow an invalid address to be read.
        const auto safeIndex = emitInst ({ YdspIrOp::selectB, newValue (YdspValueType::int32Type), valid, index, emitConstI (0) });
        const auto loaded = emitInst ({ op, newValue (type), safeIndex, -1, -1, base });
        const auto zero = isFloatValueType (type) ? emitConstFFor (0.0, type) : emitConstIFor (0, type);
        return emitInst ({ YdspIrOp::selectB, newValue (type), valid, loaded, zero });
    }
    if (instanceIndex >= 0)
        valid = emitBinary (YdspIrOp::andB, YdspValueType::boolType, valid, inRange (instanceIndex, emitConstI (instanceCount)));

    const auto result = value < 0 ? newValue (type) : -1;
    if (result >= 0)
    {
        const auto zero = isFloatValueType (type) ? emitConstFFor (0.0, type) : emitConstIFor (0, type);
        emitInst ({ moveOpcodeFor (type), result, zero });
    }
    const auto conditionBlock = currentBlock;
    fn.blocks[static_cast<size_t> (conditionBlock)].preservesConditionalEvaluation = true;
    const auto accessBlock = newBlock();
    currentBlock = accessBlock;
    auto element = index;
    if (instanceIndex >= 0)
    {
        const auto scaled = emitBinary (YdspIrOp::mulI, YdspValueType::int32Type, instanceIndex, emitConstI (stride));
        element = emitBinary (YdspIrOp::addI, YdspValueType::int32Type, scaled, index);
    }
    if (result >= 0)
    {
        const auto loaded = emitInst ({ op, newValue (type), element, -1, -1, base });
        emitInst ({ moveOpcodeFor (type), result, loaded });
    }
    else
    {
        emitInst ({ op, -1, element, value, -1, base });
    }
    const auto join = newBlock();
    currentBlock = conditionBlock;
    setTerminator (YdspIrTerm::branchIf, valid, accessBlock, join);
    currentBlock = accessBlock;
    setTerminator (YdspIrTerm::branch, -1, join);
    currentBlock = join;
    return result;
}

//==============================================================================

int YdspIrBuilder::lowerConditional (const YdspExpr& condition, const YdspExpr* thenExpr, const YdspExpr* elseExpr)
{
    fn.hasGuardedControlFlow = true;
    const auto cond = lowerExpr (condition);
    const auto condBlock = currentBlock;
    fn.blocks[static_cast<size_t> (condBlock)].preservesConditionalEvaluation = true;
    const auto outputsBefore = lastOutputValue;

    const auto thenBlock = newBlock();
    currentBlock = thenBlock;
    const auto thenValue = thenExpr != nullptr ? lowerExpr (*thenExpr) : emitConstB (true);
    const auto thenTail = currentBlock;

    const auto elseBlock = newBlock();
    currentBlock = elseBlock;
    lastOutputValue = outputsBefore;
    const auto elseValue = elseExpr != nullptr ? lowerExpr (*elseExpr) : emitConstB (false);
    const auto elseTail = currentBlock;

    auto thenUnified = thenValue;
    auto elseUnified = elseValue;
    auto resultType = YdspValueType::boolType;
    if (thenExpr != nullptr && elseExpr != nullptr)
        unifyOperands (*thenExpr, thenValue, *elseExpr, elseValue, thenUnified, elseUnified, resultType, thenTail, elseTail);

    const auto result = newValue (resultType);
    currentBlock = thenTail;
    emitInst ({ moveOpcodeFor (resultType), result, thenUnified });
    currentBlock = elseTail;
    emitInst ({ moveOpcodeFor (resultType), result, elseUnified });

    const auto join = newBlock();
    currentBlock = condBlock;
    setTerminator (YdspIrTerm::branchIf, cond, thenBlock, elseBlock);
    currentBlock = thenTail;
    setTerminator (YdspIrTerm::branch, -1, join);
    currentBlock = elseTail;
    setTerminator (YdspIrTerm::branch, -1, join);
    currentBlock = join;
    lastOutputValue.clear();
    return result;
}

//==============================================================================

int YdspIrBuilder::lowerBinary (const YdspExpr& expr)
{
    if (expr.op == YdspOperator::andL)
        return lowerConditional (*expr.children[0], expr.children[1].get(), nullptr);

    if (expr.op == YdspOperator::orL)
        return lowerConditional (*expr.children[0], nullptr, expr.children[1].get());

    const auto lhs = lowerExpr (*expr.children[0]);
    const auto rhs = lowerExpr (*expr.children[1]);

    int lhsU, rhsU;
    YdspValueType resultType;
    unifyOperands (*expr.children[0], lhs, *expr.children[1], rhs, lhsU, rhsU, resultType);

    const bool isFloat = isFloatValueType (resultType);

    if (! isFloat && (expr.op == YdspOperator::add || expr.op == YdspOperator::sub || expr.op == YdspOperator::mul))
    {
        const auto op = expr.op == YdspOperator::add ? YdspIrOp::addI
                      : expr.op == YdspOperator::sub ? YdspIrOp::subI : YdspIrOp::mulI;
        YdspIrInst arithmetic { op, newValue (resultType), lhsU, rhsU };
        arithmetic.saturatingIntegerArithmetic = boundedIntegerArithmetic == nullptr || boundedIntegerArithmetic->count (&expr) == 0;
        return emitInst (arithmetic);
    }

    switch (expr.op)
    {
        case YdspOperator::add:
            return emitBinary (isFloat ? YdspIrOp::addF : YdspIrOp::addI, resultType, lhsU, rhsU);
        case YdspOperator::sub:
            return emitBinary (isFloat ? YdspIrOp::subF : YdspIrOp::subI, resultType, lhsU, rhsU);
        case YdspOperator::mul:
            return emitBinary (isFloat ? YdspIrOp::mulF : YdspIrOp::mulI, resultType, lhsU, rhsU);
        case YdspOperator::div:
            return emitBinary (isFloat ? YdspIrOp::divF : YdspIrOp::divI, resultType, lhsU, rhsU);
        case YdspOperator::mod:
            return emitBinary (isFloat ? YdspIrOp::modF : YdspIrOp::modI, resultType, lhsU, rhsU);

        case YdspOperator::eq:
            return emitBinary (isFloat ? YdspIrOp::eqF : YdspIrOp::eqI, YdspValueType::boolType, lhsU, rhsU);
        case YdspOperator::ne:
            return emitBinary (isFloat ? YdspIrOp::neF : YdspIrOp::neI, YdspValueType::boolType, lhsU, rhsU);
        case YdspOperator::lt:
            return emitBinary (isFloat ? YdspIrOp::ltF : YdspIrOp::ltI, YdspValueType::boolType, lhsU, rhsU);
        case YdspOperator::le:
            return emitBinary (isFloat ? YdspIrOp::leF : YdspIrOp::leI, YdspValueType::boolType, lhsU, rhsU);
        case YdspOperator::gt:
            return emitBinary (isFloat ? YdspIrOp::gtF : YdspIrOp::gtI, YdspValueType::boolType, lhsU, rhsU);
        case YdspOperator::ge:
            return emitBinary (isFloat ? YdspIrOp::geF : YdspIrOp::geI, YdspValueType::boolType, lhsU, rhsU);

        case YdspOperator::bitAnd:
            return emitBinary (YdspIrOp::andI, resultType, lhsU, rhsU);
        case YdspOperator::bitOr:
            return emitBinary (YdspIrOp::orI, resultType, lhsU, rhsU);
        case YdspOperator::bitXor:
            return emitBinary (YdspIrOp::xorI, resultType, lhsU, rhsU);
        case YdspOperator::shl:
            return emitBinary (YdspIrOp::shlI, resultType, lhsU, rhsU);
        case YdspOperator::shr:
            return emitBinary (YdspIrOp::shrI, resultType, lhsU, rhsU);

        default:
            diagnostics.addError (expr.location, "Invalid binary operator");
            return emitConstF (0.0);
    }
}

//==============================================================================

int YdspIrBuilder::lowerCall (const YdspExpr& expr)
{
    const auto& callee = expr.text;

    if (callee == "mem")
    {
        const auto slot = newHiddenFloatScalar();
        const auto previous = promoteHiddenSlot (slot, YdspValueType::float32Type);

        pendingPrevStores.emplace_back (slot, previous, expr.children[0].get());
        return previous;
    }

    if (callee == "smooth")
    {
        const auto target = coerceTo (lowerExpr (*expr.children[0]), YdspValueType::float32Type);
        const auto tau = coerceTo (lowerExpr (*expr.children[1]), YdspValueType::float32Type);

        const auto valueSlot = newHiddenFloatScalar();
        const auto primedSlot = layout.int32ScalarCount + hiddenInt32Scalars++;

        const auto previous = promoteHiddenSlot (valueSlot, YdspValueType::float32Type);
        const auto primed = promoteHiddenSlot (primedSlot, YdspValueType::int32Type);

        const auto rate = emitInst ({ YdspIrOp::loadSampleRate, newValue (YdspValueType::float32Type) });
        const auto period = emitBinary (YdspIrOp::divF, YdspValueType::float32Type, emitConstF (1.0), rate);
        const auto exponent = emitBinary (YdspIrOp::divF, YdspValueType::float32Type, emitInst ({ YdspIrOp::negF, newValue (YdspValueType::float32Type), period }), tau);
        const auto decay = emitInst ({ YdspIrOp::expF, newValue (YdspValueType::float32Type), exponent });
        const auto coeff = emitBinary (YdspIrOp::subF, YdspValueType::float32Type, emitConstF (1.0), decay);

        const auto ramped = emitInst ({ YdspIrOp::lerpF, newValue (YdspValueType::float32Type), previous, target, coeff });

        const auto hasArrived = emitBinary (YdspIrOp::eqF, YdspValueType::boolType, ramped, previous);
        const auto isFirstSample = emitBinary (YdspIrOp::eqI, YdspValueType::boolType, primed, emitConstI (0));
        const auto snap = emitInst ({ YdspIrOp::orB, newValue (YdspValueType::boolType), isFirstSample, hasArrived });

        const auto next = emitInst ({ YdspIrOp::selectB, newValue (YdspValueType::float32Type), snap, target, ramped });

        writeHiddenSlot (valueSlot, previous, next, YdspValueType::float32Type);
        writeHiddenSlot (primedSlot, primed, emitConstI (1), YdspValueType::int32Type);

        return next;
    }

    if (callee == "int" || callee == "int32")
    {
        auto value = lowerExpr (*expr.children[0]);
        if (boundedFloatCasts != nullptr && boundedFloatCasts->count (&expr) != 0)
        {
            const auto type = valueTypes[static_cast<size_t> (value)];
            const auto zero = emitConstFFor (0.0, type);
            const auto ordered = emitBinary (YdspIrOp::geF, YdspValueType::boolType, value, zero);
            value = emitInst ({ YdspIrOp::selectB, newValue (type), ordered, value, zero });
            YdspIrInst conversion { YdspIrOp::ftoi, newValue (YdspValueType::int32Type), value };
            conversion.boundedFloatToInt = true;
            return emitInst (conversion);
        }
        return lowerCastToInt (value, YdspValueType::int32Type);
    }

    if (callee == "int64")
        return lowerCastToInt (lowerExpr (*expr.children[0]), YdspValueType::int64Type);

    if (callee == "float" || callee == "float32")
        return lowerCastToFloat (lowerExpr (*expr.children[0]), YdspValueType::float32Type);

    if (callee == "float64")
        return lowerCastToFloat (lowerExpr (*expr.children[0]), YdspValueType::float64Type);

    if (callee == "abs" || callee == "sign")
    {
        const auto arg = lowerExpr (*expr.children[0]);

        if (isIntValue (arg))
        {
            const auto argType = intWidthOf (arg);
            return emitInst ({ callee == "abs" ? YdspIrOp::absI : YdspIrOp::signI, newValue (argType), coerceTo (arg, argType) });
        }

        const auto argType = floatWidthOf (arg);
        return emitInst ({ callee == "abs" ? YdspIrOp::absF : YdspIrOp::signF, newValue (argType), coerceTo (arg, argType) });
    }

    if (callee == "min" || callee == "max")
    {
        const auto argA = lowerExpr (*expr.children[0]);
        const auto argB = lowerExpr (*expr.children[1]);

        if (anyIsIntValue (argA, argB))
        {
            const auto argType = intWidthOf (argA, argB);
            return emitInst ({ callee == "min" ? YdspIrOp::minI : YdspIrOp::maxI, newValue (argType), coerceTo (argA, argType), coerceTo (argB, argType) });
        }

        const auto argType = floatWidthOf (argA, argB);
        return emitInst ({ callee == "min" ? YdspIrOp::minF : YdspIrOp::maxF, newValue (argType), coerceTo (argA, argType), coerceTo (argB, argType) });
    }

    if (callee == "clamp")
    {
        const auto argA = lowerExpr (*expr.children[0]);
        const auto argB = lowerExpr (*expr.children[1]);
        const auto argC = lowerExpr (*expr.children[2]);

        if (anyIsIntValue (argA, argB, argC))
        {
            const auto argType = intWidthOf (argA, argB, argC);
            return emitInst ({ YdspIrOp::clampI, newValue (argType), coerceTo (argA, argType), coerceTo (argB, argType), coerceTo (argC, argType) });
        }

        const auto argType = floatWidthOf (argA, argB, argC);
        return emitInst ({ YdspIrOp::clampF, newValue (argType), coerceTo (argA, argType), coerceTo (argB, argType), coerceTo (argC, argType) });
    }

    static constexpr std::array<std::pair<const char*, YdspIrOp>, 20> unaryIntrinsics = { {
        { "sqrt", YdspIrOp::sqrtF },
        { "floor", YdspIrOp::floorF },
        { "ceil", YdspIrOp::ceilF },
        { "rint", YdspIrOp::rintF },
        { "sin", YdspIrOp::sinF },
        { "cos", YdspIrOp::cosF },
        { "tan", YdspIrOp::tanF },
        { "asin", YdspIrOp::asinF },
        { "acos", YdspIrOp::acosF },
        { "atan", YdspIrOp::atanF },
        { "sinh", YdspIrOp::sinhF },
        { "cosh", YdspIrOp::coshF },
        { "tanh", YdspIrOp::tanhF },
        { "exp", YdspIrOp::expF },
        { "log", YdspIrOp::logF },
        { "log10", YdspIrOp::log10F },
        { "asinh", YdspIrOp::asinhF },
        { "acosh", YdspIrOp::acoshF },
        { "atanh", YdspIrOp::atanhF },
        { "round", YdspIrOp::roundF },
    } };

    if (const auto* op = findIntrinsicOp (unaryIntrinsics, callee))
    {
        const auto arg = lowerExpr (*expr.children[0]);
        const auto argType = floatWidthOf (arg);
        return emitInst ({ *op, newValue (argType), coerceTo (arg, argType) });
    }

    static constexpr std::array<std::pair<const char*, YdspIrOp>, 4> binaryIntrinsics = { {
        { "pow", YdspIrOp::powF },
        { "fmod", YdspIrOp::fmodF },
        { "atan2", YdspIrOp::atan2F },
        { "copysign", YdspIrOp::copysignF },
    } };

    if (const auto* op = findIntrinsicOp (binaryIntrinsics, callee))
    {
        const auto argA = lowerExpr (*expr.children[0]);
        const auto argB = lowerExpr (*expr.children[1]);
        const auto argType = floatWidthOf (argA, argB);
        return emitInst ({ *op, newValue (argType), coerceTo (argA, argType), coerceTo (argB, argType) });
    }

    if (callee == "lerp")
    {
        const auto argA = lowerExpr (*expr.children[0]);
        const auto argB = lowerExpr (*expr.children[1]);
        const auto argC = lowerExpr (*expr.children[2]);
        const auto argType = floatWidthOf (argA, argB, argC);
        return emitInst ({ YdspIrOp::lerpF, newValue (argType), coerceTo (argA, argType), coerceTo (argB, argType), coerceTo (argC, argType) });
    }

    if (callee == "fma")
    {
        const auto argA = lowerExpr (*expr.children[0]);
        const auto argB = lowerExpr (*expr.children[1]);
        const auto argC = lowerExpr (*expr.children[2]);

        return emitInst ({ YdspIrOp::fmaF,
                           newValue (YdspValueType::float32Type),
                           coerceTo (argA, YdspValueType::float32Type),
                           coerceTo (argB, YdspValueType::float32Type),
                           coerceTo (argC, YdspValueType::float32Type) });
    }

    if (callee == "select")
    {
        const auto cond = lowerExpr (*expr.children[0]);
        const auto thenValue = lowerExpr (*expr.children[1]);
        const auto elseValue = lowerExpr (*expr.children[2]);

        int thenU, elseU;
        YdspValueType resultType;
        unifyOperands (*expr.children[1], thenValue, *expr.children[2], elseValue, thenU, elseU, resultType);

        return emitInst ({ YdspIrOp::selectB,
                           newValue (resultType),
                           cond,
                           thenU,
                           elseU });
    }

    if (const auto* func = findFunctionInScope (callee, &processor.functions, programFunctions))
        return lowerFunctionCall (*func, expr);

    diagnostics.addError (expr.location, "Unknown function '" + callee + "'");
    return emitConstF (0.0);
}

//==============================================================================

bool YdspIrBuilder::isAdaptableLiteral (const YdspExpr& expr)
{
    if (expr.kind == YdspExprKind::intLiteral || expr.kind == YdspExprKind::floatLiteral)
        return true;

    if (expr.kind == YdspExprKind::unary && expr.op == YdspOperator::neg && ! expr.children.empty())
        return isAdaptableLiteral (*expr.children[0]);

    return false;
}

//==============================================================================

void YdspIrBuilder::unifyOperands (const YdspExpr& a, int av, const YdspExpr& b, int bv, int& outA, int& outB, YdspValueType& outType, int aBlock, int bBlock)
{
    const auto convertInBlock = [this] (int value, YdspValueType type, int block, bool literal)
    {
        const auto savedBlock = currentBlock;
        if (block >= 0)
            currentBlock = block;
        const auto converted = literal ? widenConst (value, type) : coerceTo (value, type);
        currentBlock = savedBlock;
        return converted;
    };

    const auto ta = valueTypes[static_cast<size_t> (av)];
    const auto tb = valueTypes[static_cast<size_t> (bv)];

    if (ta == tb)
    {
        outA = av;
        outB = bv;
        outType = ta;
        return;
    }

    const bool aIsLiteral = isAdaptableLiteral (a);
    const bool bIsLiteral = isAdaptableLiteral (b);

    if (aIsLiteral && ! bIsLiteral)
    {
        if (const auto w = convertInBlock (av, tb, aBlock, true); w >= 0)
        {
            outA = w;
            outB = bv;
            outType = tb;
            return;
        }
    }
    else if (bIsLiteral && ! aIsLiteral)
    {
        if (const auto w = convertInBlock (bv, ta, bBlock, true); w >= 0)
        {
            outA = av;
            outB = w;
            outType = ta;
            return;
        }
    }
    else if (aIsLiteral && bIsLiteral)
    {
        if (const auto w = convertInBlock (av, tb, aBlock, true); w >= 0)
        {
            outA = w;
            outB = bv;
            outType = tb;
            return;
        }

        if (const auto w = convertInBlock (bv, ta, bBlock, true); w >= 0)
        {
            outA = av;
            outB = w;
            outType = ta;
            return;
        }
    }

    if (const auto coerced = convertInBlock (bv, ta, bBlock, false); coerced >= 0)
    {
        outA = av;
        outB = coerced;
        outType = ta;
        return;
    }

    if (const auto coerced = convertInBlock (av, tb, aBlock, false); coerced >= 0)
    {
        outA = coerced;
        outB = bv;
        outType = tb;
        return;
    }

    outA = av;
    outB = bv;
    outType = ta;
}

//==============================================================================

int YdspIrBuilder::widenConst (int value, YdspValueType targetType)
{
    const auto sourceType = valueTypes[static_cast<size_t> (value)];

    if (sourceType == targetType)
        return value;

    if (const auto it = intConstPayloads.find (value); it != intConstPayloads.end())
    {
        if (isIntValueType (targetType))
            return emitConstIFor (it->second, targetType);

        if (isFloatValueType (targetType))
            return emitConstFFor (static_cast<double> (it->second), targetType);
    }

    if (const auto it = floatConstPayloads.find (value); it != floatConstPayloads.end() && isFloatValueType (targetType))
        return emitConstFFor (it->second, targetType);

    return -1;
}

//==============================================================================

int YdspIrBuilder::coerceTo (int value, YdspValueType targetType)
{
    const auto sourceType = valueTypes[static_cast<size_t> (value)];

    if (sourceType == targetType)
        return value;

    if (const auto w = widenConst (value, targetType); w >= 0)
        return w;

    if (isIntValueType (sourceType) && isIntValueType (targetType))
        return emitInst ({ sourceType == YdspValueType::int32Type ? YdspIrOp::extI : YdspIrOp::truncI, newValue (targetType), value });

    if (isFloatValueType (sourceType) && isFloatValueType (targetType))
        return emitInst ({ sourceType == YdspValueType::float32Type ? YdspIrOp::extF : YdspIrOp::truncF, newValue (targetType), value });

    if (isIntValueType (sourceType) && isFloatValueType (targetType))
        return emitInst ({ YdspIrOp::itof, newValue (targetType), value });

    if (isFloatValueType (sourceType) && isIntValueType (targetType))
        return emitInst ({ YdspIrOp::ftoi, newValue (targetType), value });

    if (isIntValueType (targetType))
        return emitInst ({ targetType == YdspValueType::int64Type ? YdspIrOp::extI : YdspIrOp::movI, newValue (targetType), value });

    return emitInst ({ YdspIrOp::itof, newValue (targetType), value });
}

//==============================================================================

int YdspIrBuilder::lowerCastToInt (int value, YdspValueType targetType)
{
    return coerceTo (value, targetType);
}

//==============================================================================

int YdspIrBuilder::lowerCastToFloat (int value, YdspValueType targetType)
{
    return coerceTo (value, targetType);
}

//==============================================================================

bool YdspIrBuilder::resolveStructFieldLayout (const YdspExpr& memberExpr, int& outBase, int& outStride, int& outInstanceIndex, YdspValueType& outType, int& outFieldLength, int& outInstanceCount)
{
    outBase = 0;
    outStride = 0;
    outInstanceIndex = -1;

    if (memberExpr.kind != YdspExprKind::member || memberExpr.children.empty())
        return false;

    const auto& base = *memberExpr.children[0];
    const YdspExpr* nameExpr = &base;

    if (base.kind == YdspExprKind::index)
    {
        if (base.children.empty())
            return false;

        outInstanceIndex = lowerExpr (*base.children[1]);
        nameExpr = base.children[0].get();
    }

    if (nameExpr->kind != YdspExprKind::identifier)
        return false;

    const auto* state = findState (nameExpr->text);

    if (state == nullptr || state->structName.isEmpty())
        return false;

    const auto* structDecl = findStructDecl (state->structName);

    if (structDecl == nullptr)
        return false;

    for (size_t f = 0; f < structDecl->fields.size(); ++f)
    {
        if (structDecl->fields[f].name != memberExpr.text)
            continue;

        const auto key = std::make_pair (state, static_cast<int> (f));

        const auto baseIt = layout.structFieldBases.find (key);
        const auto strideIt = layout.structFieldStrides.find (key);

        if (baseIt == layout.structFieldBases.end() || strideIt == layout.structFieldStrides.end())
            return false;

        outBase = baseIt->second;
        outStride = strideIt->second;
        outFieldLength = jmax (1, structDecl->fields[f].arraySize);
        outInstanceCount = jmax (1, state->arraySize);
        outType = toStorageType (structDecl->fields[f].type);
        return true;
    }

    return false;
}

//==============================================================================

void YdspIrBuilder::lowerStateStore (const YdspStateDecl* state, int value)
{
    if (fn.isSampleMode && state->arraySize <= 0)
    {
        const auto reg = stateRegs[state];
        const auto regType = valueTypes[static_cast<size_t> (reg)];
        const auto coerced = coerceTo (value, regType);
        emitInst ({ moveOpcodeFor (regType), reg, coerced });
        return;
    }

    const auto stateType = toStorageType (state->type);

    emitInst ({ stateIsInt (state) ? YdspIrOp::storeStateI : YdspIrOp::storeStateF,
                -1,
                coerceTo (value, stateType),
                -1,
                -1,
                stateScalarSlot (state) });
}

} // namespace yup
