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
        , basicMenuButton ("basicMenuButton")
        , subMenuButton ("subMenuButton")
        , customMenuButton ("customMenuButton")
        , nativeMenuButton ("nativeMenuButton")
        , statusLabel ("statusLabel")
    {
        addAndMakeVisible (statusLabel);
        statusLabel.setTitle ("Right-click anywhere to show context menu");

        addAndMakeVisible (basicMenuButton);
        basicMenuButton.setButtonText ("Show Basic Menu");
        basicMenuButton.onClick = [this]
        {
            showBasicMenu();
        };

        addAndMakeVisible (subMenuButton);
        subMenuButton.setButtonText ("Show Sub-Menu");
        subMenuButton.onClick = [this]
        {
            showSubMenu();
        };

        addAndMakeVisible (customMenuButton);
        customMenuButton.setButtonText ("Show Custom Menu");
        customMenuButton.onClick = [this]
        {
            showCustomMenu();
        };

        addAndMakeVisible (nativeMenuButton);
        nativeMenuButton.setButtonText ("Show Native Menu");
        nativeMenuButton.onClick = [this]
        {
            showNativeMenu();
        };

        setSize ({ 400, 300 });
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (20);

        area.removeFromTop (20);
        statusLabel.setBounds (area.removeFromTop (30));

        basicMenuButton.setBounds (area.removeFromTop (40).reduced (0, 5));
        subMenuButton.setBounds (area.removeFromTop (40).reduced (0, 5));
        customMenuButton.setBounds (area.removeFromTop (40).reduced (0, 5));
        nativeMenuButton.setBounds (area.removeFromTop (40).reduced (0, 5));
    }

    void paint (yup::Graphics& g) override
    {
        auto area = getLocalBounds().reduced (5);

        auto styledText = yup::StyledText();
        {
            auto modifier = styledText.startUpdate();
            modifier.appendText ("PopupMenu Demo", yup::ApplicationTheme::getGlobalTheme()->getDefaultFont());
        }

        g.setFillColor (yup::Color (0xffffffff));
        g.fillFittedText (styledText, area.removeFromTop (20).to<float>());
    }

    void mouseDown (const yup::MouseEvent& event) override
    {
        if (event.isRightButtonDown())
        {
            showContextMenu (event.getPosition());
        }
    }

private:
    enum MenuItemIDs
    {
        newFile = 1,
        openFile,
        saveFile,
        saveAsFile,
        recentFile1,
        recentFile2,
        exitApp,

        checkedItem = 998,
        disabledItem = 999,

        editUndo = 10,
        editRedo,
        editCut,
        editCopy,
        editPaste,

        colorRed = 20,
        colorGreen,
        colorBlue,

        customSlider = 30,
        customButton = 31
    };

    void showBasicMenu()
    {
        auto options = yup::PopupMenu::Options {}
                           .withParentComponent (this)
                           .withRelativePosition (&basicMenuButton, yup::PopupMenu::Placement::below);

        auto menu = yup::PopupMenu::create (options);

        menu->addItem ("New File", newFile, true, false, "Cmd+N");
        menu->addItem ("Open File", openFile, true, false, "Cmd+O");
        menu->addSeparator();
        menu->addItem ("Save File", saveFile, true, false, "Cmd+S");
        menu->addItem ("Save As...", saveAsFile, true, false, "Shift+Cmd+S");
        menu->addSeparator();
        menu->addItem ("Disabled Item", disabledItem, false);
        menu->addItem ("Checked Item", checkedItem, true, isChecked);
        menu->addSeparator();
        menu->addItem ("Exit", exitApp, true, false, "Cmd+Q");

        menu->onItemSelected = [this] (int selectedID)
        {
            handleMenuSelection (selectedID);
        };

        menu->show();
    }

    void showSubMenu()
    {
        auto recentFilesMenu = yup::PopupMenu::create();
        recentFilesMenu->addItem ("Recent File 1.txt", recentFile1);
        recentFilesMenu->addItem ("Recent File 2.txt", recentFile2);

        auto colorMenu = yup::PopupMenu::create();
        colorMenu->addItem ("Red", colorRed);
        colorMenu->addItem ("Green", colorGreen);
        colorMenu->addItem ("Blue", colorBlue);

        auto options = yup::PopupMenu::Options {}
                           .withParentComponent (this)
                           .withRelativePosition (&subMenuButton, yup::PopupMenu::Placement::toRight);
        auto menu = yup::PopupMenu::create (options);
        menu->addItem ("New", newFile);
        menu->addItem ("Open", openFile);
        menu->addSubMenu ("Recent Files", recentFilesMenu);
        menu->addSeparator();
        menu->addSubMenu ("Colors", colorMenu);
        menu->addSeparator();
        menu->addItem ("Exit", exitApp);

        menu->show([this] (int selectedID)
        {
            handleMenuSelection (selectedID);
        });
    }

    void showCustomMenu()
    {
        auto options = yup::PopupMenu::Options {}
                           .withParentComponent (this)
                           .withRelativePosition (&customMenuButton, yup::PopupMenu::Placement::above);
        auto menu = yup::PopupMenu::create (options);

        menu->addItem ("Regular Item", 1);
        menu->addSeparator();

        // Add custom slider component
        auto slider = std::make_unique<yup::Slider> ("CustomSlider");
        slider->setSize ({ 250, 250 });
        slider->setValue (0.5);
        menu->addCustomItem (std::move (slider), customSlider);

        menu->addSeparator();

        // Add custom button component
        auto button = std::make_unique<yup::TextButton> ("CustomButton");
        button->setSize ({ 120, 30 });
        button->setTitle ("Custom Button");
        button->onClick = []
        {
            YUP_DBG ("Clicked!");
        };
        menu->addCustomItem (std::move (button), customButton);

        menu->addSeparator();
        menu->addItem ("Another Item", 2);

        menu->onItemSelected = [this] (int selectedID)
        {
            handleMenuSelection (selectedID);
        };

        menu->show();
    }

    void showNativeMenu()
    {
        auto options = yup::PopupMenu::Options {}
                           .withParentComponent (this)
                           .withRelativePosition (&nativeMenuButton, yup::PopupMenu::Placement::centered);

        auto menu = yup::PopupMenu::create (options);

        menu->addItem ("Native Item 1", 1);
        menu->addItem ("Native Item 2", 2);
        menu->addItem ("Native Item 3", 3);

        menu->onItemSelected = [this] (int selectedID)
        {
            handleMenuSelection (selectedID);
        };

        menu->show();
    }

    void showContextMenu (yup::Point<float> position)
    {
        auto options = yup::PopupMenu::Options {}
                           .withParentComponent (this)
                           .withPosition (position, yup::Justification::topLeft);

        auto contextMenu = yup::PopupMenu::create (options);

        contextMenu->addItem ("Context Item 1", 1);
        contextMenu->addItem ("Context Item 2", 2);
        contextMenu->addSeparator();
        contextMenu->addItem ("Context Item 3", 3);


        contextMenu->show ([this] (int selectedID)
        {
            handleMenuSelection (selectedID);
        });
    }

    void handleMenuSelection (int selectedID)
    {
        yup::String message = "Selected item ID: " + yup::String (selectedID);

        switch (selectedID)
        {
            case newFile:
                message = "New File selected";
                break;

            case openFile:
                message = "Open File selected";
                break;

            case saveFile:
                message = "Save File selected";
                break;

            case saveAsFile:
                message = "Save As selected";
                break;

            case exitApp:
                message = "Exit selected";
                break;

            case editCopy:
                message = "Copy selected";
                break;

            case editPaste:
                message = "Paste selected";
                break;

            case colorRed:
                message = "Red color selected";
                break;

            case colorGreen:
                message = "Green color selected";
                break;

            case colorBlue:
                message = "Blue color selected";
                break;

            case customSlider:
                message = "Custom slider interacted";
                break;

            case customButton:
                message = "Custom button clicked";
                break;

            case disabledItem:
                message = "I'm disabled!";
                break;

            case checkedItem:
                message = "I'm checked!";
                isChecked = ! isChecked;
                break;

            default:
                message = "Cancelled or unknown!";
                break;
        }

        statusLabel.setText (message);
    }

    yup::TextButton basicMenuButton;
    yup::TextButton subMenuButton;
    yup::TextButton customMenuButton;
    yup::TextButton nativeMenuButton;
    yup::Label statusLabel;
    bool isChecked = true;
};
