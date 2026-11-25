import gc
import weakref

import yup

"""
FrameAttachment calls a function for every frame of the window showing a component.

Frames only happen in a live window, so what is covered here is what the binding adds on top
of the C++ class: how the attachment and the component own each other from Python.
"""


class AnimatedComponent(yup.Component):
    def __init__(self):
        yup.Component.__init__(self)
        self.frameAttachment = yup.FrameAttachment(self, self.advance)

    def advance(self, elapsedSeconds):
        pass


def test_attachment_of_own_method_does_not_keep_the_component_alive():
    component = AnimatedComponent()
    reference = weakref.ref(component)

    del component
    gc.collect()

    assert reference() is None


def test_attachment_can_outlive_its_component():
    component = yup.Component()
    attachment = yup.FrameAttachment(component, lambda elapsedSeconds: None)

    del component
    gc.collect()

    del attachment
