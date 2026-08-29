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

namespace
{

void renameLibraryCalls (YdspExpr& expr, const std::unordered_set<String>& names, const String& nsPrefix)
{
    if (expr.kind == YdspExprKind::call && ! expr.text.contains (".") && names.find (expr.text) != names.end())
        expr.text = nsPrefix + "." + expr.text;

    for (auto& child : expr.children)
        if (child != nullptr)
            renameLibraryCalls (*child, names, nsPrefix);

    for (auto& [paramName, valueExpr] : expr.overrides)
        if (valueExpr != nullptr)
            renameLibraryCalls (*valueExpr, names, nsPrefix);
}

void renameLibraryCalls (YdspStmt& stmt, const std::unordered_set<String>& names, const String& nsPrefix)
{
    for (auto* expr : { stmt.cond.get(), stmt.startExpr.get(), stmt.endExpr.get(), stmt.target.get(), stmt.value.get(), stmt.returnExpr.get() })
        if (expr != nullptr)
            renameLibraryCalls (*expr, names, nsPrefix);

    for (auto* child : { stmt.thenStmt.get(), stmt.elseStmt.get(), stmt.body.get() })
        if (child != nullptr)
            renameLibraryCalls (*child, names, nsPrefix);

    for (auto& child : stmt.children)
        if (child != nullptr)
            renameLibraryCalls (*child, names, nsPrefix);
}

void renameLibraryCalls (const std::vector<std::unique_ptr<YdspStmt>>& body, const std::unordered_set<String>& names, const String& nsPrefix)
{
    for (const auto& stmt : body)
        if (stmt != nullptr)
            renameLibraryCalls (*stmt, names, nsPrefix);
}

/** Maps a dotted import path and the importing file's resolved path to the
    imported file's resolved path: `import X.Y.Z` inside `lib/outer.ydsp`
    resolves to `lib/X/Y/Z.ydsp`. */
String resolveImportPath (StringRef dottedPath, const String& parentPath)
{
    String relativePath = String (dottedPath).replaceCharacter ('.', '/');
    relativePath += ".ydsp";

    if (parentPath.isNotEmpty())
    {
        const auto lastSlash = jmax (parentPath.lastIndexOfChar ('/'), parentPath.lastIndexOfChar ('\\'));
        return (lastSlash >= 0 ? parentPath.substring (0, lastSlash + 1) : String()) + relativePath;
    }

    return relativePath;
}

//==============================================================================
// Deep clone of the parsed AST

YdspStmtPtr cloneStmt (const YdspStmt& stmt)
{
    auto copy = std::make_unique<YdspStmt>();
    copy->kind = stmt.kind;
    copy->location = stmt.location;
    copy->name = stmt.name;
    copy->isLet = stmt.isLet;
    copy->hasDeclType = stmt.hasDeclType;
    copy->declType = stmt.declType;

    for (const auto& child : stmt.children)
        copy->children.push_back (child != nullptr ? cloneStmt (*child) : nullptr);

    copy->cond = stmt.cond != nullptr ? YdspExprFactory::clone (*stmt.cond) : nullptr;
    copy->thenStmt = stmt.thenStmt != nullptr ? cloneStmt (*stmt.thenStmt) : nullptr;
    copy->elseStmt = stmt.elseStmt != nullptr ? cloneStmt (*stmt.elseStmt) : nullptr;
    copy->startExpr = stmt.startExpr != nullptr ? YdspExprFactory::clone (*stmt.startExpr) : nullptr;
    copy->endExpr = stmt.endExpr != nullptr ? YdspExprFactory::clone (*stmt.endExpr) : nullptr;
    copy->body = stmt.body != nullptr ? cloneStmt (*stmt.body) : nullptr;
    copy->target = stmt.target != nullptr ? YdspExprFactory::clone (*stmt.target) : nullptr;
    copy->value = stmt.value != nullptr ? YdspExprFactory::clone (*stmt.value) : nullptr;
    copy->returnExpr = stmt.returnExpr != nullptr ? YdspExprFactory::clone (*stmt.returnExpr) : nullptr;

    copy->shapeName = stmt.shapeName;
    copy->endpointName = stmt.endpointName;

    for (const auto& [fieldName, valueExpr] : stmt.emitFields)
        copy->emitFields.emplace_back (fieldName, valueExpr != nullptr ? YdspExprFactory::clone (*valueExpr) : nullptr);

    return copy;
}

void cloneBody (const std::vector<YdspStmtPtr>& source, std::vector<YdspStmtPtr>& target)
{
    for (const auto& stmt : source)
        target.push_back (stmt != nullptr ? cloneStmt (*stmt) : nullptr);
}

YdspEndpointDecl cloneEndpoint (const YdspEndpointDecl& endpoint)
{
    YdspEndpointDecl copy;
    copy.kind = endpoint.kind;
    copy.type = endpoint.type;
    copy.name = endpoint.name;
    copy.channelCount = endpoint.channelCount;
    copy.location = endpoint.location;
    copy.defaultValue = endpoint.defaultValue != nullptr ? YdspExprFactory::clone (*endpoint.defaultValue) : nullptr;
    copy.annotations = endpoint.annotations;
    return copy;
}

YdspStateDecl cloneState (const YdspStateDecl& state)
{
    YdspStateDecl copy;
    copy.type = state.type;
    copy.name = state.name;
    copy.arraySize = state.arraySize;
    copy.arraySizeName = state.arraySizeName;
    copy.structName = state.structName;
    copy.location = state.location;

    for (const auto& initialiser : state.initialisers)
        copy.initialisers.push_back (initialiser != nullptr ? YdspExprFactory::clone (*initialiser) : nullptr);

    copy.annotations = state.annotations;
    return copy;
}

YdspStructDecl cloneStruct (const YdspStructDecl& decl)
{
    YdspStructDecl copy;
    copy.name = decl.name;
    copy.location = decl.location;
    copy.fields = decl.fields;
    return copy;
}

YdspProcessDecl cloneProcess (const YdspProcessDecl& process)
{
    YdspProcessDecl copy;
    copy.mode = process.mode;
    copy.location = process.location;
    cloneBody (process.body, copy.body);
    return copy;
}

YdspEventHandlerDecl cloneEventHandler (const YdspEventHandlerDecl& handler)
{
    YdspEventHandlerDecl copy;
    copy.endpointName = handler.endpointName;
    copy.shapeName = handler.shapeName;
    copy.paramName = handler.paramName;
    copy.location = handler.location;
    cloneBody (handler.body, copy.body);
    return copy;
}

YdspFuncDecl cloneFunc (const YdspFuncDecl& func)
{
    YdspFuncDecl copy;
    copy.name = func.name;
    copy.location = func.location;
    copy.params = func.params;
    copy.returnType = func.returnType;
    copy.hasReturnType = func.hasReturnType;
    cloneBody (func.body, copy.body);
    return copy;
}

std::unique_ptr<YdspProcessorDecl> cloneProcessor (const YdspProcessorDecl& processor)
{
    auto copy = std::make_unique<YdspProcessorDecl>();
    copy->name = processor.name;
    copy->location = processor.location;

    for (const auto& endpoint : processor.endpoints)
        copy->endpoints.push_back (cloneEndpoint (endpoint));

    for (const auto& decl : processor.structs)
        copy->structs.push_back (cloneStruct (decl));

    for (const auto& state : processor.states)
        copy->states.push_back (cloneState (state));

    for (const auto& func : processor.functions)
        copy->functions.push_back (cloneFunc (func));

    for (const auto& handler : processor.eventHandlers)
        copy->eventHandlers.push_back (cloneEventHandler (handler));

    copy->process = processor.process != nullptr ? std::make_unique<YdspProcessDecl> (cloneProcess (*processor.process)) : nullptr;
    copy->init = processor.init != nullptr ? std::make_unique<YdspProcessDecl> (cloneProcess (*processor.init)) : nullptr;
    copy->annotations = processor.annotations;

    return copy;
}

YdspNodeDecl cloneNode (const YdspNodeDecl& node)
{
    YdspNodeDecl copy;
    copy.instanceName = node.instanceName;
    copy.processorName = node.processorName;
    copy.location = node.location;
    copy.voiceCount = node.voiceCount;
    copy.rateMultiplier = node.rateMultiplier;
    copy.rateDivider = node.rateDivider;

    for (const auto& [paramName, valueExpr] : node.overrides)
        copy.overrides.emplace_back (paramName, valueExpr != nullptr ? YdspExprFactory::clone (*valueExpr) : nullptr);

    copy.annotations = node.annotations;
    return copy;
}

YdspGraphDecl cloneGraph (const YdspGraphDecl& graph)
{
    YdspGraphDecl copy;
    copy.name = graph.name;
    copy.location = graph.location;

    for (const auto& endpoint : graph.endpoints)
        copy.endpoints.push_back (cloneEndpoint (endpoint));

    for (const auto& node : graph.nodes)
        copy.nodes.push_back (cloneNode (node));

    copy.annotations = graph.annotations;
    copy.isImported = graph.isImported;
    copy.bodyKind = graph.bodyKind;
    copy.connections = graph.connections;
    copy.algebraRoot = graph.algebraRoot != nullptr ? YdspExprFactory::clone (*graph.algebraRoot) : nullptr;

    return copy;
}

YdspLetDecl cloneLet (const YdspLetDecl& decl)
{
    YdspLetDecl copy;
    copy.name = decl.name;
    copy.value = decl.value != nullptr ? YdspExprFactory::clone (*decl.value) : nullptr;
    copy.location = decl.location;
    return copy;
}

std::unique_ptr<YdspProgram> cloneProgram (const YdspProgram& program)
{
    auto copy = std::make_unique<YdspProgram>();
    copy->declares = program.declares;
    copy->imports = program.imports;

    for (const auto& constant : program.constants)
        copy->constants.push_back (cloneLet (constant));

    for (const auto& func : program.functions)
        copy->functions.push_back (cloneFunc (func));

    for (const auto& processor : program.processors)
        copy->processors.push_back (std::move (*cloneProcessor (processor)));

    for (const auto& graph : program.graphs)
        copy->graphs.push_back (cloneGraph (graph));

    for (const auto& processor : program.synthesizedProcessors)
        copy->synthesizedProcessors.push_back (cloneProcessor (*processor));

    return copy;
}

/** Merges an imported program's constants, functions, processors and graphs
    into `prog` under the namespace prefix. Plain-name calls inside library
    function bodies follow the rename, so `wrap` calling `scale (x)` becomes
    `nsPrefix.scale (x)` once the declarations are prefixed. */
void mergeImportedDecls (YdspProgram& prog, YdspProgram& importedProg, const String& nsPrefix)
{
    // Imported program constants come first: the importing file's own
    // declarations may refer to them (e.g. `state float buf[fx.maxDelay];`).
    for (auto& importedConstant : importedProg.constants)
    {
        importedConstant.name = nsPrefix + "." + importedConstant.name;
        prog.constants.push_back (std::move (importedConstant));
    }

    // Imported program functions, like constants. Only the file's own
    // non-dotted function names are rewritten, and intrinsic names are left
    // alone so a library function named like a builtin keeps resolving to the
    // builtin, matching processor-scope shadowing.
    std::unordered_set<String> libraryFunctionNames;

    for (const auto& importedFunction : importedProg.functions)
        if (! importedFunction.name.contains (".") && ! isIntrinsicName (importedFunction.name))
            libraryFunctionNames.insert (importedFunction.name);

    for (auto& importedFunction : importedProg.functions)
    {
        renameLibraryCalls (importedFunction.body, libraryFunctionNames, nsPrefix);
        importedFunction.name = nsPrefix + "." + importedFunction.name;
        prog.functions.push_back (std::move (importedFunction));
    }

    // The same plain-name calls inside imported processor bodies follow the
    // rename, unless a processor-local function of that name shadows them
    // (those keep resolving locally and must stay plain).
    for (auto& importedProcessor : importedProg.processors)
    {
        auto localNames = libraryFunctionNames;

        for (const auto& localFunc : importedProcessor.functions)
            localNames.erase (localFunc.name);

        for (auto& localFunc : importedProcessor.functions)
            renameLibraryCalls (localFunc.body, localNames, nsPrefix);

        for (auto& endpoint : importedProcessor.endpoints)
            if (endpoint.defaultValue != nullptr)
                renameLibraryCalls (*endpoint.defaultValue, localNames, nsPrefix);

        for (auto& state : importedProcessor.states)
            for (auto& initialiser : state.initialisers)
                if (initialiser != nullptr)
                    renameLibraryCalls (*initialiser, localNames, nsPrefix);

        if (importedProcessor.process != nullptr)
            renameLibraryCalls (importedProcessor.process->body, localNames, nsPrefix);

        if (importedProcessor.init != nullptr)
            renameLibraryCalls (importedProcessor.init->body, localNames, nsPrefix);

        for (auto& handler : importedProcessor.eventHandlers)
            renameLibraryCalls (handler.body, localNames, nsPrefix);
    }

    // Imported processors.
    for (auto& importedProcessor : importedProg.processors)
    {
        importedProcessor.name = nsPrefix + "." + importedProcessor.name;
        prog.processors.push_back (std::move (importedProcessor));
    }

    // Imported graphs. Their nodes name targets that were just prefixed, so
    // those references move with them. The recursion that ran before this has
    // already prefixed anything the imported file itself imported, which is
    // what makes a two-level import resolve to "outer.inner.Name".
    for (auto& importedGraph : importedProg.graphs)
    {
        importedGraph.name = nsPrefix + "." + importedGraph.name;

        for (auto& node : importedGraph.nodes)
        {
            node.processorName = nsPrefix + "." + node.processorName;

            for (auto& [paramName, valueExpr] : node.overrides)
                if (valueExpr != nullptr)
                    renameLibraryCalls (*valueExpr, libraryFunctionNames, nsPrefix);
        }

        // An imported graph is a library component: it may well be its own
        // file's entry point, but never this program's.
        importedGraph.isImported = true;

        prog.graphs.push_back (std::move (importedGraph));
    }
}

} // namespace

bool YdspDiagnostics::hasErrors() const noexcept
{
    for (const auto& item : items)
        if (item.severity == YdspSeverity::error)
            return true;

    return false;
}

int YdspDiagnostics::getCount() const noexcept
{
    return static_cast<int> (items.size());
}

const YdspDiagnostic& YdspDiagnostics::getItem (int index) const noexcept
{
    jassert (static_cast<size_t> (index) < items.size());
    return items[static_cast<size_t> (index)];
}

void YdspDiagnostics::setSource (StringRef source)
{
    sourceText = String (source);
}

void YdspDiagnostics::addError (int line, int column, StringRef message)
{
    items.push_back ({ YdspSeverity::error, line, column, String (message) });
}

void YdspDiagnostics::addWarning (int line, int column, StringRef message)
{
    items.push_back ({ YdspSeverity::warning, line, column, String (message) });
}

void YdspDiagnostics::addInfo (int line, int column, StringRef message)
{
    items.push_back ({ YdspSeverity::info, line, column, String (message) });
}

int YdspDiagnostics::mark() const noexcept
{
    return static_cast<int> (items.size());
}

void YdspDiagnostics::rollbackTo (int marker)
{
    if (marker >= 0 && static_cast<size_t> (marker) < items.size())
        items.resize (static_cast<size_t> (marker));
}

String YdspDiagnostics::toString() const
{
    String result;

    for (const auto& item : items)
    {
        if (result.isNotEmpty())
            result += "\n";

        switch (item.severity)
        {
            case YdspSeverity::error:
                result += "error";
                break;
            case YdspSeverity::warning:
                result += "warning";
                break;
            case YdspSeverity::info:
                result += "info";
                break;
        }

        if (item.line > 0)
            result += ":" + String (item.line) + ":" + String (item.column);

        result += ": " + item.message;

        // Render the source line with a caret marker when source text is available.
        if (item.line > 0 && sourceText.isNotEmpty())
        {
            int lineStart = 0;

            for (int l = 1; l < item.line && lineStart < sourceText.length(); ++lineStart)
                if (sourceText[lineStart] == '\n')
                    ++l;

            const auto lineEnd = sourceText.indexOf (lineStart, "\n");
            const auto sourceLine = lineEnd >= 0 ? sourceText.substring (lineStart, lineEnd)
                                                 : sourceText.substring (lineStart);

            // Strip a trailing carriage return if present.
            auto displayLine = sourceLine;
            if (! displayLine.isEmpty() && displayLine.getLastCharacter() == '\r')
                displayLine = displayLine.dropLastCharacters (1);

            // Show line number and source.
            result += "\n  " + String (item.line) + " | " + displayLine;

            // Caret marker at the column (1-based).
            if (item.column > 0)
            {
                const auto padLength = String (item.line).length() + 3 + item.column - 1;
                result += "\n  " + String::repeatedString (" ", padLength) + "^";
            }
        }
    }

    return result;
}

//==============================================================================

// YdspCompiler

namespace
{

struct YdspHostTargetInfo
{
    YdspNativeTarget preferredTarget = YdspNativeTarget::scalar;
    bool supportsFusedMultiplyAdd = false;
    bool supportsSse2 = false;
    bool supportsAvx2 = false;
    bool supportsAvx512 = false;
    String microarchitecture;
};

struct YdspTargetSelection
{
    YdspNativeTarget target = YdspNativeTarget::scalar;
    int vectorWidth = 1;
    bool supportsFusedMultiplyAdd = false;
    StringArray rejectedTransforms;
};

class YdspCompileTimer
{
public:
    YdspCompileTimer (YdspOptimizationReport& reportToUpdate, bool shouldRecord) noexcept
        : report (reportToUpdate)
        , enabled (shouldRecord)
        , start (std::chrono::steady_clock::now())
    {
    }

    ~YdspCompileTimer()
    {
        if (enabled)
            report.compileTimeMilliseconds = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - start).count();
    }

private:
    YdspOptimizationReport& report;
    bool enabled = false;
    std::chrono::steady_clock::time_point start;
};

YdspHostTargetInfo detectHostTarget()
{
    YdspHostTargetInfo result;

#if YUP_WASM
    result.microarchitecture = "WebAssembly";
#elif ASMJIT_ARCH_X86
    const auto& cpu = asmjit::CpuInfo::host();
    const auto& features = cpu.features().x86();

    result.supportsFusedMultiplyAdd = features.has_fma();
    result.supportsSse2 = features.has_sse2();
    result.supportsAvx2 = features.has_avx2();
    result.supportsAvx512 = features.has_avx512_f() && features.has_avx512_dq() && features.has_avx512_vl();
    result.preferredTarget = result.supportsAvx2 ? YdspNativeTarget::avx2
                                                 : (result.supportsSse2 ? YdspNativeTarget::sse2 : YdspNativeTarget::scalar);
    result.microarchitecture = String (cpu.brand());

    if (result.microarchitecture.isEmpty())
        result.microarchitecture = "x86-64";
#elif ASMJIT_ARCH_ARM
    result.preferredTarget = YdspNativeTarget::asimd;
    result.supportsFusedMultiplyAdd = true;
    result.microarchitecture = "AArch64";
#else
    result.microarchitecture = "Unknown";
#endif

    return result;
}

bool targetIsAvailable (YdspNativeTarget target, const YdspHostTargetInfo& host) noexcept
{
    switch (target)
    {
        case YdspNativeTarget::scalar:
            return true;

        case YdspNativeTarget::sse2:
#if ! YUP_WASM && ASMJIT_ARCH_X86
            return host.supportsSse2;
#else
            return false;
#endif

        case YdspNativeTarget::avx2:
#if ! YUP_WASM && ASMJIT_ARCH_X86
            return host.supportsAvx2;
#else
            return false;
#endif

        case YdspNativeTarget::avx512:
#if ! YUP_WASM && ASMJIT_ARCH_X86
            return host.supportsAvx512;
#else
            return false;
#endif

        case YdspNativeTarget::asimd:
#if ! YUP_WASM && ASMJIT_ARCH_ARM
            return true;
#else
            return false;
#endif
    }

    return false;
}

String targetName (YdspNativeTarget target)
{
    switch (target)
    {
        case YdspNativeTarget::scalar: return "scalar";
        case YdspNativeTarget::sse2: return "SSE2";
        case YdspNativeTarget::avx2: return "AVX2";
        case YdspNativeTarget::avx512: return "AVX-512";
        case YdspNativeTarget::asimd: return "ASIMD";
    }

    return "unknown";
}

YdspTargetSelection selectTarget (const YdspCompileOptions& options, const YdspHostTargetInfo& host)
{
    YdspTargetSelection result;
    auto requested = options.targetPolicy == YdspTargetPolicy::host ? host.preferredTarget : options.baselineTarget;

    if (options.targetPolicy == YdspTargetPolicy::host && host.supportsAvx512)
        result.rejectedTransforms.add ("AVX-512 was rejected because no empirical microarchitecture cost model is available");

    // AVX-512 can lower clock frequency on several CPU families. Until the
    // compiler has target-class benchmark feedback, selecting it would turn a
    // capability check into an unjustified profitability claim. Keep AVX2 as
    // the wide x86 path and record the decision for callers that asked for it.
    if (requested == YdspNativeTarget::avx512)
    {
        if (options.targetPolicy != YdspTargetPolicy::host)
            result.rejectedTransforms.add ("AVX-512 was rejected because no empirical microarchitecture cost model is available");

        requested = host.supportsAvx2 ? YdspNativeTarget::avx2 : YdspNativeTarget::sse2;
    }

    if (! targetIsAvailable (requested, host))
    {
        result.rejectedTransforms.add ("Requested " + targetName (requested) + " target is unavailable on this host; using scalar code");
        requested = YdspNativeTarget::scalar;
    }

    result.target = requested;

    switch (requested)
    {
        case YdspNativeTarget::sse2:
        case YdspNativeTarget::asimd:
            result.vectorWidth = 4;
            break;

        case YdspNativeTarget::avx2:
            result.vectorWidth = 8;
            result.supportsFusedMultiplyAdd = host.supportsFusedMultiplyAdd;
            break;

        case YdspNativeTarget::scalar:
            break;

        case YdspNativeTarget::avx512:
            jassertfalse;
            break;
    }

    if (requested == YdspNativeTarget::asimd)
        result.supportsFusedMultiplyAdd = true;

#if YUP_WASM && defined (__wasm_simd128__)
    // wasm has no native-target tiers; -msimd128 enables the portable f32x4
    // lowering, a four-lane width. The scalar baseline tier stays scalar so
    // its report keeps vectorWidth 1.
    if (options.optimizationTier != YdspOptimizationTier::baseline)
        result.vectorWidth = 4;
#endif

    return result;
}

} // namespace

struct YdspCompiler::Pimpl
{
    Pimpl()
        : hostTarget (detectHostTarget())
    {
    }

    YdspDiagnostics diagnostics;
    YdspOptimizationReport optimizationReport;
    YdspHostTargetInfo hostTarget;
};

//==============================================================================

YdspCompiler::YdspCompiler()
    : pimpl (std::make_unique<Pimpl>())
{
}

YdspCompiler::~YdspCompiler() = default;

//==============================================================================

const YdspDiagnostics& YdspCompiler::getDiagnostics() const noexcept
{
    return pimpl->diagnostics;
}

//==============================================================================

const YdspOptimizationReport& YdspCompiler::getOptimizationReport() const noexcept
{
    return pimpl->optimizationReport;
}

//==============================================================================

ResultValue<YdspAudioGraph> YdspCompiler::compile (StringRef source, StringRef importBasePath, ThreadPool* threadPool)
{
    return compile (source, YdspCompileOptions {}, importBasePath, threadPool);
}

//==============================================================================

ResultValue<YdspAudioGraph> YdspCompiler::compile (StringRef source, const YdspCompileOptions& options, StringRef importBasePath, ThreadPool* threadPool)
{
    pimpl->diagnostics = YdspDiagnostics();
    pimpl->optimizationReport = YdspOptimizationReport {};

    auto& optimizationReport = pimpl->optimizationReport;
    YdspCompileTimer compileTimer (optimizationReport, options.emitOptimizationReport);
    const auto target = selectTarget (options, pimpl->hostTarget);

    if (options.emitOptimizationReport)
    {
        optimizationReport.optimizationTier = options.optimizationTier;
        optimizationReport.fastMath = options.fastMath;
        optimizationReport.selectedIsa = target.target;
        optimizationReport.selectedMicroarchitecture = pimpl->hostTarget.microarchitecture;
        optimizationReport.vectorWidth = target.vectorWidth;
        optimizationReport.rejectedTransforms = target.rejectedTransforms;
        optimizationReport.cacheDecision = "No persistent native-code cache is configured";
        optimizationReport.benchmarkDecision = "No benchmark feedback was supplied";
    }

    auto& diagnostics = pimpl->diagnostics;
    diagnostics.setSource (source);

    // 1. Lex
    YdspLexer lexer (source, diagnostics);
    auto tokens = lexer.tokenize();

    if (diagnostics.hasErrors())
        return ResultValue<YdspAudioGraph>::fail (diagnostics.getItem (0).message);

    // 2. Parse
    YdspParser parser (std::move (tokens), diagnostics);
    auto program = parser.parseProgram();
    if (program == nullptr || diagnostics.hasErrors())
        return ResultValue<YdspAudioGraph>::fail (diagnostics.getItem (0).message);

    // 2b. Resolve imports
    if (! program->imports.empty())
    {
        String basePath = importBasePath;
        if (basePath.isNotEmpty())
        {
            const auto isDirectory = File::getCurrentWorkingDirectory().getChildFile (basePath).isDirectory();

            if (! isDirectory)
            {
                const auto lastSlash = jmax (basePath.lastIndexOfChar ('/'), basePath.lastIndexOfChar ('\\'));

                if (lastSlash >= 0)
                    basePath = basePath.substring (0, lastSlash + 1);
                else
                    basePath.clear();
            }
            else if (basePath.getLastCharacter() != '/' && basePath.getLastCharacter() != '\\')
            {
                basePath += '/';
            }
        }

        // Phase A: read, lex and parse every reachable imported file, each
        // exactly once. Disk IO and lexing/parsing are isolated per file, so
        // they run in parallel on the caller's ThreadPool when one is provided
        // and inline otherwise; the merge into the program stays single
        // threaded (phase B), so the parallel path is deterministic.
        struct ParsedImport
        {
            std::unique_ptr<YdspProgram> program;
            bool found = false;                               // the file exists on disk
            std::vector<std::tuple<int, int, String>> errors; // (line, column, message)
        };

        std::mutex parseMutex; // guards parsedImports and queuedParses
        CountDownLatch parseLatch;
        std::unordered_map<String, ParsedImport> parsedImports;
        std::unordered_set<String> queuedParses;

        std::function<void (const String&)> parseOneFile;
        parseOneFile = [&] (const String& resolvedPath)
        {
            const auto importedFile = File::getCurrentWorkingDirectory().getChildFile (resolvedPath);
            ParsedImport parsed;

            if (importedFile.existsAsFile())
            {
                parsed.found = true;

                auto importedSource = importedFile.loadFileAsString();

                // Lex and parse the imported source. Each job owns its lexer,
                // parser and diagnostics, so no shared state is touched here.
                YdspDiagnostics importDiagnostics;
                YdspLexer importLexer (importedSource, importDiagnostics);
                auto importTokens = importLexer.tokenize();

                if (! importDiagnostics.hasErrors())
                {
                    YdspParser importParser (std::move (importTokens), importDiagnostics);
                    parsed.program = importParser.parseProgram();
                }

                for (int i = 0; i < importDiagnostics.getCount(); ++i)
                {
                    const auto& item = importDiagnostics.getItem (i);
                    parsed.errors.emplace_back (item.line, 0, importedFile.getFullPathName() + ":" + String (item.line) + ": " + item.message);
                }
            }
            else
            {
                parsed.errors.emplace_back (0, 0, "Imported file not found: '" + resolvedPath + "'");
            }

            // Discover the file's own imports so the graph stays fully parsed.
            std::vector<String> nestedPaths;

            if (parsed.program != nullptr)
                for (const auto& importDecl : parsed.program->imports)
                    nestedPaths.push_back (resolveImportPath (importDecl.path, resolvedPath));

            std::vector<String> toEnqueue;

            {
                std::lock_guard lock (parseMutex);

                parsedImports[resolvedPath] = std::move (parsed);

                for (const auto& nestedPath : nestedPaths)
                    if (queuedParses.insert (nestedPath).second)
                    {
                        toEnqueue.push_back (nestedPath);
                        parseLatch.addCount (1);
                    }
            }

            if (threadPool != nullptr)
            {
                for (const auto& nestedPath : toEnqueue)
                    threadPool->addJob ([&, nestedPath]
                    {
                        parseOneFile (nestedPath);
                    });
            }
            else
            {
                for (const auto& nestedPath : toEnqueue)
                    parseOneFile (nestedPath);
            }

            parseLatch.countDown();
        };

        // Seed the queue with the top-level program's imports. The queue is
        // deduplicated by resolved path, so one job runs per unique file.
        std::vector<String> initialPaths;

        {
            std::lock_guard lock (parseMutex);

            for (const auto& importDecl : program->imports)
            {
                const auto resolvedPath = resolveImportPath (importDecl.path, basePath);

                if (queuedParses.insert (resolvedPath).second)
                {
                    initialPaths.push_back (resolvedPath);
                    parseLatch.addCount (1);
                }
            }
        }

        for (const auto& resolvedPath : initialPaths)
        {
            if (threadPool != nullptr)
            {
                threadPool->addJob ([&, resolvedPath]
                {
                    parseOneFile (resolvedPath);
                });
            }
            else
            {
                parseOneFile (resolvedPath);
            }
        }

        parseLatch.wait();

        // Phase B: merge the parsed programs into the importing ones, in the
        // same single-threaded traversal order as the sequential resolver.
        std::unordered_set<String> resolvingPaths;          // files on the current merge stack (cycles)
        std::unordered_map<String, String> namespaceToFile; // namespace -> resolved file path

        std::function<void (YdspProgram&, const String&, std::unordered_set<String>&)> resolveImports;
        resolveImports = [&] (YdspProgram& prog, const String& parentPath, std::unordered_set<String>& seenCombos)
        {
            for (const auto& importDecl : prog.imports)
            {
                const auto resolvedPath = resolveImportPath (importDecl.path, parentPath);

                // Detect circular imports: the file is already being merged up
                // the current import stack.
                if (resolvingPaths.find (resolvedPath) != resolvingPaths.end())
                {
                    diagnostics.addError (importDecl.location.line, importDecl.location.column, "Circular import detected for '" + importDecl.path + "'");
                    continue;
                }

                // Namespace prefix: the alias if provided, otherwise the last
                // module path segment (`import X.Y.Z` forces access as `Z.*`).
                String nsPrefix = importDecl.alias;
                if (nsPrefix.isEmpty())
                {
                    const auto lastDot = importDecl.path.lastIndexOfChar ('.');
                    nsPrefix = lastDot >= 0 ? importDecl.path.substring (lastDot + 1) : importDecl.path;
                }

                // Two different files resolving to the same namespace make
                // `Ns.symbol` ambiguous; fail at the offending import.
                const auto existing = namespaceToFile.find (nsPrefix);
                if (existing != namespaceToFile.end() && existing->second != resolvedPath)
                {
                    diagnostics.addError (importDecl.location.line,
                                          importDecl.location.column,
                                          "Import '" + importDecl.path + "' would share the namespace '" + nsPrefix
                                              + "' with '" + existing->second + "'; use 'as' to disambiguate");
                    continue;
                }

                const String comboKey = resolvedPath + "\n" + nsPrefix;

                const auto parsedIt = parsedImports.find (resolvedPath);

                const bool importFailed = parsedIt == parsedImports.end()
                                       || parsedIt->second.program == nullptr
                                       || ! parsedIt->second.errors.empty();

                if (importFailed)
                {
                    // The file failed to read or parse in phase A; report it
                    // here so diagnostics keep the traversal order.
                    if (parsedIt != parsedImports.end())
                    {
                        if (! parsedIt->second.found)
                            diagnostics.addError (importDecl.location.line, importDecl.location.column, "Imported file not found: '" + resolvedPath + "'");
                        else
                            for (const auto& [line, column, message] : parsedIt->second.errors)
                                diagnostics.addError (line, column, message);
                    }
                    else
                    {
                        // Phase A parses every path phase B can reach, so this
                        // is a compiler bug rather than a user error; fail
                        // loudly instead of silently dropping the file.
                        diagnostics.addError (importDecl.location.line, importDecl.location.column, "Internal error: import '" + importDecl.path + "' was not parsed");
                    }

                    continue;
                }

                if (! seenCombos.insert (comboKey).second)
                    continue;

                auto workingProg = cloneProgram (*parsedIt->second.program);

                // Recursively merge the imported program's own imports first.
                resolvingPaths.insert (resolvedPath);
                std::unordered_set<String> nestedSeenCombos;
                resolveImports (*workingProg, resolvedPath, nestedSeenCombos);
                resolvingPaths.erase (resolvedPath);

                mergeImportedDecls (prog, *workingProg, nsPrefix);

                // The file is fully merged; record its namespace so a later
                // import resolving to the same namespace is flagged above.
                namespaceToFile[nsPrefix] = resolvedPath;
            }
        };

        std::unordered_set<String> topLevelSeenCombos;
        resolveImports (*program, basePath, topLevelSeenCombos);
    }

    // 3. Analyze
    YdspSemanticAnalyzer analyzer (diagnostics);
    auto analyzed = analyzer.analyze (std::move (program));
    if (analyzed == nullptr || diagnostics.hasErrors())
        return ResultValue<YdspAudioGraph>::fail (diagnostics.getItem (0).message);

    // 4. Build + optimise IR
    YdspOptimizer optimizer (diagnostics);

    const bool useNativeTier = options.optimizationTier != YdspOptimizationTier::baseline;

#if YUP_WASM && ! defined (__wasm_simd128__)
    // A scalar wasm build (no -msimd128) keeps every loop transform off.
    constexpr bool hasLoopTransforms = false;
#else
    constexpr bool hasLoopTransforms = true;
#endif

    // The wasm SIMD backend (compiled with -msimd128, the emscripten default)
    // applies the same automatic-tier transform set as native: vectorisation
    // at four f32x4 lanes, unrolling of the widened loops, and splitting of
    // the widened reduction chains the unrolled copies expose. All three are
    // pure IR passes that run before codegen, so the wasm backend consumes
    // exactly the IR the native suite already validates.
    const bool enableVectorization = useNativeTier && hasLoopTransforms && target.vectorWidth > 1;
    const bool enableUnrolling = useNativeTier && hasLoopTransforms;
    const bool enableReductionSplitting = useNativeTier && hasLoopTransforms;

    // Only fast-math compilation may change an expression's evaluation order.
    // An explicit YDSP fma() remains a one-rounding operation in every tier;
    // targetHasFusedMultiplyAdd selects its native or exact float64 lowering.
    optimizer.setContractionEnabled (options.fastMath);
    optimizer.setTargetHasFusedMultiplyAdd (target.supportsFusedMultiplyAdd);
    optimizer.setTargetHasPackedFusedMultiplyAdd (target.supportsFusedMultiplyAdd && target.vectorWidth > 1);

    optimizer.setVectorizationEnabled (enableVectorization);
    optimizer.setVectorWidth (target.vectorWidth);
    optimizer.setUnrollingEnabled (enableUnrolling);
    optimizer.setReductionSplittingEnabled (enableReductionSplitting);

    if (options.emitOptimizationReport)
    {
        optimizationReport.vectorizationEnabled = enableVectorization;
        optimizationReport.unrollingEnabled = enableUnrolling;
        optimizationReport.reductionSplittingEnabled = enableReductionSplitting;
        optimizationReport.contractionEnabled = options.fastMath;

#if YUP_WASM && ! defined (__wasm_simd128__)
        if (useNativeTier)
            optimizationReport.rejectedTransforms.add ("WASM SIMD lowering requires compiling with -msimd128");
#endif

        if (options.optimizationTier == YdspOptimizationTier::aggressive)
            optimizationReport.rejectedTransforms.add ("Advanced loop versioning, masked tails and e-graph rewrites require benchmark validation and are not enabled");
    }

    auto ir = optimizer.build (*analyzed);
    if (ir == nullptr || diagnostics.hasErrors())
        return ResultValue<YdspAudioGraph>::fail (diagnostics.getItem (0).message);

    // Missed-vectorization remarks: "why is this loop scalar?", as info
    // diagnostics, only when the optimization report was asked for - the same
    // opt-in that LLVM's -Rpass=loop-vectorize uses. The structured per-loop
    // outcome is always available through YdspKernelReport::loopVectorization.
    if (options.emitOptimizationReport && enableVectorization)
        for (const auto& kernel : ir->kernels)
            for (const auto& result : kernel->vectorizationResults)
                if (! result.widened())
                    diagnostics.addInfo (0, 0, "kernel '" + kernel->name + "': " + result.describe());

    // 5. Codegen into the graph's runtime
    auto graphPimpl = std::make_unique<YdspAudioGraph::Pimpl>();

    const auto& analyzedGraph = analyzed->graph;

    // ---- Compile one kernel per analyzed processor (node) ----
    struct KernelInfo
    {
        String processorName;
        YdspCompiledKernel kernel;
        bool isInit = false;
        int numInputs = 0;
        int numOutputs = 0;
        int numParams = 0;
        int numParamOuts = 0;
        size_t stateSize = 0;
        size_t stateScalarSize = 0;
        int activityByteOffset = -1;

        // Compiled event handlers for this processor (shared by its nodes):
        // one table per declared event input, each indexed by eventShapeIndex().
        struct EventHandlerTable
        {
            String inputName;
            std::array<YdspCompiledKernel, numProcessorEventShapes> handlers;
        };

        std::vector<EventHandlerTable> eventHandlerTables;
    };

    std::vector<KernelInfo> kernelInfos;

#if YUP_WASM
    // Compiles an IR function to a wasm module and registers it in the
    // current (main) JS realm; the graph keeps the bytes for re-registration
    // in other realms and owns the handle.
    auto compileWasmFn = [&graphPimpl, &optimizationReport, &options] (const YdspIrFunction& irFn, YdspDiagnostics& diag) -> YdspCompiledKernel
    {
        auto bytes = YdspWasmCodegen::compile (irFn, diag);

        if (bytes.empty() || diag.hasErrors())
            return {};

        if (options.emitOptimizationReport)
            optimizationReport.generatedCodeSize += bytes.size();

        String errorMessage;
        const auto handle = YdspWasmRuntime::registerKernel (bytes.data(), bytes.size(), errorMessage);

        if (handle < 0)
        {
            diag.addError (0, 0, errorMessage);
            return {};
        }

        graphPimpl->wasmModules.push_back (std::move (bytes));
        graphPimpl->wasmHandles.push_back (handle);

        // The wrapper carries the unique registry key and the position of the
        // bytes in wasmModules, so other realms (e.g. the audio worklet) can
        // lazily instantiate their own copy on first use.
        return YdspCompiledKernel (handle, &graphPimpl->wasmModules, graphPimpl->wasmModules.size() - 1);
    };
#endif

    for (const auto& kernel : ir->kernels)
    {
        KernelInfo info;
        info.processorName = kernel->name;
        info.isInit = kernel->isInit;
        info.numInputs = kernel->numInputs;
        info.numOutputs = kernel->numOutputs;
        info.numParams = kernel->numParams;
        info.numParamOuts = kernel->numParamsOut;
        info.stateSize = kernel->stateSize();
        info.stateScalarSize = kernel->stateScalarSize();
        info.activityByteOffset = kernel->activityByteOffset;

#if YUP_WASM
        info.kernel = compileWasmFn (*kernel, diagnostics);
#else
        size_t generatedCodeSize = 0;
        info.kernel = YdspCompiledKernel (YdspAsmJitCodegen::compile (graphPimpl->runtime, *kernel, diagnostics, &generatedCodeSize));

        if (options.emitOptimizationReport)
            optimizationReport.generatedCodeSize += generatedCodeSize;
#endif

        if (! info.kernel.isValid())
            return ResultValue<YdspAudioGraph>::fail (diagnostics.getItem (0).message);

        kernelInfos.push_back (std::move (info));
    }

    // ---- Compile one event-handler function per analyzed handler ----
    for (const auto& handler : ir->eventHandlers)
    {
#if YUP_WASM
        auto handlerFn = compileWasmFn (*handler, diagnostics);
#else
        size_t generatedCodeSize = 0;
        auto handlerFn = YdspCompiledKernel (YdspAsmJitCodegen::compileEventHandler (graphPimpl->runtime, *handler, diagnostics, &generatedCodeSize));

        if (options.emitOptimizationReport)
            optimizationReport.generatedCodeSize += generatedCodeSize;
#endif

        if (! handlerFn.isValid())
            return ResultValue<YdspAudioGraph>::fail (diagnostics.getItem (0).message);

        const auto shapeIndex = eventShapeIndex (handler->eventShape);

        if (shapeIndex < 0)
            continue; // `midi` is graph-scope only and has no handler

        for (auto& info : kernelInfos)
        {
            if (info.processorName == handler->ownerProcessorName)
            {
                auto table = std::find_if (info.eventHandlerTables.begin(),
                                           info.eventHandlerTables.end(),
                                           [&] (const KernelInfo::EventHandlerTable& candidate)
                {
                    return candidate.inputName == handler->eventInputName;
                });

                if (table == info.eventHandlerTables.end())
                {
                    info.eventHandlerTables.push_back ({ handler->eventInputName, {} });
                    table = info.eventHandlerTables.end() - 1;
                }

                table->handlers[static_cast<size_t> (shapeIndex)] = handlerFn;
                break;
            }
        }
    }

    // ---- Graph stream types (drives host buffer element sizes) ----
    for (const auto* endpoint : analyzedGraph.inputStreams)
        graphPimpl->inputStreamTypes.push_back (toStorageType (endpoint->type));

    for (const auto* endpoint : analyzedGraph.outputStreams)
        graphPimpl->outputStreamTypes.push_back (toStorageType (endpoint->type));

    // ---- Graph event inputs (declaration order; feeds the host API and the
    // per-input MPE instruments) ----
    for (const auto* endpoint : analyzedGraph.inputEvents)
        graphPimpl->eventInputNames.push_back (endpoint->name);

    graphPimpl->graphInputRouting.resize (graphPimpl->eventInputNames.size());

    graphPimpl->latencySamples = analyzedGraph.latencySamples;

    // ---- Build node table ----
    for (const auto& node : analyzedGraph.nodes)
    {
        YdspAudioGraph::Pimpl::Node runtimeNode;
        runtimeNode.instanceName = node.instanceName;
        runtimeNode.processorName = node.processor->name;

        const KernelInfo* kernelInfo = nullptr;

        for (const auto& candidate : kernelInfos)
        {
            if (candidate.processorName == node.processor->name && ! candidate.isInit)
            {
                kernelInfo = &candidate;
                break;
            }
        }

        if (kernelInfo == nullptr)
            return ResultValue<YdspAudioGraph>::fail ("Unknown processor '" + node.processor->name + "' referenced in graph");

        runtimeNode.kernel = kernelInfo->kernel;
        runtimeNode.numInputs = kernelInfo->numInputs;
        runtimeNode.numOutputs = kernelInfo->numOutputs;
        runtimeNode.numParams = kernelInfo->numParams;
        runtimeNode.numParamOuts = kernelInfo->numParamOuts;
        runtimeNode.stateSize = kernelInfo->stateSize;
        runtimeNode.stateScalarSize = kernelInfo->stateScalarSize;
        runtimeNode.activityByteOffset = kernelInfo->activityByteOffset;
        runtimeNode.rateMultiplier = node.rateMultiplier;
        runtimeNode.rateDivider = node.rateDivider;

        runtimeNode.voiceCount = node.voiceCount;
        runtimeNode.isEventDriven = node.isEventDriven;
        runtimeNode.isMidiOnly = node.isMidiOnly;

        // One event binding per input event endpoint this processor declares,
        // keyed by its local position (not by any graph port) - the handler
        // table for that input is the processor's, shared by every node
        // instantiating it. Which graph input or node feeds this slot, if
        // any, is a separate question resolved entirely through explicit
        // connections (see the event-edge routing table below).
        int localEventInputSlot = 0;

        for (const auto& eventInput : node.processor->endpoints)
        {
            if (eventInput.kind != YdspEndpointKind::inputEvent)
                continue;

            const auto table = std::find_if (kernelInfo->eventHandlerTables.begin(),
                                             kernelInfo->eventHandlerTables.end(),
                                             [&] (const KernelInfo::EventHandlerTable& candidate)
            {
                return candidate.inputName == eventInput.name;
            });

            if (table != kernelInfo->eventHandlerTables.end())
            {
                YdspAudioGraph::Pimpl::Node::EventInputBinding binding;
                binding.eventInputSlot = localEventInputSlot;
                binding.handlers = table->handlers;
                runtimeNode.eventInputs.push_back (std::move (binding));
            }

            ++localEventInputSlot;
        }

        runtimeNode.voiceMode = node.voiceMode;
        runtimeNode.stealing = node.stealing;
        runtimeNode.monoPriority = node.monoPriority;
        runtimeNode.voiceSlots.assign (static_cast<size_t> (runtimeNode.voiceCount), {});
        runtimeNode.voicePendingCalls.assign (static_cast<size_t> (runtimeNode.voiceCount), {});

        // Element sizes and types per stream, from the processor's endpoint
        // types. The sizes drive buffer arithmetic; the types drive the mix
        // path, which cannot tell int32 from float32 by width alone.
        for (const auto& endpoint : node.processor->endpoints)
        {
            const auto type = toStorageType (endpoint.type);

            if (endpoint.kind == YdspEndpointKind::inputStream)
            {
                runtimeNode.inputElemSizes.push_back (detail::elementSize (type));
                runtimeNode.inputElemTypes.push_back (type);
            }
            else if (endpoint.kind == YdspEndpointKind::outputStream)
            {
                runtimeNode.outputElemSizes.push_back (detail::elementSize (type));
                runtimeNode.outputElemTypes.push_back (type);
            }
        }

        runtimeNode.inputConnectionStart.assign (static_cast<size_t> (runtimeNode.numInputs) + 1, 0);
        runtimeNode.inputMixPtrs.assign (static_cast<size_t> (runtimeNode.numInputs), nullptr);
        runtimeNode.outputSlotBuffer.assign (static_cast<size_t> (runtimeNode.numOutputs), -2);
        runtimeNode.outputSlotScratch.assign (static_cast<size_t> (runtimeNode.numOutputs), -1);
        runtimeNode.outputSlotGraphOut.assign (static_cast<size_t> (runtimeNode.numOutputs), 0);

        graphPimpl->nodes.push_back (std::move (runtimeNode));
    }

    // ---- Runtime event-edge routing table ----
    // edge.dstEndpoint is already the destination's own local input-event
    // slot (its position among that processor's declared input events), so
    // it is used directly - no more indirection through a per-node graph-port
    // mapping.
    for (const auto& edge : analyzedGraph.eventEdges)
    {
        YdspAudioGraph::Pimpl::Node::RoutedEventEdge routed;
        routed.compensationSamples = edge.compensationSamples;

        if (edge.dstNode >= 0)
        {
            routed.dstNode = edge.dstNode;
            routed.dstEventInputIndex = edge.dstEndpoint;
        }

        if (edge.srcNode >= 0)
        {
            auto& srcNode = graphPimpl->nodes[static_cast<size_t> (edge.srcNode)];

            if (srcNode.outputRouting.size() <= static_cast<size_t> (edge.srcEndpoint))
                srcNode.outputRouting.resize (static_cast<size_t> (edge.srcEndpoint) + 1);

            srcNode.outputRouting[static_cast<size_t> (edge.srcEndpoint)].push_back (routed);
        }
        else
        {
            graphPimpl->graphInputRouting[static_cast<size_t> (edge.srcEndpoint)].push_back (routed);
        }
    }

    // ---- Attach one-shot init kernels to their nodes ----
    for (const auto& info : kernelInfos)
    {
        if (! info.isInit)
            continue;

        for (auto& node : graphPimpl->nodes)
        {
            if (node.processorName == info.processorName)
            {
                node.initKernel = info.kernel;
                break;
            }
        }
    }

    // ---- Resolve stream wiring from edges ----
    //
    // Any destination may be fed by several edges, so this is built by counting
    // then placing rather than by assignment: count per slot, prefix-sum into
    // the CSR start arrays, then walk the edges once more placing each at its
    // slot's running cursor. That last pass is a *counting sort*, deliberately
    // not std::sort, which is not stable - the summation order of a mix point is
    // observable (float addition does not associate), and it is defined as the
    // order the edges appear in analyzedGraph.edges.
    //
    // That order is deterministic per patch but is NOT "the order the patch
    // author wrote them": inlineSubgraphs appends spliced edges after the kept
    // ones, and fuseNodeChains rebuilds the list. Deterministic is the whole
    // guarantee.
    const auto numGraphOutputs = static_cast<int> (graphPimpl->outputStreamTypes.size());

    graphPimpl->graphOutputSourceStart.assign (static_cast<size_t> (numGraphOutputs) + 1, 0);
    graphPimpl->graphOutputDirect.assign (static_cast<size_t> (numGraphOutputs), false);

    for (const auto& edge : analyzedGraph.edges)
    {
        if (edge.dstNode >= 0)
            ++graphPimpl->nodes[static_cast<size_t> (edge.dstNode)].inputConnectionStart[static_cast<size_t> (edge.dstStream) + 1];
        else
            ++graphPimpl->graphOutputSourceStart[static_cast<size_t> (edge.dstStream) + 1];
    }

    for (auto& node : graphPimpl->nodes)
        for (size_t s = 1; s < node.inputConnectionStart.size(); ++s)
            node.inputConnectionStart[s] += node.inputConnectionStart[s - 1];

    for (size_t o = 1; o < graphPimpl->graphOutputSourceStart.size(); ++o)
        graphPimpl->graphOutputSourceStart[o] += graphPimpl->graphOutputSourceStart[o - 1];

    for (auto& node : graphPimpl->nodes)
        node.inputConnections.assign (static_cast<size_t> (node.inputConnectionStart.back()), {});

    graphPimpl->graphOutputSources.assign (static_cast<size_t> (graphPimpl->graphOutputSourceStart.back()), {});

    // Running write cursor per slot, seeded from the starts.
    std::vector<std::vector<int>> nodeCursors (graphPimpl->nodes.size());

    for (size_t n = 0; n < graphPimpl->nodes.size(); ++n)
        nodeCursors[n] = graphPimpl->nodes[n].inputConnectionStart;

    auto graphOutputCursors = graphPimpl->graphOutputSourceStart;

    for (const auto& edge : analyzedGraph.edges)
    {
        YdspAudioGraph::Pimpl::StreamConnection connection;
        connection.srcNode = edge.srcNode;
        connection.srcIndex = edge.srcStream;
        // The one place the two delays are summed: `delaySamples` is what the
        // author wrote and `compensationSamples` is what the latency pass added
        // to align this edge with its siblings. They stay separate in the
        // analyzed graph so the fusion predicate keeps its meaning and the pass
        // stays idempotent; the ring does not care which is which.
        connection.delaySamples = edge.delaySamples + edge.compensationSamples;

        if (edge.dstNode >= 0)
        {
            auto& dstNode = graphPimpl->nodes[static_cast<size_t> (edge.dstNode)];
            auto& cursor = nodeCursors[static_cast<size_t> (edge.dstNode)][static_cast<size_t> (edge.dstStream)];
            dstNode.inputConnections[static_cast<size_t> (cursor++)] = std::move (connection);
        }
        else
        {
            auto& cursor = graphOutputCursors[static_cast<size_t> (edge.dstStream)];
            graphPimpl->graphOutputSources[static_cast<size_t> (cursor++)] = std::move (connection);
        }
    }

    // ---- Direct-write fast path ----
    // Count each node output's destinations first: whether an output may be
    // written straight into a host buffer depends on how many places read it,
    // which is not knowable from any single edge.
    std::vector<std::vector<int>> outputDestCounts (graphPimpl->nodes.size());

    for (size_t n = 0; n < graphPimpl->nodes.size(); ++n)
        outputDestCounts[n].assign (static_cast<size_t> (graphPimpl->nodes[n].numOutputs), 0);

    for (const auto& edge : analyzedGraph.edges)
        if (edge.srcNode >= 0)
            ++outputDestCounts[static_cast<size_t> (edge.srcNode)][static_cast<size_t> (edge.srcStream)];

    for (int o = 0; o < numGraphOutputs; ++o)
    {
        const auto first = graphPimpl->graphOutputSourceStart[static_cast<size_t> (o)];
        const auto last = graphPimpl->graphOutputSourceStart[static_cast<size_t> (o) + 1];

        if (last - first != 1)
            continue;

        const auto& source = graphPimpl->graphOutputSources[static_cast<size_t> (first)];

        if (source.srcNode < 0 || source.delaySamples != 0)
            continue;

        if (outputDestCounts[static_cast<size_t> (source.srcNode)][static_cast<size_t> (source.srcIndex)] != 1)
            continue;

        graphPimpl->graphOutputDirect[static_cast<size_t> (o)] = true;

        auto& srcNode = graphPimpl->nodes[static_cast<size_t> (source.srcNode)];
        srcNode.outputSlotBuffer[static_cast<size_t> (source.srcIndex)] = -1;
        srcNode.outputSlotGraphOut[static_cast<size_t> (source.srcIndex)] = o;
    }

    // ---- Parameters ----
    // Layout: graph input values first, then one contiguous byte block per
    // node. Slots can have heterogeneous element sizes (f32/f64/i32/i64).
    int paramByteCursor = 0;
    int paramSlotCount = 0;

    for (size_t i = 0; i < analyzedGraph.inputValues.size(); ++i)
    {
        const auto* endpoint = analyzedGraph.inputValues[i];
        const auto type = toStorageType (endpoint->type);

        YdspParameterInfo info;
        info.name = endpoint->name;
        info.displayName = detail::annotationString (*endpoint, "name", endpoint->name);
        info.type = detail::toElementType (type);
        info.defaultValue = detail::constDefaultAsDouble (analyzedGraph.inputValueDefaults[i]);
        info.minValue = detail::annotationValue (*endpoint, "min", 0.0);
        info.maxValue = detail::annotationValue (*endpoint, "max", 1.0);
        info.discreteValues = detail::annotationValues (*endpoint, "values");
        info.unit = detail::annotationString (*endpoint, "unit", {});
        info.stepSize = detail::annotationValue (*endpoint, "step", 0.0);
        info.style = detail::annotationString (*endpoint, "style", {});
        graphPimpl->paramInfos.push_back (std::move (info));
        graphPimpl->paramSlotByName[endpoint->name] = paramSlotCount;
        graphPimpl->paramSlotTypes.push_back (type);
        graphPimpl->paramOffsets.push_back (paramByteCursor);
        paramByteCursor += detail::elementSize (type);
        ++paramSlotCount;
    }

    graphPimpl->paramSlotToNode.assign (static_cast<size_t> (paramSlotCount), std::vector<std::pair<int, int>> {});

    for (size_t n = 0; n < graphPimpl->nodes.size(); ++n)
    {
        auto& node = graphPimpl->nodes[n];
        node.paramOffset = paramByteCursor;
        node.paramByteOffsets.clear();
        node.paramGlobalSlots.clear();

        const auto* processor = analyzedGraph.nodes[n].processor;

        // A fused node stands for several original nodes, so it carries the
        // host-visible name of each parameter rather than letting it be derived
        // from its own (synthesized) instance name.
        const auto& publicNames = analyzedGraph.nodes[n].paramPublicNames;

        const auto publicName = [&] (int index, const YdspEndpointDecl& endpoint)
        {
            if (static_cast<size_t> (index) < publicNames.size())
                return publicNames[static_cast<size_t> (index)];

            return node.instanceName + "." + endpoint.name;
        };

        int paramIndex = 0;

        for (const auto& endpoint : processor->endpoints)
        {
            if (endpoint.kind != YdspEndpointKind::inputValue)
                continue;

            const auto type = toStorageType (endpoint.type);
            node.paramByteOffsets.push_back (paramByteCursor - node.paramOffset);
            node.paramGlobalSlots.push_back (paramSlotCount);
            graphPimpl->paramSlotTypes.push_back (type);
            graphPimpl->paramOffsets.push_back (paramByteCursor);
            paramByteCursor += detail::elementSize (type);

            YdspParameterInfo info;
            info.name = publicName (paramIndex, endpoint);
            info.displayName = detail::annotationString (endpoint, "name", endpoint.name);
            info.type = detail::toElementType (type);
            info.defaultValue = detail::constDefaultAsDouble (analyzedGraph.nodes[n].paramDefaults[static_cast<size_t> (paramIndex)]);
            info.minValue = detail::annotationValue (endpoint, "min", 0.0);
            info.maxValue = detail::annotationValue (endpoint, "max", 1.0);
            info.discreteValues = detail::annotationValues (endpoint, "values");
            info.unit = detail::annotationString (endpoint, "unit", {});
            info.stepSize = detail::annotationValue (endpoint, "step", 0.0);
            info.style = detail::annotationString (endpoint, "style", {});

            // The node's own private slot maps straight to its local param
            // (the audio-thread automation path writes into the node block).
            graphPimpl->paramSlotToNode.push_back ({ { static_cast<int> (n), paramIndex } });

            // Is this parameter connected from a graph parameter?
            bool connected = false;

            for (const auto& valueEdge : analyzedGraph.valueEdges)
            {
                if (valueEdge.dstNode == static_cast<int> (n) && valueEdge.dstParam == paramIndex)
                {
                    // The graph parameter's slot is authoritative; the driver
                    // copies it into the node's block before each process call.
                    graphPimpl->paramSlotByName[publicName (paramIndex, endpoint)] = valueEdge.srcParam;
                    node.paramCopies.emplace_back (valueEdge.srcParam, paramIndex);
                    connected = true;

                    // Route the graph param's slot to this node as well, so
                    // automation under the graph-param name lands here too. The
                    // graph param may drive several nodes, so this appends.
                    if (valueEdge.srcParam >= 0 && static_cast<size_t> (valueEdge.srcParam) < graphPimpl->paramSlotToNode.size())
                        graphPimpl->paramSlotToNode[static_cast<size_t> (valueEdge.srcParam)].push_back ({ static_cast<int> (n), paramIndex });

                    break;
                }
            }

            // A connected node parameter is *aliased* by its graph parameter:
            // it shares the graph slot, so only the graph parameter appears in
            // the host-UI enumeration (no separate phantom entry).
            if (! connected)
                graphPimpl->paramInfos.push_back (std::move (info));

            if (! connected)
                graphPimpl->paramSlotByName[publicName (paramIndex, endpoint)] = paramSlotCount;

            ++paramIndex;
            ++paramSlotCount;
        }
    }

    graphPimpl->params.assign (static_cast<size_t> (paramByteCursor), 0);

    // Initialize parameter values from node defaults (graph params use the
    // graph-level declared defaults).
    for (size_t i = 0; i < analyzedGraph.inputValues.size(); ++i)
        detail::writeConstValue (graphPimpl->params.data() + static_cast<size_t> (graphPimpl->paramOffsets[static_cast<size_t> (i)]),
                                 analyzedGraph.inputValueDefaults[static_cast<size_t> (i)]);

    for (size_t n = 0; n < graphPimpl->nodes.size(); ++n)
    {
        auto& node = graphPimpl->nodes[n];
        const auto& analyzedNode = analyzedGraph.nodes[n];

        for (size_t p = 0; p < analyzedNode.paramDefaults.size(); ++p)
        {
            const auto slot = node.paramGlobalSlots[static_cast<size_t> (p)];
            detail::writeConstValue (graphPimpl->params.data() + static_cast<size_t> (graphPimpl->paramOffsets[static_cast<size_t> (slot)]),
                                     analyzedNode.paramDefaults[p]);
        }
    }

    // ---- Meters ----
    int meterByteCursor = 0;
    int meterSlotCount = 0;

    for (size_t n = 0; n < graphPimpl->nodes.size(); ++n)
    {
        auto& node = graphPimpl->nodes[n];
        node.paramOutOffset = meterByteCursor;

        const auto* processor = analyzedGraph.nodes[n].processor;

        // As with the parameters above: a fused node carries the host-visible
        // name of each meter, because it stands for several original nodes.
        const auto& publicNames = analyzedGraph.nodes[n].meterPublicNames;

        const auto publicName = [&] (int index, const YdspEndpointDecl& endpoint)
        {
            if (static_cast<size_t> (index) < publicNames.size())
                return publicNames[static_cast<size_t> (index)];

            return node.instanceName + "." + endpoint.name;
        };

        int meterIndex = 0;
        int nodeMeterCount = 0;

        for (const auto& endpoint : processor->endpoints)
        {
            if (endpoint.kind != YdspEndpointKind::outputValue)
                continue;

            const auto type = toStorageType (endpoint.type);
            graphPimpl->meterSlotTypes.push_back (type);
            meterByteCursor += detail::elementSize (type);

            graphPimpl->meterSlotByName[publicName (meterIndex, endpoint)] = meterSlotCount;

            // One name per slot, not one per map insertion: the loop below adds a
            // second spelling for the same slot when the meter is routed to a
            // graph-scope `output value`, and that graph-level name replaces the
            // qualified one rather than adding an entry.
            graphPimpl->meterSlotNames.push_back (publicName (meterIndex, endpoint));

            for (const auto& meterEdge : analyzedGraph.meterEdges)
            {
                if (meterEdge.srcNode == static_cast<int> (n) && meterEdge.srcMeter == meterIndex)
                {
                    const auto& graphMeterName = analyzedGraph.outputValues[static_cast<size_t> (meterEdge.dstMeter)]->name;

                    graphPimpl->meterSlotByName[graphMeterName] = meterSlotCount;
                    graphPimpl->meterSlotNames.back() = graphMeterName;
                    break;
                }
            }

            ++meterSlotCount;
            ++meterIndex;
            ++nodeMeterCount;
        }

        node.numParamOuts = nodeMeterCount;
    }

    graphPimpl->paramOut.assign (static_cast<size_t> (meterByteCursor), 0);

    // ---- Report ----
    YdspOptimizer::buildReport (*ir, graphPimpl->report);

    graphPimpl->topoOrder = analyzedGraph.topoOrder;
    graphPimpl->valid = true;
    graphPimpl->diagnostics = std::move (diagnostics);

    YdspAudioGraph graph;
    graph.pimpl = std::move (graphPimpl);

    return ResultValue<YdspAudioGraph>::ok (std::move (graph));
}

} // namespace yup
