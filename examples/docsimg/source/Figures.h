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

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

// Every figure file includes this header: changing it recompiles all of them.

//==============================================================================

using Role = yup::ThemePalette::Role;

/** The color the current theme gives a role. Figures are captured once per color mode,
    so they take their colors from here unless they are about color. */
inline yup::Color role (Role paletteRole)
{
    return yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (paletteRole);
}

/** The font of the theme, at a height. */
inline yup::Font font (float height)
{
    return yup::ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (height);
}

//==============================================================================
/** Fails the figure being drawn, for what its paint function cannot do. */
void failFigure (const yup::String& reason);

/** A file of examples/docsimg. */
inline yup::File docsimgFile (yup::StringRef path)
{
    return yup::File (YUP_DOCSIMG_PATH).getChildFile (path);
}

/** An image of examples/docsimg, invalid when it cannot be loaded. */
inline yup::Image docsimgImage (yup::StringRef path)
{
    yup::MemoryBlock data;
    docsimgFile (path).loadFileAsData (data);
    return yup::Image::loadFromData (data.asBytes()).valueOr (yup::Image());
}

//==============================================================================
/** One figure of the documentation. */
struct Figure
{
    using Factory = std::function<std::unique_ptr<yup::Component>()>;

    yup::String id;        ///< The image path under docs/_static/images, without color mode and extension.
    yup::Size<float> size; ///< The logical size, images have twice as many pixels.
    Factory create;        ///< Creates the component filling the figure inside its padding.
    float padding = 24.0f; ///< The space left around the component, filled with the background.
    int settleMs = 0;      ///< How long to wait before capturing, for what loads asynchronously.
    int frames = 1;        ///< How many times to draw, keeping the last, for what refines over frames.
    bool themed = true;    ///< False captures once, for figures that do not follow the color mode.
};

//==============================================================================
/** The figures, filled by one function per documentation section.

    The `.yui` files of examples/docsimg/figures are figures too, found when the tool runs.
*/
class Figures
{
public:
    using PaintFunction = std::function<void (yup::Graphics&, yup::Rectangle<float>)>;

    static constexpr yup::Size<float> defaultSize { 560.0f, 210.0f };

    /** Adds a figure painted by a function, which is given the area inside the padding. */
    Figure& paint (const yup::String& id, yup::Size<float> size, PaintFunction paintFunction);

    Figure& paint (const yup::String& id, PaintFunction paintFunction) { return paint (id, defaultSize, std::move (paintFunction)); }

    /** Adds a figure showing a component. */
    Figure& component (const yup::String& id, yup::Size<float> size, Figure::Factory create);

    Figure& component (const yup::String& id, Figure::Factory create) { return component (id, defaultSize, std::move (create)); }

    const std::vector<Figure>& getAll() const { return figures; }

private:
    std::vector<Figure> figures;
};

//==============================================================================
/** Splits an area into a grid of cells with their name below, and draws each one.

    The drawing function is given the index and the area of its cell, and whatever it sets
    on the Graphics is restored after it.
*/
void labelled (yup::Graphics& g, yup::Rectangle<float> area, std::initializer_list<const char*> names, int columns, const std::function<void (int, yup::Rectangle<float>)>& draw);

//==============================================================================
/** An axis of a plot, linear or logarithmic. */
struct Axis
{
    float getValue (float proportion) const
    {
        return logarithmic ? from * std::pow (to / from, proportion) : from + (to - from) * proportion;
    }

    float getProportion (float value) const
    {
        return logarithmic ? std::log (value / from) / std::log (to / from) : (value - from) / (to - from);
    }

    float from = 0.0f;
    float to = 1.0f;
    const char* fromLabel = "";
    const char* toLabel = "";
    bool logarithmic = false;
};

/** A named function of the horizontal axis. */
struct Trace
{
    const char* name;
    std::function<float (float)> function;
};

/** Plots functions of the horizontal axis, named below when there are several of them.

    @returns The area the axes span.
*/
yup::Rectangle<float> plot (yup::Graphics& g, yup::Rectangle<float> area, const Axis& x, const Axis& y, const std::vector<Trace>& traces);

//==============================================================================
/** Components shown in a grid, each with its name below. */
struct Gallery : yup::Component
{
    explicit Gallery (int columnsToUse, float insetToLeave = 0.0f)
        : columns (columnsToUse)
        , inset (insetToLeave)
    {
        setOpaque (false);
    }

    template <class ComponentType>
    ComponentType& add (const char* name, std::unique_ptr<ComponentType> component)
    {
        auto& result = *component;
        addAndMakeVisible (result);

        names.push_back (name);
        components.push_back (std::move (component));
        return result;
    }

    yup::Rectangle<float> getCell (std::size_t index) const
    {
        const auto rows = (static_cast<int> (components.size()) + columns - 1) / columns;
        const auto width = getWidth() / static_cast<float> (columns);
        const auto height = getHeight() / static_cast<float> (rows);
        const auto column = static_cast<float> (static_cast<int> (index) % columns);
        const auto row = static_cast<float> (static_cast<int> (index) / columns);

        return yup::Rectangle<float> (column * width, row * height, width, height).reduced (0.0f, 2.0f);
    }

    void resized() override
    {
        for (std::size_t index = 0; index < components.size(); ++index)
            components[index]->setBounds (getCell (index).withTrimmedBottom (18.0f).reduced (6.0f + inset, inset));
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (role (Role::textMuted));

        for (std::size_t index = 0; index < components.size(); ++index)
            g.fillFittedText (names[index], font (12.0f), getCell (index).removeFromBottom (18.0f), yup::Justification::center);
    }

    std::vector<const char*> names;
    std::vector<std::unique_ptr<yup::Component>> components;
    int columns;
    float inset;
};

//==============================================================================

void addGraphicsFigures (Figures& f);
void addImagingFigures (Figures& f);
void addLayoutFigures (Figures& f);
void addWidgetFigures (Figures& f);
void addDspFigures (Figures& f);
void addAudioFigures (Figures& f);
void addRhiFigures (Figures& f);
void addScene3DFigures (Figures& f);
