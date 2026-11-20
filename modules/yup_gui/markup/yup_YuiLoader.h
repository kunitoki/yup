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

    Every load goes through YuiDocument::loadFromString, so a reload updates the interface
    in place and a failing one keeps the last good interface. On desktop platforms, a file
    source can be watched: when the main file or one of its imports changes on disk, the
    document is reloaded on the message thread.

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

    /** Destroys the loader, stopping any watching. The document is kept. */
    ~YuiLoader() override;

    //==============================================================================
    /** Loads a `.yui` file, whose imports are paths relative to its folder.

        @returns A failure when the file can't be read or the document can't be loaded.
    */
    Result loadFile (const File& file);

    /** Loads an embedded `.yui` file, finding it and its imports with a resource finder.

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

    /** Loads the current source again. */
    Result reload();

    //==============================================================================
    /** Uses a text in place of a file of the current source, until another source is loaded.

        This suits an editor showing unsaved changes: the loads read the overridden files from
        memory and the others from the source, and watching ignores the overridden files.

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

    String getMainPath() const;
    std::optional<String> resolve (const String& path);
    void watchFiles();
    void timerCallback() override;

    YuiDocument& document;
    File file;
    String resourcePath;
    ResourceFinder findResource;
    std::map<String, String> overrides;
    std::vector<LoadedFile> loadedFiles;

#if YUP_LINUX || YUP_WINDOWS || YUP_MAC
    std::shared_ptr<Watchdog> watchdog;
#endif

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YuiLoader)
};

} // namespace yup
