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

#include <yup_renderer/yup_renderer.h>

#include "scene3d/MpcFaceplate.h"
#include "scene3d/NpcLogo.h"
#include "scene3d/StudioEnvironment.h"

#include <array>
#include <cmath>
#include <functional>

//==============================================================================

/**
    Loads a glTF model into a yup::Scene and shows it with a yup::SceneComponent.

    The device is seen from the top. The model has no images: its faceplate, with the labels,
    the knob scales and the caps, is painted with yup::Graphics into a GPU texture. Its screen
    shows a live Component: it is rendered to a texture used as the emissive texture of the
    screen material. Clicking the screen animates the camera onto it, with a custom Node part
    ticked by Scene::update(); from there clicks are mapped onto the LCD through a
    MeshSurfaceMapper, and clicking outside it zooms back out.
    The scene is only redrawn continuously while the camera or the light show moves. The "Scene
    tree" button shows the entity tree of the scene, and "Free movement" lets the mouse orbit,
    pan and zoom the camera, as do one-finger drags, pinches, two-finger twists, tilts and pans on touch
    screens, and trackpad pinches. "Render settings" edits, on three tabs, how it is rendered and shadowed, the angular
    size of the key light, and turns on a light show of colored point lights orbiting the device:
    64 on the full tier, 16 on the raster tier, which shades every draw with its own short list of
    lights. The key light and a small spot light over the pads cast shadows, soft by default on
    the full tier, with ambient occlusion at the scale of its knobs, a reflection probe over its
    panel and an irradiance volume for the light the device bounces onto itself; "Bake probes"
    captures them again. While shown, the panel also reports the GPU time of the frames and of
    their three most expensive passes, where the device measures them (Metal).
*/
class Scene3DDemo : public yup::Component
{
public:
    Scene3DDemo()
        : yup::Component ("Scene3DDemo")
    {
        treeToggle.setButtonText ("Scene tree");
        treeToggle.onClick = [this]
        {
            setSceneTreeVisible (treeToggle.getToggleState());
        };
        addAndMakeVisible (treeToggle);

        // The switch notifies on toggling and on release, so apply its state rather than flipping
        freeMovementSwitch.onClick = [this]
        {
            view.setFreeMovement (freeMovementSwitch.getToggleState());
        };
        addAndMakeVisible (freeMovementSwitch);

        freeMovementLabel.setText ("Free movement", yup::dontSendNotification);
        addAndMakeVisible (freeMovementLabel);

        settingsToggle.setTitle ("Render settings");
        settingsToggle.onClick = [this]
        {
            settingsPanel.setVisible (settingsToggle.getToggleState());
            resized();
        };
        addAndMakeVisible (settingsToggle);

        view.onFramePainted = [this]
        {
            settingsPanel.refresh();
        };
        addChildComponent (settingsPanel);

        // The camera can't move freely while it frames the screen
        view.onScreenZoomChanged = [this] (bool isZoomedOnScreen)
        {
            freeMovementSwitch.setEnabled (! isZoomedOnScreen);
            freeMovementLabel.setEnabled (! isZoomedOnScreen);
        };

        sceneTree.setRootItemVisible (true);
        sceneTree.onSelectionChanged = [this]
        {
            showSelectedBounds();
        };
        addChildComponent (sceneTree);

        addAndMakeVisible (view);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
        g.fillAll();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10.0f);

        // The render settings icon sits on the right; the free movement label takes what is left, up to its width
        auto toolbar = bounds.removeFromTop (30.0f);
        settingsToggle.setBounds (toolbar.removeFromRight (toolbar.getHeight()));
        toolbar.removeFromRight (8.0f);
        treeToggle.setBounds (toolbar.removeFromLeft (yup::jmin (140.0f, toolbar.getWidth() * 0.35f)));
        toolbar.removeFromLeft (16.0f);
        freeMovementSwitch.setBounds (toolbar.removeFromLeft (44.0f).reduced (0.0f, 5.0f));
        toolbar.removeFromLeft (8.0f);
        freeMovementLabel.setBounds (toolbar.removeFromLeft (yup::jmin (140.0f, toolbar.getWidth())));

        bounds.removeFromTop (6.0f);

        if (sceneTree.isVisible())
        {
            sceneTree.setBounds (bounds.removeFromLeft (260.0f));
            bounds.removeFromLeft (6.0f);
        }

        if (settingsPanel.isVisible())
        {
            settingsPanel.setBounds (bounds.removeFromRight (320.0f));
            bounds.removeFromRight (6.0f);
        }

        view.setBounds (bounds);
    }

private:
    //==============================================================================
    /** A simulated MPC LCD: a boot logo, then a mixer page with tabs and soft keys. */
    class MpcLcdScreen final : public yup::Component
        , private yup::Timer
    {
    public:
        static constexpr float screenWidth = 480.0f;
        static constexpr float screenHeight = 128.0f;

        MpcLcdScreen()
            : yup::Component ("mpcLcdScreen")
        {
            setSize (screenWidth, screenHeight);
            startTimer (2500);
        }

        /** True once after each repaint of the screen, so its texture is only replaced when it changed. */
        bool consumeRedrawn() noexcept { return std::exchange (redrawn, false); }

        void paint (yup::Graphics& g) override
        {
            redrawn = true;

            g.setFillColor (backgroundColor);
            g.fillAll();

            if (booting)
                paintLogo (g);
            else
                paintMixer (g);

            // The faint horizontal lines of the LCD matrix
            g.setFillColor (yup::Colors::white.withAlpha (0.05f));
            for (float y = 0.0f; y < screenHeight; y += 3.0f)
                g.fillRect (0.0f, y, screenWidth, 1.0f);
        }

        void mouseDown (const yup::MouseEvent& event) override
        {
            if (booting)
            {
                timerCallback();
                return;
            }

            const auto position = event.getPosition();

            if (getFieldArea().contains (position))
                fieldIndex[tab] = (fieldIndex[tab] + 1) % static_cast<int> (fieldValues[tab].size());

            for (int i = 0; i < 3; ++i)
            {
                if (getTabArea (i).contains (position))
                    tab = i;

                if (getModeArea (i).contains (position))
                    mode = i;
            }

            for (int i = 0; i < 4; ++i)
            {
                if (! getRowArea (i).contains (position))
                    continue;

                selectedRow = i;

                // On the edit page the mute and solo rows toggle
                if (mode == 2 && i == 2)
                    muted = ! muted;
                else if (mode == 2 && i == 3)
                    soloed = ! soloed;
            }

            repaint();
        }

    private:
        void timerCallback() override
        {
            stopTimer();
            booting = false;
            repaint();
        }

        //==============================================================================
        static yup::Rectangle<float> getFieldArea() { return { 132.0f, 6.0f, 124.0f, 18.0f }; }
        static yup::Rectangle<float> getFrameArea() { return { 14.0f, 28.0f, 452.0f, 70.0f }; }
        static yup::Rectangle<float> getRowArea (int row) { return { 16.0f, 31.0f + static_cast<float> (row) * 16.0f, 448.0f, 16.0f }; }
        static yup::Rectangle<float> getTabArea (int index) { return getSoftKeyArea (index); }
        static yup::Rectangle<float> getModeArea (int index) { return getSoftKeyArea (index + 3); }

        /** Six soft keys of the same size, spanning the frame. */
        static yup::Rectangle<float> getSoftKeyArea (int index)
        {
            const auto frame = getFrameArea();
            constexpr float gap = 4.0f;
            const auto width = (frame.getWidth() - gap * 5.0f) / 6.0f;
            return { frame.getX() + static_cast<float> (index) * (width + gap), 100.0f, width, 22.0f };
        }

        yup::Font getLcdFont() const
        {
            return yup::ApplicationTheme::getGlobalTheme()->getDefaultMonospaceFont().withHeight (13.0f);
        }

        yup::String getRowText (int row) const
        {
            static const std::array<std::array<const char*, 4>, 3> inserts { {
                { "Insert 1: Compressor", "Insert 2: PEQ 4-Band", "Insert 3: Off", "Insert 4: Off" },
                { "Insert 1: PEQ 4-Band", "Insert 2: Phaser 2", "Insert 3: Distortion Fuzz", "Insert 4: Reverb Large" },
                { "Insert 1: Bit Grunger", "Insert 2: Off", "Insert 3: Off", "Insert 4: Off" },
            } };

            static const std::array<const char*, 4> sends { "Send 1: Reverb Medium  -6dB", "Send 2: Delay Sync  -12dB", "Send 3: Off", "Send 4: Off" };

            if (mode == 0)
                return inserts[static_cast<size_t> (tab)][static_cast<size_t> (row)];

            if (mode == 1)
                return sends[static_cast<size_t> (row)];

            switch (row)
            {
                case 0:
                    return "Level: 100";
                case 1:
                    return "Pan: Center";
                case 2:
                    return yup::String ("Mute: ") + (muted ? "On" : "Off");
                default:
                    return yup::String ("Solo: ") + (soloed ? "On" : "Off");
            }
        }

        //==============================================================================
        void paintLogo (yup::Graphics& g)
        {
            const auto logo = createNpcLogoPath (yup::Rectangle<float> (264.0f, 70.0f).withCenter (getLocalBounds().getCenter()));

            g.setStrokeJoin (yup::StrokeJoin::Miter);
            g.setStrokeColor (inkColor);
            g.setStrokeWidth (3.0f);
            g.strokePath (logo);
        }

        void paintMixer (yup::Graphics& g)
        {
            static const std::array<const char*, 3> tabNames { "Master", "Track", "Pad" };
            static const std::array<const char*, 3> modeNames { "Insert", "Send", "Edit" };
            static const std::array<const char*, 3> fieldLabels { "Master:", "Track:", "Pad:" };

            const auto font = getLcdFont();
            const auto tabIndex = static_cast<size_t> (tab);

            // Header with the selectable field
            g.setFillColor (inkColor);
            g.fillFittedText (fieldLabels[tabIndex], font, { 40.0f, 6.0f, 86.0f, 18.0f }, yup::Justification::centerRight);

            const auto field = getFieldArea();
            g.fillRect (field);
            g.setFillColor (darkColor);
            g.fillFittedText (fieldValues[tabIndex][static_cast<size_t> (fieldIndex[tabIndex])], font, field.reduced (4.0f, 0.0f), yup::Justification::centerLeft);

            // List of the current page
            g.setStrokeColor (inkColor);
            g.setStrokeWidth (1.5f);
            g.strokeRect (getFrameArea());

            for (int row = 0; row < 4; ++row)
            {
                const auto area = getRowArea (row);
                const auto selected = row == selectedRow;

                if (selected)
                {
                    g.setFillColor (inkColor);
                    g.fillRect (area.reduced (2.0f, 1.0f));
                }

                g.setFillColor (selected ? darkColor : inkColor);
                g.fillFittedText (getRowText (row), font, area.withTrimmedLeft (24.0f), yup::Justification::centerLeft);
            }

            // Tabs: only the active one is lit, the others are just outlined
            for (int i = 0; i < 3; ++i)
            {
                const auto area = getTabArea (i);

                if (i == tab)
                {
                    g.setFillColor (inkColor);
                    g.fillRect (area);
                    g.setFillColor (darkColor);
                }
                else
                {
                    yup::Path outline;
                    outline.moveTo (area.getX(), area.getY())
                        .lineTo (area.getX(), area.getBottom())
                        .lineTo (area.getRight(), area.getBottom())
                        .lineTo (area.getRight(), area.getY());

                    g.setStrokeColor (inkColor);
                    g.strokePath (outline);
                    g.setFillColor (inkColor);
                }

                g.fillFittedText (tabNames[static_cast<size_t> (i)], font, area, yup::Justification::center);
            }

            // Soft keys: the active one is filled, the others outlined
            for (int i = 0; i < 3; ++i)
            {
                const auto area = getModeArea (i);

                if (i == mode)
                {
                    g.setFillColor (inkColor);
                    g.fillRoundedRect (area, 2.0f);
                    g.setFillColor (darkColor);
                }
                else
                {
                    g.setStrokeColor (inkColor);
                    g.strokeRoundedRect (area, 2.0f);
                    g.setFillColor (inkColor);
                }

                g.fillFittedText (modeNames[static_cast<size_t> (i)], font, area, yup::Justification::center);
            }
        }

        //==============================================================================
        const yup::Color backgroundColor { 0xff5a52ec };
        const yup::Color inkColor { 0xffd4d4ff };
        const yup::Color darkColor { 0xff3a32b8 };

        const std::array<std::vector<yup::String>, 3> fieldValues { {
            { "Stereo Out", "Out 1-2", "Out 3-4" },
            { "A01 Track 01", "A02 Track 02", "A03 Track 03", "A04 Track 04" },
            { "A01 Kick", "A02 Snare", "A03 Hat" },
        } };

        bool booting = true;
        bool redrawn = false;
        int tab = 1;
        int mode = 0;
        int selectedRow = -1;
        std::array<int, 3> fieldIndex {};
        bool muted = false;
        bool soloed = false;
    };

    //==============================================================================
    /** A camera position and orientation. */
    struct CameraPose
    {
        yup::Vector3<float> position;
        yup::Quaternion rotation;

        /** The pose of a camera at @a eye looking at @a target. */
        static CameraPose lookingAt (const yup::Vector3<float>& eye, const yup::Vector3<float>& target, const yup::Vector3<float>& up)
        {
            const auto world = yup::Matrix4::lookAt (eye, target, up).inverted();
            return { eye, yup::Quaternion::fromRotationMatrix (world) };
        }

        /** Interpolates two poses, the rotation along the shorter way round. */
        static CameraPose interpolate (const CameraPose& a, const CameraPose& b, float t)
        {
            const auto& p = a.rotation;
            const auto& q = b.rotation;
            const auto sign = (p.getX() * q.getX() + p.getY() * q.getY() + p.getZ() * q.getZ() + p.getW() * q.getW()) < 0.0f ? -1.0f : 1.0f;

            const yup::Quaternion rotation (p.getX() + (sign * q.getX() - p.getX()) * t,
                                            p.getY() + (sign * q.getY() - p.getY()) * t,
                                            p.getZ() + (sign * q.getZ() - p.getZ()) * t,
                                            p.getW() + (sign * q.getW() - p.getW()) * t);

            return { a.position + (b.position - a.position) * t, rotation.normalized() };
        }

        void applyTo (yup::Entity entity) const
        {
            auto& transform = entity.getOrAttach<yup::TransformNode>();
            transform.setPosition (position);
            transform.setRotation (rotation);
        }
    };

    //==============================================================================
    /** A part that moves its entity to a camera pose with an eased animation, ticked by Scene::update(). */
    class CameraAnimator final : public yup::Node
    {
    public:
        void animateTo (const CameraPose& target, double durationSeconds)
        {
            const auto& transform = getEntity().getOrAttach<yup::TransformNode>();
            start = { transform.getPosition(), transform.getRotation() };
            end = target;
            elapsed = 0.0;
            duration = durationSeconds;
            animating = true;
        }

        bool isAnimating() const noexcept { return animating; }

        void update (double deltaSeconds) override
        {
            if (! animating)
                return;

            // Frames can be far apart after an idle period: never jump more than a short step
            elapsed += yup::jmin (deltaSeconds, 1.0 / 30.0);

            const auto t = static_cast<float> (yup::jlimit (0.0, 1.0, elapsed / duration));
            const auto eased = t * t * (3.0f - 2.0f * t);

            CameraPose::interpolate (start, end, eased).applyTo (getEntity());
            animating = t < 1.0f;
        }

    private:
        CameraPose start;
        CameraPose end;
        double elapsed = 0.0;
        double duration = 1.0;
        bool animating = false;
    };

    //==============================================================================
    /** A node that pushes its entity in along its local -Y while pressed and lets it spring back
        on release, ticked by Scene::update().
    */
    class SimpleButtonNode final : public yup::Node
    {
    public:
        /** @param travelDistance How far the button goes in, in the units of its parent. */
        explicit SimpleButtonNode (float travelDistance)
            : travel (travelDistance)
        {
        }

        void setPressed (bool shouldBePressed) noexcept { pressed = shouldBePressed; }

        bool isPressed() const noexcept { return pressed; }

        bool isAnimating() const noexcept { return amount != (pressed ? 1.0f : 0.0f); }

        void update (double deltaSeconds) override
        {
            if (! isAnimating())
                return;

            auto& transform = getEntity().getOrAttach<yup::TransformNode>();
            if (! restPosition)
                restPosition = transform.getPosition();

            const auto step = static_cast<float> (yup::jmin (deltaSeconds, 1.0 / 30.0)) / (pressed ? pressSeconds : releaseSeconds);
            amount = pressed ? yup::jmin (1.0f, amount + step) : yup::jmax (0.0f, amount - step);

            const auto eased = amount * amount * (3.0f - 2.0f * amount);
            transform.setPosition (*restPosition + transform.getRotation().rotate ({ 0.0f, -travel * eased, 0.0f }));
        }

    private:
        static constexpr float pressSeconds = 0.05f;
        static constexpr float releaseSeconds = 0.12f;

        float travel;
        float amount = 0.0f;
        bool pressed = false;
        std::optional<yup::Vector3<float>> restPosition;
    };

    //==============================================================================
    /** A part that flies the children of its entity along elliptic orbits over a box, ticked by Scene::update().

        Child i orbits at its own height, size, speed and direction, so the lights it carries sweep
        over the whole device.
    */
    class LightShowAnimator final : public yup::Node
    {
    public:
        explicit LightShowAnimator (const yup::BoundingBox& boxToOrbit)
            : box (boxToOrbit)
        {
        }

        void update (double deltaSeconds) override
        {
            time += yup::jmin (deltaSeconds, 1.0 / 30.0);

            const auto center = box.getCenter();
            const auto halfSize = box.getSize() * 0.5f;
            const auto children = getEntity().getChildren();

            for (int i = 0; i < children.size(); ++i)
            {
                const auto index = static_cast<float> (i);
                const auto ring = static_cast<float> (i % 4);
                const auto scale = 0.3f + 0.25f * ring;
                const auto speed = (i % 2 == 0 ? 0.35f : -0.25f) * (1.0f + 0.15f * ring);
                const auto angle = index * 2.39996f + speed * static_cast<float> (time);
                const auto height = halfSize.getY() * (0.6f + 0.25f * ring);

                children[i].getOrAttach<yup::TransformNode>().setPosition ({ center.getX() + std::cos (angle) * halfSize.getX() * scale,
                                                                            center.getY() + height,
                                                                            center.getZ() + std::sin (angle) * halfSize.getZ() * scale });
            }
        }

    private:
        yup::BoundingBox box;
        double time = 0.0;
    };

    //==============================================================================
    /** The 3D view: shows the device from the top, and zooms onto its screen to operate the LCD. */
    class MpcView final : public yup::SceneComponent
    {
    public:
        MpcView()
            : yup::SceneComponent ("mpcView")
        {
            lcd.setManuallyComposited (true);
            addAndMakeVisible (lcd);

            auto scene = yup::Scene::Ptr (new yup::Scene());
            auto radius = 1.0f;

            auto model = yup::GltfModel::loadFromFile (getAssetPath ("data/gltf/mpc.glb"));
            if (model.wasOk())
            {
                for (const auto& warning : model.getReference().getWarnings())
                    yup::Logger::outputDebugString ("Scene3DDemo: " + warning);

                auto gltfRoot = model.getReference().instantiate (*scene);
                tuneMaterials (gltfRoot, coatedMaterials, sheenMaterials);

                auto device = gltfRoot.findChild ("Cube");
                if (device.isValid())
                {
                    attachScreen (device);
                    faceplate.setDevice (device);
                }

                // Center the device on the origin
                auto framed = device.isValid() ? device : gltfRoot;
                gltfRoot.attach<yup::TransformNode> (-framed.computeWorldBounds().getCenter());

                focusEntity = framed;
                deviceBounds = framed.computeWorldBounds();
                radius = yup::jmax (0.001f, deviceBounds.getRadius());

                for (auto child : framed.getChildren())
                {
                    if (! child.getName().startsWith ("Pad."))
                        continue;

                    padBounds.expand (child.computeWorldBounds());

                    // The rubber cap is a child of the pad, so it moves while the lit rim of the pad stays. It has
                    // no transform of its own: it travels along the tilted Y of the pad, in its units
                    for (auto cap : child.getChildren())
                    {
                        if (const auto* node = cap.get<yup::MeshNode>(); node != nullptr && node->mesh != nullptr && cap.getName().endsWith (" Cap"))
                            cap.attach<SimpleButtonNode> (node->mesh->getBounds().getSize().getY() * padTravel);
                    }
                }
            }
            else
            {
                status = "Unable to load mpc.glb: " + model.getErrorMessage();
            }

            cameraEntity = scene->createEntity ("camera");
            cameraNode = &cameraEntity.attach<yup::CameraNode>();
            animator = &cameraEntity.attach<CameraAnimator>();
            cameraNode->zNear = radius * 0.02f;
            cameraNode->zFar = radius * 40.0f;

            auto rootEntity = scene->getRoot();
            scene->setActiveCamera (cameraEntity);

            // Lit like a product shot: one large, low key light from the back left, its soft box in the
            // studio, rakes across the panel and casts long shadows; the dim studio fills the rest. On the
            // full tier contact shadows tighten them where the knobs and pads meet the panel
            auto keyLightEntity = scene->createEntity ("keyLight");
            auto& keyLight = keyLightEntity.attach<yup::LightNode>();
            keyLight.castsShadows = true;
            keyLight.contactShadows = true;
            keyLight.intensity = 4.0f;
            keyLight.radius = 0.08f;
            keyLight.color = { 1.0f, 0.96f, 0.9f };
            CameraPose::lookingAt (directionFromDegrees (studioKeyAzimuthDegrees, studioKeyElevationDegrees) * 10.0f, {}, { 0.0f, 1.0f, 0.0f }).applyTo (keyLightEntity);

            // A small spot light from the front right lights the pads and casts their shadows across the next row
            auto padLightEntity = scene->createEntity ("padLight");
            auto& padLight = padLightEntity.attach<yup::LightNode> (yup::LightNode::Type::spot);
            padLight.castsShadows = true;
            padLight.intensity = 10.0f;
            padLight.radius = 0.03f;
            padLight.range = 4.0f;
            padLight.innerConeAngle = 0.3f;
            padLight.outerConeAngle = 0.6f;
            padLight.color = { 0.9f, 0.94f, 1.0f };
            const auto padCenter = padBounds.getCenter();
            CameraPose::lookingAt (padCenter + directionFromDegrees (-45.0f, 45.0f) * 1.6f, padCenter, { 0.0f, 1.0f, 0.0f }).applyTo (padLightEntity);

            // Probed from above the pads, the glossy caps and the screen glass reflect the panel around
            // them instead of the studio floor. Parallax correction places the panel where it lies
            // around each knob, rather than as if far away, which bent it across the knobs' rims
            const auto deviceCenter = deviceBounds.getCenter();
            const auto deviceSize = deviceBounds.getSize();
            const auto probePosition = padBounds.isEmpty() ? deviceCenter + yup::Vector3<float> (0.0f, deviceSize.getY(), 0.0f)
                                                           : padBounds.getCenter() + yup::Vector3<float> (0.0f, padBounds.getSize().getY() * 0.5f + 0.25f, 0.0f);
            probeEntity = scene->createEntity ("reflectionProbe");
            probeEntity.attach<yup::TransformNode> (probePosition);
            reflectionProbe = &probeEntity.attach<yup::ReflectionProbeNode>();
            reflectionProbe->shape = yup::ReflectionProbeNode::Shape::box;
            reflectionProbe->influenceExtents = { std::abs (deviceCenter.getX() - probePosition.getX()) + deviceSize.getX() * 0.5f + 0.3f,
                                                  probePosition.getY() - deviceBounds.getMin().getY() + 0.1f,
                                                  std::abs (deviceCenter.getZ() - probePosition.getZ()) + deviceSize.getZ() * 0.5f + 0.3f };
            reflectionProbe->falloff = 0.2f;
            reflectionProbe->parallaxCorrection = true;

            // Bakes the light the case and the screen bounce onto the panel, from 4 x 2 x 4 probes. The probes
            // sit at the cell centers: both layers stay above the panel, the lower one just over the pads, since
            // a probe inside the body only sees its inside and lights the pads with stair-stepped seams
            const auto panelTop = padBounds.isEmpty() ? deviceCenter.getY() : padBounds.getMax().getY();
            const auto layerSpacing = 0.45f;
            const auto volumeBottom = panelTop + 0.05f - layerSpacing * 0.5f;
            const auto volumeTop = volumeBottom + layerSpacing * 2.0f;
            volumeEntity = scene->createEntity ("irradianceVolume");
            volumeEntity.attach<yup::TransformNode> (yup::Vector3<float> (deviceCenter.getX(), (volumeBottom + volumeTop) * 0.5f, deviceCenter.getZ()));
            irradianceVolume = &volumeEntity.attach<yup::IrradianceVolumeNode>();
            irradianceVolume->extents = { deviceSize.getX() * 0.5f + 0.6f, (volumeTop - volumeBottom) * 0.5f, deviceSize.getZ() * 0.5f + 0.6f };
            irradianceVolume->resolutionX = 4;
            irradianceVolume->resolutionY = 2;
            irradianceVolume->resolutionZ = 4;
            irradianceVolume->falloff = 0.15f;

            lightShow = scene->createEntity ("lightShow");
            lightShow.setVisible (false);
            lightShow.attach<LightShowAnimator> (deviceBounds);
            setNumShowLights (numShowLightsFull);

            auto& environment = rootEntity.attach<yup::EnvironmentNode> (createStudioEnvironment());
            environment.intensity = 0.35f;
            environment.visible = true;
            environment.blur = 0.5f;
            rootEntity.attach<yup::AmbientLightNode> (yup::Colors::black);
            rootEntity.attach<yup::ExposureNode> (1.1f);
            rootEntity.attach<yup::ToneMappingNode> (yup::ToneMappingOperator::agx);

            // Off until the fog toggle switches it on, the sliders shape it meanwhile
            yup::WorldFog fog;
            fog.enabled = false;
            rootEntity.attach<yup::WorldFogNode> (fog);

            // Ambient occlusion on both tiers, at the scale of the knobs: they are about 0.08 tall
            auto settings = getRenderer().getRenderSettings();
            settings.ambientOcclusion = true;
            settings.aoDistance = 0.08f;
            getRenderer().setRenderSettings (settings);

            setScene (scene);

            updateStatus();
        }

        //==============================================================================
        /** Called when the camera zooms onto the screen or back out. */
        std::function<void (bool isZoomedOnScreen)> onScreenZoomChanged;

        /** Called at the end of every paint, once the renderer holds the capabilities of the frame. */
        std::function<void()> onFramePainted;

        /** The graphics backend of the last paint. */
        yup::GpuPlatform getPlatform() const noexcept { return platform; }

        /** The camera of the view, whose depth of field fields the settings panel edits. */
        yup::CameraNode& getCamera() const noexcept { return *cameraNode; }

        /** Lets the mouse orbit, pan and zoom the camera, or brings it back to the top view.

            Ignored while zoomed onto the screen.
        */
        void setFreeMovement (bool shouldMoveFreely)
        {
            if (freeMovement == shouldMoveFreely || zoomedOnScreen)
                return;

            freeMovement = shouldMoveFreely;
            setWantedGestures (freeMovement ? (yup::GestureType::pinch | yup::GestureType::rotate | yup::GestureType::pan) : yup::GestureType::none);

            // Orbit from where the top view is, tilted just enough for a stable up direction
            if (freeMovement)
            {
                const auto top = getTopPose();
                orbitTarget = deviceBounds.getCenter();
                orbitDistance = (top.position - orbitTarget).length();
                orbitYaw = 0.0f;
                orbitPitch = maxOrbitPitch;
            }

            animator->animateTo (getTargetPose(), zoomSeconds);
            setContinuousUpdates (true);
            updateStatus();
            repaint();
        }

        /** The number of lights of the light show, shown or not. */
        int getNumShowLights() const noexcept { return lightShow.getNumChildren(); }

        /** Shows or hides the light show: small colored point lights orbiting the device. */
        void setLightShow (bool shouldShow)
        {
            lightShow.setVisible (shouldShow);

            if (shouldShow)
                setContinuousUpdates (true);

            repaint();
        }

        /** Captures the reflection probe and the irradiance volume again at the next frame.

            They only bake by themselves when attached or moved, so they keep the light show and the
            screen as they were then.
        */
        void bakeProbes()
        {
            reflectionProbe->requestBake();
            irradianceVolume->requestBake();
            repaint();
        }

        /** Shows or hides the reflection probe over the pads: without it, the glossy parts reflect the studio. */
        void setReflectionProbe (bool shouldShow)
        {
            probeEntity.setVisible (shouldShow);
            repaint();
        }

        /** Shows or hides the irradiance volume over the panel: without it, the panel takes no bounce light. */
        void setIrradianceVolume (bool shouldShow)
        {
            volumeEntity.setVisible (shouldShow);
            repaint();
        }

        /** Varnishes the knobs and the black trim, or leaves them satin. */
        void setClearcoat (bool shouldCoat)
        {
            for (const auto& material : coatedMaterials)
                material->clearcoatFactor = shouldCoat ? knobClearcoat : 0.0f;

            repaint();
        }

        /** Gives the rubber pads their velvet sheen, or leaves them plain. */
        void setSheen (bool shouldShine)
        {
            for (const auto& material : sheenMaterials)
                material->sheenColorFactor = shouldShine ? padSheenColor : std::array<float, 3> {};

            repaint();
        }

        //==============================================================================
        void resized() override
        {
            // Both poses depend on the aspect ratio: keep the camera framed while resizing
            if (! animator->isAnimating())
                getTargetPose().applyTo (cameraEntity);
        }

        /** Renders the next paint 300 times offscreen at 1920x1080 and reports the average frame time. */
        void requestBenchmark()
        {
            benchmarkPending = true;
            repaint();
        }

        void paint (yup::Graphics& g) override
        {
            auto& context = g.getGraphicsContext();
            if (context.isGpuAvailable())
            {
                platform = context.getGpuDevice()->getPlatform();
                faceplate.updateTexture (context);

                // Every new texture restarts the TAA accumulation, so only hand over actual changes
                if (lcdTexture != nullptr)
                {
                    auto texture = lcd.renderToTexture (context, lcdTextureScale);
                    if (lcd.consumeRedrawn())
                        lcdTexture->setGpuTexture (std::move (texture));
                }
            }

            if (benchmarkPending && context.isGpuAvailable() && getScene() != nullptr)
            {
                benchmarkPending = false;
                status = runBenchmark (context);
                yup::Logger::outputDebugString (status);
            }

            yup::SceneComponent::paint (g);

            // Input is mapped with the camera that was actually drawn
            updateScreenMapping();

            if (onFramePainted)
                onFramePainted();
        }

        void paintOverChildren (yup::Graphics& g) override
        {
            const auto font = yup::ApplicationTheme::getGlobalTheme()->getDefaultFont();
            auto area = getLocalBounds().reduced (10.0f).withHeight (24.0f);
            const auto numLights = countLights (getScene()->getRoot());

            g.setFillColor (yup::Colors::white.withAlpha (0.7f));
            g.fillFittedText (yup::String (numLights) + (numLights == 1 ? " light" : " lights"), font, area.removeFromRight (80.0f), yup::Justification::topRight);
            g.fillFittedText (status, font, area, yup::Justification::topLeft);
        }

        void refreshDisplay (double lastFrameTimeSeconds) override
        {
            yup::SceneComponent::refreshDisplay (lastFrameTimeSeconds);

            // The raster tier shades each draw with its own short list of lights: fewer show lights keep it fast
            if (const auto* capabilities = getRenderer().getCapabilities(); capabilities != nullptr)
            {
                const auto isFull = capabilities->getTier() == yup::RenderCapabilities::Tier::full;
                if (setNumShowLights (isFull ? numShowLightsFull : numShowLightsRaster))
                    repaint();
            }

            // Only animate while the camera, the light show or a pad moves, the scene is static otherwise
            const auto padsMoving = getScene()->view<SimpleButtonNode>()
                                        .where ([] (const SimpleButtonNode& button) { return button.isAnimating(); })
                                        .first()
                                        .isValid();

            if (! animator->isAnimating() && ! lightShow.isVisible() && ! padsMoving)
                setContinuousUpdates (false);
        }

        //==============================================================================
        void mouseDown (const yup::MouseEvent& event) override
        {
            if (event.isTouch())
                ++fingersDown;

            if (animator->isAnimating())
                return;

            // Clicks on the LCD while zoomed in reach the LCD itself, so any click here is outside it
            if (zoomedOnScreen)
            {
                zoomTo (false);
                return;
            }

            if (fingersDown <= 1)
            {
                if (auto* button = findButtonAt (event.getPosition()))
                {
                    button->setPressed (true);
                    pressedButton = *button;
                    setContinuousUpdates (true);
                    return;
                }
            }

            if (! freeMovement)
            {
                if (hitsScreen (event.getPosition()))
                    zoomTo (true);
                else
                    focusAt (event.getPosition());

                return;
            }

            // A second finger hands the camera over to the pinch, rotate and pan gestures
            if (fingersDown > 1)
            {
                dragMode = DragMode::none;
                multiTouch = true;
                return;
            }

            lastDragPosition = event.getPosition();
            dragDistance = 0.0f;
            dragMode = event.isMiddleButtonDown() ? DragMode::pan
                     : event.isRightButtonDown()  ? DragMode::zoom
                                                  : DragMode::rotate;
        }

        void mouseDrag (const yup::MouseEvent& event) override
        {
            if (dragMode == DragMode::none)
                return;

            const auto delta = event.getPosition() - lastDragPosition;
            lastDragPosition = event.getPosition();
            dragDistance += std::abs (delta.getX()) + std::abs (delta.getY());

            if (dragMode == DragMode::rotate)
            {
                orbitYaw -= delta.getX() * 0.008f;
                orbitPitch = yup::jlimit (minOrbitPitch, maxOrbitPitch, orbitPitch + delta.getY() * 0.008f);
            }
            else if (dragMode == DragMode::pan)
            {
                panOrbit (delta);
            }
            else
            {
                zoomOrbit (delta.getY() * 0.01f);
            }

            getOrbitPose().applyTo (cameraEntity);
            repaint();
        }

        void mouseUp (const yup::MouseEvent& event) override
        {
            if (event.isTouch())
                fingersDown = yup::jmax (0, fingersDown - 1);

            if (auto* button = pressedButton.get())
            {
                button->setPressed (false);
                pressedButton = {};
                setContinuousUpdates (true);
                return;
            }

            // Fingers that pinched, rotated or panned never count as a click
            if (multiTouch)
            {
                dragMode = DragMode::none;
                multiTouch = fingersDown > 0;
                return;
            }

            // A left click that didn't drag still zooms onto the screen
            const auto wasClick = dragMode == DragMode::rotate && dragDistance < 4.0f;
            dragMode = DragMode::none;

            if (! wasClick)
                return;

            if (hitsScreen (event.getPosition()))
                zoomTo (true);
            else
                focusAt (event.getPosition());
        }

        void mouseWheel (const yup::MouseEvent&, const yup::MouseWheelData& wheelData) override
        {
            if (! freeMovement || zoomedOnScreen || animator->isAnimating())
                return;

            zoomOrbit (-wheelData.getDeltaY() * 0.15f);
            getOrbitPose().applyTo (cameraEntity);
            repaint();
        }

        void gesturePinch (const yup::GestureEvent& event) override
        {
            if (! canMoveCamera() || event.getScaleDelta() <= 0.0f)
                return;

            zoomOrbit (-std::log (event.getScaleDelta()));
            getOrbitPose().applyTo (cameraEntity);
            repaint();
        }

        void gestureRotate (const yup::GestureEvent& event) override
        {
            if (! canMoveCamera())
                return;

            orbitYaw += event.getRotationDelta();
            getOrbitPose().applyTo (cameraEntity);
            repaint();
        }

        void gesturePan (const yup::GestureEvent& event) override
        {
            if (! canMoveCamera())
                return;

            // Vertical drags tilt the view toward the horizon, as in map applications
            const auto delta = event.getTranslationDelta();
            orbitPitch = yup::jlimit (minOrbitPitch, maxOrbitPitch, orbitPitch + delta.getY() * 0.008f);
            panOrbit ({ delta.getX(), 0.0f });
            getOrbitPose().applyTo (cameraEntity);
            repaint();
        }

        //==============================================================================
        std::optional<yup::Point<float>> getChildPointFromLocal (const yup::Component& child, yup::Point<float> localPoint) const override
        {
            if (&child != &lcd)
                return yup::SceneComponent::getChildPointFromLocal (child, localPoint);

            // From the top view, or while moving, the LCD isn't operable: clicks zoom instead
            if (! zoomedOnScreen || animator->isAnimating())
                return std::nullopt;

            const yup::SpinLock::ScopedLockType sl (mapperLock);

            const auto uv = screenMapper.viewportToUV (localPoint);
            if (! uv)
                return std::nullopt;

            return yup::Point<float> (uv->getX() * lcd.getWidth(), uv->getY() * lcd.getHeight());
        }

        std::optional<yup::Point<float>> getLocalPointFromChild (const yup::Component& child, yup::Point<float> childPoint) const override
        {
            if (&child != &lcd)
                return yup::SceneComponent::getLocalPointFromChild (child, childPoint);

            const yup::SpinLock::ScopedLockType sl (mapperLock);

            return screenMapper.uvToViewport ({ childPoint.getX() / lcd.getWidth(), childPoint.getY() / lcd.getHeight() });
        }

    private:
        //==============================================================================
        /** Replaces the screen glass of the device with the LCD component, and drops the modeled logo. */
        void attachScreen (yup::Entity device)
        {
            auto* meshNode = device.get<yup::MeshNode>();
            if (meshNode == nullptr || meshNode->mesh == nullptr)
                return;

            lcdTexture = new yup::Texture (yup::GpuTexture::Ptr(), yup::GpuSamplerDesc (yup::GpuFilter::linear, yup::GpuWrapMode::clampToEdge), true);

            auto lcdMaterial = yup::Material::Ptr (new yup::Material());
            lcdMaterial->name = "LCD";
            lcdMaterial->baseColorFactor = { 0.02f, 0.02f, 0.02f, 1.0f };
            // Glossy glass over the display: it reflects the environment, strongly at grazing angles
            lcdMaterial->metallicFactor = 0.0f;
            lcdMaterial->roughnessFactor = 0.05f;
            lcdMaterial->emissiveTexture = lcdTexture;
            lcdMaterial->emissiveFactor = { 1.6f, 1.6f, 1.6f };

            const auto& original = *meshNode->mesh;
            auto rebuilt = yup::Mesh::Ptr (new yup::Mesh (original.getName()));

            for (const auto& primitive : original.getPrimitives())
            {
                const auto materialName = primitive.material != nullptr ? primitive.material->name : yup::String();

                if (materialName == "Material.001")
                    continue;

                if (materialName != "Glass")
                {
                    rebuilt->addPrimitive (primitive.vertices, primitive.indices, primitive.material);
                    continue;
                }

                // The glass faces +X: seen from the front its left edge is at +Z and its top at +Y
                auto vertices = primitive.vertices;
                const auto& bounds = primitive.bounds;
                const auto center = bounds.getCenter();

                std::vector<yup::Vector3<float>> positions;
                std::vector<yup::Point<float>> uvs;
                yup::Vector3<float> top, bottom, left, right;
                int numTop = 0, numBottom = 0, numLeft = 0, numRight = 0;

                for (auto& vertex : vertices)
                {
                    const auto& p = vertex.position;

                    vertex.uv = { (bounds.getMax().getZ() - p.getZ()) / bounds.getSize().getZ(),
                                  (bounds.getMax().getY() - p.getY()) / bounds.getSize().getY() };

                    positions.push_back (p);
                    uvs.push_back (vertex.uv);

                    // Midpoints of the edges, to frame the screen with the camera
                    auto& edge = p.getY() > center.getY() ? top : bottom;
                    auto& edgeCount = p.getY() > center.getY() ? numTop : numBottom;
                    edge += p;
                    ++edgeCount;

                    if (std::abs (p.getZ() - center.getZ()) > 1.0e-4f)
                    {
                        auto& side = p.getZ() > center.getZ() ? left : right;
                        auto& sideCount = p.getZ() > center.getZ() ? numLeft : numRight;
                        side += p;
                        ++sideCount;
                    }
                }

                if (numTop == 0 || numBottom == 0 || numLeft == 0 || numRight == 0)
                    return;

                screenCenter = center;
                screenUp = top / static_cast<float> (numTop) - bottom / static_cast<float> (numBottom);
                screenRight = right / static_cast<float> (numRight) - left / static_cast<float> (numLeft);

                {
                    const yup::SpinLock::ScopedLockType sl (mapperLock);
                    screenMapper.setMesh (positions, uvs, primitive.indices);
                }

                rebuilt->addPrimitive (std::move (vertices), primitive.indices, lcdMaterial);
            }

            meshNode->mesh = rebuilt;
            screenEntity = device;
        }

        /** Makes the painted panel matte, varnishes the black knobs and trim with a sharp clearcoat over
            a satin base, and gives the rubber pads a soft velvet sheen at their edges. Collects the
            varnished and the velvet materials into @p coated and @p sheened, for setClearcoat() and setSheen().
        */
        static void tuneMaterials (yup::Entity entity, std::vector<yup::Material::Ptr>& coated, std::vector<yup::Material::Ptr>& sheened)
        {
            if (auto* meshNode = entity.get<yup::MeshNode>(); meshNode != nullptr && meshNode->mesh != nullptr)
            {
                for (const auto& primitive : meshNode->mesh->getPrimitives())
                {
                    if (primitive.material == nullptr)
                        continue;

                    auto& material = *primitive.material;

                    if (material.name == "Faceplate" || material.name == "Case")
                    {
                        material.roughnessFactor = 0.7f;
                    }
                    else if (material.name == "Knob Rubber" || material.name == "Black Plastic")
                    {
                        material.roughnessFactor = 0.45f;
                        material.clearcoatFactor = knobClearcoat;
                        material.clearcoatRoughnessFactor = 0.08f;
                        coated.push_back (primitive.material);
                    }
                    else if (material.name == "Pad Rubber")
                    {
                        material.sheenColorFactor = padSheenColor;
                        material.sheenRoughnessFactor = 0.5f;
                        sheened.push_back (primitive.material);
                    }
                }
            }

            for (const auto child : entity.getChildren())
                tuneMaterials (child, coated, sheened);
        }

        /** Rebuilds the light show with @p count lights, their hues around the color wheel, or
            returns false if it already has as many. LightShowAnimator spreads them over its orbits.
        */
        bool setNumShowLights (int count)
        {
            if (lightShow.getNumChildren() == count)
                return false;

            while (lightShow.getNumChildren() > 0)
                lightShow.getChild (0).destroy();

            for (int i = 0; i < count; ++i)
            {
                auto showLightEntity = lightShow.createChild ("showLight");
                auto& showLight = showLightEntity.attach<yup::LightNode> (yup::LightNode::Type::point);
                const auto color = yup::Color::fromHSV (static_cast<float> (i) / static_cast<float> (count), 0.85f, 1.0f);
                showLight.color = { color.getRedFloat(), color.getGreenFloat(), color.getBlueFloat() };
                showLight.intensity = 0.5f;
                showLight.range = 0.5f;
            }

            return true;
        }

        /** The number of lights in the visible part of a tree. */
        static int countLights (yup::Entity entity)
        {
            if (! entity.isVisible())
                return 0;

            auto count = entity.has<yup::LightNode>() ? 1 : 0;

            for (const auto child : entity.getChildren())
                count += countLights (child);

            return count;
        }

        //==============================================================================
        float getAspectRatio() const
        {
            const auto bounds = getLocalBounds();
            return bounds.getHeight() > 0.0f ? bounds.getWidth() / bounds.getHeight() : 1.0f;
        }

        /** The distance where an area of the given half extents fills the view. */
        float getFittingDistance (float halfWidth, float halfHeight) const
        {
            const auto tanHalfFov = std::tan (cameraNode->yFov * 0.5f);
            return yup::jmax (halfHeight / tanHalfFov, halfWidth / (tanHalfFov * getAspectRatio()));
        }

        /** Looking straight down on the device, its front (+X) at the bottom of the view.

            The body is fitted at the height of its center, where the panel is: only the screen
            rises above it, well inside the outline.
        */
        CameraPose getTopPose() const
        {
            const auto center = deviceBounds.getCenter();
            const auto size = deviceBounds.getSize();
            const auto distance = getFittingDistance (size.getZ() * 0.5f, size.getX() * 0.5f) * 1.03f;

            return CameraPose::lookingAt ({ center.getX(), center.getY() + distance, center.getZ() },
                                          center,
                                          { -1.0f, 0.0f, 0.0f });
        }

        /** Orbiting the free movement center, from the yaw, pitch and distance. */
        CameraPose getOrbitPose() const
        {
            const auto horizontal = std::cos (orbitPitch);
            const yup::Vector3<float> offset (std::cos (orbitYaw) * horizontal, std::sin (orbitPitch), -std::sin (orbitYaw) * horizontal);

            return CameraPose::lookingAt (orbitTarget + offset * orbitDistance, orbitTarget, { 0.0f, 1.0f, 0.0f });
        }

        /** A unit direction from its azimuth, from +X towards +Z, and its elevation above the horizon. */
        static yup::Vector3<float> directionFromDegrees (float azimuth, float elevation)
        {
            const auto a = yup::degreesToRadians (azimuth);
            const auto e = yup::degreesToRadians (elevation);
            return { std::cos (e) * std::cos (a), std::sin (e), std::cos (e) * std::sin (a) };
        }

        bool canMoveCamera() const
        {
            return freeMovement && ! zoomedOnScreen && ! animator->isAnimating();
        }

        /** Moves the orbit center in the view plane, at the speed of the surface under it. */
        void panOrbit (yup::Point<float> delta)
        {
            const auto pose = getOrbitPose();
            const auto forward = (orbitTarget - pose.position).normalized();
            const auto right = forward.crossProduct ({ 0.0f, 1.0f, 0.0f }).normalized();
            const auto up = right.crossProduct (forward);
            const auto unitsPerPixel = 2.0f * orbitDistance * std::tan (cameraNode->yFov * 0.5f) / yup::jmax (1.0f, getHeight());

            orbitTarget = orbitTarget - right * (delta.getX() * unitsPerPixel) + up * (delta.getY() * unitsPerPixel);
        }

        void zoomOrbit (float amount)
        {
            const auto radius = deviceBounds.getRadius();
            orbitDistance = yup::jlimit (radius * 0.3f, radius * 10.0f, orbitDistance * std::exp (amount));
        }

        /** With depth of field on, focuses the camera on what the click at @p position hits, if anything. */
        void focusAt (yup::Point<float> position)
        {
            const auto* capabilities = getRenderer().getCapabilities();
            if (capabilities == nullptr || ! capabilities->isEnabled (yup::RenderCapabilities::Feature::depthOfField, getRenderer().getRenderSettings()))
                return;

            if (const auto distance = findFocusDistance (position))
            {
                cameraNode->focusDistance = *distance;
                repaint();
            }
        }

        /** A camera ray through a point of the view. One unit of t is one unit of view depth (along -Z),
            the camera being unscaled: the t of a hit is its focus distance.
        */
        struct PickRay
        {
            std::array<float, 3> origin;
            std::array<float, 3> direction;
            float zNear = 0.0f;

            static std::array<float, 3> toArray (const yup::Vector3<float>& v) { return { v.getX(), v.getY(), v.getZ() }; }

            /** Where the ray enters @p box past the near plane; a box it starts inside is a miss. */
            std::optional<float> hit (const yup::BoundingBox& box) const
            {
                if (box.isEmpty())
                    return std::nullopt;

                const auto low = toArray (box.getMin()), high = toArray (box.getMax());
                auto enter = std::numeric_limits<float>::lowest(), leave = std::numeric_limits<float>::max();

                for (size_t axis = 0; axis < 3; ++axis)
                {
                    const auto t0 = (low[axis] - origin[axis]) / direction[axis], t1 = (high[axis] - origin[axis]) / direction[axis];
                    enter = yup::jmax (enter, yup::jmin (t0, t1));
                    leave = yup::jmin (leave, yup::jmax (t0, t1));
                }

                return enter >= zNear && enter <= leave ? std::optional (enter) : std::nullopt;
            }

            /** Where the ray enters the mesh bounds of @p entity or of its visible descendants. Each box
                is tested in its mesh's own space, where t is the same: a tilted pad's world box would
                reach over the pad next to it.
            */
            std::optional<float> hit (yup::Entity entity) const
            {
                return hit (entity, entity.getWorldMatrix());
            }

            std::optional<float> hit (yup::Entity entity, const yup::Matrix4& world) const
            {
                if (! entity.isVisible())
                    return std::nullopt;

                std::optional<float> nearest;
                if (const auto* node = entity.get<yup::MeshNode>(); node != nullptr && node->mesh != nullptr)
                {
                    const auto toLocal = world.inverted();
                    const PickRay local { toArray (toLocal.transformPoint ({ origin[0], origin[1], origin[2] })),
                                          toArray (toLocal.transformVector ({ direction[0], direction[1], direction[2] })),
                                          zNear };
                    nearest = local.hit (node->mesh->getBounds());
                }

                for (const auto child : entity.getChildren())
                    if (const auto t = hit (child, child.getLocalMatrix().followedBy (world)); t && (! nearest || *t < *nearest))
                        nearest = t;

                return nearest;
            }
        };

        std::optional<PickRay> getPickRay (yup::Point<float> position) const
        {
            if (getWidth() <= 0.0f || getHeight() <= 0.0f)
                return std::nullopt;

            const auto tanHalfFov = std::tan (cameraNode->yFov * 0.5f);
            const auto world = cameraEntity.getWorldMatrix();

            return PickRay { PickRay::toArray (world.transformPoint ({})),
                             PickRay::toArray (world.transformVector ({ (2.0f * position.getX() / getWidth() - 1.0f) * tanHalfFov * getAspectRatio(),
                                                                        (1.0f - 2.0f * position.getY() / getHeight()) * tanHalfFov,
                                                                        -1.0f })),
                             cameraNode->zNear };
        }

        /** The nearest pad whose bounds the camera ray through @p position hits, if any. */
        SimpleButtonNode* findButtonAt (yup::Point<float> position) const
        {
            const auto ray = getPickRay (position);
            if (! ray)
                return nullptr;

            SimpleButtonNode* nearest = nullptr;
            auto nearestDistance = 0.0f;

            getScene()->view<SimpleButtonNode>().forEach ([&] (yup::Entity entity, SimpleButtonNode& button)
            {
                if (! entity.isActive())
                    return;

                if (const auto t = ray->hit (entity); t && (nearest == nullptr || *t < nearestDistance))
                {
                    nearest = &button;
                    nearestDistance = *t;
                }
            });

            return nearest;
        }

        /** The view depth of the nearest child of the device the camera ray through @p position hits,
            or else of the device's own meshes (the bare panel), by a slab test of their world bounds.
        */
        std::optional<float> findFocusDistance (yup::Point<float> position) const
        {
            const auto ray = getPickRay (position);
            if (! focusEntity.isValid() || ! ray)
                return std::nullopt;

            std::optional<float> nearest;
            for (const auto child : focusEntity.getChildren())
                if (const auto t = ray->hit (child); t && (! nearest || *t < *nearest))
                    nearest = t;

            // The body only when no control was hit: its box encloses them all, so it would always win
            if (nearest || ! focusEntity.isVisible())
                return nearest;

            return ray->hit (focusEntity);
        }

        bool hitsScreen (yup::Point<float> position) const
        {
            if (! screenEntity.isValid())
                return false;

            // hitTest() and not viewportToUV(), which extrapolates beyond the screen for drags
            const yup::SpinLock::ScopedLockType sl (mapperLock);
            return screenMapper.hitTest (position).has_value();
        }

        /** Facing the screen along its normal, close enough for it to fill the view. */
        CameraPose getScreenPose() const
        {
            const auto world = screenEntity.getWorldMatrix();
            const auto center = world.transformPoint (screenCenter);
            const auto up = world.transformVector (screenUp);
            const auto right = world.transformVector (screenRight);
            const auto normal = right.crossProduct (up).normalized();

            const auto distance = getFittingDistance (right.length() * 0.5f, up.length() * 0.5f) * 1.15f;

            return CameraPose::lookingAt (center + normal * distance, center, up.normalized());
        }

        CameraPose getTargetPose() const
        {
            if (zoomedOnScreen && screenEntity.isValid())
                return getScreenPose();

            return freeMovement ? getOrbitPose() : getTopPose();
        }

        void zoomTo (bool shouldZoomOnScreen)
        {
            zoomedOnScreen = shouldZoomOnScreen;
            animator->animateTo (getTargetPose(), zoomSeconds);
            setContinuousUpdates (true);
            updateStatus();
            repaint();

            if (onScreenZoomChanged)
                onScreenZoomChanged (zoomedOnScreen);
        }

        yup::String runBenchmark (yup::GraphicsContext& context)
        {
            constexpr int numFrames = 300;
            constexpr int width = 1920;
            constexpr int height = 1080;

            yup::SceneRenderer benchmarkRenderer;
            const auto start = yup::Time::getMillisecondCounterHiRes();

            yup::GpuTexture::Ptr texture;
            for (int i = 0; i < numFrames; ++i)
                texture = benchmarkRenderer.render (context.getGpuDevice(), *getScene(), width, height);

            // Reading the last frame back waits for the GPU to finish all of them
            auto canvas = yup::GpuCanvas::create (context, width, height);
            if (texture == nullptr || canvas == nullptr)
                return "Benchmark failed: " + benchmarkRenderer.getLastError();

            auto& g = canvas->beginDraw();
            g.drawTexture (texture, { 0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height) });
            const auto image = canvas->asImage();

            const auto elapsed = yup::Time::getMillisecondCounterHiRes() - start;
            return yup::String::formatted ("Benchmark: %d frames at %dx%d, %.3f ms/frame",
                                           numFrames, width, height, elapsed / numFrames);
        }

        void updateStatus()
        {
            if (! screenEntity.isValid())
                return;

            if (zoomedOnScreen)
                status = "Click outside the screen to zoom out";
            else if (freeMovement)
                status = "Drag to rotate, middle drag to pan, right drag or wheel to zoom, click the screen to zoom in";
            else
                status = "Click the screen to zoom in";
        }

        void updateScreenMapping()
        {
            if (! screenEntity.isValid())
                return;

            const auto viewport = getLocalBounds();
            if (viewport.getWidth() <= 0.0f || viewport.getHeight() <= 0.0f)
                return;

            const auto view = cameraEntity.getWorldMatrix().inverted();
            const auto projection = cameraNode->getProjectionMatrix (getAspectRatio());

            const yup::SpinLock::ScopedLockType sl (mapperLock);
            screenMapper.setModelViewProjection (screenEntity.getWorldMatrix().followedBy (view).followedBy (projection), viewport);
        }

        //==============================================================================
        static constexpr float lcdTextureScale = 2.0f;
        static constexpr float faceplatePixelWidth = 2048.0f; // Without mip levels: larger shimmers when zoomed out
        static constexpr double zoomSeconds = 0.7;
        static constexpr float minOrbitPitch = 0.05f;
        static constexpr float maxOrbitPitch = 1.56f;
        static constexpr int numShowLightsFull = 64;
        static constexpr int numShowLightsRaster = 16;
        static constexpr float knobClearcoat = 1.0f;
        static constexpr float padTravel = 0.3f; // How far a pressed pad goes in, as a fraction of its height
        static constexpr std::array<float, 3> padSheenColor { 0.35f, 0.36f, 0.38f };

        enum class DragMode
        {
            none,
            rotate,
            pan,
            zoom
        };

        MpcFaceplate faceplate { faceplatePixelWidth };

        MpcLcdScreen lcd;
        yup::Texture::Ptr lcdTexture;

        // Shared between paint() and the input mapping, which can run on different threads
        mutable yup::SpinLock mapperLock;
        yup::MeshSurfaceMapper screenMapper;

        yup::Entity cameraEntity;
        yup::CameraNode* cameraNode = nullptr;
        CameraAnimator* animator = nullptr;

        yup::Entity lightShow;
        yup::Entity probeEntity;
        yup::ReflectionProbeNode* reflectionProbe = nullptr;
        yup::Entity volumeEntity;
        yup::IrradianceVolumeNode* irradianceVolume = nullptr;
        std::vector<yup::Material::Ptr> coatedMaterials;
        std::vector<yup::Material::Ptr> sheenMaterials;

        yup::Entity screenEntity;
        yup::Entity focusEntity; // The device, whose children, or else its body, a click focuses on
        yup::NodeHandle<SimpleButtonNode> pressedButton;
        yup::Vector3<float> screenCenter;
        yup::Vector3<float> screenUp;
        yup::Vector3<float> screenRight;
        yup::BoundingBox deviceBounds { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } };
        yup::BoundingBox padBounds;

        bool zoomedOnScreen = false;

        bool benchmarkPending = false;
        yup::GpuPlatform platform = yup::GpuPlatform::Headless;
        yup::String status;

        bool freeMovement = false;
        yup::Vector3<float> orbitTarget;
        float orbitYaw = 0.0f;
        float orbitPitch = maxOrbitPitch;
        float orbitDistance = 1.0f;

        DragMode dragMode = DragMode::none;
        yup::Point<float> lastDragPosition;
        float dragDistance = 0.0f;
        int fingersDown = 0;
        bool multiTouch = false;
    };

    //==============================================================================
    /** Edits the render settings of the scene shown by a view, and reports the tier in effect and,
        while shown, the GPU time of its frames.
    */
    /** A toggle showing the theme icon font gear, small enough for the toolbar of a phone. */
    class SettingsIconButton final : public yup::ToggleButton
    {
    public:
        using yup::ToggleButton::ToggleButton;

        void paintButton (yup::Graphics& g) override
        {
            yup::ToggleButton::paintButton (g);

            const auto theme = yup::ApplicationTheme::getGlobalTheme();
            const auto bounds = getLocalBounds().reduced (getHeight() * 0.2f);

            g.setFillColor (theme->getPalette().getColor (getToggleState() ? yup::ThemePalette::Role::onAccent : yup::ThemePalette::Role::text));
            g.fillFittedText (yup::String (yup::CharPointer_UTF8 (YUP_ICON_GEAR)),
                              theme->getDefaultIconFont().withHeight (bounds.getHeight()),
                              bounds,
                              yup::Justification::center);
        }
    };

    class RenderSettingsPanel final : public yup::Component
        , private yup::Timer
    {
    public:
        explicit RenderSettingsPanel (MpcView& viewToControl)
            : yup::Component ("renderSettingsPanel")
            , view (viewToControl)
        {
            auto* scene = view.getScene().get();
            const auto settings = view.getRenderer().getRenderSettings();

            tierLabel.setText ("Tier", yup::dontSendNotification);
            tierCombo.addItem ("Automatic", 1);
            tierCombo.addItem ("Raster", 2);
            tierCombo.addItem ("Full", 3);
            tierCombo.setSelectedId (static_cast<int> (settings.tier) + 1, yup::dontSendNotification);
            tierCombo.onSelectedItemChanged = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.tier = static_cast<yup::RenderSettings::TierPreference> (tierCombo.getSelectedId() - 1);
                });
            };

            taaLabel.setText ("TAA samples", yup::dontSendNotification);
            taaSlider.setRange (1.0, 32.0, 1.0);
            taaSlider.setNumDecimalPlacesToDisplay (0);
            taaSlider.setValue (settings.taaSamples, yup::dontSendNotification);
            taaSlider.onValueChanged = [this] (double value)
            {
                editSettings ([value] (yup::RenderSettings& s)
                {
                    s.taaSamples = static_cast<int> (value);
                });
            };

            upscalingLabel.setText ("Upscaling", yup::dontSendNotification);
            upscalingCombo.addItem ("Off", 1);
            upscalingCombo.addItem ("Temporal", 2);
            upscalingCombo.setSelectedId (static_cast<int> (settings.upscaling) + 1, yup::dontSendNotification);
            upscalingCombo.onSelectedItemChanged = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.upscaling = static_cast<yup::RenderSettings::Upscaling> (upscalingCombo.getSelectedId() - 1);
                });
            };

            renderScaleLabel.setText ("Render scale", yup::dontSendNotification);
            renderScaleSlider.setRange (yup::RenderCapabilities::minRenderScale, 1.0, 0.05);
            renderScaleSlider.setNumDecimalPlacesToDisplay (2);
            renderScaleSlider.setValue (settings.renderScale, yup::dontSendNotification);
            renderScaleSlider.onValueChanged = [this] (double value)
            {
                editSettings ([value] (yup::RenderSettings& s)
                {
                    s.renderScale = static_cast<float> (value);
                });
            };

            bloomToggle.setButtonText ("Bloom");
            bloomToggle.setToggleState (settings.bloom, yup::dontSendNotification);
            bloomToggle.onClick = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.bloom = bloomToggle.getToggleState();
                });
            };

            const auto setupBloomSlider = [this] (yup::Label& label, const char* text, yup::Slider& slider, double maximum, double value, auto field)
            {
                label.setText (text, yup::dontSendNotification);
                slider.setRange (0.0, maximum, 0.0);
                slider.setNumDecimalPlacesToDisplay (2);
                slider.setValue (value, yup::dontSendNotification);
                slider.onValueChanged = [this, field] (double newValue)
                {
                    editSettings ([field, newValue] (yup::RenderSettings& s)
                    {
                        s.*field = static_cast<float> (newValue);
                    });
                };
            };

            setupBloomSlider (bloomThresholdLabel, "Bloom threshold", bloomThresholdSlider, 4.0, settings.bloomThreshold, &yup::RenderSettings::bloomThreshold);
            setupBloomSlider (bloomIntensityLabel, "Bloom intensity", bloomIntensitySlider, 1.0, settings.bloomIntensity, &yup::RenderSettings::bloomIntensity);
            setupBloomSlider (bloomRadiusLabel, "Bloom radius", bloomRadiusSlider, 1.0, settings.bloomRadius, &yup::RenderSettings::bloomRadius);
            bloomIntensitySlider.setSkewFactorFromMidpoint (0.1);

            toneMappingLabel.setText ("Tone mapping", yup::dontSendNotification);
            toneMappingCombo.addItem ("Reinhard", 1);
            toneMappingCombo.addItem ("ACES", 2);
            toneMappingCombo.addItem ("AgX", 3);
            toneMappingCombo.setSelectedId (static_cast<int> (scene->getRoot().get<yup::ToneMappingNode>()->type) + 1, yup::dontSendNotification);
            toneMappingCombo.onSelectedItemChanged = [this]
            {
                view.getScene()->getRoot().get<yup::ToneMappingNode>()->type = static_cast<yup::ToneMappingOperator> (toneMappingCombo.getSelectedId() - 1);
                view.repaint();
            };

            exposureLabel.setText ("Exposure", yup::dontSendNotification);
            exposureSlider.setRange (0.1, 8.0, 0.0);
            exposureSlider.setSkewFactorFromMidpoint (1.0);
            exposureSlider.setNumDecimalPlacesToDisplay (2);
            exposureSlider.setValue (scene->getRoot().get<yup::ExposureNode>()->exposure, yup::dontSendNotification);
            exposureSlider.onValueChanged = [this] (double value)
            {
                view.getScene()->getRoot().get<yup::ExposureNode>()->exposure = static_cast<float> (value);
                view.repaint();
            };

            keyLightSizeLabel.setText ("Key light size", yup::dontSendNotification);
            keyLightSizeSlider.setRange (0.0, 0.3, 0.0);
            keyLightSizeSlider.setNumDecimalPlacesToDisplay (3);

            if (auto* keyLight = findKeyLight())
                keyLightSizeSlider.setValue (keyLight->radius, yup::dontSendNotification);

            keyLightSizeSlider.onValueChanged = [this] (double value)
            {
                if (auto* keyLight = findKeyLight())
                {
                    keyLight->radius = static_cast<float> (value);
                    view.repaint();
                }
            };

            cascadesLabel.setText ("Sun cascades", yup::dontSendNotification);
            cascadesCombo.addItem ("Automatic", 1);
            for (int count = 1; count <= 4; ++count)
                cascadesCombo.addItem (yup::String (count), count + 1);
            cascadesCombo.setSelectedId (settings.sunCascades + 1, yup::dontSendNotification);
            cascadesCombo.onSelectedItemChanged = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.sunCascades = cascadesCombo.getSelectedId() - 1;
                });
            };

            shadowResolutionLabel.setText ("Shadow resolution", yup::dontSendNotification);
            for (const auto resolution : { 1024, 2048, 4096 })
                shadowResolutionCombo.addItem (yup::String (resolution), resolution);
            shadowResolutionCombo.setSelectedId (settings.shadowResolution, yup::dontSendNotification);
            shadowResolutionCombo.onSelectedItemChanged = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.shadowResolution = shadowResolutionCombo.getSelectedId();
                });
            };

            // The shadow toggles show what the last frame ran with: the tier default until clicked, see refresh()
            wireframeToggle.setButtonText ("Wireframe");
            wireframeToggle.onClick = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.wireframe = wireframeToggle.getToggleState();
                });
            };

            softShadowsToggle.setButtonText ("Soft shadows");
            softShadowsToggle.onClick = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.softShadows = softShadowsToggle.getToggleState();
                });
            };

            contactShadowsToggle.setButtonText ("Contact shadows");
            contactShadowsToggle.onClick = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.contactShadows = contactShadowsToggle.getToggleState();
                });
            };

            // Like the shadow toggles, it shows what the last frame ran with, see refresh()
            ambientOcclusionToggle.setButtonText ("Ambient occlusion");
            ambientOcclusionToggle.onClick = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.ambientOcclusion = ambientOcclusionToggle.getToggleState();
                });
            };

            aoDistanceLabel.setText ("AO distance", yup::dontSendNotification);
            aoDistanceSlider.setRange (0.01, 0.4, 0.0);
            aoDistanceSlider.setSkewFactorFromMidpoint (0.1);
            aoDistanceSlider.setNumDecimalPlacesToDisplay (3);
            aoDistanceSlider.setValue (settings.aoDistance, yup::dontSendNotification);
            aoDistanceSlider.onValueChanged = [this] (double value)
            {
                editSettings ([value] (yup::RenderSettings& s)
                {
                    s.aoDistance = static_cast<float> (value);
                });
            };

            // Like the shadow toggles, they show what the last frame ran with, see refresh()
            screenSpaceReflectionsToggle.setButtonText ("Reflections (SSR)");
            screenSpaceReflectionsToggle.onClick = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.screenSpaceReflections = screenSpaceReflectionsToggle.getToggleState();
                });
            };

            screenSpaceGIToggle.setButtonText ("Diffuse GI (SSGI)");
            screenSpaceGIToggle.onClick = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.screenSpaceGI = screenSpaceGIToggle.getToggleState();
                });
            };

            debugViewLabel.setText ("Debug view", yup::dontSendNotification);
            debugViewCombo.addItem ("None", 1);
            debugViewCombo.addItem ("Ambient occlusion", 2);
            debugViewCombo.addItem ("Reflections", 3);
            debugViewCombo.addItem ("Indirect diffuse (SSGI)", 4);
            debugViewCombo.addItem ("Circle of confusion", 5);
            debugViewCombo.addItem ("Depth", 6);
            debugViewCombo.addItem ("Normals", 7);
            debugViewCombo.addItem ("Motion vectors", 8);
            debugViewCombo.addItem ("Base color (albedo)", 9);
            debugViewCombo.addItem ("Roughness", 10);
            debugViewCombo.addItem ("Metallic", 11);
            debugViewCombo.setSelectedId (static_cast<int> (settings.debugView) + 1, yup::dontSendNotification);
            debugViewCombo.onSelectedItemChanged = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.debugView = static_cast<yup::RenderSettings::DebugView> (debugViewCombo.getSelectedId() - 1);
                });
            };

            // Depth of field applies after the accumulation: every change repaints, nothing restarts
            depthOfFieldToggle.setButtonText ("Depth of field");
            depthOfFieldToggle.setToggleState (settings.depthOfField, yup::dontSendNotification);
            depthOfFieldToggle.onClick = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.depthOfField = depthOfFieldToggle.getToggleState();
                });
            };

            const auto setupCameraSlider = [this] (yup::Label& label, const char* text, yup::Slider& slider, double minimum, double maximum, int decimals, float yup::CameraNode::*field)
            {
                label.setText (text, yup::dontSendNotification);
                slider.setRange (minimum, maximum, 0.0);
                slider.setSkewFactorFromMidpoint (std::sqrt (minimum * maximum));
                slider.setNumDecimalPlacesToDisplay (decimals);
                slider.setValue (view.getCamera().*field, yup::dontSendNotification);
                slider.onValueChanged = [this, field] (double value)
                {
                    view.getCamera().*field = static_cast<float> (value);
                    view.repaint();
                };
            };

            setupCameraSlider (fStopLabel, "f-stop", fStopSlider, 0.7, 16.0, 1, &yup::CameraNode::fStop);
            setupCameraSlider (focusDistanceLabel, "Focus distance", focusDistanceSlider, 0.1, 10.0, 2, &yup::CameraNode::focusDistance);

            // The ids are the blade counts plus one: round is 0 blades
            bladesLabel.setText ("Blades", yup::dontSendNotification);
            for (const auto blades : { 0, 5, 6, 8 })
                bladesCombo.addItem (blades == 0 ? yup::String ("Round") : yup::String (blades), blades + 1);
            bladesCombo.setSelectedId (view.getCamera().apertureBlades + 1, yup::dontSendNotification);
            bladesCombo.onSelectedItemChanged = [this]
            {
                view.getCamera().apertureBlades = bladesCombo.getSelectedId() - 1;
                view.repaint();
            };

            // Motion blur applies after the accumulation too, and only to frames where something moved
            motionBlurToggle.setButtonText ("Motion blur");
            motionBlurToggle.setToggleState (settings.motionBlur, yup::dontSendNotification);
            motionBlurToggle.onClick = [this]
            {
                editSettings ([this] (yup::RenderSettings& s)
                {
                    s.motionBlur = motionBlurToggle.getToggleState();
                });
            };

            shutterLabel.setText ("Shutter", yup::dontSendNotification);
            shutterSlider.setRange (0.0, 1.0, 0.0);
            shutterSlider.setNumDecimalPlacesToDisplay (2);
            shutterSlider.setValue (settings.motionBlurShutter, yup::dontSendNotification);
            shutterSlider.onValueChanged = [this] (double value)
            {
                editSettings ([value] (yup::RenderSettings& s)
                {
                    s.motionBlurShutter = static_cast<float> (value);
                });
            };

            lightShowToggle.setButtonText (getLightShowText());
            lightShowToggle.onClick = [this]
            {
                view.setLightShow (lightShowToggle.getToggleState());
            };

            // The scene's local indirect light and material layers, on both tiers, to see what each adds
            const auto setupSceneToggle = [this] (const char* text, yup::ToggleButton& toggle, void (MpcView::*apply) (bool))
            {
                toggle.setButtonText (text);
                toggle.setToggleState (true, yup::dontSendNotification);
                toggle.onClick = [this, &toggle, apply]
                {
                    (view.*apply) (toggle.getToggleState());
                };
            };

            setupSceneToggle ("Reflection probe", reflectionProbeToggle, &MpcView::setReflectionProbe);
            setupSceneToggle ("Irradiance volume", irradianceVolumeToggle, &MpcView::setIrradianceVolume);
            setupSceneToggle ("Clearcoat", clearcoatToggle, &MpcView::setClearcoat);
            setupSceneToggle ("Sheen", sheenToggle, &MpcView::setSheen);

            // The scene's world fog, in its default white: the device is about 3.6 units long, seen from a few
            // units away, so a density of 0.08 leaves about two thirds of its light
            fogToggle.setButtonText ("Fog");
            fogToggle.setToggleState (scene->getRoot().get<yup::WorldFogNode>()->fog.enabled, yup::dontSendNotification);
            fogToggle.onClick = [this]
            {
                editFog ([this] (yup::WorldFog& fog)
                {
                    fog.enabled = fogToggle.getToggleState();
                });
            };

            const auto setupFogSlider = [this] (yup::Label& label, const char* text, yup::Slider& slider, double maximum, double midpoint, double value, float yup::WorldFog::*field)
            {
                label.setText (text, yup::dontSendNotification);
                slider.setRange (0.0, maximum, 0.0);
                slider.setSkewFactorFromMidpoint (midpoint);
                slider.setNumDecimalPlacesToDisplay (3);
                slider.setValue (value, yup::dontSendNotification);
                slider.onValueChanged = [this, field] (double newValue)
                {
                    editFog ([field, newValue] (yup::WorldFog& fog)
                    {
                        fog.*field = static_cast<float> (newValue);
                    });
                };
            };

            const auto initialFog = scene->getRoot().get<yup::WorldFogNode>()->fog;
            setupFogSlider (fogDensityLabel, "Fog density", fogDensitySlider, 0.5, 0.08, initialFog.enabled ? initialFog.density : 0.08, &yup::WorldFog::density);
            setupFogSlider (fogFalloffLabel, "Height falloff", fogFalloffSlider, 4.0, 1.0, initialFog.heightFalloff, &yup::WorldFog::heightFalloff);

            // Three pages of the settings by topic, then the status, always shown below them
            for (auto [label, control, value] : { std::tuple<yup::Label*, yup::Component*, yup::Label*> { &tierLabel, &tierCombo, nullptr },
                                                  { &taaLabel, &taaSlider, &taaValue },
                                                  { &upscalingLabel, &upscalingCombo, nullptr },
                                                  { &renderScaleLabel, &renderScaleSlider, &renderScaleValue },
                                                  { &toneMappingLabel, &toneMappingCombo, nullptr },
                                                  { &exposureLabel, &exposureSlider, &exposureValue },
                                                  { &keyLightSizeLabel, &keyLightSizeSlider, &keyLightSizeValue },
                                                  { &cascadesLabel, &cascadesCombo, nullptr },
                                                  { &shadowResolutionLabel, &shadowResolutionCombo, nullptr },
                                                  { &debugViewLabel, &debugViewCombo, nullptr } })
                qualityPage.addValue (*label, *control, value);

            qualityPage.addSwitches ({ &softShadowsToggle, &contactShadowsToggle, &wireframeToggle });

            benchmarkButton.onClick = [this]
            {
                view.requestBenchmark();
            };

            bakeProbesButton.onClick = [this]
            {
                view.bakeProbes();
            };

            qualityPage.addSwitches ({ &benchmarkButton, &bakeProbesButton });

            for (auto [label, control, value] : { std::tuple<yup::Label*, yup::Component*, yup::Label*> { &bloomThresholdLabel, &bloomThresholdSlider, &bloomThresholdValue },
                                                  { &bloomIntensityLabel, &bloomIntensitySlider, &bloomIntensityValue },
                                                  { &bloomRadiusLabel, &bloomRadiusSlider, &bloomRadiusValue },
                                                  { &aoDistanceLabel, &aoDistanceSlider, &aoDistanceValue },
                                                  { &fStopLabel, &fStopSlider, &fStopValue },
                                                  { &focusDistanceLabel, &focusDistanceSlider, &focusDistanceValue },
                                                  { &bladesLabel, &bladesCombo, nullptr },
                                                  { &shutterLabel, &shutterSlider, &shutterValue } })
                effectsPage.addValue (*label, *control, value);

            effectsPage.addSwitches ({ &bloomToggle, &ambientOcclusionToggle, &screenSpaceGIToggle, &screenSpaceReflectionsToggle, &depthOfFieldToggle, &motionBlurToggle, &lightShowToggle });
            for (auto [label, control, value] : { std::tuple<yup::Label*, yup::Component*, yup::Label*> { &fogDensityLabel, &fogDensitySlider, &fogDensityValue },
                                                  { &fogFalloffLabel, &fogFalloffSlider, &fogFalloffValue } })
                scenePage.addValue (*label, *control, value);

            scenePage.addSwitches ({ &reflectionProbeToggle, &irradianceVolumeToggle, &clearcoatToggle, &sheenToggle, &fogToggle });

            tabs.addTab ("quality", "Quality", qualityPage);
            tabs.addTab ("effects", "Effects", effectsPage);
            tabs.addTab ("scene", "Scene", scenePage);
            addAndMakeVisible (tabs);

            footerGrid.templateColumns.addArray (yup::Grid::repeat (2, yup::Grid::TrackInfo::fr (1.0f)));
            footerGrid.autoRows = rowHeight;
            footerGrid.gap = rowGap;

            for (auto* label : { &hintLabel, &statusLabel, &detailLabel, &gpuTimeLabel, &gpuPassesLabel })
                label->setJustification (yup::Justification::centerLeft);

            footerGrid.items.add (yup::GridItem (hintLabel).withColumnSpan (2));
            footerGrid.items.add (yup::GridItem (statusLabel));
            footerGrid.items.add (yup::GridItem (detailLabel));
            footerGrid.items.add (yup::GridItem (gpuTimeLabel).withColumnSpan (2));
            footerGrid.items.add (yup::GridItem (gpuPassesLabel).withColumnSpan (2));

            for (auto& item : footerGrid.items)
                addAndMakeVisible (*item.associatedComponent);
        }

        /** Updates the status and the enabled state of the controls from what the last frame ran with. */
        void refresh()
        {
            const auto* capabilities = view.getRenderer().getCapabilities();
            if (capabilities == nullptr)
                return;

            const auto settings = view.getRenderer().getRenderSettings();
            const auto isFull = capabilities->getTier() == yup::RenderCapabilities::Tier::full;
            const auto canTaa = capabilities->isAvailable (yup::RenderCapabilities::Feature::temporalAntiAliasing);

            const auto detail = isFull ? (view.getRenderer().isConverged() ? yup::String ("TAA converged") : yup::String ("TAA accumulating"))
                                       : "MSAA " + yup::String (view.getRenderer().getSampleCount()) + "x";

            setLabelText (statusLabel, yup::String (isFull ? "Full, " : "Raster, ") + getPlatformName (view.getPlatform()));
            setLabelText (detailLabel, detail);

            if (lightShowToggle.getButtonText() != getLightShowText())
                lightShowToggle.setButtonText (getLightShowText());

            const auto canBloom = capabilities->isAvailable (yup::RenderCapabilities::Feature::bloom);
            const auto canContactShadows = capabilities->isAvailable (yup::RenderCapabilities::Feature::contactShadows);
            const auto canAmbientOcclusion = capabilities->isAvailable (yup::RenderCapabilities::Feature::ambientOcclusion);
            const auto canDepthOfField = capabilities->isAvailable (yup::RenderCapabilities::Feature::depthOfField);
            const auto canMotionBlur = capabilities->isAvailable (yup::RenderCapabilities::Feature::motionBlur);
            const auto canFog = capabilities->isAvailable (yup::RenderCapabilities::Feature::worldFog);
            const auto ambientOcclusion = capabilities->isEnabled (yup::RenderCapabilities::Feature::ambientOcclusion, settings);
            const auto taa = capabilities->isEnabled (yup::RenderCapabilities::Feature::temporalAntiAliasing, settings);

            // The screen-space effects read the TAA history of the full tier, and the GI extends the ambient occlusion
            const auto canReflections = isFull && taa;
            const auto canGI = canReflections && ambientOcclusion;

            // A click on the view focuses the camera: the slider follows, within its range
            const auto focusRange = focusDistanceSlider.getRange();
            const auto focusDistance = yup::jlimit (focusRange.start, focusRange.end, static_cast<double> (view.getCamera().focusDistance));
            if (static_cast<float> (focusDistanceSlider.getValue()) != static_cast<float> (focusDistance))
                focusDistanceSlider.setValue (focusDistance, yup::dontSendNotification);

            for (auto [slider, value] : { std::pair<yup::Slider*, yup::Label*> { &taaSlider, &taaValue }, { &renderScaleSlider, &renderScaleValue }, { &exposureSlider, &exposureValue }, { &bloomThresholdSlider, &bloomThresholdValue },
                                          { &bloomIntensitySlider, &bloomIntensityValue }, { &bloomRadiusSlider, &bloomRadiusValue }, { &keyLightSizeSlider, &keyLightSizeValue },
                                          { &aoDistanceSlider, &aoDistanceValue }, { &fStopSlider, &fStopValue }, { &focusDistanceSlider, &focusDistanceValue },
                                          { &shutterSlider, &shutterValue }, { &fogDensitySlider, &fogDensityValue }, { &fogFalloffSlider, &fogFalloffValue } })
                setLabelText (*value, yup::String (slider->getValue(), slider->getNumDecimalPlacesToDisplay()));

            for (auto* control : std::initializer_list<yup::Component*> { &taaLabel, &taaSlider, &taaValue })
                setAvailable (*control, canTaa);

            // Upscaling reconstructs the frame with the TAA of the full tier
            const auto canUpscale = isFull && taa;
            for (auto* control : std::initializer_list<yup::Component*> { &upscalingLabel, &upscalingCombo })
                setAvailable (*control, canUpscale);

            for (auto* control : std::initializer_list<yup::Component*> { &renderScaleLabel, &renderScaleSlider, &renderScaleValue })
                setAvailable (*control, canUpscale && settings.upscaling != yup::RenderSettings::Upscaling::off);

            for (auto* control : std::initializer_list<yup::Component*> { &bloomThresholdLabel, &bloomThresholdSlider, &bloomThresholdValue, &bloomIntensityLabel, &bloomIntensitySlider,
                                                                          &bloomIntensityValue, &bloomRadiusLabel, &bloomRadiusSlider, &bloomRadiusValue })
                setAvailable (*control, canBloom);

            for (auto* control : std::initializer_list<yup::Component*> { &aoDistanceLabel, &aoDistanceSlider, &aoDistanceValue })
                setAvailable (*control, canAmbientOcclusion);

            for (auto* control : std::initializer_list<yup::Component*> { &fStopLabel, &fStopSlider, &fStopValue, &focusDistanceLabel, &focusDistanceSlider, &focusDistanceValue, &bladesLabel, &bladesCombo })
                setAvailable (*control, canDepthOfField);

            for (auto* control : std::initializer_list<yup::Component*> { &shutterLabel, &shutterSlider, &shutterValue })
                setAvailable (*control, canMotionBlur);

            for (auto* control : std::initializer_list<yup::Component*> { &fogDensityLabel, &fogDensitySlider, &fogDensityValue, &fogFalloffLabel, &fogFalloffSlider, &fogFalloffValue })
                setAvailable (*control, canFog);

            for (auto* control : std::initializer_list<yup::Component*> { &debugViewLabel, &debugViewCombo })
                setAvailable (*control, isFull);

            refreshToggle (bloomToggle, settings.bloom, canBloom);
            refreshToggle (wireframeToggle, settings.wireframe, true);
            refreshToggle (softShadowsToggle, capabilities->isEnabled (yup::RenderCapabilities::Feature::softShadows, settings), true);
            refreshToggle (contactShadowsToggle, capabilities->isEnabled (yup::RenderCapabilities::Feature::contactShadows, settings), canContactShadows);
            refreshToggle (ambientOcclusionToggle, ambientOcclusion, canAmbientOcclusion);
            refreshToggle (screenSpaceReflectionsToggle, capabilities->isEnabled (yup::RenderCapabilities::Feature::screenSpaceReflections, settings), canReflections);
            refreshToggle (screenSpaceGIToggle, capabilities->isEnabled (yup::RenderCapabilities::Feature::screenSpaceGI, settings), canGI);
            refreshToggle (depthOfFieldToggle, capabilities->isEnabled (yup::RenderCapabilities::Feature::depthOfField, settings), canDepthOfField);
            refreshToggle (motionBlurToggle, capabilities->isEnabled (yup::RenderCapabilities::Feature::motionBlur, settings), canMotionBlur);
            const auto& fog = view.getScene()->getRoot().get<yup::WorldFogNode>()->fog;
            refreshToggle (fogToggle, fog.enabled && fog.density > 0.0f && canFog, canFog);

            // One line says why the dimmed controls are off, so no label changes its text or size
            // Raster bloom, DoF and fog need multisampled half floats ("f16"), raster AO, DoF and fog the
            // prepass, and motion blur (MB) the motion of the full tier, which the limited lines leave out
            // since raster never has it; the lines stay within the panel's width at the default font
            // (the longest is 288.8 of 300 pixels), so none shrinks
            auto hint = yup::String();
            if (! isFull && ! canBloom)
                hint = "No f16: bloom, DoF, fog; full: TAA, SSR, GI, debug";
            else if (! isFull && ! canDepthOfField)
                hint = "No prepass: AO, DoF, fog; full: TAA, SSR, GI, debug";
            else if (! isFull)
                hint = "TAA, MB, SSR, SSGI, contact shadows, debug: full";
            else if (! taa)
                hint = "SSR, SSGI, upscaling need TAA samples above 1";
            else if (settings.debugView == yup::RenderSettings::DebugView::circleOfConfusion && ! settings.depthOfField)
                hint = "The CoC debug view needs depth of field on";
            else if (! ambientOcclusion)
                hint = "SSGI needs ambient occlusion";

            setLabelText (hintLabel, hint);
        }

        void paint (yup::Graphics& g) override
        {
            g.setFillColor (yup::ApplicationTheme::getGlobalTheme()->getPalette().getColor (yup::ThemePalette::Role::background));
            g.fillAll();
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (10.0f);

            footerGrid.performLayout (bounds.removeFromBottom (getRowsHeight (numFooterRows)));
            bounds.removeFromBottom (sectionGap);
            tabs.setBounds (bounds);
        }

        /** The renderer collects the GPU times of its frames only while the panel shows them. */
        void visibilityChanged() override
        {
            view.getRenderer().setFrameStatsEnabled (isVisible());

            if (isVisible())
                startTimerHz (2);
            else
                stopTimer();
        }

    private:
        /** A page of settings: the values with their labels in one column, the switches two per row
            below them. The sliders show their value in a third column, which the combo boxes span,
            so both end at the same edge. */
        class SettingsPage final : public yup::Component
        {
        public:
            SettingsPage()
            {
                setOpaque (false);

                valuesGrid.templateColumns.add (yup::Grid::TrackInfo::px (labelWidth));
                valuesGrid.templateColumns.add (yup::Grid::TrackInfo::fr (1.0f));
                valuesGrid.templateColumns.add (yup::Grid::TrackInfo::px (valueWidth));
                switchesGrid.templateColumns.addArray (yup::Grid::repeat (2, yup::Grid::TrackInfo::fr (1.0f)));

                for (auto* grid : { &valuesGrid, &switchesGrid })
                {
                    grid->autoRows = rowHeight;
                    grid->gap = rowGap;
                }
            }

            /** Adds a row: @p label, then @p control with @p value after it, or spanning both without one. */
            void addValue (yup::Label& label, yup::Component& control, yup::Label* value)
            {
                label.setJustification (yup::Justification::centerLeft);
                add (valuesGrid, yup::GridItem (label));
                ++numValueRows;

                if (value == nullptr)
                {
                    add (valuesGrid, yup::GridItem (control).withColumnSpan (2));
                    return;
                }

                value->setJustification (yup::Justification::centerRight);
                add (valuesGrid, yup::GridItem (control));
                add (valuesGrid, yup::GridItem (*value));
            }

            void addSwitches (std::initializer_list<yup::Component*> switches)
            {
                for (auto* child : switches)
                    add (switchesGrid, yup::GridItem (*child));
            }

            void resized() override
            {
                auto bounds = getLocalBounds();

                for (auto [grid, rows] : { std::pair { &valuesGrid, numValueRows }, std::pair { &switchesGrid, (switchesGrid.items.size() + 1) / 2 } })
                {
                    if (rows == 0)
                        continue;

                    bounds.removeFromTop (sectionGap);
                    grid->performLayout (bounds.removeFromTop (getRowsHeight (rows)));
                }
            }

        private:
            void add (yup::Grid& grid, yup::GridItem item)
            {
                addAndMakeVisible (*item.associatedComponent);
                grid.items.add (std::move (item));
            }

            yup::Grid valuesGrid;
            yup::Grid switchesGrid;
            int numValueRows = 0;
        };

        /** The height of @p rows rows of a grid. */
        static float getRowsHeight (int rows)
        {
            return static_cast<float> (rows) * (rowHeight + rowGap) - rowGap;
        }

        void timerCallback() override
        {
            const auto& stats = view.getRenderer().getLastFrameStats();
            setLabelText (gpuTimeLabel, getGpuTimeText (stats));
            setLabelText (gpuPassesLabel, getGpuPassesText (stats));
        }

        /** The GPU time of a frame and its size: "GPU 8.42 ms at 2048 x 1324". */
        static yup::String getGpuTimeText (const yup::SceneRenderer::FrameStats& stats)
        {
            if (stats.frameIndex == 0)
                return "GPU n/a";

            return yup::String ("GPU ") + (stats.gpu.available ? yup::String (stats.gpu.frameMilliseconds, 2) + " ms" : yup::String ("n/a"))
                 + " at " + yup::String (stats.internalWidth) + " x " + yup::String (stats.internalHeight);
        }

        /** The three passes of a frame adding the most to its GPU time: "lighting 3.1, G-buffer 2.0, AO 1.2". */
        static yup::String getGpuPassesText (const yup::SceneRenderer::FrameStats& stats)
        {
            std::vector<const yup::GpuScopeTiming*> scopes;
            for (const auto& scope : stats.gpu.scopes)
                if (scope.available)
                    scopes.push_back (&scope);

            const auto count = std::min<size_t> (scopes.size(), 3);
            std::partial_sort (scopes.begin(), scopes.begin() + static_cast<std::ptrdiff_t> (count), scopes.end(), [] (const auto* a, const auto* b)
            {
                return a->milliseconds > b->milliseconds;
            });

            yup::String text;
            for (size_t i = 0; i < count; ++i)
                text << (i == 0 ? "" : ", ") << getPassName (scopes[i]->name) << " " << yup::String (scopes[i]->milliseconds, 1);

            return text;
        }

        /** The name shown for the GPU scope of a pass (see yup::SceneRenderer::setFrameStatsEnabled()),
            short enough for the three longest to fit one row of the panel unshrunk. */
        static yup::String getPassName (const yup::String& scopeName)
        {
            static const std::pair<const char*, const char*> shortNames[] = {
                { "probeBake", "probe bake" }, { "lightCulling", "light culling" }, { "gbuffer", "G-buffer" }, { "ambientOcclusion", "AO" },
                { "hiZ", "Hi-Z" }, { "screenSpaceReflections", "SSR" }, { "indirectDiffuse", "GI add" }, { "depthNormalPrepass", "prepass" },
                { "temporalAA", "TAA" }, { "depthOfField", "DoF" }, { "motionBlur", "MB" }, { "toneMapping", "tone map" }, { "debugView", "debug" }
            };

            for (const auto& [name, shortName] : shortNames)
                if (scopeName == name)
                    return shortName;

            return scopeName;
        }

        /** The light the scene setup names "keyLight". */
        yup::LightNode* findKeyLight() const
        {
            return view.getScene()->getRoot().findChild ("keyLight").get<yup::LightNode>();
        }

        template <class Callback>
        void editSettings (Callback&& edit)
        {
            auto settings = view.getRenderer().getRenderSettings();
            edit (settings);
            view.getRenderer().setRenderSettings (settings);
            view.repaint();
        }

        /** Edits the scene's world fog, which the sliders shape even while it is off. */
        template <class Callback>
        void editFog (Callback&& edit)
        {
            auto& fog = view.getScene()->getRoot().get<yup::WorldFogNode>()->fog;
            fog.density = static_cast<float> (fogDensitySlider.getValue());
            fog.heightFalloff = static_cast<float> (fogFalloffSlider.getValue());
            edit (fog);
            view.repaint();
        }

        /** Shows whether a feature ran, dimmed and disabled when @p isAvailable is false. */
        static void refreshToggle (yup::ToggleButton& toggle, bool isOn, bool isAvailable)
        {
            if (toggle.getToggleState() != isOn)
                toggle.setToggleState (isOn, yup::dontSendNotification);

            setAvailable (toggle, isAvailable);
        }

        /** Enables @p component, or disables and dims it: the theme draws disabled buttons and labels as enabled ones. */
        static void setAvailable (yup::Component& component, bool isAvailable)
        {
            if (component.isEnabled() == isAvailable)
                return;

            component.setEnabled (isAvailable);
            component.setOpacity (isAvailable ? 1.0f : 0.4f);
        }

        yup::String getLightShowText() const
        {
            return "Light show (" + yup::String (view.getNumShowLights()) + ")";
        }

        static void setLabelText (yup::Label& label, const yup::String& text)
        {
            if (label.getText() != text)
                label.setText (text, yup::dontSendNotification);
        }

        static yup::String getPlatformName (yup::GpuPlatform platform)
        {
            switch (platform)
            {
                case yup::GpuPlatform::OpenGL:   return "OpenGL";
                case yup::GpuPlatform::OpenGLES: return "OpenGL ES";
                case yup::GpuPlatform::Direct3D: return "Direct3D";
                case yup::GpuPlatform::Metal:    return "Metal";
                case yup::GpuPlatform::WebGPU:   return "WebGPU";
                case yup::GpuPlatform::Vulkan:   return "Vulkan";
                default:                         return "Headless";
            }
        }

        MpcView& view;

        yup::Label tierLabel { "tierLabel" };
        yup::ComboBox tierCombo { "tierCombo" };
        yup::Label taaLabel { "taaLabel" };
        yup::Slider taaSlider { yup::Slider::LinearHorizontal, "taaSlider" };
        yup::Label upscalingLabel { "upscalingLabel" };
        yup::ComboBox upscalingCombo { "upscalingCombo" };
        yup::Label renderScaleLabel { "renderScaleLabel" };
        yup::Slider renderScaleSlider { yup::Slider::LinearHorizontal, "renderScaleSlider" };
        yup::ToggleButton bloomToggle { "bloomToggle" };
        yup::Label bloomThresholdLabel { "bloomThresholdLabel" };
        yup::Slider bloomThresholdSlider { yup::Slider::LinearHorizontal, "bloomThresholdSlider" };
        yup::Label bloomIntensityLabel { "bloomIntensityLabel" };
        yup::Slider bloomIntensitySlider { yup::Slider::LinearHorizontal, "bloomIntensitySlider" };
        yup::Label bloomRadiusLabel { "bloomRadiusLabel" };
        yup::Slider bloomRadiusSlider { yup::Slider::LinearHorizontal, "bloomRadiusSlider" };
        yup::Label toneMappingLabel { "toneMappingLabel" };
        yup::ComboBox toneMappingCombo { "toneMappingCombo" };
        yup::Label exposureLabel { "exposureLabel" };
        yup::Slider exposureSlider { yup::Slider::LinearHorizontal, "exposureSlider" };
        yup::Label keyLightSizeLabel { "keyLightSizeLabel" };
        yup::Slider keyLightSizeSlider { yup::Slider::LinearHorizontal, "keyLightSizeSlider" };
        yup::ToggleButton lightShowToggle { "lightShowToggle" };
        yup::Label cascadesLabel { "cascadesLabel" };
        yup::ComboBox cascadesCombo { "cascadesCombo" };
        yup::Label shadowResolutionLabel { "shadowResolutionLabel" };
        yup::ComboBox shadowResolutionCombo { "shadowResolutionCombo" };
        yup::ToggleButton softShadowsToggle { "softShadowsToggle" };
        yup::ToggleButton wireframeToggle { "wireframeToggle" };
        yup::ToggleButton contactShadowsToggle { "contactShadowsToggle" };
        yup::ToggleButton ambientOcclusionToggle { "ambientOcclusionToggle" };
        yup::Label aoDistanceLabel { "aoDistanceLabel" };
        yup::Slider aoDistanceSlider { yup::Slider::LinearHorizontal, "aoDistanceSlider" };
        yup::ToggleButton screenSpaceReflectionsToggle { "screenSpaceReflectionsToggle" };
        yup::ToggleButton screenSpaceGIToggle { "screenSpaceGIToggle" };
        yup::ToggleButton depthOfFieldToggle { "depthOfFieldToggle" };
        yup::Label fStopLabel { "fStopLabel" };
        yup::Slider fStopSlider { yup::Slider::LinearHorizontal, "fStopSlider" };
        yup::Label focusDistanceLabel { "focusDistanceLabel" };
        yup::Slider focusDistanceSlider { yup::Slider::LinearHorizontal, "focusDistanceSlider" };
        yup::Label bladesLabel { "bladesLabel" };
        yup::ComboBox bladesCombo { "bladesCombo" };
        yup::ToggleButton motionBlurToggle { "motionBlurToggle" };
        yup::Label shutterLabel { "shutterLabel" };
        yup::Slider shutterSlider { yup::Slider::LinearHorizontal, "shutterSlider" };
        yup::ToggleButton reflectionProbeToggle { "reflectionProbeToggle" };
        yup::ToggleButton irradianceVolumeToggle { "irradianceVolumeToggle" };
        yup::ToggleButton clearcoatToggle { "clearcoatToggle" };
        yup::ToggleButton sheenToggle { "sheenToggle" };
        yup::ToggleButton fogToggle { "fogToggle" };
        yup::Label fogDensityLabel { "fogDensityLabel" };
        yup::Slider fogDensitySlider { yup::Slider::LinearHorizontal, "fogDensitySlider" };
        yup::Label fogFalloffLabel { "fogFalloffLabel" };
        yup::Slider fogFalloffSlider { yup::Slider::LinearHorizontal, "fogFalloffSlider" };
        yup::Label debugViewLabel { "debugViewLabel" };
        yup::ComboBox debugViewCombo { "debugViewCombo" };
        yup::Label taaValue { "taaValue" };
        yup::Label renderScaleValue { "renderScaleValue" };
        yup::Label exposureValue { "exposureValue" };
        yup::Label bloomThresholdValue { "bloomThresholdValue" };
        yup::Label bloomIntensityValue { "bloomIntensityValue" };
        yup::Label bloomRadiusValue { "bloomRadiusValue" };
        yup::Label keyLightSizeValue { "keyLightSizeValue" };
        yup::Label aoDistanceValue { "aoDistanceValue" };
        yup::Label fStopValue { "fStopValue" };
        yup::Label focusDistanceValue { "focusDistanceValue" };
        yup::Label shutterValue { "shutterValue" };
        yup::Label fogDensityValue { "fogDensityValue" };
        yup::Label fogFalloffValue { "fogFalloffValue" };
        yup::Label hintLabel { "hintLabel" };
        yup::Label statusLabel { "statusLabel" };
        yup::Label detailLabel { "detailLabel" };
        yup::Label gpuTimeLabel { "gpuTimeLabel" };
        yup::Label gpuPassesLabel { "gpuPassesLabel" };

        SettingsPage qualityPage;
        yup::TextButton benchmarkButton { "Benchmark" };
        yup::TextButton bakeProbesButton { "Bake probes" };
        SettingsPage effectsPage;
        SettingsPage scenePage;
        yup::TabComponent tabs { "renderSettingsTabs" };
        yup::Grid footerGrid;

        static constexpr int numFooterRows = 4;
        static constexpr float labelWidth = 120.0f;
        static constexpr float valueWidth = 40.0f;
        static constexpr float rowHeight = 24.0f;
        static constexpr float rowGap = 4.0f;
        static constexpr float sectionGap = 12.0f;
    };

    //==============================================================================
    /** An entity of the scene, with the parts it holds. */
    class EntityItem final : public yup::TreeViewItem
    {
    public:
        explicit EntityItem (yup::Entity entityToShow)
            : entity (entityToShow)
        {
            for (const auto child : entity.getChildren())
                addSubItem (std::make_unique<EntityItem> (child));
        }

        yup::String getItemText() const override
        {
            yup::StringArray parts;
            addPart<yup::MeshNode> (parts, "mesh");
            addPart<yup::MaterialNode> (parts, "material");
            addPart<yup::CameraNode> (parts, "camera");
            addPart<yup::LightNode> (parts, "light");

            auto text = entity.getName().isNotEmpty() ? entity.getName() : yup::String ("(unnamed)");

            if (! parts.isEmpty())
                text << "  [" << parts.joinIntoString (", ") << "]";

            if (! entity.isVisible())
                text << "  (hidden)";

            return text;
        }

        yup::String getUniqueName() const override
        {
            return entity.getName() + "#" + yup::String (getIndexInParent());
        }

        yup::Entity getEntity() const noexcept { return entity; }

    private:
        template <class T>
        void addPart (yup::StringArray& parts, const char* name) const
        {
            if (entity.has<T>())
                parts.add (name);
        }

        yup::Entity entity;
    };

    void setSceneTreeVisible (bool shouldBeVisible)
    {
        // Rebuilt when shown, so it reflects the current tree
        if (shouldBeVisible && view.getScene() != nullptr)
        {
            sceneTree.setRootItem (std::make_unique<EntityItem> (view.getScene()->getRoot()));
            sceneTree.getRootItem()->setOpenRecursively (true);
        }

        sceneTree.setVisible (shouldBeVisible);
        showSelectedBounds();
        resized();
    }

    /** Outlines the bounds of the entities selected in the scene tree, while it shows. */
    void showSelectedBounds()
    {
        std::vector<yup::Entity> selected;
        if (sceneTree.isVisible())
        {
            for (auto* item : sceneTree.getSelectedItems())
            {
                if (auto* entityItem = dynamic_cast<EntityItem*> (item))
                    selected.push_back (entityItem->getEntity());
            }
        }

        view.getRenderer().setBoundsOverlay (std::move (selected));
        view.repaint();
    }

    //==============================================================================
    yup::ToggleButton treeToggle { "treeToggle" };
    yup::SwitchButton freeMovementSwitch { "freeMovementSwitch" };
    yup::Label freeMovementLabel { "freeMovementLabel" };
    SettingsIconButton settingsToggle { "settingsToggle" };
    yup::TreeView sceneTree { "sceneTree" };
    MpcView view;
    RenderSettingsPanel settingsPanel { view };

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scene3DDemo)
};
