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

constexpr float separatorHeight = 8.0f;    // TODO: move to Options
constexpr float verticalPadding = 4.0f;    // TODO: move to Style
constexpr float itemHeight = 22.0f;        // TODO: move to Options
constexpr float defaultMenuWidth = 200.0f; // TODO: move to Options
constexpr float horizontalTextPadding = 12.0f;
constexpr float tickedTextIndent = 8.0f;
constexpr float submenuArrowWidth = 24.0f;
constexpr float shortcutTextWidth = 80.0f;
constexpr float itemTextHeight = 14.0f;
constexpr float shortcutTextHeight = 13.0f;
constexpr float screenEdgePadding = 5.0f;

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

PopupMenu* findActivePopupAt (Point<float> globalPos)
{
    for (auto it = activePopups.rbegin(); it != activePopups.rend(); ++it)
    {
        auto* popupMenu = dynamic_cast<PopupMenu*> (it->get());
        if (popupMenu != nullptr && popupMenu->getScreenBounds().contains (globalPos))
            return popupMenu;
    }

    return nullptr;
}

bool isInsideAnyActivePopup (Point<float> globalPos)
{
    return findActivePopupAt (globalPos) != nullptr;
}

MouseEvent makePopupMouseEvent (const MouseEvent& event, PopupMenu& popupMenu, Point<float> globalPos)
{
    return event.withPosition (popupMenu.screenToLocal (globalPos))
        .withSourceComponent (&popupMenu);
}

void installGlobalMouseListener()
{
    static bool mouseListenerAdded = []
    {
        static struct GlobalMouseListener : MouseListener
        {
            void mouseDown (const MouseEvent& event) override
            {
                const auto globalPos = event.getScreenPosition();

                if (! isInsideAnyActivePopup (globalPos))
                {
                    if (! activePopups.empty())
                        PopupMenu::dismissAllPopups();

                    return;
                }

                // Walk the component hierarchy from the event source.
                // If any ancestor is a PopupMenu the click is inside a popup — don't dismiss.
                auto* comp = event.getSourceComponent();
                while (comp != nullptr)
                {
                    if (dynamic_cast<PopupMenu*> (comp) != nullptr)
                        return;
                    comp = comp->getParentComponent();
                }

                if (auto* popupMenu = findActivePopupAt (globalPos))
                {
                    popupMenu->mouseDown (makePopupMouseEvent (event, *popupMenu, globalPos));
                    return;
                }

                if (! activePopups.empty())
                    PopupMenu::dismissAllPopups();
            }

            void mouseMove (const MouseEvent& event) override
            {
                const auto globalPos = event.getScreenPosition();
                if (auto* popupMenu = findActivePopupAt (globalPos))
                    popupMenu->mouseMove (makePopupMouseEvent (event, *popupMenu, globalPos));
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

    switch (alignment.getFlags())
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
    auto constrainedArea = availableArea.reduced (static_cast<int> (screenEdgePadding));

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

float measureMenuTextWidth (const String& text, const Font& font)
{
    if (text.isEmpty())
        return 0.0f;

    auto styledText = StyledText();
    {
        auto modifier = styledText.startUpdate();
        modifier.setWrap (StyledText::noWrap);
        modifier.appendText (text, font);
    }

    return styledText.getComputedTextBounds().getWidth();
}

float getItemHeight (const PopupMenu::Item& item)
{
    if (item.isCustomComponent())
        return item.customComponent->getHeight();

    return item.isSeparator() ? separatorHeight : itemHeight;
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
    , focusComponent (nullptr)
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

PopupMenu::Options& PopupMenu::Options::withFocusComponent (Component* component)
{
    this->focusComponent = component;
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
    const auto globalTheme = ApplicationTheme::getGlobalTheme();
    const auto defaultFont = globalTheme != nullptr ? globalTheme->getDefaultFont()
                                                    : Font();
    const auto itemFont = defaultFont.withHeight (itemTextHeight);
    const auto shortcutFont = defaultFont.withHeight (shortcutTextHeight);
    bool anyItemIsTicked = false;
    for (const auto& item : items)
    {
        if (item->isTicked)
        {
            anyItemIsTicked = true;
            break;
        }
    }

    const auto minimumWidth = static_cast<float> (jmax (0, options.minWidth.value_or (static_cast<int> (defaultMenuWidth))));
    const auto maximumWidth = static_cast<float> (jmax (static_cast<int> (minimumWidth),
                                                        options.maxWidth.value_or (std::numeric_limits<int>::max())));

    float width = minimumWidth;

    // First pass: calculate total content height and determine width
    totalContentHeight = verticalPadding; // Top padding
    for (const auto& item : items)
    {
        if (item->isCustomComponent())
        {
            width = jmax (width, static_cast<float> (item->customComponent->getWidth()));
            totalContentHeight += item->customComponent->getHeight();
        }
        else
        {
            totalContentHeight += item->isSeparator() ? separatorHeight : itemHeight;

            if (! item->isSeparator())
            {
                auto itemWidth = horizontalTextPadding * 2.0f
                               + measureMenuTextWidth (item->text, itemFont);

                if (anyItemIsTicked)
                    itemWidth += tickedTextIndent;

                if (item->shortcutKeyText.isNotEmpty())
                    itemWidth += jmax (shortcutTextWidth,
                                       measureMenuTextWidth (item->shortcutKeyText, shortcutFont) + horizontalTextPadding);

                if (item->isSubMenu())
                    itemWidth += submenuArrowWidth;

                width = jmax (width, itemWidth);
            }
        }
    }
    totalContentHeight += verticalPadding; // Bottom padding
    width = jlimit (minimumWidth, maximumWidth, width);

    // Calculate available content height properly (without depending on current position)
    calculateAvailableHeight();

    // Determine if scrolling is needed
    showScrollIndicators = needsScrolling();

    // Initialize visible item range if not set
    if (visibleItemRange.isEmpty())
        visibleItemRange = Range<int> (0, 0);

    updateVisibleItemRange();

    const auto menuHeight = verticalPadding * 2.0f
                          + getVisibleItemsHeight()
                          + (showScrollIndicators ? scrollIndicatorHeight * 2.0f : 0.0f);
    setSize (static_cast<int> (std::ceil (width)),
             static_cast<int> (std::ceil (jmax (itemHeight, menuHeight))));

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
            Screen::Ptr screen;

            if (options.positioningMode == PositioningMode::atPoint)
                screen = desktop->getScreenContaining (options.targetPosition.to<float>());
            else if (options.positioningMode == PositioningMode::relativeToArea)
                screen = desktop->getScreenContaining (options.targetArea.to<float>());
            else if (options.positioningMode == PositioningMode::relativeToComponent && options.targetComponent)
                screen = desktop->getScreenContaining (options.targetComponent);

            if (screen == nullptr)
                screen = desktop->getScreenContaining (this);

            if (screen != nullptr)
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

    isBeingDismissed = false;
    this->options = options;
    menuCallback = std::move (callback);

    if (! isSubmenu)
        focusComponentToRestore = getFocusComponentForDismissal();

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
                                 .withResizableWindow (false)
                                 .withTemporaryWindow (true);

        if (! isOnDesktop())
            addToDesktop (nativeOptions);
    }

    removeActivePopup (this);
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

    if (getParentComponent() == nullptr && isOnDesktop())
        removeFromDesktop();

    selectedItemIndex = -1;

    restoreFocusAfterDismissal();

    if (auto itemCallback = std::exchange (menuCallback, {}))
        itemCallback (itemID);

    if (onItemSelected != nullptr)
        onItemSelected (itemID);

    removeActivePopup (this);
}

//==============================================================================

bool PopupMenu::isBeingShown() const
{
    return isVisible() && ! isBeingDismissed;
}

//==============================================================================

Component* PopupMenu::getFocusComponentForDismissal() const
{
    if (options.focusComponent != nullptr)
        return options.focusComponent;

    auto* referenceComponent = options.targetComponent != nullptr ? options.targetComponent
                                                                  : options.parentComponent;
    if (referenceComponent == nullptr)
        return nullptr;

    if (auto* nativeComponent = referenceComponent->getNativeComponent())
        return nativeComponent->getFocusedComponent();

    return nullptr;
}

void PopupMenu::restoreFocusAfterDismissal()
{
    auto focusToRestore = std::exchange (focusComponentToRestore, {});
    if (focusToRestore == nullptr || focusToRestore == this)
        return;

    if (auto* nativeToRestore = focusToRestore->getNativeComponent())
        nativeToRestore->setFocusedComponent (focusToRestore.get());
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

    setSelectedItemIndex (itemIndex, true);

    if (item.isSubMenu())
    {
        // For submenus, we show them on hover, not on click
        showSubmenu (itemIndex);
    }
}

void PopupMenu::mouseUp (const MouseEvent& event)
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
    if (item.isSeparator() || ! item.isEnabled || item.isSubMenu())
        return;

    hideSubmenus();
    dismiss (item.itemID);
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
    {
        setSelectedItemIndex (itemIndex, true);

        auto& item = *items[itemIndex];
        if (item.isSubMenu() && item.isEnabled)
            showSubmenu (itemIndex);
    }
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
        dismissAllPopups();

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

    currentSubmenu->parentMenu = this;

    // Reset the submenu's state before showing to ensure clean positioning
    currentSubmenu->resetInternalState();
    currentSubmenu->parentMenu = this;

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
            Screen::Ptr screen = desktop->getScreenContaining (getScreenBounds().to<float>());
            if (screen == nullptr)
                screen = desktop->getPrimaryScreen();
            if (screen != nullptr)
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

    submenu->resetInternalState();
}

void PopupMenu::resetInternalState()
{
    hideSubmenus();

    // Reset flags that might prevent re-showing
    isBeingDismissed = false;
    setSelectedItemIndex (-1, false);

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

void PopupMenu::selectCurrentItem()
{
    int currentIndex = getSelectedItemIndex();
    if (currentIndex < 0 || currentIndex >= static_cast<int> (items.size()))
        return;

    auto& item = *items[currentIndex];

    if (item.isEnabled && ! item.isSeparator())
    {
        if (item.isSubMenu())
        {
            if (! isItemShowingSubmenu (currentIndex))
            {
                showSubmenu (currentIndex);
                if (currentSubmenu)
                    currentSubmenu->parentMenu = this;
            }
        }
        else
        {
            dismiss (item.itemID);
        }
    }
}

void PopupMenu::setSelectedItemIndex (int index, bool fromMouse)
{
    if (selectedItemIndex == index)
    {
        if (fromMouse)
            updateSubmenuVisibility (index);

        return;
    }

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
        return forward ? getFirstSelectableItemIndex() : getLastSelectableItemIndex();

    const int step = forward ? 1 : -1;
    int nextIndex = currentIndex + step;

    // Wrap around
    if (nextIndex >= itemCount)
        nextIndex = 0;
    else if (nextIndex < 0)
        nextIndex = itemCount - 1;

    // Find the next selectable item
    const int startIndex = nextIndex;

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

//==============================================================================

void PopupMenu::calculateAvailableHeight()
{
    const auto minimumMenuHeight = itemHeight + (verticalPadding * 2.0f);

    if (options.parentComponent)
    {
        const auto parentBounds = options.parentComponent->getLocalBounds().to<float>();
        availableContentHeight = jmax (minimumMenuHeight, parentBounds.getHeight() - (screenEdgePadding * 2.0f));
        return;
    }

    // Use screen bounds
    if (auto* desktop = Desktop::getInstance())
    {
        Screen::Ptr screen;

        if (options.positioningMode == PositioningMode::atPoint)
            screen = desktop->getScreenContaining (options.targetPosition.to<float>());
        else if (options.positioningMode == PositioningMode::relativeToArea)
            screen = desktop->getScreenContaining (options.targetArea.to<float>());
        else if (options.positioningMode == PositioningMode::relativeToComponent && options.targetComponent)
            screen = desktop->getScreenContaining (options.targetComponent);

        if (screen == nullptr)
            screen = desktop->getPrimaryScreen();

        if (screen != nullptr)
            availableContentHeight = jmax (minimumMenuHeight, screen->workArea.getHeight() - (screenEdgePadding * 2.0f));
        else
            availableContentHeight = 800.0f; // Fallback

        return;
    }

    availableContentHeight = 800.0f; // Fallback
}

void PopupMenu::layoutVisibleItems (float width)
{
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
        const auto itemHeightToAdd = getItemHeight (*items[i]);

        if (usedHeight + itemHeightToAdd > availableHeight)
            break;

        usedHeight += itemHeightToAdd;
        visibleCount++;
    }

    // Ensure we show at least one item
    if (visibleCount == 0 && startIndex < static_cast<int> (items.size()))
        visibleCount = 1;

    while (startIndex > 0)
    {
        const auto previousItemHeight = getItemHeight (*items[startIndex - 1]);
        if (usedHeight + previousItemHeight > availableHeight)
            break;

        --startIndex;
        ++visibleCount;
        usedHeight += previousItemHeight;
    }

    visibleItemRange = Range<int> (startIndex, startIndex + visibleCount);
}

float PopupMenu::getVisibleItemsHeight() const
{
    float height = 0.0f;

    for (int i = visibleItemRange.getStart(); i < visibleItemRange.getEnd() && i < static_cast<int> (items.size()); ++i)
        height += getItemHeight (*items[i]);

    return height;
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

        // Re-layout visible items using the exact height of the visible rows.
        const auto menuHeight = verticalPadding * 2.0f
                              + getVisibleItemsHeight()
                              + (showScrollIndicators ? scrollIndicatorHeight * 2.0f : 0.0f);
        setSize (getWidth(), static_cast<int> (std::ceil (jmax (itemHeight, menuHeight))));
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

        // Re-layout visible items using the exact height of the visible rows.
        const auto menuHeight = verticalPadding * 2.0f
                              + getVisibleItemsHeight()
                              + (showScrollIndicators ? scrollIndicatorHeight * 2.0f : 0.0f);
        setSize (getWidth(), static_cast<int> (std::ceil (jmax (itemHeight, menuHeight))));
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
        if (lastIndex < 0)
            return;

        setSelectedItemIndex (lastIndex, false);

        // Ensure the selected item is visible by scrolling if needed
        if (needsScrolling() && lastIndex < visibleItemRange.getStart())
        {
            while (lastIndex < visibleItemRange.getStart() && canScrollUp())
                scrollUp();
        }
    }
    else
    {
        // Move to previous selectable item
        int newIndex = getPreviousSelectableItemIndex (currentIndex);
        if (newIndex < 0)
            return;

        setSelectedItemIndex (newIndex, false);

        // Ensure the selected item is visible by scrolling if needed
        if (needsScrolling() && newIndex < visibleItemRange.getStart())
        {
            while (newIndex < visibleItemRange.getStart() && canScrollUp())
                scrollUp();
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
        if (firstIndex < 0)
            return;

        setSelectedItemIndex (firstIndex, false);

        // Ensure the selected item is visible by scrolling if needed
        if (needsScrolling() && firstIndex >= visibleItemRange.getEnd())
        {
            while (firstIndex >= visibleItemRange.getEnd() && canScrollDown())
                scrollDown();
        }
    }
    else
    {
        // Move to next selectable item
        int newIndex = getNextSelectableItemIndex (currentIndex);
        if (newIndex < 0)
            return;

        setSelectedItemIndex (newIndex, false);

        // Ensure the selected item is visible by scrolling if needed
        if (needsScrolling() && newIndex >= visibleItemRange.getEnd())
        {
            while (newIndex >= visibleItemRange.getEnd() && canScrollDown())
                scrollDown();
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

    if (! item.isSubMenu() || ! item.isEnabled)
        return;

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

void PopupMenu::enterSubmenuViaKeyboard (int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int> (items.size()))
        return;

    auto& item = *items[itemIndex];
    if (! item.isSubMenu() || ! item.isEnabled)
        return;

    if (! isItemShowingSubmenu (itemIndex))
        showSubmenu (itemIndex);

    if (currentSubmenu == nullptr)
        return;

    currentSubmenu->parentMenu = this;

    const int firstIndex = currentSubmenu->getFirstSelectableItemIndex();
    if (firstIndex >= 0)
        currentSubmenu->setSelectedItemIndex (firstIndex, false);
}

} // namespace yup
