import gc

import pytest

import yup

"""
BindingProvider, the model side of a .yui document: DataTreeBindingProvider exposing the properties
of a DataTree, and a Python subclass of BindingProvider.

A provider as the model of a document is covered by test_yup_gui_markup/test_BindingProvider.py,
which takes its helpers from here.

The file runs through the gui_app fixture, for the reason test_YuiDocument.py gives.

Not covered: BindingProvider.Listener is not bound, so the notifications of a provider are observed
through the components linked to it, in that file.
"""

pytestmark = pytest.mark.usefixtures("gui_app")

NUMBER = yup.ComponentProperty.Type.number



def set_values(tree, **values):
    transaction = tree.beginTransaction()

    for name, value in values.items():
        transaction.setProperty(yup.Identifier(name), value)

    transaction.commit()


def make_params(**values):
    tree = yup.DataTree(yup.Identifier("params"))
    set_values(tree, **values)

    return tree


def value_of(tree, name):
    return tree.getProperty(yup.Identifier(name))


class GainProvider(yup.BindingProvider):
    """A model with a single number, recording its gestures like the C++ RecordingProvider."""

    def __init__(self, gain=0.5):
        super().__init__()
        self.gain = gain
        self.gestures = []

    def getProperties(self):
        return [yup.BindingProvider.Property("gain", NUMBER)]

    def getValue(self, name):
        return self.gain

    def setValue(self, name, value):
        self.gain = value
        self.notifyValueChanged(name)

    def beginGesture(self, name):
        self.gestures.append("begin " + str(name))

    def endGesture(self, name):
        self.gestures.append("end " + str(name))


# ==============================================================================
# Property
# ==============================================================================

def test_property_is_untyped_by_default():
    prop = yup.BindingProvider.Property()

    assert not prop.name.isValid()
    assert prop.type == yup.ComponentProperty.Type.any
    assert prop.elementType == yup.ComponentProperty.Type.any


def test_property_describes_a_value():
    prop = yup.BindingProvider.Property("curve", yup.ComponentProperty.Type.array, NUMBER)

    assert str(prop.name) == "curve"
    assert prop.type == yup.ComponentProperty.Type.array
    assert prop.elementType == NUMBER

    prop.name = "bypass"
    prop.type = yup.ComponentProperty.Type.boolean

    assert str(prop.name) == "bypass"
    assert prop.type == yup.ComponentProperty.Type.boolean


# ==============================================================================
# DataTreeBindingProvider
# ==============================================================================

def test_data_tree_provider_types_the_properties_from_their_values():
    tree = make_params(cutoff=0.4, steps=4, bypass=False, name="Lead")
    provider = yup.DataTreeBindingProvider(tree)

    types = {str(prop.name): prop.type for prop in provider.getProperties()}

    assert types == {
        "cutoff": NUMBER,
        "steps": NUMBER,
        "bypass": yup.ComponentProperty.Type.boolean,
        "name": yup.ComponentProperty.Type.string,
    }


def test_data_tree_provider_finds_single_properties():
    provider = yup.DataTreeBindingProvider(make_params(cutoff=0.4))

    cutoff = provider.findProperty("cutoff")
    assert cutoff is not None
    assert str(cutoff.name) == "cutoff"
    assert cutoff.type == NUMBER

    assert provider.findProperty("missing") is None


def test_data_tree_provider_reads_and_writes_the_tree():
    tree = make_params(cutoff=0.4)
    provider = yup.DataTreeBindingProvider(tree)

    assert provider.getTree() == tree
    assert provider.getValue("cutoff") == 0.4

    provider.setValue("cutoff", 0.8)
    assert value_of(tree, "cutoff") == 0.8


def test_data_tree_provider_exposes_the_properties_added_later():
    tree = make_params(cutoff=0.4)
    provider = yup.DataTreeBindingProvider(tree)

    set_values(tree, resonance=0.7)

    assert "resonance" in [str(prop.name) for prop in provider.getProperties()]
    assert provider.findProperty("resonance").type == NUMBER
    assert provider.getValue("resonance") == 0.7


def test_data_tree_provider_makes_each_gesture_an_undo_transaction():
    tree = make_params(cutoff=0.4)
    undo_manager = yup.UndoManager()
    provider = yup.DataTreeBindingProvider(tree, undo_manager)

    provider.beginGesture("cutoff")
    provider.setValue("cutoff", 0.5)
    provider.setValue("cutoff", 0.6)
    provider.endGesture("cutoff")

    provider.beginGesture("cutoff")
    provider.setValue("cutoff", 0.9)
    provider.endGesture("cutoff")

    assert undo_manager.getNumTransactions() == 2

    undo_manager.undo()
    assert value_of(tree, "cutoff") == 0.6

    undo_manager.undo()
    assert value_of(tree, "cutoff") == 0.4


def test_data_tree_provider_keeps_its_undo_manager_alive():
    tree = make_params(cutoff=0.4)
    provider = yup.DataTreeBindingProvider(tree, yup.UndoManager())
    gc.collect()

    provider.setValue("cutoff", 0.8)
    assert value_of(tree, "cutoff") == 0.8


# ==============================================================================
# A Python provider
# ==============================================================================

def test_python_provider_is_reached_through_the_base_class():
    provider = GainProvider(0.25)

    assert [str(prop.name) for prop in provider.getProperties()] == ["gain"]

    # findProperty is not overridden: the base class looks the name up in getProperties.
    gain = provider.findProperty("gain")
    assert gain is not None
    assert str(gain.name) == "gain"
    assert gain.type == NUMBER

    assert provider.findProperty("missing") is None
    assert provider.getValue("gain") == 0.25


def test_python_provider_must_override_the_pure_virtuals():
    class Incomplete(yup.BindingProvider):
        pass

    with pytest.raises(RuntimeError):
        Incomplete().getProperties()
