import yup

#==================================================================================================
# Unlike AbstractFifo, this one is for single-threaded use and does not reserve a slot: a fifo of
# size N can hold N readable items.
#==================================================================================================

def test_default_construction_is_empty():
    fifo = yup.SingleThreadedAbstractFifo()

    assert fifo.getSize() == 0
    assert fifo.getNumReadable() == 0
    assert fifo.getRemainingSpace() == 0

#==================================================================================================

def test_size_constructor_reports_full_capacity():
    fifo = yup.SingleThreadedAbstractFifo(8)

    assert fifo.getSize() == 8
    assert fifo.getNumReadable() == 0
    assert fifo.getRemainingSpace() == 8

#==================================================================================================

def test_write_then_read_round_trip():
    fifo = yup.SingleThreadedAbstractFifo(8)

    written = fifo.write(3)

    assert len(written) == 2

    firstBlock = written[0]
    assert firstBlock.getStart() == 0
    assert firstBlock.getEnd() == 3
    assert written[1].isEmpty()

    assert fifo.getNumReadable() == 3
    assert fifo.getRemainingSpace() == 5

    read = fifo.read(3)

    assert len(read) == 2
    assert read[0].getStart() == 0
    assert read[0].getEnd() == 3

    assert fifo.getNumReadable() == 0

#==================================================================================================

def test_filling_the_buffer_then_reading_it_back():
    fifo = yup.SingleThreadedAbstractFifo(4)

    fifo.write(4)

    assert fifo.getNumReadable() == 4
    assert fifo.getRemainingSpace() == 0

    # A write that does not fit reports a single empty block for the remainder.
    overflow = fifo.write(2)
    assert overflow[0].isEmpty() and overflow[1].isEmpty()

    fifo.read(4)

    assert fifo.getNumReadable() == 0
    assert fifo.getRemainingSpace() == 4

#==================================================================================================

def test_writing_across_the_end_of_the_buffer_reports_two_blocks():
    fifo = yup.SingleThreadedAbstractFifo(4)

    # Leave two readable, so the next write of four starts at index 2 and wraps.
    fifo.write(2)

    blocks = fifo.write(2)

    assert blocks[0].getStart() == 2
    assert blocks[0].getEnd() == 4
    assert blocks[1].getStart() == 0
    assert blocks[1].getEnd() == 0

#==================================================================================================

def test_reading_less_than_was_written_leaves_the_remainder():
    fifo = yup.SingleThreadedAbstractFifo(8)

    fifo.write(5)
    fifo.read(2)

    assert fifo.getNumReadable() == 3
    assert fifo.getRemainingSpace() == 5
