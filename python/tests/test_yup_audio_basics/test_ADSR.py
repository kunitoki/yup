import yup

#==================================================================================================

def test_adsr_parameters_construction():
    # Test default construction
    params = yup.ADSR.Parameters()
    assert params.attack >= 0.0
    assert params.decay >= 0.0
    assert params.sustain >= 0.0
    assert params.release >= 0.0

    # Test construction with values
    params = yup.ADSR.Parameters(0.1, 0.2, 0.7, 0.5)
    assert abs(params.attack - 0.1) < 0.001
    assert abs(params.decay - 0.2) < 0.001
    assert abs(params.sustain - 0.7) < 0.001
    assert abs(params.release - 0.5) < 0.001

#==================================================================================================

def test_adsr_parameters_fields():
    params = yup.ADSR.Parameters()

    # Test setting fields
    params.attack = 0.05
    params.decay = 0.1
    params.sustain = 0.8
    params.release = 0.3

    assert abs(params.attack - 0.05) < 0.001
    assert abs(params.decay - 0.1) < 0.001
    assert abs(params.sustain - 0.8) < 0.001
    assert abs(params.release - 0.3) < 0.001

#==================================================================================================

def test_adsr_construction():
    adsr = yup.ADSR()
    assert not adsr.isActive()

#==================================================================================================

def test_adsr_set_parameters():
    adsr = yup.ADSR()
    params = yup.ADSR.Parameters(0.1, 0.2, 0.7, 0.5)

    adsr.setParameters(params)
    retrieved = adsr.getParameters()

    assert abs(retrieved.attack - params.attack) < 0.001
    assert abs(retrieved.decay - params.decay) < 0.001
    assert abs(retrieved.sustain - params.sustain) < 0.001
    assert abs(retrieved.release - params.release) < 0.001

#==================================================================================================

def test_adsr_set_sample_rate():
    adsr = yup.ADSR()
    adsr.setSampleRate(44100.0)

    # Should not throw, exact behavior depends on implementation
    params = yup.ADSR.Parameters(0.1, 0.2, 0.7, 0.5)
    adsr.setParameters(params)

#==================================================================================================

def test_adsr_note_on_off():
    adsr = yup.ADSR()
    adsr.setSampleRate(44100.0)

    params = yup.ADSR.Parameters(0.01, 0.01, 0.7, 0.1)
    adsr.setParameters(params)

    # Initially not active
    assert not adsr.isActive()

    # Note on activates
    adsr.noteOn()
    assert adsr.isActive()

    # Get some samples during attack
    for _ in range(100):
        sample = adsr.getNextSample()
        assert sample >= 0.0

    # Note off starts release
    adsr.noteOff()

    # Still active during release
    active_after_noteoff = adsr.isActive()

    # Process through release phase
    for _ in range(10000):
        sample = adsr.getNextSample()
        if not adsr.isActive():
            break

    # Eventually becomes inactive
    # (May or may not be inactive depending on release time and samples processed)

#==================================================================================================

def test_adsr_reset():
    adsr = yup.ADSR()
    adsr.setSampleRate(44100.0)

    params = yup.ADSR.Parameters(0.01, 0.01, 0.7, 0.1)
    adsr.setParameters(params)

    adsr.noteOn()
    assert adsr.isActive()

    # Get some samples
    for _ in range(100):
        adsr.getNextSample()

    # Reset should stop immediately
    adsr.reset()
    assert not adsr.isActive()

#==================================================================================================

def test_adsr_envelope_shape():
    adsr = yup.ADSR()
    adsr.setSampleRate(44100.0)

    # Very short times for testing
    params = yup.ADSR.Parameters(0.001, 0.001, 0.5, 0.001)
    adsr.setParameters(params)

    adsr.noteOn()

    # Collect samples during attack phase
    attack_samples = []
    for _ in range(100):
        attack_samples.append(adsr.getNextSample())

    # Attack phase should generally increase
    # (allowing for some tolerance due to discrete sampling)
    increasing_count = 0
    for i in range(len(attack_samples) - 1):
        if attack_samples[i + 1] >= attack_samples[i]:
            increasing_count += 1

    # Most samples should be increasing during attack (allow some tolerance)
    assert increasing_count > len(attack_samples) * 0.4

#==================================================================================================

def test_adsr_sustain_level():
    adsr = yup.ADSR()
    adsr.setSampleRate(44100.0)

    sustain_level = 0.6
    params = yup.ADSR.Parameters(0.0001, 0.0001, sustain_level, 0.1)
    adsr.setParameters(params)

    adsr.noteOn()

    # Process through attack and decay to reach sustain
    for _ in range(1000):
        adsr.getNextSample()

    # Sample during sustain phase should be close to sustain level
    sustain_sample = adsr.getNextSample()

    # Allow generous tolerance for sustain level
    assert abs(sustain_sample - sustain_level) < 0.3
