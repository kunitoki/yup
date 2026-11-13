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
/** Draws vector graphics described by an expression program, once per data row.

    The program assigns well known outputs, evaluated for every row and turned into Graphics calls:

    | Output                | Type             | Meaning                                                        |
    |-----------------------|------------------|----------------------------------------------------------------|
    | `visible`             | `bool`           | Skips the row when false.                                      |
    | `rect`                | `rect`           | A rectangle, rounded by `cornerRadius` (`float`) when assigned.|
    | `topLeftRadius`...    | `float`          | Per corner radii of `rect` (`topLeftRadius`, `topRightRadius`, |
    |                       |                  | `bottomLeftRadius`, `bottomRightRadius`), default cornerRadius.|
    | `ellipse`             | `rect`           | An ellipse fitting the rectangle.                              |
    | `point`               | `point`          | A circle of `radius` (`float`, default 2) centered on it.      |
    | `line`                | `line`           | A stroked line.                                                |
    | `path`                | `path`           | A path, filled with its own winding rule.                      |
    | `vertex`              | `point`          | A vertex of the polyline of the row `series` (`int`, default 0)|
    | `label`               | `string`         | Text fitted in `labelRect` (`rect`).                           |
    | `fill`                | `color`          | Fills the shapes of the row.                                   |
    | `fillGradient`        | `gradient`       | Fills the shapes with a gradient, instead of `fill`.           |
    | `stroke`              | `color`          | Strokes the shapes of the row.                                 |
    | `strokeGradient`      | `gradient`       | Strokes the shapes with a gradient, instead of `stroke`.       |
    | `strokeWidth`         | `float`          | Stroke width.                                                  |
    | `strokeCap`           | `cap`            | `butt`, `round` or `square` line ends.                         |
    | `strokeJoin`          | `join`           | `miter`, `round` or `bevel` corners.                           |
    | `strokeMiterLimit`    | `float`          | Longest miter join, in stroke widths.                          |
    | `strokePosition`      | `strokePosition` | Stroke `inside`, `center` or `outside` the shapes.             |
    | `opacity`             | `float`          | Multiplies the opacity of the Graphics.                        |
    | `feather`             | `float`          | Blurs the edges of the shapes (glows and shadows).             |
    | `blend`               | `blend`          | Blend mode (`multiply`, `screen`, `additive`...).              |
    | `tint`                | `color`          | Tint applied to the colors.                                    |
    | `transform`           | `transform`      | Transforms the shapes and labels of the row.                   |
    | `clip`                | `rect`           | Clips the row, in the coordinates of the drawing.              |
    | `clipPath`            | `path`           | Clips the row to a path, in the coordinates of the drawing.    |
    | `labelColor`          | `color`          | Fills the label.                                               |
    | `labelStroke`         | `color`          | Strokes the label with `strokeWidth`.                          |
    | `labelFont`           | `font`           | Font of the label, instead of the one passed to draw.          |
    | `labelJustification`  | `justification`  | Placement of the label in `labelRect`, default `center`.       |

    Shapes are filled with `fill` (or `fillGradient`) and stroked with `stroke` (or `strokeGradient`)
    when assigned; when neither is assigned they are filled with the current fill color of the Graphics.
    Lines are stroked with the stroke paint, or the fill paint when it is the only one assigned. Each
    `series` draws one polyline joining its visible rows in order, with the style of its first visible
    row: stroked with the stroke paint (or the current stroke color) and filled with the fill paint when
    assigned. Polylines are drawn after the other shapes. Labels are filled with `labelColor` (or the
    fill paint of the row) unless only `labelStroke` is assigned, and stroked with `labelStroke`.

    The `transform` of a row is applied before the transform of the Graphics, so shapes are positioned
    in the coordinates of the drawing as usual; `clip` and `clipPath` are not transformed by it. Bare
    enumeration values like `round` resolve to the type of the output they are assigned to.

    Consecutive rows drawn with the same style are merged into a single path, and drawn at once, when
    it can't change the result: when filled shapes don't overlap, or when shapes are only filled or only
    stroked with an opaque color (overlapping filled shapes must also be of the same kind, and not paths).
    Feathered rows are never merged, and overlapping shapes aren't merged when drawn with gradients,
    opacity, blend modes, translucent tints or strokes not centered on the shapes.

    The graphics types (see addGraphicsExpressionTypes) are always available and outputs are converted
    to the expected types, so `strokeWidth = 2` assigns a float.

    Compiling happens once in create; draw only evaluates the compiled program. A program compiled
    elsewhere (with a context prepared by declareOutputs) can be wrapped without compiling it again.

    Per row data comes from the columns and arrays bound to the ExpressionState, per draw values from
    its inputs:

    @code
    ExpressionContext context;
    auto values = context.addArray<float> ("values");
    auto row = context.addInput<int> ("row");
    auto bounds = context.addInput<Rectangle<float>> ("bounds");

    auto drawing = ExpressionDrawing::create (R"(
        let w = bounds.width / len (values);
        let h = values[row] * bounds.height;
        rect = rect (bounds.x + row * w, bounds.bottom - h, w - 2, h);
        fill = values[row] > 0.5 ? rgb (255, 80, 80) : rgb (80, 160, 255);
    )", context);

    ExpressionState state (drawing.getReference().getProgram());
    state.bindArray (values, data.data(), (int) data.size());
    state.bindRowIndex (row);
    state.set (bounds, getLocalBounds());

    drawing.getReference().draw (g, state, (int) data.size());
    @endcode

    @see CompiledExpression, ExpressionState, addGraphicsExpressionTypes

    @tags{Graphics}
*/
class YUP_API ExpressionDrawing
{
public:
    //==============================================================================
    /** Creates an empty drawing, which draws nothing. */
    ExpressionDrawing();

    /** Compiles a drawing program against a context.

        Fails when the program doesn't compile, when an output has an unexpected type, when no shape
        output is assigned or when `label` and `labelRect` are not assigned together.
    */
    static ResultValue<ExpressionDrawing> create (StringRef text, const ExpressionContext& context);

    /** Wraps an already compiled program, without compiling it again.

        Use it to share one compiled program between drawings, or to compile once and draw many times.
        Compile the program with a context passed to declareOutputs, so outputs like `strokeWidth = 2`
        are converted to the expected types; outputs with unexpected types are ignored.
    */
    static ResultValue<ExpressionDrawing> create (const CompiledExpression& program);

    /** Registers the graphics types and declares the expected types of the drawing outputs on a context. */
    static void declareOutputs (ExpressionContext& context);

    //==============================================================================
    /** Returns the compiled program, used to create the ExpressionState passed to draw. */
    const CompiledExpression& getProgram() const noexcept { return program; }

    //==============================================================================
    /** Keeps the shapes built by draw, to draw them again without rebuilding them while the outputs don't change.

        Pass the same cache to every draw of a drawing: when the outputs of all the rows, the number of rows
        and the merging conditions of the Graphics (opacity, feather, blend mode, tint, stroke position) match
        the previous draw, the cached shapes are drawn again as they are, which saves building their paths
        and lets the renderer reuse them. Programs are still evaluated on every draw.

        Paths assigned to `path` and `clipPath` are compared by identity (copies of a Path share its data), so assign new
        paths instead of modifying a path in place.

        A cache is not thread safe and must only be used by one drawing at a time.
    */
    class YUP_API Cache
    {
    public:
        /** Creates an empty cache. */
        Cache();
        ~Cache();

        /** Returns true if the last draw using this cache had to rebuild its shapes. */
        bool wasRebuilt() const noexcept;

        /** Forgets the cached shapes, so the next draw rebuilds them. */
        void reset();

    private:
        friend class ExpressionDrawing;
        struct Data;

        std::unique_ptr<Data> data;

        YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Cache)
    };

    //==============================================================================
    /** Evaluates rows 0 to numRows - 1 and draws them.

        The state must have been created from getProgram(). The Graphics state is restored on return.
        Labels are only drawn when a font is provided, or assigned to `labelFont`. When a cache is provided, the shapes of the previous
        draw are reused as long as the outputs don't change (see Cache).
    */
    void draw (Graphics& g, ExpressionState& state, int numRows = 1, const Font* labelFont = nullptr, Cache* cache = nullptr) const;

private:
    template <template <class> class Field>
    struct Fields;
    struct DrawBuffers;

    using Outputs = Fields<ExpressionOutput>;

    CompiledExpression program;
    std::shared_ptr<const Outputs> outputs;

    YUP_LEAK_DETECTOR (ExpressionDrawing)
};

} // namespace yup
