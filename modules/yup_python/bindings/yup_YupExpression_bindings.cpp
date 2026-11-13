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

#include "yup_YupExpression_bindings.h"

#define YUP_PYTHON_INCLUDE_PYBIND11_OPERATORS
#define YUP_PYTHON_INCLUDE_PYBIND11_FUNCTIONAL
#define YUP_PYTHON_INCLUDE_PYBIND11_STL
#include "../utilities/yup_PyBind11Includes.h"

#include <optional>

//==============================================================================

namespace yup::Bindings
{

namespace py = pybind11;
using namespace py::literals;

void registerYupExpressionBindings (py::module_& m)
{
    // clang-format off

    // ============================================================================================ yup::Expression

    auto throwOnFailure = [] (const auto& result)
    {
        if (result.failed())
            throw py::value_error (result.getErrorMessage().toStdString());
    };

    auto findTypeOrThrow = [] (const ExpressionContext& context, const String& typeName) -> const ExpressionType&
    {
        if (auto* type = context.findType (typeName))
            return *type;

        throw py::value_error (("Unknown type '" + typeName + "'").toStdString());
    };

    auto expressionFromObject = [] (py::handle value)
    {
        if (py::isinstance<py::bool_> (value))
            return Expression (value.cast<bool>());

        if (py::isinstance<py::int_> (value))
            return Expression (value.cast<int>());

        if (py::isinstance<py::float_> (value))
            return Expression (value.cast<double>());

        if (py::isinstance<py::str> (value))
            return Expression::text (value.cast<String>());

        return value.cast<Expression>();
    };

    py::class_<Expression> classExpression (m, "Expression");

    classExpression
        .def (py::init<>())
        .def (py::init ([expressionFromObject] (py::object value) { return expressionFromObject (value); }), "value"_a)
        .def_static ("parse", [throwOnFailure] (const String& text)
        {
            auto result = Expression::parse (text);
            throwOnFailure (result);
            return result.getValue();
        }, "text"_a)
        .def_static ("parseExpression", [] (const String& text, int index)
        {
            // Python indexes characters, the parser reports UTF-8 byte offsets
            const auto start = CharPointer_UTF8 (text.toRawUTF8());
            auto end = start;

            for (int i = 0; i < index && ! end.isEmpty(); ++i)
                ++end;

            auto offset = (int) (end.getAddress() - start.getAddress());
            auto result = Expression::parseExpression (text, offset);
            const auto newIndex = (int) start.lengthUpTo (CharPointer_UTF8 (start.getAddress() + offset));

            if (result.failed())
            {
                PyErr_SetObject (PyExc_ValueError, py::make_tuple (result.getErrorMessage(), newIndex).ptr());
                throw py::error_already_set();
            }

            return py::make_tuple (result.getValue(), newIndex);
        }, "text"_a, "index"_a = 0)
        .def_static ("text", &Expression::text, "value"_a)
        .def_static ("symbol", &Expression::symbol, "name"_a)
        .def_static ("call", [expressionFromObject] (const String& functionName, py::list arguments)
        {
            std::vector<Expression> argumentList;

            for (auto item : arguments)
                argumentList.push_back (expressionFromObject (item));

            return Expression::call (functionName, std::move (argumentList));
        }, "functionName"_a, "arguments"_a)
        .def ("toString", &Expression::toString)
        .def ("__str__", &Expression::toString)
        .def ("getName", &Expression::getName)
        .def ("getNumInputs", &Expression::getNumInputs)
        .def ("getInput", &Expression::getInput, "index"_a)
        .def ("getReferencedSymbols", [] (const Expression& self)
        {
            py::list result;

            for (const auto& symbol : self.getReferencedSymbols())
                result.append (symbol);

            return result;
        })
        .def ("referencesSymbol", [] (const Expression& self, const String& name) { return self.referencesSymbol (name); }, "name"_a)
        .def ("withRenamedSymbol", [] (const Expression& self, const String& oldName, const String& newName) { return self.withRenamedSymbol (oldName, newName); }, "oldName"_a, "newName"_a)
        .def ("evaluate", [throwOnFailure] (const Expression& self)
        {
            auto result = self.evaluate();
            throwOnFailure (result);
            return result.getValue();
        })
        .def ("evaluate", [throwOnFailure] (const Expression& self, const ExpressionContext& context)
        {
            auto result = self.evaluate (context);
            throwOnFailure (result);
            return result.getValue();
        }, "context"_a)
        .def (py::self + py::self)
        .def (py::self - py::self)
        .def (py::self * py::self)
        .def (py::self / py::self)
        .def (py::self % py::self)
        .def (- py::self);

    // ============================================================================================ yup::ExpressionContext

    py::class_<ExpressionContext> classExpressionContext (m, "ExpressionContext");

    classExpressionContext
        .def (py::init<>())
        .def ("addInput", [findTypeOrThrow] (ExpressionContext& self, const String& name, const String& typeName)
        {
            return self.addInput (name, findTypeOrThrow (self, typeName));
        }, "name"_a, "typeName"_a = "double")
        .def ("addConstant", [] (ExpressionContext& self, const String& name, py::object value)
        {
            if (py::isinstance<py::bool_> (value))
                self.addConstant (name, value.cast<bool>());
            else if (py::isinstance<py::int_> (value))
                self.addConstant (name, value.cast<int>());
            else if (py::isinstance<py::float_> (value))
                self.addConstant (name, value.cast<double>());
            else if (py::isinstance<py::str> (value))
                self.addConstant (name, value.cast<String>());
            else
                throw py::type_error ("Constants must be bool, int, float or str");
        }, "name"_a, "value"_a)
        .def ("addExpression", [throwOnFailure] (ExpressionContext& self, const String& name, const String& text)
        {
            throwOnFailure (self.addExpression (name, text));
        }, "name"_a, "text"_a)
        .def ("addExpression", [] (ExpressionContext& self, const String& name, const Expression& expression)
        {
            self.addExpression (name, expression);
        }, "name"_a, "expression"_a)
        .def ("declareOutput", [findTypeOrThrow] (ExpressionContext& self, const String& name, const String& typeName)
        {
            self.declareOutput (name, findTypeOrThrow (self, typeName));
        }, "name"_a, "typeName"_a)
        .def ("getNumInputs", &ExpressionContext::getNumInputs)
        .def ("getInputName", &ExpressionContext::getInputName, "index"_a)
        .def ("findInput", [] (const ExpressionContext& self, const String& name) { return self.findInput (name); }, "name"_a)
        .def ("setSymbolResolver", [] (ExpressionContext& self, py::object resolver)
        {
            if (resolver.is_none())
            {
                self.setSymbolResolver (nullptr);
                return;
            }

            // Python contexts only hold built-in types, so the resolver doesn't need to reference the context
            self.setSymbolResolver ([resolver = std::move (resolver)] (const String& name) -> const ExpressionType*
            {
                py::gil_scoped_acquire gil;

                const auto typeName = resolver (name);

                if (typeName.is_none())
                    return nullptr;

                if (auto* type = ExpressionType::findBuiltin (typeName.cast<String>()))
                    return type;

                throw py::value_error (("Unknown type '" + typeName.cast<String>() + "' for symbol '" + name + "'").toStdString());
            });
        }, "resolver"_a);

    // ============================================================================================ yup::CompiledExpression

    py::class_<CompiledExpression> classCompiledExpression (m, "CompiledExpression");

    classCompiledExpression
        .def (py::init<>())
        .def_static ("compile", [throwOnFailure] (const String& text, const ExpressionContext& context)
        {
            auto result = CompiledExpression::compile (text, context);
            throwOnFailure (result);
            return result.getValue();
        }, "text"_a, "context"_a = ExpressionContext())
        .def_static ("compile", [throwOnFailure] (const Expression& expression, const ExpressionContext& context)
        {
            auto result = CompiledExpression::compile (expression, context);
            throwOnFailure (result);
            return result.getValue();
        }, "expression"_a, "context"_a = ExpressionContext())
        .def ("getNumOutputs", &CompiledExpression::getNumOutputs)
        .def ("getOutputName", &CompiledExpression::getOutputName, "index"_a)
        .def ("getOutputTypeName", [] (const CompiledExpression& self, int index)
        {
            auto* type = self.getOutputType (index);
            return type != nullptr ? type->getName() : String();
        }, "index"_a)
        .def ("findOutput", [] (const CompiledExpression& self, const String& name) { return self.findOutput (name); }, "name"_a)
        .def ("getNumInputs", &CompiledExpression::getNumInputs)
        .def ("getInputName", &CompiledExpression::getInputName, "index"_a)
        .def ("getInputTypeName", [] (const CompiledExpression& self, int index)
        {
            auto* type = self.getInputType (index);
            return type != nullptr ? type->getName() : String();
        }, "index"_a)
        .def ("findInput", [] (const CompiledExpression& self, const String& name) { return self.findInput (name); }, "name"_a)
        .def ("usesInput", &CompiledExpression::usesInput, "index"_a)
        .def ("isConstant", &CompiledExpression::isConstant)
        .def ("isAllocationFree", &CompiledExpression::isAllocationFree)
        .def ("getNumInstructions", &CompiledExpression::getNumInstructions);

    // ============================================================================================ yup::ExpressionState

    py::class_<ExpressionState> classExpressionState (m, "ExpressionState");

    classExpressionState
        .def (py::init<const CompiledExpression&, int>(), "program"_a, "blockSize"_a = 64)
        .def ("getProgram", &ExpressionState::getProgram, py::return_value_policy::reference_internal)
        .def ("getBlockSize", &ExpressionState::getBlockSize)
        .def ("set", [throwOnFailure] (ExpressionState& self, const String& name, const var& value)
        {
            throwOnFailure (self.setVar (StringRef (name), value));
        }, "name"_a, "value"_a)
        .def ("set", [throwOnFailure] (ExpressionState& self, int index, const var& value)
        {
            throwOnFailure (self.setVar (index, value));
        }, "index"_a, "value"_a)
        .def ("get", [] (const ExpressionState& self, const String& name, int lane)
        {
            const auto index = self.getProgram().findOutput (name);

            if (index < 0)
                throw py::key_error (("Unknown output '" + name + "'").toStdString());

            return self.getVar (index, lane);
        }, "name"_a, "lane"_a = 0)
        .def ("get", &ExpressionState::getVar, "index"_a, "lane"_a = 0)
        .def ("evaluate", &ExpressionState::evaluate, "row"_a = 0)
        .def ("evaluateBlock", &ExpressionState::evaluateBlock, "startRow"_a, "numRows"_a)
        .def ("hasChanged", [] (const ExpressionState& self, std::optional<int> index)
        {
            return index ? self.hasOutputChanged (*index) : self.hasChanged();
        }, "index"_a = py::none());

    // clang-format on
}

} // namespace yup::Bindings
