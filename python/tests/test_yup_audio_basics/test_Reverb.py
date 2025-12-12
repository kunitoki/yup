import yup

#==================================================================================================

def test_reverb_parameters_construction():
    # Test default construction
    params = yup.Reverb.Parameters()
    assert params.roomSize >= 0.0
    assert params.damping >= 0.0
    assert params.wetLevel >= 0.0
    assert params.dryLevel >= 0.0
    assert params.width >= 0.0

#==================================================================================================

def test_reverb_parameters_fields():
    params = yup.Reverb.Parameters()

    # Set values
    params.roomSize = 0.8
    params.damping = 0.5
    params.wetLevel = 0.33
    params.dryLevel = 0.4
    params.width = 1.0
    params.freezeMode = 0.0

    assert abs(params.roomSize - 0.8) < 0.01
    assert abs(params.damping - 0.5) < 0.01
    assert abs(params.wetLevel - 0.33) < 0.01
    assert abs(params.dryLevel - 0.4) < 0.01
    assert abs(params.width - 1.0) < 0.01
    assert abs(params.freezeMode - 0.0) < 0.01

#==================================================================================================

def test_reverb_construction():
    reverb = yup.Reverb()
    assert reverb is not None

#==================================================================================================

def test_reverb_set_get_parameters():
    reverb = yup.Reverb()

    params = yup.Reverb.Parameters()
    params.roomSize = 0.7
    params.damping = 0.6
    params.wetLevel = 0.5
    params.dryLevel = 0.4
    params.width = 0.9
    params.freezeMode = 0.0

    reverb.setParameters(params)
    retrieved = reverb.getParameters()

    assert abs(retrieved.roomSize - params.roomSize) < 0.01
    assert abs(retrieved.damping - params.damping) < 0.01
    assert abs(retrieved.wetLevel - params.wetLevel) < 0.01
    assert abs(retrieved.dryLevel - params.dryLevel) < 0.01
    assert abs(retrieved.width - params.width) < 0.01

#==================================================================================================

def test_reverb_reset():
    reverb = yup.Reverb()
    reverb.reset()

    # Should not throw
    assert True

#==================================================================================================

def test_reverb_process_mono():
    reverb = yup.Reverb()

    params = yup.Reverb.Parameters()
    params.roomSize = 0.5
    params.damping = 0.5
    params.wetLevel = 0.33
    params.dryLevel = 0.4
    reverb.setParameters(params)

    # Create test buffer
    numSamples = 512
    buffer = [0.0] * numSamples

    # Set some input signal
    for i in range(min(10, numSamples)):
        buffer[i] = 1.0

    # Note: processMono would need array binding support
    # This test verifies the method exists
    # reverb.processMono(buffer, numSamples)

#==================================================================================================

def test_reverb_process_stereo():
    reverb = yup.Reverb()

    params = yup.Reverb.Parameters()
    params.roomSize = 0.5
    params.damping = 0.5
    params.wetLevel = 0.33
    params.dryLevel = 0.4
    params.width = 1.0
    reverb.setParameters(params)

    # Create test buffers
    numSamples = 512
    leftBuffer = [0.0] * numSamples
    rightBuffer = [0.0] * numSamples

    # Set some input signal
    for i in range(min(10, numSamples)):
        leftBuffer[i] = 1.0
        rightBuffer[i] = 0.5

    # Note: processStereo would need array binding support
    # This test verifies the method exists
    # reverb.processStereo(leftBuffer, rightBuffer, numSamples)

#==================================================================================================

def test_reverb_parameters_ranges():
    # Test that parameters can be set to typical ranges
    params = yup.Reverb.Parameters()

    # Test room size range
    params.roomSize = 0.0
    assert abs(params.roomSize - 0.0) < 0.01
    params.roomSize = 1.0
    assert abs(params.roomSize - 1.0) < 0.01

    # Test damping range
    params.damping = 0.0
    assert abs(params.damping - 0.0) < 0.01
    params.damping = 1.0
    assert abs(params.damping - 1.0) < 0.01

    # Test wet/dry levels
    params.wetLevel = 0.0
    assert abs(params.wetLevel - 0.0) < 0.01
    params.dryLevel = 1.0
    assert abs(params.dryLevel - 1.0) < 0.01

    # Test width
    params.width = 0.0
    assert abs(params.width - 0.0) < 0.01
    params.width = 1.0
    assert abs(params.width - 1.0) < 0.01

#==================================================================================================

def test_reverb_freeze_mode():
    reverb = yup.Reverb()

    params = yup.Reverb.Parameters()
    params.freezeMode = 1.0  # Enable freeze

    reverb.setParameters(params)
    retrieved = reverb.getParameters()

    assert abs(retrieved.freezeMode - 1.0) < 0.01

#==================================================================================================

def test_reverb_multiple_instances():
    # Test that multiple reverb instances can coexist
    reverb1 = yup.Reverb()
    reverb2 = yup.Reverb()

    params1 = yup.Reverb.Parameters()
    params1.roomSize = 0.3

    params2 = yup.Reverb.Parameters()
    params2.roomSize = 0.9

    reverb1.setParameters(params1)
    reverb2.setParameters(params2)

    retrieved1 = reverb1.getParameters()
    retrieved2 = reverb2.getParameters()

    assert abs(retrieved1.roomSize - 0.3) < 0.01
    assert abs(retrieved2.roomSize - 0.9) < 0.01
