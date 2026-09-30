import pytest

import yup

"""
TabBar, TabButton and TabComponent: tabs addressed by identifier, the selection callbacks, and
pages owned by a TabComponent.

The bars are given a size but never shown, so no native window is needed. Animations are turned
off so the layout is final as soon as it changes.
"""

pytestmark = pytest.mark.usefixtures("juce_app")


def new_bar(*tab_ids):
    bar = yup.TabBar()
    bar.setAnimationDuration(0.0)
    bar.setBounds(0.0, 0.0, 600.0, 36.0)

    for tab_id in tab_ids:
        bar.addTab(tab_id, tab_id.upper())

    return bar


def tab_order(bar):
    return [str(bar.getTabId(index)) for index in range(bar.getNumTabs())]


def test_first_tab_is_selected_and_reported():
    bar = new_bar()
    selections = []
    bar.onSelectionChanged = lambda tab_id: selections.append(str(tab_id))

    button = bar.addTab("table", "Table")

    assert str(bar.getSelectedTabId()) == "table"
    assert selections == ["table"]
    assert button.getText() == "Table"
    assert str(button.getTabId()) == "table"


def test_tabs_are_addressed_by_identifier():
    bar = new_bar("a", "b", "c")

    bar.moveTab("a", 2)
    assert tab_order(bar) == ["b", "c", "a"]
    assert bar.indexOfTab("a") == 2

    bar.removeTab("c")
    assert tab_order(bar) == ["b", "a"]
    assert bar.getTabButton("c") is None


def test_tab_content_can_be_configured():
    bar = new_bar("a")
    button = bar.getTabButton("a")
    length = button.getPreferredLength()

    button.setIconGlyph("x")
    button.setClosable(True)

    assert button.hasIcon()
    assert button.isClosable()
    assert button.getPreferredLength() > length


def test_settings_round_trip():
    bar = new_bar()

    bar.setVariant(yup.TabBar.Variant.underline)
    bar.setLayout(yup.TabBar.Layout.fill)
    bar.setOverflow(yup.TabBar.Overflow.scroll)
    bar.setOrientation(yup.TabBar.Orientation.vertical)
    bar.setReorderable(True)

    assert bar.getVariant() == yup.TabBar.Variant.underline
    assert bar.getLayout() == yup.TabBar.Layout.fill
    assert bar.getOverflow() == yup.TabBar.Overflow.scroll
    assert bar.getOrientation() == yup.TabBar.Orientation.vertical
    assert bar.isReorderable()
    assert yup.TabBar.minimumTabLength > 0


def test_tab_component_shows_the_selected_page():
    tabs = yup.TabComponent()
    tabs.getTabBar().setAnimationDuration(0.0)
    tabs.setBounds(0.0, 0.0, 400.0, 300.0)

    tabs.addTab("first", "First", yup.Component())
    tabs.addTab("second", "Second", yup.Component())

    assert tabs.getTabContent("first").isVisible()
    assert not tabs.getTabContent("second").isVisible()

    tabs.getTabBar().setSelectedTab("second")

    assert not tabs.getTabContent("first").isVisible()
    assert tabs.getTabContent("second").isVisible()

    tabs.removeTab("second")
    assert tabs.getTabContent("second") is None
