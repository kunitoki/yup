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
    for (int i = 0; i < tree.getNumProperties(); ++i)
    {
        const auto name = tree.getPropertyName (i);
        const auto value = tree.getProperty (name);

        auto type = ComponentProperty::Type::any;

        if (value.isBool())
            type = ComponentProperty::Type::boolean;
        else if (value.isInt() || value.isInt64() || value.isDouble())
            type = ComponentProperty::Type::number;
        else if (value.isString())
            type = ComponentProperty::Type::string;

        properties.push_back ({ name, type });
    }

    tree.addListener (this);
}

DataTreeBindingProvider::~DataTreeBindingProvider()
{
    tree.removeListener (this);
}

//==============================================================================

std::vector<BindingProvider::Property> DataTreeBindingProvider::getProperties() const
{
    return properties;
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
    if (undoManager != nullptr)
        undoManager->beginNewTransaction ("Change " + name.toString());
}

//==============================================================================

void DataTreeBindingProvider::propertyChanged (DataTree& changedTree, const Identifier& property)
{
    if (changedTree == tree)
        notifyValueChanged (property);
}

} // namespace yup
