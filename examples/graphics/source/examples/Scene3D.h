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

#include <yup_3d/yup_3d.h>

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
    the knob scales and the caps, is painted with yup::Graphics into a texture once. Its screen
    shows a live Component: it is rendered to a texture used as the emissive texture of the
    screen material. Clicking the screen animates the camera onto it, with a custom Node part
    ticked by Scene::update(); from there clicks are mapped onto the LCD through a
    MeshSurfaceMapper, and clicking outside it zooms back out.
    The scene is only redrawn continuously while the camera moves. The "Scene tree" button
    shows the entity tree of the scene, and "Free movement" lets the mouse orbit, pan and zoom
    the camera.
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

        // The camera can't move freely while it frames the screen
        view.onScreenZoomChanged = [this] (bool isZoomedOnScreen)
        {
            freeMovementSwitch.setEnabled (! isZoomedOnScreen);
            freeMovementLabel.setEnabled (! isZoomedOnScreen);
        };

        sceneTree.setRootItemVisible (true);
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

        auto toolbar = bounds.removeFromTop (30.0f);
        treeToggle.setBounds (toolbar.removeFromLeft (140.0f));
        toolbar.removeFromLeft (16.0f);
        freeMovementSwitch.setBounds (toolbar.removeFromLeft (44.0f).reduced (0.0f, 5.0f));
        toolbar.removeFromLeft (8.0f);
        freeMovementLabel.setBounds (toolbar.removeFromLeft (140.0f));

        bounds.removeFromTop (6.0f);

        if (sceneTree.isVisible())
        {
            sceneTree.setBounds (bounds.removeFromLeft (260.0f));
            bounds.removeFromLeft (6.0f);
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

        void paint (yup::Graphics& g) override
        {
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

        void applyTo (yup::EntityNode& entity) const
        {
            entity.setPosition (position);
            entity.setRotation (rotation);
        }
    };

    //==============================================================================
    /** A part that moves its entity to a camera pose with an eased animation, ticked by Scene::update(). */
    class CameraAnimator final : public yup::Node
    {
    public:
        void animateTo (const CameraPose& target, double durationSeconds)
        {
            start = { getEntity()->getPosition(), getEntity()->getRotation() };
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

            CameraPose::interpolate (start, end, eased).applyTo (*getEntity());
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

                auto gltfRoot = model.getReference().createEntity();
                scene->getRoot()->addChild (gltfRoot);

                auto* device = gltfRoot->findChild ("Cube");
                if (device != nullptr)
                {
                    attachScreen (*device);
                    faceplate.setDevice (*device);
                }

                // Center the device on the origin
                auto* framed = device != nullptr ? device : gltfRoot.get();
                gltfRoot->setPosition (-framed->computeWorldBounds().getCenter());

                deviceBounds = framed->computeWorldBounds();
                radius = yup::jmax (0.001f, deviceBounds.getRadius());
            }
            else
            {
                status = "Unable to load mpc.glb: " + model.getErrorMessage();
            }

            cameraEntity = new yup::EntityNode ("camera");
            cameraNode = &cameraEntity->attach<yup::CameraNode>();
            animator = &cameraEntity->attach<CameraAnimator>();
            cameraNode->zNear = radius * 0.02f;
            cameraNode->zFar = radius * 40.0f;

            scene->getRoot()->addChild (cameraEntity);
            scene->setActiveCamera (cameraEntity.get());

            // Lit like a product shot: the studio gives the reflections and the soft light, a key
            // light along its main soft box, front left above the device, gives the shadows
            auto keyLightEntity = yup::EntityNode::Ptr (new yup::EntityNode ("keyLight"));
            auto& keyLight = keyLightEntity->attach<yup::LightNode>();
            keyLight.castsShadows = true;
            keyLight.intensity = 3.5f;
            keyLight.color = { 1.0f, 0.98f, 0.95f };
            CameraPose::lookingAt (directionFromDegrees (35.0f, 50.0f) * 10.0f, {}, { 0.0f, 1.0f, 0.0f }).applyTo (*keyLightEntity);
            scene->getRoot()->addChild (keyLightEntity);

            scene->setEnvironment (createStudioEnvironment());
            scene->setEnvironmentIntensity (0.55f);
            scene->setAmbientColor (yup::Colors::black);
            scene->setExposure (0.9f);
            scene->setToneMapping (yup::Scene::ToneMapping::aces);
            scene->setEnvironmentVisible (true);
            scene->setEnvironmentBlur (0.5f);

            setScene (scene);

            updateStatus();
        }

        //==============================================================================
        /** Called when the camera zooms onto the screen or back out. */
        std::function<void (bool isZoomedOnScreen)> onScreenZoomChanged;

        /** Lets the mouse orbit, pan and zoom the camera, or brings it back to the top view.

            Ignored while zoomed onto the screen.
        */
        void setFreeMovement (bool shouldMoveFreely)
        {
            if (freeMovement == shouldMoveFreely || zoomedOnScreen)
                return;

            freeMovement = shouldMoveFreely;

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

        //==============================================================================
        void resized() override
        {
            // Both poses depend on the aspect ratio: keep the camera framed while resizing
            if (! animator->isAnimating())
                getTargetPose().applyTo (*cameraEntity);
        }

        void paint (yup::Graphics& g) override
        {
            auto& context = g.getGraphicsContext();
            if (context.isGpuAvailable())
            {
                faceplate.updateTexture (context);

                if (lcdTexture != nullptr)
                    lcdTexture->setGpuTexture (lcd.renderToTexture (context, lcdTextureScale));
            }

            yup::SceneComponent::paint (g);

            // Input is mapped with the camera that was actually drawn
            updateScreenMapping();
        }

        void paintOverChildren (yup::Graphics& g) override
        {
            g.setFillColor (yup::Colors::white.withAlpha (0.7f));
            g.fillFittedText (status,
                              yup::ApplicationTheme::getGlobalTheme()->getDefaultFont(),
                              getLocalBounds().reduced (10.0f).withHeight (24.0f),
                              yup::Justification::topLeft);
        }

        void refreshDisplay (double lastFrameTimeSeconds) override
        {
            yup::SceneComponent::refreshDisplay (lastFrameTimeSeconds);

            // Only animate while the camera moves, the scene is static otherwise
            if (! animator->isAnimating())
                setContinuousUpdates (false);
        }

        //==============================================================================
        void mouseDown (const yup::MouseEvent& event) override
        {
            if (animator->isAnimating())
                return;

            // Clicks on the LCD while zoomed in reach the LCD itself, so any click here is outside it
            if (zoomedOnScreen)
            {
                zoomTo (false);
                return;
            }

            if (! freeMovement)
            {
                if (hitsScreen (event.getPosition()))
                    zoomTo (true);

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
                // Move the orbit center in the view plane, at the speed of the surface under it
                const auto pose = getOrbitPose();
                const auto forward = (orbitTarget - pose.position).normalized();
                const auto right = forward.crossProduct ({ 0.0f, 1.0f, 0.0f }).normalized();
                const auto up = right.crossProduct (forward);
                const auto unitsPerPixel = 2.0f * orbitDistance * std::tan (cameraNode->yFov * 0.5f) / yup::jmax (1.0f, getHeight());

                orbitTarget = orbitTarget - right * (delta.getX() * unitsPerPixel) + up * (delta.getY() * unitsPerPixel);
            }
            else
            {
                zoomOrbit (delta.getY() * 0.01f);
            }

            getOrbitPose().applyTo (*cameraEntity);
            repaint();
        }

        void mouseUp (const yup::MouseEvent& event) override
        {
            // A left click that didn't drag still zooms onto the screen
            const auto wasClick = dragMode == DragMode::rotate && dragDistance < 4.0f;
            dragMode = DragMode::none;

            if (wasClick && hitsScreen (event.getPosition()))
                zoomTo (true);
        }

        void mouseWheel (const yup::MouseEvent&, const yup::MouseWheelData& wheelData) override
        {
            if (! freeMovement || zoomedOnScreen || animator->isAnimating())
                return;

            zoomOrbit (-wheelData.getDeltaY() * 0.15f);
            getOrbitPose().applyTo (*cameraEntity);
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
        void attachScreen (yup::EntityNode& device)
        {
            auto* meshNode = device.getNode<yup::MeshNode>();
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
            screenEntity = &device;
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

        void zoomOrbit (float amount)
        {
            const auto radius = deviceBounds.getRadius();
            orbitDistance = yup::jlimit (radius * 0.3f, radius * 10.0f, orbitDistance * std::exp (amount));
        }

        bool hitsScreen (yup::Point<float> position) const
        {
            if (screenEntity == nullptr)
                return false;

            // hitTest() and not viewportToUV(), which extrapolates beyond the screen for drags
            const yup::SpinLock::ScopedLockType sl (mapperLock);
            return screenMapper.hitTest (position).has_value();
        }

        /** Facing the screen along its normal, close enough for it to fill the view. */
        CameraPose getScreenPose() const
        {
            const auto world = screenEntity->getWorldMatrix();
            const auto center = world.transformPoint (screenCenter);
            const auto up = world.transformVector (screenUp);
            const auto right = world.transformVector (screenRight);
            const auto normal = right.crossProduct (up).normalized();

            const auto distance = getFittingDistance (right.length() * 0.5f, up.length() * 0.5f) * 1.15f;

            return CameraPose::lookingAt (center + normal * distance, center, up.normalized());
        }

        CameraPose getTargetPose() const
        {
            if (zoomedOnScreen && screenEntity != nullptr)
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

        void updateStatus()
        {
            if (screenEntity == nullptr)
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
            if (screenEntity == nullptr)
                return;

            const auto viewport = getLocalBounds();
            if (viewport.getWidth() <= 0.0f || viewport.getHeight() <= 0.0f)
                return;

            const auto view = cameraEntity->getWorldMatrix().inverted();
            const auto projection = cameraNode->getProjectionMatrix (getAspectRatio());

            const yup::SpinLock::ScopedLockType sl (mapperLock);
            screenMapper.setModelViewProjection (screenEntity->getWorldMatrix().followedBy (view).followedBy (projection), viewport);
        }

        //==============================================================================
        static constexpr float lcdTextureScale = 2.0f;
        static constexpr float faceplatePixelWidth = 4096.0f;
        static constexpr double zoomSeconds = 0.7;
        static constexpr float minOrbitPitch = 0.05f;
        static constexpr float maxOrbitPitch = 1.56f;

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

        yup::EntityNode::Ptr cameraEntity;
        yup::CameraNode* cameraNode = nullptr;
        CameraAnimator* animator = nullptr;

        yup::EntityNode* screenEntity = nullptr;
        yup::Vector3<float> screenCenter;
        yup::Vector3<float> screenUp;
        yup::Vector3<float> screenRight;
        yup::BoundingBox deviceBounds { { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f } };

        bool zoomedOnScreen = false;
        yup::String status;

        bool freeMovement = false;
        yup::Vector3<float> orbitTarget;
        float orbitYaw = 0.0f;
        float orbitPitch = maxOrbitPitch;
        float orbitDistance = 1.0f;

        DragMode dragMode = DragMode::none;
        yup::Point<float> lastDragPosition;
        float dragDistance = 0.0f;
    };

    //==============================================================================
    /** An entity of the scene, with the parts it holds. */
    class EntityItem final : public yup::TreeViewItem
    {
    public:
        explicit EntityItem (yup::EntityNode::Ptr entityToShow)
            : entity (std::move (entityToShow))
        {
            for (const auto& child : entity->getChildren())
                addSubItem (std::make_unique<EntityItem> (child));
        }

        yup::String getItemText() const override
        {
            yup::StringArray parts;
            addPartCount<yup::MeshNode> (parts, "mesh");
            addPartCount<yup::MaterialNode> (parts, "material");
            addPartCount<yup::CameraNode> (parts, "camera");
            addPartCount<yup::LightNode> (parts, "light");

            auto text = entity->getName().isNotEmpty() ? entity->getName() : yup::String ("(unnamed)");

            if (! parts.isEmpty())
                text << "  [" << parts.joinIntoString (", ") << "]";

            if (! entity->isVisible())
                text << "  (hidden)";

            return text;
        }

        yup::String getUniqueName() const override
        {
            return entity->getName() + "#" + yup::String (getIndexInParent());
        }

    private:
        template <class T>
        void addPartCount (yup::StringArray& parts, const char* name) const
        {
            if (const auto count = entity->getNumNodes<T>(); count > 0)
                parts.add (count == 1 ? yup::String (name) : yup::String (count) + " " + name + "s");
        }

        yup::EntityNode::Ptr entity;
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
        resized();
    }

    //==============================================================================
    yup::ToggleButton treeToggle { "treeToggle" };
    yup::SwitchButton freeMovementSwitch { "freeMovementSwitch" };
    yup::Label freeMovementLabel { "freeMovementLabel" };
    yup::TreeView sceneTree { "sceneTree" };
    MpcView view;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scene3DDemo)
};
