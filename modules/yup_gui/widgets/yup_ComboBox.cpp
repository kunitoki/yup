/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

const Identifier ComboBox::Style::backgroundColorId = "comboBoxBackground";
const Identifier ComboBox::Style::textColorId = "comboBoxText";
const Identifier ComboBox::Style::borderColorId = "comboBoxBorder";
const Identifier ComboBox::Style::arrowColorId = "comboBoxArrow";
const Identifier ComboBox::Style::focusedBorderColorId = "comboBoxFocusedBorder";

//==============================================================================

ComboBox::ComboBox (StringRef componentID)
    : Component (componentID)
{
    setWantsKeyboardFocus (true);
}

ComboBox::~ComboBox() = default;

//==============================================================================

void ComboBox::addItem (String newItemText, int newItemId)
{
    items.add ({ newItemText, newItemId, false });
    updateDisplayText();
}

void ComboBox::addItemList (const StringArray& itemsToAdd, int firstItemId)
{
    for (int i = 0; i < itemsToAdd.size(); ++i)
        addItem (itemsToAdd[i], firstItemId + i);
}

void ComboBox::addSeparator()
{
    items.add ({ {}, 0, true });
}

//==============================================================================

void ComboBox::clear()
{
    items.clear();

    selectedItemId = 0;

    updateDisplayText();
}

//==============================================================================

int ComboBox::getNumItems() const noexcept
{
    return items.size();
}

String ComboBox::getItemText (int index) const
{
    return isPositiveAndBelow (index, items.size()) ? items[index].text : String();
}

int ComboBox::getItemId (int index) const
{
    return isPositiveAndBelow (index, items.size()) ? items[index].itemId : 0;
}

void ComboBox::changeItemText (int index, String newText)
{
    if (isPositiveAndBelow (index, items.size()))
    {
        items.getReference (index).text = newText;
        updateDisplayText();
    }
}

//==============================================================================

int ComboBox::getSelectedItemIndex() const noexcept
{
    for (int i = 0; i < items.size(); ++i)
    {
        if (items[i].itemId == selectedItemId && ! items[i].isSeparator)
            return i;
    }
    return -1;
}

int ComboBox::getSelectedId() const noexcept
{
    return selectedItemId;
}

String ComboBox::getText() const noexcept
{
    for (const auto& item : items)
    {
        if (item.itemId == selectedItemId && ! item.isSeparator)
            return item.text;
    }

    return textWhenNothingSelected;
}

void ComboBox::setSelectedItemIndex (int newItemIndex, NotificationType notification)
{
    int newItemId = 0;
    if (isPositiveAndBelow (newItemIndex, items.size()) && ! items[newItemIndex].isSeparator)
        newItemId = items[newItemIndex].itemId;

    setSelectedId (newItemId, notification);
}

void ComboBox::setSelectedId (int newItemId, NotificationType notification)
{
    if (selectedItemId != newItemId)
    {
        // Validate that the item ID exists (unless it's 0 which means no selection)
        bool isValidId = (newItemId == 0);
        if (newItemId != 0)
        {
            for (const auto& item : items)
            {
                if (item.itemId == newItemId && ! item.isSeparator)
                {
                    isValidId = true;
                    break;
                }
            }
        }

        if (isValidId)
        {
            selectedItemId = newItemId;
            updateDisplayText();

            if (notification != dontSendNotification)
                comboBoxChanged();

            repaint();
        }
    }
}

void ComboBox::setTextWhenNothingSelected (String newPlaceholderText)
{
    if (textWhenNothingSelected != newPlaceholderText)
    {
        textWhenNothingSelected = newPlaceholderText;

        updateDisplayText();
    }
}

//==============================================================================

void ComboBox::setEditableText (bool isEditable)
{
    textIsEditable = isEditable;
}

//==============================================================================

void ComboBox::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

//==============================================================================

void ComboBox::resized()
{
    updateDisplayText();
}

//==============================================================================

void ComboBox::mouseDown (const MouseEvent& event)
{
    takeKeyboardFocus();

    if (popupMenu == nullptr || ! popupMenu->isBeingShown())
        showPopup();
    else
        hidePopup();

    repaint();
}

//==============================================================================

void ComboBox::focusGained()
{
    repaint();
}

void ComboBox::focusLost()
{
    repaint();
}

//==============================================================================

void ComboBox::showPopup()
{
    popupMenu = PopupMenu::create (PopupMenu::Options {}
                                       .withParentComponent (getTopLevelComponent())
                                       .withMinimumWidth (getWidth())
                                       .withRelativePosition (this));

    auto selectedItemID = getSelectedId();

    for (const auto& item : items)
    {
        const bool isTicked = item.itemId == selectedItemID;

        if (item.isSeparator)
            popupMenu->addSeparator();
        else
            popupMenu->addItem (item.text, item.itemId, true, isTicked);
    }

    popupMenu->show ([this] (int selectedItemID)
    {
        if (selectedItemID != 0)
            setSelectedId (selectedItemID);

         takeKeyboardFocus();
    });
}

void ComboBox::hidePopup()
{
    if (isPopupShown())
        popupMenu->dismiss();
}

bool ComboBox::isPopupShown() const
{
    return popupMenu != nullptr && popupMenu->isBeingShown();
}

//==============================================================================

void ComboBox::updateDisplayText()
{
    auto bounds = getLocalBounds();
    auto textBounds = bounds.reduced (8.0f, 4.0f);
    textBounds.removeFromRight (20.0f); // Arrow width

    bool foundSelectedItem = false;
    for (const auto& item : items)
    {
        if (item.itemId == selectedItemId && ! item.isSeparator)
        {
            displayText = item.text;
            foundSelectedItem = true;
            break;
        }
    }

    if (! foundSelectedItem)
        displayText = textWhenNothingSelected;

    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();

    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (textBounds.getSize());
        modifier.setHorizontalAlign (StyledText::left);
        modifier.setVerticalAlign (StyledText::middle);
        modifier.clear();

        if (displayText.isNotEmpty())
            modifier.appendText (displayText, font, getHeight() * 0.35f);
    }

    repaint();
}

} // namespace yup
