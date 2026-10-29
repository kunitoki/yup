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

#include "PresetFileOperations.h"

namespace PresetFileOperations
{
namespace
{
const char* const factoryError = "Factory presets can't be changed";

/** The entry as it is about to be written to a file: with that file and name, and an id,
    which is also its key from now on. */
PresetEntry prepareForFile (const PresetEntry& entry, const yup::File& file, const yup::String& name, bool needsNewId)
{
    auto result = entry;
    result.file = file;
    result.name = name;

    if (needsNewId || result.id.isEmpty())
        result.id = yup::Uuid().toDashedString();

    result.key = result.id;
    return result;
}

void moveMarks (PresetUserData& userData, const yup::String& fromKey, const yup::String& toKey)
{
    if (fromKey == toKey)
        return;

    userData.copyKey (fromKey, toKey);
    userData.forgetKey (fromKey);
}
} // namespace

yup::File getFileFor (const yup::File& folder, const yup::String& name, const PresetFormat& format)
{
    auto fileName = yup::File::createLegalFileName (name).trim();

    if (fileName.endsWithIgnoreCase (format.extension))
        fileName = fileName.dropLastCharacters (format.extension.length()).trimEnd();

    if (fileName.isEmpty())
        return {};

    return folder.getChildFile (fileName + format.extension);
}

yup::ResultValue<PresetEntry> rename (const PresetEntry& entry, const yup::String& newName, const PresetFormat& format, PresetUserData& userData)
{
    if (entry.isFactory())
        return yup::makeResultValueFail (factoryError);

    const auto name = newName.trim();

    if (name.isEmpty())
        return yup::makeResultValueFail ("A preset needs a name");

    const auto target = getFileFor (entry.file.getParentDirectory(), name, format);

    if (target == yup::File())
        return yup::makeResultValueFail ("The name has no characters a file name can use");

    const auto storedName = target.getFileNameWithoutExtension();

    if (target != entry.file)
    {
        if (target.exists())
            return yup::makeResultValueFail ("A preset called '" + storedName + "' already exists");

        if (! entry.file.moveFileTo (target))
            return yup::makeResultValueFail ("Can't rename " + entry.file.getFullPathName());
    }

    auto renamed = prepareForFile (entry, target != entry.file ? target : entry.file, storedName, false);

    if (const auto result = format.writeMetadata (renamed.file, renamed); result.failed())
    {
        if (renamed.file != entry.file)
            renamed.file.moveFileTo (entry.file);

        return yup::makeResultValueFail (result.getErrorMessage());
    }

    moveMarks (userData, entry.key, renamed.key);
    return yup::makeResultValueOk (std::move (renamed));
}

yup::ResultValue<yup::File> duplicate (const PresetEntry& entry, const PresetFormat& format, PresetUserData& userData)
{
    if (entry.isFactory())
        return yup::makeResultValueFail (factoryError);

    auto target = getFileFor (entry.file.getParentDirectory(), entry.name + " copy", format);

    if (target == yup::File())
        return yup::makeResultValueFail ("The name has no characters a file name can use");

    target = target.getNonexistentSibling();

    if (! entry.file.copyFileTo (target))
        return yup::makeResultValueFail ("Can't copy " + entry.file.getFullPathName());

    const auto copy = prepareForFile (entry, target, target.getFileNameWithoutExtension(), true);

    if (const auto result = format.writeMetadata (target, copy); result.failed())
    {
        target.deleteFile();
        return yup::makeResultValueFail (result.getErrorMessage());
    }

    userData.copyKey (entry.key, copy.key);
    return yup::makeResultValueOk (target);
}

yup::Result remove (const PresetEntry& entry, PresetUserData& userData)
{
    if (entry.isFactory())
        return yup::Result::fail (factoryError);

    if (! entry.file.deleteFile())
        return yup::Result::fail ("Can't delete " + entry.file.getFullPathName());

    userData.forgetKey (entry.key);
    return yup::Result::ok();
}

yup::ResultValue<PresetEntry> writeMetadata (const PresetEntry& edited, const PresetFormat& format, PresetUserData& userData)
{
    if (edited.isFactory())
        return yup::makeResultValueFail (factoryError);

    auto written = prepareForFile (edited, edited.file, edited.name, false);

    if (const auto result = format.writeMetadata (written.file, written); result.failed())
        return yup::makeResultValueFail (result.getErrorMessage());

    moveMarks (userData, edited.key, written.key);
    return yup::makeResultValueOk (std::move (written));
}
} // namespace PresetFileOperations
