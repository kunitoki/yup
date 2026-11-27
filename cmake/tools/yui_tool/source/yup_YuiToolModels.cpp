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

#include "yup_YuiToolModels.h"

using namespace yup;

//==============================================================================

YuiToolModels::~YuiToolModels()
{
    for (auto& tree : trees)
        tree.removeListener (this);
}

Result YuiToolModels::load (const String& path)
{
    const auto file = File::getCurrentWorkingDirectory().getChildFile (path);

    if (! file.existsAsFile())
        return Result::fail ("can't read '" + path + "'");

    if (const auto result = loadFile (file); result.failed())
        return Result::fail (file.getFullPathName() + ": error: " + result.getErrorMessage());

    return Result::ok();
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

File findYuiToolModelsFile (const File& document)
{
    return document.getSiblingFile (document.getFileNameWithoutExtension() + ".models.json");
}
