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
// Type name mapping (D3)
//==============================================================================

static const char* wgslTypeName (TypeKind kind)
{
    switch (kind)
    {
        case TypeKind::voidType:
            return "";
        case TypeKind::floatType:
            return "f32";
        case TypeKind::intType:
            return "i32";
        case TypeKind::uintType:
            return "u32";
        case TypeKind::boolType:
            return "bool";
        case TypeKind::doubleType:
            return "f64"; // parses but would error in lowering
        case TypeKind::vec2:
            return "vec2<f32>";
        case TypeKind::vec3:
            return "vec3<f32>";
        case TypeKind::vec4:
            return "vec4<f32>";
        case TypeKind::ivec2:
            return "vec2<i32>";
        case TypeKind::ivec3:
            return "vec3<i32>";
        case TypeKind::ivec4:
            return "vec4<i32>";
        case TypeKind::uvec2:
            return "vec2<u32>";
        case TypeKind::uvec3:
            return "vec3<u32>";
        case TypeKind::uvec4:
            return "vec4<u32>";
        case TypeKind::bvec2:
            return "vec2<bool>";
        case TypeKind::bvec3:
            return "vec3<bool>";
        case TypeKind::bvec4:
            return "vec4<bool>";
        case TypeKind::dvec2:
            return "vec2<f64>";
        case TypeKind::dvec3:
            return "vec3<f64>";
        case TypeKind::dvec4:
            return "vec4<f64>";
        case TypeKind::mat2:
            return "mat2x2<f32>";
        case TypeKind::mat3:
            return "mat3x3<f32>";
        case TypeKind::mat4:
            return "mat4x4<f32>";
        case TypeKind::mat2x2:
            return "mat2x2<f32>";
        case TypeKind::mat2x3:
            return "mat2x3<f32>";
        case TypeKind::mat2x4:
            return "mat2x4<f32>";
        case TypeKind::mat3x2:
            return "mat3x2<f32>";
        case TypeKind::mat3x3:
            return "mat3x3<f32>";
        case TypeKind::mat3x4:
            return "mat3x4<f32>";
        case TypeKind::mat4x2:
            return "mat4x2<f32>";
        case TypeKind::mat4x3:
            return "mat4x3<f32>";
        case TypeKind::mat4x4:
            return "mat4x4<f32>";
        case TypeKind::dmat2:
            return "mat2x2<f64>";
        case TypeKind::dmat2x2:
            return "mat2x2<f64>";
        case TypeKind::dmat2x3:
            return "mat2x3<f64>";
        case TypeKind::dmat2x4:
            return "mat2x4<f64>";
        case TypeKind::dmat3x2:
            return "mat3x2<f64>";
        case TypeKind::dmat3x3:
            return "mat3x3<f64>";
        case TypeKind::dmat3x4:
            return "mat3x4<f64>";
        case TypeKind::dmat4x2:
            return "mat4x2<f64>";
        case TypeKind::dmat4x3:
            return "mat4x3<f64>";
        case TypeKind::dmat4x4:
            return "mat4x4<f64>";

        // Samplers → texture types
        case TypeKind::sampler1D:
            return "texture_1d<f32>";
        case TypeKind::sampler2D:
            return "texture_2d<f32>";
        case TypeKind::sampler3D:
            return "texture_3d<f32>";
        case TypeKind::samplerCube:
            return "texture_cube<f32>";
        case TypeKind::sampler2DShadow:
            return "texture_depth_2d";
        case TypeKind::sampler2DArray:
            return "texture_2d_array<f32>";
        case TypeKind::isampler2D:
            return "texture_2d<i32>";
        case TypeKind::isampler3D:
            return "texture_3d<i32>";
        case TypeKind::isamplerCube:
            return "texture_cube<i32>";
        case TypeKind::isampler2DArray:
            return "texture_2d_array<i32>";
        case TypeKind::usampler2D:
            return "texture_2d<u32>";
        case TypeKind::usampler3D:
            return "texture_3d<u32>";
        case TypeKind::usamplerCube:
            return "texture_cube<u32>";
        case TypeKind::usampler2DArray:
            return "texture_2d_array<u32>";

        // Separate texture types (non-sampler)
        case TypeKind::texture1D:
            return "texture_1d<f32>";
        case TypeKind::texture2D:
            return "texture_2d<f32>";
        case TypeKind::texture3D:
            return "texture_3d<f32>";
        case TypeKind::textureCube:
            return "texture_cube<f32>";
        case TypeKind::texture2DArray:
            return "texture_2d_array<f32>";
        case TypeKind::texture2DMS:
            return "texture_multisampled_2d<f32>";

        // Separate sampler types
        case TypeKind::samplerType:
            return "sampler";
        case TypeKind::samplerShadow:
            return "sampler_comparison";

        default:
            return "f32"; // fallback
    }
}

static bool isTextureType (TypeKind kind)
{
    return isSamplerType (kind);
}

static bool isSeparateTextureType (TypeKind kind)
{
    switch (kind)
    {
        case TypeKind::texture1D:
        case TypeKind::texture2D:
        case TypeKind::texture3D:
        case TypeKind::textureCube:
        case TypeKind::texture1DArray:
        case TypeKind::texture2DArray:
        case TypeKind::texture2DRect:
        case TypeKind::texture2DMS:
        case TypeKind::texture2DMSArray:
            return true;
        default:
            return false;
    }
}

static TypeKind samplerToTextureKind (TypeKind sk)
{
    // Map sampler types to their texture counterparts
    // For simplicity, just return the same kind; the emitter uses wgslTypeName
    return sk;
}

//==============================================================================
// Function name mapping (D5)
//==============================================================================

static const char* mapFunctionName (const std::string& glslName)
{
    if (glslName == "texture")
        return "textureSample";
    if (glslName == "textureLod")
        return "textureSampleLevel";
    if (glslName == "textureSize")
        return "textureDimensions";
    if (glslName == "texelFetch")
        return "textureLoad";
    if (glslName == "atan")
        return "atan2";
    if (glslName == "dFdx")
        return "dpdx";
    if (glslName == "dFdy")
        return "dpdy";
    if (glslName == "fwidth")
        return "fwidth";
    if (glslName == "inversesqrt")
        return "inverseSqrt";
    if (glslName == "mod")
        return nullptr; // special inline handling
    if (glslName == "clamp")
        return "clamp";
    if (glslName == "mix")
        return "mix";
    if (glslName == "step")
        return "step";
    if (glslName == "smoothstep")
        return "smoothstep";
    if (glslName == "normalize")
        return "normalize";
    if (glslName == "length")
        return "length";
    if (glslName == "distance")
        return "distance";
    if (glslName == "dot")
        return "dot";
    if (glslName == "cross")
        return "cross";
    if (glslName == "reflect")
        return "reflect";
    if (glslName == "refract")
        return "refract";
    if (glslName == "faceforward")
        return "faceForward";
    if (glslName == "transpose")
        return "transpose";
    if (glslName == "degrees")
        return "degrees";
    if (glslName == "radians")
        return "radians";
    if (glslName == "sin")
        return "sin";
    if (glslName == "cos")
        return "cos";
    if (glslName == "tan")
        return "tan";
    if (glslName == "asin")
        return "asin";
    if (glslName == "acos")
        return "acos";
    if (glslName == "atan2")
        return "atan2";
    if (glslName == "pow")
        return "pow";
    if (glslName == "exp")
        return "exp";
    if (glslName == "log")
        return "log";
    if (glslName == "exp2")
        return "exp2";
    if (glslName == "log2")
        return "log2";
    if (glslName == "sqrt")
        return "sqrt";
    if (glslName == "inversesqrt")
        return "inverseSqrt";
    if (glslName == "abs")
        return "abs";
    if (glslName == "sign")
        return "sign";
    if (glslName == "floor")
        return "floor";
    if (glslName == "trunc")
        return "trunc";
    if (glslName == "round")
        return "round";
    if (glslName == "ceil")
        return "ceil";
    if (glslName == "fract")
        return "fract";
    if (glslName == "min")
        return "min";
    if (glslName == "max")
        return "max";
    if (glslName == "isnan")
        return "isNan";
    if (glslName == "isinf")
        return "isInf";
    // Vector relational functions
    if (glslName == "lessThan")
        return nullptr; // use operators
    if (glslName == "lessThanEqual")
        return nullptr;
    if (glslName == "greaterThan")
        return nullptr;
    if (glslName == "greaterThanEqual")
        return nullptr;
    if (glslName == "equal")
        return nullptr;
    if (glslName == "notEqual")
        return nullptr;

    return glslName.c_str(); // Pass through
}

//==============================================================================
// WGSL emitter implementation
//==============================================================================

class Emitter
{
public:
    Emitter (const LoweredProgram& prog, const WgslEmitOptions& opts)
        : program (prog)
        , options (opts)
    {
    }

    std::string emit()
    {
        std::string out;

        // Emit polyfills first
        for (auto& pf : program.polyfills)
            out += pf + "\n";

        // Emit user-defined struct types (must precede globals that reference them)
        emitStructTypes (out);

        // Emit module-scope declarations (resources with @group/@binding)
        emitResources (out);

        // Emit stage IO private variables
        emitStageIOPrivateVars (out);

        // Emit functions (original user functions)
        emitFunctions (out);

        // Emit entry-point wrapper
        emitEntryPoint (out);

        return out;
    }

private:
    //==========================================================================
    // Struct type emission
    //==========================================================================

    void emitStructTypes (std::string& out)
    {
        for (auto& decl : program.ast.declarations)
        {
            if (! std::holds_alternative<Declaration> (decl))
                continue;

            auto& d = std::get<Declaration> (decl);
            if (! d.structSpecifier)
                continue;

            // Skip structs from unnamed interface blocks — they are flattened
            // into individual global variables.
            if (! d.initDeclaratorList && d.qualifier
                && (d.qualifier->hasStorage (StorageQualifier::uniform)
                    || d.qualifier->hasStorage (StorageQualifier::buffer)))
                continue;

            auto& ss = *d.structSpecifier;

            // Emit struct definition
            out += "struct " + ss.name + " {\n";
            for (auto& field : ss.fields)
                out += "    " + field.name + ": " + typeSpecToString (field.type) + ",\n";
            out += "}\n\n";
        }
    }

    //==========================================================================
    // Resource emission
    //==========================================================================

    void emitResources (std::string& out)
    {
        for (auto& decl : program.ast.declarations)
        {
            if (! std::holds_alternative<Declaration> (decl))
                continue;

            auto& d = std::get<Declaration> (decl);

            // Handle unnamed interface blocks (structSpecifier + qualifier but no initDeclaratorList).
            // Each struct field becomes a separate global variable in WGSL.
            if (! d.initDeclaratorList && d.structSpecifier && d.qualifier
                && (d.qualifier->hasStorage (StorageQualifier::uniform)
                    || d.qualifier->hasStorage (StorageQualifier::buffer)))
            {
                bool isBuffer = d.qualifier->hasStorage (StorageQualifier::buffer);
                std::string addrSpace = isBuffer ? "storage" : "uniform";

                for (auto& field : d.structSpecifier->fields)
                {
                    const LoweredProgram::ResourceAssignment* assign = nullptr;
                    for (auto& r : program.resources)
                    {
                        if (r.name == field.name)
                        {
                            assign = &r;
                            break;
                        }
                    }

                    if (assign)
                    {
                        out += "@group(" + std::to_string (assign->group) + ") "
                             + "@binding(" + std::to_string (assign->binding) + ") ";
                    }

                    out += "var<" + addrSpace + (isBuffer ? ", read_write" : "") + "> " + field.name + ": " + genericTypeName (field.type) + ";\n";
                }
                continue;
            }

            if (! d.initDeclaratorList)
                continue;

            auto& il = *d.initDeclaratorList;
            if (! il.qualifier)
                continue;

            bool isUniform = il.qualifier->hasStorage (StorageQualifier::uniform);
            bool isBuffer = il.qualifier->hasStorage (StorageQualifier::buffer);
            bool isIn = il.qualifier->hasStorage (StorageQualifier::in);
            bool isOut = il.qualifier->hasStorage (StorageQualifier::out);

            // Stage IO variables → handled separately
            if ((isIn || isOut) && ! isUniform && ! isBuffer)
                continue;

            for (auto& sd : il.declarations)
            {
                const LoweredProgram::ResourceAssignment* assign = nullptr;
                for (auto& r : program.resources)
                {
                    if (r.name == sd.name)
                    {
                        assign = &r;
                        break;
                    }
                }

                std::string addrSpace = "uniform";
                if (isBuffer)
                    addrSpace = "storage";

                // Textures and samplers are handle resources → omit address space
                bool isHandle = isSamplerType (il.type.kind)
                             || isSeparateTextureType (il.type.kind)
                             || il.type.kind == TypeKind::samplerType
                             || il.type.kind == TypeKind::samplerShadow;

                if (assign)
                {
                    out += "@group(" + std::to_string (assign->group) + ") "
                         + "@binding(" + std::to_string (assign->binding) + ") ";
                }

                if (isSamplerType (il.type.kind))
                {
                    // Combined sampler → texture_2d<f32> + sampler
                    out += "var " + sd.name + ": " + wgslTypeName (il.type.kind) + ";\n";

                    // Companion sampler
                    if (assign && assign->samplerBinding != ~0u)
                    {
                        out += "@group(" + std::to_string (assign->group) + ") "
                             + "@binding(" + std::to_string (assign->samplerBinding) + ") ";
                    }

                    out += "var " + sd.name + "_sampler: sampler;\n";
                }
                else if (isHandle)
                {
                    out += "var " + sd.name + ": " + genericTypeName (il.type) + ";\n";
                }
                else
                {
                    out += "var<" + addrSpace + (isBuffer ? ", read_write" : "") + "> " + sd.name + ": " + genericTypeName (il.type) + ";\n";
                }
            }
        }
    }

    //==========================================================================
    // Stage IO private variables (Task 2.5 / D1)
    //==========================================================================

    void emitStageIOPrivateVars (std::string& out)
    {
        for (auto& io : program.entryPoint.inputs)
        {
            if (! io.isBuiltin)
                out += "var<private> " + io.name + ": " + typeSpecToString (io.wgslType) + ";\n";
        }

        for (auto& io : program.entryPoint.outputs)
        {
            if (! io.isBuiltin)
                out += "var<private> " + io.name + ": " + typeSpecToString (io.wgslType) + ";\n";
        }

        // gl_Position is used as an implicit builtin in vertex shaders
        if (program.entryPoint.isVertex)
        {
            out += "var<private> gl_Position: vec4<f32>;\n";
            out += "var<private> gl_VertexIndex: u32;\n";
            out += "var<private> gl_InstanceIndex: u32;\n";
        }

        // gl_FragCoord / gl_FrontFacing as implicit builtins in fragment shaders
        if (program.entryPoint.isFragment)
        {
            out += "var<private> gl_FragCoord: vec4<f32>;\n";
            out += "var<private> gl_FrontFacing: bool;\n";
        }

        // Compute builtins (gl_GlobalInvocationID, etc.) — declared as private
        // so main_inner() can access them.
        if (program.entryPoint.isCompute)
        {
            for (auto& io : program.entryPoint.inputs)
            {
                if (io.isBuiltin)
                    out += "var<private> " + io.name + ": " + computeInputType (io.builtinName) + ";\n";
            }
        }
    }

    //==========================================================================
    // Function emission
    //==========================================================================

    void emitFunctions (std::string& out)
    {
        for (auto& decl : program.ast.declarations)
        {
            if (std::holds_alternative<FunctionDefinition> (decl))
            {
                auto& fd = std::get<FunctionDefinition> (decl);

                std::string funcName = fd.prototype.name;

                // Rename "main" → "main_inner" (the entry-point wrapper calls it)
                if (funcName == "main")
                    funcName = "main_inner";

                // Function signature
                out += "fn " + funcName + "(";
                bool first = true;
                for (auto& param : fd.prototype.parameters)
                {
                    if (! first)
                        out += ", ";
                    first = false;

                    if (param.qualifier)
                    {
                        if (param.qualifier->hasStorage (StorageQualifier::out) || param.qualifier->hasStorage (StorageQualifier::inout))
                            out += "&";
                    }

                    out += param.name + ": " + typeSpecToString (param.type);
                }
                out += ")";

                // Return type
                if (fd.prototype.returnType.kind != TypeKind::voidType)
                    out += " -> " + typeSpecToString (fd.prototype.returnType);

                out += " ";

                // Body
                if (fd.body)
                    emitStatement (*fd.body, out, 0);
                else
                    out += "{}";

                out += "\n";
            }
        }
    }

    //==========================================================================
    // Entry-point wrapper emission (D1)
    //==========================================================================

    void emitEntryPoint (std::string& out)
    {
        if (program.entryPoint.isCompute)
        {
            emitComputeEntryPoint (out);
            return;
        }

        bool hasInputs = ! program.entryPoint.inputs.empty()
                      || program.entryPoint.isVertex
                      || program.entryPoint.isFragment; // implicit builtin inputs
        bool hasOutputs = ! program.entryPoint.outputs.empty()
                       || program.entryPoint.isVertex; // implicit position builtin

        // Generate IO structs if needed
        std::string inputStruct;
        std::string outputStruct;

        if (hasInputs)
            inputStruct = emitIOStruct (program.entryPoint.inputs,
                                        program.entryPoint.isVertex ? "VSInput" : "FSInput");

        // Implicit builtin inputs
        if (program.entryPoint.isVertex && inputStruct.find ("@builtin(vertex_index)") == std::string::npos)
        {
            if (inputStruct.empty())
                inputStruct = std::string ("struct ") + (program.entryPoint.isVertex ? "VSInput" : "FSInput") + " {\n}";
            inputStruct.pop_back(); // remove trailing }
            inputStruct += "    @builtin(vertex_index) vertex_index: u32,\n";
            inputStruct += "    @builtin(instance_index) instance_index: u32,\n}";
        }
        if (program.entryPoint.isFragment && inputStruct.find ("@builtin(position)") == std::string::npos)
        {
            if (inputStruct.empty())
                inputStruct = "struct FSInput {\n}";
            inputStruct.pop_back();
            inputStruct += "    @builtin(position) frag_coord: vec4<f32>,\n";
            inputStruct += "    @builtin(front_facing) front_facing: bool,\n}";
        }

        if (hasOutputs)
            outputStruct = emitIOStruct (program.entryPoint.outputs,
                                         program.entryPoint.isVertex ? "VSOutput" : "FSOutput");

        // Vertex shaders always need @builtin(position) in the output struct.
        if (program.entryPoint.isVertex && outputStruct.find ("@builtin(position)") == std::string::npos)
        {
            // Append position builtin to the output struct
            outputStruct.pop_back(); // remove trailing }
            outputStruct += "    @builtin(position) position: vec4<f32>,\n}";
        }

        if (! inputStruct.empty())
            out += inputStruct + "\n";
        if (! outputStruct.empty())
            out += outputStruct + "\n";

        // Entry-point attribute
        out += "@" + std::string (program.entryPoint.isVertex ? "vertex" : "fragment") + "\n";

        // Function signature
        std::string epName = options.outputEntryPoint.isNotEmpty()
                               ? options.outputEntryPoint.toStdString()
                               : "main";
        out += "fn " + epName + "(";

        if (hasInputs)
            out += "input: " + std::string (program.entryPoint.isVertex ? "VSInput" : "FSInput");
        out += ")";

        if (hasOutputs)
        {
            std::string outStructName = program.entryPoint.isVertex ? "VSOutput" : "FSOutput";
            out += " -> " + outStructName;
        }

        out += " {\n";

        // Copy inputs in
        for (auto& io : program.entryPoint.inputs)
        {
            if (! io.isBuiltin)
                out += "    " + io.name + " = input." + io.name + ";\n";
        }

        // Copy implicit builtin inputs
        if (program.entryPoint.isVertex)
        {
            out += "    gl_VertexIndex = input.vertex_index;\n";
            out += "    gl_InstanceIndex = input.instance_index;\n";
        }
        if (program.entryPoint.isFragment)
        {
            out += "    gl_FragCoord = input.frag_coord;\n";
            out += "    gl_FrontFacing = input.front_facing;\n";
        }

        // Call inner function
        out += "    main_inner();\n";

        // Build output struct
        if (hasOutputs)
        {
            std::string outStructName = program.entryPoint.isVertex ? "VSOutput" : "FSOutput";
            out += "    var output: " + outStructName + ";\n";
            for (auto& io : program.entryPoint.outputs)
            {
                if (! io.isBuiltin)
                    out += "    output." + io.name + " = " + io.name + ";\n";
            }

            // Builtins
            bool hasPosition = false;
            for (auto& io : program.entryPoint.outputs)
            {
                if (io.isBuiltin && (io.builtinName == "position" || io.builtinName == "frag_depth"))
                {
                    out += "    output." + io.builtinName + " = " + io.name + ";\n";
                    if (io.builtinName == "position")
                        hasPosition = true;
                }
            }

            // Copy gl_Position to @builtin(position)
            if (! hasPosition && program.entryPoint.isVertex)
                out += "    output.position = gl_Position;\n";

            out += "    return output;\n";
        }

        out += "}\n";
    }

    void emitComputeEntryPoint (std::string& out)
    {
        out += "@compute\n";
        out += "@workgroup_size("
             + std::to_string (program.entryPoint.workgroupSizeX) + ", "
             + std::to_string (program.entryPoint.workgroupSizeY) + ", "
             + std::to_string (program.entryPoint.workgroupSizeZ) + ")\n";

        std::string epName = options.outputEntryPoint.isNotEmpty()
                               ? options.outputEntryPoint.toStdString()
                               : "main";
        out += "fn " + epName + "(";

        // Compute builtin params
        bool first = true;
        for (auto& io : program.entryPoint.inputs)
        {
            if (io.isBuiltin)
            {
                if (! first)
                    out += ", ";
                first = false;

                out += "@builtin(" + io.builtinName + ") " + io.builtinName + ": " + computeInputType (io.builtinName);
            }
        }

        out += ") {\n";
        // Copy entry-point builtin params to private globals so main_inner() can access them
        for (auto& io : program.entryPoint.inputs)
        {
            if (io.isBuiltin)
                out += "    " + io.name + " = " + io.builtinName + ";\n";
        }
        out += "    main_inner();\n";
        out += "}\n";
    }

    std::string emitIOStruct (const std::vector<LoweredProgram::InputOutputInfo>& ios,
                              const char* structName)
    {
        std::string s = "struct " + std::string (structName) + " {\n";

        for (auto& io : ios)
        {
            if (io.isBuiltin)
                s += "    @builtin(" + io.builtinName + ") ";
            else
                s += "    @location(" + std::to_string (io.location) + ") ";

            s += io.name + ": " + typeSpecToString (io.wgslType) + ",\n";
        }

        s += "}";
        return s;
    }

    std::string computeInputType (const std::string& builtin)
    {
        if (builtin == "global_invocation_id" || builtin == "local_invocation_id"
            || builtin == "workgroup_id" || builtin == "num_workgroups")
            return "vec3<u32>";

        if (builtin == "local_invocation_index")
            return "u32";

        return "u32";
    }

    //==========================================================================
    // Statement emission
    //==========================================================================

    void emitStatement (const Statement& stmt, std::string& out, int indent)
    {
        std::string ind (indent * 4, ' ');

        if (stmt.is<StmtCompound>())
        {
            auto& comp = stmt.as<StmtCompound>();
            out += ind + "{\n";
            for (auto& s : comp.statements)
                emitStatement (s, out, indent + 1);
            out += ind + "}\n";
        }
        else if (stmt.is<StmtSelection>())
        {
            auto& sel = stmt.as<StmtSelection>();
            out += ind + "if (";
            if (sel.condition)
                emitExpr (*sel.condition, out);
            out += ") {\n";
            if (sel.thenBranch)
                emitStatement (*sel.thenBranch, out, indent + 1);
            out += ind + "}\n";

            if (sel.elseBranch)
            {
                out += ind + "else ";
                // Preserve 'else if' — emit the inner selection without wrapping it in
                // an extra layer, since emitStatement will handle its own braces.
                if (sel.elseBranch->is<StmtSelection>())
                    emitStatement (*sel.elseBranch, out, indent);
                else
                {
                    out += "{\n";
                    emitStatement (*sel.elseBranch, out, indent + 1);
                    out += ind + "}\n";
                }
            }
        }
        else if (stmt.is<StmtSwitch>())
        {
            auto& sw = stmt.as<StmtSwitch>();
            out += ind + "switch (";
            if (sw.selector)
                emitExpr (*sw.selector, out);
            out += ") {\n";
            for (auto& s : sw.body)
                emitStatement (s, out, indent + 1);
            out += ind + "}\n";
        }
        else if (stmt.is<StmtCaseLabel>())
        {
            auto& cl = stmt.as<StmtCaseLabel>();
            if (cl.label)
            {
                out += std::string ((indent - 1) * 4, ' ') + "case ";
                emitExpr (*cl.label, out);
                out += ": {}\n";
            }
            else
            {
                out += std::string ((indent - 1) * 4, ' ') + "default: {}\n";
            }
        }
        else if (stmt.is<StmtWhile>())
        {
            auto& w = stmt.as<StmtWhile>();
            out += ind + "while (";
            if (w.condition)
                emitExpr (*w.condition, out);
            out += ") {\n";
            if (w.body)
                emitStatement (*w.body, out, indent + 1);
            out += ind + "}\n";
        }
        else if (stmt.is<StmtDoWhile>())
        {
            auto& dw = stmt.as<StmtDoWhile>();
            // do-while → loop { body; if (!(cond)) { break; } }
            out += ind + "loop {\n";
            if (dw.body)
                emitStatementBody (*dw.body, out, indent + 1);
            out += std::string ((indent + 1) * 4, ' ') + "if (!(";
            if (dw.condition)
                emitExpr (*dw.condition, out);
            out += ")) { break; }\n";
            out += ind + "}\n";
        }
        else if (stmt.is<StmtFor>())
        {
            auto& f = stmt.as<StmtFor>();
            out += ind + "for (";

            // WGSL for: for (init; cond; update) { body }
            // init
            if (f.init)
            {
                if (f.init->is<StmtDeclaration>())
                {
                    emitStatementInline (*f.init, out);
                }
                else if (f.init->is<StmtExpr>() && f.init->as<StmtExpr>().expr)
                {
                    emitExpr (*f.init->as<StmtExpr>().expr, out);
                }
            }
            out += "; ";

            // cond
            if (f.condition)
                emitExpr (*f.condition, out);
            out += "; ";

            // update
            if (f.update)
            {
                auto* update = f.update.get();
                while (update->is<ExprParen>() && update->as<ExprParen>().expr)
                    update = update->as<ExprParen>().expr.get();
                emitExpr (*update, out);
            }

            out += ") {\n";
            if (f.body)
                emitStatement (*f.body, out, indent + 1);
            out += ind + "}\n";
        }
        else if (stmt.is<StmtJump>())
        {
            auto& j = stmt.as<StmtJump>();
            switch (j.kind)
            {
                case JumpKind::returnJump:
                    out += ind + "return";
                    if (j.returnValue)
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
            auto& se = stmt.as<StmtExpr>();
            out += ind;
            if (se.expr)
                emitExpr (*se.expr, out);
            out += ";\n";
        }
        else if (stmt.is<StmtDeclaration>())
        {
            auto& sd = stmt.as<StmtDeclaration>();
            emitDeclarationStmt (sd.declaration, out, indent);
        }
    }

    void emitStatementBody (const Statement& stmt, std::string& out, int indent)
    {
        // Like emitStatement but without outer braces for compound
        if (stmt.is<StmtCompound>())
        {
            auto& comp = stmt.as<StmtCompound>();
            for (auto& s : comp.statements)
                emitStatement (s, out, indent);
        }
        else
        {
            emitStatement (stmt, out, indent);
        }
    }

    void emitStatementInline (const Statement& stmt, std::string& out)
    {
        if (stmt.is<StmtDeclaration>())
        {
            auto& sd = stmt.as<StmtDeclaration>();
            if (sd.declaration.initDeclaratorList)
            {
                auto& il = *sd.declaration.initDeclaratorList;
                for (size_t i = 0; i < il.declarations.size(); ++i)
                {
                    if (i > 0)
                        out += ", ";

                    auto& dec = il.declarations[i];

                    SymbolInfo* info = symbolLookup (dec.name);
                    if (info && ! info->isReassigned && ! info->isConst)
                        out += "let ";
                    else
                        out += "var ";

                    out += dec.name;

                    if (! info || info->isReassigned || info->isConst)
                        out += ": " + genericTypeName (il.type);

                    if (dec.initializer && dec.initializer->expr)
                    {
                        out += " = ";
                        emitExpr (*dec.initializer->expr, out);
                    }
                }
            }
        }
        else if (stmt.is<StmtExpr>() && stmt.as<StmtExpr>().expr)
        {
            emitExpr (*stmt.as<StmtExpr>().expr, out);
        }
    }

    void emitDeclarationStmt (const Declaration& decl, std::string& out, int indent)
    {
        std::string ind (indent * 4, ' ');

        if (decl.initDeclaratorList)
        {
            auto& il = *decl.initDeclaratorList;
            for (auto& dec : il.declarations)
            {
                out += ind;

                SymbolInfo* info = symbolLookup (dec.name);
                if (info && ! info->isReassigned && ! info->isConst)
                    out += "let ";
                else
                    out += "var ";

                out += dec.name;

                if (! info || info->isReassigned || info->isConst)
                {
                    auto fullType = il.type;
                    fullType.arraySpecifiers.insert (fullType.arraySpecifiers.end(),
                                                     dec.arraySpecifiers.begin(),
                                                     dec.arraySpecifiers.end());
                    out += ": " + genericTypeName (fullType);
                }

                if (dec.initializer && dec.initializer->expr)
                {
                    out += " = ";
                    emitExpr (*dec.initializer->expr, out);
                }

                out += ";\n";
            }
        }
    }

    //==========================================================================
    // Expression emission
    //==========================================================================

    void emitExpr (const Expr& expr, std::string& out)
    {
        if (expr.is<ExprVariable>())
        {
            auto& var = expr.as<ExprVariable>();
            out += var.name;
        }
        else if (expr.is<ExprIntConst>())
        {
            auto& ic = expr.as<ExprIntConst>();
            out += std::to_string (ic.value);
        }
        else if (expr.is<ExprUIntConst>())
        {
            auto& uc = expr.as<ExprUIntConst>();
            out += std::to_string (uc.value) + "u";
        }
        else if (expr.is<ExprFloatConst>())
        {
            auto& fc = expr.as<ExprFloatConst>();
            out += formatFloat (fc.value);
        }
        else if (expr.is<ExprBoolConst>())
        {
            auto& bc = expr.as<ExprBoolConst>();
            out += bc.value ? "true" : "false";
        }
        else if (expr.is<ExprUnary>())
        {
            auto& un = expr.as<ExprUnary>();
            switch (un.op)
            {
                case UnaryOp::plus:
                    out += "+";
                    if (un.operand)
                        emitExpr (*un.operand, out);
                    break;
                case UnaryOp::minus:
                    out += "-";
                    if (un.operand)
                        emitExpr (*un.operand, out);
                    break;
                case UnaryOp::logicalNot:
                    out += "!";
                    if (un.operand)
                        emitExpr (*un.operand, out);
                    break;
                case UnaryOp::bitwiseNot:
                    out += "~";
                    if (un.operand)
                        emitExpr (*un.operand, out);
                    break;
                case UnaryOp::preInc:
                    if (un.operand)
                        emitExpr (*un.operand, out);
                    out += " += 1";
                    break;
                case UnaryOp::preDec:
                    if (un.operand)
                        emitExpr (*un.operand, out);
                    out += " -= 1";
                    break;
                case UnaryOp::postInc:
                    if (un.operand)
                        emitExpr (*un.operand, out);
                    out += "++";
                    break;
                case UnaryOp::postDec:
                    if (un.operand)
                        emitExpr (*un.operand, out);
                    out += "--";
                    break;
            }
        }
        else if (expr.is<ExprBinary>())
        {
            auto& bin = expr.as<ExprBinary>();
            if (bin.op == BinaryOp::mod)
            {
                // Floor-mod: mod(x,y) → (x - y * floor(x / y))
                out += "(";
                if (bin.left)
                    emitExpr (*bin.left, out);
                out += " - ";
                if (bin.right)
                    emitExpr (*bin.right, out);
                out += " * floor(";
                if (bin.left)
                    emitExpr (*bin.left, out);
                out += " / ";
                if (bin.right)
                    emitExpr (*bin.right, out);
                out += "))";
            }
            else
            {
                out += "(";
                if (bin.left)
                    emitExpr (*bin.left, out);
                out += " " + binaryOpSymbol (bin.op) + " ";
                if (bin.right)
                    emitExpr (*bin.right, out);
                out += ")";
            }
        }
        else if (expr.is<ExprTernary>())
        {
            // Ternary → select(c, b, a) for simple cases
            auto& tern = expr.as<ExprTernary>();

            // Check if branches are side-effect free → use select()
            // For now, always use select() as branches typically are simple in shader code
            bool useSelect = true; // Heuristic: most shader ternaries are simple

            if (useSelect)
            {
                out += "select(";
                if (tern.falseBranch)
                    emitExpr (*tern.falseBranch, out);
                out += ", ";
                if (tern.trueBranch)
                    emitExpr (*tern.trueBranch, out);
                out += ", ";
                if (tern.condition)
                    emitExpr (*tern.condition, out);
                out += ")";
            }
            else
            {
                // Fallback: emit as if/else (handled at statement level by lowering)
                out += "(0)"; // placeholder - this case is handled by statement lowering
            }
        }
        else if (expr.is<ExprAssignment>())
        {
            auto& assign = expr.as<ExprAssignment>();
            if (assign.lhs)
                emitExpr (*assign.lhs, out);
            out += " " + assignOpSymbol (assign.op) + " ";
            if (assign.rhs)
                emitExpr (*assign.rhs, out);
        }
        else if (expr.is<ExprBracket>())
        {
            auto& br = expr.as<ExprBracket>();
            if (br.base)
                emitExpr (*br.base, out);
            out += "[";
            if (br.index)
                emitExpr (*br.index, out);
            out += "]";
        }
        else if (expr.is<ExprFunCall>())
        {
            auto& fc = expr.as<ExprFunCall>();

            if (fc.callee && fc.callee->is<ExprVariable>())
            {
                auto& calleeName = fc.callee->as<ExprVariable>().name;
                auto* mapped = mapFunctionName (calleeName);

                if (mapped == nullptr && calleeName == "mod")
                {
                    // Inline floor-mod
                    out += "(";
                    if (! fc.args.empty())
                        emitExpr (fc.args[0], out);
                    out += " - ";
                    if (fc.args.size() > 1)
                        emitExpr (fc.args[1], out);
                    out += " * floor(";
                    if (! fc.args.empty())
                        emitExpr (fc.args[0], out);
                    out += " / ";
                    if (fc.args.size() > 1)
                        emitExpr (fc.args[1], out);
                    out += "))";
                    return;
                }
                else if (mapped == nullptr && (calleeName == "lessThan" || calleeName == "lessThanEqual" || calleeName == "greaterThan" || calleeName == "greaterThanEqual" || calleeName == "equal" || calleeName == "notEqual"))
                {
                    // Vector relational → use comparison operators
                    std::string opSym;
                    if (calleeName == "lessThan")
                        opSym = " < ";
                    else if (calleeName == "lessThanEqual")
                        opSym = " <= ";
                    else if (calleeName == "greaterThan")
                        opSym = " > ";
                    else if (calleeName == "greaterThanEqual")
                        opSym = " >= ";
                    else if (calleeName == "equal")
                        opSym = " == ";
                    else if (calleeName == "notEqual")
                        opSym = " != ";

                    out += "(";
                    if (! fc.args.empty())
                        emitExpr (fc.args[0], out);
                    out += opSym;
                    if (fc.args.size() > 1)
                        emitExpr (fc.args[1], out);
                    out += ")";
                    return;
                }
                else if (calleeName == "texture" && isTextureSampleCall (fc))
                {
                    // texture(sampler, uv) → textureSample(sampler, sampler_sampler, uv)
                    const char* textureFunc = program.entryPoint.isFragment ? "textureSample" : "textureSampleLevel";
                    out += std::string (textureFunc) + "(";

                    if (! fc.args.empty())
                        emitExpr (fc.args[0], out);
                    out += ", ";

                    // Companion sampler
                    if (fc.args.size() >= 1 && fc.args[0].is<ExprVariable>())
                        out += fc.args[0].as<ExprVariable>().name + "_sampler";
                    else
                        out += "samp";

                    out += ", ";

                    if (fc.args.size() > 1)
                        emitExpr (fc.args[1], out);

                    // For vertex/compute, textureSampleLevel needs lod arg
                    if (! program.entryPoint.isFragment)
                        out += ", 0.0";

                    out += ")";
                    return;
                }
                else if (calleeName == "texture" && fc.args.size() >= 2
                         && fc.args[0].is<ExprTypeConstructor>()
                         && fc.args[0].as<ExprTypeConstructor>().args.size() == 2)
                {
                    // texture(sampler2D(tex, sampler), uv) → textureSample(tex, sampler, uv)
                    auto& samplerCtor = fc.args[0].as<ExprTypeConstructor>();
                    const char* textureFunc = program.entryPoint.isFragment ? "textureSample" : "textureSampleLevel";
                    out += std::string (textureFunc) + "(";
                    emitExpr (samplerCtor.args[0], out); // tex
                    out += ", ";
                    emitExpr (samplerCtor.args[1], out); // sampler
                    out += ", ";
                    emitExpr (fc.args[1], out); // uv
                    if (fc.args.size() > 2)
                    {
                        out += ", ";
                        emitExpr (fc.args[2], out); // bias / extra arg
                    }
                    if (! program.entryPoint.isFragment)
                        out += ", 0.0";
                    out += ")";
                    return;
                }
                else if (calleeName == "textureLod" && fc.args.size() >= 2
                         && fc.args[0].is<ExprTypeConstructor>()
                         && fc.args[0].as<ExprTypeConstructor>().args.size() == 2)
                {
                    // textureLod(sampler2D(tex, sampler), uv, lod) → textureSampleLevel(tex, sampler, uv, lod)
                    auto& samplerCtor = fc.args[0].as<ExprTypeConstructor>();
                    out += "textureSampleLevel(";
                    emitExpr (samplerCtor.args[0], out); // tex
                    out += ", ";
                    emitExpr (samplerCtor.args[1], out); // sampler
                    out += ", ";
                    emitExpr (fc.args[1], out); // uv
                    if (fc.args.size() > 2)
                    {
                        out += ", ";
                        emitExpr (fc.args[2], out); // lod
                    }
                    out += ")";
                    return;
                }

                // Regular function name mapping
                out += mapped ? std::string (mapped) : calleeName;
            }
            else if (fc.callee)
            {
                emitExpr (*fc.callee, out);
            }

            out += "(";
            for (size_t i = 0; i < fc.args.size(); ++i)
            {
                if (i > 0)
                    out += ", ";
                emitExpr (fc.args[i], out);
            }
            out += ")";
        }
        else if (expr.is<ExprDot>())
        {
            auto& dot = expr.as<ExprDot>();
            if (dot.base)
                emitExpr (*dot.base, out);
            out += "." + dot.member;
        }
        else if (expr.is<ExprComma>())
        {
            auto& com = expr.as<ExprComma>();
            // Comma operator is not directly supported in WGSL; emit as statement sequence
            // This is handled by statement legalization
            if (com.left)
                emitExpr (*com.left, out);
            out += "; /* comma operator */ ";
            if (com.right)
                emitExpr (*com.right, out);
        }
        else if (expr.is<ExprTypeConstructor>())
        {
            auto& tc = expr.as<ExprTypeConstructor>();
            out += genericTypeName (tc.type);
            out += "(";
            for (size_t i = 0; i < tc.args.size(); ++i)
            {
                if (i > 0)
                    out += ", ";
                emitExpr (tc.args[i], out);
            }
            out += ")";
        }
        else if (expr.is<ExprParen>())
        {
            auto& p = expr.as<ExprParen>();
            out += "(";
            if (p.expr)
                emitExpr (*p.expr, out);
            out += ")";
        }
    }

    //==========================================================================
    // Helpers
    //==========================================================================

    std::string typeSpecToString (const TypeSpecifier& ts)
    {
        if (ts.kind == TypeKind::namedStruct)
            return ts.structName;

        std::string s = wgslTypeName (ts.kind);

        for (auto& arr : ts.arraySpecifiers)
        {
            if (arr.isUnsized || ! arr.sizeExpr)
                s = "array<" + s + ">";
            else if (arr.sizeExpr->is<ExprIntConst>())
                s = "array<" + s + ", " + std::to_string (arr.sizeExpr->as<ExprIntConst>().value) + ">";
        }

        return s;
    }

    std::string genericTypeName (const TypeSpecifier& ts)
    {
        return typeSpecToString (ts);
    }

    std::string binaryOpSymbol (BinaryOp op)
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

    std::string assignOpSymbol (AssignmentOp op)
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

    std::string formatFloat (double v)
    {
        // Format float with WGSL-compatible notation
        // Always include a decimal point and at least one digit after

        if (std::isnan (v))
            return "0.0 / 0.0"; // NaN
        if (std::isinf (v))
            return v > 0 ? "1.0 / 0.0" : "-1.0 / 0.0"; // Inf

        std::ostringstream oss;
        oss << std::fixed << v;

        std::string s = oss.str();

        // Ensure it has a decimal point
        if (s.find ('.') == std::string::npos)
            s += ".0";

        return s;
    }

    bool isTextureSampleCall (const ExprFunCall& fc)
    {
        return fc.callee
            && fc.callee->is<ExprVariable>()
            && fc.callee->as<ExprVariable>().name == "texture"
            && fc.args.size() >= 1
            && fc.args[0].is<ExprVariable>()
            && isSamplerVariable (fc.args[0].as<ExprVariable>().name);
    }

    bool isSamplerVariable (const std::string& name)
    {
        for (auto& r : program.resources)
            if (r.name == name && ! r.isSampler && r.samplerBinding != ~0u)
                return true;

        // Also check global symbol table
        auto* info = symbolLookup (name);
        return info && isSamplerType (info->type.kind);
    }

    SymbolInfo* symbolLookup (const std::string& name)
    {
        // Simple symbol table lookup - we maintain our own for emission
        static std::map<std::string, SymbolInfo> emitterSymbols;

        auto it = emitterSymbols.find (name);
        if (it != emitterSymbols.end())
            return &it->second;

        return nullptr;
    }

    const LoweredProgram& program;
    const WgslEmitOptions& options;
};

} // namespace

//==============================================================================
// WgslEmitter::emit()
//==============================================================================

ResultValue<String> WgslEmitter::emit (const LoweredProgram& program,
                                       const WgslEmitOptions& options)
{
    try
    {
        Emitter emitter (program, options);
        std::string wgsl = emitter.emit();
        return makeResultValueOk (String (wgsl));
    }
    catch (const std::exception& e)
    {
        return makeResultValueFail (String ("WGSL emitter error: ") + e.what());
    }
}

} // namespace wgsl
} // namespace yup
