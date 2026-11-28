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

#include "yup_YuiToolMissingModels.h"
#include "yup_YuiToolCheck.h"

#include <algorithm>
#include <limits>
#include <optional>
#include <set>
#include <vector>

using namespace yup;

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
std::optional<MissingProperty> readMissingProperty (YuiDocument& document, const DataTreeDiagnostic& diagnostic, const StringArray& ids)
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
String findTextOf (const DataTreeDiagnostic& diagnostic)
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

        if (combined.loadValues (mergeModels (declared, listLacking (lacking))).failed())
        {
            jassertfalse;
            return missing;
        }

        YuiDocument document;
        YuiLoader loader (document);
        combined.attachTo (document);

        const auto result = loadYuiToolFileForChecking (document, loader, file);

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
