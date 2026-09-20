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
/** Compiles YDSP source text into a realtime-runnable YdspAudioGraph.

    Compilation is a control-thread operation (it allocates and generates
    machine code); the resulting YdspAudioGraph runs in the audio callback.

    @see YdspAudioGraph
*/
class YUP_API YdspCompiler
{
public:
    /** Constructor. */
    YdspCompiler();

    /** Destructor. */
    ~YdspCompiler();

    YdspCompiler (YdspCompiler&&) = default;
    YdspCompiler& operator= (YdspCompiler&&) = default;

    //==============================================================================
    /** Compiles YDSP source text into a runnable graph.

        `import` directives in the source resolve relative to the directory of
        the importing file: nested imports inside an imported file resolve
        against that file's own folder, and the top-level source resolves
        against `importBasePath`. Pass the directory (or the file path) of the
        patch when the source text was read from disk so relative import paths
        like `import fx.Delay` resolve to the expected folder; when
        empty, top-level imports resolve relative to the process's current
        working directory.

        When `threadPool` is non-null, reading, lexing and parsing the
        imported files runs in parallel on that pool (the merge into the
        program stays single-threaded). Pass your own `ThreadPool`; the
        compiler never adds or removes jobs it does not own. When it is null,
        imports are resolved sequentially with identical results.

        On failure the returned ResultValue is a failure and the detailed
        diagnostics are available through getDiagnostics().
    */
    ResultValue<YdspAudioGraph> compile (StringRef source, StringRef importBasePath = {}, ThreadPool* threadPool = nullptr);

    /** Compiles YDSP source text with an explicit native-code policy.

        The import and thread-pool arguments have the same meaning as the
        backward-compatible overload above. `baselineTarget` is only used
        when options.targetPolicy is YdspTargetPolicy::baseline.
    */
    ResultValue<YdspAudioGraph> compile (StringRef source, const YdspCompileOptions& options, StringRef importBasePath = {}, ThreadPool* threadPool = nullptr);

    /** Parses, resolves imports, and checks semantics without generating code.
        Processor and function libraries do not need a graph. Sources containing
        a root graph still receive graph validation. Unsaved imports may be
        supplied through options.sourceOverrides. Diagnostics retain file ranges.
    */
    Result validate (StringRef source, const YdspCompileOptions& options = {}, StringRef importBasePath = {}, ThreadPool* threadPool = nullptr);

    /** Loads a .ydsp-project manifest and compiles its listed source files.

        Files retain separate scopes and require explicit imports. Each file
        retains its own diagnostic path and relative imports. All imported
        files must be listed. The selected main name must occur in exactly
        one listed file.
        mainOverride, when nonempty, replaces the manifest's main selection.
        A processor entry point exposes all its endpoints through a generated
        graph; an existing graph entry point uses its declared interface.
        All file IO, YAML parsing and compilation happen on the calling control
        thread, with explicit imports optionally parsed using threadPool.
    */
    ResultValue<YdspAudioGraph> compileProject (const File& projectFile, const YdspCompileOptions& options = {}, StringRef mainOverride = {}, ThreadPool* threadPool = nullptr);

    /** Compiles a project's selected processor or graph into a self-contained bundle.
        Explicit imports are resolved relative to their source files. mainOverride,
        when nonempty, replaces the manifest's main. No source files are needed
        when instantiating the resulting bundle.
    */
    ResultValue<YdspBundle> compileProjectBundle (const File& projectFile, const YdspBundleCompileOptions& options, StringRef mainOverride = {}, ThreadPool* threadPool = nullptr);

    /** Packages the source closure and emits every requested target without
        installing or executing kernels. Native targets use baseline scalar
        code so bundles do not depend on the compiling machine's CPU features.
        An empty nativeTargets list requests no native artifacts.
    */
    ResultValue<YdspBundle> compileBundle (StringRef source, const YdspBundleCompileOptions& options, StringRef importBasePath = {}, ThreadPool* threadPool = nullptr);

    /** Returns the diagnostics of the most recent compile. */
    const YdspDiagnostics& getDiagnostics() const noexcept;

    /** Returns the native-code report of the most recent compile.

        When the most recent options did not request a report, this returns an
        empty report whose generatedCodeSize and compileTimeMilliseconds are 0.
    */
    const YdspOptimizationReport& getOptimizationReport() const noexcept;

private:
    friend class YdspBundle;

    ResultValue<YdspAudioGraph> compileInternal (StringRef source, const YdspCompileOptions& options, StringRef importBasePath, ThreadPool* threadPool, YdspBundle* bundleOutput, const YdspBundle* bundleInput, const YdspBundleCompileOptions* bundleOptions, const YdspProject* project = nullptr, StringRef mainOverride = {}, bool validationOnly = false);

    struct Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (YdspCompiler)
};

} // namespace yup
