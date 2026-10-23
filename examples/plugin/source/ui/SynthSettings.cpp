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
constexpr const char* rootTag = "SETTINGS";

yup::File getFile()
{
    return yup::File::getSpecialLocation (yup::File::userPreferencesDirectory)
        .getChildFile (yup::File::createLegalFileName (YupPlugin_Vendor))
        .getChildFile (yup::File::createLegalFileName (YupPlugin_Name))
        .getChildFile ("settings.xml");
}

/** The switches by name, so the file stays readable and survives new switches. */
std::vector<std::pair<const char*, bool*>> getSwitches (Values& values)
{
    auto& neon = values.neon;

    return { { "undo", &values.undo }, { "postEffect", &values.postEffect }, { "glow", &neon.glow }, { "aberration", &neon.aberration },
             { "grade", &neon.grade }, { "scanlines", &neon.scanlines }, { "vignette", &neon.vignette },
             { "grain", &neon.grain }, { "curvature", &neon.curvature }, { "phosphor", &neon.phosphor } };
}

/** Reads the settings, keeping the default of anything the file does not hold. */
Values load()
{
    Values values;
    const auto xml = yup::XmlDocument::parse (getFile());

    if (xml == nullptr || ! xml->hasTagName (rootTag))
        return values;

    values.theme = xml->getStringAttribute ("theme", values.theme);
    values.frameRate = xml->getIntAttribute ("frameRate", values.frameRate);

    for (const auto& [name, flag] : getSwitches (values))
        *flag = xml->getBoolAttribute (name, *flag);

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
    yup::XmlElement xml (rootTag);
    xml.setAttribute ("theme", values.theme);
    xml.setAttribute ("frameRate", values.frameRate);

    for (const auto& [name, flag] : getSwitches (values))
        xml.setAttribute (name, *flag ? 1 : 0);

    const auto file = getFile();
    file.getParentDirectory().createDirectory();
    xml.writeTo (file);
}
} // namespace SynthSettings
