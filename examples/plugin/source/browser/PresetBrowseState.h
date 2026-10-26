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

#include "PresetLibrary.h"
#include "PresetQuery.h"
#include "PresetUserData.h"

#include <functional>
#include <unordered_map>
#include <vector>

//==============================================================================
/** One instance's view of the shared library: its query and the ordered results.

    The results are kept as keys, so a rescan never leaves them pointing at the wrong
    entry. They are recomputed when the query, the library or the user data change.

    All of the methods are for the message thread. The library and the user data must
    outlive this object.

    @see PresetLibrary, PresetQuery, PresetUserData
*/
class PresetBrowseState : private yup::ChangeListener
{
public:
    /** Starts with the default query over the library. */
    PresetBrowseState (PresetLibrary& library, PresetUserData& userData);

    ~PresetBrowseState() override;

    /** Returns the library browsed. */
    PresetLibrary& getLibrary() noexcept;

    /** Returns the favorites and ratings. */
    PresetUserData& getUserData() noexcept;

    /** Returns the current query. */
    const PresetQuery& getQuery() const noexcept;

    /** Replaces the query and recomputes the results at once, notifying when they changed. */
    void setQuery (PresetQuery newQuery);

    /** Returns the number of listed presets. */
    int getNumResults() const noexcept;

    /** Returns the entry listed at the row, or nullptr when the row is out of range. */
    const PresetEntry* getResult (int row) const;

    /** Returns the row listing the key, or -1 when it is not listed. */
    int findRow (const yup::String& key) const;

    /** Returns the key delta rows away from currentKey, wrapping at both ends.

        When currentKey is not listed this is the first row for a positive delta and the
        last one for a negative delta. Empty when nothing is listed.
    */
    yup::String getSteppedKey (const yup::String& currentKey, int delta) const;

    /** Called when the results changed: a new order or set from setQuery(), and every
        library or user data change, which can change what the rows show.
    */
    std::function<void()> onResultsChanged;

private:
    void changeListenerCallback (yup::ChangeBroadcaster* source) override;
    bool recompute();

    PresetLibrary& library;
    PresetUserData& userData;
    PresetQuery query;
    std::vector<yup::String> results;
    std::unordered_map<yup::String, int> rowByKey;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBrowseState)
};
