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

namespace PathEditor
{

//==============================================================================

inline yup::Color getPaletteColor (yup::ThemePalette::Role role)
{
    return yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (role);
}

inline void drawCheckerboard (yup::Graphics& g, yup::Rectangle<float> area, float cellSize)
{
    yup::Path darkCells;
    for (float y = area.getY(); y < area.getBottom(); y += cellSize)
    {
        for (float x = area.getX(); x < area.getRight(); x += cellSize)
        {
            if ((static_cast<int> ((x - area.getX()) / cellSize) + static_cast<int> ((y - area.getY()) / cellSize)) % 2 == 0)
                continue;

            darkCells.addRectangle (x, y, yup::jmin (cellSize, area.getRight() - x), yup::jmin (cellSize, area.getBottom() - y));
        }
    }

    g.setFillColor (yup::Color (0xffd0d0d0));
    g.fillRect (area);
    g.setFillColor (yup::Color (0xff9a9a9a));
    g.fillPath (darkCells);
}

//==============================================================================

/** Renders a PathDocument and edits it with a select tool and a pen tool. */
class PathCanvas : public yup::Component
{
public:
    enum class Tool
    {
        select,
        pen
    };

    PathCanvas (PathDocument& documentToEdit, Selection& selectionToUse)
        : yup::Component ("PathCanvas")
        , document (documentToEdit)
        , selection (selectionToUse)
    {
    }

    //==============================================================================
    void setTool (Tool newTool)
    {
        if (tool == newTool)
            return;

        finishPen();
        tool = newTool;
        setMouseCursor (tool == Tool::pen ? yup::MouseCursor::Crosshair : yup::MouseCursor::Default);
        repaint();
    }

    Tool getTool() const noexcept { return tool; }

    /** True while the pen is adding vertices to a contour (undo can remove it from the document). */
    bool isDrawingPath() const { return penContour.isAChildOf (document.getRoot()); }

    void finishPen()
    {
        if (! penContour.isValid())
            return;

        penContour = {};
        repaint();
    }

    /** Drops the current drag without a mouseUp (the window lost focus); the owner ends the gesture. */
    void cancelDrag()
    {
        drag = {};
    }

    std::function<void()> onSelectionChanged;

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        g.setFillColor (getPaletteColor (yup::ThemePalette::Role::surface));
        g.fillAll();

        const auto artboard = document.getArtboardBounds();

        {
            const auto state = g.saveState();
            g.addTransform (getViewTransform());

            drawCheckerboard (g, artboard, 16.0f);
            g.setFillColor (PathEditor::getColor (document.getRoot(), Ids::background));
            g.fillRect (artboard);

            g.setClipPath (artboard);
            for (const auto& layer : document.getRoot())
                drawLayer (g, layer);
        }

        // The overlay is drawn in view coordinates, so handles keep their size whatever the view scale
        g.setStrokeColor (getPaletteColor (yup::ThemePalette::Role::outline));
        g.setStrokeWidth (1.0f);
        g.strokeRect (yup::Rectangle<float> (toView (artboard.getTopLeft()), artboard.getWidth() * getViewScale(), artboard.getHeight() * getViewScale()).enlarged (0.5f));

        drawSelectionOverlay (g);
    }

    //==============================================================================
    void mouseDown (const yup::MouseEvent& event) override
    {
        // A second button pressed mid-drag sends another mouseDown
        if (drag.kind != DragKind::none || ! event.isLeftButtonDown())
            return;

        const auto position = toDocument (event.getPosition());

        if (tool == Tool::pen)
        {
            penDown (position);
            return;
        }

        const auto hit = findHit (position);
        switch (hit.kind)
        {
            case HitKind::gradientStart:
            case HitKind::gradientEnd:
                startDrag (hit.kind == HitKind::gradientStart ? DragKind::gradientStart : DragKind::gradientEnd, hit.node, position, "Move gradient point");
                break;

            case HitKind::inHandle:
            case HitKind::outHandle:
                startDrag (hit.kind == HitKind::inHandle ? DragKind::inHandle : DragKind::outHandle, hit.node, position, "Move handle");
                break;

            case HitKind::anchor:
                select (selection.layer, hit.node);
                startDrag (DragKind::anchor, hit.node, position, "Move vertex");
                break;

            case HitKind::segment:
            case HitKind::layer:
                select (hit.layer, hit.layer == selection.layer ? selection.vertex : yup::DataTree());
                startDrag (DragKind::layer, hit.layer, position, "Move layer");
                break;

            case HitKind::none:
                select ({}, {});
                break;
        }
    }

    void mouseDrag (const yup::MouseEvent& event) override
    {
        if (drag.kind == DragKind::none)
            return;

        const auto position = toDocument (event.getPosition());
        const auto delta = position - drag.lastPosition;
        if (delta.isOrigin())
            return;

        switch (drag.kind)
        {
            case DragKind::layer:
                document.apply ([&]
                {
                    document.translateLayer (drag.node, delta);
                });
                break;

            case DragKind::anchor:
                document.apply ([&]
                {
                    document.setPoint (drag.node, Ids::x, Ids::y, getAnchor (drag.node) + delta);
                });
                break;

            case DragKind::inHandle:
                moveHandle (drag.node, false, getInOffset (drag.node) + delta, event.getModifiers().isAltDown());
                break;

            case DragKind::outHandle:
                moveHandle (drag.node, true, getOutOffset (drag.node) + delta, event.getModifiers().isAltDown());
                break;

            case DragKind::penHandle:
            {
                const auto offset = position - getAnchor (drag.node);
                if (offset.magnitude() < 2.0f)
                    return;

                document.apply ([&]
                {
                    document.setProperty (drag.node, Ids::smooth, true);
                    document.setPoint (drag.node, Ids::outX, Ids::outY, offset);
                    document.setPoint (drag.node, Ids::inX, Ids::inY, -offset);
                });
                break;
            }

            case DragKind::gradientStart:
            case DragKind::gradientEnd:
                document.apply ([&]
                {
                    if (drag.kind == DragKind::gradientStart)
                        document.setPoint (drag.node, Ids::x1, Ids::y1, getPoint (drag.node, Ids::x1, Ids::y1) + delta);
                    else
                        document.setPoint (drag.node, Ids::x2, Ids::y2, getPoint (drag.node, Ids::x2, Ids::y2) + delta);
                });
                break;

            case DragKind::none:
                break;
        }

        drag.lastPosition = position;
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        endDrag();
    }

    void mouseMove (const yup::MouseEvent& event) override
    {
        hoverPosition = toDocument (event.getPosition());

        if (isDrawingPath())
            repaint();
    }

    void mouseDoubleClick (const yup::MouseEvent& event) override
    {
        if (tool != Tool::select)
            return;

        endDrag();

        const auto hit = findHit (toDocument (event.getPosition()));
        if (hit.kind == HitKind::anchor)
        {
            document.edit ("Toggle smooth", [&]
            {
                document.toggleSmooth (hit.node);
            });
        }
        else if (hit.kind == HitKind::segment)
        {
            yup::DataTree newVertex;
            document.edit ("Insert vertex", [&]
            {
                newVertex = document.splitSegment (hit.node, hit.segmentIndex, hit.t);
            });

            select (selection.layer, newVertex);
        }
    }

private:
    enum class HitKind
    {
        none,
        gradientStart,
        gradientEnd,
        inHandle,
        outHandle,
        anchor,
        segment,
        layer
    };

    enum class DragKind
    {
        none,
        layer,
        anchor,
        inHandle,
        outHandle,
        penHandle,
        gradientStart,
        gradientEnd
    };

    struct Hit
    {
        HitKind kind = HitKind::none;
        yup::DataTree node; // paint, vertex or contour, depending on kind
        yup::DataTree layer;
        int segmentIndex = -1;
        float t = 0.0f;
    };

    struct DragState
    {
        DragKind kind = DragKind::none;
        yup::DataTree node;
        yup::Point<float> lastPosition;
    };

    // In view pixels, divided by the view scale when hit testing in document coordinates
    static constexpr float handleRadius = 6.0f;
    static constexpr float segmentTolerance = 5.0f;
    static constexpr float viewMargin = 16.0f;

    //==============================================================================
    /** Scales the artboard down to fit the canvas, never above 100%. */
    float getViewScale() const
    {
        const auto artboard = document.getArtboardBounds();
        const float fitScale = yup::jmin ((getWidth() - viewMargin * 2.0f) / artboard.getWidth(), (getHeight() - viewMargin * 2.0f) / artboard.getHeight());
        return yup::jlimit (0.05f, 1.0f, fitScale);
    }

    yup::Point<float> getViewOffset() const
    {
        const auto artboard = document.getArtboardBounds();
        const float scale = getViewScale();
        return { std::floor ((getWidth() - artboard.getWidth() * scale) * 0.5f), std::floor ((getHeight() - artboard.getHeight() * scale) * 0.5f) };
    }

    yup::AffineTransform getViewTransform() const
    {
        return yup::AffineTransform::scaling (getViewScale()).translated (getViewOffset());
    }

    yup::Point<float> toDocument (yup::Point<float> localPosition) const
    {
        return (localPosition - getViewOffset()) / getViewScale();
    }

    yup::Point<float> toView (yup::Point<float> documentPosition) const
    {
        return documentPosition * getViewScale() + getViewOffset();
    }

    void select (const yup::DataTree& layer, const yup::DataTree& vertex)
    {
        if (selection.layer == layer && selection.vertex == vertex)
            return;

        selection.layer = layer;
        selection.vertex = vertex;

        repaint();

        if (onSelectionChanged)
            onSelectionChanged();
    }

    void startDrag (DragKind kind, const yup::DataTree& node, yup::Point<float> position, yup::StringRef gestureName)
    {
        document.beginGesture (gestureName);
        drag = { kind, node, position };
    }

    void endDrag()
    {
        if (drag.kind == DragKind::none)
            return;

        drag = {};
        document.endGesture();
    }

    void moveHandle (yup::DataTree vertex, bool isOutHandle, yup::Point<float> offset, bool breakSmooth)
    {
        document.apply ([&]
        {
            if (breakSmooth)
                document.setProperty (vertex, Ids::smooth, false);

            if (isOutHandle)
                document.setPoint (vertex, Ids::outX, Ids::outY, offset);
            else
                document.setPoint (vertex, Ids::inX, Ids::inY, offset);

            const float length = offset.magnitude();
            if (! getBool (vertex, Ids::smooth) || length < 0.01f)
                return;

            const auto opposite = isOutHandle ? getInOffset (vertex) : getOutOffset (vertex);
            const float oppositeLength = opposite.magnitude() > 0.01f ? opposite.magnitude() : length;
            const auto mirrored = offset * (-oppositeLength / length);

            if (isOutHandle)
                document.setPoint (vertex, Ids::inX, Ids::inY, mirrored);
            else
                document.setPoint (vertex, Ids::outX, Ids::outY, mirrored);
        });
    }

    //==============================================================================
    void penDown (yup::Point<float> position)
    {
        if (! isDrawingPath())
            penContour = {};

        if (const int numVertices = penContour.getNumChildren(); numVertices > 0)
        {
            const auto first = getAnchor (penContour.getChild (0));
            const auto last = getAnchor (penContour.getChild (numVertices - 1));

            if (numVertices > 2 && position.distanceTo (first) <= handleRadius / getViewScale())
            {
                document.edit ("Close path", [&]
                {
                    document.setProperty (penContour, Ids::closed, true);
                });

                finishPen();
                return;
            }

            if (position.distanceTo (last) <= 3.0f / getViewScale())
            {
                finishPen();
                return;
            }
        }

        auto layer = penContour.getParent();
        auto vertex = document.createVertex (position);

        document.beginGesture ("Add vertex");
        document.apply ([&]
        {
            if (! penContour.isValid())
            {
                auto& schema = document.getSchema();
                auto contour = schema.createNode (Ids::contour);
                setDetachedProperties (contour, { { Ids::closed, false } });

                layer = document.createLayer ("Path " + yup::String (document.getRoot().getNumChildren() + 1), yup::Path(), getPaletteColor (yup::ThemePalette::Role::accent));
                setDetachedProperties (getFill (layer), { { Ids::kind, "none" } });
                setDetachedProperties (getStroke (layer), { { Ids::kind, "solid" }, { Ids::width, 3.0f } });

                auto transaction = layer.beginTransaction();
                transaction.addChild (contour);
                transaction.commit();

                document.addChild (document.getRoot(), layer);
                penContour = contour;
            }

            document.addChild (penContour, vertex);
        });

        drag = { DragKind::penHandle, vertex, position };
        select (layer, vertex);
    }

    //==============================================================================
    std::vector<yup::DataTree> getHandleVertices() const
    {
        std::vector<yup::DataTree> vertices;
        if (! selection.vertex.isValid())
            return vertices;

        const auto contour = selection.vertex.getParent();
        const int numVertices = contour.getNumChildren();
        const int index = contour.indexOf (selection.vertex);
        const bool closed = getBool (contour, Ids::closed);

        vertices.push_back (selection.vertex);

        if (index > 0 || (closed && numVertices > 2))
            vertices.push_back (contour.getChild ((index + numVertices - 1) % numVertices));

        if (index < numVertices - 1 || (closed && numVertices > 2))
            vertices.push_back (contour.getChild ((index + 1) % numVertices));

        return vertices;
    }

    static float distanceToSegment (yup::Point<float> p, yup::Point<float> a, yup::Point<float> b)
    {
        const auto ab = b - a;
        const float lengthSquared = ab.dotProduct (ab);
        if (lengthSquared <= 0.0f)
            return p.distanceTo (a);

        const float t = yup::jlimit (0.0f, 1.0f, (p - a).dotProduct (ab) / lengthSquared);
        return p.distanceTo (a + ab * t);
    }

    static bool layerContains (const yup::DataTree& layer, yup::Point<float> p, float minimumTolerance)
    {
        const auto polylines = flattenLayer (layer);

        if (isPaintVisible (getFill (layer)))
        {
            int winding = 0;
            for (const auto& polyline : polylines)
            {
                for (std::size_t i = 0; i < polyline.size(); ++i)
                {
                    const auto a = polyline[i];
                    const auto b = polyline[(i + 1) % polyline.size()];
                    const float side = (b.getX() - a.getX()) * (p.getY() - a.getY()) - (p.getX() - a.getX()) * (b.getY() - a.getY());

                    if (a.getY() <= p.getY() && b.getY() > p.getY() && side > 0.0f)
                        ++winding;
                    else if (a.getY() > p.getY() && b.getY() <= p.getY() && side < 0.0f)
                        --winding;
                }
            }

            const bool evenOdd = getString (layer, Ids::fillRule) == "evenodd";
            if (evenOdd ? (winding % 2 != 0) : (winding != 0))
                return true;
        }

        const auto stroke = getStroke (layer);
        const float tolerance = yup::jmax (isPaintVisible (stroke) ? getFloat (stroke, Ids::width) * 0.5f : 0.0f, minimumTolerance);

        for (const auto& polyline : polylines)
        {
            for (std::size_t i = 1; i < polyline.size(); ++i)
            {
                if (distanceToSegment (p, polyline[i - 1], polyline[i]) <= tolerance)
                    return true;
            }
        }

        return false;
    }

    Hit findHit (yup::Point<float> p) const
    {
        const float radius = handleRadius / getViewScale();

        if (const auto layer = selection.layer; layer.isValid())
        {
            for (const auto& paint : { getFill (layer), getStroke (layer) })
            {
                if (! isGradientKind (getString (paint, Ids::kind)))
                    continue;

                if (p.distanceTo (getPoint (paint, Ids::x1, Ids::y1)) <= radius)
                    return { HitKind::gradientStart, paint, layer };

                if (p.distanceTo (getPoint (paint, Ids::x2, Ids::y2)) <= radius)
                    return { HitKind::gradientEnd, paint, layer };
            }

            for (const auto& vertex : getHandleVertices())
            {
                const auto anchor = getAnchor (vertex);

                if (! getOutOffset (vertex).isOrigin() && p.distanceTo (anchor + getOutOffset (vertex)) <= radius)
                    return { HitKind::outHandle, vertex, layer };

                if (! getInOffset (vertex).isOrigin() && p.distanceTo (anchor + getInOffset (vertex)) <= radius)
                    return { HitKind::inHandle, vertex, layer };
            }

            Hit nearestSegment;
            float nearestDistance = segmentTolerance / getViewScale();

            for (const auto& contour : layer)
            {
                if (contour.getType() != Ids::contour)
                    continue;

                for (const auto& vertex : contour)
                {
                    if (p.distanceTo (getAnchor (vertex)) <= radius)
                        return { HitKind::anchor, vertex, layer };
                }

                for (int i = 0; i < getNumSegments (contour); ++i)
                {
                    const auto segment = getSegment (contour, i);

                    for (int step = 0; step <= 32; ++step)
                    {
                        const float t = static_cast<float> (step) / 32.0f;
                        const float distance = p.distanceTo (segment.pointAt (t));

                        if (distance <= nearestDistance)
                        {
                            nearestDistance = distance;
                            nearestSegment = { HitKind::segment, contour, layer, i, t };
                        }
                    }
                }
            }

            if (nearestSegment.kind != HitKind::none)
                return nearestSegment;
        }

        const auto& root = document.getRoot();
        for (int i = root.getNumChildren(); --i >= 0;)
        {
            const auto layer = root.getChild (i);
            if (getBool (layer, Ids::visible) && layerContains (layer, p, 4.0f / getViewScale()))
                return { HitKind::layer, layer, layer };
        }

        return {};
    }

    //==============================================================================
    static void setPaint (yup::Graphics& g, const yup::DataTree& paint, bool asStroke)
    {
        if (getString (paint, Ids::kind) == "solid")
        {
            const auto paintColor = PathEditor::getColor (paint, Ids::color);
            asStroke ? g.setStrokeColor (paintColor) : g.setFillColor (paintColor);
        }
        else
        {
            auto gradient = createGradient (paint);
            asStroke ? g.setStrokeColorGradient (std::move (gradient)) : g.setFillColorGradient (std::move (gradient));
        }
    }

    static void renderLayerContent (yup::Graphics& g, const yup::DataTree& layer, const yup::Path& path, yup::BlendMode blendMode)
    {
        const auto fill = getFill (layer);
        const auto stroke = getStroke (layer);
        const float feather = getFloat (layer, Ids::feather);
        const float strokeWidth = getFloat (stroke, Ids::width);

        g.setBlendMode (blendMode);
        g.setFeather (feather);

        if (isPaintVisible (fill))
        {
            setPaint (g, fill, false);
            g.fillPath (path);
        }

        if (! isPaintVisible (stroke) || strokeWidth <= 0.0f)
            return;

        if (feather > 0.0f)
        {
            // Feather only applies to fills: fill the outline of the stroke instead.
            setPaint (g, stroke, false);
            g.fillPath (path.createStrokePolygon (strokeWidth, getStrokeJoin (stroke), getStrokeCap (stroke)));
            return;
        }

        setPaint (g, stroke, true);
        g.setStrokeWidth (strokeWidth);
        g.setStrokeJoin (getStrokeJoin (stroke));
        g.setStrokeCap (getStrokeCap (stroke));
        g.strokePath (path);
    }

    static void drawLayer (yup::Graphics& g, const yup::DataTree& layer)
    {
        const float opacity = getFloat (layer, Ids::opacity);
        if (! getBool (layer, Ids::visible) || opacity <= 0.0f)
            return;

        const auto path = buildPath (layer);
        if (path.isEmpty())
            return;

        const auto stroke = getStroke (layer);
        const bool overlapping = isPaintVisible (getFill (layer)) && isPaintVisible (stroke) && getFloat (stroke, Ids::width) > 0.0f;

        if (opacity < 1.0f && overlapping)
        {
            // Fill and stroke overlap: composite them as a group, like SVG opacity on the element
            // Miter tips reach up to twice the stroke width from the vertex (miter limit 4)
            const float padding = getFloat (stroke, Ids::width) * 2.0f + getFloat (layer, Ids::feather) * 2.0f + 2.0f;
            const auto area = path.getBounds().enlarged (padding);

            auto transparency = g.beginTransparencyLayer (area, opacity);
            if (transparency.isValid())
            {
                auto& layerGraphics = transparency.getGraphics();
                layerGraphics.setTransform (yup::AffineTransform::translation (-area.getX(), -area.getY()));
                renderLayerContent (layerGraphics, layer, path, yup::BlendMode::SrcOver);

                const auto state = g.saveState();
                g.setBlendMode (getBlendMode (layer));
                transparency.commit();
                return;
            }
        }

        const auto state = g.saveState();
        g.setOpacity (opacity);
        renderLayerContent (g, layer, path, getBlendMode (layer));
    }

    //==============================================================================
    void drawSelectionOverlay (yup::Graphics& g)
    {
        const auto accent = getPaletteColor (yup::ThemePalette::Role::accent);
        const auto& layer = selection.layer;

        if (isDrawingPath() && penContour.getNumChildren() > 0)
        {
            g.setStrokeColor (accent.withAlpha (0.6f));
            g.setStrokeWidth (1.0f);
            g.strokeLine (toView (getAnchor (penContour.getChild (penContour.getNumChildren() - 1))), toView (hoverPosition));
        }

        if (! layer.isValid())
            return;

        const auto path = buildPath (layer).transformed (getViewTransform());

        g.setStrokeColor (accent.withAlpha (0.35f));
        g.setStrokeWidth (1.0f);
        g.strokeRect (path.getBounds().enlarged (4.0f));

        g.setStrokeColor (accent);
        g.setStrokeWidth (1.5f);
        g.strokePath (path);

        for (const auto& vertex : getHandleVertices())
        {
            const auto anchor = getAnchor (vertex);

            for (const auto offset : { getInOffset (vertex), getOutOffset (vertex) })
            {
                if (offset.isOrigin())
                    continue;

                const auto handle = toView (anchor + offset);

                g.setStrokeColor (accent.withAlpha (0.8f));
                g.setStrokeWidth (1.0f);
                g.strokeLine (toView (anchor), handle);

                g.setFillColor (yup::Colors::white);
                g.fillEllipse (yup::Rectangle<float> (0.0f, 0.0f, 8.0f, 8.0f).withCenter (handle));
                g.setStrokeColor (accent);
                g.strokeEllipse (yup::Rectangle<float> (0.0f, 0.0f, 8.0f, 8.0f).withCenter (handle));
            }
        }

        for (const auto& contour : layer)
        {
            if (contour.getType() != Ids::contour)
                continue;

            for (const auto& vertex : contour)
            {
                const auto box = yup::Rectangle<float> (0.0f, 0.0f, 8.0f, 8.0f).withCenter (toView (getAnchor (vertex)));

                g.setFillColor (vertex == selection.vertex ? accent : yup::Colors::white);
                g.fillRect (box);
                g.setStrokeColor (accent);
                g.setStrokeWidth (1.0f);
                g.strokeRect (box);
            }
        }

        drawGradientHandles (g, getFill (layer), accent);
        drawGradientHandles (g, getStroke (layer), accent.contrasting (0.5f));
    }

    void drawGradientHandles (yup::Graphics& g, const yup::DataTree& paint, yup::Color handleColor) const
    {
        if (! isGradientKind (getString (paint, Ids::kind)))
            return;

        const auto start = toView (getPoint (paint, Ids::x1, Ids::y1));
        const auto end = toView (getPoint (paint, Ids::x2, Ids::y2));

        g.setStrokeColor (yup::Colors::black.withAlpha (0.5f));
        g.setStrokeWidth (3.0f);
        g.strokeLine (start, end);
        g.setStrokeColor (handleColor);
        g.setStrokeWidth (1.0f);
        g.strokeLine (start, end);

        for (const auto& [point, diameter] : { std::pair { start, 12.0f }, std::pair { end, 9.0f } })
        {
            const auto box = yup::Rectangle<float> (0.0f, 0.0f, diameter, diameter).withCenter (point);
            g.setFillColor (handleColor);
            g.fillEllipse (box);
            g.setStrokeColor (yup::Colors::black);
            g.strokeEllipse (box);
        }
    }

    PathDocument& document;
    Selection& selection;
    Tool tool = Tool::select;
    DragState drag;
    yup::DataTree penContour;
    yup::Point<float> hoverPosition;
};

} // namespace PathEditor
