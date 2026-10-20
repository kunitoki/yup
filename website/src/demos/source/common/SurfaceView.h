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
#include <limits>
#include <memory>
#include <utility>
#include <vector>

//==============================================================================
/** A vertex of a SurfaceView mesh, in the world of pixelViewProjection() for the view's size. */
struct SurfaceVertex
{
    yup::Vector3<float> position;
    yup::Point<float> uv; // 0..1 across the panel, v pointing down like the panel
    yup::Vector3<float> normal;
};

//==============================================================================
/**
    Shows live Components on 3D meshes, lit, and keeps them usable.

    Every panel added with addSurface() is taken out of the normal compositing
    (setManuallyComposited), rendered to a texture each frame it changes, and drawn
    on its mesh through one GpuPipeline with depth and alpha blending, in the order
    the panels were added (back to front). Input goes back through a
    MeshSurfaceMapper fed the same mesh and camera, so the pointer always reaches the
    widget drawn under it, as in the Component3D example.

    Meshes are placed in view pixels, so a panel can be shown bigger or smaller than
    its own size: setContentScale() renders its texture sharp enough for that. At a
    lift of 0 the panels are drawn in plain 2D over the rectangle their mesh covers,
    so a demo must give its meshes their flat shape there (flatQuad() for
    rectangles); the lift blends in the lighting. The pointer always reaches the
    panels through their meshes, flat or not.
*/
class SurfaceView : public yup::Component
{
public:
    //==============================================================================
    SurfaceView()
        : yup::Component ("SurfaceView")
    {
        setOpaque (false);
    }

    //==============================================================================
    /** Adds a panel as a manually composited child and returns its surface index. */
    int addSurface (yup::Component& panel)
    {
        panel.setManuallyComposited (true);
        addAndMakeVisible (panel);

        auto surface = std::make_unique<Surface>();
        surface->panel = &panel;
        surfaces.push_back (std::move (surface));

        return static_cast<int> (surfaces.size()) - 1;
    }

    /** Replaces the mesh of a surface.

        The pointer is mapped through @p pickingVertices when given: the same mesh (same
        indices) without the deformations that react to hovering, such as a key dipping under
        the pointer. Picking against the drawn shape would then move the surface out from under
        the pointer, unhover it, move it back, and so on every frame.
    */
    void setSurfaceMesh (int index, std::vector<SurfaceVertex> vertices, std::vector<yup::uint32> indices, std::vector<SurfaceVertex> pickingVertices = {})
    {
        if (! yup::isPositiveAndBelow (index, static_cast<int> (surfaces.size())))
            return;

        jassert (pickingVertices.empty() || pickingVertices.size() == vertices.size());
        const auto& picked = pickingVertices.empty() ? vertices : pickingVertices;

        {
            const yup::SpinLock::ScopedLockType sl (mappingLock);
            auto& surface = *surfaces[static_cast<std::size_t> (index)];

            surface.positions.clear();
            surface.uvs.clear();
            for (const auto& vertex : picked)
            {
                surface.positions.push_back (vertex.position);
                surface.uvs.push_back (vertex.uv);
            }

            auto left = std::numeric_limits<float>::max(), top = left;
            auto right = std::numeric_limits<float>::lowest(), bottom = right;

            for (const auto& vertex : vertices)
            {
                // Back from the world of pixelViewProjection() to view pixels, ignoring depth.
                const auto x = vertex.position.getX() + 0.5f * getWidth();
                const auto y = 0.5f * getHeight() - vertex.position.getY();
                left = yup::jmin (left, x);
                top = yup::jmin (top, y);
                right = yup::jmax (right, x);
                bottom = yup::jmax (bottom, y);
            }

            surface.flatBounds = vertices.empty() ? yup::Rectangle<float>() : yup::Rectangle<float> (left, top, right - left, bottom - top);

            // Face every triangle the way its normals point, so the mapper can skip the hidden ones.
            for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
            {
                const auto& a = vertices[indices[i]];
                const auto& b = vertices[indices[i + 1]];
                const auto& c = vertices[indices[i + 2]];
                const auto facing = (b.position - a.position).crossProduct (c.position - a.position);

                if (facing.dotProduct (a.normal + b.normal + c.normal) < 0.0f)
                    std::swap (indices[i + 1], indices[i + 2]);
            }

            surface.mapper.setMesh (surface.positions, surface.uvs, indices);

            // Triangle indices stay meaningful while the topology does, which animated meshes keep.
            const auto numTriangles = static_cast<int> (indices.size() / 3);
            for (auto& track : surface.tracks)
            {
                if (track.triangle >= numTriangles)
                    track.triangle = -1;
            }
            if (surface.dragTriangle >= numTriangles)
                surface.dragTriangle = -1;
            surface.vertices = std::move (vertices);
            surface.indices = std::move (indices);
            surface.meshChanged = true;
        }

        repaint();
    }

    /** Sets the model transform of a surface, applied before the camera. */
    void setSurfaceTransform (int index, const yup::Matrix4& model)
    {
        if (! yup::isPositiveAndBelow (index, static_cast<int> (surfaces.size())))
            return;

        surfaces[static_cast<std::size_t> (index)]->model = model;
        repaint();
    }

    /** Sets how much bigger than their own size the panels appear, to render their textures sharp. */
    void setContentScale (float newScale)
    {
        if (newScale == contentScale)
            return;

        contentScale = newScale;
        repaint();
    }

    /** Sets how lit the surfaces are, from 0 (exactly 2D) to 1 (fully lit). */
    void setLift (float newLift)
    {
        if (newLift == lift)
            return;

        lift = newLift;
        repaint();
    }

    /** Lets the sound play on the lit surfaces: glow flashes a rim of blue light on hits, warmth
        turns the light from the site's pale blue toward amber. Both from 0 to 1. */
    void setAudio (float newGlow, float newWarmth)
    {
        if (std::abs (newGlow - glow) < 0.002f && std::abs (newWarmth - warmth) < 0.002f)
            return;

        glow = newGlow;
        warmth = newWarmth;
        repaint();
    }

    //==============================================================================
    /** A quad at depth zero covering the given pixel bounds of a view of the given size. */
    static std::vector<SurfaceVertex> flatQuad (yup::Rectangle<float> pixelBounds, yup::Size<float> viewport)
    {
        const yup::Vector3<float> normal { 0.0f, 0.0f, 1.0f };

        return {
            { pixelToWorld (pixelBounds.getTopLeft(), 0.0f, viewport), { 0.0f, 0.0f }, normal },
            { pixelToWorld (pixelBounds.getTopRight(), 0.0f, viewport), { 1.0f, 0.0f }, normal },
            { pixelToWorld (pixelBounds.getBottomRight(), 0.0f, viewport), { 1.0f, 1.0f }, normal },
            { pixelToWorld (pixelBounds.getBottomLeft(), 0.0f, viewport), { 0.0f, 1.0f }, normal },
        };
    }

    /** The indices of a flatQuad(), counter-clockwise when seen from the front. */
    static std::vector<yup::uint32> quadIndices()
    {
        return { 0, 3, 2, 0, 2, 1 };
    }

    //==============================================================================
    std::optional<yup::Point<float>> getChildPointFromLocal (const yup::Component& child, yup::Point<float> localPoint) const override
    {
        const yup::SpinLock::ScopedLockType sl (mappingLock);

        auto* surface = findSurface (child);
        if (surface == nullptr || ! surface->mapped)
            return std::nullopt;

        const auto uv = pick (*surface, localPoint);
        if (! uv)
            return std::nullopt;

        return yup::Point<float> (uv->getX() * child.getWidth(), uv->getY() * child.getHeight());
    }

    std::optional<yup::Point<float>> getLocalPointFromChild (const yup::Component& child, yup::Point<float> childPoint) const override
    {
        const yup::SpinLock::ScopedLockType sl (mappingLock);

        auto* surface = findSurface (child);
        if (surface == nullptr || ! surface->mapped)
            return std::nullopt;

        const yup::Point<float> uv { childPoint.getX() / child.getWidth(), childPoint.getY() / child.getHeight() };

        for (const auto& track : surface->tracks)
        {
            if (track.lastUse > 0 && track.hasUV && track.uv.distanceTo (uv) < 1.0e-5f)
                return track.viewPoint;
        }

        return surface->mapper.uvToViewport (uv);
    }

    //==============================================================================
    /** A drag stays on the plane of the triangle it started on, until the button is released. */
    void mouseDown (const yup::MouseEvent&) override
    {
        const yup::SpinLock::ScopedLockType sl (mappingLock);

        buttonDown = true;
        for (auto& surface : surfaces)
        {
            const auto* latest = &surface->tracks.front();
            for (const auto& track : surface->tracks)
                latest = track.lastUse > latest->lastUse ? &track : latest;

            surface->dragTriangle = latest->lastUse > 0 ? latest->triangle : -1;
        }
    }

    void mouseUp (const yup::MouseEvent&) override
    {
        const yup::SpinLock::ScopedLockType sl (mappingLock);

        buttonDown = false;
        for (auto& surface : surfaces)
            surface->dragTriangle = -1;
    }

    //==============================================================================
    void paint (yup::Graphics& g) override
    {
        for (auto& surface : surfaces)
            listenToDescendants (*surface->panel);

        auto& context = g.getGraphicsContext();
        const auto viewport = getLocalBounds();
        const auto scale = g.getContextScale();
        const auto width = yup::roundToInt (viewport.getWidth() * scale);
        const auto height = yup::roundToInt (viewport.getHeight() * scale);

        if (width < 2 || height < 2)
            return;

        const auto viewProjection = pixelViewProjection (viewport.getSize());

        {
            const yup::SpinLock::ScopedLockType sl (mappingLock);
            for (auto& surface : surfaces)
            {
                surface->modelViewProjection = surface->model.followedBy (viewProjection);
                surface->inverseModelViewProjection = surface->modelViewProjection.inverted();
                surface->viewport = viewport;
                surface->mapper.setModelViewProjection (surface->modelViewProjection, viewport);
                surface->mapped = ! surface->indices.empty();
            }
        }

        if (lift <= 0.001f || ! ensurePipeline (context) || ! target.ensure (device, width, height) || ! uploadMeshes())
        {
            drawFlat (g, context, scale);
            return;
        }

        std::vector<yup::GpuTexture::Ptr> textures;
        for (auto& surface : surfaces)
            textures.push_back (surface->panel->renderToTexture (context, scale * contentScale));

        auto frame = yup::GpuFrame::begin (device);
        if (! frame.isValid())
            return;

        {
            auto pass = target.color->beginRenderPass (frame, { true, yup::Colors::transparentBlack });
            pass.setDepthStencilAttachment (target.depth);
            pass.setPipeline (pipeline);

            for (std::size_t i = 0; i < surfaces.size(); ++i)
            {
                const auto& surface = surfaces[i];
                if (textures[i] == nullptr || surface->indexBuffer == nullptr)
                    continue;

                Uniforms uniforms;
                std::copy_n (viewProjection.getData(), 16, uniforms.viewProjection);
                std::copy_n (surface->model.getData(), 16, uniforms.model);
                uniforms.params[0] = lift;
                uniforms.params[2] = glow;
                uniforms.params[3] = warmth;
                uniforms.eye[2] = pixelCameraDistance (viewport.getSize());

                pass.setUniformBuffer (0, 0, &uniforms, sizeof (uniforms));
                pass.setTexture (0, 1, textures[i]);
                pass.setVertexBuffer (0, surface->vertexBuffer);
                pass.setIndexBuffer (yup::GpuIndexFormat::uint32, surface->indexBuffer);
                pass.drawIndexed (static_cast<yup::uint32> (surface->indices.size()));
            }

            pass.finish();
        }

        frame.submit();

        g.drawTexture (target.color->asTexture(), viewport);
    }

private:
    //==============================================================================
    /** What was picked last around one point that keeps being mapped, such as the pointer. */
    struct PickTrack
    {
        yup::Point<float> viewPoint;
        yup::Point<float> uv;
        int triangle = -1;
        bool hasUV = false;
        yup::uint32 lastUse = 0; // 0 while unused
    };

    static constexpr float trackRadius = 24.0f; // a query this close to a recent one continues its state

    struct Surface
    {
        yup::Component* panel = nullptr;
        yup::Matrix4 model;

        std::vector<SurfaceVertex> vertices;
        std::vector<yup::uint32> indices;
        std::vector<yup::Vector3<float>> positions;
        std::vector<yup::Point<float>> uvs;
        yup::Rectangle<float> flatBounds; // in view pixels, what the mesh covers when flat
        bool meshChanged = false;

        yup::GpuBuffer::Ptr vertexBuffer;
        yup::GpuBuffer::Ptr indexBuffer;

        yup::MeshSurfaceMapper mapper;
        bool mapped = false;
        mutable std::array<PickTrack, 4> tracks {}; // per point being mapped, for the hysteresis and the way back
        mutable int dragTriangle = -1;             // under the pointer when the button went down, until it goes up

        // The camera the mapper was last given, for picking against a single triangle
        yup::Matrix4 modelViewProjection;
        yup::Matrix4 inverseModelViewProjection;
        yup::Rectangle<float> viewport;
    };

    struct alignas (16) Uniforms
    {
        float viewProjection[16];
        float model[16];
        float params[4] = {};
        float eye[4] = {};
    };

    static constexpr int floatsPerVertex = 8;

    //==============================================================================
    static constexpr float stickyPixels = 3.0f;     // how far past its edges the last picked triangle keeps the pointer
    static constexpr float stickyDepth = 12.0f;     // how much farther along the ray it may be than a new hit, in world units

    struct TriangleHit
    {
        yup::Point<float> uv;
        float distance = 0.0f; // along the picking ray
    };

    /** Maps a view point to panel uv: during a drag on the plane of the triangle where it started,
        otherwise a real hit or, with hysteresis, the triangle picked last near this point. */
    std::optional<yup::Point<float>> pick (const Surface& surface, yup::Point<float> localPoint) const
    {
        auto& track = trackFor (surface, localPoint);

        if (buttonDown && surface.dragTriangle >= 0)
        {
            if (const auto onPlane = intersectTriangle (surface, surface.dragTriangle, localPoint, false))
                return remember (track, localPoint, surface.dragTriangle, onPlane->uv);

            return track.hasUV ? std::optional<yup::Point<float>> (track.uv) : std::nullopt;
        }

        const auto hit = surface.mapper.hitTest (localPoint);

        if (track.triangle >= 0 && (! hit || hit->triangleIndex != track.triangle) && isNearTriangle (surface, track.triangle, localPoint))
        {
            if (const auto last = intersectTriangle (surface, track.triangle, localPoint))
            {
                const auto newDistance = hit ? intersectTriangle (surface, hit->triangleIndex, localPoint) : std::nullopt;
                if (! newDistance || last->distance - newDistance->distance < stickyDepth)
                    return remember (track, localPoint, track.triangle, last->uv);
            }
        }

        if (hit)
            return remember (track, localPoint, hit->triangleIndex, hit->uv);

        track.viewPoint = localPoint;
        track.triangle = -1;
        track.hasUV = false;
        return std::nullopt;
    }

    /** The pick state of the point being mapped. Events map more than the pointer (YUP also maps
        the last mouse down position on every event), so each point keeps its own state: the one
        of the recent point it is near, or a fresh one replacing the least recently used. */
    PickTrack& trackFor (const Surface& surface, yup::Point<float> point) const
    {
        PickTrack* nearest = nullptr;
        PickTrack* oldest = &surface.tracks.front();

        for (auto& track : surface.tracks)
        {
            if (track.lastUse > 0 && track.viewPoint.distanceTo (point) < trackRadius && (nearest == nullptr || track.viewPoint.distanceTo (point) < nearest->viewPoint.distanceTo (point)))
                nearest = &track;

            if (track.lastUse < oldest->lastUse)
                oldest = &track;
        }

        auto& track = nearest != nullptr ? *nearest : (*oldest = PickTrack {});
        track.lastUse = ++useCounter;
        return track;
    }

    /** Notes a mapping on its track, so getLocalPointFromChild() can map it back exactly. */
    static yup::Point<float> remember (PickTrack& track, yup::Point<float> viewPoint, int triangle, yup::Point<float> uv)
    {
        track.viewPoint = viewPoint;
        track.triangle = triangle;
        track.uv = uv;
        track.hasUV = true;
        return uv;
    }

    /** Where a point of a surface's mesh appears in the view, as MeshSurfaceMapper places it. */
    static std::optional<yup::Point<float>> projectToView (const Surface& surface, const yup::Vector3<float>& modelPoint)
    {
        const auto clip = surface.modelViewProjection.transformPoint4 (modelPoint.getX(), modelPoint.getY(), modelPoint.getZ(), 1.0f);
        if (clip[3] <= 0.0f)
            return std::nullopt;

        return yup::Point<float> { surface.viewport.getX() + (clip[0] / clip[3] + 1.0f) * 0.5f * surface.viewport.getWidth(),
                                   surface.viewport.getY() + (1.0f - clip[1] / clip[3]) * 0.5f * surface.viewport.getHeight() };
    }

    /** True when the point is inside the triangle as it appears in the view, or less than stickyPixels outside it. */
    bool isNearTriangle (const Surface& surface, int triangle, yup::Point<float> point) const
    {
        std::array<yup::Point<float>, 3> corners;
        for (std::size_t k = 0; k < 3; ++k)
        {
            const auto projected = projectToView (surface, surface.positions[surface.indices[static_cast<std::size_t> (triangle) * 3 + k]]);
            if (! projected)
                return false;

            corners[k] = *projected;
        }

        const auto cross = [] (yup::Point<float> a, yup::Point<float> b) { return a.getX() * b.getY() - a.getY() * b.getX(); };

        const auto area = cross (corners[1] - corners[0], corners[2] - corners[0]);
        if (std::abs (area) < 1.0e-6f)
            return false;

        const auto inside = area > 0.0f ? 1.0f : -1.0f;
        for (std::size_t k = 0; k < 3; ++k)
        {
            const auto from = corners[k];
            const auto edge = corners[(k + 1) % 3] - from;
            const auto length = std::sqrt (edge.getX() * edge.getX() + edge.getY() * edge.getY());

            if (length > 0.0f && inside * cross (edge, point - from) / length < -stickyPixels)
                return false;
        }

        return true;
    }

    /** Where the picking ray meets the plane of a triangle, as panel uv and ray distance; unless
        @p frontFacingOnly is false, only when the triangle faces the viewer. */
    std::optional<TriangleHit> intersectTriangle (const Surface& surface, int triangle, yup::Point<float> point, bool frontFacingOnly = true) const
    {
        const auto first = static_cast<std::size_t> (triangle) * 3;
        const auto ia = surface.indices[first], ib = surface.indices[first + 1], ic = surface.indices[first + 2];
        const auto& a = surface.positions[ia];
        const auto ab = surface.positions[ib] - a;
        const auto ac = surface.positions[ic] - a;
        const auto normal = ab.crossProduct (ac);

        const auto ray = yup::Ray::fromViewportPoint (point, surface.viewport, surface.inverseModelViewProjection);
        if (frontFacingOnly && normal.dotProduct (ray.getDirection()) >= 0.0f)
            return std::nullopt;

        const auto distance = ray.intersectPlane (a, normal);
        if (! distance)
            return std::nullopt;

        // Barycentric weights of the plane point, which may lie a little outside the triangle
        const auto ap = ray.getPointAt (*distance) - a;
        const auto d00 = ab.dotProduct (ab), d01 = ab.dotProduct (ac), d11 = ac.dotProduct (ac);
        const auto d20 = ap.dotProduct (ab), d21 = ap.dotProduct (ac);
        const auto denominator = d00 * d11 - d01 * d01;
        if (std::abs (denominator) < 1.0e-12f)
            return std::nullopt;

        const auto v = (d11 * d20 - d01 * d21) / denominator;
        const auto w = (d00 * d21 - d01 * d20) / denominator;
        const auto u = 1.0f - v - w;

        return TriangleHit { surface.uvs[ia] * u + surface.uvs[ib] * v + surface.uvs[ic] * w, *distance };
    }

    void listenToDescendants (yup::Component& component)
    {
        component.addMouseListener (this);

        for (int i = 0; i < component.getNumChildComponents(); ++i)
        {
            if (auto* child = component.getChildComponent (i))
                listenToDescendants (*child);
        }
    }

    const Surface* findSurface (const yup::Component& child) const
    {
        for (const auto& surface : surfaces)
        {
            if (surface->panel == &child)
                return surface.get();
        }

        return nullptr;
    }

    bool ensurePipeline (yup::GraphicsContext& context)
    {
        if (pipeline != nullptr)
            return true;

        if (compileAttempted || ! context.isGpuAvailable())
            return false;

        compileAttempted = true;
        device = context.getGpuDevice();
        if (device == nullptr)
            return false;

        yup::GpuPipelineOptions options;
        options.vertexBuffers.emplace_back (
            static_cast<uint32_t> (floatsPerVertex * sizeof (float)),
            yup::GpuVertexStepMode::vertex,
            std::vector<yup::GpuVertexAttribute> {
                { yup::GpuVertexFormat::float3, 0, 0 },
                { yup::GpuVertexFormat::float2, 12, 1 },
                { yup::GpuVertexFormat::float3, 20, 2 },
            });
        options.topology = yup::GpuPrimitiveTopology::triangleList;
        options.indexFormat = yup::GpuIndexFormat::uint32;
        options.cullMode = yup::GpuCullMode::none; // the backs of panels are shaded, not culled

        auto& colorTarget = options.colorTargets.emplace_back();
        colorTarget.format = yup::GpuTextureFormat::rgba8unorm;
        colorTarget.blendEnabled = true;
        colorTarget.blend.srcColor = yup::GpuBlendFactor::one; // panel textures are premultiplied

        options.depthStencil.enabled = true;
        options.depthStencil.format = yup::GpuTextureFormat::depth24plusStencil8;
        options.depthStencil.depthCompare = yup::GpuCompareFunction::lessEqual;
        options.depthStencil.depthWriteEnabled = true;

        auto result = compileEmbeddedPipeline (device, website::SurfaceShader_data, website::SurfaceShader_size, options);
        if (result.failed())
        {
            yup::Logger::outputDebugString ("SurfaceView: " + result.getErrorMessage());
            return false;
        }

        pipeline = result.getValue();
        return true;
    }

    bool uploadMeshes()
    {
        for (auto& surface : surfaces)
        {
            if (! surface->meshChanged)
                continue;

            std::vector<SurfaceVertex> vertices;
            std::vector<yup::uint32> indices;

            {
                const yup::SpinLock::ScopedLockType sl (mappingLock);
                vertices = surface->vertices;
                indices = surface->indices;
                surface->meshChanged = false;
            }

            std::vector<float> data;
            data.reserve (vertices.size() * floatsPerVertex);
            for (const auto& v : vertices)
            {
                data.insert (data.end(), { v.position.getX(), v.position.getY(), v.position.getZ(),
                                           v.uv.getX(), v.uv.getY(),
                                           v.normal.getX(), v.normal.getY(), v.normal.getZ() });
            }

            if (data.empty() || indices.empty())
                continue;

            surface->vertexBuffer = yup::GpuBuffer::create (device, yup::GpuBufferType::vertex, data.data(), data.size() * sizeof (float));
            surface->indexBuffer = yup::GpuBuffer::create (device, yup::GpuBufferType::index, indices.data(), indices.size() * sizeof (yup::uint32));

            if (surface->vertexBuffer == nullptr || surface->indexBuffer == nullptr)
                return false;
        }

        return true;
    }

    /** Draws the panels flat over their meshes, at rest or without the GPU path; they stay usable. */
    void drawFlat (yup::Graphics& g, yup::GraphicsContext& context, float scale)
    {
        for (auto& surface : surfaces)
        {
            if (auto texture = surface->panel->renderToTexture (context, scale * contentScale))
                g.drawTexture (texture, surface->flatBounds);
        }
    }

    //==============================================================================
    std::vector<std::unique_ptr<Surface>> surfaces;
    float lift = 0.0f;
    float glow = 0.0f, warmth = 0.0f;
    float contentScale = 1.0f;
    bool buttonDown = false;
    mutable yup::uint32 useCounter = 0;

    yup::GpuDevice::Ptr device;
    yup::GpuPipeline::Ptr pipeline;
    bool compileAttempted = false;
    DepthTarget target;

    mutable yup::SpinLock mappingLock;
};
