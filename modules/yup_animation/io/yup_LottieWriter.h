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
/** Serialises an AnimationComposition back to Lottie JSON format.

    Uses yup::JSON and yup::var internally — no external JSON dependency.
    Output is always Lottie JSON (`.json`); ZIP packaging is not supported.
*/
class YUP_API LottieWriter
{
public:
    //==============================================================================
    /** Serialises the composition to a JSON string. */
    [[nodiscard]] static String toJson (const AnimationComposition& comp,
                                        bool prettyPrint = true);

    /** Writes the composition to a file. Returns Result::ok() on success. */
    static Result toFile (const AnimationComposition& comp,
                          const File& destination,
                          bool prettyPrint = true);

private:
    //==============================================================================
    static var serializeComposition (const AnimationComposition& comp);
    static var serializeLayers (const std::vector<AnimationLayer::Ptr>& layers);
    static var serializeLayer (const AnimationLayer& layer);
    static var serializeShapeLayer (const ShapeLayer& layer);
    static var serializeGroups (const std::vector<AnimationGroup::Ptr>& groups);
    static var serializeGroup (const AnimationGroup& group);
    static var serializeChildItem (const AnimationGroup::ChildItem& item);
    static var serializeShape (const AnimationShape& shape);
    static var serializeFill (const FillPaint& fill);
    static var serializeStroke (const StrokePaint& stroke);
    static var serializeGradient (const AnimationGradient& gradient);
    static var serializeTrim (const AnimationTrim& trim);
    static var serializeRepeater (const AnimationRepeater& repeater);
    static var serializeRoundedCorner (const AnimationRoundedCorner& rc);
    static var serializeTransform (const AnimationTransform& t);
    static var serializeMasks (const std::vector<AnimationMask::Ptr>& masks);
    static var serializeMask (const AnimationMask& mask);
    static var serializeAssets (const HashMap<String, AnimationAsset::Ptr>& assets);
    static var serializeMarkers (const std::vector<AnimationMarker>& markers);

    template <typename T>
    static var serializeProperty (const AnimationProperty<T>& prop,
                                  std::function<var (const T&)> serializer);

    static var serializeEasing (const AnimationEasing& easing);

    // Value serialisers
    static var serializeColor (const Color& c);
    static var serializePoint (const Point<float>& p);
    static var serializeSize (const Size<float>& s);
    static var serializeFloat (float v);
    static var serializePath (const AnimationPathData& pd);
};

} // namespace yup
