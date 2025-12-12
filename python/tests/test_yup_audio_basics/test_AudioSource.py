import yup

#==================================================================================================

class SimpleAudioSource(yup.AudioSource):
    """A simple audio source that generates a constant value."""

    def __init__(self):
        super().__init__()
        self.prepared = False
        self.sample_rate = 0.0
        self.block_size = 0
        self.released = False
        self.blocks_processed = 0
        self.output_value = 0.5

    def prepareToPlay(self, samplesPerBlockExpected, sampleRate):
        self.prepared = True
        self.sample_rate = sampleRate
        self.block_size = samplesPerBlockExpected

    def releaseResources(self):
        self.released = True
        self.prepared = False

    def getNextAudioBlock(self, bufferToFill):
        # Fill buffer with constant value
        buffer = bufferToFill.buffer
        start = bufferToFill.startSample
        num_samples = bufferToFill.numSamples

        for channel in range(buffer.getNumChannels()):
            for sample in range(num_samples):
                buffer.setSample(channel, start + sample, self.output_value)

        self.blocks_processed += 1

#==================================================================================================

def test_audio_source_subclass_creation():
    source = SimpleAudioSource()
    assert source is not None
    assert not source.prepared
    assert not source.released

#==================================================================================================

def test_audio_source_prepare_to_play():
    source = SimpleAudioSource()

    source.prepareToPlay(512, 44100.0)

    assert source.prepared
    assert abs(source.sample_rate - 44100.0) < 0.01
    assert source.block_size == 512

#==================================================================================================

def test_audio_source_release_resources():
    source = SimpleAudioSource()

    source.prepareToPlay(512, 44100.0)
    assert source.prepared

    source.releaseResources()
    assert source.released
    assert not source.prepared

#==================================================================================================

def test_audio_source_get_next_audio_block():
    source = SimpleAudioSource()
    source.prepareToPlay(512, 44100.0)

    # Create a buffer
    buffer = yup.AudioBuffer(2, 512)
    buffer.clear()

    # Create channel info
    info = yup.AudioSourceChannelInfo(buffer, 0, 512)

    # Get next block
    source.getNextAudioBlock(info)

    assert source.blocks_processed == 1

    # Check that buffer was filled with the output value
    assert abs(buffer.getSample(0, 0) - 0.5) < 0.001
    assert abs(buffer.getSample(1, 256) - 0.5) < 0.001

#==================================================================================================

def test_audio_source_multiple_blocks():
    source = SimpleAudioSource()
    source.prepareToPlay(256, 48000.0)

    buffer = yup.AudioBuffer(2, 256)
    info = yup.AudioSourceChannelInfo(buffer, 0, 256)

    # Process multiple blocks
    for i in range(5):
        buffer.clear()
        source.getNextAudioBlock(info)

    assert source.blocks_processed == 5

#==================================================================================================

def test_audio_source_custom_output_value():
    source = SimpleAudioSource()
    source.output_value = 0.75
    source.prepareToPlay(128, 44100.0)

    buffer = yup.AudioBuffer(1, 128)
    buffer.clear()
    info = yup.AudioSourceChannelInfo(buffer, 0, 128)

    source.getNextAudioBlock(info)

    # Check custom value
    assert abs(buffer.getSample(0, 0) - 0.75) < 0.001

#==================================================================================================

class PositionableTestSource(yup.PositionableAudioSource):
    """A positionable audio source for testing."""

    def __init__(self, totalLength=1000):
        super().__init__()
        self.prepared = False
        self.position = 0
        self.total_length = totalLength
        self.looping = False

    def prepareToPlay(self, samplesPerBlockExpected, sampleRate):
        self.prepared = True

    def releaseResources(self):
        self.prepared = False

    def getNextAudioBlock(self, bufferToFill):
        # Simple implementation that advances position
        buffer = bufferToFill.buffer
        num_samples = bufferToFill.numSamples

        # Fill with position-based value
        for channel in range(buffer.getNumChannels()):
            for i in range(num_samples):
                value = float(self.position + i) / self.total_length
                buffer.setSample(channel, bufferToFill.startSample + i, value)

        self.position += num_samples
        if self.looping and self.position >= self.total_length:
            self.position = 0

    def setNextReadPosition(self, newPosition):
        self.position = newPosition

    def getNextReadPosition(self):
        return self.position

    def getTotalLength(self):
        return self.total_length

    def isLooping(self):
        return self.looping

    def setLooping(self, shouldLoop):
        self.looping = shouldLoop

#==================================================================================================

def test_positionable_audio_source_creation():
    source = PositionableTestSource(1000)
    assert source is not None
    assert source.position == 0
    assert source.total_length == 1000
    assert not source.looping

#==================================================================================================

def test_positionable_audio_source_position():
    source = PositionableTestSource(1000)

    # Test set/get position
    source.setNextReadPosition(500)
    assert source.getNextReadPosition() == 500

    source.setNextReadPosition(0)
    assert source.getNextReadPosition() == 0

#==================================================================================================

def test_positionable_audio_source_total_length():
    source = PositionableTestSource(2000)
    assert source.getTotalLength() == 2000

#==================================================================================================

def test_positionable_audio_source_looping():
    source = PositionableTestSource(1000)

    assert not source.isLooping()

    source.setLooping(True)
    assert source.isLooping()

    source.setLooping(False)
    assert not source.isLooping()

#==================================================================================================

def test_positionable_audio_source_advancing():
    source = PositionableTestSource(1000)
    source.prepareToPlay(256, 44100.0)

    buffer = yup.AudioBuffer(2, 256)
    info = yup.AudioSourceChannelInfo(buffer, 0, 256)

    initial_pos = source.getNextReadPosition()
    source.getNextAudioBlock(info)

    # Position should have advanced
    assert source.getNextReadPosition() == initial_pos + 256

#==================================================================================================

def test_positionable_audio_source_looping_wraparound():
    source = PositionableTestSource(1000)
    source.setLooping(True)
    source.setNextReadPosition(900)
    source.prepareToPlay(256, 44100.0)

    buffer = yup.AudioBuffer(2, 256)
    info = yup.AudioSourceChannelInfo(buffer, 0, 256)

    source.getNextAudioBlock(info)

    # Position should have wrapped around due to looping
    assert source.getNextReadPosition() < 900
