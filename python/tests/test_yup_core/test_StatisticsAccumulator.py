from math import isinf, isclose, sqrt

import yup

#==================================================================================================

def _accumulate(values, accumulator):
    for value in values:
        accumulator.addValue(value)

    return accumulator

#==================================================================================================

def test_empty_accumulator_reports_zero_for_everything_it_can():
    accumulator = yup.StatisticsAccumulator[float]()

    assert accumulator.getCount() == 0
    assert accumulator.getAverage() == 0.0
    assert accumulator.getEnergy() == 0.0
    assert accumulator.getVariance() == 0.0

    # With no samples min/max stay at their infinities.
    assert isinf(accumulator.getMinValue())
    assert isinf(accumulator.getMaxValue())

#==================================================================================================

def test_float_accumulator_statistics():
    accumulator = _accumulate([1.0, 2.0, 3.0, 4.0], yup.StatisticsAccumulator[float]())

    assert accumulator.getCount() == 4
    assert accumulator.getAverage() == 2.5
    assert accumulator.getEnergy() == 30.0
    assert accumulator.getVariance() == 1.25
    assert accumulator.getMinValue() == 1.0
    assert accumulator.getMaxValue() == 4.0

    assert isclose(accumulator.getStandardDeviation(), sqrt(1.25), rel_tol=1e-6)

#==================================================================================================
#==================================================================================================

def test_minimum_and_maximum_track_the_extremes():
    accumulator = _accumulate([-4.0, 0.0, 12.5], yup.StatisticsAccumulator[float]())

    assert accumulator.getMinValue() == -4.0
    assert accumulator.getMaxValue() == 12.5

#==================================================================================================

def test_a_single_value_has_zero_variance():
    accumulator = _accumulate([7.0], yup.StatisticsAccumulator[float]())

    assert accumulator.getCount() == 1
    assert accumulator.getAverage() == 7.0
    assert accumulator.getVariance() == 0.0
    assert accumulator.getStandardDeviation() == 0.0

#==================================================================================================

def test_reset_clears_every_running_statistic():
    accumulator = _accumulate([1.0, 2.0], yup.StatisticsAccumulator[float]())

    accumulator.reset()

    assert accumulator.getCount() == 0
    assert accumulator.getAverage() == 0.0
    assert accumulator.getEnergy() == 0.0
    assert isinf(accumulator.getMinValue())

#==================================================================================================

def test_accumulating_again_after_reset():
    accumulator = _accumulate([1.0, 2.0], yup.StatisticsAccumulator[float]())
    accumulator.reset()

    accumulator.addValue(10.0)

    assert accumulator.getCount() == 1
    assert accumulator.getAverage() == 10.0
    assert accumulator.getMinValue() == 10.0
    assert accumulator.getMaxValue() == 10.0
