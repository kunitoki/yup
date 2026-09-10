import yup

#==================================================================================================
# YUP's AbstractFifo is a single-reader/single-writer ring buffer that reserves one slot to tell
# a full buffer apart from an empty one: getFreeSpace() is size - getNumReady() - 1, so a FIFO
# constructed with a capacity of N can only ever hold N - 1 items.
#==================================================================================================

def test_new_fifo_reserves_one_slot():
    fifo = yup.AbstractFifo(8)

    assert fifo.getTotalSize() == 8
    assert fifo.getNumReady() == 0
    assert fifo.getFreeSpace() == 7

#==================================================================================================

def test_write_then_read_round_trip():
    fifo = yup.AbstractFifo(8)

    _, size1, _, size2 = fifo.prepareToWrite(4)
    assert size1 + size2 == 4

    fifo.finishedWrite(size1 + size2)
    assert fifo.getNumReady() == 4
    assert fifo.getFreeSpace() == 3

    _, readSize1, _, readSize2 = fifo.prepareToRead(4)
    assert readSize1 + readSize2 == 4

    fifo.finishedRead(readSize1 + readSize2)
    assert fifo.getNumReady() == 0
    assert fifo.getFreeSpace() == 7

#==================================================================================================

def test_write_is_limited_to_the_free_space():
    fifo = yup.AbstractFifo(4)

    # Asking for more than the reserved-slot-limited capacity yields the capacity only.
    _, size1, _, size2 = fifo.prepareToWrite(10)

    assert size1 + size2 == 3

#==================================================================================================

def test_the_buffer_can_be_filled_completely():
    fifo = yup.AbstractFifo(4)

    _, size1, _, size2 = fifo.prepareToWrite(3)
    fifo.finishedWrite(size1 + size2)

    assert fifo.getNumReady() == 3
    assert fifo.getFreeSpace() == 0

    # Nothing more fits once every usable slot is taken.
    _, extra1, _, extra2 = fifo.prepareToWrite(1)
    assert extra1 + extra2 == 0

#==================================================================================================

def test_writing_wraps_around_the_buffer():
    fifo = yup.AbstractFifo(4)

    # Fill two slots, read them back, then ask for more than fits before the end of the
    # buffer: the write has to describe two separate blocks, the second starting at 0.
    _, size1, _, size2 = fifo.prepareToWrite(2)
    fifo.finishedWrite(size1 + size2)

    _, readSize1, _, readSize2 = fifo.prepareToRead(2)
    fifo.finishedRead(readSize1 + readSize2)

    _, writeSize1, start2, writeSize2 = fifo.prepareToWrite(3)

    assert writeSize1 == 2
    assert writeSize2 == 1
    assert start2 == 0

#==================================================================================================

def test_wrapping_write_then_read_returns_the_same_counts():
    fifo = yup.AbstractFifo(4)

    _, size1, _, size2 = fifo.prepareToWrite(2)
    fifo.finishedWrite(size1 + size2)

    _, readSize1, _, readSize2 = fifo.prepareToRead(2)
    fifo.finishedRead(readSize1 + readSize2)

    _, writeSize1, _, writeSize2 = fifo.prepareToWrite(3)
    fifo.finishedWrite(writeSize1 + writeSize2)

    assert fifo.getNumReady() == 3

    _, readSize1, _, readSize2 = fifo.prepareToRead(3)
    assert readSize1 + readSize2 == 3

    fifo.finishedRead(readSize1 + readSize2)
    assert fifo.getNumReady() == 0

#==================================================================================================

def test_reset_empties_the_fifo():
    fifo = yup.AbstractFifo(4)

    _, size1, _, size2 = fifo.prepareToWrite(2)
    fifo.finishedWrite(size1 + size2)

    assert fifo.getNumReady() == 2

    fifo.reset()

    assert fifo.getNumReady() == 0
    assert fifo.getFreeSpace() == 3

#==================================================================================================

def test_set_total_size():
    fifo = yup.AbstractFifo(4)

    fifo.setTotalSize(16)

    assert fifo.getTotalSize() == 16
    assert fifo.getFreeSpace() == 15
