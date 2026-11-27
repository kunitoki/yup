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

namespace
{

struct ComboBoxIds
{
    static inline const Identifier items { "items" };
    static inline const Identifier selectedId { "selectedId" };
    static inline const Identifier selectedIndex { "selectedIndex" };
    static inline const Identifier text { "text" };
    static inline const Identifier open { "open" };
};

} // namespace

//==============================================================================

const Identifier ComboBox::Style::backgroundColorId = "comboBoxBackground";
const Identifier ComboBox::Style::textColorId = "comboBoxText";
const Identifier ComboBox::Style::borderColorId = "comboBoxBorder";
const Identifier ComboBox::Style::arrowColorId = "comboBoxArrow";
const Identifier ComboBox::Style::focusedBorderColorId = "comboBoxFocusedBorder";
const Identifier ComboBox::Style::cornerRadiusId = "comboBoxCornerRadius";
const Identifier ComboBox::Style::outlineWidthId = "comboBoxOutlineWidth";
const Identifier ComboBox::Style::focusedOutlineWidthId = "comboBoxFocusedOutlineWidth";
const Identifier ComboBox::Style::textPartId = "text";

//==============================================================================

const ComponentTypeInfo& ComboBox::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<ComboBox> (Identifier ("ComboBox"))
                                 .color ("backgroundColor", Style::backgroundColorId)
                                 .color ("textColor", Style::textColorId)
                                 .color ("borderColor", Style::borderColorId)
                                 .color ("arrowColor", Style::arrowColorId)
                                 .color ("focusedBorderColor", Style::focusedBorderColorId)
                                 .number ("cornerRadius", Style::cornerRadiusId, 4.0, "The corner radius.")
                                 .number ("outlineWidth", Style::outlineWidthId, 1.0, "The outline width.")
                                 .number ("focusedOutlineWidth", Style::focusedOutlineWidthId, 2.0, "The outline width when focused.")
                                 .property (ComboBoxIds::items, &ComboBox::getItems, [] (ComboBox& c, const StringArray& v) { c.setItems (v); }, {}, "The texts of the items, numbered from 1 in order.")
                                 .property (ComboBoxIds::selectedId, &ComboBox::getSelectedId, [] (ComboBox& c, int v) { c.setSelectedId (v); })
                                 .property (ComboBoxIds::selectedIndex, &ComboBox::getSelectedItemIndex, [] (ComboBox& c, int v) { c.setSelectedItemIndex (v); })
                                 .readOnly (ComboBoxIds::text, &ComboBox::getText)
                                 .state (ComboBoxIds::open, &ComboBox::isPopupShown)
                                 .readOnly ("textBounds", &ComboBox::getTextBounds, {}, "The area the text of the selected item is laid out in.")
                                 .readOnly ("arrowBounds", &ComboBox::getArrowBounds, {}, "The area of the arrow at the end of the box.")
                                 .signal ("onSelectedItemChanged", &ComboBox::onSelectedItemChanged, "Emitted when another item is selected")
                                 .part (Style::textPartId, &ComboBox::getTextBounds, "The text of the selected item, filled with the fill color in the area it is given: the one of textBounds when it is given none.")
                                 .build();

    return info;
}

//==============================================================================

ComboBox::ComboBox()
    : ComboBox (StringRef {})
{
}

ComboBox::ComboBox (StringRef componentID)
    : Component (componentID)
{
    setOpaque (false);
    setWantsKeyboardFocus (true);
    setMouseCursor (MouseCursor::Hand);
}

ComboBox::~ComboBox() = default;

//==============================================================================

void ComboBox::addItem (String newItemText, int newItemId)
{
    const auto previousText = getText();
    const auto previousIndex = getSelectedItemIndex();

    items.add ({ newItemText, newItemId, false });
    updateDisplayText();

    const BailOutChecker bailOutChecker (this);

    notifyPropertyChanged (ComboBoxIds::items);
    if (bailOutChecker.shouldBailOut())
        return;

    if (getSelectedItemIndex() != previousIndex)
    {
        notifyPropertyChanged (ComboBoxIds::selectedIndex);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (getText() != previousText)
        notifyPropertyChanged (ComboBoxIds::text);
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

void ComboBox::setItems (const StringArray& newItems, NotificationType notification)
{
    const auto previousId = selectedItemId;
    const auto previousText = getText();

    items.clear();

    for (int i = 0; i < newItems.size(); ++i)
        items.add ({ newItems[i], i + 1, false });

    if (previousId > newItems.size())
        selectedItemId = 0;

    updateDisplayText();

    const BailOutChecker bailOutChecker (this);

    notifyPropertyChanged (ComboBoxIds::items);
    if (bailOutChecker.shouldBailOut())
        return;

    if (selectedItemId != previousId)
        sendSelectionChanged (notification);
    else if (getText() != previousText)
        notifyPropertyChanged (ComboBoxIds::text);

    if (! bailOutChecker.shouldBailOut())
        repaint();
}

StringArray ComboBox::getItems() const
{
    StringArray result;

    for (const auto& item : items)
        if (! item.isSeparator)
            result.add (item.text);

    return result;
}

//==============================================================================

void ComboBox::clear()
{
    const bool hadItems = ! getItems().isEmpty();
    const auto previousId = selectedItemId;
    const auto previousIndex = getSelectedItemIndex();
    const auto previousText = getText();

    items.clear();
    selectedItemId = 0;
    updateDisplayText();

    const BailOutChecker bailOutChecker (this);

    if (hadItems)
    {
        notifyPropertyChanged (ComboBoxIds::items);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (selectedItemId != previousId)
    {
        notifyPropertyChanged (ComboBoxIds::selectedId);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (getSelectedItemIndex() != previousIndex)
    {
        notifyPropertyChanged (ComboBoxIds::selectedIndex);
        if (bailOutChecker.shouldBailOut())
            return;
    }

    if (getText() != previousText)
        notifyPropertyChanged (ComboBoxIds::text);
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
    if (isPositiveAndBelow (index, items.size()) && items.getReference (index).text != newText)
    {
        const auto previousText = getText();

        items.getReference (index).text = newText;
        updateDisplayText();

        const BailOutChecker bailOutChecker (this);

        if (! items.getReference (index).isSeparator)
        {
            notifyPropertyChanged (ComboBoxIds::items);
            if (bailOutChecker.shouldBailOut())
                return;
        }

        if (getText() != previousText)
            notifyPropertyChanged (ComboBoxIds::text);
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

            sendSelectionChanged (notification);

            repaint();
        }
    }
}

void ComboBox::sendSelectionChanged (NotificationType notification)
{
    const BailOutChecker bailOutChecker (this);

    notifyPropertyChanged (ComboBoxIds::selectedId);
    if (bailOutChecker.shouldBailOut())
        return;

    notifyPropertyChanged (ComboBoxIds::selectedIndex);
    if (bailOutChecker.shouldBailOut())
        return;

    notifyPropertyChanged (ComboBoxIds::text);
    if (bailOutChecker.shouldBailOut())
        return;

    sendChangeNotification (notification, [this]
    {
        selectedItemChanged();

        if (onSelectedItemChanged)
            onSelectedItemChanged();
    });
}

void ComboBox::setTextWhenNothingSelected (String newPlaceholderText)
{
    if (textWhenNothingSelected != newPlaceholderText)
    {
        const auto previousText = getText();
        textWhenNothingSelected = newPlaceholderText;

        updateDisplayText();

        if (getText() != previousText)
            notifyPropertyChanged (ComboBoxIds::text);
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

Rectangle<float> ComboBox::getTextBounds() const
{
    auto textBounds = getLocalBounds().reduced (8.0f, 4.0f);
    textBounds.removeFromRight (arrowWidth);

    return textBounds;
}

Rectangle<float> ComboBox::getArrowBounds() const
{
    const auto bounds = getLocalBounds();

    auto arrowBounds = bounds.reduced (4.0f);
    arrowBounds.removeFromLeft (bounds.getWidth() - arrowWidth);

    return arrowBounds;
}

//==============================================================================

void ComboBox::mouseDown (const MouseEvent& event)
{
    if (getWantsKeyboardFocus())
        takeKeyboardFocus();

    if (ignoreMouseDownAfterPopupDismissal)
    {
        ignoreMouseDownAfterPopupDismissal = false;
        popupMenu = nullptr;
        repaint();
        return;
    }

    if (popupMenu == nullptr || ! popupMenu->isBeingShown())
    {
        showPopup();
    }
    else
    {
        const auto dismissingFromThisMouseDown = ScopedValueSetter<bool> (dismissingPopupFromMouseDown, true);
        hidePopup();
    }

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
                                       .withParentComponent (getPopupParentComponent())
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

    WeakReference<Component> self = this;
    auto wasOpened = std::make_shared<bool> (false);

    popupMenu->show ([this, self, wasOpened] (int selectedItemID)
    {
        if (self.get() == nullptr)
            return;

        if (*wasOpened)
        {
            notifyPropertyChanged (ComboBoxIds::open);
            if (self.get() == nullptr)
                return;
        }

        if (selectedItemID != 0)
        {
            setSelectedId (selectedItemID);
        }
        else if (! dismissingPopupFromMouseDown)
        {
            ignoreMouseDownAfterPopupDismissal = true;

            MessageManager::callAsync ([this, self]
            {
                if (self.get() != nullptr)
                    ignoreMouseDownAfterPopupDismissal = false;
            });
        }

        if (self.get() != nullptr && getWantsKeyboardFocus())
            takeKeyboardFocus();
    });

    if (self.get() != nullptr && isPopupShown())
    {
        *wasOpened = true;
        notifyPropertyChanged (ComboBoxIds::open);
    }
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
    const auto textBounds = getTextBounds();

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
        modifier.setOverflow (StyledText::ellipsis);
        modifier.setWrap (StyledText::noWrap);
        modifier.clear();

        if (displayText.isNotEmpty())
            modifier.appendText (displayText, font.withHeight (jmin (font.getHeight(), getHeight() * 0.6f)));
    }

    repaint();
}

} // namespace yup
