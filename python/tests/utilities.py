import os
from pathlib import Path

from . import common

import yup

#==================================================================================================

def get_runtime_data_folder() -> yup.File:
    return yup.File(os.path.abspath(__file__)).getParentDirectory().getChildFile("runtime_data")

def get_runtime_data_file(name: str) -> yup.File:
    return get_runtime_data_folder().getChildFile(name)

#==================================================================================================

def remove_directory_recursively(directory, excluding_files = None, excluding_folders = None):
    directory = Path(directory)

    for item in directory.iterdir():
        if item.is_dir() and (not excluding_folders or item.name not in excluding_folders):
            remove_directory_recursively(item, excluding_files, excluding_folders)
        elif (not excluding_files or item.name not in excluding_files):
            item.unlink()

    try:
        directory.rmdir()
    except OSError:
        pass

#==================================================================================================

"""

def equal_images(lhs: yup.Image, rhs: yup.Image) -> bool:
    if lhs.getFormat() != rhs.getFormat():
        return False

    lhs_pixels = yup.Image.BitmapData(lhs, yup.Image.BitmapData.readOnly)
    rhs_pixels = yup.Image.BitmapData(rhs, yup.Image.BitmapData.readOnly)

    if lhs_pixels.size != rhs_pixels.size:
        return False

    return lhs_pixels.data == rhs_pixels.data

#==================================================================================================

def save_component_snapshot_to_file(component: yup.Component, file: yup.File) -> bool:
    format = yup.ImageFileFormat.findImageFormatForFileExtension(file)
    if not format:
        return False

    file.deleteFile()

    out = yup.FileOutputStream(file)
    if not out.openedOk():
        return False

    snapshot = component.createComponentSnapshot(component.getLocalBounds())

    format.writeImageToStream(snapshot, out)

    out.flush()

    return True

"""
