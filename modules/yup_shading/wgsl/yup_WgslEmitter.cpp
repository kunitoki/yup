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
/** Shortest decimal text that reads back as the same 32-bit float, always marked as a float literal. */
std::string formatFloat (double value, const SourceLocation& loc)
{
    const auto f = static_cast<float> (value);

    if (! std::isfinite (value) || ! std::isfinite (f))
        throw LoweringError (loc, "Floating point literal is out of range for a 32-bit float");

    std::string text;

    for (int precision = 1; precision <= std::numeric_limits<float>::max_digits10; ++precision)
    {
        std::ostringstream out;
        out.imbue (std::locale::classic());
        out << std::setprecision (precision) << f;
        text = out.str();

        std::istringstream in (text);
        in.imbue (std::locale::classic());

        float parsed = 0.0f;
        in >> parsed;

        if (! in.fail() && parsed == f)
            break;
    }

    // Plain decimals read better than exponents for everyday magnitudes
    const auto magnitude = std::fabs (f);
    if (text.find ('e') != std::string::npos && magnitude >= 1.0e-4f && magnitude < 1.0e7f)
    {
        for (int decimals = 1; decimals <= 12; ++decimals)
        {
            std::ostringstream out;
            out.imbue (std::locale::classic());
            out << std::fixed << std::setprecision (decimals) << f;

            std::istringstream in (out.str());
            in.imbue (std::locale::classic());

            float parsed = 0.0f;
            in >> parsed;

            if (! in.fail() && parsed == f)
            {
                text = out.str();
                break;
            }
        }
    }

    if (text.find_first_of (".eEn") == std::string::npos)
        text += ".0";

    return text;
}

//==============================================================================
class Emitter
{
public:
    explicit Emitter (const LoweredProgram& prog)
        : program (prog)
        , entry (prog.entryPoint)
    {
        for (const auto& r : program.resources)
            resources[r.name] = &r;
    }

    std::string emit()
    {
        std::string out;

        for (const auto& enable : program.enables)
            out += "enable " + enable + ";\n";

        // GLSL allows implicit-derivative sampling in non-uniform control flow, WGSL rejects it by default
        if (entry.isFragment)
            out += "diagnostic(off, derivative_uniformity);\n";

        if (! out.empty())
            out += "\n";

        for (const auto& polyfill : program.polyfills)
            out += polyfill + "\n";

        emitStructs (out);
        emitGlobals (out);
        emitFunctions (out);
        emitEntryPoint (out);

        return out;
    }

private:
    //==========================================================================
    // Types
    //==========================================================================

    std::string typeName (const TypeSpecifier& ts, const SourceLocation& loc)
    {
        std::string s = ts.kind == TypeKind::namedStruct ? ts.structName : wgslTypeName (ts.kind);

        if (s.empty())
            throw LoweringError (loc, "Type '" + glslTypeName (ts.kind) + "' has no WGSL equivalent");

        // Array specifiers are outermost first
        for (auto it = ts.arraySpecifiers.rbegin(); it != ts.arraySpecifiers.rend(); ++it)
        {
            if (it->isUnsized || it->sizeExpr == nullptr)
            {
                s = "array<" + s + ">";
            }
            else
            {
                std::string size;
                emitExpr (*it->sizeExpr, size);
                s = "array<" + s + ", " + size + ">";
            }
        }

        return s;
    }

    //==========================================================================
    // Module scope
    //==========================================================================

    static std::string storageTextureType (TypeKind kind, const TypeQualifier* q, const SourceLocation& loc)
    {
        static const std::map<std::string, std::string> formats = {
            { "rgba32f", "rgba32float" }, { "rgba16f", "rgba16float" }, { "rg32f", "rg32float" }, { "r32f", "r32float" },
            { "rgba8", "rgba8unorm" }, { "rgba8_snorm", "rgba8snorm" },
            { "rgba32i", "rgba32sint" }, { "rgba16i", "rgba16sint" }, { "rgba8i", "rgba8sint" }, { "rg32i", "rg32sint" }, { "r32i", "r32sint" },
            { "rgba32ui", "rgba32uint" }, { "rgba16ui", "rgba16uint" }, { "rgba8ui", "rgba8uint" }, { "rg32ui", "rg32uint" }, { "r32ui", "r32uint" }
        };

        std::string format;
        if (q != nullptr && q->layout != nullptr)
            for (const auto& entry : q->layout->entries)
                if (entry.id == LayoutQualifierId::imageFormat)
                    format = entry.name;

        const auto found = formats.find (format);
        if (found == formats.end())
            throw LoweringError (loc, "Image format '" + format + "' is not a WGSL storage texture format");

        const auto shape = textureShape (kind);
        const std::string dim = shape.dim == TextureShape::Dim::d1 ? "1d" : shape.dim == TextureShape::Dim::d3 ? "3d" : shape.arrayed ? "2d_array" : "2d";
        const std::string access = q->hasMemory (MemoryQualifier::readonlyQual) ? "read" : q->hasMemory (MemoryQualifier::writeonlyQual) ? "write" : "read_write";

        return "texture_storage_" + dim + "<" + found->second + ", " + access + ">";
    }

    void emitStructs (std::string& out)
    {
        for (const auto& external : program.ast.declarations)
        {
            const auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->structSpecifier == nullptr)
                continue;

            const auto& ss = *d->structSpecifier;

            out += "struct " + ss.name + " {\n";
            for (const auto& field : ss.fields)
            {
                out += "    ";

                if (field.sizeAttribute != 0)
                    out += "@size(" + std::to_string (field.sizeAttribute) + ") ";

                out += field.name + ": " + typeName (field.type, field.loc) + ",\n";
            }
            out += "}\n\n";
        }
    }

    std::string bindingAttributes (const std::string& name)
    {
        auto found = resources.find (name);
        if (found == resources.end())
            return "";

        return "@group(" + std::to_string (found->second->group) + ") @binding(" + std::to_string (found->second->binding) + ") ";
    }

    void emitGlobals (std::string& out)
    {
        bool emitted = false;

        for (const auto& external : program.ast.declarations)
        {
            const auto* d = std::get_if<Declaration> (&external);
            if (d == nullptr || d->initDeclaratorList == nullptr)
                continue;

            const auto& list = *d->initDeclaratorList;
            const auto* q = list.qualifier.get();

            for (const auto& single : list.declarations)
            {
                const auto type = declaratorType (list.type, single.arraySpecifiers);
                const auto binding = bindingAttributes (single.name);
                emitted = true;

                if (q != nullptr && q->hasStorage (StorageQualifier::constQual))
                {
                    if (single.initializer == nullptr || single.initializer->expr == nullptr)
                        throw LoweringError (single.loc, "Constant '" + single.name + "' needs an initializer");

                    // Specialization constants are pipeline-overridable in WGSL
                    std::string keyword = "const ";
                    if (q->layout != nullptr)
                        for (const auto& entry : q->layout->entries)
                            if (entry.id == LayoutQualifierId::constantId && entry.value != nullptr)
                                keyword = "@id(" + std::to_string (evaluateIntConstant (*entry.value).value_or (0)) + ") override ";

                    out += keyword + single.name + ": " + typeName (type, single.loc) + " = ";
                    emitExpr (*single.initializer->expr, out);
                    out += ";\n";
                }
                else if (isSamplerType (type.kind))
                {
                    // A combined image sampler splits into a texture and its companion sampler
                    const auto* ra = resources.at (single.name);
                    out += binding + "var " + single.name + ": " + typeName (type, single.loc) + ";\n";
                    out += "@group(" + std::to_string (ra->group) + ") @binding(" + std::to_string (ra->samplerBinding) + ") var "
                         + ra->samplerName + ": " + (textureShape (type.kind).shadow ? "sampler_comparison" : "sampler") + ";\n";
                }
                else if (isImageType (type.kind))
                {
                    out += binding + "var " + single.name + ": " + storageTextureType (type.kind, q, single.loc) + ";\n";
                }
                else if (isSeparateTextureType (type.kind) && program.depthTextures.count (single.name) > 0)
                {
                    const auto depthType = wgslTextureTypeName (type.kind, true);
                    if (depthType.empty())
                        throw LoweringError (single.loc, "'" + glslTypeName (type.kind) + "' has no WGSL depth texture equivalent");

                    out += binding + "var " + single.name + ": " + depthType + ";\n";
                }
                else if (isOpaqueType (type.kind))
                {
                    out += binding + "var " + single.name + ": " + typeName (type, single.loc) + ";\n";
                }
                else if (q != nullptr && q->hasStorage (StorageQualifier::uniform))
                {
                    out += binding + "var<uniform> " + single.name + ": " + typeName (type, single.loc) + ";\n";
                }
                else if (q != nullptr && q->hasStorage (StorageQualifier::buffer))
                {
                    const auto access = q->hasMemory (MemoryQualifier::readonlyQual) ? "read" : "read_write";
                    out += binding + "var<storage, " + access + "> " + single.name + ": " + typeName (type, single.loc) + ";\n";
                }
                else if (q != nullptr && q->hasStorage (StorageQualifier::shared))
                {
                    out += "var<workgroup> " + single.name + ": " + typeName (type, single.loc) + ";\n";
                }
                else
                {
                    out += "var<private> " + single.name + ": " + typeName (type, single.loc) + ";\n";
                }
            }
        }

        if (emitted)
            out += "\n";
    }

    //==========================================================================
    // Functions
    //==========================================================================

    void emitFunctions (std::string& out)
    {
        for (const auto& external : program.ast.declarations)
        {
            const auto* fd = std::get_if<FunctionDefinition> (&external);
            if (fd == nullptr)
                continue;

            out += "fn " + fd->prototype.name + "(";

            bool first = true;
            for (const auto& param : fd->prototype.parameters)
            {
                if (! first)
                    out += ", ";
                first = false;

                const auto type = typeName (declaratorType (param.type, param.arraySpecifiers), param.loc);
                const bool isReference = param.qualifier != nullptr
                                      && (param.qualifier->hasStorage (StorageQualifier::out) || param.qualifier->hasStorage (StorageQualifier::inout));

                out += param.name + ": " + (isReference ? "ptr<function, " + type + ">" : type);
            }

            out += ")";

            if (fd->prototype.returnType.kind != TypeKind::voidType)
                out += " -> " + typeName (fd->prototype.returnType, fd->prototype.loc);

            out += " ";

            if (fd->body != nullptr)
                emitStatement (*fd->body, out, 0);
            else
                out += "{}\n";

            out += "\n";
        }
    }

    //==========================================================================
    // Entry point
    //==========================================================================

    std::string emitIOStruct (const std::vector<LoweredProgram::InputOutputInfo>& ios, const std::string& structName)
    {
        std::string s = "struct " + structName + " {\n";

        for (const auto& io : ios)
        {
            s += "    ";

            if (io.isBuiltin)
                s += "@builtin(" + io.builtinName + ") ";
            else
                s += "@location(" + std::to_string (io.location) + ") ";

            if (io.blendSource >= 0)
                s += "@blend_src(" + std::to_string (io.blendSource) + ") ";

            if (! io.interpolation.empty())
                s += "@interpolate(" + io.interpolation + ") ";

            if (io.invariant)
                s += "@invariant ";

            s += io.fieldName + ": " + typeName (io.wgslType, {}) + ",\n";
        }

        s += "}\n\n";
        return s;
    }

    void emitEntryPoint (std::string& out)
    {
        const bool hasInputs = ! entry.inputs.empty();
        const bool hasOutputs = ! entry.outputs.empty() && ! entry.isCompute;

        if (hasInputs)
            out += emitIOStruct (entry.inputs, entry.inputStruct);

        if (hasOutputs)
            out += emitIOStruct (entry.outputs, entry.outputStruct);

        if (entry.isCompute)
        {
            const uint32_t sizes[] = { entry.workgroupSizeX, entry.workgroupSizeY, entry.workgroupSizeZ };
            out += "@compute @workgroup_size(";

            for (size_t i = 0; i < 3; ++i)
                out += (i > 0 ? ", " : "") + (entry.workgroupSizeOverrides[i].empty() ? std::to_string (sizes[i]) : entry.workgroupSizeOverrides[i]);

            out += ")\n";
        }
        else
        {
            out += entry.isVertex ? "@vertex\n" : "@fragment\n";
        }

        out += "fn " + entry.wgslEntryPoint + "(";

        if (hasInputs)
            out += entry.inputParameter + ": " + entry.inputStruct;

        out += ")";

        if (hasOutputs)
            out += " -> " + entry.outputStruct;

        out += " {\n";

        for (const auto& copy : entry.inputCopies)
            out += "    " + copy + ";\n";

        out += "    " + entry.innerFunction + "();\n";

        if (hasOutputs)
        {
            out += "    var " + entry.outputVariable + ": " + entry.outputStruct + ";\n";

            for (const auto& copy : entry.outputCopies)
                out += "    " + copy + ";\n";

            out += "    return " + entry.outputVariable + ";\n";
        }

        out += "}\n";
    }

    //==========================================================================
    // Statements
    //==========================================================================

    void emitBody (const Statement& stmt, std::string& out, int indent)
    {
        if (stmt.is<StmtCompound>())
        {
            for (const auto& s : stmt.as<StmtCompound>().statements)
                emitStatement (s, out, indent);
        }
        else
        {
            emitStatement (stmt, out, indent);
        }
    }

    void emitStatements (const std::vector<Statement>& statements, std::string& out, int indent)
    {
        for (const auto& s : statements)
            emitStatement (s, out, indent);
    }

    void emitStatement (const Statement& stmt, std::string& out, int indent)
    {
        const std::string ind (static_cast<size_t> (indent) * 4, ' ');

        if (stmt.is<StmtCompound>())
        {
            out += ind + "{\n";
            emitStatements (stmt.as<StmtCompound>().statements, out, indent + 1);
            out += ind + "}\n";
        }
        else if (stmt.is<StmtSelection>())
        {
            const auto& sel = stmt.as<StmtSelection>();
            out += ind + "if (";
            emitExpr (*sel.condition, out);
            out += ") {\n";
            emitBody (*sel.thenBranch, out, indent + 1);
            out += ind + "}\n";

            if (sel.elseBranch != nullptr)
            {
                out += ind + "else ";

                if (sel.elseBranch->is<StmtSelection>())
                {
                    std::string nested;
                    emitStatement (*sel.elseBranch, nested, indent);
                    out += nested.substr (ind.size());
                }
                else
                {
                    out += "{\n";
                    emitBody (*sel.elseBranch, out, indent + 1);
                    out += ind + "}\n";
                }
            }
        }
        else if (stmt.is<StmtSwitch>())
        {
            emitSwitch (stmt.as<StmtSwitch>(), out, indent);
        }
        else if (stmt.is<StmtCaseLabel>())
        {
            throw LoweringError (stmt.loc, "Case label outside of a lowered switch");
        }
        else if (stmt.is<StmtWhile>())
        {
            const auto& w = stmt.as<StmtWhile>();
            out += ind + "while (";
            emitExpr (*w.condition, out);
            out += ") {\n";
            emitBody (*w.body, out, indent + 1);
            out += ind + "}\n";
        }
        else if (stmt.is<StmtDoWhile>())
        {
            // do body while (cond) -> loop { body continuing { break if !(cond); } }
            const auto& dw = stmt.as<StmtDoWhile>();
            out += ind + "loop {\n";
            emitBody (*dw.body, out, indent + 1);
            out += ind + "    continuing {\n";
            out += ind + "        break if !(";
            emitExpr (*dw.condition, out);
            out += ");\n";
            out += ind + "    }\n";
            out += ind + "}\n";
        }
        else if (stmt.is<StmtLoop>())
        {
            const auto& loop = stmt.as<StmtLoop>();
            out += ind + "loop {\n";
            emitStatements (loop.body, out, indent + 1);

            if (! loop.continuing.empty() || loop.breakIf != nullptr)
            {
                out += ind + "    continuing {\n";
                emitStatements (loop.continuing, out, indent + 2);

                if (loop.breakIf != nullptr)
                {
                    out += ind + "        break if ";
                    emitExpr (*loop.breakIf, out);
                    out += ";\n";
                }

                out += ind + "    }\n";
            }

            out += ind + "}\n";
        }
        else if (stmt.is<StmtFor>())
        {
            const auto& f = stmt.as<StmtFor>();
            out += ind + "for (";

            if (f.init != nullptr)
                emitSimpleStatement (*f.init, out);

            out += "; ";

            if (f.condition != nullptr)
                emitExpr (*f.condition, out);

            out += "; ";

            if (f.update != nullptr)
            {
                const auto* update = f.update.get();
                while (update->is<ExprParen>() && update->as<ExprParen>().expr != nullptr)
                    update = update->as<ExprParen>().expr.get();

                emitExpr (*update, out);
            }

            out += ") {\n";
            emitBody (*f.body, out, indent + 1);
            out += ind + "}\n";
        }
        else if (stmt.is<StmtJump>())
        {
            const auto& j = stmt.as<StmtJump>();
            switch (j.kind)
            {
                case JumpKind::returnJump:
                    out += ind + "return";
                    if (j.returnValue != nullptr)
                    {
                        out += " ";
                        emitExpr (*j.returnValue, out);
                    }
                    out += ";\n";
                    break;
                case JumpKind::breakJump:
                    out += ind + "break;\n";
                    break;
                case JumpKind::continueJump:
                    out += ind + "continue;\n";
                    break;
                case JumpKind::discardJump:
                    out += ind + "discard;\n";
                    break;
            }
        }
        else if (stmt.is<StmtExpr>())
        {
            out += ind;
            emitExpr (*stmt.as<StmtExpr>().expr, out);
            out += ";\n";
        }
        else if (stmt.is<StmtDeclaration>())
        {
            const auto& decl = stmt.as<StmtDeclaration>().declaration;
            if (decl.initDeclaratorList == nullptr)
                return;

            for (const auto& single : decl.initDeclaratorList->declarations)
            {
                out += ind;
                emitLocalDeclaration (*decl.initDeclaratorList, single, out);
                out += ";\n";
            }
        }
    }

    void emitSwitch (const StmtSwitch& sw, std::string& out, int indent)
    {
        const std::string ind (static_cast<size_t> (indent) * 4, ' ');

        out += ind + "switch (";
        emitExpr (*sw.selector, out);
        out += ") {\n";

        for (const auto& clause : sw.clauses)
        {
            const bool onlyDefault = clause.labels.size() == 1 && clause.labels.front() == nullptr;
            out += ind + (onlyDefault ? "    default" : "    case ");

            for (size_t i = 0; i < clause.labels.size() && ! onlyDefault; ++i)
            {
                if (i > 0)
                    out += ", ";

                if (clause.labels[i] == nullptr)
                    out += "default";
                else
                    emitExpr (*clause.labels[i], out);
            }

            out += ": {\n";
            emitStatements (clause.body, out, indent + 2);
            out += ind + "    }\n";
        }

        out += ind + "}\n";
    }

    void emitLocalDeclaration (const InitDeclaratorList& list, const SingleDeclaration& single, std::string& out)
    {
        const bool isConst = list.qualifier != nullptr && list.qualifier->hasStorage (StorageQualifier::constQual);
        const auto type = declaratorType (list.type, single.arraySpecifiers);

        out += (! isConst ? "var " : list.isLet ? "let " : "const ") + single.name;

        // void marks a temporary whose type WGSL can't spell, like the result of frexp
        if (type.kind != TypeKind::voidType || ! type.arraySpecifiers.empty())
            out += ": " + typeName (type, single.loc);

        if (single.initializer != nullptr && single.initializer->expr != nullptr)
        {
            out += " = ";
            emitExpr (*single.initializer->expr, out);
        }
        else if (isConst)
        {
            throw LoweringError (single.loc, "Constant '" + single.name + "' needs an initializer");
        }
    }

    /** Statement forms allowed in a for loop header. */
    void emitSimpleStatement (const Statement& stmt, std::string& out)
    {
        if (stmt.is<StmtDeclaration>())
        {
            const auto& decl = stmt.as<StmtDeclaration>().declaration;

            if (decl.initDeclaratorList == nullptr || decl.initDeclaratorList->declarations.size() != 1)
                throw LoweringError (stmt.loc, "A for loop header can declare only one variable in WGSL");

            emitLocalDeclaration (*decl.initDeclaratorList, decl.initDeclaratorList->declarations.front(), out);
        }
        else if (stmt.is<StmtExpr>() && stmt.as<StmtExpr>().expr != nullptr)
        {
            emitExpr (*stmt.as<StmtExpr>().expr, out);
        }
    }

    //==========================================================================
    // Expressions
    //==========================================================================

    static bool needsParensAsUnaryOperand (const Expr& e)
    {
        return e.is<ExprUnary>()
            || (e.is<ExprIntConst>() && e.as<ExprIntConst>().value < 0)
            || e.is<ExprAssignment>() || e.is<ExprTernary>();
    }

    void emitUnaryOperand (const Expr& operand, std::string& out)
    {
        if (needsParensAsUnaryOperand (operand))
        {
            out += "(";
            emitExpr (operand, out);
            out += ")";
        }
        else
        {
            emitExpr (operand, out);
        }
    }

    void emitExpr (const Expr& expr, std::string& out)
    {
        if (expr.is<ExprVariable>())
        {
            out += expr.as<ExprVariable>().name;
        }
        else if (expr.is<ExprIntConst>())
        {
            const auto value = expr.as<ExprIntConst>().value;

            // A signed literal used as uint keeps its bit pattern: 0xFFFFFFFF is 4294967295u, not -1
            if (value < 0 && expr.type.has_value() && expr.type->kind == TypeKind::uintType)
                out += std::to_string (static_cast<uint32_t> (value)) + "u";
            else
                out += std::to_string (value);
        }
        else if (expr.is<ExprUIntConst>())
        {
            out += std::to_string (expr.as<ExprUIntConst>().value) + "u";
        }
        else if (expr.is<ExprFloatConst>())
        {
            out += formatFloat (expr.as<ExprFloatConst>().value, expr.loc);
        }
        else if (expr.is<ExprBoolConst>())
        {
            out += expr.as<ExprBoolConst>().value ? "true" : "false";
        }
        else if (expr.is<ExprUnary>())
        {
            emitUnary (expr.as<ExprUnary>(), out);
        }
        else if (expr.is<ExprBinary>())
        {
            const auto& bin = expr.as<ExprBinary>();
            out += "(";
            emitExpr (*bin.left, out);
            out += " " + binaryOpSymbol (bin.op) + " ";
            emitExpr (*bin.right, out);
            out += ")";
        }
        else if (expr.is<ExprTernary>())
        {
            const auto& tern = expr.as<ExprTernary>();
            out += "select(";
            emitExpr (*tern.falseBranch, out);
            out += ", ";
            emitExpr (*tern.trueBranch, out);
            out += ", ";
            emitExpr (*tern.condition, out);
            out += ")";
        }
        else if (expr.is<ExprAssignment>())
        {
            const auto& assign = expr.as<ExprAssignment>();
            emitExpr (*assign.lhs, out);
            out += " " + assignOpSymbol (assign.op) + " ";
            emitExpr (*assign.rhs, out);
        }
        else if (expr.is<ExprBracket>())
        {
            const auto& br = expr.as<ExprBracket>();
            emitPostfixBase (*br.base, out);
            out += "[";
            emitExpr (*br.index, out);
            out += "]";
        }
        else if (expr.is<ExprFunCall>())
        {
            emitCall (expr.as<ExprFunCall>(), out);
        }
        else if (expr.is<ExprDot>())
        {
            const auto& dot = expr.as<ExprDot>();
            emitPostfixBase (*dot.base, out);
            out += "." + dot.member;
        }
        else if (expr.is<ExprComma>())
        {
            throw LoweringError (expr.loc, "Comma operator must be lowered before emission");
        }
        else if (expr.is<ExprTypeConstructor>())
        {
            const auto& tc = expr.as<ExprTypeConstructor>();

            // A sampler2D(texture, sampler) pair outside a sampling call only needs its texture
            if (isSamplerType (tc.type.kind) && tc.args.size() == 2)
            {
                emitExpr (tc.args[0], out);
                return;
            }

            out += typeName (tc.type, tc.loc) + "(";
            emitArguments (tc.args, out);
            out += ")";
        }
        else if (expr.is<ExprParen>())
        {
            out += "(";
            emitExpr (*expr.as<ExprParen>().expr, out);
            out += ")";
        }
    }

    /** Postfix operators bind tighter than unary ones: *p.x must be written (*p).x. */
    void emitPostfixBase (const Expr& base, std::string& out)
    {
        // A dereference already prints its own parentheses
        if (base.is<ExprUnary>() && base.as<ExprUnary>().op != UnaryOp::deref)
        {
            out += "(";
            emitExpr (base, out);
            out += ")";
        }
        else
        {
            emitExpr (base, out);
        }
    }

    void emitUnary (const ExprUnary& un, std::string& out)
    {
        switch (un.op)
        {
            case UnaryOp::plus:
                emitUnaryOperand (*un.operand, out); // WGSL has no unary plus
                break;
            case UnaryOp::minus:
                // Negating INT_MIN wraps back to INT_MIN in GLSL; WGSL can spell it directly
                if (un.operand->is<ExprIntConst>() && un.operand->as<ExprIntConst>().value == std::numeric_limits<int32_t>::min())
                {
                    out += "-2147483648";
                    break;
                }

                out += "-";
                emitUnaryOperand (*un.operand, out);
                break;
            case UnaryOp::logicalNot:
                out += "!";
                emitUnaryOperand (*un.operand, out);
                break;
            case UnaryOp::bitwiseNot:
                out += "~";
                emitUnaryOperand (*un.operand, out);
                break;
            case UnaryOp::preInc:
                emitExpr (*un.operand, out);
                out += " += 1";
                break;
            case UnaryOp::preDec:
                emitExpr (*un.operand, out);
                out += " -= 1";
                break;
            case UnaryOp::postInc:
                emitExpr (*un.operand, out);
                out += "++";
                break;
            case UnaryOp::postDec:
                emitExpr (*un.operand, out);
                out += "--";
                break;
            case UnaryOp::addressOf:
                out += "&";
                emitUnaryOperand (*un.operand, out);
                break;
            case UnaryOp::deref:
                out += "(*";
                emitUnaryOperand (*un.operand, out);
                out += ")";
                break;
        }
    }

    void emitArguments (const std::vector<Expr>& args, std::string& out)
    {
        for (size_t i = 0; i < args.size(); ++i)
        {
            if (i > 0)
                out += ", ";

            emitExpr (args[i], out);
        }
    }

    static bool isLiteral (const Expr& e)
    {
        if (e.is<ExprUnary>() && e.as<ExprUnary>().op == UnaryOp::minus)
            return isLiteral (*e.as<ExprUnary>().operand);

        return e.is<ExprIntConst>() || e.is<ExprUIntConst>() || e.is<ExprFloatConst>() || e.is<ExprBoolConst>();
    }

    void emitConcreteLiteral (const Expr& e, std::string& out)
    {
        if (e.is<ExprUnary>())
        {
            out += "-";
            emitConcreteLiteral (*e.as<ExprUnary>().operand, out);
        }
        else if (e.is<ExprFloatConst>() || (e.is<ExprIntConst>() && e.type.has_value() && e.type->kind == TypeKind::floatType))
        {
            // GLSL float literals are f32; an f suffix keeps them concrete
            out += e.is<ExprFloatConst>() ? formatFloat (e.as<ExprFloatConst>().value, e.loc) : std::to_string (e.as<ExprIntConst>().value) + ".0";
            out += "f";
        }
        else if (e.is<ExprIntConst>() && e.type.has_value() && e.type->kind == TypeKind::uintType)
        {
            out += std::to_string (static_cast<uint32_t> (e.as<ExprIntConst>().value)) + "u";
        }
        else if (e.is<ExprIntConst>() && e.as<ExprIntConst>().value >= 0)
        {
            out += std::to_string (e.as<ExprIntConst>().value) + "i";
        }
        else
        {
            emitExpr (e, out);
        }
    }

    void emitCall (const ExprFunCall& fc, std::string& out)
    {
        if (fc.callee == nullptr || ! fc.callee->is<ExprVariable>())
            throw LoweringError (fc.loc, "Unsupported call expression");

        out += fc.callee->as<ExprVariable>().name + "(";

        // A call made only of abstract literals must be constant-evaluated, which not every
        // WGSL implementation supports for every builtin: typed literals keep it a runtime call
        const bool allLiterals = ! fc.args.empty() && std::all_of (fc.args.begin(), fc.args.end(), [] (const Expr& arg)
        {
            return isLiteral (arg);
        });

        if (allLiterals)
        {
            for (size_t i = 0; i < fc.args.size(); ++i)
            {
                if (i > 0)
                    out += ", ";

                emitConcreteLiteral (fc.args[i], out);
            }
        }
        else
        {
            emitArguments (fc.args, out);
        }

        out += ")";
    }

    static std::string binaryOpSymbol (BinaryOp op)
    {
        switch (op)
        {
            case BinaryOp::add:
                return "+";
            case BinaryOp::sub:
                return "-";
            case BinaryOp::mul:
                return "*";
            case BinaryOp::div:
                return "/";
            case BinaryOp::mod:
                return "%";
            case BinaryOp::shiftLeft:
                return "<<";
            case BinaryOp::shiftRight:
                return ">>";
            case BinaryOp::lessThan:
                return "<";
            case BinaryOp::greaterThan:
                return ">";
            case BinaryOp::lessEqual:
                return "<=";
            case BinaryOp::greaterEqual:
                return ">=";
            case BinaryOp::equal:
                return "==";
            case BinaryOp::notEqual:
            case BinaryOp::logicalXor:
                return "!=";
            case BinaryOp::bitwiseAnd:
                return "&";
            case BinaryOp::bitwiseXor:
                return "^";
            case BinaryOp::bitwiseOr:
                return "|";
            case BinaryOp::logicalAnd:
                return "&&";
            case BinaryOp::logicalOr:
                return "||";
        }

        return "?";
    }

    static std::string assignOpSymbol (AssignmentOp op)
    {
        switch (op)
        {
            case AssignmentOp::assign:
                return "=";
            case AssignmentOp::addAssign:
                return "+=";
            case AssignmentOp::subAssign:
                return "-=";
            case AssignmentOp::mulAssign:
                return "*=";
            case AssignmentOp::divAssign:
                return "/=";
            case AssignmentOp::modAssign:
                return "%=";
            case AssignmentOp::shiftLeftAssign:
                return "<<=";
            case AssignmentOp::shiftRightAssign:
                return ">>=";
            case AssignmentOp::bitwiseAndAssign:
                return "&=";
            case AssignmentOp::bitwiseXorAssign:
                return "^=";
            case AssignmentOp::bitwiseOrAssign:
                return "|=";
        }

        return "=";
    }

    const LoweredProgram& program;
    const LoweredProgram::EntryPointWrapper& entry;
    std::map<std::string, const LoweredProgram::ResourceAssignment*> resources;
};

} // namespace

//==============================================================================
ResultValue<String> WgslEmitter::emit (const LoweredProgram& program)
{
    try
    {
        Emitter emitter (program);
        return makeResultValueOk (String (emitter.emit()));
    }
    catch (const std::exception& e)
    {
        return makeResultValueFail (String (e.what()));
    }
}

} // namespace wgsl
} // namespace yup
