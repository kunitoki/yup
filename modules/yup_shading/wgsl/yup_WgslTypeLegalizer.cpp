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
        symbols.pushScope();

        declareBuiltins();
        collectStructsAndFunctions (ast);

        for (auto& decl : ast.declarations)
        {
            if (auto* declaration = std::get_if<Declaration> (&decl))
                visitDeclaration (*declaration);
            else if (auto* function = std::get_if<FunctionDefinition> (&decl))
                visitFunction (*function);
        }

        symbols.popScope();
    }

private:
    using ExprType = std::optional<TypeSpecifier>;

    //==========================================================================
    static TypeKind scalarKindOf (TypeKind kind)
    {
        switch (kind)
        {
            case TypeKind::floatType:
            case TypeKind::vec2:
            case TypeKind::vec3:
            case TypeKind::vec4:
            case TypeKind::mat2:
            case TypeKind::mat3:
            case TypeKind::mat4:
            case TypeKind::mat2x2:
            case TypeKind::mat2x3:
            case TypeKind::mat2x4:
            case TypeKind::mat3x2:
            case TypeKind::mat3x3:
            case TypeKind::mat3x4:
            case TypeKind::mat4x2:
            case TypeKind::mat4x3:
            case TypeKind::mat4x4:
                return TypeKind::floatType;

            case TypeKind::intType:
            case TypeKind::ivec2:
            case TypeKind::ivec3:
            case TypeKind::ivec4:
                return TypeKind::intType;

            case TypeKind::uintType:
            case TypeKind::uvec2:
            case TypeKind::uvec3:
            case TypeKind::uvec4:
                return TypeKind::uintType;

            case TypeKind::boolType:
            case TypeKind::bvec2:
            case TypeKind::bvec3:
            case TypeKind::bvec4:
                return TypeKind::boolType;

            default:
                return TypeKind::voidType;
        }
    }

    /** Returns 1 for scalars, 2 to 4 for vectors and 0 for anything else. */
    static int componentCount (TypeKind kind)
    {
        switch (kind)
        {
            case TypeKind::floatType:
            case TypeKind::intType:
            case TypeKind::uintType:
            case TypeKind::boolType:
                return 1;

            case TypeKind::vec2:
            case TypeKind::ivec2:
            case TypeKind::uvec2:
            case TypeKind::bvec2:
                return 2;

            case TypeKind::vec3:
            case TypeKind::ivec3:
            case TypeKind::uvec3:
            case TypeKind::bvec3:
                return 3;

            case TypeKind::vec4:
            case TypeKind::ivec4:
            case TypeKind::uvec4:
            case TypeKind::bvec4:
                return 4;

            default:
                return 0;
        }
    }

    static TypeKind vectorKind (TypeKind scalar, int count)
    {
        static constexpr TypeKind floats[] = { TypeKind::floatType, TypeKind::vec2, TypeKind::vec3, TypeKind::vec4 };
        static constexpr TypeKind ints[] = { TypeKind::intType, TypeKind::ivec2, TypeKind::ivec3, TypeKind::ivec4 };
        static constexpr TypeKind uints[] = { TypeKind::uintType, TypeKind::uvec2, TypeKind::uvec3, TypeKind::uvec4 };
        static constexpr TypeKind bools[] = { TypeKind::boolType, TypeKind::bvec2, TypeKind::bvec3, TypeKind::bvec4 };

        if (count < 1 || count > 4)
            return TypeKind::voidType;

        switch (scalar)
        {
            case TypeKind::floatType:
                return floats[count - 1];
            case TypeKind::intType:
                return ints[count - 1];
            case TypeKind::uintType:
                return uints[count - 1];
            case TypeKind::boolType:
                return bools[count - 1];
            default:
                return TypeKind::voidType;
        }
    }

    /** Returns { columns, rows } of a float matrix (GLSL matCxR), or { 0, 0 }. */
    static std::pair<int, int> matrixShape (TypeKind kind)
    {
        switch (kind)
        {
            case TypeKind::mat2:
            case TypeKind::mat2x2:
                return { 2, 2 };
            case TypeKind::mat2x3:
                return { 2, 3 };
            case TypeKind::mat2x4:
                return { 2, 4 };
            case TypeKind::mat3x2:
                return { 3, 2 };
            case TypeKind::mat3:
            case TypeKind::mat3x3:
                return { 3, 3 };
            case TypeKind::mat3x4:
                return { 3, 4 };
            case TypeKind::mat4x2:
                return { 4, 2 };
            case TypeKind::mat4x3:
                return { 4, 3 };
            case TypeKind::mat4:
            case TypeKind::mat4x4:
                return { 4, 4 };
            default:
                return { 0, 0 };
        }
    }

    static TypeKind matrixKind (int columns, int rows)
    {
        static constexpr TypeKind kinds[3][3] = {
            { TypeKind::mat2x2, TypeKind::mat2x3, TypeKind::mat2x4 },
            { TypeKind::mat3x2, TypeKind::mat3x3, TypeKind::mat3x4 },
            { TypeKind::mat4x2, TypeKind::mat4x3, TypeKind::mat4x4 }
        };

        if (columns < 2 || columns > 4 || rows < 2 || rows > 4)
            return TypeKind::voidType;

        return kinds[columns - 2][rows - 2];
    }

    /** GLSL converts int to uint, and both to float. Bool never converts implicitly. */
    static TypeKind promote (TypeKind a, TypeKind b)
    {
        const auto rank = [] (TypeKind scalar)
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
        };

        if (rank (a) == 0 || rank (b) == 0)
            return TypeKind::voidType;

        return rank (a) >= rank (b) ? a : b;
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

    static void wrap (Expr& expr, TypeKind kind)
    {
        ExprTypeConstructor constructor;
        constructor.loc = expr.loc;
        constructor.type = TypeSpecifier::make (expr.loc, kind);
        constructor.args.push_back (std::move (expr));

        Expr wrapped;
        wrapped.loc = constructor.loc;
        wrapped.value = std::move (constructor);
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

        if (fromScalar != toScalar && ! adaptsTo (expr, toScalar))
            wrap (expr, vectorKind (toScalar, fromCount));

        if (splat)
            wrap (expr, to);
    }

    //==========================================================================
    void declare (const std::string& name, TypeSpecifier type)
    {
        SymbolInfo info;
        info.type = std::move (type);
        symbols.declare (name, info);
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
            { "gl_GlobalInvocationID", TypeKind::uvec3 },
            { "gl_LocalInvocationID", TypeKind::uvec3 },
            { "gl_WorkGroupID", TypeKind::uvec3 },
            { "gl_NumWorkGroups", TypeKind::uvec3 },
            { "gl_LocalInvocationIndex", TypeKind::uintType }
        };

        for (const auto& [name, kind] : builtins)
            declare (name, TypeSpecifier::make ({}, kind));
    }

    void collectStructsAndFunctions (const TranslationUnit& ast)
    {
        for (const auto& decl : ast.declarations)
        {
            if (const auto* function = std::get_if<FunctionDefinition> (&decl))
            {
                functions.emplace (function->prototype.name, &function->prototype);
                continue;
            }

            const auto* declaration = std::get_if<Declaration> (&decl);
            if (declaration == nullptr || declaration->structSpecifier == nullptr)
                continue;

            const auto& structSpecifier = *declaration->structSpecifier;
            if (! structSpecifier.name.empty())
                structs[structSpecifier.name] = &structSpecifier;

            // The fields of a block without an instance name are globals
            const bool isBlock = declaration->qualifier != nullptr
                              && (declaration->qualifier->hasStorage (StorageQualifier::uniform)
                                  || declaration->qualifier->hasStorage (StorageQualifier::buffer));

            if (isBlock && declaration->initDeclaratorList == nullptr)
            {
                for (const auto& field : structSpecifier.fields)
                    declare (field.name, field.type);
            }
        }
    }

    const FunctionPrototype* findFunction (const std::string& name, std::size_t argumentCount) const
    {
        const FunctionPrototype* result = nullptr;

        const auto [first, last] = functions.equal_range (name);
        for (auto it = first; it != last; ++it)
        {
            if (it->second->parameters.size() != argumentCount)
                continue;

            // Same-arity overloads can't be told apart without full overload resolution
            if (result != nullptr)
                return nullptr;

            result = it->second;
        }

        return result;
    }

    //==========================================================================
    void visitDeclaration (Declaration& declaration)
    {
        if (declaration.initDeclaratorList == nullptr)
            return;

        auto& list = *declaration.initDeclaratorList;
        for (auto& single : list.declarations)
        {
            auto type = list.type;
            type.arraySpecifiers.insert (type.arraySpecifiers.end(),
                                         single.arraySpecifiers.begin(),
                                         single.arraySpecifiers.end());

            if (single.initializer != nullptr && single.initializer->expr != nullptr)
            {
                const auto initializerType = visit (*single.initializer->expr);

                if (type.arraySpecifiers.empty())
                    coerce (*single.initializer->expr, initializerType, type.kind);
            }

            declare (single.name, std::move (type));
        }
    }

    void visitFunction (FunctionDefinition& function)
    {
        symbols.pushScope();

        for (const auto& parameter : function.prototype.parameters)
        {
            auto type = parameter.type;
            type.arraySpecifiers.insert (type.arraySpecifiers.end(),
                                         parameter.arraySpecifiers.begin(),
                                         parameter.arraySpecifiers.end());
            declare (parameter.name, std::move (type));
        }

        returnType = function.prototype.returnType;

        if (function.body != nullptr)
            visitStatement (*function.body);

        symbols.popScope();
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
            symbols.pushScope();

            for (auto& child : statement.as<StmtCompound>().statements)
                visitStatement (child);

            symbols.popScope();
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
            visitIfPresent (switchStatement.selector);

            symbols.pushScope();

            for (auto& child : switchStatement.body)
                visitStatement (child);

            symbols.popScope();
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

            symbols.pushScope();

            visitIfPresent (loop.init);
            visitIfPresent (loop.condition);
            visitIfPresent (loop.update);
            visitIfPresent (loop.body);

            symbols.popScope();
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
        if (expr.is<ExprVariable>())
        {
            if (const auto* info = symbols.lookup (expr.as<ExprVariable>().name))
                return info->type;

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

            auto operandType = visit (*unary.operand);
            if (unary.op == UnaryOp::logicalNot)
                return makeType (TypeKind::boolType);

            return operandType;
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
                return makeType (TypeKind::boolType);

            case BinaryOp::shiftLeft:
            case BinaryOp::shiftRight:
                if (isNumeric (rightType))
                    coerce (*binary.right, rightType, vectorKind (TypeKind::uintType, componentCount (rightType->kind)));

                return leftType;

            case BinaryOp::lessThan:
            case BinaryOp::greaterThan:
            case BinaryOp::lessEqual:
            case BinaryOp::greaterEqual:
            case BinaryOp::equal:
            case BinaryOp::notEqual:
                unify (*binary.left, leftType, *binary.right, rightType);
                return makeType (TypeKind::boolType);

            default:
                break;
        }

        if (! leftType.has_value() || ! rightType.has_value())
            return std::nullopt;

        const bool isMatrixOperation = matrixShape (leftType->kind).first > 0
                                    || matrixShape (rightType->kind).first > 0;

        if (! isMatrixOperation)
            return unify (*binary.left, leftType, *binary.right, rightType);

        if (binary.op == BinaryOp::mul)
            return matrixProduct (*leftType, *rightType);

        return matrixShape (leftType->kind).first > 0 ? leftType : rightType;
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

        return trueType;
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
                coerce (*assignment.rhs, valueType, vectorKind (TypeKind::uintType, valueCount));
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
            for (std::size_t i = 0; i < constructor.args.size(); ++i)
                coerce (constructor.args[i], argumentTypes[i], type.kind);

            return type;
        }

        // Samplers and structs keep their arguments untouched
        if (scalar == TypeKind::voidType)
            return typeOf (type);

        if (matrixShape (type.kind).first > 0)
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

        if (call.callee == nullptr || ! call.callee->is<ExprVariable>())
            return std::nullopt;

        const auto& name = call.callee->as<ExprVariable>().name;

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
            const auto* prototype = findFunction (name, call.args.size());
            if (prototype == nullptr)
                return std::nullopt;

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

        switch (samplerType->kind)
        {
            case TypeKind::sampler1DShadow:
            case TypeKind::sampler2DShadow:
            case TypeKind::samplerCubeShadow:
            case TypeKind::sampler1DArrayShadow:
            case TypeKind::sampler2DArrayShadow:
            case TypeKind::sampler2DRectShadow:
                return makeType (TypeKind::floatType);

            case TypeKind::sampler1D:
            case TypeKind::sampler2D:
            case TypeKind::sampler3D:
            case TypeKind::samplerCube:
            case TypeKind::sampler1DArray:
            case TypeKind::sampler2DArray:
            case TypeKind::sampler2DRect:
            case TypeKind::sampler2DMS:
            case TypeKind::sampler2DMSArray:
                return makeType (TypeKind::vec4);

            case TypeKind::isampler1D:
            case TypeKind::isampler2D:
            case TypeKind::isampler3D:
            case TypeKind::isamplerCube:
            case TypeKind::isampler1DArray:
            case TypeKind::isampler2DArray:
            case TypeKind::isampler2DRect:
            case TypeKind::isampler2DMS:
            case TypeKind::isampler2DMSArray:
                return makeType (TypeKind::ivec4);

            case TypeKind::usampler1D:
            case TypeKind::usampler2D:
            case TypeKind::usampler3D:
            case TypeKind::usamplerCube:
            case TypeKind::usampler1DArray:
            case TypeKind::usampler2DArray:
            case TypeKind::usampler2DRect:
            case TypeKind::usampler2DMS:
            case TypeKind::usampler2DMSArray:
                return makeType (TypeKind::uvec4);

            default:
                return std::nullopt;
        }
    }

    static ExprType visitBuiltinCall (const std::string& name, ExprFunCall& call, const std::vector<ExprType>& types)
    {
        const auto count = call.args.size();
        if (count == 0)
            return std::nullopt;

        if (isOneOf (name, { "texture", "textureLod", "textureGrad", "textureOffset", "textureLodOffset", "textureProj", "texelFetch" }))
            return textureResult (types[0]);

        if (isOneOf (name, { "min", "max", "clamp" }))
            return unifyArguments (call, types, 0, count, false, true);

        if (isOneOf (name, { "abs", "sign" }))
            return unifyArguments (call, types, 0, count, false, false);

        if (isOneOf (name, { "step", "smoothstep" }))
            return unifyArguments (call, types, 0, count, true, true);

        if (name == "mix" && count == 3)
        {
            auto result = unifyArguments (call, types, 0, 2, true, false);

            if (isNumeric (types[2]))
                coerce (call.args[2], types[2], vectorKind (TypeKind::floatType, componentCount (types[2]->kind)));

            return result;
        }

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
        {
            if (! isNumeric (types[0]))
                return std::nullopt;

            return makeType (vectorKind (TypeKind::boolType, componentCount (types[0]->kind)));
        }

        if (isOneOf (name, { "any", "all" }))
            return makeType (TypeKind::boolType);

        if (name == "not")
            return types[0];

        if (name == "determinant")
            return makeType (TypeKind::floatType);

        if (name == "inverse")
            return types[0];

        if (name == "transpose")
        {
            if (! types[0].has_value())
                return std::nullopt;

            const auto [columns, rows] = matrixShape (types[0]->kind);
            return makeType (matrixKind (rows, columns));
        }

        if (isOneOf (name, { "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh",
                             "exp", "log", "exp2", "log2", "sqrt", "inversesqrt", "radians", "degrees", "normalize",
                             "floor", "ceil", "fract", "trunc", "round", "roundEven", "pow", "mod", "reflect",
                             "faceforward", "dFdx", "dFdy", "fwidth" }))
            return unifyArguments (call, types, 0, count, true, false);

        return std::nullopt;
    }

    //==========================================================================
    SymbolTable symbols;
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
