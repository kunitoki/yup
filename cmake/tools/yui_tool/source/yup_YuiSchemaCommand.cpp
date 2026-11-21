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
