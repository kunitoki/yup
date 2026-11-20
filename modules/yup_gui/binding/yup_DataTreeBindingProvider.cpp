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

DataTreeBindingProvider::DataTreeBindingProvider (DataTree treeToUse, UndoManager* undoManagerToUse)
    : tree (std::move (treeToUse))
    , undoManager (undoManagerToUse)
{
    tree.addListener (this);
}

ComponentProperty::Type DataTreeBindingProvider::inferType (const var& value)
{
    if (value.isBool())
        return ComponentProperty::Type::boolean;

    if (value.isInt() || value.isInt64() || value.isDouble())
        return ComponentProperty::Type::number;

    if (value.isString())
        return ComponentProperty::Type::string;

    return ComponentProperty::Type::any;
}

DataTreeBindingProvider::~DataTreeBindingProvider()
{
    activeGesture.reset();
    tree.removeListener (this);
}

//==============================================================================

BindingProvider::Property DataTreeBindingProvider::makeProperty (const Identifier& name, const var& value)
{
    if (const auto* elements = value.getArray())
    {
        auto elementType = elements->isEmpty() ? ComponentProperty::Type::number : inferType ((*elements)[0]);

        for (const auto& element : *elements)
            if (inferType (element) != elementType)
                elementType = ComponentProperty::Type::any;

        return { name, ComponentProperty::Type::array, elementType };
    }

    return { name, inferType (value) };
}

std::vector<BindingProvider::Property> DataTreeBindingProvider::getProperties() const
{
    std::vector<Property> result;
    result.reserve (static_cast<std::size_t> (tree.getNumProperties()));

    for (int i = 0; i < tree.getNumProperties(); ++i)
    {
        const auto name = tree.getPropertyName (i);
        result.push_back (makeProperty (name, tree.getProperty (name)));
    }

    return result;
}

std::optional<BindingProvider::Property> DataTreeBindingProvider::findProperty (const Identifier& name) const
{
    if (! tree.hasProperty (name))
        return std::nullopt;

    return makeProperty (name, tree.getProperty (name));
}

var DataTreeBindingProvider::getValue (const Identifier& name) const
{
    return tree.getProperty (name);
}

void DataTreeBindingProvider::setValue (const Identifier& name, const var& value)
{
    if (tree.getProperty (name) == value)
        return;

    tree.beginTransaction (undoManager).setProperty (name, value);
}

void DataTreeBindingProvider::beginGesture (const Identifier& name)
{
    activeGesture.reset();

    if (undoManager != nullptr)
        activeGesture.emplace (*undoManager, "Change " + name.toString());
}

void DataTreeBindingProvider::endGesture (const Identifier&)
{
    activeGesture.reset();
}

//==============================================================================

void DataTreeBindingProvider::propertyChanged (DataTree& changedTree, const Identifier& property)
{
    if (changedTree == tree)
        notifyValueChanged (property);
}

} // namespace yup
