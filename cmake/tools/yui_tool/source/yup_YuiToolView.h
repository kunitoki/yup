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

#include "yup_YuiToolCheck.h"
#include "yup_YuiToolModels.h"

#include <yup_gui_markup/yup_gui_markup.h>

#include <memory>
#include <optional>
#include <vector>

//==============================================================================
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

    /** True once the models were given with `--models`: those next to the input are then left unread. */
    bool hasModels = false;
};

/** The most logical pixels a side of the size a file is shown at can have. */
constexpr float maximumYuiToolSize = 4096.0f;

/** Returns true for a number written with digits and at most one point, as the sizes, the regions
    and the margins of the options are: with no sign, no exponent and nothing after it.
*/
bool isYuiToolNumber (const yup::String& text);

/** Reads a size written as `<width>x<height>`, both from 1 to maximumYuiToolSize, and nothing else. */
std::optional<yup::Size<float>> parseYuiToolSize (const yup::String& text);

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

//==============================================================================
/** Creates what draws with no window, or nullptr on a platform that has no such device. */
std::unique_ptr<yup::GraphicsContext> createYuiToolGraphicsContext();

/** Encodes an image as a PNG. */
bool encodeYuiToolPng (const yup::Image& image, yup::MemoryBlock& data);
