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
/** Lerp helper — specialize for types that need custom interpolation. */
template <typename T>
struct AnimationLerp
{
    [[nodiscard]] static T lerp (const T& a, const T& b, float t)
    {
        return a + static_cast<T> ((b - a) * t);
    }
};

template <>
struct AnimationLerp<float>
{
    [[nodiscard]] static float lerp (float a, float b, float t)
    {
        return a + (b - a) * t;
    }
};

template <>
struct AnimationLerp<Point<float>>
{
    [[nodiscard]] static Point<float> lerp (const Point<float>& a,
                                            const Point<float>& b,
                                            float t)
    {
        return { a.getX() + (b.getX() - a.getX()) * t,
                 a.getY() + (b.getY() - a.getY()) * t };
    }
};

template <>
struct AnimationLerp<Size<float>>
{
    [[nodiscard]] static Size<float> lerp (const Size<float>& a,
                                           const Size<float>& b,
                                           float t)
    {
        return { a.getWidth() + (b.getWidth() - a.getWidth()) * t,
                 a.getHeight() + (b.getHeight() - a.getHeight()) * t };
    }
};

template <>
struct AnimationLerp<Color>
{
    [[nodiscard]] static Color lerp (const Color& a, const Color& b, float t)
    {
        const float ia = a.getAlpha() / 255.0f;
        const float ib = b.getAlpha() / 255.0f;
        const float ir = a.getRed() / 255.0f;
        const float ig = a.getGreen() / 255.0f;
        const float ibl = a.getBlue() / 255.0f;

        return Color::fromRGBA (
            static_cast<uint8> ((ir + (b.getRed() / 255.0f - ir) * t) * 255.0f),
            static_cast<uint8> ((ig + (b.getGreen() / 255.0f - ig) * t) * 255.0f),
            static_cast<uint8> ((ibl + (b.getBlue() / 255.0f - ibl) * t) * 255.0f),
            static_cast<uint8> ((ia + (ib - ia) * t) * 255.0f));
    }
};

//==============================================================================
/** A single keyframe storing a frame position, value, and easing to the next keyframe. */
template <typename T>
struct AnimationKeyframe
{
    float frame = 0.0f;
    T value {};
    std::optional<T> endValue;
    AnimationEasing easing;
};

//==============================================================================
/** Generic animated property that holds either a static value or a list of keyframes.

    When static, getValueAt() returns the stored constant regardless of frame.
    When animated, getValueAt() finds the active keyframe interval and interpolates
    using the easing defined on the earlier keyframe.

    The builder pattern is supported via the nested Builder class.

    @tparam T  The value type. Must have an AnimationLerp<T> specialization.
*/
template <typename T>
class AnimationProperty
{
public:
    using Keyframe = AnimationKeyframe<T>;
    using KeyframeList = std::vector<Keyframe>;

    //==============================================================================
    /** Constructs a static property with a default-constructed value. */
    AnimationProperty() = default;

    /** Constructs a static property with the given value. */
    explicit AnimationProperty (T value)
        : staticVal (std::move (value))
        , animated_ (false)
    {
    }

    /** Constructs an animated property with the given keyframe list. */
    explicit AnimationProperty (KeyframeList keyframes)
        : keyframes_ (std::move (keyframes))
        , animated_ (true)
    {
        sortKeyframes();
    }

    AnimationProperty (const AnimationProperty&) = default;
    AnimationProperty (AnimationProperty&&) noexcept = default;
    AnimationProperty& operator= (const AnimationProperty&) = default;
    AnimationProperty& operator= (AnimationProperty&&) noexcept = default;

    //==============================================================================
    /** Returns true when this property has a single constant value. */
    [[nodiscard]] bool isStatic() const noexcept { return ! animated_; }

    /** Returns true when this property has keyframe animation. */
    [[nodiscard]] bool isAnimated() const noexcept { return animated_; }

    /** Returns the static value. Only valid when isStatic() is true. */
    [[nodiscard]] const T& getStaticValue() const
    {
        jassert (isStatic());
        return staticVal;
    }

    /** Returns the keyframe list. Only valid when isAnimated() is true. */
    [[nodiscard]] const KeyframeList& getKeyframes() const
    {
        jassert (isAnimated());
        return keyframes_;
    }

    //==============================================================================
    /** Returns the interpolated value at the given composition frame number. */
    [[nodiscard]] T getValueAt (float frameNo) const
    {
        if (isStatic())
            return staticVal;

        if (keyframes_.empty())
            return T {};

        if (frameNo <= keyframes_.front().frame)
            return keyframes_.front().value;

        if (frameNo >= keyframes_.back().frame)
            return keyframes_.back().endValue.value_or (keyframes_.back().value);

        // Binary search for the active keyframe interval
        int lo = 0;
        int hi = static_cast<int> (keyframes_.size()) - 2;

        while (lo < hi)
        {
            const int mid = (lo + hi + 1) / 2;
            if (keyframes_[mid].frame <= frameNo)
                lo = mid;
            else
                hi = mid - 1;
        }

        const Keyframe& k0 = keyframes_[lo];
        const Keyframe& k1 = keyframes_[lo + 1];

        if (k0.easing.isHold())
            return k0.value;

        const float span = k1.frame - k0.frame;
        if (span < 1e-6f)
            return k0.endValue.value_or (k1.value);

        const float t = k0.easing.evaluate ((frameNo - k0.frame) / span);
        return AnimationLerp<T>::lerp (k0.value, k0.endValue.has_value() ? *k0.endValue : k1.value, t);
    }

    //==============================================================================
    /** Fluent setter — sets a static value, clearing any keyframes. */
    AnimationProperty& withStaticValue (T value)
    {
        staticVal = std::move (value);
        keyframes_.clear();
        animated_ = false;
        return *this;
    }

    /** Adds a keyframe. Switches to animated mode if not already. */
    AnimationProperty& addKeyframe (float frame, T value, AnimationEasing easing = AnimationEasing::linear())
    {
        animated_ = true;
        keyframes_.push_back ({ frame, std::move (value), std::nullopt, easing });
        sortKeyframes();
        return *this;
    }

    /** Adds a keyframe with an explicit interval end value. */
    AnimationProperty& addKeyframe (float frame, T value, T endValue, AnimationEasing easing = AnimationEasing::linear())
    {
        animated_ = true;
        keyframes_.push_back ({ frame, std::move (value), std::move (endValue), easing });
        sortKeyframes();
        return *this;
    }

    //==============================================================================
    /** Returns a static AnimationProperty<T>. */
    [[nodiscard]] static AnimationProperty<T> staticValue (T value)
    {
        return AnimationProperty<T> (std::move (value));
    }

    //==============================================================================
    /** Fluent builder for animated properties. */
    class Builder
    {
    public:
        Builder& keyframe (float frame, T value, AnimationEasing easing = AnimationEasing::linear())
        {
            keyframes_.push_back ({ frame, std::move (value), std::nullopt, std::move (easing) });
            return *this;
        }

        Builder& keyframe (float frame, T value, T endValue, AnimationEasing easing = AnimationEasing::linear())
        {
            keyframes_.push_back ({ frame, std::move (value), std::move (endValue), std::move (easing) });
            return *this;
        }

        [[nodiscard]] AnimationProperty<T> build()
        {
            return AnimationProperty<T> (std::move (keyframes_));
        }

    private:
        KeyframeList keyframes_;
    };

    /** Returns a Builder to construct an animated property. */
    [[nodiscard]] static Builder animated() { return Builder {}; }

private:
    void sortKeyframes()
    {
        std::sort (keyframes_.begin(), keyframes_.end(), [] (const Keyframe& a, const Keyframe& b)
        {
            return a.frame < b.frame;
        });
    }

    T staticVal {};
    KeyframeList keyframes_;
    bool animated_ = false;
};

//==============================================================================
// Convenience type aliases

using FloatProperty = AnimationProperty<float>;
using Vec2Property = AnimationProperty<Point<float>>;
using SizeProperty = AnimationProperty<Size<float>>;
using ColorProperty = AnimationProperty<Color>;

} // namespace yup
