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

#include "SynthPresetFormat.h"

namespace SynthPresetFormat
{
namespace
{
yup::DataTree readPatch (const yup::File& file)
{
    const auto xml = yup::parseXML (file);

    if (xml == nullptr)
        return {};

    auto state = yup::DataTree::fromXml (*xml);
    return SynthPatch::isPatch (state) ? state : yup::DataTree();
}
} // namespace

PresetFormat create()
{
    PresetFormat format;
    format.extension = ".retrace";

    format.readEntry = [] (const yup::File& file) -> std::optional<PresetEntry>
    {
        const auto state = readPatch (file);

        if (! state.isValid())
            return std::nullopt;

        auto entry = fromInfo (SynthPatch::readInfo (state));

        if (entry.name.isEmpty())
            entry.name = file.getFileNameWithoutExtension();

        return entry;
    };

    format.writeMetadata = [] (const yup::File& file, const PresetEntry& entry)
    {
        auto state = readPatch (file);

        if (! state.isValid())
            return yup::Result::fail ("Not a Retrace preset: " + file.getFullPathName());

        auto info = SynthPatch::readInfo (state);
        info.name = entry.name;
        info.category = entry.category;
        info.author = entry.author;
        info.bank = entry.bank;
        info.description = entry.description;
        info.notes = entry.notes;
        info.tags = PresetEntry::joinTags (entry.tags);
        info.id = entry.id;
        SynthPatch::writeInfo (state, info);

        const auto xml = state.createXml();

        if (xml == nullptr || ! xml->writeTo (file))
            return yup::Result::fail ("Cannot write " + file.getFullPathName());

        return yup::Result::ok();
    };

    return format;
}

PresetEntry makeFactoryEntry (int index)
{
    auto entry = fromInfo (SynthPatch::getFactoryPresetInfo (index));
    entry.key = entry.id;
    entry.factoryIndex = index;
    entry.location = "Factory";
    return entry;
}

PresetEntry fromInfo (const SynthPatch::PresetInfo& info)
{
    PresetEntry entry;
    entry.name = info.name;
    entry.category = info.category;
    entry.author = info.author;
    entry.bank = info.bank;
    entry.description = info.description;
    entry.notes = info.notes;
    entry.tags = PresetEntry::parseTags (info.tags);
    entry.id = info.id;
    return entry;
}
} // namespace SynthPresetFormat
