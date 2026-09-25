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

//==============================================================================

constexpr auto cppDefinitionJson = R"json({
    "name": "C++",
    "extensions": ["cpp", "cc", "cxx", "c", "h", "hpp", "hh", "hxx", "inl"],
    "lineComment": "//",
    "blockComment": { "start": "/*", "end": "*/" },
    "strings": { "delimiters": ["\"", "'"], "escape": "\\", "multiLine": false, "rawStrings": true, "rawStringPrefixes": ["R", "u8R", "uR", "UR", "LR"], "stringPrefixes": ["u8", "L", "u", "U"] },
    "preprocessor": "#",
    "numbers": { "hex": true, "binary": true, "float": true, "exponent": true, "suffix": true },
    "keywords": ["alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "break", "case", "catch", "class", "compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype", "default", "delete", "do", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "for", "friend", "goto", "if", "inline", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast", "requires", "return", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename", "union", "using", "virtual", "while", "xor", "xor_eq"],
    "types": ["bool", "char", "char8_t", "char16_t", "char32_t", "double", "float", "int", "long", "short", "signed", "unsigned", "void", "wchar_t", "size_t", "ptrdiff_t", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t", "intptr_t", "uintptr_t"],
    "operators": ["::", "->*", "->", "<=>", "<<=", ">>=", "...", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", ".*", "##", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}", "+", "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^", "~", "#", "@"]
})json";

constexpr auto glslDefinitionJson = R"json({
    "name": "GLSL",
    "extensions": ["glsl", "vert", "frag", "geom", "comp", "tesc", "tese", "rgen", "rchit", "rmiss"],
    "lineComment": "//",
    "blockComment": { "start": "/*", "end": "*/" },
    "strings": { "delimiters": ["\""], "escape": "\\", "multiLine": false },
    "preprocessor": "#",
    "numbers": { "hex": true, "binary": false, "float": true, "exponent": true, "suffix": false },
    "keywords": ["attribute", "const", "uniform", "varying", "buffer", "shared", "coherent", "restrict", "readonly", "writeonly", "atomic_uint", "layout", "centroid", "flat", "smooth", "noperspective", "patch", "sample", "break", "continue", "do", "for", "while", "switch", "case", "default", "if", "else", "subroutine", "in", "out", "inout", "true", "false", "invariant", "precise", "discard", "return", "struct"],
    "types": ["void", "bool", "int", "uint", "float", "double", "vec2", "vec3", "vec4", "ivec2", "ivec3", "ivec4", "uvec2", "uvec3", "uvec4", "bvec2", "bvec3", "bvec4", "dvec2", "dvec3", "dvec4", "mat2", "mat3", "mat4", "mat2x2", "mat2x3", "mat2x4", "mat3x2", "mat3x3", "mat3x4", "mat4x2", "mat4x3", "mat4x4", "dmat2", "dmat3", "dmat4", "dmat2x2", "dmat2x3", "dmat2x4", "dmat3x2", "dmat3x3", "dmat3x4", "dmat4x2", "dmat4x3", "dmat4x4", "sampler1D", "sampler2D", "sampler3D", "samplerCube", "sampler2DShadow", "samplerCubeShadow", "sampler1DArray", "sampler2DArray", "sampler2DRect", "samplerBuffer", "sampler2DMS", "isampler1D", "isampler2D", "isampler3D", "isamplerCube", "usampler1D", "usampler2D", "usampler3D", "usamplerCube", "image1D", "image2D", "image3D", "imageCube"],
    "operators": ["::", "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}", "+", "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^", "~", "#"]
})json";

constexpr auto pythonDefinitionJson = R"json({
    "name": "Python",
    "extensions": ["py", "pyw"],
    "lineComment": "#",
    "strings": { "delimiters": ["\"", "'"], "multiLineDelimiters": ["\"\"\"", "'''"], "escape": "\\", "multiLine": true, "stringPrefixes": ["f", "F", "r", "R", "b", "B", "u", "U", "fr", "Fr", "fR", "FR", "rf", "rF", "Rf", "RF", "br", "Br", "bR", "BR", "rb", "rB", "Rb", "RB"] },
    "numbers": { "hex": true, "binary": true, "float": true, "exponent": true, "suffix": false },
    "keywords": ["False", "None", "True", "and", "as", "assert", "async", "await", "break", "class", "continue", "def", "del", "elif", "else", "except", "finally", "for", "from", "global", "if", "import", "in", "is", "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try", "while", "with", "yield"],
    "types": ["int", "float", "complex", "bool", "str", "bytes", "bytearray", "list", "tuple", "dict", "set", "frozenset", "object", "type"],
    "operators": ["**", "//", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "//=", "%=", "**=", "@=", "&=", "|=", "^=", "<<=", ">>=", ":=", "->", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}", "+", "-", "*", "/", "%", "@", "=", "<", ">", "!", "&", "|", "^", "~"]
})json";

constexpr auto ydspDefinitionJson = R"json({
    "name": "YDSP",
    "extensions": ["ydsp"],
    "lineComment": "//",
    "blockComment": { "start": "/*", "end": "*/" },
    "strings": { "delimiters": ["\""], "escape": "\\", "multiLine": false },
    "numbers": { "hex": false, "binary": false, "float": true, "exponent": true, "suffix": false },
    "keywords": ["processor", "graph", "node", "connection", "input", "output", "value", "stream", "state", "process", "block", "for", "if", "else", "let", "true", "false", "declare", "func", "return", "import", "as", "struct", "init", "event"],
    "types": ["float", "float32", "float64", "int", "int32", "int64", "bool"],
    "operators": ["<<=", ">>=", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "&=", "|=", "^=", "->", "<:", ":>", "..", "[[", "]]", "?", ":", ";", ",", ".", "(", ")", "[", "]", "{", "}", "+", "-", "*", "/", "%", "=", "<", ">", "!", "&", "|", "^", "~", "@", "'"]
})json";

constexpr auto xmlDefinitionJson = R"json({
    "name": "XML",
    "extensions": ["xml", "xaml", "svg", "html", "htm", "xhtml", "xsd", "xsl", "xslt", "plist", "resx", "csproj", "vcxproj"],
    "lineComment": "",
    "blockComment": { "start": "<!--", "end": "-->" },
    "strings": { "delimiters": ["\"", "'"], "escape": "\\", "multiLine": false },
    "numbers": { "hex": false, "binary": false, "float": true, "exponent": false, "suffix": false },
    "keywords": ["xml", "DOCTYPE", "ELEMENT", "ATTLIST", "ENTITY", "NOTATION", "CDATA", "PUBLIC", "SYSTEM"],
    "operators": ["]]>", "<!", "<?", "?>", "</", "/>", "<", ">", "=", ":", "&", ";", "!", "?", "/", "-", ".", ","]
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
    return keywords.find (String (word)) != keywords.end();
}

bool SyntaxDefinition::isType (StringRef word) const
{
    return types.find (String (word)) != types.end();
}

bool SyntaxDefinition::isOperator (StringRef word) const
{
    return operators.find (String (word)) != operators.end();
}

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

        for (const auto& candidate : { &cpp, &glsl, &python, &xml, &ydsp })
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

//==============================================================================

} // namespace yup
