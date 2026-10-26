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

#include "PresetBrowseState.h"

PresetBrowseState::PresetBrowseState (PresetLibrary& libraryToBrowse, PresetUserData& userDataToUse)
    : library (libraryToBrowse)
    , userData (userDataToUse)
{
    library.addChangeListener (this);
    userData.addChangeListener (this);
    recompute();
}

PresetBrowseState::~PresetBrowseState()
{
    userData.removeChangeListener (this);
    library.removeChangeListener (this);
}

PresetLibrary& PresetBrowseState::getLibrary() noexcept
{
    return library;
}

PresetUserData& PresetBrowseState::getUserData() noexcept
{
    return userData;
}

const PresetQuery& PresetBrowseState::getQuery() const noexcept
{
    return query;
}

void PresetBrowseState::setQuery (PresetQuery newQuery)
{
    query = std::move (newQuery);

    if (recompute() && onResultsChanged != nullptr)
        onResultsChanged();
}

int PresetBrowseState::getNumResults() const noexcept
{
    return static_cast<int> (results.size());
}

const PresetEntry* PresetBrowseState::getResult (int row) const
{
    if (row < 0 || row >= getNumResults())
        return nullptr;

    return library.findEntry (results[static_cast<std::size_t> (row)]);
}

int PresetBrowseState::findRow (const yup::String& key) const
{
    const auto found = rowByKey.find (key);
    return found != rowByKey.end() ? found->second : -1;
}

yup::String PresetBrowseState::getSteppedKey (const yup::String& currentKey, int delta) const
{
    if (results.empty())
        return {};

    const auto row = findRow (currentKey);

    if (row < 0)
        return delta < 0 ? results.back() : results.front();

    const auto numResults = getNumResults();
    return results[static_cast<std::size_t> (((row + delta) % numResults + numResults) % numResults)];
}

void PresetBrowseState::changeListenerCallback (yup::ChangeBroadcaster*)
{
    recompute();

    if (onResultsChanged != nullptr)
        onResultsChanged();
}

bool PresetBrowseState::recompute()
{
    const auto& entries = library.getEntries();
    std::vector<yup::String> newResults;

    for (const auto index : query.run (entries, userData))
        newResults.push_back (entries[index].key);

    if (newResults == results)
        return false;

    results = std::move (newResults);
    rowByKey.clear();

    for (std::size_t row = 0; row < results.size(); ++row)
        rowByKey.emplace (results[row], static_cast<int> (row));

    return true;
}
