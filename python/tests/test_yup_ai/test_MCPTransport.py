import yup

#==================================================================================================

class QueueTransport(yup.ai.MCPTransport):
    def __init__(self):
        super().__init__()
        self.connected = False
        self.sent_messages = []
        self.queued_messages = []
        self.handler = None

    def sendMessage(self, message):
        self.sent_messages.append(message)
        return True

    def receiveMessage(self, timeoutMs=-1):
        if not self.queued_messages:
            return None

        return self.queued_messages.pop(0)

    def setMessageHandler(self, handler):
        self.handler = handler

    def start(self):
        self.connected = True
        return True

    def stop(self):
        self.connected = False

    def isConnected(self):
        return self.connected

#==================================================================================================

def test_python_transport_subclass_sends_and_receives_messages():
    transport = QueueTransport()

    assert transport.start()
    assert transport.isConnected()

    assert transport.sendMessage({ "jsonrpc": "2.0", "method": "ping" })
    assert transport.sent_messages[0]["method"] == "ping"

    transport.queued_messages.append({
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
            "ok": True,
        },
    })

    assert transport.receiveMessage()["result"]["ok"]

    transport.stop()
    assert not transport.isConnected()

#==================================================================================================

def test_python_transport_message_handler_can_be_called():
    transport = QueueTransport()
    received = []

    transport.setMessageHandler(lambda message: received.append(message))
    transport.handler({ "jsonrpc": "2.0", "method": "notifications/test" })

    assert received[0]["method"] == "notifications/test"
