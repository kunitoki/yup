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

namespace yup
{

class DependencyObserver;
struct YuiDiagnostic;

//==============================================================================
/**
    A component style that paints a component from a compiled `.yui` drawing tree.

    A drawing is a tree of elements, the DataTree form of a `drawing` block in a `.yui`
    file. Its root has the type `drawing` and its children are drawn in order:

    - shapes: `rect`, `ellipse`, `circle`, `arc`, `line`, `path`, `polygon`, `star`, `text`,
      `image` and `svg`, each with its own geometry and paint properties (including `dash`
      and `dashOffset` for stroked shapes);
    - `let` (`name`, `value`): a local visible to the following siblings and their children;
    - `layer` (`opacity`, `blend`, `clip`, `clipPath`, `mask`, `maskMode`, `visible` and optional
      `mask` and `clipPath` children): draws its children into a transparency layer. With a `filter`
      child (`bounds`, `linearColors` and the primitives `blur`, `offset`, `flood`, `colorMatrix`,
      `composite`, `blend`, `morphology`, `lighting`, `displacement` and `dropShadow`) it draws
      them through a FilterGraph instead;
    - `clip` (`bounds`/`rect`, `path`/`clipPath`, `visible` and optional `clipPath` children
      holding the shapes to clip to): clips its children;
    - `group` (`transform`, `opacity`, `blend`, `visible`, `color` and paint properties): draws its
      children under a transform, with its paint properties as the defaults of the shapes inside it;
    - `viewport` (`bounds`, `viewBox`, `fitting`, `justification`, `visible`): draws its children
      in the coordinates of a view box fitted in a rectangle, which it clips them to;
    - `repeat` (`name`, `from`, `to`): draws its shapes once for every int from `from` up to,
      but not including, `to`, exposed to them under `name`;
    - `drawing`, `pattern` and `marker` (`name`): definitions visible to the elements after them,
      a drawing painted by `use`, a tile painting shapes through `fillPattern` and `strokePattern`,
      and content drawn at the vertices of shapes through `markerStart`, `markerMid` and `markerEnd`;
    - `use` (`drawing`, `bounds`, `transform`, `opacity`, `blend`, `visible`, `color`, paint
      properties and the properties of the drawing): paints a drawing defined inside this one, or
      found by name through the DrawingResolver;
    - `property` (`name`, `type`, `value`), only at the top of the drawing: an input of the
      drawing, visible to every element.

    The root accepts `viewBox`, `fitting` and `justification` to draw its elements in the coordinates
    of a view box fitted in the bounds of the component.

    Every property value is an expression. It resolves the component's reflected properties
    by name (see DrawingScope), the bounds its element is drawn in as `bounds` (the component's
    local bounds, a view box or the bounds given to a `use`), the theme's default font as
    `defaultFont`, and the lets in scope. Names starting with `_` are reserved.

    @code
    auto style = DrawingStyle::create (slider, drawing);
    if (style.wasOk())
        slider.setStyle (style.getReference());
    @endcode

    Styles created from the same drawing share one compilation while any of them exists,
    as long as their components resolve every name the drawing reads to the same type.
    The style is bound to the component it was created for: install it on that component only.

    @see DrawingScope, ComponentStyle, ExpressionDrawing, YuiSvgImporter

    @tags{UI}
*/
class YUP_API DrawingStyle : public ComponentStyle
{
public:
    //==============================================================================
    /** A pointer to a DrawingStyle object. */
    using Ptr = ReferenceCountedObjectPtr<DrawingStyle>;

    /** Finds the drawing a `use` element refers to by name, when it isn't defined inside the drawing being compiled.

        Return the tree of the drawing, with a root of type `drawing`, or an invalid tree for an unknown name.
    */
    using DrawingResolver = std::function<DataTree (const String& name)>;

    //==============================================================================
    /** Compiles a drawing tree for a component (or reuses an equivalent compilation).

        @param target             The component the style is created for and installed on.
        @param drawing            The drawing tree, with a root of type `drawing`.
        @param componentResolver  Optional resolver for named components and models referenced by expressions.
        @param failure            Optional diagnostic populated with the offending child element and property on failure.
        @param drawingResolver    Optional resolver for the drawings `use` elements refer to by name. It is only
                                  called while creating the style.

        @return A style on success, or a failure whose message starts with the path of the offending element.
    */
    static ResultValue<DrawingStyle::Ptr> create (Component& target,
                                                  const DataTree& drawing,
                                                  DrawingScope::ComponentResolver componentResolver = nullptr,
                                                  YuiDiagnostic* failure = nullptr,
                                                  const DrawingResolver& drawingResolver = nullptr);

    /** Destructor. */
    ~DrawingStyle() override;

    //==============================================================================
    /** An element a drawing is made of, as getElementDescriptions() lists them. */
    struct ElementDescription
    {
        /** The name of the element, such as `rect`. */
        String type;

        /** What the element is: `shape`, `media`, `container`, `definition`, `declaration`,
            `filter` or `filterPrimitive`.
        */
        String role;

        /** The properties placing a shape, in the order they are written. One with a default
            is followed by it, as `angle=0`. Empty for what is not a shape.
        */
        StringArray geometry;

        /** The other properties the element takes. A shape takes those of
            getShapePaintProperties() on top of them, and a `use` the properties its drawing
            declares.
        */
        StringArray properties;
    };

    /** Returns the elements a drawing can hold with the properties of each, as they are checked
        when a drawing is compiled, for a tool to list them.

        A `text` is listed twice: fitted in its `bounds`, and placed at its `anchor`.
    */
    static std::vector<ElementDescription> getElementDescriptions();

    /** Returns the paint properties every shape takes, such as `fill` and `stroke`. */
    static StringArray getShapePaintProperties();

    //==============================================================================
    /** Returns the properties the drawing declares, in declaration order. */
    const std::vector<ComponentProperty>& getProperties() const noexcept;

    //==============================================================================
    /** Finds the image a `source` names, or returns nothing for it to be loaded as usual. */
    using ImageResolver = std::function<std::optional<Image> (const String& source)>;

    /** Sets where the `image` elements look for their images first.

        An `image` loads the file its `source` names, or the content of a `data:` source. A resolver
        gives it images from anywhere else, such as the network or the memory. It is called while
        painting, once for each source, and what it returns is kept.

        @param newResolver The resolver, or nullptr to only load files and `data:` sources.
    */
    void setImageResolver (ImageResolver newResolver);

    //==============================================================================
    /** Feeds the component's live state and paints the drawing elements in order. */
    void paint (Graphics& g, const ApplicationTheme& theme, const Component& component) override;

    //==============================================================================
    /** Where an element of the drawing lands in the component it paints, as measureElements() tells it. */
    struct ElementBounds
    {
        /** The path of the element in the drawing, as the problems of a drawing tell it: `Knob/group[0]/arc#value`. */
        String path;

        /** The name of the element, such as `arc`. */
        String type;

        /** The id of the element, empty when it has none. */
        String id;

        /** The box of what the element paints, in the coordinates of the component: its geometry
            under the transforms of the groups and the viewports around it, without the width of
            its stroke. For a `text` fitted in a rectangle it is that rectangle, and for a container
            the box of what it holds. Empty when it paints nothing.
        */
        Rectangle<float> bounds;

        /** For a `text`, the area its glyphs cover. Empty for other elements. */
        Rectangle<float> textBounds;

        /** How many times the element is painted: more than once inside a `repeat`, where the
            bounds cover them all.
        */
        int numRows = 1;
    };

    /** Evaluates the drawing for a component as paint() does, and returns where each of its
        elements lands without drawing any.

        The elements are told in the order they are painted, a container before what it holds.
        Those not painted are left out: what a container that is not visible holds, and the masks,
        clip shapes, patterns, markers and filters, which paint nothing of their own. Texts are
        measured with the default font of the global theme.

        @param component  The component the drawing paints, usually the one the style is set on.
    */
    std::vector<ElementBounds> measureElements (const Component& component);

private:
    //==============================================================================
    struct Node;
    struct Compiled;
    struct Slot;
    struct PaintContext;
    struct PatternSource;
    struct FilterSource;
    class Compiler;
    class CompiledCache;

    static constexpr int maxRepeatCount = 4096;

    DrawingStyle (Component& target, DrawingScope::ComponentResolver componentResolver);

    void instantiate (std::shared_ptr<const Compiled> compiledToUse);
    Slot* prepare (const Node& node, const PaintContext& context);
    PatternSource& preparePattern (int index, const PaintContext& context);
    void drawPattern (Graphics& g, const PatternSource& source);
    void drawMarkers (Graphics& g, const Node& node, Slot& slot, const PaintContext& context);
    Path collectOutline (const std::vector<std::unique_ptr<Node>>& shapes, const PaintContext& context, int numRows);
    void applyClips (Graphics& g, const Node& node, Slot* slot, const PaintContext& context, int numRows);
    void drawNodes (Graphics& g, const std::vector<std::unique_ptr<Node>>& children, const PaintContext& context, int numRows);
    Rectangle<float> readComponent (const Component& component);
    bool enterGroupBounds (const Node& node, Slot& slot, AffineTransform& mapping, std::optional<Rectangle<float>>& clip);
    Rectangle<float> measureNodes (const std::vector<std::unique_ptr<Node>>& children, const PaintContext& context, const AffineTransform& transform, int numRows, std::vector<ElementBounds>& measured);
    Rectangle<float> measureNode (const Node& node, const PaintContext& context, const AffineTransform& transform, int numRows, std::vector<ElementBounds>& measured);
    void drawNode (Graphics& g, const Node& node, const PaintContext& context, int numRows);
    int getRepeatCount (const Node& node, const Slot& slot) const;
    bool drawFilteredLayer (Graphics& g, const Node& node, const PaintContext& context, int numRows);
    void drawFilteredContent (Graphics& g, const FilterSource& source);

    WeakReference<Component> targetComponent;
    ExpressionContext context;
    DrawingScope scope;
    std::shared_ptr<const Compiled> compiled;
    std::unique_ptr<Slot[]> slots;
    std::vector<Rectangle<float>> levelBounds;
    std::vector<int> loopIndices;
    int numLoopTurns = 1;
    std::vector<std::unique_ptr<PatternSource>> patternSources;
    ImageResolver imageResolver;
    std::map<const Node*, std::unique_ptr<FilterSource>> filterSources;
    const Font* paintFont = nullptr;
    uint32 paintSerial = 0;
    std::vector<var> propertyValues;
    std::vector<DrawingScope::ArrayInput> propertyArrays;
    std::vector<DrawingScope::Dependency> dependencies;
    std::unique_ptr<DependencyObserver> observer;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawingStyle)
};

} // namespace yup
