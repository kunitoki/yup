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

namespace wgsl
{

//==============================================================================
/** Source location tracked through parse/transpile phases. */
struct SourceLocation
{
    int line = 0;
    int column = 0;
};

//==============================================================================
/** Unary operator (prefix or postfix). */
enum class UnaryOp
{
    plus,
    minus,
    logicalNot,
    bitwiseNot,
    preInc,
    preDec,
    postInc,
    postDec
};

//==============================================================================
/** Binary / arithmetic operator. */
enum class BinaryOp
{
    add,
    sub,
    mul,
    div,
    mod,
    shiftLeft,
    shiftRight,
    lessThan,
    greaterThan,
    lessEqual,
    greaterEqual,
    equal,
    notEqual,
    bitwiseAnd,
    bitwiseXor,
    bitwiseOr,
    logicalAnd,
    logicalOr
};

//==============================================================================
/** Compound assignment operator. */
enum class AssignmentOp
{
    assign,
    addAssign,
    subAssign,
    mulAssign,
    divAssign,
    modAssign,
    shiftLeftAssign,
    shiftRightAssign,
    bitwiseAndAssign,
    bitwiseXorAssign,
    bitwiseOrAssign
};

//==============================================================================
/** Type qualifier storage classes. */
enum class StorageQualifier
{
    none,
    constQual,
    in,
    out,
    inout,
    uniform,
    buffer,
    shared,
    centroid,
    sample
};

//==============================================================================
/** Interpolation qualifier. */
enum class InterpolationQualifier
{
    none,
    smooth,
    flat,
    noPerspective
};

//==============================================================================
/** Precision qualifier. */
enum class PrecisionQualifier
{
    none,
    lowp,
    mediump,
    highp
};

//==============================================================================
/** Type specifier kind used for basic/matrix/sampler discrimination. */
enum class TypeKind
{
    voidType,
    floatType,
    intType,
    uintType,
    boolType,
    doubleType,
    vec2,
    vec3,
    vec4,
    ivec2,
    ivec3,
    ivec4,
    uvec2,
    uvec3,
    uvec4,
    bvec2,
    bvec3,
    bvec4,
    dvec2,
    dvec3,
    dvec4,
    mat2,
    mat3,
    mat4,
    mat2x2,
    mat2x3,
    mat2x4,
    mat3x2,
    mat3x3,
    mat3x4,
    mat4x2,
    mat4x3,
    mat4x4,
    dmat2,
    dmat3,
    dmat4,
    dmat2x2,
    dmat2x3,
    dmat2x4,
    dmat3x2,
    dmat3x3,
    dmat3x4,
    dmat4x2,
    dmat4x3,
    dmat4x4,
    sampler1D,
    sampler2D,
    sampler3D,
    samplerCube,
    sampler1DShadow,
    sampler2DShadow,
    samplerCubeShadow,
    sampler1DArray,
    sampler2DArray,
    sampler1DArrayShadow,
    sampler2DArrayShadow,
    sampler2DRect,
    sampler2DRectShadow,
    samplerBuffer,
    sampler2DMS,
    sampler2DMSArray,
    isampler1D,
    isampler2D,
    isampler3D,
    isamplerCube,
    isampler1DArray,
    isampler2DArray,
    isampler2DRect,
    isamplerBuffer,
    isampler2DMS,
    isampler2DMSArray,
    usampler1D,
    usampler2D,
    usampler3D,
    usamplerCube,
    usampler1DArray,
    usampler2DArray,
    usampler2DRect,
    usamplerBuffer,
    usampler2DMS,
    usampler2DMSArray,
    image1D,
    image2D,
    image3D,
    imageCube,
    image1DArray,
    image2DArray,
    image2DRect,
    imageBuffer,
    image2DMS,
    image2DMSArray,
    iimage1D,
    iimage2D,
    iimage3D,
    iimageCube,
    iimage1DArray,
    iimage2DArray,
    iimage2DRect,
    iimageBuffer,
    iimage2DMS,
    iimage2DMSArray,
    uimage1D,
    uimage2D,
    uimage3D,
    uimageCube,
    uimage1DArray,
    uimage2DArray,
    uimage2DRect,
    uimageBuffer,
    uimage2DMS,
    uimage2DMSArray,
    atomicUint,

    // Vulkan separate texture / sampler types
    texture1D,
    texture2D,
    texture3D,
    textureCube,
    texture1DArray,
    texture2DArray,
    texture2DRect,
    textureBuffer,
    texture2DMS,
    texture2DMSArray,
    samplerType,
    samplerShadow,
    subpassInput,
    subpassInputMS,

    namedStruct
};

//==============================================================================
/** Layout qualifier identifier. */
enum class LayoutQualifierId
{
    location,
    binding,
    descriptorSet,
    component,
    localSizeX,
    localSizeY,
    localSizeZ,
    vertices,
    invocations,
    tessSpacing,
    tessVertices,
    tessOutputTopology,
    tessInputMode,
    points,
    lines,
    lineStrip,
    linesAdjacency,
    triangles,
    triangleStrip,
    trianglesAdjacency,
    fractionalEvenSpacing,
    fractionalOddSpacing,
    equalSpacing,
    cw,
    ccw,
    isolines,
    quads,
    xfbBuffer,
    xfbStride,
    xfbOffset,
    inputAttachmentIndex,
    std140,
    std430,
    columnMajor,
    rowMajor,
    earlyFragmentTests,
    depthGreater,
    depthLess,
    depthUnchanged,
    depthAny
};

//==============================================================================
/** Selection restriction statement variant. */
enum class SelectionRestriction
{
    standard,
    flatten,
    dontFlatten
};

//==============================================================================
/** Jump statement kind. */
enum class JumpKind
{
    returnJump,
    breakJump,
    continueJump,
    discardJump
};

// Forward declarations for recursive types:
struct Expr;
struct Statement;
struct TypeSpecifier;
struct ArraySpecifier;
struct LayoutQualifier;
struct Declaration;

//==============================================================================
/** A single layout qualifier id=value pair (or id-only). */
struct LayoutQualifierEntry
{
    SourceLocation loc;
    LayoutQualifierId id;
    std::unique_ptr<Expr> value; // null for id-only qualifiers like std140
};

//==============================================================================
/** A full layout(qualifier1, qualifier2, ...) spec. */
struct LayoutQualifier
{
    SourceLocation loc;
    std::vector<LayoutQualifierEntry> entries;
};

//==============================================================================
/** Holds all type qualifiers attached to a declaration. */
struct TypeQualifier
{
    SourceLocation loc;
    std::vector<StorageQualifier> storage;
    std::vector<InterpolationQualifier> interpolation;
    std::vector<PrecisionQualifier> precision;
    bool invariant = false;
    bool precise = false;
    std::unique_ptr<LayoutQualifier> layout;

    bool hasStorage (StorageQualifier sq) const
    {
        for (auto& s : storage)
            if (s == sq)
                return true;
        return false;
    }
};

// Deep-copy helper for Expr (defined after all expression types)
Expr copyExpr (const Expr& e);

//==============================================================================
/** Array specifier: array of type, either sized or unsized. */
struct ArraySpecifier
{
    SourceLocation loc;
    bool isUnsized = false;
    std::unique_ptr<Expr> sizeExpr; // null when isUnsized

    ArraySpecifier() = default;

    // Copy ctor/assign defined after Expr is complete
    ArraySpecifier (const ArraySpecifier& other);
    ArraySpecifier& operator= (const ArraySpecifier& other);

    ArraySpecifier (ArraySpecifier&&) = default;
    ArraySpecifier& operator= (ArraySpecifier&&) = default;
};

//==============================================================================
/** Full type specifier (base type + optional array specifiers). */
struct TypeSpecifier
{
    SourceLocation loc;
    TypeKind kind = TypeKind::voidType;
    std::string structName; // populated when kind == namedStruct
    std::vector<ArraySpecifier> arraySpecifiers;

    static TypeSpecifier make (SourceLocation loc, TypeKind kind)
    {
        TypeSpecifier ts;
        ts.loc = loc;
        ts.kind = kind;
        return ts;
    }

    static TypeSpecifier makeNamed (SourceLocation loc, const std::string& name)
    {
        TypeSpecifier ts;
        ts.loc = loc;
        ts.kind = TypeKind::namedStruct;
        ts.structName = name;
        return ts;
    }
};

//==============================================================================
/** Struct field declaration. */
struct StructFieldSpecifier
{
    SourceLocation loc;
    TypeSpecifier type;
    std::string name;
    std::unique_ptr<TypeQualifier> qualifier;
};

//==============================================================================
/** Struct specifier: struct name { fields } */
struct StructSpecifier
{
    SourceLocation loc;
    std::string name;
    std::vector<StructFieldSpecifier> fields;
};

//==============================================================================
/** Initializer for a variable declaration. */
struct Initializer
{
    SourceLocation loc;
    std::unique_ptr<Expr> expr;         // simple initializer
    std::vector<Initializer> aggregate; // aggregate initializer list (mutually exclusive with expr)
};

//==============================================================================
/** A single named declaration (name + optional array + optional init). */
struct SingleDeclaration
{
    SourceLocation loc;
    std::string name;
    std::vector<ArraySpecifier> arraySpecifiers;
    std::unique_ptr<Initializer> initializer;
};

//==============================================================================
/** Init declarator list: type qualifiers + type + list of single declarations. */
struct InitDeclaratorList
{
    SourceLocation loc;
    std::unique_ptr<TypeQualifier> qualifier;
    TypeSpecifier type;
    std::vector<SingleDeclaration> declarations;
};

//==============================================================================
/** Declaration in the AST (init declarator, block, struct, precision, global). */
struct Declaration
{
    SourceLocation loc;
    std::unique_ptr<InitDeclaratorList> initDeclaratorList; // null for non-decl stmts
    std::unique_ptr<StructSpecifier> structSpecifier;       // null if not a struct decl
    std::unique_ptr<TypeQualifier> qualifier;               // standalone qualifier decl (e.g. layout(local_size_x=8) in;)
};

//==============================================================================
/** Function parameter declaration. */
struct FunctionParameterDeclaration
{
    SourceLocation loc;
    std::string name;
    std::unique_ptr<TypeQualifier> qualifier;
    TypeSpecifier type;
    std::vector<ArraySpecifier> arraySpecifiers;
};

//==============================================================================
/** Function prototype: return type + name + params. */
struct FunctionPrototype
{
    SourceLocation loc;
    TypeSpecifier returnType;
    std::string name;
    std::vector<FunctionParameterDeclaration> parameters;
};

//==============================================================================
// Expression node variants
//==============================================================================

/** Variable / identifier reference. */
struct ExprVariable
{
    SourceLocation loc;
    std::string name;
};

/** Integer literal. */
struct ExprIntConst
{
    SourceLocation loc;
    int value = 0;
};

/** Unsigned integer literal. */
struct ExprUIntConst
{
    SourceLocation loc;
    unsigned int value = 0;
};

/** Float literal. */
struct ExprFloatConst
{
    SourceLocation loc;
    double value = 0.0;
};

/** Boolean literal. */
struct ExprBoolConst
{
    SourceLocation loc;
    bool value = false;
};

/** Unary expression: op operand. */
struct ExprUnary
{
    SourceLocation loc;
    UnaryOp op;
    std::unique_ptr<Expr> operand;
};

/** Binary expression: left op right. */
struct ExprBinary
{
    SourceLocation loc;
    BinaryOp op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

/** Ternary / conditional expression: cond ? trueBranch : falseBranch. */
struct ExprTernary
{
    SourceLocation loc;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> trueBranch;
    std::unique_ptr<Expr> falseBranch;
};

/** Assignment expression: lhs op rhs. */
struct ExprAssignment
{
    SourceLocation loc;
    AssignmentOp op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

/** Bracket / array subscript: base[index]. */
struct ExprBracket
{
    SourceLocation loc;
    std::unique_ptr<Expr> base;
    std::unique_ptr<Expr> index;
};

/** Function call: callee, args. */
struct ExprFunCall
{
    SourceLocation loc;
    std::unique_ptr<Expr> callee;
    std::vector<Expr> args;
};

/** Dot / member / swizzle access: base.member. */
struct ExprDot
{
    SourceLocation loc;
    std::unique_ptr<Expr> base;
    std::string member;
};

/** Comma expression (sequence): left, right. */
struct ExprComma
{
    SourceLocation loc;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;
};

/** Type constructor call: type(args...). */
struct ExprTypeConstructor
{
    SourceLocation loc;
    TypeSpecifier type;
    std::vector<Expr> args;
};

/** Parenthesized expression. */
struct ExprParen
{
    SourceLocation loc;
    std::unique_ptr<Expr> expr;
};

//==============================================================================
/** Top-level expression type as a variant. */
using ExprVariant = std::variant<
    ExprVariable,
    ExprIntConst,
    ExprUIntConst,
    ExprFloatConst,
    ExprBoolConst,
    ExprUnary,
    ExprBinary,
    ExprTernary,
    ExprAssignment,
    ExprBracket,
    ExprFunCall,
    ExprDot,
    ExprComma,
    ExprTypeConstructor,
    ExprParen>;

//==============================================================================
/** Expression node wrapping the variant. */
struct Expr
{
    SourceLocation loc;
    ExprVariant value;

    Expr() = default;
    Expr (Expr&&) = default;
    Expr& operator= (Expr&&) = default;

    template <typename T>
    bool is() const
    {
        return std::holds_alternative<T> (value);
    }

    template <typename T>
    const T& as() const
    {
        return std::get<T> (value);
    }

    template <typename T>
    T& as()
    {
        return std::get<T> (value);
    }
};

//==============================================================================
// Statement node variants
//==============================================================================

/** Selection statement: if/else. */
struct StmtSelection
{
    SourceLocation loc;
    SelectionRestriction restriction = SelectionRestriction::standard;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> thenBranch;
    std::unique_ptr<Statement> elseBranch; // null if no else
};

/** Switch statement. */
struct StmtSwitch
{
    SourceLocation loc;
    std::unique_ptr<Expr> selector;
    std::vector<Statement> body; // expected to be case/default + statements
};

/** Case label statement. */
struct StmtCaseLabel
{
    SourceLocation loc;
    std::unique_ptr<Expr> label; // null for default:
};

/** While loop: while (cond) body. */
struct StmtWhile
{
    SourceLocation loc;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> body;
};

/** Do-while loop: do body while (cond). */
struct StmtDoWhile
{
    SourceLocation loc;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> body;
};

/** For loop: for (init; cond; update) body. */
struct StmtFor
{
    SourceLocation loc;
    std::unique_ptr<Statement> init; // null if empty
    std::unique_ptr<Expr> condition; // null if empty
    std::unique_ptr<Expr> update;    // null if empty
    std::unique_ptr<Statement> body;
};

/** Jump statement: return / break / continue / discard. */
struct StmtJump
{
    SourceLocation loc;
    JumpKind kind;
    std::unique_ptr<Expr> returnValue; // null except for return
};

/** Expression as a statement. */
struct StmtExpr
{
    SourceLocation loc;
    std::unique_ptr<Expr> expr;
};

/** Compound statement: { stmt* }. */
struct StmtCompound
{
    SourceLocation loc;
    std::vector<Statement> statements;
};

/** Declaration statement. */
struct StmtDeclaration
{
    SourceLocation loc;
    Declaration declaration;
};

//==============================================================================
/** Top-level statement type as a variant. */
using StatementVariant = std::variant<
    StmtSelection,
    StmtSwitch,
    StmtCaseLabel,
    StmtWhile,
    StmtDoWhile,
    StmtFor,
    StmtJump,
    StmtExpr,
    StmtCompound,
    StmtDeclaration>;

//==============================================================================
/** Statement node wrapping the variant. */
struct Statement
{
    SourceLocation loc;
    StatementVariant value;

    template <typename T>
    bool is() const
    {
        return std::holds_alternative<T> (value);
    }

    template <typename T>
    const T& as() const
    {
        return std::get<T> (value);
    }

    template <typename T>
    T& as()
    {
        return std::get<T> (value);
    }

    /** Create a compound statement from a vector of statements. */
    static Statement makeCompound (SourceLocation loc, std::vector<Statement> stmts)
    {
        Statement s;
        s.loc = loc;
        s.value = StmtCompound { loc, std::move (stmts) };
        return s;
    }

    /** Create an expression statement. */
    static Statement makeExpr (SourceLocation loc, Expr expr)
    {
        Statement s;
        s.loc = loc;
        s.value = StmtExpr { loc, std::make_unique<Expr> (std::move (expr)) };
        return s;
    }

    /** Create a jump statement (return). */
    static Statement makeReturn (SourceLocation loc, std::unique_ptr<Expr> value)
    {
        Statement s;
        s.loc = loc;
        s.value = StmtJump { loc, JumpKind::returnJump, std::move (value) };
        return s;
    }

    /** Create an empty statement. */
    static Statement makeEmpty (SourceLocation loc)
    {
        Statement s;
        s.loc = loc;
        s.value = StmtCompound { loc, {} };
        return s;
    }
};

//==============================================================================
// External declaration variants
//==============================================================================

/** Function definition: prototype + body. */
struct FunctionDefinition
{
    SourceLocation loc;
    FunctionPrototype prototype;
    std::unique_ptr<Statement> body; // compound statement
};

/** Preprocessing directive (e.g., #extension, #pragma). Parsed but discarded in v1. */
struct PreprocessorDirective
{
    SourceLocation loc;
    std::string directive;
};

//==============================================================================
/** External declaration in the translation unit (function def, decl, or preprocessor). */
using ExternalDeclaration = std::variant<
    FunctionDefinition,
    Declaration,
    PreprocessorDirective>;

//==============================================================================
/** Top-level translation unit (the root of the AST). */
struct TranslationUnit
{
    SourceLocation loc;
    std::vector<ExternalDeclaration> declarations;
};

//==============================================================================
/** Deep-copy an Expr, recursively copying all unique_ptr children. */

inline ArraySpecifier::ArraySpecifier (const ArraySpecifier& other)
    : loc (other.loc)
    , isUnsized (other.isUnsized)
    , sizeExpr (other.sizeExpr ? std::make_unique<Expr> (copyExpr (*other.sizeExpr)) : nullptr)
{
}

inline ArraySpecifier& ArraySpecifier::operator= (const ArraySpecifier& other)
{
    if (this != &other)
    {
        loc = other.loc;
        isUnsized = other.isUnsized;
        sizeExpr = other.sizeExpr ? std::make_unique<Expr> (copyExpr (*other.sizeExpr)) : nullptr;
    }
    return *this;
}

inline Expr copyExpr (const Expr& e)
{
    Expr result;
    result.loc = e.loc;

    std::visit ([&] (const auto& alt)
    {
        using T = std::decay_t<decltype (alt)>;

        if constexpr (std::is_same_v<T, ExprVariable> || std::is_same_v<T, ExprIntConst>
                      || std::is_same_v<T, ExprUIntConst> || std::is_same_v<T, ExprFloatConst>
                      || std::is_same_v<T, ExprBoolConst>)
        {
            result.value = alt;
        }
        else if constexpr (std::is_same_v<T, ExprUnary>)
        {
            result.value = ExprUnary { alt.loc, alt.op, alt.operand ? std::make_unique<Expr> (copyExpr (*alt.operand)) : nullptr };
        }
        else if constexpr (std::is_same_v<T, ExprBinary>)
        {
            result.value = ExprBinary { alt.loc, alt.op, alt.left ? std::make_unique<Expr> (copyExpr (*alt.left)) : nullptr, alt.right ? std::make_unique<Expr> (copyExpr (*alt.right)) : nullptr };
        }
        else if constexpr (std::is_same_v<T, ExprTernary>)
        {
            result.value = ExprTernary { alt.loc,
                                         alt.condition ? std::make_unique<Expr> (copyExpr (*alt.condition)) : nullptr,
                                         alt.trueBranch ? std::make_unique<Expr> (copyExpr (*alt.trueBranch)) : nullptr,
                                         alt.falseBranch ? std::make_unique<Expr> (copyExpr (*alt.falseBranch)) : nullptr };
        }
        else if constexpr (std::is_same_v<T, ExprAssignment>)
        {
            result.value = ExprAssignment { alt.loc, alt.op, alt.lhs ? std::make_unique<Expr> (copyExpr (*alt.lhs)) : nullptr, alt.rhs ? std::make_unique<Expr> (copyExpr (*alt.rhs)) : nullptr };
        }
        else if constexpr (std::is_same_v<T, ExprBracket>)
        {
            result.value = ExprBracket { alt.loc,
                                         alt.base ? std::make_unique<Expr> (copyExpr (*alt.base)) : nullptr,
                                         alt.index ? std::make_unique<Expr> (copyExpr (*alt.index)) : nullptr };
        }
        else if constexpr (std::is_same_v<T, ExprFunCall>)
        {
            ExprFunCall copy;
            copy.loc = alt.loc;
            copy.callee = alt.callee ? std::make_unique<Expr> (copyExpr (*alt.callee)) : nullptr;

            for (const auto& arg : alt.args)
                copy.args.push_back (copyExpr (arg));

            result.value = std::move (copy);
        }
        else if constexpr (std::is_same_v<T, ExprDot>)
        {
            result.value = ExprDot { alt.loc,
                                     alt.base ? std::make_unique<Expr> (copyExpr (*alt.base)) : nullptr,
                                     alt.member };
        }
        else if constexpr (std::is_same_v<T, ExprComma>)
        {
            result.value = ExprComma { alt.loc,
                                       alt.left ? std::make_unique<Expr> (copyExpr (*alt.left)) : nullptr,
                                       alt.right ? std::make_unique<Expr> (copyExpr (*alt.right)) : nullptr };
        }
        else if constexpr (std::is_same_v<T, ExprTypeConstructor>)
        {
            ExprTypeConstructor copy;
            copy.loc = alt.loc;
            copy.type = alt.type;

            for (const auto& arg : alt.args)
                copy.args.push_back (copyExpr (arg));

            result.value = std::move (copy);
        }
        else if constexpr (std::is_same_v<T, ExprParen>)
        {
            result.value = ExprParen { alt.loc,
                                       alt.expr ? std::make_unique<Expr> (copyExpr (*alt.expr)) : nullptr };
        }
    },
                e.value);

    return result;
}

} // namespace wgsl
} // namespace yup
