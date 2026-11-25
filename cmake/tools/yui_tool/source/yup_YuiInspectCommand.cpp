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

#include "yup_YuiCommands.h"
#include "yup_YuiToolSupport.h"

#include <cmath>
#include <iostream>

using namespace yup;

namespace
{

//==============================================================================
/** A component a document describes, at its place in the interface. */
struct Described
{
    Component* component = nullptr;
    DataTree element;
    Point<float> origin;
    bool isShown = true;
};

// The components a document describes right under another one. Those a widget creates for itself are
// passed through, so what they hold is listed in their place
void collectDescribed (const YuiBuilder& builder, const Component& parent, Point<float> origin, bool isShown, std::vector<Described>& children)
{
    for (int index = 0; index < parent.getNumChildComponents(); ++index)
    {
        auto* child = parent.getChildComponent (index);
        const Point<float> childOrigin { origin.getX() + child->getX(), origin.getY() + child->getY() };
        const auto element = builder.findElementFor (child);

        if (element.isValid())
            children.push_back ({ child, element, childOrigin, isShown && child->isVisible() });
        else
            collectDescribed (builder, *child, childOrigin, isShown && child->isVisible(), children);
    }
}

std::vector<Described> findDescribedChildren (const YuiBuilder& builder, const Described& parent)
{
    std::vector<Described> children;
    collectDescribed (builder, *parent.component, parent.origin, parent.isShown, children);
    return children;
}

double rounded (float value)
{
    return std::round (static_cast<double> (value) * 100.0) / 100.0;
}

// The first element under another that has a type and an id, itself included
DataTree findElement (const DataTree& parent, const String& type, const String& id)
{
    if (parent.getType().toString() == type && parent.getProperty ("id").toString() == id)
        return parent;

    for (const auto& child : parent)
        if (const auto found = findElement (child, type, id); found.isValid())
            return found;

    return {};
}

DataTree findDrawing (const DataTree& parent, const String& name)
{
    if (parent.getType().toString() == "drawing" && parent.getProperty ("name").toString() == name)
        return parent;

    for (const auto& child : parent)
        if (const auto found = findDrawing (child, name); found.isValid())
            return found;

    return {};
}

// The element of a document a measured element of a drawing was written as: in the drawing its path starts
// with, else anywhere, as what a `use` paints is written in another drawing
DataTree findDrawingElement (const DataTree& document, const DrawingStyle::ElementBounds& measured)
{
    const auto drawing = findDrawing (document, measured.path.upToFirstOccurrenceOf ("/", false, false));

    if (const auto found = drawing.isValid() ? findElement (drawing, measured.type, measured.id) : DataTree(); found.isValid())
        return found;

    return findElement (document, measured.type, measured.id);
}

// Moves to a component the document describes, looked for under another one, and tells its path
bool findDescribed (const YuiBuilder& builder, Described& current, const Component& target, String& path)
{
    if (current.component == &target)
        return true;

    const auto children = findDescribedChildren (builder, current);

    for (std::size_t index = 0; index < children.size(); ++index)
    {
        auto child = children[index];
        auto childPath = path.isEmpty() ? String (static_cast<int> (index)) : path + "/" + String (static_cast<int> (index));

        if (findDescribed (builder, child, target, childPath))
        {
            current = child;
            path = childPath;
            return true;
        }
    }

    return false;
}

//==============================================================================
/** Writes the layout of an interface down, with what looks wrong in it. */
class LayoutDescription
{
public:
    LayoutDescription (YuiDocument& documentToDescribe, const File& inputFile, bool shouldDescribeDrawings)
        : document (documentToDescribe)
        , input (inputFile)
        , builder (documentToDescribe.getBuilder())
        , sourceMap (documentToDescribe.getSourceMap())
        , root (*documentToDescribe.getRoot())
        , describesDrawings (shouldDescribeDrawings)
    {
    }

    // Describes the interface, or one of its components with what it holds
    var describe (const Component* only)
    {
        Described described { &root, builder.findElementFor (&root), {}, root.isVisible() };
        String path;

        if (only != nullptr && only != &root && ! findDescribed (builder, described, *only, path))
            return {};

        const auto tree = describe (described, path, true);

        auto* layout = new DynamicObject();
        layout->setProperty ("file", input.getFullPathName());
        layout->setProperty ("width", rounded (root.getWidth()));
        layout->setProperty ("height", rounded (root.getHeight()));
        layout->setProperty ("root", tree);
        layout->setProperty ("warnings", var (warnings));

        return var (layout);
    }

private:
    // Where the element of a component was written, when the document still knows
    void addPosition (DynamicObject& record, const DataTree& element) const
    {
        if (sourceMap == nullptr || ! element.isValid())
            return;

        DataTreeDiagnostic probe;
        probe.element = element;

        const auto located = sourceMap->locate (probe);

        if (located.line <= 0)
            return;

        if (located.source.isNotEmpty())
            record.setProperty ("file", input.getParentDirectory().getChildFile (located.source).getFullPathName());

        record.setProperty ("line", located.line);
        record.setProperty ("column", located.column);
    }

    static var describeBounds (const Rectangle<float>& bounds, Point<float> origin)
    {
        return var (Array<var> { rounded (bounds.getX() + origin.getX()), rounded (bounds.getY() + origin.getY()), rounded (bounds.getWidth()), rounded (bounds.getHeight()) });
    }

    // The elements of the drawing painting a component that have an id, where they land in the interface
    Array<var> describeDrawing (DrawingStyle& drawing, const Component& component, Point<float> origin) const
    {
        Array<var> elements;

        for (const auto& measured : drawing.measureElements (component))
        {
            if (measured.id.isEmpty())
                continue;

            auto* record = new DynamicObject();
            record->setProperty ("type", measured.type);
            record->setProperty ("id", measured.id);
            addPosition (*record, findDrawingElement (document.getTree(), measured));
            record->setProperty ("bounds", describeBounds (measured.bounds, origin));

            if (measured.numRows > 1)
                record->setProperty ("rows", measured.numRows);

            if (! measured.textBounds.isEmpty())
                record->setProperty ("text", describeBounds (measured.textBounds, origin));

            elements.add (var (record));
        }

        return elements;
    }

    void warn (const Described& described, const String& label, const String& path, const String& problem)
    {
        auto* warning = new DynamicObject();
        warning->setProperty ("message", label + " " + problem);
        warning->setProperty ("path", path);
        addPosition (*warning, described.element);

        warnings.add (var (warning));
    }

    var describe (const Described& described, const String& path, bool checkPlace)
    {
        const auto& component = *described.component;
        const auto typeName = described.element.isValid() ? described.element.getType().toString() : component.getTypeInfo().getTypeName().toString();
        const auto id = component.getComponentID();
        const Rectangle<float> bounds { described.origin.getX(), described.origin.getY(), component.getWidth(), component.getHeight() };

        auto* record = new DynamicObject();
        record->setProperty ("type", typeName);

        if (id.isNotEmpty())
            record->setProperty ("id", id);

        record->setProperty ("path", path);
        addPosition (*record, described.element);
        record->setProperty ("bounds", var (Array<var> { rounded (bounds.getX()), rounded (bounds.getY()), rounded (bounds.getWidth()), rounded (bounds.getHeight()) }));

        // What a component may paint outside its bounds, as a shadow or a glow
        if (! component.getPaintMargin().isZero())
            record->setProperty ("paintBounds", describeBounds (component.getPaintBounds(), described.origin));

        if (! component.isVisible())
            record->setProperty ("visible", false);

        static const Identifier textProperty ("text");

        if (const auto text = component.getProperty (textProperty); text.isString() && text.toString().isNotEmpty())
            record->setProperty ("text", text.toString().substring (0, 80));

        if (describesDrawings)
            if (auto* drawing = dynamic_cast<DrawingStyle*> (component.getStyle().get()))
                if (const auto elements = describeDrawing (*drawing, component, described.origin); ! elements.isEmpty())
                    record->setProperty ("drawing", var (elements));

        // One kept out of the layout of what holds it has no place in the interface, nor has what it holds: a part of a
        // 3D scene, and what it shows on its surface
        if (component.isExcludedFromParentLayout())
            checkPlace = false;

        // What is wrong with a component is wrong with all it holds, which is then not told again
        if (checkPlace && described.isShown)
        {
            const auto label = id.isNotEmpty() ? typeName + " #" + id : typeName;
            const auto hasNoWidth = bounds.getWidth() < 1.0f;
            const auto hasNoHeight = bounds.getHeight() < 1.0f;

            if (hasNoWidth || hasNoHeight)
            {
                warn (described, label, path, String ("has no ") + (hasNoWidth && hasNoHeight ? "size" : (hasNoWidth ? "width" : "height")) + ", so nothing of it shows: inside a Row or a Column it needs a preferred size or a flexGrow");
                checkPlace = false;
            }
            else if (! bounds.intersects (root.getLocalBounds()))
            {
                warn (described, label, path, "lies outside the interface");
                checkPlace = false;
            }
        }

        Array<var> children;
        int index = 0;

        for (const auto& child : findDescribedChildren (builder, described))
            children.add (describe (child, path.isEmpty() ? String (index++) : path + "/" + String (index++), checkPlace));

        if (! children.isEmpty())
            record->setProperty ("children", var (children));

        return var (record);
    }

    YuiDocument& document;
    const File input;
    const YuiBuilder& builder;
    std::shared_ptr<const DataTreeSourceMap> sourceMap;
    Component& root;
    const bool describesDrawings;
    Array<var> warnings;
};

} // namespace

//==============================================================================

var describeYuiToolLayout (YuiDocument& document, const File& input, bool withDrawings, const Component* only)
{
    if (document.getRoot() == nullptr)
        return {};

    return LayoutDescription (document, input, withDrawings).describe (only);
}

Component* findYuiToolComponent (YuiDocument& document, const String& path)
{
    if (document.getRoot() == nullptr)
        return nullptr;

    const auto& builder = document.getBuilder();
    Described current { document.getRoot(), {}, {}, true };

    for (const auto& step : StringArray::fromTokens (path, "/", ""))
    {
        // A step is the place of a child, which a few digits always tell
        if (step.isEmpty() || step.length() > 6 || ! step.containsOnly ("0123456789"))
            return nullptr;

        const auto children = findDescribedChildren (builder, current);
        const auto index = step.getIntValue();

        if (index >= static_cast<int> (children.size()))
            return nullptr;

        current = children[static_cast<std::size_t> (index)];
    }

    return current.component;
}

var describeYuiToolComponent (YuiDocument& document, const Component& component)
{
    if (document.getRoot() == nullptr)
        return {};

    const auto& builder = document.getBuilder();
    const auto* described = &component;

    // What a widget made for itself is told as the widget
    while (described != nullptr && described != document.getRoot() && ! builder.findElementFor (described).isValid())
        described = described->getParentComponent();

    String path;

    Described root { document.getRoot(), {}, {}, true };

    if (described == nullptr || ! findDescribed (builder, root, *described, path))
        return {};

    auto* record = new DynamicObject();
    record->setProperty ("type", described->getTypeInfo().getTypeName().toString());

    if (described->getComponentID().isNotEmpty())
        record->setProperty ("id", described->getComponentID());

    record->setProperty ("path", path);

    return var (record);
}

int runYuiInspectCommand (int argc, char** argv)
{
    YuiToolViewOptions options;
    YuiToolModels models;

    for (int i = 2; i < argc; ++i)
    {
        const auto isValid = readYuiToolViewOption (argc, argv, i, options, models);

        if (! isValid.has_value())
        {
            std::cerr << "invalid argument: " << argv[i] << "\n";
            return 2;
        }

        if (! *isValid)
            return 2;
    }

    const auto& target = options.target;

    if (options.input == File() || (target.drawing.isNotEmpty() && target.palette.isNotEmpty()) || (target.hostType.isNotEmpty() && target.palette.isNotEmpty()))
    {
        std::cerr << "Inspecting requires an input file, with --drawing or --palette but not both\n";
        return 2;
    }

    if (! options.input.existsAsFile())
    {
        std::cerr << "can't read '" << options.input.getFullPathName() << "'\n";
        return 2;
    }

    YuiToolView view (options, models);
    const auto& shown = view.getShown();

    if (shown.status == YuiToolShown::Status::nothing)
    {
        std::cerr << options.input.getFullPathName() << ": error: nothing to inspect, " << shown.message << "\n";
        return 1;
    }

    if (shown.status == YuiToolShown::Status::invalidName)
    {
        std::cerr << "invalid name given to --drawing, --palette or --as\n";
        return 2;
    }

    for (const auto& problem : view.findProblems())
        if (problem.isError)
            problem.print();

    if (shown.needsHost)
        std::cerr << "hint: a drawing reading the properties of a widget needs it as its host, for example --as Slider\n";

    if (! view.hasInterface())
        return 1;

    view.settle();

    // A component named is described alone, as is the one a drawing or a type shown in context is framed on
    const Component* only = view.getUse();

    if (options.element.isNotEmpty())
    {
        only = findYuiToolElement (view.getDocument(), options.element);

        if (only == nullptr)
        {
            std::cerr << options.input.getFullPathName() << ": error: nothing to inspect, there is no component with the id or at the path '" << options.element << "'\n";
            return 1;
        }
    }

    std::cout << JSON::toString (describeYuiToolLayout (view.getDocument(), view.getInput(), false, only)).toStdString() << "\n";
    return 0;
}
