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

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

//==============================================================================
/**
    A panel of knobs that folds into a dodecahedron as the pointer comes near.

    The panel is twelve pentagons in a grid (four across on wide screens, three on
    tall ones), one rotary knob on each, sized as large as the screen allows both
    flat and folded. As it lifts, every face moves rigidly (its knob never distorts)
    from its place in the panel to its face of the solid, and the solid turns
    toward the pointer. The knobs keep turning by vertical drag on the folded faces.

    While the demo is open it plays an FM loop (see FacetSound). The four knobs on
    the faces turned toward the viewer when folded set its FM amounts; the other
    eight do nothing.
*/
class Facet : public Demo
{
public:
    //==============================================================================
    Facet()
        : Demo ("Facet")
    {
        addAndMakeVisible (view);
        surface = view.addSurface (panel);

        for (int i = 0; i < FacetSound::numAmounts; ++i)
        {
            auto& knob = panel.getKnob (soundKnobs[(std::size_t) i].face);
            knob.setValue (soundKnobs[(std::size_t) i].initial, yup::dontSendNotification);
            knob.onValueChanged = [this, i] (double value) { sound.setAmount (i, static_cast<float> (value)); };
            sound.setAmount (i, static_cast<float> (knob.getValue()));
        }
    }

    void resized() override
    {
        view.setBounds (getLocalBounds());

        const auto columns = getWidth() >= getHeight() ? 4 : 3;
        const auto designSize = gridSize (columns);
        const auto available = getLocalBounds().reduced (margin);

        // As large as fits both the flat grid and the folded solid, whose diameter is 2.8 edges.
        const auto scale = yup::jmax (0.1f, yup::jmin (available.getWidth() / designSize.getWidth(),
                                                       available.getHeight() / designSize.getHeight(),
                                                       yup::jmin (available.getWidth(), available.getHeight()) / (2.8f * designEdge)));

        view.setContentScale (scale);
        screenBounds = getLocalBounds().withSizeKeepingCenter (designSize.getWidth() * scale, designSize.getHeight() * scale);
        panel.setLayout (columns);
        panel.setBounds (screenBounds.getX(), screenBounds.getY(), designSize.getWidth(), designSize.getHeight());

        prepareFaces (columns, scale);
        meshFold = -1.0f;
    }

    yup::Component* getFocusTarget() override { return panel.getFirstKnob(); }

    SoundSource* getSoundSource() override { return &sound; }

private:
    //==============================================================================
    static constexpr int numFaces = 12;
    static constexpr float designEdge = 120.0f;                 // pentagon edge in the panel
    static constexpr float circumradius = designEdge * 0.8507f; // pentagon center to corner: edge / (2 sin 36)
    static constexpr float cellWidth = designEdge * 1.75f;      // a pentagon is 1.618 edges wide
    static constexpr float cellHeight = designEdge * 1.65f;     // and 1.539 edges tall
    static constexpr float margin = 24.0f;
    static constexpr double knobDragSensitivity = 1.0; // a drag as tall as the knob sweeps its whole range

    /** The faces whose normals point at the viewer when folded, (+-1, 0, phi) and (0, +-phi, 1) of
        dodecahedronFaces(), with the FM amount each plays and its starting value. */
    struct SoundKnob
    {
        int face;
        double initial;
    };

    static constexpr std::array<SoundKnob, FacetSound::numAmounts> soundKnobs { {
        { 3, 0.35 },  // left: operator 1 feedback
        { 4, 0.45 },  // bottom: 2 into 1
        { 9, 0.3 },   // right: 3 into 2
        { 10, 0.2 },  // top: 4 into 3
    } };

    using Pentagon = std::array<yup::Point<float>, 5>;

    static yup::Size<float> gridSize (int columns)
    {
        return { cellWidth * columns, cellHeight * (numFaces / columns) };
    }

    /** The faces in panel coordinates, pointing up, row after row. */
    static std::array<Pentagon, numFaces> gridPentagons (int columns)
    {
        constexpr auto twoPi = yup::MathConstants<float>::twoPi;
        constexpr auto pentagonHeight = circumradius * 1.809f; // radius + radius * cos 36

        std::array<Pentagon, numFaces> pentagons;

        for (int i = 0; i < numFaces; ++i)
        {
            const yup::Point<float> center { cellWidth * (i % columns + 0.5f),
                                             cellHeight * (i / columns) + 0.5f * (cellHeight - pentagonHeight) + circumradius };

            for (int k = 0; k < 5; ++k)
            {
                const auto angle = twoPi * k / 5.0f;
                pentagons[static_cast<std::size_t> (i)][static_cast<std::size_t> (k)] = center + yup::Point<float> (std::sin (angle), -std::cos (angle)) * circumradius;
            }
        }

        return pentagons;
    }

    static yup::Point<float> centerOf (const Pentagon& pentagon)
    {
        yup::Point<float> sum;
        for (const auto& corner : pentagon)
            sum = sum + corner;

        return sum * 0.2f;
    }

    //==============================================================================
    class Panel : public yup::Component
    {
    public:
        Panel()
            : yup::Component ("FacetPanel")
        {
            setOpaque (false);

            auto& random = yup::Random::getSystemRandom();
            for (int i = 0; i < numFaces; ++i)
            {
                auto knob = std::make_unique<yup::Slider> (yup::Slider::RotaryVerticalDrag, "knob");
                knob->setRange (0.0, 1.0);
                knob->setMouseDragSensitivity (knobDragSensitivity);
                knob->setValue (0.2 + 0.6 * random.nextDouble());
                addAndMakeVisible (*knob);
                knobs.push_back (std::move (knob));
            }
        }

        void setLayout (int newColumns)
        {
            columns = newColumns;
            pentagons = gridPentagons (columns);

            // The knob fills most of the circle inscribed in its pentagon (0.688 edges).
            const auto knobSize = designEdge * 0.688f * 2.0f * 0.85f;
            for (std::size_t i = 0; i < knobs.size(); ++i)
                knobs[i]->setBounds (yup::Rectangle<float> (knobSize, knobSize).withCenter (centerOf (pentagons[i])));

            repaint();
        }

        yup::Slider* getFirstKnob() const { return knobs.front().get(); }

        yup::Slider& getKnob (int index) const { return *knobs[(std::size_t) index]; }

        /** How lit each face is by the sound, from 0 to 1. */
        void setFaceGlow (const std::array<float, numFaces>& newGlow)
        {
            glow = newGlow;
            repaint();
        }

        void paint (yup::Graphics& g) override
        {
            const auto& palette = yup::ApplicationTheme::getGlobalTheme()->getPalette();

            for (std::size_t i = 0; i < pentagons.size(); ++i)
            {
                const auto& pentagon = pentagons[i];

                yup::Path path;
                path.moveTo (pentagon[0]);
                for (std::size_t k = 1; k < pentagon.size(); ++k)
                    path.lineTo (pentagon[k]);
                path.closeSubPath();

                g.setFillColor (palette.getColor (yup::ThemePalette::Role::surfaceRaised).mixedWith (blueColor, 0.65f * glow[i], yup::ColorSpace::SRGB));
                g.fillPath (path);
                g.setStrokeColor (palette.getColor (yup::ThemePalette::Role::outline));
                g.setStrokeWidth (1.5f);
                g.strokePath (path);
            }
        }

    private:
        std::vector<std::unique_ptr<yup::Slider>> knobs;
        std::array<Pentagon, numFaces> pentagons {};
        std::array<float, numFaces> glow {};
        int columns = 4;
    };

    //==============================================================================
    using Corners = std::array<yup::Vector3<float>, 5>;

    /** The faces of a dodecahedron with edge 2 / phi around the origin, corners counter-clockwise seen from outside. */
    static std::array<Corners, numFaces> dodecahedronFaces()
    {
        constexpr auto phi = 1.618034f;
        constexpr auto inv = 1.0f / phi;

        std::vector<yup::Vector3<float>> vertices;
        for (const auto x : { -1.0f, 1.0f })
            for (const auto y : { -1.0f, 1.0f })
                for (const auto z : { -1.0f, 1.0f })
                    vertices.push_back ({ x, y, z });

        for (const auto a : { -1.0f, 1.0f })
        {
            for (const auto b : { -1.0f, 1.0f })
            {
                vertices.push_back ({ 0.0f, a * inv, b * phi });
                vertices.push_back ({ a * inv, b * phi, 0.0f });
                vertices.push_back ({ a * phi, 0.0f, b * inv });
            }
        }

        // Face normals: (1, 0, phi) and its cyclic permutations, every sign. Each touches exactly
        // five vertices, all at the same distance along it, e.g. (0, phi, 1) the vertices
        // (+-1/phi, phi, 0), (+-1, 1, 1) and (0, 1/phi, phi).
        std::vector<yup::Vector3<float>> directions;
        for (const auto a : { -1.0f, 1.0f })
        {
            for (const auto b : { -1.0f, 1.0f })
            {
                directions.push_back ({ a, 0.0f, b * phi });
                directions.push_back ({ 0.0f, a * phi, b });
                directions.push_back ({ a * phi, b, 0.0f });
            }
        }

        std::array<Corners, numFaces> faces;

        for (std::size_t f = 0; f < directions.size(); ++f)
        {
            const auto direction = directions[f].normalized();

            auto nearest = vertices;
            std::sort (nearest.begin(), nearest.end(), [&] (const auto& a, const auto& b)
            {
                return a.dotProduct (direction) > b.dotProduct (direction);
            });

            yup::Vector3<float> center;
            for (std::size_t k = 0; k < 5; ++k)
                center = center + nearest[k] * 0.2f;

            const auto u = (nearest[0] - center).normalized();
            const auto w = direction.crossProduct (u);

            std::sort (nearest.begin(), nearest.begin() + 5, [&] (const auto& a, const auto& b)
            {
                return std::atan2 ((a - center).dotProduct (w), (a - center).dotProduct (u))
                     < std::atan2 ((b - center).dotProduct (w), (b - center).dotProduct (u));
            });

            std::copy (nearest.begin(), nearest.begin() + 5, faces[f].begin());
        }

        return faces;
    }

    //==============================================================================
    /** One face: its corners flat in the panel and folded on the solid, and the rotation between the two. */
    struct Face
    {
        Corners flat;
        std::array<yup::Point<float>, 5> uv;
        yup::Vector3<float> flatCenter;
        yup::Vector3<float> foldedCenter;
        yup::Quaternion foldRotation;
    };

    /** Works out each face's flat and folded pose for the current layout and size. */
    void prepareFaces (int columns, float scale)
    {
        const auto viewport = getLocalBounds().getSize();
        const auto designSize = gridSize (columns);
        const auto solidCenter = pixelToWorld (screenBounds.getCenter(), 0.0f, viewport);
        const auto solidScale = scale * designEdge / (2.0f / 1.618034f);
        const auto pentagons = gridPentagons (columns);
        const auto solid = dodecahedronFaces();

        for (std::size_t i = 0; i < faces.size(); ++i)
        {
            auto& face = faces[i];
            Corners folded;

            for (std::size_t c = 0; c < 5; ++c)
            {
                // Flat corners from the panel are clockwise on screen, which is counter-clockwise in the y up world.
                face.flat[c] = pixelToWorld (pentagons[i][c] * scale + screenBounds.getTopLeft(), 0.0f, viewport);
                face.uv[c] = { pentagons[i][c].getX() / designSize.getWidth(), pentagons[i][c].getY() / designSize.getHeight() };
                folded[c] = solidCenter + solid[i][c] * solidScale;
            }

            if ((face.flat[1] - face.flat[0]).crossProduct (face.flat[2] - face.flat[0]).getZ() < 0.0f)
            {
                std::reverse (face.flat.begin(), face.flat.end());
                std::reverse (face.uv.begin(), face.uv.end());
            }

            face.flatCenter = average (face.flat);
            face.foldedCenter = average (folded);
            face.foldRotation = rotationBetween (face.flat, folded);
        }
    }

    static yup::Vector3<float> average (const Corners& corners)
    {
        yup::Vector3<float> sum;
        for (const auto& corner : corners)
            sum = sum + corner;

        return sum * 0.2f;
    }

    /** The rotation turning the frame of one regular polygon onto the frame of another of the same shape. */
    static yup::Quaternion rotationBetween (const Corners& from, const Corners& to)
    {
        const auto frame = [] (const Corners& t)
        {
            const auto u = (t[1] - t[0]).normalized();
            const auto n = (t[1] - t[0]).crossProduct (t[2] - t[0]).normalized();
            return std::array { u, n.crossProduct (u), n };
        };

        const auto e = frame (from);
        const auto f = frame (to);

        const auto component = [] (const yup::Vector3<float>& v, int index)
        {
            return index == 0 ? v.getX() : (index == 1 ? v.getY() : v.getZ());
        };

        // R = F * transpose (E), column-major
        std::array<float, 16> m {};
        for (int column = 0; column < 3; ++column)
        {
            for (int row = 0; row < 3; ++row)
            {
                auto sum = 0.0f;
                for (std::size_t k = 0; k < 3; ++k)
                    sum += component (f[k], row) * component (e[k], column);

                m[static_cast<std::size_t> (column * 4 + row)] = sum;
            }
        }
        m[15] = 1.0f;

        return yup::Quaternion::fromRotationMatrix (yup::Matrix4 (m));
    }

    static yup::Quaternion nlerp (const yup::Quaternion& a, const yup::Quaternion& b, float t)
    {
        const auto dot = a.getX() * b.getX() + a.getY() * b.getY() + a.getZ() * b.getZ() + a.getW() * b.getW();
        const auto sign = dot < 0.0f ? -1.0f : 1.0f;

        return yup::Quaternion (a.getX() + (sign * b.getX() - a.getX()) * t,
                                a.getY() + (sign * b.getY() - a.getY()) * t,
                                a.getZ() + (sign * b.getZ() - a.getZ()) * t,
                                a.getW() + (sign * b.getW() - a.getW()) * t)
            .normalized();
    }

    //==============================================================================
    void advance (float deltaSeconds) override
    {
        const auto lift = facetLift.update (pointer, screenBounds, deltaSeconds);
        view.setLift (lift);

        const auto fold = lift * lift * (3.0f - 2.0f * lift);
        if (std::abs (fold - meshFold) > 0.001f)
        {
            meshFold = fold;
            buildMesh (fold);
        }

        // A rim of light on hits, warmer with the bass, both easing out slowly.
        lightFaces (deltaSeconds);
        rimGlow = easeToward (rimGlow, hearing.hit, deltaSeconds, 0.01f, 0.5f);
        warmth = easeToward (warmth, hearing.lowAccent, deltaSeconds, 0.02f, 0.8f);
        view.setAudio (rimGlow, warmth);

        const auto center = screenBounds.getCenter();
        if (pointer.has_value())
        {
            pointerOffset = { yup::jlimit (-1.0f, 1.0f, (pointer->getX() - center.getX()) / (0.5f * screenBounds.getWidth())),
                              yup::jlimit (-1.0f, 1.0f, (pointer->getY() - center.getY()) / (0.5f * screenBounds.getHeight())) };
        }

        const auto yaw = fold * (0.6f * pointerOffset.getX() + 0.15f * std::sin (time * 0.5f));
        const auto pitch = fold * 0.5f * pointerOffset.getY();

        if (std::abs (yaw - modelYaw) > 0.0001f || std::abs (pitch - modelPitch) > 0.0001f)
        {
            modelYaw = yaw;
            modelPitch = pitch;

            const auto solidCenter = pixelToWorld (center, 0.0f, getLocalBounds().getSize());
            view.setSurfaceTransform (surface, yup::Matrix4::translation (-solidCenter)
                                                   .followedBy (yup::Matrix4::rotationY (yaw))
                                                   .followedBy (yup::Matrix4::rotationX (pitch))
                                                   .followedBy (yup::Matrix4::translation (solidCenter)));
        }
    }

    /** Every hit of the loop lights a face in blue that fades out, the next face each time, so the
        light travels around the solid with the rhythm while nothing moves under the knobs. Faces light
        at once and let go slowly, so the light flows rather than flickers. */
    void lightFaces (float deltaSeconds)
    {
        static constexpr std::array<int, numFaces> order { 3, 10, 9, 4, 0, 7, 11, 1, 6, 5, 8, 2 };

        if (hearing.hit > lastHeardHit + 0.5f)
        {
            faceTarget[(std::size_t) order[(std::size_t) nextLitFace]] = 1.0f;
            nextLitFace = (nextLitFace + 1) % numFaces;
        }
        lastHeardHit = hearing.hit;

        // Each lit face lights at once and fades out over most of a second.
        auto anyLit = false;
        for (std::size_t i = 0; i < faceGlow.size(); ++i)
        {
            faceTarget[i] *= std::exp (-deltaSeconds / 0.2f);
            faceGlow[i] = easeToward (faceGlow[i], faceTarget[i], deltaSeconds, 0.01f, 0.6f);

            if (faceGlow[i] < 0.003f && faceTarget[i] < 0.003f)
                faceGlow[i] = faceTarget[i] = 0.0f;

            anyLit = anyLit || faceGlow[i] > 0.0f;
        }

        if (anyLit || wasLit)
            panel.setFaceGlow (faceGlow);

        wasLit = anyLit;
    }

    /** Places every face rigidly between its flat and folded pose. */
    void buildMesh (float fold)
    {
        std::vector<SurfaceVertex> vertices;
        std::vector<yup::uint32> indices;

        for (const auto& face : faces)
        {
            const auto rotation = nlerp (yup::Quaternion::identity(), face.foldRotation, fold);
            const auto center = face.flatCenter + (face.foldedCenter - face.flatCenter) * fold;
            const auto normal = rotation.rotate ({ 0.0f, 0.0f, 1.0f });
            const auto first = static_cast<yup::uint32> (vertices.size());

            for (std::size_t c = 0; c < 5; ++c)
                vertices.push_back ({ center + rotation.rotate (face.flat[c] - face.flatCenter), face.uv[c], normal });

            for (yup::uint32 k = 1; k < 4; ++k)
                indices.insert (indices.end(), { first, first + k, first + k + 1 });
        }

        view.setSurfaceMesh (surface, std::move (vertices), std::move (indices));
    }

    //==============================================================================
    SurfaceView view;
    Panel panel;
    yup::Rectangle<float> screenBounds; // where the panel appears, scaled to fit
    int surface = -1;

    std::array<Face, numFaces> faces;

    Lift facetLift;
    yup::Point<float> pointerOffset;
    float meshFold = -1.0f;
    std::array<float, numFaces> faceGlow {}, faceTarget {};
    float rimGlow = 0.0f, warmth = 0.0f;
    float lastHeardHit = 0.0f;
    int nextLitFace = 0;
    bool wasLit = false;
    float modelYaw = 0.0f;
    float modelPitch = 0.0f;

    FacetSound sound;
};
