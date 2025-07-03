import yup

#==================================================================================================

def test_empty_constructor():
    a = yup.Identifier()
    assert a.isNull()
    assert not a.isValid()

    b = yup.Identifier()
    assert a == b

    c = yup.Identifier("123")
    assert a != c
    assert b != c
    assert c != a
    assert c != b

#==================================================================================================

def test_constructor_python_str():
    a = yup.Identifier("abc")
    assert not a.isNull()
    assert a.isValid()

    b = yup.Identifier("abc")
    assert a == b

#==================================================================================================

def test_constructor_copy():
    a = yup.Identifier("abc")
    b = yup.Identifier(a)
    assert a.isNull() == b.isNull()
    assert a.isValid() == b.isValid()
    assert a.toString() == b.toString()

#==================================================================================================

def test_constructor_juce_String():
    a = yup.Identifier("abc")
    assert not a.isNull()
    assert a.isValid()

    b = yup.Identifier("abc")
    assert a == b

#==================================================================================================

def test_comparisons():
    a = yup.Identifier("abc")
    b = yup.Identifier("abc")
    c = yup.Identifier("abcd")
    assert a == b
    assert a <= b
    assert a >= b
    assert not (a < b)
    assert not (a > b)

    assert a != c
    assert a <= c
    assert not (a >= c)
    assert a < c
    assert not (a > c)

#==================================================================================================

def test_to_string():
    a = yup.Identifier("abc")
    b = yup.Identifier("abc")
    assert a.toString() == b.toString()

    c = yup.Identifier("123")
    assert a.toString() != c.toString()

#==================================================================================================

def test_is_valid_identifier():
    assert not yup.Identifier.isValidIdentifier("")
    assert not yup.Identifier.isValidIdentifier(" ")
    assert yup.Identifier.isValidIdentifier("abcf123")
