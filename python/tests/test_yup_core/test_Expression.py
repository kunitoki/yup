import pytest

import yup

#==================================================================================================

def test_default_construction_evaluates_to_zero():
    assert yup.Expression().evaluate() == 0.0

#==================================================================================================

def test_constant_expression():
    assert yup.Expression(2.5).evaluate() == 2.5

#==================================================================================================

def test_parsed_arithmetic_respects_precedence():
    assert yup.Expression("1 + 2 * 3").evaluate() == 7.0
    assert yup.Expression("(1 + 2) * 3").evaluate() == 9.0

#==================================================================================================

def test_unparseable_text_raises_value_error():
    with pytest.raises(ValueError):
        yup.Expression("1 +")

#==================================================================================================

def test_arithmetic_operators():
    two = yup.Expression(2.0)
    three = yup.Expression(3.0)

    assert (two + three).evaluate() == 5.0
    assert (three - two).evaluate() == 1.0
    assert (two * three).evaluate() == 6.0
    assert (three / two).evaluate() == 1.5
    assert (-two).evaluate() == -2.0

#==================================================================================================

def test_to_string_describes_the_expression():
    assert yup.Expression("1 + 2").toString() != ""
    assert yup.Expression(2.5).toString() != ""

#==================================================================================================

def test_uses_any_symbols():
    assert not yup.Expression("1 + 2").usesAnySymbols()
    assert yup.Expression.symbol("x").usesAnySymbols()

#==================================================================================================

def test_built_in_function_without_a_scope():
    call = yup.Expression.function("sin", [yup.Expression(0.0)])

    assert call.evaluate() == 0.0

#==================================================================================================

class _ConstantScope(yup.Expression.Scope):
    """Resolves any symbol to a fixed value, so symbol arithmetic can be pinned."""

    def __init__(self, value):
        super().__init__()
        self.value = value

    def getScopeUID(self):
        return "python-test-scope"

    def getSymbolValue(self, symbol):
        return yup.Expression(self.value)


#==================================================================================================

def test_python_scope_resolves_symbols():
    scope = _ConstantScope(4.0)

    assert yup.Expression("x * 2").evaluate(scope) == 8.0

#==================================================================================================

def test_scope_uid_comes_from_python():
    assert _ConstantScope(1.0).getScopeUID() == "python-test-scope"

#==================================================================================================

def test_adjusted_to_give_new_result_uses_the_scope():
    scope = _ConstantScope(5.0)

    adjusted = yup.Expression("x + 10").adjustedToGiveNewResult(8.0, scope)

    assert adjusted.evaluate(scope) == 8.0
