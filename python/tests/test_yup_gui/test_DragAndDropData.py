import yup

"""
DragAndDropData is the payload Component's drag-and-drop hooks carry, so these tests cover
both the value type itself and the component entry points that deliver it.

The platform (SDL) is what actually starts a drag, so nothing here goes through a real drop:
the component hooks are driven directly, which is what they are for.
"""


def text_payload(text="hello"):
    return yup.DragAndDropData().withText(text)


# ==============================================================================
# DragAndDropData
# ==============================================================================

def test_default_is_empty():
    data = yup.DragAndDropData()

    assert data.isEmpty() is True
    assert data.hasFiles() is False
    assert data.hasText() is False
    assert data.hasUris() is False
    assert data.getFiles().isEmpty() is True
    assert data.getText() == ""
    assert data.getUris().isEmpty() is True


def test_with_files_sets_files():
    expected = [yup.File("/tmp/one.txt"), yup.File("/tmp/two.txt")]

    data = yup.DragAndDropData().withFiles(expected)

    assert data.hasFiles() is True
    assert data.isEmpty() is False
    assert data.getFiles().size() == 2
    # File normalises and absolutises the path it is given, so compare File objects instead of
    # raw path strings (on Windows a "/tmp" path gains the current drive prefix).
    assert list(data.getFiles()) == expected


def test_with_files_rejects_anything_that_is_not_a_file():
    try:
        yup.DragAndDropData().withFiles(["not a file"])
    except TypeError:
        pass
    else:
        raise AssertionError("withFiles should reject non-File values")


def test_with_text_sets_text():
    data = text_payload()

    assert data.hasText() is True
    assert data.getText() == "hello"


def test_empty_text_does_not_count_as_text():
    data = text_payload("")

    assert data.hasText() is False
    assert data.isEmpty() is True


def test_with_uris_sets_uris():
    data = yup.DragAndDropData().withUris(yup.StringArray(["https://example.com"]))

    assert data.hasUris() is True
    assert data.isEmpty() is False
    assert data.getUris().size() == 1
    assert data.getUris()[0] == "https://example.com"


def test_builders_are_immutable():
    original = yup.DragAndDropData()

    withText = original.withText("hello")

    assert original.isEmpty() is True
    assert withText.hasText() is True


def test_builders_chain_and_preserve_previous_values():
    data = yup.DragAndDropData().withFiles([yup.File("/tmp/one.txt")]).withText("hello")

    assert data.hasFiles() is True
    assert data.hasText() is True
    assert data.getFiles().size() == 1
    assert data.getText() == "hello"


def test_get_files_hands_back_a_copy():
    data = yup.DragAndDropData().withFiles([yup.File("/tmp/one.txt")])

    files = data.getFiles()
    files.clear()

    assert data.getFiles().size() == 1


# ==============================================================================
# Component drag-and-drop hooks
# ==============================================================================

class DropTarget(yup.Component):
    def __init__(self):
        super().__init__()
        self.interested = False
        self.handlesDrop = False
        self.log = []

    def isInterestedInDrag(self, data):
        self.log.append(("isInterestedInDrag", data))
        return self.interested

    def itemsDropped(self, position, data):
        self.log.append(("itemsDropped", position, data))
        return self.handlesDrop

    def itemDragEnter(self, data, position):
        self.log.append(("itemDragEnter", data, position))

    def itemDragMove(self, data, position):
        self.log.append(("itemDragMove", data, position))

    def itemDragExit(self, data):
        self.log.append(("itemDragExit", data))


def test_default_hooks_do_not_handle_the_payload():
    component = yup.Component()
    data = text_payload()

    assert component.isInterestedInDrag(data) is False
    assert component.itemsDropped(yup.Point[float](10.0, 20.0), data) is False
    component.itemDragEnter(data, yup.Point[float](10.0, 20.0))
    component.itemDragMove(data, yup.Point[float](15.0, 25.0))
    component.itemDragExit(data)


def test_python_overrides_receive_the_payload():
    component = DropTarget()
    data = text_payload()
    position = yup.Point[float](10.0, 20.0)

    component.interested = True
    component.handlesDrop = True

    assert component.isInterestedInDrag(data) is True
    assert component.itemsDropped(position, data) is True

    component.itemDragEnter(data, position)
    component.itemDragMove(data, position)
    component.itemDragExit(data)

    assert [entry[0] for entry in component.log] == [
        "isInterestedInDrag",
        "itemsDropped",
        "itemDragEnter",
        "itemDragMove",
        "itemDragExit",
    ]


def test_override_receives_the_payload_and_position_the_caller_passed():
    component = DropTarget()

    component.itemDragEnter(text_payload(), yup.Point[float](1.0, 2.0))

    _, data, position = component.log[0]

    assert data.hasText() is True
    assert data.getText() == "hello"
    assert position.getX() == 1.0
    assert position.getY() == 2.0


def test_override_can_read_a_file_payload():
    component = DropTarget()

    component.itemsDropped(yup.Point[float](0.0, 0.0), yup.DragAndDropData().withFiles([yup.File("/tmp/one.txt")]))

    _, _, data = component.log[0]

    assert data.hasFiles() is True
    assert data.getFiles()[0] == yup.File("/tmp/one.txt")
