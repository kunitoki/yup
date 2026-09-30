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

constexpr const char* componentNames = "xyzw";

/** A WGSL type name usable inside an identifier: vec3<f32> gives vec3f32. */
std::string typeKey (const TypeSpecifier& type)
{
    std::string key;
    for (const char c : wgslTypeName (type.kind))
        if (c != '<' && c != '>' && c != ',' && c != ' ')
            key += c;

    return key;
}

//==============================================================================
class BuiltinLowerer
{
public:
    BuiltinLowerer (Statements& statements, WgslBuiltinHost& builtinHost, SourceLocation location)
        : pre (statements)
        , host (builtinHost)
        , l (location)
    {
    }

    //==========================================================================
    Expr lowerCall (Expr e)
    {
        auto& call = e.as<ExprFunCall>();
        const auto name = call.callee->as<ExprVariable>().name;
        auto args = std::move (call.args);
        const auto resultType = e.type;

        static const std::set<std::string> sameName = {
            "radians", "degrees", "sin", "cos", "tan", "asin", "acos", "sinh", "cosh", "tanh", "asinh", "acosh",
            "atanh", "pow", "exp", "log", "exp2", "log2", "sqrt", "abs", "sign", "floor", "trunc", "round", "ceil",
            "fract", "min", "max", "clamp", "step", "smoothstep", "fma", "length", "distance", "dot", "cross",
            "normalize", "reflect", "refract", "transpose", "determinant", "any", "all", "ldexp"
        };

        static const std::map<std::string, std::string> renamed = {
            { "inversesqrt", "inverseSqrt" },
            { "faceforward", "faceForward" },
            { "roundEven", "round" },
            { "bitfieldReverse", "reverseBits" },
            { "packUnorm2x16", "pack2x16unorm" },
            { "packSnorm2x16", "pack2x16snorm" },
            { "packUnorm4x8", "pack4x8unorm" },
            { "packSnorm4x8", "pack4x8snorm" },
            { "packHalf2x16", "pack2x16float" },
            { "unpackUnorm2x16", "unpack2x16unorm" },
            { "unpackSnorm2x16", "unpack2x16snorm" },
            { "unpackUnorm4x8", "unpack4x8unorm" },
            { "unpackSnorm4x8", "unpack4x8snorm" },
            { "unpackHalf2x16", "unpack2x16float" }
        };

        static const std::map<std::string, std::string> derivatives = {
            { "dFdx", "dpdx" },
            { "dFdy", "dpdy" },
            { "fwidth", "fwidth" },
            { "dFdxFine", "dpdxFine" },
            { "dFdyFine", "dpdyFine" },
            { "fwidthFine", "fwidthFine" },
            { "dFdxCoarse", "dpdxCoarse" },
            { "dFdyCoarse", "dpdyCoarse" },
            { "fwidthCoarse", "fwidthCoarse" }
        };

        if (sameName.count (name) > 0)
            return makeCall (l, name, std::move (args), resultType);

        if (auto found = renamed.find (name); found != renamed.end())
            return makeCall (l, found->second, std::move (args), resultType);

        if (auto found = derivatives.find (name); found != derivatives.end())
        {
            if (host.getStage() != ShaderStage::fragment)
                throw LoweringError (l, name + "() is only available in fragment shaders");

            return makeCall (l, found->second, std::move (args), resultType);
        }

        if (name == "atan")
        {
            const auto* wgslName = args.size() == 2 ? "atan2" : "atan"; // read the size before args is moved
            return makeCall (l, wgslName, std::move (args), resultType);
        }

        if (name == "mix")
        {
            // A bool selector picks components instead of interpolating
            if (args.size() == 3 && scalarKindOf (typeOf (args[2]).kind) == TypeKind::boolType)
                return makeCall (l, "select", std::move (args), resultType);

            return makeCall (l, "mix", std::move (args), resultType);
        }

        if (name == "not")
            return makeUnary (l, UnaryOp::logicalNot, makeParen (l, std::move (args[0])), resultType);

        static const std::map<std::string, BinaryOp> relational = {
            { "lessThan", BinaryOp::lessThan },
            { "lessThanEqual", BinaryOp::lessEqual },
            { "greaterThan", BinaryOp::greaterThan },
            { "greaterThanEqual", BinaryOp::greaterEqual },
            { "equal", BinaryOp::equal },
            { "notEqual", BinaryOp::notEqual }
        };

        if (auto found = relational.find (name); found != relational.end())
            return makeBinary (l, found->second, std::move (args[0]), std::move (args[1]), resultType);

        if (name == "mod")
            return lowerMod (std::move (args), resultType);

        if (name == "isnan" || name == "isinf")
            return lowerClassify (name, std::move (args), resultType);

        if (name == "bitCount" || name == "findLSB" || name == "findMSB")
        {
            const auto wgslName = name == "bitCount" ? "countOneBits" : name == "findLSB" ? "firstTrailingBit" : "firstLeadingBit";
            const auto argType = typeOf (args[0]);
            auto result = makeCall (l, wgslName, std::move (args), argType);
            return convertTo (resultType->kind, std::move (result));
        }

        if (name == "bitfieldExtract")
        {
            args[1] = convertTo (TypeKind::uintType, std::move (args[1]));
            args[2] = convertTo (TypeKind::uintType, std::move (args[2]));
            return makeCall (l, "extractBits", std::move (args), resultType);
        }

        if (name == "bitfieldInsert")
        {
            args[2] = convertTo (TypeKind::uintType, std::move (args[2]));
            args[3] = convertTo (TypeKind::uintType, std::move (args[3]));
            return makeCall (l, "insertBits", std::move (args), resultType);
        }

        if (name == "floatBitsToInt" || name == "floatBitsToUint" || name == "intBitsToFloat" || name == "uintBitsToFloat")
            return makeCall (l, "bitcast<" + wgslTypeName (resultType->kind) + ">", std::move (args), resultType);

        if (name == "frexp" || name == "modf")
            return lowerDecompose (name, std::move (args), resultType);

        if (name == "inverse")
            return lowerInverse (std::move (args), resultType);

        if (name == "outerProduct")
            return lowerOuterProduct (std::move (args), resultType);

        if (name == "matrixCompMult")
            return lowerMatrixCompMult (std::move (args), resultType);

        if (name == "uaddCarry" || name == "usubBorrow" || name == "umulExtended")
            return lowerExtendedArithmetic (name, std::move (args), resultType);

        if (name == "barrier" || name == "memoryBarrierShared" || name == "groupMemoryBarrier"
            || name == "memoryBarrierBuffer" || name == "memoryBarrierImage" || name == "memoryBarrier")
            return lowerBarrier (name);

        if (isTextureFunction (name))
            return lowerTexture (name, std::move (args), resultType);

        if (name == "imageLoad" || name == "imageStore" || name == "imageSize")
            return lowerImage (name, std::move (args), resultType);

        if (name.rfind ("atomic", 0) == 0)
            return lowerAtomic (name, std::move (args), resultType);

        if (name.rfind ("interpolateAt", 0) == 0)
            throw LoweringError (l, name + "() has no WGSL equivalent");

        throw LoweringError (l, "Unsupported builtin function '" + name + "'");
    }

    //==========================================================================
    Expr lowerConstructor (Expr e)
    {
        auto& ctor = e.as<ExprTypeConstructor>();
        const auto type = ctor.type;

        if (! type.arraySpecifiers.empty() || type.kind == TypeKind::namedStruct || isOpaqueType (type.kind))
            return e;

        auto args = std::move (ctor.args);

        if (isMatrixType (type.kind))
            return lowerMatrixConstructor (type, std::move (args));

        if (componentCount (type.kind) > 1)
            return lowerVectorConstructor (type, std::move (args));

        // Scalar conversion: GLSL takes the first component of a vector or matrix
        if (args.size() == 1)
        {
            const auto argType = typeOf (args[0]);

            if (isMatrixType (argType.kind))
                args[0] = makeDot (l, makeIndex (l, std::move (args[0]), makeIntLiteral (l, 0)), "x", makeType (TypeKind::floatType));
            else if (componentCount (argType.kind) > 1)
                args[0] = makeDot (l, std::move (args[0]), "x", makeType (scalarKindOf (argType.kind)));

            if (args[0].type.has_value() && args[0].type->kind == type.kind)
                return std::move (args[0]);
        }

        return makeConstruct (l, type, std::move (args));
    }

private:
    //==========================================================================
    // Helpers
    //==========================================================================

    TypeSpecifier typeOf (const Expr& e) const
    {
        if (! e.type.has_value())
            throw LoweringError (e.loc, "Cannot determine the type of this expression");

        return *e.type;
    }

    /** Reads of variables, members and swizzles are safe to repeat within one lowered call. */
    static bool isPureRead (const Expr& e)
    {
        if (e.is<ExprVariable>() || e.is<ExprIntConst>() || e.is<ExprUIntConst>() || e.is<ExprFloatConst>() || e.is<ExprBoolConst>())
            return true;

        if (e.is<ExprDot>())
            return isPureRead (*e.as<ExprDot>().base);

        if (e.is<ExprParen>())
            return isPureRead (*e.as<ExprParen>().expr);

        if (e.is<ExprBracket>())
            return isPureRead (*e.as<ExprBracket>().base) && isPureRead (*e.as<ExprBracket>().index);

        return false;
    }

    Expr stable (Expr e)
    {
        if (isPureRead (e))
            return e;

        return host.spill (std::move (e), pre);
    }

    Expr convertTo (TypeKind kind, Expr e)
    {
        if (e.type.has_value() && e.type->kind == kind && e.type->arraySpecifiers.empty())
            return e;

        std::vector<Expr> args;
        args.push_back (std::move (e));
        return makeConstruct (l, makeType (kind), std::move (args));
    }

    Expr component (const Expr& base, int index, TypeKind scalar)
    {
        return makeDot (l, copyExpr (base), std::string (1, componentNames[index]), makeType (scalar));
    }

    /** The first count components of a vector: v.xy, v.xyz, ... */
    Expr leading (Expr base, int count)
    {
        const auto type = typeOf (base);
        if (componentCount (type.kind) == count)
            return base;

        return makeDot (l, std::move (base), std::string (componentNames, static_cast<size_t> (count)), makeType (vectorKind (scalarKindOf (type.kind), count)));
    }

    void polyfill (const std::string& name, const std::string& code)
    {
        host.requirePolyfill (name, code);
    }

    //==========================================================================
    // Math
    //==========================================================================

    Expr lowerMod (std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        const auto x = typeOf (args[0]);
        const auto y = typeOf (args[1]);
        const auto name = "glsl_mod_" + typeKey (x) + "_" + typeKey (y);

        polyfill (name, "fn " + name + "(x: " + wgslTypeName (x.kind) + ", y: " + wgslTypeName (y.kind) + ") -> " + wgslTypeName (x.kind)
                            + " {\n    return x - y * floor(x / y);\n}\n");

        return makeCall (l, name, std::move (args), resultType);
    }

    Expr lowerClassify (const std::string& glslName, std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        const auto type = typeOf (args[0]);
        const auto count = componentCount (type.kind);
        const auto name = "glsl_" + glslName + "_" + typeKey (type);
        const auto bits = count > 1 ? "vec" + std::to_string (count) + "<u32>" : std::string ("u32");
        const auto constant = [&bits, count] (const char* value)
        {
            return count > 1 ? bits + "(" + value + ")" : std::string (value);
        };

        // Bit tests stay correct even where the implementation assumes floats are never NaN
        const auto op = glslName == "isnan" ? " > " : " == ";
        polyfill (name, "fn " + name + "(x: " + wgslTypeName (type.kind) + ") -> " + wgslTypeName (resultType->kind) + " {\n"
                            + "    return (bitcast<" + bits + ">(x) & " + constant ("0x7fffffffu") + ")" + op + constant ("0x7f800000u") + ";\n}\n");

        return makeCall (l, name, std::move (args), resultType);
    }

    /** frexp(x, out e) and modf(x, out i): WGSL returns both parts in a struct. */
    Expr lowerDecompose (const std::string& name, std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        if (args.size() != 2)
            throw LoweringError (l, name + "() takes two arguments");

        // The out argument arrives as &temporary or as a forwarded pointer parameter
        auto target = args[1].is<ExprUnary>() && args[1].as<ExprUnary>().op == UnaryOp::addressOf
                        ? std::move (*args[1].as<ExprUnary>().operand)
                        : makeUnary (l, UnaryOp::deref, std::move (args[1]));

        std::vector<Expr> callArgs;
        callArgs.push_back (std::move (args[0]));

        auto inferred = makeType (TypeKind::voidType); // the result struct can't be named in WGSL
        auto result = host.spill (makeCall (l, name, std::move (callArgs), inferred), pre);

        auto part = makeDot (l, copyExpr (result), name == "frexp" ? "exp" : "whole", target.type);
        pre.push_back (makeExprStatement (l, makeAssign (l, AssignmentOp::assign, std::move (target), std::move (part))));

        return makeDot (l, std::move (result), "fract", resultType);
    }

    Expr lowerInverse (std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        const auto type = typeOf (args[0]);
        const auto [columns, rows] = matrixShape (type.kind);

        if (columns != rows)
            throw LoweringError (l, "inverse() needs a square matrix");

        const auto typeName = wgslTypeName (type.kind);
        const auto name = "glsl_inverse_" + typeKey (type);
        std::string body;

        if (columns == 2)
        {
            body = "    return " + typeName + "(m[1][1], -m[0][1], -m[1][0], m[0][0]) * (1.0 / determinant(m));\n";
        }
        else if (columns == 3)
        {
            body = "    let r0 = cross(m[1], m[2]);\n"
                   "    let r1 = cross(m[2], m[0]);\n"
                   "    let r2 = cross(m[0], m[1]);\n"
                   "    return transpose(mat3x3<f32>(r0, r1, r2)) * (1.0 / dot(r2, m[2]));\n";
        }
        else
        {
            for (int c = 0; c < 4; ++c)
                for (int r = 0; r < 4; ++r)
                    body += "    let a" + std::to_string (c) + std::to_string (r) + " = m[" + std::to_string (c) + "][" + std::to_string (r) + "];\n";

            body += "    let b00 = a00 * a11 - a01 * a10;\n"
                    "    let b01 = a00 * a12 - a02 * a10;\n"
                    "    let b02 = a00 * a13 - a03 * a10;\n"
                    "    let b03 = a01 * a12 - a02 * a11;\n"
                    "    let b04 = a01 * a13 - a03 * a11;\n"
                    "    let b05 = a02 * a13 - a03 * a12;\n"
                    "    let b06 = a20 * a31 - a21 * a30;\n"
                    "    let b07 = a20 * a32 - a22 * a30;\n"
                    "    let b08 = a20 * a33 - a23 * a30;\n"
                    "    let b09 = a21 * a32 - a22 * a31;\n"
                    "    let b10 = a21 * a33 - a23 * a31;\n"
                    "    let b11 = a22 * a33 - a23 * a32;\n"
                    "    let det = b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;\n"
                    "    return mat4x4<f32>(\n"
                    "        vec4<f32>(a11 * b11 - a12 * b10 + a13 * b09, a02 * b10 - a01 * b11 - a03 * b09, a31 * b05 - a32 * b04 + a33 * b03, a22 * b04 - a21 * b05 - a23 * b03),\n"
                    "        vec4<f32>(a12 * b08 - a10 * b11 - a13 * b07, a00 * b11 - a02 * b08 + a03 * b07, a32 * b02 - a30 * b05 - a33 * b01, a20 * b05 - a22 * b02 + a23 * b01),\n"
                    "        vec4<f32>(a10 * b10 - a11 * b08 + a13 * b06, a01 * b08 - a00 * b10 - a03 * b06, a30 * b04 - a31 * b02 + a33 * b00, a21 * b02 - a20 * b04 - a23 * b00),\n"
                    "        vec4<f32>(a11 * b07 - a10 * b09 - a12 * b06, a00 * b09 - a01 * b07 + a02 * b06, a31 * b01 - a30 * b03 - a32 * b00, a20 * b03 - a21 * b01 + a22 * b00)) * (1.0 / det);\n";
        }

        polyfill (name, "fn " + name + "(m: " + typeName + ") -> " + typeName + " {\n" + body + "}\n");
        return makeCall (l, name, std::move (args), resultType);
    }

    Expr lowerOuterProduct (std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        const auto c = typeOf (args[0]);
        const auto r = typeOf (args[1]);
        const auto name = "glsl_outerProduct_" + typeKey (c) + "_" + typeKey (r);
        const auto result = wgslTypeName (resultType->kind);

        std::string columns;
        for (int i = 0; i < componentCount (r.kind); ++i)
            columns += std::string (i > 0 ? ", " : "") + "c * r." + componentNames[i];

        polyfill (name, "fn " + name + "(c: " + wgslTypeName (c.kind) + ", r: " + wgslTypeName (r.kind) + ") -> " + result + " {\n"
                            + "    return " + result + "(" + columns + ");\n}\n");

        return makeCall (l, name, std::move (args), resultType);
    }

    Expr lowerMatrixCompMult (std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        const auto type = typeOf (args[0]);
        const auto name = "glsl_matrixCompMult_" + typeKey (type);
        const auto typeName = wgslTypeName (type.kind);

        std::string columns;
        for (int i = 0; i < matrixShape (type.kind).first; ++i)
            columns += (i > 0 ? ", " : "") + std::string ("a[") + std::to_string (i) + "] * b[" + std::to_string (i) + "]";

        polyfill (name, "fn " + name + "(a: " + typeName + ", b: " + typeName + ") -> " + typeName + " {\n    return " + typeName + "(" + columns + ");\n}\n");
        return makeCall (l, name, std::move (args), resultType);
    }

    Expr lowerExtendedArithmetic (const std::string& glslName, std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        const auto type = typeOf (args[0]);
        const auto t = wgslTypeName (type.kind);
        const auto name = "glsl_" + glslName + "_" + typeKey (type);
        const auto splat = [&t] (const std::string& value)
        {
            return t + "(" + value + ")";
        };

        std::string code;

        if (glslName == "uaddCarry")
        {
            code = "fn " + name + "(x: " + t + ", y: " + t + ", carry: ptr<function, " + t + ">) -> " + t + " {\n"
                 + "    let r = x + y;\n"
                 + "    *carry = select(" + splat ("0u") + ", " + splat ("1u") + ", r < x);\n"
                 + "    return r;\n}\n";
        }
        else if (glslName == "usubBorrow")
        {
            code = "fn " + name + "(x: " + t + ", y: " + t + ", borrow: ptr<function, " + t + ">) -> " + t + " {\n"
                 + "    *borrow = select(" + splat ("0u") + ", " + splat ("1u") + ", x < y);\n"
                 + "    return x - y;\n}\n";
        }
        else
        {
            code = "fn " + name + "(x: " + t + ", y: " + t + ", msb: ptr<function, " + t + ">, lsb: ptr<function, " + t + ">) {\n"
                 + "    let xl = x & " + splat ("0xffffu") + ";\n"
                 + "    let xh = x >> " + splat ("16u") + ";\n"
                 + "    let yl = y & " + splat ("0xffffu") + ";\n"
                 + "    let yh = y >> " + splat ("16u") + ";\n"
                 + "    let ll = xl * yl;\n"
                 + "    let lh = xl * yh;\n"
                 + "    let hl = xh * yl;\n"
                 + "    let mid = (ll >> " + splat ("16u") + ") + (lh & " + splat ("0xffffu") + ") + (hl & " + splat ("0xffffu") + ");\n"
                 + "    *lsb = (ll & " + splat ("0xffffu") + ") | (mid << " + splat ("16u") + ");\n"
                 + "    *msb = xh * yh + (lh >> " + splat ("16u") + ") + (hl >> " + splat ("16u") + ") + (mid >> " + splat ("16u") + ");\n}\n";
        }

        polyfill (name, code);
        return makeCall (l, name, std::move (args), resultType);
    }

    Expr lowerBarrier (const std::string& name)
    {
        if (host.getStage() != ShaderStage::compute)
            throw LoweringError (l, name + "() is only available in compute shaders for WGSL");

        if (name == "memoryBarrierBuffer")
            return makeCall (l, "storageBarrier", {});

        if (name == "memoryBarrierImage")
            return makeCall (l, "textureBarrier", {});

        if (name == "memoryBarrier")
        {
            pre.push_back (makeExprStatement (l, makeCall (l, "storageBarrier", {})));
            pre.push_back (makeExprStatement (l, makeCall (l, "textureBarrier", {})));
        }

        return makeCall (l, "workgroupBarrier", {});
    }

    //==========================================================================
    // Atomics
    //==========================================================================

    Expr lowerAtomic (const std::string& name, std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        static const std::set<std::string> direct = { "atomicAdd", "atomicMin", "atomicMax", "atomicAnd", "atomicOr", "atomicXor", "atomicExchange" };

        if (args.empty())
            throw LoweringError (l, name + "() needs arguments");

        auto memory = std::move (args[0]);

        if (direct.count (name) > 0 && args.size() == 2)
        {
            std::vector<Expr> callArgs;
            callArgs.push_back (makeUnary (l, UnaryOp::addressOf, std::move (memory)));
            callArgs.push_back (std::move (args[1]));
            return makeCall (l, name, std::move (callArgs), resultType);
        }

        if (name != "atomicCompSwap" || args.size() != 3)
            throw LoweringError (l, "Unsupported atomic function '" + name + "'");

        // GLSL's compare-and-swap is strong, WGSL's may fail spuriously: retry until it either swaps or sees another value
        const auto type = resultType.has_value() ? *resultType : typeOf (args[1]);
        auto compare = stable (std::move (args[1]));
        auto value = stable (std::move (args[2]));

        const auto resultName = host.allocateName ("_t");
        pre.push_back (makeVarDeclaration (l, resultName, type, std::nullopt));

        const auto exchangeName = host.allocateName ("_t");
        std::vector<Expr> exchangeArgs;
        exchangeArgs.push_back (makeUnary (l, UnaryOp::addressOf, std::move (memory)));
        exchangeArgs.push_back (copyExpr (compare));
        exchangeArgs.push_back (std::move (value));

        StmtLoop loop;
        loop.loc = l;
        loop.body.push_back (makeVarDeclaration (l, exchangeName, makeType (TypeKind::voidType),
                                                 makeCall (l, "atomicCompareExchangeWeak", std::move (exchangeArgs)), true));

        auto exchange = makeVariable (l, exchangeName);
        auto done = makeBinary (l, BinaryOp::logicalOr,
                                makeDot (l, copyExpr (exchange), "exchanged", makeType (TypeKind::boolType)),
                                makeBinary (l, BinaryOp::notEqual, makeDot (l, copyExpr (exchange), "old_value", type), std::move (compare), makeType (TypeKind::boolType)),
                                makeType (TypeKind::boolType));

        std::vector<Statement> onDone;
        onDone.push_back (makeExprStatement (l, makeAssign (l, AssignmentOp::assign, makeVariable (l, resultName, type), makeDot (l, std::move (exchange), "old_value", type))));
        onDone.push_back (makeJumpStatement (l, JumpKind::breakJump));
        loop.body.push_back (makeIf (l, std::move (done), makeBlock (l, std::move (onDone))));

        pre.push_back (makeStatement (l, std::move (loop)));
        return makeVariable (l, resultName, type);
    }

    //==========================================================================
    // Constructors
    //==========================================================================

    /** Splits vector and matrix arguments into scalars. */
    std::vector<Expr> flattenComponents (std::vector<Expr> args)
    {
        std::vector<Expr> scalars;

        for (auto& arg : args)
        {
            const auto type = typeOf (arg);
            const auto scalar = scalarKindOf (type.kind);

            if (componentCount (type.kind) == 1)
            {
                scalars.push_back (std::move (arg));
            }
            else if (const auto [columns, rows] = matrixShape (type.kind); columns > 0)
            {
                auto m = stable (std::move (arg));

                for (int c = 0; c < columns; ++c)
                    for (int r = 0; r < rows; ++r)
                        scalars.push_back (makeDot (l, makeIndex (l, copyExpr (m), makeIntLiteral (l, c), makeType (vectorKind (TypeKind::floatType, rows))),
                                                    std::string (1, componentNames[r]),
                                                    makeType (TypeKind::floatType)));
            }
            else
            {
                auto v = stable (std::move (arg));

                for (int i = 0; i < componentCount (type.kind); ++i)
                    scalars.push_back (component (v, i, scalar));
            }
        }

        return scalars;
    }

    Expr lowerMatrixConstructor (const TypeSpecifier& type, std::vector<Expr> args)
    {
        const auto [columns, rows] = matrixShape (type.kind);
        const auto columnType = makeType (vectorKind (TypeKind::floatType, rows));

        const auto identityEntry = [this] (int c, int r, const Expr* diagonal)
        {
            if (c == r)
                return diagonal != nullptr ? copyExpr (*diagonal) : makeFloatLiteral (l, 1.0);

            return makeFloatLiteral (l, 0.0);
        };

        if (args.size() == 1)
        {
            const auto argType = typeOf (args[0]);

            // matN(s): s on the diagonal
            if (componentCount (argType.kind) == 1)
            {
                auto s = stable (std::move (args[0]));
                std::vector<Expr> cols;

                for (int c = 0; c < columns; ++c)
                {
                    std::vector<Expr> entries;
                    for (int r = 0; r < rows; ++r)
                        entries.push_back (identityEntry (c, r, &s));

                    cols.push_back (makeConstruct (l, columnType, std::move (entries)));
                }

                return makeConstruct (l, type, std::move (cols));
            }

            // matN(m): the overlapping part of m, completed with the identity
            if (const auto [sourceColumns, sourceRows] = matrixShape (argType.kind); sourceColumns > 0)
            {
                if (sourceColumns == columns && sourceRows == rows)
                    return std::move (args[0]);

                auto m = stable (std::move (args[0]));
                std::vector<Expr> cols;

                for (int c = 0; c < columns; ++c)
                {
                    if (c >= sourceColumns)
                    {
                        std::vector<Expr> entries;
                        for (int r = 0; r < rows; ++r)
                            entries.push_back (identityEntry (c, r, nullptr));

                        cols.push_back (makeConstruct (l, columnType, std::move (entries)));
                        continue;
                    }

                    auto column = makeIndex (l, copyExpr (m), makeIntLiteral (l, c), makeType (vectorKind (TypeKind::floatType, sourceRows)));

                    if (rows <= sourceRows)
                    {
                        cols.push_back (leading (std::move (column), rows));
                        continue;
                    }

                    std::vector<Expr> entries;
                    entries.push_back (std::move (column));
                    for (int r = sourceRows; r < rows; ++r)
                        entries.push_back (identityEntry (c, r, nullptr));

                    cols.push_back (makeConstruct (l, columnType, std::move (entries)));
                }

                return makeConstruct (l, type, std::move (cols));
            }
        }

        // WGSL takes either all columns or all scalars, GLSL any mix of them
        bool allColumns = static_cast<int> (args.size()) == columns;
        bool allScalars = static_cast<int> (args.size()) == columns * rows;

        for (const auto& arg : args)
        {
            const auto argType = typeOf (arg);
            allColumns = allColumns && argType.kind == columnType.kind;
            allScalars = allScalars && componentCount (argType.kind) == 1;
        }

        if (allColumns || allScalars)
            return makeConstruct (l, type, std::move (args));

        auto scalars = flattenComponents (std::move (args));
        if (static_cast<int> (scalars.size()) < columns * rows)
            throw LoweringError (l, "Not enough components to construct " + glslTypeName (type.kind));

        scalars.resize (static_cast<size_t> (columns * rows));
        return makeConstruct (l, type, std::move (scalars));
    }

    Expr lowerVectorConstructor (const TypeSpecifier& type, std::vector<Expr> args)
    {
        const auto count = componentCount (type.kind);
        const auto scalar = scalarKindOf (type.kind);

        if (args.size() == 1)
        {
            const auto argType = typeOf (args[0]);
            const auto argCount = componentCount (argType.kind);

            if (argCount == 1)
                return makeConstruct (l, type, std::move (args)); // splat

            if (argCount >= count)
            {
                auto truncated = leading (std::move (args[0]), count);
                if (scalarKindOf (argType.kind) == scalar)
                    return truncated;

                std::vector<Expr> converted;
                converted.push_back (std::move (truncated));
                return makeConstruct (l, type, std::move (converted));
            }
        }

        int total = 0;
        bool hasMatrix = false;

        for (const auto& arg : args)
        {
            const auto argType = typeOf (arg);
            hasMatrix = hasMatrix || isMatrixType (argType.kind);
            total += std::max (componentCount (argType.kind), matrixShape (argType.kind).first * matrixShape (argType.kind).second);
        }

        if (total == count && ! hasMatrix)
            return makeConstruct (l, type, std::move (args));

        // GLSL drops the components past the ones needed; WGSL needs the exact count
        auto scalars = flattenComponents (std::move (args));
        if (static_cast<int> (scalars.size()) < count)
            throw LoweringError (l, "Not enough components to construct " + glslTypeName (type.kind));

        scalars.resize (static_cast<size_t> (count));

        for (auto& s : scalars)
            if (typeOf (s).kind != scalar)
                s = convertTo (scalar, std::move (s));

        return makeConstruct (l, type, std::move (scalars));
    }

    //==========================================================================
    // Textures
    //==========================================================================

    static bool isTextureFunction (const std::string& name)
    {
        static const std::set<std::string> names = {
            "texture", "textureLod", "textureGrad", "textureOffset", "textureLodOffset", "textureGradOffset",
            "textureProj", "textureProjLod", "textureProjGrad", "textureProjOffset", "textureProjLodOffset", "textureProjGradOffset",
            "texelFetch", "texelFetchOffset", "textureSize", "textureQueryLevels", "textureSamples", "textureGather",
            "textureGatherOffset", "textureGatherOffsets", "textureQueryLod"
        };

        return names.count (name) > 0;
    }

    struct SampledTexture
    {
        Expr texture;
        std::optional<Expr> sampler;
        TextureShape shape;
    };

    SampledTexture splitSampledTexture (Expr arg)
    {
        SampledTexture result;
        const auto type = typeOf (arg);
        result.shape = textureShape (type.kind);

        if (arg.is<ExprTypeConstructor>() && arg.as<ExprTypeConstructor>().args.size() == 2)
        {
            auto& pair = arg.as<ExprTypeConstructor>().args;
            result.texture = std::move (pair[0]);
            result.sampler = std::move (pair[1]);

            if (host.isDepthTexture (result.texture))
                result.shape.shadow = true;
        }
        else if (isSeparateTextureType (type.kind))
        {
            result.texture = std::move (arg);
            result.shape.shadow = host.isDepthTexture (result.texture);
        }
        else
        {
            throw LoweringError (arg.loc, "Texture functions need a sampler2D(texture, sampler) pair or a combined image sampler");
        }

        return result;
    }

    static int coordinateCount (const TextureShape& shape)
    {
        switch (shape.dim)
        {
            case TextureShape::Dim::d1:
                return 1;
            case TextureShape::Dim::d2:
                return 2;
            default:
                return 3;
        }
    }

    struct Coordinates
    {
        Expr coords;
        std::optional<Expr> layer;
        std::optional<Expr> depthReference;
    };

    /** Splits a GLSL texture coordinate into the WGSL coordinate, array layer and depth reference. */
    Coordinates splitCoordinates (Expr p, const TextureShape& shape, bool referenceInCoordinates, bool integerLayer)
    {
        Coordinates result;
        const auto n = coordinateCount (shape);
        const auto pType = typeOf (p);
        const auto available = componentCount (pType.kind);
        const bool needsLayer = shape.arrayed;
        const bool needsReference = shape.shadow && referenceInCoordinates;

        if (! needsLayer && ! needsReference && available == n)
        {
            result.coords = std::move (p);
            return result;
        }

        auto stableP = stable (std::move (p));
        const auto scalar = scalarKindOf (pType.kind);
        result.coords = leading (copyExpr (stableP), n);

        int next = n;

        if (needsLayer)
        {
            auto layer = component (stableP, next++, scalar);

            if (! integerLayer)
            {
                // GLSL rounds the layer: floor(layer + 0.5)
                std::vector<Expr> args;
                args.push_back (makeBinary (l, BinaryOp::add, std::move (layer), makeFloatLiteral (l, 0.5), makeType (TypeKind::floatType)));
                layer = makeCall (l, "floor", std::move (args), makeType (TypeKind::floatType));
            }

            result.layer = convertTo (TypeKind::intType, std::move (layer));
        }

        if (needsReference)
        {
            if (next >= available)
                throw LoweringError (l, "Missing depth reference in shadow texture coordinate");

            result.depthReference = component (stableP, next, scalar);
        }

        return result;
    }

    /** textureProj divides the coordinate by its last component. */
    Expr projectCoordinates (Expr p, const TextureShape& shape)
    {
        const auto pType = typeOf (p);
        const auto available = componentCount (pType.kind);
        const auto n = coordinateCount (shape) + (shape.shadow ? 1 : 0);

        if (shape.arrayed || shape.dim == TextureShape::Dim::cube)
            throw LoweringError (l, "textureProj() is not available for array and cube textures");

        auto stableP = stable (std::move (p));
        auto divisor = component (stableP, available - 1, TypeKind::floatType);
        auto numerator = leading (copyExpr (stableP), n);
        const auto resultType = makeType (vectorKind (TypeKind::floatType, n));
        return makeBinary (l, BinaryOp::div, std::move (numerator), std::move (divisor), resultType);
    }

    Expr lowerTexture (const std::string& name, std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        if (name == "textureQueryLod" || name == "textureGatherOffsets")
            throw LoweringError (l, name + "() has no WGSL equivalent");

        auto tex = splitSampledTexture (std::move (args[0]));
        const auto& shape = tex.shape;

        if (shape.dim == TextureShape::Dim::none)
            throw LoweringError (l, name + "() needs a texture argument");

        if (name == "textureSize")
            return lowerTextureSize (std::move (tex.texture), shape, args.size() > 1 ? std::optional<Expr> (std::move (args[1])) : std::nullopt, resultType);

        if (name == "textureQueryLevels")
            return convertTo (TypeKind::intType, makeCall (l, "textureNumLevels", makeArgs (std::move (tex.texture)), makeType (TypeKind::uintType)));

        if (name == "textureSamples")
            return convertTo (TypeKind::intType, makeCall (l, "textureNumSamples", makeArgs (std::move (tex.texture)), makeType (TypeKind::uintType)));

        if (name == "texelFetch" || name == "texelFetchOffset")
        {
            auto coordinates = splitCoordinates (std::move (args[1]), shape, false, true);
            std::vector<Expr> callArgs;
            callArgs.push_back (std::move (tex.texture));

            if (name == "texelFetchOffset")
            {
                auto coordinateType = coordinates.coords.type;
                coordinates.coords = makeBinary (l, BinaryOp::add, std::move (coordinates.coords), std::move (args[3]), std::move (coordinateType));
            }

            callArgs.push_back (std::move (coordinates.coords));

            if (coordinates.layer.has_value())
                callArgs.push_back (std::move (*coordinates.layer));

            if (args.size() > 2)
                callArgs.push_back (std::move (args[2])); // level or sample index

            return makeCall (l, "textureLoad", std::move (callArgs), resultType);
        }

        if (shape.sampledScalar != TypeKind::floatType && name.rfind ("textureGather", 0) != 0)
            throw LoweringError (l, "Integer textures can only be read with texelFetch() in WGSL");

        if (! tex.sampler.has_value())
            throw LoweringError (l, name + "() needs a sampler");

        if (name == "textureGather" || name == "textureGatherOffset")
            return lowerGather (name, std::move (tex), std::move (args), resultType);

        const bool isProjective = name.rfind ("textureProj", 0) == 0;
        const bool hasLod = name == "textureLod" || name == "textureLodOffset" || name == "textureProjLod" || name == "textureProjLodOffset";
        const bool hasGrad = name == "textureGrad" || name == "textureGradOffset" || name == "textureProjGrad" || name == "textureProjGradOffset";
        const bool hasOffset = name.find ("Offset") != std::string::npos;

        auto p = std::move (args[1]);
        if (isProjective)
            p = projectCoordinates (std::move (p), shape);

        // samplerCubeArrayShadow takes its depth reference as a separate argument
        const bool separateReference = shape.shadow && shape.arrayed && shape.dim == TextureShape::Dim::cube;
        auto coordinates = splitCoordinates (std::move (p), shape, ! separateReference, false);

        size_t next = 2;
        std::optional<Expr> reference = std::move (coordinates.depthReference);
        if (separateReference)
            reference = std::move (args[next++]);

        std::optional<Expr> lod;
        std::optional<Expr> ddx;
        std::optional<Expr> ddy;

        if (hasLod)
            lod = std::move (args[next++]);

        if (lod.has_value() && ! shape.shadow)
            lod = convertTo (TypeKind::floatType, std::move (*lod));

        if (hasGrad)
        {
            ddx = std::move (args[next++]);
            ddy = std::move (args[next++]);
        }

        std::optional<Expr> offset;
        if (hasOffset)
            offset = std::move (args[next++]);

        std::optional<Expr> bias;
        if (next < args.size())
            bias = convertTo (TypeKind::floatType, std::move (args[next++]));

        const bool isFragment = host.getStage() == ShaderStage::fragment;

        std::vector<Expr> callArgs;
        callArgs.push_back (std::move (tex.texture));
        callArgs.push_back (std::move (*tex.sampler));
        callArgs.push_back (std::move (coordinates.coords));

        if (coordinates.layer.has_value())
            callArgs.push_back (std::move (*coordinates.layer));

        std::string function;

        if (shape.shadow)
        {
            if (hasGrad)
                throw LoweringError (l, name + "() on a shadow sampler has no WGSL equivalent");

            if (bias.has_value())
                throw LoweringError (l, "Biased shadow texture lookups have no WGSL equivalent");

            if (lod.has_value())
            {
                const auto level = evaluateIntConstant (unparenthesized (*lod));
                const bool isZero = (level.has_value() && *level == 0) || (lod->is<ExprFloatConst>() && lod->as<ExprFloatConst>().value == 0.0);

                if (! isZero)
                    throw LoweringError (l, "WGSL only compares shadow textures at level 0");
            }

            function = (isFragment && ! lod.has_value()) ? "textureSampleCompare" : "textureSampleCompareLevel";
            callArgs.push_back (std::move (*reference));
        }
        else if (hasGrad)
        {
            function = "textureSampleGrad";
            callArgs.push_back (std::move (*ddx));
            callArgs.push_back (std::move (*ddy));
        }
        else if (lod.has_value())
        {
            function = "textureSampleLevel";
            callArgs.push_back (std::move (*lod));
        }
        else if (! isFragment)
        {
            // Implicit derivatives only exist in fragment shaders: GLSL samples the base level elsewhere
            function = "textureSampleLevel";
            callArgs.push_back (makeFloatLiteral (l, 0.0));
        }
        else if (bias.has_value())
        {
            function = "textureSampleBias";
            callArgs.push_back (std::move (*bias));
        }
        else
        {
            function = "textureSample";
        }

        if (offset.has_value())
            callArgs.push_back (std::move (*offset));

        return makeCall (l, function, std::move (callArgs), resultType);
    }

    static const Expr& unparenthesized (const Expr& e)
    {
        const auto* current = &e;
        while (current->is<ExprParen>() && current->as<ExprParen>().expr != nullptr)
            current = current->as<ExprParen>().expr.get();

        return *current;
    }

    Expr lowerGather (const std::string& name, SampledTexture tex, std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        auto coordinates = splitCoordinates (std::move (args[1]), tex.shape, false, false);
        size_t next = 2;

        // GLSL order: (sampler, P, refZ, offset) for shadow samplers, (sampler, P, offset, comp) otherwise
        std::optional<Expr> reference;
        if (tex.shape.shadow)
        {
            if (next >= args.size())
                throw LoweringError (l, "Shadow texture gathers need a depth reference");

            reference = std::move (args[next++]);
        }

        std::optional<Expr> offset;
        if (name == "textureGatherOffset")
            offset = std::move (args[next++]);

        std::vector<Expr> callArgs;

        if (tex.shape.shadow)
        {
            callArgs.push_back (std::move (tex.texture));
            callArgs.push_back (std::move (*tex.sampler));
            callArgs.push_back (std::move (coordinates.coords));

            if (coordinates.layer.has_value())
                callArgs.push_back (std::move (*coordinates.layer));

            callArgs.push_back (std::move (*reference));

            if (offset.has_value())
                callArgs.push_back (std::move (*offset));

            return makeCall (l, "textureGatherCompare", std::move (callArgs), resultType);
        }

        callArgs.push_back (next < args.size() ? std::move (args[next]) : makeIntLiteral (l, 0));
        callArgs.push_back (std::move (tex.texture));
        callArgs.push_back (std::move (*tex.sampler));
        callArgs.push_back (std::move (coordinates.coords));

        if (coordinates.layer.has_value())
            callArgs.push_back (std::move (*coordinates.layer));

        if (offset.has_value())
            callArgs.push_back (std::move (*offset));

        return makeCall (l, "textureGather", std::move (callArgs), resultType);
    }

    std::vector<Expr> makeArgs (Expr a)
    {
        std::vector<Expr> args;
        args.push_back (std::move (a));
        return args;
    }

    Expr lowerTextureSize (Expr texture, const TextureShape& shape, std::optional<Expr> lod, const std::optional<TypeSpecifier>& resultType)
    {
        auto stableTexture = stable (std::move (texture));
        const auto n = shape.dim == TextureShape::Dim::d1 ? 1 : shape.dim == TextureShape::Dim::d3 ? 3 : 2;

        std::vector<Expr> args;
        args.push_back (copyExpr (stableTexture));
        if (lod.has_value() && ! shape.multisampled)
            args.push_back (std::move (*lod));

        auto dimensions = makeCall (l, "textureDimensions", std::move (args), makeType (vectorKind (TypeKind::uintType, n)));

        if (! shape.arrayed)
            return convertTo (resultType->kind, std::move (dimensions));

        std::vector<Expr> parts;
        parts.push_back (std::move (dimensions));
        parts.push_back (makeCall (l, "textureNumLayers", makeArgs (std::move (stableTexture)), makeType (TypeKind::uintType)));
        auto combined = makeConstruct (l, makeType (vectorKind (TypeKind::uintType, n + 1)), std::move (parts));
        return convertTo (resultType->kind, std::move (combined));
    }

    //==========================================================================
    // Storage images
    //==========================================================================

    Expr lowerImage (const std::string& name, std::vector<Expr> args, const std::optional<TypeSpecifier>& resultType)
    {
        const auto type = typeOf (args[0]);
        const auto shape = textureShape (type.kind);

        if (name == "imageSize")
            return lowerTextureSize (std::move (args[0]), shape, std::nullopt, resultType);

        auto coordinates = splitCoordinates (std::move (args[1]), shape, false, true);

        std::vector<Expr> callArgs;
        callArgs.push_back (std::move (args[0]));
        callArgs.push_back (std::move (coordinates.coords));

        if (coordinates.layer.has_value())
            callArgs.push_back (std::move (*coordinates.layer));

        if (name == "imageLoad")
            return makeCall (l, "textureLoad", std::move (callArgs), resultType);

        callArgs.push_back (std::move (args[2]));
        return makeCall (l, "textureStore", std::move (callArgs));
    }

    Statements& pre;
    WgslBuiltinHost& host;
    SourceLocation l;
};

} // namespace

//==============================================================================
Expr WgslBuiltins::lowerCall (Expr call, std::vector<Statement>& pre, WgslBuiltinHost& host)
{
    const auto loc = call.loc;
    return BuiltinLowerer (pre, host, loc).lowerCall (std::move (call));
}

Expr WgslBuiltins::lowerConstructor (Expr constructor, std::vector<Statement>& pre, WgslBuiltinHost& host)
{
    const auto loc = constructor.loc;
    return BuiltinLowerer (pre, host, loc).lowerConstructor (std::move (constructor));
}

} // namespace wgsl
} // namespace yup
