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

#include "PresetEntry.h"
#include "PresetUserData.h"

#include <vector>

//==============================================================================
/** What the browser lists: a source, filters, a fuzzy search and a sort order.

    Several categories match any of them, several tags must all match, and every filter
    combines with the others and with the search.

    @see PresetEntry, PresetFuzzyMatch, PresetBrowseState
*/
struct PresetQuery
{
    /** Where the presets come from. */
    enum class Source
    {
        all,
        favorites,
        factory,
        folder
    };

    /** What the list is sorted by. */
    enum class SortKey
    {
        favorite,
        name,
        category,
        author,
        location,
        modified,
        rating,
        relevance /**< How well the search matches, best first when ascending. */
    };

    Source source = Source::all;
    yup::File folder; /**< Source::folder: this folder and every subfolder. */
    yup::StringArray categories; /**< Any of them, case-insensitive; empty passes all. */
    yup::StringArray tags; /**< All of them, case-insensitive; empty passes all. */
    int minimumRating = 0;
    yup::String search; /**< Whitespace separated words, each matched loosely against any field. */
    SortKey sortKey = SortKey::name;
    bool ascending = true;

    bool operator== (const PresetQuery&) const = default;

    /** Returns the indices of the entries that pass, in list order.

        The sort key always orders the list; sorting by relevance puts the better search
        matches first. Ties fall back to the name, then the key.
    */
    std::vector<std::size_t> run (const std::vector<PresetEntry>& entries, const PresetUserData& userData) const;

    /** Returns every category found, without empty ones or case duplicates, in natural order. */
    static yup::StringArray collectCategories (const std::vector<PresetEntry>& entries);

    /** Returns every tag found, without case duplicates, in natural order. */
    static yup::StringArray collectTags (const std::vector<PresetEntry>& entries);
};
