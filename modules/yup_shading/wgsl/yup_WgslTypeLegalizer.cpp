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

class TypeLegalizer
{
public:
    void run (TranslationUnit& ast)
    {
        pushScope();

        declareBuiltins();
        collectStructsAndFunctions (ast);

        for (auto& decl : ast.declarations)
        {
            if (auto* declaration = std::get_if<Declaration> (&decl))
                visitDeclaration (*declaration);
            else if (auto* function = std::get_if<FunctionDefinition> (&decl))
                visitFunction (*function);
        }

        popScope();
    }

private:
    using ExprType = std::optional<TypeSpecifier>;

    //==========================================================================
    /** GLSL converts int to uint, and both to float. Bool never converts implicitly. */
    static int conversionRank (TypeKind scalar)
    {
        switch (scalar)
        {
            case TypeKind::intType:
                return 1;
            case TypeKind::uintType:
                return 2;
            case TypeKind::floatType:
                return 3;
            default:
                return 0;
        }
    }

    static TypeKind promote (TypeKind a, TypeKind b)
    {
        if (conversionRank (a) == 0 || conversionRank (b) == 0)
            return TypeKind::voidType;

        return conversionRank (a) >= conversionRank (b) ? a : b;
    }

    static ExprType typeOf (const TypeSpecifier& type)
    {
        if (type.kind == TypeKind::voidType)
            return std::nullopt;

        return type;
    }

    static ExprType makeType (TypeKind kind)
    {
        return typeOf (TypeSpecifier::make ({}, kind));
    }

    /** True for a scalar or vector value, the only types conversions apply to. */
    static bool isNumeric (const ExprType& type)
    {
        return type.has_value()
            && type->arraySpecifiers.empty()
            && componentCount (type->kind) > 0;
    }

    static bool isOneOf (const std::string& name, std::initializer_list<const char*> names)
    {
        for (const auto* candidate : names)
        {
            if (name == candidate)
                return true;
        }

        return false;
    }

    static bool isSwizzle (const std::string& member)
    {
        if (member.empty() || member.size() > 4)
            return false;

        return member.find_first_not_of ("xyzwrgbastpq") == std::string::npos;
    }

    /** Abstract WGSL literals convert on their own: integers to any numeric scalar, floats to f32. */
    static bool adaptsTo (const Expr& expr, TypeKind scalar)
    {
        if (expr.is<ExprIntConst>())
            return scalar == TypeKind::intType || scalar == TypeKind::uintType || scalar == TypeKind::floatType;

        if (expr.is<ExprFloatConst>())
            return scalar == TypeKind::floatType;

        if (expr.is<ExprParen>())
        {
            const auto& paren = expr.as<ExprParen>();
            return paren.expr != nullptr && adaptsTo (*paren.expr, scalar);
        }

        if (expr.is<ExprUnary>())
        {
            const auto& unary = expr.as<ExprUnary>();
            return (unary.op == UnaryOp::minus || unary.op == UnaryOp::plus)
                && unary.operand != nullptr
                && adaptsTo (*unary.operand, scalar);
        }

        return false;
    }

    /** Re-types an abstract literal expression in place after it adapted to @p kind. */
    static void retypeLiteral (Expr& expr, TypeKind kind)
    {
        expr.type = TypeSpecifier::make (expr.loc, kind);

        if (expr.is<ExprParen>() && expr.as<ExprParen>().expr != nullptr)
            retypeLiteral (*expr.as<ExprParen>().expr, kind);
        else if (expr.is<ExprUnary>() && expr.as<ExprUnary>().operand != nullptr)
            retypeLiteral (*expr.as<ExprUnary>().operand, kind);
    }

    static void wrap (Expr& expr, TypeKind kind)
    {
        ExprTypeConstructor constructor;
        constructor.loc = expr.loc;
        constructor.type = TypeSpecifier::make (expr.loc, kind);
        constructor.args.push_back (std::move (expr));

        Expr wrapped;
        wrapped.loc = constructor.loc;
        wrapped.value = std::move (constructor);
        wrapped.type = TypeSpecifier::make (wrapped.loc, kind);
        expr = std::move (wrapped);
    }

    /** Converts a scalar or vector expression to another scalar type, splatting scalars to vector targets. */
    static void coerce (Expr& expr, const ExprType& from, TypeKind to, bool allowBool = false)
    {
        if (! isNumeric (from))
            return;

        const auto fromScalar = scalarKindOf (from->kind);
        const auto toScalar = scalarKindOf (to);
        const auto fromCount = componentCount (from->kind);
        const auto toCount = componentCount (to);

        if (toCount == 0 || toScalar == TypeKind::voidType)
            return;

        if (! allowBool && (fromScalar == TypeKind::boolType || toScalar == TypeKind::boolType))
            return;

        const bool splat = fromCount == 1 && toCount > 1;
        if (! splat && fromCount != toCount)
            return;

        if (fromScalar != toScalar)
        {
            if (adaptsTo (expr, toScalar))
                retypeLiteral (expr, vectorKind (toScalar, fromCount));
            else
                wrap (expr, vectorKind (toScalar, fromCount));
        }

        if (splat)
            wrap (expr, to);
    }

    //==========================================================================
    void pushScope() { scopes.emplace_back(); }

    void popScope() { scopes.pop_back(); }

    void declare (const std::string& name, TypeSpecifier type)
    {
        scopes.back()[name] = std::move (type);
    }

    const TypeSpecifier* lookup (const std::string& name) const
    {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
        {
            if (auto found = it->find (name); found != it->end())
                return &found->second;
        }

        return nullptr;
    }

    void declareBuiltins()
    {
        static const std::pair<const char*, TypeKind> builtins[] = {
            { "gl_Position", TypeKind::vec4 },
            { "gl_FragCoord", TypeKind::vec4 },
            { "gl_FragDepth", TypeKind::floatType },
            { "gl_FrontFacing", TypeKind::boolType },
            { "gl_PointSize", TypeKind::floatType },
            { "gl_VertexIndex", TypeKind::intType },
            { "gl_VertexID", TypeKind::intType },
            { "gl_InstanceIndex", TypeKind::intType },
            { "gl_InstanceID", TypeKind::intType },
            { "gl_SampleID", TypeKind::intType },
            { "gl_GlobalInvocationID", TypeKind::uvec3 },
            { "gl_LocalInvocationID", TypeKind::uvec3 },
            { "gl_WorkGroupID", TypeKind::uvec3 },
            { "gl_NumWorkGroups", TypeKind::uvec3 },
            { "gl_WorkGroupSize", TypeKind::uvec3 },
            { "gl_LocalInvocationIndex", TypeKind::uintType }
        };

        for (const auto& [name, kind] : builtins)
            declare (name, TypeSpecifier::make ({}, kind));

        // int gl_SampleMask[1] / gl_SampleMaskIn[1]
        auto maskType = TypeSpecifier::make ({}, TypeKind::intType);
        ArraySpecifier size;
        size.sizeExpr = std::make_unique<Expr>();
        size.sizeExpr->value = ExprIntConst { {}, 1 };
        maskType.arraySpecifiers.push_back (std::move (size));
        declare ("gl_SampleMask", maskType);
        declare ("gl_SampleMaskIn", maskType);
    }

    void collectStructsAndFunctions (const TranslationUnit& ast)
    {
        for (const auto& decl : ast.declarations)
        {
            if (const auto* function = std::get_if<FunctionDefinition> (&decl))
            {
                const auto& proto = function->prototype;
                functions.emplace (proto.originalName.empty() ? proto.name : proto.originalName, &proto);
                continue;
            }

            const auto* declaration = std::get_if<Declaration> (&decl);
            if (declaration == nullptr || declaration->structSpecifier == nullptr)
                continue;

            const auto& structSpecifier = *declaration->structSpecifier;
            if (! structSpecifier.name.empty())
                structs[structSpecifier.name] = &structSpecifier;
        }
    }

    static int conversionCost (const ExprType& from, const TypeSpecifier& to)
    {
        if (! from.has_value())
            return -1;

        if (sameType (*from, to))
            return 0;

        if (! isNumeric (from) || ! to.arraySpecifiers.empty() || componentCount (from->kind) != componentCount (to.kind))
            return -1;

        const auto fromRank = conversionRank (scalarKindOf (from->kind));
        const auto toRank = conversionRank (scalarKindOf (to.kind));
        return fromRank > 0 && toRank > fromRank ? 1 : -1;
    }

    /** GLSL overload resolution: an exact match wins, otherwise the one candidate reachable by implicit conversions. */
    const FunctionPrototype* findFunction (const std::string& name, const std::vector<ExprType>& argumentTypes, SourceLocation loc) const
    {
        const FunctionPrototype* exact = nullptr;
        std::vector<const FunctionPrototype*> convertible;

        const auto [first, last] = functions.equal_range (name);
        for (auto it = first; it != last; ++it)
        {
            const auto& params = it->second->parameters;
            if (params.size() != argumentTypes.size())
                continue;

            int total = 0;
            for (std::size_t i = 0; i < params.size() && total >= 0; ++i)
            {
                const auto paramType = declaratorType (params[i].type, params[i].arraySpecifiers);

                const bool isOut = params[i].qualifier != nullptr
                                && (params[i].qualifier->hasStorage (StorageQualifier::out) || params[i].qualifier->hasStorage (StorageQualifier::inout));

                // Opaque and unknown argument types match loosely: glslang already checked the call
                const int cost = argumentTypes[i].has_value() ? conversionCost (argumentTypes[i], paramType) : 0;
                total = (cost < 0 || (isOut && cost > 0)) ? -1 : total + cost;
            }

            if (total == 0)
            {
                if (exact != nullptr)
                    throw LoweringError (loc, "Ambiguous call to overloaded function '" + name + "'");

                exact = it->second;
            }
            else if (total > 0)
            {
                convertible.push_back (it->second);
            }
        }

        if (exact != nullptr)
            return exact;

        if (convertible.size() > 1)
            throw LoweringError (loc, "Ambiguous call to overloaded function '" + name + "'");

        return convertible.empty() ? nullptr : convertible.front();
    }

    //==========================================================================
    void visitDeclaration (Declaration& declaration)
    {
        if (declaration.initDeclaratorList == nullptr)
            return;

        auto& list = *declaration.initDeclaratorList;
        for (auto& single : list.declarations)
        {
            auto type = declaratorType (list.type, single.arraySpecifiers);

            if (single.initializer != nullptr && single.initializer->expr != nullptr)
            {
                const auto initializerType = visit (*single.initializer->expr);

                if (type.arraySpecifiers.empty())
                    coerce (*single.initializer->expr, initializerType, type.kind);
                else if (type.arraySpecifiers.front().isUnsized && initializerType.has_value() && ! initializerType->arraySpecifiers.empty())
                    type.arraySpecifiers.front() = initializerType->arraySpecifiers.front();
            }

            declare (single.name, std::move (type));
        }
    }

    void visitFunction (FunctionDefinition& function)
    {
        pushScope();

        for (const auto& parameter : function.prototype.parameters)
        {
            declare (parameter.name, declaratorType (parameter.type, parameter.arraySpecifiers));
        }

        returnType = function.prototype.returnType;

        if (function.body != nullptr)
            visitStatement (*function.body);

        popScope();
    }

    void visitIfPresent (std::unique_ptr<Expr>& expr)
    {
        if (expr != nullptr)
            visit (*expr);
    }

    void visitIfPresent (std::unique_ptr<Statement>& statement)
    {
        if (statement != nullptr)
            visitStatement (*statement);
    }

    void visitStatement (Statement& statement)
    {
        if (statement.is<StmtCompound>())
        {
            pushScope();

            for (auto& child : statement.as<StmtCompound>().statements)
                visitStatement (child);

            popScope();
        }
        else if (statement.is<StmtDeclaration>())
        {
            visitDeclaration (statement.as<StmtDeclaration>().declaration);
        }
        else if (statement.is<StmtExpr>())
        {
            visitIfPresent (statement.as<StmtExpr>().expr);
        }
        else if (statement.is<StmtSelection>())
        {
            auto& selection = statement.as<StmtSelection>();
            visitIfPresent (selection.condition);
            visitIfPresent (selection.thenBranch);
            visitIfPresent (selection.elseBranch);
        }
        else if (statement.is<StmtSwitch>())
        {
            auto& switchStatement = statement.as<StmtSwitch>();
            const auto selectorType = switchStatement.selector != nullptr ? visit (*switchStatement.selector) : std::nullopt;

            pushScope();

            for (auto& child : switchStatement.body)
            {
                if (child.is<StmtCaseLabel>() && child.as<StmtCaseLabel>().label != nullptr)
                {
                    auto& label = *child.as<StmtCaseLabel>().label;
                    const auto labelType = visit (label);

                    if (isNumeric (selectorType))
                        coerce (label, labelType, selectorType->kind);
                }
                else
                {
                    visitStatement (child);
                }
            }

            popScope();
        }
        else if (statement.is<StmtWhile>())
        {
            auto& loop = statement.as<StmtWhile>();
            visitIfPresent (loop.condition);
            visitIfPresent (loop.body);
        }
        else if (statement.is<StmtDoWhile>())
        {
            auto& loop = statement.as<StmtDoWhile>();
            visitIfPresent (loop.body);
            visitIfPresent (loop.condition);
        }
        else if (statement.is<StmtFor>())
        {
            auto& loop = statement.as<StmtFor>();

            pushScope();

            visitIfPresent (loop.init);
            visitIfPresent (loop.condition);
            visitIfPresent (loop.update);
            visitIfPresent (loop.body);

            popScope();
        }
        else if (statement.is<StmtJump>())
        {
            auto& jump = statement.as<StmtJump>();
            if (jump.returnValue == nullptr)
                return;

            const auto valueType = visit (*jump.returnValue);

            if (returnType.arraySpecifiers.empty())
                coerce (*jump.returnValue, valueType, returnType.kind);
        }
    }

    //==========================================================================
    ExprType visit (Expr& expr)
    {
        auto type = visitExpression (expr);

        // Conversions inserted by the visitor replace expr with a typed constructor
        if (! expr.type.has_value())
            expr.type = type;

        return type;
    }

    ExprType visitExpression (Expr& expr)
    {
        if (expr.is<ExprVariable>())
        {
            if (const auto* type = lookup (expr.as<ExprVariable>().name))
                return *type;

            return std::nullopt;
        }

        if (expr.is<ExprIntConst>())
            return makeType (TypeKind::intType);

        if (expr.is<ExprUIntConst>())
            return makeType (TypeKind::uintType);

        if (expr.is<ExprFloatConst>())
            return makeType (TypeKind::floatType);

        if (expr.is<ExprBoolConst>())
            return makeType (TypeKind::boolType);

        if (expr.is<ExprParen>())
        {
            auto& paren = expr.as<ExprParen>();
            if (paren.expr == nullptr)
                return std::nullopt;

            return visit (*paren.expr);
        }

        if (expr.is<ExprUnary>())
        {
            auto& unary = expr.as<ExprUnary>();
            if (unary.operand == nullptr)
                return std::nullopt;

            return visit (*unary.operand);
        }

        if (expr.is<ExprComma>())
        {
            auto& comma = expr.as<ExprComma>();
            visitIfPresent (comma.left);

            if (comma.right == nullptr)
                return std::nullopt;

            return visit (*comma.right);
        }

        if (expr.is<ExprBinary>())
            return visitBinary (expr.as<ExprBinary>());

        if (expr.is<ExprTernary>())
            return visitTernary (expr.as<ExprTernary>());

        if (expr.is<ExprAssignment>())
            return visitAssignment (expr.as<ExprAssignment>());

        if (expr.is<ExprBracket>())
            return visitBracket (expr.as<ExprBracket>());

        if (expr.is<ExprDot>())
            return visitDot (expr.as<ExprDot>());

        if (expr.is<ExprFunCall>())
            return visitCall (expr.as<ExprFunCall>());

        if (expr.is<ExprTypeConstructor>())
            return visitConstructor (expr.as<ExprTypeConstructor>());

        return std::nullopt;
    }

    /** Converts both operands to their promoted scalar type and returns the type of the result. */
    static ExprType unify (Expr& a, const ExprType& aType, Expr& b, const ExprType& bType)
    {
        if (! isNumeric (aType) || ! isNumeric (bType))
            return std::nullopt;

        const auto scalar = promote (scalarKindOf (aType->kind), scalarKindOf (bType->kind));
        if (scalar == TypeKind::voidType)
            return std::nullopt;

        const auto aCount = componentCount (aType->kind);
        const auto bCount = componentCount (bType->kind);

        coerce (a, aType, vectorKind (scalar, aCount));
        coerce (b, bType, vectorKind (scalar, bCount));

        return makeType (vectorKind (scalar, std::max (aCount, bCount)));
    }

    static ExprType matrixProduct (const TypeSpecifier& left, const TypeSpecifier& right)
    {
        const auto [leftColumns, leftRows] = matrixShape (left.kind);
        const auto [rightColumns, rightRows] = matrixShape (right.kind);

        if (leftColumns > 0 && rightColumns > 0)
            return makeType (matrixKind (rightColumns, leftRows));

        if (leftColumns > 0)
            return componentCount (right.kind) > 1 ? makeType (vectorKind (TypeKind::floatType, leftRows)) : typeOf (left);

        return componentCount (left.kind) > 1 ? makeType (vectorKind (TypeKind::floatType, rightColumns)) : typeOf (right);
    }

    ExprType visitBinary (ExprBinary& binary)
    {
        if (binary.left == nullptr || binary.right == nullptr)
            return std::nullopt;

        const auto leftType = visit (*binary.left);
        const auto rightType = visit (*binary.right);

        switch (binary.op)
        {
            case BinaryOp::logicalAnd:
            case BinaryOp::logicalOr:
            case BinaryOp::logicalXor:
                return makeType (TypeKind::boolType);

            case BinaryOp::shiftLeft:
            case BinaryOp::shiftRight:
                // WGSL shifts a vector by a vector of u32
                if (isNumeric (rightType))
                {
                    const auto count = isNumeric (leftType) ? std::max (componentCount (leftType->kind), componentCount (rightType->kind))
                                                            : componentCount (rightType->kind);
                    coerce (*binary.right, rightType, vectorKind (TypeKind::uintType, count));
                }

                return leftType;

            case BinaryOp::bitwiseAnd:
            case BinaryOp::bitwiseOr:
            case BinaryOp::bitwiseXor:
            {
                // WGSL bitwise operands must have the same type: splat a scalar to the vector's size
                auto result = unify (*binary.left, leftType, *binary.right, rightType);
                if (! result.has_value())
                    return result;

                const auto count = componentCount (result->kind);
                const auto scalar = makeType (scalarKindOf (result->kind));

                if (componentCount (leftType->kind) == 1 && count > 1)
                    coerce (*binary.left, scalar, result->kind);

                if (componentCount (rightType->kind) == 1 && count > 1)
                    coerce (*binary.right, scalar, result->kind);

                return result;
            }

            case BinaryOp::lessThan:
            case BinaryOp::greaterThan:
            case BinaryOp::lessEqual:
            case BinaryOp::greaterEqual:
            case BinaryOp::equal:
            case BinaryOp::notEqual:
                // GLSL == and != compare whole values to a single bool, even for vectors
                unify (*binary.left, leftType, *binary.right, rightType);
                return makeType (TypeKind::boolType);

            default:
                break;
        }

        if (! leftType.has_value() || ! rightType.has_value())
            return std::nullopt;

        const bool leftIsMatrix = isMatrixType (leftType->kind);
        const bool rightIsMatrix = isMatrixType (rightType->kind);

        if (! leftIsMatrix && ! rightIsMatrix)
            return unify (*binary.left, leftType, *binary.right, rightType);

        // Scalars and vectors combined with float matrices convert to float
        if (! leftIsMatrix && isNumeric (leftType))
            coerce (*binary.left, leftType, vectorKind (TypeKind::floatType, componentCount (leftType->kind)));

        if (! rightIsMatrix && isNumeric (rightType))
            coerce (*binary.right, rightType, vectorKind (TypeKind::floatType, componentCount (rightType->kind)));

        if (binary.op == BinaryOp::mul)
            return matrixProduct (*leftType, *rightType);

        return leftIsMatrix ? leftType : rightType;
    }

    ExprType visitTernary (ExprTernary& ternary)
    {
        visitIfPresent (ternary.condition);

        if (ternary.trueBranch == nullptr || ternary.falseBranch == nullptr)
            return std::nullopt;

        const auto trueType = visit (*ternary.trueBranch);
        const auto falseType = visit (*ternary.falseBranch);

        if (auto unified = unify (*ternary.trueBranch, trueType, *ternary.falseBranch, falseType))
            return unified;

        return trueType.has_value() ? trueType : falseType;
    }

    ExprType visitAssignment (ExprAssignment& assignment)
    {
        if (assignment.lhs == nullptr || assignment.rhs == nullptr)
            return std::nullopt;

        const auto targetType = visit (*assignment.lhs);
        const auto valueType = visit (*assignment.rhs);

        if (! isNumeric (targetType) || ! isNumeric (valueType))
            return targetType;

        const auto valueCount = componentCount (valueType->kind);

        switch (assignment.op)
        {
            case AssignmentOp::assign:
                coerce (*assignment.rhs, valueType, targetType->kind);
                break;

            case AssignmentOp::shiftLeftAssign:
            case AssignmentOp::shiftRightAssign:
                coerce (*assignment.rhs, valueType, vectorKind (TypeKind::uintType, std::max (valueCount, componentCount (targetType->kind))));
                break;

            case AssignmentOp::bitwiseAndAssign:
            case AssignmentOp::bitwiseOrAssign:
            case AssignmentOp::bitwiseXorAssign:
                coerce (*assignment.rhs, valueType, targetType->kind);
                break;

            default:
                coerce (*assignment.rhs, valueType, vectorKind (scalarKindOf (targetType->kind), valueCount));
                break;
        }

        return targetType;
    }

    ExprType visitBracket (ExprBracket& bracket)
    {
        visitIfPresent (bracket.index);

        if (bracket.base == nullptr)
            return std::nullopt;

        auto baseType = visit (*bracket.base);
        if (! baseType.has_value())
            return std::nullopt;

        if (! baseType->arraySpecifiers.empty())
        {
            baseType->arraySpecifiers.erase (baseType->arraySpecifiers.begin());
            return baseType;
        }

        if (componentCount (baseType->kind) > 1)
            return makeType (scalarKindOf (baseType->kind));

        const auto rows = matrixShape (baseType->kind).second;
        if (rows > 0)
            return makeType (vectorKind (TypeKind::floatType, rows));

        return std::nullopt;
    }

    ExprType visitDot (ExprDot& dot)
    {
        if (dot.base == nullptr)
            return std::nullopt;

        const auto baseType = visit (*dot.base);
        if (! baseType.has_value() || ! baseType->arraySpecifiers.empty())
            return std::nullopt;

        if (baseType->kind == TypeKind::namedStruct)
        {
            const auto found = structs.find (baseType->structName);
            if (found == structs.end())
                return std::nullopt;

            for (const auto& field : found->second->fields)
            {
                if (field.name == dot.member)
                    return field.type;
            }

            return std::nullopt;
        }

        if (componentCount (baseType->kind) == 0 || ! isSwizzle (dot.member))
            return std::nullopt;

        return makeType (vectorKind (scalarKindOf (baseType->kind), static_cast<int> (dot.member.size())));
    }

    ExprType visitConstructor (ExprTypeConstructor& constructor)
    {
        std::vector<ExprType> argumentTypes;
        argumentTypes.reserve (constructor.args.size());

        for (auto& argument : constructor.args)
            argumentTypes.push_back (visit (argument));

        const auto& type = constructor.type;
        const auto scalar = scalarKindOf (type.kind);

        if (! type.arraySpecifiers.empty())
        {
            auto elementType = type;
            elementType.arraySpecifiers.erase (elementType.arraySpecifiers.begin());

            if (elementType.arraySpecifiers.empty())
            {
                for (std::size_t i = 0; i < constructor.args.size(); ++i)
                    coerce (constructor.args[i], argumentTypes[i], elementType.kind);
            }

            // An unsized array constructor takes its size from the argument count
            auto result = type;
            if (result.arraySpecifiers.front().isUnsized)
            {
                auto& size = result.arraySpecifiers.front();
                size.isUnsized = false;
                size.sizeExpr = std::make_unique<Expr>();
                size.sizeExpr->loc = constructor.loc;
                size.sizeExpr->value = ExprIntConst { constructor.loc, static_cast<int64_t> (constructor.args.size()) };
                constructor.type = result;
            }

            return result;
        }

        if (type.kind == TypeKind::namedStruct)
        {
            if (const auto found = structs.find (type.structName); found != structs.end())
            {
                const auto& fields = found->second->fields;

                for (std::size_t i = 0; i < constructor.args.size() && i < fields.size(); ++i)
                {
                    if (fields[i].type.arraySpecifiers.empty())
                        coerce (constructor.args[i], argumentTypes[i], fields[i].type.kind);
                }
            }

            return type;
        }

        // Samplers keep their arguments untouched
        if (scalar == TypeKind::voidType)
            return typeOf (type);

        if (isMatrixType (type.kind))
        {
            for (std::size_t i = 0; i < constructor.args.size(); ++i)
            {
                if (isNumeric (argumentTypes[i]))
                    coerce (constructor.args[i], argumentTypes[i], vectorKind (TypeKind::floatType, componentCount (argumentTypes[i]->kind)));
            }

            return type;
        }

        if (constructor.args.size() == 1)
        {
            // A single argument converts on its own, except a scalar splatted to a vector
            if (isNumeric (argumentTypes[0]) && componentCount (argumentTypes[0]->kind) == 1 && componentCount (type.kind) > 1)
                coerce (constructor.args[0], argumentTypes[0], scalar, true);

            return type;
        }

        for (std::size_t i = 0; i < constructor.args.size(); ++i)
        {
            if (isNumeric (argumentTypes[i]))
                coerce (constructor.args[i], argumentTypes[i], vectorKind (scalar, componentCount (argumentTypes[i]->kind)), true);
        }

        return type;
    }

    ExprType visitCall (ExprFunCall& call)
    {
        std::vector<ExprType> argumentTypes;
        argumentTypes.reserve (call.args.size());

        for (auto& argument : call.args)
            argumentTypes.push_back (visit (argument));

        if (call.callee == nullptr)
            return std::nullopt;

        // Method call: only array.length() and vector/matrix .length() exist in GLSL
        if (call.callee->is<ExprDot>())
        {
            auto& dot = call.callee->as<ExprDot>();
            if (dot.base != nullptr)
                visit (*dot.base);

            return dot.member == "length" ? makeType (TypeKind::intType) : std::nullopt;
        }

        if (! call.callee->is<ExprVariable>())
            return std::nullopt;

        auto& name = call.callee->as<ExprVariable>().name;

        if (const auto found = structs.find (name); found != structs.end())
        {
            const auto& fields = found->second->fields;

            for (std::size_t i = 0; i < call.args.size() && i < fields.size(); ++i)
            {
                if (fields[i].type.arraySpecifiers.empty())
                    coerce (call.args[i], argumentTypes[i], fields[i].type.kind);
            }

            return TypeSpecifier::makeNamed (call.loc, name);
        }

        if (functions.count (name) > 0)
        {
            const auto* prototype = findFunction (name, argumentTypes, call.loc);
            if (prototype == nullptr)
                return std::nullopt;

            name = prototype->name;

            for (std::size_t i = 0; i < call.args.size(); ++i)
            {
                const auto& parameter = prototype->parameters[i];
                const bool isReference = parameter.qualifier != nullptr
                                      && (parameter.qualifier->hasStorage (StorageQualifier::out)
                                          || parameter.qualifier->hasStorage (StorageQualifier::inout));

                if (! isReference && parameter.arraySpecifiers.empty() && parameter.type.arraySpecifiers.empty())
                    coerce (call.args[i], argumentTypes[i], parameter.type.kind);
            }

            return typeOf (prototype->returnType);
        }

        return visitBuiltinCall (name, call, argumentTypes);
    }

    /** Converts the arguments in [first, last) to a common scalar type, optionally splatting
        scalars to the widest vector, and returns the common type.
    */
    static ExprType unifyArguments (ExprFunCall& call,
                                    const std::vector<ExprType>& types,
                                    std::size_t first,
                                    std::size_t last,
                                    bool floatOnly,
                                    bool splat)
    {
        if (last > call.args.size() || first >= last)
            return std::nullopt;

        auto scalar = floatOnly ? TypeKind::floatType : TypeKind::intType;
        int count = 1;

        for (std::size_t i = first; i < last; ++i)
        {
            if (! isNumeric (types[i]))
                return std::nullopt;

            scalar = promote (scalar, scalarKindOf (types[i]->kind));
            count = std::max (count, componentCount (types[i]->kind));
        }

        if (scalar == TypeKind::voidType)
            return std::nullopt;

        for (std::size_t i = first; i < last; ++i)
            coerce (call.args[i], types[i], vectorKind (scalar, splat ? count : componentCount (types[i]->kind)));

        return makeType (vectorKind (scalar, count));
    }

    static ExprType textureResult (const ExprType& samplerType)
    {
        if (! samplerType.has_value())
            return std::nullopt;

        const auto shape = textureShape (samplerType->kind);
        if (shape.dim == TextureShape::Dim::none)
            return std::nullopt;

        if (shape.shadow)
            return makeType (TypeKind::floatType);

        return makeType (vectorKind (shape.sampledScalar, 4));
    }

    static ExprType textureSizeResult (const ExprType& samplerType)
    {
        if (! samplerType.has_value())
            return std::nullopt;

        const auto shape = textureShape (samplerType->kind);
        int count = 0;

        switch (shape.dim)
        {
            case TextureShape::Dim::d1:
            case TextureShape::Dim::buffer:
                count = 1;
                break;
            case TextureShape::Dim::d2:
            case TextureShape::Dim::cube:
            case TextureShape::Dim::rect:
                count = 2;
                break;
            case TextureShape::Dim::d3:
                count = 3;
                break;
            case TextureShape::Dim::none:
                return std::nullopt;
        }

        if (shape.arrayed)
            ++count;

        return makeType (vectorKind (TypeKind::intType, count));
    }

    static ExprType withComponents (const ExprType& type, TypeKind scalar)
    {
        if (! isNumeric (type))
            return std::nullopt;

        return makeType (vectorKind (scalar, componentCount (type->kind)));
    }

    static ExprType visitBuiltinCall (const std::string& name, ExprFunCall& call, const std::vector<ExprType>& types)
    {
        const auto count = call.args.size();

        if (isOneOf (name, { "barrier", "memoryBarrier", "memoryBarrierShared", "memoryBarrierBuffer", "memoryBarrierImage",
                             "groupMemoryBarrier", "imageStore" }))
            return std::nullopt;

        if (count == 0)
            return std::nullopt;

        if (isOneOf (name, { "texture", "textureLod", "textureGrad", "textureOffset", "textureLodOffset", "textureGradOffset",
                             "textureProj", "textureProjLod", "textureProjGrad", "textureProjOffset", "textureProjLodOffset",
                             "textureProjGradOffset", "texelFetch", "texelFetchOffset", "imageLoad" }))
            return textureResult (types[0]);

        if (isOneOf (name, { "textureGather", "textureGatherOffset" }))
        {
            if (! types[0].has_value())
                return std::nullopt;

            const auto shape = textureShape (types[0]->kind);
            return makeType (vectorKind (shape.shadow ? TypeKind::floatType : shape.sampledScalar, 4));
        }

        if (isOneOf (name, { "textureSize", "imageSize" }))
            return textureSizeResult (types[0]);

        if (isOneOf (name, { "textureQueryLevels", "textureSamples", "imageSamples" }))
            return makeType (TypeKind::intType);

        if (isOneOf (name, { "min", "max", "clamp" }))
            return unifyArguments (call, types, 0, count, false, true);

        if (isOneOf (name, { "abs", "sign" }))
            return unifyArguments (call, types, 0, count, false, false);

        if (isOneOf (name, { "step", "smoothstep" }))
            return unifyArguments (call, types, 0, count, true, true);

        if (name == "mix" && count == 3)
        {
            // A bool selector picks components, a float one interpolates
            if (isNumeric (types[2]) && scalarKindOf (types[2]->kind) == TypeKind::boolType)
                return unifyArguments (call, types, 0, 2, false, false);

            auto result = unifyArguments (call, types, 0, 2, true, false);

            if (isNumeric (types[2]))
                coerce (call.args[2], types[2], vectorKind (TypeKind::floatType, componentCount (types[2]->kind)));

            return result;
        }

        if (name == "fma" && count == 3)
            return unifyArguments (call, types, 0, 3, true, false);

        if (name == "refract" && count == 3)
        {
            auto result = unifyArguments (call, types, 0, 2, true, false);
            coerce (call.args[2], types[2], TypeKind::floatType);
            return result;
        }

        if (isOneOf (name, { "length", "distance", "dot" }))
        {
            unifyArguments (call, types, 0, count, true, false);
            return makeType (TypeKind::floatType);
        }

        if (name == "cross")
        {
            unifyArguments (call, types, 0, count, true, false);
            return makeType (TypeKind::vec3);
        }

        if (isOneOf (name, { "lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual", "equal", "notEqual" }))
        {
            const auto unified = unifyArguments (call, types, 0, count, false, false);
            if (! unified.has_value())
                return std::nullopt;

            return makeType (vectorKind (TypeKind::boolType, componentCount (unified->kind)));
        }

        if (isOneOf (name, { "isnan", "isinf" }))
            return withComponents (types[0], TypeKind::boolType);

        if (isOneOf (name, { "any", "all" }))
            return makeType (TypeKind::boolType);

        if (name == "not")
            return types[0];

        if (name == "determinant")
            return makeType (TypeKind::floatType);

        if (isOneOf (name, { "inverse", "matrixCompMult" }))
            return types[0];

        if (name == "transpose")
        {
            if (! types[0].has_value())
                return std::nullopt;

            const auto [columns, rows] = matrixShape (types[0]->kind);
            return makeType (matrixKind (rows, columns));
        }

        if (name == "outerProduct" && count == 2)
        {
            if (! isNumeric (types[0]) || ! isNumeric (types[1]))
                return std::nullopt;

            return makeType (matrixKind (componentCount (types[1]->kind), componentCount (types[0]->kind)));
        }

        if (isOneOf (name, { "bitCount", "findLSB", "findMSB" }))
            return withComponents (types[0], TypeKind::intType);

        if (isOneOf (name, { "bitfieldReverse", "bitfieldExtract", "bitfieldInsert", "uaddCarry", "usubBorrow" }))
            return types[0];

        if (isOneOf (name, { "umulExtended", "imulExtended" }))
            return std::nullopt;

        if (name == "floatBitsToInt")
            return withComponents (types[0], TypeKind::intType);

        if (name == "floatBitsToUint")
            return withComponents (types[0], TypeKind::uintType);

        if (isOneOf (name, { "intBitsToFloat", "uintBitsToFloat" }))
            return withComponents (types[0], TypeKind::floatType);

        if (isOneOf (name, { "packUnorm2x16", "packSnorm2x16", "packUnorm4x8", "packSnorm4x8", "packHalf2x16" }))
            return makeType (TypeKind::uintType);

        if (isOneOf (name, { "unpackUnorm2x16", "unpackSnorm2x16", "unpackHalf2x16" }))
            return makeType (TypeKind::vec2);

        if (isOneOf (name, { "unpackUnorm4x8", "unpackSnorm4x8" }))
            return makeType (TypeKind::vec4);

        if (isOneOf (name, { "frexp", "modf", "ldexp" }))
            return types[0];

        if (isOneOf (name, { "atomicAdd", "atomicMin", "atomicMax", "atomicAnd", "atomicOr", "atomicXor", "atomicExchange", "atomicCompSwap" }))
            return types[0];

        if (name == "atan" && count == 2)
            return unifyArguments (call, types, 0, 2, true, false);

        if (isOneOf (name, { "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
                             "exp", "log", "exp2", "log2", "sqrt", "inversesqrt", "radians", "degrees", "normalize",
                             "floor", "ceil", "fract", "trunc", "round", "roundEven", "pow", "mod", "reflect",
                             "faceforward", "dFdx", "dFdy", "fwidth", "dFdxFine", "dFdyFine", "fwidthFine",
                             "dFdxCoarse", "dFdyCoarse", "fwidthCoarse" }))
            return unifyArguments (call, types, 0, count, true, false);

        return std::nullopt;
    }

    //==========================================================================
    std::vector<std::map<std::string, TypeSpecifier>> scopes;
    std::map<std::string, const StructSpecifier*> structs;
    std::multimap<std::string, const FunctionPrototype*> functions;
    TypeSpecifier returnType;
};

} // namespace

//==============================================================================
void WgslTypeLegalizer::legalize (TranslationUnit& ast)
{
    TypeLegalizer().run (ast);
}

} // namespace wgsl
} // namespace yup
