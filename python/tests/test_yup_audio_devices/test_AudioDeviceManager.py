import pytest
import yup


# ==============================================================================
# AudioDeviceManager
# ==============================================================================

def test_device_manager_construction():
    manager = yup.AudioDeviceManager()
    assert manager is not None


def test_device_manager_get_current_device():
    manager = yup.AudioDeviceManager()
    assert manager.getCurrentAudioDevice() is None


def test_device_manager_get_audio_device_setup():
    manager = yup.AudioDeviceManager()
    setup = manager.getAudioDeviceSetup()
    assert setup is not None


def test_device_manager_device_setup_defaults():
    manager = yup.AudioDeviceManager()
    setup = manager.getAudioDeviceSetup()
    assert setup.sampleRate == 0.0
    assert setup.bufferSize == 0


def test_device_manager_set_audio_device_setup():
    manager = yup.AudioDeviceManager()
    setup = manager.getAudioDeviceSetup()
    setup.sampleRate = 48000.0
    setup.bufferSize = 256
    result = manager.setAudioDeviceSetup(setup, False)
    assert isinstance(result, str)


def test_device_manager_get_cpu_usage():
    manager = yup.AudioDeviceManager()
    usage = manager.getCpuUsage()
    assert 0.0 <= usage <= 1.0


def test_device_manager_get_current_device_type():
    manager = yup.AudioDeviceManager()
    deviceType = manager.getCurrentAudioDeviceType()
    assert isinstance(deviceType, str)


# ==============================================================================
# AudioDeviceSetup
# ==============================================================================

def test_audio_device_setup_defaults():
    setup = yup.AudioDeviceSetup()
    assert setup.sampleRate == 0.0
    assert setup.bufferSize == 0
    assert setup.useDefaultInputChannels is True
    assert setup.useDefaultOutputChannels is True


def test_audio_device_setup_equality():
    a = yup.AudioDeviceSetup()
    b = yup.AudioDeviceSetup()
    assert a == b

    a.sampleRate = 44100.0
    assert a != b


def test_audio_device_setup_fields():
    setup = yup.AudioDeviceSetup()
    setup.sampleRate = 48000.0
    setup.bufferSize = 512
    setup.useDefaultInputChannels = False
    setup.useDefaultOutputChannels = False

    assert setup.sampleRate == 48000.0
    assert setup.bufferSize == 512
    assert setup.useDefaultInputChannels is False
    assert setup.useDefaultOutputChannels is False


# ==============================================================================
# AudioIODeviceCallbackContext
# ==============================================================================

def test_callback_context_defaults():
    ctx = yup.AudioIODeviceCallbackContext()
    assert ctx.hostTimeNs is None


# ==============================================================================
# AudioIODeviceCallback (trampoline base)
# ==============================================================================

def test_callback_construction():
    callback = yup.AudioIODeviceCallback()
    assert callback is not None
