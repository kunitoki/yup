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

namespace
{

template <size_t N>
const char* getStringProperty (const var& object, const char (&propertyName)[N], const char* fallback = "")
{
    if (object.isObject() && object.hasProperty (Identifier (propertyName)) && object[propertyName].isString())
        return object[propertyName].toString().toRawUTF8();

    return fallback;
}

void addStringArrayProperty (const var& object, const char* propertyName, std::vector<String>& output)
{
    if (! object.isObject() || ! object.hasProperty (Identifier (propertyName)))
        return;

    const auto& value = object[propertyName];
    if (value.isArray())
    {
        for (const auto& item : *value.getArray())
        {
            if (item.isString())
                output.push_back (item.toString());
        }
    }
    else if (value.isString())
    {
        output.push_back (value.toString());
    }
}

void addStringArrayProperty (const var& object, const char* propertyName, std::unordered_set<String>& output)
{
    std::vector<String> values;
    addStringArrayProperty (object, propertyName, values);

    for (auto& value : values)
        output.insert (std::move (value));
}

bool getBoolProperty (const var& object, const char* propertyName, bool fallback)
{
    if (object.isObject() && object.hasProperty (Identifier (propertyName)) && object[propertyName].isBool())
        return static_cast<bool> (object[propertyName]);

    return fallback;
}

Result parseCharacterPairs (const var& object, const char* propertyName, std::vector<SyntaxDefinition::CharacterPair>& output)
{
    std::vector<String> values;
    addStringArrayProperty (object, propertyName, values);

    output.clear();

    for (const auto& value : values)
    {
        if (value.length() != 2)
            return Result::fail ("\"" + String (propertyName) + "\" entries must be two characters, got \"" + value + "\"");

        output.emplace_back (value[0], value[1]);
    }

    return Result::ok();
}

Result parseRule (const var& object, SyntaxDefinition::Rule& rule)
{
    if (! object.isObject())
        return Result::fail ("Syntax definition rules must be objects");

    rule.match = getStringProperty (object, "match");
    if (rule.match.isEmpty())
        return Result::fail ("Syntax definition rule is missing a \"match\"");

    if (const String scope = getStringProperty (object, "scope"); scope.isNotEmpty())
        rule.scope = Identifier (scope);

    rule.push = getStringProperty (object, "push");
    rule.pop = getBoolProperty (object, "pop", false);
    rule.skipWords = getBoolProperty (object, "skipWords", false);

    if (auto* captures = object["captures"].getDynamicObject())
    {
        for (const auto& capture : captures->getProperties())
        {
            const auto group = capture.name.toString().getIntValue();
            if (group > 0 && capture.value.isString() && capture.value.toString().isNotEmpty())
                rule.captures.emplace_back (group, Identifier (capture.value.toString()));
        }
    }

    return Result::ok();
}

bool referencesPushCaptures (const String& pattern)
{
    for (int i = 0; i + 1 < pattern.length(); ++i)
    {
        if (pattern[i] == '\\')
            ++i;
        else if (pattern[i] == '$' && pattern[i + 1] >= '1' && pattern[i + 1] <= '9')
            return true;
    }

    return false;
}

//==============================================================================

constexpr auto cppDefinitionJson = R"json({
    "name": "C++",
    "extensions": ["cpp", "cc", "cxx", "c", "h", "hpp", "hh", "hxx", "inl"],
    "lineComment": "//",
    "blockComment": { "start": "/*", "end": "*/" },
    "strings": { "delimiters": ["\"", "'"], "escape": "\\", "multiLine": false, "rawStrings": true, "rawStringPrefixes": ["R", "u8R", "uR", "UR", "LR"], "stringPrefixes": ["u8", "L", "u", "U"] },
    "preprocessor": "#",
    "numbers": { "hex": true, "binary": true, "float": true, "exponent": true, "suffix": true },
    "keywords": ["alignas", "alignof", "asm", "auto", "class", "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "decltype", "delete", "dynamic_cast", "enum", "explicit", "export", "extern", "final", "friend", "inline", "mutable", "namespace", "new", "noexcept", "operator", "override", "private", "protected", "public", "register", "reinterpret_cast", "requires", "sizeof", "static", "static_assert", "static_cast", "struct", "template", "thread_local", "typedef", "typeid", "typename", "union", "using", "virtual", "volatile"],
    "types": ["bool", "char", "char8_t", "char16_t", "char32_t", "double", "float", "int", "long", "short", "signed", "unsigned", "void", "wchar_t", "size_t", "ssize_t", "ptrdiff_t", "nullptr_t", "max_align_t", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t", "intmax_t", "uintmax_t"],
    "operators": ["::", "->*", "->", "<=>", "<<=", ">>=", "...", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", ".*", "##", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}", "+", "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^", "~", "#", "@"],
    "wordSets": {
        "keyword.control": ["break", "case", "catch", "continue", "co_await", "co_return", "co_yield", "default", "do", "else", "for", "goto", "if", "return", "switch", "throw", "try", "while"],
        "keyword.constant": ["true", "false", "nullptr", "this", "NULL"],
        "keyword.operator": ["and", "and_eq", "bitand", "bitor", "compl", "not", "not_eq", "or", "or_eq", "xor", "xor_eq"],
        "type.std": ["string", "string_view", "wstring", "u8string", "u16string", "u32string", "vector", "array", "deque", "list", "forward_list", "map", "multimap", "set", "multiset", "unordered_map", "unordered_set", "unordered_multimap", "unordered_multiset", "unique_ptr", "shared_ptr", "weak_ptr", "optional", "variant", "any", "tuple", "pair", "function", "byte", "atomic", "mutex", "recursive_mutex", "shared_mutex", "lock_guard", "unique_lock", "scoped_lock", "condition_variable", "thread", "jthread", "future", "promise", "ostream", "istream", "iostream", "stringstream", "ostringstream", "istringstream", "initializer_list", "span", "bitset", "chrono"]
    },
    "states": {
        "root": { "rules": [
            { "match": "\\[\\[\\s*[A-Za-z_][^\\]]*\\]\\]", "scope": "keyword.attribute" },
            { "match": "(?:0[xX][0-9A-Fa-f]+(?:'[0-9A-Fa-f]+)+|0[bB][01]+(?:'[01]+)+|\\d+(?:'\\d+)+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?)[A-Za-z_]*", "scope": "number" },
            { "match": "\\b(class|struct|union|enum(?:\\s+(?:class|struct))?|concept)(\\s+)(?:([A-Z_][A-Z0-9_]*_API)(\\s+))?([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "identifier.macro", "5": "type.class" } },
            { "match": "\\b(namespace)(\\s+)([A-Za-z_][\\w:]*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "type.namespace" } },
            { "match": "[A-Za-z_]\\w*(?=\\s*::)", "scope": "type.namespace", "skipWords": true },
            { "match": "(\\.|->)([A-Za-z_]\\w*)(?=\\s*\\()", "captures": { "1": "operator", "2": "identifier.function" } },
            { "match": "(\\.|->)([A-Za-z_]\\w*)\\b(?!\\s*::)", "captures": { "1": "operator", "2": "identifier.member" } },
            { "match": "[A-Z][A-Z0-9_]*[A-Z0-9_](?=\\s*\\()", "scope": "identifier.macro", "skipWords": true },
            { "match": "[A-Z][A-Z0-9_]*[A-Z0-9_]\\b(?![\\\"'])", "scope": "identifier.constant", "skipWords": true },
            { "match": "[A-Z][A-Za-z0-9_]*[a-z][A-Za-z0-9_]*\\b(?![\\\"'])", "scope": "type.class", "skipWords": true },
            { "match": "[A-Za-z_]\\w*(?=\\s*\\()", "scope": "identifier.function", "skipWords": true }
        ] },
        "commentContent": { "rules": [
            { "match": "[@\\\\](?:param|tparam|returns?|brief|details|note|warning|see|throws?|code|endcode|deprecated|since|pre|post|todo|file|retval)\\b", "scope": "comment.doc.tag" },
            { "match": "\\b(?:TODO|FIXME|XXX|HACK|NOTE|BUG)\\b:?", "scope": "comment.todo" },
            { "match": "https?://[^\\s)>\\]\\\"']+", "scope": "comment.link" }
        ] },
        "stringContent": { "rules": [
            { "match": "\\\\(?:x[0-9A-Fa-f]+|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8}|[0-7]{1,3}|.)", "scope": "string.escape" },
            { "match": "%%|%[-+#0]*(?:\\d+|\\*)?(?:\\.(?:\\d+|\\*))?(?:hh|h|ll|l|j|z|t|L)?[diouxXeEfFgGaAcspn]", "scope": "string.format" },
            { "match": "\\{\\d*(?::[^{}\\\"]*)?\\}", "scope": "string.format" }
        ] },
        "preprocessorContent": { "rules": [
            { "match": "^(\\s*#\\s*)(include|include_next|import)(\\s*)(<[^>]*>|\\\"[^\\\"]*\\\")", "captures": { "2": "keyword.directive", "4": "string.include" } },
            { "match": "^(\\s*#\\s*)(define|undef|ifdef|ifndef)(\\s+)([A-Za-z_]\\w*)", "captures": { "2": "keyword.directive", "4": "identifier.macro" } },
            { "match": "^(\\s*#\\s*)([A-Za-z_]\\w*)", "captures": { "2": "keyword.directive" } },
            { "match": "\\bdefined\\b", "scope": "keyword" },
            { "match": "\"(?:[^\"\\\\]|\\\\.)*\"?", "scope": "string" },
            { "match": "'(?:[^'\\\\]|\\\\.)*'?", "scope": "string" },
            { "match": "//.*", "scope": "comment" },
            { "match": "/\\*.*?\\*/", "scope": "comment" },
            { "match": "\\b(?:0[xX][0-9A-Fa-f']+|\\d[\\d']*(?:\\.\\d*)?(?:[eE][+-]?\\d+)?)[uUlLfF]*\\b", "scope": "number" }
        ] }
    },
    "injections": { "comment": "commentContent", "string": "stringContent", "preprocessor": "preprocessorContent" }
})json";

constexpr auto glslDefinitionJson = R"json({
    "name": "GLSL",
    "extensions": ["glsl", "vert", "frag", "geom", "comp", "tesc", "tese", "rgen", "rchit", "rmiss"],
    "lineComment": "//",
    "blockComment": { "start": "/*", "end": "*/" },
    "strings": { "delimiters": ["\""], "escape": "\\", "multiLine": false },
    "preprocessor": "#",
    "numbers": { "hex": true, "binary": false, "float": true, "exponent": true, "suffix": true },
    "keywords": ["attribute", "const", "uniform", "varying", "buffer", "shared", "coherent", "volatile", "restrict", "readonly", "writeonly", "layout", "centroid", "flat", "smooth", "noperspective", "patch", "sample", "subroutine", "in", "out", "inout", "invariant", "precise", "struct", "highp", "mediump", "lowp", "precision"],
    "types": ["void", "bool", "int", "uint", "float", "double", "atomic_uint", "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "uvec2", "uvec3", "uvec4", "bvec2", "bvec3", "bvec4", "dvec2", "dvec3", "dvec4", "mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4", "dmat2", "dmat3", "dmat4", "dmat2x2", "dmat2x3", "dmat2x4", "dmat3x2", "dmat3x3", "dmat3x4", "dmat4x2", "dmat4x3", "dmat4x4", "sampler1D", "sampler2D", "sampler3D", "samplerCube", "sampler2DRect", "sampler1DArray", "sampler2DArray", "samplerCubeArray", "samplerBuffer", "sampler2DMS", "sampler2DMSArray", "sampler1DShadow", "sampler2DShadow", "samplerCubeShadow", "sampler2DRectShadow", "sampler1DArrayShadow", "sampler2DArrayShadow", "samplerCubeArrayShadow", "isampler1D", "isampler2D", "isampler3D", "isamplerCube", "isampler2DRect", "isampler1DArray", "isampler2DArray", "isamplerCubeArray", "isamplerBuffer", "isampler2DMS", "isampler2DMSArray", "usampler1D", "usampler2D", "usampler3D", "usamplerCube", "usampler2DRect", "usampler1DArray", "usampler2DArray", "usamplerCubeArray", "usamplerBuffer", "usampler2DMS", "usampler2DMSArray", "image1D", "image2D", "image3D", "imageCube", "image2DRect", "image1DArray", "image2DArray", "imageCubeArray", "imageBuffer", "image2DMS", "image2DMSArray", "iimage1D", "iimage2D", "iimage3D", "iimageCube", "iimage2DArray", "iimageBuffer", "uimage1D", "uimage2D", "uimage3D", "uimageCube", "uimage2DArray", "uimageBuffer", "sampler", "samplerShadow", "texture1D", "texture2D", "texture3D", "textureCube", "texture2DArray", "subpassInput"],
    "operators": ["++", "--", "<<=", ">>=", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "^^", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}", "+", "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^", "~", "#"],
    "wordSets": {
        "keyword.control": ["break", "continue", "do", "for", "while", "switch", "case", "default", "if", "else", "discard", "return", "demote", "terminateInvocation"],
        "keyword.constant": ["true", "false"],
        "identifier.builtin": ["radians", "degrees", "sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh", "pow", "exp", "log", "exp2", "log2", "sqrt", "inversesqrt", "abs", "sign", "floor", "trunc", "round", "roundEven", "ceil", "fract", "mod", "modf", "min", "max", "clamp", "mix", "step", "smoothstep", "isnan", "isinf", "fma", "frexp", "ldexp", "floatBitsToInt", "floatBitsToUint", "intBitsToFloat", "uintBitsToFloat", "packUnorm2x16", "packSnorm2x16", "packUnorm4x8", "packSnorm4x8", "unpackUnorm2x16", "unpackSnorm2x16", "unpackUnorm4x8", "unpackSnorm4x8", "packHalf2x16", "unpackHalf2x16", "packDouble2x32", "unpackDouble2x32", "length", "distance", "dot", "cross", "normalize", "faceforward", "reflect", "refract", "matrixCompMult", "outerProduct", "transpose", "determinant", "inverse", "lessThan", "lessThanEqual", "greaterThan", "greaterThanEqual", "equal", "notEqual", "any", "all", "not", "uaddCarry", "usubBorrow", "umulExtended", "imulExtended", "bitfieldExtract", "bitfieldInsert", "bitfieldReverse", "bitCount", "findLSB", "findMSB", "textureSize", "textureQueryLod", "textureQueryLevels", "textureSamples", "texture", "textureProj", "textureLod", "textureOffset", "texelFetch", "texelFetchOffset", "textureProjOffset", "textureLodOffset", "textureProjLod", "textureProjLodOffset", "textureGrad", "textureGradOffset", "textureProjGrad", "textureProjGradOffset", "textureGather", "textureGatherOffset", "textureGatherOffsets", "dFdx", "dFdy", "dFdxFine", "dFdyFine", "dFdxCoarse", "dFdyCoarse", "fwidth", "fwidthFine", "fwidthCoarse", "interpolateAtCentroid", "interpolateAtSample", "interpolateAtOffset", "atomicCounterIncrement", "atomicCounterDecrement", "atomicCounter", "atomicAdd", "atomicMin", "atomicMax", "atomicAnd", "atomicOr", "atomicXor", "atomicExchange", "atomicCompSwap", "imageSize", "imageSamples", "imageLoad", "imageStore", "imageAtomicAdd", "imageAtomicMin", "imageAtomicMax", "imageAtomicAnd", "imageAtomicOr", "imageAtomicXor", "imageAtomicExchange", "imageAtomicCompSwap", "barrier", "memoryBarrier", "memoryBarrierAtomicCounter", "memoryBarrierBuffer", "memoryBarrierShared", "memoryBarrierImage", "groupMemoryBarrier", "EmitVertex", "EndPrimitive", "EmitStreamVertex", "EndStreamPrimitive"],
        "identifier.property": ["location", "binding", "set", "offset", "component", "index", "std140", "std430", "packed", "push_constant", "local_size_x", "local_size_y", "local_size_z", "origin_upper_left", "pixel_center_integer", "early_fragment_tests", "input_attachment_index"]
    },
    "states": {
        "root": { "rules": [
            { "match": "gl_\\w+", "scope": "identifier.builtin" },
            { "match": "\\b(struct)(\\s+)([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "type.class" } },
            { "match": "(\\.|->)([A-Za-z_]\\w*)(?=\\s*\\()", "captures": { "1": "operator", "2": "identifier.function" } },
            { "match": "(\\.|->)([A-Za-z_]\\w*)\\b(?!\\s*::)", "captures": { "1": "operator", "2": "identifier.member" } },
            { "match": "[A-Z][A-Z0-9_]*[A-Z0-9_](?=\\s*\\()", "scope": "identifier.macro", "skipWords": true },
            { "match": "[A-Z][A-Z0-9_]*[A-Z0-9_]\\b(?![\\\"'])", "scope": "identifier.constant", "skipWords": true },
            { "match": "[A-Z][A-Za-z0-9_]*[a-z][A-Za-z0-9_]*\\b(?![\\\"'])", "scope": "type.class", "skipWords": true },
            { "match": "[A-Za-z_]\\w*(?=\\s*\\()", "scope": "identifier.function", "skipWords": true }
        ] },
        "commentContent": { "rules": [
            { "match": "[@\\\\](?:param|tparam|returns?|brief|details|note|warning|see|throws?|code|endcode|deprecated|since|pre|post|todo|file|retval)\\b", "scope": "comment.doc.tag" },
            { "match": "\\b(?:TODO|FIXME|XXX|HACK|NOTE|BUG)\\b:?", "scope": "comment.todo" },
            { "match": "https?://[^\\s)>\\]\\\"']+", "scope": "comment.link" }
        ] },
        "stringContent": { "rules": [
            { "match": "\\\\(?:x[0-9A-Fa-f]+|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8}|[0-7]{1,3}|.)", "scope": "string.escape" }
        ] },
        "preprocessorContent": { "rules": [
            { "match": "^(\\s*#\\s*)(include|include_next|import)(\\s*)(<[^>]*>|\\\"[^\\\"]*\\\")", "captures": { "2": "keyword.directive", "4": "string.include" } },
            { "match": "^(\\s*#\\s*)(define|undef|ifdef|ifndef)(\\s+)([A-Za-z_]\\w*)", "captures": { "2": "keyword.directive", "4": "identifier.macro" } },
            { "match": "^(\\s*#\\s*)([A-Za-z_]\\w*)", "captures": { "2": "keyword.directive" } },
            { "match": "\\bdefined\\b", "scope": "keyword" },
            { "match": "\"(?:[^\"\\\\]|\\\\.)*\"?", "scope": "string" },
            { "match": "'(?:[^'\\\\]|\\\\.)*'?", "scope": "string" },
            { "match": "//.*", "scope": "comment" },
            { "match": "/\\*.*?\\*/", "scope": "comment" },
            { "match": "\\b(?:0[xX][0-9A-Fa-f']+|\\d[\\d']*(?:\\.\\d*)?(?:[eE][+-]?\\d+)?)[uUlLfF]*\\b", "scope": "number" }
        ] }
    },
    "injections": { "comment": "commentContent", "string": "stringContent", "preprocessor": "preprocessorContent" }
})json";

constexpr auto pythonDefinitionJson = R"json({
    "name": "Python",
    "extensions": ["py", "pyw", "pyi"],
    "lineComment": "#",
    "strings": { "delimiters": ["\"", "'"], "multiLineDelimiters": ["\"\"\"", "'''"], "escape": "\\", "multiLine": true, "stringPrefixes": ["f", "F", "r", "R", "b", "B", "u", "U", "fr", "Fr", "fR", "FR", "rf", "rF", "Rf", "RF", "br", "Br", "bR", "BR", "rb", "rB", "Rb", "RB"] },
    "numbers": { "hex": true, "binary": true, "float": true, "exponent": true, "suffix": false },
    "keywords": ["as", "assert", "class", "def", "del", "from", "global", "import", "lambda", "nonlocal"],
    "types": ["int", "float", "complex", "bool", "str", "bytes", "bytearray", "memoryview", "list", "tuple", "dict", "set", "frozenset", "object", "range", "slice", "type"],
    "operators": ["**=", "//=", ">>=", "<<=", "**", "//", "<<", ">>", "<=", ">=", "==", "!=", "+=", "-=", "*=", "/=", "%=", "@=", "&=", "|=", "^=", ":=", "->", "...", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}", "+", "-", "*", "/", "%", "@", "=", "<", ">", "!", "&", "|", "^", "~"],
    "increaseIndentPattern": ":\\s*(#.*)?$",
    "wordSets": {
        "keyword.control": ["async", "await", "break", "continue", "elif", "else", "except", "finally", "for", "if", "pass", "raise", "return", "try", "while", "with", "yield"],
        "keyword.constant": ["True", "False", "None", "Ellipsis", "NotImplemented", "__debug__"],
        "keyword.operator": ["and", "or", "not", "in", "is"],
        "identifier.language": ["self", "cls"],
        "identifier.builtin": ["abs", "aiter", "all", "anext", "any", "ascii", "bin", "breakpoint", "callable", "chr", "classmethod", "compile", "delattr", "dir", "divmod", "enumerate", "eval", "exec", "filter", "format", "getattr", "globals", "hasattr", "hash", "help", "hex", "id", "input", "isinstance", "issubclass", "iter", "len", "locals", "map", "max", "min", "next", "oct", "open", "ord", "pow", "print", "property", "repr", "reversed", "round", "setattr", "sorted", "staticmethod", "sum", "super", "vars", "zip", "__import__"],
        "type.exception": ["BaseException", "Exception", "BaseExceptionGroup", "ExceptionGroup", "ArithmeticError", "AssertionError", "AttributeError", "BlockingIOError", "BrokenPipeError", "BufferError", "ChildProcessError", "ConnectionError", "ConnectionAbortedError", "ConnectionRefusedError", "ConnectionResetError", "EOFError", "FileExistsError", "FileNotFoundError", "FloatingPointError", "GeneratorExit", "ImportError", "IndentationError", "IndexError", "InterruptedError", "IsADirectoryError", "KeyError", "KeyboardInterrupt", "LookupError", "MemoryError", "ModuleNotFoundError", "NameError", "NotADirectoryError", "NotImplementedError", "OSError", "OverflowError", "PermissionError", "ProcessLookupError", "RecursionError", "ReferenceError", "RuntimeError", "StopAsyncIteration", "StopIteration", "SyntaxError", "SystemError", "SystemExit", "TabError", "TimeoutError", "TypeError", "UnboundLocalError", "UnicodeDecodeError", "UnicodeEncodeError", "UnicodeError", "ValueError", "ZeroDivisionError", "Warning", "BytesWarning", "DeprecationWarning", "FutureWarning", "ImportWarning", "PendingDeprecationWarning", "ResourceWarning", "RuntimeWarning", "SyntaxWarning", "UnicodeWarning", "UserWarning"]
    },
    "states": {
        "root": { "rules": [
            { "match": "^(\\s*)(@[A-Za-z_][\\w.]*)", "scope": "whitespace", "captures": { "2": "preprocessor.decorator" } },
            { "match": "\\b(def)(\\s+)([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "identifier.function" } },
            { "match": "\\b(class)(\\s+)([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "type.class" } },
            { "match": "(?:0[xX][0-9A-Fa-f_]+|0[oO][0-7_]+|0[bB][01_]+|\\d[\\d_]*(?:\\.[\\d_]*)?(?:[eE][+-]?\\d[\\d_]*)?[jJ]?|\\.\\d[\\d_]*(?:[eE][+-]?\\d[\\d_]*)?[jJ]?)", "scope": "number" },
            { "match": "__\\w+__\\b", "scope": "identifier.builtin", "skipWords": true },
            { "match": "(\\.)([A-Za-z_]\\w*)(?=\\s*\\()", "captures": { "1": "operator", "2": "identifier.function" } },
            { "match": "(\\.)([A-Za-z_]\\w*)", "captures": { "1": "operator", "2": "identifier.member" } },
            { "match": "[A-Z][A-Z0-9_]*[A-Z0-9_]\\b(?![\\\"'])", "scope": "identifier.constant", "skipWords": true },
            { "match": "[A-Z][A-Za-z0-9_]*[a-z][A-Za-z0-9_]*\\b(?![\\\"'])", "scope": "type.class", "skipWords": true },
            { "match": "[A-Za-z_]\\w*(?=\\s*\\()", "scope": "identifier.function", "skipWords": true }
        ] },
        "commentContent": { "rules": [
            { "match": "[@\\\\](?:param|tparam|returns?|brief|details|note|warning|see|throws?|code|endcode|deprecated|since|pre|post|todo|file|retval)\\b", "scope": "comment.doc.tag" },
            { "match": "\\b(?:TODO|FIXME|XXX|HACK|NOTE|BUG)\\b:?", "scope": "comment.todo" },
            { "match": "https?://[^\\s)>\\]\\\"']+", "scope": "comment.link" }
        ] },
        "stringContent": { "rules": [
            { "match": "\\\\(?:N\\{[^}]*\\}|x[0-9A-Fa-f]{2}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8}|[0-7]{1,3}|.)", "scope": "string.escape" },
            { "match": "\\{\\{|\\}\\}", "scope": "string.escape" },
            { "match": "\\{[^{}\\n]*\\}", "scope": "string.interpolation" },
            { "match": "%%|%(?:\\([A-Za-z_]\\w*\\))?[-+#0]*(?:\\d+|\\*)?(?:\\.(?:\\d+|\\*))?[diouxXeEfFgGcrsa]", "scope": "string.format" }
        ] }
    },
    "injections": { "comment": "commentContent", "string": "stringContent" }
})json";

constexpr auto ydspDefinitionJson = R"json({
    "name": "YDSP",
    "extensions": ["ydsp"],
    "lineComment": "//",
    "blockComment": { "start": "/*", "end": "*/" },
    "strings": { "delimiters": ["\""], "escape": "\\", "multiLine": false },
    "numbers": { "hex": true, "binary": true, "float": true, "exponent": true, "suffix": false },
    "keywords": ["processor", "graph", "node", "connection", "input", "output", "parameter", "stream", "state", "process", "block", "let", "declare", "func", "import", "as", "struct", "init", "event"],
    "types": ["float", "float32", "float64", "int", "int32", "int64", "bool"],
    "operators": ["<<=", ">>=", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "&=", "|=", "^=", "->", "=>", "<:", ":>", "..", "[[", "]]", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}", "+", "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^", "~", "@", "'"],
    "wordSets": {
        "keyword.control": ["for", "if", "match", "else", "return", "emit"],
        "keyword.constant": ["true", "false", "pi"],
        "keyword.operator": ["_"],
        "identifier.language": ["in", "out", "sampleRate", "blockSize"],
        "identifier.builtin": ["abs", "sqrt", "min", "max", "fmod", "floor", "ceil", "rint", "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh", "asinh", "acosh", "atanh", "exp", "log", "log10", "pow", "round", "copysign", "clamp", "sign", "lerp", "fma", "select", "smooth", "mem", "size"],
        "type.event": ["noteOn", "noteOff", "pitchBend", "pressure", "slide", "controlChange", "programChange"]
    },
    "states": {
        "root": { "rules": [
            { "match": "\\[\\[", "scope": "operator", "push": "annotation" },
            { "match": "\\d[\\d_]*_[\\d_]*(?:\\.[\\d_]+)?(?:[eE][+-]?\\d+)?", "scope": "number" },
            { "match": "\\b(processor|graph|struct)(\\s+)([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "type.class" } },
            { "match": "\\b(func)(\\s+)([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "identifier.function" } },
            { "match": "\\b(declare)(\\s+)([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "identifier.property" } },
            { "match": "\\b(import|as)(\\s+)([A-Za-z_][\\w.]*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "type.namespace" } },
            { "match": "(\\.)([A-Za-z_]\\w*)(?=\\s*\\()", "captures": { "1": "operator", "2": "identifier.function" } },
            { "match": "(\\.)([A-Za-z_]\\w*)", "captures": { "1": "operator", "2": "identifier.member" } },
            { "match": "[A-Z][A-Z0-9_]*[A-Z0-9_]\\b(?![\\\"'])", "scope": "identifier.constant", "skipWords": true },
            { "match": "[A-Z][A-Za-z0-9_]*[a-z][A-Za-z0-9_]*\\b(?![\\\"'])", "scope": "type.class", "skipWords": true },
            { "match": "[A-Za-z_]\\w*(?=\\s*\\()", "scope": "identifier.function", "skipWords": true }
        ] },
        "annotation": { "scope": "other", "rules": [
            { "match": "\\]\\]", "scope": "operator", "pop": true },
            { "match": "[A-Za-z_]\\w*(?=\\s*:)", "scope": "identifier.property" },
            { "match": "\"(?:[^\"\\\\]|\\\\.)*\"?", "scope": "string" },
            { "match": "\\d+(?:\\.\\d+)?(?:[eE][+-]?\\d+)?", "scope": "number" },
            { "match": "\\b(?:true|false)\\b", "scope": "keyword.constant" },
            { "match": "[A-Za-z_]\\w*", "scope": "identifier" },
            { "match": "\\s+", "scope": "whitespace" },
            { "match": "[:,{}()\\[\\]+\\-.]", "scope": "operator" }
        ] },
        "commentContent": { "rules": [
            { "match": "[@\\\\](?:param|tparam|returns?|brief|details|note|warning|see|throws?|code|endcode|deprecated|since|pre|post|todo|file|retval)\\b", "scope": "comment.doc.tag" },
            { "match": "\\b(?:TODO|FIXME|XXX|HACK|NOTE|BUG)\\b:?", "scope": "comment.todo" },
            { "match": "https?://[^\\s)>\\]\\\"']+", "scope": "comment.link" }
        ] },
        "stringContent": { "rules": [
            { "match": "\\\\(?:x[0-9A-Fa-f]+|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8}|[0-7]{1,3}|.)", "scope": "string.escape" },
            { "match": "%%|%[-+#0]*(?:\\d+|\\*)?(?:\\.(?:\\d+|\\*))?(?:hh|h|ll|l|j|z|t|L)?[diouxXeEfFgGaAcspn]", "scope": "string.format" }
        ] }
    },
    "injections": { "comment": "commentContent", "string": "stringContent" }
})json";

constexpr auto yuiDefinitionJson = R"json({
    "name": "YUI",
    "extensions": ["yui"],
    "lineComment": "//",
    "blockComment": { "start": "/*", "end": "*/" },
    "strings": { "delimiters": ["\""], "escape": "\\", "multiLine": false },
    "numbers": { "hex": false, "binary": false, "float": true, "exponent": true, "suffix": false },
    "keywords": ["import", "tokens", "drawing", "palette", "style", "component", "property", "alias", "signal", "let", "repeat", "in", "changed"],
    "types": ["number", "integer", "boolean", "string", "color", "point", "rectangle", "transform", "font", "any", "bool", "int", "float", "double"],
    "operators": ["<=>", "=>", "..", "&&", "||", "==", "!=", "<=", ">=", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}", "+", "-", "*", "/", "%", "<", ">", "!", "=", "#", "@"],
    "wordSets": {
        "keyword.constant": ["true", "false", "pi", "tau"],
        "type.shape": ["rect", "circle", "ellipse", "arc", "line", "path", "polygon", "star", "text", "image", "svg", "layer", "group", "clip", "mask", "viewport", "clipPath", "filter", "blur", "flood", "colorMatrix", "composite", "morphology", "lighting", "displacement", "dropShadow"],
        "identifier.language": ["self", "parent", "root", "theme"],
        "identifier.builtin": ["sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sqrt", "pow", "exp", "log", "log2", "log10", "abs", "floor", "ceil", "round", "trunc", "sign", "fmod", "min", "max", "clamp", "lerp", "mix", "map", "smoothstep", "hypot", "deg", "rad", "len", "format", "rgb", "rgba", "hsl", "hsla", "polar", "linearGradient", "radialGradient", "angularGradient", "action"]
    },
    "states": {
        "root": { "rules": [
            { "match": "#(?:[0-9A-Fa-f]{8}|[0-9A-Fa-f]{6}|[0-9A-Fa-f]{3,4})\\b", "scope": "number.color" },
            { "match": "#[A-Za-z_][\\w-]*", "scope": "type.selector" },
            { "match": "\\.[A-Za-z_][\\w-]*(?=[^;{}()]*(?:\\{|,\\s*(?://.*)?$))", "scope": "type.selector" },
            { "match": ":[A-Za-z][\\w-]*(?=[^;{}()]*(?:\\{|,\\s*(?://.*)?$))", "scope": "type.selector" },
            { "match": "@[A-Za-z][\\w-]*", "scope": "keyword" },
            { "match": "\\b(drawing|palette|component|pattern|marker|use)(\\s+)([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "type.class" } },
            { "match": "\\b(property)(\\s+)(?:(alias)(\\s+))?([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "keyword", "5": "identifier.property" } },
            { "match": "\\b(signal)(\\s+)([A-Za-z_]\\w*)", "scope": "whitespace", "captures": { "1": "keyword", "3": "identifier.function" } },
            { "match": "(\\d+(?:\\.\\d+)?)(deg|rad|grad|turn|ms|px|s|%)(?![\\w(])", "scope": "number", "captures": { "2": "number.unit" } },
            { "match": "^(\\s*)([A-Za-z_][\\w-]*)(?=\\s*:(?![\\w-]+[^;{}()]*(?:\\{|,\\s*(?://.*)?$)))", "scope": "whitespace", "captures": { "2": "identifier.property" } },
            { "match": "([{;])(\\s*)([A-Za-z_][\\w-]*)(?=\\s*:(?![\\w-]+[^;{}()]*(?:\\{|,\\s*(?://.*)?$)))", "scope": "whitespace", "captures": { "1": "operator", "3": "identifier.property" } },
            { "match": "(\\.)([A-Za-z_]\\w*)(?=\\s*\\()", "captures": { "1": "operator", "2": "identifier.function" } },
            { "match": "(\\.)([A-Za-z_]\\w*)", "captures": { "1": "operator", "2": "identifier.member" } },
            { "match": "\\b(?:hue-rotate|drop-shadow)(?=\\s*\\()", "scope": "identifier.function" },
            { "match": "[A-Z][A-Z0-9_]*[A-Z0-9_]\\b(?![\\\"'])", "scope": "identifier.constant", "skipWords": true },
            { "match": "[A-Z][A-Za-z0-9_]*[a-z][A-Za-z0-9_]*\\b(?![\\\"'])", "scope": "type.class", "skipWords": true },
            { "match": "[A-Za-z_]\\w*(?=\\s*\\()", "scope": "identifier.function", "skipWords": true }
        ] },
        "commentContent": { "rules": [
            { "match": "[@\\\\](?:param|tparam|returns?|brief|details|note|warning|see|throws?|code|endcode|deprecated|since|pre|post|todo|file|retval)\\b", "scope": "comment.doc.tag" },
            { "match": "\\b(?:TODO|FIXME|XXX|HACK|NOTE|BUG)\\b:?", "scope": "comment.todo" },
            { "match": "https?://[^\\s)>\\]\\\"']+", "scope": "comment.link" }
        ] },
        "stringContent": { "rules": [
            { "match": "\\\\(?:x[0-9A-Fa-f]+|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8}|[0-7]{1,3}|.)", "scope": "string.escape" },
            { "match": "%%|%[-+#0]*(?:\\d+|\\*)?(?:\\.(?:\\d+|\\*))?(?:hh|h|ll|l|j|z|t|L)?[diouxXeEfFgGaAcspn]", "scope": "string.format" }
        ] }
    },
    "injections": { "comment": "commentContent", "string": "stringContent" }
})json";

constexpr auto xmlDefinitionJson = R"json({
    "name": "XML",
    "extensions": ["xml", "xaml", "svg", "html", "htm", "xhtml", "xsd", "xsl", "xslt", "plist", "resx", "csproj", "vcxproj"],
    "lineComment": "",
    "blockComment": { "start": "<!--", "end": "-->" },
    "strings": { "delimiters": ["\"", "'"], "multiLine": false },
    "numbers": { "hex": false, "binary": false, "float": true, "exponent": false, "suffix": false },
    "keywords": ["xml", "DOCTYPE", "ELEMENT", "ATTLIST", "ENTITY", "NOTATION", "CDATA", "PUBLIC", "SYSTEM"],
    "operators": ["]]>", "<!", "<?", "?>", "</", "/>", "<", ">", "=", ":", "&", ";", "!", "?", "/", "-", ".", ","],
    "states": {
        "root": { "rules": [
            { "match": "<!\\[CDATA\\[", "scope": "keyword", "push": "cdata" },
            { "match": "(<\\?)([A-Za-z_][\\w:.-]*)", "captures": { "1": "operator", "2": "keyword.tag" }, "push": "tag" },
            { "match": "(<!)([A-Za-z]+)", "captures": { "1": "operator", "2": "keyword" }, "push": "declaration" },
            { "match": "(</?)([A-Za-z_][\\w:.-]*)", "captures": { "1": "operator", "2": "keyword.tag" }, "push": "tag" },
            { "match": "&(?:[A-Za-z_][\\w.-]*|#\\d+|#x[0-9A-Fa-f]+);", "scope": "string.escape" },
            { "match": "[^<&\\s]+", "scope": "identifier.text" }
        ] },
        "tag": { "rules": [
            { "match": "\\?>|/?>", "scope": "operator", "pop": true },
            { "match": "\\s+", "scope": "whitespace" },
            { "match": "[A-Za-z_][\\w:.-]*", "scope": "type.attribute" },
            { "match": "=", "scope": "operator" },
            { "match": "\"[^\"]*\"?", "scope": "string" },
            { "match": "'[^']*'?", "scope": "string" }
        ] },
        "declaration": { "scope": "other", "rules": [
            { "match": ">", "scope": "operator", "pop": true },
            { "match": "\\b[A-Z]+\\b", "scope": "keyword" },
            { "match": "\"[^\"]*\"?", "scope": "string" },
            { "match": "'[^']*'?", "scope": "string" },
            { "match": "\\s+", "scope": "whitespace" }
        ] },
        "cdata": { "scope": "string.cdata", "rules": [
            { "match": "\\]\\]>", "scope": "keyword", "pop": true }
        ] },
        "commentContent": { "rules": [
            { "match": "[@\\\\](?:param|tparam|returns?|brief|details|note|warning|see|throws?|code|endcode|deprecated|since|pre|post|todo|file|retval)\\b", "scope": "comment.doc.tag" },
            { "match": "\\b(?:TODO|FIXME|XXX|HACK|NOTE|BUG)\\b:?", "scope": "comment.todo" },
            { "match": "https?://[^\\s)>\\]\\\"']+", "scope": "comment.link" }
        ] },
        "stringContent": { "rules": [
            { "match": "&(?:[A-Za-z_][\\w.-]*|#\\d+|#x[0-9A-Fa-f]+);", "scope": "string.escape" }
        ] }
    },
    "injections": { "comment": "commentContent", "string": "stringContent" }
})json";

SyntaxDefinition parseBuiltIn (const char* jsonText)
{
    SyntaxDefinition definition;
    const auto result = definition.loadFromData (jsonText);
    jassert (result.wasOk());

    return definition;
}

} // namespace

//==============================================================================

Result SyntaxDefinition::loadFromFile (const File& file)
{
    auto stream = file.createInputStream();
    if (stream == nullptr || ! stream->openedOk())
        return Result::fail ("Unable to open syntax definition file");

    return loadFromData (stream->readEntireStreamAsString());
}

Result SyntaxDefinition::loadFromData (StringRef jsonText)
{
    var parsed;
    if (auto result = JSON::parse (String (jsonText), parsed); result.failed())
        return result;

    if (! parsed.isObject())
        return Result::fail ("Syntax definition JSON must be an object");

    name = getStringProperty (parsed, "name");
    if (name.isEmpty())
        return Result::fail ("Syntax definition is missing a \"name\"");

    addStringArrayProperty (parsed, "extensions", fileExtensions);

    lineCommentPrefix = getStringProperty (parsed, "lineComment");

    if (parsed.hasProperty (Identifier ("blockComment")) && parsed["blockComment"].isObject())
    {
        Delimiters delimiters;
        delimiters.start = getStringProperty (parsed["blockComment"], "start");
        delimiters.end = getStringProperty (parsed["blockComment"], "end");
        blockComment = delimiters;
    }

    if (parsed.hasProperty (Identifier ("strings")) && parsed["strings"].isObject())
    {
        const auto& strings = parsed["strings"];

        addStringArrayProperty (strings, "delimiters", stringDelimiters);
        addStringArrayProperty (strings, "multiLineDelimiters", multiLineStringDelimiters);

        const auto escape = getStringProperty (strings, "escape", "");
        if (escape[0] != 0)
            escapeCharacter = static_cast<yup_wchar> (escape[0]);

        multiLineStrings = getBoolProperty (strings, "multiLine", false);
        rawStrings = getBoolProperty (strings, "rawStrings", false);
        addStringArrayProperty (strings, "rawStringPrefixes", rawStringPrefixes);
        addStringArrayProperty (strings, "stringPrefixes", stringPrefixes);
    }

    preprocessorPrefix = getStringProperty (parsed, "preprocessor");

    if (parsed.hasProperty (Identifier ("numbers")) && parsed["numbers"].isObject())
    {
        const auto& numbers = parsed["numbers"];

        allowHex = getBoolProperty (numbers, "hex", true);
        allowBinary = getBoolProperty (numbers, "binary", true);
        allowFloat = getBoolProperty (numbers, "float", true);
        allowExponent = getBoolProperty (numbers, "exponent", true);
        allowSuffix = getBoolProperty (numbers, "suffix", true);
    }

    addStringArrayProperty (parsed, "keywords", keywords);
    addStringArrayProperty (parsed, "types", types);
    addStringArrayProperty (parsed, "operators", operators);

    if (auto* wordSets = parsed["wordSets"].getDynamicObject())
    {
        for (const auto& wordSet : wordSets->getProperties())
        {
            std::vector<String> words;
            addStringArrayProperty (parsed["wordSets"], wordSet.name.toString().toRawUTF8(), words);

            for (auto& word : words)
                wordScopes[std::move (word)] = wordSet.name;
        }
    }

    if (parsed.hasProperty (Identifier ("brackets")))
    {
        if (auto result = parseCharacterPairs (parsed, "brackets", bracketPairs); result.failed())
            return result;
    }

    if (parsed.hasProperty (Identifier ("autoClosingPairs")))
    {
        if (auto result = parseCharacterPairs (parsed, "autoClosingPairs", autoClosingPairs); result.failed())
            return result;
    }
    else
    {
        autoClosingPairs = bracketPairs;

        for (const auto& delimiter : stringDelimiters)
            if (delimiter.length() == 1)
                autoClosingPairs.emplace_back (delimiter[0], delimiter[0]);
    }

    if (parsed.hasProperty (Identifier ("wordSeparators")))
        wordSeparators = getStringProperty (parsed, "wordSeparators");

    if (const String pattern = getStringProperty (parsed, "increaseIndentPattern"); pattern.isNotEmpty())
    {
        auto compiled = CodeRegex::compile (pattern);
        if (compiled.failed())
            return Result::fail ("Invalid \"increaseIndentPattern\": " + compiled.getErrorMessage());

        increaseIndentPattern = compiled.getValue();
    }

    if (auto* statesObject = parsed["states"].getDynamicObject())
    {
        for (const auto& stateProperty : statesObject->getProperties())
        {
            State state;
            state.name = stateProperty.name.toString();

            if (const String scope = getStringProperty (stateProperty.value, "scope"); scope.isNotEmpty())
                state.scope = Identifier (scope);

            if (const auto* rules = stateProperty.value["rules"].getArray())
            {
                for (const auto& ruleObject : *rules)
                {
                    Rule rule;
                    if (auto result = parseRule (ruleObject, rule); result.failed())
                        return Result::fail (result.getErrorMessage() + " in state \"" + state.name + "\"");

                    state.rules.push_back (std::move (rule));
                }
            }

            if (auto result = addState (std::move (state)); result.failed())
                return result;
        }
    }

    if (auto* injectionsObject = parsed["injections"].getDynamicObject())
    {
        for (const auto& injection : injectionsObject->getProperties())
        {
            const auto type = scopeToTokenType (injection.name);
            if (tokenTypeToString (type) != injection.name.toString())
                return Result::fail ("Unknown token type \"" + injection.name.toString() + "\" in \"injections\"");

            const auto stateName = injection.value.toString();
            if (findState (stateName) < 0)
                return Result::fail ("Unknown state \"" + stateName + "\" in \"injections\"");

            setInjection (type, stateName);
        }
    }

    return Result::ok();
}

//==============================================================================

const String& SyntaxDefinition::getName() const
{
    return name;
}

const std::vector<String>& SyntaxDefinition::getFileExtensions() const
{
    return fileExtensions;
}

const String& SyntaxDefinition::getLineCommentPrefix() const
{
    return lineCommentPrefix;
}

const std::optional<SyntaxDefinition::Delimiters>& SyntaxDefinition::getBlockComment() const
{
    return blockComment;
}

const std::vector<String>& SyntaxDefinition::getStringDelimiters() const
{
    return stringDelimiters;
}

const std::vector<String>& SyntaxDefinition::getMultiLineStringDelimiters() const
{
    return multiLineStringDelimiters;
}

yup_wchar SyntaxDefinition::getEscapeCharacter() const
{
    return escapeCharacter;
}

bool SyntaxDefinition::areStringsMultiLine() const
{
    return multiLineStrings;
}

bool SyntaxDefinition::supportsRawStrings() const
{
    return rawStrings;
}

bool SyntaxDefinition::isRawStringPrefix (StringRef word) const
{
    const String prefix (word);

    return std::find (rawStringPrefixes.begin(), rawStringPrefixes.end(), prefix) != rawStringPrefixes.end();
}

const std::vector<String>& SyntaxDefinition::getRawStringPrefixes() const
{
    return rawStringPrefixes;
}

bool SyntaxDefinition::isStringPrefix (StringRef word) const
{
    const String prefix (word);

    return std::find (stringPrefixes.begin(), stringPrefixes.end(), prefix) != stringPrefixes.end();
}

const std::vector<String>& SyntaxDefinition::getStringPrefixes() const
{
    return stringPrefixes;
}

const String& SyntaxDefinition::getPreprocessorPrefix() const
{
    return preprocessorPrefix;
}

bool SyntaxDefinition::numbersAllowHex() const
{
    return allowHex;
}

bool SyntaxDefinition::numbersAllowBinary() const
{
    return allowBinary;
}

bool SyntaxDefinition::numbersAllowFloat() const
{
    return allowFloat;
}

bool SyntaxDefinition::numbersAllowExponent() const
{
    return allowExponent;
}

bool SyntaxDefinition::numbersAllowSuffix() const
{
    return allowSuffix;
}

bool SyntaxDefinition::isKeyword (StringRef word) const
{
    if (keywords.find (String (word)) != keywords.end())
        return true;

    const auto scope = getWordScope (word);
    return scope.isValid() && scopeToTokenType (scope) == TokenType::keyword;
}

bool SyntaxDefinition::isType (StringRef word) const
{
    if (types.find (String (word)) != types.end())
        return true;

    const auto scope = getWordScope (word);
    return scope.isValid() && scopeToTokenType (scope) == TokenType::type;
}

bool SyntaxDefinition::isOperator (StringRef word) const
{
    return operators.find (String (word)) != operators.end();
}

Identifier SyntaxDefinition::getWordScope (StringRef word) const
{
    if (wordScopes.empty())
        return {};

    const auto found = wordScopes.find (String (word));
    return found != wordScopes.end() ? found->second : Identifier();
}

void SyntaxDefinition::addWords (const Identifier& scope, const StringArray& words)
{
    for (const auto& word : words)
        wordScopes[word] = scope;
}

bool SyntaxDefinition::isKnownWord (StringRef word) const
{
    return isKeyword (word) || isType (word) || getWordScope (word).isValid();
}

//==============================================================================

Result SyntaxDefinition::addState (State state)
{
    if (state.name.isEmpty())
        return Result::fail ("Syntax definition state is missing a name");

    CompiledState compiled;
    compiled.regexes.reserve (state.rules.size());

    for (const auto& rule : state.rules)
    {
        if (referencesPushCaptures (rule.match))
        {
            compiled.regexes.emplace_back();
            continue;
        }

        auto regex = CodeRegex::compile (rule.match);
        if (regex.failed())
            return Result::fail (regex.getErrorMessage() + " in state \"" + state.name + "\"");

        compiled.regexes.push_back (regex.getValue());
    }

    compiled.state = std::move (state);

    if (const auto index = findState (compiled.state.name); index >= 0)
        states[static_cast<size_t> (index)] = std::move (compiled);
    else
        states.push_back (std::move (compiled));

    return Result::ok();
}

int SyntaxDefinition::getNumStates() const noexcept
{
    return static_cast<int> (states.size());
}

const SyntaxDefinition::State& SyntaxDefinition::getState (int stateIndex) const
{
    jassert (isPositiveAndBelow (stateIndex, getNumStates()));
    return states[static_cast<size_t> (stateIndex)].state;
}

int SyntaxDefinition::findState (StringRef stateName) const
{
    for (size_t i = 0; i < states.size(); ++i)
    {
        if (states[i].state.name == stateName)
            return static_cast<int> (i);
    }

    return -1;
}

const CodeRegex* SyntaxDefinition::getCompiledRule (int stateIndex, int ruleIndex) const
{
    if (! isPositiveAndBelow (stateIndex, getNumStates()))
        return nullptr;

    const auto& regexes = states[static_cast<size_t> (stateIndex)].regexes;
    if (! isPositiveAndBelow (ruleIndex, static_cast<int> (regexes.size())))
        return nullptr;

    const auto& regex = regexes[static_cast<size_t> (ruleIndex)];
    return regex.isValid() ? &regex : nullptr;
}

void SyntaxDefinition::setInjection (TokenType type, const String& stateName)
{
    injections[static_cast<size_t> (type)] = stateName;
}

int SyntaxDefinition::getInjectionState (TokenType type) const
{
    const auto& stateName = injections[static_cast<size_t> (type)];
    return stateName.isNotEmpty() ? findState (stateName) : -1;
}

//==============================================================================

bool SyntaxDefinition::isWordSeparator (yup_wchar character) const
{
    return CharacterFunctions::isWhitespace (character) || wordSeparators.containsChar (character);
}

const CodeRegex* SyntaxDefinition::getIncreaseIndentPattern() const noexcept
{
    return increaseIndentPattern.isValid() ? &increaseIndentPattern : nullptr;
}

//==============================================================================

bool SyntaxDefinition::isIdentifierStart (yup_wchar character) const
{
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z')
        || character == '_';
}

bool SyntaxDefinition::isIdentifierPart (yup_wchar character) const
{
    return isIdentifierStart (character) || (character >= '0' && character <= '9');
}

//==============================================================================

const SyntaxDefinition& SyntaxDefinition::getBuiltIn (StringRef languageName)
{
    static const SyntaxDefinition cpp = parseBuiltIn (cppDefinitionJson);
    static const SyntaxDefinition glsl = parseBuiltIn (glslDefinitionJson);
    static const SyntaxDefinition python = parseBuiltIn (pythonDefinitionJson);
    static const SyntaxDefinition xml = parseBuiltIn (xmlDefinitionJson);
    static const SyntaxDefinition ydsp = parseBuiltIn (ydspDefinitionJson);
    static const SyntaxDefinition yui = parseBuiltIn (yuiDefinitionJson);
    static const SyntaxDefinition inert;

    const String language (languageName);

    if (language == "cpp" || language == "c++" || language == "c")
        return cpp;

    if (language == "glsl")
        return glsl;

    if (language == "python" || language == "py")
        return python;

    if (language == "xml")
        return xml;

    if (language == "ydsp")
        return ydsp;

    if (language == "yui")
        return yui;

    return inert;
}

const SyntaxDefinition* SyntaxDefinition::getBuiltInForExtension (StringRef fileExtension)
{
    const String extension (fileExtension);

    if (extension.isNotEmpty())
    {
        const auto& cpp = getBuiltIn ("cpp");
        const auto& glsl = getBuiltIn ("glsl");
        const auto& python = getBuiltIn ("python");
        const auto& xml = getBuiltIn ("xml");
        const auto& ydsp = getBuiltIn ("ydsp");
        const auto& yui = getBuiltIn ("yui");

        for (const auto& candidate : { &cpp, &glsl, &python, &xml, &ydsp, &yui })
        {
            for (const auto& candidateExtension : candidate->getFileExtensions())
            {
                if (candidateExtension == extension)
                    return candidate;
            }
        }
    }

    return nullptr;
}

//==============================================================================

String SyntaxDefinition::tokenTypeToString (TokenType type)
{
    switch (type)
    {
        case TokenType::comment:
            return "comment";
        case TokenType::string:
            return "string";
        case TokenType::number:
            return "number";
        case TokenType::keyword:
            return "keyword";
        case TokenType::type:
            return "type";
        case TokenType::operator_:
            return "operator";
        case TokenType::preprocessor:
            return "preprocessor";
        case TokenType::identifier:
            return "identifier";
        case TokenType::whitespace:
            return "whitespace";
        case TokenType::other:
            return "other";
    }

    return "other";
}

SyntaxDefinition::TokenType SyntaxDefinition::scopeToTokenType (const Identifier& scope)
{
    const auto name = scope.toString();
    const auto root = name.upToFirstOccurrenceOf (".", false, false);

    for (int i = 0; i <= static_cast<int> (TokenType::other); ++i)
    {
        const auto type = static_cast<TokenType> (i);
        if (root == tokenTypeToString (type))
            return type;
    }

    return TokenType::other;
}

//==============================================================================

} // namespace yup
