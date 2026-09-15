import yup

"""
DragAndDropData is the payload carried by a drag-and-drop operation. These tests cover the value
type itself; the C++ side is what turns it into drops.
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
# DragAndDropData: MIME store, image and native object
# ==============================================================================

def test_new_payload_predicates_default_to_false():
    data = yup.DragAndDropData()

    assert data.hasImage() is False
    assert data.hasNativeObject() is False
    assert data.hasMimeData("application/x-yup") is False
    assert data.getMimeTypes().isEmpty() is True
    assert data.getImage().isValid() is False


def test_mime_types_reflect_the_stored_entries():
    data = yup.DragAndDropData().withText("hi")

    types = data.getMimeTypes()
    assert types.size() == 1
    assert types[0] == "text/plain;charset=utf-8"
    assert data.hasMimeData("text/plain;charset=utf-8") is True


def test_native_object_round_trips():
    data = yup.DragAndDropData().withNativeObject(42)

    assert data.hasNativeObject() is True
    assert data.isEmpty() is False
    assert data.getNativeObject() == 42


# ==============================================================================
# DragAndDropTargetComponent
# ==============================================================================

class DropTarget(yup.DragAndDropTargetComponent):
    def __init__(self):
        super().__init__()
        self.interested = False
        self.handlesDrop = False
        self.log = []

    def isInterestedInDragSource(self, details):
        self.log.append(("isInterestedInDragSource", details))
        return self.interested

    def itemDropped(self, details):
        self.log.append(("itemDropped", details))
        return self.handlesDrop

    def itemDragEnter(self, details):
        self.log.append(("itemDragEnter", details))

    def itemDragMove(self, details):
        self.log.append(("itemDragMove", details))

    def itemDragExit(self, details):
        self.log.append(("itemDragExit", details))


def details_for(data=None):
    details = yup.DragAndDropSourceDetails()
    details.data = data if data is not None else text_payload()
    details.localPosition = yup.Point[float](10.0, 20.0)
    return details


def test_target_defaults_are_not_interested():
    target = yup.DragAndDropTargetComponent()
    details = details_for()

    assert target.isInterestedInDragSource(details) is False
    assert target.itemDropped(details) is False


def test_target_is_a_component():
    target = yup.DragAndDropTargetComponent("target")

    assert isinstance(target, yup.Component)
    assert target.getTargetComponent() is not None
    assert target.getComponentID() == "target"


def test_python_target_receives_the_details():
    target = DropTarget()
    details = details_for()

    target.interested = True
    target.handlesDrop = True

    assert target.isInterestedInDragSource(details) is True
    assert target.itemDropped(details) is True

    target.itemDragEnter(details)
    target.itemDragMove(details)
    target.itemDragExit(details)

    assert [entry[0] for entry in target.log] == [
        "isInterestedInDragSource",
        "itemDropped",
        "itemDragEnter",
        "itemDragMove",
        "itemDragExit",
    ]


def test_details_carry_the_payload_and_position():
    target = DropTarget()
    details = details_for(yup.DragAndDropData().withFiles([yup.File("/tmp/one.txt")]))

    target.itemDragEnter(details)

    _, received = target.log[0]

    assert received.data.hasFiles() is True
    assert received.data.getFiles()[0] == yup.File("/tmp/one.txt")
    assert received.localPosition.getX() == 10.0
    assert received.localPosition.getY() == 20.0


def test_details_can_be_copied_out_of_the_callback():
    details = details_for()

    copy = yup.DragAndDropSourceDetails(details)

    assert copy.data.hasText() is True
    assert copy.data.getText() == "hello"
    assert copy.localPosition.getX() == details.localPosition.getX()


def test_std_function_hooks_are_assignable():
    target = yup.DragAndDropTargetComponent()
    seen = []

    target.onIsInterestedInDragSource = lambda details: True
    target.onItemDragEnter = lambda details: seen.append("enter")
    target.onItemDropped = lambda details: (seen.append(details.data.getText()), True)[1]

    details = details_for()

    assert target.onIsInterestedInDragSource(details) is True
    target.onItemDragEnter(details)
    assert target.onItemDropped(details) is True

    assert seen == ["enter", "hello"]
