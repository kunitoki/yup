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

namespace yup
{

class AudioPluginInstance;

/**
    Abstract interface implemented by each native plugin format backend.

    One concrete subclass exists per supported format (VST3, CLAP, AUv2).
    Clients register format objects with AudioPluginScanner.

    scanFile() may be called from scanner worker threads. Other methods must
    be safe for the host call site that invokes them.
*/
class AudioPluginFormat
{
public:
    virtual ~AudioPluginFormat() = default;

    //==============================================================================

    /** Returns the format type this backend handles. */
    virtual AudioPluginFormatType getFormatType() const = 0;

    /** Returns a human-readable name for the format (e.g. "VST3"). */
    virtual String getFormatName() const = 0;

    /**
        Returns the file extensions this format recognises, including the leading dot
        (e.g. { ".vst3" } or { ".clap" }).

        The scanner uses these extensions to filter filesystem entries before calling
        scanFile(). Return an empty array for registry-based formats (e.g. AUv2) that
        do not perform file-system scanning — the scanner will call scanFile() with an
        invalid File instead.

        Platform-specific extensions (e.g. ".dll" on Windows, ".so" on Linux) should
        be returned conditionally so only the relevant extensions are active on each
        platform.
    */
    virtual StringArray getFileExtensions() const = 0;

    //==============================================================================

    /**
        Returns the platform-standard search paths for this format.

        - VST3 macOS:   /Library/Audio/Plug-Ins/VST3, ~/Library/Audio/Plug-Ins/VST3
        - VST3 Windows: %CommonProgramFiles%\VST3, %APPDATA%\VST3
        - VST3 Linux:   /usr/lib/vst3, ~/.vst3
        - CLAP macOS:   /Library/Audio/Plug-Ins/CLAP, ~/Library/Audio/Plug-Ins/CLAP
        - CLAP Windows: %CommonProgramFiles%\CLAP, %APPDATA%\CLAP
        - CLAP Linux:   /usr/lib/clap, ~/.clap
        - AUv2:         Returns empty — AUv2 discovery uses AudioComponent registry.
    */
    virtual FileSearchPath getDefaultSearchPaths() const = 0;

    //==============================================================================

    /**
        Scans one file or bundle and returns all plugin descriptions it contains.

        Returns a failure result if the file is not a supported plugin for this
        format. The scanner calls this only for files whose extension matches
        expectations (e.g. ".vst3", ".clap").

        @param file   Absolute path to a file or bundle.
        @return       All descriptions found, or a failure message.
    */
    virtual ResultValue<std::vector<AudioPluginDescription>> scanFile (const File& file) = 0;

    //==============================================================================

    /**
        Loads and activates a plugin described by @p description.

        The returned instance is ready for prepareToPlay() but is not yet
        prepared. Returns a failure result on any error.

        @param description  A description previously produced by scanFile().
        @param context      Host context supplying sample rate, block size, etc.
        @return             Owning pointer to the live instance, or a failure.
    */
    virtual ResultValue<std::unique_ptr<AudioPluginInstance>> loadPlugin (
        const AudioPluginDescription& description,
        const AudioPluginHostContext& context) = 0;
};

} // namespace yup
