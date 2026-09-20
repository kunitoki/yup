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

#include "yup_YdspCommands.h"
#include <yup_dsp_jit/yup_dsp_jit.h>

#include <iostream>

using namespace yup;

static String readLine()
{
    String result;
    char character = 0;

    while (std::cin.get (character) && character != '\n')
        result += character;

    return result;
}

static void sendLsp (const String& body)
{
    std::cout << "Content-Length: " << body.getNumBytesAsUTF8() << "\r\n\r\n";
    std::cout.write (body.toRawUTF8(), static_cast<std::streamsize> (body.getNumBytesAsUTF8()));
    std::cout.flush();
}

static String quoted (const String& value)
{
    return JSON::toString (var (value), true);
}

static File projectForSource (const File& source, const YdspCompileOptions& options)
{
    if (source.hasFileExtension ("ydsp-project"))
        return source;
    for (auto directory = source.getParentDirectory();;)
    {
        for (const auto& candidate : directory.findChildFiles (File::findFiles, false, "*.ydsp-project"))
        {
            YdspDiagnostics diagnostics;
            const auto overlay = options.sourceOverrides.find (candidate.getFullPathName());
            auto project = overlay != options.sourceOverrides.end()
                             ? YdspProject::parse (overlay->second, candidate, diagnostics)
                             : YdspProject::load (candidate, diagnostics);
            if (project.wasOk())
                for (const auto& path : project.getReference().getSources())
                    if (directory.getChildFile (path) == source)
                        return candidate;
        }
        const auto parent = directory.getParentDirectory();
        if (parent == directory)
            return {};
        directory = parent;
    }
}

static int utf16Column (const StringArray& lines, int line, int column)
{
    if (! isPositiveAndBelow (line, lines.size()))
        return column;
    int units = 0;
    for (const auto character : lines[line].substring (0, column))
        units += character > 0xffff ? 2 : 1;
    return units;
}

int runYdspLspCommand()
{
    YdspCompileOptions options;
    std::unordered_set<String> published;
    const auto validate = [&]
    {
        std::unordered_map<String, String> items;
        std::unordered_set<String> roots;
        for (const auto& [path, source] : options.sourceOverrides)
        {
            const File file (path);
            const auto project = projectForSource (file, options);
            const auto root = project != File() ? project : file;
            if (! roots.insert (root.getFullPathName()).second)
                continue;
            YdspCompiler compiler;
            if (project != File())
                compiler.compileProject (project, options);
            else
                compiler.compile (source, options, path);
            const auto& diagnostics = compiler.getDiagnostics();
            for (int i = 0; i < diagnostics.getCount(); ++i)
            {
                const auto& item = diagnostics.getItem (i);
                if (item.severity == YdspSeverity::info)
                    continue;
                const auto location = item.sourceId.isNotEmpty() && File::isAbsolutePath (item.sourceId)
                                        ? File (item.sourceId) : root;
                auto& list = items[URL (location).toString (false)];
                if (list.isNotEmpty())
                    list += ',';
                const auto line = jmax (0, item.range.startLine - 1);
                const auto column = jmax (0, item.range.startColumn - 1);
                const auto endLine = jmax (line, item.range.endLine - 1);
                const auto endColumn = jmax (endLine == line ? column + 1 : 0, item.range.endColumn - 1);
                const auto overlay = options.sourceOverrides.find (location.getFullPathName());
                const auto text = overlay != options.sourceOverrides.end() ? overlay->second : location.loadFileAsString();
                const auto lines = StringArray::fromLines (text);
                list += "{\"range\":{\"start\":{\"line\":" + String (line) + ",\"character\":" + String (utf16Column (lines, line, column))
                      + "},\"end\":{\"line\":" + String (endLine) + ",\"character\":" + String (utf16Column (lines, endLine, endColumn))
                      + "}},\"severity\":" + String (item.severity == YdspSeverity::error ? 1 : 2)
                      + ",\"source\":\"ydsp\",\"message\":" + quoted (item.message) + "}";
            }
        }
        for (const auto& uri : published)
            items.try_emplace (uri, String());
        published.clear();
        for (const auto& [uri, list] : items)
        {
            sendLsp ("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{\"uri\":"
                     + quoted (uri) + ",\"diagnostics\":[" + list + "]}}");
            if (list.isNotEmpty())
                published.insert (uri);
        }
    };

    while (std::cin.good())
    {
        int length = 0;
        for (;;)
        {
            const auto header = readLine().trim();
            if (header.isEmpty())
                break;
            if (header.startsWithIgnoreCase ("Content-Length:"))
                length = header.substring (15).getIntValue();
        }
        if (length <= 0 || length > 16 * 1024 * 1024)
            return 0;
        MemoryBlock data (static_cast<size_t> (length), false);
        if (! std::cin.read (static_cast<char*> (data.getData()), length))
            return 0;
        var message;
        if (JSON::parse (String::fromUTF8 (static_cast<const char*> (data.getData()), length), message).failed())
            continue;
        const auto method = message["method"].toString();
        const auto id = JSON::toString (message["id"], true);
        const auto& params = message["params"];
        if (method == "initialize")
            sendLsp ("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":{\"capabilities\":{\"textDocumentSync\":{\"openClose\":true,\"change\":1,\"save\":true}}}}");
        else if (method == "shutdown")
            sendLsp ("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":null}");
        else if (method == "exit")
            return 0;
        else if (method == "textDocument/didOpen" || method == "textDocument/didChange" || method == "textDocument/didClose")
        {
            const auto& document = params["textDocument"];
            const URL uri (document["uri"].toString());
            if (! uri.isLocalFile())
                continue;
            const auto path = uri.getLocalFile().getFullPathName();
            if (method == "textDocument/didClose")
                options.sourceOverrides.erase (path);
            else if (method == "textDocument/didOpen")
                options.sourceOverrides[path] = document["text"].toString();
            else if (const auto* changes = params["contentChanges"].getArray(); changes != nullptr && ! changes->isEmpty())
                options.sourceOverrides[path] = changes->getLast()["text"].toString();
            validate();
        }
        else if (method == "textDocument/didSave" || method == "workspace/didChangeWatchedFiles")
            validate();
        else if (message.hasProperty ("id"))
            sendLsp ("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"error\":{\"code\":-32601,\"message\":\"Method not found\"}}");
    }
    return 0;
}

