import pytest
import yup

#==================================================================================================

def test_audio_buffer_construction():
    # Test default construction
    buffer = yup.AudioBuffer()
    assert buffer.getNumChannels() == 0
    assert buffer.getNumSamples() == 0

    # Test construction with size
    buffer = yup.AudioBuffer(2, 512)
    assert buffer.getNumChannels() == 2
    assert buffer.getNumSamples() == 512

#==================================================================================================

def test_audio_buffer_set_size():
    buffer = yup.AudioBuffer()
    buffer.setSize(2, 1024)
    assert buffer.getNumChannels() == 2
    assert buffer.getNumSamples() == 1024

    # Resize
    buffer.setSize(4, 512)
    assert buffer.getNumChannels() == 4
    assert buffer.getNumSamples() == 512

#==================================================================================================

def test_audio_buffer_clear():
    buffer = yup.AudioBuffer(2, 512)

    # Fill with non-zero values
    for channel in range(buffer.getNumChannels()):
        for sample in range(buffer.getNumSamples()):
            buffer.setSample(channel, sample, 1.0)

    # Clear all
    buffer.clear()
    assert buffer.hasBeenCleared()

    # Verify cleared
    for channel in range(buffer.getNumChannels()):
        for sample in range(min(10, buffer.getNumSamples())):
            assert abs(buffer.getSample(channel, sample)) < 0.0001

#==================================================================================================

def test_audio_buffer_samples():
    buffer = yup.AudioBuffer(2, 512)

    # Set and get samples
    buffer.setSample(0, 0, 0.5)
    buffer.setSample(0, 1, -0.5)
    buffer.setSample(1, 0, 0.25)

    assert abs(buffer.getSample(0, 0) - 0.5) < 0.001
    assert abs(buffer.getSample(0, 1) - (-0.5)) < 0.001
    assert abs(buffer.getSample(1, 0) - 0.25) < 0.001

#==================================================================================================

def test_audio_buffer_add_sample():
    buffer = yup.AudioBuffer(2, 512)
    buffer.clear()

    buffer.setSample(0, 0, 0.5)
    buffer.addSample(0, 0, 0.25)

    assert abs(buffer.getSample(0, 0) - 0.75) < 0.001

#==================================================================================================

def test_audio_buffer_apply_gain():
    buffer = yup.AudioBuffer(2, 512)

    # Fill with values
    for channel in range(buffer.getNumChannels()):
        for sample in range(buffer.getNumSamples()):
            buffer.setSample(channel, sample, 1.0)

    # Apply gain
    buffer.applyGain(0.5)

    # Check a few samples
    for channel in range(buffer.getNumChannels()):
        for sample in range(min(10, buffer.getNumSamples())):
            assert abs(buffer.getSample(channel, sample) - 0.5) < 0.001

#==================================================================================================

def test_audio_buffer_copy():
    source = yup.AudioBuffer(2, 512)
    dest = yup.AudioBuffer(2, 512)

    # Fill source with values
    for channel in range(source.getNumChannels()):
        for sample in range(source.getNumSamples()):
            source.setSample(channel, sample, 0.7)

    dest.clear()

    # Copy from source
    dest.copyFrom(0, 0, source, 0, 0, 512)

    # Verify
    for sample in range(min(10, dest.getNumSamples())):
        assert abs(dest.getSample(0, sample) - 0.7) < 0.001

#==================================================================================================

def test_audio_buffer_find_min_max():
    buffer = yup.AudioBuffer(1, 512)
    buffer.clear()

    buffer.setSample(0, 0, -0.8)
    buffer.setSample(0, 1, 0.9)
    buffer.setSample(0, 2, 0.3)

    result = buffer.findMinMax(0, 0, 512)
    assert result.getStart() <= -0.79  # Allow small tolerance
    assert result.getEnd() >= 0.89  # Allow small tolerance

#==================================================================================================

def test_audio_buffer_get_magnitude():
    buffer = yup.AudioBuffer(2, 512)
    buffer.clear()

    # Set some known values
    buffer.setSample(0, 0, 1.0)
    buffer.setSample(0, 1, -1.0)
    buffer.setSample(0, 2, 0.5)

    mag = buffer.getMagnitude(0, 0, 3)
    assert mag >= 0.49  # Allow small tolerance

#==================================================================================================

def test_audio_buffer_get_rms_level():
    buffer = yup.AudioBuffer(1, 512)

    # Fill with constant value
    for sample in range(buffer.getNumSamples()):
        buffer.setSample(0, sample, 0.5)

    rms = buffer.getRMSLevel(0, 0, 512)
    assert abs(rms - 0.5) < 0.01

#==================================================================================================

def test_audio_buffer_double():
    # Test double precision buffer
    buffer = yup.AudioBufferDouble(2, 512)
    assert buffer.getNumChannels() == 2
    assert buffer.getNumSamples() == 512

    buffer.setSample(0, 0, 0.123456789)
    value = buffer.getSample(0, 0)
    assert abs(value - 0.123456789) < 0.0000001
