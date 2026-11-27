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

#include "yup_YuiToolView.h"
#include "yup_YuiToolResources.h"

#include <algorithm>
#include <cmath>

using namespace yup;

//==============================================================================

Size<float> findYuiToolDeclaredSize (const DataTree& document)
{
    const auto rootIndex = findYuiToolRootIndex (document);

    if (rootIndex < 0)
        return {};

    const auto read = [root = document.getChild (rootIndex)] (const Identifier& name)
    {
        const auto text = root.getProperty (name).toString();
        return text.containsOnly ("0123456789.") ? text.getFloatValue() : 0.0f;
    };

    return { read ("width"), read ("height") };
}

//==============================================================================

namespace
{

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

// The declaration of a drawing or of a palette of a document, in a style block too: the one of a name, or the first
// of them when no name is given
DataTree findDeclaration (const DataTree& document, const Identifier& type, const String& declared = {})
{
    static const Identifier name ("name");
    static const Identifier style ("style");

    for (const auto& element : document)
    {
        if (element.getType() == type && (declared.isEmpty() || element.getProperty (name).toString() == declared))
            return element;

        if (element.getType() != style)
            continue;

        for (const auto& nested : element)
            if (nested.getType() == type && (declared.isEmpty() || nested.getProperty (name).toString() == declared))
                return nested;
    }

    return {};
}

// The size of the view box a drawing sets with plain numbers, which is the one it was drawn for
Size<float> findViewBoxSize (const DataTree& drawing)
{
    constexpr float largestSide = 4096.0f;

    const auto text = drawing.getProperty ("viewBox").toString().trim();

    if (! text.startsWith ("rect"))
        return {};

    const auto numbers = StringArray::fromTokens (text.fromFirstOccurrenceOf ("(", false, false).upToLastOccurrenceOf (")", false, false), ",", "");

    if (numbers.size() != 4)
        return {};

    for (const auto& number : numbers)
        if (number.trim().isEmpty() || ! number.trim().containsOnly ("0123456789."))
            return {};

    const Size<float> size { numbers[2].getFloatValue(), numbers[3].getFloatValue() };

    if (jmin (size.getWidth(), size.getHeight()) < 1.0f || jmax (size.getWidth(), size.getHeight()) > largestSide)
        return {};

    return size;
}

// True for a component that is of a target: one its drawing paints, or else one of its type
bool isUseOf (const Component& component, const YuiToolTarget& target)
{
    if (target.drawing.isEmpty())
        return component.getTypeInfo().getTypeName().toString() == target.hostType;

    const auto* drawing = dynamic_cast<const DrawingStyle*> (component.getStyle().get());

    return drawing != nullptr && drawing->getName() == target.drawing;
}

// True for a component of an interface that is not hidden, itself or by what holds it, and paints inside it
bool showsWithSize (const Component& component, const Component& root)
{
    for (const auto* shown = &component; shown != nullptr && shown != &root; shown = shown->getParentComponent())
        if (! shown->isVisible())
            return false;

    const auto area = findYuiToolPaintArea (component, root);

    return area.getWidth() >= 1.0f && area.getHeight() >= 1.0f;
}

void collectUses (const YuiBuilder& builder, Component& component, const Component& root, const YuiToolTarget& target, std::vector<Component*>& uses)
{
    if (builder.findElementFor (&component).isValid() && isUseOf (component, target) && showsWithSize (component, root))
        uses.push_back (&component);

    for (int index = 0; index < component.getNumChildComponents(); ++index)
        collectUses (builder, *component.getChildComponent (index), root, target, uses);
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

YuiToolShown loadYuiToolTarget (YuiDocument& document, YuiLoader& loader, const File& input, YuiToolTarget target, Size<float> size, Size<float> fallbackSize)
{
    YuiToolShown shown;
    shown.result = loader.loadFile (input);

    // The files of its scenes are there, with what they have wrong, before the interface is told about or drawn
    if (auto* root = document.getRoot())
        finishLoadingYuiToolScenes (*root);

#if YUP_WASM
    // The host of the module gave it the files: it is told those the document names that it was not given
    noteYuiToolMissingFiles (document);
#endif

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
        const auto firstPalette = findDeclaration (parsed.document, "palette").getProperty ("name").toString();

        if (target.drawing.isNotEmpty() && ! findDeclaration (parsed.document, "drawing", target.drawing).isValid())
            return nothing ("there is no drawing '" + target.drawing + "'");

        if (target.palette.isNotEmpty() && ! findDeclaration (parsed.document, "palette", target.palette).isValid())
            return nothing ("there is no palette '" + target.palette + "'");

        // A drawing given no host, and a component type, are shown where the interface uses them:
        // there they have the palette, the styles and the models they are made for
        const auto isUsed = target.palette.isEmpty() && target.drawing.isEmpty() != target.hostType.isEmpty();

        if (isUsed && hasRoot && shown.result.wasOk() && document.getRoot() != nullptr)
        {
            auto& root = *document.getRoot();

            auto layoutSize = size;

            if (layoutSize.getWidth() < 1.0f)
                layoutSize = root.getParentComponent() == nullptr ? root.getSize() : findYuiToolDeclaredSize (parsed.document);

            if (layoutSize.getWidth() < 1.0f || layoutSize.getHeight() < 1.0f)
                layoutSize = fallbackSize.getWidth() >= 1.0f ? fallbackSize : shown.defaultSize;

            root.setSize (layoutSize);

            if (findYuiToolUse (document, target) != nullptr)
            {
                shown.isInContext = true;
                shown.target = target;
                return shown;
            }
        }

        // A file of declarations shows the first thing it declares that can be drawn
        if (! hasRoot && ! isPreview)
        {
            target.drawing = findDeclaration (parsed.document, "drawing").getProperty ("name").toString();

            if (target.drawing.isEmpty())
                target.palette = firstPalette;
        }

        String hostText;

        if (target.palette.isNotEmpty())
        {
            hostText = makePaletteSample (target.palette);
            shown.defaultSize = { 360.0f, 300.0f };
        }
        else if (target.drawing.isNotEmpty() || target.hostType.isNotEmpty())
        {
            // What stands in for the interface has none of its styles: the first palette of the file is the
            // one its drawings most likely read the colors of
            hostText = (target.hostType.isNotEmpty() ? target.hostType : String ("Component")) + " { "
                     + (firstPalette.isEmpty() ? String() : "palette: " + firstPalette + "; ")
                     + (target.drawing.isNotEmpty() ? "drawing: " + target.drawing + "; " : String()) + "}";

            const auto viewBox = target.drawing.isNotEmpty() ? findViewBoxSize (findDeclaration (parsed.document, "drawing", target.drawing)) : Size<float>();

            shown.defaultSize = viewBox.getWidth() >= 1.0f ? viewBox : Size<float> (200.0f, 200.0f);
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
            shown.result = document.load (withRoot (parsed.document, host.document.getChild (0)), hasRoot ? DataTreeSourceMap() : parsed.sourceMap);

            const auto& diagnostics = document.getDiagnostics();

            shown.needsHost = target.drawing.isNotEmpty() && target.hostType.isEmpty()
                           && std::any_of (diagnostics.begin(), diagnostics.end(), [] (const DataTreeDiagnostic& diagnostic)
            {
                return diagnostic.isError() && diagnostic.message.startsWith ("Component.drawing: ") && diagnostic.message.contains ("Unknown symbol '");
            });
        }
    }

    shown.target = target;
    return shown;
}

std::vector<Component*> findYuiToolUses (YuiDocument& document, const YuiToolTarget& target)
{
    std::vector<Component*> uses;

    if (auto* root = document.getRoot(); root != nullptr && (target.drawing.isNotEmpty() || target.hostType.isNotEmpty()))
        collectUses (document.getBuilder(), *root, *root, target, uses);

    return uses;
}

Component* findYuiToolUse (YuiDocument& document, const YuiToolTarget& target)
{
    const auto uses = findYuiToolUses (document, target);

    return uses.empty() ? nullptr : uses.front();
}

String describeYuiToolName (const Component& component)
{
    const auto id = component.getComponentID();

    return component.getTypeInfo().getTypeName().toString() + (id.isNotEmpty() ? " #" + id : String());
}

Point<float> findYuiToolOrigin (const Component& component, const Component& root)
{
    Point<float> origin;

    for (const auto* placed = &component; placed != nullptr && placed != &root; placed = placed->getParentComponent())
        origin = origin + placed->getPosition();

    return origin;
}

Rectangle<float> clipYuiToolRegion (const Rectangle<float>& region, const Rectangle<float>& whole)
{
    const auto x = std::floor (region.getX());
    const auto y = std::floor (region.getY());

    return Rectangle<float> (x, y, std::ceil (region.getRight()) - x, std::ceil (region.getBottom()) - y).intersection (whole);
}

Rectangle<float> findYuiToolPaintArea (const Component& component, const Component& root)
{
    return clipYuiToolRegion (component.getPaintBounds().translated (findYuiToolOrigin (component, root)), root.getLocalBounds());
}

//==============================================================================

bool isYuiToolNumber (const String& text)
{
    return text.containsAnyOf ("0123456789") && text.containsOnly ("0123456789.") && text.indexOfChar ('.') == text.lastIndexOfChar ('.');
}

std::optional<Size<float>> parseYuiToolSize (const String& text)
{
    const auto width = text.upToFirstOccurrenceOf ("x", false, true);
    const auto height = text.fromFirstOccurrenceOf ("x", false, true);

    if (! isYuiToolNumber (width) || ! isYuiToolNumber (height))
        return std::nullopt;

    const Size<float> size { width.getFloatValue(), height.getFloatValue() };

    if (jmin (size.getWidth(), size.getHeight()) < 1.0f || jmax (size.getWidth(), size.getHeight()) > maximumYuiToolSize)
        return std::nullopt;

    return size;
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
    shown = loadYuiToolTarget (document, loader, input, options.target, options.size, options.fallbackSize);

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

    // Nothing is under a pointer in a component that is not visible
    backdrop->setVisible (true);

    if (shown.isInContext)
        use = findYuiToolUse (document, shown.target);
}

YuiToolView::~YuiToolView()
{
    // The interface goes before the theme it was styled with
    backdrop.reset();
    document.clear();

    ApplicationTheme::getGlobalTheme()->setPalette (previousPalette);
}

Rectangle<float> YuiToolView::getFrame() const
{
    if (backdrop == nullptr)
        return {};

    const auto whole = backdrop->getLocalBounds();

    if (use == nullptr)
        return whole;

    // A use that lost its size since it was found leaves all of the interface to show
    const auto area = findYuiToolPaintArea (*use.get(), *document.getRoot());

    return area.getWidth() >= 1.0f && area.getHeight() >= 1.0f ? area : whole;
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
    // brings those running to their end, which may in turn change what other styles read. The files a scene was
    // asked for by then are waited for first, for the pass to style it loaded
    for (int pass = 0; pass < 4; ++pass)
    {
        finishLoadingYuiToolScenes (*root);
        context->refresh (3600.0);
    }

    context->restyleNow();
}

//==============================================================================

YuiToolFrame::YuiToolFrame (Component& interfaceToShow, const Rectangle<float>& region)
    : shown (interfaceToShow)
{
    setOpaque (false);
    setSize (region.getSize());

    addAndMakeVisible (shown);
    shown.setTopLeft ({ -region.getX(), -region.getY() });
}

YuiToolFrame::~YuiToolFrame()
{
    removeChildComponent (shown);
    shown.setTopLeft ({ 0.0f, 0.0f });
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
