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

#include <yup_events/yup_events.h>

//==============================================================================
/** The user's own marks on presets, favorites and ratings, keyed by PresetEntry::key.

    They belong to the user rather than to a preset, so the storage is left to the
    implementation. Implementations call sendChangeMessage() on every change.

    @see PresetEntry, PresetQuery
*/
class PresetUserData : public yup::ChangeBroadcaster
{
public:
    /** True when the preset is a favorite. */
    virtual bool isFavorite (const yup::String& key) const = 0;

    /** Marks or unmarks the preset as a favorite. */
    virtual void setFavorite (const yup::String& key, bool shouldBeFavorite) = 0;

    /** Returns the preset's rating, from 0 (unrated) to 5 stars. */
    virtual int getRating (const yup::String& key) const = 0;

    /** Sets the preset's rating, clamped to 0..5; 0 forgets it. */
    virtual void setRating (const yup::String& key, int stars) = 0;

    /** Gives the preset keyed to the favorite and rating of the preset keyed from. */
    void copyKey (const yup::String& from, const yup::String& to);

    /** Clears the preset's favorite and rating. */
    void forgetKey (const yup::String& key);
};
