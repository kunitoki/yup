import yup

"""
TextInputTarget is the interface a component implements to accept text input, so the system
can show an on-screen keyboard (mobile) or an IME window (desktop) without covering the text
being edited.

Only the platform-independent half is testable here. The rect is what the system is told
about, while the native component those calls forward to only exists for a component that is
on the desktop - a target that is not a component at all therefore exercises the bookkeeping
without a native window, which is what these tests do. The forwarding path is covered
indirectly through TextEditor, which is a component that implements this interface.
"""


class Target(yup.TextInputTarget):
    def __init__(self, rect=None):
        super().__init__()
        self.rect = rect or yup.Rectangle[float](10.0, 20.0, 30.0, 40.0)

    def getTextInputRect(self):
        return self.rect


# ==============================================================================
# The pure virtual method
# ==============================================================================

def test_override_supplies_the_text_input_rect():
    target = Target()

    assert target.getTextInputRect() == yup.Rectangle[float](10.0, 20.0, 30.0, 40.0)


def test_override_is_consulted_on_every_call():
    target = Target()
    target.rect = yup.Rectangle[float](0.0, 0.0, 5.0, 6.0)

    assert target.getTextInputRect() == yup.Rectangle[float](0.0, 0.0, 5.0, 6.0)


def test_the_base_implementation_is_not_usable():
    # getTextInputRect is pure virtual, so the trampoline raises rather than inventing a rect.
    try:
        yup.TextInputTarget().getTextInputRect()
    except RuntimeError:
        pass
    else:
        raise AssertionError("the base implementation should not return a rect")


# ==============================================================================
# Text input bookkeeping
# ==============================================================================

def test_text_input_is_inactive_by_default():
    assert Target().isTextInputActive() is False


def test_requesting_text_input_activates_the_target():
    target = Target()
    target.requestTextInput()

    assert target.isTextInputActive() is True


def test_requesting_text_input_twice_is_a_no_op():
    target = Target()
    target.requestTextInput()
    target.requestTextInput()

    assert target.isTextInputActive() is True


def test_relinquishing_text_input_deactivates_the_target():
    target = Target()
    target.requestTextInput()
    target.relinquishTextInput()

    assert target.isTextInputActive() is False


def test_relinquishing_text_input_that_was_not_requested_is_a_no_op():
    target = Target()
    target.relinquishTextInput()

    assert target.isTextInputActive() is False


def test_updating_the_rect_while_inactive_is_a_no_op():
    target = Target()
    target.updateTextInputRect()

    assert target.isTextInputActive() is False


def test_updating_the_rect_while_active_keeps_the_target_active():
    target = Target()
    target.requestTextInput()
    target.updateTextInputRect()

    assert target.isTextInputActive() is True
