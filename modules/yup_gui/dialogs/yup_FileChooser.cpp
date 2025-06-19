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

//==============================================================================
#if !YUP_LINUX
class FileChooserImpl
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
    YUP_ASSERT_MESSAGE_THREAD

    // Set additional flags based on construction parameters
    if (packageDirsAsFiles)
        flags |= treatFilePackagesAsDirs;

    if (useNativeDialogBox)
    {
        showPlatformDialog (createCapturingCallback (std::move (callback)), flags);
    }
    else
    {
        // TODO: Implement YUP-based file browser when needed
        // For now, fall back to platform dialog
        showPlatformDialog (createCapturingCallback (std::move (callback)), flags);
    }
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
    return [self = Ptr { this }, callback = std::move (callback)] (bool success, const Array<File>& results)
    {
        callback (success, results);
    };
}

} // namespace yup
