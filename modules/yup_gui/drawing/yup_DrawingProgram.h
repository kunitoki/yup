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
      but not including, `to`, exposed to them under `name`.

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

    @see DrawingStyle, DrawingScope, ExpressionDrawing

    @tags{UI}
*/
class YUP_API DrawingProgram
{
public:
    //==============================================================================
    /** Compiles a drawing tree for a component.

        @param target     The component whose properties and bounds the drawing reads.
        @param drawing    The drawing tree, with a root of type `drawing`.

        @return A compiled program on success, or a failure whose message starts with the
                path of the offending element, like `Knob/layer[0]/arc[1].to`.
    */
    static ResultValue<std::unique_ptr<DrawingProgram>> create (Component& target, const DataTree& drawing);

    /** Destructor. */
    ~DrawingProgram();

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

private:
    //==============================================================================
    struct Node;
    class Compiler;

    explicit DrawingProgram (Component& target);

    bool prepare (Node& node, Rectangle<float> bounds, const Font& font);
    void drawNodes (Graphics& g, std::vector<std::unique_ptr<Node>>& children, Rectangle<float> bounds, const Font& font, int numRows);
    void drawNode (Graphics& g, Node& node, Rectangle<float> bounds, const Font& font, int numRows);

    ExpressionContext context;
    ExpressionInput<Rectangle<float>> boundsInput;
    ExpressionInput<Font> fontInput;
    ExpressionInput<int> indexInput;
    DrawingScope scope;
    std::vector<std::unique_ptr<Node>> nodes;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawingProgram)
};

} // namespace yup
