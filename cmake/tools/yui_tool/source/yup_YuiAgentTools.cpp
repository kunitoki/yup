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
#include "yup_YuiToolGuides.h"
#include "yup_YuiToolSupport.h"

#include <cmath>
#include <utility>

using namespace yup;

namespace
{

constexpr float maximumSize = 4096.0f;
constexpr double maximumScale = 4.0;
constexpr double maximumZoom = 8.0;
constexpr float defaultMargin = 8.0f;
constexpr float minimumGridStep = 4.0f;
constexpr int longestListTold = 16;

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

var describeEvents()
{
    const auto point = [] (const char* description)
    {
        return makeObject ({ { "type", "array" }, { "items", makeObject ({ { "type", "number" } }) }, { "minItems", 2 }, { "maxItems", 2 }, { "description", description } });
    };

    const auto event = makeObject ({ { "type", "object" },
                                     { "properties", makeObject ({ { "id", describeText ("The id of the component.") },
                                                                   { "path", describeText ("The path yui_inspect gives the component, for one with no id.") },
                                                                   { "type", makeObject ({ { "type", "string" }, { "enum", var (StringArray { "click", "doubleClick", "drag", "wheel", "hover" }) } }) },
                                                                   { "at", point ("Where in the component, as [x, y] from its top left. Its middle when not given.") },
                                                                   { "by", point ("How far a drag goes, as [dx, dy].") },
                                                                   { "to", point ("Where a drag ends, as [x, y] from the top left of the component.") },
                                                                   { "delta", point ("How far a wheel turns, as [dx, dy].") } }) },
                                     { "required", var (StringArray { "type" }) } });

    return makeObject ({ { "type", "array" },
                         { "items", event },
                         { "description", "Pointer events played in order before the interface is told or drawn, each going where a pointer at that place would. "
                                          "The answer tells the component that received each, the model values they changed with the ones before, and the "
                                          "actions the interface called. The pointer stays where the last event left it." } });
}

// What yui_schema describes: the component types, then the topics telling what else a file can name
StringArray listSchemaTopics()
{
    auto topics = getYuiToolSchemaTopics();
    topics.insert (0, "components");

    return topics;
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

// An answer is told on one line, where it takes a third of the room it takes spread over many
YuiAgentResult succeed (const var& found)
{
    YuiAgentResult result;
    result.text = JSON::toString (found, true);

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

    return "the document can't be loaded:\n" + describeProblems (view.findProblems());
}

// The component an entry of a list names by its id or by its path
Result findNamed (const var& entry, YuiToolView& view, const char* what, Component*& component)
{
    const auto id = entry["id"].toString();
    const auto path = entry["path"].toString();

    if (id.isEmpty() && ! entry.hasProperty ("path"))
        return Result::fail (String (what) + " names its component by 'id' or by 'path'");

    component = id.isNotEmpty() ? view.getDocument().findById (id) : findYuiToolComponent (view.getDocument(), path);

    if (component == nullptr)
        return Result::fail (id.isNotEmpty() ? "there is no component with the id '" + id + "'" : "there is no component at the path '" + path + "'");

    return Result::ok();
}

Result applyStates (const var& arguments, YuiToolView& view)
{
    const auto& given = arguments["states"];
    const auto* states = given.getArray();

    if (states == nullptr)
        return given.isVoid() ? Result::ok() : Result::fail ("'states' is a list of components with the state to show each in");

    for (const auto& entry : *states)
    {
        const auto state = entry["state"].toString();
        Component* component = nullptr;

        if (const auto found = findNamed (entry, view, "a state", component); found.failed())
            return found;

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
bool readPoint (const var& given, Point<float>& point)
{
    const auto* values = given.getArray();

    if (values == nullptr || values->size() != 2)
        return false;

    point = { static_cast<float> ((*values)[0]), static_cast<float> ((*values)[1]) };
    return true;
}

String nameComponent (const var& described)
{
    if (described.isVoid())
        return "a component the document does not describe";

    return described["type"].toString() + (described.hasProperty ("id") ? " '" + described["id"].toString() + "'" : " at the path '" + described["path"].toString() + "'");
}

// Why an event did not do what a pointer on the component named does, nothing when it did
String findWhyNotReached (YuiToolView& view, const Component& named, const Component* received)
{
    if (received == nullptr)
        return "nothing is under the pointer there: the component is hidden, has no size or lies outside the interface";

    auto isInside = false;

    for (const auto* component = received; component != nullptr && ! isInside; component = component->getParentComponent())
        isInside = component == &named;

    if (! isInside)
        return "the event went to " + nameComponent (describeYuiToolComponent (view.getDocument(), *received)) + ", which is over the component there";

    if (! received->isEnabled())
        return "the component is disabled, so it ignores the pointer";

    return {};
}

// Plays the pointer events a call asks for, in order, and tells what received each
Result playEvents (const var& arguments, YuiToolView& view, PointerSimulator& pointer, Array<var>& played)
{
    const auto* events = arguments["events"].getArray();

    if (events == nullptr)
        return Result::fail ("'events' is a list of pointer events, each on a component");

    for (const auto& entry : *events)
    {
        const auto type = entry["type"].toString();
        Component* component = nullptr;

        if (const auto found = findNamed (entry, view, "an event", component); found.failed())
            return found;

        // Where in the component, its middle when not told
        const auto bounds = component->getBoundsRelativeToTopLevelComponent();
        Point<float> at { bounds.getWidth() * 0.5f, bounds.getHeight() * 0.5f };

        if (entry.hasProperty ("at") && ! readPoint (entry["at"], at))
            return Result::fail ("'at' is [x, y] in logical pixels from the top left of the component");

        const auto position = bounds.getTopLeft() + at;
        Component* received = nullptr;

        if (type == "click")
        {
            received = pointer.click (position);
        }
        else if (type == "doubleClick")
        {
            received = pointer.doubleClick (position);
        }
        else if (type == "hover")
        {
            received = pointer.moveTo (position);
        }
        else if (type == "drag")
        {
            Point<float> to, by;

            if (readPoint (entry["to"], to))
                received = pointer.drag (position, bounds.getTopLeft() + to);
            else if (readPoint (entry["by"], by))
                received = pointer.drag (position, position + by);
            else
                return Result::fail ("a drag tells where it goes: 'by' as [dx, dy], or 'to' as [x, y] in the component");
        }
        else if (type == "wheel")
        {
            Point<float> delta;

            if (! readPoint (entry["delta"], delta))
                return Result::fail ("a wheel event tells how far it turns as 'delta', [dx, dy]");

            received = pointer.wheel (position, delta);
        }
        else
        {
            return Result::fail ("the 'type' of an event is click, doubleClick, drag, wheel or hover");
        }

        auto told = makeObject ({ { "type", type } });

        if (received != nullptr)
            told.getDynamicObject()->setProperty ("received", describeYuiToolComponent (view.getDocument(), *received));

        if (const auto warning = findWhyNotReached (view, *component, received); warning.isNotEmpty())
            told.getDynamicObject()->setProperty ("warning", warning);

        played.add (told);

        // What the event changed may be the range of a repeat, whose components the next event can be on
        view.getDocument().applyPendingChanges();
    }

    return Result::ok();
}

// A value as it is told: a long list is the data of a meter, told by its length
var summarize (const var& value)
{
    const auto* list = value.getArray();

    return list != nullptr && list->size() > longestListTold ? var ("<" + String (list->size()) + " values>") : value;
}

// The values of the models the events changed, each with the one it had before
var describeChanges (const var& before, const var& after)
{
    auto changes = makeObject ({});
    const auto* models = after.getDynamicObject();

    if (models == nullptr)
        return changes;

    for (const auto& model : models->getProperties())
    {
        const auto* properties = model.value.getDynamicObject();
        auto changed = makeObject ({});

        if (properties == nullptr)
            continue;

        for (const auto& property : properties->getProperties())
        {
            const auto& was = before[model.name][property.name];

            if (was == property.value)
                continue;

            const auto* wasList = was.getArray();
            const auto* isList = property.value.getArray();

            if (wasList == nullptr || isList == nullptr || wasList->size() != isList->size())
            {
                changed.getDynamicObject()->setProperty (property.name, var (Array<var> { summarize (was), summarize (property.value) }));
                continue;
            }

            // Of a list, the elements that changed are told by their index
            auto elements = makeObject ({});

            for (int index = 0; index < isList->size() && elements.getDynamicObject()->getProperties().size() < longestListTold; ++index)
                if (! ((*wasList)[index] == (*isList)[index]))
                    elements.getDynamicObject()->setProperty (Identifier (String (index)), var (Array<var> { (*wasList)[index], (*isList)[index] }));

            changed.getDynamicObject()->setProperty (property.name, elements);
        }

        if (changed.getDynamicObject()->getProperties().size() > 0)
            changes.getDynamicObject()->setProperty (model.name, changed);
    }

    return changes;
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

// What of the interface a call draws: the region or the element it names, else the component a drawing or a
// type shown in context is framed on, else all of it
Result readRegion (const var& arguments, YuiToolView& view, Rectangle<float>& region, bool& isPart)
{
    const auto whole = view.getComponent()->getLocalBounds();
    const auto& given = arguments["region"];
    const auto element = arguments["element"].toString();

    region = view.getFrame();
    isPart = ! given.isVoid() || element.isNotEmpty();

    if (! isPart)
    {
        isPart = view.getUse() != nullptr;
        return Result::ok();
    }

    if (! given.isVoid() && element.isNotEmpty())
        return Result::fail ("'region' and 'element' both say what to draw: give one of them");

    if (element.isNotEmpty())
    {
        const auto* component = findYuiToolElement (view.getDocument(), element);

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

    if (! std::isfinite (region.getX() + region.getY() + region.getWidth() + region.getHeight()))
        return Result::fail ("'region' and 'margin' are numbers of logical pixels");

    region = clipYuiToolRegion (region, whole);

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

    if (! std::isfinite (spacing) || spacing * static_cast<float> (scale) < minimumGridStep)
        return Result::fail ("'grid' is the logical pixels between two lines, at least " + String (minimumGridStep / scale, 1) + " at this scale");

    return Result::ok();
}

// True for a file inside a folder with no link on the way to it, which could lead out of the folder
bool isInsideFolder (const File& file, const File& folder)
{
    if (! file.isAChildOf (folder))
        return false;

    for (auto part = file; part != folder; part = part.getParentDirectory())
        if (part.isSymbolicLink())
            return false;

    return true;
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

    if (! isInsideFolder (output, host.baseFolder) || output.isDirectory())
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

    if (! isInsideFolder (file, host.baseFolder))
        return Result::fail ("'reference' is a file inside '" + host.baseFolder.getFullPathName() + "'");

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

    auto found = makeObject ({ { "file", file.getFullPathName() },
                               { "errors", numErrors },
                               { "warnings", static_cast<int> (problems.size()) - numErrors },
                               { "problems", listProblems (problems) } });

    // What the models lack is told ready to be added to their file, with how many errors that leaves
    if (const auto missing = numErrors > 0 ? findYuiToolMissingModels (file, models) : YuiToolMissingModels(); ! missing.models.isVoid())
    {
        found.getDynamicObject()->setProperty ("missingModels", missing.models);
        found.getDynamicObject()->setProperty ("errorsWithMissingModels", missing.numErrorsLeft);
    }

    return succeed (found);
}

// How a call asks for its image, beyond what it shows
struct Drawn
{
    double scale = 1.0;
    float grid = 0.0f;
    Image reference;
    File output;
};

// Adds to an answer the component a drawing or a type shown in context is framed on, and what a drawing shown
// on a plain component lacks
void addUse (var& told, YuiToolView& view)
{
    if (const auto* use = view.getUse())
        told.getDynamicObject()->setProperty ("use", describeYuiToolComponent (view.getDocument(), *use));

    if (view.getShown().needsHost)
        told.getDynamicObject()->setProperty ("hint", "a drawing reading the properties of a widget needs it as its host, for example 'as': 'Slider'");
}

// Adds to an answer what the events of the call did
void addPlayed (var& told, const var& played)
{
    if (const auto* done = played.getDynamicObject())
        for (const auto& member : done->getProperties())
            told.getDynamicObject()->setProperty (member.name, member.value);
}

YuiAgentResult drawView (const var& arguments, const YuiAgentHost& host, YuiToolView& view, const YuiToolViewOptions& options, const Drawn& drawn, const var& played)
{
    Rectangle<float> region;
    bool isPart = false;

    if (const auto read = readRegion (arguments, view, region, isPart); read.failed())
        return YuiAgentResult::fail (read.getErrorMessage());

    const auto tooLarge = [] (const String& what, int width, int height)
    {
        return YuiAgentResult::fail (what + " would be " + String (width) + "x" + String (height) + " pixels, more than "
                                     + String (maximumYuiAgentImageSide) + " a side: lower 'scale' or draw a smaller 'region'");
    };

    const auto width = roundToInt (region.getWidth() * drawn.scale);
    const auto height = roundToInt (region.getHeight() * drawn.scale);

    if (jmax (width, height) > maximumYuiAgentImageSide)
        return tooLarge ("the image", width, height);

    // The components the guides are drawn over
    std::vector<Component*> marked;

    if (const auto* guided = arguments["guides"].getArray())
    {
        for (const auto& named : *guided)
        {
            auto* component = findYuiToolElement (view.getDocument(), named.toString());

            if (component == nullptr)
                return YuiAgentResult::fail ("there is no component with the id or at the path '" + named.toString() + "' to draw guides over");

            marked.push_back (component);
        }
    }
    else if (! arguments["guides"].isVoid())
    {
        return YuiAgentResult::fail ("'guides' is a list of components, each named by its id or its path");
    }

    const auto hasMarks = drawn.grid > 0.0f || ! marked.empty();

    // A scene only shows its final picture after some frames, which are drawn until it does. One that can't be drawn
    // leaves no image, and why
    String failure;

    const auto draw = [&host, &drawn, &failure] (Component& component)
    {
        const auto result = drawYuiToolImage (component, [&]
        {
            return host.snapshot (component, drawn.scale);
        });

        if (result.failed())
            failure = result.getErrorMessage();

        return result.valueOr (Image());
    };

    // Draws the interface, with the grid and the guides over it unless it is asked plain
    const auto snap = [&] (bool isPlain)
    {
        if (host.snapshot == nullptr)
            return Image();

        if (! isPart && (isPlain || ! hasMarks))
            return draw (*view.getComponent());

        // Moving the interface into the frame may restyle it
        YuiToolFrame frame (*view.getComponent(), region);
        std::unique_ptr<GridOverlay> overlay;
        YuiToolGuides guides;

        if (! isPlain && ! marked.empty())
        {
            guides.setDocument (&view.getDocument());
            guides.setView (region.getPosition(), 1.0f);
            guides.setPicked (marked);

            frame.addAndMakeVisible (guides);
            guides.setBounds (frame.getLocalBounds());
        }

        if (! isPlain && drawn.grid > 0.0f)
        {
            overlay = std::make_unique<GridOverlay> (region, drawn.grid, static_cast<float> (drawn.scale));
            frame.addAndMakeVisible (*overlay);
            overlay->setBounds (frame.getLocalBounds());
        }

        view.settle();

        return draw (frame);
    };

    auto image = snap (false);

    if (! image.isValid())
        return YuiAgentResult::fail (failure.isNotEmpty() ? failure : "there is no graphics device to render with here: yui_inspect describes the layout without one");

    auto told = makeObject ({ { "file", options.input.getFullPathName() },
                              { "scale", drawn.scale },
                              { "theme", options.mode == ThemePalette::Mode::light ? "light" : "dark" } });
    auto* record = told.getDynamicObject();

    if (isPart)
        record->setProperty ("region", describeRegion (region));

    addUse (told, view);

    if (drawn.grid > 0.0f)
        record->setProperty ("grid", drawn.grid);

    if (drawn.reference.isValid())
    {
        // The grid and the guides are shown over the render, and kept out of what is compared
        const auto plain = hasMarks ? snap (true) : image;
        const auto background = ApplicationTheme::getGlobalTheme()->getPalette (*view.getRoot()).getColor (ThemePalette::Role::background);
        const auto comparison = compareYuiToolImages (plain, image, drawn.reference, region, view.getComponent()->getSize(), background);

        if (jmax (comparison.panels.getWidth(), comparison.panels.getHeight()) > maximumYuiAgentImageSide)
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
    addPlayed (told, played);
    record->setProperty ("problems", listProblems (view.findProblems()));

    result.text = JSON::toString (told, true);
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

    // The pointer stays where the last event left it until the interface is told or drawn
    std::optional<PointerSimulator> pointer;
    var played;

    if (! arguments["events"].isVoid())
    {
        Array<var> events;
        Array<var> actions;
        const auto before = models.toJson().clone();

        view.getDocument().setFallbackAction ([&actions] (const String& name, const Array<var>& given)
        {
            actions.add (makeObject ({ { "name", name }, { "arguments", var (given) } }));
        });

        pointer.emplace (*view.getComponent());

        const auto done = playEvents (arguments, view, *pointer, events);

        view.getDocument().setFallbackAction (nullptr);

        if (done.failed())
            return YuiAgentResult::fail (done.getErrorMessage());

        view.settle();

        played = makeObject ({ { "events", var (events) }, { "changed", describeChanges (before, models.toJson()) }, { "actions", var (actions) } });
    }

    if (! draw)
    {
        // A component named is described alone, as is the one a drawing or a type shown in context is framed on
        const Component* only = view.getUse();

        if (const auto element = arguments["element"].toString(); element.isNotEmpty())
        {
            only = findYuiToolElement (view.getDocument(), element);

            if (only == nullptr)
                return YuiAgentResult::fail ("there is no component with the id or at the path '" + element + "'");
        }

        auto layout = describeYuiToolLayout (view.getDocument(), view.getInput(), static_cast<bool> (arguments["drawings"]), only);
        addUse (layout, view);
        addPlayed (layout, played);
        layout.getDynamicObject()->setProperty ("problems", listProblems (view.findProblems()));

        return succeed (layout);
    }

    return drawView (arguments, host, view, options, drawn, played);
}

YuiAgentResult describeSchema (const var& arguments)
{
    registerYuiComponentTypes();

    const auto types = ComponentTypeRegistry::getInstance().getRegisteredTypes();
    const auto typeName = arguments["type"].toString();
    const auto topic = arguments["topic"].toString();

    if (topic.isNotEmpty() && topic != "components")
    {
        if (typeName.isNotEmpty())
            return YuiAgentResult::fail ("'type' names a component type, which the components topic describes");

        const auto described = describeYuiToolSchemaTopic (topic);

        return described.isVoid() ? YuiAgentResult::fail ("'topic' is one of " + listSchemaTopics().joinIntoString (", ")) : succeed (described);
    }

    if (typeName.isNotEmpty())
    {
        for (const auto* info : types)
        {
            if (info->getTypeName().toString() != typeName)
                continue;

            auto described = describeYuiToolComponentType (*info, static_cast<bool> (arguments["inherited"]));
            StringArray bases;

            for (const auto* base = info->getBaseTypeInfo(); base != nullptr; base = base->getBaseTypeInfo())
                bases.add (base->getTypeName().toString());

            // The types declaring the properties and signals left out, nearest first
            described.getDynamicObject()->setProperty ("bases", var (bases));

            return succeed (described);
        }

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

    return succeed (makeObject ({ { "types", var (names) },
                                  { "paletteRoles", var (ThemePalette::getRoleNames()) },
                                  { "topics", var (listSchemaTopics()) } }));
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
           "yui_inspect describes its layout as text and yui_render draws it. yui_schema tells what a file can name, and how its shaders "
           "and its 3D scenes are written.";
}

var getYuiAgentTools (bool withPreview)
{
    const auto drawing = std::pair<Identifier, var> { "drawing", describeText ("Name of a drawing the file declares, to show the first component of the interface it paints, as it is styled there. "
                                                                                "The answer names that component as 'use', and 'element' picks another. A drawing the interface does not "
                                                                                "use is shown painting a component alone, of the type given as 'as'.") };
    const auto palette = std::pair<Identifier, var> { "palette", describeText ("Name of a palette the file declares, to show a few widgets and a swatch of each of its roles.") };
    const auto host = std::pair<Identifier, var> { "as", describeText ("Type of a component the file declares, to show the first one of the interface, or one alone when it has none. "
                                                                        "With 'drawing', the type of the component shown alone painted by it, such as Slider.") };
    const auto size = std::pair<Identifier, var> { "size", describeText ("Size in logical pixels as <width>x<height>, such as 640x400. Without it, the width and height the root sets, else 640x400.") };
    const auto theme = std::pair<Identifier, var> { "theme", makeObject ({ { "type", "string" }, { "enum", var (StringArray { "dark", "light" }) }, { "description", "Mode of the default theme, dark when not given." } }) };
    const auto values = std::pair<Identifier, var> { "values", makeObject ({ { "type", "object" }, { "description", "Model values to set over those of the models file, as { \"<model>\": { \"<property>\": <value> } }." } }) };
    const auto states = std::pair<Identifier, var> { "states", describeStates() };
    const auto events = std::pair<Identifier, var> { "events", describeEvents() };
    const auto region = std::pair<Identifier, var> { "region", makeObject ({ { "type", "array" }, { "items", makeObject ({ { "type", "number" } }) }, { "minItems", 4 }, { "maxItems", 4 }, { "description", "Part of the interface to draw, as [x, y, width, height] in its logical pixels. With a scale, it looks closer." } }) };
    const auto element = std::pair<Identifier, var> { "element", describeText ("The id of a component, or the path yui_inspect gives it, to draw it alone with a margin around it.") };
    const auto margin = std::pair<Identifier, var> { "margin", describeNumber ("Logical pixels drawn around the element, 8 when not given.") };
    const auto guides = std::pair<Identifier, var> { "guides", makeObject ({ { "type", "array" }, { "items", makeObject ({ { "type", "string" } }) }, { "description", "Components to draw guides over, each by its id or the path yui_inspect gives it: its bounds, the area it paints when it has a paint margin, dotted, and its distances to the bounds of what holds it." } }) };
    const auto grid = std::pair<Identifier, var> { "grid", describeNumber ("Draws lines this many logical pixels apart over the image, with the coordinates of the interface they are at, to read places and sizes from it.") };
    const auto reference = std::pair<Identifier, var> { "reference", describeText ("Path of an image of what the interface is meant to look like. The image returned then holds the reference, the render and their "
                                                                                    "difference, and 'comparison' tells how alike they are (1 is the same) and the regions differing most, to draw next.") };

    Array<var> tools;

    tools.add (describeTool (checkTool,
                             "Reports the problems of a .yui file and of the files it imports, as an application loading it meets them: "
                             "each with its file, line, column, severity and message. Run it after every edit. When the file binds to models "
                             "or properties its models file lacks, 'missingModels' lists them with sample values, ready to be added to that file.",
                             { fileArgument(), modelsArgument() },
                             { "file" }));

    tools.add (describeTool (inspectTool,
                             "Describes the layout of a .yui interface without drawing it: the tree of its components, each with its type, id, source line, "
                             "text and bounds as [x, y, width, height] in logical pixels from the top left of the interface. It warns about components "
                             "that have no size or lie outside the interface. Use it to check places and sizes, and where an image is of no use.",
                             { fileArgument(), drawing, palette, host, size, theme, modelsArgument(), values, states, events,
                               { "element", describeText ("The id of a component, or the path this tool gives it, to describe it alone with what it holds instead of the whole interface.") },
                               { "drawings", makeObject ({ { "type", "boolean" }, { "description", "True to also tell, for each component a drawing paints, where the elements of the drawing that have an id land: "
                                                                                                    "their bounds in the interface, how many times a repeat paints them, and for a text the area its glyphs cover." } }) } },
                             { "file" }));

    tools.add (describeTool (renderTool,
                             "Draws a .yui interface, or a drawing, a palette or a component the file declares, and returns the image as a PNG "
                             "with the problems of the document. Use it to see what the file looks like: all of it, or a region or an element of it "
                             "drawn larger, under a grid of coordinates, or next to a reference image it is compared with.",
                             { fileArgument(), drawing, palette, host, size, scaleArgument(), region, element, margin, grid, guides, reference, outputArgument(), theme, modelsArgument(), values, states, events },
                             { "file" }));

    tools.add (describeTool (schemaTool,
                             "Tells what a .yui file can name, as the code checking it knows it. Without arguments, the component types and the palette roles. "
                             "With a type, the properties and signals it declares. With a topic, the elements of a drawing with their properties, what a "
                             "style rule and a selector take, the functions, members and enumerations of expressions, or how a shader and a 3D scene "
                             "are written, each with an example.",
                             { { "type", describeText ("Name of a component type, such as Slider.") },
                               { "inherited", makeObject ({ { "type", "boolean" }, { "description", "True to also list the properties and signals a type inherits. They are left out when not given, as every type inherits about 40 of them from Component." } }) },
                               { "topic", makeObject ({ { "type", "string" }, { "enum", var (listSchemaTopics()) }, { "description", "What to describe, components when not given." } }) } }));

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

    if (! std::isfinite (scale) || scale <= 0.0 || scale > maximum)
        return Result::fail ("'scale' is a number of pixels per logical pixel, at most " + String (static_cast<int> (maximumScale))
                             + ", or " + String (static_cast<int> (maximumZoom)) + " for a 'region' or an 'element'");

    return Result::ok();
}
