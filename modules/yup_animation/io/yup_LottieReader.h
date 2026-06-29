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
        Returns nullptr on failure; writes an error message into @p outError when non-null.
    */
    [[nodiscard]] static AnimationComposition::Ptr parseFile (const File& file,
                                                              const LottieLoadOptions& options = {},
                                                              String* outError = nullptr);

    /** Parses a Lottie JSON string.
        Returns nullptr on failure.
    */
    [[nodiscard]] static AnimationComposition::Ptr parseData (const String& jsonText,
                                                              const LottieLoadOptions& options = {},
                                                              String* outError = nullptr);

    /** Lists animation IDs contained inside a `.lottie` ZIP archive.
        Returns an empty vector if the file is not a valid .lottie file.
    */
    [[nodiscard]] static std::vector<String> listAnimationIds (const File& lottieZipFile);

    /** Parses a specific animation from a `.lottie` ZIP archive.
        If @p animationId is empty the first animation is used.
    */
    [[nodiscard]] static AnimationComposition::Ptr parseFromZip (const File& lottieZipFile,
                                                                 const String& animationId = {},
                                                                 const LottieLoadOptions& options = {},
                                                                 String* outError = nullptr);

private:
    //==============================================================================
    LottieReader (const LottieLoadOptions& options, String* outError);

    AnimationComposition::Ptr parseRoot (const var& root);

    void parseLayers (const var& layersArray, std::vector<AnimationLayer::Ptr>& out);
    AnimationLayer::Ptr parseLayer (const var& layerObj);
    void parseShapeContents (const var& itemsArray, ShapeLayer& layer);
    void parseGroupItems (const var& itemsArray, AnimationGroup& group);
    void parseSingleItem (const var& itemObj, AnimationGroup& group);

    void parseTransform (const var& ksObj, AnimationTransform& transform, bool ddd = false);

    template <typename T>
    AnimationProperty<T> parseProperty (const var& propObj,
                                        std::function<T (const var&)> extractor);

    AnimationEasing parseEasing (const var& kfObj);

    void parseGradient (const var& gradObj, AnimationGradient& gradient);
    void parseMasks (const var& masksArray, AnimationLayer& layer);
    void parseAssets (const var& assetsArray, AnimationComposition& comp);

    void resolveLayerAssets (AnimationComposition& comp);

    // Value extractors
    [[nodiscard]] static Color extractColor (const var& v);
    [[nodiscard]] static Point<float> extractPoint (const var& v);
    [[nodiscard]] static Size<float> extractSize (const var& v);
    [[nodiscard]] static float extractFloat (const var& v);
    [[nodiscard]] static AnimationPathData extractPath (const var& v);

    LottieLoadOptions options_;
    String* errorOut_ = nullptr;
};

} // namespace yup
