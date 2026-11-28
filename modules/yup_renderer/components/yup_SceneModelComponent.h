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
/** A node that shows a glTF model: the `Model3D` of a `.yui` document.

    `source` is the path of a `.glb` or `.gltf` file, read through the document as its other
    files are (see SceneComponent::loadModel), and `scene` picks one of the scenes of the file.
    The model is instantiated under the entity of the node, so the node places it:

    @code
    Scene3D {
        Model3D {
            source: "models/device.glb"; scale: 0.5;

            Node3D { node: "Knob"; rotation: [0, cutoff.value * 270deg, 0]; }
            Light3D { type: point; position: [0, 1, 0]; }
        }
    }
    @endcode

    The models of one file share their meshes, their materials and their textures, in every
    `Model3D` of the same `Scene3D`. When the document tells that the file changed, the model is
    loaded and instantiated again, and stays as it was until then. The file is parsed on a thread
    of the scene: the node shows nothing until its model is first there, with no instance, no
    clips, and a `duration` and a `time` of 0 (see SceneComponent::isLoading).

    A `Node3D` with a `node` written inside the model is bound to an entity of its instance, to
    move it or hide it (see SceneNodeComponent), and is bound again each time the model is
    instantiated. The other 3D components inside the model are placed by its node.

    A file that can't be read or loaded draws nothing and is reported in the diagnostics of the
    document, as is a `scene` the file does not have. What the loader could not convert is
    reported as warnings.

    The clips of an animated model are played by its properties, which are properties like any
    other: bindings, style rules, transitions and keyframes drive them.

    - `clip`: the name of the clip to play, or its index among the clips of the file when no
      clip has that name. Without it the model plays its first clip. Naming a clip poses the
      model where the clip starts: at its start, or at its end when it plays backwards. A clip
      the model does not have is reported: nothing plays, and `time` and `duration` read 0.
    - `playing`: plays the clip while true. A clip that does not loop stops when it reaches the
      end it plays towards and tells with `onClipEnded`: `playing` stays as it was set, and the
      clip plays again, from the other end, the next time `playing` turns true.
    - `speed`: how fast the clip plays, 1 as it was made, negative backwards.
    - `loop`: whether the clip carries on from its other end when it reaches one.
    - `time`: where the clip is, in seconds. Setting it moves the clip there, playing or not,
      and it tells its bindings on every frame while the clip plays: link a slider to it with
      `<=>`, as a binding is dropped when something else writes its property.
    - `duration` and `clips`, read only: the length of the clip in seconds, and the names of
      the clips of the model. When another clip is named, or another model loaded,
      `duration` and `time` are told in the order that keeps the slider below, limited to
      the first and linked to the second, inside its range: `time` first when the clip got
      shorter.

    @code
    Model3D {
        id: robot; source: "models/robot.glb";
        clip: "Walk"; speed: 1.5; loop: true; playing: run.checked;
        onClipEnded: action ("stopped");
    }

    Slider { value <=> robot.time; maximum: robot.duration; }
    @endcode

    They are what the model was asked, kept when it is instantiated again: once its file
    changed, or its node got another entity, the new instance plays the same clip from where the
    one before was. Another `source` or `scene` is another model, which starts over: its clip is
    where it starts, also when the clip of the one before had ended.

    While a clip plays the SceneComponent updates its scene and repaints on every display frame,
    once whatever the number of models playing, and nothing runs per frame once none does. The
    root of the instance, from getInstance(), carries the AnimationNode the properties drive.

    @see SceneNodeComponent, SceneComponent::loadModel, GltfModel, AnimationNode
*/
class YUP_API SceneModelComponent : public SceneNodeComponent
{
public:
    //==============================================================================
    /** Creates a model without a source.

        @param componentID The ID of the component.
    */
    explicit SceneModelComponent (StringRef componentID = {});

    //==============================================================================
    /** Changes the file of the model, the `source` of a `Model3D`.

        The clip of another model starts over, playing when the model was asked to: `time` goes
        back to where the clip starts.

        @param newSource The path of a `.glb` or `.gltf` file, as written in the document, or an
                         empty string for no model.
    */
    void setSource (const String& newSource);

    /** Returns the path of the file of the model. */
    const String& getSource() const noexcept { return source; }

    /** Changes which scene of the file is shown, the `scene` of a `Model3D`.

        Its clip starts over, as it does for another source.

        @param newSceneIndex The index of the glTF scene, or -1 for the default one.
    */
    void setSceneIndex (int newSceneIndex);

    /** Returns the index of the scene of the file being shown, -1 for the default one. */
    int getSceneIndex() const noexcept { return sceneIndex; }

    //==============================================================================
    /** Returns the root entity of the instance of the model, a child of getEntity().

        It is named after the glTF scene, holds the entities of its nodes, and carries the
        AnimationNode of a model that has animations. It is another entity each time the model
        is instantiated: when the source, the scene or the file changes, and when the node gets
        another entity.

        @returns The entity, or an invalid entity while the node is not in a scene, while the
                 model is being loaded or when it could not be loaded.
    */
    Entity getInstance() const noexcept;

    //==============================================================================
    /** Chooses the clip the model plays, the `clip` of a `Model3D`.

        The clip is selected where it starts, its start or its end when it plays backwards,
        where the model is posed at once, and plays from there when the model is playing: also
        one that had stopped at its end.

        @param newClip The name of the clip. When no clip has that name and it is a number, it
                       is the index of the clip among those of the file, from 0. An empty string
                       is the first clip. A clip the model does not have is reported: nothing
                       plays, the model stays as it was posed, and getTime() and getDuration()
                       return 0.
    */
    void setClip (const String& newClip);

    /** Returns the clip the model was asked to play, as it was given. */
    const String& getClip() const noexcept { return clip; }

    /** Plays or pauses the clip, the `playing` of a `Model3D`.

        Playing carries on from where the clip is, where the model is posed at once. A clip that
        does not loop stops at the end it plays towards, where it stays, asked to play, until this
        is called again: playing then starts it over from the other end, as
        AnimationNode::setPlaying() does.

        @param shouldPlay True to play the clip.
    */
    void setPlaying (bool shouldPlay);

    /** Returns true if the model was asked to play, also once its clip has stopped at its end. */
    bool isPlaying() const noexcept { return playing; }

    /** Changes how fast the clip plays, the `speed` of a `Model3D`.

        @param newSpeed 1 for the speed the clip was made for, a negative speed to play backwards.
                        A speed that is not a finite number is ignored.
    */
    void setSpeed (float newSpeed);

    /** Returns how fast the clip plays. */
    float getSpeed() const noexcept { return speed; }

    /** Chooses whether the clip carries on from its other end when it reaches one, the `loop`
        of a `Model3D`.

        @param shouldLoop False, the default, for the clip to stop there.
    */
    void setLooping (bool shouldLoop);

    /** Returns true if the clip carries on from its other end when it reaches one. */
    bool isLooping() const noexcept { return looping; }

    /** Moves the clip to a time and poses the model there, playing or not, the `time` of a
        `Model3D`.

        @param newTime The time in seconds, limited to the clip once the model has one. A time
                       that is not a number is ignored. One given while the file of the model
                       is being loaded is where its clip is once the model is there.
    */
    void setTime (double newTime);

    /** Returns where the clip is, in seconds: 0 while the file of the model is being loaded,
        as its clip lasts nothing by then.
    */
    double getTime() const noexcept;

    /** Returns the length of the clip the model plays in seconds, 0 without one: the `duration`
        of a `Model3D`.
    */
    double getDuration() const noexcept;

    /** Returns the names of the clips of the model, in the order of its file: the `clips` of a
        `Model3D`.
    */
    StringArray getClipNames() const;

    /** Called when a clip that does not loop stops at the end it plays towards: the
        `onClipEnded` of a `Model3D`.

        The scene is being updated when it is called, and the model is posed at that end. The
        function may select another clip, which then plays, or play the same one again.
    */
    std::function<void()> onClipEnded;

    //==============================================================================
    /** Returns the static type info describing the Model3D type. */
    static const ComponentTypeInfo& getClassTypeInfo();

    /** @internal */
    const ComponentTypeInfo& getDefaultTypeInfo() const override { return getClassTypeInfo(); }

    //==============================================================================
    /** @internal Loads the model again when it is made from the file that changed. */
    void assetChanged (const String& path);
    /** @internal Returns true when a Model3D shown by a scene plays a clip: the scene is then updated on every display frame. */
    static bool isPlayingIn (Scene& scene);

protected:
    //==============================================================================
    /** @internal */
    void entityChanged() override;

private:
    //==============================================================================
    struct Playback;

    void reload (std::initializer_list<Identifier> properties);
    void instantiate();
    AnimationNode* findAnimation() const noexcept;
    void updateAnimation();
    void updatePlayback();
    void showPose();
    void clipAdvanced();
    void handleClipEnded();
    void updateClipProblem();
    void tellAnimationChanged (std::initializer_list<Identifier> properties, double previousDuration);
    void notifyChanged (std::initializer_list<Identifier> properties);

    String source;
    int sceneIndex = -1;
    Entity instance;
    bool isModelLoading = false; // The file is being loaded: the model has no instance yet
    String clip;
    double time = 0.0;
    float speed = 1.0f; // As an AnimationNode starts: at the speed of the clip, without looping
    bool looping = false;
    bool playing = false;
    bool hasEnded = false;
    SignalConnection clipEndedConnection;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SceneModelComponent)
};

} // namespace yup
