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
    FileChooser chooser("Select an audio file", File::getSpecialLocation(File::userDocumentsDirectory), "*.wav;*.mp3;*.aiff");
    if (chooser.browseForFileToOpen())
    {
        File selectedFile = chooser.getResult();
        // Process the selected file...
    }

    // For opening multiple files
    FileChooser chooser("Select audio files", File(), "*.wav;*.mp3");
    if (chooser.browseForMultipleFilesToOpen())
    {
        Array<File> selectedFiles = chooser.getResults();
        // Process the selected files...
    }

    // For saving a file
    FileChooser chooser("Save project", File(), "*.proj", true);
    if (chooser.browseForFileToSave(true))
    {
        File saveLocation = chooser.getResult();
        // Save to the selected location...
    }
    @endcode

    @tags{GUI}
*/
class YUP_API FileChooser
{
public:
    //==============================================================================
    /** Creates a FileChooser.

        @param dialogBoxTitle       a text string to display in the dialog box to
                                    tell the user what the dialog is for.
        @param initialFileOrDirectory  the file or directory that should be selected
                                       when the dialog opens. If this is File(), a sensible
                                       default location will be used.
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
    FileChooser (const String& dialogBoxTitle,
                 const File& initialFileOrDirectory = File(),
                 const String& filePatternsAllowed = String(),
                 bool useOSNativeDialogBox = true,
                 bool treatFilePackagesAsDirs = false);

    /** Destructor. */
    ~FileChooser();

    //==============================================================================
    /** Shows a dialog box to choose a file to open.

        This will display the dialog, and if the user chooses a file, the full
        pathname is returned by getResult().

        @param previewComponent   an optional component to display inside the dialog,
                                 for showing previews of the files that the user
                                 is browsing. The component will not be deleted by this
                                 object, so the caller must take care of it.

        @returns    true if the user selected a file, false if they cancelled

        @see getResult, browseForMultipleFilesToOpen, browseForFileToSave, browseForDirectory
    */
    bool browseForFileToOpen (Component* previewComponent = nullptr);

    /** Same as browseForFileToOpen, but allows the user to select multiple files.

        The chosen files are returned by getResults(). If you only need a single file,
        use browseForFileToOpen() instead.

        @param previewComponent   an optional component to display inside the dialog

        @returns    true if the user selected one or more files, false if they cancelled

        @see browseForFileToOpen, getResults
    */
    bool browseForMultipleFilesToOpen (Component* previewComponent = nullptr);

    /** Same as browseForMultipleFilesToOpen and browseForDirectory, but allows the user to select multiple files or directories.

        The chosen files are returned by getResults().

        @param previewComponent   an optional component to display inside the dialog

        @returns    true if the user selected one or more files or directories, false if they cancelled

        @see browseForMultipleFilesToOpen, getResults
    */
    bool browseForMultipleFilesOrDirectoriesToOpen (Component* previewComponent = nullptr);

    /** Shows a dialog box to choose a file to save.

        This will display the dialog, and if the user chooses a file, the full
        pathname is returned by getResult().

        @param warnAboutOverwritingExistingFiles     if true, the dialog will ask
                                                    the user if they're sure they want to
                                                    overwrite a file that already exists

        @returns    true if the user chose a file, false if they cancelled

        @see getResult, browseForFileToOpen
    */
    bool browseForFileToSave (bool warnAboutOverwritingExistingFiles);

    /** Shows a dialog box to choose a directory.

        This will display the dialog, and if the user chooses a directory, the full
        pathname is returned by getResult().

        @returns    true if the user chose a directory, false if they cancelled

        @see getResult, browseForFileToOpen
    */
    bool browseForDirectory();

    /** Runs a modal file browser.

        @param flags                a set of flags to specify which type of browser to use
        @param previewComponent     an optional component to display inside the dialog

        @returns    true if the user chose a file, false if they cancelled
    */
    bool showDialog (int flags, Component* previewComponent);

    //==============================================================================
    /** Returns the last file that was chosen by one of the browse methods.

        After calling the appropriate browse method, this method lets you
        find out what file or directory they chose.

        Note that the file returned is only valid if the browse method returned true
        (i.e. if the user pressed 'ok' rather than cancelling).

        If you're using a multiple-file select, then use getResults() to get the
        list of all files chosen.

        @see getResults, browseForFileToOpen, browseForFileToSave, browseForDirectory
    */
    File getResult() const;

    /** Returns a list of all the files that were chosen during the last call to a
        browse method.

        This array may be empty if no files were chosen, or can contain multiple entries
        if multiple files were chosen.

        @see getResult
    */
    Array<File> getResults() const;

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

    friend void yup_fileChooserAddFileResult (FileChooser& chooser, File path);

    void showPlatformDialog (int flags, Component* previewComponent);
    String getFilePatternsForPlatform() const;

    String title, filters;
    File startingFile;
    Array<File> results;
    bool useNativeDialogBox;
    bool packageDirsAsFiles;

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileChooser)
};

} // namespace yup
