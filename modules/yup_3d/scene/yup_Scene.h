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

namespace yup
{

//==============================================================================
/** A 3D world: a root entity, the camera that views it and the global lighting settings.

    Build the world under getRoot(), then show it with a SceneComponent.

    @code
    auto scene = yup::Scene::Ptr (new yup::Scene());
    scene->getRoot()->addChild (yup::GltfModel::loadFromFile (file).getReference().createEntity());

    auto camera = yup::EntityNode::Ptr (new yup::EntityNode ("camera"));
    camera->attach<yup::CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 3.0f });
    scene->getRoot()->addChild (camera);
    scene->setActiveCamera (camera.get());
    @endcode

    Like Component, scenes are meant to be used from the message thread only.

    @see EntityNode, SceneComponent, SceneRenderer
*/
class YUP_API Scene : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<Scene>;

    //==============================================================================
    /** Creates a scene with an empty root entity. */
    Scene();

    /** Destructor. */
    ~Scene() override;

    //==============================================================================
    /** Returns the root entity, which always exists. */
    const EntityNode::Ptr& getRoot() const noexcept { return root; }

    //==============================================================================
    /** Chooses the camera that renders the scene.

        The entity is referenced weakly, and the camera is looked up again every frame, so
        replacing the CameraNode in that slot is picked up automatically.

        @param entity  The entity holding the camera, or nullptr to use the fallback.
        @param slot    The CameraNode slot on the entity.
    */
    void setActiveCamera (EntityNode* entity, int slot = 0);

    /** Returns the camera that renders the scene.

        This is the camera chosen with setActiveCamera(). When its entity was deleted or has no
        camera in that slot, it is the first CameraNode found depth-first in the visible part of
        the tree. When there is no camera at all it is nullptr, and the renderer uses a default
        perspective camera at (0, 0, 5) looking at the origin.
    */
    CameraNode* getActiveCamera() const noexcept;

    //==============================================================================
    /** Returns the color the background is cleared to. */
    Color getBackgroundColor() const noexcept { return backgroundColor; }

    /** Changes the color the background is cleared to. */
    void setBackgroundColor (Color newColor) noexcept { backgroundColor = newColor; }

    /** Returns the flat ambient light color added to every surface. */
    Color getAmbientColor() const noexcept { return ambientColor; }

    /** Changes the flat ambient light color added to every surface. */
    void setAmbientColor (Color newColor) noexcept { ambientColor = newColor; }

    /** Returns the exposure multiplier applied before tone mapping. */
    float getExposure() const noexcept { return exposure; }

    /** Changes the exposure multiplier applied before tone mapping. */
    void setExposure (float newExposure) noexcept { exposure = newExposure; }

    /** Returns true if a default directional light is used when the tree has no LightNode. */
    bool isUsingDefaultLight() const noexcept { return useDefaultLight; }

    /** Chooses whether a default directional light is used when the tree has no LightNode. */
    void setUsingDefaultLight (bool shouldUseDefaultLight) noexcept { useDefaultLight = shouldUseDefaultLight; }

    //==============================================================================
    /** Calls Node::update() on every part of every visible entity, depth-first.

        Parts must not attach or detach parts of their own entity from update().

        @param deltaSeconds The time elapsed since the previous update.
    */
    void update (double deltaSeconds);

private:
    EntityNode::Ptr root;
    WeakReference<EntityNode> activeCameraEntity;
    int activeCameraSlot = 0;

    Color backgroundColor { 0xff1a1a2e };
    Color ambientColor { 0xff595959 };
    float exposure = 1.0f;
    bool useDefaultLight = true;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scene)
};

} // namespace yup
