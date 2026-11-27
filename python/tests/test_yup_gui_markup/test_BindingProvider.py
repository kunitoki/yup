import gc

import pytest

import yup

from ..test_yup_gui.test_BindingProvider import GainProvider, make_params, set_values, value_of

"""
A BindingProvider as the model of a .yui document: a DataTreeBindingProvider, and a Python subclass
of BindingProvider.

The markup is the one the C++ tests of the builder link to a model: a property linked with <=> is
written to the model and follows it, and an expression reading the model is evaluated again when
the model changes.

The file runs through the gui_app fixture, for the reason test_YuiDocument.py gives.
"""

pytestmark = pytest.mark.usefixtures("gui_app")

LINKED = """
    Component {
        Slider { id: knob; value <=> params.cutoff; }
        Label { id: readout; text: format ("cutoff %.1f", params.cutoff); }
    }"""

SYNTH = """
    Component {
        Slider { id: knob; value <=> synth.gain; }
        Slider { id: other; value <=> synth.gain; }
    }"""


def load(document, text):
    result = document.loadFromString(text)

    assert result.wasOk(), result.getErrorMessage()
    assert document.getDiagnostics() == [], yup.DataTreeDiagnostic.join(document.getDiagnostics())


# ==============================================================================
# A DataTree as the model of a document
# ==============================================================================

def test_data_tree_model_drives_the_interface_both_ways():
    tree = make_params(cutoff=0.4)
    provider = yup.DataTreeBindingProvider(tree, yup.UndoManager())

    document = yup.YuiDocument()
    document.setModel("params", provider)
    load(document, LINKED)

    knob = document.findById("knob")
    readout = document.findById("readout")
    assert knob.getProperty("value") == pytest.approx(0.4)

    # A change made in the interface is written to the model, and read by the expressions.
    knob.setProperty("value", 0.7)
    assert value_of(tree, "cutoff") == pytest.approx(0.7)
    assert readout.getProperty("text").startswith("cutoff 0.7")

    # A change made to the model shows up in the interface.
    set_values(tree, cutoff=0.2)
    assert knob.getProperty("value") == pytest.approx(0.2)
    assert readout.getProperty("text").startswith("cutoff 0.2")


def test_document_keeps_its_model_alive():
    document = yup.YuiDocument()

    # The document only refers to the provider, so it has to keep the Python object alive.
    document.setModel("params", yup.DataTreeBindingProvider(make_params(cutoff=0.4)))
    gc.collect()

    load(document, LINKED)

    assert document.findById("knob").getProperty("value") == pytest.approx(0.4)


def test_link_to_a_missing_model_is_reported():
    document = yup.YuiDocument()
    result = document.loadFromString("Slider { id: gain; value <=> model.gain; }")

    assert result.wasOk(), result.getErrorMessage()

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) == 1
    assert diagnostics[0].message == "Slider.value: unknown element 'model'"


def test_removed_model_is_no_longer_found():
    text = "Slider { id: gain; value <=> model.gain; }"
    provider = yup.DataTreeBindingProvider(make_params(gain=0.4))

    document = yup.YuiDocument()
    document.setModel("model", provider)
    load(document, text)

    assert document.findById("gain").getProperty("value") == pytest.approx(0.4)

    document.removeModel("model")

    # Links are applied again by each load, and this one has nothing to link to anymore.
    assert document.loadFromString(text).wasOk()

    diagnostics = document.getDiagnostics()
    assert len(diagnostics) == 1
    assert diagnostics[0].message == "Slider.value: unknown element 'model'"


# ==============================================================================
# A Python provider as the model of a document
# ==============================================================================

def test_python_provider_is_the_model_of_a_document():
    provider = GainProvider()

    document = yup.YuiDocument()
    document.setModel("synth", provider)
    load(document, SYNTH)

    knob = document.findById("knob")
    other = document.findById("other")
    assert knob.getProperty("value") == pytest.approx(0.5)
    assert other.getProperty("value") == pytest.approx(0.5)

    # A change made in the interface is written to the model, as a gesture of its own, and
    # notifyValueChanged tells the other component linked to it.
    knob.setProperty("value", 0.8)

    assert provider.gain == pytest.approx(0.8)
    assert provider.gestures == ["begin gain", "end gain"]
    assert other.getProperty("value") == pytest.approx(0.8)


def test_python_provider_notifies_the_interface_of_its_own_changes():
    provider = GainProvider()

    document = yup.YuiDocument()
    document.setModel("synth", provider)
    load(document, SYNTH)

    provider.setValue("gain", 0.2)

    assert document.findById("knob").getProperty("value") == pytest.approx(0.2)
    assert document.findById("other").getProperty("value") == pytest.approx(0.2)
    assert provider.gain == pytest.approx(0.2)


def test_document_keeps_a_python_provider_alive():
    document = yup.YuiDocument()
    document.setModel("synth", GainProvider(0.3))
    gc.collect()

    load(document, SYNTH)

    assert document.findById("knob").getProperty("value") == pytest.approx(0.3)
