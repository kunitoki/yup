import pytest

import yup

from utilities import pump_until

#==================================================================================================

class ActionListener(yup.ActionListener):
    timesCalled = 0
    lastMessage = None

    def actionListenerCallback(self, message):
        self.timesCalled += 1
        self.lastMessage = message

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_single_send(juce_app):
    b = yup.ActionBroadcaster()

    l = ActionListener()
    b.addActionListener(l)

    b.sendActionMessage("abc")
    assert l.timesCalled == 0
    assert l.lastMessage is None
    pump_until(juce_app, lambda: (l.timesCalled == 1) and (l.lastMessage == "abc"))
    assert l.timesCalled == 1
    assert l.lastMessage == "abc"

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_multi_send(juce_app):
    b = yup.ActionBroadcaster()

    l = ActionListener()
    b.addActionListener(l)

    b.sendActionMessage("1")
    b.sendActionMessage("2")
    b.sendActionMessage("3")
    assert l.timesCalled == 0
    assert l.lastMessage is None
    pump_until(juce_app, lambda: (l.timesCalled == 3) and (l.lastMessage == "3"))
    assert l.timesCalled == 3
    assert l.lastMessage == "3"

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_remove_listener(juce_app):
    b = yup.ActionBroadcaster()

    l = ActionListener()
    b.addActionListener(l)

    b.sendActionMessage("1")
    pump_until(juce_app, lambda: (l.timesCalled == 1) and (l.lastMessage == "1"))
    assert l.timesCalled == 1
    assert l.lastMessage == "1"

    b.removeActionListener(l)
    b.sendActionMessage("2")
    pump_until(juce_app, lambda: (l.timesCalled == 1) and (l.lastMessage == "1"))
    assert l.timesCalled == 1
    assert l.lastMessage == "1"

#==================================================================================================

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_remove_all_listeners(juce_app):
    b = yup.ActionBroadcaster()

    l1 = ActionListener()
    l2 = ActionListener()
    l3 = ActionListener()
    b.addActionListener(l1)
    b.addActionListener(l2)
    b.addActionListener(l3)

    b.sendActionMessage("bark")
    pump_until(juce_app, lambda: (l1.timesCalled == 1) and (l2.timesCalled == 1) and (l3.timesCalled == 1))
    assert l1.timesCalled == 1
    assert l2.timesCalled == 1
    assert l3.timesCalled == 1

    b.removeAllActionListeners()
    b.sendActionMessage("bork")
    pump_until(juce_app, lambda: (l1.timesCalled == 1) and (l2.timesCalled == 1) and (l3.timesCalled == 1))
    assert l1.timesCalled == 1
    assert l2.timesCalled == 1
    assert l3.timesCalled == 1
