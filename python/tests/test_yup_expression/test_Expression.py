import pytest

import yup

#==================================================================================================

def test_default_construction_evaluates_to_zero():
    assert yup.Expression().evaluate() == 0.0

#==================================================================================================

def test_constant_expressions():
    assert yup.Expression(2.5).evaluate() == 2.5
    assert yup.Expression(7).evaluate() == 7.0
    assert yup.Expression(True).evaluate() == 1.0

#==================================================================================================

def test_parsed_arithmetic_respects_precedence():
    assert yup.Expression.parse("1 + 2 * 3").evaluate() == 7.0
    assert yup.Expression.parse("(1 + 2) * 3").evaluate() == 9.0
    assert yup.Expression.parse("-2 ^ 2").evaluate() == -4.0

#==================================================================================================

def test_ternary_and_locals():
    assert yup.Expression.parse("let a = 3; a > 2 ? a * 2 : 0").evaluate() == 6.0

#==================================================================================================

def test_parse_error_raises_value_error():
    with pytest.raises(ValueError):
        yup.Expression.parse("1 +")

#==================================================================================================

def test_unknown_symbol_raises_value_error_on_evaluate():
    with pytest.raises(ValueError):
        yup.Expression.symbol("missing").evaluate()

#==================================================================================================

def test_builder_operators():
    two = yup.Expression(2)
    three = yup.Expression(3)

    assert (two + three).evaluate() == 5.0
    assert (two - three).evaluate() == -1.0
    assert (two * three).evaluate() == 6.0
    assert (yup.Expression(7) % three).evaluate() == 1.0
    assert (yup.Expression(3.0) / yup.Expression(2.0)).evaluate() == 1.5
    assert (-two).evaluate() == -2.0

#==================================================================================================

def test_to_string_and_call():
    assert str((yup.Expression(1) + yup.Expression(2)) * yup.Expression(3)) == "(1 + 2) * 3"
    assert yup.Expression.call("max", [1, 2]).toString() == "max (1, 2)"
    assert yup.Expression.text("hi").toString() == "\"hi\""
    assert yup.Expression.call("max", [1.5, 2.5]).evaluate() == 2.5

#==================================================================================================

def test_symbol_introspection_and_renaming():
    expression = yup.Expression.parse("let k = a * 2; out = k + b.c + sin (d); z = out")
    assert expression.getReferencedSymbols() == ["a", "b.c", "d"]
    assert yup.Expression.parse("x + 1").referencesSymbol("x")
    assert yup.Expression.parse("a + b").withRenamedSymbol("a", "z").toString() == "z + b"

#==================================================================================================

def test_compile_once_evaluate_many():
    context = yup.ExpressionContext()
    x = context.addInput("x", "float")
    context.addConstant("k", 2)

    program = yup.CompiledExpression.compile("x * k", context)
    assert program.getOutputName(0) == "result"
    assert program.getOutputTypeName(0) == "float"
    assert not program.isConstant()
    assert program.usesInput(x)

    state = yup.ExpressionState(program)

    for value in range(4):
        state.set("x", value + 0.5)
        state.evaluate()
        assert state.get("result") == pytest.approx((value + 0.5) * 2)

    state.set(x, 10)
    state.evaluate()
    assert state.get(0) == pytest.approx(20.0)

#==================================================================================================

def test_reports_changed_outputs():
    context = yup.ExpressionContext()
    context.addInput("x", "float")

    state = yup.ExpressionState(yup.CompiledExpression.compile("a = x * 2; b = x > 1", context))
    state.set("x", 2.0)
    state.evaluate()
    assert state.hasChanged()

    state.set("x", 2.0)
    state.evaluate()
    assert not state.hasChanged()

    state.set("x", 3.0)
    state.evaluate()
    assert state.hasChanged(0)
    assert not state.hasChanged(1)

#==================================================================================================

def test_multiple_outputs_and_sub_expressions():
    context = yup.ExpressionContext()
    context.addInput("x", "double")
    context.addExpression("twice", "x * 2")
    context.declareOutput("visible", "bool")

    program = yup.CompiledExpression.compile("let a = twice + 1; y = a; visible = x > 0", context)
    assert program.getNumOutputs() == 2
    assert program.findOutput("missing") < 0

    state = yup.ExpressionState(program)
    state.set("x", 3.0)
    state.evaluate()
    assert state.get("y") == 7.0
    assert state.get("visible") is True

    with pytest.raises(KeyError):
        state.get("missing")

#==================================================================================================

def test_string_inputs():
    context = yup.ExpressionContext()
    context.addInput("name", "string")

    state = yup.ExpressionState(yup.CompiledExpression.compile("\"hi \" + name", context))
    state.set("name", "yup")
    state.evaluate()
    assert state.get("result") == "hi yup"

#==================================================================================================

def test_constant_program_and_errors():
    assert yup.CompiledExpression.compile("1 + 2").isConstant()

    context = yup.ExpressionContext()

    with pytest.raises(ValueError):
        context.addInput("x", "unknown")

    with pytest.raises(ValueError):
        yup.CompiledExpression.compile("1 + missing", context)

    with pytest.raises(ValueError):
        context.addExpression("broken", "1 +")

    state = yup.ExpressionState(yup.CompiledExpression.compile("1"))

    with pytest.raises(ValueError):
        state.set("missing", 1)

#==================================================================================================

def test_parse_expression_embedded_in_text():
    text = "fill: a > 1 ? 2 : b; né: f (x) }"

    expression, index = yup.Expression.parseExpression(text, 6)
    assert expression.toString() == "a > 1 ? 2 : b"
    assert index == text.index(";")

    expression, index = yup.Expression.parseExpression(text, text.index("f ("))
    assert expression.toString() == "f (x)"
    assert index == text.index("}")

    with pytest.raises(ValueError) as error:
        yup.Expression.parseExpression("a + * b", 0)

    assert error.value.args[1] == 4

#==================================================================================================

def test_symbol_resolver_declares_inputs():
    context = yup.ExpressionContext()
    context.addInput("x", "double")
    context.setSymbolResolver(lambda name: "float" if name.startswith("model.") else None)

    program = yup.CompiledExpression.compile("model.gain * 2 + x", context)
    assert program.getNumInputs() == 2
    assert program.getInputName(1) == "model.gain"
    assert program.getInputTypeName(1) == "float"
    assert program.findInput("model.gain") == 1

    state = yup.ExpressionState(program)
    state.set("model.gain", 3.0)
    state.set("x", 1.0)
    state.evaluate()
    assert state.get("result") == pytest.approx(7.0)

    with pytest.raises(ValueError):
        yup.CompiledExpression.compile("other + 1", context)

    context.setSymbolResolver(lambda name: "unknown")

    with pytest.raises(ValueError):
        yup.CompiledExpression.compile("other + 1", context)

    context.setSymbolResolver(None)

    with pytest.raises(ValueError):
        yup.CompiledExpression.compile("model.gain", context)
