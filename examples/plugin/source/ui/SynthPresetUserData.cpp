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
    return SynthSettings::get().favorites.contains (key);
}

void SynthPresetUserData::setFavorite (const yup::String& key, bool shouldBeFavorite)
{
    auto& favorites = SynthSettings::get().favorites;

    if (favorites.contains (key) == shouldBeFavorite)
        return;

    if (shouldBeFavorite)
        favorites.add (key);
    else
        favorites.removeString (key);

    SynthSettings::save();
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

    if (getRating (key) == stars)
        return;

    auto& ratings = SynthSettings::get().ratings;

    if (stars == 0)
        ratings.erase (key);
    else
        ratings[key] = stars;

    SynthSettings::save();
    sendChangeMessage();
}
