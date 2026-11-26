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

namespace yup
{

//==============================================================================
/** The components a SceneNodeComponent shows on the mesh of its entity: what a node makes of the
    components inside it that are not part of the scene.

    The surface is the area from the origin to the far corner of those components, in their own
    units, and lies over the texture coordinates 0 to 1 of the mesh. It takes the components out
    of the painting of their parent (Component::setManuallyComposited), renders them to a texture
    with Component::renderToTexture() when the scene is painted, and gives the texture to the
    material the entity is drawn with: the one a MaterialNode of the entity holds, or else one of
    its own, a copy of the one the mesh would be drawn with. A MeshSurfaceMapper takes the points
    of the scene to the ones of the surface and back, for the two mapping hooks of the node.

    @internal
*/
class SceneSurface
{
public:
    //==============================================================================
    /** Creates the surface of a node, which shows nothing until it is given components. */
    explicit SceneSurface (SceneNodeComponent& nodeToShowOn);

    /** Gives the material back what the surface took, and the components back to their parent. */
    ~SceneSurface();

    //==============================================================================
    /** Changes the components the surface shows, which are children of the node. */
    void setComponents (const std::vector<Component*>& newComponents);

    /** Returns the area the components take, from the origin, in their own units. */
    Rectangle<float> getArea() const;

    //==============================================================================
    /** Finds the mesh and the material the surface is shown with. Not to be called while painting.

        @param severity Set to a warning when what is returned does not prevent the components
                        from being shown, left as it is otherwise.

        @returns What is to be reported about the node, once: that it draws no mesh to show the
                 components on, one whose texture coordinates span no area for them to lie
                 over, or one of several primitives that the material of the surface repaints
                 all. Empty when there is nothing to tell, or when it was told already.
    */
    String update (DataTreeDiagnostic::Severity& severity);

    /** Has the components rendered again when the scene is next painted: what they show changed
        with something above them, which does not ask them to repaint.
    */
    void repaintComponents();

    /** Renders the components that were repainted and gives their texture to the material.
        Called while the scene is painted, before it is rendered.
    */
    void render (GraphicsContext& context, float scale);

    /** Gives the material back what the surface took from the slot it is shown in. */
    void restoreMaterial();

    /** Lets go of the material and of what was rendered, until the next update(). */
    void release();

    //==============================================================================
    /** Returns the point of the surface shown at a point of the scene, on the mesh or on the
        plane of its nearest triangle.

        Where no triangle faces the point anymore, past the horizon of a mesh that leans back or
        on one that turned away, it is the one the mesh last showed there, or else where it last
        showed the pointer: at the point it was last looked for, see setPointerPosition(), or at
        the last one asked about for the first time since. So a component that holds the pointer
        stays where it was, as does the point it was pressed at.

        @returns The point, or nothing when there is no mesh to map onto, none whose texture
                 coordinates span an area, or nothing the mesh showed before.
    */
    std::optional<Point<float>> getSurfacePoint (Point<float> scenePoint);

    /** Tells where the pointer is in the scene, when it is looked for there. */
    void setPointerPosition (Point<float> scenePoint) noexcept { pointerPoint = scenePoint; }

    /** Returns the point of the scene a point of the surface is shown at, or nothing when the
        mesh does not show it.
    */
    std::optional<Point<float>> getScenePoint (Point<float> surfacePoint);

private:
    //==============================================================================
    /** What the composite of several components was last drawn from. */
    struct Layer
    {
        GpuTexture::Ptr texture;
        Rectangle<float> bounds;
        AffineTransform transform;

        bool operator== (const Layer&) const = default;
    };

    /** A point of the scene the surface was asked about, and the one of the surface it was answered with. */
    struct Answer
    {
        Point<float> scenePoint;
        Point<float> surfacePoint;
    };

    /** What is to be told about how the node shows the components, as it was last reported. */
    enum class Problem
    {
        none,
        noMesh,
        noTextureArea,
        severalPrimitives
    };

    Mesh::Ptr findMesh() const;
    bool updateMesh();
    bool updateMapping (SceneComponent& view);
    GpuTexture::Ptr renderComponents (GraphicsContext& context, float scale);
    void applyTexture();
    void releaseMaterial();
    void releaseTexture();

    SceneNodeComponent& node;
    std::vector<WeakReference<Component>> components;
    Problem reportedProblem = Problem::none;

    // The material showing the surface: the one of the entity, or one of its own given to it
    Material::Ptr host;
    Material::Ptr ownMaterial;
    Texture::Ptr replacedTexture;
    std::array<float, 3> replacedFactor { 1.0f, 1.0f, 1.0f };

    // What was rendered, and the device it was rendered with: kept, as another one could be made where it was once it is gone
    Texture::Ptr texture;
    GpuTexture::Ptr content;
    GpuCanvas::Ptr composite;
    std::vector<Layer> layers;
    GpuDevice::Ptr device;
    bool areCanvasesOutdated = false;

    // The mesh the components lie over, as it was last looked at, and the mapper given its triangles when next asked
    MeshSurfaceMapper mapper;
    Mesh::Ptr mappedMesh;
    uint64 mappedChangeCount = 0;
    bool hasTextureArea = false;
    bool isMapperOutdated = false;

    // The last points of the scene asked about, the latest first, and the one the pointer is at
    std::vector<Answer> answers;
    Point<float> pointerPoint;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneSurface)
};

} // namespace yup
