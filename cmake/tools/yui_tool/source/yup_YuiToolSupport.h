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
#include <optional>
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

    /** Reads the models of a file given on the command line, printing why when it can't. */
    bool load (const yup::String& path);

    /** Reads the models of a file, failing with the reason when it can't. */
    yup::Result loadFile (const yup::File& file);

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

/** Returns the problems of a file, checked as `verify` and `lint` check it.

    @param unsavedText The text to check in place of the one of the file, when given.
*/
std::vector<YuiToolProblem> findYuiToolFileProblems (const yup::File& file, const std::optional<yup::String>& unsavedText, const YuiToolModels& models);

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

//==============================================================================
/** What to show of a file and how, as the options of `render` and `inspect` say. */
struct YuiToolViewOptions
{
    yup::File input;
    YuiToolTarget target;

    /** The size to show it at in logical pixels, none while it has no width. */
    yup::Size<float> size;

    /** The size used when neither `size` nor the root gives one, none while it has no width. */
    yup::Size<float> fallbackSize;

    yup::ThemePalette::Mode mode = yup::ThemePalette::Mode::dark;
};

/** Reads a size written as `<width>x<height>`, both at least 1. */
std::optional<yup::Size<float>> parseYuiToolSize (const yup::String& text);

/** Reads the argument at an index when it is one of the options saying what to show of a file and
    how, moving the index past its value. What is wrong with it is printed.

    @returns nothing when the argument is none of these options, otherwise whether it is valid.
*/
std::optional<bool> readYuiToolViewOption (int argc, char** argv, int& index, YuiToolViewOptions& options, YuiToolModels& models);

//==============================================================================
/** A target of a file loaded and laid out at its size with no window: what `render` draws and
    `inspect` describes.

    The theme is in the mode asked for while the view lives, and as it was before once it is gone.
*/
class YuiToolView
{
public:
    /** Loads what the options ask for. The models must outlive the view. */
    YuiToolView (const YuiToolViewOptions& options, const YuiToolModels& models);
    ~YuiToolView();

    /** Returns the outcome of the load. */
    const YuiToolShown& getShown() const noexcept { return shown; }

    /** Returns true when the load gave an interface, which is then laid out. */
    bool hasInterface() const noexcept { return backdrop != nullptr; }

    /** Returns the problems of the document. */
    std::vector<YuiToolProblem> findProblems() const;

    /** Returns the file shown. */
    const yup::File& getInput() const noexcept { return input; }

    /** Returns the document of the interface. */
    yup::YuiDocument& getDocument() noexcept { return document; }

    /** Returns the root of the interface, or nullptr without one. */
    yup::Component* getRoot() const noexcept { return hasInterface() ? document.getRoot() : nullptr; }

    /** Returns what is drawn, the interface over the background of its palette, or nullptr without one. */
    yup::Component* getComponent() const noexcept { return backdrop.get(); }

    /** Brings the styles up to date with the size and the states of the interface, and their
        transitions to their end.

        A view is styled when it is loaded, before it has its size: what its styles read of that
        size, or of a state set since, only shows once this ran.
    */
    void settle();

private:
    yup::File input;
    yup::ThemePalette previousPalette;
    yup::YuiDocument document;
    yup::YuiLoader loader { document };
    YuiToolShown shown;
    std::unique_ptr<yup::Component> backdrop;
};

/** Describes the layout of a view as `inspect` prints it: its components with their place, and
    what looks wrong with them.
*/
yup::var describeYuiToolLayout (YuiToolView& view);

/** Returns the component a description of the layout gives a path, or nullptr. */
yup::Component* findYuiToolComponent (YuiToolView& view, const yup::String& path);

//==============================================================================
/** Describes a component type as `schema` does.

    @param includeInherited True to list the properties and signals of its bases too, each with
                            the type it comes from.
*/
yup::var describeYuiToolComponentType (const yup::ComponentTypeInfo& info, bool includeInherited);

/** Creates what draws with no window, or nullptr on a platform that has no such device. */
std::unique_ptr<yup::GraphicsContext> createYuiToolGraphicsContext();

/** Encodes an image as a PNG. */
bool encodeYuiToolPng (const yup::Image& image, yup::MemoryBlock& data);
