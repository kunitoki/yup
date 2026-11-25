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
/** A part that plays animation clips on the entities of a model instance.

    The node holds the clips of a model and the target entities their tracks refer to by index,
    and plays one clip at a time. While it is playing, Scene::update() advances its time and
    poses the targets: every track writes the translation, the rotation or the scale it animates
    into the TransformNode of its target, or the weights of morph targets into its DeformNode,
    if it has one. What the clip doesn't animate keeps the value it has, and targets that were
    destroyed are skipped. An update in which no time goes by, with a delta or a speed of 0,
    poses nothing: what was written to a target since the clip posed it stays until the clip
    moves on.

    GltfModel attaches one to the root entity of every instance of a model that has animations,
    with the first clip selected and paused, so the instance stays as it was built until asked:

    @code
    yup::Entity instance = model.instantiate (*scene);

    if (auto* animation = instance.get<yup::AnimationNode>())
    {
        animation->setCurrentClip (animation->findClip ("Open"));
        animation->setPlaying (true);
    }

    scene->update (deltaSeconds); // on every frame
    @endcode

    Selecting a clip or setting the time poses the targets at once, also while paused, so a clip
    can be scrubbed. A clip that doesn't loop stops at the end it plays towards, which onClipEnded()
    tells, and playing it again starts it over from the other end.

    Meshes that are skinned or morphed follow the pose when their DeformNode is updated, which
    Scene::update() does after the AnimationNodes: a clip scrubbed while nothing updates the
    scene shows on them after a Scene::update (0.0).

    @see AnimationClip, GltfModel, TransformNode, DeformNode
*/
class YUP_API AnimationNode : public Node
{
public:
    //==============================================================================
    /** Creates a paused node, with the first clip selected.

        @param clipsToPlay     The clips, shared with the other instances of the model.
        @param targetEntities  The entities the clips animate: a track whose target is N animates
                               the entity at index N. A track whose target is out of range or
                               not valid is skipped.
    */
    AnimationNode (std::vector<AnimationClip::Ptr> clipsToPlay, std::vector<Entity> targetEntities);

    //==============================================================================
    /** Returns the number of clips. */
    int getNumClips() const noexcept { return static_cast<int> (clips.size()); }

    /** Returns a clip, or nullptr if the index is out of range. */
    AnimationClip::Ptr getClip (int index) const noexcept;

    /** Returns the index of the first clip with a name, or -1 if there is none. */
    int findClip (StringRef clipName) const noexcept;

    //==============================================================================
    /** Selects the clip to play, from its start, and poses the targets there at once.

        A node that is playing carries on with the new clip.

        @param index The index of the clip. Out of range, -1 for example, no clip is selected:
                     nothing plays and the targets keep the pose they have.
    */
    void setCurrentClip (int index);

    /** Returns the index of the selected clip, or -1 if there is none. */
    int getCurrentClip() const noexcept { return currentClip; }

    /** Returns the length of the selected clip in seconds, or 0 if there is none. */
    double getDuration() const noexcept;

    //==============================================================================
    /** Moves to a time of the selected clip and poses the targets there at once, playing or not.

        @param newTime The time in seconds, limited to the range from 0 to getDuration().
    */
    void setTime (double newTime);

    /** Returns the time reached in the selected clip, in seconds. */
    double getTime() const noexcept { return time; }

    //==============================================================================
    /** Changes how fast the clip plays: 1, the default, is the speed it was made for, 2 is twice
        as fast and a negative speed plays backwards.
    */
    void setSpeed (float newSpeed) noexcept { speed = newSpeed; }

    /** Returns how fast the clip plays. */
    float getSpeed() const noexcept { return speed; }

    /** Chooses what happens at the end the clip plays towards: looping it carries on from the
        other end, else it stops there, which is the default.
    */
    void setLooping (bool shouldLoop) noexcept { looping = shouldLoop; }

    /** Returns true if the clip carries on from its other end when it reaches one. */
    bool isLooping() const noexcept { return looping; }

    /** Plays or pauses the selected clip.

        Playing carries on from the time the clip is at, unless the clip is stopped at the end
        it would run into: its end with a positive speed, its start with a negative one. The
        time then goes to the other end first, so a clip that has ended, or that was scrubbed
        to its end, plays again as in a media player. Nothing starts over for a looping clip,
        for a clip that is playing already or with a speed of 0.

        A clip that starts playing poses the targets at once, where it starts.

        The speed that counts is the one set when this is called, so set it before: a clip at
        its start that is turned backwards once it plays stops there at the next update, while
        a node that never played, given a negative speed first, plays its clip from the end.
    */
    void setPlaying (bool shouldPlay);

    /** Returns true while the clip plays: false once paused, or stopped at an end. */
    bool isPlaying() const noexcept { return playing; }

    //==============================================================================
    /** Emitted when a clip that doesn't loop stops at its end, or at its start playing backwards.

        It is emitted once, by the update that reaches the end, after the targets were posed and
        with isPlaying() already false. Like the other signals of a scene it calls back on the
        thread updating the scene, the message thread, and the callback may select another clip,
        play again or destroy the entity.

        @code
        ended = animation->onClipEnded().connect ([this] { lidOpened(); });
        @endcode
    */
    Signal<>& onClipEnded() noexcept { return clipEndedSignal; }

    //==============================================================================
    /** Returns the weights of the morph targets of a target, as the last weights track posing it
        left them, or nothing if no weights track ever posed it.

        @param target The index of the target, as in AnimationClip::Track::target.

        @return The weights, which stay there until the targets are posed again.
    */
    Span<const float> getWeights (int target) const noexcept;

    //==============================================================================
    /** Advances the time while playing and poses the targets, called by Scene::update().

        @param deltaSeconds The time elapsed since the previous update. With 0 nothing is posed.
    */
    void update (double deltaSeconds) override;

private:
    void applyPose();

    std::vector<AnimationClip::Ptr> clips;
    std::vector<Entity> targets;
    std::unordered_map<int, std::vector<float>> weights;
    Signal<> clipEndedSignal;
    int currentClip = -1;
    double time = 0.0;
    float speed = 1.0f;
    bool looping = false;
    bool playing = false;
};

} // namespace yup
