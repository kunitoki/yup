import yup

millisecondsInSecond = 1000
secondsInMinute = 60
minutesInHour = 60
hoursInDay = 24
daysInWeek = 7

#==================================================================================================

def test_construct_empty():
    a = yup.RelativeTime.seconds(0)
    assert a.inMilliseconds() == 0
    assert a.inSeconds() == 0
    assert a.inMinutes() == 0
    assert a.inHours() == 0
    assert a.inDays() == 0
    assert a.inWeeks() == 0

#==================================================================================================

def test_construct_valid():
    a = yup.RelativeTime.milliseconds(1000)
    assert yup.approximatelyEqual (a.inMilliseconds(), 1000)
    assert yup.approximatelyEqual (a.inSeconds(), 1000 / millisecondsInSecond)
    assert yup.approximatelyEqual (a.inMinutes(), 1000 / millisecondsInSecond / secondsInMinute)
    assert yup.approximatelyEqual (a.inHours(), 1000 / millisecondsInSecond / secondsInMinute / minutesInHour)
    assert yup.approximatelyEqual (a.inDays(), 1000 / millisecondsInSecond / secondsInMinute / minutesInHour / hoursInDay)
    assert yup.approximatelyEqual (a.inWeeks(), 1000 / millisecondsInSecond / secondsInMinute / minutesInHour / hoursInDay / daysInWeek)

    a = yup.RelativeTime.seconds(5)
    assert yup.approximatelyEqual (a.inMilliseconds(), millisecondsInSecond * 5)
    assert yup.approximatelyEqual (a.inSeconds(), 5)
    assert yup.approximatelyEqual (a.inMinutes(), 5 / secondsInMinute)
    assert yup.approximatelyEqual (a.inHours(), 5 / secondsInMinute / minutesInHour)
    assert yup.approximatelyEqual (a.inDays(), 5 / secondsInMinute / minutesInHour / hoursInDay)
    assert yup.approximatelyEqual (a.inWeeks(), 5 / secondsInMinute / minutesInHour / hoursInDay / daysInWeek)

    a = yup.RelativeTime.minutes(120)
    assert yup.approximatelyEqual (a.inMilliseconds(), millisecondsInSecond * secondsInMinute * 120)
    assert yup.approximatelyEqual (a.inSeconds(), secondsInMinute * 120)
    assert yup.approximatelyEqual (a.inMinutes(), 120)
    assert yup.approximatelyEqual (a.inHours(), 120 / minutesInHour)
    assert yup.approximatelyEqual (a.inDays(), 120 / minutesInHour / hoursInDay)
    assert yup.approximatelyEqual (a.inWeeks(), 120 / minutesInHour / hoursInDay / daysInWeek)

    a = yup.RelativeTime.hours(25)
    assert yup.approximatelyEqual (a.inMilliseconds(), millisecondsInSecond * secondsInMinute * minutesInHour * 25)
    assert yup.approximatelyEqual (a.inSeconds(), secondsInMinute * minutesInHour * 25)
    assert yup.approximatelyEqual (a.inMinutes(), minutesInHour * 25)
    assert yup.approximatelyEqual (a.inHours(), 25)
    assert yup.approximatelyEqual (a.inDays(), 25 / hoursInDay)
    assert yup.approximatelyEqual (a.inWeeks(), 25 / hoursInDay / daysInWeek)

    a = yup.RelativeTime.days(5)
    assert yup.approximatelyEqual (a.inMilliseconds(), millisecondsInSecond * secondsInMinute * minutesInHour * hoursInDay * 5)
    assert yup.approximatelyEqual (a.inSeconds(), secondsInMinute * minutesInHour * hoursInDay * 5)
    assert yup.approximatelyEqual (a.inMinutes(), minutesInHour * hoursInDay * 5)
    assert yup.approximatelyEqual (a.inHours(), hoursInDay * 5)
    assert yup.approximatelyEqual (a.inDays(), 5)
    assert yup.approximatelyEqual (a.inWeeks(), 5 / daysInWeek)

    a = yup.RelativeTime.weeks(2)
    assert yup.approximatelyEqual (a.inMilliseconds(), millisecondsInSecond * secondsInMinute * minutesInHour * hoursInDay * daysInWeek * 2)
    assert yup.approximatelyEqual (a.inSeconds(), secondsInMinute * minutesInHour * hoursInDay * daysInWeek * 2)
    assert yup.approximatelyEqual (a.inMinutes(), minutesInHour * hoursInDay * daysInWeek * 2)
    assert yup.approximatelyEqual (a.inHours(), hoursInDay * daysInWeek * 2)
    assert yup.approximatelyEqual (a.inDays(), daysInWeek * 2)
    assert yup.approximatelyEqual (a.inWeeks(), 2)

#==================================================================================================

def test_operation_get_description():
    a = yup.RelativeTime.milliseconds(0)
    assert a.getDescription() == "0"
    assert a.getDescription("infinite") == "infinite"

    a = yup.RelativeTime.milliseconds(500)
    assert a.getDescription() == "500 ms"
    assert a.getDescription("infinite") == "500 ms"

    a += yup.RelativeTime.seconds(1)
    assert a.getDescription() == "1 sec"
    assert a.getDescription("infinite") == "1 sec"

    a += yup.RelativeTime.minutes(1)
    assert a.getDescription() == "1 min 1 sec"
    assert a.getDescription("infinite") == "1 min 1 sec"

    a += yup.RelativeTime.hours(1)
    assert a.getDescription() == "1 hr 1 min"
    assert a.getDescription("infinite") == "1 hr 1 min"

    a += yup.RelativeTime.days(1)
    assert a.getDescription() == "1 day 1 hr"
    assert a.getDescription("infinite") == "1 day 1 hr"

    a += yup.RelativeTime.weeks(1)
    assert a.getDescription() == "1 week 1 day"
    assert a.getDescription("infinite") == "1 week 1 day"

#==================================================================================================

def test_operation_get_approximate_description():
    a = yup.RelativeTime.milliseconds(0)
    assert a.getApproximateDescription() == "< 1 sec"

    a = yup.RelativeTime.milliseconds(500)
    assert a.getApproximateDescription() == "< 1 sec"

    a = yup.RelativeTime.seconds(140)
    assert a.getApproximateDescription() == "2 mins"

    a = yup.RelativeTime.minutes(110)
    assert a.getApproximateDescription() == "1 hr"

    a = yup.RelativeTime.hours(23) + yup.RelativeTime.minutes(60)
    assert a.getApproximateDescription() == "24 hrs"

    a = yup.RelativeTime.days(8)
    assert a.getApproximateDescription() == "8 days"

    a = yup.RelativeTime.weeks(2)
    assert a.getApproximateDescription() == "2 weeks"
