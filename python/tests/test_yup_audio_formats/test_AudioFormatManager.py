import pytest
import yup


# ==============================================================================
# AudioFormatType enum
# ==============================================================================

def test_audio_format_type_enum():
    assert yup.AudioFormatType.wav is not None
    assert yup.AudioFormatType.mp3 is not None
    assert yup.AudioFormatType.flac is not None
    assert yup.AudioFormatType.ogg is not None
    assert yup.AudioFormatType.opus is not None


# ==============================================================================
# AudioFormatManager
# ==============================================================================

def test_construction():
    mgr = yup.AudioFormatManager()
    assert mgr is not None


def test_register_default_formats_all():
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()
    # Should succeed without error


def test_register_default_formats_wav_only():
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats(yup.AudioFormatType.wav)


def test_create_reader_for_nonexistent_file():
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()
    result = mgr.createReaderFor(yup.File("/nonexistent/file.wav"))
    assert result is None


def test_create_reader_for_directory():
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()
    result = mgr.createReaderFor(yup.File("/tmp"))
    assert result is None


# ==============================================================================
# AudioFormatReader (via AudioFormatManager)
# ==============================================================================

@pytest.fixture
def temp_wav_file():
    import tempfile
    import os

    # Minimal WAV: 44-byte header + 200 bytes of silence (100 samples, 16-bit mono)
    wav = bytearray()
    wav += b"RIFF"
    wav += (36 + 200).to_bytes(4, "little")
    wav += b"WAVE"
    wav += b"fmt "
    wav += (16).to_bytes(4, "little")
    wav += (1).to_bytes(2, "little")           # PCM
    wav += (1).to_bytes(2, "little")           # mono
    wav += (44100).to_bytes(4, "little")       # sample rate
    wav += (44100 * 2).to_bytes(4, "little")   # byte rate
    wav += (2).to_bytes(2, "little")           # block align
    wav += (16).to_bytes(2, "little")          # bits per sample
    wav += b"data"
    wav += (200).to_bytes(4, "little")
    wav += b"\x00" * 200

    fd, path = tempfile.mkstemp(suffix=".wav")
    os.write(fd, wav)
    os.close(fd)
    yield path
    os.unlink(path)


def test_reader_properties(temp_wav_file):
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()
    reader = mgr.createReaderFor(yup.File(temp_wav_file))
    assert reader is not None
    assert reader.sampleRate == 44100.0
    assert reader.numChannels >= 1
    assert reader.bitsPerSample >= 16
    assert reader.lengthInSamples == 100


def test_reader_get_format_name(temp_wav_file):
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()
    reader = mgr.createReaderFor(yup.File(temp_wav_file))
    assert reader is not None
    name = reader.getFormatName()
    assert isinstance(name, str)
    assert len(name) > 0


def test_reader_read_into_audio_buffer(temp_wav_file):
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()
    reader = mgr.createReaderFor(yup.File(temp_wav_file))
    assert reader is not None

    buffer = yup.AudioBuffer(reader.numChannels, 50)
    ok = reader.read(buffer, 0, 50, 0, True, True)
    # Silly parser allows read of all-zero data
    assert ok is True
