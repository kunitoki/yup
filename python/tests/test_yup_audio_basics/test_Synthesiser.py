import yup
import math

#==================================================================================================

class SimpleSynthSound(yup.SynthesiserSound):
    """A simple synthesiser sound that applies to all notes and channels."""

    def __init__(self, minNote=0, maxNote=127):
        super().__init__()
        self.minNote = minNote
        self.maxNote = maxNote

    def appliesToNote(self, midiNoteNumber):
        return self.minNote <= midiNoteNumber <= self.maxNote

    def appliesToChannel(self, midiChannel):
        # Apply to all channels
        return True

#==================================================================================================

class SimpleSynthVoice(yup.SynthesiserVoice):
    """A simple sine wave synthesiser voice."""

    def __init__(self):
        super().__init__()
        self.current_note = -1
        self.velocity = 0.0
        self.sample_rate = 44100.0
        self.phase = 0.0
        self.phase_delta = 0.0
        self.active = False
        self.level = 0.0
        self.pitch_wheel = 0
        self.controller_values = {}

    def canPlaySound(self, sound):
        # Check if sound is a SimpleSynthSound
        return isinstance(sound, SimpleSynthSound)

    def startNote(self, midiNoteNumber, velocity, sound, currentPitchWheelPosition):
        self.current_note = midiNoteNumber
        self.velocity = velocity
        self.pitch_wheel = currentPitchWheelPosition
        self.active = True
        self.level = velocity
        self.phase = 0.0

        # Calculate frequency from MIDI note
        frequency = 440.0 * math.pow(2.0, (midiNoteNumber - 69) / 12.0)
        self.phase_delta = frequency / self.sample_rate

    def stopNote(self, velocity, allowTailOff):
        self.active = False
        self.level = 0.0

    def pitchWheelMoved(self, newPitchWheelValue):
        self.pitch_wheel = newPitchWheelValue

    def controllerMoved(self, controllerNumber, newControllerValue):
        self.controller_values[controllerNumber] = newControllerValue

    def renderNextBlock(self, outputBuffer, startSample, numSamples):
        if not self.active:
            return

        # Generate sine wave
        for channel in range(outputBuffer.getNumChannels()):
            phase = self.phase
            for i in range(numSamples):
                sample = math.sin(phase * 2.0 * math.pi) * self.level
                current_value = outputBuffer.getSample(channel, startSample + i)
                outputBuffer.setSample(channel, startSample + i, current_value + sample)
                phase += self.phase_delta
                if phase >= 1.0:
                    phase -= 1.0

        self.phase += self.phase_delta * numSamples
        while self.phase >= 1.0:
            self.phase -= 1.0

    def setCurrentPlaybackSampleRate(self, newRate):
        self.sample_rate = newRate
        # Recalculate phase delta if we have an active note
        if self.current_note >= 0:
            frequency = 440.0 * math.pow(2.0, (self.current_note - 69) / 12.0)
            self.phase_delta = frequency / self.sample_rate

    def isVoiceActive(self):
        return self.active

#==================================================================================================

def test_synthesiser_sound_creation():
    sound = SimpleSynthSound()
    assert sound is not None

#==================================================================================================

def test_synthesiser_sound_applies_to_note():
    sound = SimpleSynthSound(60, 72)  # Middle C to C above

    # Should apply to notes in range
    assert sound.appliesToNote(60)
    assert sound.appliesToNote(66)
    assert sound.appliesToNote(72)

    # Should not apply to notes outside range
    assert not sound.appliesToNote(59)
    assert not sound.appliesToNote(73)

#==================================================================================================

def test_synthesiser_sound_applies_to_channel():
    sound = SimpleSynthSound()

    # Should apply to all channels
    for channel in range(16):
        assert sound.appliesToChannel(channel)

#==================================================================================================

def test_synthesiser_voice_creation():
    voice = SimpleSynthVoice()
    assert voice is not None
    assert voice.current_note == -1
    assert not voice.active

#==================================================================================================

def test_synthesiser_voice_can_play_sound():
    voice = SimpleSynthVoice()
    sound = SimpleSynthSound()

    assert voice.canPlaySound(sound)

#==================================================================================================

def test_synthesiser_voice_start_note():
    voice = SimpleSynthVoice()
    sound = SimpleSynthSound()

    voice.startNote(60, 0.8, sound, 8192)

    assert voice.current_note == 60
    assert abs(voice.velocity - 0.8) < 0.001
    assert voice.pitch_wheel == 8192
    assert voice.active
    assert abs(voice.level - 0.8) < 0.001

#==================================================================================================

def test_synthesiser_voice_stop_note():
    voice = SimpleSynthVoice()
    sound = SimpleSynthSound()

    voice.startNote(60, 0.8, sound, 8192)
    assert voice.active

    voice.stopNote(0.0, True)
    assert not voice.active
    assert abs(voice.level - 0.0) < 0.001

#==================================================================================================

def test_synthesiser_voice_pitch_wheel():
    voice = SimpleSynthVoice()

    voice.pitchWheelMoved(10000)
    assert voice.pitch_wheel == 10000

    voice.pitchWheelMoved(4096)
    assert voice.pitch_wheel == 4096

#==================================================================================================

def test_synthesiser_voice_controller_moved():
    voice = SimpleSynthVoice()

    voice.controllerMoved(7, 100)  # Volume controller
    assert voice.controller_values[7] == 100

    voice.controllerMoved(1, 64)  # Mod wheel
    assert voice.controller_values[1] == 64

#==================================================================================================

def test_synthesiser_voice_render_next_block():
    voice = SimpleSynthVoice()
    sound = SimpleSynthSound()

    voice.setCurrentPlaybackSampleRate(44100.0)
    voice.startNote(69, 0.5, sound, 8192)  # A440

    buffer = yup.AudioBuffer(2, 512)
    buffer.clear()

    voice.renderNextBlock(buffer, 0, 512)

    # Check that buffer is no longer silent
    mag = buffer.getMagnitude(0, 0, 512)
    assert mag > 0.01

#==================================================================================================

def test_synthesiser_voice_is_active():
    voice = SimpleSynthVoice()
    sound = SimpleSynthSound()

    assert not voice.isVoiceActive()

    voice.startNote(60, 0.8, sound, 8192)
    assert voice.isVoiceActive()

    voice.stopNote(0.0, False)
    assert not voice.isVoiceActive()

#==================================================================================================

def test_synthesiser_voice_set_sample_rate():
    voice = SimpleSynthVoice()

    voice.setCurrentPlaybackSampleRate(48000.0)
    assert abs(voice.sample_rate - 48000.0) < 0.01

    voice.setCurrentPlaybackSampleRate(96000.0)
    assert abs(voice.sample_rate - 96000.0) < 0.01

#==================================================================================================

def test_synthesiser_voice_multiple_notes():
    voice = SimpleSynthVoice()
    sound = SimpleSynthSound()

    # Start first note
    voice.startNote(60, 0.5, sound, 8192)
    assert voice.current_note == 60

    # Stop and start second note
    voice.stopNote(0.0, False)
    voice.startNote(64, 0.7, sound, 8192)
    assert voice.current_note == 64
    assert abs(voice.velocity - 0.7) < 0.001

#==================================================================================================

def test_synthesiser_with_voice_and_sound():
    """Test using Synthesiser with custom voice and sound."""
    synth = yup.Synthesiser()

    # Add voice and sound
    voice = SimpleSynthVoice()
    sound = SimpleSynthSound(60, 72)

    synth.addVoice(voice)
    synth.addSound(sound)

    assert synth.getNumVoices() == 1
    assert synth.getNumSounds() == 1

#==================================================================================================

def test_synthesiser_note_on_off():
    """Test triggering notes on the synthesiser."""
    synth = yup.Synthesiser()

    voice = SimpleSynthVoice()
    sound = SimpleSynthSound(60, 72)

    synth.addVoice(voice)
    synth.addSound(sound)

    synth.setCurrentPlaybackSampleRate(44100.0)

    # Trigger a note
    synth.noteOn(1, 60, 0.8)

    # Voice should become active
    assert voice.isVoiceActive()
    assert voice.current_note == 60

    # Release the note
    synth.noteOff(1, 60, 0.0, True)

    # Voice should become inactive
    assert not voice.isVoiceActive()

#==================================================================================================

def test_synthesiser_render_voices():
    """Test rendering audio with synthesiser."""
    synth = yup.Synthesiser()

    voice = SimpleSynthVoice()
    sound = SimpleSynthSound()

    synth.addVoice(voice)
    synth.addSound(sound)

    synth.setCurrentPlaybackSampleRate(44100.0)

    # Trigger a note
    synth.noteOn(1, 69, 0.5)  # A440

    # Render some audio
    buffer = yup.AudioBuffer(2, 512)
    buffer.clear()

    # Create empty MIDI buffer for rendering
    midi_buffer = yup.MidiBuffer()

    synth.renderNextBlock(buffer, midi_buffer, 0, 512)

    # Check that audio was rendered
    mag = buffer.getMagnitude(0, 0, 512)
    assert mag > 0.01
