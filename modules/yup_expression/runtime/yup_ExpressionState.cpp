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
ExpressionState::ExpressionState (const CompiledExpression& compiledProgram, int maxBlockSize)
    : program (compiledProgram)
    , blockSize (jmax (1, maxBlockSize))
{
    const auto& p = *program.program;

    // Every register holds a value per lane, followed by its previous uniform value
    const auto numValues = blockSize + 1;

    constexpr size_t baseAlignment = 64;
    std::vector<size_t> offsets;
    size_t totalSize = 0;

    for (const auto& reg : p.registers)
    {
        const auto alignment = jmax ((size_t) 16, reg.type->getAlignment());
        totalSize = (totalSize + alignment - 1) / alignment * alignment;
        offsets.push_back (totalSize);
        totalSize += reg.type->getSize() * (size_t) numValues;
    }

    storage = std::make_unique<uint8[]> (totalSize + baseAlignment);
    auto* base = storage.get() + (baseAlignment - (reinterpret_cast<uintptr_t> (storage.get()) % baseAlignment)) % baseAlignment;

    for (size_t i = 0; i < p.registers.size(); ++i)
    {
        const auto& reg = p.registers[i];
        lanes.push_back (base + offsets[i]);
        reg.type->construct (lanes.back(), numValues);

        if (reg.constant != nullptr)
            reg.type->copy (lanes.back(), reg.type->getSize(), reg.constant.get(), 0, blockSize);
    }

    const auto numRegisters = p.registers.size();
    isVarying.resize (numRegisters, 0);
    isDirty.resize (numRegisters, 0);
    needsBroadcast.resize (numRegisters, 0);
    isChanged.resize (numRegisters, 1);
    broadcastRegisters.reserve (numRegisters);
    volatileRegisters.reserve (p.inputs.size());
    uniformCalls.reserve (p.code.size());
    varyingCalls.reserve (p.code.size());

    size_t numArguments = 0;

    for (const auto& instruction : p.code)
        numArguments += instruction.inputs.size();

    arguments.reserve (numArguments);
    argumentRegisters.reserve (numArguments);

    for (const auto& instruction : p.code)
    {
        const auto* args = arguments.data() + arguments.size();
        const auto* argRegisters = argumentRegisters.data() + argumentRegisters.size();

        for (auto reg : instruction.inputs)
        {
            arguments.push_back (lanes[(size_t) reg]);
            argumentRegisters.push_back (reg);
        }

        const auto* outputType = p.registers[(size_t) instruction.output].type;

        calls.push_back ({ instruction.thunk,
                           instruction.state.get(),
                           lanes[(size_t) instruction.output],
                           lanes[(size_t) instruction.output] + outputType->getSize() * (size_t) blockSize,
                           outputType,
                           args,
                           argRegisters,
                           (int) instruction.inputs.size(),
                           instruction.output,
                           instruction.isPure });
    }
}

ExpressionState::~ExpressionState()
{
    const auto& p = *program.program;

    for (size_t i = 0; i < p.registers.size(); ++i)
        p.registers[i].type->destroy (lanes[i], blockSize + 1);
}

//==============================================================================
int ExpressionState::findInputRegister (int index, std::type_index type) const
{
    const auto& inputs = program.program->inputs;

    if (! isPositiveAndBelow (index, (int) inputs.size()))
        return -1;

    const auto& input = inputs[(size_t) index];

    if (input.type->getCppType() != type)
    {
        jassertfalse; // The handle type doesn't match the input type
        return -1;
    }

    return input.reg;
}

bool ExpressionState::unbindRegister (int reg)
{
    const auto numColumns = inputColumns.size();

    inputColumns.erase (std::remove_if (inputColumns.begin(), inputColumns.end(), [reg] (const auto& column)
    {
        return column.reg == reg;
    }),
                        inputColumns.end());

    bool wasBound = inputColumns.size() != numColumns;

    if (rowIndexRegister == reg)
    {
        rowIndexRegister = -1;
        wasBound = true;
    }

    scheduleChanged |= wasBound;
    return wasBound;
}

void* ExpressionState::prepareInput (int index, std::type_index type, const void* value)
{
    const auto reg = findInputRegister (index, type);

    if (reg < 0)
        return nullptr;

    if (! unbindRegister (reg) && program.program->registers[(size_t) reg].type->areEqual (lanes[(size_t) reg], value))
        return nullptr;

    isDirty[(size_t) reg] = 1;
    needsBroadcast[(size_t) reg] = 1;
    uniformsChanged = true;
    return lanes[(size_t) reg];
}

void ExpressionState::bindColumn (int index, std::type_index type, const void* firstRow, size_t strideBytes)
{
    const auto reg = findInputRegister (index, type);

    if (reg < 0)
        return;

    unbindRegister (reg);
    scheduleChanged = true;
    inputColumns.push_back ({ reg, program.program->registers[(size_t) reg].type, static_cast<uint8*> (const_cast<void*> (firstRow)), strideBytes });
}

void ExpressionState::bindRowIndex (ExpressionInput<int> input)
{
    const auto reg = findInputRegister (input.index, typeid (int));

    if (reg < 0)
        return;

    unbindRegister (reg);
    scheduleChanged = true;
    rowIndexRegister = reg;
}

void ExpressionState::bindOutputColumn (int index, std::type_index type, void* firstRow, size_t strideBytes)
{
    const auto& outputs = program.program->outputs;

    if (! isPositiveAndBelow (index, (int) outputs.size()) || outputs[(size_t) index].type->getCppType() != type)
    {
        jassertfalse; // The handle doesn't belong to this program
        return;
    }

    const auto reg = outputs[(size_t) index].reg;
    outputColumns.push_back ({ reg, outputs[(size_t) index].type, static_cast<uint8*> (firstRow), strideBytes });
}

void ExpressionState::clearBindings()
{
    inputColumns.clear();
    outputColumns.clear();
    rowIndexRegister = -1;
    scheduleChanged = true;
}

//==============================================================================
void ExpressionState::updateSchedule() noexcept
{
    const auto& p = *program.program;

    std::fill (isVarying.begin(), isVarying.end(), 0);

    for (const auto& column : inputColumns)
        isVarying[(size_t) column.reg] = 1;

    if (rowIndexRegister >= 0)
        isVarying[(size_t) rowIndexRegister] = 1;

    // Registers read per row (by varying calls or as outputs) must hold uniform values in every lane
    std::vector<uint8>& isBroadcast = needsBroadcast;
    std::fill (isBroadcast.begin(), isBroadcast.end(), 0);

    uniformCalls.clear();
    varyingCalls.clear();

    for (size_t i = 0; i < calls.size(); ++i)
    {
        const auto& call = calls[i];
        const auto* first = call.inputRegisters;
        const auto* last = first + call.numInputs;

        const bool varying = ! call.isPure || std::any_of (first, last, [this] (int reg)
        {
            return isVarying[(size_t) reg] != 0;
        });

        if (! varying)
        {
            uniformCalls.push_back ((int) i);
            continue;
        }

        varyingCalls.push_back ((int) i);
        isVarying[(size_t) call.outputRegister] = 1;

        for (const auto* reg = first; reg != last; ++reg)
            isBroadcast[(size_t) *reg] = isVarying[(size_t) *reg] == 0;
    }

    for (const auto& output : p.outputs)
        isBroadcast[(size_t) output.reg] = isVarying[(size_t) output.reg] == 0;

    broadcastRegisters.clear();

    for (size_t reg = 0; reg < isBroadcast.size(); ++reg)
    {
        if (isBroadcast[reg] != 0 && p.registers[reg].constant == nullptr)
            broadcastRegisters.push_back ((int) reg);
    }

    // Array elements can change in place, so anything reading them is computed again on every evaluation
    volatileRegisters.clear();

    for (const auto& input : p.inputs)
    {
        if (input.reg >= 0 && input.type->getElementType() != nullptr && isVarying[(size_t) input.reg] == 0)
            volatileRegisters.push_back (input.reg);
    }

    std::fill (isDirty.begin(), isDirty.end(), 1);
    std::fill (needsBroadcast.begin(), needsBroadcast.end(), 1);

    scheduleChanged = false;
    uniformsChanged = true;
    forceUniformCalls = true;
}

void ExpressionState::runUniformCalls() noexcept
{
    for (auto index : volatileRegisters)
        isDirty[(size_t) index] = 1;

    for (auto index : uniformCalls)
    {
        const auto& call = calls[(size_t) index];

        const bool inputsChanged = std::any_of (call.inputRegisters, call.inputRegisters + call.numInputs, [this] (int reg)
        {
            return isDirty[(size_t) reg] != 0;
        });

        if ((inputsChanged || forceUniformCalls) && runUniformCall (call))
        {
            isDirty[(size_t) call.outputRegister] = 1;
            needsBroadcast[(size_t) call.outputRegister] = 1;
        }
    }

    if (forceUniformCalls)
    {
        std::fill (isChanged.begin(), isChanged.end(), 1);
    }
    else
    {
        for (size_t reg = 0; reg < isDirty.size(); ++reg)
            if (isDirty[reg] != 0)
                isChanged[reg] = 1;
    }

    std::fill (isDirty.begin(), isDirty.end(), 0);
    uniformsChanged = ! volatileRegisters.empty();
    forceUniformCalls = false;
}

bool ExpressionState::runUniformCall (const Call& call) noexcept
{
    const auto* type = call.outputType;

    if (type->isTriviallyCopyable())
    {
        std::memcpy (call.previousOutput, call.output, type->getSize());
        call.thunk (call.state, call.output, call.args, 1);
        return forceUniformCalls || std::memcmp (call.previousOutput, call.output, type->getSize()) != 0;
    }

    type->copy (call.previousOutput, 0, call.output, 0, 1);
    call.thunk (call.state, call.output, call.args, 1);
    return forceUniformCalls || ! type->areEqual (call.previousOutput, call.output);
}

//==============================================================================
void ExpressionState::evaluate (int row) noexcept
{
    evaluateBlock (row, 1);
}

int ExpressionState::evaluateBlock (int startRow, int numRows) noexcept
{
    std::fill (isChanged.begin(), isChanged.end(), 0);
    return runBlock (startRow, numRows);
}

int ExpressionState::runBlock (int startRow, int numRows) noexcept
{
    numRows = jlimit (0, blockSize, numRows);

    if (numRows == 0)
        return 0;

    if (scheduleChanged)
        updateSchedule();

    for (const auto& column : inputColumns)
    {
        const auto size = column.type->getSize();
        column.type->copy (lanes[(size_t) column.reg], size, column.data + (size_t) startRow * column.stride, column.stride, numRows);
    }

    if (rowIndexRegister >= 0)
    {
        auto* rows = reinterpret_cast<int*> (lanes[(size_t) rowIndexRegister]);

        for (int i = 0; i < numRows; ++i)
            rows[i] = startRow + i;
    }

    if (uniformsChanged)
        runUniformCalls();

    if (numRows > 1)
    {
        const auto& registers = program.program->registers;

        for (auto reg : broadcastRegisters)
        {
            if (needsBroadcast[(size_t) reg] != 0)
            {
                const auto size = registers[(size_t) reg].type->getSize();
                registers[(size_t) reg].type->copy (lanes[(size_t) reg] + size, size, lanes[(size_t) reg], 0, blockSize - 1);
                needsBroadcast[(size_t) reg] = 0;
            }
        }
    }

    for (auto index : varyingCalls)
    {
        const auto& call = calls[(size_t) index];
        call.thunk (call.state, call.output, call.args, numRows);
    }

    for (const auto& column : outputColumns)
    {
        const auto size = column.type->getSize();
        column.type->copy (column.data + (size_t) startRow * column.stride, column.stride, lanes[(size_t) column.reg], size, numRows);
    }

    numEvaluatedRows = numRows;
    return numRows;
}

void ExpressionState::evaluateRows (int numRows) noexcept
{
    std::fill (isChanged.begin(), isChanged.end(), 0);

    for (int row = 0; row < numRows;)
    {
        const auto numEvaluated = runBlock (row, numRows - row);

        if (numEvaluated <= 0)
            break;

        row += numEvaluated;
    }
}

//==============================================================================
bool ExpressionState::hasOutputChanged (int outputIndex) const noexcept
{
    const auto& outputs = program.program->outputs;

    if (! isPositiveAndBelow (outputIndex, (int) outputs.size()))
    {
        jassertfalse; // The handle doesn't belong to this program
        return false;
    }

    const auto reg = (size_t) outputs[(size_t) outputIndex].reg;
    return isChanged[reg] != 0 || isVarying[reg] != 0;
}

bool ExpressionState::hasChanged() const noexcept
{
    for (int i = 0; i < program.getNumOutputs(); ++i)
        if (hasOutputChanged (i))
            return true;

    return false;
}

//==============================================================================
const void* ExpressionState::getOutputLanes (int index, std::type_index type, int lane) const noexcept
{
    const auto& outputs = program.program->outputs;

    if (! isPositiveAndBelow (index, (int) outputs.size()) || ! isPositiveAndBelow (lane, blockSize) || outputs[(size_t) index].type->getCppType() != type)
    {
        jassertfalse; // The handle doesn't belong to this program, or the lane is out of range
        return nullptr;
    }

    const auto& output = outputs[(size_t) index];
    return lanes[(size_t) output.reg] + (size_t) lane * output.type->getSize();
}

var ExpressionState::getVar (int outputIndex, int lane) const
{
    const auto* type = program.getOutputType (outputIndex);

    if (type == nullptr || ! isPositiveAndBelow (lane, blockSize))
        return {};

    const auto* value = lanes[(size_t) program.program->outputs[(size_t) outputIndex].reg] + (size_t) lane * type->getSize();

    if (type == &ExpressionType::boolType())
        return *reinterpret_cast<const bool*> (value);

    if (type == &ExpressionType::intType())
        return *reinterpret_cast<const int*> (value);

    if (type == &ExpressionType::floatType())
        return (double) *reinterpret_cast<const float*> (value);

    if (type == &ExpressionType::doubleType())
        return *reinterpret_cast<const double*> (value);

    if (type == &ExpressionType::stringType())
        return *reinterpret_cast<const String*> (value);

    return {};
}

Result ExpressionState::setVar (StringRef inputName, const var& value)
{
    const auto index = program.findInput (inputName);

    if (index < 0)
        return Result::fail ("Unknown input '" + String (inputName) + "'");

    return setVar (index, value);
}

Result ExpressionState::setVar (int inputIndex, const var& value)
{
    const auto* type = program.getInputType (inputIndex);

    if (type == nullptr)
        return Result::fail ("Invalid input index");

    if (type == &ExpressionType::boolType())
        set (ExpressionInput<bool> { inputIndex }, (bool) value);
    else if (type == &ExpressionType::intType())
        set (ExpressionInput<int> { inputIndex }, (int) value);
    else if (type == &ExpressionType::floatType())
        set (ExpressionInput<float> { inputIndex }, (float) (double) value);
    else if (type == &ExpressionType::doubleType())
        set (ExpressionInput<double> { inputIndex }, (double) value);
    else if (type == &ExpressionType::stringType())
        set (ExpressionInput<String> { inputIndex }, value.toString());
    else
        return Result::fail ("The input '" + program.getInputName (inputIndex) + "' of type '" + type->getName() + "' can't be set from a var");

    return Result::ok();
}

} // namespace yup
