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

#include <array>
#include <cmath>
#include <memory>
#include <vector>

//==============================================================================
/**
    Two octaves of piano keys that rise under the pointer like a hand gliding over them.

    Every key is its own small Component on a box-shaped surface of a SurfaceView:
    flat at rest, a plain 2D keyboard. Near the pointer the board tilts back, the
    keys around the pointer rise in a wave and the one under it dips, lit like
    glossy keys. Keys light up on hover through the 3D; pressing them does nothing
    yet, there is no sound.
*/
class Keys : public Demo
{
public:
    //==============================================================================
    Keys()
        : Demo ("Keys")
    {
        addAndMakeVisible (view);

        // White keys first, so the black ones draw over them and are hit first.
        for (const auto black : { false, true })
        {
            for (int note = 0; note < numSemitones; ++note)
            {
                if (isBlack (note) != black)
                    continue;

                auto key = std::make_unique<Key> (black);
                key->surface = view.addSurface (*key);
                key->note = note;
                keys.push_back (std::move (key));
            }
        }
    }

    void resized() override
    {
        const auto scale = contentScale();
        view.setBounds (getLocalBounds());
        view.setContentScale (scale);

        board = centeredContent (boardWidth, boardHeight);
        const auto whiteWidth = board.getWidth() / numWhiteKeys;

        for (auto& key : keys)
        {
            const auto whiteIndex = whiteKeysBefore (key->note);
            const auto rect = key->black ? yup::Rectangle<float> (board.getX() + whiteIndex * whiteWidth - 0.3f * whiteWidth, board.getY(), 0.6f * whiteWidth, 0.62f * board.getHeight())
                                         : yup::Rectangle<float> (board.getX() + whiteIndex * whiteWidth, board.getY(), whiteWidth, board.getHeight());

            key->screenBounds = rect.reduced (1.0f * scale, 0.0f);
            key->setBounds (key->screenBounds.getX(), key->screenBounds.getY(), key->screenBounds.getWidth() / scale, key->screenBounds.getHeight() / scale);
            key->meshState = { -1.0e9f, -1.0e9f, -1.0e9f };
        }

        meshTilt = -1.0f;
    }

private:
    //==============================================================================
    static constexpr int numSemitones = 25; // C to C, two octaves
    static constexpr int numWhiteKeys = 15;
    static constexpr float boardWidth = 720.0f;
    static constexpr float boardHeight = 210.0f;
    static constexpr float whiteThickness = 16.0f;
    static constexpr float blackThickness = 24.0f;
    static constexpr float maxRise = 26.0f;
    static constexpr float blackRaise = 14.0f; // black key tops above the white ones, at full lift
    static constexpr float blackDip = 8.0f;    // smaller than blackRaise, so a pressed down black key stays above the white ones
    static constexpr float dip = 8.0f;
    static constexpr float maxTiltRadians = 0.5f;

    static bool isBlack (int note)
    {
        const auto step = note % 12;
        return step == 1 || step == 3 || step == 6 || step == 8 || step == 10;
    }

    /** For a white key its index among the white keys; for a black key the white key it sits after, plus one. */
    static int whiteKeysBefore (int note)
    {
        int count = 0;
        for (int n = 0; n < note; ++n)
            count += isBlack (n) ? 0 : 1;

        return count;
    }

    //==============================================================================
    class Key : public yup::Component
    {
    public:
        explicit Key (bool isBlackKey)
            : yup::Component ("Key")
            , black (isBlackKey)
        {
            setOpaque (false);
        }

        void paint (yup::Graphics& g) override
        {
            const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();
            auto color = palette.getColor (black ? yup::ThemePalette::Role::surface : yup::ThemePalette::Role::text);
            if (hovered)
                color = color.mixedWith (palette.getColor (yup::ThemePalette::Role::accent), 0.45f, yup::ColorSpace::SRGB);

            g.setFillColor (color);
            g.fillRoundedRect (getLocalBounds(), 0.0f, 0.0f, 5.0f, 5.0f);
            g.setStrokeColor (palette.getColor (yup::ThemePalette::Role::outline));
            g.setStrokeWidth (1.0f);
            g.strokeRoundedRect (getLocalBounds().reduced (0.5f), 0.0f, 0.0f, 5.0f, 5.0f);
        }

        void mouseEnter (const yup::MouseEvent&) override
        {
            hovered = true;
            repaint();
        }

        void mouseExit (const yup::MouseEvent&) override
        {
            hovered = false;
            repaint();
        }

        const bool black;
        int note = 0;
        int surface = -1;
        bool hovered = false;
        yup::Rectangle<float> screenBounds; // where the key appears, scaled with the demo
        float height = 0.0f;                // current rise toward the viewer as drawn, in design units
        float restHeight = 0.0f;            // the same without the dip under the pointer, for picking
        std::array<float, 3> meshState { -1.0e9f, -1.0e9f, -1.0e9f }; // height, rest height and lift of the last mesh
    };

    //==============================================================================
    void advance (float deltaSeconds) override
    {
        const auto scale = contentScale();
        const auto lift = keysLift.update (pointer, board, deltaSeconds);
        view.setLift (lift);

        const auto follow = 1.0f - std::exp (-deltaSeconds / 0.08f);
        const auto wave = 2.2f * board.getWidth() / numWhiteKeys;

        // White keys first: each black key then follows the higher of the two white keys around it,
        // so a dipped black key still stands above its neighbours.
        std::array<float, numSemitones> heights {}, restHeights {};

        for (auto& key : keys)
        {
            const auto dipping = pointer.has_value() && key->hovered;

            if (key->black)
            {
                // Already smoothed through its neighbours, so it never lags below them.
                const auto left = static_cast<std::size_t> (key->note - 1), right = static_cast<std::size_t> (key->note + 1);
                key->restHeight = yup::jmax (restHeights[left], restHeights[right]);
                key->height = yup::jmax (heights[left], heights[right]) - (dipping ? blackDip * lift : 0.0f);
            }
            else
            {
                auto rise = 0.0f;
                if (pointer.has_value() && lift > 0.0f)
                {
                    const auto across = (key->screenBounds.getCenterX() - pointer->getX()) / wave;
                    rise = maxRise * std::exp (-across * across);
                }

                key->restHeight += (rise * lift - key->restHeight) * follow;
                key->height += ((dipping ? -dip : rise) * lift - key->height) * follow;

                for (auto* value : { &key->restHeight, &key->height })
                {
                    if (std::abs (*value) < 0.01f && rise == 0.0f && ! dipping)
                        *value = 0.0f;
                }
            }

            heights[static_cast<std::size_t> (key->note)] = key->height;
            restHeights[static_cast<std::size_t> (key->note)] = key->restHeight;

            // The mesh also depends on the lift, which sets how far black keys stand above the white ones.
            const std::array<float, 3> meshState { key->height, key->restHeight, lift };
            if (std::abs (meshState[0] - key->meshState[0]) > 0.01f || std::abs (meshState[1] - key->meshState[1]) > 0.01f || std::abs (meshState[2] - key->meshState[2]) > 0.0005f)
            {
                key->meshState = meshState;
                buildKey (*key, scale, lift);
            }
        }

        const auto tilt = maxTiltRadians * lift;
        if (std::abs (tilt - meshTilt) > 0.0001f)
        {
            meshTilt = tilt;

            const auto pivot = pixelToWorld (board.getCenter(), 0.0f, getLocalBounds().getSize());
            const auto model = yup::Matrix4::translation (-pivot)
                                   .followedBy (yup::Matrix4::rotationX (-tilt))
                                   .followedBy (yup::Matrix4::translation (pivot));

            for (auto& key : keys)
                view.setSurfaceTransform (key->surface, model);
        }
    }

    /** Draws the key at its height and picks it at its rest height, without the dip under the pointer:
        picking the dipped key would move it out from under the pointer, unhover it, and so on. */
    void buildKey (Key& key, float scale, float lift)
    {
        // Never level with the white tops, or the two would fight over the same depth and flicker;
        // a small gap at rest keeps the mapping within a fraction of a pixel of the flat drawing.
        const auto raise = key.black ? blackRaise * (0.15f + 0.85f * lift) : 0.0f;
        const auto thickness = (key.black ? blackThickness : whiteThickness) * scale;

        std::vector<yup::uint32> indices;
        auto drawn = boxVertices (key.screenBounds, (key.height + raise) * scale, thickness, indices);
        std::vector<yup::uint32> unused;
        auto picked = boxVertices (key.screenBounds, (key.restHeight + raise) * scale, thickness, unused);

        view.setSurfaceMesh (key.surface, std::move (drawn), std::move (indices), std::move (picked));
    }

    /** A box: the top shows the key, the sides sample its own edge color. */
    std::vector<SurfaceVertex> boxVertices (yup::Rectangle<float> rect, float top, float thickness, std::vector<yup::uint32>& indices) const
    {
        const auto viewport = getLocalBounds().getSize();
        const auto bottom = top - thickness;

        const auto corner = [&] (yup::Point<float> p, float z) { return pixelToWorld (p, z, viewport); };
        const auto tl = rect.getTopLeft(), tr = rect.getTopRight(), br = rect.getBottomRight(), bl = rect.getBottomLeft();

        std::vector<SurfaceVertex> vertices;

        const auto face = [&] (std::array<yup::Vector3<float>, 4> positions, std::array<yup::Point<float>, 4> uvs, yup::Vector3<float> normal)
        {
            const auto first = static_cast<yup::uint32> (vertices.size());
            for (std::size_t i = 0; i < 4; ++i)
                vertices.push_back ({ positions[i], uvs[i], normal });

            indices.insert (indices.end(), { first, first + 1, first + 2, first, first + 2, first + 3 });
        };

        constexpr float edge = 0.04f; // sides sample just inside the key, away from its outline
        face ({ corner (tl, top), corner (tr, top), corner (br, top), corner (bl, top) }, { { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } } }, { 0.0f, 0.0f, 1.0f });
        face ({ corner (bl, top), corner (br, top), corner (br, bottom), corner (bl, bottom) }, { { { 0.0f, 1.0f - edge }, { 1.0f, 1.0f - edge }, { 1.0f, 1.0f - edge }, { 0.0f, 1.0f - edge } } }, { 0.0f, -1.0f, 0.0f });
        face ({ corner (tr, top), corner (tl, top), corner (tl, bottom), corner (tr, bottom) }, { { { 1.0f, edge }, { 0.0f, edge }, { 0.0f, edge }, { 1.0f, edge } } }, { 0.0f, 1.0f, 0.0f });
        face ({ corner (tl, top), corner (bl, top), corner (bl, bottom), corner (tl, bottom) }, { { { edge, 0.0f }, { edge, 1.0f }, { edge, 1.0f }, { edge, 0.0f } } }, { -1.0f, 0.0f, 0.0f });
        face ({ corner (br, top), corner (tr, top), corner (tr, bottom), corner (br, bottom) }, { { { 1.0f - edge, 1.0f }, { 1.0f - edge, 0.0f }, { 1.0f - edge, 0.0f }, { 1.0f - edge, 1.0f } } }, { 1.0f, 0.0f, 0.0f });

        return vertices;
    }

    //==============================================================================
    SurfaceView view;
    std::vector<std::unique_ptr<Key>> keys;
    yup::Rectangle<float> board;

    Lift keysLift;
    float meshTilt = -1.0f;
};
