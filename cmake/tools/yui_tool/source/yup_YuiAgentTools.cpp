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

#include "yup_YuiAgentTools.h"
#include "yup_YuiImageCompare.h"
#include "yup_YuiToolSupport.h"

#include <cmath>
#include <utility>

using namespace yup;

namespace
{

constexpr float maximumSize = 4096.0f;
constexpr double maximumScale = 4.0;
constexpr double maximumZoom = 8.0;
constexpr int maximumImageSide = 4096;
constexpr float defaultMargin = 8.0f;
constexpr float minimumGridStep = 4.0f;

const char* const checkTool = "yui_check";
const char* const inspectTool = "yui_inspect";
const char* const renderTool = "yui_render";
const char* const schemaTool = "yui_schema";
const char* const captureTool = "yui_capture_preview";

//==============================================================================
var makeObject (std::initializer_list<std::pair<Identifier, var>> members)
{
    auto* object = new DynamicObject();

    for (const auto& [name, value] : members)
        object->setProperty (name, value);

    return var (object);
}

var describeText (const char* description)
{
    return makeObject ({ { "type", "string" }, { "description", description } });
}

var describeTool (const char* name, const char* description, std::initializer_list<std::pair<Identifier, var>> properties, const StringArray& required = {})
{
    return makeObject ({ { "name", name },
                         { "description", description },
                         { "inputSchema", makeObject ({ { "type", "object" }, { "properties", makeObject (properties) }, { "required", var (required) } }) } });
}

// The arguments saying what to show of a file and how, which the tools showing one share
std::pair<Identifier, var> fileArgument()
{
    return { "file", describeText ("Path of the .yui file, absolute or relative to the folder of the workspace.") };
}

std::pair<Identifier, var> modelsArgument()
{
    return { "models", describeText ("Path of the JSON file declaring the models the file binds to. Without it, the <name>.models.json next to a <name>.yui file is used when there is one.") };
}

var describeNumber (const char* description)
{
    return makeObject ({ { "type", "number" }, { "description", description } });
}

std::pair<Identifier, var> scaleArgument()
{
    return { "scale", describeNumber ("Pixels per logical pixel: 1 when not given, at most 4, or 8 when drawing a region or an element.") };
}

std::pair<Identifier, var> outputArgument()
{
    return { "output", describeText ("Path of a .png file to also write the image to, inside the folder of the workspace. Its folders are made, and a file there is replaced.") };
}

var describeStates()
{
    const auto state = makeObject ({ { "type", "object" },
                                     { "properties", makeObject ({ { "id", describeText ("The id of the component.") },
                                                                   { "path", describeText ("The path yui_inspect gives the component, for one with no id.") },
                                                                   { "state", makeObject ({ { "type", "string" }, { "enum", var (StringArray { "hover", "pressed" }) } }) } }) },
                                     { "required", var (StringArray { "state" }) } });

    return makeObject ({ { "type", "array" },
                         { "items", state },
                         { "description", "Components to show in an interaction state, as their :hover and :pressed rules style them." } });
}

//==============================================================================
String describeProblems (const std::vector<YuiToolProblem>& problems)
{
    StringArray lines;

    for (const auto& problem : problems)
    {
        if (! problem.isError)
            continue;

        const auto position = problem.line > 0 ? ":" + String (problem.line) + ":" + String (problem.column) : String();

        lines.add (problem.file.getFullPathName() + position + ": error: " + problem.message);
    }

    return lines.joinIntoString ("\n");
}

var listProblems (const std::vector<YuiToolProblem>& problems)
{
    Array<var> records;

    for (const auto& problem : problems)
        records.add (problem.toJson());

    return var (records);
}

YuiAgentResult succeed (const var& found)
{
    YuiAgentResult result;
    result.text = JSON::toString (found);

    return result;
}

//==============================================================================
Result findFile (const var& arguments, const YuiAgentHost& host, File& file)
{
    const auto path = arguments["file"].toString();

    if (path.isEmpty())
        return Result::fail ("give the path of a .yui file as 'file'");

    file = host.baseFolder.getChildFile (path);

    if (! file.existsAsFile())
        return Result::fail ("can't read '" + file.getFullPathName() + "'");

    return Result::ok();
}

// The models of the file given, else those next to the document, with the values set over them
Result loadModels (const var& arguments, const YuiAgentHost& host, const File& file, YuiToolModels& models)
{
    const auto given = arguments["models"].toString();
    const auto modelsFile = given.isNotEmpty() ? host.baseFolder.getChildFile (given)
                                               : file.getSiblingFile (file.getFileNameWithoutExtension() + ".models.json");
    const auto* values = arguments["values"].getDynamicObject();

    if (! modelsFile.existsAsFile())
    {
        if (given.isNotEmpty())
            return Result::fail ("can't read '" + modelsFile.getFullPathName() + "'");

        return values == nullptr ? Result::ok() : Result::fail ("there are no models to set the 'values' of: name their file as 'models'");
    }

    if (const auto loaded = models.loadFile (modelsFile); loaded.failed())
        return Result::fail (modelsFile.getFullPathName() + ": " + loaded.getErrorMessage());

    if (values == nullptr)
        return arguments["values"].isVoid() ? Result::ok() : Result::fail ("'values' is an object giving each model the values of its properties");

    const auto declared = models.toJson();

    for (const auto& model : values->getProperties())
    {
        const auto* properties = model.value.getDynamicObject();

        if (properties == nullptr)
            return Result::fail ("'values' is an object giving each model the values of its properties");

        for (const auto& property : properties->getProperties())
        {
            if (! declared[model.name].hasProperty (property.name))
                return Result::fail ("the models have no '" + model.name.toString() + "." + property.name.toString() + "'");

            models.setValue (model.name.toString(), property.name.toString(), property.value);
        }
    }

    return Result::ok();
}

Result readViewOptions (const var& arguments, YuiToolViewOptions& options)
{
    options.target = { arguments["drawing"].toString(), arguments["palette"].toString(), arguments["as"].toString() };

    if (options.target.palette.isNotEmpty() && (options.target.drawing.isNotEmpty() || options.target.hostType.isNotEmpty()))
        return Result::fail ("'palette' goes with neither 'drawing' nor 'as'");

    if (const auto size = arguments["size"].toString(); size.isNotEmpty())
    {
        const auto parsed = parseYuiToolSize (size);

        if (! parsed.has_value() || parsed->getWidth() > maximumSize || parsed->getHeight() > maximumSize)
            return Result::fail ("'size' is <width>x<height> in logical pixels, such as 640x400, each at most " + String (static_cast<int> (maximumSize)));

        options.size = *parsed;
    }

    const auto theme = arguments["theme"].toString();

    if (theme.isNotEmpty() && theme != "dark" && theme != "light")
        return Result::fail ("'theme' is dark or light");

    options.mode = theme == "light" ? ThemePalette::Mode::light : ThemePalette::Mode::dark;
    return Result::ok();
}

// Why a view has no interface to work on, nothing when it has one
String findWhyNothingShows (const YuiToolView& view)
{
    const auto& shown = view.getShown();

    if (shown.status == YuiToolShown::Status::nothing)
        return "nothing to show, " + shown.message;

    if (shown.status == YuiToolShown::Status::invalidName)
        return "'drawing', 'palette' and 'as' take the name of what the file declares";

    if (view.hasInterface())
        return {};

    auto reason = "the document can't be loaded:\n" + describeProblems (view.findProblems());

    if (shown.target.drawing.isNotEmpty() && shown.target.hostType.isEmpty())
        reason << "\nhint: a drawing reading the properties of a widget needs it as its host, for example 'as': 'Slider'";

    return reason;
}

Result applyStates (const var& arguments, YuiToolView& view)
{
    const auto& given = arguments["states"];
    const auto* states = given.getArray();

    if (states == nullptr)
        return given.isVoid() ? Result::ok() : Result::fail ("'states' is a list of components with the state to show each in");

    for (const auto& entry : *states)
    {
        const auto id = entry["id"].toString();
        const auto hasPath = entry.hasProperty ("path");
        const auto path = entry["path"].toString();
        const auto state = entry["state"].toString();

        if (id.isEmpty() && ! hasPath)
            return Result::fail ("a state names its component by 'id' or by 'path'");

        auto* component = id.isNotEmpty() ? view.getDocument().findById (id) : findYuiToolComponent (view, path);

        if (component == nullptr)
            return Result::fail (id.isNotEmpty() ? "there is no component with the id '" + id + "'" : "there is no component at the path '" + path + "'");

        if (state == "hover")
            component->setHovered (true);
        else if (state == "pressed")
            component->setPressed (true);
        else
            return Result::fail ("'state' is hover or pressed");
    }

    return Result::ok();
}

//==============================================================================
/** Lines a fixed distance apart over a region of the interface, each with the coordinate of the interface it is at. */
class GridOverlay final : public Component
{
public:
    GridOverlay (const Rectangle<float>& regionToRule, float spacingToUse, float scaleToUse)
        : region (regionToRule)
        , spacing (spacingToUse)
        , pixel (1.0f / scaleToUse)
    {
        setOpaque (false);
    }

    void paint (Graphics& g) override
    {
        // The lines and the labels have the size of the image, whatever the scale it is drawn at
        const auto font = ApplicationTheme::getGlobalTheme()->getDefaultFont().withHeight (10.0f * pixel);

        g.setStrokeWidth (pixel);

        rule (g, font, true);
        rule (g, font, false);
    }

private:
    void rule (Graphics& g, const Font& font, bool isVertical)
    {
        const Size<float> label (30.0f * pixel, 12.0f * pixel);
        const auto start = isVertical ? region.getX() : region.getY();
        const auto length = isVertical ? region.getWidth() : region.getHeight();
        const auto labelEvery = jmax (1, static_cast<int> (std::ceil ((isVertical ? label.getWidth() + 4.0f * pixel : label.getHeight() + 2.0f * pixel) / spacing)));

        for (auto index = static_cast<int> (std::ceil (start / spacing)); static_cast<float> (index) * spacing < start + length; ++index)
        {
            const auto coordinate = static_cast<float> (index) * spacing;
            const auto position = coordinate - start;

            g.setStrokeColor (Color (0xffff00ff).withAlpha (0.5f));

            if (isVertical)
                g.strokeLine (position, 0.0f, position, getHeight());
            else
                g.strokeLine (0.0f, position, getWidth(), position);

            // The row of labels at the top is left to the lines going down
            if (index % labelEvery != 0 || (! isVertical && position < label.getHeight()))
                continue;

            const auto box = isVertical ? Rectangle<float> (position + pixel, 0.0f, label.getWidth(), label.getHeight())
                                        : Rectangle<float> (0.0f, position + pixel, label.getWidth(), label.getHeight());
            const auto isWhole = std::abs (coordinate - std::round (coordinate)) < 0.001f;

            g.setFillColor (Color (0xff000000).withAlpha (0.75f));
            g.fillRect (box);
            g.setFillColor (Color (0xffffffff));
            g.fillFittedText (isWhole ? String (roundToInt (coordinate)) : String (coordinate, 1), font, box, Justification::center);
        }
    }

    const Rectangle<float> region;
    const float spacing;
    const float pixel;
};

/** Shows a region of the interface in place of all of it, under a grid when one is asked for.

    The interface keeps its size and is only moved, so it is laid out as it is when shown whole.
*/
class Frame final : public Component
{
public:
    Frame (Component& interfaceToShow, const Rectangle<float>& region, float gridSpacing, float scale)
        : shown (interfaceToShow)
    {
        setOpaque (false);
        setSize (region.getSize());

        addAndMakeVisible (shown);
        shown.setTopLeft ({ -region.getX(), -region.getY() });

        if (gridSpacing <= 0.0f)
            return;

        grid = std::make_unique<GridOverlay> (region, gridSpacing, scale);
        addAndMakeVisible (*grid);
        grid->setBounds (getLocalBounds());
    }

    ~Frame() override
    {
        removeChildComponent (shown);
        shown.setTopLeft ({ 0.0f, 0.0f });
    }

private:
    Component& shown;
    std::unique_ptr<GridOverlay> grid;
};

// What of the interface a call draws: all of it, or the region or the element it names
Result readRegion (const var& arguments, YuiToolView& view, Rectangle<float>& region, bool& isPart)
{
    const auto whole = view.getComponent()->getLocalBounds();
    const auto& given = arguments["region"];
    const auto element = arguments["element"].toString();

    region = whole;
    isPart = ! given.isVoid() || element.isNotEmpty();

    if (! isPart)
        return Result::ok();

    if (! given.isVoid() && element.isNotEmpty())
        return Result::fail ("'region' and 'element' both say what to draw: give one of them");

    if (element.isNotEmpty())
    {
        auto* component = view.getDocument().findById (element);

        if (component == nullptr)
            component = findYuiToolComponent (view, element);

        if (component == nullptr)
            return Result::fail ("there is no component with the id or at the path '" + element + "'");

        const auto& margin = arguments["margin"];

        if (! margin.isVoid() && static_cast<double> (margin) < 0.0)
            return Result::fail ("'margin' is a number of logical pixels around the element, 0 or more");

        region = component->getBoundsRelativeToTopLevelComponent().enlarged (margin.isVoid() ? defaultMargin : static_cast<float> (margin));
    }
    else
    {
        const auto* values = given.getArray();

        if (values == nullptr || values->size() != 4)
            return Result::fail ("'region' is [x, y, width, height] in logical pixels of the interface");

        region = { static_cast<float> ((*values)[0]), static_cast<float> ((*values)[1]), static_cast<float> ((*values)[2]), static_cast<float> ((*values)[3]) };
    }

    // Whole pixels, inside the interface
    const auto x = std::floor (region.getX());
    const auto y = std::floor (region.getY());

    region = Rectangle<float> (x, y, std::ceil (region.getRight()) - x, std::ceil (region.getBottom()) - y).intersection (whole);

    if (region.getWidth() < 1.0f || region.getHeight() < 1.0f)
        return Result::fail ("there is nothing of the interface there: it is " + String (roundToInt (whole.getWidth())) + "x" + String (roundToInt (whole.getHeight())) + " logical pixels");

    return Result::ok();
}

Result readGrid (const var& arguments, double scale, float& spacing)
{
    const auto& given = arguments["grid"];
    spacing = given.isVoid() ? 0.0f : static_cast<float> (given);

    if (given.isVoid())
        return Result::ok();

    if (spacing * static_cast<float> (scale) < minimumGridStep)
        return Result::fail ("'grid' is the logical pixels between two lines, at least " + String (minimumGridStep / scale, 1) + " at this scale");

    return Result::ok();
}

// The file a call asks its image to be written to, none when it asks for none
Result findOutput (const var& arguments, const YuiAgentHost& host, File& output)
{
    const auto path = arguments["output"].toString();

    if (path.isEmpty())
        return Result::ok();

    if (! host.savesImages)
        return Result::fail ("there is nowhere to write 'output' to here");

    output = host.baseFolder.getChildFile (path);

    if (! output.hasFileExtension ("png"))
        return Result::fail ("'output' is the path of a .png file");

    if (! output.isAChildOf (host.baseFolder) || output.isDirectory())
        return Result::fail ("'output' is a file inside '" + host.baseFolder.getFullPathName() + "'");

    return Result::ok();
}

Result loadReference (const var& arguments, const YuiAgentHost& host, Image& reference)
{
    const auto path = arguments["reference"].toString();

    if (path.isEmpty())
        return Result::ok();

    const auto file = host.baseFolder.getChildFile (path);
    MemoryBlock data;

    if (! file.existsAsFile() || ! file.loadFileAsData (data))
        return Result::fail ("can't read '" + file.getFullPathName() + "'");

    auto loaded = Image::loadFromData ({ static_cast<const uint8*> (data.getData()), data.getSize() });

    if (loaded.failed() || ! loaded.getReference().isValid())
        return Result::fail ("'" + file.getFullPathName() + "' is not an image that can be read");

    reference = std::move (loaded.getReference());
    return Result::ok();
}

double roundedTo (double value, double steps)
{
    return std::round (value * steps) / steps;
}

var describeRegion (const Rectangle<float>& region)
{
    return var (Array<var> { roundedTo (region.getX(), 10.0), roundedTo (region.getY(), 10.0), roundedTo (region.getWidth(), 10.0), roundedTo (region.getHeight(), 10.0) });
}

var describeComparison (const YuiToolComparison& comparison, const Image& render)
{
    Array<var> worst;

    for (const auto& difference : comparison.worst)
        worst.add (makeObject ({ { "region", describeRegion (difference.region) }, { "error", roundedTo (difference.error, 1000.0) } }));

    return makeObject ({ { "similarity", roundedTo (comparison.similarity, 10000.0) },
                         { "differentPixels", roundedTo (comparison.differentPixels, 10000.0) },
                         { "reference", var (Array<var> { comparison.referenceSize.getWidth(), comparison.referenceSize.getHeight() }) },
                         { "resized", comparison.wasResized },
                         { "stretched", comparison.wasStretched },
                         { "panels", var (StringArray { "reference", "render", "difference" }) },
                         { "arrangement", comparison.isColumn ? "column" : "row" },
                         { "panel", var (Array<var> { render.getWidth(), render.getHeight() }) },
                         { "worst", var (worst) } });
}

//==============================================================================
YuiAgentResult check (const var& arguments, const YuiAgentHost& host)
{
    File file;
    YuiToolModels models;

    if (const auto found = findFile (arguments, host, file); found.failed())
        return YuiAgentResult::fail (found.getErrorMessage());

    if (const auto loaded = loadModels (arguments, host, file, models); loaded.failed())
        return YuiAgentResult::fail (loaded.getErrorMessage());

    const auto problems = findYuiToolFileProblems (file, std::nullopt, models);
    int numErrors = 0;

    for (const auto& problem : problems)
        numErrors += problem.isError ? 1 : 0;

    return succeed (makeObject ({ { "file", file.getFullPathName() },
                                  { "errors", numErrors },
                                  { "warnings", static_cast<int> (problems.size()) - numErrors },
                                  { "problems", listProblems (problems) } }));
}

// How a call asks for its image, beyond what it shows
struct Drawn
{
    double scale = 1.0;
    float grid = 0.0f;
    Image reference;
    File output;
};

YuiAgentResult drawView (const var& arguments, const YuiAgentHost& host, YuiToolView& view, const YuiToolViewOptions& options, const Drawn& drawn)
{
    Rectangle<float> region;
    bool isPart = false;

    if (const auto read = readRegion (arguments, view, region, isPart); read.failed())
        return YuiAgentResult::fail (read.getErrorMessage());

    const auto tooLarge = [] (const String& what, int width, int height)
    {
        return YuiAgentResult::fail (what + " would be " + String (width) + "x" + String (height) + " pixels, more than "
                                     + String (maximumImageSide) + " a side: lower 'scale' or draw a smaller 'region'");
    };

    const auto width = roundToInt (region.getWidth() * drawn.scale);
    const auto height = roundToInt (region.getHeight() * drawn.scale);

    if (jmax (width, height) > maximumImageSide)
        return tooLarge ("the image", width, height);

    const auto snap = [&] (float grid)
    {
        if (host.snapshot == nullptr)
            return Image();

        if (! isPart && grid <= 0.0f)
            return host.snapshot (*view.getComponent(), drawn.scale);

        // Moving the interface into the frame may restyle it
        Frame frame (*view.getComponent(), region, grid, static_cast<float> (drawn.scale));
        view.settle();

        return host.snapshot (frame, drawn.scale);
    };

    auto image = snap (drawn.grid);

    if (! image.isValid())
        return YuiAgentResult::fail ("there is no graphics device to render with here: yui_inspect describes the layout without one");

    auto told = makeObject ({ { "file", options.input.getFullPathName() },
                              { "scale", drawn.scale },
                              { "theme", options.mode == ThemePalette::Mode::light ? "light" : "dark" } });
    auto* record = told.getDynamicObject();

    if (isPart)
        record->setProperty ("region", describeRegion (region));

    if (drawn.grid > 0.0f)
        record->setProperty ("grid", drawn.grid);

    if (drawn.reference.isValid())
    {
        // The grid is shown over the render, and kept out of what is compared
        const auto plain = drawn.grid > 0.0f ? snap (0.0f) : image;
        const auto background = ApplicationTheme::getGlobalTheme()->getPalette (*view.getRoot()).getColor (ThemePalette::Role::background);
        const auto comparison = compareYuiToolImages (plain, image, drawn.reference, region, view.getComponent()->getSize(), background);

        if (jmax (comparison.panels.getWidth(), comparison.panels.getHeight()) > maximumImageSide)
            return tooLarge ("the three panels of the comparison", comparison.panels.getWidth(), comparison.panels.getHeight());

        record->setProperty ("comparison", describeComparison (comparison, plain));
        image = comparison.panels;
    }

    YuiAgentResult result;

    if (! encodeYuiToolPng (image, result.image))
        return YuiAgentResult::fail ("the image can't be encoded");

    if (drawn.output != File())
    {
        if (const auto made = drawn.output.getParentDirectory().createDirectory(); made.failed() || ! drawn.output.replaceWithData (result.image.getData(), result.image.getSize()))
            return YuiAgentResult::fail ("can't write '" + drawn.output.getFullPathName() + "'");

        record->setProperty ("output", drawn.output.getFullPathName());
    }

    record->setProperty ("width", image.getWidth());
    record->setProperty ("height", image.getHeight());
    record->setProperty ("problems", listProblems (view.findProblems()));

    result.text = JSON::toString (told);
    return result;
}

// Loads what a call asks to see, in the states it asks for, then inspects or draws it
YuiAgentResult show (const var& arguments, const YuiAgentHost& host, bool draw)
{
    YuiToolViewOptions options;
    YuiToolModels models;
    Drawn drawn;

    if (const auto found = findFile (arguments, host, options.input); found.failed())
        return YuiAgentResult::fail (found.getErrorMessage());

    if (const auto loaded = loadModels (arguments, host, options.input, models); loaded.failed())
        return YuiAgentResult::fail (loaded.getErrorMessage());

    if (const auto read = readViewOptions (arguments, options); read.failed())
        return YuiAgentResult::fail (read.getErrorMessage());

    if (draw)
    {
        const auto isPart = ! arguments["region"].isVoid() || arguments["element"].toString().isNotEmpty();

        if (const auto read = readYuiAgentScale (arguments, drawn.scale, isPart); read.failed())
            return YuiAgentResult::fail (read.getErrorMessage());

        if (const auto read = readGrid (arguments, drawn.scale, drawn.grid); read.failed())
            return YuiAgentResult::fail (read.getErrorMessage());

        if (const auto found = findOutput (arguments, host, drawn.output); found.failed())
            return YuiAgentResult::fail (found.getErrorMessage());

        if (const auto loaded = loadReference (arguments, host, drawn.reference); loaded.failed())
            return YuiAgentResult::fail (loaded.getErrorMessage());

        // An interface with no size of its own takes the one of the image it is meant to look like
        if (drawn.reference.isValid() && jmax (drawn.reference.getWidth(), drawn.reference.getHeight()) <= static_cast<int> (maximumSize))
            options.fallbackSize = Size<int> (drawn.reference.getWidth(), drawn.reference.getHeight()).to<float>();
    }

    YuiToolView view (options, models);

    if (const auto reason = findWhyNothingShows (view); reason.isNotEmpty())
        return YuiAgentResult::fail (reason);

    if (const auto applied = applyStates (arguments, view); applied.failed())
        return YuiAgentResult::fail (applied.getErrorMessage());

    view.settle();

    if (! draw)
    {
        auto layout = describeYuiToolLayout (view);
        layout.getDynamicObject()->setProperty ("problems", listProblems (view.findProblems()));

        return succeed (layout);
    }

    return drawView (arguments, host, view, options, drawn);
}

YuiAgentResult describeSchema (const var& arguments)
{
    registerBuiltInComponentTypes();

    const auto types = ComponentTypeRegistry::getInstance().getRegisteredTypes();
    const auto typeName = arguments["type"].toString();

    if (typeName.isNotEmpty())
    {
        for (const auto* info : types)
            if (info->getTypeName().toString() == typeName)
                return succeed (describeYuiToolComponentType (*info, true));

        return YuiAgentResult::fail ("there is no component type '" + typeName + "': call the tool without a type for those there are");
    }

    Array<var> names;

    for (const auto* info : types)
    {
        auto described = makeObject ({ { "name", info->getTypeName().toString() }, { "instantiable", info->hasFactory() } });

        if (const auto* base = info->getBaseTypeInfo())
            described.getDynamicObject()->setProperty ("base", base->getTypeName().toString());

        names.add (described);
    }

    return succeed (makeObject ({ { "types", var (names) }, { "paletteRoles", var (ThemePalette::getRoleNames()) } }));
}

} // namespace

//==============================================================================

YuiAgentResult YuiAgentResult::fail (const String& reason)
{
    YuiAgentResult result;
    result.text = reason;
    result.isError = true;

    return result;
}

String getYuiAgentInstructions()
{
    return "Tools for .yui files, the declarative UI language of the YUP library. After editing a file, yui_check reports its problems, "
           "yui_inspect describes its layout as text and yui_render draws it. yui_schema tells what a file can name.";
}

var getYuiAgentTools (bool withPreview)
{
    const auto drawing = std::pair<Identifier, var> { "drawing", describeText ("Name of a drawing the file declares, to show it painting a component in place of the interface.") };
    const auto palette = std::pair<Identifier, var> { "palette", describeText ("Name of a palette the file declares, to show a few widgets and a swatch of each of its roles.") };
    const auto host = std::pair<Identifier, var> { "as", describeText ("Type of the component painted by the drawing, such as Slider, or of a component the file declares to show on its own.") };
    const auto size = std::pair<Identifier, var> { "size", describeText ("Size in logical pixels as <width>x<height>, such as 640x400. Without it, the width and height the root sets, else 640x400.") };
    const auto theme = std::pair<Identifier, var> { "theme", makeObject ({ { "type", "string" }, { "enum", var (StringArray { "dark", "light" }) }, { "description", "Mode of the default theme, dark when not given." } }) };
    const auto values = std::pair<Identifier, var> { "values", makeObject ({ { "type", "object" }, { "description", "Model values to set over those of the models file, as { \"<model>\": { \"<property>\": <value> } }." } }) };
    const auto states = std::pair<Identifier, var> { "states", describeStates() };
    const auto region = std::pair<Identifier, var> { "region", makeObject ({ { "type", "array" }, { "items", makeObject ({ { "type", "number" } }) }, { "minItems", 4 }, { "maxItems", 4 }, { "description", "Part of the interface to draw, as [x, y, width, height] in its logical pixels. With a scale, it looks closer." } }) };
    const auto element = std::pair<Identifier, var> { "element", describeText ("The id of a component, or the path yui_inspect gives it, to draw it alone with a margin around it.") };
    const auto margin = std::pair<Identifier, var> { "margin", describeNumber ("Logical pixels drawn around the element, 8 when not given.") };
    const auto grid = std::pair<Identifier, var> { "grid", describeNumber ("Draws lines this many logical pixels apart over the image, with the coordinates of the interface they are at, to read places and sizes from it.") };
    const auto reference = std::pair<Identifier, var> { "reference", describeText ("Path of an image of what the interface is meant to look like. The image returned then holds the reference, the render and their "
                                                                                    "difference, and 'comparison' tells how alike they are (1 is the same) and the regions differing most, to draw next.") };

    Array<var> tools;

    tools.add (describeTool (checkTool,
                             "Reports the problems of a .yui file and of the files it imports, as an application loading it meets them: "
                             "each with its file, line, column, severity and message. Run it after every edit.",
                             { fileArgument(), modelsArgument() },
                             { "file" }));

    tools.add (describeTool (inspectTool,
                             "Describes the layout of a .yui interface without drawing it: the tree of its components, each with its type, id, source line, "
                             "text and bounds as [x, y, width, height] in logical pixels from the top left of the interface. It warns about components "
                             "that have no size or lie outside the interface. Use it to check places and sizes, and where an image is of no use.",
                             { fileArgument(), drawing, palette, host, size, theme, modelsArgument(), values, states },
                             { "file" }));

    tools.add (describeTool (renderTool,
                             "Draws a .yui interface, or a drawing, a palette or a component the file declares, and returns the image as a PNG "
                             "with the problems of the document. Use it to see what the file looks like: all of it, or a region or an element of it "
                             "drawn larger, under a grid of coordinates, or next to a reference image it is compared with.",
                             { fileArgument(), drawing, palette, host, size, scaleArgument(), region, element, margin, grid, reference, outputArgument(), theme, modelsArgument(), values, states },
                             { "file" }));

    tools.add (describeTool (schemaTool,
                             "Tells what a .yui file can name. Without a type, the component types and the palette roles. "
                             "With one, its properties and signals, those of its bases included.",
                             { { "type", describeText ("Name of a component type, such as Slider.") } }));

    if (withPreview)
    {
        tools.add (describeTool (captureTool,
                                 "Returns an image of the live preview of the editor as it is now, with the file it shows, its theme and the values "
                                 "of its models, which the user may have changed by hand. It fails while the preview is closed.",
                                 { scaleArgument(), outputArgument() }));
    }

    return var (tools);
}

bool doesYuiAgentToolDraw (const String& name)
{
    return name == renderTool || name == captureTool;
}

YuiAgentResult callYuiAgentTool (const String& name, const var& arguments, const YuiAgentHost& host)
{
    if (name == checkTool)
        return check (arguments, host);

    if (name == inspectTool)
        return show (arguments, host, false);

    if (name == renderTool)
        return show (arguments, host, true);

    if (name == schemaTool)
        return describeSchema (arguments);

    if (name == captureTool)
        return host.capturePreview != nullptr ? host.capturePreview (arguments) : YuiAgentResult::fail ("there is no live preview here to capture");

    return YuiAgentResult::fail ("there is no tool '" + name + "'");
}

Result readYuiAgentScale (const var& arguments, double& scale, bool isPart)
{
    const auto& given = arguments["scale"];
    const auto maximum = isPart ? maximumZoom : maximumScale;

    scale = given.isVoid() ? 1.0 : static_cast<double> (given);

    if (scale <= 0.0 || scale > maximum)
        return Result::fail ("'scale' is a number of pixels per logical pixel, at most " + String (static_cast<int> (maximumScale))
                             + ", or " + String (static_cast<int> (maximumZoom)) + " for a 'region' or an 'element'");

    return Result::ok();
}
