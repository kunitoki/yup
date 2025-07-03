import yup

#==================================================================================================

def test_ok():
    result = yup.Result.ok()
    assert not result.failed()
    assert result.wasOk()

    if result: pass
    else: assert False

    if not result: assert False

#==================================================================================================

def test_fail():
    result = yup.Result.fail("The cricket splatted")
    assert result.failed()
    assert not result.wasOk()
    assert result.getErrorMessage() == "The cricket splatted"

    if result: assert False

    if not result: pass
    else: assert False

#==================================================================================================

def test_comparison():
    ok1 = yup.Result.ok()
    ok2 = yup.Result.ok()
    fail1 = yup.Result.fail("The cricket splatted")
    fail2 = yup.Result.fail("The fox died")
    fail3 = yup.Result.fail("The cricket splatted")

    assert ok1 == ok2
    assert ok1 == ok1
    assert ok1 != fail1
    assert fail1 != fail2
    assert fail1 == fail3
    assert fail1 == fail1
