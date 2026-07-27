import yup

#==================================================================================================

def test_audio_channel_set_construction():
    # Test default construction
    channels = yup.AudioChannelSet()
    assert channels.size() == 0

#==================================================================================================

def test_audio_channel_set_mono():
    channels = yup.AudioChannelSet.mono()
    assert channels.size() == 1
    assert channels.getDescription() == "Mono"

#==================================================================================================

def test_audio_channel_set_stereo():
    channels = yup.AudioChannelSet.stereo()
    assert channels.size() == 2
    assert channels.getDescription() == "Stereo"

#==================================================================================================

def test_audio_channel_set_5_1():
    channels = yup.AudioChannelSet.create5point1()
    assert channels.size() == 6
    assert "5.1" in channels.getDescription()

#==================================================================================================

def test_audio_channel_set_7_1():
    channels = yup.AudioChannelSet.create7point1()
    assert channels.size() == 8
    assert "7.1" in channels.getDescription()

#==================================================================================================

def test_audio_channel_set_discrete():
    channels = yup.AudioChannelSet.discreteChannels(4)
    assert channels.size() == 4
    assert channels.isDiscreteLayout()

#==================================================================================================

def test_audio_channel_set_add_channel():
    channels = yup.AudioChannelSet()
    channels.addChannel(yup.AudioChannelSet.ChannelType.Left)
    channels.addChannel(yup.AudioChannelSet.ChannelType.Right)

    assert channels.size() == 2

#==================================================================================================

def test_audio_channel_set_remove_channel():
    channels = yup.AudioChannelSet.stereo()
    assert channels.size() == 2

    channels.removeChannel(yup.AudioChannelSet.ChannelType.Right)
    assert channels.size() == 1

#==================================================================================================

def test_audio_channel_set_get_type_of_channel():
    channels = yup.AudioChannelSet.stereo()

    leftType = channels.getTypeOfChannel(0)
    assert leftType == yup.AudioChannelSet.ChannelType.Left

    rightType = channels.getTypeOfChannel(1)
    assert rightType == yup.AudioChannelSet.ChannelType.Right

#==================================================================================================

def test_audio_channel_set_get_channel_index_for_type():
    channels = yup.AudioChannelSet.stereo()

    leftIndex = channels.getChannelIndexForType(yup.AudioChannelSet.ChannelType.Left)
    assert leftIndex == 0

    rightIndex = channels.getChannelIndexForType(yup.AudioChannelSet.ChannelType.Right)
    assert rightIndex == 1

    # Non-existent channel
    centerIndex = channels.getChannelIndexForType(yup.AudioChannelSet.ChannelType.Center)
    assert centerIndex == -1

#==================================================================================================

def test_audio_channel_set_comparison():
    stereo1 = yup.AudioChannelSet.stereo()
    stereo2 = yup.AudioChannelSet.stereo()
    mono = yup.AudioChannelSet.mono()

    assert stereo1 == stereo2
    assert stereo1 != mono

#==================================================================================================

def test_audio_channel_set_channel_types():
    channels = yup.AudioChannelSet.create5point1()
    types = channels.getChannelTypes()

    # getChannelTypes returns an Array, not a Python list
    assert types.size() == 6
    assert types.contains(yup.AudioChannelSet.ChannelType.Left)
    assert types.contains(yup.AudioChannelSet.ChannelType.Right)
    assert types.contains(yup.AudioChannelSet.ChannelType.Center)
    assert types.contains(yup.AudioChannelSet.ChannelType.LFE)

#==================================================================================================

def test_audio_channel_set_abbreviated_name():
    leftName = yup.AudioChannelSet.getAbbreviatedChannelTypeName(yup.AudioChannelSet.ChannelType.Left)
    assert "L" in leftName

    rightName = yup.AudioChannelSet.getAbbreviatedChannelTypeName(yup.AudioChannelSet.ChannelType.Right)
    assert "R" in rightName

    centerName = yup.AudioChannelSet.getAbbreviatedChannelTypeName(yup.AudioChannelSet.ChannelType.Center)
    assert "C" in centerName

#==================================================================================================

def test_audio_channel_set_canonical():
    # Test canonical channel sets for common channel counts
    mono = yup.AudioChannelSet.canonicalChannelSet(1)
    assert mono == yup.AudioChannelSet.mono()

    stereo = yup.AudioChannelSet.canonicalChannelSet(2)
    assert stereo == yup.AudioChannelSet.stereo()

#==================================================================================================

def test_audio_channel_set_channel_sets_with_number_of_channels():
    # Get all known layouts for 2 channels
    layouts = yup.AudioChannelSet.channelSetsWithNumberOfChannels(2)
    assert layouts.size() >= 1  # At least stereo layout exists

    # All should have 2 channels
    for layout in layouts:
        assert layout.size() == 2
