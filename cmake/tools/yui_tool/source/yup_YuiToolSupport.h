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

#include <yup_audio_gui/yup_audio_gui.h>
#include <yup_gui_markup/yup_gui_markup.h>

#include <functional>
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

#if YUP_WASM
/** Notes the files a document names that the tool was not given, in the tool built for the web.

    That tool is a module reading the files its host gave it in memory. It calls this with each
    document it loaded from one of them, and the host then asks for the files that document reads
    beside its texts and that are not there, to give those it has. It is defined with the entry
    points of the module, see web/main.cpp.
*/
void noteYuiToolMissingFiles (yup::YuiDocument& document);
#endif

/** The files a build embeds for the documents an application loads with YuiLoader::loadResource. */
struct YuiToolEmbedded
{
    /** The folder the paths of the files are relative to. */
    yup::File base;

    /** The paths of the files from that folder, separated by slashes. A shader is listed as the
        source its document names, which stands for the bundles a build compiles from it.
    */
    yup::StringArray paths;
};

/** Returns the problems of a file, checked as `verify` and `lint` check it.

    What a file with an interface needs is checked whether it is used or not: every shader it
    declares is loaded, from its source where the tool compiles shaders, and its scenes run the
    frame a window showing them starts with, where they build what they left to it. A file of
    declarations is checked without them: what it needs is read from the folder of the file
    importing it, so it is checked with that file.

    @param unsavedText The text to check in place of the one of the file, when given.
    @param embedded    The files a build embeds, to check the file as an application loads it from
                       them: as the resource of its path, reading nothing but the files listed.
                       What it reads that is in their folder without being listed is a problem,
                       and so is a file that is not one of them. Null to read the folder of the file.
*/
std::vector<YuiToolProblem> findYuiToolFileProblems (const yup::File& file,
                                                     const std::optional<yup::String>& unsavedText,
                                                     const YuiToolModels& models,
                                                     const YuiToolEmbedded* embedded = nullptr);

/** What a document binds to that its models lack. */
struct YuiToolMissingModels
{
    /** The models and properties lacking, in the shape of a models file: each property with a
        sample value of the type the document reads it as, 0 when nothing tells. Void when
        nothing lacks.
    */
    yup::var models;

    /** How many errors the document still has once they are added, which are not theirs to fix. */
    int numErrorsLeft = 0;
};

/** Finds what a file binds to that its models lack, by adding what its problems name until
    none names more.
*/
YuiToolMissingModels findYuiToolMissingModels (const yup::File& file, const YuiToolModels& models);

/** Returns the index of the root component of a parsed document, or -1 when it only holds declarations. */
int findYuiToolRootIndex (const yup::DataTree& document);

/** Returns the size the root of a parsed document sets with plain numbers, which is the one it asks
    to be shown at. It has no width or no height when the root sets none, or computes it.
*/
yup::Size<float> findYuiToolDeclaredSize (const yup::DataTree& document);

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

    /** True when the target is shown where the interface uses it: the document is loaded as it
        is, with its palette, its styles and its models, and one of the components the drawing
        paints, or of the type, is what to frame. False when a component standing in for them
        took the place of the root.
    */
    bool isInContext = false;

    /** True when a drawing shown on a plain component reads what that component does not have,
        so it needs the type it is made for as its host.
    */
    bool needsHost = false;
};

/** Loads a file into a document, showing a target of it.

    A drawing given with no host type, and a component type, are shown in context when the
    interface of the file has a component the drawing paints, or one of the type, that shows
    with a size. Otherwise a component standing in for them takes the place of the root.

    The interface is left at the size the uses were looked for at. A root that a view shows
    took the size of that view, so the one it sets is then read from the file.

    @param size         The size the interface is shown at when it is known, which the uses of
                        a target are looked for at. None while it has no width.
    @param fallbackSize The size used for it when neither `size` nor the root gives one.
*/
YuiToolShown loadYuiToolTarget (yup::YuiDocument& document,
                                yup::YuiLoader& loader,
                                const yup::File& input,
                                YuiToolTarget target,
                                yup::Size<float> size = {},
                                yup::Size<float> fallbackSize = {});

/** Returns the components of the interface of a document that a target can be framed on: those
    its drawing paints, or else those of its type, that show with a size, in the order of the tree.

    Only the components the document describes are told, as they are the ones a path names.
*/
std::vector<yup::Component*> findYuiToolUses (yup::YuiDocument& document, const YuiToolTarget& target);

/** Returns the one of the uses of a target that is framed when no other is asked for, the first
    of them, or nullptr when there is none.
*/
yup::Component* findYuiToolUse (yup::YuiDocument& document, const YuiToolTarget& target);

/** Returns the component of an interface named by its id, or else by the path a description of
    the layout gives it, or nullptr.
*/
yup::Component* findYuiToolElement (yup::YuiDocument& document, const yup::String& idOrPath);

/** Returns what a component is called where its interface is told about: its type, then its id
    when it has one, as `Slider #cutoff`.
*/
yup::String describeYuiToolName (const yup::Component& component);

/** Returns where the top left of a component is in the interface it belongs to. */
yup::Point<float> findYuiToolOrigin (const yup::Component& component, const yup::Component& root);

/** Brings a part of an interface to whole pixels inside it. What is left has no size when
    nothing of the interface is there.
*/
yup::Rectangle<float> clipYuiToolRegion (const yup::Rectangle<float>& region, const yup::Rectangle<float>& whole);

/** Returns the part of an interface a component paints: its bounds with its paint margin, in
    whole pixels inside the interface.
*/
yup::Rectangle<float> findYuiToolPaintArea (const yup::Component& component, const yup::Component& root);

//==============================================================================
/** What to show of a file and how, as the options of `render` and `inspect` say. */
struct YuiToolViewOptions
{
    yup::File input;
    YuiToolTarget target;

    /** A component of the interface to show in place of all of it, by its id or by the path a
        description of the layout gives it. None when empty.
    */
    yup::String element;

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

    /** Returns the component a target shown in context is framed on, or nullptr when the target
        is all of the interface.
    */
    yup::Component* getUse() const noexcept { return use.get(); }

    /** Returns the part of the interface to show: what the use of a target shown in context
        paints, else all of it.
    */
    yup::Rectangle<float> getFrame() const;

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
    yup::WeakReference<yup::Component> use;
};

//==============================================================================
/** Shows a part of an interface in place of all of it, for as long as it lives.

    The interface keeps its size and is only moved, so it is laid out as it is when shown whole.
*/
class YuiToolFrame final : public yup::Component
{
public:
    /** Frames a part of an interface.

        @param interfaceToShow What is drawn of a view, which must outlive the frame.
        @param region          The part to show, in the coordinates of the interface.
    */
    YuiToolFrame (yup::Component& interfaceToShow, const yup::Rectangle<float>& region);
    ~YuiToolFrame() override;

private:
    yup::Component& shown;
};

/** Describes the layout of a document as `inspect` prints it: its components with their place,
    and what looks wrong with them.

    @param document     A document loaded and laid out, as the one of a view is.
    @param input        The file the document was loaded from.
    @param withDrawings True to tell, for each component a drawing paints, where the elements of
                        the drawing that have an id land in the interface.
    @param only         A component of the interface to describe alone with what it holds, or
                        nullptr for the whole interface.

    @returns The description, or nothing for a document with no interface.
*/
yup::var describeYuiToolLayout (yup::YuiDocument& document, const yup::File& input, bool withDrawings = false, const yup::Component* only = nullptr);

/** Returns the component a description of the layout gives a path, or nullptr. */
yup::Component* findYuiToolComponent (yup::YuiDocument& document, const yup::String& path);

/** Tells a component as a description of the layout names it: its type, its id when it has one
    and its path. A component the document does not describe, such as a part a widget made for
    itself, is told as the one of the document it belongs to.

    @returns The description, or nothing for a component that is not of the interface.
*/
yup::var describeYuiToolComponent (yup::YuiDocument& document, const yup::Component& component);

//==============================================================================
/** Describes a component type as `schema` does.

    @param includeInherited True to list the properties and signals of its bases too, each with
                            the type it comes from.
*/
yup::var describeYuiToolComponentType (const yup::ComponentTypeInfo& info, bool includeInherited);

/** Describes what a document can name beyond its component types, as the code checking documents
    knows it: `drawing` for the elements of a drawing with their properties, `styles` for what a
    rule and a selector take, `expressions` for the functions, members and enumerations, `shaders`
    for how a shader is declared, written and used and, in a tool that has the 3D component types,
    `scenes` for how a scene is written.

    @returns The description, or nothing for a name that is none of these topics.
*/
yup::var describeYuiToolSchemaTopic (const yup::String& topic);

/** Returns the names of the topics describeYuiToolSchemaTopic() describes in this tool. */
yup::StringArray getYuiToolSchemaTopics();

/** Creates what draws with no window, or nullptr on a platform that has no such device. */
std::unique_ptr<yup::GraphicsContext> createYuiToolGraphicsContext();

/** Draws a component with no window, as a window showing it ends up drawing it.

    A window draws frame after frame, and a `Scene3D` only shows its final picture after some of
    them: it builds what it left to its first frame, then refines what it shows over the next ones.
    So the component is drawn a frame at a time until every scene in it is final, for a bounded
    number of frames. They are given no time, so what is animated stands where it starts. A scene
    still changing after them is reported as a warning of its document. A component holding no
    scene is drawn once.

    @param component What to draw: the component of a view, or a frame showing a part of it.
    @param snapshot  Draws the component once, into an image that is not valid where nothing can draw.

    @returns The image, which is not valid where nothing can draw, or why a scene could not be
             drawn: it then shows that in its place, which is not the picture asked for.
*/
yup::ResultValue<yup::Image> drawYuiToolImage (yup::Component& component, const std::function<yup::Image()>& snapshot);

/** Encodes an image as a PNG. */
bool encodeYuiToolPng (const yup::Image& image, yup::MemoryBlock& data);

//==============================================================================
/** What the audio displays of an interface show where no application gives them a state: a signal made here.

    A document can't write the state of a `SpectrumAnalyzer`, a `Spectrogram` or a `KMeter`: an application gives
    it, and a tool showing the document has none to give, so they would stay still. While this lives, every such
    display of the tree it looks through that has no state is given one, fed with a chord swelling under a partial
    sweeping the spectrum: what a drawing or a shader makes of their levels is seen moving. A keyboard needs
    nothing, it shows the keys held on it.

    The displays are looked for again at every tick, so those a document makes when it is loaded again are fed too,
    and the ones that went are let go. A display still there when this goes is left without a state.
*/
class YuiToolSignals final : private yup::Timer
{
public:
    YuiToolSignals();
    ~YuiToolSignals() override;

    /** Sets the component whose tree is looked through for displays, or nullptr for none. */
    void setRoot (yup::Component* newRoot);

private:
    void timerCallback() override;
    void feed (yup::Component& component);
    void makeSignal();

    static constexpr double sampleRate = 48000.0;
    static constexpr int ticksPerSecond = 30;

    /** A display fed by a state of its own: two reading one state would take each other's samples. */
    struct Fed
    {
        yup::WeakReference<yup::Component> display;
        std::unique_ptr<yup::SpectrumAnalyzerState> state;
    };

    yup::WeakReference<yup::Component> root;
    std::vector<Fed> spectrums;
    std::vector<yup::WeakReference<yup::Component>> meters;
    yup::KMeterState meterState { sampleRate, 2 };
    double lowPhase = 0.0, midPhase = 0.0, highPhase = 0.0, sweepPhase = 0.0, swellPhase = 0.0;
};
