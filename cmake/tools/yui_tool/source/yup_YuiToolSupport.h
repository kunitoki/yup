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

#include <yup_gui/yup_gui.h>

#include <memory>
#include <vector>

//==============================================================================
/** The models the documents bind to.

    They belong to the application, which sets them with YuiDocument::setModel, so their
    properties are declared to the tool in a JSON file: an object with a member for each model,
    itself an object giving each property a sample value.
*/
class YuiToolModels
{
public:
    /** Reads the models of a file given on the command line, printing why when it can't. */
    bool load (const yup::String& path);

    /** Sets the models on a document, which they must outlive. */
    void attachTo (yup::YuiDocument& document) const;

private:
    yup::StringArray names;
    std::vector<std::unique_ptr<yup::DataTreeBindingProvider>> providers;
};

//==============================================================================
/** A problem of a document, in the file it was found in. */
struct YuiToolProblem
{
    yup::File file;
    int line = 0;
    int column = 0;
    bool isError = true;
    yup::String message;

    /** Prints the problem as `path:line:column: error: message`. */
    void print() const;

    /** Returns the problem as an object with its file, line, column, severity and message. */
    yup::var toJson() const;
};

/** Returns the problems of a document loaded from a file, the failure of the load included. */
std::vector<YuiToolProblem> findYuiToolProblems (const yup::YuiDocument& document, const yup::File& file, const yup::Result& loadResult);

/** Returns the index of the root component of a parsed document, or -1 when it only holds declarations. */
int findYuiToolRootIndex (const yup::DataTree& document);

//==============================================================================
/** What of a document is shown: its interface, or one of its declarations hosted by a component. */
struct YuiToolTarget
{
    /** The drawing painting the host. */
    yup::String drawing;

    /** The palette shown with a few widgets and a swatch of each role. */
    yup::String palette;

    /** The type of the component hosting the drawing or shown on its own, a plain Component when empty. */
    yup::String hostType;
};

/** The outcome of loading the target of a file. */
struct YuiToolShown
{
    enum class Status
    {
        loaded,     ///< The document was loaded, which may have failed.
        nothing,    ///< There is nothing to show, for the reason of the message.
        invalidName ///< A name of the target can't be a name of the language.
    };

    Status status = Status::loaded;
    yup::Result result = yup::Result::ok();
    yup::String message;

    /** The target shown: the first declaration of a file with no interface when none was asked. */
    YuiToolTarget target;

    /** The size to show it at when nothing asks for one. */
    yup::Size<float> defaultSize { 640.0f, 400.0f };
};

/** Loads a file into a document, showing a target of it. */
YuiToolShown loadYuiToolTarget (yup::YuiDocument& document, yup::YuiLoader& loader, const yup::File& input, YuiToolTarget target);
