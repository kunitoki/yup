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
/** Stateful playback controller for an Animation.

    Call `advanceTime(deltaSeconds)` each tick (e.g. from a timer callback) to
    advance internal time, then call `render()` to draw the current frame.

    Supports forward, reverse, and ping-pong modes, optional looping, and an
    optional frame range clamp.

    @code
    AnimationPlayer player (Animation::loadFromFile (animFile));
    player.setLooping (true);
    player.play();

    // In your timer callback:
    if (player.advanceTime (1.0f / 60.0f))
        repaint();

    // In your paint():
    player.render (g, getLocalBounds().toFloat());
    @endcode
*/
class YUP_API AnimationPlayer
{
public:
    //==============================================================================
    enum class Direction
    {
        Forward,
        Reverse,
        PingPong
    };

    //==============================================================================
    explicit AnimationPlayer (Animation animation = {});

    //==============================================================================
    /** Replaces the current animation. Resets playback state. */
    void setAnimation (Animation animation);

    /** Returns the current animation. */
    [[nodiscard]] const Animation& getAnimation() const noexcept;

    //==============================================================================
    /** Starts playback. */
    void play();

    /** Pauses playback without resetting position. */
    void pause();

    /** Stops playback and resets to the start frame. */
    void stop();

    /** Returns true if currently playing. */
    [[nodiscard]] bool isPlaying() const noexcept;

    //==============================================================================
    /** Seek to the given frame number (clamped to the frame range). */
    void seekToFrame (float frameNo);

    /** Seek to a normalised progress in [0, 1]. */
    void seekToProgress (float progress);

    //==============================================================================
    /** Returns the current frame number. */
    [[nodiscard]] float currentFrame() const noexcept;

    /** Returns the current playback progress in [0, 1]. */
    [[nodiscard]] float currentProgress() const noexcept;

    //==============================================================================
    /** Sets the playback speed multiplier (1.0 = normal, 2.0 = double speed). */
    void setSpeed (float speed);

    /** Returns the current speed multiplier. */
    [[nodiscard]] float getSpeed() const noexcept;

    //==============================================================================
    /** Sets whether playback loops when it reaches the end. */
    void setLooping (bool shouldLoop);

    /** Returns true if looping is enabled. */
    [[nodiscard]] bool isLooping() const noexcept;

    //==============================================================================
    /** Sets the playback direction. */
    void setDirection (Direction direction);

    /** Returns the current playback direction. */
    [[nodiscard]] Direction getDirection() const noexcept;

    //==============================================================================
    /** Optionally restricts playback to [startFrame, endFrame]. Pass 0,0 to use the full range. */
    void setFrameRange (float startFrame, float endFrame);

    //==============================================================================
    /** Advances time by @p deltaSeconds.
        @return true if the current frame changed (useful for triggering repaints).
    */
    bool advanceTime (float deltaSeconds);

    //==============================================================================
    /** Renders the current frame into @p g using the given @p fitting and
        @p justification, mirroring Drawable::paint semantics. */
    void render (Graphics& g,
                 Rectangle<float> bounds,
                 Fitting fitting = Fitting::scaleToFit,
                 Justification justification = Justification::center) const;

    //==============================================================================
    /** Called when the displayed frame changes (may be called multiple times per tick
        if frames are skipped at low frame rates). */
    std::function<void (float frameNo)> onFrameChanged;

    /** Called each time the animation loops. */
    std::function<void()> onLoopCompleted;

    /** Called when playback reaches the end and looping is disabled. */
    std::function<void()> onPlaybackEnded;

private:
    void clampFrame();
    float rangeStart() const noexcept;
    float rangeEnd() const noexcept;

    Animation animation_;
    float currentFrame_ = 0.0f;
    float speed_ = 1.0f;
    float rangeStart_ = 0.0f;
    float rangeEnd_ = 0.0f; ///< 0.0 means "use composition total"
    Direction direction_ = Direction::Forward;
    bool playing_ = false;
    bool looping_ = false;
    bool pingPongForward_ = true; ///< Internal state for ping-pong
};

} // namespace yup
