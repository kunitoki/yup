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

class SceneSurface;

//==============================================================================
/** A component that is an Entity of the Scene its SceneComponent shows.

    This is the `Node3D` of a `.yui` document, and the base of the other 3D components: the
    tree of components written inside a `Scene3D` is the tree of entities of its scene. A node
    makes its entity under the entity of the node it is in, or under the root of the scene when
    it is directly inside the `Scene3D`. The entity is there as soon as the component is under a
    scene, keeps among its siblings the place the component has among the components, and is
    destroyed with the component or when it leaves the scene, taking the entities under it
    along. getEntity() gives it to code. A component moved to another parent leaves its scene
    on the way: its entity is made again under the entity of the new parent, as are those of
    the nodes inside it, with all that the components describe. What code attached to the
    entity is not kept: a subclass attaches it from entityChanged().

    A node paints nothing and takes no room in a layout: the properties of a 2D component it
    inherits (its bounds, the layout item ones, the 2D transform, the opacity) have no effect.
    Its own ones place the node, and are properties like any other, so bindings, style rules,
    transitions and keyframes drive them:

    - `position`: [x, y, z], relative to the parent.
    - `rotation`: [x, y, z] Euler angles in radians, so `45deg` is written as it is. The turn
      around z is applied first, then the one around x, then the one around y: x and y aim what
      looks down -z, a camera or a light, as an elevation and a heading. The angles are kept as
      they are given, so animating to `[0, 360deg, 0]` is a full turn.
    - `scale`: a number for every axis, or [x, y, z]. Write both ends of a transition the same
      way: a number and an array are not blended.
    - `lookAt`: [x, y, z], a point relative to the parent that the node turns its -z axis to. It
      replaces the rotation, and is followed when the position or the point changes.
    - `travel`: [x, y, z], how far the node moves over its range, relative to the parent, from
      where it is without it.
    - `turn`: [x, y, z] Euler angles in radians, as those of `rotation`, the node turns by over
      its range, in its own space: a knob turns around its own axis, whatever way its rotation
      orients it.
    - `value`: where the node is along its travel and its turn, from 0 to 1.
    - `visible`: false hides the entity, and so everything under it.
    - `morphWeights`: [w0, w1, ...], the weight of each morph target of the mesh the entity
      deforms, in the order of the targets, for a node bound to the part of a model that has
      them. A target given no weight keeps the one of the model. Not set, the weights stay as
      the model, or a clip animating them, has them, and the mesh gets back the ones it had
      once the node sets them no more. A clip animating them sets them each time it poses the
      model, so the two take turns: the last one to write shows.

    The entity only gets a TransformNode once one of them is set, so a node used as a plain
    group costs nothing. A value that is not set reads as the identity one once the node has
    its entity, and as nothing before.

    @code
    Scene3D {
        Node3D {
            id: arm; position: [0, 1, 0]; rotation: [0, 0, lever.value * 90deg];

            Mesh3D { vertices: [0, 0, 0,  1, 0, 0,  0, 1, 0]; }
        }
    }
    @endcode

    With `node`, a `Node3D` inside a `Model3D` (directly, or inside another bound node of it) is
    bound to an entity of the model instead of making one: `node` is the name of the entity, the
    first one of that name met depth first, or the names leading to it from the root of the
    model separated by `/`. The node then drives the transform and the visibility of that
    entity, which it does not own: what it does not set is left as the model has it, and reads
    as nothing, and the entity gets back what the model gave it when the node stops setting it
    or goes. The 3D components inside a bound node make their entities under the entity of the
    model, which is how a light is put on a part of it. Only a `Node3D` can be bound.

    @code
    Model3D {
        source: "models/device.glb";

        Node3D { node: "Knob"; rotation: [0, cutoff.value * 270deg, 0]; }
        Node3D { node: "Body/Lid"; visible: lid.checked;  Light3D { type: point; } }
    }
    @endcode

    A node takes the pointer where what it shows is the nearest thing under it, to the triangle
    (see SceneComponent::findNodeAt): a `Mesh3D` for its mesh, a `Node3D` bound to a part of a
    model for that part and the ones under it, a `Model3D` for the rest of its model. It then is
    a component like any other for the mouse: it has the `:hover` and `:pressed` states, a mouse
    cursor, and `onClick`, emitted when a press is released over it. A node that is not visible
    is not there for the pointer, one that is not enabled is, and takes it for nothing.

    @code
    style {
        Model3D:hover { scale: 1.1; transition: scale 150ms easeOut; }
    }

    Model3D {
        source: "models/device.glb";

        Node3D { node: "Knob"; onClick: action ("knob"); }
    }
    @endcode

    A node that sets `travel` or `turn` is a control. Its `value` places it: at 0 it is where
    its `position` and its `rotation` put it, or else where the model put the entity it is
    bound to, and at 1 it has moved by its travel and turned by its turn. Pressing it and moving
    the pointer changes the value, as the handle of a slider is dragged: along the line the node
    travels on, as the scene shows it, so the node stays under the pointer whatever the camera,
    or up and down for a node that only turns, the whole range taking 200 units of the pointer.
    The pointer keeps the node until it is released, also when it leaves what the node shows. A
    press that moved the value is not a click. The value tells its changes, so `<=>` links it
    with the value of a Slider, or of a model.

    @code
    Model3D {
        source: "models/computer.glb";

        Node3D { node: "FaderA"; travel: [0, 0, -0.78]; value <=> cutoff.value; }
        Node3D { node: "KnobA"; rotation: [0, -45deg, 0]; turn: [0, -270deg, 0]; value <=> resonance.value; }
    }
    @endcode

    A component that is not part of the scene, written inside a node, is shown on the mesh the
    node draws: the one of its own entity, so the mesh of a `Mesh3D`, or of the part of a model
    a `Node3D` is bound to. The components of a node make its surface, the area from the origin
    to their far corner in their own units, which lies over the texture coordinates 0 to 1 of
    the mesh. They are rendered to a texture when they repaint, given to the material of the
    node in the slot `surface` names: `emissive`, the default, for an interface that glows like
    a screen whatever the light, or `baseColor` for one lit as the rest of the object. That
    material is the `Material3D` of the node when it has one, or else one of the surface's own,
    which looks as the mesh would without it. The pointer reaches the components where the mesh
    shows them, before the node itself and after the nodes inside it that are in front of the
    mesh there, so they are used as anywhere else. A node that draws no mesh of its own, or one
    whose texture coordinates span no area, as those of a mesh that was given none, shows
    nothing of them and says so in the diagnostics of the document.

    @code
    Mesh3D {
        vertices: [-1, -1, 0,  1, -1, 0,  1, 1, 0,  -1, 1, 0];
        uvs: [0, 1,  1, 1,  1, 0,  0, 0];
        indices: [0, 1, 2,  0, 2, 3];

        TextButton { width: 200; height: 200; text: "On a quad"; onClick: action ("pressed"); }
    }
    @endcode

    @see SceneComponent, SceneCameraComponent, SceneLightComponent, SceneMeshComponent, SceneModelComponent, SceneMaterialComponent
*/
class YUP_API SceneNodeComponent : public Component
{
public:
    //==============================================================================
    /** Creates a node, which makes its entity once it is added under a SceneComponent.

        @param componentID The ID of the component.
    */
    explicit SceneNodeComponent (StringRef componentID = {});

    /** Destructor. The entity the node made is destroyed with the entities under it, the entity
        of a model it is bound to gets back what the model gave it.
    */
    ~SceneNodeComponent() override;

    //==============================================================================
    /** Returns the entity of the node: the one it made, or the one of a model it is bound to.

        @returns The entity, or an invalid entity while the node is not in a scene or is bound
                 to an entity its model does not have.
    */
    Entity getEntity() const noexcept;

    //==============================================================================
    /** Binds the node to an entity of the model it is in, the `node` of a `Node3D`.

        @param newName The name of the entity, or the names leading to it from the root of the
                       model separated by `/`. Empty for the node to make an entity of its own.
    */
    void setNodeName (const String& newName);

    /** Returns the name of the entity of a model the node is bound to, or an empty string. */
    const String& getNodeName() const noexcept { return nodeName; }

    //==============================================================================
    /** Where the material of a node takes the texture of the components shown on its mesh. */
    enum class SurfaceSlot
    {
        emissive, ///< As emitted light: the components keep their colors whatever the light, like a screen.
        baseColor ///< As the base color: the components are lit like the rest of the object.
    };

    /** Chooses how the components inside the node are shown on its mesh, the `surface` of a `Node3D`.

        As emitted light the texture goes with an emissive factor of 1, and as base color with a
        base color of 1 that keeps its alpha: the components are shown with their own colors.
        Nothing else of the material changes, and it gets back what it had when the node has no
        component to show anymore.

        @param newSlot The slot of the material that takes the texture.
    */
    void setSurfaceSlot (SurfaceSlot newSlot);

    /** Returns how the components inside the node are shown on its mesh. */
    SurfaceSlot getSurfaceSlot() const noexcept { return surfaceSlot; }

    //==============================================================================
    /** Places the node along its travel and its turn, the `value` of a `Node3D`.

        A node that sets neither `travel` nor `turn` keeps the value, and is not moved by it.

        @param newValue Where the node is along its range: 0 where it is without its travel and
                        its turn, 1 once it has moved and turned by all of them. Limited to that
                        range.
    */
    void setValue (double newValue);

    /** Returns where the node is along its travel and its turn, from 0 to 1. */
    double getValue() const noexcept { return currentValue; }

    //==============================================================================
    /** Called when the node is clicked: pressed, then released over it while it is enabled.

        A press on a component shown on the mesh of the node is the one of that component, and
        is not a click of the node. Nor is a press that dragged the node along its travel or
        its turn.
    */
    std::function<void()> onClick;

    //==============================================================================
    /** Returns the static type info describing the Node3D type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal */
    void setVisible (bool shouldBeVisible) override;
    /** @internal */
    bool hitTest (float x, float y) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    std::optional<Point<float>> getChildPointFromLocal (const Component& child, Point<float> localPoint) const override;
    /** @internal */
    std::optional<Point<float>> getLocalPointFromChild (const Component& child, Point<float> childPoint) const override;
    /** @internal */
    void childrenChanged() override;
    /** @internal */
    void visibilityChanged() override;
    /** @internal */
    void enablementChanged() override;
    /** @internal */
    void parentHierarchyChanged() override;
    /** @internal */
    void parentSizeChanged() override;
    /** @internal */
    void resized() override;
    /** @internal */
    void moved() override;
    /** @internal Gives the node components below a component the entities their place asks for: what a scene or a model instance that changed calls. */
    static void updateEntitiesBelow (Component& parent);
    /** @internal Returns what shows the components inside the node on its mesh, or nullptr when it has none: what its scene updates, renders and lets go of. */
    SceneSurface* getSurface() const noexcept { return surface.get(); }

protected:
    //==============================================================================
    /** Called when the node has another entity than before, or none anymore.

        A subclass attaches what it shows to getEntity() from here, when it is valid. Nothing
        has to be taken away from the entity the node had: it was destroyed with its nodes.
    */
    virtual void entityChanged() {}

    /** Repaints the SceneComponent the node is in, which draws its scene again. Call it after
        changing what the entity shows.
    */
    void repaintScene();

private:
    //==============================================================================
    struct Transform
    {
        Vector3<float> position;
        Quaternion rotation;
        Vector3<float> scale { 1.0f, 1.0f, 1.0f };
    };

    /* A press of the pointer on the node, which drags it when it travels or turns. */
    struct Press
    {
        Point<float> position;      // Where the pointer was, in the scene
        double value = 0.0;         // Where the node was along its range
        bool hasMovedValue = false; // A press that dragged the node is not a click
    };

    bool isBound() const;
    void updateEntity();
    void releaseEntity();
    Entity findParentEntity();
    Entity findModelEntity (String& problem);
    void placeEntity (Entity parentEntity);
    void applyTransform();
    Vector3<float> getRestPosition() const;
    std::optional<double> getDraggedValue (Point<float> pointerPosition);
    void updateBounds();
    var getTransformValue (const var& value, const var& identity) const;
    void setTransformValue (var SceneNodeComponent::* member, const var& newValue, const Identifier& property);
    void applyMorphWeights (bool isLeaving = false);
    void setMorphWeights (const var& newWeights);

    Scene::Ptr scene; // Kept with the entity, which can then be asked at any time whether it is still there
    Entity entity;
    bool isEntityOwned = false;
    std::optional<Transform> boundTransform;
    bool boundVisible = true;
    bool drivesPosition = false;
    bool drivesRotation = false;
    bool drivesScale = false;
    String nodeName;
    var position;
    var rotation;
    var scale;
    var lookAt;
    var travel;
    var turn;
    double currentValue = 0.0;
    var morphWeights;
    std::optional<std::vector<float>> modelWeights; // What the morph targets of the mesh had when the node first set their weights
    SurfaceSlot surfaceSlot = SurfaceSlot::emissive;
    std::optional<Press> press;
    std::optional<Point<float>> coveredPosition; // Where the pointer was last looked for and found over a node inside this one
    std::unique_ptr<SceneSurface> surface;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneNodeComponent)
};

} // namespace yup
