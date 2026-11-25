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

YuiLoader::~YuiLoader()
{
    document.setAssetReader (nullptr);
}

//==============================================================================
Result YuiLoader::loadFile (const File& fileToLoad)
{
    file = fileToLoad;
    resourcePath = {};
    findResource = nullptr;
    overrides.clear();
    document.setBaseDirectory (fileToLoad != File() ? fileToLoad.getParentDirectory() : File());

    return loadSource();
}

Result YuiLoader::loadResource (const String& path, ResourceFinder finder)
{
    file = File();
    resourcePath = detail::normalizeYuiPath (path);
    findResource = std::move (finder);
    overrides.clear();
    document.setBaseDirectory ({});

    return loadSource();
}

Result YuiLoader::reloadFromString (const String& text)
{
    setOverride (getMainPath(), text);

    return reload();
}

Result YuiLoader::reload()
{
    // Before the load, so that what it creates reads the assets as they are now and isn't told about them again
    reportChangedAssets();

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
// Loads the source the loader was just given
Result YuiLoader::loadSource()
{
    document.setAssetReader ([this] (const String& assetPath)
    {
        return readAsset (assetPath);
    });

    // A load is in place, so what it keeps still has the assets of the source before: it is told about every one that
    // was read, before the load as a reload tells. The ones read again are then remembered for this source
    for (const auto& asset : std::exchange (loadedAssets, {}))
        document.notifyAssetChanged (asset.path);

    return reload();
}

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

// The asset as the source has it now: it changed when this isn't what is remembered of it
YuiLoader::LoadedAsset YuiLoader::findAsset (const String& path) const
{
    const auto isResource = file == File() && findResource != nullptr;

    LoadedAsset asset;
    asset.path = path;
    asset.sourcePath = isResource ? detail::resolveRelativeYuiPath (resourcePath, path) : detail::normalizeYuiPath (path);

    if (const auto foundOverride = overrides.find (asset.sourcePath); foundOverride != overrides.end())
        asset.overrideText = foundOverride->second;

    if (file == File())
        return asset;

    asset.file = file.getParentDirectory().getChildFile (asset.sourcePath);

    if (! asset.overrideText.has_value())
    {
        asset.modified = asset.file.getLastModificationTime();
        asset.size = asset.file.getSize();
    }

    return asset;
}

std::optional<MemoryBlock> YuiLoader::readAsset (const String& path)
{
    // An absolute path is no file of the source: the document reads it
    if (File::isAbsolutePath (path))
        return std::nullopt;

    const auto asset = findAsset (path);

    const auto known = std::find_if (loadedAssets.begin(), loadedAssets.end(), [&path] (const auto& candidate)
    {
        return candidate.path == path;
    });

    // Remembered the first time only, and as it is before being read: what read it earlier still has to be told
    // about a change, and one made meanwhile is still noticed. A later read of something else only marks it
    if (known == loadedAssets.end())
    {
        loadedAssets.push_back (asset);
        watchFiles();
    }
    else if (*known != asset)
    {
        known->needsReport = true;
    }

    if (asset.overrideText.has_value())
        return MemoryBlock (asset.overrideText->toRawUTF8(), asset.overrideText->getNumBytesAsUTF8());

    if (file != File())
    {
        MemoryBlock bytes;
        return asset.file.loadFileAsData (bytes) ? std::optional<MemoryBlock> (std::move (bytes)) : std::nullopt;
    }

    if (findResource == nullptr)
        return std::nullopt;

    std::size_t size = 0;
    const auto* data = findResource (asset.sourcePath.toRawUTF8(), size);

    return data != nullptr ? std::optional<MemoryBlock> (MemoryBlock (data, size)) : std::nullopt;
}

bool YuiLoader::reportChangedAssets()
{
    StringArray changed;

    for (auto& asset : loadedAssets)
    {
        auto current = findAsset (asset.path);

        // An asset marked by a read isn't the same either, whatever the source is back to
        if (current == asset)
            continue;

        asset = std::move (current);
        changed.add (asset.path);
    }

    // Told after the loop: what is told reads assets, and one read for the first time adds to those remembered
    for (const auto& path : changed)
        document.notifyAssetChanged (path);

    return ! changed.isEmpty();
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

    for (const auto& asset : loadedAssets)
    {
        if (asset.file != File())
            folders.addIfNotAlreadyThere (asset.file.getParentDirectory());
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

    // A changed asset is told first, for what read it to read it again. The document is then loaded again as for one of
    // its texts, so that what it lists as wrong is what still is: an asset saved broken, and no longer once it is fixed
    if (hasChanged || reportChangedAssets())
        reload();
#endif
}

} // namespace yup
