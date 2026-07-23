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
/** Options controlling how a Lottie file is loaded. */
struct LottieLoadOptions
{
    /** Base directory used to resolve relative image paths. */
    File resourceDirectory;

    /** Optional callback to load image assets. Return {} to fall back to file I/O. */
    std::function<std::optional<Image> (const String& ref, const File& dir)> imageResolver;
};

//==============================================================================
/** Parses Lottie JSON (`.json`) and Lottie ZIP (`.lottie`) files into an AnimationComposition.

    Both formats share the same JSON parsing path after extraction.
    ZIP handling uses yup::ZipFile from yup_core.
*/
class YUP_API LottieReader
{
public:
    //==============================================================================
    /** Parses a Lottie file (either `.json` or `.lottie` ZIP).
        Returns a successful ResultValue holding the composition, or a failed
        ResultValue carrying an error message.
    */
    [[nodiscard]] static ResultValue<AnimationComposition::Ptr> parseFile (const File& file,
                                                                           const LottieLoadOptions& options = {});

    /** Parses a Lottie JSON string.
        Returns a successful ResultValue holding the composition, or a failed
        ResultValue carrying an error message.
    */
    [[nodiscard]] static ResultValue<AnimationComposition::Ptr> parseData (const String& jsonText,
                                                                           const LottieLoadOptions& options = {});

    /** Parses a Lottie animation from an InputStream.
        The stream is fully consumed. Both plain Lottie JSON and .lottie ZIP (binary)
        streams are supported; the format is auto-detected.
        Returns a successful ResultValue holding the composition, or a failed
        ResultValue carrying an error message.
    */
    [[nodiscard]] static ResultValue<AnimationComposition::Ptr> parseStream (InputStream& stream,
                                                                             const LottieLoadOptions& options = {});

    /** Lists animation IDs contained inside a `.lottie` ZIP archive.
        Returns an empty vector if the file is not a valid .lottie file.
    */
    [[nodiscard]] static std::vector<String> listAnimationIds (const File& lottieZipFile);

    /** Parses a specific animation from a `.lottie` ZIP archive.
        If @p animationId is empty the first animation is used.
        Returns a successful ResultValue holding the composition, or a failed
        ResultValue carrying an error message.
    */
    [[nodiscard]] static ResultValue<AnimationComposition::Ptr> parseFromZip (const File& lottieZipFile,
                                                                              const String& animationId = {},
                                                                              const LottieLoadOptions& options = {});

private:
    //==============================================================================
    explicit LottieReader (const LottieLoadOptions& options);

    //==============================================================================
    void resolveLayerExpressions (const AnimationComposition& comp,
                                  const Array<var>& layerArray,
                                  std::vector<AnimationLayer::Ptr>& layers,
                                  size_t firstLayerIndex,
                                  const std::vector<int>& parsedLayerIndices);

    void resolveShapeLayerExpressions (const Array<var>& itemsArray, ShapeLayer& layer);

    void resolveGroupExpressions (const Array<var>& itemsArray, AnimationGroup& group);

    void resolveLayerAssets (AnimationComposition& comp);

    void resolveLayerAssets (AnimationComposition& comp,
                             std::vector<AnimationLayer::Ptr>& layers,
                             StringArray& resolvingPrecomps);

    //==============================================================================
    ResultValue<AnimationComposition::Ptr> parseRoot (const var& root);
    AnimationLayer::Ptr parseLayer (const var& layerObj);
    void parseLayers (const var& layersArray,
                      std::vector<AnimationLayer::Ptr>& out,
                      std::vector<int>& parsedIndicesOut);
    void parseShapeContents (const var& itemsArray, ShapeLayer& layer);
    void parseGroupItems (const var& itemsArray, AnimationGroup& group);
    void parseSingleItem (const var& itemObj, AnimationGroup& group);
    void parseTransform (const var& ksObj, AnimationTransform& transform, bool ddd = false);
    void parsePositionBounce (const var& positionObj, AnimationTransform& transform);
    void parseGradient (const var& gradObj, AnimationGradient& gradient);
    void parseMasks (const var& masksArray, AnimationLayer& layer);
    void parseEffects (const var& effectsArray, AnimationLayer& layer);
    void parseAssets (const var& assetsArray, AnimationComposition& comp);
    AnimationEasing parseEasing (const var& kfObj);

    template <typename T>
    AnimationProperty<T> parseProperty (const var& propObj, std::function<T (const var&)> extractor);

    //==============================================================================
    static void applyLayerPropertyRef (const String& property,
                                       const AnimationLayer& source,
                                       AnimationTransform& target);

    static void applyStaticTransformValue (const String& propName,
                                           const var& value,
                                           AnimationTransform& transform);

    //==============================================================================
    AnimationEasing lookupInterpolator (const String& name, float ox, float oy, float ix, float iy);

    //==============================================================================
    [[nodiscard]] static Color extractColor (const var& v);
    [[nodiscard]] static Point<float> extractPoint (const var& v);
    [[nodiscard]] static Size<float> extractSize (const var& v);
    [[nodiscard]] static float extractFloat (const var& v);
    [[nodiscard]] static AnimationPathData extractPath (const var& v);

    LottieLoadOptions options;
    String errorMessage;
    HashMap<String, AnimationEasing> interpolatorCache;
    float frameRate_ = 60.0f;
};

} // namespace yup
