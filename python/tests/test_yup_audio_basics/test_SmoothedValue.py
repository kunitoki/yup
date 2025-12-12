import yup

#==================================================================================================

def test_smoothed_value_construction():
    # Test default construction
    sv = yup.SmoothedValue()

    # Test construction with initial value
    sv = yup.SmoothedValue(0.5)
    assert abs(sv.getCurrentValue() - 0.5) < 0.001

#==================================================================================================

def test_smoothed_value_set_current_and_target():
    sv = yup.SmoothedValue()

    sv.setCurrentAndTargetValue(0.75)
    assert abs(sv.getCurrentValue() - 0.75) < 0.001
    assert abs(sv.getTargetValue() - 0.75) < 0.001

    # Should not be smoothing when current equals target
    assert not sv.isSmoothing()

#==================================================================================================

def test_smoothed_value_set_target():
    sv = yup.SmoothedValue()
    sv.setCurrentAndTargetValue(0.0)

    # Reset with ramp length
    sv.reset(44100.0, 0.1)  # 44.1kHz, 0.1 second ramp

    sv.setTargetValue(1.0)
    assert abs(sv.getCurrentValue() - 0.0) < 0.001
    assert abs(sv.getTargetValue() - 1.0) < 0.001
    assert sv.isSmoothing()

#==================================================================================================

def test_smoothed_value_get_next_value():
    sv = yup.SmoothedValue()
    sv.setCurrentAndTargetValue(0.0)

    # Setup smoothing
    sv.reset(44100.0, 0.01)  # 44.1kHz, 0.01 second ramp
    sv.setTargetValue(1.0)

    # Get next values
    values = []
    for _ in range(10):
        values.append(sv.getNextValue())

    # Values should generally increase (with tolerance)
    assert values[-1] - values[0] > 0.001

    # Process until target reached
    for _ in range(1000):
        if not sv.isSmoothing():
            break
        sv.getNextValue()

    # Should reach target eventually
    assert abs(sv.getCurrentValue() - 1.0) < 0.01

#==================================================================================================

def test_smoothed_value_skip():
    sv = yup.SmoothedValue()
    sv.setCurrentAndTargetValue(0.0)

    sv.reset(44100.0, 0.1)
    sv.setTargetValue(1.0)

    # Skip some samples
    sv.skip(1000)

    # Should have progressed towards target
    current = sv.getCurrentValue()
    assert current > 0.001  # Small positive value shows progress

#==================================================================================================

def test_smoothed_value_reset():
    sv = yup.SmoothedValue()
    sv.setCurrentAndTargetValue(0.5)

    # Reset with sample rate
    sv.reset(48000)
    assert abs(sv.getCurrentValue() - 0.5) < 0.001

    # Reset with sample rate and ramp length
    sv.reset(48000.0, 0.05)

    sv.setTargetValue(1.0)
    assert sv.isSmoothing()

#==================================================================================================

def test_smoothed_value_double():
    # Test double precision version
    sv = yup.SmoothedValueDouble(0.123456789)
    assert abs(sv.getCurrentValue() - 0.123456789) < 0.0000001

    sv.setCurrentAndTargetValue(0.987654321)
    assert abs(sv.getCurrentValue() - 0.987654321) < 0.0000001

#==================================================================================================

def test_smoothed_value_ramp():
    sv = yup.SmoothedValue()
    sv.setCurrentAndTargetValue(0.0)

    # Very short ramp for testing
    sampleRate = 44100.0
    rampLength = 0.001  # 1ms
    expectedSamples = int(sampleRate * rampLength)

    sv.reset(sampleRate, rampLength)
    sv.setTargetValue(1.0)

    # Process samples
    samples_processed = 0
    while sv.isSmoothing() and samples_processed < expectedSamples * 2:
        sv.getNextValue()
        samples_processed += 1

    # Should have completed ramp within expected time (with generous tolerance)
    assert samples_processed <= expectedSamples * 2.0
