/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

namespace
{

//==============================================================================

static std::vector<PopupMenu::Ptr> activePopups;

void removeActivePopup (PopupMenu* popupMenu)
{
    for (auto it = activePopups.begin(); it != activePopups.end();)
    {
        if (it->get() == popupMenu)
            it = activePopups.erase (it);
        else
            ++it;
    }
}

void installGlobalMouseListener()
{
    static bool mouseListenerAdded = []
    {
        static struct GlobalMouseListener : MouseListener
        {
            void mouseDown (const MouseEvent& event) override
            {
                Point<float> globalPos = event.getScreenPosition().to<float>();

                bool clickedInsidePopup = false;
                for (const auto& popup : activePopups)
                {
                    if (auto* popupMenu = dynamic_cast<PopupMenu*> (popup.get()))
                    {
                        if (popupMenu->getScreenBounds().contains (globalPos))
                        {
                            clickedInsidePopup = true;
                            break;
                        }

                        // Also check if clicked inside any submenu
                        if (popupMenu->submenuContains (globalPos))
                        {
                            clickedInsidePopup = true;
                            break;
                        }
                    }
                }

                if (! clickedInsidePopup && ! activePopups.empty())
                    PopupMenu::dismissAllPopups();
            }
        } globalMouseListener {};

        Desktop::getInstance()->addGlobalMouseListener (&globalMouseListener);

        MessageManager::getInstance()->registerShutdownCallback ([]
        {
            PopupMenu::dismissAllPopups();
        });

        return true;
    }();
}

//==============================================================================

Point<int> calculatePositionAtPoint (Point<int> targetPoint, Size<int> menuSize, Justification alignment)
{
    Point<int> position = targetPoint;

    switch (alignment)
    {
        default:
        case Justification::topLeft:
            // Menu's top-left at target point (default)
            break;

        case Justification::centerTop:
            position.setX (targetPoint.getX() - menuSize.getWidth() / 2);
            break;

        case Justification::topRight:
            position.setX (targetPoint.getX() - menuSize.getWidth());
            break;

        case Justification::centerLeft:
            position.setY (targetPoint.getY() - menuSize.getHeight() / 2);
            break;

        case Justification::center:
            position = targetPoint - (menuSize / 2).toPoint<int>();
            break;

        case Justification::centerRight:
            position.setX (targetPoint.getX() - menuSize.getWidth());
            position.setY (targetPoint.getY() - menuSize.getHeight() / 2);
            break;

        case Justification::bottomLeft:
            position.setY (targetPoint.getY() - menuSize.getHeight());
            break;

        case Justification::centerBottom:
            position.setX (targetPoint.getX() - menuSize.getWidth() / 2);
            position.setY (targetPoint.getY() - menuSize.getHeight());
            break;

        case Justification::bottomRight:
            position = targetPoint - menuSize.toPoint<int>();
            break;
    }

    return position;
}

//==============================================================================

Point<int> calculatePositionRelativeToArea (Rectangle<int> targetArea, Size<int> menuSize, PopupMenu::Placement placement)
{
    Point<int> position;

    // Handle special case first
    if (placement.side == PopupMenu::Side::centered)
    {
        return targetArea.getCenter() - (menuSize / 2).toPoint<int>();
    }

    // Set position based on side (primary axis)
    switch (placement.side)
    {
        case PopupMenu::Side::below:
            position.setY (targetArea.getBottom());
            break;

        case PopupMenu::Side::above:
            position.setY (targetArea.getY() - menuSize.getHeight());
            break;

        case PopupMenu::Side::toRight:
            position.setX (targetArea.getRight());
            break;

        case PopupMenu::Side::toLeft:
            position.setX (targetArea.getX() - menuSize.getWidth());
            break;

        default:
            break;
    }

    // Set alignment on perpendicular axis (secondary axis)
    if (placement.side == PopupMenu::Side::below || placement.side == PopupMenu::Side::above)
    {
        // For above/below: align horizontally
        if (placement.alignment == Justification::centerTop || placement.alignment == Justification::center || placement.alignment == Justification::centerBottom)
        {
            position.setX (targetArea.getCenterX() - menuSize.getWidth() / 2);
        }
        else if (placement.alignment == Justification::topRight || placement.alignment == Justification::centerRight || placement.alignment == Justification::bottomRight)
        {
            position.setX (targetArea.getRight() - menuSize.getWidth());
        }
        else // Default: left-aligned
        {
            position.setX (targetArea.getX());
        }
    }
    else if (placement.side == PopupMenu::Side::toLeft || placement.side == PopupMenu::Side::toRight)
    {
        // For left/right: align vertically
        if (placement.alignment == Justification::centerLeft || placement.alignment == Justification::center || placement.alignment == Justification::centerRight)
        {
            position.setY (targetArea.getCenterY() - menuSize.getHeight() / 2);
        }
        else if (placement.alignment == Justification::bottomLeft || placement.alignment == Justification::centerBottom || placement.alignment == Justification::bottomRight)
        {
            position.setY (targetArea.getBottom() - menuSize.getHeight());
        }
        else // Default: top-aligned
        {
            position.setY (targetArea.getY());
        }
    }

    return position;
}

//==============================================================================

Point<int> constrainPositionToAvailableArea (Point<int> desiredPosition,
                                             const Size<int>& menuSize,
                                             const Rectangle<int>& availableArea,
                                             const Rectangle<int>& targetArea)
{
    // Add padding to keep menu slightly away from screen edges
    const int padding = 5;
    auto constrainedArea = availableArea.reduced (padding);

    Point<int> position = desiredPosition;

    // Only make minimal adjustments to keep menu visible
    // Don't override the placement strategy, just nudge the menu if needed
    Rectangle<int> menuBounds (position, menuSize);

    // Horizontal constraint - only adjust if menu goes off screen
    if (menuBounds.getRight() > constrainedArea.getRight())
    {
        // Move left just enough to fit
        position.setX (constrainedArea.getRight() - menuSize.getWidth());
    }
    else if (menuBounds.getX() < constrainedArea.getX())
    {
        // Move right just enough to fit
        position.setX (constrainedArea.getX());
    }

    // Vertical constraint - only adjust if menu goes off screen
    if (menuBounds.getBottom() > constrainedArea.getBottom())
    {
        // Move up just enough to fit
        position.setY (constrainedArea.getBottom() - menuSize.getHeight());
    }
    else if (menuBounds.getY() < constrainedArea.getY())
    {
        // Move down just enough to fit
        position.setY (constrainedArea.getY());
    }

    return position;
}

} // namespace

//==============================================================================

PopupMenu::Item::Item (const String& itemText, int itemID, bool isEnabled, bool isTicked)
    : text (itemText)
    , itemID (itemID)
    , isEnabled (isEnabled)
    , isTicked (isTicked)
{
}

PopupMenu::Item::Item (const String& itemText, PopupMenu::Ptr subMenu, bool isEnabled)
    : text (itemText)
    , isEnabled (isEnabled)
    , subMenu (std::move (subMenu))
{
}

PopupMenu::Item::Item (std::unique_ptr<Component> component, int itemID)
    : itemID (itemID)
    , customComponent (std::move (component))
{
}

PopupMenu::Item::~Item() = default;

bool PopupMenu::Item::isSeparator() const
{
    return text.isEmpty() && itemID == 0 && subMenu == nullptr && customComponent == nullptr;
}

bool PopupMenu::Item::isSubMenu() const
{
    return subMenu != nullptr;
}

bool PopupMenu::Item::isCustomComponent() const
{
    return customComponent != nullptr;
}

//==============================================================================

PopupMenu::Options::Options()
    : parentComponent (nullptr)
    , targetComponent (nullptr)
    , alignment (Justification::topLeft)
    , placement (Placement::below())
    , positioningMode (PositioningMode::atPoint)
    , dismissOnSelection (true)
    , dismissAllPopups (true)
{
}

PopupMenu::Options& PopupMenu::Options::withParentComponent (Component* parentComponent)
{
    this->parentComponent = parentComponent;
    return *this;
}

PopupMenu::Options& PopupMenu::Options::withPosition (Point<int> position, Justification alignment)
{
    this->positioningMode = PositioningMode::atPoint;
    this->targetPosition = position;
    this->alignment = alignment;
    return *this;
}

PopupMenu::Options& PopupMenu::Options::withPosition (Point<float> position, Justification alignment)
{
    return withPosition (position.to<int>(), alignment);
}

PopupMenu::Options& PopupMenu::Options::withTargetArea (Rectangle<int> area, Placement placement)
{
    this->positioningMode = PositioningMode::relativeToArea;
    this->targetArea = area;
    this->placement = placement;
    return *this;
}

PopupMenu::Options& PopupMenu::Options::withTargetArea (Rectangle<float> area, Placement placement)
{
    return withTargetArea (area.to<int>(), placement);
}

PopupMenu::Options& PopupMenu::Options::withRelativePosition (Component* component, Placement placement)
{
    this->positioningMode = PositioningMode::relativeToComponent;
    this->targetComponent = component;
    this->placement = placement;
    return *this;
}

PopupMenu::Options& PopupMenu::Options::withMinimumWidth (int minWidth)
{
    this->minWidth = minWidth;
    return *this;
}

PopupMenu::Options& PopupMenu::Options::withMaximumWidth (int maxWidth)
{
    this->maxWidth = maxWidth;
    return *this;
}

//==============================================================================

PopupMenu::PopupMenu (const Options& options)
    : options (options)
{
}

PopupMenu::~PopupMenu()
{
    if (isVisible())
        dismiss();
}

//==============================================================================

PopupMenu::Ptr PopupMenu::create (const Options& options)
{
    return new PopupMenu (options);
}

//==============================================================================

void PopupMenu::dismissAllPopups()
{
    // Make a copy to avoid issues with the vector being modified during iteration
    auto popupsToClose = std::move (activePopups);

    for (const auto& popup : popupsToClose)
    {
        if (auto* popupMenu = dynamic_cast<PopupMenu*> (popup.get()))
            popupMenu->dismiss();
    }

    activePopups.clear();
}

//==============================================================================

void PopupMenu::addItem (const String& text, int itemID, bool isEnabled, bool isTicked, const String& shortcutText)
{
    auto item = std::make_unique<Item> (text, itemID, isEnabled, isTicked);
    item->shortcutKeyText = shortcutText;
    items.push_back (std::move (item));
}

void PopupMenu::addSeparator()
{
    items.push_back (std::make_unique<Item>());
}

void PopupMenu::addSubMenu (const String& text, PopupMenu::Ptr subMenu, bool isEnabled)
{
    auto item = std::make_unique<Item> (text, std::move (subMenu), isEnabled);
    items.push_back (std::move (item));
}

void PopupMenu::addCustomItem (std::unique_ptr<Component> component, int itemID)
{
    auto item = std::make_unique<Item> (std::move (component), itemID);
    items.push_back (std::move (item));
}

//==============================================================================

int PopupMenu::getNumItems() const
{
    return static_cast<int> (items.size());
}

void PopupMenu::clear()
{
    items.clear();
}

//==============================================================================

int PopupMenu::getHoveredItem() const
{
    int currentIndex = 0;

    for (auto& item : items)
    {
        if (item->isHovered)
            return currentIndex;

        ++currentIndex;
    }

    return -1;
}

void PopupMenu::setHoveredItem (int itemIndex)
{
    bool hasChanged = false;

    int currentIndex = 0;
    for (auto& item : items)
    {
        bool newHoveredState = (currentIndex == itemIndex);

        if (newHoveredState != item->isHovered)
            hasChanged = true;

        item->isHovered = newHoveredState;

        ++currentIndex;
    }

    if (hasChanged)
    {
        updateSubmenuVisibility (itemIndex);

        repaint();
    }
}

//==============================================================================

void PopupMenu::setSelectedItemID (int itemID)
{
    if (selectedItemID != itemID)
    {
        selectedItemID = itemID;

        if (auto itemCallback = std::exchange (menuCallback, {}))
            itemCallback (itemID);

        if (onItemSelected != nullptr)
            onItemSelected (itemID);

        isBeingDismissed = false;
    }
}

//==============================================================================

void PopupMenu::setupMenuItems()
{
    constexpr float separatorHeight = 8.0f;
    constexpr float verticalPadding = 4.0f;

    float y = verticalPadding; // Top padding
    float itemHeight = static_cast<float> (22);
    float width = options.minWidth.value_or (200);

    for (const auto& item : items)
    {
        if (item->isCustomComponent())
            width = jmax (width, item->customComponent->getWidth());
    }

    for (auto& item : items)
    {
        if (item->isCustomComponent())
        {
            addChildComponent (*item->customComponent);

            float horizontalOffset = 0.0f;

            auto compWidth = item->customComponent->getWidth();
            auto compHeight = item->customComponent->getHeight();
            jassert (compWidth != 0 && compHeight != 0);
            if (compWidth < width)
                horizontalOffset = (width - compWidth) / 2.0f;

            item->area = { horizontalOffset, y, compWidth, compHeight };
            item->customComponent->setBounds (item->area);
            item->customComponent->setVisible (true);

            y += compHeight;
        }
        else
        {
            const auto height = item->isSeparator() ? separatorHeight : itemHeight;
            item->area = { 0, y, width, height };
            y += height;
        }
    }

    totalContentHeight = y + verticalPadding; // Store total content height

    // Update scrolling calculations
    updateScrolling();

    // Calculate final menu size considering scrolling
    float finalHeight = totalContentHeight;
    if (needsScrolling())
    {
        finalHeight = availableContentHeight;
        if (showScrollIndicators)
            finalHeight += 2 * scrollIndicatorHeight;
    }

    setSize ({ width, finalHeight });
}

//==============================================================================

void PopupMenu::positionMenu()
{
    auto menuSize = getSize().to<int>();
    Rectangle<int> targetArea;
    Rectangle<int> availableArea;

    // Determine coordinate system and available area
    if (options.parentComponent)
    {
        // Working in parent component's local coordinates
        availableArea = options.parentComponent->getLocalBounds().to<int>();
    }
    else
    {
        // Working in screen coordinates
        if (auto* desktop = Desktop::getInstance())
        {
            if (auto screen = desktop->getPrimaryScreen())
                availableArea = screen->workArea;
            else
                availableArea = Rectangle<int> (0, 0, 1920, 1080); // Fallback
        }
        else
        {
            availableArea = Rectangle<int> (0, 0, 1920, 1080); // Fallback
        }
    }

    // Calculate position based on positioning mode
    Point<int> position;

    switch (options.positioningMode)
    {
        case PositioningMode::atPoint:
            position = calculatePositionAtPoint (options.targetPosition, menuSize, options.alignment);
            break;

        case PositioningMode::relativeToArea:
            targetArea = options.targetArea;
            position = calculatePositionRelativeToArea (targetArea, menuSize, options.placement);
            break;

        case PositioningMode::relativeToComponent:
            if (options.targetComponent)
            {
                // Get target component bounds in appropriate coordinate system
                if (options.parentComponent)
                {
                    // Check if target is a direct child of parent
                    if (options.targetComponent->getParentComponent() == options.parentComponent)
                    {
                        // Target is direct child - use its bounds directly
                        targetArea = options.targetComponent->getBounds().to<int>();
                    }
                    else
                    {
                        // Target is not a direct child - need coordinate conversion
                        targetArea = options.parentComponent->getRelativeArea (options.targetComponent, options.targetComponent->getBounds()).to<int>();
                    }
                }
                else
                {
                    // No parent component - use screen coordinates
                    targetArea = options.targetComponent->getScreenBounds().to<int>();
                }

                position = calculatePositionRelativeToArea (targetArea, menuSize, options.placement);
            }
            else
            {
                // Fallback to center of available area
                position = availableArea.getCenter() - Point<int> { menuSize.getWidth() / 2, menuSize.getHeight() / 2 };
            }
            break;
    }

    // Adjust position to fit within available area
    position = constrainPositionToAvailableArea (position, menuSize, availableArea, targetArea);

    setTopLeft (position);
}

//==============================================================================

int PopupMenu::getItemIndexAt (Point<float> position) const
{
    // Adjust position for scrolling
    if (needsScrolling())
    {
        auto contentBounds = getMenuContentBounds();
        if (! contentBounds.contains (position))
            return -1; // Click was outside content area (maybe on scroll indicators)

        position.setY (position.getY() + scrollOffset);
    }

    int itemIndex = 0;

    for (const auto& item : items)
    {
        if (item->area.contains (position))
            return itemIndex;

        ++itemIndex;
    }

    return -1;
}

//==============================================================================

void PopupMenu::show (std::function<void (int)> callback)
{
    showCustom (options, std::move (callback));
}

//==============================================================================

void PopupMenu::showCustom (const Options& options, std::function<void (int)> callback)
{
    if (options.dismissAllPopups)
        dismissAllPopups();

    this->options = options;
    menuCallback = std::move (callback);

    if (isEmpty())
    {
        dismiss();
        return;
    }

    installGlobalMouseListener();

    setWantsKeyboardFocus (true);

    if (options.parentComponent)
    {
        // When we have a parent component, add as child to work in local coordinates
        options.parentComponent->addChildComponent (this);
    }
    else
    {
        // When we have no parent component, add to desktop to work in screen coordinates
        auto nativeOptions = ComponentNative::Options {}
                                 .withDecoration (false)
                                 .withResizableWindow (false);

        addToDesktop (nativeOptions);
    }

    activePopups.push_back (this);

    setupMenuItems();
    positionMenu();

    setVisible (true);
    toFront (true);
}

//==============================================================================

void PopupMenu::dismiss()
{
    dismiss (0);
}

void PopupMenu::dismiss (int itemID)
{
    if (isBeingDismissed)
        return;

    isBeingDismissed = true;

    // Hide any submenus first
    hideSubmenus();

    setVisible (false);

    setSelectedItemID (itemID);

    removeActivePopup (this);
}

//==============================================================================

void PopupMenu::paint (Graphics& g)
{
    auto state = g.saveState();

    if (needsScrolling())
    {
        // Create a clipping region for scrollable content
        auto contentBounds = getMenuContentBounds();
        g.setClipPath (contentBounds);

        // Translate graphics context by scroll offset
        g.setTransform (AffineTransform::translation (0.0f, -scrollOffset));
    }

    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);

    if (needsScrolling())
    {
        state.restore();

        // Paint scroll indicators
        paintScrollIndicators (g);
    }
}

//==============================================================================

void PopupMenu::mouseDown (const MouseEvent& event)
{
    if (! getLocalBounds().contains (event.getPosition()))
    {
        dismiss();
        return;
    }

    auto itemIndex = getItemIndexAt (event.getPosition());
    if (! isPositiveAndBelow (itemIndex, getNumItems()))
        return;

    auto& item = *items[itemIndex];
    if (item.isSeparator() || ! item.isEnabled)
        return;

    if (item.isSubMenu())
    {
        // For submenus, we show them on hover, not on click
        showSubmenu (itemIndex);
    }
    else
    {
        dismiss (item.itemID);
    }
}

void PopupMenu::mouseMove (const MouseEvent& event)
{
    // Cancel hide timers when mouse is actively moving in menu
    submenuHideTimer.stopTimer();

    setHoveredItem (getItemIndexAt (event.getPosition()));
}

void PopupMenu::mouseEnter (const MouseEvent& event)
{
    // Call custom mouse enter callback for submenu coordination
    if (onMouseEnter)
        onMouseEnter();

    // Cancel any pending hide timers when mouse enters
    submenuHideTimer.stopTimer();
}

void PopupMenu::mouseExit (const MouseEvent& event)
{
    setHoveredItem (-1);

    // Call custom mouse exit callback for submenu coordination
    if (onMouseExit)
        onMouseExit();

    // Don't start hide timer on mouse exit - let the hover logic handle submenu visibility
    // This prevents the main menu from disappearing when moving to submenus
}

void PopupMenu::mouseWheel (const MouseEvent& event, const MouseWheelData& wheel)
{
    if (! needsScrolling())
        return;

    auto deltaY = wheel.getDeltaY() * scrollSpeed;
    scrollOffset = jlimit (0.0f, getMaxScrollOffset(), scrollOffset - deltaY);

    constrainScrollOffset();
    repaint();
}

void PopupMenu::keyDown (const KeyPress& key, const Point<float>& position)
{
    if (key.getKey() == KeyPress::escapeKey)
        dismiss();
}

//==============================================================================

void PopupMenu::focusLost()
{
    // Don't dismiss if we have a visible submenu or are in the process of showing one
    if (hasVisibleSubmenu() || isShowingSubmenu)
        return;

    //dismiss();
}

//==============================================================================
// Submenu functionality

void PopupMenu::showSubmenu (int itemIndex)
{
    if (! isPositiveAndBelow (itemIndex, getNumItems()))
        return;

    auto& item = *items[itemIndex];
    if (! item.isSubMenu() || ! item.subMenu)
        return;

    // If we're already showing this submenu, no need to do anything
    if (submenuItemIndex == itemIndex && currentSubmenu && currentSubmenu == item.subMenu && currentSubmenu->isVisible())
        return;

    // Set flag to prevent dismissal during submenu operations
    isShowingSubmenu = true;

    // Hide current submenu if different item
    if (submenuItemIndex != itemIndex)
        hideSubmenus();

    submenuItemIndex = itemIndex;
    currentSubmenu = item.subMenu;

    // Stop any pending timers that might interfere
    submenuShowTimer.stopTimer();
    submenuHideTimer.stopTimer();

    // Position submenu to the right of the current menu item
    auto itemBounds = item.area;

    // Account for scroll offset if menu is scrollable
    if (needsScrolling())
    {
        itemBounds.setY (itemBounds.getY() - static_cast<int> (scrollOffset));
    }

    Options submenuOptions;
    submenuOptions.parentComponent = options.parentComponent; // Respect parent component
    submenuOptions.dismissAllPopups = false;

    if (options.parentComponent)
    {
        // Position relative to parent component - need to transform coordinates properly
        auto menuPosInParent = getTopLeft(); // This menu's position within parent
        auto itemBoundsInParent = itemBounds.translated (menuPosInParent);

        submenuOptions.withTargetArea (itemBoundsInParent, Placement::toRight (Justification::topRight));
    }
    else
    {
        // Use screen coordinates when no parent
        auto itemScreenPos = getScreenBounds().getTopLeft() + itemBounds.getTopRight();
        submenuOptions.withTargetArea (Rectangle<float> (itemScreenPos.getX(), itemScreenPos.getY(), 1, itemBounds.getHeight()),
                                       Placement::toRight (Justification::topLeft));
    }

    // Add mouse listeners to handle submenu interaction
    currentSubmenu->showCustom (submenuOptions, [this] (int selectedID)
    {
        if (selectedID != 0)
        {
            dismiss (selectedID);
        }
    });

    // Set up submenu mouse tracking to prevent premature hiding
    setupSubmenuMouseTracking (currentSubmenu);

    // Clear the flag after showing submenu
    isShowingSubmenu = false;
}

void PopupMenu::hideSubmenus()
{
    if (currentSubmenu)
    {
        // Use the cleanup method to hide without triggering callbacks
        cleanupSubmenu (currentSubmenu);
        currentSubmenu = nullptr;
    }

    submenuItemIndex = -1;
    isShowingSubmenu = false;
}

void PopupMenu::setupSubmenuMouseTracking (PopupMenu::Ptr submenu)
{
    if (! submenu)
        return;

    // Set up callbacks for coordinating mouse events between parent and submenu
    auto parentMenu = this; // Capture parent menu reference

    // When mouse enters submenu, cancel any hide timers on parent
    submenu->onMouseEnter = [parentMenu]()
    {
        parentMenu->submenuHideTimer.stopTimer();
    };

    // When mouse exits submenu, start hide timer with generous delay
    submenu->onMouseExit = [parentMenu]()
    {
        // Only start hide timer if we're not returning to parent menu
        parentMenu->submenuHideTimer.onTimer = [parentMenu]
        {
            parentMenu->hideSubmenus();
            parentMenu->submenuHideTimer.stopTimer();
        };

        parentMenu->submenuHideTimer.startTimer (400); // Longer delay for better UX
    };
}

void PopupMenu::cleanupSubmenu (PopupMenu::Ptr submenu)
{
    if (! submenu)
        return;

    // Just hide without triggering callbacks
    submenu->setVisible (false);

    // Remove from activePopups list
    removeActivePopup (submenu.get());
}

bool PopupMenu::hasVisibleSubmenu() const
{
    return currentSubmenu != nullptr && currentSubmenu->isVisible();
}

bool PopupMenu::submenuContains (const Point<float>& position) const
{
    if (! hasVisibleSubmenu())
        return false;

    return currentSubmenu->getScreenBounds().contains (position);
}

void PopupMenu::updateSubmenuVisibility (int hoveredItemIndex)
{
    // Always stop existing timers first
    submenuShowTimer.stopTimer();
    submenuHideTimer.stopTimer();

    if (isPositiveAndBelow (hoveredItemIndex, getNumItems()))
    {
        auto& item = *items[hoveredItemIndex];
        if (item.isSubMenu() && item.isEnabled)
        {
            // If this is the same submenu item that's already showing, do nothing
            if (submenuItemIndex == hoveredItemIndex && hasVisibleSubmenu())
                return;

            // Show submenu immediately if we're hovering over a submenu item
            // No timer delay to prevent the main menu from disappearing
            showSubmenu (hoveredItemIndex);
            return;
        }
    }

    // If we're not hovering over a submenu item and we have a visible submenu,
    // use a longer delay before hiding to allow mouse movement to submenu
    if (hasVisibleSubmenu() && submenuItemIndex != hoveredItemIndex)
    {
        submenuHideTimer.onTimer = [this]
        {
            hideSubmenus();
            submenuHideTimer.stopTimer();
        };

        submenuHideTimer.startTimer (200);
    }
}

//==============================================================================
// Scrolling functionality

void PopupMenu::updateScrolling()
{
    auto bounds = getLocalBounds().to<float>();

    if (options.parentComponent)
    {
        // Calculate available height within parent component bounds
        auto parentBounds = options.parentComponent->getLocalBounds().to<float>();
        auto menuScreenPos = getScreenPosition().to<float>();
        auto parentScreenPos = options.parentComponent->getScreenPosition().to<float>();

        // Calculate available space from current position to parent bottom
        availableContentHeight = parentBounds.getBottom() - (menuScreenPos.getY() - parentScreenPos.getY());
        availableContentHeight = jmax (100.0f, availableContentHeight); // Minimum height
    }
    else
    {
        // Use screen bounds
        if (auto* desktop = Desktop::getInstance())
        {
            if (auto screen = desktop->getPrimaryScreen())
            {
                auto screenBounds = screen->workArea.to<float>();
                auto menuScreenPos = getScreenPosition().to<float>();
                availableContentHeight = screenBounds.getBottom() - menuScreenPos.getY();
                availableContentHeight = jmax (100.0f, availableContentHeight);
            }
        }
    }

    totalContentHeight = 0.0f;
    for (const auto& item : items)
    {
        totalContentHeight += item->area.getHeight();
    }

    // Add padding
    totalContentHeight += 8.0f; // Top + bottom padding

    showScrollIndicators = needsScrolling();

    if (showScrollIndicators)
        availableContentHeight -= 2 * scrollIndicatorHeight;

    constrainScrollOffset();
}

void PopupMenu::constrainScrollOffset()
{
    auto maxOffset = getMaxScrollOffset();
    scrollOffset = jlimit (0.0f, maxOffset, scrollOffset);
}

float PopupMenu::getMaxScrollOffset() const
{
    if (! needsScrolling())
        return 0.0f;

    return jmax (0.0f, totalContentHeight - availableContentHeight);
}

bool PopupMenu::needsScrolling() const
{
    return totalContentHeight > availableContentHeight;
}

Rectangle<float> PopupMenu::getMenuContentBounds() const
{
    auto bounds = getLocalBounds().to<float>();

    if (showScrollIndicators)
    {
        bounds.removeFromTop (scrollIndicatorHeight);
        bounds.removeFromBottom (scrollIndicatorHeight);
    }

    return bounds;
}

Rectangle<float> PopupMenu::getScrollUpIndicatorBounds() const
{
    if (! showScrollIndicators)
        return {};

    auto bounds = getLocalBounds().to<float>();
    return bounds.removeFromTop (scrollIndicatorHeight);
}

Rectangle<float> PopupMenu::getScrollDownIndicatorBounds() const
{
    if (! showScrollIndicators)
        return {};

    auto bounds = getLocalBounds().to<float>();
    return bounds.removeFromBottom (scrollIndicatorHeight);
}

void PopupMenu::paintScrollIndicators (Graphics& g)
{
    if (! showScrollIndicators)
        return;

    auto theme = ApplicationTheme::getGlobalTheme();
    g.setFillColor (findColor (Style::menuItemText).value_or (Color (0xff000000)));

    // Up arrow
    if (scrollOffset > 0.0f)
    {
        /*
        auto upBounds = getScrollUpIndicatorBounds();
        auto center = upBounds.getCenter();
        auto arrowSize = 4.0f;

        Path upArrow;
        upArrow.addTriangle (center.getX(), center.getY() - arrowSize * 0.5f,
                             center.getX() - arrowSize, center.getY() + arrowSize * 0.5f,
                             center.getX() + arrowSize, center.getY() + arrowSize * 0.5f);
        g.fillPath (upArrow);
        */
    }

    // Down arrow
    if (scrollOffset < getMaxScrollOffset())
    {
        /*
        auto downBounds = getScrollDownIndicatorBounds();
        auto center = downBounds.getCenter();
        auto arrowSize = 4.0f;

        Path downArrow;
        downArrow.addTriangle (center.getX(), center.getY() + arrowSize * 0.5f,
                               center.getX() - arrowSize, center.getY() - arrowSize * 0.5f,
                               center.getX() + arrowSize, center.getY() - arrowSize * 0.5f);
        g.fillPath (downArrow);
        */
    }
}

} // namespace yup
