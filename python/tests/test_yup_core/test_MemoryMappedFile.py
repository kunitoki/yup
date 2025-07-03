import os
import sys
import pytest

import yup
from popsicle import int64

#==================================================================================================

this_file = yup.File(os.path.abspath(__file__))
data_folder = this_file.getSiblingFile("data")

#==================================================================================================

def test_memory_mapped_file_read_only_mode():
    mmf = yup.MemoryMappedFile(data_folder.getChildFile("somefile.txt"), yup.MemoryMappedFile.readOnly)
    assert mmf.getData() is not None
    assert mmf.getSize() > 0
    assert mmf.getRange().getStart() >= 0
    assert mmf.getRange().getEnd() > mmf.getRange().getStart()

#==================================================================================================

def test_memory_mapped_file_read_write_mode():
    mmf = yup.MemoryMappedFile(data_folder.getChildFile("somefile.txt"), yup.MemoryMappedFile.readWrite)
    assert mmf.getData() is not None
    assert mmf.getSize() > 0
    assert mmf.getRange().getStart() >= 0
    assert mmf.getRange().getEnd() > mmf.getRange().getStart()

#==================================================================================================

@pytest.mark.skipif(sys.platform == "win32", reason="On windows it seems like exclusive mode fails")
def test_memory_mapped_file_exclusive_mode():
    mmf = yup.MemoryMappedFile(data_folder.getChildFile("somefile.txt"), yup.MemoryMappedFile.readOnly, exclusive=True)
    assert mmf.getData() is not None
    assert mmf.getSize() > 0

#==================================================================================================

def test_memory_mapped_file_non_existent_file():
    mmf = yup.MemoryMappedFile(data_folder.getChildFile("nonexistent.txt"), yup.MemoryMappedFile.readOnly)
    assert mmf.getData() is None
    assert mmf.getSize() == 0

#==================================================================================================

def test_memory_mapped_file_section_read_only_mode():
    mmf = yup.MemoryMappedFile(data_folder.getChildFile("somefile.txt"), yup.Range[int64](0, 10), yup.MemoryMappedFile.readOnly)
    assert mmf.getData() is not None
    assert mmf.getSize() == 10
    assert mmf.getRange().getLength() == 10

#==================================================================================================

def test_memory_mapped_file_section_read_write_mode():
    mmf = yup.MemoryMappedFile(data_folder.getChildFile("somefile.txt"), yup.Range[int64](0, 10), yup.MemoryMappedFile.readWrite)
    assert mmf.getData() is not None
    assert mmf.getSize() == 10
    assert mmf.getRange().getLength() == 10

#==================================================================================================

@pytest.mark.skipif(sys.platform == "win32", reason="On windows it seems like exclusive mode fails")
def test_memory_mapped_file_section_exclusive_mode():
    mmf = yup.MemoryMappedFile(data_folder.getChildFile("somefile.txt"), yup.Range[int64](0, 10), yup.MemoryMappedFile.readOnly, exclusive=True)
    assert mmf.getData() is not None
    assert mmf.getSize() == 10
