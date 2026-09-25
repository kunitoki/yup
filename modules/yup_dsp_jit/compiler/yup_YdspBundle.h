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

    bool operator== (const YdspTargetTriple&) const noexcept = default;
};

/** Position-independent native kernel and its load-time helper bindings.

    Code contains no addresses from the compiling process. Each symbol names
    an eight-byte address slot resolved by the native loader. Native artifacts
    are executable code and must only be loaded from trusted sources.
*/
struct YdspNativeArtifact
{
    struct Symbol
    {
        String name;
        uint64_t offset = 0;
    };

    std::vector<uint8_t> code;
    std::vector<Symbol> symbols;
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
    /** Preserve trace recording in bundled kernels. Disabled by default. */
    bool enableTracing = false;
};

//==============================================================================
/** A portable DSP bundle containing sources and compiled target artifacts.

    Native kernels are emitted for the requested OS/architecture pairs without
    executing them. Instantiation rebuilds graph metadata from the bundled
    source closure and loads a matching artifact without regenerating code or
    reading imports from disk. WebAssembly builds use the stored wasm32 modules.

    This class is movable but not copyable. Native bundles contain executable
    code and must only be loaded from trusted sources.
*/
class YUP_API YdspBundle final
{
public:
    //==============================================================================
    /** Kernels for one OS/architecture, in compiler kernel/handler order.
        Bundles use the architecture's baseline instruction set.
    */
    struct NativeTarget
    {
        YdspTargetTriple target;
        std::vector<YdspNativeArtifact> kernels;
    };

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
    /** Instantiates a matching compiled target on the control thread.

        Fails if no artifact matches the running OS/architecture, or if graph
        reconstruction or helper resolution fails. No source-compilation
        fallback is performed. Call prepare() before processing audio.

        @return The instantiated graph, or a descriptive failure.
    */
    ResultValue<YdspAudioGraph> instantiate() const;

    //==============================================================================
    /** Get the diagnostics for the YdspBundle. */
    const YdspDiagnostics& getDiagnostics() const noexcept { return diagnostics; }

    /** Get the compile options for the YdspBundle. */
    const StringArray& getNativeTargets() const noexcept { return nativeTargets; }

    /** Returns the emitted native artifacts, including foreign targets. */
    const std::vector<NativeTarget>& getNativeArtifacts() const noexcept { return nativeArtifacts; }

    /** Returns emitted wasm32 modules in compiler kernel/handler order. */
    const std::vector<std::vector<uint8_t>>& getWasmModules() const noexcept { return wasmModules; }

    /** Returns the packaged root and imported source files. */
    const std::vector<SourceFile>& getSources() const noexcept { return sources; }

private:
    friend class YdspCompiler;

    YdspBundle() = default;

    String projectMain;
    var projectMetadata;
    std::vector<SourceFile> sources;
    std::vector<ImportEdge> importEdges;
    YdspDiagnostics diagnostics;
    StringArray nativeTargets;
    std::vector<NativeTarget> nativeArtifacts;
    std::vector<std::vector<uint8_t>> wasmModules;
    bool hasWasm = false;
    bool fastMath = false;
    /** Preserve trace recording in bundled kernels. Disabled by default. */
    bool enableTracing = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspBundle)
};

} // namespace yup
