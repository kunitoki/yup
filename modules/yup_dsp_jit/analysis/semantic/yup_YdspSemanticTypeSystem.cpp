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

//==============================================================================

String YdspLoopBound::toString() const
{
    switch (kind)
    {
        case YdspLoopBoundKind::constant:
            return String (constant);
        case YdspLoopBoundKind::blockSize:
            return "blockSize";
        case YdspLoopBoundKind::blockSizeMinusConst:
            return "blockSize - " + String (constant);
        case YdspLoopBoundKind::blockSizePlusConst:
            return "blockSize + " + String (constant);
    }

    return "?";
}

//==============================================================================

const YdspAnalyzedProcessor* YdspAnalyzedProgram::findProcessor (const YdspProcessorDecl* decl) const
{
    for (const auto& processor : processors)
        if (processor.decl == decl)
            return &processor;

    return nullptr;
}

//==============================================================================

namespace
{

StringRef typeName (YdspValueType type)
{
    switch (type)
    {
        case YdspValueType::float32Type:
            return "float32";
        case YdspValueType::float64Type:
            return "float64";
        case YdspValueType::int32Type:
            return "int32";
        case YdspValueType::int64Type:
            return "int64";
        case YdspValueType::boolType:
            return "bool";
    }

    return "?";
}

bool isFloatType (YdspValueType type) noexcept
{
    return type == YdspValueType::float32Type || type == YdspValueType::float64Type;
}

bool isIntType (YdspValueType type) noexcept
{
    return type == YdspValueType::int32Type || type == YdspValueType::int64Type;
}

struct IntrinsicInfo
{
    int minArgs;
    int maxArgs;
};

constexpr std::array<std::pair<const char*, IntrinsicInfo>, 40> intrinsicTable = { {
    { "abs", { 1, 1 } },
    { "sqrt", { 1, 1 } },
    { "floor", { 1, 1 } },
    { "ceil", { 1, 1 } },
    { "rint", { 1, 1 } },
    { "sin", { 1, 1 } },
    { "cos", { 1, 1 } },
    { "tan", { 1, 1 } },
    { "asin", { 1, 1 } },
    { "acos", { 1, 1 } },
    { "atan", { 1, 1 } },
    { "sinh", { 1, 1 } },
    { "cosh", { 1, 1 } },
    { "tanh", { 1, 1 } },
    { "exp", { 1, 1 } },
    { "log", { 1, 1 } },
    { "log10", { 1, 1 } },
    { "sign", { 1, 1 } },
    { "pow", { 2, 2 } },
    { "min", { 2, 2 } },
    { "max", { 2, 2 } },
    { "fmod", { 2, 2 } },
    { "atan2", { 2, 2 } },
    { "clamp", { 3, 3 } },
    { "lerp", { 3, 3 } },
    { "fma", { 3, 3 } },
    { "select", { 3, 3 } },
    { "asinh", { 1, 1 } },
    { "acosh", { 1, 1 } },
    { "atanh", { 1, 1 } },
    { "round", { 1, 1 } },
    { "copysign", { 2, 2 } },
    { "int", { 1, 1 } },
    { "int32", { 1, 1 } },
    { "int64", { 1, 1 } },
    { "float", { 1, 1 } },
    { "float32", { 1, 1 } },
    { "float64", { 1, 1 } },
    { "mem", { 1, 1 } },
    { "smooth", { 2, 2 } },
} };

constexpr std::array<std::pair<const char*, YdspValueType>, 9> builtinConstants = { {
    { "sampleRate", YdspValueType::float32Type },
    { "samplePeriod", YdspValueType::float32Type },
    { "pi", YdspValueType::float32Type },
    { "e", YdspValueType::float32Type },
    { "inf", YdspValueType::float32Type },
    { "nan", YdspValueType::float32Type },
    { "blockSize", YdspValueType::int32Type },
    { "true", YdspValueType::boolType },
    { "false", YdspValueType::boolType },
} };

const IntrinsicInfo* findIntrinsic (StringRef name) noexcept
{
    for (const auto& [intrinsicName, info] : intrinsicTable)
        if (name == StringRef (intrinsicName))
            return &info;

    return nullptr;
}

YdspValueType toValueType (YdspPrimitiveType type)
{
    switch (type)
    {
        case YdspPrimitiveType::float32Type:
            return YdspValueType::float32Type;
        case YdspPrimitiveType::float64Type:
            return YdspValueType::float64Type;
        case YdspPrimitiveType::int32Type:
            return YdspValueType::int32Type;
        case YdspPrimitiveType::int64Type:
            return YdspValueType::int64Type;
        case YdspPrimitiveType::boolType:
            return YdspValueType::boolType;
    }

    return YdspValueType::float32Type;
}
} // namespace

bool isIntrinsicName (StringRef name) noexcept
{
    return findIntrinsic (name) != nullptr;
}

//==============================================================================

YdspSemanticAnalyzer::YdspSemanticAnalyzer (YdspDiagnostics& diagnostics)
    : diagnostics (diagnostics)
{
}

void YdspSemanticAnalyzer::error (const YdspLocation& location, StringRef message)
{
    diagnostics.addError (location.line, location.column, message);
}

bool YdspSemanticAnalyzer::resolveTypeName (const YdspToken& token, YdspPrimitiveType& out) const
{
    if (token.text == "float" || token.text == "float32")
    {
        out = YdspPrimitiveType::float32Type;
        return true;
    }
    if (token.text == "float64")
    {
        out = YdspPrimitiveType::float64Type;
        return true;
    }
    if (token.text == "int" || token.text == "int32")
    {
        out = YdspPrimitiveType::int32Type;
        return true;
    }
    if (token.text == "int64")
    {
        out = YdspPrimitiveType::int64Type;
        return true;
    }
    if (token.text == "bool")
    {
        out = YdspPrimitiveType::boolType;
        return true;
    }

    return false;
}

bool YdspSemanticAnalyzer::canCoerce (YdspValueType from, YdspValueType to) const
{
    return from == to;
}

bool YdspSemanticAnalyzer::isImplicitlyConvertibleTo (YdspValueType from, YdspValueType to) const
{
    if (from == to)
        return true;

    if (from == YdspValueType::float32Type && to == YdspValueType::float64Type)
        return true;

    if (from == YdspValueType::int32Type && to == YdspValueType::int64Type)
        return true;

    if (isIntType (from) && isFloatType (to))
        return true;

    return false;
}

bool YdspSemanticAnalyzer::isAdaptableLiteral (const YdspExpr& expr) const
{
    if (expr.kind == YdspExprKind::intLiteral || expr.kind == YdspExprKind::floatLiteral)
        return true;

    if (expr.kind == YdspExprKind::unary && expr.op == YdspOperator::neg && ! expr.children.empty())
        return isAdaptableLiteral (*expr.children[0]);

    return false;
}

bool YdspSemanticAnalyzer::isAdaptableTo (const YdspExpr& expr, YdspValueType target) const
{
    if (! isAdaptableLiteral (expr))
        return false;

    const auto kind = (expr.kind == YdspExprKind::unary && ! expr.children.empty()) ? expr.children[0]->kind : expr.kind;

    if (kind == YdspExprKind::intLiteral)
        return isIntType (target) || isFloatType (target); // int literals adapt to int *and* float widths

    if (kind == YdspExprKind::floatLiteral)
        return isFloatType (target); // float literals never adapt to int

    return false;
}

std::optional<YdspValueType> YdspSemanticAnalyzer::unifyTypes (const YdspExpr& a, YdspValueType aType, const YdspExpr& b, YdspValueType bType) const
{
    if (aType == bType)
        return aType;

    if (isAdaptableTo (a, bType))
        return bType;

    if (isAdaptableTo (b, aType))
        return aType;

    return std::nullopt;
}

YdspConstValue YdspSemanticAnalyzer::constEvalDefault (const YdspExpr* expr, YdspPrimitiveType type) const
{
    const double raw = expr != nullptr ? constantValue (*expr) : 0.0;

    YdspConstValue out;
    out.type = toValueType (type);

    switch (out.type)
    {
        case YdspValueType::float32Type:
            out.asDouble = static_cast<double> (static_cast<float> (raw));
            break;
        case YdspValueType::float64Type:
            out.asDouble = raw;
            break;
        case YdspValueType::int32Type:
            out.asInt = static_cast<int64_t> (raw);
            break;
        case YdspValueType::int64Type:
            out.asInt = static_cast<int64_t> (raw);
            break;
        case YdspValueType::boolType:
            out.asBool = raw != 0.0;
            break;
    }

    return out;
}

bool YdspSemanticAnalyzer::addSymbol (const String& name, YdspSymbolInfo info, const YdspLocation& location)
{
    if (symbols.find (name) != symbols.end())
    {
        error (location, "Duplicate symbol '" + name + "'");
        return false;
    }

    symbols[name] = info;

    if (! localScopes.empty())
        localScopes.back().push_back (name);

    return true;
}

void YdspSemanticAnalyzer::pushLocalScope()
{
    localScopes.emplace_back();
}

void YdspSemanticAnalyzer::popLocalScope()
{
    for (const auto& name : localScopes.back())
        symbols.erase (name);

    localScopes.pop_back();
}

bool YdspSemanticAnalyzer::findSymbol (const String& name, YdspSymbolInfo& out) const
{
    const auto it = symbols.find (name);

    if (it == symbols.end())
        return false;

    out = it->second;
    return true;
}

double YdspSemanticAnalyzer::constantValue (const YdspExpr& expr) const
{
    double value = 0.0;
    return tryConstantFold (expr, value) ? value : 0.0;
}

bool YdspSemanticAnalyzer::tryConstantFold (const YdspExpr& expr, double& out) const
{
    switch (expr.kind)
    {
        case YdspExprKind::intLiteral:
        case YdspExprKind::floatLiteral:
            out = expr.number;
            return true;

        case YdspExprKind::boolLiteral:
            out = expr.flag ? 1.0 : 0.0;
            return true;

        case YdspExprKind::identifier:
        {
            if (expr.text == "pi")
                out = 3.14159265358979323846;
            else if (expr.text == "e")
                out = 2.71828182845904523536;
            else if (expr.text == "inf")
                out = std::numeric_limits<double>::infinity();
            else if (expr.text == "true")
                out = 1.0;
            else if (expr.text == "false")
                out = 0.0;
            else
                return false;

            return true;
        }

        case YdspExprKind::unary:
        {
            if (expr.children.empty())
                return false;

            double operand = 0.0;

            if (! tryConstantFold (*expr.children[0], operand))
                return false;

            switch (expr.op)
            {
                case YdspOperator::neg:
                    out = -operand;
                    return true;
                case YdspOperator::notI:
                    out = static_cast<double> (~static_cast<int64_t> (operand));
                    return true;
                case YdspOperator::notL:
                    out = operand != 0.0 ? 0.0 : 1.0;
                    return true;
                default:
                    return false;
            }
        }

        case YdspExprKind::binary:
        {
            if (expr.children.size() != 2)
                return false;

            double lhs = 0.0, rhs = 0.0;

            if (! tryConstantFold (*expr.children[0], lhs) || ! tryConstantFold (*expr.children[1], rhs))
                return false;

            switch (expr.op)
            {
                case YdspOperator::add:
                    out = lhs + rhs;
                    return true;
                case YdspOperator::sub:
                    out = lhs - rhs;
                    return true;
                case YdspOperator::mul:
                    out = lhs * rhs;
                    return true;
                case YdspOperator::div:
                    if (rhs == 0.0)
                        return false;
                    out = lhs / rhs;
                    return true;
                case YdspOperator::mod:
                    if (rhs == 0.0)
                        return false;
                    out = std::fmod (lhs, rhs);
                    return true;
                case YdspOperator::bitAnd:
                    out = static_cast<double> (static_cast<int64_t> (lhs) & static_cast<int64_t> (rhs));
                    return true;
                case YdspOperator::bitOr:
                    out = static_cast<double> (static_cast<int64_t> (lhs) | static_cast<int64_t> (rhs));
                    return true;
                case YdspOperator::bitXor:
                    out = static_cast<double> (static_cast<int64_t> (lhs) ^ static_cast<int64_t> (rhs));
                    return true;
                case YdspOperator::shl:
                {
                    const auto shiftAmount = static_cast<int64_t> (rhs);
                    if (shiftAmount < 0 || shiftAmount >= 64)
                        return false;
                    out = static_cast<double> (static_cast<int64_t> (lhs) << shiftAmount);
                    return true;
                }
                case YdspOperator::shr:
                {
                    const auto shiftAmount = static_cast<int64_t> (rhs);
                    if (shiftAmount < 0 || shiftAmount >= 64)
                        return false;
                    out = static_cast<double> (static_cast<int64_t> (lhs) >> shiftAmount);
                    return true;
                }
                default:
                    return false;
            }
        }

        default:
            break;
    }

    return false;
}

bool YdspSemanticAnalyzer::resolveLoopBound (const YdspExpr& expr, YdspLoopBound& out) const
{
    if (expr.kind == YdspExprKind::intLiteral)
    {
        const auto value = static_cast<long long> (expr.number);

        if (value < 0)
            return false;

        out = { YdspLoopBoundKind::constant, static_cast<int> (value) };
        return true;
    }

    if (expr.kind == YdspExprKind::identifier && expr.text == "blockSize")
    {
        out = { YdspLoopBoundKind::blockSize, 0 };
        return true;
    }

    if (expr.kind == YdspExprKind::binary && expr.children.size() == 2)
    {
        const bool lhsIsBlockSize = expr.children[0]->kind == YdspExprKind::identifier && expr.children[0]->text == "blockSize";
        const bool rhsIsBlockSize = expr.children[1]->kind == YdspExprKind::identifier && expr.children[1]->text == "blockSize";

        if (expr.op == YdspOperator::sub && lhsIsBlockSize && expr.children[1]->kind == YdspExprKind::intLiteral)
        {
            const auto value = static_cast<long long> (expr.children[1]->number);

            if (value < 0)
                return false;

            out = { YdspLoopBoundKind::blockSizeMinusConst, static_cast<int> (value) };
            return true;
        }

        if (expr.op == YdspOperator::add && (lhsIsBlockSize || rhsIsBlockSize))
        {
            const auto& constSide = lhsIsBlockSize ? *expr.children[1] : *expr.children[0];

            if (constSide.kind == YdspExprKind::intLiteral)
            {
                const auto value = static_cast<long long> (constSide.number);

                if (value < 0)
                    return false;

                out = { YdspLoopBoundKind::blockSizePlusConst, static_cast<int> (value) };
                return true;
            }
        }
    }

    return false;
}

//==============================================================================

std::unique_ptr<YdspAnalyzedProgram> YdspSemanticAnalyzer::analyze (std::unique_ptr<YdspProgram> program)
{
    jassert (program != nullptr);

    auto analyzed = std::make_unique<YdspAnalyzedProgram>();
    analyzed->ast = std::move (program);

    if (analyzed->ast->graphs.empty())
    {
        diagnostics.addError (0, 0, "The program must define at least one graph");
        return nullptr;
    }

    static constexpr std::array<const char*, 5> allowedDeclareKeys = {
        "name", "author", "version", "license", "description"
    };

    for (const auto& declare : analyzed->ast->declares)
    {
        const auto isAllowed = std::any_of (allowedDeclareKeys.begin(), allowedDeclareKeys.end(), [&] (const char* key)
        {
            return declare.key == key;
        });

        if (! isAllowed)
            diagnostics.addWarning (declare.location.line, declare.location.column, "Unknown metadata key '" + declare.key + "'");
    }

    preprocessProgram (*analyzed->ast);

    if (diagnostics.hasErrors())
        return nullptr;

    analyzeProgramFunctions (*analyzed->ast, analyzed->functions);

    if (diagnostics.hasErrors())
        return nullptr;

    for (const auto& processorDecl : analyzed->ast->processors)
    {
        auto proc = analyzeProcessor (processorDecl);
        analyzed->processors.push_back (std::move (*proc));
    }

    if (diagnostics.hasErrors())
        return nullptr;

    const int mainIndex = selectMainGraph (*analyzed->ast);

    if (mainIndex < 0)
        return nullptr;

    std::vector<int> graphOrder;

    if (! orderGraphsByDependency (*analyzed->ast, mainIndex, graphOrder) || diagnostics.hasErrors())
        return nullptr;

    std::vector<YdspAnalyzedGraph> analyzedGraphs (analyzed->ast->graphs.size());

    for (const int g : graphOrder)
    {
        const auto& graphDecl = analyzed->ast->graphs[static_cast<size_t> (g)];

        auto gr = analyzeGraph (graphDecl, *analyzed);

        if (diagnostics.hasErrors())
            return nullptr;

        inlineSubgraphs (*gr, graphDecl, analyzedGraphs);

        if (diagnostics.hasErrors())
            return nullptr;

        analyzedGraphs[static_cast<size_t> (g)] = std::move (*gr);
    }

    analyzed->graph = std::move (analyzedGraphs[static_cast<size_t> (mainIndex)]);

    fuseNodeChains (*analyzed);

    if (diagnostics.hasErrors())
        return nullptr;

    computeLatencyAndCompensate (analyzed->graph, analyzed->ast->graphs[static_cast<size_t> (mainIndex)].location);

    if (diagnostics.hasErrors())
        return nullptr;

    return analyzed;
}

} // namespace yup
