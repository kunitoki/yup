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

#include "SynthSettings.h"

namespace SynthSettings
{
namespace
{
yup::File getFile()
{
    return yup::File::getSpecialLocation (yup::File::userApplicationDataDirectory)
        .getChildFile ("Prism")
        .getChildFile ("Settings.txt");
}

/** The switches by name, so the file stays readable and survives new switches. */
std::vector<std::pair<const char*, bool*>> getSwitches (Values& values)
{
    auto& neon = values.neon;

    return { { "postEffect", &values.postEffect }, { "glow", &neon.glow }, { "aberration", &neon.aberration },
             { "grade", &neon.grade }, { "scanlines", &neon.scanlines }, { "vignette", &neon.vignette },
             { "grain", &neon.grain }, { "curvature", &neon.curvature }, { "phosphor", &neon.phosphor } };
}

Values load()
{
    Values values;
    const auto switches = getSwitches (values);

    for (const auto& line : yup::StringArray::fromLines (getFile().loadFileAsString()))
    {
        const auto key = line.upToFirstOccurrenceOf ("=", false, false).trim();
        const auto value = line.fromFirstOccurrenceOf ("=", false, false).trim();

        if (key == "theme")
        {
            values.theme = value;
            continue;
        }

        if (key == "frameRate")
        {
            values.frameRate = value.getIntValue();
            continue;
        }

        for (const auto& [name, flag] : switches)
            if (key == name)
                *flag = value.getIntValue() != 0;
    }

    return values;
}
} // namespace

Values& get()
{
    static Values values = load();
    return values;
}

void save()
{
    auto& values = get();
    yup::StringArray lines;
    lines.add ("theme=" + values.theme);
    lines.add ("frameRate=" + yup::String (values.frameRate));

    for (const auto& [name, flag] : getSwitches (values))
        lines.add (yup::String (name) + "=" + (*flag ? "1" : "0"));

    const auto file = getFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText (lines.joinIntoString ("\n"));
}
} // namespace SynthSettings
