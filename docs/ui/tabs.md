# Tabs

Three classes in `yup_gui` make tabs:

- `TabBar` is a strip of tabs with one of them selected. On its own it works as
  a segmented control.
- `TabButton` is one tab of the strip.
- `TabComponent` is a `TabBar` with a page of content for each tab.

Tabs are addressed by identifier rather than by index, so a tab keeps its
identity when tabs are moved, added or removed. The selection indicator slides
from tab to tab, and a dragged tab pushes its neighbours aside as it moves.

```cpp
yup::TabBar clockFormat;
clockFormat.setLayout (yup::TabBar::Layout::fill);
clockFormat.addTab ("12h", "12-hour");
clockFormat.addTab ("24h", "24-hour");
clockFormat.onSelectionChanged = [] (const yup::Identifier& tabId)
{
    DBG ("Selected " << tabId.toString());
};
addAndMakeVisible (clockFormat);
```

## Tabs and selection

`addTab (id, text, insertIndex)` returns the new `TabButton`, which you can
configure further. The first tab added to an empty bar becomes the selected
tab.

| Method | Purpose |
| --- | --- |
| `addTab`, `removeTab`, `clearTabs`, `moveTab` | Change the tabs. |
| `getNumTabs`, `getTabId (index)`, `indexOfTab (id)`, `getTabButton (id)` | Look tabs up. |
| `setSelectedTab (id, notification)` | Select a tab. A null `Identifier` clears the selection. |
| `getSelectedTabId`, `getSelectedTabIndex` | The selected tab, or a null `Identifier` / -1 when there is none. |

When the selected tab is removed, the tab that takes its place is selected, or
the previous tab if the removed one was last. An empty bar has no selection.

A tab shows an optional icon, a text and an optional close button:

```cpp
auto& tab = bar.addTab ("table", "Table");
tab.setIconGlyph (YUP_ICON_TABLE);  // a glyph from the theme icon font
tab.setClosable (true);
```

- `setIconImage` shows an image as the icon instead of a glyph.
- A tab with an icon and no text is an icon-only tab.
- `setCustomComponent` replaces the icon and the text with any component. The
  tab takes ownership, keeps the component at its original size, and stops it
  from receiving mouse events, so clicking and dragging still act on the tab.
- `setEnabled (false)` on a tab makes clicks and keyboard navigation skip it.

## Callbacks and listeners

| Callback | Called when |
| --- | --- |
| `onSelectionChanged (id)` | The selection changes. |
| `onTabMoved (id, oldIndex, newIndex)` | The user dropped a dragged tab at a new position, or `moveTab` was called. |
| `onTabCloseRequested (id)` | The close button of a tab was clicked. When this is not set, the tab is removed. |

`setSelectedTab` and `moveTab` take a `NotificationType`. It applies to these
callbacks only.

A `TabBar::Listener` (`tabSelectionChanged`, `tabRemoved`, `tabMoved`) is
always told about changes, synchronously. It is meant for code that has to stay
in sync with the bar, like `TabComponent`.

## Looks and layout

| Setting | Values |
| --- | --- |
| `setVariant` | `pill` (the default) is a raised pill inside a rounded track. `underline` is an accent bar on the edge facing the content. |
| `setOrientation` | `horizontal` (the default) or `vertical`. |
| `setLayout` | `natural` (the default) makes each tab as long as its content. `fill` shares the length of the bar equally between the tabs. |
| `setFlipped` | Puts the underline on the top or left edge instead of the bottom or right one. |
| `setAnimationDuration` | How long the indicator and the tabs take to slide, in seconds. The default is 0.18; 0 moves them at once. |

A tab is sized for its text in the bold font of the selected tab, so tabs keep
their width when the selection changes.

## Tabs that do not fit

`setOverflow` chooses what happens when the tabs are longer than the bar:

- `menu` (the default): the trailing tabs are hidden behind a "More" button
  (`setOverflowText` changes its text), which lists them in a popup menu. When
  the selected tab is hidden, the button shows its icon and text and the
  indicator sits on it.
- `scroll`: the tabs scroll with the mouse wheel, and the selected tab always
  scrolls into view.
- `shrink`: the tabs shrink down to `TabBar::minimumTabLength`, and their text
  is shortened with an ellipsis.

## Reordering

`setReorderable (true)` lets the user drag tabs. A drag starts after the
pointer has moved 8 points along the bar. The dragged tab follows the pointer,
and its neighbours slide aside as it passes them. `onTabMoved` is called once,
on release, with the original and the final index. Tabs hidden behind the
overflow button cannot be dragged.

## Keyboard

The bar takes the keyboard focus when clicked. The arrow keys along the bar
(Left and Right, or Up and Down for a vertical bar) select the previous or next
enabled tab, wrapping around at the ends. Home and End select the first and the
last enabled tab. After keyboard navigation, the theme draws a focus ring
until the bar is clicked or loses the focus.

## Tab component

`TabComponent` adds a page for each tab. Only the page of the selected tab is
visible. The other pages stay children of the component, so they keep their
state.

```cpp
yup::TabComponent tabs;
tabs.addTab ("general", "General", std::make_unique<GeneralPage>());  // owned
tabs.addTab ("audio", "Audio", audioSettingsPage);                    // not owned
tabs.setTabBarPlacement (yup::TabComponent::Placement::left);
tabs.getTabBar().setVariant (yup::TabBar::Variant::underline);
```

- A page passed as `std::unique_ptr` is deleted with its tab. A page passed by
  reference must outlive its tab.
- `getTabBar()` gives access to every other setting, and pages follow changes
  made there: removing a tab from the bar also drops its page.
- `setTabBarPlacement` (`top`, `bottom`, `left`, `right`) also sets the bar
  orientation, and puts the underline on the edge facing the content.
- `setTabBarThickness` sets the size of the bar across its tabs. The default
  is 36.

## Styling

The tabs follow the theme palette. See [Component styling](component-styling.md)
for how colors resolve.

| Style | IDs |
| --- | --- |
| `TabBar::Style` | `trackColorId`, `indicatorColorId`, `underlineColorId`, `focusOutlineColorId` |
| `TabButton::Style` | `textColorId`, `textSelectedColorId`, `hoveredBackgroundColorId`, `closeButtonColorId` |

Colors set on a bar also apply to its tabs, since color lookups walk up the
parents.
