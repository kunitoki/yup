import pytest
import yup
import os


# ==============================================================================
# AudioFormatReaderSource
# ==============================================================================

def test_reader_source_construction_with_null():
    # Passing None should work; the source just produces silence
    source = yup.AudioFormatReaderSource(None, False)
    assert source is not None


def test_reader_source_looping_defaults():
    source = yup.AudioFormatReaderSource(None, False)
    assert source.isLooping() is False


def test_reader_source_set_looping():
    source = yup.AudioFormatReaderSource(None, False)
    source.setLooping(True)
    assert source.isLooping() is True
    source.setLooping(False)
    assert source.isLooping() is False


def test_reader_source_total_length():
    source = yup.AudioFormatReaderSource(None, False)
    assert source.getTotalLength() >= 0


def test_reader_source_position():
    source = yup.AudioFormatReaderSource(None, False)
    assert source.getNextReadPosition() >= 0
    source.setNextReadPosition(100)
    assert source.getNextReadPosition() == 100


def test_reader_source_negative_position_clamped():
    source = yup.AudioFormatReaderSource(None, False)
    source.setNextReadPosition(-50)
    assert source.getNextReadPosition() == 0


def test_reader_source_get_audio_format_reader():
    source = yup.AudioFormatReaderSource(None, False)
    reader = source.getAudioFormatReader()
    assert reader is None


# ==============================================================================
# Integration: AudioFormatManager -> AudioFormatReader -> AudioFormatReaderSource
# ==============================================================================

@pytest.fixture
def temp_wav_file():
    """Create a minimal WAV file for testing."""
    import tempfile

    # Minimal 44-byte WAV header + 100 samples of silence (16-bit mono, 44100 Hz)
    wav_data = bytearray()
    # RIFF header
    wav_data += b"RIFF"
    wav_data += (36 + 200).to_bytes(4, "little")  # chunk size
    wav_data += b"WAVE"
    # fmt chunk
    wav_data += b"fmt "
    wav_data += (16).to_bytes(4, "little")  # subchunk size
    wav_data += (1).to_bytes(2, "little")  # PCM
    wav_data += (1).to_bytes(2, "little")  # mono
    wav_data += (44100).to_bytes(4, "little")  # sample rate
    wav_data += (44100 * 2).to_bytes(4, "little")  # byte rate
    wav_data += (2).to_bytes(2, "little")  # block align
    wav_data += (16).to_bytes(2, "little")  # bits per sample
    # data chunk
    wav_data += b"data"
    wav_data += (200).to_bytes(4, "little")  # data size
    wav_data += b"\x00" * 200  # 100 silent samples

    fd, path = tempfile.mkstemp(suffix=".wav")
    os.write(fd, wav_data)
    os.close(fd)
    yield path
    os.unlink(path)


def test_format_manager_construction():
    mgr = yup.AudioFormatManager()
    assert mgr is not None


def test_format_manager_register_default_formats():
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()
    # Should not raise


def test_format_manager_create_reader_for_invalid_file():
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()
    reader = mgr.createReaderFor(yup.File("/nonexistent/file.wav"))
    assert reader is None


def test_format_manager_create_reader_for_valid_wav(temp_wav_file):
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()

    reader = mgr.createReaderFor(yup.File(temp_wav_file))
    assert reader is not None
    assert reader.sampleRate == 44100.0
    assert reader.numChannels >= 1
    assert reader.bitsPerSample >= 16
    assert reader.lengthInSamples == 100


def test_format_reader_integration(temp_wav_file):
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()

    reader = mgr.createReaderFor(yup.File(temp_wav_file))
    assert reader is not None

    # Create AudioFormatReaderSource from the reader
    # Don't transfer ownership — Python still manages the reader
    source = yup.AudioFormatReaderSource(reader, False)
    assert source is not None
    assert source.getTotalLength() >= 0

    retrieved = source.getAudioFormatReader()
    assert retrieved is not None
    assert retrieved.sampleRate == 44100.0
