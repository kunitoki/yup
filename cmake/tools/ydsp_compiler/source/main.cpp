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

#include <yup_dsp_jit/yup_dsp_jit.h>

#include <iostream>

using namespace yup;

static void printHelp()
{
    std::cerr << "yup_dsp_compiler <input.ydsp> --output <output.ydsb> [--target <os>-<arch>]... [--fast-math]\n"
              << "yup_dsp_compiler --inspect <bundle.ydsb> [--list]\n";
}

static std::optional<YdspTargetTriple> parseTarget (const String& value)
{
    if (value == "macos-arm64")
        return YdspTargetTriple { YdspTargetOperatingSystem::macosTarget, YdspTargetArchitecture::arm64 };

    if (value == "macos-x64")
        return YdspTargetTriple { YdspTargetOperatingSystem::macosTarget, YdspTargetArchitecture::x64 };

    if (value == "linux-arm64")
        return YdspTargetTriple { YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::arm64 };

    if (value == "linux-x64")
        return YdspTargetTriple { YdspTargetOperatingSystem::linuxTarget, YdspTargetArchitecture::x64 };

    if (value == "windows-arm64")
        return YdspTargetTriple { YdspTargetOperatingSystem::windowsTarget, YdspTargetArchitecture::arm64 };

    if (value == "windows-x64")
        return YdspTargetTriple { YdspTargetOperatingSystem::windowsTarget, YdspTargetArchitecture::x64 };

    return {};
}

static int inspectBundle (const File& file, bool listSources)
{
    auto loaded = YdspBundle::loadFromFile (file);
    if (loaded.failed())
    {
        std::cerr << loaded.getErrorMessage().toStdString() << "\n";
        return 2;
    }

    const auto& bundle = loaded.getReference();
    std::cout << "sources: " << bundle.getSources().size() << "\n"
              << "diagnostics: " << bundle.getDiagnostics().getCount() << "\n"
              << "native targets: " << bundle.getNativeTargets().joinIntoString (", ").toStdString() << "\n";

    if (listSources)
    {
        for (const auto& source : bundle.getSources())
            std::cout << source.id.toStdString() << (source.isRoot ? " (root)" : "") << "\n";
    }

    return 0;
}

static String readLine()
{
    String result;
    char character = 0;

    while (std::cin.get (character) && character != '\n')
        result += character;

    return result;
}

static String jsonString (const String& text, StringRef key, int from = 0)
{
    const auto keyPosition = text.indexOf (from, "\"" + String (key) + "\"");
    if (keyPosition < 0)
        return {};

    auto start = text.indexOf (keyPosition + key.length() + 2, "\"");
    if (start < 0)
        return {};
    ++start;

    String result;
    for (auto i = start; i < text.length(); ++i)
    {
        if (text[i] == '"')
            return result;

        if (text[i] == '\\' && i + 1 < text.length())
        {
            switch (text[++i])
            {
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'n': result += '\n'; break;
                case 'r': result += '\r'; break;
                case 't': result += '\t'; break;
                case '\\': result += '\\'; break;
                case '/': result += '/'; break;
                case '"': result += '"'; break;
                default: result += text[i]; break;
            }
            continue;
        }

        result += text[i];
    }
    return {};
}

static String jsonEscape (const String& value)
{
    String result;
    for (auto i = 0; i < value.length(); ++i)
    {
        const auto character = value[i];

        if (character == '\\' || character == '"') result += '\\';

        if (character == '\n') result += "\\n";
        else if (character == '\r') result += "\\r";
        else result += character;
    }
    return result;
}

static String jsonId (const String& text)
{
    const auto keyPosition = text.indexOf ("\"id\"");
    if (keyPosition < 0) return "null";

    const auto valueStart = text.indexOfAnyOf ("0123456789\"", keyPosition + 4);
    if (valueStart < 0) return "null";

    if (text[valueStart] == '"')
    {
        const auto end = text.indexOf (valueStart + 1, "\"");
        return end < 0 ? String ("null") : text.substring (valueStart, end + 1);
    }

    const auto end = text.indexOfAnyOf (",}", valueStart);
    return text.substring (valueStart, end < 0 ? text.length() : end);
}

static void sendLsp (const String& body)
{
    std::cout << "Content-Length: " << body.getNumBytesAsUTF8() << "\r\n\r\n";
    std::cout.write (body.toRawUTF8(), static_cast<std::streamsize> (body.getNumBytesAsUTF8()));
    std::cout.flush();
}

static int runLsp()
{
    auto headers = readLine();
    while (headers.isNotEmpty())
    {
        if (! headers.startsWith ("Content-Length:"))
        {
            headers = readLine();
            continue;
        }

        const auto length = static_cast<size_t> (headers.substring (15).getIntValue());
        readLine();

        MemoryBlock messageData (length, false);
        std::cin.read (static_cast<char*> (messageData.getData()), static_cast<std::streamsize> (length));

        const auto message = String::fromUTF8 (static_cast<const char*> (messageData.getData()), static_cast<int> (length));
        const auto method = jsonString (message, "method");
        const auto id = jsonId (message);
        if (method == "initialize")
        {
            sendLsp ("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":{\"capabilities\":{\"textDocumentSync\":1}}}");
        }
        else if (method == "shutdown")
        {
            sendLsp ("{\"jsonrpc\":\"2.0\",\"id\":" + id + ",\"result\":null}");
        }
        else if (method == "textDocument/didOpen" || method == "textDocument/didChange")
        {
            const auto uri = jsonString (message, "uri");
            const auto source = jsonString (message, "text", message.indexOf ("textDocument"));
            const auto documentUrl = URL (uri);
            const auto importBasePath = documentUrl.isLocalFile() ? documentUrl.getLocalFile().getFullPathName() : String {};

            YdspCompiler compiler;
            compiler.compile (source, importBasePath);
            const auto& diagnostics = compiler.getDiagnostics();

            String items = "[";
            bool hasDiagnostic = false;
            for (int i = 0; i < diagnostics.getCount(); ++i)
            {
                const auto& item = diagnostics.getItem (i);
                if (item.severity == YdspSeverity::info)
                    continue;

                if (hasDiagnostic) items += ',';
                hasDiagnostic = true;
                const auto line = jmax (0, item.range.startLine > 0 ? item.range.startLine - 1 : 0);
                const auto column = jmax (0, item.range.startColumn > 0 ? item.range.startColumn - 1 : 0);
                const auto endLine = jmax (line, item.range.endLine > 0 ? item.range.endLine - 1 : line);
                const auto endColumn = jmax (column + 1, item.range.endColumn > 0 ? item.range.endColumn : column + 1);
                items += "{\"range\":{\"start\":{\"line\":" + String (line)
                       + ",\"character\":" + String (column) + "},\"end\":{\"line\":"
                       + String (endLine) + ",\"character\":" + String (endColumn)
                       + "}},\"message\":\"" + jsonEscape (item.message)
                       + "\",\"severity\":" + String (item.severity == YdspSeverity::error ? 1 : 2) + "}";
            }
            items += ']';

            sendLsp ("{\"jsonrpc\":\"2.0\",\"method\":\"textDocument/publishDiagnostics\",\"params\":{\"uri\":\""
                     + jsonEscape (uri) + "\",\"diagnostics\":" + items + "}}");
        }
        else if (method == "exit")
        {
            return 0;
        }

        headers = readLine();
    }
    return 0;
}

int main (int argc, char** argv)
{
    if (argc > 1 && String (argv[1]) == "--lsp")
        return runLsp();

    if (argc < 2)
    {
        printHelp();
        return 2;
    }

    File input;
    File output;
    bool fastMath = false;
    bool listSources = false;
    String inspectPath;
    std::vector<YdspTargetTriple> targets;

    for (int i = 1; i < argc; ++i)
    {
        const String argument (argv[i]);

        if (argument == "--help" || argument == "-h")
        {
            printHelp();
            return 0;
        }

        if (argument == "--fast-math")
        {
            fastMath = true;
            continue;
        }

        if (argument == "--list")
        {
            listSources = true;
            continue;
        }

        if (argument == "--inspect" && i + 1 < argc)
        {
            inspectPath = argv[++i];
            continue;
        }

        if (argument == "--target" && i + 1 < argc)
        {
            const auto target = parseTarget (argv[++i]);
            if (! target)
            {
                std::cerr << "invalid target triple\n";
                return 2;
            }

            targets.push_back (*target);
            continue;
        }

        if (argument == "--output" && i + 1 < argc)
        {
            output = File (argv[++i]);
            continue;
        }

        if (input == File() && ! argument.startsWithChar ('-'))
        {
            input = File (argument);
        }
        else
        {
            std::cerr << "invalid argument: " << argv[i] << "\n";
            return 2;
        }
    }

    if (inspectPath.isNotEmpty())
        return inspectBundle (File (inspectPath), listSources);

    if (input == File() || output == File())
    {
        printHelp();
        return 2;
    }

    YdspCompiler compiler;

    YdspBundleCompileOptions options;
    options.fastMath = fastMath;
    options.nativeTargets = std::move (targets);

    const auto result = compiler.compileBundle (input.loadFileAsString(), options, input.getFullPathName());
    if (! result)
    {
        std::cerr << compiler.getDiagnostics().toString().toStdString() << "\n";
        return 1;
    }

    if (result.getReference().saveToFile (output).failed())
        return 3;

    return 0;
}
