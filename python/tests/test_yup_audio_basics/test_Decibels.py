import yup
import math

#==================================================================================================

def test_decibels_to_gain():
    # Test 0 dB = gain of 1
    gain = yup.Decibels.decibelsToGain(0.0)
    assert abs(gain - 1.0) < 0.001

    # Test 6 dB ≈ gain of 2
    gain = yup.Decibels.decibelsToGain(6.0)
    assert abs(gain - 2.0) < 0.1

    # Test -6 dB ≈ gain of 0.5
    gain = yup.Decibels.decibelsToGain(-6.0)
    assert abs(gain - 0.5) < 0.1

    # Test -infinity
    gain = yup.Decibels.decibelsToGain(-100.0)
    assert gain == 0.0

#==================================================================================================

def test_gain_to_decibels():
    # Test gain of 1 = 0 dB
    db = yup.Decibels.gainToDecibels(1.0)
    assert abs(db - 0.0) < 0.001

    # Test gain of 2 ≈ 6 dB
    db = yup.Decibels.gainToDecibels(2.0)
    assert abs(db - 6.0) < 0.1

    # Test gain of 0.5 ≈ -6 dB
    db = yup.Decibels.gainToDecibels(0.5)
    assert abs(db - (-6.0)) < 0.1

    # Test gain of 0 = -infinity
    db = yup.Decibels.gainToDecibels(0.0)
    assert db == -100.0

#==================================================================================================

def test_decibels_round_trip():
    # Test round-trip conversion
    original_db = -12.0
    gain = yup.Decibels.decibelsToGain(original_db)
    converted_db = yup.Decibels.gainToDecibels(gain)

    assert abs(original_db - converted_db) < 0.001

#==================================================================================================

def test_gain_with_lower_bound():
    # Test that gain is clamped to lower bound
    gain = yup.Decibels.gainWithLowerBound(0.0001, -40.0)
    lowerBoundGain = yup.Decibels.decibelsToGain(-40.0)

    # Allow small tolerance for floating point comparison
    assert gain >= lowerBoundGain * 0.99

    # Test that higher gains pass through unchanged
    gain = yup.Decibels.gainWithLowerBound(1.0, -40.0)
    assert abs(gain - 1.0) < 0.001

#==================================================================================================

def test_decibels_to_string():
    # Test basic formatting
    s = yup.Decibels.toString(0.0)
    assert "+0" in s
    assert "dB" in s

    # Test negative value
    s = yup.Decibels.toString(-6.0)
    assert "-6" in s
    assert "dB" in s

    # Test positive value
    s = yup.Decibels.toString(3.0)
    assert "+3" in s

    # Test decimal places
    s = yup.Decibels.toString(-12.345, 2)
    assert "-12.3" in s or "-12.4" in s

    # Test without suffix
    s = yup.Decibels.toString(0.0, 2, -100.0, False)
    assert "dB" not in s

    # Test minus infinity
    s = yup.Decibels.toString(-100.0, 2, -100.0)
    assert "INF" in s

#==================================================================================================

def test_decibels_custom_minus_infinity():
    # Test custom minus infinity threshold
    gain = yup.Decibels.decibelsToGain(-50.0, -50.0)
    assert gain == 0.0

    db = yup.Decibels.gainToDecibels(0.0, -50.0)
    assert db == -50.0

    s = yup.Decibels.toString(-50.0, 2, -50.0)
    assert "INF" in s
