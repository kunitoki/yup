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

#include "PresetQuery.h"
#include "PresetFuzzyMatch.h"

#include <algorithm>
#include <optional>

namespace
{
struct PresetCandidate
{
    std::size_t index = 0;
    int score = 0;
    bool favorite = false;
    int rating = 0;
};

std::optional<int> scoreSearchWord (const PresetEntry& entry, const yup::String& word)
{
    std::optional<int> best;

    const auto consider = [&] (const yup::String& field, int weight)
    {
        if (const auto score = PresetFuzzyMatch::match (word, field); score.has_value() && (! best.has_value() || *score * weight > *best))
            best = *score * weight;
    };

    consider (entry.name, 2);
    consider (entry.category, 1);
    consider (entry.author, 1);
    consider (entry.description, 1);

    for (const auto& tag : entry.tags)
        consider (tag, 1);

    return best;
}

std::optional<int> scoreSearch (const PresetEntry& entry, const yup::StringArray& words)
{
    int total = 0;

    for (const auto& word : words)
    {
        const auto score = scoreSearchWord (entry, word);

        if (! score.has_value())
            return std::nullopt;

        total += *score;
    }

    return total;
}

bool passesSource (const PresetQuery& query, const PresetEntry& entry, bool favorite)
{
    switch (query.source)
    {
        case PresetQuery::Source::all:
            return true;
        case PresetQuery::Source::favorites:
            return favorite;
        case PresetQuery::Source::factory:
            return entry.isFactory();
        case PresetQuery::Source::folder:
            return ! entry.isFactory() && entry.file.isAChildOf (query.folder);
    }

    return false;
}

bool containsAllTags (const yup::StringArray& tags, const yup::StringArray& wanted)
{
    return std::all_of (wanted.begin(), wanted.end(), [&] (const yup::String& tag)
    {
        return tags.contains (tag, true);
    });
}

template <typename T>
int compareValues (const T& a, const T& b)
{
    return a < b ? -1 : (b < a ? 1 : 0);
}

int compareBySortKey (PresetQuery::SortKey sortKey, const PresetCandidate& a, const PresetEntry& entryA, const PresetCandidate& b, const PresetEntry& entryB)
{
    switch (sortKey)
    {
        case PresetQuery::SortKey::favorite:
            return compareValues (! a.favorite, ! b.favorite);
        case PresetQuery::SortKey::name:
            return entryA.name.compareNatural (entryB.name);
        case PresetQuery::SortKey::category:
            return entryA.category.compareNatural (entryB.category);
        case PresetQuery::SortKey::author:
            return entryA.author.compareNatural (entryB.author);
        case PresetQuery::SortKey::location:
            return entryA.location.compareNatural (entryB.location);
        case PresetQuery::SortKey::modified:
            return compareValues (entryA.modified.toMilliseconds(), entryB.modified.toMilliseconds());
        case PresetQuery::SortKey::rating:
            return compareValues (a.rating, b.rating);
        case PresetQuery::SortKey::relevance:
            return compareValues (b.score, a.score);
    }

    return 0;
}
} // namespace

std::vector<std::size_t> PresetQuery::run (const std::vector<PresetEntry>& entries, const PresetUserData& userData) const
{
    auto words = yup::StringArray::fromTokens (search, false);
    words.removeEmptyStrings();

    std::vector<PresetCandidate> candidates;

    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        const auto& entry = entries[index];
        const auto favorite = userData.isFavorite (entry.key);
        const auto rating = userData.getRating (entry.key);

        if (! passesSource (*this, entry, favorite) || rating < minimumRating)
            continue;

        if (! categories.isEmpty() && ! categories.contains (entry.category, true))
            continue;

        if (! containsAllTags (entry.tags, tags))
            continue;

        const auto score = scoreSearch (entry, words);

        if (! score.has_value())
            continue;

        candidates.push_back ({ index, *score, favorite, rating });
    }

    std::sort (candidates.begin(), candidates.end(), [&] (const PresetCandidate& a, const PresetCandidate& b)
    {
        const auto& entryA = entries[a.index];
        const auto& entryB = entries[b.index];
        auto order = compareBySortKey (sortKey, a, entryA, b, entryB);

        if (order == 0)
            order = entryA.name.compareNatural (entryB.name);

        if (order == 0)
            order = entryA.key.compare (entryB.key);

        return ascending ? order < 0 : order > 0;
    });

    std::vector<std::size_t> result;
    result.reserve (candidates.size());

    for (const auto& candidate : candidates)
        result.push_back (candidate.index);

    return result;
}

yup::StringArray PresetQuery::collectCategories (const std::vector<PresetEntry>& entries)
{
    yup::StringArray result;

    for (const auto& entry : entries)
    {
        if (entry.category.isNotEmpty())
            result.addIfNotAlreadyThere (entry.category, true);
    }

    result.sortNatural();
    return result;
}

yup::StringArray PresetQuery::collectTags (const std::vector<PresetEntry>& entries)
{
    yup::StringArray result;

    for (const auto& entry : entries)
    {
        for (const auto& tag : entry.tags)
        {
            if (tag.isNotEmpty())
                result.addIfNotAlreadyThere (tag, true);
        }
    }

    result.sortNatural();
    return result;
}
