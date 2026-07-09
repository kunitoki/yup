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
/** Property keys useable for runtime overrides via KeyPath matching.

    Corresponds to rlottie's Property enum.
*/
enum class AnimationPropertyID
{
    FillColor,
    FillOpacity,
    StrokeColor,
    StrokeOpacity,
    StrokeWidth,
    TrPosition,
    TrRotation,
    TrScale,
    TrOpacity,
    TrAnchor,
    TrimStart,
    TrimEnd
};

//==============================================================================
/** Callback for property overrides - receives the frame number and returns
    the override value if active, or the default fallback.

    @tparam T The property value type (float, Color, Point<float>, Size<float>)
*/
template <typename T>
using AnimationPropertyOverride = std::function<std::optional<T> (float frameNo)>;

//==============================================================================
/** Parses and matches dot-separated keypaths with `*` (single glob) and
    `**` (globstar) wildcards.

    Keypaths look like `"layer_name.group_name.fill"` and are used for
    runtime property overrides and theming.

    Wildcard semantics:
    - `*` matches any single path component
    - `**` matches zero or more path components (globstar)

    Example keypaths:
    - `"**"` - matches everything
    - `"*.Stroke 1.Color"` - matches "Stroke 1.Color" on any layer
    - `"Shape Layer 1.**"` - matches everything under "Shape Layer 1"
*/
class YUP_API KeyPath
{
public:
    //==============================================================================
    /** Constructs a KeyPath from a dot-separated string. */
    explicit KeyPath (const String& keyPath);

    /** Returns the number of path components. */
    [[nodiscard]] size_t size() const noexcept { return keys.size(); }

    /** Returns true if this keypath matches the given component at @p depth. */
    [[nodiscard]] bool matchesComponent (const String& key, size_t depth) const noexcept;

    /** Returns the next depth after matching the given component.
        Handles globstar semantics (globstar doesn't advance depth unless
        the next literal component matches). */
    [[nodiscard]] size_t nextDepth (const String& key, size_t depth) const noexcept;

    /** Returns true when the current depth fully resolves to the target. */
    [[nodiscard]] bool fullyResolvesTo (const String& key, size_t depth) const noexcept;

    /** Returns true when the keypath should continue propagating into children. */
    [[nodiscard]] bool propagate (const String& key, size_t depth) const noexcept;

private:
    [[nodiscard]] bool isGlobstar (size_t depth) const noexcept
    {
        return depth < keys.size() && keys[depth] == "**";
    }

    [[nodiscard]] bool isGlob (size_t depth) const noexcept
    {
        return depth < keys.size() && keys[depth] == "*";
    }

    [[nodiscard]] bool endsWithGlobstar() const noexcept
    {
        return ! keys.empty() && keys.back() == "**";
    }

    std::vector<String> keys;
};

//==============================================================================
/** Holds property override callbacks for a single matched keypath target.

    Each entry maps an AnimationPropertyID to a typed callback. The callback
    returns std::nullopt when it doesn't want to override, or the override value.
*/
class YUP_API PropertyOverrideSet
{
public:
    //==============================================================================
    /** Sets a float override (used for opacity, width, trim, etc.). */
    void setFloatOverride (AnimationPropertyID id, AnimationPropertyOverride<float> callback);

    /** Sets a color override. */
    void setColorOverride (AnimationPropertyID id, AnimationPropertyOverride<Color> callback);

    /** Sets a point override (used for position). */
    void setPointOverride (AnimationPropertyID id, AnimationPropertyOverride<Point<float>> callback);

    /** Sets a size override (used for scale). */
    void setSizeOverride (AnimationPropertyID id, AnimationPropertyOverride<Size<float>> callback);

    /** Returns true if there is an override registered for the given property. */
    [[nodiscard]] bool hasOverride (AnimationPropertyID id) const noexcept;

    /** Evaluates the float override at the given frame, or returns the fallback. */
    [[nodiscard]] float evaluateFloat (AnimationPropertyID id, float frameNo, float fallback) const;

    /** Evaluates the color override at the given frame, or returns the fallback. */
    [[nodiscard]] Color evaluateColor (AnimationPropertyID id, float frameNo, const Color& fallback) const;

    /** Evaluates the point override, or returns the fallback. */
    [[nodiscard]] Point<float> evaluatePoint (AnimationPropertyID id, float frameNo, const Point<float>& fallback) const;

    /** Evaluates the size override, or returns the fallback. */
    [[nodiscard]] Size<float> evaluateSize (AnimationPropertyID id, float frameNo, const Size<float>& fallback) const;

private:
    struct TypedOverride
    {
        AnimationPropertyID id;
        std::function<std::optional<float> (float)> floatFunc;
        std::function<std::optional<Color> (float)> colorFunc;
        std::function<std::optional<Point<float>> (float)> pointFunc;
        std::function<std::optional<Size<float>> (float)> sizeFunc;
    };

    TypedOverride* findOrCreate (AnimationPropertyID id);
    const TypedOverride* find (AnimationPropertyID id) const noexcept;

    std::vector<TypedOverride> overrides;
};

} // namespace yup
