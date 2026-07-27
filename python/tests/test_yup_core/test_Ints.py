import pytest

import yup

#==================================================================================================

def test_ints_constructor_positive():
    a = yup.int8(1)
    assert a.get() == 1

    a = yup.int16(1)
    assert a.get() == 1

    a = yup.int32(1)
    assert a.get() == 1

    a = yup.int64(1)
    assert a.get() == 1

#==================================================================================================

def test_ints_constructor_negative():
    a = yup.int8(-1)
    assert a.get() == -1

    a = yup.int16(-1)
    assert a.get() == -1

    a = yup.int32(-1)
    assert a.get() == -1

    a = yup.int64(-1)
    assert a.get() == -1

#==================================================================================================

def test_ints_constructor_out_of_range():
    with pytest.raises(TypeError):
        a = yup.int8(1_024)

    with pytest.raises(TypeError):
        a = yup.int16(65_537)

    with pytest.raises(TypeError):
        a = yup.int32(2_147_483_648)

    with pytest.raises(TypeError):
        a = yup.int64(9_223_372_036_854_775_808)

#==================================================================================================

def test_uints_constructor_positive():
    a = yup.uint8(1)
    assert a.get() == 1

    a = yup.uint16(1)
    assert a.get() == 1

    a = yup.uint32(1)
    assert a.get() == 1

    a = yup.uint64(1)
    assert a.get() == 1

#==================================================================================================

def test_uints_constructor_negative():
    with pytest.raises(TypeError):
        a = yup.uint8(-1)

    with pytest.raises(TypeError):
        a = yup.uint16(-1)

    with pytest.raises(TypeError):
        a = yup.uint32(-1)

    with pytest.raises(TypeError):
        a = yup.uint64(-1)

#==================================================================================================

def test_uints_constructor_out_of_range():
    with pytest.raises(TypeError):
        a = yup.uint8(1_024)

    with pytest.raises(TypeError):
        a = yup.uint16(65_537)

    with pytest.raises(TypeError):
        a = yup.uint32(4_294_967_296)

    with pytest.raises(TypeError):
        a = yup.uint64(18_446_744_073_709_551_616)
