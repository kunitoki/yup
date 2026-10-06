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

//==============================================================================
std::vector<uint32_t> assignWgslCompanionSamplerBindings (const std::vector<WgslBindingSlot>& slots)
{
    std::map<uint32_t, uint32_t> nextFree;
    std::vector<size_t> combined;

    for (size_t i = 0; i < slots.size(); ++i)
    {
        auto& next = nextFree[slots[i].group];
        next = std::max (next, slots[i].binding + 1);

        if (slots[i].isCombinedSampler)
            combined.push_back (i);
    }

    std::stable_sort (combined.begin(), combined.end(), [&slots] (size_t a, size_t b)
    {
        return std::make_pair (slots[a].group, slots[a].binding) < std::make_pair (slots[b].group, slots[b].binding);
    });

    std::vector<uint32_t> result (slots.size(), ~0u);

    for (const auto index : combined)
        result[index] = nextFree[slots[index].group]++;

    return result;
}

namespace
{

//==============================================================================
/** WGSL keywords, reserved words and the predeclared names the emitted code relies on. */
const std::unordered_set<std::string>& reservedWgslNames()
{
    static const std::unordered_set<std::string> names = {
        // Keywords
        "alias", "break", "case", "const", "const_assert", "continue", "continuing", "default", "diagnostic", "discard",
        "else", "enable", "false", "fn", "for", "if", "let", "loop", "override", "requires", "return", "struct", "switch",
        "true", "var", "while",

        // Reserved words
        "NULL", "Self", "abstract", "active", "alignas", "alignof", "as", "asm", "asm_fragment", "async", "attribute",
        "auto", "await", "become", "binding_array", "cast", "catch", "class", "co_await", "co_return", "co_yield",
        "coherent", "column_major", "common", "compile", "compile_fragment", "concept", "const_cast", "consteval",
        "constexpr", "constinit", "crate", "debugger", "decltype", "delete", "demote", "demote_to_helper", "do",
        "dynamic_cast", "enum", "explicit", "export", "extends", "extern", "external", "fallthrough", "filter", "final",
        "finally", "friend", "from", "fxgroup", "get", "goto", "groupshared", "highp", "impl", "implements", "import",
        "inline", "instanceof", "interface", "layout", "lowp", "macro", "macro_rules", "match", "mediump", "meta", "mod",
        "module", "move", "mut", "mutable", "namespace", "new", "nil", "noexcept", "noinline", "nointerpolation",
        "noperspective", "null", "nullptr", "of", "operator", "package", "packoffset", "partition", "pass", "patch",
        "pixelfragment", "precise", "precision", "premerge", "priv", "protected", "pub", "public", "readonly", "ref",
        "regardless", "register", "reinterpret_cast", "require", "resource", "restrict", "self", "set", "shared",
        "sizeof", "smooth", "snorm", "static", "static_assert", "static_cast", "std", "subroutine", "super", "target",
        "template", "this", "thread_local", "throw", "trait", "try", "type", "typedef", "typeid", "typename", "typeof",
        "union", "unless", "unorm", "unsafe", "unsized", "use", "using", "varying", "virtual", "volatile", "wgsl",
        "where", "with", "writeonly", "yield",

        // Predeclared types, enumerants and aliases
        "bool", "f16", "f32", "i32", "u32", "vec2", "vec3", "vec4", "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3",
        "mat3x4", "mat4x2", "mat4x3", "mat4x4", "array", "atomic", "ptr", "sampler", "sampler_comparison",
        "texture_1d", "texture_2d", "texture_2d_array", "texture_3d", "texture_cube", "texture_cube_array",
        "texture_multisampled_2d", "texture_depth_multisampled_2d", "texture_external", "texture_storage_1d",
        "texture_storage_2d", "texture_storage_2d_array", "texture_storage_3d", "texture_depth_2d",
        "texture_depth_2d_array", "texture_depth_cube", "texture_depth_cube_array", "function", "private",
        "workgroup", "uniform", "storage", "handle", "read", "write", "read_write", "vec2f", "vec3f", "vec4f", "vec2i",
        "vec3i", "vec4i", "vec2u", "vec3u", "vec4u", "vec2h", "vec3h", "vec4h", "mat2x2f", "mat2x3f", "mat2x4f",
        "mat3x2f", "mat3x3f", "mat3x4f", "mat4x2f", "mat4x3f", "mat4x4f",

        // Builtin functions the lowering emits
        "bitcast", "all", "any", "select", "arrayLength", "abs", "acos", "acosh", "asin", "asinh", "atan", "atanh",
        "atan2", "ceil", "clamp", "cos", "cosh", "countLeadingZeros", "countOneBits", "countTrailingZeros", "cross",
        "degrees", "determinant", "distance", "dot", "dot4U8Packed", "dot4I8Packed", "exp", "exp2", "extractBits",
        "faceForward", "firstLeadingBit", "firstTrailingBit", "floor", "fma", "fract", "frexp", "insertBits",
        "inverseSqrt", "ldexp", "length", "log", "log2", "max", "min", "mix", "modf", "normalize", "pow",
        "quantizeToF16", "radians", "reflect", "refract", "reverseBits", "round", "saturate", "sign", "sin", "sinh",
        "smoothstep", "sqrt", "step", "tan", "tanh", "transpose", "trunc", "dpdx", "dpdxCoarse", "dpdxFine", "dpdy",
        "dpdyCoarse", "dpdyFine", "fwidth", "fwidthCoarse", "fwidthFine", "textureDimensions", "textureGather",
        "textureGatherCompare", "textureLoad", "textureNumLayers", "textureNumLevels", "textureNumSamples",
        "textureSample", "textureSampleBias", "textureSampleCompare", "textureSampleCompareLevel", "textureSampleGrad",
        "textureSampleLevel", "textureSampleBaseClampToEdge", "textureStore", "atomicLoad", "atomicStore", "atomicAdd",
        "atomicSub", "atomicMax", "atomicMin", "atomicAnd", "atomicOr", "atomicXor", "atomicExchange",
        "atomicCompareExchangeWeak", "pack4x8snorm", "pack4x8unorm", "pack2x16snorm", "pack2x16unorm",
        "pack2x16float", "unpack4x8snorm", "unpack4x8unorm", "unpack2x16snorm", "unpack2x16unorm", "unpack2x16float",
        "storageBarrier", "textureBarrier", "workgroupBarrier", "workgroupUniformLoad"
    };

    return names;
}

bool needsRename (const std::string& name)
{
    return reservedWgslNames().count (name) > 0 || name == "_" || name.rfind ("__", 0) == 0;
}

bool isBuiltinVariableName (const std::string& name)
{
    return name.rfind ("gl_", 0) == 0;
}

std::optional<int64_t> layoutValue (const TypeQualifier* qualifier, LayoutQualifierId id)
{
    if (qualifier == nullptr || qualifier->layout == nullptr)
        return std::nullopt;

    std::optional<int64_t> result;

    for (const auto& entry : qualifier->layout->entries)
    {
        if (entry.id != id)
            continue;

        if (entry.value == nullptr)
            throw LoweringError (entry.loc, "layout(" + entry.name + ") requires a value");

        result = evaluateIntConstant (*entry.value);

        if (! result.has_value() || *result < 0)
            throw LoweringError (entry.loc, "layout(" + entry.name + ") must be a non-negative integer constant");
    }

    return result;
}

bool hasLayout (const TypeQualifier* qualifier, LayoutQualifierId id)
{
    if (qualifier == nullptr || qualifier->layout == nullptr)
        return false;

    for (const auto& entry : qualifier->layout->entries)
        if (entry.id == id)
            return true;

    return false;
}

bool isBlock (const Declaration& d)
{
    return d.structSpecifier != nullptr && d.qualifier != nullptr
        && (d.qualifier->hasStorage (StorageQualifier::uniform) || d.qualifier->hasStorage (StorageQualifier::buffer)
            || d.qualifier->hasStorage (StorageQualifier::in) || d.qualifier->hasStorage (StorageQualifier::out));
}

bool isResourceBlock (const Declaration& d)
{
    return isBlock (d) && (d.qualifier->hasStorage (StorageQualifier::uniform) || d.qualifier->hasStorage (StorageQualifier::buffer));
}

//==============================================================================
/** Visits every TypeSpecifier stored in the AST. */
template <typename F>
void forEachTypeSpecifier (TranslationUnit& ast, F&& f)
{
    const auto visitExprTypes = [&f] (Expr& root)
    {
        walkExpr (root, [&f] (Expr& e)
        {
            if (e.is<ExprTypeConstructor>())
                f (e.as<ExprTypeConstructor>().type);
        });
    };

    const auto visitDeclaration = [&] (Declaration& d)
    {
        if (d.structSpecifier != nullptr)
            for (auto& field : d.structSpecifier->fields)
                f (field.type);

        if (d.initDeclaratorList != nullptr)
            f (d.initDeclaratorList->type);

        walkDeclarationExprs (d, visitExprTypes);
    };

    std::function<void (Statement&)> visitStatement = [&] (Statement& s)
    {
        if (s.is<StmtDeclaration>())
            visitDeclaration (s.as<StmtDeclaration>().declaration);
        else
            forEachOwnExpr (s, visitExprTypes);

        forEachChildStatement (s, visitStatement);
    };

    for (auto& external : ast.declarations)
    {
        if (auto* d = std::get_if<Declaration> (&external))
            visitDeclaration (*d);
        else if (auto* fd = std::get_if<FunctionDefinition> (&external))
        {
            f (fd->prototype.returnType);

            for (auto& param : fd->prototype.parameters)
                f (param.type);

            if (fd->body != nullptr)
                visitStatement (*fd->body);
        }
    }
}

/** Visits every statement of every function, parents first. */
template <typename F>
void forEachStatement (Statement& s, F&& f)
{
    f (s);
    forEachChildStatement (s, [&f] (Statement& child)
    {
        forEachStatement (child, f);
    });
}

/** Visits every expression of every function body and global initializer. */
template <typename F>
void forEachExpression (TranslationUnit& ast, F&& f)
{
    for (auto& external : ast.declarations)
    {
        if (auto* d = std::get_if<Declaration> (&external))
            walkDeclarationExprs (*d, f);
        else if (auto* fd = std::get_if<FunctionDefinition> (&external); fd != nullptr && fd->body != nullptr)
        {
            forEachStatement (*fd->body, [&f] (Statement& s)
            {
                if (s.is<StmtDeclaration>())
                    walkDeclarationExprs (s.as<StmtDeclaration>().declaration, f);
                else
                    forEachOwnExpr (s, [&f] (Expr& e)
                    {
                        walkExpr (e, f);
                    });
            });
        }
    }
}

//==============================================================================
/** Scope-aware rewriting of variable references inside function bodies. */
class ReferenceRewriter
{
public:
    using Rewrite = std::function<bool (Expr&)>; // returns true when it replaced the expression

    ReferenceRewriter (std::set<std::string> globalNames, Rewrite rewriteFn)
        : globals (std::move (globalNames))
        , rewrite (std::move (rewriteFn))
    {
    }

    /** True if name refers to one of the globals of interest at the current point. */
    bool refersToGlobal (const std::string& name) const
    {
        if (globals.count (name) == 0)
            return false;

        for (const auto& scope : scopes)
            if (scope.count (name) > 0)
                return false;

        return true;
    }

    void run (TranslationUnit& ast)
    {
        for (auto& external : ast.declarations)
        {
            if (auto* fd = std::get_if<FunctionDefinition> (&external); fd != nullptr && fd->body != nullptr)
            {
                scopes.clear();
                scopes.emplace_back();

                for (const auto& param : fd->prototype.parameters)
                    scopes.back().insert (param.name);

                visitStatement (*fd->body);
            }
        }
    }

private:
    void visitExpr (Expr& e)
    {
        if (rewrite (e))
            return;

        forEachChildExpr (e, [this] (Expr& child)
        {
            visitExpr (child);
        });
    }

    void visitDeclaration (Declaration& d)
    {
        forEachDeclarationExpr (d, [this] (Expr& e)
        {
            visitExpr (e);
        });

        if (d.initDeclaratorList != nullptr)
            for (const auto& single : d.initDeclaratorList->declarations)
                scopes.back().insert (single.name);
    }

    void visitStatement (Statement& s)
    {
        const bool opensScope = s.is<StmtCompound>() || s.is<StmtFor>() || s.is<StmtSwitch>() || s.is<StmtLoop>();

        if (opensScope)
            scopes.emplace_back();

        if (s.is<StmtDeclaration>())
        {
            visitDeclaration (s.as<StmtDeclaration>().declaration);
        }
        else if (s.is<StmtFor>())
        {
            auto& f = s.as<StmtFor>();

            if (f.init != nullptr)
                visitStatement (*f.init);

            if (f.condition != nullptr)
                visitExpr (*f.condition);

            if (f.update != nullptr)
                visitExpr (*f.update);

            if (f.body != nullptr)
                visitStatement (*f.body);
        }
        else
        {
            forEachOwnExpr (s, [this] (Expr& e)
            {
                visitExpr (e);
            });

            forEachChildStatement (s, [this] (Statement& child)
            {
                visitStatement (child);
            });
        }

        if (opensScope)
            scopes.pop_back();
    }

    std::set<std::string> globals;
    Rewrite rewrite;
    std::vector<std::set<std::string>> scopes;
};

//==============================================================================
class LoweringImpl
{
public:
    explicit LoweringImpl (const WgslLoweringOptions& opts)
        : options (opts)
    {
        context.stage = opts.stage;
    }

    LoweredProgram run (TranslationUnit ast)
    {
        LoweredProgram result;
        result.ast = std::move (ast);
        auto& unit = result.ast;

        checkStage();
        sanitizeNames (unit);
        nameUnnamedParameters (unit);
        renameOverloads (unit);
        normalizeBlocks (unit);
        hoistLocalStructs (unit);
        runDiagnostics (unit);
        findDepthTextures (unit);

        setupEntryPoint (result);
        assignBindings (result);
        extractWorkgroupSize (result);
        lowerStageIO (result);

        convertAggregateInitializers (unit);
        WgslTypeLegalizer::legalize (unit);
        sizeArraysFromInitializers (unit);
        lowerGlobalInitializers (unit);
        WgslHostLayout::apply (unit, context);
        WgslStatementLowering::lower (unit, context);

        result.polyfills = std::move (context.polyfills);
        result.depthTextures = context.depthTextures;
        result.warnings.insert (result.warnings.end(), context.warnings.begin(), context.warnings.end());
        return result;
    }

private:
    void warn (const SourceLocation& loc, const std::string& message)
    {
        context.warnings.push_back (formatDiagnostic (loc, message));
    }

    void checkStage()
    {
        if (options.stage == ShaderStage::geometry || options.stage == ShaderStage::tessControl || options.stage == ShaderStage::tessEval)
            throw LoweringError ({ 0, 0 }, "Geometry and tessellation stages are not supported for WGSL output");
    }

    //==========================================================================
    // Identifier sanitization
    //==========================================================================

    /** Collects every identifier the shader declares, in any namespace. */
    std::set<std::string> collectDeclaredNames (TranslationUnit& ast)
    {
        std::set<std::string> names;

        const auto addDeclaration = [&names] (Declaration& d)
        {
            if (d.structSpecifier != nullptr)
            {
                names.insert (d.structSpecifier->name);

                for (const auto& field : d.structSpecifier->fields)
                    names.insert (field.name);
            }

            if (d.initDeclaratorList != nullptr)
                for (const auto& single : d.initDeclaratorList->declarations)
                    names.insert (single.name);
        };

        for (auto& external : ast.declarations)
        {
            if (auto* d = std::get_if<Declaration> (&external))
            {
                addDeclaration (*d);
            }
            else if (auto* fd = std::get_if<FunctionDefinition> (&external))
            {
                names.insert (fd->prototype.name);
                userFunctions.insert (fd->prototype.name);

                for (const auto& param : fd->prototype.parameters)
                    names.insert (param.name);

                if (fd->body != nullptr)
                {
                    forEachStatement (*fd->body, [&] (Statement& s)
                    {
                        if (s.is<StmtDeclaration>())
                            addDeclaration (s.as<StmtDeclaration>().declaration);
                    });
                }
            }
        }

        names.erase ("");

        for (auto it = names.begin(); it != names.end();)
            it = isBuiltinVariableName (*it) ? names.erase (it) : std::next (it);

        return names;
    }

    void sanitizeNames (TranslationUnit& ast)
    {
        auto declared = collectDeclaredNames (ast);

        for (const auto& name : declared)
            context.names.reserve (name);

        for (const auto& name : reservedWgslNames())
            context.names.reserve (name);

        // The requested entry point name belongs to the wrapper, not to user code
        context.names.reserve (options.entryPointName);

        std::map<std::string, std::string> renames;

        for (const auto& name : declared)
        {
            const bool clashesWithEntryPoint = name == options.entryPointName && name != "main";

            if (! needsRename (name) && ! clashesWithEntryPoint)
                continue;

            const auto base = name[0] == '_' ? "u" + name : name + "_";
            renames[name] = context.names.allocate (base);
        }

        // Only calls to user functions and struct constructors are renamed: GLSL builtins keep their names
        // even when a variable shares one (float step = ...; step (a, b))
        std::set<std::string> renamableCallees = userFunctions;
        forEachTypeSpecifier (ast, [&renamableCallees] (TypeSpecifier& type)
        {
            if (type.kind == TypeKind::namedStruct)
                renamableCallees.insert (type.structName);
        });

        for (const auto& [from, to] : renames)
            if (userFunctions.erase (from) > 0)
                userFunctions.insert (to);

        if (renames.empty())
            return;

        const auto renamed = [&renames] (std::string& name)
        {
            if (auto found = renames.find (name); found != renames.end())
                name = found->second;
        };

        forEachTypeSpecifier (ast, [&renamed] (TypeSpecifier& type)
        {
            if (type.kind == TypeKind::namedStruct)
                renamed (type.structName);
        });

        const auto renameDeclaration = [&renamed] (Declaration& d)
        {
            if (d.structSpecifier != nullptr)
            {
                renamed (d.structSpecifier->name);

                for (auto& field : d.structSpecifier->fields)
                    renamed (field.name);
            }

            if (d.initDeclaratorList != nullptr)
                for (auto& single : d.initDeclaratorList->declarations)
                    renamed (single.name);

            for (auto& name : d.qualifiedNames)
                renamed (name);
        };

        for (auto& external : ast.declarations)
        {
            if (auto* d = std::get_if<Declaration> (&external))
            {
                renameDeclaration (*d);
            }
            else if (auto* fd = std::get_if<FunctionDefinition> (&external))
            {
                renamed (fd->prototype.name);

                for (auto& param : fd->prototype.parameters)
                    renamed (param.name);

                if (fd->body != nullptr)
                {
                    forEachStatement (*fd->body, [&] (Statement& s)
                    {
                        if (s.is<StmtDeclaration>())
                            renameDeclaration (s.as<StmtDeclaration>().declaration);
                    });
                }
            }
        }

        std::set<const Expr*> methodCallees;

        forEachExpression (ast, [&] (Expr& e)
        {
            if (e.is<ExprDot>())
            {
                // x.length() is a method, not a member
                if (methodCallees.count (&e) == 0)
                    renamed (e.as<ExprDot>().member);
            }
            else if (e.is<ExprFunCall>())
            {
                auto& callee = e.as<ExprFunCall>().callee;

                if (callee != nullptr && callee->is<ExprDot>())
                    methodCallees.insert (callee.get());

                if (callee != nullptr && callee->is<ExprVariable>())
                {
                    auto& name = callee->as<ExprVariable>().name;
                    if (renames.count (name) > 0 && renamableCallees.count (name) > 0)
                        name = renames[name];

                    // Mark as visited so the variable rename below doesn't apply twice
                    callee->type = TypeSpecifier::make ({}, TypeKind::voidType);
                }
            }
            else if (e.is<ExprVariable>() && ! (e.type.has_value() && e.type->kind == TypeKind::voidType))
            {
                renamed (e.as<ExprVariable>().name);
            }
        });

        // Drop the visited marks again
        forEachExpression (ast, [] (Expr& e)
        {
            if (e.is<ExprVariable>())
                e.type.reset();
        });
    }

    //==========================================================================
    // Declarations
    //==========================================================================

    void nameUnnamedParameters (TranslationUnit& ast)
    {
        for (auto& external : ast.declarations)
            if (auto* fd = std::get_if<FunctionDefinition> (&external))
                for (auto& param : fd->prototype.parameters)
                    if (param.name.empty())
                        param.name = context.names.allocate ("_param");
    }

    /** WGSL has no overloading: every overload but the first gets a unique name, resolved at call sites by the type legalizer. */
    void renameOverloads (TranslationUnit& ast)
    {
        std::map<std::string, int> seen;

        for (auto& external : ast.declarations)
        {
            auto* fd = std::get_if<FunctionDefinition> (&external);
            if (fd == nullptr)
                continue;

            auto& proto = fd->prototype;
            if (seen[proto.name]++ == 0)
                continue;

            if (proto.name == "main")
                throw LoweringError (proto.loc, "main() cannot be overloaded");

            proto.originalName = proto.name;
            proto.name = context.names.allocate (proto.name);
        }

        // The first overload of a name also needs its GLSL name for resolution
        for (auto& external : ast.declarations)
            if (auto* fd = std::get_if<FunctionDefinition> (&external); fd != nullptr && seen[fd->prototype.name] > 1)
                fd->prototype.originalName = fd->prototype.name;
    }

    /** Names every uniform/buffer block and flattens stage IO blocks into plain in/out variables. */
    void normalizeBlocks (TranslationUnit& ast)
    {
        std::map<std::string, std::string> memberToInstance;
        std::map<std::string, std::map<std::string, std::string>> ioInstances; // instance -> member -> flattened name

        std::vector<ExternalDeclaration> output;
        output.reserve (ast.declarations.size());

        for (auto& external : ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);

            if (d == nullptr || ! isBlock (*d))
            {
                output.push_back (std::move (external));
                continue;
            }

            if (d->initDeclaratorList != nullptr)
            {
                const auto& single = d->initDeclaratorList->declarations.front();
                if (! single.arraySpecifiers.empty() || d->initDeclaratorList->declarations.size() > 1)
                    throw LoweringError (d->loc, "Arrays of interface blocks are not supported");
            }

            if (isResourceBlock (*d))
            {
                auto& block = *d->structSpecifier;

                if (block.name.empty())
                    block.name = context.names.allocate ("Block");

                if (d->initDeclaratorList == nullptr)
                {
                    // WGSL reserves identifiers starting with two underscores
                    const auto instance = context.names.allocate ((block.name[0] == '_' ? "v" : "_") + block.name);

                    for (const auto& field : block.fields)
                        memberToInstance[field.name] = instance;

                    auto list = std::make_unique<InitDeclaratorList>();
                    list->loc = d->loc;
                    list->qualifier = copyTypeQualifier (d->qualifier.get());
                    list->type = TypeSpecifier::makeNamed (d->loc, block.name);

                    SingleDeclaration single;
                    single.loc = d->loc;
                    single.name = instance;
                    list->declarations.push_back (std::move (single));
                    d->initDeclaratorList = std::move (list);
                }
                else
                {
                    d->initDeclaratorList->type.structName = block.name;
                }

                output.push_back (std::move (external));
                continue;
            }

            // Stage IO block: redeclaring gl_PerVertex only restates builtins
            if (d->structSpecifier->name == "gl_PerVertex" || d->structSpecifier->name == "gl_PerFragment")
                continue;

            flattenIOBlock (*d, output, ioInstances);
        }

        ast.declarations = std::move (output);

        if (memberToInstance.empty() && ioInstances.empty())
            return;

        std::set<std::string> globals;
        for (const auto& [member, instance] : memberToInstance)
            globals.insert (member);
        for (const auto& [instance, members] : ioInstances)
            globals.insert (instance);

        ReferenceRewriter* rewriterPtr = nullptr;
        ReferenceRewriter rewriter (globals, [&] (Expr& e) -> bool
        {
            if (e.is<ExprVariable>())
            {
                const auto name = e.as<ExprVariable>().name;

                if (auto found = memberToInstance.find (name); found != memberToInstance.end() && rewriterPtr->refersToGlobal (name))
                {
                    e = makeDot (e.loc, makeVariable (e.loc, found->second), name);
                    return true;
                }

                if (ioInstances.count (name) > 0 && rewriterPtr->refersToGlobal (name))
                    throw LoweringError (e.loc, "Stage IO block '" + name + "' can only be accessed member by member");
            }
            else if (e.is<ExprDot>())
            {
                auto& dot = e.as<ExprDot>();

                if (dot.base != nullptr && dot.base->is<ExprVariable>())
                {
                    const auto& instance = dot.base->as<ExprVariable>().name;

                    if (auto found = ioInstances.find (instance); found != ioInstances.end() && rewriterPtr->refersToGlobal (instance))
                    {
                        auto member = found->second.find (dot.member);
                        if (member == found->second.end())
                            throw LoweringError (e.loc, "Unknown member '" + dot.member + "' of block '" + instance + "'");

                        e = makeVariable (e.loc, member->second);
                        return true;
                    }
                }
            }

            return false;
        });

        rewriterPtr = &rewriter;
        rewriter.run (ast);
    }

    void flattenIOBlock (Declaration& d,
                         std::vector<ExternalDeclaration>& output,
                         std::map<std::string, std::map<std::string, std::string>>& ioInstances)
    {
        const bool isInput = d.qualifier->hasStorage (StorageQualifier::in);
        const std::string instance = d.initDeclaratorList != nullptr ? d.initDeclaratorList->declarations.front().name : "";

        auto nextLocation = layoutValue (d.qualifier.get(), LayoutQualifierId::location);

        for (auto& field : d.structSpecifier->fields)
        {
            auto qualifier = std::make_unique<TypeQualifier>();
            qualifier->loc = field.loc;
            qualifier->storage.push_back (isInput ? StorageQualifier::in : StorageQualifier::out);
            qualifier->interpolation = d.qualifier->interpolation;

            for (const auto storage : d.qualifier->storage)
                if (storage == StorageQualifier::centroid || storage == StorageQualifier::sample)
                    qualifier->storage.push_back (storage);

            if (field.qualifier != nullptr)
            {
                if (! field.qualifier->interpolation.empty())
                    qualifier->interpolation = field.qualifier->interpolation;

                for (const auto storage : field.qualifier->storage)
                    if (storage == StorageQualifier::centroid || storage == StorageQualifier::sample)
                        qualifier->storage.push_back (storage);

                qualifier->invariant = field.qualifier->invariant;

                if (auto memberLocation = layoutValue (field.qualifier.get(), LayoutQualifierId::location))
                    nextLocation = memberLocation;
            }

            if (nextLocation.has_value())
            {
                LayoutQualifierEntry entry;
                entry.loc = field.loc;
                entry.id = LayoutQualifierId::location;
                entry.name = "location";
                entry.value = boxed (makeIntLiteral (field.loc, *nextLocation));

                qualifier->layout = std::make_unique<LayoutQualifier>();
                qualifier->layout->entries.push_back (std::move (entry));

                nextLocation = *nextLocation + locationCount (field.type, field.loc);
            }

            const auto flatName = instance.empty() ? field.name : context.names.allocate (instance + "_" + field.name);

            if (! instance.empty())
                ioInstances[instance][field.name] = flatName;

            auto list = std::make_unique<InitDeclaratorList>();
            list->loc = field.loc;
            list->qualifier = std::move (qualifier);
            list->type = field.type;
            list->type.arraySpecifiers.clear();

            SingleDeclaration single;
            single.loc = field.loc;
            single.name = flatName;
            single.arraySpecifiers = field.type.arraySpecifiers;
            list->declarations.push_back (std::move (single));

            Declaration member;
            member.loc = field.loc;
            member.initDeclaratorList = std::move (list);
            output.push_back (std::move (member));
        }
    }

    /** Number of consecutive locations a stage IO variable of this type occupies. */
    static int64_t locationCount (const TypeSpecifier& type, const SourceLocation& loc)
    {
        int64_t count = std::max (1, matrixShape (type.kind).first);

        for (const auto& array : type.arraySpecifiers)
        {
            const auto size = array.sizeExpr != nullptr ? evaluateIntConstant (*array.sizeExpr) : std::nullopt;
            if (! size.has_value() || *size <= 0)
                throw LoweringError (loc, "Stage IO arrays need a constant size");

            count *= *size;
        }

        return count;
    }

    /** WGSL has no local struct declarations: they move to module scope, just before their function. */
    void hoistLocalStructs (TranslationUnit& ast)
    {
        std::set<std::string> globalStructs;

        for (auto& external : ast.declarations)
            if (auto* d = std::get_if<Declaration> (&external); d != nullptr && d->structSpecifier != nullptr && ! isBlock (*d))
                globalStructs.insert (d->structSpecifier->name);

        std::vector<ExternalDeclaration> output;

        for (auto& external : ast.declarations)
        {
            if (auto* fd = std::get_if<FunctionDefinition> (&external); fd != nullptr && fd->body != nullptr)
            {
                forEachStatement (*fd->body, [&] (Statement& s)
                {
                    if (! s.is<StmtDeclaration>())
                        return;

                    auto& decl = s.as<StmtDeclaration>().declaration;
                    if (decl.structSpecifier == nullptr)
                        return;

                    if (! globalStructs.insert (decl.structSpecifier->name).second)
                        throw LoweringError (decl.loc, "Struct '" + decl.structSpecifier->name + "' is declared more than once; WGSL needs unique struct names");

                    Declaration hoisted;
                    hoisted.loc = decl.loc;
                    hoisted.structSpecifier = std::move (decl.structSpecifier);
                    output.push_back (std::move (hoisted));

                    if (decl.initDeclaratorList == nullptr)
                        s = Statement::makeEmpty (s.loc);
                });
            }

            output.push_back (std::move (external));
        }

        ast.declarations = std::move (output);
    }

    //==========================================================================
    // Diagnostics: everything WGSL can't express fails here with a location
    //==========================================================================

    void checkType (const TypeSpecifier& type, const SourceLocation& loc, bool allowOpaque, bool allowUnsized)
    {
        const auto kind = type.kind;
        const auto name = glslTypeName (kind);

        if (isDoubleType (kind))
            throw LoweringError (loc, "Double precision type '" + name + "' is not supported in WGSL");

        if (kind == TypeKind::atomicUint)
            throw LoweringError (loc, "Atomic counters (atomic_uint) are not supported in WGSL");

        if (kind == TypeKind::subpassInput || kind == TypeKind::subpassInputMS)
            throw LoweringError (loc, "Subpass inputs are not supported in WGSL");

        if (isOpaqueType (kind))
        {
            if (! allowOpaque)
                throw LoweringError (loc, "Variables of opaque type '" + name + "' must be uniforms or function parameters");

            if (! type.arraySpecifiers.empty())
                throw LoweringError (loc, "Arrays of '" + name + "' are not supported in WGSL");

            if ((isSamplerType (kind) || isSeparateTextureType (kind)) && wgslTextureTypeName (kind, false).empty())
                throw LoweringError (loc, "'" + name + "' has no WGSL equivalent");

            if (isImageType (kind))
            {
                const auto shape = textureShape (kind);
                const bool supported = ! shape.multisampled
                                    && (shape.dim == TextureShape::Dim::d1 || shape.dim == TextureShape::Dim::d2 || shape.dim == TextureShape::Dim::d3)
                                    && ! (shape.arrayed && shape.dim != TextureShape::Dim::d2);

                if (! supported)
                    throw LoweringError (loc, "Storage image type '" + name + "' has no WGSL equivalent");
            }
        }

        for (const auto& array : type.arraySpecifiers)
        {
            if (array.isUnsized && ! allowUnsized)
                throw LoweringError (loc, "Unsized arrays are only supported as the last member of a buffer block");
        }
    }

    void checkLayout (const TypeQualifier* qualifier)
    {
        if (qualifier == nullptr || qualifier->layout == nullptr)
            return;

        for (const auto& entry : qualifier->layout->entries)
        {
            switch (entry.id)
            {
                case LayoutQualifierId::location:
                case LayoutQualifierId::binding:
                case LayoutQualifierId::descriptorSet:
                case LayoutQualifierId::localSizeX:
                case LayoutQualifierId::localSizeY:
                case LayoutQualifierId::localSizeZ:
                case LayoutQualifierId::std140:
                case LayoutQualifierId::std430:
                case LayoutQualifierId::columnMajor:
                case LayoutQualifierId::imageFormat:
                case LayoutQualifierId::originUpperLeft:
                    break;

                case LayoutQualifierId::depthGreater:
                case LayoutQualifierId::depthLess:
                case LayoutQualifierId::depthUnchanged:
                case LayoutQualifierId::depthAny:
                    break; // optimization hints without a WGSL equivalent

                case LayoutQualifierId::earlyFragmentTests:
                    warn (entry.loc, "layout(early_fragment_tests) has no WGSL equivalent and is ignored");
                    break;

                case LayoutQualifierId::pushConstant:
                    throw LoweringError (entry.loc, "Push constants are not supported for WGSL; use a uniform block");

                case LayoutQualifierId::rowMajor:
                    throw LoweringError (entry.loc, "row_major matrices are not supported for WGSL");

                case LayoutQualifierId::component:
                    throw LoweringError (entry.loc, "layout(component) is not supported for WGSL");

                case LayoutQualifierId::pixelCenterInteger:
                    throw LoweringError (entry.loc, "layout(pixel_center_integer) is not supported for WGSL");

                case LayoutQualifierId::sharedLayout:
                case LayoutQualifierId::packedLayout:
                case LayoutQualifierId::scalarLayout:
                case LayoutQualifierId::bufferReference:
                    throw LoweringError (entry.loc, "layout(" + entry.name + ") is not supported for WGSL; use std140 or std430");

                case LayoutQualifierId::inputAttachmentIndex:
                    throw LoweringError (entry.loc, "Subpass inputs are not supported in WGSL");

                case LayoutQualifierId::xfbBuffer:
                case LayoutQualifierId::xfbStride:
                case LayoutQualifierId::xfbOffset:
                    throw LoweringError (entry.loc, "Transform feedback is not supported in WGSL");

                case LayoutQualifierId::offset:
                case LayoutQualifierId::align:
                    break; // reproduced by WgslHostLayout

                case LayoutQualifierId::index:
                case LayoutQualifierId::constantId:
                case LayoutQualifierId::localSizeXId:
                case LayoutQualifierId::localSizeYId:
                case LayoutQualifierId::localSizeZId:
                    break;

                default:
                    throw LoweringError (entry.loc, "layout(" + entry.name + ") is only valid in geometry or tessellation shaders");
            }
        }
    }

    void checkDeclaration (Declaration& d, bool isGlobal)
    {
        checkLayout (d.qualifier.get());

        if (d.qualifier != nullptr && ! d.qualifiedNames.empty())
        {
            for (const auto& name : d.qualifiedNames)
            {
                if (d.qualifier->invariant && name == "gl_Position")
                    invariantPosition = true;
                else if (d.qualifier->invariant)
                    warn (d.loc, "invariant has no effect on '" + name + "' in WGSL");
            }
        }

        if (d.structSpecifier != nullptr)
        {
            const bool isBuffer = d.qualifier != nullptr && d.qualifier->hasStorage (StorageQualifier::buffer);
            const auto& fields = d.structSpecifier->fields;

            for (size_t i = 0; i < fields.size(); ++i)
            {
                checkLayout (fields[i].qualifier.get());
                checkType (fields[i].type, fields[i].loc, false, isBuffer && i + 1 == fields.size());
            }
        }

        if (d.initDeclaratorList == nullptr)
            return;

        auto& list = *d.initDeclaratorList;
        checkLayout (list.qualifier.get());

        const auto* q = list.qualifier.get();
        const bool isUniform = q != nullptr && q->hasStorage (StorageQualifier::uniform);
        const bool isBuffer = q != nullptr && q->hasStorage (StorageQualifier::buffer);

        if (q != nullptr && q->hasStorage (StorageQualifier::shared) && options.stage != ShaderStage::compute)
            throw LoweringError (list.loc, "shared variables are only allowed in compute shaders");

        if (isBuffer && options.stage == ShaderStage::vertex && ! q->hasMemory (MemoryQualifier::readonlyQual))
            throw LoweringError (list.loc, "WGSL vertex shaders can't write storage buffers; declare the buffer readonly");

        if (hasLayout (q, LayoutQualifierId::constantId))
        {
            const bool isScalar = list.type.arraySpecifiers.empty() && componentCount (list.type.kind) == 1;
            if (! q->hasStorage (StorageQualifier::constQual) || ! isScalar || ! isGlobal)
                throw LoweringError (list.loc, "layout(constant_id) needs a global scalar constant");
        }

        if (q != nullptr && q->invariant)
            warn (list.loc, "invariant only applies to gl_Position in WGSL and is ignored here");

        if (q != nullptr && q->precise)
            warn (list.loc, "precise has no WGSL equivalent and is ignored");

        for (auto& single : list.declarations)
        {
            const auto type = declaratorType (list.type, single.arraySpecifiers);
            const bool sizedByInitializer = single.initializer != nullptr;
            checkType (type, list.type.loc, isGlobal && (isUniform || type.kind == TypeKind::namedStruct), sizedByInitializer);

            if (isImageType (type.kind) && ! hasLayout (q, LayoutQualifierId::imageFormat))
                throw LoweringError (single.loc, "Storage image '" + single.name + "' needs a layout format qualifier for WGSL");
        }
    }

    void runDiagnostics (TranslationUnit& ast)
    {
        for (auto& external : ast.declarations)
        {
            if (auto* d = std::get_if<Declaration> (&external))
            {
                checkDeclaration (*d, true);
                continue;
            }

            auto* fd = std::get_if<FunctionDefinition> (&external);
            if (fd == nullptr)
                continue;

            checkType (fd->prototype.returnType, fd->prototype.loc, false, false);

            for (auto& param : fd->prototype.parameters)
            {
                const auto type = declaratorType (param.type, param.arraySpecifiers);
                checkType (type, param.loc, true, false);

                if (isImageType (type.kind))
                    throw LoweringError (param.loc, "Storage images can't be passed to functions in WGSL");
            }

            if (fd->body != nullptr)
            {
                forEachStatement (*fd->body, [this] (Statement& s)
                {
                    if (s.is<StmtDeclaration>())
                        checkDeclaration (s.as<StmtDeclaration>().declaration, false);

                    if (s.is<StmtJump>() && s.as<StmtJump>().kind == JumpKind::discardJump && options.stage != ShaderStage::fragment)
                        throw LoweringError (s.loc, "discard is only allowed in fragment shaders");
                });
            }
        }

        forEachExpression (ast, [this] (Expr& e)
        {
            if (e.is<ExprFloatConst>() && e.as<ExprFloatConst>().isDouble)
                throw LoweringError (e.loc, "Double precision literals are not supported in WGSL");

            if (e.is<ExprTypeConstructor>())
                checkType (e.as<ExprTypeConstructor>().type, e.loc, true, true);

            if (e.is<ExprVariable>())
                checkBuiltinVariable (e.as<ExprVariable>().name, e.loc);
        });
    }

    void checkBuiltinVariable (const std::string& name, const SourceLocation& loc)
    {
        if (! isBuiltinVariableName (name))
            return;

        static const std::set<std::string> vertexBuiltins = { "gl_Position", "gl_PointSize", "gl_VertexIndex", "gl_VertexID", "gl_InstanceIndex", "gl_InstanceID" };
        static const std::set<std::string> fragmentBuiltins = { "gl_FragCoord", "gl_FrontFacing", "gl_FragDepth", "gl_SampleID", "gl_SampleMaskIn", "gl_SampleMask" };
        static const std::set<std::string> computeBuiltins = { "gl_GlobalInvocationID", "gl_LocalInvocationID", "gl_LocalInvocationIndex", "gl_WorkGroupID", "gl_NumWorkGroups", "gl_WorkGroupSize" };

        const auto& supported = options.stage == ShaderStage::vertex     ? vertexBuiltins
                              : options.stage == ShaderStage::fragment ? fragmentBuiltins
                                                                       : computeBuiltins;

        if (supported.count (name) == 0)
            throw LoweringError (loc, "Builtin '" + name + "' has no WGSL equivalent in this stage");
    }

    //==========================================================================
    // Resources
    //==========================================================================

    /** A separate texture combined with a shadow sampler type is a depth texture in WGSL. */
    void findDepthTextures (TranslationUnit& ast)
    {
        forEachExpression (ast, [this] (Expr& e)
        {
            if (! e.is<ExprTypeConstructor>())
                return;

            const auto& ctor = e.as<ExprTypeConstructor>();
            if (textureShape (ctor.type.kind).shadow && ! ctor.args.empty() && ctor.args[0].is<ExprVariable>())
                context.depthTextures.insert (ctor.args[0].as<ExprVariable>().name);
        });
    }

    void setupEntryPoint (LoweredProgram& result)
    {
        auto& ep = result.entryPoint;
        ep.isVertex = options.stage == ShaderStage::vertex;
        ep.isFragment = options.stage == ShaderStage::fragment;
        ep.isCompute = options.stage == ShaderStage::compute;
        ep.wgslEntryPoint = options.entryPointName;
        ep.innerFunction = context.names.allocate ("main_inner");
        context.innerFunction = ep.innerFunction;

        bool foundMain = false;

        for (auto& external : result.ast.declarations)
        {
            if (auto* fd = std::get_if<FunctionDefinition> (&external); fd != nullptr && fd->prototype.name == "main")
            {
                if (! fd->prototype.parameters.empty() || fd->prototype.returnType.kind != TypeKind::voidType)
                    throw LoweringError (fd->loc, "main() must take no parameters and return void");

                fd->prototype.name = ep.innerFunction;
                foundMain = true;
            }
        }

        if (! foundMain)
            throw LoweringError ({ 0, 0 }, "Missing main() function");
    }

    /** Globals referenced by main and the functions it calls, the resources glslang considers live. */
    std::set<std::string> collectLiveNames (TranslationUnit& ast)
    {
        std::map<std::string, FunctionDefinition*> functions;
        for (auto& external : ast.declarations)
            if (auto* fd = std::get_if<FunctionDefinition> (&external))
                functions[fd->prototype.name] = fd;

        std::set<std::string> live;
        std::set<std::string> visited;
        std::vector<std::string> pending { context.innerFunction };

        while (! pending.empty())
        {
            const auto name = pending.back();
            pending.pop_back();

            auto found = functions.find (name);
            if (found == functions.end() || ! visited.insert (name).second || found->second->body == nullptr)
                continue;

            forEachStatement (*found->second->body, [&] (Statement& s)
            {
                const auto collect = [&] (Expr& root)
                {
                    walkExpr (root, [&] (Expr& e)
                    {
                        if (e.is<ExprVariable>())
                        {
                            live.insert (e.as<ExprVariable>().name);

                            if (functions.count (e.as<ExprVariable>().name) > 0)
                                pending.push_back (e.as<ExprVariable>().name);
                        }
                    });
                };

                if (s.is<StmtDeclaration>())
                    walkDeclarationExprs (s.as<StmtDeclaration>().declaration, collect);
                else
                    forEachOwnExpr (s, collect);
            });
        }

        return live;
    }

    void assignBindings (LoweredProgram& result)
    {
        // glslang only maps the bindings of live resources: unbound ones main never reaches are dropped
        const auto live = collectLiveNames (result.ast);
        auto& decls = result.ast.declarations;

        for (auto& external : decls)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->initDeclaratorList == nullptr || d->structSpecifier != nullptr)
                continue;

            const auto* q = d->initDeclaratorList->qualifier.get();
            const bool isResource = q != nullptr && (q->hasStorage (StorageQualifier::uniform) || q->hasStorage (StorageQualifier::buffer));

            if (! isResource || hasLayout (q, LayoutQualifierId::binding))
                continue;

            auto& singles = d->initDeclaratorList->declarations;
            singles.erase (std::remove_if (singles.begin(), singles.end(), [&live] (const SingleDeclaration& single)
                           {
                               return live.count (single.name) == 0;
                           }),
                           singles.end());
        }

        decls.erase (std::remove_if (decls.begin(), decls.end(), [] (ExternalDeclaration& external)
                     {
                         auto* d = std::get_if<Declaration> (&external);
                         return d != nullptr && d->initDeclaratorList != nullptr && d->initDeclaratorList->declarations.empty();
                     }),
                     decls.end());

        struct Pending
        {
            std::string name;
            SourceLocation loc;
            uint32_t group = 0;
            std::optional<uint32_t> binding;
            TypeKind kind = TypeKind::voidType;
            bool isLive = false;
        };

        std::vector<Pending> pending;

        for (auto& external : result.ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->initDeclaratorList == nullptr)
                continue;

            auto& list = *d->initDeclaratorList;
            const auto* q = list.qualifier.get();

            if (q == nullptr || ! (q->hasStorage (StorageQualifier::uniform) || q->hasStorage (StorageQualifier::buffer)))
                continue;

            const auto set = layoutValue (q, LayoutQualifierId::descriptorSet);
            const auto binding = layoutValue (q, LayoutQualifierId::binding);

            for (auto& single : list.declarations)
            {
                Pending p;
                p.name = single.name;
                p.loc = single.loc;
                p.group = set.has_value() ? static_cast<uint32_t> (*set) : options.defaultGroup;
                p.kind = list.type.kind;
                p.isLive = live.count (single.name) > 0;

                if (binding.has_value())
                    p.binding = static_cast<uint32_t> (*binding);

                pending.push_back (p);
            }
        }

        // Explicit bindings first, then glslang's automatic mapping: next free slot in declaration order
        std::map<std::pair<uint32_t, uint32_t>, std::string> taken;

        for (const auto& p : pending)
        {
            if (! p.binding.has_value())
                continue;

            if (auto [it, inserted] = taken.emplace (std::make_pair (p.group, *p.binding), p.name); ! inserted)
                throw LoweringError (p.loc, "'" + p.name + "' and '" + it->second + "' share @group(" + std::to_string (p.group) + ") @binding(" + std::to_string (*p.binding) + ")");
        }

        for (auto& p : pending)
        {
            if (p.binding.has_value())
                continue;

            uint32_t next = 0;
            while (taken.count ({ p.group, next }) > 0)
                ++next;

            p.binding = next;
            taken.emplace (std::make_pair (p.group, next), p.name);
        }

        // Like reflection, the companion rule only sees live resources; an unused resource may share
        // a companion binding, which WGSL allows because it is never statically used
        std::vector<WgslBindingSlot> slots;
        std::vector<size_t> liveIndices;

        for (size_t i = 0; i < pending.size(); ++i)
        {
            if (pending[i].isLive)
            {
                slots.push_back ({ pending[i].group, *pending[i].binding, isSamplerType (pending[i].kind) });
                liveIndices.push_back (i);
            }
        }

        const auto liveCompanions = assignWgslCompanionSamplerBindings (slots);
        std::vector<uint32_t> companions (pending.size(), ~0u);

        for (size_t i = 0; i < liveIndices.size(); ++i)
            companions[liveIndices[i]] = liveCompanions[i];

        // A dead combined sampler still needs a sampler declaration: give it the next binding after everything else
        std::map<uint32_t, uint32_t> nextDeadBinding;
        for (size_t i = 0; i < pending.size(); ++i)
            nextDeadBinding[pending[i].group] = std::max ({ nextDeadBinding[pending[i].group], *pending[i].binding + 1, companions[i] != ~0u ? companions[i] + 1 : 0u });

        for (size_t i = 0; i < pending.size(); ++i)
            if (! pending[i].isLive && isSamplerType (pending[i].kind))
                companions[i] = nextDeadBinding[pending[i].group]++;

        for (size_t i = 0; i < pending.size(); ++i)
        {
            LoweredProgram::ResourceAssignment ra;
            ra.name = pending[i].name;
            ra.group = pending[i].group;
            ra.binding = *pending[i].binding;
            ra.isSampler = isSeparateSamplerType (pending[i].kind);

            if (companions[i] != ~0u)
            {
                ra.samplerBinding = companions[i];
                ra.samplerName = context.names.allocate (ra.name + "_sampler");
                context.samplerCompanions[ra.name] = ra.samplerName;
            }

            result.resources.push_back (ra);
        }
    }

    void extractWorkgroupSize (LoweredProgram& result)
    {
        auto& ep = result.entryPoint;

        if (! ep.isCompute)
            return;

        ep.workgroupSizeX = options.defaultWorkgroupSize[0];
        ep.workgroupSizeY = options.defaultWorkgroupSize[1];
        ep.workgroupSizeZ = options.defaultWorkgroupSize[2];

        for (auto& external : result.ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->qualifier == nullptr || d->initDeclaratorList != nullptr || d->structSpecifier != nullptr)
                continue;

            if (auto x = layoutValue (d->qualifier.get(), LayoutQualifierId::localSizeX))
                ep.workgroupSizeX = static_cast<uint32_t> (*x);
            if (auto y = layoutValue (d->qualifier.get(), LayoutQualifierId::localSizeY))
                ep.workgroupSizeY = static_cast<uint32_t> (*y);
            if (auto z = layoutValue (d->qualifier.get(), LayoutQualifierId::localSizeZ))
                ep.workgroupSizeZ = static_cast<uint32_t> (*z);
        }

        if (ep.workgroupSizeX == 0 || ep.workgroupSizeY == 0 || ep.workgroupSizeZ == 0)
            throw LoweringError ({ 0, 0 }, "Workgroup size must be at least 1 in every dimension");

        // local_size_*_id makes a dimension a specialization constant: a WGSL override defaulting to the size
        static const LayoutQualifierId ids[] = { LayoutQualifierId::localSizeXId, LayoutQualifierId::localSizeYId, LayoutQualifierId::localSizeZId };
        static const char* names[] = { "_workgroup_size_x", "_workgroup_size_y", "_workgroup_size_z" };
        const uint32_t sizes[] = { ep.workgroupSizeX, ep.workgroupSizeY, ep.workgroupSizeZ };
        std::vector<ExternalDeclaration> overrides;

        for (auto& external : result.ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->qualifier == nullptr || d->initDeclaratorList != nullptr || d->structSpecifier != nullptr)
                continue;

            for (int i = 0; i < 3; ++i)
            {
                const auto id = layoutValue (d->qualifier.get(), ids[i]);
                if (! id.has_value())
                    continue;

                ep.workgroupSizeOverrides[static_cast<size_t> (i)] = context.names.allocate (names[i]);

                auto list = std::make_unique<InitDeclaratorList>();
                list->loc = d->loc;
                list->type = makeType (TypeKind::uintType);
                list->qualifier = std::make_unique<TypeQualifier>();
                list->qualifier->storage.push_back (StorageQualifier::constQual);
                list->qualifier->layout = std::make_unique<LayoutQualifier>();
                list->qualifier->layout->entries.push_back ({ d->loc, LayoutQualifierId::constantId, "constant_id", boxed (makeIntLiteral (d->loc, *id)) });

                SingleDeclaration single;
                single.loc = d->loc;
                single.name = ep.workgroupSizeOverrides[static_cast<size_t> (i)];
                single.initializer = std::make_unique<Initializer>();
                single.initializer->expr = boxed (makeUIntLiteral (d->loc, sizes[i]));
                list->declarations.push_back (std::move (single));

                Declaration decl;
                decl.loc = d->loc;
                decl.initDeclaratorList = std::move (list);
                overrides.push_back (std::move (decl));
            }
        }

        result.ast.declarations.insert (result.ast.declarations.begin(),
                                        std::make_move_iterator (overrides.begin()),
                                        std::make_move_iterator (overrides.end()));
    }

    //==========================================================================
    // Stage IO
    //==========================================================================

    struct IOVariable
    {
        std::string name;
        TypeSpecifier type; // full type, including declarator arrays
        const TypeQualifier* qualifier = nullptr;
        SourceLocation loc;
        std::optional<int64_t> location;
        int64_t index = 0; // dual-source blending index
    };

    static std::string interpolationFor (const IOVariable& v)
    {
        const auto* q = v.qualifier;
        const bool isInteger = scalarKindOf (v.type.kind) == TypeKind::intType || scalarKindOf (v.type.kind) == TypeKind::uintType;
        const bool flat = isInteger || (q != nullptr && std::find (q->interpolation.begin(), q->interpolation.end(), InterpolationQualifier::flat) != q->interpolation.end());

        if (flat)
            return "flat";

        const bool linear = q != nullptr && std::find (q->interpolation.begin(), q->interpolation.end(), InterpolationQualifier::noPerspective) != q->interpolation.end();
        const bool centroid = q != nullptr && q->hasStorage (StorageQualifier::centroid);
        const bool sample = q != nullptr && q->hasStorage (StorageQualifier::sample);

        const std::string sampling = centroid ? ", centroid" : sample ? ", sample" : "";

        if (linear)
            return "linear" + sampling;

        return sampling.empty() ? "" : "perspective" + sampling;
    }

    void lowerStageIO (LoweredProgram& result)
    {
        auto& ep = result.entryPoint;
        const bool interpolatedInputs = ep.isFragment;
        const bool interpolatedOutputs = ep.isVertex;

        std::vector<IOVariable> inputs;
        std::vector<IOVariable> outputs;

        for (auto& external : result.ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->initDeclaratorList == nullptr || d->structSpecifier != nullptr)
                continue;

            auto& list = *d->initDeclaratorList;
            auto* q = list.qualifier.get();

            if (q == nullptr || ! (q->hasStorage (StorageQualifier::in) || q->hasStorage (StorageQualifier::out)))
                continue;

            const bool isInput = q->hasStorage (StorageQualifier::in);

            if (ep.isCompute)
                throw LoweringError (list.loc, "Compute shaders have no user stage inputs or outputs");

            for (auto& single : list.declarations)
            {
                if (isBuiltinVariableName (single.name))
                    continue; // redeclared builtin

                IOVariable v;
                v.name = single.name;
                v.type = declaratorType (list.type, single.arraySpecifiers);
                v.qualifier = q;
                v.loc = single.loc;
                v.location = layoutValue (q, LayoutQualifierId::location);
                v.index = layoutValue (q, LayoutQualifierId::index).value_or (0);

                if (v.index > 1 || (v.index > 0 && (! ep.isFragment || isInput)))
                    throw LoweringError (v.loc, "layout(index) is only valid as 0 or 1 on fragment outputs");

                if (v.type.kind == TypeKind::namedStruct || scalarKindOf (v.type.kind) == TypeKind::voidType
                    || scalarKindOf (v.type.kind) == TypeKind::boolType)
                    throw LoweringError (v.loc, "Stage IO variable '" + v.name + "' must be a numeric scalar, vector or matrix");

                (isInput ? inputs : outputs).push_back (std::move (v));
            }
        }

        // Drop redeclared builtins (e.g. out vec4 gl_Position;), declared below when used
        auto& decls = result.ast.declarations;
        decls.erase (std::remove_if (decls.begin(), decls.end(), [] (ExternalDeclaration& external)
                     {
                         auto* d = std::get_if<Declaration> (&external);
                         return d != nullptr && d->initDeclaratorList != nullptr && d->structSpecifier == nullptr
                             && ! d->initDeclaratorList->declarations.empty()
                             && isBuiltinVariableName (d->initDeclaratorList->declarations.front().name);
                     }),
                     decls.end());

        ep.inputStruct = context.names.allocate (ep.isVertex ? "VSInput" : ep.isFragment ? "FSInput" : "CSInput");
        ep.outputStruct = context.names.allocate (ep.isVertex ? "VSOutput" : "FSOutput");
        ep.inputParameter = context.names.allocate ("input");
        ep.outputVariable = context.names.allocate ("output");

        WgslNameAllocator inputFields;
        WgslNameAllocator outputFields;

        assignLocations (inputs);
        assignLocations (outputs);

        for (const auto& v : inputs)
            addUserIO (ep.inputs, ep.inputCopies, inputFields, v, true, interpolatedInputs, ep.inputParameter);

        for (const auto& v : outputs)
            addUserIO (ep.outputs, ep.outputCopies, outputFields, v, false, interpolatedOutputs, ep.outputVariable);

        // Dual-source blending: WGSL wants exactly the two sources at location 0, tagged @blend_src
        const bool dualSource = std::any_of (outputs.begin(), outputs.end(), [] (const IOVariable& v)
        {
            return v.index > 0;
        });

        if (dualSource)
        {
            if (ep.outputs.size() != 2 || ep.outputs[0].location != 0 || ep.outputs[1].location != 0)
                throw LoweringError (outputs.front().loc, "Dual-source blending in WGSL needs exactly two outputs at location 0 with index 0 and 1");

            for (size_t i = 0; i < outputs.size(); ++i)
                ep.outputs[i].blendSource = static_cast<int> (outputs[i].index);

            result.enables.push_back ("dual_source_blending");
        }

        // Stage IO variables become private globals the entry point copies from and to
        for (auto& external : result.ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->initDeclaratorList == nullptr || d->structSpecifier != nullptr)
                continue;

            auto* q = d->initDeclaratorList->qualifier.get();
            if (q != nullptr && (q->hasStorage (StorageQualifier::in) || q->hasStorage (StorageQualifier::out)))
                d->initDeclaratorList->qualifier.reset();
        }

        addBuiltinIO (result, inputFields, outputFields);
    }

    /** Explicit locations stay; the rest take the next free locations in declaration order, like glslang's auto-mapping. */
    static void assignLocations (std::vector<IOVariable>& variables)
    {
        std::set<int64_t> used;
        std::set<std::pair<int64_t, int64_t>> usedWithIndex;

        for (const auto& v : variables)
        {
            if (! v.location.has_value())
                continue;

            for (int64_t i = 0; i < locationCount (v.type, v.loc); ++i)
            {
                used.insert (*v.location + i);

                if (! usedWithIndex.insert ({ *v.location + i, v.index }).second)
                    throw LoweringError (v.loc, "Stage IO location " + std::to_string (*v.location + i) + " is used more than once");
            }
        }

        for (auto& v : variables)
        {
            if (v.location.has_value())
                continue;

            const auto count = locationCount (v.type, v.loc);
            int64_t start = 0;

            for (;; ++start)
            {
                bool free = true;
                for (int64_t i = 0; i < count && free; ++i)
                    free = used.count (start + i) == 0;

                if (free)
                    break;
            }

            for (int64_t i = 0; i < count; ++i)
                used.insert (start + i);

            v.location = start;
        }
    }

    void addUserIO (std::vector<LoweredProgram::InputOutputInfo>& fields,
                    std::vector<std::string>& copies,
                    WgslNameAllocator& fieldNames,
                    const IOVariable& v,
                    bool isInput,
                    bool interpolated,
                    const std::string& structVariable)
    {
        const auto interpolation = interpolated ? interpolationFor (v) : std::string();
        const auto [columns, rows] = matrixShape (v.type.kind);
        const bool isArray = ! v.type.arraySpecifiers.empty();

        if (isArray && (columns > 0 || v.type.arraySpecifiers.size() > 1))
            throw LoweringError (v.loc, "Stage IO arrays of matrices or arrays are not supported");

        const auto makeField = [&] (const std::string& fieldName, TypeKind kind, int64_t location)
        {
            LoweredProgram::InputOutputInfo io;
            io.name = v.name;
            io.fieldName = fieldName;
            io.wgslType = TypeSpecifier::make (v.loc, kind);
            io.location = static_cast<uint32_t> (location);
            io.interpolation = interpolation;
            fields.push_back (io);
        };

        if (columns == 0 && ! isArray)
        {
            const auto field = fieldNames.allocate (v.name);
            makeField (field, v.type.kind, *v.location);

            copies.push_back (isInput ? v.name + " = " + structVariable + "." + field
                                      : structVariable + "." + field + " = " + v.name);
            return;
        }

        const auto count = columns > 0 ? columns : locationCount (v.type, v.loc);
        const auto elementKind = columns > 0 ? vectorKind (TypeKind::floatType, rows) : v.type.kind;
        const auto wholeType = columns > 0 ? wgslTypeName (v.type.kind)
                                           : "array<" + wgslTypeName (v.type.kind) + ", " + std::to_string (count) + ">";

        std::string assembled = wholeType + "(";

        for (int64_t i = 0; i < count; ++i)
        {
            const auto field = fieldNames.allocate (v.name + "_" + std::to_string (i));
            makeField (field, elementKind, *v.location + i);

            if (isInput)
                assembled += (i > 0 ? ", " : "") + structVariable + "." + field;
            else
                copies.push_back (structVariable + "." + field + " = " + v.name + "[" + std::to_string (i) + "]");
        }

        if (isInput)
            copies.push_back (v.name + " = " + assembled + ")");
    }

    void addBuiltinIO (LoweredProgram& result, WgslNameAllocator& inputFields, WgslNameAllocator& outputFields)
    {
        auto& ep = result.entryPoint;

        std::set<std::string> used;
        std::map<std::string, bool> written;

        forEachExpression (result.ast, [&] (Expr& e)
        {
            if (e.is<ExprVariable>() && isBuiltinVariableName (e.as<ExprVariable>().name))
                used.insert (e.as<ExprVariable>().name);
        });

        const auto declarePrivate = [&] (const std::string& name, const TypeSpecifier& type)
        {
            auto list = std::make_unique<InitDeclaratorList>();
            list->type = type;

            SingleDeclaration single;
            single.name = name;
            list->declarations.push_back (std::move (single));

            Declaration d;
            d.initDeclaratorList = std::move (list);
            builtinDeclarations.push_back (std::move (d));
        };

        const auto input = [&] (const std::string& glslName, TypeKind glslType, const std::string& builtin, TypeKind wgslType, const std::string& conversion)
        {
            declarePrivate (glslName, makeType (glslType));

            LoweredProgram::InputOutputInfo io;
            io.name = glslName;
            io.fieldName = inputFields.allocate (builtin == "position" ? "frag_coord" : builtin);
            io.wgslType = makeType (wgslType);
            io.isBuiltin = true;
            io.builtinName = builtin;
            ep.inputs.push_back (io);

            const auto field = ep.inputParameter + "." + io.fieldName;
            ep.inputCopies.push_back (glslName + " = " + (conversion.empty() ? field : conversion + "(" + field + ")"));
        };

        const auto output = [&] (const std::string& glslName, TypeKind glslType, const std::string& builtin, TypeKind wgslType, const std::string& value)
        {
            declarePrivate (glslName, makeType (glslType));

            LoweredProgram::InputOutputInfo io;
            io.name = glslName;
            io.fieldName = outputFields.allocate (builtin);
            io.wgslType = makeType (wgslType);
            io.isBuiltin = true;
            io.builtinName = builtin;
            io.invariant = builtin == "position" && invariantPosition;
            ep.outputs.push_back (io);

            ep.outputCopies.push_back (ep.outputVariable + "." + io.fieldName + " = " + value);
        };

        if (ep.isVertex)
        {
            input ("gl_VertexIndex", TypeKind::intType, "vertex_index", TypeKind::uintType, "i32");
            input ("gl_InstanceIndex", TypeKind::intType, "instance_index", TypeKind::uintType, "i32");

            // Without a base vertex/instance, the GL spellings alias the Vulkan ones
            if (used.count ("gl_VertexID") > 0)
            {
                declarePrivate ("gl_VertexID", makeType (TypeKind::intType));
                ep.inputCopies.push_back ("gl_VertexID = gl_VertexIndex");
            }

            if (used.count ("gl_InstanceID") > 0)
            {
                declarePrivate ("gl_InstanceID", makeType (TypeKind::intType));
                ep.inputCopies.push_back ("gl_InstanceID = gl_InstanceIndex");
            }

            output ("gl_Position", TypeKind::vec4, "position", TypeKind::vec4, "gl_Position");

            if (used.count ("gl_PointSize") > 0)
                declarePrivate ("gl_PointSize", makeType (TypeKind::floatType));
        }
        else if (ep.isFragment)
        {
            input ("gl_FragCoord", TypeKind::vec4, "position", TypeKind::vec4, "");
            input ("gl_FrontFacing", TypeKind::boolType, "front_facing", TypeKind::boolType, "");

            if (used.count ("gl_SampleID") > 0)
                input ("gl_SampleID", TypeKind::intType, "sample_index", TypeKind::uintType, "i32");

            if (used.count ("gl_SampleMaskIn") > 0)
            {
                builtinDeclarationsMask ("gl_SampleMaskIn");
                LoweredProgram::InputOutputInfo io;
                io.name = "gl_SampleMaskIn";
                io.fieldName = inputFields.allocate ("sample_mask");
                io.wgslType = makeType (TypeKind::uintType);
                io.isBuiltin = true;
                io.builtinName = "sample_mask";
                ep.inputs.push_back (io);
                ep.inputCopies.push_back ("gl_SampleMaskIn = array<i32, 1>(i32(" + ep.inputParameter + "." + io.fieldName + "))");
            }

            if (used.count ("gl_FragDepth") > 0)
                output ("gl_FragDepth", TypeKind::floatType, "frag_depth", TypeKind::floatType, "gl_FragDepth");

            if (used.count ("gl_SampleMask") > 0)
            {
                builtinDeclarationsMask ("gl_SampleMask");
                LoweredProgram::InputOutputInfo io;
                io.name = "gl_SampleMask";
                io.fieldName = outputFields.allocate ("sample_mask");
                io.wgslType = makeType (TypeKind::uintType);
                io.isBuiltin = true;
                io.builtinName = "sample_mask";
                ep.outputs.push_back (io);
                ep.outputCopies.push_back (ep.outputVariable + "." + io.fieldName + " = u32(gl_SampleMask[0])");
            }
        }
        else
        {
            static const std::tuple<const char*, const char*, TypeKind> computeInputs[] = {
                { "gl_GlobalInvocationID", "global_invocation_id", TypeKind::uvec3 },
                { "gl_LocalInvocationID", "local_invocation_id", TypeKind::uvec3 },
                { "gl_LocalInvocationIndex", "local_invocation_index", TypeKind::uintType },
                { "gl_WorkGroupID", "workgroup_id", TypeKind::uvec3 },
                { "gl_NumWorkGroups", "num_workgroups", TypeKind::uvec3 }
            };

            for (const auto& [glslName, builtin, type] : computeInputs)
                if (used.count (glslName) > 0)
                    input (glslName, type, builtin, type, "");

            const bool hasOverrides = ! ep.workgroupSizeOverrides[0].empty() || ! ep.workgroupSizeOverrides[1].empty() || ! ep.workgroupSizeOverrides[2].empty();

            if (used.count ("gl_WorkGroupSize") > 0 && hasOverrides)
            {
                // A module-scope const can't depend on overrides: fill a private variable instead
                declarePrivate ("gl_WorkGroupSize", makeType (TypeKind::uvec3));

                const uint32_t sizes[] = { ep.workgroupSizeX, ep.workgroupSizeY, ep.workgroupSizeZ };
                std::string value = "gl_WorkGroupSize = vec3<u32>(";

                for (size_t i = 0; i < 3; ++i)
                    value += (i > 0 ? ", " : "") + (ep.workgroupSizeOverrides[i].empty() ? std::to_string (sizes[i]) + "u" : ep.workgroupSizeOverrides[i]);

                ep.inputCopies.push_back (value + ")");
            }
            else if (used.count ("gl_WorkGroupSize") > 0)
            {
                auto list = std::make_unique<InitDeclaratorList>();
                list->qualifier = std::make_unique<TypeQualifier>();
                list->qualifier->storage.push_back (StorageQualifier::constQual);
                list->type = makeType (TypeKind::uvec3);

                SingleDeclaration single;
                single.name = "gl_WorkGroupSize";
                single.initializer = std::make_unique<Initializer>();
                single.initializer->expr = boxed (makeConstruct ({}, makeType (TypeKind::uvec3), [&]
                {
                    std::vector<Expr> args;
                    args.push_back (makeUIntLiteral ({}, ep.workgroupSizeX));
                    args.push_back (makeUIntLiteral ({}, ep.workgroupSizeY));
                    args.push_back (makeUIntLiteral ({}, ep.workgroupSizeZ));
                    return args;
                }()));
                list->declarations.push_back (std::move (single));

                Declaration d;
                d.initDeclaratorList = std::move (list);
                builtinDeclarations.push_back (std::move (d));
            }
        }

        // Builtin globals go first so every function can see them
        std::vector<ExternalDeclaration> merged;
        for (auto& d : builtinDeclarations)
            merged.push_back (std::move (d));
        for (auto& external : result.ast.declarations)
            merged.push_back (std::move (external));
        result.ast.declarations = std::move (merged);
        builtinDeclarations.clear();
    }

    void builtinDeclarationsMask (const std::string& name)
    {
        auto list = std::make_unique<InitDeclaratorList>();
        list->type = makeArrayType (makeType (TypeKind::intType), 1);

        SingleDeclaration single;
        single.name = name;
        list->declarations.push_back (std::move (single));

        Declaration d;
        d.initDeclaratorList = std::move (list);
        builtinDeclarations.push_back (std::move (d));
    }

    //==========================================================================
    // Globals
    //==========================================================================

    const StructSpecifier* findStruct (TranslationUnit& ast, const std::string& name)
    {
        for (auto& external : ast.declarations)
            if (auto* d = std::get_if<Declaration> (&external); d != nullptr && d->structSpecifier != nullptr && d->structSpecifier->name == name)
                return d->structSpecifier.get();

        return nullptr;
    }

    /** Turns a GLSL initializer list into the equivalent constructor call for the declared type. */
    Expr aggregateToConstructor (TranslationUnit& ast, Initializer& init, const TypeSpecifier& type)
    {
        if (init.expr != nullptr)
            return std::move (*init.expr);

        std::vector<TypeSpecifier> elementTypes;
        auto constructedType = type;

        if (! type.arraySpecifiers.empty())
        {
            elementTypes.assign (init.aggregate.size(), elementType (type));

            if (constructedType.arraySpecifiers.front().isUnsized)
                constructedType = makeArrayType (elementType (type), static_cast<int64_t> (init.aggregate.size()));
        }
        else if (type.kind == TypeKind::namedStruct)
        {
            const auto* ss = findStruct (ast, type.structName);
            if (ss == nullptr || ss->fields.size() != init.aggregate.size())
                throw LoweringError (init.loc, "Initializer list does not match struct '" + type.structName + "'");

            for (const auto& field : ss->fields)
                elementTypes.push_back (field.type);
        }
        else if (const auto [columns, rows] = matrixShape (type.kind); columns > 0)
        {
            elementTypes.assign (init.aggregate.size(), makeType (vectorKind (TypeKind::floatType, rows)));
        }
        else if (componentCount (type.kind) > 1)
        {
            elementTypes.assign (init.aggregate.size(), makeType (scalarKindOf (type.kind)));
        }
        else
        {
            throw LoweringError (init.loc, "Initializer list used for a scalar");
        }

        std::vector<Expr> args;
        for (size_t i = 0; i < init.aggregate.size(); ++i)
            args.push_back (aggregateToConstructor (ast, init.aggregate[i], elementTypes[i]));

        return makeConstruct (init.loc, constructedType, std::move (args));
    }

    /** float a[] = float[](1.0, 2.0) takes its size from the initializer. */
    void sizeArraysFromInitializers (TranslationUnit& ast)
    {
        const auto resolve = [] (Declaration& d)
        {
            if (d.initDeclaratorList == nullptr)
                return;

            auto& list = *d.initDeclaratorList;

            for (auto& single : list.declarations)
            {
                auto type = declaratorType (list.type, single.arraySpecifiers);
                if (type.arraySpecifiers.empty() || ! type.arraySpecifiers.front().isUnsized)
                    continue;

                const auto* init = single.initializer != nullptr ? single.initializer->expr.get() : nullptr;
                if (init == nullptr || ! init->type.has_value() || init->type->arraySpecifiers.empty() || init->type->arraySpecifiers.front().isUnsized)
                    throw LoweringError (single.loc, "Cannot determine the size of array '" + single.name + "'");

                if (! single.arraySpecifiers.empty())
                    single.arraySpecifiers.front() = init->type->arraySpecifiers.front();
                else
                    list.type.arraySpecifiers.front() = init->type->arraySpecifiers.front();
            }
        };

        for (auto& external : ast.declarations)
        {
            if (auto* d = std::get_if<Declaration> (&external))
                resolve (*d);
            else if (auto* fd = std::get_if<FunctionDefinition> (&external); fd != nullptr && fd->body != nullptr)
                forEachStatement (*fd->body, [&resolve] (Statement& s)
                {
                    if (s.is<StmtDeclaration>())
                        resolve (s.as<StmtDeclaration>().declaration);
                });
        }
    }

    void convertAggregateInitializers (TranslationUnit& ast)
    {
        const auto convert = [this, &ast] (Declaration& d)
        {
            if (d.initDeclaratorList == nullptr)
                return;

            for (auto& single : d.initDeclaratorList->declarations)
            {
                if (single.initializer == nullptr || single.initializer->aggregate.empty())
                    continue;

                const auto type = declaratorType (d.initDeclaratorList->type, single.arraySpecifiers);
                auto ctor = aggregateToConstructor (ast, *single.initializer, type);

                // int a[] = { 1, 2 } takes its size from the initializer
                if (! single.arraySpecifiers.empty() && single.arraySpecifiers.front().isUnsized)
                    single.arraySpecifiers.front() = ctor.as<ExprTypeConstructor>().type.arraySpecifiers.front();

                single.initializer->aggregate.clear();
                single.initializer->expr = boxed (std::move (ctor));
            }
        };

        for (auto& external : ast.declarations)
        {
            if (auto* d = std::get_if<Declaration> (&external))
                convert (*d);
            else if (auto* fd = std::get_if<FunctionDefinition> (&external); fd != nullptr && fd->body != nullptr)
                forEachStatement (*fd->body, [&convert] (Statement& s)
                {
                    if (s.is<StmtDeclaration>())
                        convert (s.as<StmtDeclaration>().declaration);
                });
        }
    }

    /** WGSL private variables need constant initializers, GLSL globals don't: initialize them at the start of main instead. */
    void lowerGlobalInitializers (TranslationUnit& ast)
    {
        std::vector<Statement> prologue;

        for (auto& external : ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->initDeclaratorList == nullptr || d->structSpecifier != nullptr)
                continue;

            auto& list = *d->initDeclaratorList;
            const auto* q = list.qualifier.get();

            if (q != nullptr && (q->hasStorage (StorageQualifier::constQual) || q->hasStorage (StorageQualifier::uniform)
                                 || q->hasStorage (StorageQualifier::buffer) || q->hasStorage (StorageQualifier::shared)))
                continue;

            for (auto& single : list.declarations)
            {
                if (single.initializer == nullptr || single.initializer->expr == nullptr)
                    continue;

                auto target = makeVariable (single.loc, single.name, declaratorType (list.type, single.arraySpecifiers));
                prologue.push_back (makeExprStatement (single.loc, makeAssign (single.loc, AssignmentOp::assign, std::move (target), std::move (*single.initializer->expr))));
                single.initializer.reset();
            }
        }

        if (prologue.empty())
            return;

        for (auto& external : ast.declarations)
        {
            auto* fd = std::get_if<FunctionDefinition> (&external);
            if (fd == nullptr || fd->prototype.name != context.innerFunction || fd->body == nullptr || ! fd->body->is<StmtCompound>())
                continue;

            auto& statements = fd->body->as<StmtCompound>().statements;
            statements.insert (statements.begin(), std::make_move_iterator (prologue.begin()), std::make_move_iterator (prologue.end()));
        }
    }

    //==========================================================================
    WgslLoweringOptions options;
    WgslLoweringContext context;
    std::set<std::string> userFunctions;
    bool invariantPosition = false;

    std::vector<Declaration> builtinDeclarations;
};

} // namespace

//==============================================================================
ResultValue<LoweredProgram> WgslLowering::lower (TranslationUnit ast,
                                                 const WgslLoweringOptions& options)
{
    try
    {
        LoweringImpl impl (options);
        return makeResultValueOk (impl.run (std::move (ast)));
    }
    catch (const std::exception& e)
    {
        return makeResultValueFail (String (e.what()));
    }
}

} // namespace wgsl
} // namespace yup
