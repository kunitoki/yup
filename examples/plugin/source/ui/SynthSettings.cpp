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
constexpr const char* favoriteTag = "FAVORITE";
constexpr const char* ratingTag = "RATING";
constexpr const char* folderTag = "FOLDER";

yup::File getFile()
{
    return yup::File::getSpecialLocation (yup::File::userPreferencesDirectory)
        .getChildFile (yup::File::createLegalFileName (YupPlugin_Vendor))
        .getChildFile (yup::File::createLegalFileName (YupPlugin_Name))
        .getChildFile ("settings.xml");
}

/** The global switches by name, so the file stays readable and survives new switches. */
std::vector<std::pair<const char*, bool*>> getSwitches (Values& values)
{
    return { { "undo", &values.undo }, { "mcpServer", &values.mcpServer } };
}

std::unique_ptr<yup::XmlElement> readFile()
{
    auto xml = yup::XmlDocument::parse (getFile());
    return xml != nullptr && xml->hasTagName (rootTag) ? std::move (xml) : nullptr;
}

void readBrowserData (const yup::XmlElement& xml, Values& values)
{
    values.favorites.clear();
    values.ratings.clear();
    values.presetFolders.clear();

    for (const auto* favorite : xml.getChildWithTagNameIterator (favoriteTag))
        values.favorites.insert (favorite->getStringAttribute ("key"));

    for (const auto* rating : xml.getChildWithTagNameIterator (ratingTag))
    {
        if (const auto stars = yup::jlimit (0, 5, rating->getIntAttribute ("stars")); stars > 0)
            values.ratings[rating->getStringAttribute ("key")] = stars;
    }

    for (const auto* folder : xml.getChildWithTagNameIterator (folderTag))
        values.presetFolders.addIfNotAlreadyThere (folder->getStringAttribute ("path"));
}

/** Reads the settings, keeping the default of anything the file does not hold. */
Values load()
{
    Values values;
    const auto xml = readFile();

    if (xml == nullptr)
        return values;

    // Files written before the view options were per instance hold them on the root.
    const auto* view = xml->getChildByName (SynthViewOptions::treeType.toString());
    values.defaults = SynthViewOptions::fromDataTree (yup::DataTree::fromXml (view != nullptr ? *view : *xml), values.defaults);

    for (const auto& [name, flag] : getSwitches (values))
        *flag = xml->getBoolAttribute (name, *flag);

    readBrowserData (*xml, values);
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
    for (const auto& [name, flag] : getSwitches (values))
        xml.setAttribute (name, *flag ? 1 : 0);

    if (auto view = values.defaults.toDataTree().createXml())
        xml.addChildElement (view.release());

    for (const auto& key : values.favorites)
        xml.createNewChildElement (favoriteTag)->setAttribute ("key", key);

    for (const auto& [key, stars] : values.ratings)
    {
        auto* rating = xml.createNewChildElement (ratingTag);
        rating->setAttribute ("key", key);
        rating->setAttribute ("stars", stars);
    }

    for (const auto& path : values.presetFolders)
        xml.createNewChildElement (folderTag)->setAttribute ("path", path);

    const auto file = getFile();
    file.getParentDirectory().createDirectory();
    xml.writeTo (file);
}

bool updateBrowserData (const std::function<bool (Values&)>& change)
{
    auto& values = get();

    if (const auto xml = readFile())
        readBrowserData (*xml, values);

    if (! change (values))
        return false;

    save();
    return true;
}
} // namespace SynthSettings
