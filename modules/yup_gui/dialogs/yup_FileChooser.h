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
/**
    A cross-platform file chooser dialog that allows users to select files or directories.

    This class provides a native file dialog experience on each platform, supporting:
    - Single file selection
    - Multiple file selection
    - File saving with optional default filename
    - Directory selection
    - File type filtering with extensions
    - Custom dialog titles and initial directories

    Example usage:
    @code
    // For opening a single file
    auto chooser = FileChooser::create ("Select an audio file", File::getSpecialLocation(File::userDocumentsDirectory), "*.wav;*.mp3;*.aiff");
    chooser->browseForFileToOpen ([](bool success, const Array<File>& results)
    {
        if (success && results.size() > 0)
        {
            File selectedFile = results[0];
            // Process the selected file...
        }
    });

    // For opening multiple files
    auto chooser = FileChooser::create ("Select audio files", File(), "*.wav;*.mp3");
    chooser->browseForMultipleFilesToOpen ([](bool success, const Array<File>& results)
    {
        if (success && results.size() > 0)
        {
            // Process the selected files...
            for (const auto& file : results)
            {
                // Process each file...
            }
        }
    });

    // For saving a file
    auto chooser = FileChooser::create ("Save project", File(), "*.proj", true);
    chooser->browseForFileToSave ([](bool success, const Array<File>& results)
    {
        if (success && results.size() > 0)
        {
            File saveLocation = results[0];
            // Save to the selected location...
        }
    }, true);
    @endcode

    @tags{GUI}
*/
class YUP_API FileChooser : public ReferenceCountedObject
{
public:
    using Ptr = ReferenceCountedObjectPtr<FileChooser>;

    //==============================================================================
    /** Destructor. */
    ~FileChooser();

    //==============================================================================
    /** Creates a FileChooser.

        @param dialogBoxTitle       a text string to display in the dialog box to tell the user what the dialog is for.
        @param initialFileOrDirectory  the file or directory that should be selected when the dialog opens.
                                      If this is File(), a sensible default location will be used.
        @param filePatternsAllowed  a set of file patterns to specify which files can be
                                   selected - each pattern should be separated by a comma or
                                   semicolon, e.g. "*.wav;*.aiff" or "*.txt,*.doc".
                                   An empty string means that all files are allowed. To specify
                                   that sub-directories can also be selected, add "<Directories>"
                                   to the end of the pattern.
        @param useOSNativeDialogBox if true, then on platforms that support it, a native
                                    dialog box will be used; if false, then a YUP-based
                                    browser dialog box will be displayed. Some platforms may not
                                    support native dialogs, in which case the YUP dialog will
                                    be used regardless of this setting.
        @param treatFilePackagesAsDirs  if true, then the file chooser will allow the selection
                                       of files inside packages when they are opened in the OS.
    */
    static Ptr create (const String& dialogBoxTitle,
                       const File& initialFileOrDirectory = File(),
                       const String& filePatternsAllowed = String(),
                       bool useOSNativeDialogBox = true,
                       bool treatFilePackagesAsDirs = false);

    //==============================================================================
    /** A callback function type used by the async file chooser methods.

        @param success   true if the user selected files, false if they cancelled
        @param results   the array of selected files (may be empty if cancelled)
    */
    using CompletionCallback = std::function<void (bool, const Array<File>&)>;

    //==============================================================================
    /** Shows a dialog box to choose a file to open asynchronously.

        This will display the dialog, and when the user makes a selection or cancels,
        the callback will be invoked on the message thread.

        @param callback           function to call when the dialog completes

        @see browseForMultipleFilesToOpen, browseForFileToSave, browseForDirectory
    */
    void browseForFileToOpen (CompletionCallback callback);

    /** Same as browseForFileToOpen, but allows the user to select multiple files.

        @param callback           function to call when the dialog completes

        @see browseForFileToOpen
    */
    void browseForMultipleFilesToOpen (CompletionCallback callback);

    /** Same as browseForMultipleFilesToOpen and browseForDirectory, but allows the user to select multiple files or directories.

        @param callback           function to call when the dialog completes

        @see browseForMultipleFilesToOpen
    */
    void browseForMultipleFilesOrDirectoriesToOpen (CompletionCallback callback);

    /** Shows a dialog box to choose a file to save asynchronously.

        This will display the dialog, and when the user makes a selection or cancels,
        the callback will be invoked on the message thread.

        @param callback                             function to call when the dialog completes
        @param warnAboutOverwritingExistingFiles    if true, the dialog will ask
                                                   the user if they're sure they want to
                                                   overwrite a file that already exists

        @see browseForFileToOpen
    */
    void browseForFileToSave (CompletionCallback callback, bool warnAboutOverwritingExistingFiles);

    /** Shows a dialog box to choose a directory asynchronously.

        This will display the dialog, and when the user makes a selection or cancels,
        the callback will be invoked on the message thread.

        @param callback function to call when the dialog completes

        @see browseForFileToOpen
    */
    void browseForDirectory (CompletionCallback callback);

    /** Runs a modal file browser asynchronously.

        @param callback             function to call when the dialog completes
        @param flags                a set of flags to specify which type of browser to use
    */
    void showDialog (CompletionCallback callback, int flags);

private:
    //==============================================================================
    static constexpr int openMode = 1 << 0;
    static constexpr int saveMode = 1 << 1;
    static constexpr int canSelectFiles = 1 << 2;
    static constexpr int canSelectDirectories = 1 << 3;
    static constexpr int canSelectMultipleItems = 1 << 4;
    static constexpr int useDialogForAll = 1 << 5;
    static constexpr int treatFilePackagesAsDirs = 1 << 6;
    static constexpr int doNotResolveSymlinks = 1 << 7;
    static constexpr int warnAboutOverwriting = 1 << 8;

    FileChooser (const String& dialogBoxTitle,
                 const File& initialFileOrDirectory = File(),
                 const String& filePatternsAllowed = String(),
                 bool useOSNativeDialogBox = true,
                 bool treatFilePackagesAsDirs = false);

    void showPlatformDialog (CompletionCallback callback, int flags);
    String getFilePatternsForPlatform() const;
    void invokeCallback (CompletionCallback callback, bool success, const Array<File>& results);
    CompletionCallback createCapturingCallback (CompletionCallback callback);

    String title, filters;
    File startingFile;
    bool useNativeDialogBox;
    bool packageDirsAsFiles;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileChooser)
};

} // namespace yup
