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
/** A Component that displays a Scene.

    The scene is rendered at the pixel size of the component (its bounds times the display
    scale) and drawn over the whole component, so the component is opaque. When no GPU is
    available it fills its background and shows a message instead. Clicking the component gives
    it the keyboard focus, so mouse wheel events reach it.

    By default the scene is drawn only when the component repaints: call repaint() after
    changing it. With setContinuousUpdates (true) the component calls Scene::update() and
    repaints on every display frame, for animated scenes.

    @see Scene, SceneRenderer
*/
class YUP_API SceneComponent : public Component
{
public:
    //==============================================================================
    /** Creates a component without a scene.

        @param componentID The ID of the component.
    */
    explicit SceneComponent (StringRef componentID = {});

    /** Destructor. */
    ~SceneComponent() override;

    //==============================================================================
    /** Changes the scene to display. */
    void setScene (Scene::Ptr newScene);

    /** Returns the scene being displayed, or nullptr. */
    const Scene::Ptr& getScene() const noexcept { return scene; }

    /** Returns the renderer, for example to read its last error. */
    SceneRenderer& getRenderer() noexcept { return renderer; }

    //==============================================================================
    /** Chooses whether the scene is updated and repainted on every display frame.

        @param shouldUpdateContinuously True to call Scene::update() and repaint every frame.
    */
    void setContinuousUpdates (bool shouldUpdateContinuously) noexcept { continuousUpdates = shouldUpdateContinuously; }

    /** Returns true if the scene is updated and repainted on every display frame. */
    bool isUpdatingContinuously() const noexcept { return continuousUpdates; }

    //==============================================================================
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds) override;

private:
    Scene::Ptr scene;
    SceneRenderer renderer;
    bool continuousUpdates = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneComponent)
};

} // namespace yup
