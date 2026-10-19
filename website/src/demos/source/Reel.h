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

#include <cmath>
#include <vector>

//==============================================================================
/**
    A plain scrolling list that bends into a reel as the pointer comes near.

    The list is a real ListBox on a SurfaceView: flat at rest, and bent around a
    horizontal and a vertical axis as it lifts. The pointer picks how: over the top
    half the list bends forward, its edges coming toward the viewer, over the bottom
    half backward like a slot machine reel, and the same sideways for the left and
    right halves, flat in the middle. It keeps scrolling, highlighting and selecting
    on the curve.
*/
class Reel : public Demo
{
public:
    //==============================================================================
    Reel()
        : Demo ("Reel")
    {
        list.setModel (&presets);
        panel.addAndMakeVisible (list);

        addAndMakeVisible (view);
        surface = view.addSurface (panel);
    }

    void resized() override
    {
        view.setBounds (getLocalBounds());
        view.setContentScale (contentScale());

        screenBounds = centeredContent (panelWidth, panelHeight);
        panel.setBounds (screenBounds.getX(), screenBounds.getY(), panelWidth, panelHeight);
        list.setBounds (panel.getLocalBounds().reduced (12.0f));
        meshBend = { -2.0f, -2.0f };
    }

    yup::Component* getFocusTarget() override { return &list; }

private:
    //==============================================================================
    static constexpr float panelWidth = 340.0f;
    static constexpr float panelHeight = 460.0f;
    static constexpr int numRows = 48;
    static constexpr float maxArcRadians = 2.6f;
    static constexpr float maxLateralArcRadians = 2.0f;
    static constexpr int numColumns = 24;
    static constexpr int numPresets = 48;

    //==============================================================================
    class Panel : public yup::Component
    {
    public:
        Panel()
            : yup::Component ("ReelPanel")
        {
            setOpaque (false);
        }

        void paint (yup::Graphics& g) override
        {
            const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

            g.setFillColor (palette.getColor (yup::ThemePalette::Role::surface));
            g.fillRoundedRect (getLocalBounds(), 10.0f);
            g.setStrokeColor (palette.getColor (yup::ThemePalette::Role::outline));
            g.setStrokeWidth (1.5f);
            g.strokeRoundedRect (getLocalBounds().reduced (0.75f), 10.0f);
        }
    };

    class Presets : public yup::ListBoxModel
    {
    public:
        int getNumRows() override { return numPresets; }

        yup::String getRowText (int rowIndex) override
        {
            static const char* const names[] = { "glass pad", "deep bell", "soft pluck", "warm drone", "bright lead", "dark keys" };
            return yup::String (names[rowIndex % 6]) + " " + yup::String (rowIndex / 6 + 1).paddedLeft ('0', 2);
        }
    };

    //==============================================================================
    void advance (float deltaSeconds) override
    {
        const auto lift = reelLift.update (pointer, screenBounds, deltaSeconds);
        view.setLift (lift);

        // +1 at the top (left) edge, -1 at the bottom (right) one; kept while the pointer is away so
        // the list settles flat smoothly.
        if (pointer.has_value())
        {
            direction = { yup::jlimit (-1.0f, 1.0f, (screenBounds.getCenterX() - pointer->getX()) / (0.5f * screenBounds.getWidth())),
                          yup::jlimit (-1.0f, 1.0f, (screenBounds.getCenterY() - pointer->getY()) / (0.5f * screenBounds.getHeight())) };
        }

        const auto bend = direction * lift;
        if (std::abs (bend.getX() - meshBend.getX()) > 0.001f || std::abs (bend.getY() - meshBend.getY()) > 0.001f)
        {
            meshBend = bend;
            buildMesh (bend);
        }
    }

    /** Where a point at @p t (0..1) along a side of length @p length lands on that side bent by @p bend. */
    struct Bent
    {
        float offset; // from the middle of the side
        float depth;  // toward the viewer
        float slope;  // depth change per unit of offset
    };

    static Bent bendAlong (float t, float length, float bend, float maxArc)
    {
        const auto arc = maxArc * std::abs (bend);
        if (arc < 0.001f)
            return { (t - 0.5f) * length, 0.0f, 0.0f };

        const auto radius = length / arc;
        const auto angle = (t - 0.5f) * arc;
        const auto towardViewer = bend > 0.0f ? 1.0f : -1.0f;

        return { radius * std::sin (angle), towardViewer * (radius - radius * std::cos (angle)), towardViewer * std::tan (angle) };
    }

    /** A grid bent around both axes: forward (edges toward the viewer) for a positive bend, backward for a negative one. */
    void buildMesh (yup::Point<float> bend)
    {
        const auto viewport = getLocalBounds().getSize();
        const auto center = pixelToWorld (screenBounds.getCenter(), 0.0f, viewport);

        if (std::abs (bend.getX()) < 0.0005f && std::abs (bend.getY()) < 0.0005f)
        {
            view.setSurfaceMesh (surface, SurfaceView::flatQuad (screenBounds, viewport), SurfaceView::quadIndices());
            return;
        }

        std::vector<SurfaceVertex> vertices;
        std::vector<yup::uint32> indices;

        for (int row = 0; row <= numRows; ++row)
        {
            const auto v = row / static_cast<float> (numRows);
            const auto vertical = bendAlong (v, screenBounds.getHeight(), bend.getY(), maxArcRadians);

            for (int column = 0; column <= numColumns; ++column)
            {
                const auto u = column / static_cast<float> (numColumns);
                const auto lateral = bendAlong (u, screenBounds.getWidth(), bend.getX(), maxLateralArcRadians);

                // y grows downward on screen but upward in the world
                const yup::Vector3<float> position { center.getX() + lateral.offset, center.getY() - vertical.offset, lateral.depth + vertical.depth };
                const auto normal = yup::Vector3<float> { -lateral.slope, vertical.slope, 1.0f }.normalized();

                vertices.push_back ({ position, { u, v }, normal });
            }
        }

        const auto stride = static_cast<yup::uint32> (numColumns + 1);
        for (yup::uint32 row = 0; row < static_cast<yup::uint32> (numRows); ++row)
        {
            for (yup::uint32 column = 0; column < static_cast<yup::uint32> (numColumns); ++column)
            {
                const auto a = row * stride + column;
                indices.insert (indices.end(), { a, a + stride, a + stride + 1, a, a + stride + 1, a + 1 });
            }
        }

        view.setSurfaceMesh (surface, std::move (vertices), std::move (indices));
    }

    //==============================================================================
    SurfaceView view;
    Panel panel;
    yup::Rectangle<float> screenBounds; // where the panel appears, scaled with the demo
    yup::ListBox list;
    Presets presets;
    int surface = -1;

    Lift reelLift;
    yup::Point<float> direction;
    yup::Point<float> meshBend { -2.0f, -2.0f };
};
