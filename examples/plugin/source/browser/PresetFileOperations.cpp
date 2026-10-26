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

PresetEntry withFile (const PresetEntry& entry, const yup::File& file, const yup::String& name)
{
    auto result = entry;
    result.key = file.getFullPathName();
    result.file = file;
    result.name = name;
    return result;
}
} // namespace

yup::File getFileFor (const yup::File& folder, const yup::String& name, const PresetFormat& format)
{
    const auto fileName = yup::File::createLegalFileName (name).trim();

    if (fileName.isEmpty())
        return {};

    return folder.getChildFile (fileName + format.extension);
}

yup::ResultValue<yup::File> rename (const PresetEntry& entry, const yup::String& newName, const PresetFormat& format, PresetUserData& userData)
{
    if (entry.isFactory())
        return yup::makeResultValueFail (factoryError);

    const auto name = newName.trim();

    if (name.isEmpty())
        return yup::makeResultValueFail ("A preset needs a name");

    const auto target = getFileFor (entry.file.getParentDirectory(), name, format);

    if (target == yup::File())
        return yup::makeResultValueFail ("The name has no characters a file name can use");

    if (target == entry.file)
    {
        if (const auto result = format.writeMetadata (entry.file, withFile (entry, entry.file, name)); result.failed())
            return yup::makeResultValueFail (result.getErrorMessage());

        return yup::makeResultValueOk (entry.file);
    }

    if (target.exists())
        return yup::makeResultValueFail ("A preset called '" + name + "' already exists");

    if (! entry.file.moveFileTo (target))
        return yup::makeResultValueFail ("Can't rename " + entry.file.getFullPathName());

    if (const auto result = format.writeMetadata (target, withFile (entry, target, name)); result.failed())
    {
        target.moveFileTo (entry.file);
        return yup::makeResultValueFail (result.getErrorMessage());
    }

    userData.copyKey (entry.key, target.getFullPathName());
    userData.forgetKey (entry.key);
    return yup::makeResultValueOk (target);
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

    if (const auto result = format.writeMetadata (target, withFile (entry, target, target.getFileNameWithoutExtension())); result.failed())
    {
        target.deleteFile();
        return yup::makeResultValueFail (result.getErrorMessage());
    }

    userData.copyKey (entry.key, target.getFullPathName());
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

yup::Result writeMetadata (const PresetEntry& edited, const PresetFormat& format)
{
    if (edited.isFactory())
        return yup::Result::fail (factoryError);

    return format.writeMetadata (edited.file, edited);
}
} // namespace PresetFileOperations
