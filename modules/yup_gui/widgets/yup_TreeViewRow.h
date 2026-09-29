/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2026 - kunitoki@gmail.com

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

#pragma once

namespace yup
{

//==============================================================================
/**
    The component that shows one row of a TreeView.

    Rows are created and recycled by the TreeView: a row shows whatever item it was last given. It
    indents its content by the item's depth, hosts the disclosure button that opens and closes the
    item, and either hosts the item's custom content (see TreeViewItem::refreshItemComponent()) or
    has the theme paint the item's icon and text.

    It is public so themes and custom content can read its state. The theme paints it through the
    ComponentStyle registered for TreeViewRow.

    @see TreeView, TreeViewItem
*/
class YUP_API TreeViewRow : public Component
{
public:
    //==============================================================================
    /** Creates an empty row. */
    TreeViewRow();

    /** Destructor. */
    ~TreeViewRow() override;

    //==============================================================================
    /** Returns the item the row shows, or nullptr. */
    TreeViewItem* getItem() const noexcept;

    /** Returns the TreeView the row belongs to, or nullptr. */
    TreeView* getOwnerView() const noexcept;

    /** Returns the indentation level of the row: the item's depth, minus one when the root is hidden. */
    int getDepth() const noexcept;

    /** Returns true if the item is selected. */
    bool isItemSelected() const noexcept;

    /** Returns how open the item is, from 0 (closed) to 1 (open), in between while it animates. */
    float getOpenFraction() const;

    /** Returns the full height of the item.

        While a row is revealed or hidden by an animation, the row is shorter than its item and the
        content stays laid out at this height, anchored to the top.
    */
    float getItemHeight() const noexcept;

    /** Returns the text of the item, read when the row was last refreshed. */
    const String& getItemText() const noexcept;

    /** Returns true if the item shows an icon, read when the row was last refreshed. */
    bool hasItemIcon() const noexcept;

    /** Returns true if the item provides custom content, in which case the theme paints no icon nor text. */
    bool hasCustomContent() const noexcept;

    /** Returns the area of the disclosure button, empty when the item shows none. */
    Rectangle<float> getDisclosureBounds() const;

    /** Returns the area the built-in icon is painted in, empty when the item has no icon. */
    Rectangle<float> getIconBounds() const;

    /** Returns the area the built-in text is painted in. */
    Rectangle<float> getTextBounds() const;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;

private:
    friend class TreeView;
    class DisclosureButton;

    void update (TreeView& newOwnerView, TreeViewItem& newItem, int newDepth, bool shouldBeSelected);
    float getContentX() const;
    bool showsDisclosure() const;

    TreeView* ownerView = nullptr;
    TreeViewItem* item = nullptr;
    int depth = 0;
    bool selected = false;
    bool itemMightContainSubItems = false;
    float itemHeight = 0.0f;
    String itemText;
    bool itemHasIcon = false;
    std::unique_ptr<Component> content;
    std::unique_ptr<DisclosureButton> disclosureButton;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TreeViewRow)
};

} // namespace yup
