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

using Statements = std::vector<Statement>;

bool isOutQualified (const TypeQualifier* q)
{
    return q != nullptr && (q->hasStorage (StorageQualifier::out) || q->hasStorage (StorageQualifier::inout));
}

bool isIncDec (UnaryOp op)
{
    return op == UnaryOp::preInc || op == UnaryOp::preDec || op == UnaryOp::postInc || op == UnaryOp::postDec;
}

const Expr& unparen (const Expr& e)
{
    const auto* current = &e;
    while (current->is<ExprParen>() && current->as<ExprParen>().expr != nullptr)
        current = current->as<ExprParen>().expr.get();

    return *current;
}

Expr takeUnparen (Expr e)
{
    while (e.is<ExprParen>() && e.as<ExprParen>().expr != nullptr)
    {
        auto inner = std::move (*e.as<ExprParen>().expr);
        e = std::move (inner);
    }

    return e;
}

/** GLSL builtins that write through an out parameter, and the index of that parameter. */
std::vector<size_t> builtinOutParameters (const std::string& name, size_t argumentCount)
{
    if ((name == "frexp" || name == "modf") && argumentCount == 2)
        return { 1 };

    if ((name == "uaddCarry" || name == "usubBorrow") && argumentCount == 3)
        return { 2 };

    if ((name == "umulExtended" || name == "imulExtended") && argumentCount == 4)
        return { 2, 3 };

    return {};
}

bool isSideEffectBuiltin (const std::string& name)
{
    static const std::set<std::string> names = {
        "barrier", "memoryBarrier", "memoryBarrierShared", "memoryBarrierBuffer", "memoryBarrierImage", "groupMemoryBarrier",
        "imageStore", "atomicAdd", "atomicMin", "atomicMax", "atomicAnd", "atomicOr", "atomicXor", "atomicExchange", "atomicCompSwap",
        "atomicStore"
    };

    return names.count (name) > 0;
}

//==============================================================================
class StatementLowerer : private WgslBuiltinHost
{
public:
    StatementLowerer (TranslationUnit& unit, WgslLoweringContext& ctx)
        : ast (unit)
        , context (ctx)
    {
    }

    void run()
    {
        for (auto& external : ast.declarations)
        {
            if (auto* fd = std::get_if<FunctionDefinition> (&external))
                functions[fd->prototype.name] = &fd->prototype;
            else if (auto* d = std::get_if<Declaration> (&external); d != nullptr && d->initDeclaratorList != nullptr)
                declareGlobals (*d);
        }

        for (auto& external : ast.declarations)
        {
            ScopeGuard scope (*this);

            if (auto* d = std::get_if<Declaration> (&external))
            {
                // Global initializers are constant expressions: only expression-level rewrites apply
                forEachDeclarationExpr (*d, [this] (Expr& e)
                {
                    Statements pre;
                    e = lowerValue (std::move (e), pre);

                    if (! pre.empty())
                        throw LoweringError (e.loc, "Global initializers must be constant expressions");
                });
            }
            else if (auto* fd = std::get_if<FunctionDefinition> (&external); fd != nullptr && fd->body != nullptr)
            {
                lowerFunction (*fd);
            }
        }

        // Combined sampler parameters become a texture and a sampler parameter
        for (auto& external : ast.declarations)
            if (auto* fd = std::get_if<FunctionDefinition> (&external))
                expandSamplerParameters (fd->prototype);
    }

private:
    //==========================================================================
    // Symbols
    //==========================================================================

    enum class VarKind
    {
        global,
        local,
        constant,
        valueParameter,
        pointerParameter,
        temporary
    };

    struct VarInfo
    {
        VarKind kind = VarKind::local;
        bool isWgslConst = false; // declared as a WGSL const, usable in constant expressions
        TypeSpecifier type;
        std::string companionSampler; // for combined sampler globals and parameters
    };

    void declareGlobals (const Declaration& d)
    {
        const auto& list = *d.initDeclaratorList;
        const bool isConst = list.qualifier != nullptr && list.qualifier->hasStorage (StorageQualifier::constQual);

        bool isOverride = false;
        if (list.qualifier != nullptr && list.qualifier->layout != nullptr)
            for (const auto& entry : list.qualifier->layout->entries)
                isOverride = isOverride || entry.id == LayoutQualifierId::constantId;

        for (const auto& single : list.declarations)
        {
            VarInfo info;
            info.kind = isConst ? VarKind::constant : VarKind::global;
            info.isWgslConst = isConst && ! isOverride;
            info.type = declaratorType (list.type, single.arraySpecifiers);

            if (auto found = context.samplerCompanions.find (single.name); found != context.samplerCompanions.end())
                info.companionSampler = found->second;

            globals[single.name] = info;
        }
    }

    const VarInfo* lookup (const std::string& name) const
    {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            if (auto found = it->find (name); found != it->end())
                return &found->second;

        if (auto found = globals.find (name); found != globals.end())
            return &found->second;

        return nullptr;
    }

    void declare (const std::string& name, VarInfo info)
    {
        scopes.back()[name] = std::move (info);
    }

    struct ScopeGuard
    {
        explicit ScopeGuard (StatementLowerer& l)
            : lowerer (l)
        {
            lowerer.scopes.emplace_back();
        }

        ~ScopeGuard() { lowerer.scopes.pop_back(); }

        StatementLowerer& lowerer;
    };

    //==========================================================================
    // Functions
    //==========================================================================

    void lowerFunction (FunctionDefinition& fd)
    {
        auto& proto = fd.prototype;
        ScopeGuard functionScope (*this);

        const auto written = collectWrittenParameters (fd);
        Statements prologue;

        for (auto& param : proto.parameters)
        {
            VarInfo info;
            info.type = declaratorType (param.type, param.arraySpecifiers);

            if (isOutQualified (param.qualifier.get()))
            {
                info.kind = VarKind::pointerParameter;
            }
            else if (written.count (param.name) > 0)
            {
                // WGSL parameters are immutable: copy a written one into a local of the same name
                const auto local = param.name;
                param.name = context.names.allocate ("_" + local);
                prologue.push_back (makeVarDeclaration (param.loc, local, info.type, makeVariable (param.loc, param.name, info.type)));
                info.kind = VarKind::local;
                declare (local, info);
                continue;
            }
            else
            {
                info.kind = VarKind::valueParameter;
            }

            if (isSamplerType (info.type.kind))
                info.companionSampler = samplerParameterName (param.name);

            declare (param.name, info);
        }

        auto& body = fd.body->as<StmtCompound>().statements;
        Statements lowered = std::move (prologue);

        for (auto& s : body)
            lowerStatement (s, lowered);

        body = std::move (lowered);
    }

    std::string samplerParameterName (const std::string& textureParam)
    {
        auto& name = samplerParameters[textureParam];
        if (name.empty())
            name = context.names.allocate (textureParam + "_sampler");

        return name;
    }

    void expandSamplerParameters (FunctionPrototype& proto)
    {
        std::vector<FunctionParameterDeclaration> expanded;

        for (auto& param : proto.parameters)
        {
            const bool isCombined = isSamplerType (param.type.kind);
            const auto samplerKind = textureShape (param.type.kind).shadow ? TypeKind::samplerShadow : TypeKind::samplerType;
            const auto name = param.name;
            const auto loc = param.loc;

            expanded.push_back (std::move (param));

            if (isCombined)
            {
                FunctionParameterDeclaration sampler;
                sampler.loc = loc;
                sampler.name = samplerParameterName (name);
                sampler.type = TypeSpecifier::make (loc, samplerKind);
                expanded.push_back (std::move (sampler));
            }
        }

        proto.parameters = std::move (expanded);
    }

    /** Value parameters written anywhere in the body, including through out arguments. */
    std::set<std::string> collectWrittenParameters (FunctionDefinition& fd)
    {
        std::set<std::string> params;
        for (const auto& p : fd.prototype.parameters)
            if (! isOutQualified (p.qualifier.get()))
                params.insert (p.name);

        std::set<std::string> written;
        std::vector<std::set<std::string>> shadows (1);

        const auto rootName = [] (const Expr* e) -> std::string
        {
            for (;;)
            {
                e = &unparen (*e);

                if (e->is<ExprVariable>())
                    return e->as<ExprVariable>().name;
                if (e->is<ExprDot>())
                    e = e->as<ExprDot>().base.get();
                else if (e->is<ExprBracket>())
                    e = e->as<ExprBracket>().base.get();
                else
                    return {};
            }
        };

        const auto markWrite = [&] (const Expr& target)
        {
            const auto name = rootName (&target);
            if (params.count (name) == 0)
                return;

            for (const auto& scope : shadows)
                if (scope.count (name) > 0)
                    return;

            written.insert (name);
        };

        const auto visitExpr = [&] (Expr& root)
        {
            walkExpr (root, [&] (Expr& e)
            {
                if (e.is<ExprAssignment>())
                    markWrite (*e.as<ExprAssignment>().lhs);
                else if (e.is<ExprUnary>() && isIncDec (e.as<ExprUnary>().op))
                    markWrite (*e.as<ExprUnary>().operand);
                else if (e.is<ExprFunCall>())
                {
                    for (const auto index : outArgumentIndices (e.as<ExprFunCall>()))
                        markWrite (e.as<ExprFunCall>().args[index]);
                }
            });
        };

        std::function<void (Statement&)> visitStatement = [&] (Statement& s)
        {
            const bool opensScope = s.is<StmtCompound>() || s.is<StmtFor>();
            if (opensScope)
                shadows.emplace_back();

            if (s.is<StmtDeclaration>())
            {
                auto& decl = s.as<StmtDeclaration>().declaration;
                forEachDeclarationExpr (decl, visitExpr);

                if (decl.initDeclaratorList != nullptr)
                    for (const auto& single : decl.initDeclaratorList->declarations)
                        shadows.back().insert (single.name);
            }
            else
            {
                forEachOwnExpr (s, visitExpr);
            }

            forEachChildStatement (s, visitStatement);

            if (opensScope)
                shadows.pop_back();
        };

        visitStatement (*fd.body);
        return written;
    }

    std::vector<size_t> outArgumentIndices (const ExprFunCall& call) const
    {
        if (call.callee == nullptr || ! call.callee->is<ExprVariable>())
            return {};

        const auto& name = call.callee->as<ExprVariable>().name;

        if (auto found = functions.find (name); found != functions.end())
        {
            std::vector<size_t> indices;
            const auto& params = found->second->parameters;

            for (size_t i = 0; i < params.size() && i < call.args.size(); ++i)
                if (isOutQualified (params[i].qualifier.get()))
                    indices.push_back (i);

            return indices;
        }

        return builtinOutParameters (name, call.args.size());
    }

    //==========================================================================
    // Expression analysis
    //==========================================================================

    bool isUserFunction (const ExprFunCall& call) const
    {
        return call.callee != nullptr && call.callee->is<ExprVariable>() && functions.count (call.callee->as<ExprVariable>().name) > 0;
    }

    /** True if evaluating e may write memory, so it can't be duplicated or evaluated speculatively. */
    bool hasEffects (const Expr& e) const
    {
        bool effects = false;

        walkExpr (const_cast<Expr&> (e), [&] (Expr& node)
        {
            if (node.is<ExprAssignment>() || node.is<ExprComma>() || (node.is<ExprUnary>() && isIncDec (node.as<ExprUnary>().op)))
                effects = true;
            else if (node.is<ExprFunCall>())
            {
                const auto& call = node.as<ExprFunCall>();
                if (isUserFunction (call) || ! outArgumentIndices (call).empty()
                    || (call.callee != nullptr && call.callee->is<ExprVariable>() && isSideEffectBuiltin (call.callee->as<ExprVariable>().name)))
                    effects = true;
            }
        });

        return effects;
    }

    static bool isSelectable (const Expr& ternary)
    {
        return ternary.type.has_value() && ternary.type->arraySpecifiers.empty() && componentCount (ternary.type->kind) > 0;
    }

    /** True if e contains something WGSL can't evaluate inside an expression. */
    bool requiresHoisting (const Expr& e) const
    {
        const auto& node = unparen (e);

        if (node.is<ExprAssignment>() || node.is<ExprComma>())
            return true;

        if (node.is<ExprUnary>() && isIncDec (node.as<ExprUnary>().op))
            return true;

        if (node.is<ExprFunCall>())
        {
            const auto& call = node.as<ExprFunCall>();

            if (! outArgumentIndices (call).empty())
                return true;

            // Builtins may evaluate their arguments into temporaries, and atomicCompSwap lowers to a loop
            if (! isUserFunction (call) && call.callee != nullptr && call.callee->is<ExprVariable>())
            {
                if (call.callee->as<ExprVariable>().name == "atomicCompSwap")
                    return true;

                for (const auto& arg : call.args)
                    if (hasEffects (arg))
                        return true;
            }
        }

        if (node.is<ExprTernary>())
        {
            const auto& t = node.as<ExprTernary>();
            if (! isSelectable (node) || hasEffects (*t.trueBranch) || hasEffects (*t.falseBranch))
                return true;
        }

        bool result = false;
        forEachChildExpr (const_cast<Expr&> (node), [&] (Expr& child)
        {
            result = result || requiresHoisting (child);
        });

        return result;
    }

    /** Values that can be read again later and still give the same result. */
    bool isStable (const Expr& e) const
    {
        const auto& node = unparen (e);

        if (node.is<ExprIntConst>() || node.is<ExprUIntConst>() || node.is<ExprFloatConst>() || node.is<ExprBoolConst>())
            return true;

        if (node.type.has_value() && isOpaqueType (node.type->kind))
            return true;

        if (node.is<ExprVariable>())
        {
            const auto* info = lookup (node.as<ExprVariable>().name);
            return info != nullptr && (info->kind == VarKind::constant || info->kind == VarKind::temporary);
        }

        if (node.is<ExprUnary>() && ! isIncDec (node.as<ExprUnary>().op) && node.as<ExprUnary>().op != UnaryOp::deref)
            return isStable (*node.as<ExprUnary>().operand);

        return false;
    }

    /** Expressions WGSL evaluates at shader creation: literals, WGSL consts and operators on them. */
    bool isWgslConstantExpression (const Expr& e) const
    {
        const auto& node = unparen (e);

        if (node.is<ExprIntConst>() || node.is<ExprUIntConst>() || node.is<ExprFloatConst>() || node.is<ExprBoolConst>())
            return true;

        if (node.is<ExprVariable>())
        {
            const auto* info = lookup (node.as<ExprVariable>().name);
            return info != nullptr && info->isWgslConst;
        }

        if (node.is<ExprUnary>())
            return node.as<ExprUnary>().op != UnaryOp::deref && node.as<ExprUnary>().op != UnaryOp::addressOf
                && ! isIncDec (node.as<ExprUnary>().op) && isWgslConstantExpression (*node.as<ExprUnary>().operand);

        if (node.is<ExprBinary>())
            return isWgslConstantExpression (*node.as<ExprBinary>().left) && isWgslConstantExpression (*node.as<ExprBinary>().right);

        if (node.is<ExprTypeConstructor>())
        {
            for (const auto& arg : node.as<ExprTypeConstructor>().args)
                if (! isWgslConstantExpression (arg))
                    return false;

            return true;
        }

        return false;
    }

    //==========================================================================
    // Temporaries
    //==========================================================================

    TypeSpecifier typeOf (const Expr& e) const
    {
        if (! e.type.has_value())
            throw LoweringError (e.loc, "Cannot determine the type of this expression");

        return *e.type;
    }

    Expr makeTemporary (SourceLocation l, const TypeSpecifier& type, std::optional<Expr> init, Statements& pre, bool isConst)
    {
        const auto name = context.names.allocate ("_t");
        pre.push_back (makeVarDeclaration (l, name, type, std::move (init), isConst));

        VarInfo info;
        info.kind = isConst ? VarKind::temporary : VarKind::local;
        info.type = type;
        declare (name, info);

        return makeVariable (l, name, type);
    }

    //==========================================================================
    // WgslBuiltinHost
    //==========================================================================

    void requirePolyfill (const std::string& name, const std::string& code) override
    {
        if (context.polyfillNames.insert (name).second)
            context.polyfills.push_back (code);
    }

    ShaderStage getStage() const override { return context.stage; }

    std::string allocateName (const std::string& base) override { return context.names.allocate (base); }

    bool isDepthTexture (const Expr& texture) const override
    {
        const auto& node = unparen (texture);
        return node.is<ExprVariable>() && context.depthTextures.count (node.as<ExprVariable>().name) > 0;
    }

    /** Evaluates e once into an immutable temporary. */
    Expr spill (Expr e, Statements& pre) override
    {
        if (isStable (e))
            return e;

        const auto type = typeOf (e);
        const auto l = e.loc;
        return makeTemporary (l, type, std::move (e), pre, true);
    }

    //==========================================================================
    // Values
    //==========================================================================

    /** Lowers e for its value, moving whatever WGSL can't evaluate inline into pre. */
    Expr lowerValue (Expr e, Statements& pre)
    {
        const auto l = e.loc;

        if (e.is<ExprParen>())
        {
            auto inner = lowerValue (std::move (*e.as<ExprParen>().expr), pre);
            return makeParen (l, std::move (inner));
        }

        if (e.is<ExprAssignment>())
            return lowerAssignment (std::move (e), pre, true);

        if (e.is<ExprComma>())
        {
            auto& comma = e.as<ExprComma>();
            lowerEffect (std::move (*comma.left), pre);
            return lowerValue (std::move (*comma.right), pre);
        }

        if (e.is<ExprUnary>() && isIncDec (e.as<ExprUnary>().op))
            return lowerIncDec (std::move (e), pre, true);

        if (e.is<ExprTernary>())
            return lowerTernary (std::move (e), pre);

        if (e.is<ExprBinary>() && (e.as<ExprBinary>().op == BinaryOp::logicalAnd || e.as<ExprBinary>().op == BinaryOp::logicalOr))
            return lowerShortCircuit (std::move (e), pre);

        if (e.is<ExprFunCall>())
            return lowerCall (std::move (e), pre, true);

        lowerChildrenInOrder (e, pre);
        return rewriteNode (std::move (e), pre);
    }

    /** Lowers the children of e left to right, spilling earlier ones when a later one hoists effects. */
    void lowerChildrenInOrder (Expr& e, Statements& pre)
    {
        std::vector<Expr*> children;
        forEachChildExpr (e, [&children] (Expr& child)
        {
            children.push_back (&child);
        });

        // The callee of a call is a name, not a value
        if (e.is<ExprFunCall>() && ! children.empty())
            children.erase (children.begin());

        size_t lastHoisting = 0;
        bool anyHoisting = false;

        for (size_t i = 0; i < children.size(); ++i)
        {
            if (requiresHoisting (*children[i]))
            {
                lastHoisting = i;
                anyHoisting = true;
            }
        }

        for (size_t i = 0; i < children.size(); ++i)
        {
            *children[i] = lowerValue (std::move (*children[i]), pre);

            // The base of an element or member access names storage: reading it is deferred to the access itself
            const bool isAccessBase = i == 0 && (e.is<ExprBracket>() || e.is<ExprDot>()) && isLocationPath (*children[i]);

            if (anyHoisting && i < lastHoisting && ! isAccessBase)
                *children[i] = spill (std::move (*children[i]), pre);
        }
    }

    /** Variables, pointer dereferences and member or element accesses of them. */
    static bool isLocationPath (const Expr& e)
    {
        const auto& node = unparen (e);

        if (node.is<ExprVariable>())
            return true;

        if (node.is<ExprUnary>() && node.as<ExprUnary>().op == UnaryOp::deref)
            return isLocationPath (*node.as<ExprUnary>().operand);

        if (node.is<ExprDot>())
            return isLocationPath (*node.as<ExprDot>().base);

        if (node.is<ExprBracket>())
            return isLocationPath (*node.as<ExprBracket>().base);

        return false;
    }

    /** Expression-level rewrites applied once the children are lowered. */
    Expr rewriteNode (Expr e, Statements& pre)
    {
        if (e.is<ExprVariable>())
        {
            const auto& name = e.as<ExprVariable>().name;

            if (name == "gl_PointSize")
                throw LoweringError (e.loc, "gl_PointSize has no WGSL equivalent");

            const auto* info = lookup (name);
            if (info != nullptr && info->kind == VarKind::pointerParameter)
            {
                auto type = e.type;
                return makeUnary (e.loc, UnaryOp::deref, std::move (e), std::move (type));
            }

            return e;
        }

        if (e.is<ExprDot>())
        {
            auto& dot = e.as<ExprDot>();
            const auto& baseType = dot.base->type;

            // GLSL lets scalars be swizzled with x, r or s
            if (baseType.has_value() && baseType->arraySpecifiers.empty() && componentCount (baseType->kind) == 1)
            {
                if (dot.member.find_first_not_of ("xrs") != std::string::npos)
                    throw LoweringError (e.loc, "Invalid swizzle '" + dot.member + "' of a scalar");

                if (dot.member.size() == 1)
                    return std::move (*dot.base);

                const auto type = makeType (vectorKind (baseType->kind, static_cast<int> (dot.member.size())));
                std::vector<Expr> args;
                args.push_back (std::move (*dot.base));
                return makeConstruct (e.loc, type, std::move (args));
            }

            lowerSwizzle (dot);
            return e;
        }

        if (e.is<ExprBinary>())
        {
            auto& bin = e.as<ExprBinary>();

            if ((bin.op == BinaryOp::equal || bin.op == BinaryOp::notEqual) && bin.left->type.has_value())
            {
                const auto& operandType = *bin.left->type;

                if (operandType.kind == TypeKind::namedStruct || ! operandType.arraySpecifiers.empty() || isMatrixType (operandType.kind))
                    throw LoweringError (e.loc, "Comparing structs, arrays or matrices with == or != is not supported for WGSL");

                // GLSL compares whole vectors to a single bool
                if (componentCount (operandType.kind) > 1)
                {
                    const auto reduce = bin.op == BinaryOp::equal ? "all" : "any";
                    const auto l = e.loc;
                    e.type = makeType (vectorKind (TypeKind::boolType, componentCount (operandType.kind)));

                    std::vector<Expr> args;
                    args.push_back (std::move (e));
                    return makeCall (l, reduce, std::move (args), makeType (TypeKind::boolType));
                }
            }

            return e;
        }

        if (e.is<ExprTypeConstructor>())
            return WgslBuiltins::lowerConstructor (std::move (e), pre, *this);

        if (e.is<ExprFunCall>() && ! isUserFunction (e.as<ExprFunCall>()) && ! isAlreadyWgsl (e.as<ExprFunCall>()))
            return WgslBuiltins::lowerCall (std::move (e), pre, *this);

        return e;
    }

    /** Calls earlier lowering passes produced in WGSL form: storage accessors and polyfills. */
    bool isAlreadyWgsl (const ExprFunCall& call) const
    {
        if (call.callee == nullptr || ! call.callee->is<ExprVariable>())
            return false;

        const auto& name = call.callee->as<ExprVariable>().name;
        return name == "atomicLoad" || name == "atomicStore" || name == "select" || context.polyfillNames.count (name) > 0;
    }

    /** WGSL only has xyzw and rgba swizzles. */
    static void lowerSwizzle (ExprDot& dot)
    {
        if (dot.base == nullptr || ! dot.base->type.has_value() || componentCount (dot.base->type->kind) == 0
            || ! dot.base->type->arraySpecifiers.empty())
            return;

        for (auto& c : dot.member)
        {
            static const std::map<char, char> mapping = { { 's', 'x' }, { 't', 'y' }, { 'p', 'z' }, { 'q', 'w' } };
            if (auto found = mapping.find (c); found != mapping.end())
                c = found->second;
        }
    }

    /** x.length() is the only method in GLSL: a constant for sized arrays, vectors and matrices, arrayLength for runtime arrays. */
    Expr lowerLengthMethod (Expr e, Statements& pre)
    {
        auto& call = e.as<ExprFunCall>();
        auto& dot = call.callee->as<ExprDot>();

        if (dot.member != "length" || ! call.args.empty())
            throw LoweringError (e.loc, "Unsupported method call '." + dot.member + "()'");

        if (dot.base == nullptr || ! dot.base->type.has_value())
            throw LoweringError (e.loc, "Cannot determine the type of the .length() operand");

        const auto type = *dot.base->type;

        if (! type.arraySpecifiers.empty())
        {
            const auto& outer = type.arraySpecifiers.front();

            if (outer.isUnsized)
            {
                auto base = lowerLValue (std::move (*dot.base), pre, false);

                std::vector<Expr> args;
                args.push_back (makeUnary (e.loc, UnaryOp::addressOf, std::move (base)));

                std::vector<Expr> converted;
                converted.push_back (makeCall (e.loc, "arrayLength", std::move (args), makeType (TypeKind::uintType)));
                return makeConstruct (e.loc, makeType (TypeKind::intType), std::move (converted));
            }

            if (outer.sizeExpr != nullptr)
            {
                if (const auto size = evaluateIntConstant (*outer.sizeExpr))
                    return makeIntLiteral (e.loc, *size);

                std::vector<Expr> args;
                args.push_back (copyExpr (*outer.sizeExpr));
                return makeConstruct (e.loc, makeType (TypeKind::intType), std::move (args));
            }
        }

        if (const auto count = componentCount (type.kind); count > 1)
            return makeIntLiteral (e.loc, count);

        if (const auto columns = matrixShape (type.kind).first; columns > 0)
            return makeIntLiteral (e.loc, columns);

        throw LoweringError (e.loc, ".length() needs an array, vector or matrix operand");
    }

    //==========================================================================
    // Calls
    //==========================================================================

    /** A combined sampler argument becomes the explicit sampler2D(texture, sampler) pair. */
    Expr pairCombinedSampler (Expr arg)
    {
        const auto& node = unparen (arg);
        if (! node.is<ExprVariable>() || ! node.type.has_value() || ! isSamplerType (node.type->kind))
            return arg;

        const auto* info = lookup (node.as<ExprVariable>().name);
        if (info == nullptr || info->companionSampler.empty())
            throw LoweringError (arg.loc, "Combined image sampler '" + node.as<ExprVariable>().name + "' has no companion sampler");

        const auto type = *node.type;
        const auto samplerKind = textureShape (type.kind).shadow ? TypeKind::samplerShadow : TypeKind::samplerType;
        const auto l = arg.loc;

        std::vector<Expr> args;
        args.push_back (takeUnparen (std::move (arg)));
        args.push_back (makeVariable (l, info->companionSampler, makeType (samplerKind)));
        return makeConstruct (l, type, std::move (args));
    }

    Expr lowerCall (Expr e, Statements& pre, bool valueNeeded)
    {
        auto& call = e.as<ExprFunCall>();

        if (call.callee != nullptr && call.callee->is<ExprDot>())
            return lowerLengthMethod (std::move (e), pre);

        if (call.callee == nullptr || ! call.callee->is<ExprVariable>())
            throw LoweringError (e.loc, "Unsupported call expression");

        const auto name = call.callee->as<ExprVariable>().name;
        const auto outIndices = outArgumentIndices (call);
        const bool userFunction = functions.count (name) > 0;

        for (auto& arg : call.args)
            arg = pairCombinedSampler (std::move (arg));

        if (outIndices.empty())
        {
            lowerChildrenInOrder (e, pre);

            if (userFunction)
                expandSamplerArguments (call);

            return rewriteNode (std::move (e), pre);
        }

        // Copy-in/copy-out: out arguments that aren't plain locals go through a temporary written back after the call
        std::vector<std::pair<Expr, Expr>> writeBacks; // target, temporary
        const auto isOut = [&outIndices] (size_t i)
        {
            return std::find (outIndices.begin(), outIndices.end(), i) != outIndices.end();
        };

        // A local can only be passed by address when nothing else in the call reads or writes it
        std::vector<std::set<std::string>> argumentNames (call.args.size());
        std::vector<bool> hoists (call.args.size(), false);

        for (size_t i = 0; i < call.args.size(); ++i)
        {
            walkExpr (call.args[i], [&argumentNames, i] (Expr& node)
            {
                if (node.is<ExprVariable>())
                    argumentNames[i].insert (node.as<ExprVariable>().name);
            });

            hoists[i] = requiresHoisting (call.args[i]);
        }

        const auto canPassByAddress = [&] (size_t index, const std::string& name)
        {
            for (size_t j = 0; j < call.args.size(); ++j)
            {
                if (j != index && argumentNames[j].count (name) > 0)
                    return false;

                if (j > index && hoists[j])
                    return false;
            }

            return true;
        };

        for (size_t i = 0; i < call.args.size(); ++i)
        {
            auto arg = std::move (call.args[i]);

            if (! isOut (i))
            {
                arg = spill (lowerValue (std::move (arg), pre), pre);
                call.args[i] = std::move (arg);
                continue;
            }

            const bool isInout = userFunction && functions.at (name)->parameters[i].qualifier->hasStorage (StorageQualifier::inout);
            const auto l = arg.loc;
            auto target = lowerLValue (std::move (arg), pre, true);
            const auto& node = unparen (target);

            if (node.is<ExprVariable>())
            {
                const auto* info = lookup (node.as<ExprVariable>().name);
                if (info != nullptr && info->kind == VarKind::local && canPassByAddress (i, node.as<ExprVariable>().name))
                {
                    auto type = target.type;
                    call.args[i] = makeUnary (l, UnaryOp::addressOf, takeUnparen (std::move (target)), std::move (type));
                    continue;
                }
            }

            if (node.is<ExprUnary>() && node.as<ExprUnary>().op == UnaryOp::deref)
            {
                // Forward an existing pointer parameter
                call.args[i] = std::move (*takeUnparen (std::move (target)).as<ExprUnary>().operand);
                continue;
            }

            const auto type = typeOf (target);
            auto temporary = makeTemporary (l, type, isInout ? std::optional<Expr> (copyExpr (target)) : std::nullopt, pre, false);
            call.args[i] = makeUnary (l, UnaryOp::addressOf, copyExpr (temporary));
            writeBacks.emplace_back (std::move (target), std::move (temporary));
        }

        if (userFunction)
            expandSamplerArguments (call);

        Expr result;
        const bool returnsValue = e.type.has_value();

        if (returnsValue && valueNeeded)
        {
            const auto type = *e.type;
            const auto l = e.loc;
            auto callExpr = rewriteNode (std::move (e), pre);
            result = makeTemporary (l, type, std::move (callExpr), pre, true);
        }
        else
        {
            auto callExpr = rewriteNode (std::move (e), pre);
            pre.push_back (makeExprStatement (callExpr.loc, userFunction || ! returnsValue ? std::move (callExpr) : phony (std::move (callExpr))));
        }

        for (auto& [target, temporary] : writeBacks)
            emitStore (std::move (target), AssignmentOp::assign, std::move (temporary), pre);

        return result;
    }

    /** Each sampler2D(texture, sampler) argument of a user function call becomes two arguments. */
    static void expandSamplerArguments (ExprFunCall& call)
    {
        std::vector<Expr> expanded;

        for (auto& arg : call.args)
        {
            if (arg.is<ExprTypeConstructor>() && isSamplerType (arg.as<ExprTypeConstructor>().type.kind) && arg.as<ExprTypeConstructor>().args.size() == 2)
            {
                auto& pair = arg.as<ExprTypeConstructor>().args;
                expanded.push_back (std::move (pair[0]));
                expanded.push_back (std::move (pair[1]));
            }
            else
            {
                expanded.push_back (std::move (arg));
            }
        }

        call.args = std::move (expanded);
    }

    static Expr phony (Expr e)
    {
        const auto l = e.loc;
        return makeAssign (l, AssignmentOp::assign, makeVariable (l, "_"), std::move (e));
    }

    //==========================================================================
    // Assignments
    //==========================================================================

    /** Lowers an assignment target, evaluating its index expressions (spilling them if requested). */
    Expr lowerLValue (Expr e, Statements& pre, bool spillIndices)
    {
        const auto l = e.loc;

        if (e.is<ExprParen>())
            return lowerLValue (std::move (*e.as<ExprParen>().expr), pre, spillIndices);

        if (e.is<ExprVariable>())
            return rewriteNode (std::move (e), pre);

        if (e.is<ExprDot>())
        {
            auto& dot = e.as<ExprDot>();
            *dot.base = lowerLValue (std::move (*dot.base), pre, spillIndices);
            lowerSwizzle (dot);
            return e;
        }

        if (e.is<ExprBracket>())
        {
            auto& br = e.as<ExprBracket>();
            *br.base = lowerLValue (std::move (*br.base), pre, spillIndices);
            *br.index = lowerValue (std::move (*br.index), pre);

            if (spillIndices)
                *br.index = spill (std::move (*br.index), pre);

            return e;
        }

        throw LoweringError (l, "Expression can't be assigned to");
    }

    static bool isMultiComponentSwizzle (const Expr& target)
    {
        const auto& node = unparen (target);
        return node.is<ExprDot>() && node.as<ExprDot>().member.size() > 1
            && node.as<ExprDot>().base->type.has_value() && componentCount (node.as<ExprDot>().base->type->kind) > 1
            && node.as<ExprDot>().base->type->arraySpecifiers.empty();
    }

    static BinaryOp binaryOpFor (AssignmentOp op)
    {
        switch (op)
        {
            case AssignmentOp::addAssign:
                return BinaryOp::add;
            case AssignmentOp::subAssign:
                return BinaryOp::sub;
            case AssignmentOp::mulAssign:
                return BinaryOp::mul;
            case AssignmentOp::divAssign:
                return BinaryOp::div;
            case AssignmentOp::modAssign:
                return BinaryOp::mod;
            case AssignmentOp::shiftLeftAssign:
                return BinaryOp::shiftLeft;
            case AssignmentOp::shiftRightAssign:
                return BinaryOp::shiftRight;
            case AssignmentOp::bitwiseAndAssign:
                return BinaryOp::bitwiseAnd;
            case AssignmentOp::bitwiseXorAssign:
                return BinaryOp::bitwiseXor;
            case AssignmentOp::bitwiseOrAssign:
                return BinaryOp::bitwiseOr;
            case AssignmentOp::assign:
                break;
        }

        return BinaryOp::add;
    }

    /** Emits target op= value as statements, splitting stores to multi-component swizzles, which WGSL can't assign. */
    void emitStore (Expr target, AssignmentOp op, Expr value, Statements& pre)
    {
        const auto l = target.loc;

        if (! isMultiComponentSwizzle (target))
        {
            pre.push_back (makeExprStatement (l, makeAssign (l, op, std::move (target), std::move (value))));
            return;
        }

        auto swizzle = takeUnparen (std::move (target));
        auto& dot = swizzle.as<ExprDot>();

        if (op != AssignmentOp::assign)
            value = makeBinary (l, binaryOpFor (op), copyExpr (swizzle), std::move (value), swizzle.type);

        auto stored = spill (std::move (value), pre);
        const auto scalarType = makeType (scalarKindOf (dot.base->type->kind));
        static const char components[] = "xyzw";

        for (size_t i = 0; i < dot.member.size(); ++i)
        {
            auto component = makeDot (l, copyExpr (*dot.base), std::string (1, dot.member[i]), scalarType);
            auto part = makeDot (l, copyExpr (stored), std::string (1, components[i]), scalarType);
            pre.push_back (makeExprStatement (l, makeAssign (l, AssignmentOp::assign, std::move (component), std::move (part))));
        }
    }

    Expr lowerAssignment (Expr e, Statements& pre, bool valueNeeded)
    {
        auto& assign = e.as<ExprAssignment>();
        const auto op = assign.op;
        const bool rhsHoists = requiresHoisting (*assign.rhs);

        // Swizzle stores repeat the target once per component: its indices must be evaluated once
        const bool repeatsTarget = isMultiComponentSwizzle (*assign.lhs);

        auto target = lowerLValue (std::move (*assign.lhs), pre, rhsHoists || valueNeeded || repeatsTarget);
        auto value = lowerValue (std::move (*assign.rhs), pre);

        auto read = copyExpr (target);
        emitStore (std::move (target), op, std::move (value), pre);
        return read;
    }

    Expr lowerIncDec (Expr e, Statements& pre, bool valueNeeded)
    {
        auto& un = e.as<ExprUnary>();
        const bool increment = un.op == UnaryOp::preInc || un.op == UnaryOp::postInc;
        const bool isPost = un.op == UnaryOp::postInc || un.op == UnaryOp::postDec;
        const auto l = e.loc;

        const bool repeatsTarget = isMultiComponentSwizzle (*un.operand);
        auto target = lowerLValue (std::move (*un.operand), pre, valueNeeded || repeatsTarget);
        const auto type = typeOf (target);

        if (isMatrixType (type.kind) || ! type.arraySpecifiers.empty() || type.kind == TypeKind::namedStruct)
            throw LoweringError (l, "++ and -- on matrices, arrays or structs are not supported for WGSL");

        Expr result;
        if (valueNeeded)
            result = isPost ? spill (copyExpr (target), pre) : copyExpr (target);

        const bool isIntegerScalar = type.kind == TypeKind::intType || type.kind == TypeKind::uintType;

        if (isPost && isIntegerScalar)
        {
            pre.push_back (makeExprStatement (l, makeUnary (l, un.op, std::move (target), type)));
        }
        else
        {
            auto one = scalarKindOf (type.kind) == TypeKind::floatType ? makeFloatLiteral (l, 1.0) : makeIntLiteral (l, 1);
            one.type = makeType (scalarKindOf (type.kind));
            emitStore (std::move (target), increment ? AssignmentOp::addAssign : AssignmentOp::subAssign, std::move (one), pre);
        }

        return result;
    }

    //==========================================================================
    // Control flow inside expressions
    //==========================================================================

    /** select() when both arms are plain values, otherwise an if/else assigning a temporary. */
    Expr lowerTernary (Expr e, Statements& pre)
    {
        auto& t = e.as<ExprTernary>();
        const auto l = e.loc;
        const bool selectable = isSelectable (e) && ! hasEffects (*t.trueBranch) && ! hasEffects (*t.falseBranch);

        auto condition = lowerValue (std::move (*t.condition), pre);

        Statements thenPre;
        Statements elsePre;
        Expr thenValue;
        Expr elseValue;

        {
            ScopeGuard scope (*this);
            thenValue = lowerValue (std::move (*t.trueBranch), thenPre);
        }

        {
            ScopeGuard scope (*this);
            elseValue = lowerValue (std::move (*t.falseBranch), elsePre);
        }

        if (selectable && thenPre.empty() && elsePre.empty())
        {
            *t.condition = std::move (condition);
            *t.trueBranch = std::move (thenValue);
            *t.falseBranch = std::move (elseValue);
            return e;
        }

        const auto type = typeOf (e);
        auto result = makeTemporary (l, type, std::nullopt, pre, false);

        thenPre.push_back (makeExprStatement (l, makeAssign (l, AssignmentOp::assign, copyExpr (result), std::move (thenValue))));
        elsePre.push_back (makeExprStatement (l, makeAssign (l, AssignmentOp::assign, copyExpr (result), std::move (elseValue))));

        pre.push_back (makeIf (l, std::move (condition), makeBlock (l, std::move (thenPre)), makeBlock (l, std::move (elsePre))));
        return result;
    }

    /** a && b and a || b stay inline unless evaluating b needs statements, which must only run when b is evaluated. */
    Expr lowerShortCircuit (Expr e, Statements& pre)
    {
        auto& bin = e.as<ExprBinary>();
        const bool isAnd = bin.op == BinaryOp::logicalAnd;
        const auto l = e.loc;

        auto left = lowerValue (std::move (*bin.left), pre);

        Statements rhs;
        Expr right;
        {
            ScopeGuard scope (*this);
            right = lowerValue (std::move (*bin.right), rhs);
        }

        if (rhs.empty())
        {
            *bin.left = std::move (left);
            *bin.right = std::move (right);
            return e;
        }

        auto result = makeTemporary (l, makeType (TypeKind::boolType), std::move (left), pre, false);
        rhs.push_back (makeExprStatement (l, makeAssign (l, AssignmentOp::assign, copyExpr (result), std::move (right))));

        auto condition = isAnd ? copyExpr (result) : makeUnary (l, UnaryOp::logicalNot, copyExpr (result), makeType (TypeKind::boolType));
        pre.push_back (makeIf (l, std::move (condition), makeBlock (l, std::move (rhs))));
        return result;
    }

    //==========================================================================
    // Expression statements
    //==========================================================================

    /** Lowers e evaluated only for its side effects. */
    void lowerEffect (Expr e, Statements& out)
    {
        e = takeUnparen (std::move (e));

        if (e.is<ExprAssignment>())
        {
            lowerAssignment (std::move (e), out, false);
            return;
        }

        if (e.is<ExprUnary>() && isIncDec (e.as<ExprUnary>().op))
        {
            lowerIncDec (std::move (e), out, false);
            return;
        }

        if (e.is<ExprComma>())
        {
            auto& comma = e.as<ExprComma>();
            lowerEffect (std::move (*comma.left), out);
            lowerEffect (std::move (*comma.right), out);
            return;
        }

        if (e.is<ExprTernary>() && hasEffects (e))
        {
            auto& t = e.as<ExprTernary>();
            const auto l = e.loc;
            auto condition = lowerValue (std::move (*t.condition), out);

            Statements thenBranch;
            Statements elseBranch;
            {
                ScopeGuard scope (*this);
                lowerEffect (std::move (*t.trueBranch), thenBranch);
            }
            {
                ScopeGuard scope (*this);
                lowerEffect (std::move (*t.falseBranch), elseBranch);
            }

            out.push_back (makeIf (l, std::move (condition), makeBlock (l, std::move (thenBranch)), makeBlock (l, std::move (elseBranch))));
            return;
        }

        if (e.is<ExprBinary>() && (e.as<ExprBinary>().op == BinaryOp::logicalAnd || e.as<ExprBinary>().op == BinaryOp::logicalOr)
            && hasEffects (*e.as<ExprBinary>().right))
        {
            auto& bin = e.as<ExprBinary>();
            const auto l = e.loc;
            auto left = lowerValue (std::move (*bin.left), out);

            if (bin.op == BinaryOp::logicalOr)
                left = makeUnary (l, UnaryOp::logicalNot, makeParen (l, std::move (left)), makeType (TypeKind::boolType));

            Statements rhs;
            {
                ScopeGuard scope (*this);
                lowerEffect (std::move (*bin.right), rhs);
            }

            out.push_back (makeIf (l, std::move (left), makeBlock (l, std::move (rhs))));
            return;
        }

        if (e.is<ExprFunCall>())
        {
            auto& call = e.as<ExprFunCall>();
            const bool isMethod = call.callee != nullptr && call.callee->is<ExprDot>();
            const bool userFunction = ! isMethod && isUserFunction (call);
            const auto name = ! isMethod && call.callee != nullptr && call.callee->is<ExprVariable>() ? call.callee->as<ExprVariable>().name : std::string();
            const bool returnsValue = e.type.has_value();

            if (! outArgumentIndices (call).empty())
            {
                lowerCall (std::move (e), out, false);
                return;
            }

            auto lowered = lowerCall (std::move (e), out, false);

            if (userFunction || ! returnsValue)
                out.push_back (makeExprStatement (lowered.loc, std::move (lowered)));
            else if (isSideEffectBuiltin (name))
                out.push_back (makeExprStatement (lowered.loc, phony (std::move (lowered))));

            // Other builtins without effects are dropped along with their unused value
            return;
        }

        // Anything else only matters for the side effects of its operands
        if (hasEffects (e))
        {
            forEachChildExpr (e, [this, &out] (Expr& child)
            {
                lowerEffect (std::move (child), out);
            });
        }
    }

    //==========================================================================
    // Statements
    //==========================================================================

    Statement lowerToBlock (Statement& s)
    {
        ScopeGuard scope (*this);
        Statements body;

        if (s.is<StmtCompound>())
        {
            for (auto& child : s.as<StmtCompound>().statements)
                lowerStatement (child, body);
        }
        else
        {
            lowerStatement (s, body);
        }

        return makeBlock (s.loc, std::move (body));
    }

    void lowerStatement (Statement& s, Statements& out)
    {
        const auto l = s.loc;

        if (s.is<StmtCompound>())
        {
            out.push_back (lowerToBlock (s));
        }
        else if (s.is<StmtExpr>())
        {
            if (s.as<StmtExpr>().expr != nullptr)
            {
                if (isPointSizeWrite (*s.as<StmtExpr>().expr))
                    return;

                lowerEffect (std::move (*s.as<StmtExpr>().expr), out);
            }
        }
        else if (s.is<StmtDeclaration>())
        {
            lowerDeclaration (s.as<StmtDeclaration>().declaration, out);
        }
        else if (s.is<StmtSelection>())
        {
            auto& sel = s.as<StmtSelection>();
            auto condition = lowerValue (std::move (*sel.condition), out);
            auto thenBranch = lowerToBlock (*sel.thenBranch);

            std::optional<Statement> elseBranch;
            if (sel.elseBranch != nullptr)
            {
                ScopeGuard scope (*this);
                Statements elseStatements;
                lowerStatement (*sel.elseBranch, elseStatements);

                if (elseStatements.size() == 1 && elseStatements.front().is<StmtSelection>())
                    elseBranch = std::move (elseStatements.front());
                else if (elseStatements.size() == 1 && elseStatements.front().is<StmtCompound>())
                    elseBranch = std::move (elseStatements.front());
                else
                    elseBranch = makeBlock (l, std::move (elseStatements));
            }

            out.push_back (makeIf (l, std::move (condition), std::move (thenBranch), std::move (elseBranch)));
        }
        else if (s.is<StmtJump>())
        {
            auto& jump = s.as<StmtJump>();
            if (jump.returnValue != nullptr)
                *jump.returnValue = lowerValue (std::move (*jump.returnValue), out);

            out.push_back (std::move (s));
        }
        else if (s.is<StmtWhile>())
        {
            lowerWhile (s, out);
        }
        else if (s.is<StmtDoWhile>())
        {
            lowerDoWhile (s, out);
        }
        else if (s.is<StmtFor>())
        {
            lowerFor (s, out);
        }
        else if (s.is<StmtSwitch>())
        {
            lowerSwitch (s, out);
        }
        else if (s.is<StmtCaseLabel>())
        {
            throw LoweringError (l, "Case label outside of a switch statement");
        }
        else if (s.is<StmtLoop>())
        {
            out.push_back (std::move (s));
        }
    }

    void lowerDeclaration (Declaration& decl, Statements& out)
    {
        if (decl.initDeclaratorList == nullptr)
            return;

        auto& list = *decl.initDeclaratorList;
        const bool isConst = list.qualifier != nullptr && list.qualifier->hasStorage (StorageQualifier::constQual);

        for (auto& single : list.declarations)
        {
            const auto type = declaratorType (list.type, single.arraySpecifiers);
            std::optional<Expr> init;

            if (single.initializer != nullptr && single.initializer->expr != nullptr)
                init = lowerValue (std::move (*single.initializer->expr), out);

            // A constant expression stays a WGSL const, usable as an array size or case label; anything else is a let
            const bool isConstantExpression = isConst && init.has_value() && isWgslConstantExpression (*init);

            auto declaration = makeVarDeclaration (single.loc, single.name, type, std::move (init), isConst);
            declaration.as<StmtDeclaration>().declaration.initDeclaratorList->isLet = isConst && ! isConstantExpression;
            out.push_back (std::move (declaration));

            VarInfo info;
            info.kind = isConst ? VarKind::constant : VarKind::local;
            info.isWgslConst = isConstantExpression;
            info.type = type;
            declare (single.name, info);
        }
    }

    /** gl_PointSize has no WGSL equivalent: writing 1.0 (the only size WebGPU draws points with) is dropped, anything else fails. */
    bool isPointSizeWrite (const Expr& e)
    {
        const auto& node = unparen (e);
        if (! node.is<ExprAssignment>())
            return false;

        const auto& assign = node.as<ExprAssignment>();
        if (! unparen (*assign.lhs).is<ExprVariable>() || unparen (*assign.lhs).as<ExprVariable>().name != "gl_PointSize")
            return false;

        const auto& value = unparen (*assign.rhs);
        const bool isOne = assign.op == AssignmentOp::assign
                        && ((value.is<ExprFloatConst>() && value.as<ExprFloatConst>().value == 1.0)
                            || (value.is<ExprIntConst>() && value.as<ExprIntConst>().value == 1)
                            || (value.is<ExprTypeConstructor>() && value.as<ExprTypeConstructor>().args.size() == 1
                                && unparen (value.as<ExprTypeConstructor>().args[0]).is<ExprIntConst>()
                                && unparen (value.as<ExprTypeConstructor>().args[0]).as<ExprIntConst>().value == 1));

        if (! isOne)
            throw LoweringError (e.loc, "gl_PointSize has no WGSL equivalent; only writing 1.0 is supported");

        context.warnings.push_back (formatDiagnostic (e.loc, "gl_PointSize = 1.0 has no WGSL equivalent and is dropped"));
        return true;
    }

    //==========================================================================
    // Loops
    //==========================================================================

    Expr lowerCondition (Expr condition, Statements& pre)
    {
        return lowerValue (std::move (condition), pre);
    }

    static Statement breakUnless (SourceLocation l, Expr condition)
    {
        auto negated = makeUnary (l, UnaryOp::logicalNot, makeParen (l, std::move (condition)), makeType (TypeKind::boolType));
        Statements body;
        body.push_back (makeJumpStatement (l, JumpKind::breakJump));
        return makeIf (l, std::move (negated), makeBlock (l, std::move (body)));
    }

    /** The body stays a nested block: WGSL's continuing block sees the loop body's declarations, GLSL's condition doesn't. */
    void appendLoweredBody (Statement& body, Statements& out)
    {
        out.push_back (lowerToBlock (body));
    }

    void lowerWhile (Statement& s, Statements& out)
    {
        auto& w = s.as<StmtWhile>();
        const auto l = s.loc;

        ScopeGuard scope (*this);
        Statements conditionPre;
        auto condition = lowerCondition (std::move (*w.condition), conditionPre);

        if (conditionPre.empty())
        {
            *w.condition = std::move (condition);
            *w.body = lowerToBlock (*w.body);
            out.push_back (std::move (s));
            return;
        }

        // Statements the condition needs run at the top of every iteration
        StmtLoop loop;
        loop.loc = l;
        loop.body = std::move (conditionPre);
        loop.body.push_back (breakUnless (l, std::move (condition)));
        appendLoweredBody (*w.body, loop.body);

        out.push_back (makeStatement (l, std::move (loop)));
    }

    void lowerDoWhile (Statement& s, Statements& out)
    {
        auto& dw = s.as<StmtDoWhile>();
        const auto l = s.loc;

        ScopeGuard scope (*this);
        StmtLoop loop;
        loop.loc = l;

        appendLoweredBody (*dw.body, loop.body);

        // continue jumps to the continuing block, which evaluates the condition like GLSL does
        auto condition = lowerCondition (std::move (*dw.condition), loop.continuing);
        loop.breakIf = boxed (makeUnary (l, UnaryOp::logicalNot, makeParen (l, std::move (condition)), makeType (TypeKind::boolType)));

        out.push_back (makeStatement (l, std::move (loop)));
    }

    /** A statement WGSL accepts in a for header: one declaration, assignment, increment or call. */
    static bool isForHeaderStatement (const Statement& s)
    {
        if (s.is<StmtDeclaration>())
        {
            const auto& list = s.as<StmtDeclaration>().declaration.initDeclaratorList;
            return list != nullptr && list->declarations.size() == 1;
        }

        if (! s.is<StmtExpr>())
            return false;

        const auto& e = *s.as<StmtExpr>().expr;
        return e.is<ExprAssignment>() || e.is<ExprFunCall>() || (e.is<ExprUnary>() && isIncDec (e.as<ExprUnary>().op));
    }

    void lowerFor (Statement& s, Statements& out)
    {
        auto& f = s.as<StmtFor>();
        const auto l = s.loc;

        ScopeGuard scope (*this);

        Statements init;
        if (f.init != nullptr)
            lowerStatement (*f.init, init);

        Statements conditionPre;
        std::optional<Expr> condition;
        if (f.condition != nullptr)
            condition = lowerCondition (std::move (*f.condition), conditionPre);

        Statements update;
        if (f.update != nullptr)
        {
            ScopeGuard updateScope (*this);
            lowerEffect (std::move (*f.update), update);
        }

        const bool simple = init.size() <= 1 && (init.empty() || isForHeaderStatement (init.front()))
                         && conditionPre.empty()
                         && update.size() <= 1 && (update.empty() || isForHeaderStatement (update.front()));

        if (simple)
        {
            StmtFor lowered;
            lowered.loc = l;

            if (! init.empty())
                lowered.init = std::make_unique<Statement> (std::move (init.front()));

            if (condition.has_value())
                lowered.condition = boxed (std::move (*condition));

            if (! update.empty())
                lowered.update = std::move (update.front().as<StmtExpr>().expr);

            lowered.body = std::make_unique<Statement> (lowerToBlock (*f.body));
            out.push_back (makeStatement (l, std::move (lowered)));
            return;
        }

        // { init; loop { condition; if (!cond) { break; } body continuing { update } } }
        StmtLoop loop;
        loop.loc = l;

        for (auto& p : conditionPre)
            loop.body.push_back (std::move (p));

        if (condition.has_value())
            loop.body.push_back (breakUnless (l, std::move (*condition)));

        appendLoweredBody (*f.body, loop.body);
        loop.continuing = std::move (update);

        Statements block = std::move (init);
        block.push_back (makeStatement (l, std::move (loop)));
        out.push_back (makeBlock (l, std::move (block)));
    }

    //==========================================================================
    // Switch
    //==========================================================================

    static bool endsWithJump (const Statements& statements)
    {
        if (statements.empty())
            return false;

        const auto& last = statements.back();

        if (last.is<StmtJump>())
            return true;

        if (last.is<StmtCompound>())
            return endsWithJump (last.as<StmtCompound>().statements);

        return false;
    }

    void lowerSwitch (Statement& s, Statements& out)
    {
        auto& sw = s.as<StmtSwitch>();
        const auto l = s.loc;

        *sw.selector = lowerValue (std::move (*sw.selector), out);

        // Group the parsed body: consecutive labels share the statements that follow them
        struct Group
        {
            SourceLocation loc;
            std::vector<std::unique_ptr<Expr>> labels;
            Statements statements;
        };

        std::vector<Group> groups;
        bool lastWasLabel = false;

        for (auto& child : sw.body)
        {
            if (child.is<StmtCaseLabel>())
            {
                if (! lastWasLabel)
                    groups.push_back ({ child.loc, {}, {} });

                groups.back().labels.push_back (std::move (child.as<StmtCaseLabel>().label));
                lastWasLabel = true;
            }
            else
            {
                // Statements before the first label are unreachable
                if (! groups.empty())
                    groups.back().statements.push_back (std::move (child));

                lastWasLabel = false;
            }
        }

        sw.body.clear();

        bool hasDefault = false;

        for (size_t i = 0; i < groups.size(); ++i)
        {
            // WGSL has no fallthrough: a group that doesn't end in a jump runs the following groups' statements too
            Statements statements;
            for (size_t j = i; j < groups.size(); ++j)
            {
                for (const auto& child : groups[j].statements)
                    statements.push_back (copyStatement (child));

                if (endsWithJump (groups[j].statements))
                    break;
            }

            // break at the end of a clause is implicit
            if (! statements.empty() && statements.back().is<StmtJump>() && statements.back().as<StmtJump>().kind == JumpKind::breakJump)
                statements.pop_back();

            SwitchClause clause;
            clause.loc = groups[i].loc;

            for (auto& label : groups[i].labels)
            {
                hasDefault = hasDefault || label == nullptr;
                clause.labels.push_back (label != nullptr ? boxed (lowerValue (std::move (*label), out)) : nullptr);
            }

            {
                ScopeGuard scope (*this);
                for (auto& child : statements)
                    lowerStatement (child, clause.body);
            }

            sw.clauses.push_back (std::move (clause));
        }

        if (! hasDefault)
        {
            SwitchClause clause;
            clause.loc = l;
            clause.labels.push_back (nullptr);
            sw.clauses.push_back (std::move (clause));
        }

        out.push_back (std::move (s));
    }

    //==========================================================================
    TranslationUnit& ast;
    WgslLoweringContext& context;

    std::map<std::string, const FunctionPrototype*> functions;
    std::map<std::string, VarInfo> globals;
    std::vector<std::map<std::string, VarInfo>> scopes;
    std::map<std::string, std::string> samplerParameters;
};

} // namespace

//==============================================================================
void WgslStatementLowering::lower (TranslationUnit& ast, WgslLoweringContext& context)
{
    StatementLowerer (ast, context).run();
}

} // namespace wgsl
} // namespace yup
