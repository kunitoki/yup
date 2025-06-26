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

#include <yup_gui/yup_gui.h>

//==============================================================================

class PopupMenuDemo : public yup::Component
{
public:
    PopupMenuDemo()
        : Component ("PopupMenuDemo")
        , targetButton ("targetButton")
        , statusLabel ("statusLabel")
        , currentPlacementIndex (0)
    {
        addAndMakeVisible (statusLabel);
        statusLabel.setTitle ("Click the button to test placements. Right-click for submenus and scrollable menus.");

        addAndMakeVisible (targetButton);
        targetButton.setButtonText ("Test Placement (Click Me!)");
        targetButton.onClick = [this]
        {
            showPlacementTest();
        };

        // Initialize all placement combinations
        initializePlacements();

        setSize ({ 600, 500 });
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20);

        area.removeFromTop (20);
        statusLabel.setBounds (area.removeFromTop (30));

        // Center the target button in the middle of the remaining area
        auto buttonArea = area.reduced (100);
        auto buttonCenter = buttonArea.getCenter();
        auto buttonBounds = yup::Rectangle<int> (buttonCenter.getX() - 100, buttonCenter.getY() - 20, 200, 40);
        targetButton.setBounds (buttonBounds);
    }

    void paint (yup::Graphics& g) override
    {
        auto area = getLocalBounds().reduced (5);

        auto styledText = yup::StyledText();
        {
            auto modifier = styledText.startUpdate();
            modifier.appendText ("PopupMenu Features: Placement, Submenus, Scrolling", yup::ApplicationTheme::getGlobalTheme()->getDefaultFont());
        }

        g.setFillColor (yup::Color (0xffffffff));
        g.fillFittedText (styledText, area.removeFromTop (20).to<float>());

        // Draw grid lines to help visualize positioning
        g.setStrokeColor (yup::Color (0x33ffffff));
        g.setStrokeWidth (1.0f);

        auto buttonBounds = targetButton.getBounds().to<float>();
        auto bounds = getLocalBounds().to<float>();

        // Horizontal lines through button center and edges
        g.strokeLine ({ 0, buttonBounds.getCenterY() }, { bounds.getWidth(), buttonBounds.getCenterY() });
        g.strokeLine ({ 0, buttonBounds.getY() }, { bounds.getWidth(), buttonBounds.getY() });
        g.strokeLine ({ 0, buttonBounds.getBottom() }, { bounds.getWidth(), buttonBounds.getBottom() });

        // Vertical lines through button center and edges
        g.strokeLine ({ buttonBounds.getCenterX(), 0 }, { buttonBounds.getCenterX(), bounds.getHeight() });
        g.strokeLine ({ buttonBounds.getX(), 0 }, { buttonBounds.getX(), bounds.getHeight() });
        g.strokeLine ({ buttonBounds.getRight(), 0 }, { buttonBounds.getRight(), bounds.getHeight() });
    }

    void keyDown (const yup::KeyPress& key, const yup::Point<float>& position) override
    {
        if (key.getKey() == yup::KeyPress::spaceKey || key.getKey() == yup::KeyPress::enterKey)
        {
            showPlacementTest();
        }
        else if (key.getKey() == yup::KeyPress::rightKey)
        {
            currentPlacementIndex = (currentPlacementIndex + 1) % static_cast<int>(placements.size());
            showPlacementTest();
        }
        else if (key.getKey() == yup::KeyPress::leftKey)
        {
            currentPlacementIndex = (currentPlacementIndex - 1 + static_cast<int>(placements.size())) % static_cast<int>(placements.size());
            showPlacementTest();
        }
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        if (event.isRightButtonDown())
        {
            showContextMenu (event.getPosition());
        }
    }

private:
    struct PlacementTest
    {
        yup::PopupMenu::Placement placement;
        yup::String description;

        PlacementTest (yup::PopupMenu::Placement p, const yup::String& desc)
            : placement (p), description (desc) {}
    };

    void initializePlacements()
    {
        using Side = yup::PopupMenu::Side;
        using J = yup::Justification;
        using Placement = yup::PopupMenu::Placement;

        placements.clear();

        // Below placements
        placements.emplace_back (Placement::below (J::topLeft), "Below - Left Aligned");
        placements.emplace_back (Placement::below (J::centerTop), "Below - Center Aligned");
        placements.emplace_back (Placement::below (J::topRight), "Below - Right Aligned");

        // Above placements
        placements.emplace_back (Placement::above (J::topLeft), "Above - Left Aligned");
        placements.emplace_back (Placement::above (J::centerTop), "Above - Center Aligned");
        placements.emplace_back (Placement::above (J::topRight), "Above - Right Aligned");

        // Right placements
        placements.emplace_back (Placement::toRight (J::topLeft), "Right - Top Aligned");
        placements.emplace_back (Placement::toRight (J::centerLeft), "Right - Center Aligned");
        placements.emplace_back (Placement::toRight (J::bottomLeft), "Right - Bottom Aligned");

        // Left placements
        placements.emplace_back (Placement::toLeft (J::topRight), "Left - Top Aligned");
        placements.emplace_back (Placement::toLeft (J::centerRight), "Left - Center Aligned");
        placements.emplace_back (Placement::toLeft (J::bottomRight), "Left - Bottom Aligned");

        // Centered
        placements.emplace_back (Placement::centered(), "Centered");

        // Additional interesting combinations
        placements.emplace_back (Placement::below (J::center), "Below - Center (any)");
        placements.emplace_back (Placement::above (J::center), "Above - Center (any)");
        placements.emplace_back (Placement::toRight (J::center), "Right - Center (any)");
        placements.emplace_back (Placement::toLeft (J::center), "Left - Center (any)");
    }

    void showPlacementTest()
    {
        if (placements.empty()) return;

        auto& test = placements[currentPlacementIndex];

        auto options = yup::PopupMenu::Options {}
                           .withParentComponent (this)
                           .withRelativePosition (&targetButton, test.placement);

        auto menu = yup::PopupMenu::create (options);

        // Add items to show menu content clearly
        menu->addItem ("Item 1", 1);
        menu->addItem ("Item 2", 2);
        menu->addItem ("Item 3", 3);
        menu->addSeparator();

        // Add a small submenu as well
        auto quickSubmenu = yup::PopupMenu::create();
        quickSubmenu->addItem ("Quick Action 1", 501);
        quickSubmenu->addItem ("Quick Action 2", 502);
        menu->addSubMenu ("More Actions", std::move (quickSubmenu));

        auto scrollableMenu = yup::PopupMenu::create();
        for (int i = 1; i <= 25; ++i)
        {
            scrollableMenu->addItem (yup::String::formatted ("Scroll Item %d", i), 400 + i);
            if (i % 5 == 0)
                scrollableMenu->addSeparator();
        }
        menu->addSubMenu ("Scrollable Menu", std::move (scrollableMenu));

        menu->addSeparator();

        menu->addItem ("Previous (<)", 998);
        menu->addItem ("Next (>)", 999);

        menu->show ([this, test] (int selectedID)
        {
            handlePlacementMenuSelection (selectedID, test);
        });

        // Update status
        auto statusText = yup::String::formatted ("Test %d/%d: %s",
                                                  currentPlacementIndex + 1,
                                                  (int)placements.size(),
                                                  test.description.toRawUTF8());
        statusLabel.setText (statusText);
    }

    void showContextMenu (yup::Point<float> position)
    {
        auto options = yup::PopupMenu::Options {}
                           .withPosition (localToScreen (position), yup::Justification::topLeft);

        auto contextMenu = yup::PopupMenu::create (options);

        contextMenu->addItem ("Reset to first test", 1);
        contextMenu->addItem ("Show all placements info", 2);
        contextMenu->addSeparator();

        // Add submenu example
        auto submenu = yup::PopupMenu::create();
        submenu->addItem ("Submenu Item 1", 201);
        submenu->addItem ("Submenu Item 2", 202);
        submenu->addSeparator();

        // Create nested submenu to demonstrate recursive submenus
        auto nestedSubmenu = yup::PopupMenu::create();
        nestedSubmenu->addItem ("Nested Item 1", 301);
        nestedSubmenu->addItem ("Nested Item 2", 302);
        nestedSubmenu->addItem ("Nested Item 3", 303);

        submenu->addSubMenu ("Nested Menu", std::move (nestedSubmenu));
        submenu->addItem ("Submenu Item 3", 203);

        contextMenu->addSubMenu ("Submenu Example", std::move (submenu));

        // Add scrollable menu example
        auto scrollableMenu = yup::PopupMenu::create();
        for (int i = 1; i <= 25; ++i)
        {
            scrollableMenu->addItem (yup::String::formatted ("Scroll Item %d", i), 400 + i);
            if (i % 5 == 0)
                scrollableMenu->addSeparator();
        }

        contextMenu->addSubMenu ("Scrollable Menu (25 items)", std::move (scrollableMenu));
        contextMenu->addItem ("Toggle grid lines", 3);

        contextMenu->show ([this] (int selectedID)
        {
            switch (selectedID)
            {
                case 1:
                    currentPlacementIndex = 0;
                    statusLabel.setText ("Reset to first placement test");
                    break;
                case 2:
                    showPlacementInfo();
                    break;
                case 3:
                    repaint(); // Grid lines are always shown in this demo
                    break;
                default:
                    if (selectedID >= 200)
                    {
                        auto text = yup::String::formatted ("Selected submenu item ID: %d", selectedID);
                        statusLabel.setText (text);
                    }
                    break;
            }
        });
    }

    void showPlacementInfo()
    {
        auto options = yup::PopupMenu::Options {}
                           .withParentComponent (this)
                           .withRelativePosition (&targetButton, yup::PopupMenu::Placement::centered());

        auto infoMenu = yup::PopupMenu::create (options);

        infoMenu->addItem (L"PopupMenu Features:", 0, false);
        infoMenu->addSeparator();
        infoMenu->addItem (L"• Placement: Side + Justification", 0, false);
        infoMenu->addItem (L"• Submenus: Hover to show", 0, false);
        infoMenu->addItem (L"• Scrolling: Mouse wheel support", 0, false);
        infoMenu->addSeparator();
        infoMenu->addItem (L"Controls:", 0, false);
        infoMenu->addItem (L"• Click button: Next test", 0, false);
        infoMenu->addItem (L"• ← →: Navigate tests", 0, false);
        infoMenu->addItem (L"• Right-click: Feature demo", 0, false);

        infoMenu->show ([this] (int selectedID) {
            // Info only, no actions
        });
    }

    void handlePlacementMenuSelection (int selectedID, const PlacementTest& test)
    {
        yup::String message;

        switch (selectedID)
        {
            case 998: // Previous
                currentPlacementIndex = (currentPlacementIndex - 1 + static_cast<int>(placements.size())) % static_cast<int>(placements.size());
                showPlacementTest();
                return;

            case 999: // Next
                currentPlacementIndex = (currentPlacementIndex + 1) % static_cast<int>(placements.size());
                showPlacementTest();
                return;

            case 1:
            case 2:
            case 3:
                message = yup::String::formatted ("Selected Item %d from: %s", selectedID, test.description.toRawUTF8());
                break;

            case 501:
            case 502:
                message = yup::String::formatted ("Selected submenu action %d from: %s", selectedID, test.description.toRawUTF8());
                break;

            default:
                message = "No selection";
                break;
        }

        statusLabel.setText (message);
    }

    yup::TextButton targetButton;
    yup::Label statusLabel;

    std::vector<PlacementTest> placements;
    int currentPlacementIndex;
};
