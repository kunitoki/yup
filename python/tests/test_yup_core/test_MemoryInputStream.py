import os
import pytest

import yup

#==================================================================================================

def test_constructor_with_empty_source_data():
    source_data = b""

    stream = yup.MemoryInputStream(source_data, False)
    assert stream.getData() == source_data
    assert stream.getDataSize() == len(source_data)

    stream = yup.MemoryInputStream(source_data, True)
    assert stream.getData() == source_data
    assert stream.getDataSize() == len(source_data)

#==================================================================================================

def test_constructor_with_source_data():
    source_data = b"Hello, world!"

    stream = yup.MemoryInputStream(source_data, False)
    assert stream.getData() == source_data
    assert stream.getDataSize() == len(source_data)

    stream = yup.MemoryInputStream(source_data, True)
    assert stream.getData() == source_data
    assert stream.getDataSize() == len(source_data)

#==================================================================================================

def test_constructor_with_empty_memory_block():
    source_data = b""
    memory_block = yup.MemoryBlock(source_data)

    stream = yup.MemoryInputStream(memory_block, False)
    assert stream.getData() == source_data
    assert stream.getDataSize() == len(source_data)

    stream = yup.MemoryInputStream(memory_block, True)
    assert stream.getData() == source_data
    assert stream.getDataSize() == len(source_data)

#==================================================================================================

def test_constructor_with_memory_block():
    source_data = b"Hello, world!"
    memory_block = yup.MemoryBlock(source_data)

    stream = yup.MemoryInputStream(memory_block, False)
    assert stream.getData() == source_data
    assert stream.getDataSize() == len(source_data)

    stream = yup.MemoryInputStream(memory_block, True)
    assert stream.getData() == source_data
    assert stream.getDataSize() == len(source_data)

#==================================================================================================

def test_memory_input_stream_construct_from_data():
    data = b"hello world"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.getTotalLength() == len(data)

#==================================================================================================

def test_memory_input_stream_construct_from_memory_block():
    data = b"hello world"
    memory_block = yup.MemoryBlock(data)
    input_stream = yup.MemoryInputStream(memory_block, False)
    assert input_stream.getTotalLength() == len(data)

#==================================================================================================

def test_memory_input_stream_get_position():
    data = b"hello world"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.getPosition() == 0

#==================================================================================================

def test_memory_input_stream_set_position():
    data = b"hello world"
    input_stream = yup.MemoryInputStream(data, False)
    new_position = 5
    input_stream.setPosition(new_position)
    assert input_stream.getPosition() == new_position

#==================================================================================================

def test_memory_input_stream_get_total_length():
    data = b"hello world"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.getTotalLength() == len(data)

#==================================================================================================

def test_memory_input_stream_is_exhausted():
    data = b"hello world"
    input_stream = yup.MemoryInputStream(data, False)
    assert not input_stream.isExhausted()

#==================================================================================================

def test_memory_input_stream_read():
    data = b"hello world"
    input_stream = yup.MemoryInputStream(data, False)
    buffer = bytearray(len(data))
    bytes_read = input_stream.read(buffer)
    assert bytes_read == len(data)
    assert buffer == data

#==================================================================================================

def test_memory_input_stream_skip_next_bytes():
    data = b"hello world"
    input_stream = yup.MemoryInputStream(data, False)
    num_bytes_to_skip = 5
    input_stream.skipNextBytes(num_bytes_to_skip)
    assert input_stream.getPosition() == num_bytes_to_skip

#==================================================================================================

def test_memory_input_stream_read_into_memory_block():
    data = b"hello world"
    input_stream = yup.MemoryInputStream(data, False)
    memory_block = yup.MemoryBlock()
    max_num_bytes_to_read = len(data)
    bytes_read = input_stream.readIntoMemoryBlock(memory_block, max_num_bytes_to_read)
    assert bytes_read == len(data)
    assert memory_block.getData() == data

#==================================================================================================

def test_memory_input_stream_read_next_line():
    data = b"hello\nworld"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readNextLine() == "hello"

#==================================================================================================

def test_memory_input_stream_read_string():
    data = b"hello\x00world"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readString() == "hello"

#==================================================================================================

def test_memory_input_stream_read_entire_stream_as_string():
    data = b"hello world"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readEntireStreamAsString() == "hello world"

#==================================================================================================

def test_memory_input_stream_read_byte():
    data = b"\x01\x02\x03\x04"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readByte() == "\x01"

#==================================================================================================

def test_memory_input_stream_read_bool():
    data = b"\x01\x00"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readBool() == True
    assert input_stream.readBool() == False

#==================================================================================================

def test_memory_input_stream_read_short():
    data = b"\x01\x02\x03\x04"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readShort() == 513
    assert input_stream.readShortBigEndian() == 772

#==================================================================================================

def test_memory_input_stream_read_int():
    data = b"\x01\x02\x03\x04\x01\x02\x03\x04"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readInt() == 67305985
    assert input_stream.readIntBigEndian() == 16909060

#==================================================================================================

def test_memory_input_stream_read_int64():
    data = b"\x01\x02\x03\x04\x05\x06\x07\x08\x01\x02\x03\x04\x05\x06\x07\x08"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readInt64() == 578437695752307201
    assert input_stream.readInt64BigEndian() == 72623859790382856

#==================================================================================================

def test_memory_input_stream_read_float():
    data = b"\x00\x00\x80?\x00\x00\x80?"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readFloat() == 1.0
    assert input_stream.readFloatBigEndian() == pytest.approx(1.1754943508222875e-38)

#==================================================================================================

def test_memory_input_stream_read_double():
    data = b"\x00\x00\x00\x00\x00\x00\xf0?\x00\x00\x00\x00\x00\x00\xf0?"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readDouble() == 1.0
    assert input_stream.readDoubleBigEndian() == pytest.approx(2.2250738585072014e-308)

#==================================================================================================

def test_memory_input_stream_read_compressed_int():
    data = b"\x04\x01\x02\x03\x04"
    input_stream = yup.MemoryInputStream(data, False)
    assert input_stream.readCompressedInt() == 67305985

#==================================================================================================

def test_buffer_data_is_copied():
    data = bytearray(b"abcdef")
    stream = yup.MemoryInputStream(data, False)

    data[:] = b""
    del data

    assert stream.readEntireStreamAsString() == "abcdef"

#==================================================================================================

def test_memory_block_is_kept_alive():
    stream = yup.MemoryInputStream(yup.MemoryBlock(b"abcdef"), False)
    assert stream.readEntireStreamAsString() == "abcdef"

#==================================================================================================

def test_read_requires_writable_contiguous_buffer():
    stream = yup.MemoryInputStream(b"abcdef", True)

    with pytest.raises(BufferError):
        stream.read(b"\x00\x00")

    with pytest.raises(BufferError):
        stream.read(memoryview(bytearray(8))[::2])

    destination = bytearray(3)
    assert stream.read(destination) == 3
    assert destination == b"abc"

#==================================================================================================

def test_memory_output_stream_buffer_is_pinned():
    target = bytearray(8)
    stream = yup.MemoryOutputStream(target)
    stream.write(b"abcd")
    assert target[:4] == b"abcd"

    with pytest.raises(BufferError):
        target.extend(b"x")

    del stream
    target.extend(b"x")
    assert len(target) == 9

#==================================================================================================

def test_memory_output_stream_rejects_read_only_buffer():
    with pytest.raises(BufferError):
        yup.MemoryOutputStream(b"\x00\x00\x00\x00")

#==================================================================================================

def test_memory_output_stream_get_data_outlives_stream():
    stream = yup.MemoryOutputStream()
    stream.write(b"abcd")
    data = stream.getData()

    del stream
    assert bytes(data) == b"abcd"
