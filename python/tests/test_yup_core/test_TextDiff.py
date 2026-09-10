import yup

#==================================================================================================

def test_identical_strings_produce_no_changes():
    diff = yup.TextDiff("unchanged", "unchanged")

    assert diff.getChanges() == []
    assert diff.appliedTo("unchanged") == "unchanged"

#==================================================================================================

def test_applying_the_diff_reproduces_the_target():
    diff = yup.TextDiff("hello world", "hello there")

    assert diff.appliedTo("hello world") == "hello there"

#==================================================================================================

def test_insertion_is_reported_as_a_change():
    diff = yup.TextDiff("abc", "abcd")
    changes = diff.getChanges()

    assert len(changes) == 1
    assert not changes[0].isDeletion()
    assert changes[0].insertedText == "d"

#==================================================================================================

def test_deletion_is_reported_as_a_change():
    diff = yup.TextDiff("abcd", "abc")
    changes = diff.getChanges()

    assert len(changes) == 1
    assert changes[0].isDeletion()
    assert changes[0].insertedText == ""

#==================================================================================================

def test_change_applied_to_reproduces_the_target():
    diff = yup.TextDiff("hello world", "hello there")
    changes = diff.getChanges()

    assert len(changes) >= 1

    text = "hello world"

    for change in changes:
        text = change.appliedTo(text)

    assert text == "hello there"

#==================================================================================================

def test_empty_to_non_empty():
    diff = yup.TextDiff("", "added")

    assert diff.appliedTo("") == "added"

#==================================================================================================

def test_non_empty_to_empty():
    diff = yup.TextDiff("removed", "")

    assert diff.appliedTo("removed") == ""
