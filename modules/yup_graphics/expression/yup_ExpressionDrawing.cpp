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

namespace yup
{

//==============================================================================
/* Every output recognized by a drawing, as a Field of the output type. */
template <template <class> class Field>
struct ExpressionDrawing::Fields
{
    Field<bool> visible;
    Field<int> series;
    Field<Color> fill, stroke, tint, labelColor, labelStroke;
    Field<ColorGradient> fillGradient, strokeGradient;
    Field<PatternPaint> fillPattern, strokePattern;
    Field<float> strokeWidth, strokeMiterLimit, strokeDashOffset, opacity, fillOpacity, strokeOpacity, feather, radius, vertexSmooth;
    Field<float> cornerRadius, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius;
    Field<Size<float>> cornerSize;
    Field<StrokeDashPattern> strokeDash;
    Field<StrokeCap> strokeCap;
    Field<StrokeJoin> strokeJoin;
    Field<StrokePosition> strokePosition;
    Field<BlendMode> blend;
    Field<AffineTransform> transform;
    Field<Rectangle<float>> rect, ellipse, clip, labelRect;
    Field<Point<float>> point, vertex, vertexBase, labelAnchor;
    Field<Line<float>> line;
    Field<Path> path, clipPath;
    Field<String> label;
    Field<Font> labelFont;
    Field<Justification::Flags> labelJustification;
    Field<AnchoredText::Anchor> labelTextAnchor;
    Field<float> labelLetterSpacing;
};

/* The values of an output for the rows of a drawing, kept by a Cache. */
template <class T>
struct ExpressionDrawingColumn
{
    bool reserve (int numRows)
    {
        if (numRows <= capacity)
            return false;

        values = std::make_unique<T[]> ((size_t) numRows);
        capacity = numRows;
        return true;
    }

    bool update (Span<const T> lanes, int startRow)
    {
        if (lanes.empty())
            return false;

        auto* destination = values.get() + startRow;
        if (std::equal (lanes.begin(), lanes.end(), destination))
            return false;

        std::copy (lanes.begin(), lanes.end(), destination);
        return true;
    }

    std::unique_ptr<T[]> values;
    int capacity = 0;
};

namespace
{

template <class T>
using ExpressionDrawingLanes = const T*;

template <class F, class... Values>
void forEachDrawingField (F&& f, Values&&... fields)
{
    f ("visible", fields.visible...);
    f ("series", fields.series...);
    f ("fill", fields.fill...);
    f ("stroke", fields.stroke...);
    f ("tint", fields.tint...);
    f ("labelColor", fields.labelColor...);
    f ("labelStroke", fields.labelStroke...);
    f ("fillGradient", fields.fillGradient...);
    f ("strokeGradient", fields.strokeGradient...);
    f ("fillPattern", fields.fillPattern...);
    f ("strokePattern", fields.strokePattern...);
    f ("strokeWidth", fields.strokeWidth...);
    f ("strokeMiterLimit", fields.strokeMiterLimit...);
    f ("strokeDash", fields.strokeDash...);
    f ("strokeDashOffset", fields.strokeDashOffset...);
    f ("opacity", fields.opacity...);
    f ("fillOpacity", fields.fillOpacity...);
    f ("strokeOpacity", fields.strokeOpacity...);
    f ("feather", fields.feather...);
    f ("radius", fields.radius...);
    f ("cornerRadius", fields.cornerRadius...);
    f ("topLeftRadius", fields.topLeftRadius...);
    f ("topRightRadius", fields.topRightRadius...);
    f ("bottomLeftRadius", fields.bottomLeftRadius...);
    f ("bottomRightRadius", fields.bottomRightRadius...);
    f ("cornerSize", fields.cornerSize...);
    f ("strokeCap", fields.strokeCap...);
    f ("strokeJoin", fields.strokeJoin...);
    f ("strokePosition", fields.strokePosition...);
    f ("blend", fields.blend...);
    f ("transform", fields.transform...);
    f ("rect", fields.rect...);
    f ("ellipse", fields.ellipse...);
    f ("clip", fields.clip...);
    f ("labelRect", fields.labelRect...);
    f ("point", fields.point...);
    f ("vertex", fields.vertex...);
    f ("vertexBase", fields.vertexBase...);
    f ("vertexSmooth", fields.vertexSmooth...);
    f ("line", fields.line...);
    f ("path", fields.path...);
    f ("clipPath", fields.clipPath...);
    f ("label", fields.label...);
    f ("labelFont", fields.labelFont...);
    f ("labelJustification", fields.labelJustification...);
    f ("labelAnchor", fields.labelAnchor...);
    f ("labelTextAnchor", fields.labelTextAnchor...);
    f ("labelLetterSpacing", fields.labelLetterSpacing...);
}

/* Paints with an opacity of its own, on top of the one of the Graphics. */
template <class Paint>
void paintWithDrawingOpacity (Graphics& g, const std::optional<float>& opacity, Paint&& paint)
{
    if (! opacity || *opacity >= 1.0f)
    {
        paint();
        return;
    }

    const auto previous = g.getOpacity();
    g.setOpacity (previous * jmax (0.0f, *opacity));
    paint();
    g.setOpacity (previous);
}

template <class T>
void findDrawingOutput (const CompiledExpression& program, const char* name, ExpressionOutput<T>& output)
{
    output = program.findTypedOutput<T> (name);
}

template <class T>
void declareDrawingOutput (ExpressionContext& context, const char* name, const ExpressionOutput<T>&)
{
    context.declareOutput<T> (name);
}

/* The pixels a unit along the x of a drawing takes where a Graphics paints it: the columns of pixels a polyline is
   reduced to are counted along it, whatever else the transform does. */
float getDrawingPixelsPerUnit (const Graphics& g)
{
    const auto transform = g.getTransform();

    return g.getContextScale() * std::hypot (transform.getScaleX(), transform.getShearY());
}

} // namespace

//==============================================================================
/* Turns rows into operations, drawn at once or recorded for a Cache.

   Consecutive shapes are appended to one path and drawn together when merging can't change the
   result: shapes with the same style that don't overlap, or overlapping shapes drawn with a single
   opaque operation (filled shapes also need the same winding direction).
*/
struct ExpressionDrawing::DrawBuffers
{
    using Lanes = Fields<ExpressionDrawingLanes>;

    enum class ShapeKind
    {
        area,
        ellipse,
        line,
        path,
        mixed
    };

    struct Style
    {
        bool shouldFill = false;
        bool shouldStroke = false;
        std::optional<Color> fill, stroke, tint;
        std::optional<ColorGradient> fillGradient, strokeGradient;
        std::optional<PatternPaint> fillPattern, strokePattern;
        std::optional<float> strokeWidth, miterLimit, strokeDashOffset, opacity, fillOpacity, strokeOpacity, feather;
        std::optional<StrokeDashPattern> strokeDash;
        std::optional<StrokeCap> cap;
        std::optional<StrokeJoin> join;
        std::optional<StrokePosition> strokePosition;
        std::optional<BlendMode> blend;
        std::optional<AffineTransform> transform;
        std::optional<Rectangle<float>> clip;
        std::optional<Path> clipPath;

        bool operator== (const Style& other) const = default;

        bool hasFill() const noexcept { return fill.has_value() || fillGradient.has_value() || fillPattern.has_value(); }

        bool hasStroke() const noexcept { return stroke.has_value() || strokeGradient.has_value() || strokePattern.has_value(); }

        bool hasDash() const noexcept { return strokeDash.has_value() && ! strokeDash->isEmpty(); }

        bool isFillOnly() const noexcept { return shouldFill && ! shouldStroke; }

        bool canMerge() const noexcept { return ! feather.has_value() || *feather <= 0.0f; }

        bool canMergeOverlapping() const noexcept
        {
            if (shouldFill == shouldStroke || fillGradient || strokeGradient || fillPattern || strokePattern)
                return false;

            if (shouldStroke && hasDash())
                return false;

            if ((opacity && *opacity < 1.0f) || (blend && *blend != BlendMode::SrcOver) || (tint && ! tint->isOpaque()))
                return false;

            if ((fillOpacity && *fillOpacity < 1.0f) || (strokeOpacity && *strokeOpacity < 1.0f))
                return false;

            if (strokePosition && *strokePosition != StrokePosition::Center)
                return false;

            const auto& color = shouldFill ? fill : stroke;
            return color.has_value() && color->isOpaque();
        }

        // A pattern stays on the Graphics until its state is restored, so it has to be
        bool changesGraphicsState() const noexcept
        {
            return clip.has_value() || clipPath.has_value() || transform.has_value() || opacity.has_value()
                || fillPattern.has_value() || strokePattern.has_value();
        }

        void apply (Graphics& g) const
        {
            // Clips are in the coordinates of the drawing, the row transform only moves the shapes
            if (clip)
                g.setClipPath (*clip);

            if (clipPath)
                g.setClipPath (*clipPath);

            if (transform)
                g.setTransform (transform->followedBy (g.getTransform()));

            if (opacity)
                g.setOpacity (g.getOpacity() * *opacity);

            if (fill)
                g.setFillColor (*fill);

            if (fillGradient)
                g.setFillColorGradient (*fillGradient);

            if (stroke)
                g.setStrokeColor (*stroke);

            if (strokeGradient)
                g.setStrokeColorGradient (*strokeGradient);

            // Patterns are set after the transform they are rasterized for, and replace the colors
            constexpr ImageSampling tiled { ImageWrap::Repeat, ImageWrap::Repeat, ImageFilter::Linear };

            if (fillPattern)
                g.setFillSource (*fillPattern->source, fillPattern->transform, tiled);

            if (strokePattern)
                g.setStrokeSource (*strokePattern->source, strokePattern->transform, tiled);

            if (strokeWidth)
                g.setStrokeWidth (*strokeWidth);

            if (miterLimit)
                g.setStrokeMiterLimit (*miterLimit);

            if (cap)
                g.setStrokeCap (*cap);

            if (join)
                g.setStrokeJoin (*join);

            if (strokePosition)
                g.setStrokePosition (*strokePosition);

            if (tint)
                g.setTint (*tint);

            if (feather)
                g.setFeather (*feather);

            if (blend)
                g.setBlendMode (*blend);
        }
    };

    struct Settings
    {
        // The settings to collect outlines with: shapes merge freely, as they are never drawn
        Settings() noexcept
            : canMerge (true)
            , canMergeOverlapping (true)
            , hasLabelFont (false)
        {
        }

        Settings (const Graphics& g, bool drawsLabels) noexcept
            : canMerge (g.getFeather() <= 0.0f)
            , canMergeOverlapping (g.getOpacity() >= 1.0f
                                   && g.getBlendMode() == BlendMode::SrcOver
                                   && g.getTint().isOpaque()
                                   && g.getStrokePosition() == StrokePosition::Center)
            , hasLabelFont (drawsLabels)
        {
        }

        bool operator== (const Settings& other) const = default;

        bool canMerge, canMergeOverlapping, hasLabelFont;
    };

    /* A label, filled and stroked like the shapes of its row: fitted in an area, or placed from a point on its baseline. */
    struct Text
    {
        String text;
        Rectangle<float> area;
        std::optional<Point<float>> anchorPoint;
        AnchoredText::Anchor anchor = AnchoredText::Anchor::start;
        float letterSpacing = 0.0f;
        std::optional<Color> fill, stroke;
        std::optional<Font> font;
        Justification justification = Justification::center;
        mutable std::shared_ptr<StyledText> fittedText;
        mutable AnchoredText anchoredText;
        mutable Font shapedFont;
        mutable bool isShaped = false;

        bool changesColors() const noexcept { return fill.has_value() || stroke.has_value(); }

        void shape (const Font& textFont) const
        {
            shapedFont = textFont;
            isShaped = true;

            if (anchorPoint)
            {
                anchoredText = AnchoredText (text, textFont, *anchorPoint, anchor, letterSpacing);
                return;
            }

            fittedText = std::make_shared<StyledText>();

            auto modifier = fittedText->startUpdate();
            modifier.setMaxSize (area.getSize());
            modifier.appendText (text, textFont, -1.0f, letterSpacing);
            modifier.setHorizontalAlign (StyledText::toHorizontalAlign (justification));
            modifier.setVerticalAlign (StyledText::toVerticalAlign (justification));
        }

        /* Lays the text out with its font, or the one given when it has none. Returns false when there is nothing to draw. */
        bool prepare (const Font* defaultFont) const
        {
            if (text.isEmpty())
                return false;

            const auto* textFont = font.has_value() && ! font->isEmpty() ? &*font : defaultFont;
            if (textFont == nullptr || textFont->isEmpty())
                return false;

            if (! isShaped || shapedFont != *textFont)
                shape (*textFont);

            return ! (anchorPoint && anchoredText.isEmpty());
        }

        /* Where the text lands: the part of its area its glyphs take, or all of it. */
        Rectangle<float> getBounds (const Font* defaultFont, bool ofGlyphs) const
        {
            if (! prepare (defaultFont))
                return {};

            if (anchorPoint)
                return anchoredText.getBounds();

            if (! ofGlyphs)
                return area;

            const auto offset = fittedText->getOffset (area);
            return fittedText->getComputedTextBounds().translated (area.getX() + offset.getX(), area.getY() + offset.getY());
        }

        void draw (Graphics& g, const Font* defaultFont, const std::optional<float>& fillOpacity, const std::optional<float>& strokeOpacity) const
        {
            if (! prepare (defaultFont))
                return;

            const auto& shaped = anchorPoint ? anchoredText.getStyledText() : *fittedText;
            const auto shapedArea = anchorPoint ? anchoredText.getBounds() : area;

            if (fill)
                g.setFillColor (*fill);

            if (stroke)
                g.setStrokeColor (*stroke);

            if (fill || ! stroke)
                paintWithDrawingOpacity (g, fillOpacity, [&] { g.fillFittedText (shaped, shapedArea); });

            if (stroke)
                paintWithDrawingOpacity (g, strokeOpacity, [&] { g.strokeFittedText (shaped, shapedArea); });
        }
    };

    /* A shape or a label drawn with a style. */
    struct Operation
    {
        Operation (Style shapeStyle, Path shape)
            : style (std::move (shapeStyle))
            , path (std::move (shape))
        {
            if (style.shouldStroke && style.hasDash())
                dashedPath = path->createDashed (style.strokeDash->asSpan(), style.strokeDashOffset.value_or (0.0f));
        }

        Operation (Style labelStyle, Text labelText)
            : style (std::move (labelStyle))
            , label (std::move (labelText))
        {
        }

        Style style;
        std::optional<Path> path;
        std::optional<Path> dashedPath;
        std::optional<Text> label;

        void draw (Graphics& g, const Font* labelFont) const
        {
            std::optional<Graphics::SavedState> savedState;

            // Label colors are restored, as shapes without a paint use the colors of the Graphics
            if (style.changesGraphicsState() || (label && label->changesColors()))
                savedState.emplace (g.saveState());

            style.apply (g);

            if (path)
            {
                if (style.shouldFill)
                    paintWithDrawingOpacity (g, style.fillOpacity, [&] { g.fillPath (*path); });

                if (style.shouldStroke)
                    paintWithDrawingOpacity (g, style.strokeOpacity, [&] { g.strokePath (dashedPath ? *dashedPath : *path); });
            }
            else if (label)
            {
                label->draw (g, labelFont, style.fillOpacity, style.strokeOpacity);
            }
        }
    };

    /* The vertices of the rows of a series, drawn with the style of its first row: joined by a line, which goes back
       along their bases to close an area when they have one. A drawing assigns `vertexBase` in every row or in none,
       so a series has a base for each of its vertices or no base at all. */
    struct Polyline
    {
        int series = 0;
        float smooth = 0.0f;
        Style style;
        std::vector<Point<float>> vertices, bases;
    };

    DrawBuffers (const Settings& drawSettings, Graphics& graphics, const Font* font, std::vector<Operation>* recordedOperations, float polylinePixelsPerUnit)
        : settings (drawSettings)
        , g (std::addressof (graphics))
        , labelFont (font)
        , recorded (recordedOperations)
        , pixelsPerUnit (polylinePixelsPerUnit)
    {
    }

    /* Collects the shapes into a path instead of drawing them. */
    DrawBuffers (Path& outlineToBuild, bool shouldApplyTransforms)
        : outline (std::addressof (outlineToBuild))
        , appliesTransforms (shouldApplyTransforms)
    {
    }

    /* Collects the area the labels cover instead of drawing them. */
    DrawBuffers (Rectangle<float>& labelBoundsToBuild, const Font* font, bool shouldMeasureGlyphs)
        : labelFont (font)
        , labelBounds (std::addressof (labelBoundsToBuild))
        , measuresGlyphs (shouldMeasureGlyphs)
    {
    }

    void addRow (const Lanes& lanes, int lane);

    /* Evaluates the rows of a state and adds them all. */
    void addRows (const Outputs& outputs, ExpressionState& state, int numRows)
    {
        for (int startRow = 0; startRow < numRows;)
        {
            const auto numEvaluated = state.evaluateBlock (startRow, numRows - startRow);
            if (numEvaluated <= 0)
                break;

            Lanes lanes {};

            forEachDrawingField ([&state] (const char*, const auto& output, auto& values)
            {
                values = state.getLanes (output).data();
            }, outputs, lanes);

            for (int lane = 0; lane < numEvaluated; ++lane)
                addRow (lanes, lane);

            startRow += numEvaluated;
        }

        finish();
    }

    void finish()
    {
        flush();

        for (int i = 0; i < numPolylines; ++i)
            emitPolyline ((*polylines)[(size_t) i]);

        numPolylines = 0;
    }

    /* Collects the vertices in buffers outliving the draw, so that the next one finds them already allocated. */
    void setPolylineStorage (std::vector<Polyline>& storage) noexcept
    {
        polylines = std::addressof (storage);
    }

    /* Whether a polyline was drawn with fewer points than its rows gave it, for the scale it was drawn at. */
    bool hasReducedPolylines() const noexcept
    {
        return reducedPolylines;
    }

private:
    template <class F>
    void addShape (const Style& style, ShapeKind kind, const Rectangle<float>& bounds, bool usesNonZeroWinding, F&& addToPath)
    {
        if (canAppend (style, kind, bounds, usesNonZeroWinding))
        {
            batchKind = kind == batchKind ? kind : ShapeKind::mixed;
            batchBounds = batchBounds.unionWith (bounds);
        }
        else
        {
            flush();

            batchStyle = style;
            batchKind = kind;
            batchBounds = bounds;
            batch.setUsingNonZeroWinding (usesNonZeroWinding);
        }

        addToPath (batch);
    }

    void flush()
    {
        if (batch.isEmpty())
            return;

        // The batch style is only compared again after a new batch reassigns it
        emit (std::move (batchStyle), std::move (batch));

        // A new path, as the renderer or the cache still reference the emitted one
        batch = Path();
    }

    void emit (Style style, Path shape)
    {
        if (outline == nullptr)
            emit (Operation (std::move (style), std::move (shape)));
        else if (appliesTransforms && style.transform)
            outline->appendPath (shape, *style.transform);
        else
            outline->appendPath (shape);
    }

    void emit (Operation operation)
    {
        if (recorded != nullptr)
            recorded->push_back (std::move (operation));
        else if (g != nullptr)
            operation.draw (*g, labelFont);
        else if (labelBounds != nullptr && operation.label)
            addLabelBounds (*operation.label, operation.style);
    }

    void addLabelBounds (const Text& label, const Style& style)
    {
        auto bounds = label.getBounds (labelFont, measuresGlyphs);

        if (bounds.isEmpty())
            return;

        if (style.transform)
            bounds = bounds.transformed (*style.transform);

        *labelBounds = labelBounds->isEmpty() ? bounds : labelBounds->unionWith (bounds);
    }

    void addVertex (const Point<float>& vertex, const Point<float>* base, float smooth, const Style& style, int series)
    {
        auto& polyline = findPolyline (series, style, smooth);

        polyline.vertices.push_back (vertex);

        if (base != nullptr)
            polyline.bases.push_back (*base);
    }

    /* Returns the polyline of a series, started with the style and the smoothing of the row that is its first. */
    Polyline& findPolyline (int series, const Style& style, float smooth)
    {
        // A row mostly goes on with the series of the one before
        for (int i = numPolylines; --i >= 0;)
        {
            if ((*polylines)[(size_t) i].series == series)
                return (*polylines)[(size_t) i];
        }

        if (numPolylines == static_cast<int> (polylines->size()))
            polylines->emplace_back();

        auto& polyline = (*polylines)[(size_t) numPolylines++];
        polyline.series = series;
        polyline.smooth = smooth;
        polyline.style = style;
        polyline.vertices.clear();
        polyline.bases.clear();

        return polyline;
    }

    /* Adds points to a path in order, or backwards: joined by lines, or by the curves of a cardinal spline going
       through them when smoothed, whose tangent at a point follows the points around it. The first point starts a
       subpath, or is joined by a line to where the path is. */
    static void addPoints (Path& path, const std::vector<Point<float>>& points, float smooth, bool startsSubpath, bool backwards)
    {
        const auto numPoints = static_cast<int> (points.size());

        if (numPoints == 0)
            return;

        const auto pointAt = [&points, numPoints, backwards] (int index)
        {
            const auto clamped = jlimit (0, numPoints - 1, index);
            return points[(size_t) (backwards ? numPoints - 1 - clamped : clamped)];
        };

        if (startsSubpath)
            path.moveTo (pointAt (0));
        else
            path.lineTo (pointAt (0));

        if (smooth <= 0.0f || numPoints < 3)
        {
            for (int i = 1; i < numPoints; ++i)
                path.lineTo (pointAt (i));

            return;
        }

        const auto tension = jmin (smooth, 1.0f) / 6.0f;

        for (int i = 0; i + 1 < numPoints; ++i)
        {
            const auto from = pointAt (i);
            const auto to = pointAt (i + 1);
            const auto first = from + (to - pointAt (i - 1)) * tension;
            const auto second = to - (pointAt (i + 2) - from) * tension;

            path.cubicTo (first.getX(), first.getY(), second.getX(), second.getY(), to.getX(), to.getY());
        }
    }

    /* Leaves a line with more points than the pixels it is drawn across the ones each column of pixels shows.
       Returns true when it took some away. */
    bool reduceToPixelColumns (std::vector<Point<float>>& points) const
    {
        const auto numKept = reducePolylineToPixelColumns (points, pixelsPerUnit);

        if (numKept == points.size())
            return false;

        points.resize (numKept);
        return true;
    }

    void emitPolyline (Polyline& polyline)
    {
        // Only straight lines painted with the transform of the Graphics alone, and with no dashes: a curve follows the
        // points around each of its points, the transform of a row gives its units another size than the Graphics
        // does, and dashes are laid along the whole length of the line
        if (pixelsPerUnit > 0.0f && polyline.smooth <= 0.0f && ! polyline.style.transform && ! polyline.style.hasDash())
        {
            const bool reducedVertices = reduceToPixelColumns (polyline.vertices);
            const bool reducedBases = reduceToPixelColumns (polyline.bases);

            reducedPolylines = reducedPolylines || reducedVertices || reducedBases;
        }

        auto style = polyline.style;

        // With a base the fill takes the area between the vertices and their bases, and the stroke the line alone.
        // The area is the outline of the series, whatever paints it
        const bool hasBases = ! polyline.bases.empty();
        const bool drawsArea = hasBases && (style.hasFill() || outline != nullptr);
        const bool drawsLine = ! hasBases || (outline == nullptr && (style.hasStroke() || ! style.hasFill()));

        // A curve takes three points where a line takes one. The area adds its bases to the line, and closes
        const auto pointsPerVertex = polyline.smooth > 0.0f ? 3 : 1;
        const auto numLinePoints = static_cast<int> (polyline.vertices.size()) * pointsPerVertex;
        const auto numAreaPoints = static_cast<int> (polyline.bases.size()) * pointsPerVertex + 1;

        Path line;
        line.reserveSpace (drawsLine ? numLinePoints : numLinePoints + numAreaPoints);
        addPoints (line, polyline.vertices, polyline.smooth, true, false);

        if (drawsArea)
        {
            // The area goes on from the line, whose points are computed once: from a copy of them when the line is
            // drawn as well
            auto area = drawsLine ? line : std::move (line);
            area.reserveSpace (numAreaPoints);
            addPoints (area, polyline.bases, polyline.smooth, false, true);
            area.close();

            auto areaStyle = style;
            areaStyle.shouldFill = true;
            areaStyle.shouldStroke = false;
            emit (std::move (areaStyle), std::move (area));
        }

        if (! drawsLine)
            return;

        style.shouldFill = ! hasBases && style.hasFill();
        style.shouldStroke = hasBases || style.hasStroke() || ! style.hasFill();
        emit (std::move (style), std::move (line));
    }

    bool canAppend (const Style& style, ShapeKind kind, const Rectangle<float>& bounds, bool usesNonZeroWinding) const
    {
        if (! settings.canMerge || ! style.canMerge() || batch.isEmpty() || ! (style == batchStyle) || usesNonZeroWinding != batch.isUsingNonZeroWinding())
            return false;

        // Antialiasing reaches outside the shapes, so they must be at least a pixel apart
        if (style.isFillOnly() && ! bounds.enlarged (1.0f).intersects (batchBounds))
            return true;

        if (! settings.canMergeOverlapping || ! style.canMergeOverlapping())
            return false;

        return style.shouldStroke || (kind == batchKind && kind != ShapeKind::path);
    }

    const Settings settings;
    Graphics* g = nullptr;
    const Font* labelFont = nullptr;
    std::vector<Operation>* recorded = nullptr;
    Path* outline = nullptr;
    bool appliesTransforms = true;
    Rectangle<float>* labelBounds = nullptr;
    bool measuresGlyphs = true;

    Path batch;
    Style batchStyle;
    ShapeKind batchKind = ShapeKind::mixed;
    Rectangle<float> batchBounds;

    // The polylines of the draw are the first ones of the storage, which keeps the buffers of those of the draws before
    std::vector<Polyline> ownPolylines;
    std::vector<Polyline>* polylines = std::addressof (ownPolylines);
    int numPolylines = 0;

    // The pixels a unit along x takes where the polylines are drawn, 0 where they keep all their points
    float pixelsPerUnit = 0.0f;
    bool reducedPolylines = false;
};

void ExpressionDrawing::DrawBuffers::addRow (const Lanes& lanes, int lane)
{
    auto getDrawingOutput = [lane] (const auto* values)
    {
        return values != nullptr ? values + lane : nullptr;
    };

    auto getOptionalOutput = [lane] (const auto* values)
    {
        using Value = std::remove_cv_t<std::remove_reference_t<decltype (*values)>>;
        return values != nullptr ? std::optional<Value> (values[lane]) : std::optional<Value>();
    };

    if (const auto* isVisible = getDrawingOutput (lanes.visible); isVisible != nullptr && ! *isVisible)
        return;

    Style style;
    style.fill = getOptionalOutput (lanes.fill);
    style.stroke = getOptionalOutput (lanes.stroke);
    style.tint = getOptionalOutput (lanes.tint);
    style.fillGradient = getOptionalOutput (lanes.fillGradient);
    style.strokeGradient = getOptionalOutput (lanes.strokeGradient);
    style.fillPattern = getOptionalOutput (lanes.fillPattern);
    style.strokePattern = getOptionalOutput (lanes.strokePattern);
    style.strokeWidth = getOptionalOutput (lanes.strokeWidth);
    style.miterLimit = getOptionalOutput (lanes.strokeMiterLimit);
    style.strokeDash = getOptionalOutput (lanes.strokeDash);
    style.strokeDashOffset = getOptionalOutput (lanes.strokeDashOffset);
    style.opacity = getOptionalOutput (lanes.opacity);
    style.fillOpacity = getOptionalOutput (lanes.fillOpacity);
    style.strokeOpacity = getOptionalOutput (lanes.strokeOpacity);
    style.feather = getOptionalOutput (lanes.feather);
    style.cap = getOptionalOutput (lanes.strokeCap);
    style.join = getOptionalOutput (lanes.strokeJoin);
    style.strokePosition = getOptionalOutput (lanes.strokePosition);
    style.blend = getOptionalOutput (lanes.blend);
    style.transform = getOptionalOutput (lanes.transform);
    style.clip = getOptionalOutput (lanes.clip);
    style.clipPath = getOptionalOutput (lanes.clipPath);

    // A pattern without a source paints nothing, like no pattern at all
    if (style.fillPattern && style.fillPattern->source == nullptr)
        style.fillPattern.reset();

    if (style.strokePattern && style.strokePattern->source == nullptr)
        style.strokePattern.reset();

    style.shouldFill = style.hasFill() || ! style.hasStroke();
    style.shouldStroke = style.hasStroke();

    if (const auto* r = getDrawingOutput (lanes.rect))
    {
        const auto corner = getOptionalOutput (lanes.cornerRadius).value_or (0.0f);

        const std::array<float, 4> radii {
            getOptionalOutput (lanes.topLeftRadius).value_or (corner),
            getOptionalOutput (lanes.topRightRadius).value_or (corner),
            getOptionalOutput (lanes.bottomLeftRadius).value_or (corner),
            getOptionalOutput (lanes.bottomRightRadius).value_or (corner)
        };

        const bool isRounded = std::any_of (radii.begin(), radii.end(), [] (float value) { return value > 0.0f; });
        const auto cornerSize = getOptionalOutput (lanes.cornerSize).value_or (Size<float>());
        const bool isElliptical = cornerSize.getWidth() > 0.0f && cornerSize.getHeight() > 0.0f;

        addShape (style, ShapeKind::area, *r, true, [&] (Path& p)
        {
            if (isElliptical)
                p.addRoundedRectangle (*r, cornerSize);
            else if (isRounded)
                p.addRoundedRectangle (*r, radii[0], radii[1], radii[2], radii[3]);
            else
                p.addRectangle (*r);
        });
    }

    auto addEllipse = [&] (const Rectangle<float>& area)
    {
        addShape (style, ShapeKind::ellipse, area, true, [&] (Path& p)
        {
            p.addEllipse (area);
        });
    };

    if (const auto* e = getDrawingOutput (lanes.ellipse))
        addEllipse (*e);

    if (const auto* p = getDrawingOutput (lanes.point))
    {
        const auto pointRadius = getOptionalOutput (lanes.radius).value_or (2.0f);

        addEllipse ({ p->getX() - pointRadius, p->getY() - pointRadius, pointRadius * 2.0f, pointRadius * 2.0f });
    }

    if (const auto* l = getDrawingOutput (lanes.line))
    {
        auto lineStyle = style;
        lineStyle.shouldFill = false;
        lineStyle.shouldStroke = true;

        if (! style.hasStroke())
        {
            lineStyle.stroke = style.fill;
            lineStyle.strokeGradient = style.fillGradient;
            lineStyle.strokePattern = style.fillPattern;
            lineStyle.strokeOpacity = style.fillOpacity;
        }

        lineStyle.fill.reset();
        lineStyle.fillGradient.reset();
        lineStyle.fillPattern.reset();

        // Lines are only stroked, so their bounds are never used for merging
        addShape (lineStyle, ShapeKind::line, {}, true, [&] (Path& p)
        {
            p.moveTo (l->getStart());
            p.lineTo (l->getEnd());
        });
    }

    if (const auto* shape = getDrawingOutput (lanes.path); shape != nullptr && ! shape->isEmpty())
    {
        addShape (style, ShapeKind::path, shape->getBounds(), shape->isUsingNonZeroWinding(), [&] (Path& p)
        {
            p.appendPath (*shape);
        });
    }

    if (const auto* v = getDrawingOutput (lanes.vertex))
    {
        addVertex (*v,
                   getDrawingOutput (lanes.vertexBase),
                   getOptionalOutput (lanes.vertexSmooth).value_or (0.0f),
                   style,
                   getOptionalOutput (lanes.series).value_or (0));
    }

    if ((settings.hasLabelFont || labelBounds != nullptr) && lanes.label != nullptr)
    {
        flush();

        Text text;
        text.text = lanes.label[lane];
        text.letterSpacing = getOptionalOutput (lanes.labelLetterSpacing).value_or (0.0f);
        text.fill = getOptionalOutput (lanes.labelColor);
        text.stroke = getOptionalOutput (lanes.labelStroke);
        text.font = getOptionalOutput (lanes.labelFont);

        if (const auto* justification = getDrawingOutput (lanes.labelJustification))
            text.justification = *justification;

        if (const auto* anchorPoint = getDrawingOutput (lanes.labelAnchor))
        {
            text.anchorPoint = *anchorPoint;
            text.anchor = getOptionalOutput (lanes.labelTextAnchor).value_or (AnchoredText::Anchor::start);
        }
        else
        {
            text.area = lanes.labelRect[lane];
        }

        emit (Operation (std::move (style), std::move (text)));
    }
}

//==============================================================================
struct ExpressionDrawing::Cache::Data
{
    std::shared_ptr<const Outputs> outputs;
    std::optional<DrawBuffers::Settings> settings;
    int numRows = 0;
    Fields<ExpressionDrawingColumn> snapshot;
    std::vector<DrawBuffers::Operation> operations;
    std::vector<DrawBuffers::Polyline> polylines;

    // The pixels a unit took where the polylines of the operations were reduced to them, 0 when they have all their points
    float reducedPixelsPerUnit = 0.0f;
    bool wasRebuilt = false;
    bool outputsChanged = false;
    bool isRecorded = false;
};

ExpressionDrawing::Cache::Cache()
    : data (std::make_unique<Data>())
{
}

ExpressionDrawing::Cache::~Cache() = default;

bool ExpressionDrawing::Cache::wasRebuilt() const noexcept
{
    return data->wasRebuilt;
}

void ExpressionDrawing::Cache::reset()
{
    data = std::make_unique<Data>();
}

//==============================================================================
ExpressionDrawing::ExpressionDrawing()
    : outputs (std::make_shared<const Outputs>())
{
}

void ExpressionDrawing::declareOutputs (ExpressionContext& context)
{
    addGraphicsExpressionTypes (context);

    forEachDrawingField ([&context] (const char* name, const auto& output)
    {
        declareDrawingOutput (context, name, output);
    }, Outputs {});
}

ResultValue<ExpressionDrawing> ExpressionDrawing::create (StringRef text, const ExpressionContext& context)
{
    ExpressionContext drawingContext (context);
    declareOutputs (drawingContext);

    auto compiled = CompiledExpression::compile (text, drawingContext);
    if (compiled.failed())
        return makeResultValueFail (compiled.getErrorMessage());

    return create (compiled.getReference());
}

ResultValue<ExpressionDrawing> ExpressionDrawing::create (const CompiledExpression& program)
{
    auto found = std::make_shared<Outputs>();

    forEachDrawingField ([&program] (const char* name, auto& output)
    {
        findDrawingOutput (program, name, output);
    }, *found);

    const int numLabelPlacements = (found->labelRect.isValid() ? 1 : 0) + (found->labelAnchor.isValid() ? 1 : 0);

    if (numLabelPlacements != (found->label.isValid() ? 1 : 0))
        return makeResultValueFail ("The output 'label' must be assigned together with either 'labelRect' or 'labelAnchor'");

    const bool hasShape = found->rect.isValid()
                       || found->ellipse.isValid()
                       || found->point.isValid()
                       || found->line.isValid()
                       || found->path.isValid()
                       || found->vertex.isValid()
                       || found->label.isValid();

    if (! hasShape)
        return makeResultValueFail ("The drawing assigns no shape: assign one of 'rect', 'ellipse', 'point', 'line', 'path', 'vertex' or 'label'");

    ExpressionDrawing drawing;
    drawing.program = program;
    drawing.outputs = std::move (found);
    return makeResultValueOk (std::move (drawing));
}

//==============================================================================
void ExpressionDrawing::draw (Graphics& g, ExpressionState& state, int numRows, const Font* labelFont, Cache* cache) const
{
    const auto savedState = g.saveState();
    const DrawBuffers::Settings settings (g, labelFont != nullptr || outputs->labelFont.isValid());

    // Only a series has points to lose to the pixels it is drawn across
    const auto pixelsPerUnit = outputs->vertex.isValid() ? getDrawingPixelsPerUnit (g) : 0.0f;

    if (cache == nullptr)
    {
        DrawBuffers (settings, g, labelFont, nullptr, pixelsPerUnit).addRows (*outputs, state, numRows);
        return;
    }

    auto& data = *cache->data;

    // Polylines reduced to the pixels of a smaller scale lack the points a larger one shows
    bool hasChanged = data.outputs != outputs
                   || data.settings != settings
                   || (data.reducedPixelsPerUnit > 0.0f && pixelsPerUnit > data.reducedPixelsPerUnit);

    data.outputs = outputs;
    data.settings = settings;

    forEachDrawingField ([&hasChanged, numRows] (const char*, const auto& output, auto& column)
    {
        if (output.isValid() && column.reserve (numRows))
            hasChanged = true;
    }, *outputs, data.snapshot);

    int startRow = 0;

    while (startRow < numRows)
    {
        const auto numEvaluated = state.evaluateBlock (startRow, numRows - startRow);
        if (numEvaluated <= 0)
            break;

        forEachDrawingField ([&] (const char*, const auto& output, auto& column)
        {
            if (column.update (state.getLanes (output), startRow))
                hasChanged = true;
        }, *outputs, data.snapshot);

        startRow += numEvaluated;
    }

    const bool outputsChanged = hasChanged || data.numRows != startRow;

    // Outputs changing on consecutive draws are animated: recording them would only be thrown away
    const bool shouldRecord = ! (outputsChanged && data.outputsChanged);

    data.numRows = startRow;
    data.outputsChanged = outputsChanged;
    data.wasRebuilt = outputsChanged || ! data.isRecorded;

    if (data.wasRebuilt)
    {
        data.operations.clear();
        data.isRecorded = shouldRecord;

        DrawBuffers::Lanes lanes {};

        forEachDrawingField ([] (const char*, const auto& output, const auto& column, auto& values)
        {
            values = output.isValid() ? column.values.get() : nullptr;
        }, *outputs, data.snapshot, lanes);

        DrawBuffers buffers (settings, g, labelFont, shouldRecord ? &data.operations : nullptr, pixelsPerUnit);
        buffers.setPolylineStorage (data.polylines);

        for (int row = 0; row < data.numRows; ++row)
            buffers.addRow (lanes, row);

        buffers.finish();

        data.reducedPixelsPerUnit = shouldRecord && buffers.hasReducedPolylines() ? pixelsPerUnit : 0.0f;

        if (! shouldRecord)
            return;
    }

    for (const auto& operation : data.operations)
        operation.draw (g, labelFont);
}

Path ExpressionDrawing::getOutline (ExpressionState& state, int numRows, bool applyTransforms) const
{
    Path outline;
    DrawBuffers (outline, applyTransforms).addRows (*outputs, state, numRows);
    return outline;
}

Rectangle<float> ExpressionDrawing::getLabelBounds (ExpressionState& state, int numRows, const Font* labelFont, bool ofGlyphs) const
{
    Rectangle<float> bounds;
    DrawBuffers (bounds, labelFont, ofGlyphs).addRows (*outputs, state, numRows);
    return bounds;
}

} // namespace yup
