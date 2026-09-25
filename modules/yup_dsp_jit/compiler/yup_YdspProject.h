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

/** A YAML .ydsp-project manifest, loaded on the control thread.

    Version 1 requires formatVersion: 1, a nonempty sources list and a main
    processor or graph name. Source paths resolve relative to the manifest.
    Optional string metadata includes id, name, version, description,
    manufacturer, author, license and category; isInstrument is a boolean.
    Files retain separate scopes. References between files require explicit
    imports, which resolve relative to the importing file. Every imported
    source must be listed in the manifest.
*/
class YUP_API YdspProject
{
public:
    /** Parses a manifest. projectFile identifies its path and base directory.
        Errors include source excerpts in diagnostics; no source files are read. */
    static ResultValue<YdspProject> parse (StringRef text, const File& projectFile, YdspDiagnostics& diagnostics);

    /** Reads and validates a manifest from disk. Does not compile DSP code. */
    static ResultValue<YdspProject> load (const File& projectFile, YdspDiagnostics& diagnostics);

    /** Returns the manifest file, used to resolve relative source paths. */
    const File& getFile() const noexcept { return file; }

    /** Returns the declared source paths in manifest order. */
    const StringArray& getSources() const noexcept { return sources; }

    /** Returns the default entry-point name, before any host override. */
    const String& getMain() const noexcept { return main; }

    /** Returns all validated manifest properties, including patch metadata. */
    const var& getMetadata() const noexcept { return metadata; }

private:
    File file;
    StringArray sources;
    String main;
    var metadata;
};

} // namespace yup
