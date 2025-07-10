import pytest

import yup

#==================================================================================================

def test_message_manager_instance():
    mm = yup.MessageManager.getInstance()
    assert mm.isThisTheMessageThread()
    assert not mm.hasStopMessageBeenSent()
    assert mm.currentThreadHasLockedMessageManager()
    assert yup.MessageManager.existsAndIsCurrentThread()
    assert yup.MessageManager.existsAndIsLockedByCurrentThread()
    assert mm.getCurrentMessageThread() is not None

#==================================================================================================

def test_message_manager_run_dispatch_loop_until():
    mm = yup.MessageManager.getInstance()

    x = 10
    def testAsync():
        nonlocal x
        x = 11

    mm.callAsync(testAsync)
    assert x == 10
    mm.runDispatchLoopUntil(10)
    assert x == 11

#==================================================================================================

class ActionListener(yup.ActionListener):
    message = None

    def actionListenerCallback(self, message: str):
        self.message = message

def test_message_manager_broadcast_listener():
    mm = yup.MessageManager.getInstance()
    listener = ActionListener()

    mm.registerBroadcastListener(listener)
    mm.deliverBroadcastMessage("123")
    assert listener.message is None
    mm.runDispatchLoopUntil(10)
    assert listener.message == "123"

    mm.deregisterBroadcastListener(listener)
    mm.deliverBroadcastMessage("abc")
    mm.runDispatchLoopUntil(10)
    assert listener.message == "123"

#==================================================================================================

class Thread(yup.Thread):
    messageManagerWasLocked = False
    isThisTheMessageThread = True
    currentThreadHasLockedMessageManagerPre = True
    currentThreadHasLockedMessageManagerPost = False
    wasRunSuccesfully = False
    runResult = 0

    def run(self):
        mm = yup.MessageManager.getInstance()
        self.isThisTheMessageThread = mm.isThisTheMessageThread()
        self.currentThreadHasLockedMessageManagerPre = mm.currentThreadHasLockedMessageManager()

        with yup.MessageManagerLock(yup.Thread.getCurrentThread()) as ml:
            self.messageManagerWasLocked = ml.lockWasGained()
            self.currentThreadHasLockedMessageManagerPost = mm.currentThreadHasLockedMessageManager()

        self.runResult = mm.callFunctionOnMessageThread(self.runOnMessageThread)

    def runOnMessageThread(self):
        self.wasRunSuccesfully = True
        return 42

@pytest.mark.skipif(yup.__embedded_interpreter__, reason="Embedded interpreter does not support the test application")
def test_message_manager_lock(juce_app):
    t = Thread("backgrounder")
    t.startThread()

    for _ in range(10):
        next(juce_app)

    t.signalThreadShouldExit()
    t.waitForThreadToExit(1000)

    assert t.messageManagerWasLocked
    assert not t.isThisTheMessageThread
    assert not t.currentThreadHasLockedMessageManagerPre
    assert t.currentThreadHasLockedMessageManagerPost
    assert t.wasRunSuccesfully
    assert t.runResult == 42
