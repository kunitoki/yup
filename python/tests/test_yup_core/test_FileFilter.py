import os

from ..utilities import get_runtime_data_file
import yup

this_file = os.path.abspath(__file__)
this_folder = os.path.dirname(this_file)

#==================================================================================================

class CustomFileFilter(yup.FileFilter):
    def isFileSuitable(self, file):
        return False

    def isDirectorySuitable(self, file):
        return True

def test_custom_file_filter():
    w = CustomFileFilter("all folders")

    assert w.getDescription() == "all folders"
    assert not w.isFileSuitable(yup.File(this_folder).getChildFile("test.wav"))
    assert not w.isFileSuitable(yup.File(this_folder).getChildFile("test.mp3"))
    assert w.isDirectorySuitable(yup.File(this_folder).getChildFile("pass"))
    assert w.isDirectorySuitable(yup.File(this_folder).getChildFile("also_pass"))

#==================================================================================================

def test_wildcard_file_filter_files():
    w = yup.WildcardFileFilter("*.wav;*.aiff", "", "audio files")

    assert w.getDescription() == "audio files (*.wav;*.aiff)"
    assert w.isFileSuitable(yup.File(this_folder).getChildFile("test.wav"))
    assert not w.isFileSuitable(yup.File(this_folder).getChildFile("test.mp3"))

#==================================================================================================

def test_wildcard_file_filter_folders():
    w = yup.WildcardFileFilter("*", "pass", "all files")

    assert w.getDescription() == "all files (*)"
    assert w.isDirectorySuitable(yup.File(this_folder).getChildFile("pass"))
    assert not w.isDirectorySuitable(yup.File(this_folder).getChildFile("not_pass"))
