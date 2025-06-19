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
static String createAcceptAttribute (const String& filters)
{
    if (filters.isEmpty())
        return "*/*";

    StringArray extensions = StringArray::fromTokens (filters, ";,", String());
    StringArray acceptValues;

    for (const auto& ext : extensions)
    {
        String extension = ext.trim();
        if (extension.startsWith ("*."))
            extension = extension.substring (1);
        else if (extension.startsWith ("*"))
            extension = "." + extension.substring (1);
        else if (! extension.startsWith ("."))
            extension = "." + extension;

        if (extension.isNotEmpty() && extension != ".")
            acceptValues.add (extension);
    }

    if (acceptValues.isEmpty())
        return "*/*";

    return acceptValues.joinIntoString (",");
}

//==============================================================================
class EmscriptenFileChooser
{
public:
    EmscriptenFileChooser (FileChooser* chooser, int flags)
        : fileChooser (chooser)
        , dialogFlags (flags)
        , completed (false)
    {
    }

    void showDialog()
    {
        const bool isSave = (dialogFlags & FileChooser::saveMode) != 0;
        const bool canChooseDirectories = (dialogFlags & FileChooser::canSelectDirectories) != 0;
        const bool allowsMultiple = (dialogFlags & FileChooser::canSelectMultipleItems) != 0;

        if (isSave)
        {
            // For save operations, we'll create a download link
            // This is a workaround since browsers don't allow writing files directly
            showSaveDialog();
        }
        else if (canChooseDirectories)
        {
            // Directory selection is limited in browsers
            // We'll use the webkitdirectory attribute if available
            showDirectoryDialog();
        }
        else
        {
            // Regular file selection
            showOpenDialog (allowsMultiple);
        }
    }

    bool isCompleted() const { return completed; }

private:
    void showOpenDialog (bool multiple)
    {
        // Create a hidden file input element
        EM_ASM({
            var fileInput = document.createElement('input');
            fileInput.type = 'file';
            fileInput.style.display = 'none';

            if ($1) { // multiple
                fileInput.multiple = true;
            }

            var acceptAttr = UTF8ToString($2);
            if (acceptAttr) {
                fileInput.accept = acceptAttr;
            }

            fileInput.onchange = function(event) {
                var files = event.target.files;
                var fileCount = files.length;

                // Store file information
                Module.fileChooserResults = [];

                for (var i = 0; i < fileCount; i++) {
                    var file = files[i];
                    Module.fileChooserResults.push({
                        name: file.name,
                        size: file.size,
                        type: file.type,
                        lastModified: file.lastModified
                    });

                    // Create a virtual file path
                    var virtualPath = '/tmp/' + file.name;

                    // Read file content and store it in the virtual filesystem
                    var reader = new FileReader();
                    reader.onload = function(e) {
                        try {
                            var data = new Uint8Array(e.target.result);
                            FS.writeFile(virtualPath, data);
                        } catch (err) {
                            console.warn('Could not write file to virtual filesystem:', err);
                        }
                    };
                    reader.readAsArrayBuffer(file);
                }

                // Notify completion
                Module.ccall('fileChooserCallback', null, ['number'], [$0]);

                // Clean up
                document.body.removeChild(fileInput);
            };

            fileInput.oncancel = function() {
                Module.ccall('fileChooserCallback', null, ['number'], [$0]);
                document.body.removeChild(fileInput);
            };

            document.body.appendChild(fileInput);
            fileInput.click();
        }, this, allowsMultiple ? 1 : 0, createAcceptAttribute (fileChooser->filters).toRawUTF8());
    }

    void showDirectoryDialog()
    {
        EM_ASM({
            var fileInput = document.createElement('input');
            fileInput.type = 'file';
            fileInput.style.display = 'none';
            fileInput.webkitdirectory = true;

            fileInput.onchange = function(event) {
                var files = event.target.files;
                var fileCount = files.length;

                Module.fileChooserResults = [];

                if (fileCount > 0) {
                    // Get the directory name from the first file's path
                    var firstFile = files[0];
                    var pathParts = firstFile.webkitRelativePath.split('/');
                    var dirName = pathParts[0];

                    Module.fileChooserResults.push({
                        name: dirName,
                        isDirectory: true,
                        path: '/tmp/' + dirName
                    });

                    // Create directory structure in virtual filesystem
                    var processedDirs = new Set();

                    for (var i = 0; i < fileCount; i++) {
                        var file = files[i];
                        var relativePath = file.webkitRelativePath;
                        var fullPath = '/tmp/' + relativePath;

                        // Create directories
                        var pathParts = relativePath.split('/');
                        var currentPath = '/tmp';

                        for (var j = 0; j < pathParts.length - 1; j++) {
                            currentPath += '/' + pathParts[j];
                            if (!processedDirs.has(currentPath)) {
                                try {
                                    FS.mkdir(currentPath);
                                    processedDirs.add(currentPath);
                                } catch (err) {
                                    // Directory might already exist
                                }
                            }
                        }

                        // Write file
                        var reader = new FileReader();
                        reader.onload = function(e) {
                            try {
                                var data = new Uint8Array(e.target.result);
                                FS.writeFile(fullPath, data);
                            } catch (err) {
                                console.warn('Could not write file to virtual filesystem:', err);
                            }
                        };
                        reader.readAsArrayBuffer(file);
                    }
                }

                Module.ccall('fileChooserCallback', null, ['number'], [$0]);
                document.body.removeChild(fileInput);
            };

            fileInput.oncancel = function() {
                Module.ccall('fileChooserCallback', null, ['number'], [$0]);
                document.body.removeChild(fileInput);
            };

            document.body.appendChild(fileInput);
            fileInput.click();
        }, this);
    }

    void showSaveDialog()
    {
        // For save operations in the browser, we typically use download links
        // This is a placeholder - the actual save would need to be triggered
        // by the application when it has content to save

        EM_ASM({
            // Create a simple prompt for filename
            var filename = prompt('Enter filename:');

            if (filename) {
                Module.fileChooserResults = [{
                    name: filename,
                    isSave: true,
                    path: '/tmp/' + filename
                }];
            } else {
                Module.fileChooserResults = [];
            }

            Module.ccall('fileChooserCallback', null, ['number'], [$0]);
        }, this);
    }

    void processResults()
    {
        EM_ASM({
            if (Module.fileChooserResults) {
                var results = Module.fileChooserResults;

                for (var i = 0; i < results.length; i++) {
                    var result = results[i];
                    var path = result.path || ('/tmp/' + result.name);

                    // Call back to C++ with the file path
                    Module.ccall('addFileResult', null, ['number', 'string'], [$0, path]);
                }

                delete Module.fileChooserResults;
            }
        }, this);

        completed = true;
    }

    FileChooser* fileChooser;
    int dialogFlags;
    bool completed;
};

static EmscriptenFileChooser* currentFileChooser = nullptr;

extern "C"
{
    void EMSCRIPTEN_KEEPALIVE fileChooserCallback (EmscriptenFileChooser* chooser)
    {
        if (chooser != nullptr)
        {
            chooser->processResults();
        }
    }

    void EMSCRIPTEN_KEEPALIVE addFileResult (EmscriptenFileChooser* chooser, const char* path)
    {
        if (chooser != nullptr && chooser->fileChooser != nullptr && path != nullptr)
        {
            chooser->fileChooser->results.add (File (String::fromUTF8 (path)));
        }
    }
}

void FileChooser::showPlatformDialog (int flags, Component* previewComponent)
{
    EmscriptenFileChooser chooser (this, flags);
    currentFileChooser = &chooser;

    chooser.showDialog();

    // Wait for completion
    while (! chooser.isCompleted())
    {
        emscripten_sleep (10);
    }

    currentFileChooser = nullptr;
}

} // namespace yup
