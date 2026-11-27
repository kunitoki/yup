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

#pragma once

#include <yup_gui_markup/yup_gui_markup.h>

#include <functional>
#include <memory>
#include <vector>

//==============================================================================
/** The models the documents bind to.

    They belong to the application, which sets them with YuiDocument::setModel, so their
    properties are declared to the tool in a JSON file: an object with a member for each model,
    itself an object giving each property a sample value. Members whose name starts with `$` are
    left to the editors.

    The values stay live: the preview changes them with setValue, reads them back with toJson,
    and hears about the ones the interface writes through onValueChanged.
*/
class YuiToolModels : private yup::DataTreeListener
{
public:
    YuiToolModels() = default;
    ~YuiToolModels() override;

    /** Reads the models of a file given on the command line, failing with what to print when it can't. */
    yup::Result load (const yup::String& path);

    /** Reads the models of a file, failing with the reason when it can't. */
    yup::Result loadFile (const yup::File& file);

    /** Adds the models of an object in the shape of a models file, failing with the reason when it is not one. */
    yup::Result loadValues (const yup::var& declared);

    /** Sets the models on a document, which they must outlive. */
    void attachTo (yup::YuiDocument& document) const;

    /** Changes the value of a property of a model, returning false when neither exists. */
    bool setValue (const yup::String& model, const yup::String& property, const yup::var& value);

    /** Returns the current values, in the shape of the file they were loaded from. */
    yup::var toJson() const;

    /** Called with the model, the property and the value when the interface changes a value. */
    std::function<void (const yup::String&, const yup::String&, const yup::var&)> onValueChanged;

private:
    void propertyChanged (yup::DataTree& tree, const yup::Identifier& property) override;

    yup::StringArray names;
    std::vector<yup::DataTree> trees;
    std::vector<std::unique_ptr<yup::DataTreeBindingProvider>> providers;
    bool isSettingValue = false;
};

/** Returns the file declaring the models of a document that is given none: the `<name>.models.json` next to
    `<name>.yui`, whether it is there or not.
*/
yup::File findYuiToolModelsFile (const yup::File& document);
