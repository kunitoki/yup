/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

   YUP is an open source library subject to open-source licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   to use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace yup
{

//==============================================================================

namespace
{

constexpr float tabBarPillInset = 3.0f;
constexpr float tabBarUnderlineThickness = 2.0f;
constexpr float tabBarDragThreshold = 8.0f;
constexpr float tabBarWheelStep = 48.0f;
constexpr float tabBarFitTolerance = 0.5f;
constexpr double tabBarMaxFrameGapSeconds = 1.0 / 15.0;

float tabBarEaseOut (double progress)
{
    const auto remaining = 1.0 - jlimit (0.0, 1.0, progress);
    return static_cast<float> (1.0 - remaining * remaining);
}

float tabBarStart (Rectangle<float> area, bool vertical)
{
    return vertical ? area.getY() : area.getX();
}

float tabBarLength (Rectangle<float> area, bool vertical)
{
    return vertical ? area.getHeight() : area.getWidth();
}

Rectangle<float> tabBarSpan (Rectangle<float> area, float start, float length, bool vertical)
{
    return vertical ? Rectangle<float> (area.getX(), start, area.getWidth(), length)
                    : Rectangle<float> (start, area.getY(), length, area.getHeight());
}

} // namespace

//==============================================================================
Rectangle<float> TabBar::Motion::getCurrent() const
{
    if (progress >= 1.0)
        return to;

    const auto amount = tabBarEaseOut (progress);

    return { from.getX() + (to.getX() - from.getX()) * amount,
             from.getY() + (to.getY() - from.getY()) * amount,
             from.getWidth() + (to.getWidth() - from.getWidth()) * amount,
             from.getHeight() + (to.getHeight() - from.getHeight()) * amount };
}

void TabBar::Motion::retarget (Rectangle<float> target, bool animate)
{
    if (target == to && (animate || progress >= 1.0))
        return;

    from = animate ? getCurrent() : target;
    to = target;
    progress = animate ? 0.0 : 1.0;
}

//==============================================================================
TabBar::TabBar (StringRef componentID)
    : Component (componentID)
{
    setOpaque (false);
    setWantsKeyboardFocus (true);

    overflowTabButton = std::make_unique<TabButton> (*this, Identifier ("tabBarOverflow"));
    overflowTabButton->overflowButton = true;
    overflowTabButton->text = overflowText;
    addChildComponent (*overflowTabButton);
}

TabBar::~TabBar()
{
    if (overflowMenu != nullptr)
        overflowMenu->dismiss();
}

//==============================================================================
TabButton& TabBar::addTab (const Identifier& tabId, const String& text, int insertIndex)
{
    jassert (tabId.isValid());

    if (auto* existing = getTabButton (tabId))
    {
        jassertfalse; // Tab identifiers must be unique
        return *existing;
    }

    auto button = std::make_unique<TabButton> (*this, tabId);
    auto& result = *button;
    result.text = text;
    addChildComponent (result);

    const auto index = isPositiveAndBelow (insertIndex, getNumTabs()) ? insertIndex : getNumTabs();
    tabs.insert (tabs.begin() + index, Tab { std::move (button), {}, {} });

    if (selectedTabId.isNull())
    {
        selectedTabId = tabId;
        updateLayout (true, true);
        notifySelectionChanged (sendNotification);
    }
    else
    {
        updateLayout (true, false);
    }

    return result;
}

void TabBar::removeTab (const Identifier& tabId)
{
    // Copied, as the identifier may belong to the tab being deleted.
    const auto removedId = tabId;
    const auto index = indexOfTab (removedId);

    if (index < 0)
        return;

    if (press.has_value() && press->button == tabs[static_cast<size_t> (index)].button.get())
        press.reset();

    auto removed = std::move (tabs[static_cast<size_t> (index)].button);
    tabs.erase (tabs.begin() + index);
    removeChildComponent (*removed);
    removed.reset();

    const auto wasSelected = removedId == selectedTabId;

    if (wasSelected)
        selectedTabId = tabs.empty() ? Identifier() : tabs[static_cast<size_t> (jmin (index, getNumTabs() - 1))].button->getTabId();

    updateLayout (true, wasSelected);

    const BailOutChecker checker (this);
    listeners.callChecked (checker, [&] (Listener& listener)
    {
        listener.tabRemoved (*this, removedId);
    });

    if (! checker.shouldBailOut() && wasSelected)
        notifySelectionChanged (sendNotification);
}

void TabBar::clearTabs()
{
    if (tabs.empty())
        return;

    press.reset();

    std::vector<Identifier> removedIds;
    auto removedTabs = std::exchange (tabs, {});

    for (auto& tab : removedTabs)
    {
        removedIds.push_back (tab.button->getTabId());
        removeChildComponent (*tab.button);
    }

    removedTabs.clear();

    const auto hadSelection = selectedTabId.isValid();
    selectedTabId = {};
    scrollOffset = 0.0f;
    updateLayout (false, false);

    const BailOutChecker checker (this);
    for (const auto& removedId : removedIds)
    {
        listeners.callChecked (checker, [&] (Listener& listener)
        {
            listener.tabRemoved (*this, removedId);
        });

        if (checker.shouldBailOut())
            return;
    }

    if (hadSelection)
        notifySelectionChanged (sendNotification);
}

void TabBar::moveTab (const Identifier& tabId, int newIndex, NotificationType notification)
{
    const auto movedId = tabId;
    const auto oldIndex = indexOfTab (movedId);

    if (oldIndex < 0)
        return;

    newIndex = jlimit (0, getNumTabs() - 1, newIndex);

    if (newIndex == oldIndex)
        return;

    auto tab = std::move (tabs[static_cast<size_t> (oldIndex)]);
    tabs.erase (tabs.begin() + oldIndex);
    tabs.insert (tabs.begin() + newIndex, std::move (tab));

    updateLayout (true, true);
    notifyTabMoved (movedId, oldIndex, newIndex, notification);
}

int TabBar::getNumTabs() const noexcept
{
    return static_cast<int> (tabs.size());
}

Identifier TabBar::getTabId (int index) const
{
    if (! isPositiveAndBelow (index, getNumTabs()))
        return {};

    return tabs[static_cast<size_t> (index)].button->getTabId();
}

int TabBar::indexOfTab (const Identifier& tabId) const
{
    for (size_t i = 0; i < tabs.size(); ++i)
    {
        if (tabs[i].button->getTabId() == tabId)
            return static_cast<int> (i);
    }

    return -1;
}

TabButton* TabBar::getTabButton (const Identifier& tabId) const
{
    const auto index = indexOfTab (tabId);
    return index >= 0 ? tabs[static_cast<size_t> (index)].button.get() : nullptr;
}

//==============================================================================
void TabBar::setSelectedTab (const Identifier& tabId, NotificationType notification)
{
    if (tabId == selectedTabId)
        return;

    if (tabId.isValid() && indexOfTab (tabId) < 0)
    {
        jassertfalse; // There is no tab with this identifier
        return;
    }

    selectedTabId = tabId;
    updateLayout (true, true);
    notifySelectionChanged (notification);
}

Identifier TabBar::getSelectedTabId() const
{
    return selectedTabId;
}

int TabBar::getSelectedTabIndex() const
{
    return selectedTabId.isValid() ? indexOfTab (selectedTabId) : -1;
}

//==============================================================================
void TabBar::setOrientation (Orientation newOrientation)
{
    if (orientation == newOrientation)
        return;

    orientation = newOrientation;
    scrollOffset = 0.0f;

    for (auto& tab : tabs)
        tab.button->updateLayout();

    overflowTabButton->updateLayout();
    updateLayout (false, true);
}

void TabBar::setVariant (Variant newVariant)
{
    if (variant == newVariant)
        return;

    variant = newVariant;
    updateLayout (false, true);
}

void TabBar::setLayout (Layout newLayout)
{
    if (layout == newLayout)
        return;

    layout = newLayout;
    updateLayout (true, true);
}

void TabBar::setOverflow (Overflow newOverflow)
{
    if (overflow == newOverflow)
        return;

    overflow = newOverflow;
    scrollOffset = 0.0f;
    updateLayout (true, true);
}

void TabBar::setFlipped (bool shouldBeFlipped)
{
    if (flipped == shouldBeFlipped)
        return;

    flipped = shouldBeFlipped;
    repaint();
}

void TabBar::setReorderable (bool shouldBeReorderable)
{
    reorderable = shouldBeReorderable;
}

void TabBar::setOverflowText (const String& newText)
{
    if (overflowText == newText)
        return;

    overflowText = newText;
    updateLayout (true, false);
}

void TabBar::setAnimationDuration (double newDurationSeconds)
{
    animationDuration = jmax (0.0, newDurationSeconds);
}

//==============================================================================
Rectangle<float> TabBar::getIndicatorBounds() const
{
    if (! hasIndicator)
        return {};

    const auto bounds = indicatorMotion.getCurrent();

    if (variant == Variant::pill)
        return bounds;

    if (isVertical())
        return bounds.withX (flipped ? 0.0f : getWidth() - tabBarUnderlineThickness).withWidth (tabBarUnderlineThickness);

    return bounds.withY (flipped ? 0.0f : getHeight() - tabBarUnderlineThickness).withHeight (tabBarUnderlineThickness);
}

Rectangle<float> TabBar::getTabArea() const
{
    const auto bounds = getLocalBounds();
    return variant == Variant::pill ? bounds.reduced (tabBarPillInset) : bounds;
}

bool TabBar::isFocusIndicatorVisible() const
{
    return keyboardNavigated && hasKeyboardFocus();
}

//==============================================================================
void TabBar::addListener (Listener* listener)
{
    listeners.add (listener);
}

void TabBar::removeListener (Listener* listener)
{
    listeners.remove (listener);
}

//==============================================================================
void TabBar::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void TabBar::resized()
{
    updateLayout (false, true);
}

void TabBar::refreshDisplay (double lastFrameTimeSeconds)
{
    // Without a duration, motions left over from before it was set to 0 finish at once.
    const auto step = animationDuration > 0.0 ? jlimit (0.0, tabBarMaxFrameGapSeconds, lastFrameTimeSeconds) / animationDuration : 1.0;

    const auto advance = [step] (Motion& motion)
    {
        if (motion.progress >= 1.0)
            return false;

        motion.progress = jmin (1.0, motion.progress + step);
        return true;
    };

    for (auto& tab : tabs)
    {
        if (advance (tab.motion))
            tab.button->setBounds (tab.motion.getCurrent());
    }

    if (advance (overflowMotion))
        overflowTabButton->setBounds (overflowMotion.getCurrent());

    if (advance (indicatorMotion))
        repaint();
}

void TabBar::keyDown (const KeyPress& key, const Point<float>& position)
{
    ignoreUnused (position);

    const auto keyCode = key.getKey();
    const auto previousKey = isVertical() ? KeyPress::upKey : KeyPress::leftKey;
    const auto nextKey = isVertical() ? KeyPress::downKey : KeyPress::rightKey;
    const auto current = getSelectedTabIndex();

    int target = -1;

    if (keyCode == previousKey)
        target = findEnabledTab (current, -1);
    else if (keyCode == nextKey)
        target = findEnabledTab (current, 1);
    else if (keyCode == KeyPress::homeKey)
        target = findEnabledTab (-1, 1);
    else if (keyCode == KeyPress::endKey)
        target = findEnabledTab (getNumTabs(), -1);
    else
        return;

    keyboardNavigated = true;
    repaint();

    if (target >= 0)
        setSelectedTab (getTabId (target));
}

void TabBar::mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData)
{
    ignoreUnused (event);

    if (overflow != Overflow::scroll)
        return;

    // A plain vertical wheel still scrolls a horizontal bar.
    auto delta = isVertical() ? wheelData.getDeltaY() : wheelData.getDeltaX();
    if (delta == 0.0f)
        delta = isVertical() ? wheelData.getDeltaX() : wheelData.getDeltaY();

    if (delta == 0.0f)
        return;

    scrollOffset -= delta * tabBarWheelStep;
    updateLayout (false, false);
}

void TabBar::focusLost()
{
    keyboardNavigated = false;
    repaint();
}

//==============================================================================
void TabBar::buttonMouseDown (TabButton& button, const MouseEvent& event)
{
    press.reset();

    if (std::exchange (keyboardNavigated, false))
        repaint();

    if (&button == overflowTabButton.get())
    {
        showOverflowMenu();
        return;
    }

    // Disabled components still receive mouse events.
    if (! button.isEnabled())
        return;

    const auto vertical = isVertical();
    const auto position = getLocalPoint (&button, event.getPosition());

    Press newPress;
    newPress.button = &button;
    newPress.downPosition = position;
    newPress.grabOffset = (vertical ? position.getY() : position.getX()) - tabBarStart (button.getBounds(), vertical);
    newPress.onCloseButton = button.isClosable() && button.getCloseButtonBounds().contains (event.getPosition());
    press = newPress;

    if (! newPress.onCloseButton)
        setSelectedTab (button.getTabId());
}

void TabBar::buttonMouseDrag (TabButton& button, const MouseEvent& event)
{
    if (! press.has_value() || press->button != &button || press->onCloseButton || ! reorderable || ! button.isVisible())
        return;

    const auto vertical = isVertical();
    const auto position = getLocalPoint (&button, event.getPosition());
    const auto pointer = vertical ? position.getY() : position.getX();

    if (! press->dragging)
    {
        const auto downPointer = vertical ? press->downPosition.getY() : press->downPosition.getX();

        if (std::abs (pointer - downPointer) <= tabBarDragThreshold)
            return;

        press->dragging = true;
        press->originalIndex = indexOfTab (button.getTabId());
        button.toFront (false);
    }

    updateDraggedTab (pointer);
}

void TabBar::buttonMouseUp (TabButton& button, const MouseEvent& event)
{
    if (! press.has_value() || press->button != &button)
        return;

    const auto released = *press;
    press.reset();

    if (released.dragging)
    {
        // The dropped tab slides from under the pointer into its slot.
        updateLayout (true, false);

        const auto newIndex = indexOfTab (button.getTabId());
        if (newIndex != released.originalIndex)
            notifyTabMoved (button.getTabId(), released.originalIndex, newIndex, sendNotification);

        return;
    }

    if (released.onCloseButton)
    {
        if (button.getCloseButtonBounds().contains (event.getPosition()))
            requestClose (button.getTabId());

        return;
    }

    if (button.getLocalBounds().contains (event.getPosition()) && button.onClick)
        button.onClick();
}

//==============================================================================
void TabBar::updateLayout (bool animate, bool revealSelection)
{
    for (auto& tab : tabs)
        tab.button->setSelected (tab.button->getTabId() == selectedTabId);

    const auto area = getTabArea();

    if (area.isEmpty())
        return;

    const auto vertical = isVertical();
    const auto available = tabBarLength (area, vertical);
    const auto numTabs = getNumTabs();
    const auto selectedIndex = getSelectedTabIndex();

    std::vector<float> lengths;
    lengths.reserve (tabs.size());

    for (const auto& tab : tabs)
        lengths.push_back (layout == Layout::fill ? jmax (minimumTabLength, available / static_cast<float> (numTabs))
                                                  : tab.button->getPreferredLength());

    const auto sumOf = [&lengths] (int count)
    {
        auto sum = 0.0f;

        for (int i = 0; i < count; ++i)
            sum += lengths[static_cast<size_t> (i)];

        return sum;
    };

    auto numShown = numTabs;
    auto showOverflowButton = false;
    auto selectedIsHidden = false;
    auto overflowLength = 0.0f;

    if (sumOf (numTabs) > available + tabBarFitTolerance)
    {
        if (overflow == Overflow::shrink && layout == Layout::natural)
        {
            const auto scale = available / sumOf (numTabs);

            for (auto& length : lengths)
                length = jmax (jmin (length, minimumTabLength), length * scale);
        }
        else if (overflow == Overflow::menu)
        {
            showOverflowButton = true;

            const auto fitTabs = [&]
            {
                overflowLength = overflowTabButton->getPreferredLength();
                const auto room = available - overflowLength;

                numShown = 0;
                while (numShown < numTabs && sumOf (numShown + 1) <= room + tabBarFitTolerance)
                    ++numShown;
            };

            updateOverflowButtonContent (false);
            fitTabs();

            // The button now shows the selected tab, which changes its length: fit again, keeping the selected tab behind it.
            if (selectedIndex >= numShown)
            {
                selectedIsHidden = true;
                updateOverflowButtonContent (true);
                fitTabs();
                numShown = jmin (numShown, selectedIndex);
            }

            if (layout == Layout::fill && numShown > 0)
            {
                const auto share = (available - overflowLength) / static_cast<float> (numShown);

                for (int i = 0; i < numShown; ++i)
                    lengths[static_cast<size_t> (i)] = share;
            }
        }
    }

    if (! showOverflowButton)
        updateOverflowButtonContent (false);

    if (overflow == Overflow::scroll)
    {
        if (revealSelection && selectedIndex >= 0)
        {
            const auto start = sumOf (selectedIndex);
            const auto end = start + lengths[static_cast<size_t> (selectedIndex)];

            if (start < scrollOffset)
                scrollOffset = start;
            else if (end > scrollOffset + available)
                scrollOffset = end - available;
        }

        scrollOffset = jlimit (0.0f, jmax (0.0f, sumOf (numTabs) - available), scrollOffset);
    }
    else
    {
        scrollOffset = 0.0f;
    }

    auto cursor = tabBarStart (area, vertical) - scrollOffset;

    for (int i = 0; i < numTabs; ++i)
    {
        auto& tab = tabs[static_cast<size_t> (i)];
        const auto length = lengths[static_cast<size_t> (i)];
        tab.target = tabBarSpan (area, cursor, length, vertical);

        if (i >= numShown)
        {
            tab.button->setVisible (false);
            continue;
        }

        cursor += length;

        const auto appearing = ! tab.button->isVisible();
        tab.button->setVisible (true);

        // The dragged tab stays under the pointer until it is dropped.
        if (! isDragging (*tab.button))
            moveButton (*tab.button, tab.motion, tab.target, animate && ! appearing);
    }

    const auto overflowTarget = tabBarSpan (area, cursor, overflowLength, vertical);

    if (showOverflowButton)
    {
        const auto appearing = ! overflowTabButton->isVisible();
        overflowTabButton->setVisible (true);
        moveButton (*overflowTabButton, overflowMotion, overflowTarget, animate && ! appearing);
    }
    else
    {
        overflowTabButton->setVisible (false);
    }

    overflowTabButton->setSelected (selectedIsHidden);

    if (selectedIndex < 0)
    {
        hasIndicator = false;
    }
    else if (const auto* selectedButton = tabs[static_cast<size_t> (selectedIndex)].button.get(); isDragging (*selectedButton))
    {
        hasIndicator = true;
        indicatorMotion.retarget (selectedButton->getBounds(), false);
    }
    else
    {
        const auto appearing = ! hasIndicator;
        hasIndicator = true;
        indicatorMotion.retarget (selectedIsHidden ? overflowTarget : tabs[static_cast<size_t> (selectedIndex)].target,
                                  animate && ! appearing && animationDuration > 0.0);
    }

    repaint();
}

void TabBar::updateOverflowButtonContent (bool selectedIsHidden)
{
    auto& button = *overflowTabButton;
    const auto* selectedButton = selectedIsHidden ? getTabButton (selectedTabId) : nullptr;

    if (selectedButton == nullptr)
    {
        button.setIconGlyph (String());
        button.setText (overflowText);
        return;
    }

    if (selectedButton->getIconImage().isValid())
        button.setIconImage (selectedButton->getIconImage());
    else
        button.setIconGlyph (selectedButton->getIconGlyph());

    button.setText (selectedButton->getText().isNotEmpty() ? selectedButton->getText() : overflowText);
}

void TabBar::moveButton (TabButton& button, Motion& motion, Rectangle<float> target, bool animate)
{
    motion.retarget (target, animate && animationDuration > 0.0);
    button.setBounds (motion.getCurrent());
}

void TabBar::updateDraggedTab (float pointer)
{
    auto& dragged = *press->button;
    const auto vertical = isVertical();
    const auto area = getTabArea();
    const auto length = tabBarLength (dragged.getBounds(), vertical);
    const auto low = tabBarStart (area, vertical);
    auto high = low + tabBarLength (area, vertical) - length;

    if (overflowTabButton->isVisible())
        high = jmin (high, tabBarStart (overflowMotion.to, vertical) - length);

    const auto start = jlimit (low, jmax (low, high), pointer - press->grabOffset);
    const auto bounds = tabBarSpan (area, start, length, vertical);
    auto index = indexOfTab (dragged.getTabId());

    tabs[static_cast<size_t> (index)].motion = { bounds, bounds, 1.0 };
    dragged.setBounds (bounds);

    // Swap with a neighbour once the dragged tab's leading or trailing edge passes the neighbour's center,
    // which also works at the ends of the bar, where the dragged tab is held back.
    const auto end = start + length;
    const auto centerOf = [&] (int i)
    {
        const auto& target = tabs[static_cast<size_t> (i)].target;
        return tabBarStart (target, vertical) + tabBarLength (target, vertical) * 0.5f;
    };

    for (;;)
    {
        auto other = index;

        if (index > 0 && start < centerOf (index - 1))
            other = index - 1;
        else if (index + 1 < getNumTabs() && tabs[static_cast<size_t> (index + 1)].button->isVisible() && end > centerOf (index + 1))
            other = index + 1;

        if (other == index)
            break;

        std::swap (tabs[static_cast<size_t> (index)], tabs[static_cast<size_t> (other)]);
        index = other;
        updateLayout (true, false);
    }

    updateLayout (true, false);
}

bool TabBar::isDragging (const TabButton& button) const noexcept
{
    return press.has_value() && press->dragging && press->button == &button;
}

//==============================================================================
void TabBar::notifySelectionChanged (NotificationType notification)
{
    const auto tabId = selectedTabId;

    const BailOutChecker checker (this);
    listeners.callChecked (checker, [&] (Listener& listener)
    {
        listener.tabSelectionChanged (*this, tabId);
    });

    if (checker.shouldBailOut())
        return;

    // Passed as an lvalue, so the captured identifier is copied rather than moved from.
    const auto callback = [this, tabId]
    {
        if (onSelectionChanged)
            onSelectionChanged (tabId);
    };

    sendChangeNotification (notification, callback);
}

void TabBar::notifyTabMoved (Identifier tabId, int oldIndex, int newIndex, NotificationType notification)
{
    const BailOutChecker checker (this);
    listeners.callChecked (checker, [&] (Listener& listener)
    {
        listener.tabMoved (*this, tabId, oldIndex, newIndex);
    });

    if (checker.shouldBailOut())
        return;

    const auto callback = [this, tabId, oldIndex, newIndex]
    {
        if (onTabMoved)
            onTabMoved (tabId, oldIndex, newIndex);
    };

    sendChangeNotification (notification, callback);
}

void TabBar::requestClose (Identifier tabId)
{
    if (onTabCloseRequested)
        onTabCloseRequested (tabId);
    else
        removeTab (tabId);
}

void TabBar::showOverflowMenu()
{
    if (std::exchange (ignoreMouseDownAfterMenuDismissal, false))
    {
        overflowMenu = nullptr;
        return;
    }

    if (overflowMenu != nullptr && overflowMenu->isBeingShown())
    {
        const ScopedValueSetter<bool> dismissingFromThisMouseDown (dismissingMenuFromMouseDown, true);
        overflowMenu->dismiss();
        return;
    }

    overflowMenu = PopupMenu::create (PopupMenu::Options {}
                                          .withParentComponent (getPopupParentComponent())
                                          .withRelativePosition (overflowTabButton.get()));

    // Item ids start at 1, as 0 means the menu was dismissed.
    std::vector<Identifier> hiddenTabs;

    for (const auto& tab : tabs)
    {
        if (tab.button->isVisible())
            continue;

        const auto& tabId = tab.button->getTabId();
        hiddenTabs.push_back (tabId);

        overflowMenu->addItem (tab.button->getText().isNotEmpty() ? tab.button->getText() : tabId.toString(),
                               static_cast<int> (hiddenTabs.size()),
                               tab.button->isEnabled(),
                               tabId == selectedTabId);
    }

    WeakReference<Component> self = this;
    overflowMenu->show ([this, self, hiddenTabs] (int itemId)
    {
        if (self.get() == nullptr)
            return;

        if (itemId > 0 && itemId <= static_cast<int> (hiddenTabs.size()))
        {
            if (const auto& tabId = hiddenTabs[static_cast<size_t> (itemId - 1)]; indexOfTab (tabId) >= 0)
                setSelectedTab (tabId);
        }
        else if (itemId == 0 && ! dismissingMenuFromMouseDown)
        {
            ignoreMouseDownAfterMenuDismissal = true;

            MessageManager::callAsync ([this, self]
            {
                if (self.get() != nullptr)
                    ignoreMouseDownAfterMenuDismissal = false;
            });
        }
    });
}

int TabBar::findEnabledTab (int fromIndex, int step) const
{
    const auto numTabs = getNumTabs();

    if (numTabs == 0)
        return -1;

    // Without a selection, stepping backwards starts from the end.
    if (fromIndex < 0 && step < 0)
        fromIndex = numTabs;

    for (int i = 1; i <= numTabs; ++i)
    {
        const auto index = ((fromIndex + step * i) % numTabs + numTabs) % numTabs;

        if (tabs[static_cast<size_t> (index)].button->isEnabled())
            return index;
    }

    return -1;
}

} // namespace yup
