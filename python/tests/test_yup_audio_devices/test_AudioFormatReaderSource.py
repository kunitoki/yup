import gc
import os
import weakref

import pytest
import yup


# ==============================================================================
# AudioFormatReaderSource
# ==============================================================================

def test_reader_source_construction_with_null():
    # Passing None should work; the source just produces silence
    source = yup.AudioFormatReaderSource(None)
    assert source is not None


def test_reader_source_looping_defaults():
    source = yup.AudioFormatReaderSource(None)
    assert source.isLooping() is False


def test_reader_source_set_looping():
    source = yup.AudioFormatReaderSource(None)
    source.setLooping(True)
    assert source.isLooping() is True
    source.setLooping(False)
    assert source.isLooping() is False


def test_reader_source_total_length():
    source = yup.AudioFormatReaderSource(None)
    assert source.getTotalLength() >= 0


def test_reader_source_position():
    source = yup.AudioFormatReaderSource(None)
    assert source.getNextReadPosition() >= 0
    source.setNextReadPosition(100)
    assert source.getNextReadPosition() == 100


def test_reader_source_negative_position_clamped():
    source = yup.AudioFormatReaderSource(None)
    source.setNextReadPosition(-50)
    assert source.getNextReadPosition() == 0


def test_reader_source_get_audio_format_reader():
    source = yup.AudioFormatReaderSource(None)
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

    # The reader stays Python's; the source borrows it and is pinned to it
    source = yup.AudioFormatReaderSource(reader)
    assert source is not None
    assert source.getTotalLength() == 100

    retrieved = source.getAudioFormatReader()
    assert retrieved is not None
    assert retrieved.sampleRate == 44100.0


# ==============================================================================
# Reader ownership
# ==============================================================================

def test_reader_source_keeps_the_reader_alive(temp_wav_file):
    # The source borrows the reader Python owns, so it has to be pinned to the
    # source: without that the reader dies with its last Python reference and the
    # source reads freed memory, reporting a total length of 0.
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()

    reader = mgr.createReaderFor(yup.File(temp_wav_file))
    source = yup.AudioFormatReaderSource(reader)

    remaining = weakref.ref(reader)
    del reader
    gc.collect()

    assert remaining() is not None
    assert source.getTotalLength() == 100


def test_transport_of_a_reader_source_starts_and_keeps_playing(temp_wav_file):
    # What the player demo does, minus the audio device: a transport whose source
    # reports 0 as its total length looks finished as soon as it is started, so
    # start() silently reverts and the demo never sees isPlaying() come true.
    mgr = yup.AudioFormatManager()
    mgr.registerDefaultFormats()

    reader_source = yup.AudioFormatReaderSource(mgr.createReaderFor(yup.File(temp_wav_file)))
    transport = yup.AudioTransportSource()

    transport.setSource(reader_source)
    transport.prepareToPlay(512, 44100.0)
    transport.start()

    assert transport.hasStreamFinished() is False
    assert transport.isPlaying() is True
    assert transport.getLengthInSeconds() == pytest.approx(100 / 44100.0)
