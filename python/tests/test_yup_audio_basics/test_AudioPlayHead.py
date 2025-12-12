import yup

#==================================================================================================

def test_frame_rate_type_enum():
    # Test that enum values exist
    assert yup.AudioPlayHead.FrameRateType.fps24 is not None
    assert yup.AudioPlayHead.FrameRateType.fps25 is not None
    assert yup.AudioPlayHead.FrameRateType.fps30 is not None
    assert yup.AudioPlayHead.FrameRateType.fps60 is not None
    assert yup.AudioPlayHead.FrameRateType.fpsUnknown is not None

#==================================================================================================

def test_frame_rate_construction():
    # Test default construction
    frameRate = yup.AudioPlayHead.FrameRate()
    assert frameRate.getBaseRate() == 0

    # Test construction from enum
    frameRate = yup.AudioPlayHead.FrameRate(yup.AudioPlayHead.FrameRateType.fps24)
    assert frameRate.getBaseRate() == 24

#==================================================================================================

def test_frame_rate_properties():
    frameRate = yup.AudioPlayHead.FrameRate(yup.AudioPlayHead.FrameRateType.fps24)

    assert frameRate.getBaseRate() == 24
    assert not frameRate.isDrop()
    assert not frameRate.isPullDown()
    assert abs(frameRate.getEffectiveRate() - 24.0) < 0.001

    # Test 23.976
    frameRate = yup.AudioPlayHead.FrameRate(yup.AudioPlayHead.FrameRateType.fps23976)
    assert frameRate.getBaseRate() == 24
    assert frameRate.isPullDown()
    assert abs(frameRate.getEffectiveRate() - 23.976) < 0.01

#==================================================================================================

def test_frame_rate_with_methods():
    frameRate = yup.AudioPlayHead.FrameRate()

    # Test withBaseRate
    frameRate = frameRate.withBaseRate(30)
    assert frameRate.getBaseRate() == 30

    # Test withDrop
    frameRate = frameRate.withDrop(True)
    assert frameRate.isDrop()

    # Test withPullDown
    frameRate = frameRate.withPullDown(True)
    assert frameRate.isPullDown()

#==================================================================================================

def test_frame_rate_comparison():
    fr1 = yup.AudioPlayHead.FrameRate(yup.AudioPlayHead.FrameRateType.fps24)
    fr2 = yup.AudioPlayHead.FrameRate(yup.AudioPlayHead.FrameRateType.fps24)
    fr3 = yup.AudioPlayHead.FrameRate(yup.AudioPlayHead.FrameRateType.fps25)

    assert fr1 == fr2
    assert fr1 != fr3

#==================================================================================================

def test_frame_rate_get_type():
    frameRate = yup.AudioPlayHead.FrameRate(yup.AudioPlayHead.FrameRateType.fps30)
    assert frameRate.getType() == yup.AudioPlayHead.FrameRateType.fps30

    frameRate = yup.AudioPlayHead.FrameRate(yup.AudioPlayHead.FrameRateType.fps2997)
    assert frameRate.getType() == yup.AudioPlayHead.FrameRateType.fps2997

#==================================================================================================

def test_time_signature_construction():
    # Test default construction
    timeSig = yup.AudioPlayHead.TimeSignature()
    assert timeSig.numerator == 4
    assert timeSig.denominator == 4

#==================================================================================================

def test_time_signature_fields():
    timeSig = yup.AudioPlayHead.TimeSignature()

    timeSig.numerator = 3
    timeSig.denominator = 4

    assert timeSig.numerator == 3
    assert timeSig.denominator == 4

#==================================================================================================

def test_time_signature_comparison():
    ts1 = yup.AudioPlayHead.TimeSignature()
    ts1.numerator = 3
    ts1.denominator = 4

    ts2 = yup.AudioPlayHead.TimeSignature()
    ts2.numerator = 3
    ts2.denominator = 4

    ts3 = yup.AudioPlayHead.TimeSignature()
    ts3.numerator = 4
    ts3.denominator = 4

    assert ts1.numerator == ts2.numerator and ts1.denominator == ts2.denominator
    assert ts1.numerator != ts3.numerator or ts1.denominator != ts3.denominator

#==================================================================================================

def test_loop_points_construction():
    loopPoints = yup.AudioPlayHead.LoopPoints()
    assert loopPoints.ppqStart == 0.0
    assert loopPoints.ppqEnd == 0.0

#==================================================================================================

def test_loop_points_fields():
    loopPoints = yup.AudioPlayHead.LoopPoints()

    loopPoints.ppqStart = 4.0
    loopPoints.ppqEnd = 8.0

    assert abs(loopPoints.ppqStart - 4.0) < 0.001
    assert abs(loopPoints.ppqEnd - 8.0) < 0.001

#==================================================================================================

def test_loop_points_comparison():
    lp1 = yup.AudioPlayHead.LoopPoints()
    lp1.ppqStart = 4.0
    lp1.ppqEnd = 8.0

    lp2 = yup.AudioPlayHead.LoopPoints()
    lp2.ppqStart = 4.0
    lp2.ppqEnd = 8.0

    lp3 = yup.AudioPlayHead.LoopPoints()
    lp3.ppqStart = 0.0
    lp3.ppqEnd = 16.0

    assert abs(lp1.ppqStart - lp2.ppqStart) < 0.001 and abs(lp1.ppqEnd - lp2.ppqEnd) < 0.001
    assert abs(lp1.ppqStart - lp3.ppqStart) > 0.001 or abs(lp1.ppqEnd - lp3.ppqEnd) > 0.001

#==================================================================================================

def test_position_info_construction():
    posInfo = yup.AudioPlayHead.PositionInfo()

    # Default constructed should have no values set
    assert posInfo.getTimeInSamples() is None
    assert posInfo.getTimeInSeconds() is None
    assert posInfo.getBpm() is None
    assert not posInfo.getIsPlaying()
    assert not posInfo.getIsRecording()
    assert not posInfo.getIsLooping()

#==================================================================================================

def test_position_info_time_in_samples():
    posInfo = yup.AudioPlayHead.PositionInfo()

    posInfo.setTimeInSamples(44100)
    value = posInfo.getTimeInSamples()

    assert value is not None
    assert abs(value - 44100) < 1

#==================================================================================================

def test_position_info_time_in_seconds():
    posInfo = yup.AudioPlayHead.PositionInfo()

    posInfo.setTimeInSeconds(1.5)
    value = posInfo.getTimeInSeconds()

    assert value is not None
    assert abs(value - 1.5) < 0.001

#==================================================================================================

def test_position_info_bpm():
    posInfo = yup.AudioPlayHead.PositionInfo()

    posInfo.setBpm(120.0)
    value = posInfo.getBpm()

    assert value is not None
    assert abs(value - 120.0) < 0.001

#==================================================================================================

def test_position_info_time_signature():
    posInfo = yup.AudioPlayHead.PositionInfo()

    timeSig = yup.AudioPlayHead.TimeSignature()
    timeSig.numerator = 3
    timeSig.denominator = 4

    posInfo.setTimeSignature(timeSig)
    value = posInfo.getTimeSignature()

    assert value is not None
    assert value.numerator == 3
    assert value.denominator == 4

#==================================================================================================

def test_position_info_loop_points():
    posInfo = yup.AudioPlayHead.PositionInfo()

    loopPoints = yup.AudioPlayHead.LoopPoints()
    loopPoints.ppqStart = 4.0
    loopPoints.ppqEnd = 8.0

    posInfo.setLoopPoints(loopPoints)
    value = posInfo.getLoopPoints()

    assert value is not None
    assert abs(value.ppqStart - 4.0) < 0.001
    assert abs(value.ppqEnd - 8.0) < 0.001

#==================================================================================================

def test_position_info_bar_count():
    posInfo = yup.AudioPlayHead.PositionInfo()

    posInfo.setBarCount(16)
    value = posInfo.getBarCount()

    assert value is not None
    assert abs(value - 16) < 1

#==================================================================================================

def test_position_info_ppq_position():
    posInfo = yup.AudioPlayHead.PositionInfo()

    posInfo.setPpqPosition(12.5)
    value = posInfo.getPpqPosition()

    assert value is not None
    assert abs(value - 12.5) < 0.001

#==================================================================================================

def test_position_info_ppq_position_of_last_bar_start():
    posInfo = yup.AudioPlayHead.PositionInfo()

    posInfo.setPpqPositionOfLastBarStart(8.0)
    value = posInfo.getPpqPositionOfLastBarStart()

    assert value is not None
    assert abs(value - 8.0) < 0.001

#==================================================================================================

def test_position_info_frame_rate():
    posInfo = yup.AudioPlayHead.PositionInfo()

    frameRate = yup.AudioPlayHead.FrameRate(yup.AudioPlayHead.FrameRateType.fps25)
    posInfo.setFrameRate(frameRate)
    value = posInfo.getFrameRate()

    assert value is not None
    assert value.getBaseRate() == 25

#==================================================================================================

def test_position_info_edit_origin_time():
    posInfo = yup.AudioPlayHead.PositionInfo()

    posInfo.setEditOriginTime(10.0)
    value = posInfo.getEditOriginTime()

    assert value is not None
    assert abs(value - 10.0) < 0.001

#==================================================================================================

def test_position_info_host_time_ns():
    posInfo = yup.AudioPlayHead.PositionInfo()

    posInfo.setHostTimeNs(123456789)
    value = posInfo.getHostTimeNs()

    assert value is not None
    assert abs(value - 123456789) < 1

#==================================================================================================

def test_position_info_continuous_time():
    posInfo = yup.AudioPlayHead.PositionInfo()

    posInfo.setContinuousTimeInSamples(88200)
    value = posInfo.getContinuousTimeInSamples()

    assert value is not None
    assert abs(value - 88200) < 1

#==================================================================================================

def test_position_info_boolean_flags():
    posInfo = yup.AudioPlayHead.PositionInfo()

    # Test playing
    assert not posInfo.getIsPlaying()
    posInfo.setIsPlaying(True)
    assert posInfo.getIsPlaying()

    # Test recording
    assert not posInfo.getIsRecording()
    posInfo.setIsRecording(True)
    assert posInfo.getIsRecording()

    # Test looping
    assert not posInfo.getIsLooping()
    posInfo.setIsLooping(True)
    assert posInfo.getIsLooping()

#==================================================================================================

def test_position_info_comparison():
    pos1 = yup.AudioPlayHead.PositionInfo()
    pos1.setTimeInSamples(44100)
    pos1.setBpm(120.0)

    pos2 = yup.AudioPlayHead.PositionInfo()
    pos2.setTimeInSamples(44100)
    pos2.setBpm(120.0)

    pos3 = yup.AudioPlayHead.PositionInfo()
    pos3.setTimeInSamples(88200)

    # Compare individual fields instead of using equality operator
    assert pos1.getTimeInSamples() == pos2.getTimeInSamples()
    assert abs(pos1.getBpm() - pos2.getBpm()) < 0.001
    assert pos1.getTimeInSamples() != pos3.getTimeInSamples()

#==================================================================================================

def test_position_info_optional_values():
    posInfo = yup.AudioPlayHead.PositionInfo()

    # Values not set should return None
    assert posInfo.getTimeInSamples() is None
    assert posInfo.getBpm() is None
    assert posInfo.getTimeSignature() is None
    assert posInfo.getLoopPoints() is None

    # Set a value
    posInfo.setBpm(140.0)

    # Now getBpm should return a value
    assert posInfo.getBpm() is not None

    # But others should still be None
    assert posInfo.getTimeInSamples() is None
    assert posInfo.getTimeSignature() is None
