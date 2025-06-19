/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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
FileChooser::FileChooser (const String& dialogBoxTitle,
                          const File& initialFileOrDirectory,
                          const String& filePatternsAllowed,
                          bool useOSNativeDialogBox,
                          bool treatFilePackagesAsDirs)
    : title (dialogBoxTitle)
    , filters (filePatternsAllowed)
    , startingFile (initialFileOrDirectory)
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
bool FileChooser::browseForFileToOpen (Component* previewComponent)
{
    return showDialog (openMode | canSelectFiles, previewComponent);
}

bool FileChooser::browseForMultipleFilesToOpen (Component* previewComponent)
{
    return showDialog (openMode | canSelectFiles | canSelectMultipleItems, previewComponent);
}

bool FileChooser::browseForMultipleFilesOrDirectoriesToOpen (Component* previewComponent)
{
    return showDialog (openMode | canSelectFiles | canSelectDirectories | canSelectMultipleItems, previewComponent);
}

bool FileChooser::browseForFileToSave (bool warnAboutOverwritingExistingFiles)
{
    return showDialog (saveMode | canSelectFiles | (warnAboutOverwritingExistingFiles ? warnAboutOverwriting : 0), nullptr);
}

bool FileChooser::browseForDirectory()
{
    return showDialog (openMode | canSelectDirectories, nullptr);
}

bool FileChooser::showDialog (int flags, Component* previewComponent)
{
    YUP_ASSERT_MESSAGE_THREAD

    results.clear();

    showPlatformDialog (flags, previewComponent);

    return results.size() > 0;
}

//==============================================================================
File FileChooser::getResult() const
{
    return results.size() > 0 ? results[0] : File();
}

Array<File> FileChooser::getResults() const
{
    return results;
}

//==============================================================================
String FileChooser::getFilePatternsForPlatform() const
{
    if (filters.isEmpty())
        return String();

    return filters.replaceCharacter (',', ';');
}

} // namespace yup
