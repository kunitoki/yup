import pytest
import yup


# ==============================================================================
# AudioSourcePlayer
# ==============================================================================

def test_player_construction():
    player = yup.AudioSourcePlayer()
    assert player is not None


def test_player_default_source():
    player = yup.AudioSourcePlayer()
    assert player.getCurrentSource() is None


def test_player_gain_defaults():
    player = yup.AudioSourcePlayer()
    assert player.getGain() == 1.0


def test_player_set_gain():
    player = yup.AudioSourcePlayer()
    player.setGain(0.5)
    assert abs(player.getGain() - 0.5) < 1e-6

    player.setGain(2.0)
    assert abs(player.getGain() - 2.0) < 1e-6

    player.setGain(0.0)
    assert abs(player.getGain() - 0.0) < 1e-6


# ==============================================================================
# AudioTransportSource
# ==============================================================================

def test_transport_construction():
    transport = yup.AudioTransportSource()
    assert transport is not None


def test_transport_initial_state():
    transport = yup.AudioTransportSource()
    assert transport.isPlaying() is False


def test_transport_gain_defaults():
    transport = yup.AudioTransportSource()
    assert transport.getGain() == 1.0


def test_transport_set_gain():
    transport = yup.AudioTransportSource()
    transport.setGain(0.75)
    assert abs(transport.getGain() - 0.75) < 1e-6


def test_transport_position_initial():
    transport = yup.AudioTransportSource()
    assert transport.getCurrentPosition() == 0.0


def test_transport_set_position():
    transport = yup.AudioTransportSource()
    transport.setPosition(5.0)


def test_transport_length_default():
    transport = yup.AudioTransportSource()
    length = transport.getLengthInSeconds()
    assert length >= 0.0


def test_transport_has_stream_finished():
    transport = yup.AudioTransportSource()
    assert transport.hasStreamFinished() is True


def test_transport_set_source_none():
    transport = yup.AudioTransportSource()
    transport.setSource(None)
    assert transport.hasStreamFinished() is True
