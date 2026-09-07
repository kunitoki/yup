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

//==============================================================================
namespace
{

//==============================================================================
/** A small colored box used as a visual placeholder in the layout demos. */
class LayoutCell : public yup::Component
{
public:
    LayoutCell (const yup::String& label, yup::Color color)
        : Component ("layoutCell")
        , cellLabel (label)
        , cellColor (color)
        , cellFont (yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (11.0f))
    {
    }

    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced (1.0f);

        g.setFillColor (cellColor);
        g.fillRoundedRect (bounds, 4.0f);

        g.setFillColor (yup::Colors::white);
        g.fillFittedText (cellLabel, cellFont, bounds, yup::Justification::center);
    }

private:
    yup::String cellLabel;
    yup::Color cellColor;
    yup::Font cellFont;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayoutCell)
};

//==============================================================================
/** Cycles through a fixed palette of pleasant colors for the demo cells. */
static yup::Color cellColor (int index)
{
    static const yup::Color palette[] = {
        yup::Colors::tomato,
        yup::Colors::dodgerblue,
        yup::Colors::seagreen,
        yup::Colors::goldenrod,
        yup::Colors::orchid,
        yup::Colors::teal,
        yup::Colors::indianred,
        yup::Colors::steelblue,
        yup::Colors::peru,
        yup::Colors::firebrick,
        yup::Colors::mediumseagreen,
        yup::Colors::darkslateblue,
    };

    return palette[index % yup::numElementsInArray (palette)];
}

//==============================================================================
/** Base class for a labeled panel that hosts a single layout demonstration. */
class DemoPanel : public yup::Component
{
public:
    DemoPanel (const yup::String& title)
        : Component ("layoutDemoPanel")
        , panelTitle (title)
        , captionFont (yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (13.0f))
    {
        setOpaque (false); // translucent rounded-rect background
    }

    void paint (yup::Graphics& g) override
    {
        auto bounds = getLocalBounds().reduced (0.5f);

        g.setFillColor (yup::Color (0x22ffffff));
        g.fillRoundedRect (bounds, 8.0f);

        g.setStrokeColor (yup::Color (0x55ffffff));
        g.strokeRoundedRect (bounds, 8.0f);

        g.setFillColor (yup::Colors::white);
        g.fillFittedText (panelTitle, captionFont, bounds.removeFromTop (24.0f), yup::Justification::center);
    }

    /** Creates a cell, makes it visible and returns it so it can be added to a layout. */
    LayoutCell* addCell (const yup::String& label, yup::Color color, float width = 56.0f, float height = 32.0f)
    {
        auto* cell = new LayoutCell (label, color);
        cell->setSize (width, height);
        cells.add (cell);
        addAndMakeVisible (cell);
        return cell;
    }

protected:
    /** The area below the caption, where the layout is performed. */
    yup::Rectangle<float> getContentArea() const
    {
        return getLocalBounds().withTrimmedTop (26.0f).reduced (8.0f);
    }

private:
    yup::OwnedArray<LayoutCell> cells;
    yup::String panelTitle;
    yup::Font captionFont;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoPanel)
};

//==============================================================================
/** A panel that demonstrates a FlexBox layout. */
class FlexPanel : public DemoPanel
{
public:
    using DemoPanel::DemoPanel;

    /** The FlexBox used by this panel. Configure it and add items to it. */
    yup::FlexBox& box()
    {
        return flexBox;
    }

    /** Adds a cell as a flex item with the given main/cross base sizes. */
    yup::FlexItem& addItem (LayoutCell* cell, float width, float height)
    {
        flexBox.items.add (yup::FlexItem (*cell, width, height));
        return flexBox.items.getReference (flexBox.items.size() - 1);
    }

    void resized() override
    {
        flexBox.performLayout (getContentArea());
    }

private:
    yup::FlexBox flexBox;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlexPanel)
};

//==============================================================================
/** A panel that demonstrates a Grid layout. */
class GridPanel : public DemoPanel
{
public:
    using DemoPanel::DemoPanel;

    /** The Grid used by this panel. Configure it and add items to it. */
    yup::Grid& grid()
    {
        return gridBox;
    }

    /** Adds a cell as a grid item, optionally at an explicit column/row. */
    yup::GridItem& addItem (LayoutCell* cell, int column = -1, int row = -1)
    {
        gridBox.items.add (yup::GridItem (*cell));
        auto& item = gridBox.items.getReference (gridBox.items.size() - 1);
        item.column = column;
        item.row = row;
        return item;
    }

    void resized() override
    {
        gridBox.performLayout (getContentArea());
    }

private:
    yup::Grid gridBox;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridPanel)
};

//==============================================================================
/** A page that wraps a set of panels using a responsive flexbox. */
class DemoPage : public yup::Component
{
public:
    DemoPage()
        : Component ("layoutDemoPage")
    {
        setOpaque (false); // transparent container, only panels are drawn
    }

    /** Adds a panel with the given base width/height used by the wrapping layout. */
    void addPanel (DemoPanel* panel, float minWidth = 240.0f, float minHeight = 160.0f)
    {
        panels.add (panel);
        panelWidths.add (minWidth);
        panelHeights.add (minHeight);
        addAndMakeVisible (panel);
    }

    void resized() override
    {
        pageBox.flexDirection = yup::FlexBox::Direction::row;
        pageBox.flexWrap = yup::FlexBox::Wrap::wrap;
        pageBox.alignItems = yup::FlexBox::AlignItems::stretch;
        pageBox.justifyContent = yup::FlexBox::JustifyContent::flexStart;
        pageBox.alignContent = yup::FlexBox::AlignContent::flexStart;
        pageBox.gap = 10.0f;

        pageBox.items.clear();

        for (int i = 0; i < panels.size(); ++i)
        {
            auto* panel = panels.getUnchecked (i);
            pageBox.items.add (yup::FlexItem (*panel, panelWidths.getUnchecked (i), panelHeights.getUnchecked (i))
                                   .withFlex (1.0f)
                                   .withMinWidth (panelWidths.getUnchecked (i)));
        }

        pageBox.performLayout (getLocalBounds());
    }

private:
    yup::OwnedArray<DemoPanel> panels;
    yup::Array<float> panelWidths;
    yup::Array<float> panelHeights;
    yup::FlexBox pageBox;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoPage)
};

//==============================================================================
/** A component that hosts an inner flexbox, used to demonstrate nesting. */
class InnerFlexHost : public yup::Component
{
public:
    InnerFlexHost()
        : Component ("innerFlexHost")
    {
        setOpaque (false); // transparent container, only cells are drawn

        for (int i = 0; i < 3; ++i)
        {
            auto* cell = new LayoutCell (yup::String (i + 1), cellColor (i));
            cell->setSize (0, 0);
            cells.add (cell);
            addAndMakeVisible (cell);
        }
    }

    void resized() override
    {
        innerBox.flexDirection = yup::FlexBox::Direction::row;
        innerBox.gap = 6.0f;

        innerBox.items.clear();
        innerBox.items.add (yup::FlexItem (*cells[0], 0, 0).withFlex (1));
        innerBox.items.add (yup::FlexItem (*cells[1], 0, 0).withFlex (1));
        innerBox.items.add (yup::FlexItem (*cells[2], 0, 0).withFlex (2));
        innerBox.performLayout (getLocalBounds().reduced (6.0f));
    }

private:
    yup::OwnedArray<LayoutCell> cells;
    yup::FlexBox innerBox;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InnerFlexHost)
};

//==============================================================================
/** Nested demo: a flex column with a flex row in the middle (holy grail). */
class HolyGrailPanel : public DemoPanel
{
public:
    HolyGrailPanel()
        : DemoPanel ("nested: flex column (holy grail)")
    {
        headerCell = addCell ("header", yup::Colors::steelblue);
        sidebarCell = addCell ("sidebar", yup::Colors::tomato, 56.0f, 40.0f);
        mainCell = addCell ("main", yup::Colors::seagreen);
        footerCell = addCell ("footer", yup::Colors::steelblue);

        middleBox.flexDirection = yup::FlexBox::Direction::row;
        middleBox.gap = 6.0f;
    }

    void resized() override
    {
        auto area = getContentArea();

        auto headerArea = area.removeFromTop (26.0f);
        auto footerArea = area.removeFromBottom (26.0f);
        auto middleArea = area.reduced (0.0f, 6.0f);

        headerCell->setBounds (headerArea);
        footerCell->setBounds (footerArea);

        middleBox.items.clear();
        middleBox.items.add (yup::FlexItem (*sidebarCell, 56.0f, 0));
        middleBox.items.add (yup::FlexItem (*mainCell, 0, 0).withFlex (1));
        middleBox.performLayout (middleArea);
    }

private:
    LayoutCell* headerCell = nullptr;
    LayoutCell* sidebarCell = nullptr;
    LayoutCell* mainCell = nullptr;
    LayoutCell* footerCell = nullptr;
    yup::FlexBox middleBox;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HolyGrailPanel)
};

//==============================================================================
/** Nested demo: a flexbox hosted inside a grid cell. */
class FlexInGridPanel : public DemoPanel
{
public:
    FlexInGridPanel()
        : DemoPanel ("nested: flex inside grid")
    {
        leftCell = addCell ("left", yup::Colors::tomato);
        rightCell = addCell ("right", yup::Colors::dodgerblue);

        innerHost = std::make_unique<InnerFlexHost>();
        addAndMakeVisible (innerHost.get());

        gridBox.templateColumns.add (yup::Grid::TrackInfo::px (56.0f));
        gridBox.templateColumns.add (yup::Grid::TrackInfo::fr (1.0f));
        gridBox.templateColumns.add (yup::Grid::TrackInfo::px (56.0f));
        gridBox.templateRows.add (yup::Grid::TrackInfo::px (90.0f));

        gridBox.items.add (yup::GridItem (*leftCell).withColumn (0).withRow (0));
        gridBox.items.add (yup::GridItem (*innerHost).withColumn (1).withRow (0));
        gridBox.items.add (yup::GridItem (*rightCell).withColumn (2).withRow (0));
    }

    void resized() override
    {
        gridBox.performLayout (getContentArea());
    }

private:
    LayoutCell* leftCell = nullptr;
    LayoutCell* rightCell = nullptr;
    std::unique_ptr<InnerFlexHost> innerHost;
    yup::Grid gridBox;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FlexInGridPanel)
};

//==============================================================================
/** Nested demo: two grids hosted inside a flexbox row. */
class GridInFlexPanel : public DemoPanel
{
public:
    GridInFlexPanel()
        : DemoPanel ("nested: grid inside flex")
    {
        leftGrid = std::make_unique<GridPanel> ("inner grid: fr tracks");
        leftGrid->grid().templateColumns.add (yup::Grid::TrackInfo::fr (1));
        leftGrid->grid().templateColumns.add (yup::Grid::TrackInfo::fr (2));
        leftGrid->grid().templateRows.add (yup::Grid::TrackInfo::px (56.0f));
        leftGrid->grid().columnGap = 6.0f;
        leftGrid->addItem (leftGrid->addCell ("a", yup::Colors::seagreen), 0, 0);
        leftGrid->addItem (leftGrid->addCell ("b", yup::Colors::seagreen), 1, 0);
        addAndMakeVisible (leftGrid.get());

        rightGrid = std::make_unique<GridPanel> ("inner grid: 2x2");
        rightGrid->grid().templateColumns.add (yup::Grid::TrackInfo::px (60.0f));
        rightGrid->grid().templateColumns.add (yup::Grid::TrackInfo::px (60.0f));
        rightGrid->grid().templateRows.add (yup::Grid::TrackInfo::px (30.0f));
        rightGrid->grid().templateRows.add (yup::Grid::TrackInfo::px (30.0f));
        rightGrid->grid().rowGap = 6.0f;
        for (int i = 0; i < 4; ++i)
        {
            auto& item = rightGrid->addItem (rightGrid->addCell (yup::String (i + 1), yup::Colors::orchid));
            item.column = i % 2;
            item.row = i / 2;
        }
        addAndMakeVisible (rightGrid.get());

        flexBox.flexDirection = yup::FlexBox::Direction::row;
        flexBox.gap = 8.0f;
    }

    void resized() override
    {
        flexBox.items.clear();
        flexBox.items.add (yup::FlexItem (*leftGrid, 0, 0).withFlex (1));
        flexBox.items.add (yup::FlexItem (*rightGrid, 0, 0).withFlex (1));
        flexBox.performLayout (getContentArea());
    }

private:
    std::unique_ptr<GridPanel> leftGrid;
    std::unique_ptr<GridPanel> rightGrid;
    yup::FlexBox flexBox;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GridInFlexPanel)
};

//==============================================================================
/** Page: all four flex directions side by side. */
static DemoPage* makeDirectionPage()
{
    auto* page = new DemoPage();

    auto addDirectionPanel = [] (const char* title, yup::FlexBox::Direction direction)
    {
        auto* panel = new FlexPanel (title);
        panel->box().flexDirection = direction;
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        for (int i = 0; i < 3; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), 60.0f, 28.0f);

        return panel;
    };

    page->addPanel (addDirectionPanel ("flex-direction: row", yup::FlexBox::Direction::row));
    page->addPanel (addDirectionPanel ("flex-direction: row-reverse", yup::FlexBox::Direction::rowReverse));
    page->addPanel (addDirectionPanel ("flex-direction: column", yup::FlexBox::Direction::column));
    page->addPanel (addDirectionPanel ("flex-direction: column-reverse", yup::FlexBox::Direction::columnReverse));

    return page;
}

//==============================================================================
/** Page: no-wrap, wrap and wrap-reverse. */
static DemoPage* makeWrapPage()
{
    auto* page = new DemoPage();

    auto addWrapPanel = [] (const char* title, yup::FlexBox::Wrap wrap)
    {
        auto* panel = new FlexPanel (title);
        panel->box().flexWrap = wrap;
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;
        panel->box().gap = 6.0f;

        for (int i = 0; i < 5; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), 70.0f, 28.0f);

        return panel;
    };

    page->addPanel (addWrapPanel ("flex-wrap: no-wrap", yup::FlexBox::Wrap::noWrap));
    page->addPanel (addWrapPanel ("flex-wrap: wrap", yup::FlexBox::Wrap::wrap));
    page->addPanel (addWrapPanel ("flex-wrap: wrap-reverse", yup::FlexBox::Wrap::wrapReverse));

    return page;
}

//==============================================================================
/** Page: every justify-content value. */
static DemoPage* makeJustifyContentPage()
{
    auto* page = new DemoPage();

    auto addJustifyPanel = [] (const char* title, yup::FlexBox::JustifyContent value)
    {
        auto* panel = new FlexPanel (title);
        panel->box().justifyContent = value;
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        for (int i = 0; i < 3; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), 56.0f, 28.0f);

        return panel;
    };

    page->addPanel (addJustifyPanel ("justify-content: flex-start", yup::FlexBox::JustifyContent::flexStart));
    page->addPanel (addJustifyPanel ("justify-content: flex-end", yup::FlexBox::JustifyContent::flexEnd));
    page->addPanel (addJustifyPanel ("justify-content: center", yup::FlexBox::JustifyContent::center));
    page->addPanel (addJustifyPanel ("justify-content: space-between", yup::FlexBox::JustifyContent::spaceBetween));
    page->addPanel (addJustifyPanel ("justify-content: space-around", yup::FlexBox::JustifyContent::spaceAround));

    return page;
}

//==============================================================================
/** Page: every align-items value. */
static DemoPage* makeAlignItemsPage()
{
    auto* page = new DemoPage();

    auto addAlignPanel = [] (const char* title, yup::FlexBox::AlignItems value)
    {
        auto* panel = new FlexPanel (title);
        panel->box().alignItems = value;

        const float heights[] = { 24.0f, 36.0f, 28.0f };

        for (int i = 0; i < 3; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), 64.0f, heights[i]);

        return panel;
    };

    page->addPanel (addAlignPanel ("align-items: flex-start", yup::FlexBox::AlignItems::flexStart));
    page->addPanel (addAlignPanel ("align-items: flex-end", yup::FlexBox::AlignItems::flexEnd));
    page->addPanel (addAlignPanel ("align-items: center", yup::FlexBox::AlignItems::center));
    page->addPanel (addAlignPanel ("align-items: stretch", yup::FlexBox::AlignItems::stretch));

    {
        auto* panel = new FlexPanel ("align-items: baseline");
        panel->box().alignItems = yup::FlexBox::AlignItems::baseline;

        auto& item1 = panel->addItem (panel->addCell ("1", cellColor (0)), 64.0f, 24.0f);
        item1.baseline = 18.0f;

        auto& item2 = panel->addItem (panel->addCell ("2", cellColor (1)), 64.0f, 36.0f);
        item2.baseline = 26.0f;

        auto& item3 = panel->addItem (panel->addCell ("3", cellColor (2)), 64.0f, 28.0f);
        item3.baseline = 22.0f;

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: every align-content value, with wrapping enabled. */
static DemoPage* makeAlignContentPage()
{
    auto* page = new DemoPage();

    auto addAlignContentPanel = [] (const char* title, yup::FlexBox::AlignContent value)
    {
        auto* panel = new FlexPanel (title);
        panel->box().flexWrap = yup::FlexBox::Wrap::wrap;
        panel->box().alignContent = value;
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;
        panel->box().gap = 6.0f;

        for (int i = 0; i < 6; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), 64.0f, 30.0f);

        return panel;
    };

    page->addPanel (addAlignContentPanel ("align-content: flex-start", yup::FlexBox::AlignContent::flexStart), 210.0f);
    page->addPanel (addAlignContentPanel ("align-content: flex-end", yup::FlexBox::AlignContent::flexEnd), 210.0f);
    page->addPanel (addAlignContentPanel ("align-content: center", yup::FlexBox::AlignContent::center), 210.0f);
    page->addPanel (addAlignContentPanel ("align-content: space-between", yup::FlexBox::AlignContent::spaceBetween), 210.0f);
    page->addPanel (addAlignContentPanel ("align-content: space-around", yup::FlexBox::AlignContent::spaceAround), 210.0f);
    page->addPanel (addAlignContentPanel ("align-content: stretch", yup::FlexBox::AlignContent::stretch), 210.0f);

    return page;
}

//==============================================================================
/** Page: flex-grow, flex-shrink and flex-basis. */
static DemoPage* makeGrowShrinkBasisPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new FlexPanel ("flex-grow: 1, 1, 2");
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        auto& grow1 = panel->addItem (panel->addCell ("grow 1", cellColor (0)), 50.0f, 28.0f);
        grow1.flexGrow = 1.0f;

        auto& grow2 = panel->addItem (panel->addCell ("grow 1", cellColor (1)), 50.0f, 28.0f);
        grow2.flexGrow = 1.0f;

        auto& grow3 = panel->addItem (panel->addCell ("grow 2", cellColor (2)), 50.0f, 28.0f);
        grow3.flexGrow = 2.0f;

        page->addPanel (panel);
    }

    {
        auto* panel = new FlexPanel ("flex-shrink: 1, 3, 0");
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        auto& item1 = panel->addItem (panel->addCell ("shrink 1", cellColor (0)), 100.0f, 28.0f);
        item1.flexShrink = 1.0f;

        auto& item2 = panel->addItem (panel->addCell ("shrink 3", cellColor (1)), 100.0f, 28.0f);
        item2.flexShrink = 3.0f;

        auto& item3 = panel->addItem (panel->addCell ("shrink 0", cellColor (2)), 100.0f, 28.0f);
        item3.flexShrink = 0.0f;

        page->addPanel (panel);
    }

    {
        auto* panel = new FlexPanel ("flex-basis: 50, 70, 60 + grow 1");
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        const float basis[] = { 50.0f, 70.0f, 60.0f };

        for (int i = 0; i < 3; ++i)
        {
            auto& item = panel->addItem (panel->addCell (yup::String (basis[i]), cellColor (i)), 50.0f, 28.0f);
            item.flexGrow = 1.0f;
            item.flexBasis = basis[i];
        }

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: align-self and order. */
static DemoPage* makeAlignSelfOrderPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new FlexPanel ("align-self: start / end / center / stretch");
        panel->box().alignItems = yup::FlexBox::AlignItems::center;

        auto& item1 = panel->addItem (panel->addCell ("start", cellColor (0)), 56.0f, 40.0f);
        item1.alignSelf = yup::FlexItem::AlignSelf::flexStart;

        auto& item2 = panel->addItem (panel->addCell ("end", cellColor (1)), 56.0f, 40.0f);
        item2.alignSelf = yup::FlexItem::AlignSelf::flexEnd;

        auto& item3 = panel->addItem (panel->addCell ("center", cellColor (2)), 56.0f, 40.0f);
        item3.alignSelf = yup::FlexItem::AlignSelf::center;

        auto& item4 = panel->addItem (panel->addCell ("stretch", cellColor (3)), 56.0f, 40.0f);
        item4.alignSelf = yup::FlexItem::AlignSelf::stretch;

        page->addPanel (panel);
    }

    {
        auto* panel = new FlexPanel ("align-self: baseline");
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        auto& item1 = panel->addItem (panel->addCell ("1", cellColor (0)), 64.0f, 40.0f);
        item1.alignSelf = yup::FlexItem::AlignSelf::baseline;
        item1.baseline = 20.0f;

        auto& item2 = panel->addItem (panel->addCell ("2", cellColor (1)), 64.0f, 56.0f);
        item2.alignSelf = yup::FlexItem::AlignSelf::baseline;
        item2.baseline = 30.0f;

        page->addPanel (panel);
    }

    {
        auto* panel = new FlexPanel ("order: 3, 1, 0, 2");
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        const int orders[] = { 3, 1, 0, 2 };

        for (int i = 0; i < 4; ++i)
        {
            auto& item = panel->addItem (panel->addCell ("order " + yup::String (orders[i]), cellColor (i)), 56.0f, 28.0f);
            item.order = orders[i];
        }

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: margins and gap. */
static DemoPage* makeMarginsGapPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new FlexPanel ("margins: 8px");
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        for (int i = 0; i < 3; ++i)
        {
            auto& item = panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), 56.0f, 28.0f);
            item.marginLeft = item.marginRight = item.marginTop = item.marginBottom = 8.0f;
        }

        page->addPanel (panel);
    }

    {
        auto* panel = new FlexPanel ("gap: 14px");
        panel->box().gap = 14.0f;
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        for (int i = 0; i < 3; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), 56.0f, 28.0f);

        page->addPanel (panel);
    }

    {
        auto* panel = new FlexPanel ("margins + gap");
        panel->box().gap = 8.0f;
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        for (int i = 0; i < 3; ++i)
        {
            auto& item = panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), 56.0f, 28.0f);
            item.marginLeft = item.marginRight = item.marginTop = item.marginBottom = 6.0f;
        }

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: percentage sizing and min/max constraints on flex items. */
static DemoPage* makeFlexPercentMinMaxPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new FlexPanel ("width/height percent");
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        auto& item1 = panel->addItem (panel->addCell ("30%", cellColor (0)), 0, 0);
        item1.widthPercent = 30.0f;
        item1.heightPercent = 50.0f;

        auto& item2 = panel->addItem (panel->addCell ("40%", cellColor (1)), 0, 0);
        item2.widthPercent = 40.0f;
        item2.heightPercent = 50.0f;

        auto& item3 = panel->addItem (panel->addCell ("30%", cellColor (2)), 0, 0);
        item3.widthPercent = 30.0f;
        item3.heightPercent = 50.0f;

        page->addPanel (panel);
    }

    {
        auto* panel = new FlexPanel ("min/max constraints");
        panel->box().alignItems = yup::FlexBox::AlignItems::flexStart;

        auto& item1 = panel->addItem (panel->addCell ("min-w 80", cellColor (0)), 40.0f, 28.0f);
        item1.flexGrow = 1.0f;
        item1.minWidth = 80.0f;

        auto& item2 = panel->addItem (panel->addCell ("max-w 60", cellColor (1)), 40.0f, 28.0f);
        item2.flexGrow = 1.0f;
        item2.maxWidth = 60.0f;

        auto& item3 = panel->addItem (panel->addCell ("min-h 70", cellColor (2)), 40.0f, 60.0f);
        item3.minHeight = 70.0f;

        auto& item4 = panel->addItem (panel->addCell ("max-h 40", cellColor (3)), 40.0f, 60.0f);
        item4.maxHeight = 40.0f;

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: grid track sizing with px, fr and auto tracks. */
static DemoPage* makeGridTracksPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new GridPanel ("tracks: px(70), fr(1), fr(2)");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (70.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::fr (1.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::fr (2.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (60.0f));

        for (int i = 0; i < 3; ++i)
        {
            auto& item = panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)));
            item.column = i;
            item.row = 0;
        }

        page->addPanel (panel);
    }

    {
        auto* panel = new GridPanel ("tracks: auto, fr(1), fr(2)");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::auto_());
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::fr (1.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::fr (2.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (60.0f));

        for (int i = 0; i < 3; ++i)
        {
            auto& item = panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)));
            item.column = i;
            item.row = 0;
        }

        page->addPanel (panel);
    }

    {
        auto* panel = new GridPanel ("implicit tracks: autoColumns 100");
        panel->grid().autoColumns = 100.0f;
        panel->grid().autoRows = 56.0f;

        for (int i = 0; i < 2; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)));

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: explicit grid placement and spans. */
static DemoPage* makeGridPlacementPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new GridPanel ("explicit placement: 2x2");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (80.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (80.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (44.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (44.0f));

        for (int i = 0; i < 4; ++i)
        {
            auto& item = panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)));
            item.column = i % 2;
            item.row = i / 2;
        }

        page->addPanel (panel);
    }

    {
        auto* panel = new GridPanel ("spans: 2x2, 3 cols, 3 rows");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (60.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (60.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (60.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (32.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (32.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (32.0f));
        panel->grid().rowGap = 6.0f;

        {
            auto& item = panel->addItem (panel->addCell ("A", cellColor (0)), 0, 0);
            item.columnSpan = 2;
            item.rowSpan = 2;
        }
        {
            auto& item = panel->addItem (panel->addCell ("B", cellColor (1)));
            item.column = 2;
            item.row = 0;
        }
        {
            auto& item = panel->addItem (panel->addCell ("C", cellColor (2)));
            item.column = 2;
            item.row = 1;
        }
        {
            auto& item = panel->addItem (panel->addCell ("D", cellColor (3)));
            item.column = 0;
            item.row = 2;
            item.columnSpan = 2;
        }
        {
            auto& item = panel->addItem (panel->addCell ("E", cellColor (4)));
            item.column = 2;
            item.row = 2;
        }

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: auto placement flowing into template and implicit tracks. */
static DemoPage* makeGridAutoPlacementPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new GridPanel ("auto flow: 2 template columns");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (70.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (70.0f));
        panel->grid().autoRows = 36.0f;

        for (int i = 0; i < 5; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)));

        page->addPanel (panel);
    }

    {
        auto* panel = new GridPanel ("explicit + auto placement");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (60.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (60.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (60.0f));
        panel->grid().autoRows = 40.0f;

        {
            auto& item = panel->addItem (panel->addCell ("A", cellColor (0)));
            item.column = 0;
            item.row = 0;
            item.columnSpan = 2;
        }

        for (int i = 0; i < 3; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i + 1)));

        page->addPanel (panel);
    }

    {
        auto* panel = new GridPanel ("implicit growth: no template");
        panel->grid().autoColumns = 70.0f;
        panel->grid().autoRows = 44.0f;

        for (int i = 0; i < 3; ++i)
            panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)));

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: grid gaps and item margins. */
static DemoPage* makeGridGapsMarginsPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new GridPanel ("columnGap 14 / rowGap 10");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (64.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (64.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (44.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (44.0f));
        panel->grid().columnGap = 14.0f;
        panel->grid().rowGap = 10.0f;

        for (int i = 0; i < 4; ++i)
        {
            auto& item = panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)));
            item.column = i % 2;
            item.row = i / 2;
        }

        page->addPanel (panel);
    }

    {
        auto* panel = new GridPanel ("item margins: 8px");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (80.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (80.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (44.0f));

        for (int i = 0; i < 2; ++i)
        {
            auto& item = panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), i, 0);
            item.marginLeft = item.marginRight = item.marginTop = item.marginBottom = 8.0f;
        }

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: grid item alignment, both container defaults and per-item overrides. */
static DemoPage* makeGridAlignmentPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new GridPanel ("justifyItems center / alignItems flex-end");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (90.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (90.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (60.0f));
        panel->grid().justifyItems = yup::Grid::AlignItems::center;
        panel->grid().alignItems = yup::Grid::AlignItems::flexEnd;

        for (int i = 0; i < 2; ++i)
        {
            auto& item = panel->addItem (panel->addCell (yup::String (i + 1), cellColor (i)), i, 0);
            item.width = 44.0f;
            item.height = 22.0f;
        }

        page->addPanel (panel);
    }

    {
        auto* panel = new GridPanel ("justify-self / align-self overrides");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (90.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (90.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (56.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (56.0f));

        const auto start = yup::GridItem::AlignSelf::flexStart;
        const auto end = yup::GridItem::AlignSelf::flexEnd;
        const auto center = yup::GridItem::AlignSelf::center;
        const auto stretch = yup::GridItem::AlignSelf::stretch;

        {
            auto& item = panel->addItem (panel->addCell ("start", cellColor (0)), 0, 0);
            item.width = 40.0f;
            item.height = 20.0f;
            item.justifySelf = start;
            item.alignSelf = start;
        }
        {
            auto& item = panel->addItem (panel->addCell ("end", cellColor (1)), 1, 0);
            item.width = 40.0f;
            item.height = 20.0f;
            item.justifySelf = end;
            item.alignSelf = end;
        }
        {
            auto& item = panel->addItem (panel->addCell ("center", cellColor (2)), 0, 1);
            item.width = 40.0f;
            item.height = 20.0f;
            item.justifySelf = center;
            item.alignSelf = center;
        }
        {
            auto& item = panel->addItem (panel->addCell ("stretch", cellColor (3)), 1, 1);
            item.width = 40.0f;
            item.height = 20.0f;
            item.justifySelf = stretch;
            item.alignSelf = stretch;
        }

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: percentage sizing and min/max clamps on grid items. */
static DemoPage* makeGridPercentMinMaxPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new GridPanel ("percentage of cell: 60% / 50%");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::fr (1.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (90.0f));

        auto& item = panel->addItem (panel->addCell ("60% x 50%", cellColor (0)), 0, 0);
        item.widthPercent = 60.0f;
        item.heightPercent = 50.0f;
        item.justifySelf = yup::GridItem::AlignSelf::center;
        item.alignSelf = yup::GridItem::AlignSelf::center;

        page->addPanel (panel);
    }

    {
        auto* panel = new GridPanel ("min/max clamps");
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (100.0f));
        panel->grid().templateColumns.add (yup::Grid::TrackInfo::px (100.0f));
        panel->grid().templateRows.add (yup::Grid::TrackInfo::px (70.0f));

        {
            auto& item = panel->addItem (panel->addCell ("min 60x40", cellColor (0)), 0, 0);
            item.width = 40.0f;
            item.height = 30.0f;
            item.minWidth = 60.0f;
            item.minHeight = 40.0f;
            item.justifySelf = yup::GridItem::AlignSelf::center;
            item.alignSelf = yup::GridItem::AlignSelf::center;
        }
        {
            auto& item = panel->addItem (panel->addCell ("max 90x60", cellColor (1)), 1, 0);
            item.width = 120.0f;
            item.height = 80.0f;
            item.maxWidth = 90.0f;
            item.maxHeight = 60.0f;
            item.justifySelf = yup::GridItem::AlignSelf::center;
            item.alignSelf = yup::GridItem::AlignSelf::center;
        }

        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Page: nested flex/grid layouts. */
static DemoPage* makeNestedPage()
{
    auto* page = new DemoPage();

    {
        auto* panel = new HolyGrailPanel();
        page->addPanel (panel, 240.0f, 220.0f);
    }

    {
        auto* panel = new FlexInGridPanel();
        page->addPanel (panel);
    }

    {
        auto* panel = new GridInFlexPanel();
        page->addPanel (panel);
    }

    return page;
}

//==============================================================================
/** Creates the page for the given selector index. */
static DemoPage* createLayoutPage (int index)
{
    switch (index)
    {
        case 0:
            return makeDirectionPage();
        case 1:
            return makeWrapPage();
        case 2:
            return makeJustifyContentPage();
        case 3:
            return makeAlignItemsPage();
        case 4:
            return makeAlignContentPage();
        case 5:
            return makeGrowShrinkBasisPage();
        case 6:
            return makeAlignSelfOrderPage();
        case 7:
            return makeMarginsGapPage();
        case 8:
            return makeFlexPercentMinMaxPage();
        case 9:
            return makeGridTracksPage();
        case 10:
            return makeGridPlacementPage();
        case 11:
            return makeGridAutoPlacementPage();
        case 12:
            return makeGridGapsMarginsPage();
        case 13:
            return makeGridAlignmentPage();
        case 14:
            return makeGridPercentMinMaxPage();
        case 15:
            return makeNestedPage();
        default:
            return new DemoPage();
    }
}

} // namespace

//==============================================================================
/**
    Demonstrates the CSS-style FlexBox and Grid layout containers available in
    yup_gui, exercising every layout property: flex direction, wrapping,
    justification, cross-axis alignment, flex grow/shrink/basis, align-self,
    order, margins, gaps, percentage sizing, min/max constraints, grid track
    sizing (px / fr / auto), explicit placement, spans, auto placement, grid
    alignment and nested flex/grid compositions.

    Use the combo box at the top to switch between the different pages.
*/
class LayoutExample : public yup::Component
{
public:
    LayoutExample()
        : Component ("LayoutExample")
    {
        setupSelector();
        showPage (0);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10.0f);

        selector->setBounds (bounds.removeFromTop (30.0f));

        bounds.removeFromTop (10.0f);

        if (page != nullptr)
            page->setBounds (bounds);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

private:
    void setupSelector()
    {
        selector = std::make_unique<yup::ComboBox> ("layoutPageSelector");

        selector->addItem ("Flex Direction", 1);
        selector->addItem ("Flex Wrap", 2);
        selector->addItem ("Justify Content", 3);
        selector->addItem ("Align Items", 4);
        selector->addItem ("Align Content", 5);
        selector->addItem ("Grow / Shrink / Basis", 6);
        selector->addItem ("Align Self & Order", 7);
        selector->addItem ("Margins & Gap", 8);
        selector->addItem ("Percent & Min/Max", 9);
        selector->addItem ("Grid Tracks", 10);
        selector->addItem ("Grid Placement & Spans", 11);
        selector->addItem ("Grid Auto Placement", 12);
        selector->addItem ("Grid Gaps & Margins", 13);
        selector->addItem ("Grid Alignment", 14);
        selector->addItem ("Grid Percent & Min/Max", 15);
        selector->addItem ("Nested Layout", 16);

        selector->setSelectedId (1, yup::dontSendNotification);
        selector->onSelectedItemChanged = [this]
        {
            showPage (selector->getSelectedId() - 1);
        };

        addAndMakeVisible (selector.get());
    }

    void showPage (int index)
    {
        if (page != nullptr)
            removeChildComponent (page.get());

        page.reset (createLayoutPage (index));
        page->setBounds (getLocalBounds());
        addAndMakeVisible (page.get());
        resized();
    }

    std::unique_ptr<yup::ComboBox> selector;
    std::unique_ptr<yup::Component> page;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LayoutExample)
};
