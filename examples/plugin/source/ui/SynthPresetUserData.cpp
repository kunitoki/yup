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

#include "SynthPresetUserData.h"
#include "SynthSettings.h"

bool SynthPresetUserData::isFavorite (const yup::String& key) const
{
    return SynthSettings::get().favorites.count (key) > 0;
}

void SynthPresetUserData::setFavorite (const yup::String& key, bool shouldBeFavorite)
{
    const auto changed = SynthSettings::updateBrowserData ([&] (SynthSettings::Values& values)
    {
        if ((values.favorites.count (key) > 0) == shouldBeFavorite)
            return false;

        if (shouldBeFavorite)
            values.favorites.insert (key);
        else
            values.favorites.erase (key);

        return true;
    });

    if (changed)
        sendChangeMessage();
}

int SynthPresetUserData::getRating (const yup::String& key) const
{
    const auto& ratings = SynthSettings::get().ratings;
    const auto found = ratings.find (key);
    return found != ratings.end() ? found->second : 0;
}

void SynthPresetUserData::setRating (const yup::String& key, int stars)
{
    stars = yup::jlimit (0, 5, stars);

    const auto changed = SynthSettings::updateBrowserData ([&] (SynthSettings::Values& values)
    {
        if (getRating (key) == stars)
            return false;

        if (stars == 0)
            values.ratings.erase (key);
        else
            values.ratings[key] = stars;

        return true;
    });

    if (changed)
        sendChangeMessage();
}
