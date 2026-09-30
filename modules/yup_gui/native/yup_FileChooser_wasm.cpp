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
// JavaScript bridge: a hidden <input type="file"> whose picks are copied into
// the in-memory filesystem one after the other, before the chooser completes.
// clang-format off

EM_JS (void, yupFileChooserShow, (int id, int multiple, int directory, const char* accept), {
    var input = document.createElement ('input');
    input.type = 'file';
    input.style.display = 'none';
    input.multiple = multiple !== 0;
    input.webkitdirectory = directory !== 0;

    var acceptAttribute = UTF8ToString (accept);
    if (acceptAttribute && directory === 0)
        input.accept = acceptAttribute;

    var finished = false;
    var finish = function()
    {
        if (finished)
            return;

        finished = true;
        input.remove();
        Module._yupFileChooserFinished (id);
    };

    input.onchange = function()
    {
        var files = Array.from (input.files || []);

        files.reduce (function (previous, file)
        {
            return previous.then (function() { return file.arrayBuffer(); }).then (function (buffer)
            {
                var bytes = new Uint8Array (buffer);
                var ptr = Module._yupFileChooserAllocate (id, bytes.length);

                if (ptr)
                    HEAPU8.set (bytes, ptr);

                Module.ccall ('yupFileChooserAddFile', null, ['number', 'string', 'number'],
                              [id, file.webkitRelativePath || file.name, bytes.length]);
            });
        }, Promise.resolve()).catch (function (error)
        {
            console.warn ('Could not read the chosen files:', error);
        }).then (finish);
    };

    input.oncancel = finish;

    document.body.appendChild (input);
    input.click();
});

EM_JS (int, yupFileChooserPromptSaveName, (char* buffer, int size), {
    var name = prompt ('Enter filename:');
    return name ? stringToUTF8 (name, buffer, size) : 0;
});

// clang-format on

//==============================================================================
class EmscriptenFileChooser
{
public:
    EmscriptenFileChooser (FileChooser::CompletionCallback callback, bool isDirectoryPick)
        : callback (std::move (callback))
        , isDirectoryPick (isDirectoryPick)
    {
    }

    void* allocate (size_t size)
    {
        scratch.setSize (size);
        return scratch.getData();
    }

    void addFile (const String& relativePath, size_t size)
    {
        if (relativePath.isEmpty() || size > scratch.getSize() || (size > 0 && scratch.getData() == nullptr))
            return;

        if (destination == File())
        {
            destination = File::getSpecialLocation (File::tempDirectory)
                              .getChildFile ("yup_file_chooser")
                              .getNonexistentChildFile ("pick", {}, false);
        }

        const auto file = destination.getChildFile (relativePath);

        if (file.getParentDirectory().createDirectory().failed())
            return;

        const bool written = size > 0 ? file.replaceWithData (scratch.getData(), size) : file.create().wasOk();
        if (! written)
            return;

        if (isDirectoryPick)
            results.addIfNotAlreadyThere (destination.getChildFile (relativePath.upToFirstOccurrenceOf ("/", false, false)));
        else
            results.add (file);
    }

    void finish()
    {
        MessageManager::callAsync ([callback = std::move (callback), results = std::move (results)]
        {
            if (callback)
                callback (! results.isEmpty(), results);
        });
    }

private:
    FileChooser::CompletionCallback callback;
    bool isDirectoryPick;
    File destination;
    MemoryBlock scratch;
    Array<File> results;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EmscriptenFileChooser)
};

//==============================================================================
// Only touched on the browser main thread, which runs the YUP message loop.
static std::unordered_map<int, std::unique_ptr<EmscriptenFileChooser>>& getWebFileChoosers()
{
    static std::unordered_map<int, std::unique_ptr<EmscriptenFileChooser>> choosers;
    return choosers;
}

static EmscriptenFileChooser* findWebFileChooser (int id)
{
    auto& choosers = getWebFileChoosers();
    auto it = choosers.find (id);
    return it != choosers.end() ? it->second.get() : nullptr;
}

extern "C"
{
    void* EMSCRIPTEN_KEEPALIVE yupFileChooserAllocate (int id, int size)
    {
        auto* chooser = findWebFileChooser (id);
        return chooser != nullptr && size >= 0 ? chooser->allocate (static_cast<size_t> (size)) : nullptr;
    }

    void EMSCRIPTEN_KEEPALIVE yupFileChooserAddFile (int id, const char* relativePath, int size)
    {
        if (auto* chooser = findWebFileChooser (id); chooser != nullptr && relativePath != nullptr && size >= 0)
            chooser->addFile (String::fromUTF8 (relativePath), static_cast<size_t> (size));
    }

    void EMSCRIPTEN_KEEPALIVE yupFileChooserFinished (int id)
    {
        auto& choosers = getWebFileChoosers();
        auto it = choosers.find (id);
        if (it == choosers.end())
            return;

        auto chooser = std::move (it->second);
        choosers.erase (it);
        chooser->finish();
    }

} // extern "C"

//==============================================================================
void FileChooser::showPlatformDialog (CompletionCallback callback, int flags)
{
    if ((flags & saveMode) != 0)
    {
        char name[1024] = {};
        const bool chosen = yupFileChooserPromptSaveName (name, static_cast<int> (sizeof (name))) > 0;

        Array<File> results;
        if (chosen)
            results.add (File::getSpecialLocation (File::tempDirectory).getChildFile (String::fromUTF8 (name)));

        MessageManager::callAsync ([callback = std::move (callback), chosen, results]
        {
            if (callback)
                callback (chosen, results);
        });

        return;
    }

    static int nextId = 0;
    const int id = ++nextId;
    const bool isDirectoryPick = (flags & canSelectDirectories) != 0;

    getWebFileChoosers()[id] = std::make_unique<EmscriptenFileChooser> (std::move (callback), isDirectoryPick);

    yupFileChooserShow (id,
                        (flags & canSelectMultipleItems) != 0 ? 1 : 0,
                        isDirectoryPick ? 1 : 0,
                        createAcceptAttribute (filters).toRawUTF8());
}

} // namespace yup
