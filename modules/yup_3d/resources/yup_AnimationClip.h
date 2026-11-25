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
/** A named animation: tracks of keys, each animating one property of one target over time.

    A track animates the translation, the rotation, the scale or the morph target weights of
    one target, and goes from a key to the next by holding the value, blending linearly or
    following a cubic spline, as glTF 2.0 defines them. A target is only an index: the
    AnimationNode playing the clip holds the entities the indices stand for, so clips are
    shared, and every instance of a model plays the same ones.

    @code
    auto clip = yup::AnimationClip::Ptr (new yup::AnimationClip ("rise"));

    yup::AnimationClip::Track track;
    track.path = yup::AnimationClip::Path::translation;
    track.times = { 0.0f, 2.0f };
    track.values = { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    clip->addTrack (std::move (track));

    std::array<float, 3> position {};
    clip->getTracks().front().sample (1.0f, position); // 0, 0.5, 0
    @endcode

    GltfModel makes one clip per glTF animation.

    @see AnimationNode, GltfModel
*/
class YUP_API AnimationClip : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<AnimationClip>;

    /** What a track animates on its target. */
    enum class Path
    {
        translation, ///< The position: 3 values per key.
        rotation,    ///< The rotation: 4 values per key, a quaternion as x, y, z, w.
        scale,       ///< The scale: 3 values per key.
        weights      ///< The weights of the morph targets: one value per morph target per key.
    };

    /** How a track goes from a key to the next. */
    enum class Interpolation
    {
        step,       ///< The value of a key holds until the next key.
        linear,     ///< Values blend linearly, rotations at a constant speed the shorter way round.
        cubicSpline ///< Values follow a cubic Hermite spline through the tangents of the keys.
    };

    /** The keys animating one path of one target. */
    struct YUP_API Track
    {
        /** The index of the animated entity among the targets of the AnimationNode playing the
            clip. For the clips of a GltfModel it is the index of the glTF node.
        */
        int target = 0;

        Path path = Path::translation;                       ///< What is animated.
        Interpolation interpolation = Interpolation::linear; ///< How values go from a key to the next.
        std::vector<float> times;                            ///< The time of every key in seconds, not negative and never decreasing.

        /** The values of the keys, one key after the other: getNumComponents() values per key.
            Cubic spline keys take three times as many: the in-tangent, the value and the
            out-tangent, tangents being changes of the value per second.
        */
        std::vector<float> values;

        /** Returns the number of values sample() writes: 3 for a translation or a scale, 4 for
            a rotation, and for weights the number of morph targets, which is the number of
            values each key has.
        */
        int getNumComponents() const noexcept;

        /** Writes the value of the track at a time.

            The value of the first key is used before it and the value of the last key after it.
            Rotations are always written as unit quaternions.

            @param time    The time in seconds.
            @param output  Where the getNumComponents() values are written: it must hold at
                           least as many, and nothing is written if it doesn't or if the track
                           is inconsistent, see AnimationClip::addTrack().
        */
        void sample (float time, Span<float> output) const noexcept;
    };

    //==============================================================================
    /** Creates a clip without tracks.

        @param clipName The name of the clip.
    */
    explicit AnimationClip (const String& clipName = {});

    //==============================================================================
    /** Returns the name of the clip. */
    const String& getName() const noexcept { return name; }

    //==============================================================================
    /** Adds a track, if it can be played.

        A track needs a target that is not negative, at least one key, times that are finite,
        not negative and never decreasing, and as many values as its keys take.

        @param track The track to add.

        @return A failure telling what is wrong with the track, which is not added then.
    */
    Result addTrack (Track track);

    /** Returns the tracks, in the order they were added. */
    Span<const Track> getTracks() const noexcept { return tracks; }

    /** Returns the length of the clip in seconds: the time of the last key of its tracks. */
    float getDuration() const noexcept { return duration; }

private:
    String name;
    std::vector<Track> tracks;
    float duration = 0.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnimationClip)
};

} // namespace yup
