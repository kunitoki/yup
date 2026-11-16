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
      `mask` children): draws its children into a transparency layer;
    - `clip` (`bounds`/`rect`, `path`/`clipPath`, `visible`): clips its children;
    - `group` (`transform`, `visible`): draws its children under a transform;
    - `repeat` (`name`, `from`, `to`): draws its shapes once for every int from `from` up to,
      but not including, `to`, exposed to them under `name`;
    - `property` (`name`, `type`, `value`), only at the top of the drawing: an input of the
      drawing, visible to every element.

    Every property value is an expression. It resolves the component's reflected properties
    by name (see DrawingScope), the component's local bounds as `bounds`, the theme's default
    font as `defaultFont`, and the lets in scope. Names starting with `_` are reserved.

    @code
    auto style = DrawingStyle::create (slider, drawing);
    if (style.wasOk())
        slider.setStyle (style.getReference());
    @endcode

    Styles created from the same drawing share one compilation while any of them exists,
    as long as their components resolve every name the drawing reads to the same type.
    The style is bound to the component it was created for: install it on that component only.

    @see DrawingScope, ComponentStyle, ExpressionDrawing

    @tags{UI}
*/
class YUP_API DrawingStyle : public ComponentStyle
{
public:
    //==============================================================================
    /** A pointer to a DrawingStyle object. */
    using Ptr = ReferenceCountedObjectPtr<DrawingStyle>;

    //==============================================================================
    /** Compiles a drawing tree for a component (or reuses an equivalent compilation).

        @param target             The component the style is created for and installed on.
        @param drawing            The drawing tree, with a root of type `drawing`.
        @param componentResolver  Optional resolver for named components and models referenced by expressions.
        @param failure            Optional diagnostic populated with the offending child element and property on failure.

        @return A style on success, or a failure whose message starts with the path of the offending element.
    */
    static ResultValue<DrawingStyle::Ptr> create (Component& target,
                                                  const DataTree& drawing,
                                                  DrawingScope::ComponentResolver componentResolver = nullptr,
                                                  YuiDiagnostic* failure = nullptr);

    /** Destructor. */
    ~DrawingStyle() override;

    //==============================================================================
    /** Returns the properties the drawing declares, in declaration order. */
    const std::vector<ComponentProperty>& getProperties() const noexcept;

    //==============================================================================
    /** Feeds the component's live state and paints the drawing elements in order. */
    void paint (Graphics& g, const ApplicationTheme& theme, const Component& component) override;

private:
    //==============================================================================
    struct Node;
    struct Compiled;
    struct Slot;
    class Compiler;
    class CompiledCache;

    static constexpr int maxRepeatCount = 4096;

    DrawingStyle (Component& target, DrawingScope::ComponentResolver componentResolver);

    void instantiate (std::shared_ptr<const Compiled> compiledToUse);
    Slot* prepare (const Node& node, Rectangle<float> bounds, const Font& font);
    void drawNodes (Graphics& g, const std::vector<std::unique_ptr<Node>>& children, Rectangle<float> bounds, const Font& font, int numRows);
    void drawNode (Graphics& g, const Node& node, Rectangle<float> bounds, const Font& font, int numRows);

    WeakReference<Component> targetComponent;
    ExpressionContext context;
    DrawingScope scope;
    std::shared_ptr<const Compiled> compiled;
    std::unique_ptr<Slot[]> slots;
    std::vector<var> propertyValues;
    std::vector<DrawingScope::ArrayInput> propertyArrays;
    std::vector<DrawingScope::Dependency> dependencies;
    std::unique_ptr<DependencyObserver> observer;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawingStyle)
};

} // namespace yup
