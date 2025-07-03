
import sys

import yup

#==================================================================================================

def test_swap_python_integer():
    a = 1337
    assert a != yup.ByteOrder.swap(a)
    assert a == yup.ByteOrder.swap(yup.ByteOrder.swap(a))

#==================================================================================================

def test_little_endian_short_python_integer():
    assert yup.ByteOrder.littleEndianShort(b'\x01\x00') == 1
    assert yup.ByteOrder.littleEndianShort(b'\xff\x00') == 255

#==================================================================================================

def test_little_endian_int_python_integer():
    assert yup.ByteOrder.littleEndianInt(b'\x01\x00\x00\00') == 1
    assert yup.ByteOrder.littleEndianInt(b'\xff\x00\x00\00') == 255

#==================================================================================================

def test_little_endian_int64_python_integer():
    assert yup.ByteOrder.littleEndianInt64(b'\x01\x00\x00\00\x00\x00\x00\x00') == 1
    assert yup.ByteOrder.littleEndianInt64(b'\xff\x00\x00\00\x00\x00\x00\x00') == 255

#==================================================================================================

def test_big_endian_short_python_integer():
    assert yup.ByteOrder.bigEndianShort(b'\x00\01') == 1
    assert yup.ByteOrder.bigEndianShort(b'\x00\xff') == 255

#==================================================================================================

def test_big_endian_int_python_integer():
    assert yup.ByteOrder.bigEndianInt(b'\x00\x00\x00\01') == 1
    assert yup.ByteOrder.bigEndianInt(b'\x00\x00\x00\xff') == 255

#==================================================================================================

def test_big_endian_int64_python_integer():
    assert yup.ByteOrder.bigEndianInt64(b'\x00\x00\x00\x00\x00\x00\x00\01') == 1
    assert yup.ByteOrder.bigEndianInt64(b'\x00\x00\x00\x00\x00\x00\x00\xff') == 255

#==================================================================================================

def test_make_int():
    assert yup.ByteOrder.makeInt(1, 0) == 1
    assert yup.ByteOrder.makeInt(255, 0) == 255
    assert yup.ByteOrder.makeInt(0, 1) == 256
    assert yup.ByteOrder.makeInt(255, 1) == 511
    assert yup.ByteOrder.makeInt(1, 0, 0, 0) == 1
    assert yup.ByteOrder.makeInt(255, 0, 0, 0) == 255
    assert yup.ByteOrder.makeInt(0, 0, 0, 1) == 16777216
    assert yup.ByteOrder.makeInt(255, 0, 0, 1) == 16777471
    assert yup.ByteOrder.makeInt(1, 0, 0, 0, 0, 0, 0, 0) == 1
    assert yup.ByteOrder.makeInt(255, 0, 0, 0, 0, 0, 0, 0) == 255
    assert yup.ByteOrder.makeInt(0, 0, 0, 0, 0, 0, 0, 1) == 72057594037927936
    assert yup.ByteOrder.makeInt(255, 0, 0, 0, 0, 0, 0, 1) == 72057594037928191

#==================================================================================================

def test_is_big_endian():
    is_big_endian = sys.byteorder == "big"
    assert is_big_endian == yup.ByteOrder.isBigEndian()
