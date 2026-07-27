import yup
import math

#==================================================================================================

def test_iir_coefficients_construction():
    coeffs = yup.IIRCoefficients()
    assert coeffs is not None

#==================================================================================================

def test_iir_coefficients_make_low_pass():
    sampleRate = 44100.0
    frequency = 1000.0

    coeffs = yup.IIRCoefficients.makeLowPass(sampleRate, frequency)
    assert coeffs is not None

#==================================================================================================

def test_iir_coefficients_make_high_pass():
    sampleRate = 44100.0
    frequency = 1000.0

    coeffs = yup.IIRCoefficients.makeHighPass(sampleRate, frequency)
    assert coeffs is not None

#==================================================================================================

def test_iir_coefficients_make_band_pass():
    sampleRate = 44100.0
    frequency = 1000.0

    coeffs = yup.IIRCoefficients.makeBandPass(sampleRate, frequency)
    assert coeffs is not None

#==================================================================================================

def test_iir_coefficients_make_low_shelf():
    sampleRate = 44100.0
    cutOffFrequency = 1000.0
    Q = 1.0
    gainFactor = 2.0

    coeffs = yup.IIRCoefficients.makeLowShelf(sampleRate, cutOffFrequency, Q, gainFactor)
    assert coeffs is not None

#==================================================================================================

def test_iir_coefficients_make_high_shelf():
    sampleRate = 44100.0
    cutOffFrequency = 1000.0
    Q = 1.0
    gainFactor = 2.0

    coeffs = yup.IIRCoefficients.makeHighShelf(sampleRate, cutOffFrequency, Q, gainFactor)
    assert coeffs is not None

#==================================================================================================

def test_iir_coefficients_make_peak_filter():
    sampleRate = 44100.0
    centerFrequency = 1000.0
    Q = 1.0
    gainFactor = 2.0

    coeffs = yup.IIRCoefficients.makePeakFilter(sampleRate, centerFrequency, Q, gainFactor)
    assert coeffs is not None

#==================================================================================================

def test_iir_coefficients_make_notch_filter():
    sampleRate = 44100.0
    frequency = 1000.0
    Q = 1.0

    coeffs = yup.IIRCoefficients.makeNotchFilter(sampleRate, frequency, Q)
    assert coeffs is not None

#==================================================================================================

def test_iir_coefficients_make_all_pass():
    sampleRate = 44100.0
    frequency = 1000.0
    Q = 1.0

    coeffs = yup.IIRCoefficients.makeAllPass(sampleRate, frequency, Q)
    assert coeffs is not None

#==================================================================================================

def test_iir_filter_construction():
    filter = yup.IIRFilter()
    assert filter is not None

#==================================================================================================

def test_iir_filter_reset():
    filter = yup.IIRFilter()
    filter.reset()

    # Should not throw
    assert True

#==================================================================================================

def test_iir_filter_set_coefficients():
    filter = yup.IIRFilter()
    coeffs = yup.IIRCoefficients.makeLowPass(44100.0, 1000.0)

    filter.setCoefficients(coeffs)

    # Should not throw
    assert True

#==================================================================================================

def test_iir_filter_process_single_sample():
    filter = yup.IIRFilter()
    coeffs = yup.IIRCoefficients.makeLowPass(44100.0, 1000.0)
    filter.setCoefficients(coeffs)

    # Process a sample
    input_sample = 1.0
    output_sample = filter.processSingleSampleRaw(input_sample)

    # Output should be a valid number
    assert not math.isnan(output_sample)
    assert not math.isinf(output_sample)

#==================================================================================================

def test_iir_filter_low_pass_behavior():
    # Create a low pass filter at 1kHz
    filter = yup.IIRFilter()
    coeffs = yup.IIRCoefficients.makeLowPass(44100.0, 1000.0)
    filter.setCoefficients(coeffs)

    # Process DC (0 Hz) - should pass through
    filter.reset()
    dc_output = 0.0
    for _ in range(100):
        dc_output = filter.processSingleSampleRaw(1.0)

    # DC should mostly pass (might be slightly attenuated)
    assert dc_output > 0.4  # Allow more tolerance

#==================================================================================================

def test_iir_filter_high_pass_behavior():
    # Create a high pass filter at 1kHz
    filter = yup.IIRFilter()
    coeffs = yup.IIRCoefficients.makeHighPass(44100.0, 1000.0)
    filter.setCoefficients(coeffs)

    # Process DC (0 Hz) - should be blocked
    filter.reset()
    dc_output = 0.0
    for _ in range(100):
        dc_output = filter.processSingleSampleRaw(1.0)

    # DC should be blocked (output near 0)
    assert abs(dc_output) < 0.1

#==================================================================================================

def test_iir_filter_multiple_filters():
    # Test that multiple filters can be created and used independently
    filter1 = yup.IIRFilter()
    filter2 = yup.IIRFilter()

    coeffs1 = yup.IIRCoefficients.makeLowPass(44100.0, 1000.0)
    coeffs2 = yup.IIRCoefficients.makeHighPass(44100.0, 2000.0)

    filter1.setCoefficients(coeffs1)
    filter2.setCoefficients(coeffs2)

    # Process samples through both
    output1 = filter1.processSingleSampleRaw(1.0)
    output2 = filter2.processSingleSampleRaw(1.0)

    assert not math.isnan(output1)
    assert not math.isnan(output2)

#==================================================================================================

def test_iir_filter_stability():
    # Test that filter remains stable with various inputs
    filter = yup.IIRFilter()
    coeffs = yup.IIRCoefficients.makeLowPass(44100.0, 5000.0)
    filter.setCoefficients(coeffs)

    # Process various inputs
    inputs = [0.0, 1.0, -1.0, 0.5, -0.5]

    for input_val in inputs:
        output = filter.processSingleSampleRaw(input_val)
        assert not math.isnan(output)
        assert not math.isinf(output)
        assert abs(output) < 10.0  # Reasonable output range
