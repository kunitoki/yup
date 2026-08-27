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
/** Semantic analysis of a YDSP program.

    Resolves names, type-checks every expression and statement, enforces the
    realtime-safety rules (statically-bounded loops, no recursion, correct
    sample/block mode usage, parameter read/write rules), validates the graph
    (arity analysis for the Faust algebra, connection resolution, topological
    order) and produces a YdspAnalyzedProgram consumed by the optimiser and
    the asmjit backend.
*/
class YdspSemanticAnalyzer
{
public:
    /** Constructs an analyzer that reports into the given diagnostics.
    
        @param diagnostics The diagnostics object to report errors and warnings into.
    */
    explicit YdspSemanticAnalyzer (DspJitDiagnostics& diagnostics);

    /** Analyzes a parsed program; takes ownership of the AST to keep
        analysis pointers valid. Returns nullptr on any error.
    
        The returned program is a new AST with the same structure as the input,
        but with every expression and statement annotated with its type and
        other semantic information. The returned program is owned by the caller.

        @param program The parsed program to analyze. Ownership is transferred to the analyzer.
        @return A unique pointer to the analyzed program, or nullptr if analysis failed.
    */
    std::unique_ptr<YdspAnalyzedProgram> analyze (std::unique_ptr<YdspProgram> program);

private:
    //==============================================================================
    void preprocessProgram (YdspProgram& program);
    void resolveProgramConstants (YdspProgram& program);
    void rejectConstantShadowing (const YdspProgram& program);
    void substituteConstants (YdspExpr& expr) const;
    void substituteConstants (YdspStmt& stmt) const;
    void substituteConstants (const std::vector<YdspStmtPtr>& body) const;
    void resolveStateArraySizes (YdspProcessorDecl& processor);
    void lowerStateInitialisers (YdspProcessorDecl& processor);
    void applyInitAnnotationDefaults (std::vector<YdspEndpointDecl>& endpoints);
    void applySmoothingAnnotations (YdspProcessorDecl& processor);

    [[nodiscard]] static String dottedName (const YdspExpr& expr);

    //==============================================================================
    std::unique_ptr<YdspAnalyzedProcessor> analyzeProcessor (const YdspProcessorDecl& decl);

    void analyzeStatement (const YdspStmt& stmt, YdspAnalyzedProcessor& proc);
    YdspValueType analyzeExpr (const YdspExpr& expr, YdspAnalyzedProcessor& proc);
    std::optional<YdspValueType> analyzeLvalue (const YdspExpr& expr, YdspAnalyzedProcessor& proc);
    void analyzeEventHandler (const YdspEventHandlerDecl& decl, YdspAnalyzedProcessor& proc);

    void resolveActivityState (const YdspProcessorDecl& decl, YdspAnalyzedProcessor& proc);

    //==============================================================================
    std::unique_ptr<YdspAnalyzedGraph> analyzeGraph (const YdspGraphDecl& decl, const YdspAnalyzedProgram& program);

    void analyzeNodeAnnotations (const YdspNodeDecl& decl, YdspAnalyzedNode& node);
    void rebuildTopoOrder (YdspAnalyzedGraph& graph, const YdspLocation& location);

    [[nodiscard]] static int findGraphIndex (const YdspProgram& program, const String& name);
    [[nodiscard]] int selectMainGraph (const YdspProgram& program);
    [[nodiscard]] bool orderGraphsByDependency (const YdspProgram& program, int mainIndex, std::vector<int>& order);
    void inlineSubgraphs (YdspAnalyzedGraph& graph, const YdspGraphDecl& decl, const std::vector<YdspAnalyzedGraph>& analyzedGraphs);
    void fuseNodeChains (YdspAnalyzedProgram& program);
    void computeLatencyAndCompensate (YdspAnalyzedGraph& graph, const YdspLocation& location);

    void analyzeConnectionsForm (const YdspGraphDecl& decl,
                                 YdspAnalyzedGraph& graph,
                                 const std::unordered_map<String, int>& nodeIndexByName);

    void analyzeAlgebraForm (const YdspGraphDecl& decl,
                             YdspAnalyzedGraph& graph,
                             const std::unordered_map<String, int>& nodeIndexByName,
                             const YdspAnalyzedProgram& program);

    void validateConnectivity (const YdspGraphDecl& decl, YdspAnalyzedGraph& graph);

    //==============================================================================
    [[nodiscard]] bool resolveTypeName (const YdspToken& token, YdspPrimitiveType& out) const;
    [[nodiscard]] bool canCoerce (YdspValueType from, YdspValueType to) const;
    [[nodiscard]] bool isImplicitlyConvertibleTo (YdspValueType from, YdspValueType to) const;
    [[nodiscard]] bool isAdaptableLiteral (const YdspExpr& expr) const;
    [[nodiscard]] bool isAdaptableTo (const YdspExpr& expr, YdspValueType target) const;
    [[nodiscard]] std::optional<YdspValueType> unifyTypes (const YdspExpr& a, YdspValueType aType, const YdspExpr& b, YdspValueType bType) const;
    [[nodiscard]] YdspConstValue constEvalDefault (const YdspExpr* expr, YdspPrimitiveType type) const;

    bool addSymbol (const String& name, YdspSymbolInfo info, const YdspLocation& location);
    bool findSymbol (const String& name, YdspSymbolInfo& out) const;

    void pushLocalScope();
    void popLocalScope();

    [[nodiscard]] const YdspStructDecl* findStruct (const String& name) const;

    [[nodiscard]] bool resolveStructField (const YdspExpr& base, const String& fieldName, const YdspStateDecl*& outState, const YdspStructField*& outField);
    [[nodiscard]] bool resolveLoopBound (const YdspExpr& expr, YdspLoopBound& out) const;
    [[nodiscard]] double constantValue (const YdspExpr& expr) const;
    [[nodiscard]] bool tryConstantFold (const YdspExpr& expr, double& out) const;

    void analyzeFunctionBodies (YdspAnalyzedProcessor& proc);
    void analyzeProgramFunctions (YdspProgram& program, std::vector<YdspAnalyzedFunc>& out);
    [[nodiscard]] bool resolveFunctionCall (const String& name, const std::vector<YdspExprPtr>& args, const YdspLocation& location, YdspValueType& returnType);

    void error (const YdspLocation& location, StringRef message);

    //==============================================================================

    DspJitDiagnostics& diagnostics;

    std::unordered_map<String, YdspConstValue> programConstants;
    std::unordered_map<String, YdspSymbolInfo> symbols;
    std::vector<std::vector<String>> localScopes;

    const std::vector<YdspAnalyzedFunc>* currentProcessorFunctions = nullptr;
    const std::vector<YdspAnalyzedFunc>* currentProgramFunctions = nullptr;

    std::unordered_map<String, const YdspStructDecl*> structDecls;
    std::vector<const YdspStateDecl*> procStates;
    int loopDepth = 0;
    int hiddenStateCount = 0;
    bool isBlockMode = false;
    bool isInitMode = false;
    bool isEventHandlerMode = false;

    const YdspEventShapeDesc* currentEventShape = nullptr;

    int recursionDepth = 0;
};

} // namespace yup
