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

namespace wgsl
{

namespace
{

//==============================================================================
// Utility helpers
//==============================================================================

static bool isSamplerType (TypeKind kind)
{
    switch (kind)
    {
        case TypeKind::sampler1D:
        case TypeKind::sampler2D:
        case TypeKind::sampler3D:
        case TypeKind::samplerCube:
        case TypeKind::sampler1DShadow:
        case TypeKind::sampler2DShadow:
        case TypeKind::samplerCubeShadow:
        case TypeKind::sampler1DArray:
        case TypeKind::sampler2DArray:
        case TypeKind::sampler1DArrayShadow:
        case TypeKind::sampler2DArrayShadow:
        case TypeKind::sampler2DRect:
        case TypeKind::sampler2DRectShadow:
        case TypeKind::samplerBuffer:
        case TypeKind::sampler2DMS:
        case TypeKind::sampler2DMSArray:
        case TypeKind::isampler1D:
        case TypeKind::isampler2D:
        case TypeKind::isampler3D:
        case TypeKind::isamplerCube:
        case TypeKind::isampler1DArray:
        case TypeKind::isampler2DArray:
        case TypeKind::isampler2DRect:
        case TypeKind::isamplerBuffer:
        case TypeKind::isampler2DMS:
        case TypeKind::isampler2DMSArray:
        case TypeKind::usampler1D:
        case TypeKind::usampler2D:
        case TypeKind::usampler3D:
        case TypeKind::usamplerCube:
        case TypeKind::usampler1DArray:
        case TypeKind::usampler2DArray:
        case TypeKind::usampler2DRect:
        case TypeKind::usamplerBuffer:
        case TypeKind::usampler2DMS:
        case TypeKind::usampler2DMSArray:
            return true;
        default:
            return false;
    }
}

static bool isDoubleType (TypeKind kind)
{
    switch (kind)
    {
        case TypeKind::doubleType:
        case TypeKind::dvec2:
        case TypeKind::dvec3:
        case TypeKind::dvec4:
        case TypeKind::dmat2:
        case TypeKind::dmat3:
        case TypeKind::dmat4:
        case TypeKind::dmat2x2:
        case TypeKind::dmat2x3:
        case TypeKind::dmat2x4:
        case TypeKind::dmat3x2:
        case TypeKind::dmat3x3:
        case TypeKind::dmat3x4:
        case TypeKind::dmat4x2:
        case TypeKind::dmat4x3:
        case TypeKind::dmat4x4:
            return true;
        default:
            return false;
    }
}

static bool isImageType (TypeKind kind)
{
    switch (kind)
    {
        case TypeKind::image1D:
        case TypeKind::image2D:
        case TypeKind::image3D:
        case TypeKind::imageCube:
        case TypeKind::image1DArray:
        case TypeKind::image2DArray:
        case TypeKind::image2DRect:
        case TypeKind::imageBuffer:
        case TypeKind::image2DMS:
        case TypeKind::image2DMSArray:
        case TypeKind::iimage1D:
        case TypeKind::iimage2D:
        case TypeKind::iimage3D:
        case TypeKind::iimageCube:
        case TypeKind::iimage1DArray:
        case TypeKind::iimage2DArray:
        case TypeKind::iimage2DRect:
        case TypeKind::iimageBuffer:
        case TypeKind::iimage2DMS:
        case TypeKind::iimage2DMSArray:
        case TypeKind::uimage1D:
        case TypeKind::uimage2D:
        case TypeKind::uimage3D:
        case TypeKind::uimageCube:
        case TypeKind::uimage1DArray:
        case TypeKind::uimage2DArray:
        case TypeKind::uimage2DRect:
        case TypeKind::uimageBuffer:
        case TypeKind::uimage2DMS:
        case TypeKind::uimage2DMSArray:
            return true;
        default:
            return false;
    }
}

static bool isBuiltinGLName (const std::string& name)
{
    return name == "gl_Position" || name == "gl_FragCoord" || name == "gl_FragDepth"
        || name == "gl_FrontFacing" || name == "gl_PointSize"
        || name == "gl_VertexIndex" || name == "gl_VertexID"
        || name == "gl_InstanceIndex" || name == "gl_InstanceID"
        || name == "gl_GlobalInvocationID" || name == "gl_LocalInvocationID"
        || name == "gl_LocalInvocationIndex" || name == "gl_WorkGroupID"
        || name == "gl_NumWorkGroups";
}

static bool entryPointNameIsBuiltin (const std::string& name)
{
    return name == "main" || name == "vs_main" || name == "fs_main" || name == "cs_main";
}

//==============================================================================
// Symbol table (Task 2.1)
//==============================================================================

class SymbolTable
{
public:
    struct Scope
    {
        std::map<std::string, SymbolInfo> symbols;
    };

    void pushScope() { scopes.push_back (Scope()); }

    void popScope()
    {
        if (! scopes.empty())
            scopes.pop_back();
    }

    void declare (const std::string& name, const SymbolInfo& info)
    {
        if (! scopes.empty())
            scopes.back().symbols[name] = info;
    }

    SymbolInfo* lookup (const std::string& name)
    {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
        {
            auto found = it->symbols.find (name);
            if (found != it->symbols.end())
                return &found->second;
        }
        return nullptr;
    }

    void markReassigned (const std::string& name)
    {
        auto* info = lookup (name);
        if (info)
            info->isReassigned = true;
    }

private:
    std::vector<Scope> scopes;
};

//==============================================================================
// Diagnostics pass (Task 2.7)
//==============================================================================

struct DiagnosticContext
{
    ShaderStage stage;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    void addError (const SourceLocation& loc, const std::string& msg)
    {
        errors.push_back (std::to_string (loc.line) + ":" + std::to_string (loc.column) + ": " + msg);
    }

    void addWarning (const SourceLocation& loc, const std::string& msg)
    {
        warnings.push_back (std::to_string (loc.line) + ":" + std::to_string (loc.column) + ": " + msg);
    }
};

static void runDiagnostics (const TranslationUnit& ast, DiagnosticContext& diag)
{
    // Check for unsupported stages
    if (diag.stage == ShaderStage::geometry || diag.stage == ShaderStage::tessControl || diag.stage == ShaderStage::tessEval)
    {
        diag.addError ({ 0, 0 }, "Geometry and tessellation stages are not supported for WGSL output");
    }

    for (auto& decl : ast.declarations)
    {
        if (std::holds_alternative<Declaration> (decl))
        {
            auto& d = std::get<Declaration> (decl);
            if (d.initDeclaratorList)
            {
                auto& il = *d.initDeclaratorList;

                if (isDoubleType (il.type.kind))
                    diag.addError (il.loc, "Double precision floating point types are not supported in WGSL");

                if (isImageType (il.type.kind))
                    diag.addError (il.loc, "Image types are not yet supported in WGSL (v1 limitation)");

                // Check for push constants
                if (il.qualifier && il.qualifier->hasStorage (StorageQualifier::buffer))
                {
                    // Storage buffers (SSBOs) - error in v1
                    diag.addError (il.loc, "Storage buffers (SSBOs) are not yet supported in WGSL (v1 limitation)");
                }

                // Atomic counters
                if (il.type.kind == TypeKind::atomicUint)
                    diag.addError (il.loc, "Atomic counters are not supported in WGSL");
            }
        }
    }
}

//==============================================================================
// Binding assignment helper (Task 2.4 - shared allocator)
//==============================================================================

class BindingAllocator
{
public:
    BindingAllocator (uint32_t defaultGroup = 0)
        : defaultGroup (defaultGroup)
    {
    }

    void registerBinding (const std::string& name, uint32_t explicitSet, uint32_t explicitBinding)
    {
        uint32_t set = explicitSet != ~0u ? explicitSet : defaultGroup;
        uint32_t binding = explicitBinding != ~0u ? explicitBinding : autoBindings[set]++;

        assigned[name] = { .name = name, .group = set, .binding = binding, .samplerBinding = ~0u, .isSampler = false };
    }

    void registerSampledImage (const std::string& name,
                               uint32_t explicitSet,
                               uint32_t explicitBinding,
                               uint32_t& outSamplerBinding)
    {
        uint32_t set = explicitSet != ~0u ? explicitSet : defaultGroup;

        if (explicitBinding == ~0u)
        {
            // Auto-assign texture binding
            uint32_t texBinding = autoBindings[set]++;
            assigned[name] = { .name = name, .group = set, .binding = texBinding, .samplerBinding = ~0u, .isSampler = false };

            // Allocate sampler binding from the same group
            uint32_t sampBinding = samplerAutoBindings[set]++;
            assigned[name].samplerBinding = sampBinding;
            outSamplerBinding = sampBinding;
        }
        else
        {
            assigned[name] = { .name = name, .group = set, .binding = explicitBinding, .samplerBinding = ~0u, .isSampler = false };

            // Auto-assign sampler binding after the explicit texture binding
            uint32_t sampBinding = samplerAutoBindings[set]++;
            assigned[name].samplerBinding = sampBinding;
            outSamplerBinding = sampBinding;
        }
    }

    const LoweredProgram::ResourceAssignment* getAssignment (const std::string& name) const
    {
        auto it = assigned.find (name);
        if (it != assigned.end())
            return &it->second;
        return nullptr;
    }

    std::vector<LoweredProgram::ResourceAssignment> getAll() const
    {
        std::vector<LoweredProgram::ResourceAssignment> result;
        for (auto& [name, ra] : assigned)
            result.push_back (ra);
        return result;
    }

private:
    uint32_t defaultGroup;
    std::map<uint32_t, uint32_t> autoBindings;        // group → next auto binding
    std::map<uint32_t, uint32_t> samplerAutoBindings; // group → next auto sampler binding
    std::map<std::string, LoweredProgram::ResourceAssignment> assigned;
};

//==============================================================================
// Main lowering implementation
//==============================================================================

class LoweringImpl
{
public:
    LoweringImpl (const WgslLoweringOptions& opts)
        : options (opts)
        , allocator (opts.defaultGroup)
    {
        diag.stage = opts.stage;
    }

    ResultValue<LoweredProgram> run (TranslationUnit ast)
    {
        // Step 1: Diagnostics (reads AST, no mutations)
        runDiagnostics (ast, diag);

        if (! diag.errors.empty())
        {
            String err;
            for (auto& e : diag.errors)
                err << e << "\n";
            return makeResultValueFail (err);
        }

        LoweredProgram result;
        result.ast = std::move (ast);

        // Step 2: Build symbol table
        symbolTable.pushScope();
        buildGlobalSymbolTable (result.ast);

        // Step 3: Process resources (bindings, combined samplers)
        processResources (result.ast);

        // Step 4: Process functions (mutability, inout, entry points)
        processFunctions (result.ast);

        // Step 5: Legalize expressions/statements
        legalizeTranslationUnit (result.ast);

        symbolTable.popScope();
        result.resources = allocator.getAll();

        // Determine entry point info
        result.entryPoint.originalEntryPoint = "main";
        result.entryPoint.wgslEntryPoint = "main";
        result.entryPoint.innerFunction = "main_inner";

        switch (options.stage)
        {
            case ShaderStage::vertex:
                result.entryPoint.isVertex = true;
                break;
            case ShaderStage::fragment:
                result.entryPoint.isFragment = true;
                break;
            case ShaderStage::compute:
                result.entryPoint.isCompute = true;
                result.entryPoint.workgroupSizeX = options.defaultWorkgroupSize[0];
                result.entryPoint.workgroupSizeY = options.defaultWorkgroupSize[1];
                result.entryPoint.workgroupSizeZ = options.defaultWorkgroupSize[2];
                break;
            default:
                break;
        }

        // Extract workgroup sizes from explicit layout declaration
        extractWorkgroupSizesFromLayout (result);

        // Collect stage IO info
        collectStageIO (result);

        // Collect compute builtins from function body usage
        collectComputeBuiltinsFromUsage (result);

        return makeResultValueOk (std::move (result));
    }

private:
    //==========================================================================
    // Step 2: Build symbol table from global declarations
    //==========================================================================

    void buildGlobalSymbolTable (const TranslationUnit& ast)
    {
        for (auto& decl : ast.declarations)
        {
            if (std::holds_alternative<Declaration> (decl))
            {
                auto& d = std::get<Declaration> (decl);
                if (d.initDeclaratorList)
                {
                    auto& il = *d.initDeclaratorList;

                    for (auto& sd : il.declarations)
                    {
                        SymbolInfo info;
                        info.type = il.type;
                        info.type.arraySpecifiers.insert (
                            info.type.arraySpecifiers.end(),
                            sd.arraySpecifiers.begin(),
                            sd.arraySpecifiers.end());
                        info.isGlobal = true;

                        if (il.qualifier)
                        {
                            info.isConst = il.qualifier->hasStorage (StorageQualifier::constQual);
                        }

                        info.isBuiltin = isBuiltinGLName (sd.name);
                        symbolTable.declare (sd.name, info);
                    }
                }
                else if (d.structSpecifier)
                {
                    // Register struct name as a type
                    if (! d.structSpecifier->name.empty())
                    {
                        SymbolInfo info;
                        info.type = TypeSpecifier::makeNamed (d.loc, d.structSpecifier->name);
                        info.isGlobal = true;
                        symbolTable.declare (d.structSpecifier->name, info);
                    }

                    // For unnamed interface blocks (no initDeclaratorList), register
                    // each field as a global variable so they are accessible directly.
                    if (d.qualifier
                        && (d.qualifier->hasStorage (StorageQualifier::uniform)
                            || d.qualifier->hasStorage (StorageQualifier::buffer)))
                    {
                        for (auto& field : d.structSpecifier->fields)
                        {
                            SymbolInfo info;
                            info.type = field.type;
                            info.isGlobal = true;
                            symbolTable.declare (field.name, info);
                        }
                    }
                }
            }
            else if (std::holds_alternative<FunctionDefinition> (decl))
            {
                auto& fd = std::get<FunctionDefinition> (decl);

                // Register function parameters
                for (auto& param : fd.prototype.parameters)
                {
                    SymbolInfo info;
                    info.type = param.type;
                    info.isParameter = true;
                    info.isGlobal = true;

                    if (param.qualifier)
                    {
                        info.isOutParam = param.qualifier->hasStorage (StorageQualifier::out);
                        info.isInoutParam = param.qualifier->hasStorage (StorageQualifier::inout);
                    }

                    symbolTable.declare (param.name, info);
                }
            }
        }
    }

    //==========================================================================
    // Step 3: Process resources (Task 2.4)
    //==========================================================================

    void processResources (TranslationUnit& ast)
    {
        for (auto& decl : ast.declarations)
        {
            if (! std::holds_alternative<Declaration> (decl))
                continue;

            auto& d = std::get<Declaration> (decl);

            // Handle unnamed interface blocks (structSpecifier + qualifier but no initDeclaratorList)
            if (! d.initDeclaratorList && d.structSpecifier && d.qualifier
                && (d.qualifier->hasStorage (StorageQualifier::uniform)
                    || d.qualifier->hasStorage (StorageQualifier::buffer)))
            {
                uint32_t set = ~0u;
                uint32_t binding = ~0u;

                if (d.qualifier->layout)
                {
                    for (auto& entry : d.qualifier->layout->entries)
                    {
                        if (entry.id == LayoutQualifierId::descriptorSet && entry.value && entry.value->is<ExprIntConst>())
                            set = static_cast<uint32_t> (entry.value->as<ExprIntConst>().value);
                        else if (entry.id == LayoutQualifierId::binding && entry.value && entry.value->is<ExprIntConst>())
                            binding = static_cast<uint32_t> (entry.value->as<ExprIntConst>().value);
                    }
                }

                for (auto& field : d.structSpecifier->fields)
                    allocator.registerBinding (field.name, set, binding);

                continue;
            }

            if (! d.initDeclaratorList)
                continue;

            auto& il = *d.initDeclaratorList;

            // Extract layout binding info
            uint32_t set = ~0u;
            uint32_t binding = ~0u;

            if (il.qualifier && il.qualifier->layout)
            {
                for (auto& entry : il.qualifier->layout->entries)
                {
                    if (entry.id == LayoutQualifierId::descriptorSet && entry.value && entry.value->is<ExprIntConst>())
                        set = static_cast<uint32_t> (entry.value->as<ExprIntConst>().value);
                    else if (entry.id == LayoutQualifierId::binding && entry.value && entry.value->is<ExprIntConst>())
                        binding = static_cast<uint32_t> (entry.value->as<ExprIntConst>().value);
                }
            }

            for (auto& sd : il.declarations)
            {
                if (isSamplerType (il.type.kind))
                {
                    // Combined sampler → need to split
                    uint32_t samplerBinding;
                    allocator.registerSampledImage (sd.name, set, binding, samplerBinding);
                }
                else if (il.qualifier)
                {
                    bool isUniform = il.qualifier->hasStorage (StorageQualifier::uniform);
                    bool isBuffer = il.qualifier->hasStorage (StorageQualifier::buffer);

                    if (isUniform || isBuffer)
                        allocator.registerBinding (sd.name, set, binding);
                }
            }
        }
    }

    //==========================================================================
    // Step 4: Process functions (Tasks 2.2, 2.3)
    //==========================================================================

    void processFunctions (TranslationUnit& ast)
    {
        for (auto& decl : ast.declarations)
        {
            if (std::holds_alternative<FunctionDefinition> (decl))
            {
                auto& fd = std::get<FunctionDefinition> (decl);
                processFunction (fd);
            }
        }
    }

    void processFunction (FunctionDefinition& fd)
    {
        // Push function scope
        symbolTable.pushScope();

        // Register parameters in function scope
        for (auto& param : fd.prototype.parameters)
        {
            SymbolInfo info;
            info.type = param.type;
            info.isParameter = true;

            if (param.qualifier)
            {
                info.isOutParam = param.qualifier->hasStorage (StorageQualifier::out);
                info.isInoutParam = param.qualifier->hasStorage (StorageQualifier::inout);
            }

            symbolTable.declare (param.name, info);
        }

        // Walk body to mark reassigned parameters (Task 2.2)
        if (fd.body)
            markReassignedSymbols (*fd.body);

        // Shadow reassigned parameters: WGSL params are immutable, so rename
        // the param and insert a mutable local copy (Task 2.2).
        shadowReassignedParams (fd);

        // Process function body
        if (fd.body)
        {
            processStatement (*fd.body);
            legalizeStatement (*fd.body);
        }

        symbolTable.popScope();
    }

    void markReassignedSymbols (Statement& stmt)
    {
        if (stmt.is<StmtExpr>())
        {
            auto& se = stmt.as<StmtExpr>();
            if (se.expr)
                markReassignedInExpr (*se.expr);
        }
        else if (stmt.is<StmtCompound>())
        {
            for (auto& s : stmt.as<StmtCompound>().statements)
                markReassignedSymbols (s);
        }
        else if (stmt.is<StmtDeclaration>())
        {
            auto& sd = stmt.as<StmtDeclaration>();
            if (sd.declaration.initDeclaratorList)
            {
                for (auto& dec : sd.declaration.initDeclaratorList->declarations)
                {
                    // Register local
                    SymbolInfo info;
                    info.type = sd.declaration.initDeclaratorList->type;
                    info.isConst = sd.declaration.initDeclaratorList->qualifier
                                && sd.declaration.initDeclaratorList->qualifier->hasStorage (StorageQualifier::constQual);
                    symbolTable.declare (dec.name, info);

                    if (dec.initializer && dec.initializer->expr)
                        markReassignedInExpr (*dec.initializer->expr);
                }
            }
        }
        else if (stmt.is<StmtSelection>())
        {
            auto& sel = stmt.as<StmtSelection>();
            if (sel.condition)
                markReassignedInExpr (*sel.condition);
            if (sel.thenBranch)
                markReassignedSymbols (*sel.thenBranch);
            if (sel.elseBranch)
                markReassignedSymbols (*sel.elseBranch);
        }
        else if (stmt.is<StmtWhile>())
        {
            auto& w = stmt.as<StmtWhile>();
            if (w.condition)
                markReassignedInExpr (*w.condition);
            if (w.body)
                markReassignedSymbols (*w.body);
        }
        else if (stmt.is<StmtFor>())
        {
            auto& f = stmt.as<StmtFor>();
            if (f.init)
                markReassignedSymbols (*f.init);
            if (f.condition)
                markReassignedInExpr (*f.condition);
            if (f.update)
                markReassignedInExpr (*f.update);
            if (f.body)
                markReassignedSymbols (*f.body);
        }
    }

    void markReassignedInExpr (Expr& expr)
    {
        if (expr.is<ExprAssignment>())
        {
            auto& assign = expr.as<ExprAssignment>();
            if (assign.lhs && assign.lhs->is<ExprVariable>())
                symbolTable.markReassigned (assign.lhs->as<ExprVariable>().name);
            if (assign.lhs)
                markReassignedInExpr (*assign.lhs);
            if (assign.rhs)
                markReassignedInExpr (*assign.rhs);
        }
        else if (expr.is<ExprUnary>())
        {
            auto& un = expr.as<ExprUnary>();
            if (un.op == UnaryOp::preInc || un.op == UnaryOp::preDec || un.op == UnaryOp::postInc || un.op == UnaryOp::postDec)
            {
                if (un.operand && un.operand->is<ExprVariable>())
                    symbolTable.markReassigned (un.operand->as<ExprVariable>().name);
            }
            if (un.operand)
                markReassignedInExpr (*un.operand);
        }
    }

    //==========================================================================
    // Shadow reassigned parameters (Task 2.2)
    //==========================================================================

    void shadowReassignedParams (FunctionDefinition& fd)
    {
        // Collect reassigned parameter names
        std::vector<std::pair<std::string, TypeSpecifier>> shadowed;
        for (auto& param : fd.prototype.parameters)
        {
            auto* info = symbolTable.lookup (param.name);
            if (info && info->isReassigned)
                shadowed.push_back ({ param.name, param.type });
        }

        if (shadowed.empty())
            return;

        // Rename params in the prototype
        for (auto& param : fd.prototype.parameters)
        {
            for (auto& [origName, origType] : shadowed)
            {
                if (param.name == origName)
                {
                    param.name = "_" + origName;
                    break;
                }
            }
        }

        // Insert shadow var declarations at the top of the function body
        if (! fd.body || ! fd.body->is<StmtCompound>())
            return;

        auto& comp = fd.body->as<StmtCompound>();

        // Insert in reverse order so they end up in the right order at the top
        for (auto it = shadowed.rbegin(); it != shadowed.rend(); ++it)
        {
            auto& [origName, origType] = *it;

            // Build: var origName: type = _origName;
            InitDeclaratorList il;
            il.loc = fd.prototype.loc;
            il.type = origType;

            SingleDeclaration sd;
            sd.loc = fd.prototype.loc;
            sd.name = origName;

            Initializer init;
            init.loc = fd.prototype.loc;
            {
                Expr e;
                e.loc = fd.prototype.loc;
                e.value = ExprVariable { fd.prototype.loc, "_" + origName };
                init.expr = std::make_unique<Expr> (std::move (e));
            }

            sd.initializer = std::make_unique<Initializer> (std::move (init));
            il.declarations.push_back (std::move (sd));

            Declaration decl;
            decl.loc = fd.prototype.loc;
            decl.initDeclaratorList = std::make_unique<InitDeclaratorList> (std::move (il));

            StmtDeclaration sdStmt;
            sdStmt.loc = fd.prototype.loc;
            sdStmt.declaration = std::move (decl);

            Statement stmt;
            stmt.loc = fd.prototype.loc;
            stmt.value = std::move (sdStmt);
            comp.statements.insert (comp.statements.begin(), std::move (stmt));
        }
    }

    //==========================================================================
    // Step 2.5: Collect stage IO for entry-point wrapping
    //==========================================================================

    void collectStageIO (LoweredProgram& result)
    {
        for (auto& decl : result.ast.declarations)
        {
            if (! std::holds_alternative<Declaration> (decl))
                continue;

            auto& d = std::get<Declaration> (decl);
            if (! d.initDeclaratorList)
                continue;

            auto& il = *d.initDeclaratorList;

            if (! il.qualifier)
                continue;

            bool isIn = il.qualifier->hasStorage (StorageQualifier::in);
            bool isOut = il.qualifier->hasStorage (StorageQualifier::out);

            if (! isIn && ! isOut)
                continue;

            // Skip uniform/buffer blocks - those are resources, not stage IO
            if (il.qualifier->hasStorage (StorageQualifier::uniform) || il.qualifier->hasStorage (StorageQualifier::buffer))
                continue;

            uint32_t location = 0;
            if (il.qualifier->layout)
            {
                for (auto& entry : il.qualifier->layout->entries)
                {
                    if (entry.id == LayoutQualifierId::location && entry.value && entry.value->is<ExprIntConst>())
                        location = static_cast<uint32_t> (entry.value->as<ExprIntConst>().value);
                }
            }

            for (auto& sd : il.declarations)
            {
                LoweredProgram::InputOutputInfo io;
                io.name = sd.name;
                io.wgslType = il.type;
                io.location = location;

                // Check for builtins
                if (sd.name == "gl_Position")
                {
                    io.isBuiltin = true;
                    io.builtinName = "position";
                }
                else if (sd.name == "gl_FragCoord")
                {
                    io.isBuiltin = true;
                    io.builtinName = "position";
                }
                else if (sd.name == "gl_FragDepth")
                {
                    io.isBuiltin = true;
                    io.builtinName = "frag_depth";
                }
                else if (sd.name == "gl_FrontFacing")
                {
                    io.isBuiltin = true;
                    io.builtinName = "front_facing";
                }
                else if (sd.name == "gl_VertexIndex" || sd.name == "gl_VertexID")
                {
                    io.isBuiltin = true;
                    io.builtinName = "vertex_index";
                }
                else if (sd.name == "gl_InstanceIndex" || sd.name == "gl_InstanceID")
                {
                    io.isBuiltin = true;
                    io.builtinName = "instance_index";
                }
                else if (sd.name == "gl_GlobalInvocationID")
                {
                    io.isBuiltin = true;
                    io.builtinName = "global_invocation_id";
                }
                else if (sd.name == "gl_LocalInvocationID")
                {
                    io.isBuiltin = true;
                    io.builtinName = "local_invocation_id";
                }
                else if (sd.name == "gl_LocalInvocationIndex")
                {
                    io.isBuiltin = true;
                    io.builtinName = "local_invocation_index";
                }
                else if (sd.name == "gl_WorkGroupID")
                {
                    io.isBuiltin = true;
                    io.builtinName = "workgroup_id";
                }
                else if (sd.name == "gl_NumWorkGroups")
                {
                    io.isBuiltin = true;
                    io.builtinName = "num_workgroups";
                }

                if (isIn)
                    result.entryPoint.inputs.push_back (io);
                else
                    result.entryPoint.outputs.push_back (io);
            }
        }

        // Handle gl_PointSize → warning
        for (auto it = result.entryPoint.outputs.begin(); it != result.entryPoint.outputs.end();)
        {
            if (it->name == "gl_PointSize")
            {
                diag.addWarning ({ 0, 0 }, "gl_PointSize has no WGSL equivalent; dropped");
                it = result.entryPoint.outputs.erase (it);
            }
            else
            {
                ++it;
            }
        }
    }

    //==========================================================================
    // Extract workgroup sizes from standalone layout(local_size_x/y/z) in;
    //==========================================================================

    void extractWorkgroupSizesFromLayout (LoweredProgram& result)
    {
        for (auto& decl : result.ast.declarations)
        {
            if (! std::holds_alternative<Declaration> (decl))
                continue;

            auto& d = std::get<Declaration> (decl);
            if (! d.qualifier || ! d.qualifier->layout)
                continue;

            for (auto& entry : d.qualifier->layout->entries)
            {
                if (entry.id == LayoutQualifierId::localSizeX && entry.value && entry.value->is<ExprIntConst>())
                    result.entryPoint.workgroupSizeX = static_cast<uint32_t> (entry.value->as<ExprIntConst>().value);
                else if (entry.id == LayoutQualifierId::localSizeY && entry.value && entry.value->is<ExprIntConst>())
                    result.entryPoint.workgroupSizeY = static_cast<uint32_t> (entry.value->as<ExprIntConst>().value);
                else if (entry.id == LayoutQualifierId::localSizeZ && entry.value && entry.value->is<ExprIntConst>())
                    result.entryPoint.workgroupSizeZ = static_cast<uint32_t> (entry.value->as<ExprIntConst>().value);
            }
        }
    }

    //==========================================================================
    // Collect compute builtins from function body variable usage
    //==========================================================================

    void collectComputeBuiltinsFromUsage (LoweredProgram& result)
    {
        if (! result.entryPoint.isCompute)
            return;

        for (auto& decl : result.ast.declarations)
        {
            if (! std::holds_alternative<FunctionDefinition> (decl))
                continue;

            auto& fd = std::get<FunctionDefinition> (decl);
            if (fd.body)
                collectBuiltinRefsFromStmt (*fd.body, result);
        }
    }

    void collectBuiltinRefsFromStmt (Statement& stmt, LoweredProgram& result)
    {
        if (stmt.is<StmtSelection>())
        {
            auto& sel = stmt.as<StmtSelection>();
            if (sel.condition)
                collectBuiltinRefsFromExpr (*sel.condition, result);
            if (sel.thenBranch)
                collectBuiltinRefsFromStmt (*sel.thenBranch, result);
            if (sel.elseBranch)
                collectBuiltinRefsFromStmt (*sel.elseBranch, result);
        }
        else if (stmt.is<StmtSwitch>())
        {
            auto& sw = stmt.as<StmtSwitch>();
            if (sw.selector)
                collectBuiltinRefsFromExpr (*sw.selector, result);
            for (auto& s : sw.body)
                collectBuiltinRefsFromStmt (s, result);
        }
        else if (stmt.is<StmtWhile>())
        {
            auto& w = stmt.as<StmtWhile>();
            if (w.condition)
                collectBuiltinRefsFromExpr (*w.condition, result);
            if (w.body)
                collectBuiltinRefsFromStmt (*w.body, result);
        }
        else if (stmt.is<StmtDoWhile>())
        {
            auto& dw = stmt.as<StmtDoWhile>();
            if (dw.condition)
                collectBuiltinRefsFromExpr (*dw.condition, result);
            if (dw.body)
                collectBuiltinRefsFromStmt (*dw.body, result);
        }
        else if (stmt.is<StmtFor>())
        {
            auto& f = stmt.as<StmtFor>();
            if (f.init)
                collectBuiltinRefsFromStmt (*f.init, result);
            if (f.condition)
                collectBuiltinRefsFromExpr (*f.condition, result);
            if (f.update)
                collectBuiltinRefsFromExpr (*f.update, result);
            if (f.body)
                collectBuiltinRefsFromStmt (*f.body, result);
        }
        else if (stmt.is<StmtJump>())
        {
            auto& j = stmt.as<StmtJump>();
            if (j.returnValue)
                collectBuiltinRefsFromExpr (*j.returnValue, result);
        }
        else if (stmt.is<StmtExpr>())
        {
            auto& se = stmt.as<StmtExpr>();
            if (se.expr)
                collectBuiltinRefsFromExpr (*se.expr, result);
        }
        else if (stmt.is<StmtCompound>())
        {
            auto& sc = stmt.as<StmtCompound>();
            for (auto& s : sc.statements)
                collectBuiltinRefsFromStmt (s, result);
        }
        else if (stmt.is<StmtDeclaration>())
        {
            auto& sd = stmt.as<StmtDeclaration>();
            if (sd.declaration.initDeclaratorList)
            {
                auto& il = *sd.declaration.initDeclaratorList;
                for (auto& dec : il.declarations)
                {
                    if (dec.initializer && dec.initializer->expr)
                        collectBuiltinRefsFromExpr (*dec.initializer->expr, result);
                }
            }
        }
    }

    void collectBuiltinRefsFromExpr (Expr& expr, LoweredProgram& result)
    {
        if (expr.is<ExprVariable>())
        {
            addComputeBuiltinIfUsed (expr.as<ExprVariable>().name, result);
        }
        else if (expr.is<ExprDot>())
        {
            auto& dot = expr.as<ExprDot>();
            if (dot.base && dot.base->is<ExprVariable>())
                addComputeBuiltinIfUsed (dot.base->as<ExprVariable>().name, result);
            if (dot.base)
                collectBuiltinRefsFromExpr (*dot.base, result);
        }
        else if (expr.is<ExprTernary>())
        {
            auto& tern = expr.as<ExprTernary>();
            if (tern.condition)
                collectBuiltinRefsFromExpr (*tern.condition, result);
            if (tern.trueBranch)
                collectBuiltinRefsFromExpr (*tern.trueBranch, result);
            if (tern.falseBranch)
                collectBuiltinRefsFromExpr (*tern.falseBranch, result);
        }
        else if (expr.is<ExprBinary>())
        {
            auto& bin = expr.as<ExprBinary>();
            if (bin.left)
                collectBuiltinRefsFromExpr (*bin.left, result);
            if (bin.right)
                collectBuiltinRefsFromExpr (*bin.right, result);
        }
        else if (expr.is<ExprUnary>())
        {
            auto& un = expr.as<ExprUnary>();
            if (un.operand)
                collectBuiltinRefsFromExpr (*un.operand, result);
        }
        else if (expr.is<ExprFunCall>())
        {
            auto& fc = expr.as<ExprFunCall>();
            if (fc.callee)
                collectBuiltinRefsFromExpr (*fc.callee, result);
            for (auto& arg : fc.args)
                collectBuiltinRefsFromExpr (arg, result);
        }
        else if (expr.is<ExprAssignment>())
        {
            auto& assign = expr.as<ExprAssignment>();
            if (assign.lhs)
                collectBuiltinRefsFromExpr (*assign.lhs, result);
            if (assign.rhs)
                collectBuiltinRefsFromExpr (*assign.rhs, result);
        }
        else if (expr.is<ExprBracket>())
        {
            auto& br = expr.as<ExprBracket>();
            if (br.base)
                collectBuiltinRefsFromExpr (*br.base, result);
            if (br.index)
                collectBuiltinRefsFromExpr (*br.index, result);
        }
        else if (expr.is<ExprTypeConstructor>())
        {
            auto& tc = expr.as<ExprTypeConstructor>();
            for (auto& arg : tc.args)
                collectBuiltinRefsFromExpr (arg, result);
        }
        else if (expr.is<ExprParen>())
        {
            auto& p = expr.as<ExprParen>();
            if (p.expr)
                collectBuiltinRefsFromExpr (*p.expr, result);
        }
        else if (expr.is<ExprComma>())
        {
            auto& com = expr.as<ExprComma>();
            if (com.left)
                collectBuiltinRefsFromExpr (*com.left, result);
            if (com.right)
                collectBuiltinRefsFromExpr (*com.right, result);
        }
    }

    void addComputeBuiltinIfUsed (const std::string& name, LoweredProgram& result)
    {
        std::string builtinName;

        if (name == "gl_GlobalInvocationID")
            builtinName = "global_invocation_id";
        else if (name == "gl_LocalInvocationID")
            builtinName = "local_invocation_id";
        else if (name == "gl_LocalInvocationIndex")
            builtinName = "local_invocation_index";
        else if (name == "gl_WorkGroupID")
            builtinName = "workgroup_id";
        else if (name == "gl_NumWorkGroups")
            builtinName = "num_workgroups";
        else
            return;

        // Check if already present
        for (auto& io : result.entryPoint.inputs)
        {
            if (io.builtinName == builtinName)
                return;
        }

        LoweredProgram::InputOutputInfo io;
        io.name = name;
        io.isBuiltin = true;
        io.builtinName = builtinName;
        io.wgslType = TypeSpecifier::make ({ 0, 0 }, TypeKind::uintType);
        result.entryPoint.inputs.push_back (io);
    }

    //==========================================================================
    // Step processing (for stmt-by-stmt mutation)
    //==========================================================================

    void processStatement (Statement& stmt)
    {
        // No-op in v1; structure preserved for future passes
    }

    //==========================================================================
    // Step 2.6: Expression/statement legalization
    //==========================================================================

    void legalizeTranslationUnit (TranslationUnit& ast)
    {
        for (auto& decl : ast.declarations)
        {
            if (std::holds_alternative<FunctionDefinition> (decl))
            {
                auto& fd = std::get<FunctionDefinition> (decl);
                if (fd.body)
                    legalizeStatement (*fd.body);
            }
        }
    }

    void legalizeStatement (Statement& stmt)
    {
        if (stmt.is<StmtSelection>())
        {
            auto& sel = stmt.as<StmtSelection>();
            if (sel.condition)
                legalizeExpr (*sel.condition);
            if (sel.thenBranch)
                legalizeStatement (*sel.thenBranch);
            if (sel.elseBranch)
                legalizeStatement (*sel.elseBranch);
        }
        else if (stmt.is<StmtWhile>())
        {
            auto& w = stmt.as<StmtWhile>();
            if (w.condition)
                legalizeExpr (*w.condition);
            if (w.body)
                legalizeStatement (*w.body);
        }
        else if (stmt.is<StmtDoWhile>())
        {
            auto& dw = stmt.as<StmtDoWhile>();
            if (dw.condition)
                legalizeExpr (*dw.condition);
            if (dw.body)
                legalizeStatement (*dw.body);
        }
        else if (stmt.is<StmtFor>())
        {
            auto& f = stmt.as<StmtFor>();
            if (f.init)
                legalizeStatement (*f.init);
            if (f.condition)
                legalizeExpr (*f.condition);
            if (f.update)
                legalizeExpr (*f.update);
            if (f.body)
                legalizeStatement (*f.body);
        }
        else if (stmt.is<StmtCompound>())
        {
            for (auto& s : stmt.as<StmtCompound>().statements)
                legalizeStatement (s);
        }
        else if (stmt.is<StmtSwitch>())
        {
            auto& sw = stmt.as<StmtSwitch>();
            if (sw.selector)
                legalizeExpr (*sw.selector);
            for (auto& s : sw.body)
                legalizeStatement (s);
        }
        else if (stmt.is<StmtExpr>())
        {
            auto& se = stmt.as<StmtExpr>();
            if (se.expr)
                legalizeExpr (*se.expr);
        }
        else if (stmt.is<StmtDeclaration>())
        {
            auto& sd = stmt.as<StmtDeclaration>();
            if (sd.declaration.initDeclaratorList)
            {
                for (auto& dec : sd.declaration.initDeclaratorList->declarations)
                {
                    if (dec.initializer && dec.initializer->expr)
                        legalizeExpr (*dec.initializer->expr);
                }
            }
        }
        else if (stmt.is<StmtJump>())
        {
            auto& j = stmt.as<StmtJump>();
            if (j.returnValue)
                legalizeExpr (*j.returnValue);
        }
    }

    void legalizeExpr (Expr& expr)
    {
        // Ternary → select for side-effect-free branches,
        // or if/else into var for complex cases
        if (expr.is<ExprTernary>())
        {
            auto& tern = expr.as<ExprTernary>();
            if (tern.condition)
                legalizeExpr (*tern.condition);
            if (tern.trueBranch)
                legalizeExpr (*tern.trueBranch);
            if (tern.falseBranch)
                legalizeExpr (*tern.falseBranch);

            // Simple ternary (side-effect free) → kept as select() in emitter
            // Complex ternary → lowered to if/else in emitter based on analysis
        }
        else if (expr.is<ExprBinary>())
        {
            auto& bin = expr.as<ExprBinary>();
            if (bin.left)
                legalizeExpr (*bin.left);
            if (bin.right)
                legalizeExpr (*bin.right);

            // Floor-mod lowering: mod(x,y) → x - y * floor(x / y)
            // The emitter handles this; we just mark it for the emitter
        }
        else if (expr.is<ExprUnary>())
        {
            auto& un = expr.as<ExprUnary>();
            if (un.operand)
                legalizeExpr (*un.operand);
        }
        else if (expr.is<ExprFunCall>())
        {
            auto& fc = expr.as<ExprFunCall>();
            if (fc.callee)
                legalizeExpr (*fc.callee);
            for (auto& arg : fc.args)
                legalizeExpr (arg);
        }
        else if (expr.is<ExprAssignment>())
        {
            auto& assign = expr.as<ExprAssignment>();
            if (assign.lhs)
                legalizeExpr (*assign.lhs);
            if (assign.rhs)
                legalizeExpr (*assign.rhs);
        }
        else if (expr.is<ExprBracket>())
        {
            auto& br = expr.as<ExprBracket>();
            if (br.base)
                legalizeExpr (*br.base);
            if (br.index)
                legalizeExpr (*br.index);
        }
        else if (expr.is<ExprDot>())
        {
            auto& dot = expr.as<ExprDot>();
            if (dot.base)
                legalizeExpr (*dot.base);
        }
        else if (expr.is<ExprComma>())
        {
            auto& com = expr.as<ExprComma>();
            if (com.left)
                legalizeExpr (*com.left);
            if (com.right)
                legalizeExpr (*com.right);
        }
        else if (expr.is<ExprTypeConstructor>())
        {
            auto& tc = expr.as<ExprTypeConstructor>();
            for (auto& arg : tc.args)
                legalizeExpr (arg);
        }
        else if (expr.is<ExprParen>())
        {
            auto& p = expr.as<ExprParen>();
            if (p.expr)
                legalizeExpr (*p.expr);
        }
    }

    WgslLoweringOptions options;
    DiagnosticContext diag;
    SymbolTable symbolTable;
    BindingAllocator allocator;
};

} // namespace

//==============================================================================
// WgslLowering::lower()
//==============================================================================

ResultValue<LoweredProgram> WgslLowering::lower (TranslationUnit ast,
                                                 const WgslLoweringOptions& options)
{
    try
    {
        LoweringImpl impl (options);
        return impl.run (std::move (ast));
    }
    catch (const std::exception& e)
    {
        return makeResultValueFail (String ("WGSL lowering error: ") + e.what());
    }
}

} // namespace wgsl
} // namespace yup
