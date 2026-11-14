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
/**
    Runs the transitions and keyframe animations of a stylesheet.

    A StyleContext owns one animator. It parses the `transition:` and `animation:`
    declarations with parseTransitions() and parseAnimations(), starts the matching entries
    when a restyle changes a value, and advances them once per frame. The animator only
    computes values: every frame it reports, through a callback, the value each animated
    property shows, or nothing once an entry stops covering it, and the context writes them
    into the animated layer of the StyleState.

    @code
    .knob { bezelSize: 0.8; transition: bezelSize 150ms easeOut; }
    .knob:pressed { bezelSize: 0.7; }
    .alert { animation: pulse 1s easeInOut infinite alternate; }
    @keyframes pulse { from { glow: 0; } to { glow: 8; } }
    @endcode

    Numbers, integers and colors (component-wise in sRGB) are interpolated, every other value
    switches halfway.

    @see StyleContext, StyleState, StyleSheet

    @tags{UI}
*/
class YUP_API StyleAnimator
{
public:
    //==============================================================================
    /** One entry of a `transition:` declaration: `property duration easing delay`. */
    struct Transition
    {
        /** The name of the property to animate. */
        Identifier property;

        /** The length of the transition, in seconds. */
        double duration = 0.0;

        /** The time before the transition starts, in seconds. */
        double delay = 0.0;

        /** The easing curve applied to the progress. */
        Easing easing;
    };

    /** The order in which the iterations of an animation play its keyframes. */
    enum class Direction
    {
        normal,          ///< Every iteration plays from the first keyframe to the last.
        reverse,         ///< Every iteration plays from the last keyframe to the first.
        alternate,       ///< The iterations alternate, starting forwards.
        alternateReverse ///< The iterations alternate, starting backwards.
    };

    /** The values an animation shows outside of its active time. */
    enum class FillMode
    {
        none,      ///< Nothing before it starts nor after it ends.
        forwards,  ///< The last value shown is kept after it ends.
        backwards, ///< The first value is shown during the delay.
        both       ///< Both forwards and backwards.
    };

    /** One entry of an `animation:` declaration: `name duration easing delay iterations direction fill-mode`. */
    struct Animation
    {
        /** The name of the `@keyframes` block to play. */
        Identifier name;

        /** The length of one iteration, in seconds. */
        double duration = 0.0;

        /** The time before the first iteration starts, in seconds. */
        double delay = 0.0;

        /** The easing curve applied between each pair of keyframes. */
        Easing easing;

        /** The number of iterations, possibly fractional, or infinity for `infinite`. */
        double iterations = 1.0;

        /** The order in which the iterations play. */
        Direction direction = Direction::normal;

        /** The values shown before and after the active time. */
        FillMode fillMode = FillMode::none;
    };

    /** One keyframe of a track: a value at a position between 0 and 1. */
    struct Frame
    {
        double position = 0.0;
        var value;
    };

    /** The keyframes of one property of an animation, sorted by position, from 0 to 1. */
    struct Track
    {
        Identifier storageId;
        ComponentProperty::Type type = ComponentProperty::Type::number;
        std::vector<Frame> frames;
    };

    /** Receives the value an animated property shows, or nullptr when nothing animates it anymore. */
    using Callback = std::function<void (Component& component, const Identifier& storageId, const var* value)>;

    //==============================================================================
    /** Constructor. */
    StyleAnimator() = default;

    //==============================================================================
    /** Parses the text of a `transition:` declaration.

        The entries are separated by commas. Times take an `ms` or `s` unit, the first is the
        duration and the second the delay. Easings are `linear`, `easeIn`, `easeOut`,
        `easeInOut` and `cubicBezier (x1, y1, x2, y2)`. `none` declares no transition.

        @param text The declaration value, like `opacity 200ms easeOut, thumbColor 150ms`.

        @returns The entries, or the first error.
    */
    static ResultValue<std::vector<Transition>> parseTransitions (const String& text);

    /** Parses the text of an `animation:` declaration.

        Besides the times and easings of transitions, an entry takes an iteration count (a
        number or `infinite`), a direction (`normal`, `reverse`, `alternate`,
        `alternateReverse`) and a fill mode (`none`, `forwards`, `backwards`, `both`).
        `none` alone declares no animation.

        @param text The declaration value, like `pulse 1s easeInOut infinite alternate`.

        @returns The entries, or the first error.
    */
    static ResultValue<std::vector<Animation>> parseAnimations (const String& text);

    /** Interpolates between two values of a property.

        @param from     The value at progress 0.
        @param to       The value at progress 1.
        @param progress The eased progress, which may leave the 0 to 1 range for overshooting easings.
        @param type     The type of the property.

        @returns The interpolated value.
    */
    static var interpolate (const var& from, const var& to, double progress, ComponentProperty::Type type);

    //==============================================================================
    /** Starts a transition, replacing the one running on the same property.

        @param component  The component whose property changed.
        @param storageId  The storage id of the property.
        @param type       The type of the property.
        @param from       The value shown before the change.
        @param to         The new value.
        @param transition The timing to follow.
        @param now        The current time of the animator clock, in seconds.
    */
    void startTransition (Component& component,
                          const Identifier& storageId,
                          ComponentProperty::Type type,
                          const var& from,
                          const var& to,
                          const Transition& transition,
                          double now);

    /** Returns the value a running transition heads to, or nullptr when none runs on the property. */
    const var* findTransitionTarget (const Component& component, const Identifier& storageId) const;

    /** Returns the storage ids of the properties with a running transition. */
    Array<Identifier> getTransitions (const Component& component) const;

    /** Stops a transition where it is.

        @returns True if a transition was running on the property.
    */
    bool cancelTransition (const Component& component, const Identifier& storageId);

    //==============================================================================
    /** Starts an animation.

        @param component The component the animation plays on.
        @param animation The timing to follow.
        @param tracks    The values of the animated properties.
        @param now       The current time of the animator clock, in seconds.
    */
    void startAnimation (Component& component, const Animation& animation, std::vector<Track> tracks, double now);

    /** Stops the animations of a component.

        @returns The storage ids of the properties they animated.
    */
    Array<Identifier> stopAnimations (const Component& component);

    //==============================================================================
    /** Stops everything running on a component, without reporting it. */
    void removeComponent (const Component& component);

    /** Stops everything, without reporting it. */
    void clear();

    /** Returns true while a transition or an animation needs frames. */
    bool isAnimating() const noexcept;

    /** Advances everything to a time and reports the values to show.

        @param now      The current time of the animator clock, in seconds.
        @param callback Receives each animated value, or nullptr when an entry stops covering a property.
    */
    void advance (double now, const Callback& callback);

private:
    //==============================================================================
    struct RunningTransition
    {
        WeakReference<Component> component;
        Identifier storageId;
        ComponentProperty::Type type;
        var from, to;
        Transition transition;
        double start = 0.0;
    };

    struct RunningAnimation
    {
        WeakReference<Component> component;
        Animation animation;
        std::vector<Track> tracks;
        double start = 0.0;
        bool holding = false;
    };

    static var sample (const Track& track, double progress, const Easing& easing);

    std::vector<RunningTransition> transitions;
    std::vector<RunningAnimation> animations;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StyleAnimator)
};

} // namespace yup
