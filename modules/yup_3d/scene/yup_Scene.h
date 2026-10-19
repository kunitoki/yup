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
/** A 3D world: a root entity and the camera that views it.

    Build the world under getRoot(), then show it with a SceneComponent of the yup_renderer module.
    Scene-wide features such as the background, the environment or the fog are nodes of the
    yup_renderer module attached to the root, see SceneSettings there.

    @code
    auto scene = yup::Scene::Ptr (new yup::Scene());
    scene->getRoot()->addChild (yup::GltfModel::loadFromFile (file).getReference().createEntity());

    auto camera = yup::Entity::Ptr (new yup::Entity ("camera"));
    camera->attach<yup::CameraNode>();
    camera->setPosition ({ 0.0f, 0.0f, 3.0f });
    scene->getRoot()->addChild (camera);
    scene->setActiveCamera (camera.get());
    @endcode

    Like Component, scenes are meant to be used from the message thread only.

    @see Entity
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
    const Entity::Ptr& getRoot() const noexcept { return root; }

    //==============================================================================
    /** Chooses the camera that renders the scene.

        The entity is referenced weakly, and the camera is looked up again every frame, so
        replacing the CameraNode in that slot is picked up automatically.

        @param entity  The entity holding the camera, or nullptr to use the fallback.
        @param slot    The CameraNode slot on the entity.
    */
    void setActiveCamera (Entity::Ptr entity, int slot = 0);

    /** Returns the camera that renders the scene.

        This is the camera chosen with setActiveCamera(). When its entity was deleted or has no
        camera in that slot, it is the first CameraNode found depth-first in the visible part of
        the tree. When there is no camera at all it is nullptr, and the renderer uses a default
        perspective camera at (0, 0, 5) looking at the origin.
    */
    CameraNode* getActiveCamera() const noexcept;

    //==============================================================================
    /** Calls Node::update() on every part of every visible entity, depth-first.

        Parts must not attach or detach parts of their own entity from update().

        @param deltaSeconds The time elapsed since the previous update.
    */
    void update (double deltaSeconds);

private:
    Entity::Ptr root;
    WeakReference<Entity> activeCameraEntity;
    int activeCameraSlot = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Scene)
};

} // namespace yup
