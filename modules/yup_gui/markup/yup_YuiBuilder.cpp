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

namespace
{

bool isComponentElement (const DataTree& element)
{
    const auto type = element.getType().toString();
    return type.isNotEmpty() && CharacterFunctions::isUpperCase (type[0]);
}

} // namespace

//==============================================================================

YuiBuilder::YuiBuilder()
{
    registerBuiltInComponentTypes();

    bindings.setComponentResolver ([this] (const String& id)
    {
        return findById (id);
    });
}

YuiBuilder::~YuiBuilder()
{
    clear();
}

//==============================================================================

Result YuiBuilder::build (const DataTree& document)
{
    clear();

    auto root = document;

    if (document.getType().toString() == "yui")
    {
        root = {};

        for (const auto& element : document)
        {
            if (isComponentElement (element))
            {
                root = element;
                break;
            }
        }
    }

    if (! root.isValid() || ! isComponentElement (root))
        return Result::fail ("The document has no root component");

    if (auto result = createComponent (root, nullptr); result.failed())
    {
        clear();
        return result;
    }

    // The tree exists before any value is applied, so expressions can reference any element
    for (const auto& [component, element] : elements)
        applyProperties (*component, element);

    return Result::ok();
}

void YuiBuilder::clear()
{
    bindings.clear();
    bindings.clearDiagnostics();
    elements.clear();

    // Children go first, so that no parent is destroyed while still holding them
    while (! components.empty())
        components.pop_back();

    diagnostics.clear();
}

//==============================================================================

Component* YuiBuilder::getRoot() const noexcept
{
    return components.empty() ? nullptr : components.front().get();
}

Component* YuiBuilder::findById (const String& id) const
{
    for (const auto& component : components)
        if (component->getComponentID() == id)
            return component.get();

    return nullptr;
}

//==============================================================================

Result YuiBuilder::createComponent (const DataTree& element, Component* parent)
{
    const auto typeName = element.getType();
    const auto* typeInfo = ComponentTypeRegistry::getInstance().findType (typeName);

    if (typeInfo == nullptr)
        return Result::fail ("Unknown component type '" + typeName.toString() + "'");

    auto created = typeInfo->createInstance();

    if (created == nullptr)
        return Result::fail ("The component type '" + typeName.toString() + "' can't be created from .yui");

    auto& component = *created;
    components.push_back (std::move (created));

    if (parent != nullptr)
        parent->addAndMakeVisible (component);

    static const Identifier idProperty ("id");
    static const Identifier classProperty ("class");

    // Identity comes first, so that expressions can find the component while the tree is built
    for (int i = 0; i < element.getNumProperties(); ++i)
    {
        const auto name = element.getPropertyName (i);

        if (name == idProperty || name == classProperty)
            applyProperty (component, name, element.getProperty (name).toString());
    }

    elements.emplace_back (&component, element);

    for (const auto& child : element)
    {
        if (! isComponentElement (child))
        {
            diagnostics.add ("'" + child.getType().toString() + "' is not a component type");
            continue;
        }

        if (auto result = createComponent (child, &component); result.failed())
            return result;
    }

    return Result::ok();
}

void YuiBuilder::applyProperties (Component& component, const DataTree& element)
{
    static const Identifier idProperty ("id");
    static const Identifier classProperty ("class");

    for (int i = 0; i < element.getNumProperties(); ++i)
    {
        const auto name = element.getPropertyName (i);

        if (name == idProperty || name == classProperty)
            continue;

        applyProperty (component, name, element.getProperty (name).toString());
    }
}

void YuiBuilder::applyProperty (Component& component, const Identifier& name, const String& valueText)
{
    static const Identifier idProperty ("id");
    static const Identifier classProperty ("class");

    if (name == idProperty)
    {
        component.setComponentID (valueText.unquoted().trim());
        return;
    }

    if (name == classProperty)
    {
        Array<Identifier> classes;

        for (const auto& styleClass : StringArray::fromTokens (valueText.unquoted(), " ,", ""))
            if (styleClass.isNotEmpty())
                classes.add (Identifier (styleClass));

        component.setStyleClasses (classes);
        return;
    }

    const auto* descriptor = component.getTypeInfo().findProperty (name);
    const auto where = component.getTypeInfo().getTypeName().toString() + "." + name.toString();

    if (descriptor != nullptr && descriptor->isStateBacked() && descriptor->setter == nullptr)
    {
        diagnostics.add (where + ": the property is read-only");
        return;
    }

    if (descriptor != nullptr && descriptor->type == ComponentProperty::Type::enumeration)
    {
        const auto value = valueText.unquoted().trim();

        if (! descriptor->enumValues.contains (value))
            diagnostics.add (where + ": expected one of " + descriptor->enumValues.joinIntoString (", "));
        else
            component.setProperty (name, value);

        return;
    }

    std::optional<ComponentProperty::Type> type;

    if (descriptor != nullptr)
    {
        switch (descriptor->type)
        {
            case ComponentProperty::Type::number:
            case ComponentProperty::Type::integer:
            case ComponentProperty::Type::boolean:
            case ComponentProperty::Type::string:
            case ComponentProperty::Type::color:   type = descriptor->type; break;
            case ComponentProperty::Type::any:     break;

            default:
                diagnostics.add (where + ": properties of this type can't be set from .yui yet");
                return;
        }
    }

    if (auto result = bindings.bind (component, name, valueText, type); result.failed())
        diagnostics.add (where + ": " + result.getErrorMessage());
}

} // namespace yup
