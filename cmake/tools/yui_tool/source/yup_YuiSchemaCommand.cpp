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
#include "yup_YuiToolSupport.h"

#include <iostream>
#include <map>
#include <set>

using namespace yup;

namespace
{

// Bumped when a field is removed or changes meaning, not when one is added
constexpr int schemaVersion = 1;

const char* getTypeName (ComponentProperty::Type type)
{
    switch (type)
    {
        case ComponentProperty::Type::number:
            return "number";

        case ComponentProperty::Type::integer:
            return "integer";

        case ComponentProperty::Type::boolean:
            return "boolean";

        case ComponentProperty::Type::string:
            return "string";

        case ComponentProperty::Type::color:
            return "color";

        case ComponentProperty::Type::point:
            return "point";

        case ComponentProperty::Type::rectangle:
            return "rectangle";

        case ComponentProperty::Type::transform:
            return "transform";

        case ComponentProperty::Type::enumeration:
            return "enumeration";

        case ComponentProperty::Type::font:
            return "font";

        case ComponentProperty::Type::drawing:
            return "drawing";

        case ComponentProperty::Type::array:
            return "array";

        case ComponentProperty::Type::any:
            break;
    }

    return "any";
}

// Only the defaults a `.yui` file can spell as a literal are listed
var describeDefault (const ComponentProperty& property)
{
    const auto& value = property.defaultValue;

    if (property.type == ComponentProperty::Type::color && (value.isInt() || value.isInt64()))
        return VariantConverter<Color>::fromVar (value).toString();

    if (value.isBool() || value.isInt() || value.isInt64() || value.isDouble() || value.isString())
        return value;

    return {};
}

void describeType (DynamicObject& record, ComponentProperty::Type type, ComponentProperty::Type elementType)
{
    record.setProperty ("type", getTypeName (type));

    if (type == ComponentProperty::Type::array && elementType != ComponentProperty::Type::any)
        record.setProperty ("elementType", getTypeName (elementType));
}

var describeProperty (const ComponentProperty& property)
{
    auto* record = new DynamicObject();
    record->setProperty ("name", property.name.toString());
    describeType (*record, property.type, property.elementType);

    if (const auto defaultValue = describeDefault (property); ! defaultValue.isVoid())
        record->setProperty ("default", defaultValue);

    if (! property.enumValues.isEmpty())
        record->setProperty ("values", var (property.enumValues));

    record->setProperty ("state", property.isState);
    record->setProperty ("styleable", property.isStyleable);
    record->setProperty ("inherited", property.isInherited);
    record->setProperty ("readOnly", property.isReadOnly());

    if (property.description.isNotEmpty())
        record->setProperty ("description", property.description);

    return var (record);
}

var describeSignal (const ComponentSignal& signal)
{
    Array<var> arguments;

    for (const auto& argument : signal.arguments)
    {
        auto* record = new DynamicObject();
        record->setProperty ("name", argument.name.toString());
        describeType (*record, argument.type, argument.elementType);

        arguments.add (var (record));
    }

    auto* record = new DynamicObject();
    record->setProperty ("name", signal.name.toString());

    if (signal.description.isNotEmpty())
        record->setProperty ("description", signal.description);

    record->setProperty ("arguments", var (arguments));

    return var (record);
}

} // namespace

// The properties and signals are the ones the type declares: the inherited ones are those of its base,
// listed here too when asked, each with the type it comes from
var describeYuiToolComponentType (const ComponentTypeInfo& info, bool includeInherited)
{
    Array<var> properties;
    Array<var> signals;
    std::set<String> listedProperties;
    std::set<String> listedSignals;

    for (const auto* type = &info; type != nullptr; type = includeInherited ? type->getBaseTypeInfo() : nullptr)
    {
        for (const auto& property : type->getOwnProperties())
        {
            if (! listedProperties.insert (property.name.toString()).second)
                continue;

            auto described = describeProperty (property);

            if (type != &info)
                described.getDynamicObject()->setProperty ("from", type->getTypeName().toString());

            properties.add (described);
        }

        for (const auto& signal : type->getOwnSignals())
        {
            if (! listedSignals.insert (signal.name.toString()).second)
                continue;

            auto described = describeSignal (signal);

            if (type != &info)
                described.getDynamicObject()->setProperty ("from", type->getTypeName().toString());

            signals.add (described);
        }
    }

    auto* record = new DynamicObject();
    record->setProperty ("name", info.getTypeName().toString());

    if (const auto* base = info.getBaseTypeInfo())
        record->setProperty ("base", base->getTypeName().toString());

    record->setProperty ("instantiable", info.hasFactory());
    record->setProperty ("properties", var (properties));
    record->setProperty ("signals", var (signals));

    return var (record);
}

namespace
{

var describeDrawingTopic()
{
    const auto grammar = YuiDocument::createSchema();
    Array<var> elements;

    for (const auto& element : DrawingStyle::getElementDescriptions())
    {
        auto* record = new DynamicObject();
        record->setProperty ("type", element.type);
        record->setProperty ("role", element.role);

        if (! element.geometry.isEmpty())
            record->setProperty ("geometry", var (element.geometry));

        record->setProperty ("properties", var (element.properties));

        // What it holds is told by the grammar, which a document is read with
        if (const Identifier type (element.type); grammar != nullptr && grammar->hasNodeType (type))
            if (const auto children = grammar->getChildConstraints (type); children.allowsChildren() && ! children.allowedTypes.isEmpty())
                record->setProperty ("holds", var (children.allowedTypes));

        elements.add (var (record));
    }

    auto* topic = new DynamicObject();
    topic->setProperty ("elements", var (elements));
    topic->setProperty ("shapePaint", var (DrawingStyle::getShapePaintProperties()));
    topic->setProperty ("notes", "A shape takes its geometry, its properties and those of shapePaint. In geometry, a name followed by '=' has that default. "
                                 "A 'use' also takes the properties its drawing declares. Every value is an expression.");
    topic->setProperty ("docs", "docs/ui/yui/drawing.md");

    return var (topic);
}

var describeStylesTopic()
{
    auto* topic = new DynamicObject();
    topic->setProperty ("declarations", var (StyleSheet::getDeclarationNames()));
    topic->setProperty ("pseudoClasses", var (StyleSelector::getPseudoClassNames()));
    topic->setProperty ("paletteRoles", var (ThemePalette::getRoleNames()));
    topic->setProperty ("notes", "A rule sets the declarations listed here and the styleable properties of the components it selects, which the "
                                 "components topic lists for a type. A selector is made of a type, #id, .class, :pseudoClass and [property=value], "
                                 "joined by a space for a descendant or '>' for a child. A state property of a type, such as 'checked', is a pseudo-class too.");
    topic->setProperty ("docs", "docs/ui/yui/styles.md");

    return var (topic);
}

var describeExpressionsTopic()
{
    const auto context = PropertyExpression::createContext();
    const auto known = context.getFunctions();
    std::set<String> signatures;
    StringArray functions;
    std::map<String, StringArray> members;

    for (const auto& function : known)
        signatures.insert (function.getSignature());

    for (const auto& function : known)
    {
        const auto& name = function.name;
        const auto signature = function.getSignature();

        // A function of doubles that is also one of floats says nothing more than that one
        if (signature.contains ("double") && signatures.contains (signature.replace ("double", "float")))
            continue;

        // Operators, conversions and what the library keeps for itself are not called by name
        if (name.startsWith ("operator") || name.startsWithChar ('@') || name.startsWithChar ('_') || name.substring (1).startsWithChar ('_'))
            continue;

        if (! name.startsWithChar ('.'))
        {
            functions.add (signature);
            continue;
        }

        if (function.parameterTypes.empty())
            continue;

        // A member or a method is told for the type it belongs to, which is its first parameter
        StringArray parameters;

        for (size_t index = 1; index < function.parameterTypes.size(); ++index)
            parameters.add (function.parameterTypes[index]->getName());

        const auto call = parameters.isEmpty() ? String() : " (" + parameters.joinIntoString (", ") + ")";

        members[function.parameterTypes.front()->getName()].add (name.substring (1) + call + " -> " + function.resultType->getName());
    }

    functions.sort (false);

    auto* memberRecord = new DynamicObject();

    for (auto& [type, signatures] : members)
    {
        signatures.sort (false);
        memberRecord->setProperty (type, var (signatures));
    }

    auto* enumerationRecord = new DynamicObject();

    for (const auto& [type, values] : context.getEnumerations())
        enumerationRecord->setProperty (type, var (values));

    // The named colors are too many to list: they are those of CSS
    StringArray constants;

    for (const auto& constant : context.getConstantNames())
        if (! constant.startsWith ("colors."))
            constants.add (constant);

    auto* topic = new DynamicObject();
    topic->setProperty ("functions", var (functions));
    topic->setProperty ("members", var (memberRecord));
    topic->setProperty ("enumerations", var (enumerationRecord));
    topic->setProperty ("constants", var (constants));
    topic->setProperty ("notes", "A member is read as value.name and a method called as value.name (arguments), on a value of the type they are listed for. "
                                 "The value of an enumeration is written by its name where one is expected. colors.<name> is a named color of CSS, such as "
                                 "colors.orange. An expression also reads the properties of the component it is in, those of the components and models "
                                 "it names as id.property, palette.<role>, and tokens.");
    topic->setProperty ("docs", "docs/ui/yui/language.md");

    return var (topic);
}

} // namespace

var describeYuiToolSchemaTopic (const String& topic)
{
    if (topic == "drawing")
        return describeDrawingTopic();

    if (topic == "styles")
        return describeStylesTopic();

    if (topic == "expressions")
        return describeExpressionsTopic();

    return {};
}

int runYuiSchemaCommand (int argc, char** argv)
{
    File output;

    for (int i = 2; i < argc; ++i)
    {
        const String argument (argv[i]);

        if (argument == "--output" && i + 1 < argc)
        {
            output = File::getCurrentWorkingDirectory().getChildFile (argv[++i]);
            continue;
        }

        std::cerr << "invalid argument: " << argv[i] << "\n";
        return 2;
    }

    registerBuiltInComponentTypes();

    Array<var> types;

    for (const auto* info : ComponentTypeRegistry::getInstance().getRegisteredTypes())
        types.add (describeYuiToolComponentType (*info, false));

    auto* schema = new DynamicObject();
    schema->setProperty ("version", schemaVersion);
    schema->setProperty ("types", var (types));
    schema->setProperty ("paletteRoles", var (ThemePalette::getRoleNames()));

    Array<var> namedColors;

    for (const auto& named : Colors::getNamedColors())
        namedColors.add (String (named.name));

    schema->setProperty ("namedColors", var (namedColors));

    const auto text = JSON::toString (var (schema)) + "\n";

    if (output == File())
    {
        std::cout << text.toStdString();
        return 0;
    }

    if (! output.replaceWithText (text, false, false, "\n"))
    {
        std::cerr << "can't write '" << output.getFullPathName() << "'\n";
        return 2;
    }

    return 0;
}
