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

AnimationNode::AnimationNode (std::vector<AnimationClip::Ptr> clipsToPlay, std::vector<Entity> targetEntities)
    : clips (std::move (clipsToPlay))
    , targets (std::move (targetEntities))
    , restPoses (targets.size())
{
    const auto first = std::find_if (clips.begin(), clips.end(), [] (const AnimationClip::Ptr& clip)
    {
        return clip != nullptr;
    });

    if (first != clips.end())
        currentClip = static_cast<int> (first - clips.begin());

    for (size_t index = 0; index < targets.size(); ++index)
    {
        auto& rest = restPoses[index];

        if (const auto* transform = targets[index].get<TransformNode>())
            rest = { transform->getPosition(), transform->getRotation(), transform->getScale(), {} };

        if (const auto* deform = targets[index].get<DeformNode>())
            rest.weights.assign (deform->getWeights().begin(), deform->getWeights().end());
    }
}

//==============================================================================

AnimationClip::Ptr AnimationNode::getClip (int index) const noexcept
{
    return isPositiveAndBelow (index, getNumClips()) ? clips[static_cast<size_t> (index)] : AnimationClip::Ptr();
}

int AnimationNode::findClip (StringRef clipName) const noexcept
{
    for (int index = 0; index < getNumClips(); ++index)
    {
        const auto& clip = clips[static_cast<size_t> (index)];
        if (clip != nullptr && clip->getName() == clipName)
            return index;
    }

    return -1;
}

//==============================================================================

void AnimationNode::setCurrentClip (int index)
{
    currentClip = getClip (index) != nullptr ? index : -1;
    playing = playing && currentClip >= 0;
    setTime (0.0);
}

double AnimationNode::getDuration() const noexcept
{
    const auto clip = getClip (currentClip);
    return clip != nullptr ? clip->getDuration() : 0.0;
}

void AnimationNode::setTime (double newTime)
{
    if (std::isnan (newTime))
    {
        jassertfalse; // A time that is not a number has no place in a clip
        return;
    }

    time = jlimit (0.0, getDuration(), newTime);
    applyPose();
}

void AnimationNode::setSpeed (float newSpeed) noexcept
{
    if (! std::isfinite (newSpeed))
    {
        jassertfalse; // A speed that is not finite would leave the clip without a time
        return;
    }

    speed = newSpeed;
}

void AnimationNode::setPlaying (bool shouldPlay)
{
    // Without a clip there is nothing to play
    shouldPlay = shouldPlay && currentClip >= 0;

    const auto duration = getDuration();
    const auto isAtTheEndAhead = speed > 0.0f ? time >= duration : speed < 0.0f && time <= 0.0;

    // Stopped at the end it would run into, a clip starts over from the other one, as in a media player. Its targets are
    // posed where it starts either way: an update in which no time goes by leaves them alone
    if (shouldPlay && ! playing)
        setTime (! looping && isAtTheEndAhead ? (speed > 0.0f ? 0.0 : duration) : time);

    playing = shouldPlay;
}

void AnimationNode::resetPose()
{
    for (int index = 0; index < static_cast<int> (targets.size()); ++index)
    {
        for (const auto path : { AnimationClip::Path::translation, AnimationClip::Path::rotation, AnimationClip::Path::scale, AnimationClip::Path::weights })
            resetPath (index, path);
    }

    posedClip = -1;
}

Span<const float> AnimationNode::getWeights (int target) const noexcept
{
    const auto it = weights.find (target);
    return it != weights.end() ? Span<const float> (it->second) : Span<const float>();
}

//==============================================================================

void AnimationNode::update (double deltaSeconds)
{
    if (! playing || currentClip < 0)
        return;

    const auto duration = getDuration();
    const auto step = deltaSeconds * speed;

    // The clip is where it posed its targets: what was written to them since stays
    if (step == 0.0)
        return;

    if (! std::isfinite (step))
    {
        jassertfalse; // A step that is not finite would leave a looping clip without a time
        return;
    }

    auto newTime = time + step;

    if (looping && duration > 0.0)
    {
        newTime = std::fmod (newTime, duration);

        if (newTime < 0.0)
            newTime += duration;
    }

    const auto ended = ! looping && (step > 0.0 ? newTime >= duration : step < 0.0 && newTime <= 0.0);

    setTime (newTime);

    if (! ended)
        return;

    // Last of all: a callback may select another clip, play again or destroy the entity
    playing = false;
    clipEndedSignal.emit();
}

void AnimationNode::applyPose()
{
    const auto clip = getClip (currentClip);
    if (clip == nullptr)
        return;

    if (posedClip != currentClip)
    {
        // Nothing of the clip that posed the targets before is left
        if (const auto previous = getClip (posedClip); previous != nullptr)
        {
            for (const auto& track : previous->getTracks())
                resetPath (track.target, track.path);
        }

        posedClip = currentClip;
    }

    const auto sampleTime = static_cast<float> (time);

    for (const auto& track : clip->getTracks())
    {
        if (! isPositiveAndBelow (track.target, static_cast<int> (targets.size())))
            continue;

        auto& target = targets[static_cast<size_t> (track.target)];
        if (! target.isValid())
            continue;

        if (track.path == AnimationClip::Path::weights)
        {
            auto& targetWeights = weights[track.target];
            targetWeights.resize (static_cast<size_t> (track.getNumComponents()));
            track.sample (sampleTime, targetWeights);

            // The mesh they morph takes them as a TransformNode takes the other paths: when posed only
            if (auto* deform = target.get<DeformNode>())
                deform->setWeights (targetWeights);

            continue;
        }

        std::array<float, 4> value {};
        track.sample (sampleTime, value);

        auto& transform = target.getOrAttach<TransformNode>();

        if (track.path == AnimationClip::Path::translation)
            transform.setPosition ({ value[0], value[1], value[2] });
        else if (track.path == AnimationClip::Path::rotation)
            transform.setRotation ({ value[0], value[1], value[2], value[3] });
        else
            transform.setScale ({ value[0], value[1], value[2] });
    }
}

void AnimationNode::resetPath (int index, AnimationClip::Path path)
{
    if (! isPositiveAndBelow (index, static_cast<int> (targets.size())))
        return;

    const auto& target = targets[static_cast<size_t> (index)];
    const auto& rest = restPoses[static_cast<size_t> (index)];

    if (path == AnimationClip::Path::weights)
    {
        weights.erase (index);

        if (auto* deform = target.get<DeformNode>())
            deform->setWeights (rest.weights);

        return;
    }

    auto* transform = target.get<TransformNode>();
    if (transform == nullptr)
        return;

    if (path == AnimationClip::Path::translation)
        transform->setPosition (rest.position);
    else if (path == AnimationClip::Path::rotation)
        transform->setRotation (rest.rotation);
    else
        transform->setScale (rest.scale);
}

} // namespace yup
