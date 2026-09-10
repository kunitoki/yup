import pytest
import yup


# ==============================================================================
# Helpers
# ==============================================================================

def available_device_types():
    """Returns the manager and its device types, skipping when the platform has none."""
    manager = yup.AudioDeviceManager()
    device_types = manager.getAvailableDeviceTypes()

    if not device_types:
        pytest.skip("no audio device types on this platform")

    return manager, device_types


# ==============================================================================
# AudioDeviceManager.getAvailableDeviceTypes
# ==============================================================================

def test_device_manager_lists_device_types():
    manager = yup.AudioDeviceManager()
    device_types = manager.getAvailableDeviceTypes()

    assert isinstance(device_types, list)

    for device_type in device_types:
        assert isinstance(device_type, yup.AudioIODeviceType)


def test_device_manager_returns_a_fresh_list_each_time():
    manager = yup.AudioDeviceManager()
    device_types = manager.getAvailableDeviceTypes()
    device_types.append(None)

    assert len(manager.getAvailableDeviceTypes()) == len(device_types) - 1


def test_device_manager_lists_the_same_types_across_calls():
    manager = yup.AudioDeviceManager()
    first = [device_type.getTypeName() for device_type in manager.getAvailableDeviceTypes()]
    second = [device_type.getTypeName() for device_type in manager.getAvailableDeviceTypes()]

    assert first == second


# ==============================================================================
# AudioIODeviceType
# ==============================================================================

def test_device_type_reports_a_name():
    _, device_types = available_device_types()

    for device_type in device_types:
        assert isinstance(device_type.getTypeName(), str)
        assert device_type.getTypeName() != ""


def test_device_type_repr():
    _, device_types = available_device_types()

    assert "AudioIODeviceType" in repr(device_types[0])


def test_device_type_scans_and_reports_device_names():
    _, device_types = available_device_types()

    for device_type in device_types:
        device_type.scanForDevices()

        names = device_type.getDeviceNames()
        assert len(names) == names.size()

        for name in names:
            assert isinstance(name, str)
            assert name != ""


def test_device_type_returns_input_names_on_request():
    _, device_types = available_device_types()

    for device_type in device_types:
        device_type.scanForDevices()

        input_names = device_type.getDeviceNames(wantInputNames=True)
        assert len(input_names) == input_names.size()


def test_device_type_reports_whether_inputs_and_outputs_are_separate():
    _, device_types = available_device_types()

    for device_type in device_types:
        assert isinstance(device_type.hasSeparateInputsAndOutputs(), bool)


def test_device_type_reports_default_device_index():
    _, device_types = available_device_types()

    for device_type in device_types:
        device_type.scanForDevices()

        assert isinstance(device_type.getDefaultDeviceIndex(forInput=False), int)
        assert isinstance(device_type.getDefaultDeviceIndex(forInput=True), int)


def test_device_type_creates_a_device_for_a_listed_name():
    _, device_types = available_device_types()

    device_type = device_types[0]
    device_type.scanForDevices()

    names = device_type.getDeviceNames()

    if len(names) == 0:
        pytest.skip("no audio devices on this platform")

    device = device_type.createDevice(names[0], "")

    assert device is None or isinstance(device, yup.AudioIODevice)

    if device is not None:
        assert device.isOpen() is False
        assert device.getTypeName() == device_type.getTypeName()
