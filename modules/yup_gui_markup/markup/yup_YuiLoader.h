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

namespace yup
{

//==============================================================================
/**
    Loads a YuiDocument from a file or from embedded resources, and reloads it.

    The loader reads the main text and the files it imports from one source:

    - a file, whose imports are paths relative to its folder;
    - embedded resources, found by path with a ResourceFinder, such as the function
      `yup_add_yui_resources` generates in CMake;
    - a string, given to @ref reloadFromString, which stands for the main text of the
      current source and imports from it.

    Any file of the source can be overridden with a text held in memory (see
    @ref setOverride), so an editor can load its unsaved changes through the loader.

    Every load goes through the document (see YuiDocument::load), so a reload updates the
    interface in place and a failing one keeps the last good interface. A main text that
    can't be read fails the load as one that can't be parsed does: the document lists it as
    its problem and tells its listeners. On desktop platforms, a file source can be watched:
    when the main file or one of its imports changes on disk, the document is reloaded on the
    message thread.

    The source also holds the other files the document refers to, such as the source of a
    shader or a model: the loader sets the asset reader of its document, which reads them from
    the folder of the main file, from the resource finder or from an override (see
    YuiDocument::readAsset). Each reload, and each change noticed on disk while watching, tells
    the document about the ones that are no longer what was read, before it loads, and loading
    another source about every one that was read (see YuiDocument::notifyAssetChanged).

    @code
    YuiDocument document;
    YuiLoader loader (document);

    loader.onLoad = [this] (const Result& result)
    {
        if (result.failed())
            DBG (result.getErrorMessage());
    };

   #if YUP_DEBUG
    loader.loadFile (sourceFolder.getChildFile ("main.yui"));
    loader.setWatching (true);
   #else
    loader.loadResource ("main.yui", UiResources_find);
   #endif

    addAndMakeVisible (*document.getRoot());
    @endcode

    @see YuiDocument, Watchdog

    @tags{UI}
*/
class YUP_API YuiLoader : private Timer
{
public:
    //==============================================================================
    /** Finds an embedded file by its path, returning its bytes and size, or nullptr. */
    using ResourceFinder = std::function<const uint8* (const char* path, std::size_t& size)>;

    //==============================================================================
    /** Creates a loader filling a document, which must outlive it. */
    explicit YuiLoader (YuiDocument& documentToLoad);

    /** Destroys the loader, stopping any watching and removing its asset reader. The document is kept. */
    ~YuiLoader() override;

    //==============================================================================
    /** Loads a `.yui` file, whose imports are paths relative to its folder.

        The interface is updated in place, and what is kept of it still has the assets it read
        from the source loaded before (see YuiDocument::readAsset). Each of them is therefore
        reported with YuiDocument::notifyAssetChanged before the document loads, for what read
        it to read it again from the folder of this file, even when the same file is loaded
        again. From then on, only the assets read again are reported when they change (see
        @ref reload).

        @returns A failure when the file can't be read or the document can't be loaded.
    */
    Result loadFile (const File& file);

    /** Loads an embedded `.yui` file, finding it and its imports with a resource finder.

        The assets read from the source loaded before are reported as @ref loadFile reports
        them, for what read them to read them again through the finder.

        @param path          The path of the main file, as known to the finder.
        @param findResource  Returns the bytes of each path.

        @returns A failure when the file isn't found or the document can't be loaded.
    */
    Result loadResource (const String& path, ResourceFinder findResource);

    /** Loads text in place of the main text of the current source, importing from the source.

        The text stands for the main file until another source is loaded, @ref reload
        included: it is an override of the main file (see @ref setOverride).
    */
    Result reloadFromString (const String& text);

    /** Loads the current source again.

        The assets the document read through the loader are looked at first (see
        YuiDocument::readAsset). The ones that changed since they were read are reported with
        YuiDocument::notifyAssetChanged, for what read them to read them again: a file of another
        modification time or size, an asset whose override was set, changed or removed. An asset
        of embedded resources only changes with its override.
    */
    Result reload();

    //==============================================================================
    /** Uses a text in place of a file of the current source, until another source is loaded.

        This suits an editor showing unsaved changes: the loads read the overridden files from
        memory and the others from the source, and watching ignores the overridden files.

        A file the document reads with YuiDocument::readAsset is overridden the same way, the
        text standing for its bytes. The next load tells what already read it to read it again
        (see @ref reload).

        @param path  The path of the file, relative to the main file's folder as imports are,
                     so the main file is its own name (the resource path for resources).
        @param text  The text to load for the file.

        @see clearOverrides, reload
    */
    void setOverride (const String& path, const String& text);

    /** Reads every file from the source again from the next load. */
    void clearOverrides();

    /** Returns the text a file had in the last load, overridden or read, or nullopt when the
        last load didn't use the file or couldn't read it.

        @param path  The path of the file, as for @ref setOverride.
    */
    std::optional<String> getLoadedText (const String& path) const;

    //==============================================================================
    /** Returns the file loaded, or a default File when the source isn't a file. */
    File getFile() const { return file; }

    //==============================================================================
    /** Starts or stops reloading the document when the files it was loaded from change.

        Watching only has an effect for a file source on desktop platforms. A change is
        noticed through a Watchdog, and the document is reloaded on the message thread when
        the text of the main file or of one of its imports differs from the text loaded.

        The files read with YuiDocument::readAsset are watched too. When the modification time
        or the size of one changes, its listeners are told with YuiDocument::notifyAssetChanged,
        as @ref reload tells them, and what read the file reads it again. The document is then
        loaded again, which keeps the interface as it is when its texts did not change: the
        problems it lists are the ones that still stand, so one of a file saved broken goes
        once the file is fixed. As when the main file changes, the text loaded replaces what
        was changed in the tree of the document since, an edit made to it or one undone.
    */
    void setWatching (bool shouldWatch);

    /** Returns true when watching was started on a platform supporting it. */
    bool isWatching() const noexcept;

    //==============================================================================
    /** Called after each load, including the reloads started by watching, with its result. */
    std::function<void (const Result&)> onLoad;

private:
    //==============================================================================
    struct LoadedFile
    {
        String path;
        File file; // Only for the files read from disk, which are the ones watched
        std::optional<String> text;
    };

    // An asset the document read, as the source had it when first read or last reported as changed
    struct LoadedAsset
    {
        String path;       // As the document gave it, which is how its listeners are told about it
        String sourcePath; // In the source, where its override and its resource are looked for
        File file;         // Only for a file source
        std::optional<String> overrideText;

        // Of the file, which isn't looked at while the asset is overridden
        Time modified;
        int64 size = 0;

        // Set when a later read finds something else than this: what read that is told at the next report too,
        // even if the source is back to this by then
        bool needsReport = false;

        bool operator== (const LoadedAsset&) const = default;
    };

    Result loadSource();
    String getMainPath() const;
    std::optional<String> resolve (const String& path);
    LoadedAsset findAsset (const String& path) const;
    std::optional<MemoryBlock> readAsset (const String& path);
    bool reportChangedAssets();
    void watchFiles();
    void timerCallback() override;

    YuiDocument& document;
    File file;
    String resourcePath;
    ResourceFinder findResource;
    std::map<String, String> overrides;
    std::vector<LoadedFile> loadedFiles;
    std::vector<LoadedAsset> loadedAssets;

#if YUP_LINUX || YUP_WINDOWS || YUP_MAC
    std::shared_ptr<Watchdog> watchdog;
#endif

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiLoader)
};

} // namespace yup
