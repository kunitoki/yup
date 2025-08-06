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
/** A combo box for selecting items from a dropdown list.

    The ComboBox presents a list of text strings from which the user can pick one.
    It provides methods for adding, removing, and managing the list items, as well
    as responding to selection changes.

    @see Component, PopupMenu
*/
class YUP_API ComboBox : public Component
{
public:
    //==============================================================================
    /** Creates a ComboBox.

        @param componentID    The component identifier for this combo box
    */
    ComboBox (StringRef componentID = {});

    //==============================================================================
    /** Destructor. */
    ~ComboBox() override;

    //==============================================================================
    /** Adds an item to the drop-down list.

        @param newItemText     The text to show for this item
        @param newItemId       An ID number that can be used to identify this item
    */
    void addItem (String newItemText, int newItemId);

    /** Adds an array of items to the drop-down list.

        @param itemsToAdd      The list of strings to add
        @param firstItemId     The ID to use for the first item
    */
    void addItemList (const StringArray& itemsToAdd, int firstItemId);

    /** Adds a separator line to the drop-down list. */
    void addSeparator();

    /** Removes all items from the list. */
    void clear();

    //==============================================================================
    /** Returns the number of items in the list. */
    int getNumItems() const noexcept;

    /** Returns the text of one of the items.

        @param index    The index of the item (starting from 0)
        @returns        The item's text, or an empty string if the index is out of range
    */
    String getItemText (int index) const;

    /** Returns the ID of one of the items.

        @param index    The index of the item (starting from 0)
        @returns        The item's ID, or 0 if the index is out of range
    */
    int getItemId (int index) const;

    /** Changes the text for an existing item.

        @param index       The index of the item to change
        @param newText     The new text for this item
    */
    void changeItemText (int index, String newText);

    //==============================================================================
    /** Returns the index of the currently selected item.

        @returns    The index of the selected item, or -1 if nothing is selected
    */
    int getSelectedItemIndex() const noexcept;

    /** Returns the ID of the currently selected item.

        @returns    The ID of the selected item, or 0 if nothing is selected
    */
    int getSelectedId() const noexcept;

    /** Returns the text of the currently selected item.

        @returns    The text of the selected item, or an empty string if nothing is selected
    */
    String getText() const noexcept;

    /** Selects one of the items.

        @param newItemIndex      The index of the item to select
        @param notification      Whether to send a change notification
    */
    void setSelectedItemIndex (int newItemIndex, NotificationType notification = sendNotification);

    /** Selects an item with the given ID.

        @param newItemId         The ID of the item to select
        @param notification      Whether to send a change notification
    */
    void setSelectedId (int newItemId, NotificationType notification = sendNotification);

    /** Sets the text to show when no item is selected.

        @param newPlaceholderText    The placeholder text to display
    */
    void setTextWhenNothingSelected (String newPlaceholderText);

    /** Returns the current placeholder text. */
    String getTextWhenNothingSelected() const noexcept { return textWhenNothingSelected; }

    //==============================================================================
    /** Sets whether the text can be edited.

        @param isEditable    True to allow text editing
    */
    void setEditableText (bool isEditable);

    /** Returns true if the text can be edited. */
    bool isTextEditable() const noexcept { return textIsEditable; }

    //==============================================================================
    /** Called when the selected item changes.

        Override this to respond to selection changes.
    */
    virtual void selectedItemChanged() {}

    std::function<void()> onSelectedItemChanged;

    //==============================================================================
    struct Style
    {
        static const Identifier backgroundColorId;
        static const Identifier textColorId;
        static const Identifier borderColorId;
        static const Identifier arrowColorId;
        static const Identifier focusedBorderColorId;
    };

    /** */
    bool isPopupShown() const;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void focusGained() override;
    /** @internal */
    void focusLost() override;

    /** @internal */
    StyledText& getStyledText() const noexcept { return const_cast<StyledText&> (styledText); }

private:
    struct ComboBoxItem
    {
        String text;
        int itemId;
        bool isSeparator;
    };

    void showPopup();
    void hidePopup();
    void updateDisplayText();

    Array<ComboBoxItem> items;
    int selectedItemId = 0;
    String textWhenNothingSelected;
    String displayText;
    StyledText styledText;
    PopupMenu::Ptr popupMenu;
    bool textIsEditable = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComboBox)
};

} // namespace yup
