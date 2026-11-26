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

namespace yup
{

namespace
{

struct TreeViewRowIds
{
    static inline const Identifier depth { "depth" };
    static inline const Identifier selected { "selected" };
    static inline const Identifier hovered { "hovered" };
    static inline const Identifier openFraction { "openFraction" };
    static inline const Identifier itemHeight { "itemHeight" };
    static inline const Identifier text { "text" };
    static inline const Identifier hasIcon { "hasIcon" };
    static inline const Identifier hasCustomContent { "hasCustomContent" };
    static inline const Identifier disclosureBounds { "disclosureBounds" };
    static inline const Identifier iconBounds { "iconBounds" };
    static inline const Identifier textBounds { "textBounds" };
    static inline const Identifier indentSize { "indentSize" };
    static inline const Identifier indentGuidesVisible { "indentGuidesVisible" };
    static inline const Identifier icon { "icon" };
};

} // namespace

//==============================================================================
const ComponentTypeInfo& TreeViewRow::getClassTypeInfo()
{
    static const auto info = ComponentTypeBuilder<TreeViewRow> (Identifier ("TreeViewRow"))
                                 .readOnly (TreeViewRowIds::depth, &TreeViewRow::getDepth, 0, "The indentation depth of the row.")
                                 .state (TreeViewRowIds::selected, &TreeViewRow::isItemSelected, false, "Whether the row's item is selected.")
                                 .state (TreeViewRowIds::hovered, &TreeViewRow::isItemHovered, false, "Whether the mouse pointer is over the row's item.")
                                 .readOnly (TreeViewRowIds::openFraction, &TreeViewRow::getOpenFraction, 0.0, "How open the row's item is, from 0 to 1.")
                                 .readOnly (TreeViewRowIds::itemHeight, &TreeViewRow::getItemHeight, 0.0, "The full unclipped height of the row's item.")
                                 .readOnly (TreeViewRowIds::text, &TreeViewRow::getItemText, String(), "The text displayed by the row's item.")
                                 .readOnly (TreeViewRowIds::hasIcon, &TreeViewRow::hasItemIcon, false, "Whether the row's item has an icon, which the part 'icon' paints.")
                                 .readOnly (TreeViewRowIds::hasCustomContent, &TreeViewRow::hasCustomContent, false, "Whether the row's item shows a component of its own in place of its icon and text.")
                                 .readOnly (TreeViewRowIds::disclosureBounds, &TreeViewRow::getDisclosureBounds, {}, "The area of the button opening the row's item, empty for an item that can't be opened.")
                                 .readOnly (TreeViewRowIds::iconBounds, &TreeViewRow::getIconBounds, {}, "The area of the icon of the row's item.")
                                 .readOnly (TreeViewRowIds::textBounds, &TreeViewRow::getTextBounds, {}, "The area of the text of the row's item.")
                                 .readOnly (TreeViewRowIds::indentSize, [] (const TreeViewRow& r) { return r.getOwnerView() != nullptr ? r.getOwnerView()->getIndentSize() : 0.0f; }, 0.0, "How far the tree indents each level of depth.")
                                 .readOnly (TreeViewRowIds::indentGuidesVisible, [] (const TreeViewRow& r) { return r.getOwnerView() != nullptr && r.getOwnerView()->areIndentGuidesVisible(); }, false, "Whether the tree shows a guide for each level of depth.")
                                 .part (TreeViewRowIds::icon, &TreeViewRow::getIconBounds, "The icon of the row's item, as the item paints it.")
                                 .build();

    return info;
}

//==============================================================================
/** The transparent area over the disclosure glyph: it takes presses, so toggling does not select the row. */
class TreeViewRow::DisclosureButton final : public Component
{
public:
    explicit DisclosureButton (TreeViewRow& owner)
        : owner (owner)
    {
        setOpaque (false);
    }

    void mouseDown (const MouseEvent& event) override
    {
        auto* item = owner.item;

        if (item == nullptr)
            return;

        if (event.getModifiers().isAltDown())
            item->setOpenRecursively (! item->isOpen());
        else
            item->setOpen (! item->isOpen());
    }

private:
    TreeViewRow& owner;
};

//==============================================================================
TreeViewRow::TreeViewRow()
{
    setOpaque (false);
    setWantsMouseEvents (false, true);

    disclosureButton = std::make_unique<DisclosureButton> (*this);
    addChildComponent (*disclosureButton);
}

TreeViewRow::~TreeViewRow() = default;

//==============================================================================
TreeViewItem* TreeViewRow::getItem() const noexcept
{
    return item;
}

TreeView* TreeViewRow::getOwnerView() const noexcept
{
    return ownerView;
}

int TreeViewRow::getDepth() const noexcept
{
    return depth;
}

bool TreeViewRow::isItemSelected() const noexcept
{
    return selected;
}

bool TreeViewRow::isItemHovered() const noexcept
{
    return ownerView != nullptr && item != nullptr && ownerView->getHoveredItem() == item;
}

float TreeViewRow::getOpenFraction() const
{
    if (ownerView == nullptr || item == nullptr)
        return 0.0f;

    return ownerView->getOpenFraction (*item);
}

float TreeViewRow::getItemHeight() const noexcept
{
    return itemHeight;
}

const String& TreeViewRow::getItemText() const noexcept
{
    return itemText;
}

bool TreeViewRow::hasItemIcon() const noexcept
{
    return itemHasIcon;
}

bool TreeViewRow::hasCustomContent() const noexcept
{
    return content != nullptr;
}

Rectangle<float> TreeViewRow::getDisclosureBounds() const
{
    if (! showsDisclosure())
        return {};

    const auto indentSize = ownerView->getIndentSize();
    return { static_cast<float> (depth) * indentSize, 0.0f, indentSize, itemHeight };
}

Rectangle<float> TreeViewRow::getIconBounds() const
{
    if (! itemHasIcon)
        return {};

    const auto size = itemHeight * 0.7f;
    return { getContentX() + 2.0f, (itemHeight - size) * 0.5f, size, size };
}

Rectangle<float> TreeViewRow::getTextBounds() const
{
    auto x = getContentX() + 4.0f;

    if (const auto iconBounds = getIconBounds(); ! iconBounds.isEmpty())
        x = iconBounds.getRight() + 4.0f;

    return { x, 0.0f, jmax (0.0f, getWidth() - x - 4.0f), itemHeight };
}

//==============================================================================
void TreeViewRow::paint (Graphics& g)
{
    if (auto style = ApplicationTheme::findComponentStyle (*this))
        style->paint (g, *ApplicationTheme::getGlobalTheme(), *this);
}

void TreeViewRow::resized()
{
    const auto disclosureBounds = getDisclosureBounds();
    disclosureButton->setBounds (disclosureBounds);
    disclosureButton->setVisible (! disclosureBounds.isEmpty());

    if (content != nullptr)
    {
        // Laid out at the item's full height, so a row shrunk by an animation clips its content instead of squashing it.
        const auto contentX = getContentX();
        content->setBounds ({ contentX, 0.0f, jmax (0.0f, getWidth() - contentX), itemHeight });
    }
}

//==============================================================================
void TreeViewRow::update (TreeView& newOwnerView, TreeViewItem& newItem, int newDepth, bool shouldBeSelected)
{
    const auto oldDepth = depth;
    const auto oldSelected = selected;
    const auto oldHeight = itemHeight;
    const auto oldText = itemText;

    ownerView = &newOwnerView;
    item = &newItem;
    depth = newDepth;
    selected = shouldBeSelected;
    itemHeight = newOwnerView.getFullItemHeight (newItem);
    itemText = newItem.getItemText();
    itemHasIcon = newItem.hasItemIcon();
    itemMightContainSubItems = newItem.mightContainSubItems();

    newItem.refreshItemComponent (content);

    if (content != nullptr && content->getParentComponent() != this)
        addAndMakeVisible (*content);

    disclosureButton->toFront (false);

    if (depth != oldDepth)
        notifyPropertyChanged (TreeViewRowIds::depth);

    if (selected != oldSelected)
        notifyPropertyChanged (TreeViewRowIds::selected);

    if (! approximatelyEqual (itemHeight, oldHeight))
        notifyPropertyChanged (TreeViewRowIds::itemHeight);

    if (itemText != oldText)
        notifyPropertyChanged (TreeViewRowIds::text);

    resized();
    repaint();
}

float TreeViewRow::getContentX() const
{
    if (ownerView == nullptr)
        return 0.0f;

    const auto indentSize = ownerView->getIndentSize();
    const auto levels = static_cast<float> (depth) + (ownerView->areOpenCloseButtonsVisible() ? 1.0f : 0.0f);

    return levels * indentSize;
}

bool TreeViewRow::showsDisclosure() const
{
    return ownerView != nullptr && itemMightContainSubItems && ownerView->areOpenCloseButtonsVisible();
}

} // namespace yup
