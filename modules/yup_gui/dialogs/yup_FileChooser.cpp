/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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

CriticalSection& getActiveFileChoosersLock()
{
    static CriticalSection lock;
    return lock;
}

std::vector<FileChooser::Ptr>& getActiveFileChoosers()
{
    static std::vector<FileChooser::Ptr> activeChoosers;
    return activeChoosers;
}

} // namespace

//==============================================================================
#if ! YUP_LINUX && ! YUP_WINDOWS && ! YUP_ANDROID
class FileChooser::FileChooserImpl
{
public:
    virtual ~FileChooserImpl() = default;
};
#endif

//==============================================================================
FileChooser::Ptr FileChooser::create (const String& dialogBoxTitle,
                                      const File& initialFileOrDirectory,
                                      const String& filePatternsAllowed,
                                      bool useOSNativeDialogBox,
                                      bool treatFilePackagesAsDirs)
{
    return { new FileChooser (dialogBoxTitle, initialFileOrDirectory, filePatternsAllowed, useOSNativeDialogBox, treatFilePackagesAsDirs) };
}

//==============================================================================
FileChooser::FileChooser (const String& dialogBoxTitle,
                          const File& initialFileOrDirectory,
                          const String& filePatternsAllowed,
                          bool useOSNativeDialogBox,
                          bool treatFilePackagesAsDirs)
    : title (dialogBoxTitle)
    , startingFile (initialFileOrDirectory)
    , filters (filePatternsAllowed)
    , useNativeDialogBox (useOSNativeDialogBox)
    , packageDirsAsFiles (treatFilePackagesAsDirs)
{
    if (startingFile == File())
    {
        startingFile = File::getSpecialLocation (File::userHomeDirectory);
    }
    else if (startingFile.existsAsFile())
    {
        startingFile = startingFile.getParentDirectory();
    }
}

FileChooser::~FileChooser() = default;

//==============================================================================
void FileChooser::browseForFileToOpen (CompletionCallback callback)
{
    int flags = openMode | canSelectFiles;
    showDialog (std::move (callback), flags);
}

void FileChooser::browseForMultipleFilesToOpen (CompletionCallback callback)
{
    int flags = openMode | canSelectFiles | canSelectMultipleItems;
    showDialog (std::move (callback), flags);
}

void FileChooser::browseForMultipleFilesOrDirectoriesToOpen (CompletionCallback callback)
{
    int flags = openMode | canSelectFiles | canSelectDirectories | canSelectMultipleItems;
    showDialog (std::move (callback), flags);
}

void FileChooser::browseForFileToSave (CompletionCallback callback, bool warnAboutOverwritingExistingFiles)
{
    int flags = saveMode | canSelectFiles;

    if (warnAboutOverwritingExistingFiles)
        flags |= warnAboutOverwriting;

    showDialog (std::move (callback), flags);
}

void FileChooser::browseForDirectory (CompletionCallback callback)
{
    int flags = openMode | canSelectDirectories;
    showDialog (std::move (callback), flags);
}

//==============================================================================
void FileChooser::showDialog (CompletionCallback callback, int flags)
{
    // Set additional flags based on construction parameters
    if (packageDirsAsFiles)
        flags |= treatFilePackagesAsDirs;

    addToActiveFileChoosers();

    auto capturedCallback = createCapturingCallback (std::move (callback));
    WeakReference<FileChooser> weakThis (this);
    auto showOnMessageThread = [weakThis, flags, callback = std::move (capturedCallback)]() mutable
    {
        if (auto* self = weakThis.get())
        {
            Ptr retainedSelf (self);
            retainedSelf->showPlatformDialog (std::move (callback), flags);
        }
    };

    if (! MessageManager::existsAndIsCurrentThread())
    {
        if (! MessageManager::callAsync ([show = std::move (showOnMessageThread)]() mutable
        {
            show();
        }))
            removeFromActiveFileChoosers();

        return;
    }

    showOnMessageThread();
}

//==============================================================================
String FileChooser::getFilePatternsForPlatform() const
{
    return filters;
}

void FileChooser::invokeCallback (CompletionCallback callback, bool success, const Array<File>& results)
{
    if (callback)
    {
        // Invoke callback on the message thread for safety
        MessageManager::callAsync ([callback = std::move (callback), success, results]()
        {
            callback (success, results);
        });
    }
}

FileChooser::CompletionCallback FileChooser::createCapturingCallback (CompletionCallback callback)
{
    WeakReference<FileChooser> weakThis (this);

    return [weakThis, callback = std::move (callback)] (bool success, const Array<File>& results) mutable
    {
        auto* chooser = weakThis.get();
        if (chooser == nullptr)
            return;

        chooser->removeFromActiveFileChoosers();

        if (callback)
            callback (success, results);
    };
}

void FileChooser::addToActiveFileChoosers()
{
    static bool installShutdownCallback = []
    {
        MessageManager::getInstance()->registerShutdownCallback ([]
        {
            FileChooser::releaseAllActiveFileChoosers();
        });
        return true;
    }();

    ignoreUnused (installShutdownCallback);

    const ScopedLock lock (getActiveFileChoosersLock());
    getActiveFileChoosers().push_back (this);
}

void FileChooser::removeFromActiveFileChoosers()
{
    const ScopedLock lock (getActiveFileChoosersLock());
    auto& activeChoosers = getActiveFileChoosers();

    for (auto it = activeChoosers.begin(); it != activeChoosers.end();)
    {
        if (it->get() == this)
            it = activeChoosers.erase (it);
        else
            ++it;
    }
}

void FileChooser::releaseAllActiveFileChoosers()
{
    std::vector<FileChooser::Ptr> activeChoosers;

    {
        const ScopedLock lock (getActiveFileChoosersLock());
        activeChoosers.swap (getActiveFileChoosers());
    }

    for (auto& chooser : activeChoosers)
        chooser->impl.reset();
}

} // namespace yup
