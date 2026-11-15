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

//==============================================================================
/**
    A drawing compiled once against a single component, from a drawing tree.

    A drawing is a tree of elements, the DataTree form of a `drawing` block in a `.yui`
    file. Its root has the type `drawing` and its children are drawn in order:

    - shapes: `rect`, `ellipse`, `circle`, `arc`, `line`, `path`, `polygon`, `star` and `text`,
      each with its own geometry and paint properties;
    - `let` (`name`, `value`): a local visible to the following siblings and their children;
    - `layer` (`opacity`, `visible`): draws its children into a transparency layer;
    - `group` (`transform`, `visible`): draws its children under a transform;
    - `repeat` (`name`, `from`, `to`): draws its shapes once for every int from `from` up to,
      but not including, `to`, exposed to them under `name`;
    - `property` (`name`, `type`, `value`), only at the top of the drawing: an input of the
      drawing, visible to every element, see below.

    Every property value is an expression. It resolves the component's reflected properties
    by name (see DrawingScope), the component's local bounds as `bounds`, the theme's default
    font as `defaultFont` (for example `font: defaultFont.withHeight (14)`), and the lets in
    scope. Each shape compiles to its own small program, so shapes are styled independently.
    Names starting with `_` are reserved.

    In a `.yui` file:

    @code
    drawing Knob {
        let r: min (bounds.width, bounds.height) * 0.5 - 4;
        circle { center: bounds.center; radius: r - 6; fill: backgroundColor; }
        arc { center: bounds.center; radius: r; from: 135deg; to: lerp (135deg, 405deg, normalized);
              stroke: thumbColor; strokeWidth: 3; cap: round; }
    }
    @endcode

    Create one with @ref create, which fails with a message naming the offending element
    when the tree is malformed or an expression doesn't compile. Then call @ref draw from
    a paint method. A DrawingProgram is bound to the component it was created for and must
    not outlive it.

    Programs created from the same drawing share one compilation while any of them exists,
    as long as their components resolve every name the drawing reads to the same type, so
    a drawing used by many components (several knobs, say) is compiled once. Only the
    evaluation state is per component. Sharing follows the content of the drawing tree, so
    editing the tree and creating a program again compiles the edited drawing.

    @see DrawingStyle, DrawingScope, ExpressionDrawing

    @tags{UI}
*/
class YUP_API DrawingProgram
    : private ComponentListener
    , private BindingProvider::Listener
{
public:
    //==============================================================================
    /** Compiles a drawing tree for a component, or reuses the compilation of another program.

        @param target             The component whose properties and bounds the drawing reads.
        @param drawing            The drawing tree, with a root of type `drawing`.
        @param componentResolver  Optional resolver for named components referenced by expressions.
        @param modelResolver      Optional resolver for BindingProvider models referenced by expressions.

        @return A compiled program on success, or a failure whose message starts with the
                path of the offending element, like `Knob/layer[0]/arc[1].to`.
    */
    static ResultValue<std::unique_ptr<DrawingProgram>> create (Component& target,
                                                                const DataTree& drawing,
                                                                DrawingScope::ComponentResolver componentResolver = nullptr,
                                                                DrawingScope::ModelResolver modelResolver = nullptr);

    /** Destructor. */
    ~DrawingProgram() override;

    //==============================================================================
    /** Feeds the component's live state and draws the elements in order.

        Writes the component's local bounds into `bounds` and the theme's default font into
        `defaultFont`, pushes its reflected property values into every program and draws the
        tree. The Graphics state is restored on return.

        @param g            The graphics context to draw into.
        @param theme        The theme providing `defaultFont`, also used by `text` elements without a `font`.
        @param component    The component being painted, whose bounds feed `bounds`.
    */
    void draw (Graphics& g, const ApplicationTheme& theme, const Component& component);

    //==============================================================================
    /** Returns the properties the drawing declares, in declaration order.

        Each descriptor carries the name and type of a `property` element. Defaults are
        expressions evaluated while drawing, so the descriptors have no default value.
    */
    const std::vector<ComponentProperty>& getProperties() const noexcept;

private:
    //==============================================================================
    struct Node;
    struct Compiled;
    struct Slot;
    class Compiler;
    class CompiledCache;

    explicit DrawingProgram (Component& target,
                             DrawingScope::ComponentResolver componentResolver,
                             DrawingScope::ModelResolver modelResolver);

    void instantiate (std::shared_ptr<const Compiled> compiledToUse);
    Slot* prepare (const Node& node, Rectangle<float> bounds, const Font& font);
    void drawNodes (Graphics& g, const std::vector<std::unique_ptr<Node>>& children, Rectangle<float> bounds, const Font& font, int numRows);
    void drawNode (Graphics& g, const Node& node, Rectangle<float> bounds, const Font& font, int numRows);

    void componentPropertyChanged (Component& component, const Identifier& property) override;
    void componentMoved (Component& component) override;
    void componentResized (Component& component) override;
    void componentBeingDeleted (Component& component) override;
    void bindingValueChanged (BindingProvider& provider, const Identifier& name) override;
    void bindingProviderBeingDeleted (BindingProvider& provider) override;

    WeakReference<Component> targetComponent;
    ExpressionContext context;
    DrawingScope scope;
    std::shared_ptr<const Compiled> compiled;
    std::unique_ptr<Slot[]> slots;
    std::vector<var> propertyValues;
    std::vector<DrawingScope::ArrayInput> propertyArrays;
    std::vector<DrawingScope::Dependency> dependencies;
    std::set<Component*> observedComponents;
    std::set<BindingProvider*> observedProviders;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawingProgram)
};

} // namespace yup
