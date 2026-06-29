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
// AnimationPlayer

AnimationPlayer::AnimationPlayer (Animation animation)
    : animation_ (std::move (animation))
{
    currentFrame_ = rangeStart();
}

void AnimationPlayer::setAnimation (Animation animation)
{
    animation_ = std::move (animation);
    stop();
}

const Animation& AnimationPlayer::getAnimation() const noexcept
{
    return animation_;
}

//==============================================================================

void AnimationPlayer::play()
{
    playing_ = true;
}

void AnimationPlayer::pause()
{
    playing_ = false;
}

void AnimationPlayer::stop()
{
    playing_ = false;
    currentFrame_ = rangeStart();
    pingPongForward_ = true;
}

bool AnimationPlayer::isPlaying() const noexcept
{
    return playing_;
}

//==============================================================================

void AnimationPlayer::seekToFrame (float frameNo)
{
    currentFrame_ = jlimit (rangeStart(), rangeEnd(), frameNo);
}

void AnimationPlayer::seekToProgress (float progress)
{
    const float start = rangeStart();
    const float end = rangeEnd();
    currentFrame_ = start + jlimit (0.0f, 1.0f, progress) * (end - start);
}

//==============================================================================

float AnimationPlayer::currentFrame() const noexcept
{
    return currentFrame_;
}

float AnimationPlayer::currentProgress() const noexcept
{
    const float start = rangeStart();
    const float end = rangeEnd();
    if (end <= start)
        return 0.0f;
    return (currentFrame_ - start) / (end - start);
}

//==============================================================================

void AnimationPlayer::setSpeed (float speed)
{
    speed_ = jmax (0.0001f, speed);
}

float AnimationPlayer::getSpeed() const noexcept
{
    return speed_;
}

void AnimationPlayer::setLooping (bool shouldLoop)
{
    looping_ = shouldLoop;
}

bool AnimationPlayer::isLooping() const noexcept
{
    return looping_;
}

void AnimationPlayer::setDirection (Direction direction)
{
    direction_ = direction;
}

AnimationPlayer::Direction AnimationPlayer::getDirection() const noexcept
{
    return direction_;
}

void AnimationPlayer::setFrameRange (float startFrame, float endFrame)
{
    rangeStart_ = startFrame;
    rangeEnd_ = endFrame;
    clampFrame();
}

//==============================================================================

bool AnimationPlayer::advanceTime (float deltaSeconds)
{
    if (! playing_ || ! animation_.isValid())
        return false;

    const float fps = animation_.frameRate();
    const float frameDelta = deltaSeconds * fps * speed_;
    const float start = rangeStart();
    const float end = rangeEnd();
    const float endExclusive = end + 1.0f;
    const float prevFrame = currentFrame_;
    const float duration = endExclusive - start;

    auto wrapFrame = [start, duration] (float frame) noexcept
    {
        if (duration <= 1.0e-6f)
            return start;

        const float wrapped = std::fmod (frame - start, duration);
        return start + (wrapped < 0.0f ? wrapped + duration : wrapped);
    };

    auto advanceForward = [&]()
    {
        currentFrame_ += frameDelta;
        if (currentFrame_ > end)
        {
            if (looping_)
            {
                if (duration > 1.0e-6f && currentFrame_ >= endExclusive)
                {
                    currentFrame_ = wrapFrame (currentFrame_);
                    if (onLoopCompleted)
                        onLoopCompleted();
                }
            }
            else
            {
                currentFrame_ = end;
                playing_ = false;
                if (onPlaybackEnded)
                    onPlaybackEnded();
            }
        }
    };

    auto advanceReverse = [&]()
    {
        currentFrame_ -= frameDelta;
        if (currentFrame_ < start)
        {
            if (looping_)
            {
                currentFrame_ = wrapFrame (currentFrame_);
                if (onLoopCompleted)
                    onLoopCompleted();
            }
            else
            {
                currentFrame_ = start;
                playing_ = false;
                if (onPlaybackEnded)
                    onPlaybackEnded();
            }
        }
    };

    switch (direction_)
    {
        case Direction::Forward:
            advanceForward();
            break;

        case Direction::Reverse:
            advanceReverse();
            break;

        case Direction::PingPong:
            if (pingPongForward_)
            {
                currentFrame_ += frameDelta;
                if (currentFrame_ > end)
                {
                    currentFrame_ = end;
                    pingPongForward_ = false;
                    if (onLoopCompleted)
                        onLoopCompleted();
                }
            }
            else
            {
                currentFrame_ -= frameDelta;
                if (currentFrame_ < start)
                {
                    currentFrame_ = start;
                    pingPongForward_ = true;
                    if (onLoopCompleted)
                        onLoopCompleted();
                }
            }
            break;
    }

    const bool frameChanged = (currentFrame_ != prevFrame);
    if (frameChanged && onFrameChanged)
        onFrameChanged (currentFrame_);

    return frameChanged;
}

void AnimationPlayer::render (Graphics& g, Rectangle<float> bounds, bool keepAspectRatio) const
{
    animation_.renderFrame (g, currentFrame_, bounds, keepAspectRatio);
}

//==============================================================================

void AnimationPlayer::clampFrame()
{
    currentFrame_ = jlimit (rangeStart(), rangeEnd(), currentFrame_);
}

float AnimationPlayer::rangeStart() const noexcept
{
    if (! animation_.isValid())
        return 0.0f;

    if (rangeStart_ > 0.0f || rangeEnd_ > 0.0f)
        return rangeStart_;

    if (const auto* comp = animation_.getComposition())
        return comp->startFrame;

    return 0.0f;
}

float AnimationPlayer::rangeEnd() const noexcept
{
    if (! animation_.isValid())
        return 0.0f;

    if (rangeEnd_ > 0.0f)
        return rangeEnd_;

    if (const auto* comp = animation_.getComposition())
        return comp->endFrame > comp->startFrame ? comp->endFrame - 1.0f
                                                 : comp->endFrame;

    return animation_.totalFrames();
}

} // namespace yup
