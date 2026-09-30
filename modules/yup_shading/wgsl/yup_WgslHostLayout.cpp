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

enum class BlockLayout
{
    std140,
    std430
};

struct SizeAlign
{
    uint32_t align = 4;
    uint32_t size = 4;
};

uint32_t roundUp (uint32_t alignment, uint32_t value)
{
    return (value + alignment - 1) / alignment * alignment;
}

bool isBoolish (TypeKind kind)
{
    return scalarKindOf (kind) == TypeKind::boolType;
}

bool isAtomicKind (TypeKind kind)
{
    return kind == TypeKind::atomicI32 || kind == TypeKind::atomicU32;
}

std::string identifierFrom (const std::string& text)
{
    std::string id;
    for (const char c : text)
    {
        const bool alnum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (alnum)
            id += c;
        else if (id.empty() || id.back() != '_')
            id += '_';
    }

    while (! id.empty() && id.back() == '_')
        id.pop_back();

    return id;
}

//==============================================================================
class HostLayouter
{
public:
    HostLayouter (TranslationUnit& unit, WgslLoweringContext& ctx)
        : ast (unit)
        , context (ctx)
    {
    }

    void run()
    {
        collectDeclarations();
        collectAtomics();
        layoutGlobals();

        if (hostGlobals.empty())
            return;

        insertGeneratedStructs();
        rewriteAccesses();
    }

private:
    //==========================================================================
    // Collection
    //==========================================================================

    void collectDeclarations()
    {
        for (auto& external : ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr)
                continue;

            if (d->structSpecifier != nullptr)
                structs[d->structSpecifier->name] = d->structSpecifier.get();

            if (d->initDeclaratorList == nullptr || d->structSpecifier != nullptr)
                continue;

            const auto& list = *d->initDeclaratorList;
            if (list.qualifier == nullptr || ! list.qualifier->hasStorage (StorageQualifier::constQual))
                continue;

            for (const auto& single : list.declarations)
                if (single.initializer != nullptr && single.initializer->expr != nullptr)
                    if (const auto value = evaluateIntConstant (*single.initializer->expr))
                        constants[single.name] = *value;
        }
    }

    bool isResourceBlockDeclaration (const Declaration& d) const
    {
        return d.structSpecifier != nullptr && d.initDeclaratorList != nullptr && d.qualifier != nullptr
            && (d.qualifier->hasStorage (StorageQualifier::uniform) || d.qualifier->hasStorage (StorageQualifier::buffer));
    }

    /** Marks the storage that GLSL atomic functions operate on. */
    void collectAtomics()
    {
        std::map<std::string, std::string> blockInstances; // instance -> block struct
        std::set<std::string> sharedVariables;

        for (auto& external : ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->initDeclaratorList == nullptr)
                continue;

            if (isResourceBlockDeclaration (*d))
            {
                blockInstances[d->initDeclaratorList->declarations.front().name] = d->structSpecifier->name;
            }
            else if (d->initDeclaratorList->qualifier != nullptr && d->initDeclaratorList->qualifier->hasStorage (StorageQualifier::shared))
            {
                for (const auto& single : d->initDeclaratorList->declarations)
                    sharedVariables.insert (single.name);
            }
        }

        forEachFunctionExpr ([&] (Expr& e)
        {
            if (! e.is<ExprFunCall>())
                return;

            const auto& call = e.as<ExprFunCall>();
            if (call.callee == nullptr || ! call.callee->is<ExprVariable>() || call.args.empty())
                return;

            const auto& name = call.callee->as<ExprVariable>().name;
            static const std::set<std::string> atomics = { "atomicAdd", "atomicMin", "atomicMax", "atomicAnd", "atomicOr",
                                                           "atomicXor", "atomicExchange", "atomicCompSwap" };
            if (atomics.count (name) == 0)
                return;

            // mem, mem[i], block.member or block.member[i]
            const Expr* target = &call.args[0];
            while (target->is<ExprParen>())
                target = target->as<ExprParen>().expr.get();

            if (target->is<ExprBracket>())
                target = target->as<ExprBracket>().base.get();

            if (target->is<ExprVariable>() && sharedVariables.count (target->as<ExprVariable>().name) > 0)
            {
                atomicGlobals.insert (target->as<ExprVariable>().name);
                return;
            }

            if (target->is<ExprDot>() && target->as<ExprDot>().base->is<ExprVariable>())
            {
                const auto& instance = target->as<ExprDot>().base->as<ExprVariable>().name;
                if (auto found = blockInstances.find (instance); found != blockInstances.end())
                {
                    atomicFields.insert ({ found->second, target->as<ExprDot>().member });
                    return;
                }
            }

            throw LoweringError (e.loc, "Atomic functions need a buffer block member or a shared variable in WGSL");
        });
    }

    template <typename F>
    void forEachFunctionExpr (F&& f)
    {
        std::function<void (Statement&)> visitStatement = [&] (Statement& s)
        {
            const auto visit = [&f] (Expr& root)
            {
                walkExpr (root, f);
            };

            if (s.is<StmtDeclaration>())
                walkDeclarationExprs (s.as<StmtDeclaration>().declaration, f);
            else
                forEachOwnExpr (s, visit);

            forEachChildStatement (s, visitStatement);
        };

        for (auto& external : ast.declarations)
            if (auto* fd = std::get_if<FunctionDefinition> (&external); fd != nullptr && fd->body != nullptr)
                visitStatement (*fd->body);
    }

    //==========================================================================
    // Sizes
    //==========================================================================

    uint32_t arrayLength (const ArraySpecifier& array, const SourceLocation& loc) const
    {
        if (array.isUnsized || array.sizeExpr == nullptr)
            return 0;

        if (const auto value = evaluateIntConstant (*array.sizeExpr))
            return static_cast<uint32_t> (*value);

        if (array.sizeExpr->is<ExprVariable>())
            if (auto found = constants.find (array.sizeExpr->as<ExprVariable>().name); found != constants.end())
                return static_cast<uint32_t> (found->second);

        throw LoweringError (loc, "Array sizes in buffers must be integer constants");
    }

    static SizeAlign vectorSize (int count)
    {
        switch (count)
        {
            case 1:
                return { 4, 4 };
            case 2:
                return { 8, 8 };
            case 3:
                return { 16, 12 };
            default:
                return { 16, 16 };
        }
    }

    /** Size and alignment GLSL gives a type in a block of the given layout. */
    SizeAlign glslSize (const TypeSpecifier& type, BlockLayout layout, const SourceLocation& loc)
    {
        if (! type.arraySpecifiers.empty())
        {
            const auto element = glslSize (elementType (type), layout, loc);
            const auto align = layout == BlockLayout::std140 ? roundUp (16, element.align) : element.align;
            const auto stride = roundUp (align, element.size);
            const auto count = std::max (1u, arrayLength (type.arraySpecifiers.front(), loc));
            return { align, stride * count };
        }

        if (type.kind == TypeKind::namedStruct)
            return layoutStruct (type.structName, layout, loc).glsl;

        if (const auto [columns, rows] = matrixShape (type.kind); columns > 0)
        {
            const auto column = vectorSize (rows);
            const auto align = layout == BlockLayout::std140 ? 16u : column.align;
            return { align, static_cast<uint32_t> (columns) * roundUp (align, column.size) };
        }

        if (isAtomicKind (type.kind))
            return { 4, 4 };

        const auto count = componentCount (type.kind);
        if (count == 0)
            throw LoweringError (loc, "Type '" + glslTypeName (type.kind) + "' can't be stored in a buffer");

        return vectorSize (count);
    }

    /** Size and alignment WGSL gives a host type in its natural layout. */
    SizeAlign wgslSize (const TypeSpecifier& type, const SourceLocation& loc)
    {
        if (! type.arraySpecifiers.empty())
        {
            const auto element = wgslSize (elementType (type), loc);
            const auto stride = roundUp (element.align, element.size);
            const auto count = std::max (1u, arrayLength (type.arraySpecifiers.front(), loc));
            return { element.align, stride * count };
        }

        if (type.kind == TypeKind::namedStruct)
        {
            if (auto found = hostSizes.find (type.structName); found != hostSizes.end())
                return found->second;

            throw LoweringError (loc, "Unknown struct '" + type.structName + "' in a buffer");
        }

        if (const auto [columns, rows] = matrixShape (type.kind); columns > 0)
        {
            const auto column = vectorSize (rows);
            return { column.align, static_cast<uint32_t> (columns) * roundUp (column.align, column.size) };
        }

        if (isAtomicKind (type.kind))
            return { 4, 4 };

        return vectorSize (componentCount (type.kind));
    }

    //==========================================================================
    // Host types
    //==========================================================================

    std::string wrapperFor (const TypeSpecifier& inner)
    {
        const auto name = "_std140_" + identifierFrom (wgslTypeName (inner.kind));

        if (wrappers.count (name) == 0)
        {
            auto ss = std::make_unique<StructSpecifier>();
            ss->name = name;

            StructFieldSpecifier field;
            field.type = inner;
            field.name = "v";
            field.sizeAttribute = 16;
            ss->fields.push_back (std::move (field));

            hostSizes[name] = { vectorSize (componentCount (inner.kind)).align, 16 };
            wrappers[name] = inner;
            structs[name] = ss.get();
            generated.push_back (std::move (ss));
        }

        return name;
    }

    /** How GLSL type is stored in host-shareable memory of the given layout. */
    TypeSpecifier hostType (const TypeSpecifier& glsl, BlockLayout layout, bool isAtomic, const SourceLocation& loc)
    {
        auto element = elementType (glsl);
        while (! element.arraySpecifiers.empty())
            element = elementType (element);

        TypeSpecifier base = element;
        std::vector<ArraySpecifier> innerDimensions;

        if (isAtomic)
        {
            if (element.kind == TypeKind::intType)
                base = makeType (TypeKind::atomicI32);
            else if (element.kind == TypeKind::uintType)
                base = makeType (TypeKind::atomicU32);
            else
                throw LoweringError (loc, "Atomic functions need int or uint storage");
        }
        else if (element.kind == TypeKind::namedStruct)
        {
            base = TypeSpecifier::makeNamed (loc, layoutStruct (element.structName, layout, loc).hostName);
        }
        else if (isBoolish (element.kind))
        {
            base = makeType (vectorKind (TypeKind::uintType, componentCount (element.kind)));
        }
        else if (const auto [columns, rows] = matrixShape (element.kind); columns > 0 && rows == 2 && layout == BlockLayout::std140)
        {
            // std140 strides matrix columns by 16 bytes, WGSL packs vec2 columns in 8
            base = TypeSpecifier::makeNamed (loc, wrapperFor (makeType (TypeKind::vec2)));
            innerDimensions.push_back (makeArrayType (makeType (TypeKind::floatType), columns).arraySpecifiers.front());
        }

        // std140 strides array elements by 16 bytes: scalars and vec2 need a wrapper
        const bool small = ! isAtomic && componentCount (base.kind) >= 1 && componentCount (base.kind) <= 2 && innerDimensions.empty();
        if (! glsl.arraySpecifiers.empty() && layout == BlockLayout::std140 && small)
            base = TypeSpecifier::makeNamed (loc, wrapperFor (base));

        base.arraySpecifiers = glsl.arraySpecifiers;
        base.arraySpecifiers.insert (base.arraySpecifiers.end(), innerDimensions.begin(), innerDimensions.end());
        return base;
    }

    struct StructLayout
    {
        std::string hostName;
        SizeAlign glsl;
    };

    /** Computes GLSL member offsets and gives the struct the WGSL form that reproduces them. */
    StructLayout layoutStruct (const std::string& name, BlockLayout layout, const SourceLocation& loc, bool inPlace = false)
    {
        const auto key = std::make_pair (name, layout);
        if (auto found = structLayouts.find (key); found != structLayouts.end())
            return found->second;

        auto found = structs.find (name);
        if (found == structs.end())
            throw LoweringError (loc, "Unknown struct '" + name + "'");

        const auto& ss = *found->second;
        const auto& fields = ss.fields;

        std::vector<TypeSpecifier> hostTypes;
        std::vector<uint32_t> offsets;
        uint32_t cursor = 0;
        uint32_t maxAlign = 1;

        for (const auto& field : fields)
        {
            const bool isAtomic = atomicFields.count ({ name, field.name }) > 0;
            auto fieldType = field.type;
            if (isAtomic)
                fieldType = hostType (field.type, layout, true, field.loc);

            const auto size = glslSize (isAtomic ? fieldType : field.type, layout, field.loc);
            auto align = size.align;

            if (const auto explicitAlign = qualifierValue (field.qualifier.get(), LayoutQualifierId::align))
                align = std::max (align, *explicitAlign);

            auto offset = roundUp (align, cursor);

            if (const auto explicitOffset = qualifierValue (field.qualifier.get(), LayoutQualifierId::offset))
            {
                if (*explicitOffset < cursor || *explicitOffset % align != 0)
                    throw LoweringError (field.loc, "layout(offset = " + std::to_string (*explicitOffset) + ") of '" + field.name + "' is not valid for its type");

                offset = *explicitOffset;
            }

            offsets.push_back (offset);
            hostTypes.push_back (hostType (field.type, layout, isAtomic, field.loc));
            cursor = offset + size.size;
            maxAlign = std::max (maxAlign, align);
        }

        const auto structAlign = layout == BlockLayout::std140 ? roundUp (16, maxAlign) : maxAlign;
        const auto structSize = roundUp (structAlign, cursor);

        if (! offsets.empty() && offsets.front() != 0)
            throw LoweringError (fields.front().loc, "WGSL can't place the first member of '" + name + "' at a non-zero offset");

        // WGSL places each member at the next multiple of its alignment: @size fills the gaps GLSL leaves
        std::vector<uint32_t> sizeAttributes (fields.size(), 0);
        uint32_t hostAlign = 1;
        bool changed = false;

        for (size_t i = 0; i < fields.size(); ++i)
        {
            const auto natural = wgslSize (hostTypes[i], fields[i].loc);
            hostAlign = std::max (hostAlign, natural.align);
            changed = changed || ! sameType (hostTypes[i], fields[i].type);

            if (offsets[i] % natural.align != 0)
                throw LoweringError (fields[i].loc, "WGSL can't place '" + fields[i].name + "' at its GLSL offset " + std::to_string (offsets[i]));

            const bool isRuntimeArray = ! hostTypes[i].arraySpecifiers.empty() && hostTypes[i].arraySpecifiers.front().isUnsized;
            if (isRuntimeArray)
                continue;

            const auto end = i + 1 < fields.size() ? offsets[i + 1] : structSize;
            const auto desired = end - offsets[i];

            if (desired < natural.size)
                throw LoweringError (fields[i].loc, "WGSL can't reproduce the GLSL layout of '" + fields[i].name + "'");

            if (desired != natural.size)
            {
                sizeAttributes[i] = desired;
                changed = true;
            }
        }

        StructLayout result;
        result.glsl = { structAlign, structSize };
        result.hostName = name;

        if (inPlace || changed)
        {
            StructSpecifier* target = found->second;

            if (! inPlace)
            {
                auto copy = copyStructSpecifier (found->second);
                copy->name = context.names.allocate (name + (layout == BlockLayout::std140 ? "_std140" : "_std430"));
                target = copy.get();
                structs[copy->name] = copy.get();
                generated.push_back (std::move (copy));
            }

            for (size_t i = 0; i < fields.size(); ++i)
            {
                target->fields[i].type = hostTypes[i];
                target->fields[i].sizeAttribute = sizeAttributes[i];
            }

            result.hostName = target->name;
        }

        hostSizes[result.hostName] = { hostAlign, roundUp (hostAlign, structSize) };

        if (hostSizes[result.hostName].size != structSize)
            throw LoweringError (ss.loc, "WGSL can't reproduce the size of struct '" + name + "'");

        structLayouts[key] = result;
        return result;
    }

    static std::optional<uint32_t> qualifierValue (const TypeQualifier* q, LayoutQualifierId id)
    {
        if (q == nullptr || q->layout == nullptr)
            return std::nullopt;

        for (const auto& entry : q->layout->entries)
        {
            if (entry.id == id && entry.value != nullptr)
            {
                const auto value = evaluateIntConstant (*entry.value);
                if (! value.has_value() || *value < 0)
                    throw LoweringError (entry.loc, "layout(" + entry.name + ") must be a non-negative integer constant");

                return static_cast<uint32_t> (*value);
            }
        }

        return std::nullopt;
    }

    static BlockLayout blockLayoutOf (const TypeQualifier& q, bool isBuffer)
    {
        bool std140 = false;
        bool std430 = false;

        if (q.layout != nullptr)
        {
            for (const auto& entry : q.layout->entries)
            {
                std140 = std140 || entry.id == LayoutQualifierId::std140;
                std430 = std430 || entry.id == LayoutQualifierId::std430;
            }
        }

        if (std430 && ! isBuffer)
            throw LoweringError (q.loc, "std430 uniform blocks are not supported for WGSL");

        if (std140)
            return BlockLayout::std140;

        return isBuffer ? BlockLayout::std430 : BlockLayout::std140;
    }

    //==========================================================================
    // Globals
    //==========================================================================

    void layoutGlobals()
    {
        for (auto& external : ast.declarations)
        {
            auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->initDeclaratorList == nullptr)
                continue;

            auto& list = *d->initDeclaratorList;
            const auto* q = list.qualifier.get();

            if (isResourceBlockDeclaration (*d))
            {
                const bool isBuffer = d->qualifier->hasStorage (StorageQualifier::buffer);
                const auto layout = blockLayoutOf (*d->qualifier, isBuffer);
                layoutStruct (d->structSpecifier->name, layout, d->loc, true);

                const auto& instance = list.declarations.front().name;
                hostGlobals[instance] = list.type;
                continue;
            }

            if (q == nullptr || d->structSpecifier != nullptr)
                continue;

            const bool isUniform = q->hasStorage (StorageQualifier::uniform);
            const bool isShared = q->hasStorage (StorageQualifier::shared);

            if (! isShared && ! (isUniform && ! isOpaqueType (list.type.kind)))
                continue;

            for (auto& single : list.declarations)
            {
                const auto glsl = declaratorType (list.type, single.arraySpecifiers);
                const bool isAtomic = atomicGlobals.count (single.name) > 0;

                // Workgroup memory keeps its natural layout, only atomics change type
                if (isShared && ! isAtomic)
                    continue;

                const auto host = isShared ? hostType (glsl, BlockLayout::std430, true, single.loc)
                                           : hostType (glsl, BlockLayout::std140, false, single.loc);

                hostGlobals[single.name] = host;

                if (sameType (host, glsl))
                    continue;

                if (list.declarations.size() > 1)
                    throw LoweringError (single.loc, "Declare '" + single.name + "' on its own: its storage type differs in WGSL");

                list.type = host;
                single.arraySpecifiers.clear();
            }
        }
    }

    void insertGeneratedStructs()
    {
        std::vector<ExternalDeclaration> merged;

        for (auto& ss : generated)
        {
            Declaration d;
            d.loc = ss->loc;
            d.structSpecifier = std::move (ss);
            merged.push_back (std::move (d));
        }

        generated.clear();

        for (auto& external : ast.declarations)
            merged.push_back (std::move (external));

        ast.declarations = std::move (merged);
    }

    //==========================================================================
    // Conversions
    //==========================================================================

    std::string typeText (const TypeSpecifier& type, const SourceLocation& loc)
    {
        std::string s = type.kind == TypeKind::namedStruct ? type.structName : wgslTypeName (type.kind);

        for (auto it = type.arraySpecifiers.rbegin(); it != type.arraySpecifiers.rend(); ++it)
            s = it->isUnsized ? "array<" + s + ">" : "array<" + s + ", " + std::to_string (arrayLength (*it, loc)) + ">";

        return s;
    }

    /** WGSL text converting value between the GLSL type and its stored form, in either direction. */
    std::string convertText (const std::string& value, const TypeSpecifier& glsl, const TypeSpecifier& host, bool load, const SourceLocation& loc)
    {
        if (sameType (glsl, host))
            return value;

        if (host.arraySpecifiers.empty() && isAtomicKind (host.kind))
            throw LoweringError (loc, "Atomic storage can only be read and written as a whole value");

        // Value conversions map non-zero to true and true to 1u
        if (glsl.arraySpecifiers.empty() && isBoolish (glsl.kind))
            return wgslTypeName (load ? glsl.kind : host.kind) + "(" + value + ")";

        return converterFunction (glsl, host, load, loc) + "(" + value + ")";
    }

    std::string converterFunction (const TypeSpecifier& glsl, const TypeSpecifier& host, bool load, const SourceLocation& loc)
    {
        const auto glslText = typeText (glsl, loc);
        const auto hostText = typeText (host, loc);
        const auto name = std::string (load ? "_load_" : "_store_") + identifierFrom (hostText + "_as_" + glslText);

        if (context.polyfillNames.count (name) > 0)
            return name;

        const auto fromText = load ? hostText : glslText;
        const auto toText = load ? glslText : hostText;
        std::string body;

        if (! glsl.arraySpecifiers.empty())
        {
            const auto glslElement = elementType (glsl);
            auto hostElement = elementType (host);
            std::string access = "x[i]";
            std::string target = "r[i]";

            if (hostElement.arraySpecifiers.empty() && hostElement.kind == TypeKind::namedStruct && wrappers.count (hostElement.structName) > 0)
            {
                (load ? access : target) += ".v";
                hostElement = wrappers[hostElement.structName];
            }

            const auto length = arrayLength (glsl.arraySpecifiers.front(), loc);
            body = "    var r: " + toText + ";\n"
                 + "    for (var i = 0u; i < " + std::to_string (length) + "u; i++) {\n"
                 + "        " + target + " = " + convertText (access, glslElement, hostElement, load, loc) + ";\n"
                 + "    }\n"
                 + "    return r;\n";
        }
        else if (isMatrixType (glsl.kind))
        {
            // Two-row std140 matrix stored as wrapped columns
            const auto columns = matrixShape (glsl.kind).first;
            const auto wrapper = elementType (host).structName;
            std::string parts;

            for (int c = 0; c < columns; ++c)
                parts += (c > 0 ? ", " : "") + (load ? "x[" + std::to_string (c) + "].v" : wrapper + "(x[" + std::to_string (c) + "])");

            body = "    return " + toText + "(" + parts + ");\n";
        }
        else if (glsl.kind == TypeKind::namedStruct)
        {
            const auto& glslStruct = *structs.at (glsl.structName);
            const auto& hostStruct = *structs.at (host.structName);
            std::string parts;

            for (size_t i = 0; i < glslStruct.fields.size(); ++i)
            {
                const auto& field = glslStruct.fields[i];
                parts += (i > 0 ? ", " : "") + convertText ("x." + field.name, field.type, hostStruct.fields[i].type, load, loc);
            }

            body = "    return " + toText + "(" + parts + ");\n";
        }
        else
        {
            throw LoweringError (loc, "Unsupported storage conversion for " + glslText);
        }

        context.polyfillNames.insert (name);
        context.polyfills.push_back ("fn " + name + "(x: " + fromText + ") -> " + toText + " {\n" + body + "}\n");
        return name;
    }

    //==========================================================================
    // Accesses
    //==========================================================================

    bool isHostRoot (const std::string& name) const
    {
        if (hostGlobals.count (name) == 0)
            return false;

        for (const auto& scope : scopes)
            if (scope.count (name) > 0)
                return false;

        return true;
    }

    /** If e names host memory, returns its stored type, wrapping element accesses of std140 arrays as needed. */
    std::optional<TypeSpecifier> visitLocation (Expr& e)
    {
        if (e.is<ExprParen>())
            return visitLocation (*e.as<ExprParen>().expr);

        if (e.is<ExprVariable>())
        {
            const auto& name = e.as<ExprVariable>().name;
            if (isHostRoot (name))
                return hostGlobals.at (name);

            return std::nullopt;
        }

        if (e.is<ExprDot>())
        {
            auto& dot = e.as<ExprDot>();
            const auto base = visitLocation (*dot.base);
            if (! base.has_value())
                return std::nullopt;

            if (base->arraySpecifiers.empty() && base->kind == TypeKind::namedStruct)
            {
                for (const auto& field : structs.at (base->structName)->fields)
                    if (field.name == dot.member)
                        return field.type;

                throw LoweringError (e.loc, "Unknown member '" + dot.member + "'");
            }

            if (base->arraySpecifiers.empty() && componentCount (base->kind) > 1)
                return makeType (vectorKind (scalarKindOf (base->kind), static_cast<int> (dot.member.size())));

            return std::nullopt;
        }

        if (e.is<ExprBracket>())
        {
            auto& bracket = e.as<ExprBracket>();
            visitValue (*bracket.index);

            const auto base = visitLocation (*bracket.base);
            if (! base.has_value())
                return std::nullopt;

            if (! base->arraySpecifiers.empty())
            {
                auto element = elementType (*base);

                if (element.arraySpecifiers.empty() && element.kind == TypeKind::namedStruct && wrappers.count (element.structName) > 0)
                {
                    const auto inner = wrappers[element.structName];
                    auto glslType = e.type;
                    e.type = element;
                    const auto l = e.loc;
                    e = makeDot (l, std::move (e), "v", std::move (glslType));
                    return inner;
                }

                return element;
            }

            if (isMatrixType (base->kind))
                return makeType (vectorKind (TypeKind::floatType, matrixShape (base->kind).second));

            if (componentCount (base->kind) > 1)
                return makeType (scalarKindOf (base->kind));

            return std::nullopt;
        }

        visitValue (e);
        return std::nullopt;
    }

    void visitValue (Expr& e)
    {
        if (e.is<ExprVariable>() || e.is<ExprDot>() || e.is<ExprBracket>() || e.is<ExprParen>())
        {
            if (const auto host = visitLocation (e))
                loadWith (e, *host);

            return;
        }

        if (e.is<ExprAssignment>())
        {
            auto& assign = e.as<ExprAssignment>();
            const auto host = visitLocation (*assign.lhs);
            visitValue (*assign.rhs);

            if (! host.has_value() || ! assign.lhs->type.has_value())
                return;

            const auto glsl = *assign.lhs->type;
            const auto l = e.loc;

            if (host->arraySpecifiers.empty() && isAtomicKind (host->kind))
            {
                if (assign.op != AssignmentOp::assign)
                    throw LoweringError (l, "Compound assignment to atomic storage is not supported; use the atomic functions");

                std::vector<Expr> args;
                args.push_back (makeUnary (l, UnaryOp::addressOf, std::move (*assign.lhs)));
                args.push_back (std::move (*assign.rhs));
                e = makeCall (l, "atomicStore", std::move (args));
                return;
            }

            if (sameType (glsl, *host))
                return;

            if (assign.op != AssignmentOp::assign)
                throw LoweringError (l, "Compound assignment to converted buffer storage is not supported");

            *assign.rhs = makeHostConversion (std::move (*assign.rhs), glsl, *host, false);
            return;
        }

        if (e.is<ExprUnary>() && (e.as<ExprUnary>().op == UnaryOp::preInc || e.as<ExprUnary>().op == UnaryOp::preDec
                                  || e.as<ExprUnary>().op == UnaryOp::postInc || e.as<ExprUnary>().op == UnaryOp::postDec))
        {
            auto& operand = *e.as<ExprUnary>().operand;
            const auto host = visitLocation (operand);

            if (host.has_value() && operand.type.has_value() && (! sameType (*host, *operand.type) || isAtomicKind (host->kind)))
                throw LoweringError (e.loc, "++ and -- on atomic or converted buffer storage are not supported");

            return;
        }

        if (e.is<ExprFunCall>())
        {
            auto& call = e.as<ExprFunCall>();

            if (call.callee != nullptr && call.callee->is<ExprDot>())
            {
                // x.length() needs the stored array itself
                visitLocation (*call.callee->as<ExprDot>().base);
                return;
            }

            static const std::set<std::string> atomics = { "atomicAdd", "atomicMin", "atomicMax", "atomicAnd", "atomicOr",
                                                           "atomicXor", "atomicExchange", "atomicCompSwap" };

            const bool isAtomicCall = call.callee != nullptr && call.callee->is<ExprVariable>() && atomics.count (call.callee->as<ExprVariable>().name) > 0;

            for (size_t i = 0; i < call.args.size(); ++i)
            {
                if (isAtomicCall && i == 0)
                    visitLocation (call.args[0]);
                else
                    visitValue (call.args[i]);
            }

            return;
        }

        forEachChildExpr (e, [this] (Expr& child)
        {
            visitValue (child);
        });
    }

    Expr makeHostConversion (Expr value, const TypeSpecifier& glsl, const TypeSpecifier& host, bool load)
    {
        const auto l = value.loc;
        const auto type = load ? glsl : host;

        // Value conversions map non-zero to true and true to 1u
        if (glsl.arraySpecifiers.empty() && isBoolish (glsl.kind))
        {
            std::vector<Expr> args;
            args.push_back (std::move (value));
            return makeConstruct (l, type, std::move (args));
        }

        std::vector<Expr> args;
        args.push_back (std::move (value));
        return makeCall (l, converterFunction (glsl, host, load, l), std::move (args), type);
    }

    void loadWith (Expr& e, const TypeSpecifier& host)
    {
        if (! e.type.has_value())
            return;

        const auto glsl = *e.type;
        const auto l = e.loc;

        if (host.arraySpecifiers.empty() && isAtomicKind (host.kind))
        {
            std::vector<Expr> args;
            args.push_back (makeUnary (l, UnaryOp::addressOf, std::move (e)));
            e = makeCall (l, "atomicLoad", std::move (args), glsl);
            return;
        }

        if (sameType (glsl, host))
            return;

        // The location holds the stored type; the conversion produces the GLSL one
        e.type = host;
        e = makeHostConversion (std::move (e), glsl, host, true);
    }

    void visitDeclaration (Declaration& d)
    {
        forEachDeclarationExpr (d, [this] (Expr& e)
        {
            visitValue (e);
        });

        if (d.initDeclaratorList != nullptr)
            for (const auto& single : d.initDeclaratorList->declarations)
                scopes.back().insert (single.name);
    }

    void visitStatement (Statement& s)
    {
        const bool opensScope = s.is<StmtCompound>() || s.is<StmtFor>() || s.is<StmtSwitch>();
        if (opensScope)
            scopes.emplace_back();

        if (s.is<StmtDeclaration>())
            visitDeclaration (s.as<StmtDeclaration>().declaration);
        else if (s.is<StmtFor>())
        {
            auto& f = s.as<StmtFor>();

            if (f.init != nullptr)
                visitStatement (*f.init);
            if (f.condition != nullptr)
                visitValue (*f.condition);
            if (f.update != nullptr)
                visitValue (*f.update);
            if (f.body != nullptr)
                visitStatement (*f.body);
        }
        else
        {
            forEachOwnExpr (s, [this] (Expr& e)
            {
                visitValue (e);
            });

            forEachChildStatement (s, [this] (Statement& child)
            {
                visitStatement (child);
            });
        }

        if (opensScope)
            scopes.pop_back();
    }

    void rewriteAccesses()
    {
        for (auto& external : ast.declarations)
        {
            auto* fd = std::get_if<FunctionDefinition> (&external);
            if (fd == nullptr || fd->body == nullptr)
                continue;

            scopes.clear();
            scopes.emplace_back();

            for (const auto& param : fd->prototype.parameters)
                scopes.back().insert (param.name);

            visitStatement (*fd->body);
        }
    }

    //==========================================================================
    TranslationUnit& ast;
    WgslLoweringContext& context;

    std::map<std::string, StructSpecifier*> structs;
    std::map<std::string, int64_t> constants;
    std::set<std::pair<std::string, std::string>> atomicFields;
    std::set<std::string> atomicGlobals;
    std::map<std::string, TypeSpecifier> hostGlobals;
    std::map<std::pair<std::string, BlockLayout>, StructLayout> structLayouts;
    std::map<std::string, SizeAlign> hostSizes;
    std::map<std::string, TypeSpecifier> wrappers;
    std::vector<std::unique_ptr<StructSpecifier>> generated;
    std::vector<std::set<std::string>> scopes;
};

} // namespace

//==============================================================================
void WgslHostLayout::apply (TranslationUnit& ast, WgslLoweringContext& context)
{
    HostLayouter (ast, context).run();
}

} // namespace wgsl
} // namespace yup
