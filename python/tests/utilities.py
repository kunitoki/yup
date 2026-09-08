import os
import time
from pathlib import Path

import common

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

    # NOTE: Image::BitmapData was renamed to ImagePixelData (standalone class).
    # This commented code needs updating for the new API (no ReadWriteMode enum).
    # lhs_pixels = yup.ImagePixelData(lhs, yup.ImagePixelData.readOnly)
    # rhs_pixels = yup.ImagePixelData(rhs, yup.ImagePixelData.readOnly)

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

#==================================================================================================

def pump_until(app, predicate, timeout_seconds = 5.0, slice_ms = 5):
    """Pumps the message loop until `predicate()` is true, or the timeout expires.

    A single `next(app)` runs the dispatch loop for a fixed 20ms and then returns. That is a race:
    the message manager can dispatch the callback inside that window, but the callback still has to
    re-acquire the GIL to run the Python side, and on a loaded machine it can land after the pump
    has already returned. Raising the constant cannot fix a one-shot pump - waiting on the actual
    condition can.

    Returns the final value of `predicate()`, so callers can simply assert on it. The predicate is
    re-evaluated after the timeout too, in case the callback landed during the last slice.
    """
    deadline = time.monotonic() + timeout_seconds

    while True:
        if predicate():
            return True

        if time.monotonic() >= deadline:
            return predicate()

        app.processEvents(slice_ms)
