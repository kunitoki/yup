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

#include "yup_YuiToolSupport.h"

#include <algorithm>
#include <iostream>

using namespace yup;

//==============================================================================

YuiToolModels::~YuiToolModels()
{
    for (auto& tree : trees)
        tree.removeListener (this);
}

bool YuiToolModels::load (const String& path)
{
    const auto file = File::getCurrentWorkingDirectory().getChildFile (path);

    if (! file.existsAsFile())
    {
        std::cerr << "can't read '" << path << "'\n";
        return false;
    }

    if (const auto result = loadFile (file); result.failed())
    {
        std::cerr << file.getFullPathName() << ": error: " << result.getErrorMessage() << "\n";
        return false;
    }

    return true;
}

Result YuiToolModels::loadFile (const File& file)
{
    var parsed;

    if (const auto result = JSON::parse (file.loadFileAsString(), parsed); result.failed())
        return result;

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

void YuiToolProblem::print() const
{
    std::cerr << file.getFullPathName();

    if (line > 0)
        std::cerr << ":" << line << ":" << column;

    std::cerr << ": " << (isError ? "error" : "warning") << ": " << message << "\n";
}

var YuiToolProblem::toJson() const
{
    auto* record = new DynamicObject();
    record->setProperty ("file", file.getFullPathName());
    record->setProperty ("line", line);
    record->setProperty ("column", column);
    record->setProperty ("severity", isError ? "error" : "warning");
    record->setProperty ("message", message);

    return var (record);
}

std::vector<YuiToolProblem> findYuiToolProblems (const YuiDocument& document, const File& file, const Result& loadResult)
{
    std::vector<YuiToolProblem> problems;

    for (const auto& diagnostic : document.getDiagnostics())
    {
        problems.push_back ({ .file = diagnostic.source.isEmpty() ? file : file.getParentDirectory().getChildFile (diagnostic.source),
                              .line = diagnostic.line,
                              .column = diagnostic.column,
                              .isError = diagnostic.isError(),
                              .message = diagnostic.message });
    }

    const auto hasError = std::any_of (problems.begin(), problems.end(), [] (const YuiToolProblem& problem)
    {
        return problem.isError;
    });

    if (loadResult.failed() && ! hasError)
        problems.push_back ({ .file = file, .message = loadResult.getErrorMessage() });

    return problems;
}

namespace
{

// A file holding only declarations (styles, drawings, components) is written to be imported
bool hasNoRootComponent (const String& text, const YuiLoader& loader)
{
    const auto parsed = YuiParser::parse (text, [&loader] (const String& path)
    {
        return loader.getLoadedText (path);
    });

    return parsed.wasOk() && findYuiToolRootIndex (parsed.document) < 0;
}

} // namespace

std::vector<YuiToolProblem> findYuiToolFileProblems (const File& file, const std::optional<String>& unsavedText, const YuiToolModels& models)
{
    YuiDocument document;
    YuiLoader loader (document);
    models.attachTo (document);

    auto result = loader.loadFile (file);

    if (unsavedText.has_value())
        result = loader.reloadFromString (*unsavedText);

    if (result.failed())
    {
        // There is no interface to build from declarations alone, so they are checked as those of an empty one
        if (const auto text = loader.getLoadedText (file.getFileName()); text.has_value() && hasNoRootComponent (*text, loader))
            result = loader.reloadFromString (*text + "\nComponent {}\n");
    }

    return findYuiToolProblems (document, file, result);
}

int findYuiToolRootIndex (const DataTree& document)
{
    for (int index = 0; index < document.getNumChildren(); ++index)
    {
        const auto typeName = document.getChild (index).getType().toString();

        if (CharacterFunctions::isUpperCase (static_cast<yup_wchar> (typeName[0])))
            return index;
    }

    return -1;
}

//==============================================================================

namespace
{

// The names of the drawings or of the palettes a document declares, in a style block too
StringArray findDeclared (const DataTree& document, const Identifier& type)
{
    static const Identifier name ("name");
    static const Identifier style ("style");

    StringArray names;

    for (const auto& element : document)
    {
        if (element.getType() == type)
            names.add (element.getProperty (name).toString());

        if (element.getType() != style)
            continue;

        for (const auto& nested : element)
            if (nested.getType() == type)
                names.add (nested.getProperty (name).toString());
    }

    return names;
}

// A few widgets and a swatch of each role over the background, in the order of ThemePalette::Role
String makePaletteSample (const String& palette)
{
    String swatches;

    for (const auto& role : ThemePalette::getRoleNames())
        if (role != "background")
            swatches << "            Window { backgroundColor: palette." << role << "; flexGrow: 1; }\n";

    return String (R"yui(
Window { palette: NAME; backgroundColor: palette.background;
    Column { padding: 16; gap: 10;
        Label { text: "NAME"; preferredHeight: 24; }
        Row { gap: 10; preferredHeight: 32;
            TextButton { text: "Button"; flexGrow: 1; }
            ToggleButton { text: "Toggle"; checked: true; flexGrow: 1; }
        }
        Slider { value: 0.6; preferredHeight: 28; }
        ProgressBar { progress: 0.4; preferredHeight: 16; }
        TextEditor { text: "Text"; preferredHeight: 32; }
        Row { gap: 6; flexGrow: 1;
SWATCHES        }
    }
})yui")
        .replace ("NAME", palette)
        .replace ("SWATCHES", swatches);
}

// Gives a parsed document another root component, the one hosting what is previewed
DataTree withRoot (const DataTree& document, const DataTree& root)
{
    auto tree = document.clone();
    const auto previousRoot = findYuiToolRootIndex (tree);

    {
        auto transaction = tree.beginTransaction();

        if (previousRoot >= 0)
            transaction.removeChild (previousRoot);

        transaction.addChild (root.clone());
    }

    return tree;
}

} // namespace

YuiToolShown loadYuiToolTarget (YuiDocument& document, YuiLoader& loader, const File& input, YuiToolTarget target)
{
    YuiToolShown shown;
    shown.result = loader.loadFile (input);

    const auto parsed = YuiParser::parse (loader.getLoadedText (input.getFileName()).value_or (String()), [&loader] (const String& path)
    {
        return loader.getLoadedText (path);
    });

    const auto nothing = [&shown, &target] (const String& reason)
    {
        shown.status = YuiToolShown::Status::nothing;
        shown.message = reason;
        shown.target = target;
        return shown;
    };

    if (parsed.wasOk())
    {
        const auto hasRoot = findYuiToolRootIndex (parsed.document) >= 0;
        const auto isPreview = target.drawing.isNotEmpty() || target.palette.isNotEmpty() || target.hostType.isNotEmpty();
        const auto drawings = findDeclared (parsed.document, "drawing");
        const auto palettes = findDeclared (parsed.document, "palette");

        if (target.drawing.isNotEmpty() && ! drawings.contains (target.drawing))
            return nothing ("there is no drawing '" + target.drawing + "'");

        if (target.palette.isNotEmpty() && ! palettes.contains (target.palette))
            return nothing ("there is no palette '" + target.palette + "'");

        // A file of declarations shows the first thing it declares that can be drawn
        if (! hasRoot && ! isPreview)
        {
            target.drawing = drawings[0];

            if (target.drawing.isEmpty())
                target.palette = palettes[0];
        }

        String hostText;

        if (target.palette.isNotEmpty())
        {
            hostText = makePaletteSample (target.palette);
            shown.defaultSize = { 360.0f, 300.0f };
        }
        else if (target.drawing.isNotEmpty() || target.hostType.isNotEmpty())
        {
            hostText = (target.hostType.isNotEmpty() ? target.hostType : String ("Component")) + " { " + (target.drawing.isNotEmpty() ? "drawing: " + target.drawing + "; " : String()) + "}";
            shown.defaultSize = { 200.0f, 200.0f };
        }
        else if (! hasRoot)
        {
            return nothing ("the file has no component, drawing or palette");
        }

        if (hostText.isNotEmpty())
        {
            const auto host = YuiParser::parse (hostText);

            if (! host.wasOk() || host.document.getNumChildren() != 1)
            {
                shown.status = YuiToolShown::Status::invalidName;
                shown.target = target;
                return shown;
            }

            // The positions of the text only hold while its elements keep their place
            shown.result = document.load (withRoot (parsed.document, host.document.getChild (0)), hasRoot ? YuiSourceMap() : parsed.sourceMap);
        }
    }

    shown.target = target;
    return shown;
}

//==============================================================================

std::optional<Size<float>> parseYuiToolSize (const String& text)
{
    const Size<float> size { text.upToFirstOccurrenceOf ("x", false, true).getFloatValue(), text.fromFirstOccurrenceOf ("x", false, true).getFloatValue() };

    if (size.getWidth() < 1.0f || size.getHeight() < 1.0f)
        return std::nullopt;

    return size;
}

std::optional<bool> readYuiToolViewOption (int argc, char** argv, int& index, YuiToolViewOptions& options, YuiToolModels& models)
{
    const String argument (argv[index]);
    const auto hasValue = index + 1 < argc;

    if (argument == "--models" && hasValue)
        return models.load (argv[++index]);

    if (argument == "--drawing" && hasValue)
    {
        options.target.drawing = argv[++index];
        return true;
    }

    if (argument == "--palette" && hasValue)
    {
        options.target.palette = argv[++index];
        return true;
    }

    if (argument == "--as" && hasValue)
    {
        options.target.hostType = argv[++index];
        return true;
    }

    if ((argument == "--size" || argument == "--default-size") && hasValue)
    {
        const String value (argv[++index]);
        const auto size = parseYuiToolSize (value);

        if (! size.has_value())
        {
            std::cerr << "invalid size, expected <width>x<height>: " << value << "\n";
            return false;
        }

        (argument == "--size" ? options.size : options.fallbackSize) = *size;
        return true;
    }

    if (argument == "--theme" && hasValue && (String (argv[index + 1]) == "dark" || String (argv[index + 1]) == "light"))
    {
        options.mode = String (argv[++index]) == "light" ? ThemePalette::Mode::light : ThemePalette::Mode::dark;
        return true;
    }

    if (options.input == File() && ! argument.startsWithChar ('-'))
    {
        options.input = File::getCurrentWorkingDirectory().getChildFile (argument);
        return true;
    }

    return std::nullopt;
}

//==============================================================================

namespace
{

/** Fills the image behind the interface with the background of the palette its root is drawn with. */
class Backdrop final : public Component
{
public:
    explicit Backdrop (Component& rootToShow)
        : root (rootToShow)
    {
        addAndMakeVisible (root);
    }

    void paint (Graphics& g) override
    {
        g.setFillColor (ApplicationTheme::getGlobalTheme()->getPalette (root).getColor (ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        root.setBounds (getLocalBounds());
    }

private:
    Component& root;
};

} // namespace

YuiToolView::YuiToolView (const YuiToolViewOptions& options, const YuiToolModels& models)
    : input (options.input)
    , previousPalette (ApplicationTheme::getGlobalTheme()->getPalette())
{
    ApplicationTheme::getGlobalTheme()->setPalette (ThemePalette ({}, options.mode));

    models.attachTo (document);
    shown = loadYuiToolTarget (document, loader, input, options.target);

    auto* root = document.getRoot();

    if (shown.status != YuiToolShown::Status::loaded || shown.result.failed() || root == nullptr)
        return;

    // The size asked for, else the one the root asks for, else a default one
    auto size = options.size;

    if (size.getWidth() < 1.0f)
    {
        if (root->getWidth() >= 1.0f && root->getHeight() >= 1.0f)
            size = root->getSize();
        else
            size = options.fallbackSize.getWidth() >= 1.0f ? options.fallbackSize : shown.defaultSize;
    }

    backdrop = std::make_unique<Backdrop> (*root);
    backdrop->setSize (size);
}

YuiToolView::~YuiToolView()
{
    // The interface goes before the theme it was styled with
    backdrop.reset();
    document.clear();

    ApplicationTheme::getGlobalTheme()->setPalette (previousPalette);
}

std::vector<YuiToolProblem> YuiToolView::findProblems() const
{
    return findYuiToolProblems (document, input, shown.result);
}

void YuiToolView::settle()
{
    auto* root = getRoot();
    auto* context = root != nullptr ? root->getStyleContext() : nullptr;

    if (context == nullptr)
        return;

    // A pass restyles what the size, a state or the pass before changed, which may start transitions, then
    // brings those running to their end, which may in turn change what other styles read
    for (int pass = 0; pass < 4; ++pass)
        context->refresh (3600.0);

    context->restyleNow();
}

//==============================================================================

std::unique_ptr<GraphicsContext> createYuiToolGraphicsContext()
{
    // Drawing needs a device of the GPU but no window, which only some platforms give
#if YUP_MAC
    return GraphicsContext::createContext (GpuPlatform::Metal, {});
#elif YUP_WINDOWS
    GraphicsContext::Options options;
    options.allowHeadlessRendering = true;

    return GraphicsContext::createContext (GpuPlatform::Direct3D, options);
#else
    return nullptr;
#endif
}

bool encodeYuiToolPng (const Image& image, MemoryBlock& data)
{
    if (! image.isValid())
        return false;

    // The writer owns the stream, which trims the block to what was written when it goes
    PngImageFormatWriter writer (new MemoryOutputStream (data, false), image.getPixelFormat());
    return writer.writeImage (image);
}
