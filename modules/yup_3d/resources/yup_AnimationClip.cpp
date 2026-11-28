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

int AnimationClip::Track::getNumComponents() const noexcept
{
    if (path == Path::rotation)
        return 4;

    if (path != Path::weights)
        return 3;

    // One weight per morph target: the values each key has, tangents apart
    const auto numElements = times.size() * (interpolation == Interpolation::cubicSpline ? 3u : 1u);
    return numElements > 0 ? static_cast<int> (values.size() / numElements) : 0;
}

void AnimationClip::Track::sample (float time, Span<float> output) const noexcept
{
    const auto numComponents = static_cast<size_t> (getNumComponents());
    const auto isCubic = interpolation == Interpolation::cubicSpline;
    const auto stride = isCubic ? numComponents * 3 : numComponents;

    if (times.empty() || values.size() != times.size() * stride || output.size() < numComponents)
    {
        jassertfalse; // The track is inconsistent, which AnimationClip::addTrack() refuses, or the output too small
        return;
    }

    // The key at or before the time and the key after it. They are the same key before the first
    // one, from the last one on and for steps: its value then comes out as it is
    const auto after = static_cast<size_t> (std::upper_bound (times.begin(), times.end(), time) - times.begin());
    const auto key = after > 0 ? after - 1 : 0;
    const auto next = after > 0 && after < times.size() && interpolation != Interpolation::step ? after : key;

    // A cubic spline key holds its value between its two tangents
    const auto valueOffset = isCubic ? numComponents : 0;
    const auto* from = values.data() + key * stride + valueOffset;
    const auto* to = values.data() + next * stride + valueOffset;

    const auto interval = times[next] - times[key];
    const auto t = next != key ? (time - times[key]) / interval : 0.0f;

    if (isCubic)
    {
        // glTF: out of the key along its out-tangent, into the next one along its in-tangent,
        // both scaled by the time between the two keys
        const auto* outTangent = from + numComponents;
        const auto* inTangent = to - numComponents;
        const auto t2 = t * t;
        const auto t3 = t2 * t;

        // A rotation and its opposite are the same: the next key is taken with the sign that is nearer to
        // this one, its tangent with it, as slerp() does. The spline would pass near zero between the two
        const auto isOpposite = path == Path::rotation && std::inner_product (from, from + numComponents, to, 0.0f) < 0.0f;
        const auto sign = isOpposite ? -1.0f : 1.0f;

        for (size_t i = 0; i < numComponents; ++i)
        {
            output[i] = (2.0f * t3 - 3.0f * t2 + 1.0f) * from[i]
                      + (t3 - 2.0f * t2 + t) * interval * outTangent[i]
                      + (3.0f * t2 - 2.0f * t3) * sign * to[i]
                      + (t3 - t2) * interval * sign * inTangent[i];
        }
    }
    else if (path != Path::rotation)
    {
        for (size_t i = 0; i < numComponents; ++i)
            output[i] = from[i] + (to[i] - from[i]) * t;
    }

    if (path != Path::rotation)
        return;

    // Unit quaternions whatever the keys hold: slerp() needs them, and goes the shorter way round
    const auto toRotation = [] (const float* q)
    {
        return Quaternion (q[0], q[1], q[2], q[3]).normalized();
    };

    const auto rotation = isCubic ? toRotation (output.data()) : toRotation (from).slerp (toRotation (to), t);

    output[0] = rotation.getX();
    output[1] = rotation.getY();
    output[2] = rotation.getZ();
    output[3] = rotation.getW();
}

//==============================================================================

AnimationClip::AnimationClip (const String& clipName)
    : name (clipName)
{
}

Result AnimationClip::addTrack (Track track)
{
    if (track.target < 0)
        return Result::fail ("The target of a track can't be negative");

    if (track.times.empty())
        return Result::fail ("A track needs at least one key");

    auto previous = 0.0f;

    for (const auto time : track.times)
    {
        if (! std::isfinite (time) || time < previous)
            return Result::fail ("The times of the keys must be finite, not negative and never decreasing");

        previous = time;
    }

    const auto numElements = track.times.size() * (track.interpolation == Interpolation::cubicSpline ? 3u : 1u);
    const auto numValues = numElements * static_cast<size_t> (track.getNumComponents());

    if (numValues == 0 || track.values.size() != numValues)
        return Result::fail ("The number of values (" + String (track.values.size()) + ") doesn't match the number of keys (" + String (track.times.size()) + ")");

    const auto valuesAreFinite = std::all_of (track.values.begin(), track.values.end(), [] (float value)
    {
        return std::isfinite (value);
    });

    if (! valuesAreFinite)
        return Result::fail ("The values of the keys must be finite");

    duration = jmax (duration, track.times.back());
    tracks.push_back (std::move (track));
    return Result::ok();
}

} // namespace yup
