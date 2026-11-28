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

#include "yup_YuiToolCheck.h"
#include "yup_YuiToolResources.h"

#include <algorithm>
#include <iostream>
#include <map>

using namespace yup;

//==============================================================================

void YuiToolProblem::print() const
{
    std::cerr << file.getFullPathName();

    if (line > 0)
        std::cerr << ":" << line << ":" << column;

    std::cerr << ": " << (isError ? "error" : "warning") << ": " << message << "\n";
}

var YuiToolProblem::toJson() const
{
    return var::object ({ { "file", file.getFullPathName() },
                          { "line", line },
                          { "column", column },
                          { "severity", isError ? "error" : "warning" },
                          { "message", message } });
}

std::vector<YuiToolProblem> findYuiToolProblems (const YuiDocument& document, const File& file, const Result& loadResult)
{
    std::vector<YuiToolProblem> problems;

    // A problem is told on one line, which the errors of a shader compiler are not
    const auto onOneLine = [] (const String& message)
    {
        auto lines = StringArray::fromLines (message);
        lines.trim();
        lines.removeEmptyStrings();

        return lines.joinIntoString (" ");
    };

    for (const auto& diagnostic : document.getDiagnostics())
    {
        problems.push_back ({ .file = diagnostic.source.isEmpty() ? file : file.getParentDirectory().getChildFile (diagnostic.source),
                              .line = diagnostic.line,
                              .column = diagnostic.column,
                              .isError = diagnostic.isError(),
                              .message = onOneLine (diagnostic.message) });
    }

    const auto hasError = std::any_of (problems.begin(), problems.end(), [] (const YuiToolProblem& problem)
    {
        return problem.isError;
    });

    if (loadResult.failed() && ! hasError)
        problems.push_back ({ .file = file, .message = loadResult.getErrorMessage() });

    return problems;
}

namespace
{

//==============================================================================
/** Serves the files a build embeds as the function it generates does for an application, which loads a document with
    YuiLoader::loadResource: by their path, from the folder they are embedded from. Anything else is not found, and
    remembered.
*/
class EmbeddedFiles
{
public:
    EmbeddedFiles (const YuiToolEmbedded& embeddedFiles, const File& documentFile)
        : embedded (embeddedFiles)
        , document (documentFile)
        , path (documentFile.getRelativePathFrom (embeddedFiles.base).replaceCharacter ('\\', '/'))
    {
    }

    /** Returns the path the document is embedded under. */
    const String& getPath() const noexcept { return path; }

    /** Returns why the document is not one of the files, nothing when it is. */
    String findWhyNotEmbedded() const
    {
        if (! document.isAChildOf (embedded.base))
            return "this file is not in '" + embedded.base.getFullPathName() + "', the folder of the embedded files";

        return embedded.paths.contains (path) ? String() : "'" + path + "' is not one of the embedded files";
    }

    YuiLoader::ResourceFinder getFinder()
    {
        return [this] (const char* asked, std::size_t& size)
        {
            return find (asked, size);
        };
    }

    /** Returns what the document read that is in the folder of the files without being one of them. */
    std::vector<YuiToolProblem> findProblems() const
    {
        std::vector<YuiToolProblem> problems;

        for (const auto& asked : refused)
        {
            // The bundles a build compiles a shader into are asked for before its source, which is what stands for them
            if (isBundleOf (asked, ".ysl") || isBundleOf (asked, ".raster.ysl") || isBundleOf (asked, ".full.ysl"))
                continue;

            // What is not there either is told by who could not read it
            if (const auto file = embedded.base.getChildFile (asked); file.existsAsFile() && file.isAChildOf (embedded.base))
                problems.push_back ({ .file = document, .message = "'" + asked + "' is read by '" + path + "' but is not embedded" });
        }

        return problems;
    }

private:
    const uint8* find (const char* asked, std::size_t& size)
    {
        const auto name = String::fromUTF8 (asked);
        size = 0;

        if (! embedded.paths.contains (name))
        {
            refused.addIfNotAlreadyThere (name);
            return nullptr;
        }

        // Read once and kept, as the bytes of an embedded file stay where they are
        auto found = served.find (name);

        if (found == served.end())
        {
            MemoryBlock bytes;

            if (! embedded.base.getChildFile (name).loadFileAsData (bytes) || bytes.isEmpty())
                return nullptr;

            found = served.emplace (name, std::move (bytes)).first;
        }

        size = found->second.getSize();
        return static_cast<const uint8*> (found->second.getData());
    }

    bool isBundleOf (const String& asked, const char* extension) const
    {
        const auto source = asked.endsWith (extension) ? asked.dropLastCharacters (String (extension).length()) : String();

        return source.isNotEmpty() && (embedded.paths.contains (source) || refused.contains (source));
    }

    const YuiToolEmbedded& embedded;
    const File document;
    const String path;
    StringArray refused;
    std::map<String, MemoryBlock> served;
};

//==============================================================================
// A file holding only declarations (styles, drawings, components) is written to be imported
bool hasNoRootComponent (const String& text, const YuiLoader& loader, const String& mainPath)
{
    // The loader knows what a file imports by its path in the source, which starts with the folder of the file
    const auto folder = mainPath.substring (0, mainPath.lastIndexOfChar ('/') + 1);

    // The file counts as imported already, by its name: one importing it back adds no second copy of it
    const auto parsed = YuiParser::parse (text, [&loader, &folder] (const String& path)
    {
        return loader.getLoadedText (folder + path);
    }, mainPath.substring (folder.length()));

    return parsed.wasOk() && findYuiToolRootIndex (parsed.document) < 0;
}

// Loads a file as it is checked: from its folder, or from the files a build embeds as an application loads it, with the
// text given in place of its own, and what only declares as an empty interface
Result loadForChecking (YuiDocument& document, YuiLoader& loader, const File& file, const std::optional<String>& unsavedText, EmbeddedFiles* embedded = nullptr)
{
    const auto mainPath = embedded != nullptr ? embedded->getPath() : file.getFileName();
    auto result = embedded != nullptr ? loader.loadResource (mainPath, embedded->getFinder()) : loader.loadFile (file);

    if (unsavedText.has_value())
        result = loader.reloadFromString (*unsavedText);

    if (result.failed())
    {
        // There is no interface to build from declarations alone, so they are checked as those of an empty one. What they
        // need is read from the folder of the file importing them, so it is checked with that file
        if (const auto text = loader.getLoadedText (mainPath); text.has_value() && hasNoRootComponent (*text, loader, mainPath))
            return loader.reloadFromString (*text + "\nComponent {}\n");
    }

    // What an interface needs is checked whether it is used or not
    if (result.wasOk())
        checkYuiToolResources (document);

#if YUP_WASM
    // The host of the module gave it the files: it is told those the document names that it was not given
    if (result.wasOk())
        noteYuiToolMissingFiles (document);
#endif

    return result;
}

} // namespace

std::vector<YuiToolProblem> findYuiToolFileProblems (const File& file, const std::optional<String>& unsavedText, const YuiToolModels& models, const YuiToolEmbedded* embedded)
{
    // Outlives the loader, which finds its files with it
    std::optional<EmbeddedFiles> files;

    if (embedded != nullptr)
    {
        files.emplace (*embedded, file);

        if (const auto reason = files->findWhyNotEmbedded(); reason.isNotEmpty())
            return { YuiToolProblem { .file = file, .message = reason } };
    }

    YuiDocument document;
    YuiLoader loader (document);
    models.attachTo (document);

    const auto result = loadForChecking (document, loader, file, unsavedText, files.has_value() ? &*files : nullptr);
    auto problems = findYuiToolProblems (document, file, result);

    // What is not embedded comes first: it is the cause of what could not be read
    if (files.has_value())
    {
        const auto missing = files->findProblems();
        problems.insert (problems.begin(), missing.begin(), missing.end());
    }

    return problems;
}

Result loadYuiToolFileForChecking (YuiDocument& document, YuiLoader& loader, const File& file)
{
    return loadForChecking (document, loader, file, std::nullopt);
}

int findYuiToolRootIndex (const DataTree& document)
{
    for (int index = 0; index < document.getNumChildren(); ++index)
    {
        const auto typeName = document.getChild (index).getType().toString();

        if (CharacterFunctions::isUpperCase (static_cast<yup_wchar> (typeName[0])))
            return index;
    }

    return -1;
}
