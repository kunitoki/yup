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

#include <yup_shading/yup_shading.h>

#include <atomic>
#include <vector>

//==============================================================================

/**
    Demonstrates a live, interactive Component UI mapped onto a curved 3D surface.

    The panel of widgets stays a regular child of the view: it is marked with
    setManuallyComposited(), rendered with renderToTexture(), and drawn by the view on a
    cylindrical section through a GpuPipeline with depth testing and back face culling.

    The same vertex data given to the GPU is given to a MeshSurfaceMapper, and the view
    overrides getChildPointFromLocal() / getLocalPointFromChild() to map the pointer onto the
    panel through it. Clicks, drags, hover, the combo box popup and text editing all work on
    the curved surface, even while it rotates.

    Drag the empty space to orbit the panel, use the slider to change its curvature.
*/
class Component3DDemo : public yup::Component
{
public:
    Component3DDemo()
        : yup::Component ("Component3DDemo")
    {
        curvatureLabel.setText ("Curvature", yup::dontSendNotification);
        addAndMakeVisible (curvatureLabel);

        curvatureSlider.setRange (0.0, 2.8);
        curvatureSlider.setValue (1.4);
        curvatureSlider.onValueChanged = [this] (double value)
        {
            view.setCurvature (static_cast<float> (value));
        };
        addAndMakeVisible (curvatureSlider);

        autoRotateToggle.setButtonText ("Auto rotate");
        autoRotateToggle.onClick = [this]
        {
            view.setAutoRotate (autoRotateToggle.getToggleState());
        };
        addAndMakeVisible (autoRotateToggle);

        hintLabel.setText ("Drag the empty space to orbit the panel", yup::dontSendNotification);
        addAndMakeVisible (hintLabel);

        addAndMakeVisible (view);
    }

    void paint (yup::Graphics& g) override
    {
        g.setFillColor (findColor (yup::DocumentWindow::Style::backgroundColorId).value_or (yup::Colors::dimgray));
        g.fillAll();
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10.0f);

        auto toolRow = bounds.removeFromTop (30.0f);
        curvatureLabel.setBounds (toolRow.removeFromLeft (80.0f));
        curvatureSlider.setBounds (toolRow.removeFromLeft (200.0f).reduced (0.0f, 3.0f));
        toolRow.removeFromLeft (10.0f);
        autoRotateToggle.setBounds (toolRow.removeFromLeft (120.0f));
        toolRow.removeFromLeft (10.0f);
        hintLabel.setBounds (toolRow);

        bounds.removeFromTop (6.0f);
        view.setBounds (bounds);
    }

private:
    //==============================================================================
    /** The UI presented on the curved surface. */
    class DemoPanel final : public yup::Component
    {
    public:
        DemoPanel()
            : yup::Component ("demoPanel")
        {
            titleLabel.setText ("A Component on a curved surface", yup::dontSendNotification);
            addAndMakeVisible (titleLabel);

            clickButton.onClick = [this]
            {
                ++clickCount;
                statusLabel.setText ("Button clicked " + yup::String (clickCount) + " times", yup::dontSendNotification);
            };
            addAndMakeVisible (clickButton);

            slider.setRange (0.0, 100.0);
            slider.setValue (25.0);
            slider.onValueChanged = [this] (double value)
            {
                statusLabel.setText ("Slider value: " + yup::String (value, 1), yup::dontSendNotification);
            };
            addAndMakeVisible (slider);

            comboBox.addItem ("First option", 1);
            comboBox.addItem ("Second option", 2);
            comboBox.addItem ("Third option", 3);
            comboBox.setSelectedId (1);
            comboBox.onSelectedItemChanged = [this]
            {
                statusLabel.setText ("Selected: " + comboBox.getText(), yup::dontSendNotification);
            };
            addAndMakeVisible (comboBox);

            textEditor.setText ("Type here...", yup::dontSendNotification);
            addAndMakeVisible (textEditor);

            statusLabel.setText ("Interact with the widgets", yup::dontSendNotification);
            addAndMakeVisible (statusLabel);
        }

        void paint (yup::Graphics& g) override
        {
            g.setFillColor (yup::Color (0xff2b2f3a));
            g.fillAll();

            g.setStrokeColor (yup::Colors::orange);
            g.setStrokeWidth (4.0f);
            g.strokeRect (getLocalBounds().reduced (2.0f));
        }

        void resized() override
        {
            auto bounds = getLocalBounds().reduced (20.0f);

            titleLabel.setBounds (bounds.removeFromTop (30.0f));
            bounds.removeFromTop (10.0f);

            auto row = bounds.removeFromTop (32.0f);
            clickButton.setBounds (row.removeFromLeft (140.0f));
            row.removeFromLeft (10.0f);
            comboBox.setBounds (row);
            bounds.removeFromTop (14.0f);

            slider.setBounds (bounds.removeFromTop (32.0f));
            bounds.removeFromTop (14.0f);

            textEditor.setBounds (bounds.removeFromTop (32.0f));
            bounds.removeFromTop (14.0f);

            statusLabel.setBounds (bounds.removeFromTop (30.0f));
        }

    private:
        yup::Label titleLabel { "titleLabel" };
        yup::TextButton clickButton { "Click me" };
        yup::Slider slider { yup::Slider::LinearHorizontal, "slider" };
        yup::ComboBox comboBox { "comboBox" };
        yup::TextEditor textEditor { "textEditor" };
        yup::Label statusLabel { "statusLabel" };
        int clickCount = 0;
    };

    //==============================================================================
    /** Renders the panel on a cylindrical section and routes the pointer onto it. */
    class CurvedPanelView final : public yup::Component
    {
    public:
        CurvedPanelView()
            : yup::Component ("curvedPanelView")
        {
            panel.setSize (panelWidth, panelHeight);
            panel.setManuallyComposited (true);
            addAndMakeVisible (panel);

            setCurvature (1.4f);
        }

        /** Rebuilds the mesh as a cylindrical section spanning @a arcAngle radians. */
        void setCurvature (float arcAngle)
        {
            {
                const yup::SpinLock::ScopedLockType sl (meshLock);
                buildCylinderMesh (arcAngle);
                mapper.setMesh (positions, uvs, indices);
                meshChanged = true;
            }

            repaint();
        }

        void setAutoRotate (bool shouldRotate)
        {
            autoRotate = shouldRotate;
        }

        //==============================================================================
        std::optional<yup::Point<float>> getChildPointFromLocal (const yup::Component& child, yup::Point<float> localPoint) const override
        {
            if (&child != &panel)
                return yup::Component::getChildPointFromLocal (child, localPoint);

            const yup::SpinLock::ScopedLockType sl (meshLock);

            const auto uv = mapper.viewportToUV (localPoint);
            if (! uv)
                return std::nullopt;

            return yup::Point<float> (uv->getX() * panel.getWidth(), uv->getY() * panel.getHeight());
        }

        std::optional<yup::Point<float>> getLocalPointFromChild (const yup::Component& child, yup::Point<float> childPoint) const override
        {
            if (&child != &panel)
                return yup::Component::getLocalPointFromChild (child, childPoint);

            const yup::SpinLock::ScopedLockType sl (meshLock);

            return mapper.uvToViewport ({ childPoint.getX() / panel.getWidth(), childPoint.getY() / panel.getHeight() });
        }

        //==============================================================================
        void refreshDisplay (double lastFrameTimeSeconds) override
        {
            if (! autoRotate)
                return;

            yaw = yaw + static_cast<float> (lastFrameTimeSeconds) * 0.5f;
            repaint();
        }

        void mouseDown (const yup::MouseEvent& event) override
        {
            lastDragPosition = event.getPosition();
        }

        void mouseDrag (const yup::MouseEvent& event) override
        {
            const auto delta = event.getPosition() - lastDragPosition;
            lastDragPosition = event.getPosition();

            yaw = yaw + delta.getX() * 0.01f;
            pitch = yup::jlimit (-1.2f, 1.2f, pitch + delta.getY() * 0.01f);
            repaint();
        }

        //==============================================================================
        void paint (yup::Graphics& g) override
        {
            g.setFillColor (clearColor);
            g.fillAll();

            auto& context = g.getGraphicsContext();
            if (! ensurePipeline (context))
            {
                g.setFillColor (yup::Colors::white);
                g.fillFittedText (pipelineError, yup::ApplicationTheme::getGlobalTheme()->getDefaultFont(), getLocalBounds().reduced (20.0f));
                return;
            }

            const auto viewport = getLocalBounds();
            const auto width = yup::roundToInt (viewport.getWidth() * g.getContextScale());
            const auto height = yup::roundToInt (viewport.getHeight() * g.getContextScale());
            if (width < 2 || height < 2)
                return;

            auto panelTexture = panel.renderToTexture (context, g.getContextScale());
            if (panelTexture == nullptr)
                return;

            auto device = context.getGpuDevice();
            if (! ensureTargets (device, width, height) || ! uploadMeshIfChanged (device))
                return;

            const auto modelViewProjection = getModelViewProjection (viewport);

            struct Uniforms
            {
                float modelViewProjection[16];
            } uniforms;

            std::copy_n (modelViewProjection.getData(), 16, uniforms.modelViewProjection);

            auto frame = yup::GpuFrame::begin (device);
            if (! frame.isValid())
                return;

            {
                auto pass = sceneTarget->beginRenderPass (frame, { true, clearColor });
                pass.setDepthStencilAttachment (depthTexture);
                pass.setPipeline (pipeline);
                pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
                pass.setTexture (0, 1, panelTexture);
                pass.setVertexBuffer (0, vertexBuffer);
                pass.setIndexBuffer (yup::GpuIndexFormat::uint32, indexBuffer);
                pass.drawIndexed (indexCount);
                pass.finish();
            }

            frame.submit();

            g.drawTexture (sceneTarget->asTexture(), viewport);

            // Input is mapped with the camera that was actually drawn
            const yup::SpinLock::ScopedLockType sl (meshLock);
            mapper.setModelViewProjection (modelViewProjection, viewport);
        }

    private:
        //==============================================================================
        // The vertex shader only applies the MVP matrix, so the CPU picking in the
        // MeshSurfaceMapper matches the GPU rasterization exactly. Texture coordinates
        // have v pointing down, like the panel, and are sampled as they are.
        static constexpr char vertexSource[] = R"glsl(#version 450
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(set = 0, binding = 0) uniform Uniforms { mat4 modelViewProjection; } u;
layout(location = 0) out vec2 v_uv;
void main() {
    gl_Position = u.modelViewProjection * vec4(a_position, 1.0);
    v_uv = a_uv;
}
)glsl";

        static constexpr char fragmentSource[] = R"glsl(#version 450
layout(location = 0) in vec2 v_uv;
layout(set = 0, binding = 1) uniform texture2D u_tex;
layout(set = 0, binding = 2) uniform sampler u_samp;
layout(location = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(texture(sampler2D(u_tex, u_samp), v_uv).rgb, 1.0);
}
)glsl";

        static constexpr float panelWidth = 480.0f;
        static constexpr float panelHeight = 300.0f;
        static constexpr float surfaceWidth = 3.0f;
        static constexpr int segments = 48;
        static constexpr int floatsPerVertex = 5;

        const yup::Color clearColor { 0xff1a1a2e };

        //==============================================================================
        void buildCylinderMesh (float arcAngle)
        {
            const auto surfaceHeight = surfaceWidth * panelHeight / panelWidth;

            positions.clear();
            uvs.clear();
            indices.clear();

            for (int i = 0; i <= segments; ++i)
            {
                const auto u = static_cast<float> (i) / segments;
                auto x = (u - 0.5f) * surfaceWidth;
                auto z = 0.0f;

                // Bend the surface towards the viewer, like a curved monitor
                if (arcAngle > 0.001f)
                {
                    const auto radius = surfaceWidth / arcAngle;
                    const auto theta = (u - 0.5f) * arcAngle;
                    x = radius * std::sin (theta);
                    z = radius * (1.0f - std::cos (theta));
                }

                positions.push_back ({ x, surfaceHeight * 0.5f, z });
                uvs.push_back ({ u, 0.0f });

                positions.push_back ({ x, -surfaceHeight * 0.5f, z });
                uvs.push_back ({ u, 1.0f });
            }

            // Counter-clockwise when seen from the front
            for (yup::uint32 i = 0; i < static_cast<yup::uint32> (segments); ++i)
            {
                const auto top = i * 2;
                const auto bottom = top + 1;
                const auto nextTop = top + 2;
                const auto nextBottom = top + 3;

                indices.insert (indices.end(), { bottom, nextBottom, nextTop, bottom, nextTop, top });
            }
        }

        yup::Matrix4 getModelViewProjection (yup::Rectangle<float> viewport) const
        {
            const auto model = yup::Matrix4::rotationY (yaw).followedBy (yup::Matrix4::rotationX (pitch));
            const auto view = yup::Matrix4::lookAt ({ 0.0f, 0.0f, 4.2f }, {}, { 0.0f, 1.0f, 0.0f });
            const auto projection = yup::Matrix4::perspective (0.8f, viewport.getWidth() / viewport.getHeight(), 0.5f, 50.0f);

            return model.followedBy (view).followedBy (projection);
        }

        //==============================================================================
        bool ensurePipeline (yup::GraphicsContext& context)
        {
            if (pipeline != nullptr)
                return true;

            if (pipelineError.isNotEmpty())
                return false;

            if (! context.isGpuAvailable())
            {
                pipelineError = "GPU context unavailable";
                return false;
            }

            yup::GpuPipelineOptions options;
            options.vertexBuffers.emplace_back (
                static_cast<uint32_t> (floatsPerVertex * sizeof (float)),
                yup::GpuVertexStepMode::vertex,
                std::vector<yup::GpuVertexAttribute> {
                    { yup::GpuVertexFormat::float3, 0, 0 },
                    { yup::GpuVertexFormat::float2, 12, 1 },
                });
            options.topology = yup::GpuPrimitiveTopology::triangleList;
            options.indexFormat = yup::GpuIndexFormat::uint32;
            options.cullMode = yup::GpuCullMode::back;
            options.winding = yup::GpuFaceWinding::counterClockwise;

            auto& colorTarget = options.colorTargets.emplace_back();
            colorTarget.format = yup::GpuTextureFormat::rgba8unorm;
            colorTarget.blendEnabled = false;

            options.depthStencil.enabled = true;
            options.depthStencil.format = yup::GpuTextureFormat::depth24plusStencil8;
            options.depthStencil.depthCompare = yup::GpuCompareFunction::less;
            options.depthStencil.depthWriteEnabled = true;

            auto result = yup::GpuPipeline::compileFromGlsl (context.getGpuDevice(), vertexSource, fragmentSource, options);
            if (result.failed())
            {
                pipelineError = "Pipeline compile failed: " + result.getErrorMessage();
                yup::Logger::outputDebugString ("Component3DDemo: " + pipelineError);
                return false;
            }

            pipeline = result.getValue();
            return true;
        }

        bool ensureTargets (yup::GpuDevice::Ptr device, int width, int height)
        {
            if (sceneTarget != nullptr && sceneTarget->getWidth() == width && sceneTarget->getHeight() == height)
                return true;

            yup::GpuTextureDesc depthDesc;
            depthDesc.width = static_cast<uint32_t> (width);
            depthDesc.height = static_cast<uint32_t> (height);
            depthDesc.format = yup::GpuTextureFormat::depth24plusStencil8;
            depthDesc.renderTarget = true;
            depthDesc.label = "Component3DDemo depth";

            sceneTarget = yup::GpuTarget::create (device, width, height);
            depthTexture = yup::GpuTexture::create (device, depthDesc);

            return sceneTarget != nullptr && depthTexture != nullptr;
        }

        bool uploadMeshIfChanged (yup::GpuDevice::Ptr device)
        {
            std::vector<float> vertices;
            std::vector<yup::uint32> indexData;

            {
                const yup::SpinLock::ScopedLockType sl (meshLock);

                if (! meshChanged)
                    return vertexBuffer != nullptr && indexBuffer != nullptr;

                meshChanged = false;

                vertices.reserve (positions.size() * floatsPerVertex);
                for (std::size_t i = 0; i < positions.size(); ++i)
                {
                    vertices.insert (vertices.end(), { positions[i].getX(), positions[i].getY(), positions[i].getZ(), uvs[i].getX(), uvs[i].getY() });
                }

                indexData = indices;
            }

            vertexBuffer = yup::GpuBuffer::create (device, yup::GpuBufferType::vertex, vertices.data(), vertices.size() * sizeof (float));
            indexBuffer = yup::GpuBuffer::create (device, yup::GpuBufferType::index, indexData.data(), indexData.size() * sizeof (yup::uint32));
            indexCount = static_cast<yup::uint32> (indexData.size());

            return vertexBuffer != nullptr && indexBuffer != nullptr;
        }

        //==============================================================================
        DemoPanel panel;

        // Shared between paint() and the input mapping, which can run on different threads
        mutable yup::SpinLock meshLock;
        yup::MeshSurfaceMapper mapper;
        std::vector<yup::Vector3<float>> positions;
        std::vector<yup::Point<float>> uvs;
        std::vector<yup::uint32> indices;
        bool meshChanged = false;

        std::atomic<float> yaw { 0.35f };
        std::atomic<float> pitch { -0.15f };
        std::atomic<bool> autoRotate { false };
        yup::Point<float> lastDragPosition;

        yup::GpuPipeline::Ptr pipeline;
        yup::String pipelineError;
        yup::GpuTarget::Ptr sceneTarget;
        yup::GpuTexture::Ptr depthTexture;
        yup::GpuBuffer::Ptr vertexBuffer;
        yup::GpuBuffer::Ptr indexBuffer;
        yup::uint32 indexCount = 0;
    };

    //==============================================================================
    yup::Label curvatureLabel { "curvatureLabel" };
    yup::Slider curvatureSlider { yup::Slider::LinearHorizontal, "curvatureSlider" };
    yup::ToggleButton autoRotateToggle { "autoRotateToggle" };
    yup::Label hintLabel { "hintLabel" };
    CurvedPanelView view;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Component3DDemo)
};
