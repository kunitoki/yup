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

    The node keeps the rest pose of its targets: the translation, the rotation, the scale and the
    weights of the morph targets each one has when the node is created, which for a model are
    the ones of its file. What a clip animated goes back to it when another clip is selected, so
    nothing of a clip is left once the next one poses the targets, and resetPose() puts every
    target back.

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
    /** Creates a paused node, with its first clip selected.

        @param clipsToPlay     The clips, shared with the other instances of the model. An index
                               can have no clip, nullptr, as the one of an animation of a model
                               that can't be played: it is never selected.
        @param targetEntities  The entities the clips animate: a track whose target is N animates
                               the entity at index N. A track whose target is out of range or
                               not valid is skipped. The pose they are in is their rest pose,
                               see resetPose(): give them their TransformNode and their
                               DeformNode first.
    */
    AnimationNode (std::vector<AnimationClip::Ptr> clipsToPlay, std::vector<Entity> targetEntities);

    //==============================================================================
    /** Returns the number of clips, the indices without a clip included. */
    int getNumClips() const noexcept { return static_cast<int> (clips.size()); }

    /** Returns a clip, or nullptr if the index is out of range or has no clip. */
    AnimationClip::Ptr getClip (int index) const noexcept;

    /** Returns the index of the first clip with a name, or -1 if there is none. */
    int findClip (StringRef clipName) const noexcept;

    /** Returns the entities the clips animate, as the node was given them: a track whose target
        is N animates the one at index N. An entity that was destroyed since is still listed, and
        is not valid.

        An entity holds one AnimationNode, which plays one clip at a time: to play another clip
        beside it, give the same entities to a second node, on another entity. Each node puts
        back what its own clips animated.

        @code
        const auto targets = animation->getTargets();

        auto& fan = instance.createChild ("fan").attach<yup::AnimationNode> (
            std::vector<yup::AnimationClip::Ptr> { animation->getClip (animation->findClip ("Fan")) },
            std::vector<yup::Entity> (targets.begin(), targets.end()));
        @endcode
    */
    Span<const Entity> getTargets() const noexcept { return targets; }

    //==============================================================================
    /** Selects the clip to play, from its start, and poses the targets there at once.

        The values animated by the clip that posed the targets before go back to their rest pose
        first, see resetPose(), so only the selected clip shows. What no clip of the node posed
        stays as it is: the pose of the file, or what code wrote.

        A node that is playing carries on with the new clip.

        @param index The index of the clip. Out of range, -1 for example, or without a clip, no
                     clip is selected: nothing plays, isPlaying() turns false and the targets
                     keep the pose they have.
    */
    void setCurrentClip (int index);

    /** Returns the index of the selected clip, or -1 if there is none. */
    int getCurrentClip() const noexcept { return currentClip; }

    /** Returns the length of the selected clip in seconds, or 0 if there is none. */
    double getDuration() const noexcept;

    //==============================================================================
    /** Moves to a time of the selected clip and poses the targets there at once, playing or not.

        @param newTime The time in seconds, limited to the range from 0 to getDuration(). A time
                       that is not a number asserts and is ignored.
    */
    void setTime (double newTime);

    /** Returns the time reached in the selected clip, in seconds. */
    double getTime() const noexcept { return time; }

    //==============================================================================
    /** Changes how fast the clip plays: 1, the default, is the speed it was made for, 2 is twice
        as fast and a negative speed plays backwards. A speed that is not finite asserts and is
        ignored.
    */
    void setSpeed (float newSpeed) noexcept;

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

        Without a selected clip there is nothing to play: isPlaying() stays false.
    */
    void setPlaying (bool shouldPlay);

    /** Returns true while the clip plays: false once paused, stopped at an end, or without a
        selected clip.
    */
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
    /** Puts every target back in its rest pose: the translation, the rotation, the scale and the
        weights of the morph targets it had when the node was created, whatever a clip or code
        wrote since.

        A target that had no TransformNode then, and got one since, gets the identity: it is
        where it was without one. Weights only go back to a target that had its DeformNode then,
        and targets that were destroyed are skipped.

        The clip stays selected, at the time it is at: one that is playing poses what it animates
        again at the next update in which time goes by. Pause it or select no clip to keep the
        model at rest:

        @code
        animation->setCurrentClip (-1); // nothing plays, and the pose stays
        animation->resetPose();         // every target as the model was loaded
        scene->update (0.0);            // the meshes that deform follow
        @endcode

        @see setCurrentClip
    */
    void resetPose();

    /** Returns the weights of the morph targets of a target, as the last weights track posing it
        left them. There are none before a weights track poses the target, and none once it is
        put back in its rest pose.

        @param target The index of the target, as in AnimationClip::Track::target.

        @return The weights, which stay there until the targets are posed again or put back in
                their rest pose.
    */
    Span<const float> getWeights (int target) const noexcept;

    //==============================================================================
    /** Advances the time while playing and poses the targets, called by Scene::update().

        @param deltaSeconds The time elapsed since the previous update. With 0 nothing is posed,
                            and a time that is not finite asserts and is ignored.
    */
    void update (double deltaSeconds) override;

private:
    struct RestPose
    {
        Vector3<float> position;
        Quaternion rotation;
        Vector3<float> scale { 1.0f, 1.0f, 1.0f };
        std::vector<float> weights;
    };

    void applyPose();
    void resetPath (int index, AnimationClip::Path path);

    std::vector<AnimationClip::Ptr> clips;
    std::vector<Entity> targets;
    std::vector<RestPose> restPoses;
    std::unordered_map<int, std::vector<float>> weights;
    Signal<> clipEndedSignal;
    int currentClip = -1;
    int posedClip = -1; // The clip that last posed the targets
    double time = 0.0;
    float speed = 1.0f;
    bool looping = false;
    bool playing = false;
};

} // namespace yup
