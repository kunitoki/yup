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


#include "yup_YuiCommands.h"
#include "yup_YuiToolIndex.h"
#include "yup_YuiToolSupport.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>

using namespace yup;

// What an editor asks of a document: its outline, and what can be written, is declared or is described at a place
// of it. The document is read as it is typed, so it is answered for one that does not load too.

namespace
{

constexpr int maximumBases = 16;

//==============================================================================
/** What a command of an editor is asked about: a file, a place of it, and the models it binds to. */
struct Asked
{
    File file;
    YuiToolPlace place;
    YuiToolModels models;
};

// Reads the arguments of a command of an editor, printing what is wrong with them
bool readAsked (int argc, char** argv, bool needsPlace, Asked& asked)
{
    bool hasModels = false;

    for (int i = 2; i < argc; ++i)
    {
        const String argument (argv[i]);
        const auto hasValue = i + 1 < argc;

        if (argument == "--models" && hasValue)
        {
            if (! asked.models.load (argv[++i]))
                return false;

            hasModels = true;
        }
        else if (needsPlace && argument == "--at" && hasValue)
        {
            const String value (argv[++i]);

            asked.place = { value.upToFirstOccurrenceOf (":", false, false).getIntValue(), value.fromFirstOccurrenceOf (":", false, false).getIntValue() };

            if (asked.place.line < 1 || asked.place.column < 1)
            {
                std::cerr << "invalid place, expected <line>:<column>, both from 1: " << value << "\n";
                return false;
            }
        }
        else if (asked.file == File() && ! argument.startsWithChar ('-'))
        {
            asked.file = File::getCurrentWorkingDirectory().getChildFile (argument);
        }
        else
        {
            std::cerr << "invalid argument: " << argv[i] << "\n";
            return false;
        }
    }

    if (asked.file == File() || (needsPlace && asked.place.line < 1))
    {
        std::cerr << (needsPlace ? "expected a file and --at <line>:<column>\n" : "expected a file\n");
        return false;
    }

    if (! asked.file.existsAsFile())
    {
        std::cerr << "can't read '" << asked.file.getFullPathName() << "'\n";
        return false;
    }

    // Without models named, those next to the file are used, as the tools of agents use them
    if (const auto beside = asked.file.getSiblingFile (asked.file.getFileNameWithoutExtension() + ".models.json"); ! hasModels && beside.existsAsFile())
        ignoreUnused (asked.models.loadFile (beside));

    return true;
}

int answer (const var& told)
{
    std::cout << JSON::toString (told, true).toStdString() << "\n";
    return 0;
}

var makeNothing()
{
    return var (new DynamicObject());
}

var describeRange (const YuiToolIndex& index, int start, int end)
{
    const auto from = index.getPlace (start);
    const auto to = index.getPlace (end);

    return var (Array<var> { from.line, from.column, to.line, to.column });
}

//==============================================================================
// The first declaration of a kind and a name of a file or of those it imports, with the file it is in
const YuiToolDeclaration* findDeclared (const std::vector<YuiToolSource>& sources, const String& kind, const String& name, const YuiToolSource** owner = nullptr)
{
    for (const auto& source : sources)
    {
        if (const auto* declaration = source.index->findDeclaration (kind, name))
        {
            if (owner != nullptr)
                *owner = &source;

            return declaration;
        }
    }

    return nullptr;
}

// The type a block is an element of: the one a component declared is made from, else the first capitalized name
// of its header, which is the type a rule of a stylesheet selects
String findTypeName (const YuiToolBlock& block)
{
    const auto named = block.getKeyword() == "component" ? block.header.fromFirstOccurrenceOf (":", false, false) : block.header;
    auto characters = named.getCharPointer();

    while (! characters.isEmpty() && ! (*characters >= 'A' && *characters <= 'Z'))
        ++characters;

    String name;

    while (YuiToolIndex::isPartOfName (*characters))
        name << String::charToString (characters.getAndAdvance());

    return name;
}

// The registered type a name stands for: itself, or the one a component declared is made from. The components
// on the way are told too, as they declare properties and signals of their own
const ComponentTypeInfo* resolveType (const std::vector<YuiToolSource>& sources, String name, std::vector<const YuiToolDeclaration*>* components = nullptr)
{
    for (int depth = 0; depth < maximumBases && name.isNotEmpty(); ++depth)
    {
        if (const auto* info = ComponentTypeRegistry::getInstance().findType (Identifier (name)))
            return info;

        const auto* component = findDeclared (sources, "component", name);

        if (component == nullptr)
            return nullptr;

        if (components != nullptr)
            components->push_back (component);

        name = component->base;
    }

    return nullptr;
}

// True for what follows a property or an arrow, as the block of a handler is: it holds no element
bool isValueBlock (const YuiToolBlock& block)
{
    const auto last = block.header.getLastCharacter();

    return last == ':' || last == '>' || last == '=';
}

String describeSignature (const var& signal)
{
    StringArray arguments;

    if (const auto* listed = signal["arguments"].getArray())
        for (const auto& argument : *listed)
            arguments.add (argument["name"].toString() + ": " + argument["type"].toString());

    return signal["name"].toString() + " (" + arguments.joinIntoString (", ") + ")";
}

//==============================================================================
// The id a component is given, empty when it has none
String findId (const YuiToolBlock& block)
{
    for (const auto& statement : block.statements)
        if (statement.text.upToFirstOccurrenceOf (":", false, false).trim() == "id")
            return statement.text.fromFirstOccurrenceOf (":", false, false).trim();

    return {};
}

// The statement naming the file a shader is made from, as it is written: `fragment: "fx/crt.frag"`, or `material` in
// place of `fragment`. Empty for a shader that names none yet
String findShaderFile (const YuiToolIndex& index, const YuiToolDeclaration& shader)
{
    for (const auto& statement : shader.block->statements)
    {
        const auto name = statement.text.upToFirstOccurrenceOf (":", false, false).trim();

        if (name == "fragment" || name == "material")
            return index.getText (statement.start, statement.end);
    }

    return {};
}

// What a statement gives the property it starts with, as it is written and without the quotes of a string: the shader of
// a material still loads named in one, `shader: "Hologram"`, which the code of a file leaves blank
String findValue (const YuiToolIndex& index, const YuiToolStatement& statement)
{
    return index.getText (statement.start, statement.end).fromFirstOccurrenceOf (":", false, false).trim().unquoted();
}

// The shader a material names in a string at a place, with where its name starts, or an empty one
String findShaderNamedAt (const YuiToolIndex& index, int offset, int& start)
{
    const auto chain = index.findBlocksAt (offset);

    if (chain.empty())
        return {};

    for (const auto& statement : chain.back()->statements)
    {
        if (offset < statement.start || offset > statement.end || statement.text.upToFirstOccurrenceOf (":", false, false).trim() != "shader")
            continue;

        const auto name = findValue (index, statement);
        start = statement.start + index.getText (statement.start, statement.end).lastIndexOf (name);

        return offset >= start && offset <= start + name.length() ? name : String();
    }

    return {};
}

var makeSymbol (const String& name, const String& kind, const String& detail, const var& range, const var& selection, const Array<var>& children)
{
    auto* symbol = new DynamicObject();
    symbol->setProperty ("name", name);
    symbol->setProperty ("kind", kind);

    if (detail.isNotEmpty())
        symbol->setProperty ("detail", detail);

    symbol->setProperty ("range", range);
    symbol->setProperty ("selection", selection);

    if (! children.isEmpty())
        symbol->setProperty ("children", var (children));

    return var (symbol);
}

void addSymbols (const YuiToolIndex& index, const YuiToolBlock& parent, bool areRules, Array<var>& symbols);

// A block as an outline shows it, nothing for the shapes of a drawing and the blocks of handlers, which would
// bury what an outline is for
var describeSymbol (const YuiToolIndex& index, const YuiToolBlock& block, bool isRule)
{
    const auto& declarations = index.getDeclarations();
    const auto declaration = std::find_if (declarations.begin(), declarations.end(), [&block] (const YuiToolDeclaration& candidate)
    {
        return candidate.block == &block;
    });

    auto name = block.header;
    String kind;
    Array<var> children;

    if (declaration != declarations.end())
    {
        name = declaration->name;
        kind = declaration->kind;
    }
    else if (block.header == "style")
    {
        kind = "style";
    }
    else if (! isRule && block.isElement())
    {
        kind = "element";

        if (const auto id = findId (block); id.isNotEmpty())
            name << " #" << id;
    }
    else if (isRule && block.header.isNotEmpty())
    {
        kind = "rule";
    }
    else
    {
        return {};
    }

    // A drawing, a shader, a palette and the tokens hold what they declare, the rest what is written in them
    if (declaration != declarations.end() && declaration->kind != "component")
    {
        for (const auto& member : declaration->members)
        {
            const auto range = describeRange (index, member.offset, member.offset + member.name.length());
            children.add (makeSymbol (member.name, "property", member.detail, range, range, {}));
        }
    }
    else
    {
        addSymbols (index, block, kind == "style", children);
    }

    return makeSymbol (name, kind, {}, describeRange (index, block.start, block.end), describeRange (index, block.start, block.start + block.header.length()), children);
}

// The symbols of what a block holds: what repeats components is passed through, as it only holds them
void addSymbols (const YuiToolIndex& index, const YuiToolBlock& parent, bool areRules, Array<var>& symbols)
{
    for (const auto& block : parent.blocks)
    {
        if (! areRules && block->getKeyword() == "repeat")
        {
            addSymbols (index, *block, false, symbols);
            continue;
        }

        if (const auto symbol = describeSymbol (index, *block, areRules); ! symbol.isVoid())
            symbols.add (symbol);
    }
}

// What a file can show: its interface first, then what it declares, then what the files it imports declare,
// each of these told with the name of its file
var describeTargets (const std::vector<YuiToolSource>& sources)
{
    Array<var> targets;
    StringArray ids;

    const auto add = [&] (const String& id, const String& label, const String& kind, const String& name)
    {
        // The two versions of a palette share a name, and what is declared again is the first one
        if (! ids.addIfNotAlreadyThere (id))
            return;

        auto* target = new DynamicObject();
        target->setProperty ("id", id);
        target->setProperty ("label", label);
        target->setProperty ("drawing", kind == "drawing" ? name : String());
        target->setProperty ("palette", kind == "palette" ? name : String());
        target->setProperty ("component", kind == "component" ? name : String());

        targets.add (var (target));
    };

    if (sources.front().index->getRoot() != nullptr)
        add ("root", "Interface", {}, {});

    for (const auto& source : sources)
    {
        const auto from = &source == &sources.front() ? String() : " (" + source.file.getFileName() + ")";

        for (const String kind : { "drawing", "palette", "component" })
            for (const auto& declaration : source.index->getDeclarations())
                if (declaration.kind == kind)
                    add (kind + ":" + declaration.name, kind.substring (0, 1).toUpperCase() + kind.substring (1) + ": " + declaration.name + from, kind, declaration.name);
    }

    return var (targets);
}

// True for a type of a 3D scene. A scene shows what its nodes are, which paint nothing: a drawing is shown as none of them
bool isSceneType (const ComponentTypeInfo* info)
{
    return info != nullptr
        && (info->isSubtypeOf (Identifier ("Scene3D")) || info->isSubtypeOf (Identifier ("Node3D")) || info->isSubtypeOf (Identifier ("Material3D")));
}

// The types a drawing can be shown alone as: a plain component first, then those that can be made
var describeHosts (const std::vector<YuiToolSource>& sources)
{
    StringArray hosts;

    for (const auto* info : ComponentTypeRegistry::getInstance().getRegisteredTypes())
        if (info->hasFactory() && ! isSceneType (info))
            hosts.addIfNotAlreadyThere (info->getTypeName().toString());

    for (const auto& source : sources)
        for (const auto& declaration : source.index->getDeclarations())
            if (declaration.kind == "component" && ! isSceneType (resolveType (sources, declaration.name)))
                hosts.addIfNotAlreadyThere (declaration.name);

    hosts.removeString ("Component");
    hosts.sort (false);
    hosts.insert (0, "Component");

    return var (hosts);
}

var describeOutline (const std::vector<YuiToolSource>& sources)
{
    const auto& main = sources.front();
    const auto& index = *main.index;
    Array<var> imports;
    Array<var> symbols;
    Array<var> colors;

    for (const auto& imported : index.getImports())
    {
        auto* record = new DynamicObject();
        record->setProperty ("path", imported.path);
        record->setProperty ("range", describeRange (index, imported.start, imported.end));

        // Where the file is, when it was found
        const auto found = std::find_if (sources.begin(), sources.end(), [&] (const YuiToolSource& source)
        {
            return source.file == main.file.getParentDirectory().getChildFile (imported.path);
        });

        if (found != sources.end())
            record->setProperty ("file", found->file.getFullPathName());

        imports.add (var (record));
    }

    addSymbols (index, index.getDocument(), false, symbols);

    // A part is told to a thousandth, which is finer than the 255 steps it is written with
    const auto part = [] (float value)
    {
        return std::round (static_cast<double> (value) * 1000.0) / 1000.0;
    };

    for (const auto& color : index.findColors())
    {
        auto* record = new DynamicObject();
        record->setProperty ("range", describeRange (index, color.start, color.end));
        record->setProperty ("color", var (Array<var> { part (color.red), part (color.green), part (color.blue), part (color.alpha) }));

        colors.add (var (record));
    }

    auto* outline = new DynamicObject();
    outline->setProperty ("file", main.file.getFullPathName());
    outline->setProperty ("imports", var (imports));
    outline->setProperty ("symbols", var (symbols));
    outline->setProperty ("colors", var (colors));
    outline->setProperty ("targets", describeTargets (sources));
    outline->setProperty ("hosts", describeHosts (sources));

    return var (outline);
}

//==============================================================================
/** What can be written at a place, each thing once, in the order it was found. */
class Completions
{
public:
    void add (const String& label, const char* kind, const String& detail = {}, const String& documentation = {})
    {
        if (label.isEmpty() || ! labels.insert (label).second)
            return;

        auto* item = new DynamicObject();
        item->setProperty ("label", label);
        item->setProperty ("kind", kind);

        if (detail.isNotEmpty())
            item->setProperty ("detail", detail);

        if (documentation.isNotEmpty())
            item->setProperty ("documentation", documentation);

        items.add (var (item));
    }

    var toJson() const
    {
        auto* found = new DynamicObject();
        found->setProperty ("items", var (items));

        return var (found);
    }

private:
    Array<var> items;
    std::set<String> labels;
};

void addTypes (Completions& found, const std::vector<YuiToolSource>& sources)
{
    for (const auto* info : ComponentTypeRegistry::getInstance().getRegisteredTypes())
        if (info->hasFactory())
            found.add (info->getTypeName().toString(), "type");

    for (const auto& source : sources)
        for (const auto& declaration : source.index->getDeclarations())
            if (declaration.kind == "component")
                found.add (declaration.name, "type");
}

// The names of what a file and those it imports declare of a kind
void addDeclared (Completions& found, const std::vector<YuiToolSource>& sources, const char* kind)
{
    for (const auto& source : sources)
        for (const auto& declaration : source.index->getDeclarations())
            if (declaration.kind == kind)
                found.add (declaration.name, kind);
}

// The shaders a file and those it imports declare that are made from a `fragment`, which an effect, a drawing and a
// filter run, or from a `material`, which a Material3D is drawn by. Each is told with the file it is made from
void addShaders (Completions& found, const std::vector<YuiToolSource>& sources, const char* madeFrom)
{
    for (const auto& source : sources)
    {
        for (const auto& declaration : source.index->getDeclarations())
        {
            if (declaration.kind != "shader")
                continue;

            if (const auto file = findShaderFile (*source.index, declaration); file.startsWith (madeFrom))
                found.add (declaration.name, "shader", file);
        }
    }
}

// What the declarations of a kind hold, as the colors of the palettes and the tokens
void addDeclaredMembers (Completions& found, const std::vector<YuiToolSource>& sources, const char* kind, const char* itemKind)
{
    for (const auto& source : sources)
        for (const auto& declaration : source.index->getDeclarations())
            if (declaration.kind == kind)
                for (const auto& member : declaration.members)
                    found.add (member.name, itemKind, member.detail);
}

// The properties and the signals of a type, those a component declared adds to its base included
void addTypeMembers (Completions& found, const std::vector<YuiToolSource>& sources, const String& typeName)
{
    std::vector<const YuiToolDeclaration*> components;
    const auto* info = resolveType (sources, typeName, &components);

    for (const auto* component : components)
        for (const auto& member : component->members)
            found.add (member.name, member.detail == "signal" ? "signal" : "property", member.detail);

    if (info == nullptr)
        return;

    const auto described = describeYuiToolComponentType (*info, true);

    for (const auto& property : *described["properties"].getArray())
        found.add (property["name"].toString(), "property", property["type"].toString(), property["description"].toString());

    for (const auto& signal : *described["signals"].getArray())
        found.add (signal["name"].toString(), "signal", describeSignature (signal), signal["description"].toString());
}

// The members of what a name stands for: the colors of the palette, the tokens, or the properties of a model
void addMembersOf (Completions& found, const std::vector<YuiToolSource>& sources, const String& owner, const YuiToolModels& models)
{
    if (owner == "palette")
    {
        for (const auto& role : ThemePalette::getRoleNames())
            found.add (role, "value");

        found.add ("dark", "value");
        addDeclaredMembers (found, sources, "palette", "color");
    }
    else if (owner == "colors")
    {
        for (const auto& named : Colors::getNamedColors())
            found.add (String (named.name), "color");
    }
    else if (owner == "tokens")
    {
        addDeclaredMembers (found, sources, "tokens", "constant");
    }
    else
    {
        const auto values = models.toJson();

        if (const auto* model = values[Identifier (owner)].getDynamicObject())
        {
            for (const auto& property : model->getProperties())
            {
                const auto& value = property.value;

                found.add (property.name.toString(), "field", value.isArray() ? "list" : (value.isBool() ? "boolean" : (value.isString() ? "string" : "number")));
            }
        }
    }
}

// The values a property takes: the drawings, the palettes and the shaders declared, or those of an enumeration
void addValuesOf (Completions& found, const std::vector<YuiToolSource>& sources, const String& property, const YuiToolBlock* inner, bool inString)
{
    // The shader of a material is named bare, as a drawing is. It still loads named in a string, so one being typed
    // is completed with it, and with nothing else that is told here
    if (property == "shader")
        addShaders (found, sources, "material");

    if (inString)
        return;

    if (property == "drawing" || property == "palette")
    {
        addDeclared (found, sources, property == "drawing" ? "drawing" : "palette");
        found.add ("none", "value");
        return;
    }

    if (property == "effect")
    {
        addShaders (found, sources, "fragment");
        found.add ("none", "value");
        return;
    }

    const auto* info = inner != nullptr ? resolveType (sources, findTypeName (*inner)) : nullptr;

    if (info == nullptr)
        return;

    const auto described = describeYuiToolComponentType (*info, true);

    for (const auto& candidate : *described["properties"].getArray())
    {
        if (candidate["name"].toString() != property)
            continue;

        if (const auto* values = candidate["values"].getArray())
            for (const auto& value : *values)
                found.add (value.toString(), "value");
    }
}

// What an element of a drawing holds, with the properties it takes
void addDrawingStatements (Completions& found, const std::vector<YuiToolSource>& sources, const YuiToolBlock& inner)
{
    static const StringArray declaring { "property", "let", "repeat", "use", "drawing", "pattern", "marker" };

    const auto topic = describeYuiToolSchemaTopic ("drawing");
    const auto keyword = inner.getKeyword();
    const auto* elements = topic["elements"].getArray();

    if (elements == nullptr)
        return;

    const auto addAll = [&found] (const var& names, const char* kind)
    {
        if (const auto* listed = names.getArray())
            for (const auto& name : *listed)
                found.add (name.toString().upToFirstOccurrenceOf ("=", false, false), kind != nullptr ? kind : (declaring.contains (name.toString()) ? "keyword" : "element"));
    };

    // A text is described twice, for the two ways it is placed
    for (const auto& element : *elements)
        if (element["type"].toString() == keyword)
            addAll (element["holds"], nullptr);

    for (const auto& element : *elements)
    {
        if (element["type"].toString() != keyword)
            continue;

        // A property of the geometry with a default is told with it, as `angle=0`
        addAll (element["geometry"], "property");
        addAll (element["properties"], "property");

        if (element["role"].toString() == "shape")
            addAll (topic["shapePaint"], "property");
    }

    // A drawing that is used also takes the properties it declares, and so does a shader, used when no drawing has its name
    if (keyword == "use")
    {
        const auto name = inner.header.fromFirstOccurrenceOf ("use", false, false).trim();
        const auto* used = findDeclared (sources, "drawing", name);

        if (used == nullptr)
            used = findDeclared (sources, "shader", name);

        if (used != nullptr)
            for (const auto& member : used->members)
                found.add (member.name, "property", member.detail);
    }
}

// The properties of the drawing and of the shader a block picks: with `drawing` the one painting it, with `effect` the
// shader run over it, and with `shader` the one a material is drawn by
void addPickedMembers (Completions& found, const std::vector<YuiToolSource>& sources, const YuiToolBlock& block)
{
    for (const auto& statement : block.statements)
    {
        const auto name = statement.text.upToFirstOccurrenceOf (":", false, false).trim();

        if (name != "drawing" && name != "effect" && name != "shader")
            continue;

        if (const auto* picked = findDeclared (sources, name == "drawing" ? "drawing" : "shader", findValue (*sources.front().index, statement)))
            for (const auto& member : picked->members)
                found.add (member.name, "property", member.detail);
    }
}

// What can be written where a statement starts, which depends on what it is written in
void addStatements (Completions& found, const std::vector<YuiToolSource>& sources, const std::vector<const YuiToolBlock*>& chain)
{
    if (chain.empty())
    {
        for (const auto* keyword : { "import", "tokens", "palette", "drawing", "shader", "component", "style" })
            found.add (keyword, "keyword");

        addTypes (found, sources);
        return;
    }

    const auto& top = *chain.front();
    const auto& inner = *chain.back();
    const auto topKeyword = top.getKeyword();

    // A drawing, of the document or of its stylesheet
    if (topKeyword == "drawing" || (top.header == "style" && chain.size() > 1 && chain[1]->getKeyword() == "drawing"))
    {
        addDrawingStatements (found, sources, inner);
        return;
    }

    if (top.header == "tokens")
        return;

    // A shader names the file it is made from, and declares what it takes
    if (topKeyword == "shader")
    {
        found.add ("fragment", "property");
        found.add ("material", "property");
        found.add ("property", "keyword");
        return;
    }

    if (topKeyword == "palette")
    {
        found.add ("mode", "property");
        found.add ("colors", "property");

        for (const auto& role : ThemePalette::getRoleNames())
            found.add (role, "property");

        return;
    }

    if (top.header == "style" && &inner == &top)
    {
        for (const auto* keyword : { "drawing", "palette", "property" })
            found.add (keyword, "keyword");

        addTypes (found, sources);
        return;
    }

    if (top.header == "style")
    {
        // A rule sets what styles take, the properties of the drawing and of the shader it picks and those of the type it selects
        for (const auto& name : StyleSheet::getDeclarationNames())
            found.add (name, "property");

        addPickedMembers (found, sources, inner);
        addTypeMembers (found, sources, findTypeName (inner));
        return;
    }

    // A component of the interface, or of a component declared, where what repeats it stands for the one it is in
    const auto* element = &inner;

    for (auto block = chain.rbegin(); block != chain.rend(); ++block)
    {
        if ((*block)->getKeyword() != "repeat")
        {
            element = *block;
            break;
        }
    }

    if (element == &top && topKeyword == "component")
    {
        found.add ("property", "keyword");
        found.add ("signal", "keyword");
    }

    addTypes (found, sources);
    addTypeMembers (found, sources, findTypeName (*element));
    addPickedMembers (found, sources, *element);
}

// What can be written at a place of a file: what it is depends on what is written before it on its line
var complete (const std::vector<YuiToolSource>& sources, int offset, const YuiToolModels& models)
{
    const auto& index = *sources.front().index;
    const auto chain = index.findBlocksAt (offset);
    const auto* inner = chain.empty() ? nullptr : chain.back();
    const auto lineStart = index.getLineStart (offset);
    Completions found;

    // The name being typed, and what is written before it
    auto typed = offset;

    while (typed > lineStart && YuiToolIndex::isPartOfName (index.getCodeAt (typed - 1)))
        --typed;

    const auto before = index.getCode (lineStart, typed);
    const auto written = before.trimEnd();

    // What follows a dot is a member of what is before it
    if (before.endsWithChar ('.'))
    {
        int start = 0;
        const auto owner = index.findNameAt (typed - 1, start);

        if (owner.isNotEmpty() && index.getCodeAt (start - 1) != '.')
            addMembersOf (found, sources, owner, models);

        return found.toJson();
    }

    // A colon in a selector starts a pseudo class, which is made of letters and dashes
    auto pseudo = offset;

    while (pseudo > lineStart && (CharacterFunctions::isLetter (index.getCodeAt (pseudo - 1)) || index.getCodeAt (pseudo - 1) == '-'))
        --pseudo;

    if (inner != nullptr && inner->header == "style" && index.getCodeAt (pseudo - 1) == ':' && ! index.getCode (lineStart, offset).containsChar (';'))
    {
        for (const auto& name : StyleSelector::getPseudoClassNames())
            found.add (name, "value");

        for (const auto* info : ComponentTypeRegistry::getInstance().getRegisteredTypes())
            for (const auto& property : info->getOwnProperties())
                if (property.isState)
                    found.add (property.name.toString(), "value");

        return found.toJson();
    }

    // The drawing or the shader an element uses, the drawings a drawing declares for itself first
    if (written.trimStart() == "use" && written != before)
    {
        if (! chain.empty())
            for (const auto& block : chain.front()->blocks)
                if (block->getKeyword() == "drawing")
                    found.add (block->header.fromFirstOccurrenceOf ("drawing", false, false).trim(), "drawing");

        addDeclared (found, sources, "drawing");
        addShaders (found, sources, "fragment");
        return found.toJson();
    }

    // The value of a property, which a statement starts with, also right after the quote opening a string
    const auto inString = written.endsWithChar ('"');
    const auto opening = inString ? written.dropLastCharacters (1).trimEnd() : written;

    if (opening.endsWithChar (':'))
    {
        auto end = lineStart + opening.length() - 1;

        while (end > lineStart && CharacterFunctions::isWhitespace (index.getCodeAt (end - 1)))
            --end;

        int start = 0;
        const auto property = index.findNameAt (end, start);

        while (start > lineStart && CharacterFunctions::isWhitespace (index.getCodeAt (start - 1)))
            --start;

        const auto opener = index.getCodeAt (start - 1);

        if (property.isNotEmpty() && (start == lineStart || opener == '{' || opener == ';'))
            addValuesOf (found, sources, property, inner, inString);

        return found.toJson();
    }

    // A statement, which a line, a brace or a semicolon is followed by, and not in what a handler does
    const auto startsStatement = written.isEmpty() || written.endsWithChar ('{') || written.endsWithChar (';');

    if (startsStatement && (inner == nullptr || ! isValueBlock (*inner)))
        addStatements (found, sources, chain);

    return found.toJson();
}

//==============================================================================
var tellPlace (const YuiToolSource& source, int offset)
{
    const auto place = source.index->getPlace (offset);

    auto* told = new DynamicObject();
    told->setProperty ("file", source.file.getFullPathName());
    told->setProperty ("line", place.line);
    told->setProperty ("column", place.column);

    return var (told);
}

// Where what is named at a place of a file is declared, nothing when it is not
var findDefinition (const std::vector<YuiToolSource>& sources, int offset)
{
    const auto& main = sources.front();
    const auto& index = *main.index;

    // An import is the file it names
    for (const auto& imported : index.getImports())
    {
        if (offset < imported.start || offset > imported.end)
            continue;

        const auto file = main.file.getParentDirectory().getChildFile (imported.path);

        for (const auto& source : sources)
            if (source.file == file)
                return tellPlace (source, 0);

        return makeNothing();
    }

    int start = 0;
    const auto name = index.findNameAt (offset, start);

    if (name.isEmpty())
    {
        // The shader of a material still loads named in a string, which nothing else is named in
        const YuiToolSource* owning = nullptr;
        const auto* shader = findDeclared (sources, "shader", findShaderNamedAt (index, offset, start), &owning);

        return shader != nullptr ? tellPlace (*owning, shader->offset) : makeNothing();
    }

    // A color of a palette and a token are declared where they are given their value
    int ownerStart = 0;
    const auto owner = index.getCodeAt (start - 1) == '.' ? index.findNameAt (start - 1, ownerStart) : String();

    if ((owner == "palette" || owner == "tokens") && index.getCodeAt (ownerStart - 1) != '.')
    {
        for (const auto& source : sources)
            for (const auto& declaration : source.index->getDeclarations())
                if (declaration.kind == owner)
                    for (const auto& member : declaration.members)
                        if (member.name == name)
                            return tellPlace (source, member.offset);

        return makeNothing();
    }

    // What a drawing declares for itself comes before a drawing of the document of the same name
    if (const auto chain = index.findBlocksAt (offset, true); ! chain.empty())
    {
        for (const auto& block : chain.front()->blocks)
        {
            const auto keyword = block->getKeyword();

            if ((keyword == "drawing" || keyword == "pattern" || keyword == "marker") && block->header.fromFirstOccurrenceOf (keyword, false, false).trim() == name)
                return tellPlace (main, block->start + block->header.indexOf (keyword.length(), name));
        }
    }

    for (const auto* kind : { "drawing", "shader", "component", "palette" })
    {
        const YuiToolSource* owning = nullptr;

        if (const auto* declaration = findDeclared (sources, kind, name, &owning))
            return tellPlace (*owning, declaration->offset);
    }

    return makeNothing();
}

// A shader as it is declared, the defaults of its properties left out: the file it is made from and what it takes
String describeShader (const YuiToolIndex& index, const YuiToolDeclaration& shader)
{
    String code;
    code << "shader " << shader.name << " {";

    if (const auto file = findShaderFile (index, shader); file.isNotEmpty())
        code << "\n    " << file << ";";

    for (const auto& member : shader.members)
        code << "\n    property " << member.name << ": " << member.detail << ";";

    return code + "\n}";
}

// What the property or the signal a statement starts with is, or the shader named elsewhere, nothing for anything else
var describeHover (const std::vector<YuiToolSource>& sources, int offset)
{
    const auto& index = *sources.front().index;
    int start = 0;
    auto name = index.findNameAt (offset, start);

    // The shader of a material still loads named in a string
    if (name.isEmpty())
        name = findShaderNamedAt (index, offset, start);

    if (name.isEmpty())
        return makeNothing();

    // Only what a statement starts with is a property or a signal of what it is written in
    const auto lineStart = index.getLineStart (start);
    auto lead = start;

    while (lead > lineStart && CharacterFunctions::isWhitespace (index.getCodeAt (lead - 1)))
        --lead;

    if (const auto opener = index.getCodeAt (lead - 1); lead > lineStart && opener != '{' && opener != ';')
    {
        // A shader is told where it is named: where it is declared, and where an effect, a `use` or a material takes it
        const YuiToolSource* owner = nullptr;
        const auto* shader = findDeclared (sources, "shader", name, &owner);

        if (shader == nullptr)
            return makeNothing();

        auto* told = new DynamicObject();
        told->setProperty ("code", describeShader (*owner->index, *shader));
        told->setProperty ("range", describeRange (index, start, start + name.length()));

        return var (told);
    }

    const auto chain = index.findBlocksAt (start);
    const YuiToolBlock* element = nullptr;

    for (auto block = chain.rbegin(); block != chain.rend() && element == nullptr; ++block)
        if ((*block)->getKeyword() != "repeat" && ! isValueBlock (**block))
            element = *block;

    const auto* info = element != nullptr ? resolveType (sources, findTypeName (*element)) : nullptr;

    if (info == nullptr)
        return makeNothing();

    const auto described = describeYuiToolComponentType (*info, true);
    const auto range = describeRange (index, start, start + name.length());

    const auto tell = [&range] (const String& code, const var& member)
    {
        auto* told = new DynamicObject();
        told->setProperty ("code", code);

        if (const auto description = member["description"].toString(); description.isNotEmpty())
            told->setProperty ("text", description);

        if (member.hasProperty ("values"))
            told->setProperty ("values", member["values"]);

        told->setProperty ("range", range);

        return var (told);
    };

    for (const auto& property : *described["properties"].getArray())
        if (property["name"].toString() == name)
            return tell (name + ": " + property["type"].toString(), property);

    for (const auto& signal : *described["signals"].getArray())
        if (signal["name"].toString() == name)
            return tell ("signal " + describeSignature (signal), signal);

    return makeNothing();
}

} // namespace

//==============================================================================

int runYuiOutlineCommand (int argc, char** argv)
{
    Asked asked;

    if (! readAsked (argc, argv, false, asked))
        return 2;

    registerYuiComponentTypes();

    return answer (describeOutline (readYuiToolSources (asked.file)));
}

int runYuiCompleteCommand (int argc, char** argv)
{
    Asked asked;

    if (! readAsked (argc, argv, true, asked))
        return 2;

    registerYuiComponentTypes();

    const auto sources = readYuiToolSources (asked.file);

    return answer (complete (sources, sources.front().index->getOffset (asked.place), asked.models));
}

int runYuiDefinitionCommand (int argc, char** argv)
{
    Asked asked;

    if (! readAsked (argc, argv, true, asked))
        return 2;

    const auto sources = readYuiToolSources (asked.file);

    return answer (findDefinition (sources, sources.front().index->getOffset (asked.place)));
}

int runYuiHoverCommand (int argc, char** argv)
{
    Asked asked;

    if (! readAsked (argc, argv, true, asked))
        return 2;

    registerYuiComponentTypes();

    const auto sources = readYuiToolSources (asked.file);

    return answer (describeHover (sources, sources.front().index->getOffset (asked.place)));
}
