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

#include "yup_YuiToolSupport.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <set>

using namespace yup;

//==============================================================================

YuiToolModels::~YuiToolModels()
{
    for (auto& tree : trees)
        tree.removeListener (this);
}

bool YuiToolModels::load (const String& path)
{
    const auto file = File::getCurrentWorkingDirectory().getChildFile (path);

    if (! file.existsAsFile())
    {
        std::cerr << "can't read '" << path << "'\n";
        return false;
    }

    if (const auto result = loadFile (file); result.failed())
    {
        std::cerr << file.getFullPathName() << ": error: " << result.getErrorMessage() << "\n";
        return false;
    }

    return true;
}

Result YuiToolModels::loadFile (const File& file)
{
    var parsed;

    if (const auto result = JSON::parse (file.loadFileAsString(), parsed); result.failed())
        return result;

    return loadValues (parsed);
}

Result YuiToolModels::loadValues (const var& parsed)
{
    const auto* declared = parsed.getDynamicObject();

    if (declared == nullptr)
        return Result::fail ("expected an object with a member for each model");

    for (const auto& model : declared->getProperties())
    {
        // Members starting with $ are for the editors, as the ranges of the models panel of VS Code
        if (model.name.toString().startsWith ("$"))
            continue;

        const auto* properties = model.value.getDynamicObject();

        if (properties == nullptr)
            return Result::fail ("model '" + model.name.toString() + "' must be an object of properties and sample values");

        DataTree tree ("model");

        {
            auto transaction = tree.beginTransaction();

            for (const auto& property : properties->getProperties())
                transaction.setProperty (property.name, property.value);
        }

        tree.addListener (this);

        names.add (model.name.toString());
        trees.push_back (tree);
        providers.push_back (std::make_unique<DataTreeBindingProvider> (tree));
    }

    return Result::ok();
}

void YuiToolModels::attachTo (YuiDocument& document) const
{
    for (int index = 0; index < names.size(); ++index)
        document.setModel (names[index], *providers[static_cast<std::size_t> (index)]);
}

bool YuiToolModels::setValue (const String& model, const String& property, const var& value)
{
    const auto index = names.indexOf (model);

    if (index < 0 || property.isEmpty())
        return false;

    const ScopedValueSetter<bool> setting (isSettingValue, true);
    trees[static_cast<std::size_t> (index)].beginTransaction().setProperty (property, value);

    return true;
}

var YuiToolModels::toJson() const
{
    auto* models = new DynamicObject();

    for (int index = 0; index < names.size(); ++index)
    {
        const auto& tree = trees[static_cast<std::size_t> (index)];
        auto* properties = new DynamicObject();

        for (int property = 0; property < tree.getNumProperties(); ++property)
        {
            const auto name = tree.getPropertyName (property);
            properties->setProperty (name, tree.getProperty (name));
        }

        models->setProperty (names[index], var (properties));
    }

    return var (models);
}

void YuiToolModels::propertyChanged (DataTree& tree, const Identifier& property)
{
    if (isSettingValue || onValueChanged == nullptr)
        return;

    for (std::size_t index = 0; index < trees.size(); ++index)
    {
        if (trees[index] == tree)
        {
            onValueChanged (names[static_cast<int> (index)], property.toString(), tree.getProperty (property));
            return;
        }
    }
}

//==============================================================================

void YuiToolProblem::print() const
{
    std::cerr << file.getFullPathName();

    if (line > 0)
        std::cerr << ":" << line << ":" << column;

    std::cerr << ": " << (isError ? "error" : "warning") << ": " << message << "\n";
}

var YuiToolProblem::toJson() const
{
    auto* record = new DynamicObject();
    record->setProperty ("file", file.getFullPathName());
    record->setProperty ("line", line);
    record->setProperty ("column", column);
    record->setProperty ("severity", isError ? "error" : "warning");
    record->setProperty ("message", message);

    return var (record);
}

std::vector<YuiToolProblem> findYuiToolProblems (const YuiDocument& document, const File& file, const Result& loadResult)
{
    std::vector<YuiToolProblem> problems;

    for (const auto& diagnostic : document.getDiagnostics())
    {
        problems.push_back ({ .file = diagnostic.source.isEmpty() ? file : file.getParentDirectory().getChildFile (diagnostic.source),
                              .line = diagnostic.line,
                              .column = diagnostic.column,
                              .isError = diagnostic.isError(),
                              .message = diagnostic.message });
    }

    const auto hasError = std::any_of (problems.begin(), problems.end(), [] (const YuiToolProblem& problem)
    {
        return problem.isError;
    });

    if (loadResult.failed() && ! hasError)
        problems.push_back ({ .file = file, .message = loadResult.getErrorMessage() });

    return problems;
}

namespace
{

// A file holding only declarations (styles, drawings, components) is written to be imported
bool hasNoRootComponent (const String& text, const YuiLoader& loader)
{
    const auto parsed = YuiParser::parse (text, [&loader] (const String& path)
    {
        return loader.getLoadedText (path);
    });

    return parsed.wasOk() && findYuiToolRootIndex (parsed.document) < 0;
}

// Loads a file as it is checked: with the text given in place of its own, and what only declares as an empty interface
Result loadForChecking (YuiLoader& loader, const File& file, const std::optional<String>& unsavedText)
{
    auto result = loader.loadFile (file);

    if (unsavedText.has_value())
        result = loader.reloadFromString (*unsavedText);

    if (result.failed())
    {
        // There is no interface to build from declarations alone, so they are checked as those of an empty one
        if (const auto text = loader.getLoadedText (file.getFileName()); text.has_value() && hasNoRootComponent (*text, loader))
            result = loader.reloadFromString (*text + "\nComponent {}\n");
    }

    return result;
}

} // namespace

std::vector<YuiToolProblem> findYuiToolFileProblems (const File& file, const std::optional<String>& unsavedText, const YuiToolModels& models)
{
    YuiDocument document;
    YuiLoader loader (document);
    models.attachTo (document);

    const auto result = loadForChecking (loader, file, unsavedText);

    return findYuiToolProblems (document, file, result);
}

//==============================================================================

namespace
{

constexpr int maximumMissingModelPasses = 40;

/** What an expression takes a value it reads to be. */
enum class Reading
{
    unknown,
    number,
    boolean,
    text
};

var sampleOf (Reading reading)
{
    if (reading == Reading::boolean)
        return false;

    if (reading == Reading::text)
        return String();

    return 0;
}

Reading readingOf (const ComponentProperty* property, bool ofElement = false)
{
    using Type = ComponentProperty::Type;

    if (property == nullptr)
        return Reading::unknown;

    switch (ofElement ? property->elementType : property->type)
    {
        case Type::number:
        case Type::integer:
            return Reading::number;

        case Type::boolean:
            return Reading::boolean;

        case Type::string:
        case Type::enumeration:
            return Reading::text;

        default:
            return Reading::unknown;
    }
}

/** How an expression reads a symbol: as what, and whether as a list of it. */
struct Usage
{
    Reading reading = Reading::unknown;
    bool isList = false;
    bool wasFound = false;
};

/** What the place a value goes to takes. */
struct Expected
{
    Reading reading = Reading::unknown;
    bool isList = false;
};

Expected expectedBy (const ComponentProperty* property)
{
    const auto isList = property != nullptr && property->type == ComponentProperty::Type::array;

    return { readingOf (property, isList), isList };
}

bool isTextConstant (const Expression& node)
{
    return node.getKind() == Expression::Kind::constant && node.getConstantValue().isString();
}

void findUsage (const Expression& node, const String& symbol, Expected expected, Usage& usage)
{
    using Kind = Expression::Kind;
    using Operator = ExpressionOperator;

    if (usage.wasFound)
        return;

    const auto findInInputs = [&] (Expected taken, int first = 0)
    {
        for (int index = first; index < node.getNumInputs(); ++index)
            findUsage (node.getInput (index), symbol, taken, usage);
    };

    switch (node.getKind())
    {
        case Kind::symbol:
            if (node.getName() == symbol)
                usage = { expected.reading, expected.isList, true };
            else if (node.getName().startsWith (symbol + "."))
                usage = { Reading::unknown, false, true };

            break;

        case Kind::index:
            findUsage (node.getInput (0), symbol, { expected.reading, false }, usage);

            if (usage.wasFound)
                usage.isList = true;
            else
                findUsage (node.getInput (1), symbol, { Reading::number }, usage);

            break;

        case Kind::unary:
            findInInputs ({ node.getOperator() == Operator::logicalNot ? Reading::boolean : Reading::number });
            break;

        case Kind::binary:
        {
            const auto op = node.getOperator();

            if (op == Operator::logicalAnd || op == Operator::logicalOr)
                findInInputs ({ Reading::boolean });
            else if (isTextConstant (node.getInput (0)) || isTextConstant (node.getInput (1)))
                findInInputs ({ Reading::text });
            else if (op == Operator::add && expected.reading == Reading::text)
                findInInputs ({ Reading::text });
            else
                findInInputs ({ Reading::number });

            break;
        }

        case Kind::ternary:
            findUsage (node.getInput (0), symbol, { Reading::boolean }, usage);
            findInInputs (expected, 1);
            break;

        case Kind::call:
            findInInputs ({ Reading::number });
            break;

        case Kind::method:
        case Kind::member:
            findUsage (node.getInput (0), symbol, {}, usage);
            findInInputs ({ Reading::number }, 1);
            break;

        case Kind::let:
        case Kind::output:
        case Kind::block:
            findInInputs (expected);
            break;

        case Kind::constant:
            break;
    }
}

/** A property a model lacks, with a value of the kind the document reads it as. */
struct MissingProperty
{
    String model;
    String property;
    var sample;
};

String findQuotedAfter (const String& message, const char* start)
{
    return message.contains (start) ? message.fromFirstOccurrenceOf (start, false, false).upToFirstOccurrenceOf ("'", false, false) : String();
}

void collectIds (const DataTree& element, StringArray& ids)
{
    if (element.hasProperty ("id"))
        ids.add (element.getProperty ("id").toString().unquoted());

    for (const auto& child : element)
        collectIds (child, ids);
}

// What a problem tells a model lacks, nothing when it is about something else
std::optional<MissingProperty> readMissingProperty (YuiDocument& document, const YuiDiagnostic& diagnostic, const StringArray& ids)
{
    // The names an expression reads that are neither components nor models
    static const StringArray reserved { "palette", "tokens", "parent", "root", "self", "colors", "bounds" };

    const auto& element = diagnostic.element;
    String name;
    Usage usage;

    if (element.isValid() && element.getType().toString() == "bind"
        && (diagnostic.message.contains ("unknown element '") || diagnostic.message.contains ("has no property '")))
    {
        // A link: the property it links tells what the one of the model is
        const auto target = element.getProperty ("target").toString();
        const auto* info = document.findTypeInfo (element.getParent().getType());
        const auto* linked = info != nullptr ? info->findProperty (Identifier (element.getProperty ("property").toString())) : nullptr;

        name = target.upToFirstOccurrenceOf ("[", false, false).trim();
        usage.reading = expectedBy (linked).reading;
        usage.isList = target.containsChar ('[') || expectedBy (linked).isList;
    }
    else if (name = findQuotedAfter (diagnostic.message, "Unknown symbol '"); name.isNotEmpty())
    {
        // An expression: how it uses the value tells, starting from the property it gives a value to
        const auto* info = element.isValid() ? document.findTypeInfo (element.getType()) : nullptr;
        const auto* set = info != nullptr && diagnostic.property.isValid() ? info->findProperty (diagnostic.property) : nullptr;
        const auto text = element.isValid() && diagnostic.property.isValid() ? element.getProperty (diagnostic.property).toString() : String();

        if (auto parsed = Expression::parse (text); parsed.wasOk())
            findUsage (parsed.getReference(), name.upToFirstOccurrenceOf (".", false, false) + "." + name.fromFirstOccurrenceOf (".", false, false).upToFirstOccurrenceOf (".", false, false), expectedBy (set), usage);
    }
    else
    {
        return std::nullopt;
    }

    const auto model = name.upToFirstOccurrenceOf (".", false, false);
    const auto property = name.fromFirstOccurrenceOf (".", false, false).upToFirstOccurrenceOf (".", false, false);

    if (! name.containsChar ('.') || model.isEmpty() || property.isEmpty() || reserved.contains (model) || ids.contains (model))
        return std::nullopt;

    auto sample = sampleOf (usage.reading);

    // The end of a repeat is given one turn, so what its body reads is found too
    if (element.isValid() && element.getType().toString() == "repeat" && ! usage.isList && usage.reading != Reading::boolean && usage.reading != Reading::text)
        sample = 1;

    return MissingProperty { model, property, usage.isList ? var (Array<var> { sample }) : sample };
}

/** A property found lacking, with the values it can be given: the one the document seems to
    read it as first, then the other kinds, tried in turn while the place reading it fails.

    The kinds are tried twice: with the other properties the failing place reads, which are
    often of the same kind, then alone.
*/
struct Lacking
{
    Lacking (const MissingProperty& missing)
        : model (missing.model)
        , property (missing.property)
        , candidates { missing.sample }
    {
        for (const auto& other : { var (0), var (false), var (String()), var (Array<var> { 0 }), var (Array<var> { String() }), var (Array<var> { false }) })
            if (JSON::toString (other, true) != JSON::toString (missing.sample, true))
                candidates.push_back (other);
    }

    // Moves to the next kind of value, back to the first once all were tried
    void tryNext()
    {
        if (++index < candidates.size())
            return;

        index = 0;
        ++numSweeps;
    }

    bool isTriedWithOthers() const noexcept { return numSweeps == 0; }

    bool isSettled() const noexcept { return numSweeps > 1; }

    String model;
    String property;
    std::vector<var> candidates;
    size_t index = 0;
    int numSweeps = 0;
};

// Where a text reads a property of a model, or -1
int findReference (const String& text, const Lacking& lacking)
{
    const auto isNameCharacter = [] (yup_wchar character)
    {
        return CharacterFunctions::isLetterOrDigit (character) || character == '_';
    };

    const auto name = lacking.model + "." + lacking.property;

    for (auto position = text.indexOf (name); position >= 0; position = text.indexOf (position + 1, name))
    {
        const auto before = position > 0 ? text[position - 1] : yup_wchar (' ');
        const auto after = text[position + name.length()];

        if (! isNameCharacter (before) && before != '.' && ! isNameCharacter (after))
            return position;
    }

    return -1;
}

// The text a problem is about: the value of the property it names, or all those of its element
String findTextOf (const YuiDiagnostic& diagnostic)
{
    const auto& element = diagnostic.element;

    if (! element.isValid())
        return {};

    if (diagnostic.property.isValid() && element.hasProperty (diagnostic.property))
        return element.getProperty (diagnostic.property).toString();

    StringArray values;

    for (int index = 0; index < element.getNumProperties(); ++index)
        values.add (element.getProperty (element.getPropertyName (index)).toString());

    return values.joinIntoString ("\n");
}

var listLacking (const std::vector<Lacking>& lacking)
{
    var listed (new DynamicObject());

    for (const auto& entry : lacking)
    {
        if (! listed.hasProperty (Identifier (entry.model)))
            listed.getDynamicObject()->setProperty (Identifier (entry.model), var (new DynamicObject()));

        listed[Identifier (entry.model)].getDynamicObject()->setProperty (Identifier (entry.property), entry.candidates[entry.index]);
    }

    return listed;
}

// Adds to the models of a file those found lacking, as one object in the shape of a models file
var mergeModels (const var& declared, const var& lacking)
{
    auto merged = declared.clone();

    for (const auto& model : lacking.getDynamicObject()->getProperties())
    {
        if (! merged.hasProperty (model.name))
            merged.getDynamicObject()->setProperty (model.name, var (new DynamicObject()));

        for (const auto& property : model.value.getDynamicObject()->getProperties())
            merged[model.name].getDynamicObject()->setProperty (property.name, property.value);
    }

    return merged;
}

} // namespace

YuiToolMissingModels findYuiToolMissingModels (const File& file, const YuiToolModels& models)
{
    YuiToolMissingModels missing;
    std::vector<Lacking> lacking;
    const auto declared = models.toJson();

    // An expression stops at the first name it can't read, so what lacks shows a few names at a time. A property
    // given a value of the wrong kind then fails where it is read, which is what tells the kind it must be
    for (int pass = 0; pass < maximumMissingModelPasses; ++pass)
    {
        YuiToolModels combined;
        combined.loadValues (mergeModels (declared, listLacking (lacking)));

        YuiDocument document;
        YuiLoader loader (document);
        combined.attachTo (document);

        const auto result = loadForChecking (loader, file, std::nullopt);

        StringArray ids;
        collectIds (document.getTree(), ids);

        bool foundMore = false;
        std::set<size_t> misread;
        missing.numErrorsLeft = 0;

        for (const auto& diagnostic : document.getDiagnostics())
        {
            if (! diagnostic.isError())
                continue;

            ++missing.numErrorsLeft;

            const auto isKnown = [&] (const MissingProperty& found)
            {
                return declared[Identifier (found.model)].hasProperty (Identifier (found.property))
                    || std::any_of (lacking.begin(), lacking.end(), [&found] (const Lacking& entry)
                {
                    return entry.model == found.model && entry.property == found.property;
                });
            };

            if (const auto found = readMissingProperty (document, diagnostic, ids); found.has_value() && ! isKnown (*found))
            {
                lacking.emplace_back (*found);
                foundMore = true;
                continue;
            }

            // The properties the failing text reads that can still be something else: those not tried
            // together yet, else the first of them
            const auto text = findTextOf (diagnostic);
            std::vector<size_t> together;
            auto first = lacking.size();
            auto firstPosition = std::numeric_limits<int>::max();

            for (size_t index = 0; index < lacking.size(); ++index)
            {
                const auto position = lacking[index].isSettled() ? -1 : findReference (text, lacking[index]);

                if (position < 0)
                    continue;

                if (lacking[index].isTriedWithOthers())
                    together.push_back (index);

                if (position < firstPosition)
                {
                    first = index;
                    firstPosition = position;
                }
            }

            if (! together.empty())
                misread.insert (together.begin(), together.end());
            else if (first < lacking.size())
                misread.insert (first);
        }

        if (result.failed() && missing.numErrorsLeft == 0)
            missing.numErrorsLeft = 1;

        if (foundMore)
            continue;

        // The last pass counts what is left with the values as they are
        if (misread.empty() || pass == maximumMissingModelPasses - 1)
            break;

        for (const auto index : misread)
            lacking[index].tryNext();
    }

    if (! lacking.empty())
        missing.models = listLacking (lacking);

    return missing;
}

int findYuiToolRootIndex (const DataTree& document)
{
    for (int index = 0; index < document.getNumChildren(); ++index)
    {
        const auto typeName = document.getChild (index).getType().toString();

        if (CharacterFunctions::isUpperCase (static_cast<yup_wchar> (typeName[0])))
            return index;
    }

    return -1;
}

//==============================================================================

namespace
{

// The names of the drawings or of the palettes a document declares, in a style block too
StringArray findDeclared (const DataTree& document, const Identifier& type)
{
    static const Identifier name ("name");
    static const Identifier style ("style");

    StringArray names;

    for (const auto& element : document)
    {
        if (element.getType() == type)
            names.add (element.getProperty (name).toString());

        if (element.getType() != style)
            continue;

        for (const auto& nested : element)
            if (nested.getType() == type)
                names.add (nested.getProperty (name).toString());
    }

    return names;
}

// A few widgets and a swatch of each role over the background, in the order of ThemePalette::Role
String makePaletteSample (const String& palette)
{
    String swatches;

    for (const auto& role : ThemePalette::getRoleNames())
        if (role != "background")
            swatches << "            Window { backgroundColor: palette." << role << "; flexGrow: 1; }\n";

    return String (R"yui(
Window { palette: NAME; backgroundColor: palette.background;
    Column { padding: 16; gap: 10;
        Label { text: "NAME"; preferredHeight: 24; }
        Row { gap: 10; preferredHeight: 32;
            TextButton { text: "Button"; flexGrow: 1; }
            ToggleButton { text: "Toggle"; checked: true; flexGrow: 1; }
        }
        Slider { value: 0.6; preferredHeight: 28; }
        ProgressBar { progress: 0.4; preferredHeight: 16; }
        TextEditor { text: "Text"; preferredHeight: 32; }
        Row { gap: 6; flexGrow: 1;
SWATCHES        }
    }
})yui")
        .replace ("NAME", palette)
        .replace ("SWATCHES", swatches);
}

// Gives a parsed document another root component, the one hosting what is previewed
DataTree withRoot (const DataTree& document, const DataTree& root)
{
    auto tree = document.clone();
    const auto previousRoot = findYuiToolRootIndex (tree);

    {
        auto transaction = tree.beginTransaction();

        if (previousRoot >= 0)
            transaction.removeChild (previousRoot);

        transaction.addChild (root.clone());
    }

    return tree;
}

} // namespace

YuiToolShown loadYuiToolTarget (YuiDocument& document, YuiLoader& loader, const File& input, YuiToolTarget target)
{
    YuiToolShown shown;
    shown.result = loader.loadFile (input);

    const auto parsed = YuiParser::parse (loader.getLoadedText (input.getFileName()).value_or (String()), [&loader] (const String& path)
    {
        return loader.getLoadedText (path);
    });

    const auto nothing = [&shown, &target] (const String& reason)
    {
        shown.status = YuiToolShown::Status::nothing;
        shown.message = reason;
        shown.target = target;
        return shown;
    };

    if (parsed.wasOk())
    {
        const auto hasRoot = findYuiToolRootIndex (parsed.document) >= 0;
        const auto isPreview = target.drawing.isNotEmpty() || target.palette.isNotEmpty() || target.hostType.isNotEmpty();
        const auto drawings = findDeclared (parsed.document, "drawing");
        const auto palettes = findDeclared (parsed.document, "palette");

        if (target.drawing.isNotEmpty() && ! drawings.contains (target.drawing))
            return nothing ("there is no drawing '" + target.drawing + "'");

        if (target.palette.isNotEmpty() && ! palettes.contains (target.palette))
            return nothing ("there is no palette '" + target.palette + "'");

        // A file of declarations shows the first thing it declares that can be drawn
        if (! hasRoot && ! isPreview)
        {
            target.drawing = drawings[0];

            if (target.drawing.isEmpty())
                target.palette = palettes[0];
        }

        String hostText;

        if (target.palette.isNotEmpty())
        {
            hostText = makePaletteSample (target.palette);
            shown.defaultSize = { 360.0f, 300.0f };
        }
        else if (target.drawing.isNotEmpty() || target.hostType.isNotEmpty())
        {
            hostText = (target.hostType.isNotEmpty() ? target.hostType : String ("Component")) + " { " + (target.drawing.isNotEmpty() ? "drawing: " + target.drawing + "; " : String()) + "}";
            shown.defaultSize = { 200.0f, 200.0f };
        }
        else if (! hasRoot)
        {
            return nothing ("the file has no component, drawing or palette");
        }

        if (hostText.isNotEmpty())
        {
            const auto host = YuiParser::parse (hostText);

            if (! host.wasOk() || host.document.getNumChildren() != 1)
            {
                shown.status = YuiToolShown::Status::invalidName;
                shown.target = target;
                return shown;
            }

            // The positions of the text only hold while its elements keep their place
            shown.result = document.load (withRoot (parsed.document, host.document.getChild (0)), hasRoot ? YuiSourceMap() : parsed.sourceMap);
        }
    }

    shown.target = target;
    return shown;
}

//==============================================================================

std::optional<Size<float>> parseYuiToolSize (const String& text)
{
    const Size<float> size { text.upToFirstOccurrenceOf ("x", false, true).getFloatValue(), text.fromFirstOccurrenceOf ("x", false, true).getFloatValue() };

    if (size.getWidth() < 1.0f || size.getHeight() < 1.0f)
        return std::nullopt;

    return size;
}

std::optional<bool> readYuiToolViewOption (int argc, char** argv, int& index, YuiToolViewOptions& options, YuiToolModels& models)
{
    const String argument (argv[index]);
    const auto hasValue = index + 1 < argc;

    if (argument == "--models" && hasValue)
        return models.load (argv[++index]);

    if (argument == "--drawing" && hasValue)
    {
        options.target.drawing = argv[++index];
        return true;
    }

    if (argument == "--palette" && hasValue)
    {
        options.target.palette = argv[++index];
        return true;
    }

    if (argument == "--as" && hasValue)
    {
        options.target.hostType = argv[++index];
        return true;
    }

    if ((argument == "--size" || argument == "--default-size") && hasValue)
    {
        const String value (argv[++index]);
        const auto size = parseYuiToolSize (value);

        if (! size.has_value())
        {
            std::cerr << "invalid size, expected <width>x<height>: " << value << "\n";
            return false;
        }

        (argument == "--size" ? options.size : options.fallbackSize) = *size;
        return true;
    }

    if (argument == "--theme" && hasValue && (String (argv[index + 1]) == "dark" || String (argv[index + 1]) == "light"))
    {
        options.mode = String (argv[++index]) == "light" ? ThemePalette::Mode::light : ThemePalette::Mode::dark;
        return true;
    }

    if (options.input == File() && ! argument.startsWithChar ('-'))
    {
        options.input = File::getCurrentWorkingDirectory().getChildFile (argument);
        return true;
    }

    return std::nullopt;
}

//==============================================================================

namespace
{

/** Fills the image behind the interface with the background of the palette its root is drawn with. */
class Backdrop final : public Component
{
public:
    explicit Backdrop (Component& rootToShow)
        : root (rootToShow)
    {
        addAndMakeVisible (root);
    }

    void paint (Graphics& g) override
    {
        g.setFillColor (ApplicationTheme::getGlobalTheme()->getPalette (root).getColor (ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        root.setBounds (getLocalBounds());
    }

private:
    Component& root;
};

} // namespace

YuiToolView::YuiToolView (const YuiToolViewOptions& options, const YuiToolModels& models)
    : input (options.input)
    , previousPalette (ApplicationTheme::getGlobalTheme()->getPalette())
{
    ApplicationTheme::getGlobalTheme()->setPalette (ThemePalette ({}, options.mode));

    models.attachTo (document);
    shown = loadYuiToolTarget (document, loader, input, options.target);

    auto* root = document.getRoot();

    if (shown.status != YuiToolShown::Status::loaded || shown.result.failed() || root == nullptr)
        return;

    // The size asked for, else the one the root asks for, else a default one
    auto size = options.size;

    if (size.getWidth() < 1.0f)
    {
        if (root->getWidth() >= 1.0f && root->getHeight() >= 1.0f)
            size = root->getSize();
        else
            size = options.fallbackSize.getWidth() >= 1.0f ? options.fallbackSize : shown.defaultSize;
    }

    backdrop = std::make_unique<Backdrop> (*root);
    backdrop->setSize (size);

    // Nothing is under a pointer in a component that is not visible
    backdrop->setVisible (true);
}

YuiToolView::~YuiToolView()
{
    // The interface goes before the theme it was styled with
    backdrop.reset();
    document.clear();

    ApplicationTheme::getGlobalTheme()->setPalette (previousPalette);
}

std::vector<YuiToolProblem> YuiToolView::findProblems() const
{
    return findYuiToolProblems (document, input, shown.result);
}

void YuiToolView::settle()
{
    auto* root = getRoot();
    auto* context = root != nullptr ? root->getStyleContext() : nullptr;

    if (context == nullptr)
        return;

    // A pass restyles what the size, a state or the pass before changed, which may start transitions, then
    // brings those running to their end, which may in turn change what other styles read
    for (int pass = 0; pass < 4; ++pass)
        context->refresh (3600.0);

    context->restyleNow();
}

//==============================================================================

std::unique_ptr<GraphicsContext> createYuiToolGraphicsContext()
{
    // Drawing needs a device of the GPU but no window, which only some platforms give
#if YUP_MAC
    return GraphicsContext::createContext (GpuPlatform::Metal, {});
#elif YUP_WINDOWS
    GraphicsContext::Options options;
    options.allowHeadlessRendering = true;

    return GraphicsContext::createContext (GpuPlatform::Direct3D, options);
#else
    return nullptr;
#endif
}

bool encodeYuiToolPng (const Image& image, MemoryBlock& data)
{
    if (! image.isValid())
        return false;

    // The writer owns the stream, which trims the block to what was written when it goes
    PngImageFormatWriter writer (new MemoryOutputStream (data, false), image.getPixelFormat());
    return writer.writeImage (image);
}
