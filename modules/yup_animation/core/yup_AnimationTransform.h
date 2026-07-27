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
/** A spatial position keyframe with bezier tangents for curved motion paths.

    When a position property has `ti` and `to` tangent vectors, the motion path
    between consecutive keyframes follows a cubic bezier curve instead of a straight line.
*/
struct SpatialPositionKeyframe
{
    float frame = 0.0f;
    Point<float> value {};
    std::optional<Point<float>> endValue;
    Point<float> tangentIn {};  ///< "ti" - in tangent (relative to this keyframe)
    Point<float> tangentOut {}; ///< "to" - out tangent (relative to this keyframe)
    AnimationEasing easing;
};

//==============================================================================
/** Parameters for AfterEffects inertial bounce expression (amp/freq/decay).

    The expression `amp, freq, decay, n = 0` produces an exponentially-decaying
    sine-wave overshoot past the last position keyframe. Store the extracted
    parameters here so positionAt can apply them at runtime.
*/
struct InertialBounceParams
{
    float amplitude = 0.0f;  //< scale factor (Lottie: "amp = ...")
    float frequency = 0.0f;  //< Hz (Lottie: "freq = ...")
    float decay = 0.0f;      //< decay exponent (Lottie: "decay = ...")
    float frameRate = 60.0f; //< composition frame rate, for frame <-> second conversion
    float timeMax = 2.0f;    //< duration of the overshoot in seconds (Lottie: "time_max")

    /** Incoming velocity at the last keyframe (pixels per second), computed from
        the keyframe data at parse-time using the composition frame rate. */
    Point<float> velocity {};

    /** True when the parameters produce a visible bounce. */
    [[nodiscard]] bool isActive() const noexcept
    {
        return amplitude > 0.0f && frequency > 0.0f && decay > 0.0f;
    }
};

//==============================================================================
/** Bundles all Lottie transform channels as animated properties.

    Call toAffineTransform(frameNo) to collapse all channels into a single
    AffineTransform for rendering. The composition order matches Lottie:
    anchor correction → position → rotation → scale.
*/
class YUP_API AnimationTransform
{
public:
    //==============================================================================
    Vec2Property anchor { AnimationProperty<Point<float>>::staticValue ({ 0.0f, 0.0f }) };
    Vec2Property position { AnimationProperty<Point<float>>::staticValue ({ 0.0f, 0.0f }) };
    SizeProperty scale { AnimationProperty<Size<float>>::staticValue ({ 100.0f, 100.0f }) };
    FloatProperty rotation { AnimationProperty<float>::staticValue (0.0f) };
    FloatProperty opacity { AnimationProperty<float>::staticValue (100.0f) };
    FloatProperty skew { AnimationProperty<float>::staticValue (0.0f) };
    FloatProperty skewAxis { AnimationProperty<float>::staticValue (0.0f) };

    /** When true, aligns the local X axis with the instantaneous position motion path. */
    bool autoOrient = false;

    /** 3D rotation channels (Lottie "rx", "ry", "rz").
        When is3DData is true, these replace the 2D rotation for 3D layers. */
    bool is3DData = false;
    FloatProperty rotationX { AnimationProperty<float>::staticValue (0.0f) }; ///< "rx" - rotation around X axis
    FloatProperty rotationY { AnimationProperty<float>::staticValue (0.0f) }; ///< "ry" - rotation around Y axis
    FloatProperty rotationZ { AnimationProperty<float>::staticValue (0.0f) }; ///< "rz" - rotation around Z axis

    /** When true, the layer has separate X/Y position channels. */
    bool separatePosition = false;
    FloatProperty positionX { AnimationProperty<float>::staticValue (0.0f) };
    FloatProperty positionY { AnimationProperty<float>::staticValue (0.0f) };

    /** Spatial keyframes for position motion path (bezier interpolation).
        When populated, overrides the linear interpolation in position.getValueAt(). */
    std::vector<SpatialPositionKeyframe> spatialKeyframes;

    /** Inertial bounce (overshoot) applied to position past the last keyframe. */
    InertialBounceParams positionBounce;

    //==============================================================================
    /** Evaluates all channels and returns the composed AffineTransform at frameNo. */
    [[nodiscard]] AffineTransform toAffineTransform (float frameNo) const;

    /** Returns opacity in [0, 1] at the given frame. */
    [[nodiscard]] float opacityAt (float frameNo) const;

    /** Returns true when all channels are static (no keyframe animation). */
    [[nodiscard]] bool isStatic() const noexcept;

    //==============================================================================
    /** Evaluates position with spatial bezier interpolation when spatialKeyframes
        are present, otherwise uses the position property's linear interpolation. */
    [[nodiscard]] Point<float> positionAt (float frameNo) const;

private:
    /** Adds the inertial bounce overshoot to a settled position value. */
    [[nodiscard]] Point<float> applyPositionBounce (Point<float> settledValue, float frameNo) const;

    mutable std::optional<AffineTransform> cachedStaticXf;
    mutable std::vector<float> cachedSegmentLengths;
};

} // namespace yup
