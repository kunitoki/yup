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

    selectedIndex = -1;

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

int ComboBox::getSelectedId() const noexcept
{
    return isPositiveAndBelow (selectedIndex, items.size()) ? items[selectedIndex].itemId : 0;
}

String ComboBox::getText() const noexcept
{
    return isPositiveAndBelow (selectedIndex, items.size()) ? items[selectedIndex].text : String();
}

void ComboBox::setSelectedItemIndex (int newItemIndex, NotificationType notification)
{
    if (selectedIndex != newItemIndex)
    {
        selectedIndex = newItemIndex;
        updateDisplayText();

        if (notification != dontSendNotification)
            comboBoxChanged();

        repaint();
    }
}

void ComboBox::setSelectedId (int newItemId, NotificationType notification)
{
    for (int i = 0; i < items.size(); ++i)
    {
        if (items[i].itemId == newItemId && ! items[i].isSeparator)
        {
            setSelectedItemIndex (i, notification);
            return;
        }
    }

    setSelectedItemIndex (-1, notification);
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
    auto bounds = getLocalBounds();

    // Draw background
    auto bgColor = findColor (Style::backgroundColorId).value_or (Color (0xffffffff));
    g.setFillColor (bgColor);
    g.fillRoundedRect (bounds, 4.0f);

    // Draw border
    auto borderColor = hasFocus
                         ? findColor (Style::focusedBorderColorId).value_or (Color (0xff4a90e2))
                         : findColor (Style::borderColorId).value_or (Color (0xffcccccc));

    g.setStrokeColor (borderColor);
    g.setStrokeWidth (hasFocus ? 2.0f : 1.0f);
    g.strokeRoundedRect (bounds.reduced (0.5f), 4.0f);

    // Calculate text and arrow areas
    auto arrowWidth = 20.0f;
    auto textBounds = bounds.reduced (8.0f, 4.0f);
    textBounds.removeFromRight (arrowWidth);

    auto arrowBounds = bounds.reduced (4.0f);
    arrowBounds.removeFromLeft (bounds.getWidth() - arrowWidth);

    // Draw text
    if (displayText.isNotEmpty())
    {
        auto textColor = findColor (Style::textColorId).value_or (Color (0xff333333));
        g.setFillColor (textColor);
        g.fillFittedText (styledText, textBounds);
    }

    // Draw arrow
    drawArrow (g, arrowBounds);
}

//==============================================================================

void ComboBox::resized()
{
    updateDisplayText();
}

//==============================================================================

void ComboBox::mouseDown (const MouseEvent& event)
{
    if (! isPopupShown)
    {
        showPopup();
    }
}

//==============================================================================

void ComboBox::focusGained()
{
    hasFocus = true;

    repaint();
}

void ComboBox::focusLost()
{
    hasFocus = false;

    hidePopup();

    repaint();
}

//==============================================================================

void ComboBox::showPopup()
{
    // This is a simplified popup implementation
    // In a full implementation, you would create a popup window with the items
    isPopupShown = true;

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
        setSelectedId (selectedItemID);

        isPopupShown = false;
    });
}

void ComboBox::hidePopup()
{
    isPopupShown = false;

    if (popupMenu != nullptr)
        popupMenu->dismiss();
}

//==============================================================================

void ComboBox::updateDisplayText()
{
    auto bounds = getLocalBounds();
    auto textBounds = bounds.reduced (8.0f, 4.0f);
    textBounds.removeFromRight (20.0f); // Arrow width

    if (isPositiveAndBelow (selectedIndex, items.size()))
    {
        displayText = items[selectedIndex].text;
    }
    else
    {
        displayText = textWhenNothingSelected;
    }

    auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont();

    {
        auto modifier = styledText.startUpdate();
        modifier.setMaxSize (textBounds.getSize());
        modifier.setHorizontalAlign (StyledText::left);
        modifier.setVerticalAlign (StyledText::middle);
        modifier.clear();

        if (displayText.isNotEmpty())
            modifier.appendText (displayText, font, 16.0f);
    }

    repaint();
}

void ComboBox::drawArrow (Graphics& g, Rectangle<float> arrowBounds)
{
    auto arrowColor = Color (0xff666666);
    g.setFillColor (arrowColor);

    auto center = arrowBounds.getCenter();
    auto arrowSize = 4.0f;

    // Draw simple triangle using lines instead of Path
    g.setStrokeColor (arrowColor);
    g.setStrokeWidth (2.0f);

    // Draw downward arrow as lines
    g.strokeLine (center.getX() - arrowSize, center.getY() - arrowSize * 0.5f, center.getX(), center.getY() + arrowSize * 0.5f);
    g.strokeLine (center.getX() + arrowSize, center.getY() - arrowSize * 0.5f, center.getX(), center.getY() + arrowSize * 0.5f);
}

} // namespace yup
