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

#include "SynthPresetList.h"
#include "SynthTheme.h"
#include "../browser/PresetBrowseState.h"

#include <cmath>

namespace
{
constexpr const char* columnTitles[] = { "", "NAME", "CATEGORY", "AUTHOR", "LOCATION", "MODIFIED", "RATING" };
constexpr const char* sortNames[] = { "Favorite", "Name", "Category", "Author", "Location", "Modified", "Rating", "Relevance" };
constexpr int columnCount = 7;
constexpr int sortKeyCount = 8;
constexpr int ascendingId = 100;
constexpr int descendingId = 101;

yup::Font getIconFont (float height)
{
    return yup::ApplicationTheme::getGlobalTheme()->getDefaultIconFont().withHeight (height);
}

yup::Rectangle<float> getStarArea (yup::Rectangle<float> ratingColumn, int star)
{
    const auto width = ratingColumn.getWidth() / 5.0f;
    return { ratingColumn.getX() + width * static_cast<float> (star), ratingColumn.getY(), width, ratingColumn.getHeight() };
}
} // namespace

//==============================================================================
/** The list itself, which takes the presses on its rows before the list selects them, so
    a star or rating click never moves the current row and so never loads.
*/
class SynthPresetList::Rows : public yup::ListBox
{
public:
    std::function<bool (int row, const yup::MouseEvent& event)> onPress;

    void mouseDown (const yup::MouseEvent& event) override
    {
        if (onPress != nullptr && onPress (findRowAt (event.getPosition()), event))
            return;

        yup::ListBox::mouseDown (event);
    }

private:
    int findRowAt (yup::Point<float> position) const
    {
        const auto first = getRowBounds (0);

        if (first.isEmpty())
            return -1;

        const auto row = static_cast<int> (std::floor ((position.getY() - first.getY()) / first.getHeight()));
        return getRowBounds (row).contains (position) ? row : -1;
    }
};

//==============================================================================
/** One preset's row, painted from a copy of its entry; the list takes its presses. */
class SynthPresetList::Row : public yup::Component
{
public:
    Row()
    {
        setOpaque (true);
        setWantsMouseEvents (false, false);
    }

    void show (const PresetEntry* newEntry, bool isFavorite, int stars, bool isSelected)
    {
        entry = newEntry != nullptr ? *newEntry : PresetEntry {};
        favorite = isFavorite;
        rating = stars;
        selected = isSelected;
        repaint();
    }

    void paint (yup::Graphics& g) override
    {
        const auto bounds = getLocalBounds();
        const auto accent = SynthTheme::accent (*this);
        const auto dim = SynthTheme::textSecondary (*this).withAlpha (0.35f);

        g.setFillColor (SynthTheme::wellColor (*this));
        g.fillRect (bounds);

        if (selected)
        {
            g.setFillColor (accent.withAlpha (0.18f));
            g.fillRect (bounds);
        }

        const auto columns = layoutColumns (bounds);
        const auto star = yup::String::fromUTF8 (YUP_ICON_STAR);
        const auto iconFont = getIconFont (10.0f);

        g.setFillColor (favorite ? accent : dim);
        g.fillFittedText (star, iconFont, columns[0], yup::Justification::center);

        g.setFillColor (selected ? accent : SynthTheme::textPrimary (*this));
        g.fillFittedText (entry.name, SynthTheme::valueFont (13.0f), columns[1].reduced (4.0f, 0.0f), yup::Justification::centerLeft);

        const auto date = entry.isFactory() ? yup::String ("-") : entry.modified.formatted ("%Y-%m-%d");
        const std::array<yup::String, 4> details { entry.category, entry.author, entry.location, date };
        const auto detailFont = SynthTheme::valueFont (12.0f);
        g.setFillColor (SynthTheme::textSecondary (*this));

        for (std::size_t index = 0; index < details.size(); ++index)
            g.fillFittedText (details[index], detailFont, columns[index + 2].reduced (4.0f, 0.0f), yup::Justification::centerLeft);

        for (int index = 0; index < 5; ++index)
        {
            g.setFillColor (index < rating ? accent : dim);
            g.fillFittedText (star, iconFont, getStarArea (columns[6], index), yup::Justification::center);
        }
    }

private:
    PresetEntry entry;
    bool favorite = false;
    int rating = 0;
    bool selected = false;
};

//==============================================================================
SynthPresetList::SynthPresetList (RetraceProcessor& processorToControl)
    : processor (processorToControl)
    , browser (processorToControl.getPresetBrowser())
    , rows (std::make_unique<Rows>())
{
    setOpaque (false);

    searchField.setMultiLine (false);
    searchField.setFont (SynthTheme::valueFont (13.5f));
    searchField.setText (browser.getQuery().search, yup::dontSendNotification);
    searchField.onTextChange = [this]
    {
        auto query = browser.getQuery();
        query.search = searchField.getText();
        browser.setQuery (std::move (query));
        repaint();
    };
    searchField.onStarted = [this] { repaint(); };
    searchField.onFinished = [this] { repaint(); };
    addAndMakeVisible (searchField);

    SynthTheme::styleRaisedButton (sortButton);
    sortButton.onClick = [this] { showSortMenu(); };
    addAndMakeVisible (sortButton);

    rescanButton.onClick = [this] { browser.getLibrary().rescan(); };
    addAndMakeVisible (rescanButton);

    rows->setRowSize (rowHeight);
    rows->setDragSourceEnabled (false);
    rows->setWantsKeyboardFocus (true);
    rows->onPress = [this] (int row, const yup::MouseEvent& event) { return handleRowPress (row, event); };
    rows->onCurrentRowChanged = [this] (int row) { loadRow (row); };
    rows->setModel (this);
    addAndMakeVisible (*rows);

    browser.onResultsChanged = [this]
    {
        rows->updateContent();
        showLoadedPreset();
        repaint();
    };

    updateSortButton();
    showLoadedPreset();
}

SynthPresetList::~SynthPresetList()
{
    browser.onResultsChanged = nullptr;
    rows->setModel (nullptr);
}

void SynthPresetList::syncFromProcessor()
{
    if (processor.getCurrentPresetKey() != shownKey)
        showLoadedPreset();
}

void SynthPresetList::focusList()
{
    rows->takeKeyboardFocus();
}

std::array<yup::Rectangle<float>, 7> SynthPresetList::layoutColumns (yup::Rectangle<float> row)
{
    constexpr float favoriteWidth = 24.0f;
    constexpr float ratingWidth = 72.0f;
    constexpr std::array<float, 5> shares { 0.30f, 0.14f, 0.14f, 0.18f, 0.12f };
    constexpr float totalShare = 0.88f;

    std::array<yup::Rectangle<float>, 7> columns;
    columns[0] = row.removeFromLeft (favoriteWidth);
    columns[6] = row.removeFromRight (ratingWidth);

    const auto width = row.getWidth();

    for (std::size_t index = 0; index < shares.size(); ++index)
        columns[index + 1] = row.removeFromLeft (width * shares[index] / totalShare);

    return columns;
}

void SynthPresetList::resized()
{
    auto bounds = getLocalBounds();
    auto topBar = bounds.removeFromTop (topBarHeight).reduced (0.0f, 2.0f);

    rescanButton.setBounds (topBar.removeFromRight (topBar.getHeight()));
    topBar.removeFromRight (SynthTheme::controlGap);
    sortButton.setBounds (topBar.removeFromRight (120.0f));
    topBar.removeFromRight (SynthTheme::controlGap);
    searchArea = topBar;
    searchField.setBounds (searchArea.reduced (4.0f, 2.0f).withTrimmedLeft (18.0f));

    bounds.removeFromTop (SynthTheme::controlGap);
    columnHeader = bounds.removeFromTop (columnHeaderHeight);
    listArea = bounds;
    rows->setBounds (listArea.reduced (2.0f));
}

void SynthPresetList::paint (yup::Graphics& g)
{
    SynthTheme::paintWell (g, *this, searchArea, SynthTheme::displayCorner);
    g.setFillColor (SynthTheme::textSecondary (*this));
    g.fillFittedText (yup::String::fromUTF8 (YUP_ICON_MAGNIFYING_GLASS), getIconFont (10.0f), searchArea.withWidth (22.0f), yup::Justification::center);

    SynthTheme::paintWell (g, *this, listArea, SynthTheme::displayCorner);

    const auto& query = browser.getQuery();
    const auto columns = layoutColumns (columnHeader);
    const auto captionFont = SynthTheme::captionFont (10.0f);
    const auto sortColumn = static_cast<std::size_t> (query.sortKey);

    for (std::size_t index = 0; index < columns.size(); ++index)
    {
        auto area = columns[index].reduced (4.0f, 0.0f);
        const auto isSorted = index == sortColumn;
        g.setFillColor (isSorted ? SynthTheme::accent (*this) : SynthTheme::textSecondary (*this));

        if (index == 0)
            g.fillFittedText (yup::String::fromUTF8 (YUP_ICON_STAR), getIconFont (9.0f), columns[index], yup::Justification::center);
        else
            g.fillFittedText (columnTitles[index], captionFont, area, yup::Justification::centerLeft);

        if (isSorted && index > 0)
        {
            const auto caret = yup::String::fromUTF8 (query.ascending ? YUP_ICON_CARET_UP : YUP_ICON_CARET_DOWN);
            const auto titleWidth = SynthTheme::textWidth (columnTitles[index], captionFont);
            g.fillFittedText (caret, getIconFont (9.0f), area.withTrimmedLeft (titleWidth + 4.0f).withWidth (10.0f), yup::Justification::center);
        }
    }
}

void SynthPresetList::paintOverChildren (yup::Graphics& g)
{
    SynthTheme::paintDisplayBorder (g, *this, searchArea);
    SynthTheme::paintDisplayBorder (g, *this, listArea);

    if (searchField.getText().isNotEmpty() || searchField.hasKeyboardFocus())
        return;

    g.setFillColor (SynthTheme::textSecondary (*this).withAlpha (0.6f));
    g.fillFittedText ("Search", SynthTheme::valueFont (13.5f), searchField.getBounds().reduced (4.0f, 0.0f), yup::Justification::centerLeft);
}

void SynthPresetList::mouseDown (const yup::MouseEvent& event)
{
    const auto columns = layoutColumns (columnHeader);

    for (int index = 0; index < columnCount; ++index)
    {
        if (columns[static_cast<std::size_t> (index)].contains (event.getPosition()))
        {
            sortBy (index);
            return;
        }
    }
}

int SynthPresetList::getNumRows()
{
    return browser.getNumResults();
}

void SynthPresetList::refreshRowComponent (int row, bool isSelected, std::unique_ptr<yup::Component>& component)
{
    auto& rowComponent = reuseOrCreate<Row> (component);
    const auto* entry = browser.getResult (row);
    auto& userData = browser.getUserData();

    if (entry == nullptr)
    {
        rowComponent.show (nullptr, false, 0, false);
        return;
    }

    rowComponent.show (entry, userData.isFavorite (entry->key), userData.getRating (entry->key), isSelected);
}

bool SynthPresetList::handleRowPress (int row, const yup::MouseEvent& event)
{
    if (event.isRightButtonDown())
    {
        rows->takeKeyboardFocus();

        if (onMenuRequested != nullptr)
            onMenuRequested (row, getLocalPoint (rows.get(), event.getPosition()));

        return true;
    }

    const auto* entry = browser.getResult (row);

    if (entry == nullptr)
        return false;

    rows->takeKeyboardFocus();

    const auto columns = layoutColumns (rows->getRowBounds (row));
    const auto position = event.getPosition();
    auto& userData = browser.getUserData();

    if (columns[0].contains (position))
    {
        userData.setFavorite (entry->key, ! userData.isFavorite (entry->key));
        return true;
    }

    if (columns[6].contains (position))
    {
        for (int star = 0; star < 5; ++star)
        {
            if (getStarArea (columns[6], star).contains (position))
            {
                const auto stars = star + 1;
                userData.setRating (entry->key, userData.getRating (entry->key) == stars ? 0 : stars);
                break;
            }
        }

        return true;
    }

    rows->selectRow (row, false, yup::dontSendNotification);
    loadRow (row);
    return true;
}

void SynthPresetList::loadRow (int row)
{
    const auto* entry = browser.getResult (row);

    if (entry == nullptr)
        return;

    if (const auto result = processor.loadPresetEntry (*entry); result.failed())
        yup::Logger::writeToLog ("Retrace: " + result.getErrorMessage());

    showLoadedPreset();
}

/** Selects the loaded preset's row without notifying, so following it never loads. */
void SynthPresetList::showLoadedPreset()
{
    shownKey = processor.getCurrentPresetKey();

    if (const auto row = browser.findRow (shownKey); row >= 0)
    {
        if (rows->getSelectedRow() != row)
            rows->selectRow (row, true, yup::dontSendNotification);
    }
    else
    {
        rows->deselectAllRows (yup::dontSendNotification);
    }
}

void SynthPresetList::sortBy (int column)
{
    auto query = browser.getQuery();
    const auto key = static_cast<PresetQuery::SortKey> (column);

    query.ascending = query.sortKey == key ? ! query.ascending : true;
    query.sortKey = key;
    browser.setQuery (std::move (query));

    updateSortButton();
    repaint();
}

void SynthPresetList::showSortMenu()
{
    const auto& query = browser.getQuery();
    auto menu = yup::PopupMenu::create (yup::PopupMenu::Options {}
                                            .withParentComponent (getPopupParentComponent())
                                            .withMinimumWidth (static_cast<int> (sortButton.getWidth()))
                                            .withRelativePosition (&sortButton));

    for (int index = 0; index < sortKeyCount; ++index)
        menu->addItem (sortNames[index], index + 1, true, static_cast<int> (query.sortKey) == index);

    menu->addSeparator();
    menu->addItem ("Ascending", ascendingId, true, query.ascending);
    menu->addItem ("Descending", descendingId, true, ! query.ascending);

    menu->show ([safeThis = yup::WeakReference<yup::Component> (this)] (int selectedId)
    {
        auto* list = dynamic_cast<SynthPresetList*> (safeThis.get());

        if (list == nullptr || selectedId <= 0)
            return;

        auto newQuery = list->browser.getQuery();

        if (selectedId == ascendingId || selectedId == descendingId)
            newQuery.ascending = selectedId == ascendingId;
        else if (yup::isPositiveAndBelow (selectedId - 1, sortKeyCount))
            newQuery.sortKey = static_cast<PresetQuery::SortKey> (selectedId - 1);

        list->browser.setQuery (std::move (newQuery));
        list->updateSortButton();
        list->repaint();
    });
}

void SynthPresetList::updateSortButton()
{
    sortButton.setButtonText (yup::String ("Sort: ") + sortNames[static_cast<int> (browser.getQuery().sortKey)]);
}
