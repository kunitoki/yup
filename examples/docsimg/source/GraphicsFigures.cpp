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

#include "Figures.h"

#include <cmath>

namespace
{

const yup::Size<float> small { 320.0f, 140.0f };
const yup::Size<float> medium { 480.0f, 150.0f };
const yup::Size<float> wide { 560.0f, 140.0f };

//==============================================================================
/** Fills the regions `removeFrom*` slices an area into. */
void slices (yup::Graphics& g, yup::Rectangle<float> area, float header, float footer, float sidebar)
{
    const auto slice = [&g] (yup::Rectangle<float> bounds, const char* name, Role fill = Role::surfaceRaised, Role text = Role::text)
    {
        g.setFillColor (role (fill));
        g.fillRoundedRect (bounds.reduced (1.5f), 3.0f);

        g.setFillColor (role (text));
        g.fillFittedText (name, font (12.0f), bounds, yup::Justification::center);
    };

    slice (area.removeFromTop (header), "header");
    slice (area.removeFromBottom (footer), "footer");
    slice (area.removeFromLeft (sidebar), "sidebar");
    slice (area, "area", Role::accent, Role::onAccent);
}

yup::Rectangle<float> dot (yup::Point<float> center, float radius)
{
    return { center.getX() - radius, center.getY() - radius, radius * 2.0f, radius * 2.0f };
}

//==============================================================================
/** An image drawn on a grid whose vertices were moved. The mesh outlives the paint call,
    as the GPU reads its positions when the frame is rendered. */
struct WarpedImage : yup::Component
{
    WarpedImage()
    {
        setOpaque (false);
    }

    void resized() override
    {
        const auto side = getHeight() - 16.0f;
        mesh = yup::ImageMesh::createGrid (getLocalBounds().withSizeKeepingCenter (side, side), 8, 8);

        const auto vertices = mesh.getVertices();
        const std::vector<yup::Point<float>> original (vertices.begin(), vertices.end());

        for (int index = 0; index < static_cast<int> (original.size()); ++index)
        {
            const auto& vertex = original[static_cast<std::size_t> (index)];
            mesh.setVertex (index, vertex.translated (std::sin (vertex.getY() * 0.07f) * 10.0f, std::cos (vertex.getX() * 0.07f) * 6.0f));
        }
    }

    void paint (yup::Graphics& g) override
    {
        g.drawImageMesh (image, mesh);

        g.setFillColor (role (Role::text));
        for (const auto& vertex : mesh.getVertices())
            g.fillEllipse (dot (vertex, 1.5f));
    }

    yup::Image image = docsimgImage ("data/coin.png");
    yup::ImageMesh mesh;
};

//==============================================================================
// docs/graphics/drawing.md

void addDrawingFigures (Figures& f)
{
    using namespace yup;

    f.paint ("graphics/drawing_rectangle", small, [] (Graphics& g, Rectangle<float> area)
    {
        g.setFillColor (role (Role::surfaceRaised));
        g.fillRect (area);

        g.setStrokeColor (role (Role::text));
        g.setStrokeWidth (1.0f);
        g.strokeRect (area.reduced (0.5f));
    });

    f.paint ("graphics/drawing_rounded-button", medium, [] (Graphics& g, Rectangle<float> area)
    {
        labelled (g, area, { "normal", "hover", "per-corner radii" }, 3, [&g] (int index, Rectangle<float> cell)
        {
            const auto rect = cell.reduced (8.5f, 12.5f);
            const auto top = index == 2 ? 14.0f : 6.0f;
            const auto bottom = index == 2 ? 0.0f : 6.0f;

            g.setFillColor (index == 1 ? Colors::cornflowerblue : Colors::slategray);
            g.fillRoundedRect (rect, top, top, bottom, bottom);

            g.setStrokeColor (Colors::white.withMultipliedBrightness (0.8f));
            g.setStrokeWidth (1.0f);
            g.strokeRoundedRect (rect, top, top, bottom, bottom);
        });
    });

    f.paint ("graphics/drawing_circle", small, [] (Graphics& g, Rectangle<float> area)
    {
        const auto side = jmin (area.getWidth(), area.getHeight());

        g.setFillColor (Colors::tomato);
        g.fillEllipse (area.withSizeKeepingCenter (side, side));
    });

    f.paint ("graphics/drawing_line", small, [] (Graphics& g, Rectangle<float> area)
    {
        g.setStrokeColor (role (Role::text));
        g.setStrokeWidth (2.0f);
        g.strokeLine (area.getTopLeft(), area.getBottomRight());
    });

    f.paint ("graphics/drawing_text", small, [] (Graphics& g, Rectangle<float> area)
    {
        g.setStrokeColor (role (Role::outline));
        g.setStrokeWidth (1.0f);
        g.strokeRect (area.reduced (0.5f));

        g.setFillColor (role (Role::text));
        g.fillFittedText ("Label", font (28.0f), area.reduced (8.0f), Justification::center);
    });

    f.paint ("graphics/drawing_image", small, [] (Graphics& g, Rectangle<float> area)
    {
        const auto coin = docsimgImage ("data/coin.png");

        g.drawImage (coin, area.removeFromLeft (area.getHeight()));
        g.drawImage (coin, area.withTrimmedLeft (24.0f).reduced (0.0f, 20.0f));
    });

    f.paint ("graphics/drawing_gradients", medium, [] (Graphics& g, Rectangle<float> area)
    {
        labelled (g, area, { "Linear", "Radial", "Angular" }, 3, [&g] (int index, Rectangle<float> cell)
        {
            const auto center = cell.getCenter();
            const auto edge = center.translated (cell.getWidth() / 2.0f, 0.0f);

            const ColorGradient gradients[] = {
                ColorGradient (Colors::firebrick, cell.getX(), cell.getY(), Colors::darkorange, cell.getX(), cell.getBottom()),
                ColorGradient (Colors::darkorange, center, Colors::firebrick, edge, ColorGradient::Radial),
                ColorGradient (ColorGradient::Angular, { { Colors::firebrick, center, 0.0f }, { Colors::darkorange, center, 0.5f }, { Colors::firebrick, edge, 1.0f } })
            };

            g.setFillColorGradient (gradients[index]);
            g.fillRoundedRect (cell, 8.0f);
        });
    });

    f.paint ("graphics/drawing_path", small, [] (Graphics& g, Rectangle<float> area)
    {
        Path wave;
        wave.moveTo (0.0f, area.getHeight() / 2.0f);
        for (float x = 0.0f; x <= area.getWidth(); x += 4.0f)
            wave.lineTo (x, area.getHeight() / 2.0f + std::sin (x * 0.03f) * 20.0f);

        g.setStrokeColor (Colors::mediumseagreen);
        g.setStrokeWidth (2.0f);
        g.strokePath (wave);
    });

    // Each circle fading on its own shows the overlap, both in one layer fade as a whole
    f.paint ("graphics/drawing_layer", { 400.0f, 150.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto fillCircles = [] (Graphics& target, Rectangle<float> bounds)
        {
            const auto diameter = jmin (bounds.getHeight(), bounds.getWidth() / 1.8f);
            const auto circle = bounds.withSizeKeepingCenter (diameter * 1.5f, diameter).withWidth (diameter);

            target.setFillColor (Colors::red);
            target.fillEllipse (circle);
            target.fillEllipse (circle.translated (diameter * 0.5f, 0.0f));
        };

        labelled (g, area, { "each shape at 60%", "one layer at 60%" }, 2, [&] (int index, Rectangle<float> cell)
        {
            if (index == 0)
            {
                g.setOpacity (0.6f);
                fillCircles (g, cell);
            }
            else if (auto layer = g.beginTransparencyLayer (cell, 0.6f); layer.isValid())
            {
                fillCircles (layer.getGraphics(), cell.withPosition (0.0f, 0.0f));
                layer.commit();
            }
        });
    });

    f.paint ("graphics/drawing_clip", small, [] (Graphics& g, Rectangle<float> area)
    {
        const auto saved = g.saveState();

        g.setClipPath (Path().addRoundedRectangle (area.reduced (30.0f, 6.0f), 24.0f));
        g.setFillColor (Colors::hotpink);
        g.fillAll();

        g.setFillColor (Colors::white.withAlpha (0.35f));
        g.fillEllipse (area.withSizeKeepingCenter (area.getHeight(), area.getHeight()).translated (area.getWidth() * 0.3f, area.getHeight() * 0.3f));
    });

    // The arrow as built, and rotated a quarter turn around its tail
    f.paint ("graphics/drawing_transform", small, [] (Graphics& g, Rectangle<float> area)
    {
        const auto pivot = area.getCenter().translated (-32.0f, -30.0f);

        Path arrow;
        arrow.moveTo (0.0f, -5.0f).lineTo (40.0f, -5.0f).lineTo (40.0f, -14.0f).lineTo (64.0f, 0.0f);
        arrow.lineTo (40.0f, 14.0f).lineTo (40.0f, 5.0f).lineTo (0.0f, 5.0f).close();
        arrow.transform (AffineTransform::translation (pivot));

        g.setFillColor (role (Role::outline));
        g.fillPath (arrow);

        const auto saved = g.saveState();

        g.addTransform (AffineTransform::rotation (MathConstants<float>::halfPi, pivot.getX(), pivot.getY()));
        g.setFillColor (Colors::orange);
        g.fillPath (arrow);
    });

    f.paint ("graphics/drawing_blend-multiply", small, [] (Graphics& g, Rectangle<float> area)
    {
        auto stripes = area;
        for (const auto color : { Colors::tomato, Colors::mediumseagreen, Colors::cornflowerblue, Colors::white })
        {
            g.setFillColor (color);
            g.fillRect (stripes.removeFromLeft (area.getWidth() / 4.0f));
        }

        const auto saved = g.saveState();

        g.setBlendMode (BlendMode::Multiply);
        g.setFillColor (Colors::yellow.withAlpha (0.5f));
        g.fillRect (area.reduced (0.0f, 24.0f));
    });

    f.paint ("graphics/drawing_blend-glow", small, [] (Graphics& g, Rectangle<float> area)
    {
        g.setFillColor (Color (0xff10141c));
        g.fillRoundedRect (area, 8.0f);

        const auto saved = g.saveState();

        g.setBlendMode (BlendMode::Additive);
        g.setAdditiveAmount (0.6f);
        g.setFeather (12.0f);
        g.setFillColor (Colors::cyan);
        g.fillEllipse (area.withSizeKeepingCenter (120.0f, 40.0f));
    });

    f.paint ("graphics/drawing_slices", { 320.0f, 200.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        slices (g, area, 24.0f, 16.0f, 80.0f);
    });
}

//==============================================================================
// docs/graphics/primitives.md

void addPrimitivesFigures (Figures& f)
{
    using namespace yup;

    f.paint ("graphics/primitives_rectangle", { 480.0f, 148.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto r = area.removeFromLeft (200.0f).withSizeKeepingCenter (200.0f, 100.0f);
        auto legend = area.withTrimmedLeft (20.0f).reduced (0.0f, 20.0f);

        const std::tuple<Rectangle<float>, Color, const char*> shown[] = {
            { r, role (Role::textMuted), "r" },
            { r.reduced (8.0f), Colors::cornflowerblue, "r.reduced (8)" },
            { r.withSizeKeepingCenter (100.0f, 40.0f), Colors::tomato, "r.withSizeKeepingCenter (100, 40)" }
        };

        g.setStrokeWidth (1.0f);
        for (const auto& [bounds, color, name] : shown)
        {
            g.setStrokeColor (color);
            g.strokeRect (bounds.reduced (0.5f));

            g.setFillColor (color);
            g.fillFittedText (name, font (12.0f), legend.removeFromTop (20.0f), Justification::centerLeft);
        }
    });

    f.paint ("graphics/primitives_slicing", { 400.0f, 220.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        slices (g, area, 30.0f, 24.0f, 120.0f);
    });

    f.paint ("graphics/primitives_fitting", medium, [] (Graphics& g, Rectangle<float> area)
    {
        const Rectangle<float> artwork (0.0f, 0.0f, 24.0f, 24.0f);
        const std::pair<Fitting, Justification> fits[] = {
            { Fitting::scaleToFit, Justification::center },
            { Fitting::scaleToFit, Justification::centerLeft },
            { Fitting::fill, Justification::center }
        };

        Path icon;
        icon.addStar ({ 12.0f, 12.0f }, 5, 5.0f, 12.0f);

        labelled (g, area, { "scaleToFit, center", "scaleToFit, centerLeft", "fill" }, 3, [&] (int index, Rectangle<float> cell)
        {
            g.setStrokeColor (role (Role::outline));
            g.strokeRect (cell.reduced (0.5f));

            g.addTransform (artwork.getTransformToFit (cell, fits[index].first, fits[index].second));
            g.setFillColor (role (Role::accent));
            g.fillPath (icon);
        });
    });

    f.paint ("graphics/primitives_edge-spacing", small, [] (Graphics& g, Rectangle<float> area)
    {
        const EdgeSpacing<float> padding { 32.0f, 12.0f };
        const auto content = padding.subtractedFrom (area);

        g.setFillColor (role (Role::accent).withAlpha (0.35f));
        g.fillRect (area);

        g.setFillColor (role (Role::surfaceRaised));
        g.fillRect (content);

        g.setFillColor (role (Role::text));
        g.fillFittedText ("content", font (12.0f), content, Justification::center);
    });

    f.paint ("graphics/primitives_gradient-spread", medium, [] (Graphics& g, Rectangle<float> area)
    {
        labelled (g, area, { "Pad", "Repeat", "Reflect" }, 3, [&g] (int index, Rectangle<float> cell)
        {
            const ColorGradient gradient (Colors::royalblue, cell.getX() + cell.getWidth() * 0.4f, 0.0f, Colors::orange, cell.getX() + cell.getWidth() * 0.6f, 0.0f);

            g.setFillColorGradient (gradient.withSpread (static_cast<ColorGradient::Spread> (index)));
            g.fillRoundedRect (cell, 8.0f);
        });
    });

    f.paint ("graphics/primitives_path", { 320.0f, 148.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        Path p;
        p.moveTo (10.0f, 10.0f).lineTo (90.0f, 10.0f).quadTo (50.0f, 90.0f, 90.0f, 90.0f);
        p.cubicTo (10.0f, 90.0f, 10.0f, 50.0f, 10.0f, 10.0f).close();

        g.addTransform (AffineTransform::translation ((area.getWidth() - 100.0f) / 2.0f, 0.0f));
        g.setFillColor (Colors::orange);
        g.fillPath (p);
    });

    f.paint ("graphics/primitives_stroke-polygon", small, [] (Graphics& g, Rectangle<float> area)
    {
        Path spectrum;
        spectrum.moveTo (6.0f, area.getHeight() / 2.0f);
        for (float x = 6.0f; x <= area.getWidth() - 6.0f; x += 4.0f)
            spectrum.lineTo (x, area.getHeight() / 2.0f + std::sin (x * 0.05f) * std::cos (x * 0.011f) * 30.0f);

        g.setFillColor (role (Role::accent));
        g.setFeather (4.0f);
        g.fillPath (spectrum.createStrokePolygon (4.0f, StrokeJoin::Round, StrokeCap::Round));
    });

    f.paint ("graphics/primitives_stroke-types", medium, [] (Graphics& g, Rectangle<float> area)
    {
        const StrokeType strokes[] = {
            { 14.0f, StrokeJoin::Miter, StrokeCap::Butt },
            { 14.0f, StrokeJoin::Round, StrokeCap::Round },
            { 14.0f, StrokeJoin::Bevel, StrokeCap::Square }
        };

        labelled (g, area, { "Miter, Butt", "Round, Round", "Bevel, Square" }, 3, [&] (int index, Rectangle<float> cell)
        {
            const auto shape = cell.reduced (26.0f, 16.0f);

            Path p;
            p.moveTo (shape.getX(), shape.getBottom()).lineTo (shape.getCenterX(), shape.getY()).lineTo (shape.getRight(), shape.getBottom());

            g.setStrokeColor (role (Role::accent));
            g.setStrokeType (strokes[index]);
            g.strokePath (p);

            g.setStrokeColor (role (Role::onAccent));
            g.setStrokeType (StrokeType (1.0f));
            g.strokePath (p);
        });
    });

    f.paint ("graphics/primitives_transform", { 560.0f, 120.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        labelled (g, area, { "translation", "rotation", "scaling", "shearing" }, 4, [&g] (int index, Rectangle<float> cell)
        {
            const auto center = cell.getCenter();
            const auto square = cell.withSizeKeepingCenter (36.0f, 36.0f);

            const AffineTransform transforms[] = {
                AffineTransform::translation (14.0f, 8.0f),
                AffineTransform::rotation (0.5f, center),
                AffineTransform::scaling (1.5f, 0.6f, center),
                AffineTransform::shearing (0.5f, 0.0f, center)
            };

            g.setStrokeColor (role (Role::textMuted));
            g.strokeRect (square.reduced (0.5f));

            g.addTransform (transforms[index]);
            g.setFillColor (role (Role::accent).withAlpha (0.8f));
            g.fillRect (square);
        });
    });

    f.paint ("graphics/primitives_blend-modes", { 560.0f, 200.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto names = { "SrcOver", "Screen", "Overlay", "Darken", "Lighten", "ColorDodge", "ColorBurn", "HardLight", "SoftLight", "Difference", "Exclusion", "Multiply", "Hue", "Saturation", "Color", "Luminosity" };

        labelled (g, area, names, 8, [&g] (int index, Rectangle<float> cell)
        {
            const auto backdrop = cell.withSizeKeepingCenter (48.0f, 48.0f);

            g.setClipPath (backdrop);
            g.setFillColorGradient ({ Colors::royalblue, backdrop.getTopLeft(), Colors::gold, backdrop.getBottomRight() });
            g.fillRect (backdrop);

            g.setBlendMode (static_cast<BlendMode> (index));
            g.setFillColor (Colors::tomato);
            g.fillEllipse (backdrop.translated (12.0f, 12.0f).reduced (5.0f));
        });
    });
}

//==============================================================================
// docs/graphics/graphics-class.md

void addGraphicsClassFigures (Figures& f)
{
    using namespace yup;

    f.paint ("graphics/graphics-class_stroke-position", medium, [] (Graphics& g, Rectangle<float> area)
    {
        labelled (g, area, { "Inside", "Center", "Outside" }, 3, [&g] (int index, Rectangle<float> cell)
        {
            const auto shape = cell.withSizeKeepingCenter (70.0f, 44.0f);

            g.setFillColor (role (Role::surfaceRaised));
            g.fillRect (shape);

            g.setStrokeColor (role (Role::accent).withAlpha (0.6f));
            g.setStrokeWidth (12.0f);
            g.setStrokePosition (static_cast<StrokePosition> (index));
            g.strokeRect (shape);

            g.setStrokeColor (role (Role::text));
            g.setStrokeWidth (1.0f);
            g.setStrokePosition (StrokePosition::Center);
            g.strokeRect (shape);
        });
    });

    f.paint ("graphics/graphics-class_image-paint", medium, [] (Graphics& g, Rectangle<float> area)
    {
        const auto coin = docsimgImage ("data/coin.png");

        labelled (g, area, { "Clamp", "Repeat", "Mirror" }, 3, [&] (int index, Rectangle<float> cell)
        {
            const auto wrap = static_cast<ImageWrap> (index);
            const auto tile = cell.withSizeKeepingCenter (44.0f, 44.0f);

            g.setFillColor (role (Role::surfaceRaised));
            g.fillRoundedRect (cell, 8.0f);

            g.setFillImage (coin, AffineTransform::scaling (44.0f / 256.0f).translated (tile.getX(), tile.getY()), { wrap, wrap, ImageFilter::Linear });
            g.fillRoundedRect (cell, 8.0f);
        });
    });

    f.paint ("graphics/graphics-class_modulation", wide, [] (Graphics& g, Rectangle<float> area)
    {
        const auto coin = docsimgImage ("data/coin.png");

        labelled (g, area, { "as drawn", "setOpacity (0.5)", "setTint (orange)", "setFeather (6)" }, 4, [&] (int index, Rectangle<float> cell)
        {
            const auto shape = cell.withSizeKeepingCenter (96.0f, 80.0f).withSize (56.0f, 56.0f);

            if (index == 1)
                g.setOpacity (0.5f);
            else if (index == 2)
                g.setTint (Colors::orange);
            else if (index == 3)
                g.setFeather (6.0f);

            g.setFillColor (Colors::cornflowerblue);
            g.fillRoundedRect (shape, 8.0f);
            g.drawImage (coin, shape.translated (40.0f, 24.0f));
        });
    });

    f.paint ("graphics/graphics-class_clip-stroke", { 320.0f, 180.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto coin = docsimgImage ("data/coin.png");
        const auto bounds = area.withSizeKeepingCenter (area.getHeight(), area.getHeight());

        Path wave;
        wave.moveTo (bounds.getX(), bounds.getCenterY());
        for (float x = 0.0f; x <= bounds.getWidth(); x += 4.0f)
            wave.lineTo (bounds.getX() + x, bounds.getCenterY() + std::sin (x * 0.1f) * 22.0f);

        g.setOpacity (0.15f);
        g.drawImage (coin, bounds);
        g.setOpacity (1.0f);

        const auto saved = g.saveState();

        g.setClipStroke (wave, StrokeType (26.0f, StrokeJoin::Round, StrokeCap::Round));
        g.drawImage (coin, bounds);
    });

    f.component ("graphics/graphics-class_image-mesh", { 320.0f, 200.0f }, []
    {
        return std::make_unique<WarpedImage>();
    });

    f.paint ("graphics/graphics-class_layer-mask", { 320.0f, 180.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto target = area.withSizeKeepingCenter (area.getHeight(), area.getHeight());

        if (auto layer = g.beginTransparencyLayer (target, 1.0f, true); layer.isValid())
        {
            const auto bounds = target.withPosition (0.0f, 0.0f);
            layer.getGraphics().drawImage (docsimgImage ("data/coin.png"), bounds);

            if (auto* mask = layer.addMask (LayerMaskMode::Luminance))
            {
                mask->setFillColorGradient ({ Colors::white, 0.0f, 0.0f, Colors::black, bounds.getWidth(), 0.0f });
                mask->fillAll();
            }

            layer.commit();
        }
    });
}

//==============================================================================
// docs/graphics/fonts.md

void addFontFigures (Figures& f)
{
    using namespace yup;

    f.paint ("graphics/fonts_variable-axes", { 480.0f, 188.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        auto weights = area.removeFromLeft (area.getWidth() / 2.0f);

        g.setFillColor (role (Role::text));

        for (const auto weight : { 200, 400, 600, 800, 1000 })
            g.fillFittedText ("wght " + String (weight), font (20.0f).withAxisValue ("wght", static_cast<float> (weight)), weights.removeFromTop (28.0f), Justification::centerLeft);

        for (const auto width : { 25, 60, 100, 130, 151 })
            g.fillFittedText ("wdth " + String (width), font (20.0f).withAxisValue ("wdth", static_cast<float> (width)), area.removeFromTop (28.0f), Justification::centerLeft);
    });

    f.paint ("graphics/fonts_styled-text", { 320.0f, 160.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const auto box = area.withSizeKeepingCenter (200.0f, area.getHeight());

        StyledText text;
        {
            auto mod = text.startUpdate();
            mod.appendText ("Hello ", font (18.0f));
            mod.appendText ("bold", font (18.0f).withAxisValue ("wght", 800.0f));
            mod.appendText (" and ", font (18.0f));
            mod.appendText ("colored", Colors::tomato, font (18.0f));
            mod.appendText (" text, wrapped and centered in 200 pixels.", font (18.0f));
            mod.setHorizontalAlign (StyledText::center);
            mod.setMaxSize ({ 200.0f, 0.0f });
        }

        g.setStrokeColor (role (Role::outline));
        g.strokeRect (box.reduced (0.5f));

        g.setFillColor (role (Role::text));
        g.fillFittedText (text, box);
    });

    // The same point anchors the start, the middle and the end of a line on its baseline
    f.paint ("graphics/fonts_anchored-text", { 480.0f, 164.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        const char* const names[] = { "Anchor::start", "Anchor::middle", "Anchor::end" };

        for (int row = 0; row < 3; ++row)
        {
            const Point<float> point (area.getCenterX(), 28.0f + 36.0f * static_cast<float> (row));
            const AnchoredText text (names[row], font (20.0f), point, static_cast<AnchoredText::Anchor> (row));

            g.setStrokeColor (role (Role::outline));
            g.strokeLine (area.getX(), point.getY(), area.getRight(), point.getY());

            g.setFillColor (role (Role::text));
            g.fillFittedText (text.getStyledText(), text.getBounds());

            g.setFillColor (Colors::tomato);
            g.fillEllipse (dot (point, 3.0f));
        }
    });

    f.paint ("graphics/fonts_color-emoji", { 480.0f, 110.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        g.setFillColor (role (Role::text));
        g.fillFittedText (String::fromUTF8 ("Emoji just work 🎹 🔊 🎉"), font (30.0f), area, Justification::center);
    });
}

//==============================================================================
// docs/graphics/svg.md

void addSvgFigures (Figures& f)
{
    using namespace yup;

    f.paint ("graphics/svg_fitting", { 560.0f, 228.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        Drawable badge;
        badge.parseSVG (R"(<svg viewBox="0 0 40 40"><rect width="40" height="40" rx="9" fill="#7e44fc"/><circle cx="20" cy="20" r="10" fill="#fff"/></svg>)");

        labelled (g, area, { "none", "scaleToFit", "fitWidth", "fitHeight", "scaleToFill", "fill", "tile", "centerCrop" }, 4, [&] (int index, Rectangle<float> cell)
        {
            g.setStrokeColor (role (Role::outline));
            g.strokeRect (cell.reduced (0.5f));

            g.setClipPath (cell);
            badge.paint (g, cell, static_cast<Fitting> (index));
        });
    });
}

//==============================================================================
// docs/graphics/expressions.md

void addExpressionFigures (Figures& f)
{
    using namespace yup;

    f.paint ("graphics/expressions_bars", { 480.0f, 180.0f }, [] (Graphics& g, Rectangle<float> area)
    {
        ExpressionContext context;
        addGraphicsExpressionTypes (context);
        auto values = context.addArray<float> ("values");
        auto row = context.addInput<int> ("row");
        auto bounds = context.addInput<Rectangle<float>> ("bounds");

        auto bars = ExpressionDrawing::create (R"(
            let w = bounds.width / len (values);
            let h = values[row] * bounds.height;
            rect = rect (bounds.x + row * w + 1, bounds.bottom - h, w - 2, h);
            cornerRadius = min (3, w / 4);
            fill = hsl (0.6 - values[row] * 0.6, 0.8, 0.55);
        )",
                                               context);

        if (bars.failed())
            return failFigure (bars.getErrorMessage());

        std::vector<float> data (40);
        for (std::size_t index = 0; index < data.size(); ++index)
            data[index] = 0.5f + 0.45f * std::sin (static_cast<float> (index) * 0.33f) * std::cos (static_cast<float> (index) * 0.07f);

        ExpressionState state (bars.getReference().getProgram());
        state.bindArray (values, data.data(), static_cast<int> (data.size()));
        state.bindRowIndex (row);
        state.set (bounds, area);

        bars.getReference().draw (g, state, static_cast<int> (data.size()));
    });
}

} // namespace

//==============================================================================

void addGraphicsFigures (Figures& f)
{
    addDrawingFigures (f);
    addPrimitivesFigures (f);
    addGraphicsClassFigures (f);
    addFontFigures (f);
    addSvgFigures (f);
    addExpressionFigures (f);
}
