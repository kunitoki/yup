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

#pragma once

namespace yup
{

enum class YdspTargetOperatingSystem { macos, linux, windows };
enum class YdspTargetArchitecture { arm64, x64 };

struct YdspTargetTriple
{
    YdspTargetOperatingSystem operatingSystem = YdspTargetOperatingSystem::macos;
    YdspTargetArchitecture architecture = YdspTargetArchitecture::x64;
};

struct YdspBundleCompileOptions
{
    std::vector<YdspTargetTriple> nativeTargets;
    bool includeWasm = true;
    bool fastMath = false;
};

class YUP_API YdspBundle final
{
public:
    struct SourceFile
    {
        String id;
        String source;
        bool isRoot = false;
    };

    struct ImportEdge
    {
        String importingSourceId;
        String spelling;
        String importedSourceId;
    };

    YdspBundle (YdspBundle&& other) = default;
    YdspBundle& operator= (YdspBundle&& other) = default;

    Result saveToStream (OutputStream&) const;
    Result saveToFile (const File&) const;
    Result saveToMemoryBlock (MemoryBlock&) const;

    static ResultValue<YdspBundle> loadFromStream (InputStream&);
    static ResultValue<YdspBundle> loadFromFile (const File&);
    static ResultValue<YdspBundle> loadFromData (const void*, size_t);
    static ResultValue<YdspBundle> loadFromMemoryBlock (const MemoryBlock&);

    ResultValue<YdspAudioGraph> instantiate() const;

    const YdspDiagnostics& getDiagnostics() const noexcept { return diagnostics; }
    const StringArray& getNativeTargets() const noexcept { return nativeTargets; }
    const std::vector<SourceFile>& getSources() const noexcept { return sources; }

private:
    friend class YdspCompiler;

    YdspBundle() = default;

    std::vector<SourceFile> sources;
    std::vector<ImportEdge> importEdges;
    YdspDiagnostics diagnostics;
    StringArray nativeTargets;
    bool hasWasm = false;
    bool fastMath = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspBundle)
};

} // namespace yup
