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

static std::vector<WeakReference<Component>> activePopups;

} // namespace

//==============================================================================

class PopupMenu::PopupMenuItem
{
public:
    //==============================================================================

    PopupMenuItem()
    {
    }

    PopupMenuItem (const String& itemText, int itemID, bool isEnabled = true, bool isTicked = false)
        : text (itemText)
        , itemID (itemID)
        , isEnabled (isEnabled)
        , isTicked (isTicked)
    {
    }

    PopupMenuItem (const String& itemText, PopupMenu::Ptr subMenu, bool isEnabled = true)
        : text (itemText)
        , isEnabled (isEnabled)
        , subMenu (std::move (subMenu))
    {
    }

    PopupMenuItem (std::unique_ptr<Component> component, int itemID)
        : itemID (itemID)
        , customComponent (std::move (component))
    {
    }

    //==============================================================================

    ~PopupMenuItem() = default;

    //==============================================================================

    bool isSeparator() const { return text.isEmpty() && itemID == 0 && subMenu == nullptr && customComponent == nullptr; }

    bool isSubMenu() const { return subMenu != nullptr; }

    bool isCustomComponent() const { return customComponent != nullptr; }

    //==============================================================================

    String text;

    int itemID = 0;

    bool isEnabled = true;

    bool isTicked = false;

    PopupMenu::Ptr subMenu;

    std::unique_ptr<Component> customComponent;

    String shortcutKeyText;

    std::optional<Color> textColor;

private:
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PopupMenuItem)
};

//==============================================================================

class PopupMenu::MenuWindow : public Component
{
public:
    MenuWindow (PopupMenu::Ptr menu, const PopupMenu::Options& opts)
        : Component ("PopupMenuWindow")
        , owner (menu)
        , options (opts)
    {
        setWantsKeyboardFocus (true);

        // Calculate required size and create menu items
        setupMenuItems();

        // Add to desktop as a popup
        ComponentNative::Options nativeOptions;
        nativeOptions
            .withDecoration (false)
            .withResizableWindow (false);

        addToDesktop (nativeOptions);

        // Position the menu
        positionMenu();

        // Add to active popups list for modal behavior
        activePopups.push_back (this);

        setVisible (true);
        takeKeyboardFocus();
    }

    ~MenuWindow() override
    {
        activePopups.erase (std::remove (activePopups.begin(), activePopups.end(), this), activePopups.end());
    }

    void paint (Graphics& g) override
    {
        // Draw menu background
        g.setFillColor (findColor (Colours::menuBackground).value_or (Color (0xff2a2a2a)));
        g.fillRoundedRect (getLocalBounds().to<float>(), 4.0f);

        // Draw border
        g.setStrokeColor (findColor (Colours::menuBorder).value_or (Color (0xff555555)));
        g.setStrokeWidth (1.0f);
        g.strokeRoundedRect (getLocalBounds().to<float>().reduced (0.5f), 4.0f);

        // Draw menu items
        drawMenuItems (g);
    }

    void focusLost() override
    {
        dismiss (0);
    }

    bool isWithinBounds (Point<float> globalPoint) const
    {
        auto localPoint = globalPoint - getScreenPosition().to<float>();
        return getLocalBounds().to<float>().contains (localPoint);
    }

    void mouseDown (const MouseEvent& event) override
    {
        auto itemIndex = getItemIndexAt (event.getPosition());
        if (itemIndex >= 0 && itemIndex < owner->items.size())
        {
            auto& item = *owner->items[itemIndex];
            if (! item.isSeparator() && item.isEnabled)
            {
                if (item.isSubMenu())
                {
                    // TODO: Show sub-menu
                }
                else
                {
                    dismiss (item.itemID);
                }
            }
        }
    }

    void mouseMove (const MouseEvent& event) override
    {
        auto newHoveredIndex = getItemIndexAt (event.getPosition());
        if (newHoveredIndex != hoveredItemIndex)
        {
            hoveredItemIndex = newHoveredIndex;
            repaint();
        }
    }

    void mouseExit (const MouseEvent& event) override
    {
        if (hoveredItemIndex >= 0)
        {
            hoveredItemIndex = -1;
            repaint();
        }
    }

    void keyDown (const KeyPress& key, const Point<float>& position) override
    {
        if (key.getKey() == KeyPress::escapeKey)
        {
            dismiss (0);
        }
    }

    void dismiss (int itemID)
    {
        selectedItemID = itemID;
        //setVisible (false);

        // Call the owner's callback
        if (owner->onItemSelected)
            owner->onItemSelected (itemID);

        delete this;
    }

private:
    // Color identifiers for theming
    struct Colours
    {
        static inline const Identifier menuBackground { "menuBackground" };
        static inline const Identifier menuBorder { "menuBorder" };
        static inline const Identifier menuItemText { "menuItemText" };
        static inline const Identifier menuItemTextDisabled { "menuItemTextDisabled" };
        static inline const Identifier menuItemBackground { "menuItemBackground" };
        static inline const Identifier menuItemBackgroundHighlighted { "menuItemBackgroundHighlighted" };
    };

    void setupMenuItems()
    {
        constexpr float separatorHeight = 8.0f;
        constexpr float verticalPadding = 4.0f;

        float y = verticalPadding; // Top padding
        float itemHeight = static_cast<float> (options.standardItemHeight);
        float width = static_cast<float> (options.minWidth > 0 ? options.minWidth : 200);

        itemRects.clear();

        for (const auto& item : owner->items)
        {
            if (item->isCustomComponent())
                width = jmax (width, item->customComponent->getWidth());
        }

        for (const auto& item : owner->items)
        {
            if (item->isSeparator())
            {
                itemRects.push_back ({ 0, y, width, separatorHeight });
                y += separatorHeight;
            }
            else if (item->isCustomComponent())
            {
                addChildComponent (*item->customComponent);

                float horizontalOffset = 0.0f;

                auto compHeight = item->customComponent->getHeight();
                auto compWidth = item->customComponent->getWidth();
                if (compWidth < width)
                    horizontalOffset = (width - compWidth) / 2.0f;

                auto& rect = itemRects.emplace_back (horizontalOffset, y, compWidth, compHeight);
                item->customComponent->setBounds (rect);
                item->customComponent->setVisible (true);

                y += compHeight;
            }
            else
            {
                itemRects.push_back ({ 0, y, width, itemHeight });
                y += itemHeight;
            }
        }

        setSize ({ width, y + verticalPadding }); // Bottom padding
    }

    void positionMenu()
    {
        Point<float> position;

        if (options.parentComponent)
        {
            position = options.parentComponent->getBoundsRelativeToTopLevelComponent().getBottomLeft();

            if (auto native = options.parentComponent->getNativeComponent())
                position.translate (native->getPosition());
        }
        else
        {
            position = options.targetScreenPosition;
        }

        setTopLeft (position.roundToInt());
    }

    int getItemIndexAt (Point<float> position) const
    {
        for (int i = 0; i < static_cast<int> (itemRects.size()); ++i)
        {
            if (itemRects[i].contains (position))
                return i;
        }

        return -1;
    }

    void drawMenuItems (Graphics& g)
    {
        auto itemFont = ApplicationTheme::getGlobalTheme()->getDefaultFont();

        for (int i = 0; i < static_cast<int> (owner->items.size()); ++i)
        {
            const auto& item = *owner->items[i];
            const auto& rect = itemRects[i];

            // Skip custom components as they render themselves
            if (item.isCustomComponent())
                continue;

            // Draw hover background
            if (i == hoveredItemIndex && ! item.isSeparator() && item.isEnabled)
            {
                g.setFillColor (findColor (Colours::menuItemBackgroundHighlighted).value_or (Color (0xff404040)));
                g.fillRoundedRect (rect.reduced (2.0f, 1.0f), 2.0f);
            }

            if (item.isSeparator())
            {
                // Draw separator line
                auto lineY = rect.getCenterY();
                g.setStrokeColor (findColor (Colours::menuBorder).value_or (Color (0xff555555)));
                g.setStrokeWidth (1.0f);
                g.strokeLine (rect.getX() + 8.0f, lineY, rect.getRight() - 8.0f, lineY);
            }
            else
            {
                // Draw menu item text
                auto textColor = item.textColor.value_or (findColor (Colours::menuItemText).value_or (Color (0xffffffff)));
                if (! item.isEnabled)
                    textColor = findColor (Colours::menuItemTextDisabled).value_or (Color (0xff808080));

                g.setFillColor (textColor);

                auto textRect = rect.reduced (12.0f, 2.0f);

                {
                    auto styledText = yup::StyledText();
                    {
                        auto modifier = styledText.startUpdate();
                        modifier.appendText (item.text, itemFont);
                    }
                    g.fillFittedText (styledText, textRect);
                }

                // Draw checkmark if ticked
                if (item.isTicked)
                {
                    auto checkRect = Rectangle<float> (rect.getX() + 4.0f, rect.getY() + 4.0f, 12.0f, 12.0f);
                    g.setStrokeColor (textColor);
                    g.setStrokeWidth (2.0f);
                    g.strokeLine (checkRect.getX() + 2.0f, checkRect.getCenterY(), checkRect.getCenterX(), checkRect.getBottom() - 2.0f);
                    g.strokeLine (checkRect.getCenterX(), checkRect.getBottom() - 2.0f, checkRect.getRight() - 2.0f, checkRect.getY() + 2.0f);
                }

                // Draw shortcut text
                if (item.shortcutKeyText.isNotEmpty())
                {
                    auto shortcutRect = Rectangle<float> (rect.getRight() - 80.0f, rect.getY(), 75.0f, rect.getHeight());
                    g.setOpacity (0.7f);

                    auto styledText = yup::StyledText();
                    {
                        auto modifier = styledText.startUpdate();
                        modifier.setHorizontalAlign (yup::StyledText::right);
                        modifier.appendText (item.shortcutKeyText, itemFont);
                    }
                    g.fillFittedText (styledText, shortcutRect);

                    g.setOpacity (1.0f);
                }

                // Draw submenu arrow
                if (item.isSubMenu())
                {
                    auto arrowRect = Rectangle<float> (rect.getRight() - 16.0f, rect.getY() + 4.0f, 8.0f, rect.getHeight() - 8.0f);
                    g.setStrokeColor (textColor);
                    g.setStrokeWidth (1.5f);
                    g.strokeLine (arrowRect.getX() + 2.0f, arrowRect.getY() + 2.0f, arrowRect.getRight() - 2.0f, arrowRect.getCenterY());
                    g.strokeLine (arrowRect.getRight() - 2.0f, arrowRect.getCenterY(), arrowRect.getX() + 2.0f, arrowRect.getBottom() - 2.0f);
                }
            }
        }
    }

    PopupMenu::Ptr owner;
    PopupMenu::Options options;
    int selectedItemID = 0;
    int hoveredItemIndex = -1;
    std::vector<Rectangle<float>> itemRects;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MenuWindow)
};

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
            if (auto* menuWindow = dynamic_cast<PopupMenu::MenuWindow*> (popup.get()))
            {
                if (menuWindow->isWithinBounds (globalPos))
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

PopupMenu::PopupMenu() = default;

PopupMenu::~PopupMenu() = default;

//==============================================================================

PopupMenu::Ptr PopupMenu::create()
{
    return new PopupMenu();
}

//==============================================================================

void PopupMenu::dismissAllPopups()
{
    // Make a copy to avoid issues with the vector being modified during iteration
    auto popupsToClose = std::move (activePopups);

    for (const auto& popup : popupsToClose)
    {
        if (auto* menuWindow = dynamic_cast<PopupMenu::MenuWindow*> (popup.get()))
            menuWindow->dismiss (0);
    }

    activePopups.clear();
}

//==============================================================================

void PopupMenu::addItem (const String& text, int itemID, bool isEnabled, bool isTicked, const String& shortcutText)
{
    auto item = std::make_unique<PopupMenuItem> (text, itemID, isEnabled, isTicked);
    item->shortcutKeyText = shortcutText;
    items.push_back (std::move (item));
}

void PopupMenu::addSeparator()
{
    items.push_back (std::make_unique<PopupMenuItem>());
}

void PopupMenu::addSubMenu (const String& text, PopupMenu::Ptr subMenu, bool isEnabled)
{
    auto item = std::make_unique<PopupMenuItem> (text, std::move (subMenu), isEnabled);
    items.push_back (std::move (item));
}

void PopupMenu::addCustomItem (std::unique_ptr<Component> component, int itemID)
{
    auto item = std::make_unique<PopupMenuItem> (std::move (component), itemID);
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
            auto item = std::make_unique<PopupMenuItem> (otherItem->text, otherItem->itemID, otherItem->isEnabled, otherItem->isTicked);
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

void PopupMenu::show (const Options& options, std::function<void (int)> callback)
{
    if (isEmpty())
    {
        if (callback)
            callback (0);

        return;
    }

    // Dismiss any existing popups first
    dismissAllPopups();

    // Show the custom menu with callback
    showCustom (options, callback);
}

//==============================================================================

void PopupMenu::showAt (Point<int> screenPos, std::function<void (int)> callback)
{
    Options options;
    options.targetScreenPosition = screenPos;

    show (options, callback);
}

//==============================================================================

void PopupMenu::showAt (Component* targetComp, std::function<void (int)> callback)
{
    if (targetComp == nullptr)
    {
        if (callback)
            callback (0);

        return;
    }

    Options options;
    options.parentComponent = targetComp;

    show (options, callback);
}

//==============================================================================

void PopupMenu::showCustom (const Options& options, std::function<void (int)> callback)
{
    installGlobalMouseListener();

    if (isEmpty())
    {
        if (callback)
            callback (0);
        return;
    }

    // Create the menu window with a reference to this menu - it will manage its own lifetime
    new MenuWindow (this, options);
}

} // namespace yup
