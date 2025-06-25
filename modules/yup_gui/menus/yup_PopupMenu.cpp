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

//==============================================================================

namespace
{

static std::vector<PopupMenu::Ptr> activePopups;

Point<int> calculatePositionWithJustification (const Rectangle<int>& targetArea,
                                               const Size<int>& menuSize,
                                               Justification justification)
{
    Point<int> position;

    switch (justification)
    {
        default:
        case Justification::topLeft:
            position = targetArea.getTopLeft();
            break;

        case Justification::centerTop:
            position = Point<int> (targetArea.getCenterX() - menuSize.getWidth() / 2, targetArea.getTop());
            break;

        case Justification::topRight:
            position = targetArea.getTopRight().translated (-menuSize.getWidth(), 0);
            break;

        case Justification::bottomLeft:
            position = targetArea.getBottomLeft();
            break;

        case Justification::centerBottom:
            position = Point<int> (targetArea.getCenterX() - menuSize.getWidth() / 2, targetArea.getBottom());
            break;

        case Justification::bottomRight:
            position = targetArea.getBottomRight().translated (-menuSize.getWidth(), 0);
            break;

        case Justification::centerRight:
            position = Point<int> (targetArea.getRight(), targetArea.getCenterY() - menuSize.getHeight() / 2);
            break;

        case Justification::centerLeft:
            position = Point<int> (targetArea.getX() - menuSize.getWidth(), targetArea.getCenterY() - menuSize.getHeight() / 2);
            break;
    }

    return position;
}

Point<int> constrainPositionToAvailableArea (Point<int> desiredPosition,
                                             const Size<int>& menuSize,
                                             const Rectangle<int>& availableArea,
                                             const Rectangle<int>& targetArea)
{
    // Add padding to keep menu slightly away from screen edges
    const int padding = 5;
    auto constrainedArea = availableArea.reduced (padding);

    Point<int> position = desiredPosition;

    // Check if menu fits in desired position
    Rectangle<int> menuBounds (position, menuSize);

    // If menu doesn't fit, try alternative positions
    if (! constrainedArea.contains (menuBounds))
    {
        // Try to keep menu fully visible by adjusting position

        // Horizontal adjustment
        if (menuBounds.getRight() > constrainedArea.getRight())
        {
            // Try moving left
            position.setX (constrainedArea.getRight() - menuSize.getWidth());

            // If that puts us over the target, try positioning on the left side
            if (Rectangle<int> (position, menuSize).intersects (targetArea))
            {
                position.setX (targetArea.getX() - menuSize.getWidth());
            }
        }
        else if (menuBounds.getX() < constrainedArea.getX())
        {
            // Try moving right
            position.setX (constrainedArea.getX());

            // If that puts us over the target, try positioning on the right side
            if (Rectangle<int> (position, menuSize).intersects (targetArea))
            {
                position.setX (targetArea.getRight());
            }
        }

        // Vertical adjustment
        if (menuBounds.getBottom() > constrainedArea.getBottom())
        {
            // Try moving up
            position.setY (constrainedArea.getBottom() - menuSize.getHeight());

            // If that puts us over the target, try positioning above
            if (Rectangle<int> (position, menuSize).intersects (targetArea))
            {
                position.setY (targetArea.getY() - menuSize.getHeight());
            }
        }
        else if (menuBounds.getY() < constrainedArea.getY())
        {
            // Try moving down
            position.setY (constrainedArea.getY());

            // If that puts us over the target, try positioning below
            if (Rectangle<int> (position, menuSize).intersects (targetArea))
            {
                position.setY (targetArea.getBottom());
            }
        }

        // Final bounds check - ensure we're at least partially visible
        position.setX (jlimit (constrainedArea.getX(),
                               jmax (constrainedArea.getX(), constrainedArea.getRight() - menuSize.getWidth()),
                               position.getX()));
        position.setY (jlimit (constrainedArea.getY(),
                               jmax (constrainedArea.getY(), constrainedArea.getBottom() - menuSize.getHeight()),
                               position.getY()));
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

namespace
{

struct GlobalMouseListener : public MouseListener
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
            }
        }

        if (! clickedInsidePopup && ! activePopups.empty())
            PopupMenu::dismissAllPopups();
    }
};

void installGlobalMouseListener()
{
    static bool mouseListenerAdded = []
    {
        static GlobalMouseListener globalMouseListener {};
        Desktop::getInstance()->addGlobalMouseListener (&globalMouseListener);

        MessageManager::getInstance()->registerShutdownCallback ([]
        {
            PopupMenu::dismissAllPopups();
        });

        return true;
    }();
}

} // namespace

//==============================================================================

PopupMenu::Options::Options()
    : parentComponent (nullptr)
    , dismissOnSelection (true)
    , justification (Justification::bottomLeft)
    , addAsChildToTopmost (false)
    , useNativeMenus (false)
{
}

PopupMenu::Options& PopupMenu::Options::withParentComponent (Component* parentComponent)
{
    this->parentComponent = parentComponent;
    return *this;
}

PopupMenu::Options& PopupMenu::Options::withTargetArea (const Rectangle<int>& targetArea)
{
    this->targetArea = targetArea;
    return *this;
}

PopupMenu::Options& PopupMenu::Options::withJustification (Justification justification)
{
    this->justification = justification;
    return *this;
}

PopupMenu::Options& PopupMenu::Options::withTargetPosition (const Point<int>& targetPosition)
{
    this->targetPosition = targetPosition;
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

PopupMenu::Options& PopupMenu::Options::withAsChildToTopmost (bool addAsChildToTopmost)
{
    this->addAsChildToTopmost = addAsChildToTopmost;
    return *this;
}

PopupMenu::Options& PopupMenu::Options::withNativeMenus (bool useNativeMenus)
{
    this->useNativeMenus = useNativeMenus;
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
        dimiss();
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

void PopupMenu::addItemsFromMenu (const PopupMenu& otherMenu)
{
    for (const auto& otherItem : otherMenu.items)
    {
        if (otherItem->isSeparator())
        {
            addSeparator();
        }
        else if (otherItem->isSubMenu())
        {
            addSubMenu (otherItem->text, otherItem->subMenu, otherItem->isEnabled);
        }
        else if (otherItem->isCustomComponent())
        {
            // Note: Custom components can't be easily copied, so we skip them
            // In a real implementation, you might want to clone them or handle differently
        }
        else
        {
            auto item = std::make_unique<Item> (otherItem->text, otherItem->itemID, otherItem->isEnabled, otherItem->isTicked);
            item->shortcutKeyText = otherItem->shortcutKeyText;
            item->textColor = otherItem->textColor;
            items.push_back (std::move (item));
        }
    }
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
        repaint();
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

    setSize ({ width, y + verticalPadding }); // Bottom padding
}

void PopupMenu::positionMenu()
{
    auto menuSize = getSize();
    Rectangle<int> targetArea;
    Rectangle<int> availableArea;

    // Determine target area and available area
    if (options.parentComponent)
    {
        // Get the bounds relative to the screen or topmost component
        if (options.addAsChildToTopmost)
        {
            // Target area is relative to topmost component
            targetArea = options.parentComponent->getBounds().to<int>();
            availableArea = targetArea;
        }
        else
        {
            // Target area is in screen coordinates
            targetArea = options.parentComponent->getScreenBounds().to<int>();

            // Available area is the screen bounds
            if (auto* desktop = Desktop::getInstance())
            {
                if (auto screen = desktop->getScreenContaining (targetArea.getCenter()))
                    availableArea = screen->workArea;
                else if (auto screen = desktop->getPrimaryScreen())
                    availableArea = screen->workArea;
                else
                    availableArea = targetArea;
            }
        }

        // Override with explicit target area if provided
        if (! options.targetArea.isEmpty())
        {
            if (options.addAsChildToTopmost)
                targetArea = options.targetArea;
            else
                targetArea = options.targetArea.translated (targetArea.getPosition());
        }
    }
    else
    {
        if (! options.targetArea.isEmpty())
            targetArea = options.targetArea;
        else
            targetArea = Rectangle<int> (options.targetPosition, options.targetArea.getSize());

        // Get screen bounds for available area
        if (auto* desktop = Desktop::getInstance())
        {
            if (auto screen = desktop->getScreenContaining (targetArea.getCenter()))
                availableArea = screen->workArea;
            else if (auto screen = desktop->getPrimaryScreen())
                availableArea = screen->workArea;
            else
                availableArea = targetArea;
        }
    }

    // Calculate position based on justification
    Point<int> position = calculatePositionWithJustification (targetArea, menuSize.to<int>(), options.justification).to<int>();

    // Adjust position to fit within available area
    position = constrainPositionToAvailableArea (position, menuSize.to<int>(), availableArea, targetArea).to<int>();

    setTopLeft (position);
}

//==============================================================================

int PopupMenu::getItemIndexAt (Point<float> position) const
{
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
    dismissAllPopups();

    menuCallback = std::move (callback);

    if (isEmpty())
    {
        dismiss();
        return;
    }

    installGlobalMouseListener();

    setWantsKeyboardFocus (true);

    if (options.addAsChildToTopmost && options.parentComponent)
    {
        options.parentComponent->addChildComponent (this);
    }
    else
    {
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
    setVisible (false);

    setSelectedItemID (itemID);

    for (auto it = activePopups.begin(); it != activePopups.end();)
    {
        if (it->get() == this)
            it = activePopups.erase (it);
        else
            ++it;
    }
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
        // TODO: Show sub-menu
    }
    else
    {
        dismiss (item.itemID);
    }
}

void PopupMenu::mouseMove (const MouseEvent& event)
{
    setHoveredItem (getItemIndexAt (event.getPosition()));
}

void PopupMenu::mouseExit (const MouseEvent& event)
{
    setHoveredItem (-1);
}

void PopupMenu::keyDown (const KeyPress& key, const Point<float>& position)
{
    if (key.getKey() == KeyPress::escapeKey)
        dismiss();
}

//==============================================================================

void PopupMenu::focusLost()
{
    dismiss();
}

} // namespace yup
