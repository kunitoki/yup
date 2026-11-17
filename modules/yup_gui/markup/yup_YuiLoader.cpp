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

namespace yup
{

namespace
{

std::optional<String> readYuiFile (const File& file)
{
    if (! file.existsAsFile())
        return std::nullopt;

    return file.loadFileAsString();
}

} // namespace

//==============================================================================
YuiLoader::YuiLoader (YuiDocument& documentToLoad)
    : document (documentToLoad)
{
}

YuiLoader::~YuiLoader() = default;

//==============================================================================
Result YuiLoader::loadFile (const File& fileToLoad)
{
    file = fileToLoad;
    resourcePath = {};
    findResource = nullptr;
    overrides.clear();
    document.setBaseDirectory (fileToLoad != File() ? fileToLoad.getParentDirectory() : File());

    return reload();
}

Result YuiLoader::loadResource (const String& path, ResourceFinder finder)
{
    file = File();
    resourcePath = detail::normalizeYuiPath (path);
    findResource = std::move (finder);
    overrides.clear();
    document.setBaseDirectory ({});

    return reload();
}

Result YuiLoader::reloadFromString (const String& text)
{
    setOverride (getMainPath(), text);

    return reload();
}

Result YuiLoader::reload()
{
    loadedFiles.clear();

    const auto mainPath = getMainPath();
    const auto text = resolve (mainPath);

    const auto resolveImport = [this] (const String& path)
    {
        if (file == File() && findResource != nullptr)
            return resolve (detail::resolveRelativeYuiPath (resourcePath, path));

        return resolve (detail::normalizeYuiPath (path));
    };

    const auto result = text.has_value()
                          ? document.loadFromString (*text, resolveImport)
                          : Result::fail ("Can't load '" + (file != File() ? file.getFullPathName() : resourcePath) + "'");

    watchFiles();

    if (onLoad != nullptr)
        onLoad (result);

    return result;
}

//==============================================================================
void YuiLoader::setOverride (const String& path, const String& text)
{
    overrides[detail::normalizeYuiPath (path)] = text;
}

void YuiLoader::clearOverrides()
{
    overrides.clear();
}

std::optional<String> YuiLoader::getLoadedText (const String& path) const
{
    const auto normalized = detail::normalizeYuiPath (path);

    for (const auto& loaded : loadedFiles)
        if (loaded.path == normalized)
            return loaded.text;

    return std::nullopt;
}

//==============================================================================
String YuiLoader::getMainPath() const
{
    return file != File() ? detail::normalizeYuiPath (file.getFileName()) : resourcePath;
}

std::optional<String> YuiLoader::resolve (const String& path)
{
    const auto normalized = detail::normalizeYuiPath (path);

    for (const auto& existing : loadedFiles)
        if (existing.path == normalized)
            return existing.text;

    auto& loaded = loadedFiles.emplace_back();
    loaded.path = normalized;

    if (const auto foundOverride = overrides.find (normalized); foundOverride != overrides.end())
    {
        loaded.text = foundOverride->second;
    }
    else if (file != File())
    {
        loaded.file = file.getParentDirectory().getChildFile (normalized);
        loaded.text = readYuiFile (loaded.file);
    }
    else if (findResource != nullptr)
    {
        std::size_t size = 0;

        if (const auto* data = findResource (normalized.toRawUTF8(), size))
            loaded.text = String::fromUTF8 (reinterpret_cast<const char*> (data), static_cast<int> (size));
    }

    return loaded.text;
}

//==============================================================================
void YuiLoader::setWatching ([[maybe_unused]] bool shouldWatch)
{
#if YUP_LINUX || YUP_WINDOWS || YUP_MAC
    if (shouldWatch == isWatching())
        return;

    if (! shouldWatch)
    {
        stopTimer();
        watchdog.reset();
        return;
    }

    watchdog = Watchdog::createInstance (std::chrono::milliseconds (50));
    watchFiles();
    startTimer (100);
#endif
}

bool YuiLoader::isWatching() const noexcept
{
#if YUP_LINUX || YUP_WINDOWS || YUP_MAC
    return watchdog != nullptr;
#else
    return false;
#endif
}

void YuiLoader::watchFiles()
{
#if YUP_LINUX || YUP_WINDOWS || YUP_MAC
    if (watchdog == nullptr)
        return;

    Array<File> folders;

    for (const auto& loaded : loadedFiles)
    {
        if (loaded.file != File())
            folders.addIfNotAlreadyThere (loaded.file.getParentDirectory());
    }

    for (const auto& folder : watchdog->getAllWatchedFolders())
    {
        if (! folders.contains (folder))
            watchdog->unwatchFolder (folder);
    }

    for (const auto& folder : folders)
        watchdog->watchFolder (folder);
#endif
}

void YuiLoader::timerCallback()
{
#if YUP_LINUX || YUP_WINDOWS || YUP_MAC
    if (watchdog == nullptr || watchdog->dispatchEvents (nullptr) == 0)
        return;

    // Events may be about other files, or about saves which didn't change the text
    const auto hasChanged = std::any_of (loadedFiles.begin(), loadedFiles.end(), [] (const auto& loaded)
    {
        return loaded.file != File() && readYuiFile (loaded.file) != loaded.text;
    });

    if (hasChanged)
        reload();
#endif
}

} // namespace yup
