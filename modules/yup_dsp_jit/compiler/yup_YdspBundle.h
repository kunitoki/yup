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

//==============================================================================
/** Operating system and architecture pair for which a YdspBundle can be compiled.

    This is used to specify the target platforms for which a YdspBundle should be compiled.
*/
enum class YdspTargetOperatingSystem
{
    macosTarget,
    linuxTarget,
    windowsTarget
};

/** Target architecture for which a YdspBundle can be compiled.

    This is used to specify the target architecture for which a YdspBundle should be compiled.
*/
enum class YdspTargetArchitecture
{
    arm64,
    x64
};

/** Operating system and architecture triple for which a YdspBundle can be compiled.

    This is used to specify the target operating system and architecture for which a YdspBundle should be compiled.
*/
struct YdspTargetTriple
{
    /** The target operating system. */
    YdspTargetOperatingSystem operatingSystem = YdspTargetOperatingSystem::macosTarget;

    /** The target architecture. */
    YdspTargetArchitecture architecture = YdspTargetArchitecture::x64;
};

//==============================================================================
/** Compile options for a YdspBundle.

    This is used to specify the compile options for a YdspBundle.
*/
struct YdspBundleCompileOptions
{
    std::vector<YdspTargetTriple> nativeTargets;
    bool includeWasm = true;
    bool fastMath = false;
};

//==============================================================================
/** A YdspBundle represents a compiled DSP bundle.

    It contains the compiled sources, import edges, diagnostics, and compile options for a DSP bundle.
    A YdspBundle can be saved to a file, loaded from a file, and instantiated into a YdspAudioGraph.

    The YdspBundle class is non-copyable and non-movable, and should be used with std::unique_ptr or std::shared_ptr.
*/
class YUP_API YdspBundle final
{
public:
    //==============================================================================
    /** Source file for a YdspBundle. */
    struct SourceFile
    {
        String id;
        String source;
        bool isRoot = false;
    };

    /** Import edge for a YdspBundle. */
    struct ImportEdge
    {
        String importingSourceId;
        String spelling;
        String importedSourceId;
    };
    
    //==============================================================================
    /** Move constructor and move assignment operator. */
    YdspBundle (YdspBundle&& other) noexcept = default;
    YdspBundle& operator= (YdspBundle&& other) noexcept = default;

    //==============================================================================
    /** Save the YdspBundle to a stream, file, or memory block.
    
        @param stream The output stream to save the bundle to.

        @return A Result indicating success or failure.
    */
    Result saveToStream (OutputStream& stream) const;

    /** Save the YdspBundle to a file.

        @param file The file to save the bundle to.

        @return A Result indicating success or failure.
    */
    Result saveToFile (const File& file) const;

    /** Save the YdspBundle to a memory block.

        @param memoryBlock The memory block to save the bundle to.

        @return A Result indicating success or failure.
    */
    Result saveToMemoryBlock (MemoryBlock& memoryBlock) const;

    //==============================================================================
    /** Load a YdspBundle from a stream, file, or memory block.
    
        @param stream The input stream to load the bundle from.

        @return A ResultValue containing the loaded YdspBundle or an error.
    */
    static ResultValue<YdspBundle> loadFromStream (InputStream& stream);

    /** Load a YdspBundle from a file.
    
        @param file The file to load the bundle from.

        @return A ResultValue containing the loaded YdspBundle or an error.
    */
    static ResultValue<YdspBundle> loadFromFile (const File& file);

    /** Load a YdspBundle from a memory block.
    
        @param memoryBlock The memory block to load the bundle from.

        @return A ResultValue containing the loaded YdspBundle or an error.
    */
    static ResultValue<YdspBundle> loadFromData (const void* data, size_t size);
    
    /** Load a YdspBundle from a memory block.

        @param memoryBlock The memory block to load the bundle from.

        @return A ResultValue containing the loaded YdspBundle or an error.
    */
    static ResultValue<YdspBundle> loadFromMemoryBlock (const MemoryBlock& memoryBlock);

    //==============================================================================
    /** Instantiate the YdspBundle into a YdspAudioGraph.

        @return A ResultValue containing the instantiated YdspAudioGraph or an error.
    */
    ResultValue<YdspAudioGraph> instantiate() const;

    //==============================================================================
    /** Get the diagnostics for the YdspBundle. */
    const YdspDiagnostics& getDiagnostics() const noexcept { return diagnostics; }

    /** Get the compile options for the YdspBundle. */
    const StringArray& getNativeTargets() const noexcept { return nativeTargets; }

    /** Get whether the YdspBundle includes WebAssembly code. */
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
