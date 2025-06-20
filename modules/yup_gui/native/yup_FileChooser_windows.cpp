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
static String createFilterString (const String& filters)
{
    if (filters.isEmpty())
        return "All Files\0*.*\0";

    String result;
    StringArray extensions = StringArray::fromTokens (filters, ";,", String());

    if (extensions.size() > 0)
    {
        String allPatterns;
        String desc = "Supported Files";

        for (int i = 0; i < extensions.size(); ++i)
        {
            String ext = extensions[i].trim();
            if (ext.isNotEmpty())
            {
                if (! ext.startsWith ("*"))
                    ext = "*." + ext;

                if (allPatterns.isNotEmpty())
                    allPatterns += ";";
                allPatterns += ext;
            }
        }

        if (allPatterns.isNotEmpty())
        {
            result += desc + String::charToString (0) + allPatterns + String::charToString (0);
        }
    }

    result += "All Files" + String::charToString (0) + "*.*" + String::charToString (0) + String::charToString (0);
    return result;
}

//==============================================================================
static COMDLG_FILTERSPEC* createFilterSpecs (const String& filters, int& numFilters)
{
    numFilters = 0;

    if (filters.isEmpty())
    {
        numFilters = 1;
        COMDLG_FILTERSPEC* specs = new COMDLG_FILTERSPEC[1];
        specs[0].pszName = L"All Files";
        specs[0].pszSpec = L"*.*";
        return specs;
    }

    StringArray extensions = StringArray::fromTokens (filters, ";,", String());

    if (extensions.size() == 0)
    {
        numFilters = 1;
        COMDLG_FILTERSPEC* specs = new COMDLG_FILTERSPEC[1];
        specs[0].pszName = L"All Files";
        specs[0].pszSpec = L"*.*";
        return specs;
    }

    numFilters = extensions.size() + 1;
    COMDLG_FILTERSPEC* specs = new COMDLG_FILTERSPEC[numFilters];

    String allPatterns;
    for (int i = 0; i < extensions.size(); ++i)
    {
        String ext = extensions[i].trim();
        if (ext.isNotEmpty())
        {
            if (! ext.startsWith ("*"))
                ext = "*." + ext;

            if (allPatterns.isNotEmpty())
                allPatterns += ";";
            allPatterns += ext;
        }
    }

    specs[0].pszName = L"Supported Files";
    specs[0].pszSpec = allPatterns.toWideCharPointer();
    specs[numFilters - 1].pszName = L"All Files";
    specs[numFilters - 1].pszSpec = L"*.*";

    return specs;
}

//==============================================================================
class FileChooser::FileChooserImpl
{
public:
    FileChooserImpl (FileChooser& owner, CompletionCallback cb, bool isSave, bool canChooseFiles, bool canChooseDirectories, bool allowsMultipleSelection, bool warnAboutOverwrite)
        : fileChooser (owner)
        , callback (std::move (cb))
        , isSave (isSave)
        , canChooseFiles (canChooseFiles)
        , canChooseDirectories (canChooseDirectories)
        , allowsMultipleSelection (allowsMultipleSelection)
        , warnAboutOverwrite (warnAboutOverwrite)
    {
    }

    bool runDialogOnCurrentThread()
    {
        HRESULT hr = S_OK;

        if (isSave)
        {
            IFileSaveDialog* pFileSave = nullptr;

            hr = CoCreateInstance (CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**> (&pFileSave));
            if (SUCCEEDED (hr))
            {
                // Set title
                pFileSave->SetTitle (fileChooser.title.toWideCharPointer());

                // Set file type filters
                int numFilters;
                COMDLG_FILTERSPEC* filterSpecs = createFilterSpecs (fileChooser.filters, numFilters);
                if (filterSpecs != nullptr)
                {
                    pFileSave->SetFileTypes (numFilters, filterSpecs);
                    pFileSave->SetFileTypeIndex (1);
                    delete[] filterSpecs;
                }

                // Set starting directory
                if (fileChooser.startingFile.exists())
                {
                    IShellItem* psi = nullptr;
                    File dirToUse = fileChooser.startingFile.isDirectory() ? fileChooser.startingFile : fileChooser.startingFile.getParentDirectory();

                    hr = SHCreateItemFromParsingName (dirToUse.getFullPathName().toWideCharPointer(),
                                                      NULL,
                                                      IID_IShellItem,
                                                      IID_PPV_ARGS (&psi));
                    if (SUCCEEDED (hr))
                    {
                        pFileSave->SetFolder (psi);
                        psi->Release();
                    }

                    if (fileChooser.startingFile.existsAsFile())
                        pFileSave->SetFileName (fileChooser.startingFile.getFileName().toWideCharPointer());
                }

                // Set options
                DWORD options;
                pFileSave->GetOptions (&options);
                options |= FOS_FORCEFILESYSTEM;
                if (warnAboutOverwrite)
                    options |= FOS_OVERWRITEPROMPT;
                pFileSave->SetOptions (options);

                // Show the dialog
                hr = pFileSave->Show (NULL);

                if (SUCCEEDED (hr))
                {
                    IShellItem* pItem = nullptr;
                    hr = pFileSave->GetResult (&pItem);

                    if (SUCCEEDED (hr))
                    {
                        PWSTR pszFilePath = nullptr;
                        hr = pItem->GetDisplayName (SIGDN_FILESYSPATH, &pszFilePath);

                        if (SUCCEEDED (hr))
                        {
                            results.add (File (String (pszFilePath)));
                            CoTaskMemFree (pszFilePath);
                        }

                        pItem->Release();
                    }
                }

                pFileSave->Release();
            }
        }
        else
        {
            IFileOpenDialog* pFileOpen = nullptr;

            hr = CoCreateInstance (CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**> (&pFileOpen));

            if (SUCCEEDED (hr))
            {
                // Set title
                pFileOpen->SetTitle (fileChooser.title.toWideCharPointer());

                // Set file type filters
                if (canChooseFiles)
                {
                    int numFilters;
                    COMDLG_FILTERSPEC* filterSpecs = createFilterSpecs (fileChooser.filters, numFilters);
                    if (filterSpecs != nullptr)
                    {
                        pFileOpen->SetFileTypes (numFilters, filterSpecs);
                        pFileOpen->SetFileTypeIndex (1);
                        delete[] filterSpecs;
                    }
                }

                // Set starting directory
                if (fileChooser.startingFile.exists())
                {
                    IShellItem* psi = nullptr;
                    hr = SHCreateItemFromParsingName (fileChooser.startingFile.getFullPathName().toWideCharPointer(),
                                                      NULL,
                                                      IID_IShellItem,
                                                      IID_PPV_ARGS (&psi));
                    if (SUCCEEDED (hr))
                    {
                        pFileOpen->SetFolder (psi);
                        psi->Release();
                    }
                }

                // Set options
                DWORD options;
                pFileOpen->GetOptions (&options);
                options |= FOS_FORCEFILESYSTEM;

                if (allowsMultipleSelection)
                    options |= FOS_ALLOWMULTISELECT;

                if (canChooseDirectories && ! canChooseFiles)
                    options |= FOS_PICKFOLDERS;
                else if (canChooseDirectories && canChooseFiles)
                    options |= FOS_PICKFOLDERS;

                pFileOpen->SetOptions (options);

                // Show the dialog
                hr = pFileOpen->Show (NULL);

                if (SUCCEEDED (hr))
                {
                    if (allowsMultipleSelection)
                    {
                        IShellItemArray* pItems = nullptr;
                        hr = pFileOpen->GetResults (&pItems);

                        if (SUCCEEDED (hr))
                        {
                            DWORD itemCount;
                            pItems->GetCount (&itemCount);

                            for (DWORD i = 0; i < itemCount; ++i)
                            {
                                IShellItem* pItem = nullptr;
                                hr = pItems->GetItemAt (i, &pItem);

                                if (SUCCEEDED (hr))
                                {
                                    PWSTR pszFilePath = nullptr;
                                    hr = pItem->GetDisplayName (SIGDN_FILESYSPATH, &pszFilePath);

                                    if (SUCCEEDED (hr))
                                    {
                                        results.add (File (String (pszFilePath)));
                                        CoTaskMemFree (pszFilePath);
                                    }

                                    pItem->Release();
                                }
                            }

                            pItems->Release();
                        }
                    }
                    else
                    {
                        IShellItem* pItem = nullptr;
                        hr = pFileOpen->GetResult (&pItem);

                        if (SUCCEEDED (hr))
                        {
                            PWSTR pszFilePath = nullptr;
                            hr = pItem->GetDisplayName (SIGDN_FILESYSPATH, &pszFilePath);

                            if (SUCCEEDED (hr))
                            {
                                results.add (File (String (pszFilePath)));
                                CoTaskMemFree (pszFilePath);
                            }

                            pItem->Release();
                        }
                    }
                }

                pFileOpen->Release();
            }
        }

        // Schedule callback to run on the main thread
        bool success = SUCCEEDED (hr) && results.size() > 0;
        MessageManager::callAsync ([callback = std::move (callback), success, results = std::move (results)]
        {
            callback (success, results);
        });
    }

private:
    FileChooser& fileChooser;
    CompletionCallback callback;
    Array<File> results;
    bool isSave;
    bool canChooseFiles;
    bool canChooseDirectories;
    bool allowsMultipleSelection;
    bool warnAboutOverwrite;
};

//==============================================================================
void FileChooser::showPlatformDialog (CompletionCallback callback, int flags)
{
    const bool isSave = (flags & saveMode) != 0;
    const bool canChooseFiles = (flags & canSelectFiles) != 0;
    const bool canChooseDirectories = (flags & canSelectDirectories) != 0;
    const bool allowsMultipleSelection = (flags & canSelectMultipleItems) != 0;
    const bool warnAboutOverwrite = (flags & warnAboutOverwriting) != 0;

    impl = std::make_unique<FileChooserImpl> (*this, std::move (callback), isSave, canChooseFiles, canChooseDirectories, allowsMultipleSelection, warnAboutOverwrite);

    auto dialogThread = std::thread ([this]
    {
        HRESULT hr = CoInitializeEx (NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        if (SUCCEEDED (hr) && impl != nullptr)
        {
            impl->runDialogOnCurrentThread();

            CoUninitialize();
        }
    });

    dialogThread.detach();
}

} // namespace yup
