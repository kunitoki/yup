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
                    auto* popupMenu = dynamic_cast<PopupMenu*> (popup.get());
                    if (popupMenu == nullptr)
                        continue;

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
            position.setX (targetArea.getCenterX() - menuSize.getWidth() / 2);

        else if (placement.alignment == Justification::topRight || placement.alignment == Justification::centerRight || placement.alignment == Justification::bottomRight)
            position.setX (targetArea.getRight() - menuSize.getWidth());

        else // Default: left-aligned
            position.setX (targetArea.getX());
    }
    else if (placement.side == PopupMenu::Side::toLeft || placement.side == PopupMenu::Side::toRight)
    {
        // For left/right: align vertically
        if (placement.alignment == Justification::centerLeft || placement.alignment == Justification::center || placement.alignment == Justification::centerRight)
            position.setY (targetArea.getCenterY() - menuSize.getHeight() / 2);

        else if (placement.alignment == Justification::bottomLeft || placement.alignment == Justification::centerBottom || placement.alignment == Justification::bottomRight)
            position.setY (targetArea.getBottom() - menuSize.getHeight());

        else // Default: top-aligned
            position.setY (targetArea.getY());
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
    setOpaque (false);
    setWantsKeyboardFocus (true);
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
    auto popupsToClose = std::exchange (activePopups, {});

    for (const auto& popup : popupsToClose)
    {
        if (auto* popupMenu = dynamic_cast<PopupMenu*> (popup.get()))
            popupMenu->dismiss();
    }
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

void PopupMenu::setupMenuItems()
{
    constexpr float separatorHeight = 8.0f; // TODO: move to Options
    constexpr float verticalPadding = 4.0f; // TODO: move to Style ?

    float itemHeight = static_cast<float> (22);    // TODO: move to Options
    float width = options.minWidth.value_or (200); // TODO: move to magic

    // First pass: calculate total content height and determine width
    totalContentHeight = verticalPadding; // Top padding
    for (const auto& item : items)
    {
        if (item->isCustomComponent())
        {
            width = jmax (width, item->customComponent->getWidth());
            totalContentHeight += item->customComponent->getHeight();
        }
        else
        {
            const auto height = item->isSeparator() ? separatorHeight : itemHeight;
            totalContentHeight += height;
        }
    }
    totalContentHeight += verticalPadding; // Bottom padding

    // Calculate available content height properly (without depending on current position)
    calculateAvailableHeight();

    // Determine if scrolling is needed
    showScrollIndicators = needsScrolling();

    // Initialize visible item range if not set
    if (visibleItemRange.isEmpty())
        visibleItemRange = Range<int> (0, 0);

    updateVisibleItemRange();

    // Set menu bounds based on available space - do this only once
    if (getWidth() == 0 || getHeight() == 0) // Only set size if not already set
    {
        float menuHeight = jmin (totalContentHeight, availableContentHeight);
        if (showScrollIndicators)
            menuHeight -= scrollIndicatorHeight * 2.0f; // Reserve space for indicators

        setSize (static_cast<int> (width), static_cast<int> (menuHeight));
    }

    // Remove all child components first
    for (auto& item : items)
    {
        if (item->isCustomComponent() && item->customComponent != nullptr)
            removeChildComponent (item->customComponent.get());
    }

    // Second pass: set up visible items only
    layoutVisibleItems (width);

    // Force a complete repaint to avoid rendering artifacts
    repaint();
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
        availableArea = Rectangle<int> (0, 0, 1920, 1080); // TODO: Move to magic
        if (auto* desktop = Desktop::getInstance())
        {
            if (auto screen = desktop->getScreenContaining (this))
                availableArea = screen->workArea;
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
                        // Transform target component's local bounds to parent component's coordinate space
                        targetArea = options.parentComponent->getLocalArea (options.targetComponent, options.targetComponent->getLocalBounds()).to<int>();
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
    // Check if click is in scroll indicator areas
    if (showScrollIndicators)
    {
        if (getScrollUpIndicatorBounds().contains (position) || getScrollDownIndicatorBounds().contains (position))
            return -1; // Click was on scroll indicators
    }

    // Check visible items only
    for (int i = visibleItemRange.getStart(); i < jmin (visibleItemRange.getEnd(), static_cast<int> (items.size())); ++i)
    {
        const auto& item = *items[i];
        if (item.area.contains (position))
            return i;
    }

    return -1;
}

//==============================================================================

void PopupMenu::show (std::function<void (int)> callback)
{
    showCustom (options, false, std::move (callback));
}

//==============================================================================

void PopupMenu::showCustom (const Options& options, bool isSubmenu, std::function<void (int)> callback)
{
    if (! isSubmenu)
        dismissAllPopups();

    this->options = options;
    menuCallback = std::move (callback);

    if (isEmpty())
    {
        dismiss();
        return;
    }

    installGlobalMouseListener();

    if (options.parentComponent)
    {
        // When we have a parent component, add as child to work in local coordinates
        if (getParentComponent() != options.parentComponent)
            options.parentComponent->addChildComponent (this);
    }
    else
    {
        // When we have no parent component, add to desktop to work in screen coordinates
        auto nativeOptions = ComponentNative::Options {}
                                 .withDecoration (false)
                                 .withResizableWindow (false);

        if (! isOnDesktop())
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

    hideSubmenus();

    setVisible (false);

    selectedItemIndex = -1;

    if (auto itemCallback = std::exchange (menuCallback, {}))
        itemCallback (itemID);

    if (onItemSelected != nullptr)
        onItemSelected (itemID);

    removeActivePopup (this);
}

//==============================================================================

void PopupMenu::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
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
        // Hide any visible submenus when selecting a non-separator item
        hideSubmenus();

        dismiss (item.itemID);
    }
}

void PopupMenu::mouseMove (const MouseEvent& event)
{
    int itemIndex = getItemIndexAt (event.getPosition());

    if (itemIndex >= 0 && isItemSelectable (itemIndex))
    {
        // Set selection on hover for selectable items
        setSelectedItemIndex (itemIndex, true);

        // Show submenu if it's a submenu item, but don't set selection on the submenu
        auto& item = *items[itemIndex];
        if (item.isSubMenu() && item.isEnabled)
        {
            showSubmenu (itemIndex);

            // Submenu opened via hover should have no initial selection
            if (currentSubmenu)
            {
                currentSubmenu->selectedItemIndex = -1;
                currentSubmenu->repaint();
            }
        }
    }
    else if (itemIndex < 0)
    {
        // Mouse is not over any item, clear selection
        setSelectedItemIndex (-1, true);
    }
}

void PopupMenu::mouseEnter (const MouseEvent& event)
{
    int itemIndex = getItemIndexAt (event.getPosition());
    if (itemIndex >= 0 && isItemSelectable (itemIndex))
        setSelectedItemIndex (itemIndex, true);
}

void PopupMenu::mouseExit (const MouseEvent& event)
{
    setSelectedItemIndex (-1, true);
}

void PopupMenu::mouseWheel (const MouseEvent& event, const MouseWheelData& wheel)
{
    if (! needsScrolling())
        return;

    auto deltaY = wheel.getDeltaY();

    if (deltaY > 0)
        scrollUp();
    else if (deltaY < 0)
        scrollDown();
}

void PopupMenu::keyDown (const KeyPress& key, const Point<float>& position)
{
    auto keyCode = key.getKey();

    if (keyCode == KeyPress::escapeKey)
        dismiss();

    else if (keyCode == KeyPress::upKey)
        navigateUp();

    else if (keyCode == KeyPress::downKey)
        navigateDown();

    else if (keyCode == KeyPress::leftKey)
        navigateLeft();

    else if (keyCode == KeyPress::rightKey)
        navigateRight();

    else if (keyCode == KeyPress::enterKey)
        selectCurrentItem();
}

//==============================================================================
// Submenu functionality

void PopupMenu::showSubmenu (int itemIndex)
{
    if (! canShowSubmenu (itemIndex))
        return;

    auto& item = *items[itemIndex];

    // If we're already showing this submenu, no need to do anything
    if (isAlreadyShowingSubmenu (itemIndex, item))
        return;

    // Hide current submenu if different item
    if (submenuItemIndex != itemIndex)
        hideSubmenus();

    isShowingSubmenu = true;
    submenuItemIndex = itemIndex;
    currentSubmenu = item.subMenu;

    if (! currentSubmenu)
        return;

    // Reset the submenu's state before showing to ensure clean positioning
    resetSubmenuState (currentSubmenu);

    // Configure submenu options
    auto submenuOptions = prepareSubmenuOptions (currentSubmenu);

    // Position the submenu
    positionSubmenu (submenuOptions);

    // Show the submenu with callback
    currentSubmenu->showCustom (submenuOptions, true, [this] (int selectedID)
    {
        if (selectedID != 0)
            dismiss (selectedID);

        isShowingSubmenu = false;
    });

    // Repaint to show active submenu highlight
    repaint();
}

bool PopupMenu::canShowSubmenu (int itemIndex) const
{
    if (! isPositiveAndBelow (itemIndex, getNumItems()))
        return false;

    auto& item = *items[itemIndex];
    return item.isSubMenu() && item.subMenu != nullptr;
}

bool PopupMenu::isAlreadyShowingSubmenu (int itemIndex, const Item& item) const
{
    return submenuItemIndex == itemIndex
        && currentSubmenu
        && currentSubmenu == item.subMenu
        && currentSubmenu->isVisible();
}

void PopupMenu::positionSubmenu (Options& submenuOptions)
{
    if (submenuItemIndex < 0 || ! isPositiveAndBelow (submenuItemIndex, getNumItems()))
        return;

    auto& item = *items[submenuItemIndex];
    auto itemBounds = item.area;
    auto placement = calculateSubmenuPlacement (itemBounds, submenuOptions);

    applySubmenuPlacement (submenuOptions, itemBounds, placement);
}

PopupMenu::Options PopupMenu::prepareSubmenuOptions (PopupMenu::Ptr submenu)
{
    Options submenuOptions = submenu->getOptions();
    submenuOptions.parentComponent = options.parentComponent;
    return submenuOptions;
}

PopupMenu::Placement PopupMenu::calculateSubmenuPlacement (Rectangle<float> itemBounds, const Options& submenuOptions)
{
    // Calculate available space to determine best positioning
    Rectangle<float> availableArea;
    Rectangle<float> menuBounds;

    if (options.parentComponent)
    {
        availableArea = options.parentComponent->getLocalBounds().to<float>();
        menuBounds = getBounds().to<float>();
    }
    else
    {
        availableArea = Rectangle<float> (0, 0, 1920, 1080); // TODO: move to magic
        if (auto* desktop = Desktop::getInstance())
        {
            if (auto screen = desktop->getPrimaryScreen())
                availableArea = screen->workArea.to<float>();
        }
        menuBounds = getScreenBounds().to<float>();
    }

    // Calculate space available on right and left sides
    auto rightSpaceAvailable = availableArea.getRight() - menuBounds.getRight();
    auto leftSpaceAvailable = menuBounds.getX() - availableArea.getX();

    // Assume submenu needs at least 150 pixels width (reasonable minimum)
    const int minSubmenuWidth = submenuOptions.minWidth.value_or (150); // TODO: Move to Style properties
    bool useRightSide = rightSpaceAvailable >= minSubmenuWidth;

    // If right side doesn't have enough space, try left side
    if (! useRightSide && leftSpaceAvailable >= minSubmenuWidth)
        useRightSide = false;
    else if (! useRightSide)
        useRightSide = true; // Default to right even if cramped

    return useRightSide ? Placement::toRight (Justification::topLeft) : Placement::toLeft (Justification::topRight);
}

void PopupMenu::applySubmenuPlacement (Options& submenuOptions, Rectangle<float> itemBounds, Placement placement)
{
    if (options.parentComponent)
    {
        // Position relative to parent component - need to transform coordinates properly
        auto menuPosInParent = getTopLeft().to<float>(); // This menu's position within parent
        auto itemBoundsInParent = itemBounds.translated (menuPosInParent);

        submenuOptions.withTargetArea (itemBoundsInParent, placement);
    }
    else
    {
        // Use screen coordinates when no parent
        Point<float> anchorPoint;
        if (placement.side == Side::toRight)
            anchorPoint = getScreenBounds().getTopLeft().to<float>() + itemBounds.getTopRight();
        else
            anchorPoint = getScreenBounds().getTopLeft().to<float>() + itemBounds.getTopLeft();

        submenuOptions.withTargetArea (Rectangle<float> (anchorPoint.getX(), anchorPoint.getY(), 1, itemBounds.getHeight()), placement);
    }
}

void PopupMenu::hideSubmenus()
{
    if (currentSubmenu)
    {
        cleanupSubmenu (currentSubmenu);

        currentSubmenu = nullptr;
        submenuItemIndex = -1;
        isShowingSubmenu = false;
    }

    takeKeyboardFocus();

    repaint();
}

void PopupMenu::cleanupSubmenu (PopupMenu::Ptr submenu)
{
    if (! submenu)
        return;

    submenu->setVisible (false);

    removeActivePopup (submenu.get());

    if (submenu->getParentComponent())
    {
        submenu->getParentComponent()->removeChildComponent (submenu.get());
    }
    else if (submenu->isOnDesktop())
    {
        submenu->removeFromDesktop();
    }

    // Reset the submenu's internal state to allow it to be shown again
    resetSubmenuState (submenu);
}

void PopupMenu::resetSubmenuState (PopupMenu::Ptr submenu)
{
    if (! submenu)
        return;

    // Call the public method to reset the submenu's state
    submenu->resetInternalState();
}

void PopupMenu::resetInternalState()
{
    // Reset flags that might prevent re-showing
    isBeingDismissed = false;
    setSelectedItemIndex (-1, true);

    // Reset scrolling state for scrollable menus
    visibleItemRange = Range<int> (0, 0);

    // Clear any callback that might interfere
    menuCallback = nullptr;
}

bool PopupMenu::hasVisibleSubmenu() const
{
    return currentSubmenu != nullptr && currentSubmenu->isVisible();
}

bool PopupMenu::isItemShowingSubmenu (int itemIndex) const
{
    return hasVisibleSubmenu() && submenuItemIndex == itemIndex;
}

bool PopupMenu::submenuContains (const Point<float>& position) const
{
    if (! hasVisibleSubmenu())
        return false;

    return currentSubmenu->getScreenBounds().contains (position);
}

void PopupMenu::updateSubmenuVisibility (int hoveredItemIndex)
{
    if (isPositiveAndBelow (hoveredItemIndex, getNumItems()))
    {
        auto& item = *items[hoveredItemIndex];
        if (item.isSubMenu() && item.isEnabled)
        {
            if (submenuItemIndex == hoveredItemIndex && hasVisibleSubmenu())
                return;

            showSubmenu (hoveredItemIndex);
        }
        else
        {
            if (hasVisibleSubmenu())
                hideSubmenus();
        }

        return;
    }

    if (hasVisibleSubmenu() && hoveredItemIndex >= 0 && submenuItemIndex != hoveredItemIndex)
    {
        if (isPositiveAndBelow (hoveredItemIndex, getNumItems()))
        {
            auto& newItem = *items[hoveredItemIndex];
            if (newItem.isSubMenu() && newItem.isEnabled)
            {
                showSubmenu (hoveredItemIndex);
                return;
            }
            else
            {
                hideSubmenus();
                return;
            }
        }
    }
}

//==============================================================================
// Scrolling functionality

void PopupMenu::calculateAvailableHeight()
{
    if (options.parentComponent)
    {
        // Calculate available height within parent component bounds
        auto parentBounds = options.parentComponent->getLocalBounds().to<float>();

        // Use the target position/area to determine where the menu will be positioned
        float menuY = 0.0f;

        switch (options.positioningMode)
        {
            case PositioningMode::atPoint:
                menuY = options.targetPosition.getY();
                break;

            case PositioningMode::relativeToArea:
                menuY = options.targetArea.getY();
                if (options.placement.side == Side::below)
                    menuY = options.targetArea.getBottom();
                else if (options.placement.side == Side::above)
                    menuY = options.targetArea.getY(); // Will be adjusted later
                break;

            case PositioningMode::relativeToComponent:
                if (options.targetComponent)
                {
                    Rectangle<int> targetArea;
                    if (options.targetComponent->getParentComponent() == options.parentComponent)
                        targetArea = options.targetComponent->getBounds().to<int>();
                    else
                        targetArea = options.parentComponent->getLocalArea (options.targetComponent, options.targetComponent->getLocalBounds()).to<int>();

                    menuY = targetArea.getY();
                    if (options.placement.side == Side::below)
                        menuY = targetArea.getBottom();
                }
                break;
        }

        // Calculate available space from anticipated position to parent bottom
        availableContentHeight = parentBounds.getBottom() - menuY;
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

                // Estimate menu position for screen coordinate calculation
                float menuY = 0.0f;
                if (options.positioningMode == PositioningMode::atPoint)
                    menuY = options.targetPosition.getY();
                else if (options.positioningMode == PositioningMode::relativeToArea)
                    menuY = options.targetArea.getY();
                else if (options.positioningMode == PositioningMode::relativeToComponent && options.targetComponent)
                    menuY = options.targetComponent->getScreenBounds().getY();

                availableContentHeight = screenBounds.getBottom() - menuY;
                availableContentHeight = jmax (100.0f, availableContentHeight);
            }
            else
            {
                availableContentHeight = 800.0f; // Fallback
            }
        }
        else
        {
            availableContentHeight = 800.0f; // Fallback
        }
    }
}

void PopupMenu::layoutVisibleItems (float width)
{
    constexpr float separatorHeight = 8.0f; // TODO: move to Options
    constexpr float verticalPadding = 4.0f; // TODO: move to Style
    const float itemHeight = 22.0f;         // TODO: move to Options

    // Clear all item areas first to prevent rendering artifacts
    for (auto& item : items)
    {
        item->area = Rectangle<float>();
    }

    float currentY = verticalPadding;
    if (showScrollIndicators)
        currentY += scrollIndicatorHeight; // Space for up arrow

    for (int i = visibleItemRange.getStart(); i < visibleItemRange.getEnd() && i < static_cast<int> (items.size()); ++i)
    {
        auto& item = *items[i];

        if (item.isCustomComponent())
        {
            // Custom component
            const float componentHeight = item.customComponent->getHeight();
            item.area = Rectangle<float> (0.0f, currentY, width, componentHeight);
            item.customComponent->setBounds (item.area);
            addAndMakeVisible (item.customComponent.get());
            currentY += componentHeight;
        }
        else
        {
            // Regular text item or separator
            const auto height = item.isSeparator() ? separatorHeight : itemHeight;
            item.area = Rectangle<float> (0.0f, currentY, width, height);
            currentY += height;
        }
    }
}

void PopupMenu::updateScrolling()
{
    // This method is now simplified - it just updates the visible range
    // The available height calculation is done separately in calculateAvailableHeight()
    showScrollIndicators = needsScrolling();
    updateVisibleItemRange();
}

void PopupMenu::updateVisibleItemRange()
{
    if (! needsScrolling())
    {
        // All items are visible
        visibleItemRange = Range<int> (0, static_cast<int> (items.size()));
        return;
    }

    // Calculate how many items can fit in the available space
    constexpr float separatorHeight = 8.0f; // TODO: move to Options
    constexpr float verticalPadding = 4.0f; // TODO: move to Style
    const float itemHeight = 22.0f;         // TODO: move to Options

    float availableHeight = availableContentHeight;
    if (showScrollIndicators)
        availableHeight -= 2 * scrollIndicatorHeight;

    availableHeight -= 2 * verticalPadding; // Top and bottom padding

    // Get the current start index (preserve it if already set correctly)
    int startIndex = visibleItemRange.getStart();

    // Ensure start index is valid
    startIndex = jlimit (0, jmax (0, static_cast<int> (items.size()) - 1), startIndex);

    // Calculate visible item count by iterating through items starting from startIndex
    int visibleCount = 0;
    float usedHeight = 0.0f;

    for (int i = startIndex; i < static_cast<int> (items.size()); ++i)
    {
        const auto& item = *items[i];
        float itemHeightToAdd;

        if (item.isCustomComponent())
            itemHeightToAdd = item.customComponent->getHeight();
        else
            itemHeightToAdd = item.isSeparator() ? separatorHeight : itemHeight;

        if (usedHeight + itemHeightToAdd > availableHeight)
            break;

        usedHeight += itemHeightToAdd;
        visibleCount++;
    }

    // Ensure we show at least one item
    if (visibleCount == 0 && startIndex < static_cast<int> (items.size()))
        visibleCount = 1;

    visibleItemRange = Range<int> (startIndex, startIndex + visibleCount);
}

void PopupMenu::scrollUp()
{
    if (canScrollUp())
    {
        // Update the visible range start
        int newStart = jmax (0, visibleItemRange.getStart() - scrollSpeed);
        visibleItemRange = Range<int> (newStart, newStart);

        // Recalculate the end based on available space
        updateVisibleItemRange();

        // Re-layout visible items without changing menu size
        layoutVisibleItems (getWidth());

        // Repaint to update the display
        repaint();
    }
}

void PopupMenu::scrollDown()
{
    if (canScrollDown())
    {
        // Update the visible range start
        int newStart = jmin (static_cast<int> (items.size()) - 1, visibleItemRange.getStart() + scrollSpeed);
        visibleItemRange = Range<int> (newStart, newStart);

        // Recalculate the end based on available space
        updateVisibleItemRange();

        // Re-layout visible items without changing menu size
        layoutVisibleItems (getWidth());

        // Repaint to update the display
        repaint();
    }
}

bool PopupMenu::canScrollUp() const
{
    return visibleItemRange.getStart() > 0;
}

bool PopupMenu::canScrollDown() const
{
    return visibleItemRange.getEnd() < static_cast<int> (items.size());
}

int PopupMenu::getVisibleItemCount() const
{
    return jmax (0, visibleItemRange.getLength());
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

//==============================================================================
// Keyboard navigation

void PopupMenu::navigateUp()
{
    int currentIndex = getSelectedItemIndex();

    if (currentIndex == -1)
    {
        // No current selection, select the last selectable item
        int lastIndex = getLastSelectableItemIndex();
        if (lastIndex >= 0)
        {
            setSelectedItemIndex (lastIndex, false);

            // Ensure the selected item is visible by scrolling if needed
            if (needsScrolling() && lastIndex < visibleItemRange.getStart())
            {
                while (lastIndex < visibleItemRange.getStart() && canScrollUp())
                    scrollUp();
            }
        }
    }
    else
    {
        // Move to previous selectable item
        int newIndex = getPreviousSelectableItemIndex (currentIndex);
        if (newIndex >= 0)
        {
            setSelectedItemIndex (newIndex, false);

            // Ensure the selected item is visible by scrolling if needed
            if (needsScrolling() && newIndex < visibleItemRange.getStart())
            {
                while (newIndex < visibleItemRange.getStart() && canScrollUp())
                    scrollUp();
            }
        }
    }
}

void PopupMenu::navigateDown()
{
    int currentIndex = getSelectedItemIndex();

    if (currentIndex == -1)
    {
        // No current selection, select the first selectable item
        int firstIndex = getFirstSelectableItemIndex();
        if (firstIndex >= 0)
        {
            setSelectedItemIndex (firstIndex, false);

            // Ensure the selected item is visible by scrolling if needed
            if (needsScrolling() && firstIndex >= visibleItemRange.getEnd())
            {
                while (firstIndex >= visibleItemRange.getEnd() && canScrollDown())
                    scrollDown();
            }
        }
    }
    else
    {
        // Move to next selectable item
        int newIndex = getNextSelectableItemIndex (currentIndex);
        if (newIndex >= 0)
        {
            setSelectedItemIndex (newIndex, false);

            // Ensure the selected item is visible by scrolling if needed
            if (needsScrolling() && newIndex >= visibleItemRange.getEnd())
            {
                while (newIndex >= visibleItemRange.getEnd() && canScrollDown())
                    scrollDown();
            }
        }
    }
}

void PopupMenu::navigateLeft()
{
    int currentIndex = getSelectedItemIndex();
    if (currentIndex < 0 || currentIndex >= static_cast<int> (items.size()))
        return;

    auto& item = *items[currentIndex];
    if (item.isSubMenu() && isItemShowingSubmenu (currentIndex))
    {
        // Determine submenu placement to decide if left arrow should close it
        auto itemBounds = item.area;
        auto submenuOptions = prepareSubmenuOptions (item.subMenu);
        auto placement = calculateSubmenuPlacement (itemBounds, submenuOptions);

        if (placement.side == Side::toRight)
        {
            // Submenu is on the right, left arrow closes it and keeps current item selected
            hideSubmenus();
            return;
        }
        else if (placement.side == Side::toLeft)
        {
            // Submenu is on the left, left arrow enters it and selects first item
            enterSubmenuViaKeyboard (currentIndex);
            return;
        }
    }
    else if (item.isSubMenu() && item.isEnabled)
    {
        // Submenu is not open, check if left arrow should open it
        auto itemBounds = item.area;
        auto submenuOptions = prepareSubmenuOptions (item.subMenu);
        auto placement = calculateSubmenuPlacement (itemBounds, submenuOptions);

        if (placement.side == Side::toLeft)
        {
            // Submenu would be on the left, open it and select first item
            enterSubmenuViaKeyboard (currentIndex);
            return;
        }
    }

    // If we have a parent menu, return focus to it and keep the submenu item selected
    if (parentMenu)
    {
        if (auto parent = parentMenu.get())
        {
            if (auto popupParent = dynamic_cast<PopupMenu*> (parent))
                popupParent->hideSubmenus();
        }
    }
}

void PopupMenu::navigateRight()
{
    int currentIndex = getSelectedItemIndex();
    if (currentIndex < 0 || currentIndex >= static_cast<int> (items.size()))
        return;

    auto& item = *items[currentIndex];

    if (item.isSubMenu() && item.isEnabled)
    {
        if (isItemShowingSubmenu (currentIndex))
        {
            // Determine submenu placement to decide if right arrow should enter or close it
            auto itemBounds = item.area;
            auto submenuOptions = prepareSubmenuOptions (item.subMenu);
            auto placement = calculateSubmenuPlacement (itemBounds, submenuOptions);

            if (placement.side == Side::toLeft)
            {
                // Submenu is on the left, right arrow closes it
                hideSubmenus();
                return;
            }
            else if (placement.side == Side::toRight)
            {
                // Submenu is on the right and already open, enter it and select first item
                enterSubmenuViaKeyboard (currentIndex);
                return;
            }
        }
        else
        {
            // Determine if we should open the submenu
            auto itemBounds = item.area;
            auto submenuOptions = prepareSubmenuOptions (item.subMenu);
            auto placement = calculateSubmenuPlacement (itemBounds, submenuOptions);

            if (placement.side == Side::toRight)
            {
                // Submenu would be on the right, open it and select first item
                enterSubmenuViaKeyboard (currentIndex);
                return;
            }
        }
    }
}

void PopupMenu::selectCurrentItem()
{
    int currentIndex = getSelectedItemIndex();

    if (currentIndex >= 0 && currentIndex < static_cast<int> (items.size()))
    {
        auto& item = *items[currentIndex];

        if (item.isEnabled && ! item.isSeparator())
        {
            if (item.isSubMenu())
            {
                // For submenus, open them if not already open (with no initial selection)
                if (! isItemShowingSubmenu (currentIndex))
                {
                    showSubmenu (currentIndex);
                    if (currentSubmenu)
                    {
                        currentSubmenu->parentMenu = this;
                        // Don't set any initial selection on the submenu when opened via Enter
                    }
                }
            }
            else
            {
                // For regular items, dismiss with their ID
                dismiss (item.itemID);
            }
        }
    }
}

void PopupMenu::setSelectedItemIndex (int index, bool fromMouse)
{
    if (selectedItemIndex == index)
        return;

    if (selectedItemIndex >= 0 && selectedItemIndex < static_cast<int> (items.size()))
        items[selectedItemIndex]->isHovered = false;

    selectedItemIndex = index;

    if (selectedItemIndex >= 0 && selectedItemIndex < static_cast<int> (items.size()))
        items[selectedItemIndex]->isHovered = true;

    if (fromMouse)
        updateSubmenuVisibility (index);

    repaint();
}

bool PopupMenu::isItemSelectable (int index) const
{
    if (index < 0 || index >= static_cast<int> (items.size()))
        return false;

    const auto& item = *items[index];
    return item.isEnabled && ! item.isSeparator();
}

int PopupMenu::getSelectedItemIndex() const
{
    return selectedItemIndex;
}

int PopupMenu::getFirstSelectableItemIndex() const
{
    for (int i = 0; i < static_cast<int> (items.size()); ++i)
    {
        if (isItemSelectable (i))
            return i;
    }

    return -1;
}

int PopupMenu::getLastSelectableItemIndex() const
{
    for (int i = static_cast<int> (items.size()) - 1; i >= 0; --i)
    {
        if (isItemSelectable (i))
            return i;
    }

    return -1;
}

int PopupMenu::getNextSelectableItemIndex (int currentIndex, bool forward) const
{
    if (items.empty())
        return -1;

    int itemCount = static_cast<int> (items.size());

    if (currentIndex < 0)
    {
        // No current selection, return first or last depending on direction
        return forward ? getFirstSelectableItemIndex() : getLastSelectableItemIndex();
    }

    int step = forward ? 1 : -1;
    int nextIndex = currentIndex + step;

    // Wrap around
    if (nextIndex >= itemCount)
        nextIndex = 0;
    else if (nextIndex < 0)
        nextIndex = itemCount - 1;

    // Find the next selectable item
    int startIndex = nextIndex;
    do
    {
        if (isItemSelectable (nextIndex))
            return nextIndex;

        nextIndex += step;
        if (nextIndex >= itemCount)
            nextIndex = 0;
        else if (nextIndex < 0)
            nextIndex = itemCount - 1;

    } while (nextIndex != startIndex);

    return -1; // No selectable items found
}

int PopupMenu::getNextSelectableItemIndex (int currentIndex) const
{
    return getNextSelectableItemIndex (currentIndex, true);
}

int PopupMenu::getPreviousSelectableItemIndex (int currentIndex) const
{
    if (items.empty() || currentIndex < 0)
        return -1;

    int itemCount = static_cast<int> (items.size());

    // Start from the previous item
    for (int i = currentIndex - 1; i >= 0; --i)
    {
        if (isItemSelectable (i))
            return i;
    }

    // Wrap around to the end
    for (int i = itemCount - 1; i > currentIndex; --i)
    {
        if (isItemSelectable (i))
            return i;
    }

    return -1; // No selectable items found
}

void PopupMenu::enterSubmenuViaKeyboard (int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int> (items.size()))
        return;

    auto& item = *items[itemIndex];
    if (item.isSubMenu() && item.isEnabled)
    {
        if (! isItemShowingSubmenu (itemIndex))
            showSubmenu (itemIndex);

        if (currentSubmenu)
        {
            currentSubmenu->parentMenu = this;

            // When entering submenu via keyboard, select the first selectable item
            int firstIndex = currentSubmenu->getFirstSelectableItemIndex();
            if (firstIndex >= 0)
                currentSubmenu->setSelectedItemIndex (firstIndex, false);
        }
    }
}

} // namespace yup
