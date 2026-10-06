import pytest

import yup

"""
Components handed over to tabs, through the gui_app fixture, so they also run under the embedded
interpreter (test_TabBar.py needs juce_app, which only exists in the standalone bindings).
"""

pytestmark = pytest.mark.usefixtures("gui_app")


def test_tab_button_takes_over_its_custom_component():
    bar = yup.TabBar()
    tab = bar.addTab("first", "First")

    tab.setCustomComponent(yup.Label("custom"))
    assert tab.getCustomComponent() is not None

    tab.setCustomComponent(None)
    assert tab.getCustomComponent() is None


def test_tab_component_takes_over_its_pages():
    tabs = yup.TabComponent()

    button = tabs.addTab("page", "Page", yup.Component())

    assert button.getText() == "Page"
    assert tabs.getTabContent("page") is not None
