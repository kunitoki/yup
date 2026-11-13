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
struct CompiledExpression::Program
{
    struct Register
    {
        const ExpressionType* type = nullptr;
        std::shared_ptr<void> constant;
    };

    struct Instruction
    {
        ExpressionThunk thunk = nullptr;
        std::shared_ptr<const void> state;
        int output = -1;
        std::vector<int> inputs;
        bool isPure = true;
    };

    struct Variable
    {
        String name;
        const ExpressionType* type = nullptr;
        int reg = -1;
    };

    std::vector<Register> registers;
    std::vector<Instruction> code;
    std::vector<Variable> inputs, outputs;
    std::vector<std::shared_ptr<ExpressionType>> types;
};

//==============================================================================
namespace
{

struct ExpressionFusedThunks
{
    enum class Arithmetic
    {
        multiplyAdd,
        multiplySubtract,
        negativeMultiplyAdd
    };

    template <class T, Arithmetic kind>
    static void arithmetic (const void*, void* output, const void* const* args, int numRows)
    {
        auto* result = static_cast<T*> (output);
        auto* a = static_cast<const T*> (args[0]);
        auto* b = static_cast<const T*> (args[1]);
        auto* c = static_cast<const T*> (args[2]);

        for (int i = 0; i < numRows; ++i)
        {
            // Kept as a separate statement, so the product is rounded exactly like the unfused code
            const T product = a[i] * b[i];

            if constexpr (kind == Arithmetic::multiplyAdd)
                result[i] = product + c[i];
            else if constexpr (kind == Arithmetic::multiplySubtract)
                result[i] = product - c[i];
            else
                result[i] = c[i] - product;
        }
    }

    template <size_t N>
    struct Bytes
    {
        uint8 data[N];
    };

    template <class Compare, class T, class R>
    static void compareSelect (const void*, void* output, const void* const* args, int numRows)
    {
        auto* result = static_cast<R*> (output);
        auto* a = static_cast<const T*> (args[0]);
        auto* b = static_cast<const T*> (args[1]);
        auto* whenTrue = static_cast<const R*> (args[2]);
        auto* whenFalse = static_cast<const R*> (args[3]);

        for (int i = 0; i < numRows; ++i)
            result[i] = Compare() (a[i], b[i]) ? whenTrue[i] : whenFalse[i];
    }

    struct ArithmeticThunks
    {
        ExpressionThunk multiply = nullptr, add = nullptr, subtract = nullptr;
        ExpressionThunk multiplyAdd = nullptr, multiplySubtract = nullptr, negativeMultiplyAdd = nullptr;
    };

    struct ComparisonThunks
    {
        ExpressionThunk compare = nullptr;
        std::array<std::pair<size_t, ExpressionThunk>, 4> selects;

        ExpressionThunk findSelect (size_t size) const noexcept
        {
            for (const auto& [selectSize, select] : selects)
                if (selectSize == size)
                    return select;

            return nullptr;
        }
    };

    std::vector<ArithmeticThunks> arithmetics;
    std::vector<ComparisonThunks> comparisons;
};

} // namespace

//==============================================================================
struct ExpressionCompiler
{
    using Kind = Expression::Kind;
    using Program = CompiledExpression::Program;

    struct Error
    {
        String message;
        std::pair<int, int> position;
    };

    struct Value
    {
        int reg = -1;
        const ExpressionType* type = nullptr;
        bool isAbstract = false;
    };

    ExpressionCompiler (const ExpressionContext& c, Program& p)
        : context (c)
        , program (p)
    {
        program.types = context.types;

        for (const auto& input : context.inputs)
            program.inputs.push_back ({ input.name, input.type, -1 });
    }

    //==============================================================================
    void compileProgram (const Expression& root)
    {
        if (root.getKind() == Kind::block)
        {
            for (int i = 0; i < root.getNumInputs(); ++i)
                compileStatement (root.getInput (i));
        }
        else
        {
            compileStatement (root);
        }

        if (program.outputs.empty())
            fail (root, "The expression has no outputs");

        removeUnusedCode();

        if (fuseInstructions())
            removeUnusedCode();
    }

    void compileStatement (const Expression& statement)
    {
        switch (statement.getKind())
        {
            case Kind::let:
                if (findLocal (statement.getName()) != nullptr)
                    fail (statement, "'" + statement.getName() + "' is already defined");

                locals.emplace_back (statement.getName(), compile (statement.getInput (0)));
                break;

            case Kind::output:
            {
                // Bare enumeration values shared by several enumerations resolve to the type of the output
                expectedType = context.findOutputType (statement.getName());
                auto value = compile (statement.getInput (0));
                expectedType = nullptr;

                addOutput (statement.getName(), std::move (value), statement);
                break;
            }

            default:
                addOutput ("result", compile (statement), statement);
                break;
        }
    }

    void addOutput (const String& name, Value value, const Expression& where)
    {
        if (context.findInput (name) >= 0)
            fail (where, "Cannot assign to the input '" + name + "'");

        for (const auto& output : program.outputs)
            if (output.name == name)
                fail (where, "The output '" + name + "' is already assigned");

        if (findLocal (name) != nullptr)
            fail (where, "'" + name + "' is already defined");

        if (const auto* declaredType = context.findOutputType (name))
        {
            if (conversionCost (value, declaredType) < 0)
                fail (where, "Cannot convert '" + value.type->getName() + "' to '" + declaredType->getName() + "' for the output '" + name + "'");

            value = convert (value, declaredType, where);
        }

        value.isAbstract = false;
        program.outputs.push_back ({ name, value.type, value.reg });
        locals.emplace_back (name, value);
    }

    //==============================================================================
    Value compile (const Expression& e)
    {
        struct DepthGuard
        {
            DepthGuard (ExpressionCompiler& c, const Expression& e)
                : compiler (c)
            {
                if (++compiler.depth > 512)
                    compiler.fail (e, "Expression is too deeply nested");
            }

            ~DepthGuard() { --compiler.depth; }

            ExpressionCompiler& compiler;
        } guard (*this, e);

        switch (e.getKind())
        {
            case Kind::constant:
                return compileConstant (e);

            case Kind::symbol:
                return compileSymbol (e.getName(), e);

            case Kind::unary:
                if (e.getOperator() == ExpressionOperator::logicalNot)
                    return callOperator (e.getOperator(), { toBool (compile (e.getInput (0)), e.getInput (0)) }, e);

                return callOperator (e.getOperator(), { compile (e.getInput (0)) }, e);

            case Kind::binary:
                if (e.getOperator() == ExpressionOperator::logicalAnd || e.getOperator() == ExpressionOperator::logicalOr)
                    return callOperator (e.getOperator(), { toBool (compile (e.getInput (0)), e.getInput (0)), toBool (compile (e.getInput (1)), e.getInput (1)) }, e);

                return callOperator (e.getOperator(), { compile (e.getInput (0)), compile (e.getInput (1)) }, e);

            case Kind::ternary:
                return compileTernary (e);

            case Kind::call:
                return compileCall (e);

            case Kind::method:
                return compileMethod (e);

            case Kind::member:
                return compileMember (compile (e.getInput (0)), e.getName(), e);

            case Kind::index:
                return compileIndex (e);

            case Kind::let:
            case Kind::output:
            case Kind::block:
                break;
        }

        fail (e, "Statements are only allowed at the top level");
    }

    Value compileConstant (const Expression& e)
    {
        const auto& value = e.getValue();

        if (const auto* b = std::get_if<bool> (&value))
            return makeConstant (*b);

        if (const auto* i = std::get_if<int> (&value))
            return makeConstant (*i);

        if (const auto* f = std::get_if<float> (&value))
            return makeConstant (*f);

        if (const auto* d = std::get_if<double> (&value))
            return makeConstant (*d, true);

        return makeConstant (std::get<String> (value));
    }

    //==============================================================================
    Value compileSymbol (const String& name, const Expression& e)
    {
        for (auto lookup : { &ExpressionCompiler::findSymbol, &ExpressionCompiler::resolveSymbol, &ExpressionCompiler::findEnumValue })
        {
            for (auto length = name.length(); length > 0; length = name.substring (0, length).lastIndexOfChar ('.'))
            {
                if (auto value = (this->*lookup) (name.substring (0, length), e))
                {
                    for (const auto& member : StringArray::fromTokens (name.substring (length + 1), ".", {}))
                        *value = compileMember (*value, member, e);

                    return *value;
                }
            }
        }

        fail (e, "Unknown symbol '" + name + "'");
    }

    std::optional<Value> resolveSymbol (const String& name, const Expression& e)
    {
        if (context.symbolResolver == nullptr)
            return std::nullopt;

        for (const auto& input : program.inputs)
            if (input.name == name)
                return Value { input.reg, input.type };

        const auto* type = context.symbolResolver (name);

        if (type == nullptr)
            return std::nullopt;

        const auto isKnownType = ExpressionType::findBuiltin (type->getName()) == type
                              || std::any_of (program.types.begin(), program.types.end(), [type] (const auto& t) { return t.get() == type; });

        if (! isKnownType)
            fail (e, "The type of '" + name + "' isn't registered in the context");

        program.inputs.push_back ({ name, type, addRegister (type) });
        return Value { program.inputs.back().reg, type };
    }

    std::optional<Value> findSymbol (const String& name, const Expression& e)
    {
        if (const auto* local = findLocal (name))
            return *local;

        if (const auto* symbol = context.findSymbol (name))
        {
            switch (symbol->kind)
            {
                case ExpressionContext::SymbolKind::input:
                {
                    auto& input = program.inputs[(size_t) symbol->index];

                    if (input.reg < 0)
                        input.reg = addRegister (input.type);

                    return Value { input.reg, input.type };
                }

                case ExpressionContext::SymbolKind::constant:
                {
                    const auto& constant = context.constants[(size_t) symbol->index];
                    return Value { addRegister (constant.type, constant.value), constant.type };
                }

                case ExpressionContext::SymbolKind::expression:
                    return compileNamedExpression (symbol->index, e);
            }
        }

        if (name == "pi")
            return makeConstant (MathConstants<double>::pi, true);

        if (name == "tau")
            return makeConstant (MathConstants<double>::twoPi, true);

        if (name == "e")
            return makeConstant (MathConstants<double>::euler, true);

        return std::nullopt;
    }

    std::optional<Value> findEnumValue (const String& name, const Expression& e)
    {
        if (const auto it = context.enumValues.find (name); it != context.enumValues.end())
        {
            const auto& indices = it->second;

            const auto expected = std::find_if (indices.begin(), indices.end(), [this] (int index)
            {
                return context.constants[(size_t) index].type == expectedType;
            });

            if (indices.size() > 1 && expected == indices.end())
            {
                StringArray candidates;

                for (const auto index : indices)
                    candidates.add (context.constants[(size_t) index].type->getName() + "." + name);

                fail (e, "'" + name + "' is ambiguous, use one of " + candidates.joinIntoString (", "));
            }

            const auto& constant = context.constants[(size_t) (expected != indices.end() ? *expected : indices.front())];
            return Value { addRegister (constant.type, constant.value), constant.type };
        }

        return std::nullopt;
    }

    const Value* findLocal (const String& name) const
    {
        for (auto it = locals.rbegin(); it != locals.rend(); ++it)
            if (it->first == name)
                return &it->second;

        return nullptr;
    }

    Value compileNamedExpression (int index, const Expression& e)
    {
        if (const auto it = namedExpressionValues.find (index); it != namedExpressionValues.end())
            return it->second;

        const auto& [name, body] = context.expressions[(size_t) index];

        if (std::find (namedExpressionStack.begin(), namedExpressionStack.end(), index) != namedExpressionStack.end())
            fail (e, "The sub-expression '" + name + "' references itself");

        namedExpressionStack.push_back (index);
        auto savedLocals = std::exchange (locals, {});

        Value value;

        if (body.getKind() == Kind::block)
        {
            for (int i = 0; i < body.getNumInputs(); ++i)
            {
                const auto statement = body.getInput (i);

                if (statement.getKind() == Kind::output)
                    fail (e, "The sub-expression '" + name + "' can't assign outputs");

                if (statement.getKind() == Kind::let)
                    locals.emplace_back (statement.getName(), compile (statement.getInput (0)));
                else
                    value = compile (statement);
            }

            if (value.reg < 0)
                fail (e, "The sub-expression '" + name + "' has no value");
        }
        else
        {
            value = compile (body);
        }

        locals = std::move (savedLocals);
        namedExpressionStack.pop_back();
        namedExpressionValues[index] = value;
        return value;
    }

    //==============================================================================
    Value compileCall (const Expression& e)
    {
        const auto& name = e.getName();

        std::vector<Value> args;

        for (int i = 0; i < e.getNumInputs(); ++i)
            args.push_back (compile (e.getInput (i)));

        if (name == "len" && args.size() == 1 && args[0].type->getElementType() != nullptr)
            return emit (args[0].type->getLengthThunk(), {}, &ExpressionType::intType(), args, true);

        if ((name == "min" || name == "max") && args.size() > 2)
        {
            auto result = callFunction (name, { args[0], args[1] }, e);

            for (size_t i = 2; i < args.size(); ++i)
                result = callFunction (name, { result, args[i] }, e);

            return result;
        }

        if (name == "color" && findFunctions (name).empty())
            fail (e, "Unknown function 'color', color literals need a 'color' type in the context");

        return callFunction (name, std::move (args), e);
    }

    Value compileMethod (const Expression& e)
    {
        const auto object = e.getInput (0);

        std::vector<Value> args;

        for (int i = 1; i < e.getNumInputs(); ++i)
            args.push_back (compile (e.getInput (i)));

        if (object.getKind() == Kind::symbol)
        {
            const auto qualifiedName = object.getName() + "." + e.getName();

            if (! findFunctions (qualifiedName).empty())
                return callFunction (qualifiedName, std::move (args), e);
        }

        args.insert (args.begin(), compile (object));
        return callFunction ("." + e.getName(), std::move (args), e);
    }

    Value compileMember (Value object, const String& name, const Expression& e)
    {
        if (findOverload ("." + name, { object }).function != nullptr)
            return callFunction ("." + name, { object }, e);

        const auto& components = object.type->getComponentNames();

        if (components.size() > 1 && name.length() == components.size())
        {
            std::vector<Value> args;

            for (int i = 0; i < name.length(); ++i)
            {
                const auto component = String::charToString (name[i]);

                if (! components.contains (component))
                    break;

                args.push_back (compileMember (object, component, e));
            }

            if ((int) args.size() == components.size())
                return callFunction (object.type->getName(), std::move (args), e);
        }

        fail (e, "The type '" + object.type->getName() + "' has no member '" + name + "'");
    }

    Value compileIndex (const Expression& e)
    {
        const auto array = compile (e.getInput (0));
        auto index = compile (e.getInput (1));

        if (array.type->getElementType() == nullptr)
            fail (e, "The type '" + array.type->getName() + "' can't be indexed");

        if (index.type != &ExpressionType::intType() && index.type != &ExpressionType::boolType())
            fail (e.getInput (1), "An array index must be an 'int', not '" + index.type->getName() + "'");

        index = convert (index, &ExpressionType::intType(), e);
        return emit (array.type->getIndexThunk(), {}, array.type->getElementType(), { array, index }, true);
    }

    Value compileTernary (const Expression& e)
    {
        const auto condition = toBool (compile (e.getInput (0)), e.getInput (0));
        auto whenTrue = compile (e.getInput (1));
        auto whenFalse = compile (e.getInput (2));

        const auto trueToFalse = conversionCost (whenTrue, whenFalse.type);
        const auto falseToTrue = conversionCost (whenFalse, whenTrue.type);

        if (trueToFalse < 0 && falseToTrue < 0)
            fail (e, "The branches of '?:' have incompatible types '" + whenTrue.type->getName() + "' and '" + whenFalse.type->getName() + "'");

        const auto* type = falseToTrue >= 0 && (trueToFalse < 0 || falseToTrue <= trueToFalse) ? whenTrue.type : whenFalse.type;

        if (whenTrue.isAbstract && whenFalse.isAbstract)
            type = &ExpressionType::doubleType();

        if (const auto* value = getConstant (condition))
        {
            const auto& chosen = *static_cast<const bool*> (value) ? whenTrue : whenFalse;
            return chosen.isAbstract && type == &ExpressionType::doubleType() ? chosen : convert (chosen, type, e);
        }

        whenTrue = convert (whenTrue, type, e);
        whenFalse = convert (whenFalse, type, e);
        return emit (type->getSelectThunk(), {}, type, { condition, whenTrue, whenFalse }, true);
    }

    Value toBool (Value value, const Expression& e)
    {
        if (value.type == &ExpressionType::boolType())
            return value;

        if (! value.type->isNumeric())
            fail (e, "The type '" + value.type->getName() + "' can't be used as a condition");

        return callFunction ("bool", { value }, e);
    }

    //==============================================================================
    struct Overload
    {
        const ExpressionFunction* function = nullptr;
        int numBest = 0;
    };

    std::vector<const ExpressionFunction*> findFunctions (const String& name) const
    {
        std::vector<const ExpressionFunction*> result;

        if (const auto it = context.functions.find (name); it != context.functions.end())
            for (const auto& function : it->second)
                result.push_back (&function);

        const auto numCustom = result.size();
        const auto& builtins = ExpressionContext::getBuiltinFunctions();

        if (const auto it = builtins.find (name); it != builtins.end())
        {
            for (const auto& function : it->second)
            {
                const auto isReplaced = std::any_of (result.begin(), result.begin() + (std::ptrdiff_t) numCustom, [&] (const auto* custom)
                {
                    return custom->hasSameSignature (function);
                });

                if (! isReplaced)
                    result.push_back (&function);
            }
        }

        return result;
    }

    Overload findOverload (const String& name, const std::vector<Value>& args) const
    {
        Overload result;
        auto bestCost = std::numeric_limits<int>::max();

        for (const auto* function : findFunctions (name))
        {
            if (function->parameterTypes.size() != args.size())
                continue;

            auto cost = 0;

            for (size_t i = 0; i < args.size() && cost >= 0; ++i)
            {
                const auto argumentCost = conversionCost (args[i], function->parameterTypes[i]);
                cost = argumentCost < 0 ? -1 : cost + argumentCost;
            }

            if (cost < 0 || cost > bestCost)
                continue;

            result.numBest = cost == bestCost ? result.numBest + 1 : 1;
            result.function = cost == bestCost ? result.function : function;
            bestCost = cost;
        }

        return result;
    }

    Value callOperator (ExpressionOperator op, std::vector<Value> args, const Expression& e)
    {
        if (op == ExpressionOperator::none)
            fail (e, "Invalid operator");

        return callFunction (Expression::getOperatorFunctionName (op), std::move (args), e);
    }

    Value callFunction (const String& name, std::vector<Value> args, const Expression& e)
    {
        bool hasAbstract = false, hasFloat = false, hasDouble = false, allConstant = true;

        for (const auto& arg : args)
        {
            hasAbstract |= arg.isAbstract;
            hasFloat |= ! arg.isAbstract && arg.type == &ExpressionType::floatType();
            hasDouble |= ! arg.isAbstract && arg.type == &ExpressionType::doubleType();
            allConstant &= getConstant (arg) != nullptr;
        }

        const auto* abstractType = hasFloat && ! hasDouble ? &ExpressionType::floatType() : &ExpressionType::doubleType();

        for (auto& arg : args)
        {
            if (arg.isAbstract)
            {
                arg = convert (arg, abstractType, e);
                arg.isAbstract = false;
            }
        }

        const auto overload = findOverload (name, args);

        if (overload.function == nullptr)
            fail (e, describeMissingOverload (name, args));

        if (overload.numBest > 1)
            fail (e, "The call to " + describeCall (name, args) + " is ambiguous");

        auto result = emitFunction (*overload.function, std::move (args), e);
        result.isAbstract = hasAbstract && allConstant && ! hasFloat && ! hasDouble
                         && result.type == &ExpressionType::doubleType() && getConstant (result) != nullptr;
        return result;
    }

    String describeCall (const String& name, const std::vector<Value>& args) const
    {
        StringArray types;

        for (const auto& arg : args)
            types.add (arg.type->getName());

        if (name.startsWith ("operator"))
            return "'" + name.substring (8) + "' with '" + types.joinIntoString ("' and '") + "'";

        if (name.startsWithChar ('.'))
        {
            types.remove (0);
            return "'" + args.front().type->getName() + name + " (" + types.joinIntoString (", ") + ")'";
        }

        return "'" + name + " (" + types.joinIntoString (", ") + ")'";
    }

    String describeMissingOverload (const String& name, const std::vector<Value>& args) const
    {
        const auto candidates = findFunctions (name);

        if (candidates.empty())
        {
            if (name.startsWithChar ('.'))
                return "The type '" + args.front().type->getName() + "' has no method '" + name.substring (1) + "'";

            return "Unknown function '" + name + "'";
        }

        String message = "No matching overload for " + describeCall (name, args);

        if (! name.startsWith ("operator"))
        {
            StringArray signatures;

            for (const auto* candidate : candidates)
                signatures.add (candidate->getSignature());

            message << ", candidates are: " << signatures.joinIntoString ("; ");
        }

        return message;
    }

    //==============================================================================
    static int getNumericRank (const ExpressionType* type) noexcept
    {
        if (type == &ExpressionType::boolType())
            return 0;

        if (type == &ExpressionType::intType())
            return 1;

        if (type == &ExpressionType::floatType())
            return 2;

        return 3;
    }

    int conversionCost (const Value& value, const ExpressionType* target) const
    {
        if (value.type == target)
            return 0;

        if (value.isAbstract && target == &ExpressionType::floatType())
            return 0;

        if (value.type->isNumeric() && target->isNumeric() && target != &ExpressionType::boolType())
        {
            const auto from = getNumericRank (value.type), to = getNumericRank (target);

            if (to > from)
                return to - from;

            if (from == 3 && to == 2)
                return 10;
        }

        return findConversion (value.type, target) != nullptr ? 20 : -1;
    }

    const ExpressionFunction* findConversion (const ExpressionType* from, const ExpressionType* to) const
    {
        for (const auto* function : findFunctions ("@convert"))
            if (function->parameterTypes.size() == 1 && function->parameterTypes[0] == from && function->resultType == to)
                return function;

        return nullptr;
    }

    Value convert (const Value& value, const ExpressionType* target, const Expression& e)
    {
        if (value.type == target)
            return value;

        if (value.isAbstract && target == &ExpressionType::floatType())
            return makeConstant ((float) *static_cast<const double*> (getConstant (value)));

        if (conversionCost (value, target) < 0)
            fail (e, "Cannot convert '" + value.type->getName() + "' to '" + target->getName() + "'");

        const auto* function = findConversion (value.type, target);
        jassert (function != nullptr);

        return emitFunction (*function, { value }, e);
    }

    //==============================================================================
    Value emitFunction (const ExpressionFunction& function, std::vector<Value> args, const Expression& e)
    {
        for (size_t i = 0; i < args.size(); ++i)
            args[i] = convert (args[i], function.parameterTypes[i], e);

        return emit (function.thunk, function.state, function.resultType, args, function.isPure);
    }

    Value emit (ExpressionThunk thunk, std::shared_ptr<const void> state, const ExpressionType* resultType, const std::vector<Value>& args, bool isPure)
    {
        std::vector<int> inputs;
        std::vector<const void*> constants;

        for (const auto& arg : args)
        {
            inputs.push_back (arg.reg);
            constants.push_back (getConstant (arg));
        }

        if (isPure && std::find (constants.begin(), constants.end(), nullptr) == constants.end())
        {
            auto box = resultType->createValue();
            thunk (state.get(), box.get(), constants.data(), 1);
            return { addRegister (resultType, std::move (box)), resultType };
        }

        if (isPure)
        {
            for (const auto& instruction : program.code)
            {
                if (instruction.isPure && instruction.thunk == thunk && instruction.state == state && instruction.inputs == inputs)
                    return { instruction.output, resultType };
            }
        }

        const auto output = addRegister (resultType);
        program.code.push_back ({ thunk, std::move (state), output, std::move (inputs), isPure });
        return { output, resultType };
    }

    template <class T>
    Value makeConstant (const T& value, bool isAbstract = false)
    {
        const auto* type = ExpressionType::getBuiltin<T>();
        auto box = type->createValue();
        *static_cast<T*> (box.get()) = value;
        return { addRegister (type, std::move (box)), type, isAbstract };
    }

    int addRegister (const ExpressionType* type, std::shared_ptr<void> constant = {})
    {
        program.registers.push_back ({ type, std::move (constant) });
        return (int) program.registers.size() - 1;
    }

    const void* getConstant (const Value& value) const noexcept
    {
        return program.registers[(size_t) value.reg].constant.get();
    }

    //==============================================================================
    void removeUnusedCode()
    {
        std::vector<bool> isLive (program.registers.size(), false);

        for (const auto& output : program.outputs)
            isLive[(size_t) output.reg] = true;

        std::vector<Program::Instruction> liveCode;

        for (auto it = program.code.rbegin(); it != program.code.rend(); ++it)
        {
            if (! isLive[(size_t) it->output])
                continue;

            for (auto reg : it->inputs)
                isLive[(size_t) reg] = true;

            liveCode.push_back (std::move (*it));
        }

        std::reverse (liveCode.begin(), liveCode.end());

        std::vector<int> remapped (program.registers.size(), -1);
        std::vector<Program::Register> liveRegisters;

        for (size_t reg = 0; reg < program.registers.size(); ++reg)
        {
            if (isLive[reg])
            {
                remapped[reg] = (int) liveRegisters.size();
                liveRegisters.push_back (std::move (program.registers[reg]));
            }
        }

        for (auto& instruction : liveCode)
        {
            instruction.output = remapped[(size_t) instruction.output];

            for (auto& reg : instruction.inputs)
                reg = remapped[(size_t) reg];
        }

        for (auto& output : program.outputs)
            output.reg = remapped[(size_t) output.reg];

        for (auto& input : program.inputs)
            if (input.reg >= 0)
                input.reg = remapped[(size_t) input.reg];

        program.code = std::move (liveCode);
        program.registers = std::move (liveRegisters);
    }

    //==============================================================================
    bool fuseInstructions()
    {
        const auto& fused = getFusedThunks();

        std::vector<int> uses (program.registers.size(), 0);
        std::vector<int> producers (program.registers.size(), -1);

        for (const auto& output : program.outputs)
            uses[(size_t) output.reg] += 2;

        for (size_t i = 0; i < program.code.size(); ++i)
        {
            producers[(size_t) program.code[i].output] = (int) i;

            for (auto reg : program.code[i].inputs)
                ++uses[(size_t) reg];
        }

        const auto findSingleUseProducer = [&] (int reg) -> const Program::Instruction*
        {
            const auto producer = producers[(size_t) reg];
            return producer >= 0 && uses[(size_t) reg] == 1 ? &program.code[(size_t) producer] : nullptr;
        };

        bool hasFused = false;

        for (auto& instruction : program.code)
        {
            if (fuseArithmetic (instruction, fused, findSingleUseProducer) || fuseCompareSelect (instruction, fused, findSingleUseProducer))
                hasFused = true;
        }

        return hasFused;
    }

    template <class FindProducer>
    static bool fuseArithmetic (Program::Instruction& instruction, const ExpressionFusedThunks& fused, const FindProducer& findSingleUseProducer)
    {
        for (const auto& thunks : fused.arithmetics)
        {
            if (instruction.thunk != thunks.add && instruction.thunk != thunks.subtract)
                continue;

            for (size_t side = 0; side < 2; ++side)
            {
                const auto* product = findSingleUseProducer (instruction.inputs[side]);
                if (product == nullptr || product->thunk != thunks.multiply)
                    continue;

                if (instruction.thunk == thunks.add)
                    instruction.thunk = thunks.multiplyAdd;
                else
                    instruction.thunk = side == 0 ? thunks.multiplySubtract : thunks.negativeMultiplyAdd;

                instruction.inputs = { product->inputs[0], product->inputs[1], instruction.inputs[1 - side] };
                instruction.state.reset();
                return true;
            }

            return false;
        }

        return false;
    }

    template <class FindProducer>
    bool fuseCompareSelect (Program::Instruction& instruction, const ExpressionFusedThunks& fused, const FindProducer& findSingleUseProducer) const
    {
        const auto* type = program.registers[(size_t) instruction.output].type;

        if (instruction.thunk != type->getSelectThunk() || ! type->isTriviallyCopyable())
            return false;

        const auto* comparison = findSingleUseProducer (instruction.inputs[0]);
        if (comparison == nullptr)
            return false;

        for (const auto& thunks : fused.comparisons)
        {
            if (comparison->thunk != thunks.compare)
                continue;

            const auto select = thunks.findSelect (type->getSize());
            if (select == nullptr)
                return false;

            instruction.thunk = select;
            instruction.inputs = { comparison->inputs[0], comparison->inputs[1], instruction.inputs[1], instruction.inputs[2] };
            instruction.state.reset();
            return true;
        }

        return false;
    }

    static const ExpressionFusedThunks& getFusedThunks()
    {
        static const ExpressionFusedThunks thunks = []
        {
            using Fused = ExpressionFusedThunks;

            const auto findBuiltin = [] (ExpressionOperator op, const ExpressionType& type) -> ExpressionThunk
            {
                const auto& builtins = ExpressionContext::getBuiltinFunctions();
                const auto it = builtins.find (Expression::getOperatorFunctionName (op));
                if (it == builtins.end())
                    return nullptr;

                for (const auto& function : it->second)
                {
                    if (function.parameterTypes == std::vector<const ExpressionType*> { &type, &type })
                        return function.thunk;
                }

                return nullptr;
            };

            Fused result;

            const auto addArithmetic = [&] (auto tag)
            {
                using T = typename decltype (tag)::type;
                const auto& type = *ExpressionType::getBuiltin<T>();

                result.arithmetics.push_back ({ findBuiltin (ExpressionOperator::multiply, type),
                                                findBuiltin (ExpressionOperator::add, type),
                                                findBuiltin (ExpressionOperator::subtract, type),
                                                &Fused::arithmetic<T, Fused::Arithmetic::multiplyAdd>,
                                                &Fused::arithmetic<T, Fused::Arithmetic::multiplySubtract>,
                                                &Fused::arithmetic<T, Fused::Arithmetic::negativeMultiplyAdd> });
            };

            const auto addComparison = [&] (auto tag, ExpressionOperator op, auto compare)
            {
                using T = typename decltype (tag)::type;
                using Compare = decltype (compare);

                result.comparisons.push_back ({ findBuiltin (op, *ExpressionType::getBuiltin<T>()),
                                                { { { 1, &Fused::compareSelect<Compare, T, Fused::Bytes<1>> },
                                                    { 4, &Fused::compareSelect<Compare, T, Fused::Bytes<4>> },
                                                    { 8, &Fused::compareSelect<Compare, T, Fused::Bytes<8>> },
                                                    { 16, &Fused::compareSelect<Compare, T, Fused::Bytes<16>> } } } });
            };

            const auto addComparisons = [&] (auto tag)
            {
                addComparison (tag, ExpressionOperator::equal, std::equal_to<>());
                addComparison (tag, ExpressionOperator::notEqual, std::not_equal_to<>());
                addComparison (tag, ExpressionOperator::less, std::less<>());
                addComparison (tag, ExpressionOperator::lessOrEqual, std::less_equal<>());
                addComparison (tag, ExpressionOperator::greater, std::greater<>());
                addComparison (tag, ExpressionOperator::greaterOrEqual, std::greater_equal<>());
            };

            addArithmetic (std::type_identity<float>());
            addArithmetic (std::type_identity<double>());
            addComparisons (std::type_identity<int>());
            addComparisons (std::type_identity<float>());
            addComparisons (std::type_identity<double>());

            jassert (std::all_of (result.arithmetics.begin(), result.arithmetics.end(), [] (const auto& thunks)
            {
                return thunks.multiply != nullptr && thunks.add != nullptr && thunks.subtract != nullptr;
            }));

            jassert (std::all_of (result.comparisons.begin(), result.comparisons.end(), [] (const auto& thunks)
            {
                return thunks.compare != nullptr;
            }));

            return result;
        }();

        return thunks;
    }

    //==============================================================================
    [[noreturn]] void fail (const Expression& e, const String& message) const
    {
        throw Error { message, e.getPosition() };
    }

    const ExpressionContext& context;
    Program& program;
    std::vector<std::pair<String, Value>> locals;
    std::unordered_map<int, Value> namedExpressionValues;
    std::vector<int> namedExpressionStack;
    const ExpressionType* expectedType = nullptr;
    int depth = 0;
};

//==============================================================================
CompiledExpression::CompiledExpression()
    : program (std::make_shared<Program>())
{
}

ResultValue<CompiledExpression> CompiledExpression::compile (StringRef text, const ExpressionContext& context)
{
    auto expression = Expression::parse (text);

    if (expression.failed())
        return makeResultValueFail (expression.getErrorMessage());

    return compile (expression.getReference(), context);
}

ResultValue<CompiledExpression> CompiledExpression::compile (const Expression& expression, const ExpressionContext& context)
{
    auto newProgram = std::make_shared<Program>();

    try
    {
        ExpressionCompiler compiler (context, *newProgram);
        compiler.compileProgram (expression);
    }
    catch (const ExpressionCompiler::Error& error)
    {
        if (error.position.first <= 0)
            return makeResultValueFail (error.message);

        return makeResultValueFail (String (error.position.first) + ":" + String (error.position.second) + ": " + error.message);
    }

    CompiledExpression result;
    result.program = std::move (newProgram);
    return makeResultValueOk (std::move (result));
}

//==============================================================================
int CompiledExpression::getNumOutputs() const noexcept
{
    return (int) program->outputs.size();
}

const String& CompiledExpression::getOutputName (int index) const
{
    jassert (isPositiveAndBelow (index, getNumOutputs()));
    return program->outputs[(size_t) index].name;
}

const ExpressionType* CompiledExpression::getOutputType (int index) const
{
    return isPositiveAndBelow (index, getNumOutputs()) ? program->outputs[(size_t) index].type : nullptr;
}

int CompiledExpression::findOutput (StringRef name) const
{
    for (size_t i = 0; i < program->outputs.size(); ++i)
        if (program->outputs[i].name == name)
            return (int) i;

    return -1;
}

int CompiledExpression::getNumInputs() const noexcept
{
    return (int) program->inputs.size();
}

const String& CompiledExpression::getInputName (int index) const
{
    jassert (isPositiveAndBelow (index, getNumInputs()));
    return program->inputs[(size_t) index].name;
}

const ExpressionType* CompiledExpression::getInputType (int index) const
{
    return isPositiveAndBelow (index, getNumInputs()) ? program->inputs[(size_t) index].type : nullptr;
}

int CompiledExpression::findInput (StringRef name) const
{
    for (size_t i = 0; i < program->inputs.size(); ++i)
        if (program->inputs[i].name == name)
            return (int) i;

    return -1;
}

bool CompiledExpression::usesInput (int index) const
{
    return isPositiveAndBelow (index, getNumInputs()) && program->inputs[(size_t) index].reg >= 0;
}

//==============================================================================
bool CompiledExpression::isConstant() const noexcept
{
    return std::all_of (program->outputs.begin(), program->outputs.end(), [this] (const auto& output)
    {
        return program->registers[(size_t) output.reg].constant != nullptr;
    });
}

bool CompiledExpression::isAllocationFree() const noexcept
{
    return std::all_of (program->registers.begin(), program->registers.end(), [] (const auto& reg)
    {
        return reg.type->isTriviallyCopyable();
    });
}

int CompiledExpression::getNumInstructions() const noexcept
{
    return (int) program->code.size();
}

} // namespace yup
