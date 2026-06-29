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

namespace
{
//==========================================================================
// Helper to safely get a double from var
inline float varFloat (const var& v, float defaultValue = 0.0f)
{
    if (v.isVoid() || v.isUndefined())
        return defaultValue;
    return static_cast<float> ((double) v);
}

inline int varInt (const var& v, int defaultValue = 0)
{
    if (v.isVoid() || v.isUndefined())
        return defaultValue;
    return static_cast<int> ((int) v);
}

inline String varString (const var& v, const String& def = {})
{
    if (v.isVoid() || v.isUndefined())
        return def;
    return v.toString();
}

inline Array<var>* safeArray (const var& v)
{
    return v.getArray();
}

// Parse a hex colour string like "#ff0000"
inline Color parseHexColor (const String& hex)
{
    const String h = hex.trimCharactersAtStart ("#");
    if (h.length() >= 6)
    {
        const int r = h.substring (0, 2).getHexValue32();
        const int g = h.substring (2, 4).getHexValue32();
        const int b = h.substring (4, 6).getHexValue32();
        const int a = h.length() >= 8 ? h.substring (6, 8).getHexValue32() : 255;
        return Color::fromRGBA (static_cast<uint8> (r),
                                static_cast<uint8> (g),
                                static_cast<uint8> (b),
                                static_cast<uint8> (a));
    }
    return Color (0xFF000000);
}
} // namespace

//==============================================================================
LottieReader::LottieReader (const LottieLoadOptions& options, String* outError)
    : options_ (options)
    , errorOut_ (outError)
{
}

//==============================================================================
AnimationComposition::Ptr LottieReader::parseFile (const File& file,
                                                   const LottieLoadOptions& options,
                                                   String* outError)
{
    if (! file.existsAsFile())
    {
        if (outError != nullptr)
            *outError = "File not found: " + file.getFullPathName();
        return {};
    }

    // .lottie files are ZIP archives
    if (file.getFileExtension().equalsIgnoreCase (".lottie"))
        return parseFromZip (file, {}, options, outError);

    const String text = file.loadFileAsString();
    if (text.isEmpty())
    {
        if (outError != nullptr)
            *outError = "Empty file: " + file.getFullPathName();
        return {};
    }

    LottieLoadOptions opts = options;
    if (opts.resourceDirectory == File())
        opts.resourceDirectory = file.getParentDirectory();

    return parseData (text, opts, outError);
}

//==============================================================================
AnimationComposition::Ptr LottieReader::parseData (const String& jsonText,
                                                   const LottieLoadOptions& options,
                                                   String* outError)
{
    var root;
    const Result parseResult = JSON::parse (jsonText, root);
    if (parseResult.failed())
    {
        if (outError != nullptr)
            *outError = "JSON parse error: " + parseResult.getErrorMessage();
        return {};
    }

    LottieReader reader (options, outError);
    return reader.parseRoot (root);
}

//==============================================================================
std::vector<String> LottieReader::listAnimationIds (const File& lottieZipFile)
{
    ZipFile zip (lottieZipFile);
    const ZipFile::ZipEntry* manifest = zip.getEntry ("manifest.json", true);
    if (manifest == nullptr)
        return {};

    std::unique_ptr<InputStream> stream (zip.createStreamForEntry (*manifest));
    if (stream == nullptr)
        return {};

    const String text = stream->readEntireStreamAsString();
    var root;
    if (JSON::parse (text, root).failed())
        return {};

    std::vector<String> ids;
    if (const auto* anims = safeArray (root["animations"]))
    {
        for (const var& anim : *anims)
            ids.push_back (varString (anim["id"]));
    }
    return ids;
}

//==============================================================================
AnimationComposition::Ptr LottieReader::parseFromZip (const File& lottieZipFile,
                                                      const String& animationId,
                                                      const LottieLoadOptions& options,
                                                      String* outError)
{
    ZipFile zip (lottieZipFile);

    // Read manifest
    const ZipFile::ZipEntry* manifestEntry = zip.getEntry ("manifest.json", true);
    if (manifestEntry == nullptr)
    {
        if (outError != nullptr)
            *outError = "manifest.json not found in .lottie file";
        return {};
    }

    std::unique_ptr<InputStream> manifestStream (zip.createStreamForEntry (*manifestEntry));
    if (manifestStream == nullptr)
        return {};

    var manifest;
    if (JSON::parse (manifestStream->readEntireStreamAsString(), manifest).failed())
    {
        if (outError != nullptr)
            *outError = "Failed to parse manifest.json";
        return {};
    }

    // Find the target animation path from the manifest
    String jsonPath;
    const auto* anims = safeArray (manifest["animations"]);
    if (anims == nullptr || anims->isEmpty())
    {
        if (outError != nullptr)
            *outError = "No animations found in manifest";
        return {};
    }

    for (const var& anim : *anims)
    {
        const String id = varString (anim["id"]);
        if (animationId.isEmpty() || id == animationId)
        {
            jsonPath = varString (anim["path"]);
            if (! jsonPath.endsWithIgnoreCase (".json"))
                jsonPath += ".json";
            break;
        }
    }

    if (jsonPath.isEmpty())
    {
        if (outError != nullptr)
            *outError = "Animation id not found in manifest: " + animationId;
        return {};
    }

    const ZipFile::ZipEntry* jsonEntry = zip.getEntry (jsonPath, true);
    if (jsonEntry == nullptr)
    {
        if (outError != nullptr)
            *outError = "Animation JSON not found inside archive: " + jsonPath;
        return {};
    }

    std::unique_ptr<InputStream> jsonStream (zip.createStreamForEntry (*jsonEntry));
    if (jsonStream == nullptr)
        return {};

    return parseData (jsonStream->readEntireStreamAsString(), options, outError);
}

//==============================================================================
AnimationComposition::Ptr LottieReader::parseRoot (const var& root)
{
    auto comp = AnimationComposition::create (
        { varFloat (root["w"], 500.0f), varFloat (root["h"], 500.0f) },
        varFloat (root["fr"], 60.0f));

    comp->name = varString (root["nm"]);
    comp->version = varString (root["v"], "5.5.7");
    comp->startFrame = varFloat (root["ip"]);
    comp->endFrame = varFloat (root["op"], 60.0f);

    parseAssets (root["assets"], *comp);
    parseLayers (root["layers"], comp->layers);

    if (const auto* markersArr = safeArray (root["markers"]))
    {
        for (const var& m : *markersArr)
        {
            AnimationMarker marker;
            marker.comment = varString (m["cm"]);
            marker.startFrame = varFloat (m["tm"]);
            marker.duration = varFloat (m["dr"]);
            comp->markers.push_back (std::move (marker));
        }
    }

    return comp;
}

//==============================================================================
void LottieReader::parseAssets (const var& assetsVal, AnimationComposition& comp)
{
    const auto* arr = safeArray (assetsVal);
    if (arr == nullptr)
        return;

    for (const var& assetVar : *arr)
    {
        auto asset = new AnimationAsset();
        asset->id = varString (assetVar["id"]);

        if (assetVar["layers"].isArray())
        {
            // Precomp
            asset->assetType = AnimationAsset::AssetType::Precomp;
            parseLayers (assetVar["layers"], asset->layers);
        }
        else
        {
            // Image asset
            asset->assetType = AnimationAsset::AssetType::Image;
            asset->path = varString (assetVar["u"]) + varString (assetVar["p"]);
            asset->width = varInt (assetVar["w"]);
            asset->height = varInt (assetVar["h"]);

            if (options_.imageResolver)
            {
                auto img = options_.imageResolver (asset->path, options_.resourceDirectory);
                if (img.has_value())
                    asset->bitmap = std::move (img);
            }
        }

        comp.assets.set (asset->id, asset);
    }
}

//==============================================================================
void LottieReader::parseLayers (const var& layersVal,
                                std::vector<AnimationLayer::Ptr>& out)
{
    const auto* arr = safeArray (layersVal);
    if (arr == nullptr)
        return;

    for (const var& layerVar : *arr)
    {
        if (auto layer = parseLayer (layerVar))
            out.push_back (std::move (layer));
    }
}

//==============================================================================
AnimationLayer::Ptr LottieReader::parseLayer (const var& layerObj)
{
    const int ty = varInt (layerObj["ty"]);

    AnimationLayer::Ptr layer;

    if (ty == 4) // Shape
    {
        auto sl = new ShapeLayer();
        parseShapeContents (layerObj["shapes"], *sl);
        layer = sl;
    }
    else if (ty == 0) // Precomp
    {
        auto pl = new PrecompLayer();
        pl->precompRefId = varString (layerObj["refId"]);
        pl->layerSize = { varFloat (layerObj["w"]), varFloat (layerObj["h"]) };
        layer = pl;
    }
    else if (ty == 1) // Solid
    {
        auto sol = new SolidLayer();
        sol->solidColor = parseHexColor (varString (layerObj["sc"]));
        sol->layerSize = { varFloat (layerObj["sw"]), varFloat (layerObj["sh"]) };
        layer = sol;
    }
    else if (ty == 2) // Image
    {
        auto il = new ImageLayer();
        il->assetRefId = varString (layerObj["refId"]);
        layer = il;
    }
    else // Null (ty == 3) or unknown
    {
        layer = new NullLayer();
    }

    if (layer == nullptr)
        return {};

    layer->name = varString (layerObj["nm"]);
    layer->id = varInt (layerObj["ind"], -1);
    layer->parentId = varInt (layerObj["parent"], -1);
    layer->inFrame = varFloat (layerObj["ip"]);
    layer->outFrame = varFloat (layerObj["op"]);
    layer->startFrame = varFloat (layerObj["st"]);
    layer->timeStretch = varFloat (layerObj["sr"], 1.0f);
    layer->hidden = (bool) layerObj["hd"];
    layer->autoOrient = (bool) layerObj["ao"];
    layer->blendMode = static_cast<BlendMode> (varInt (layerObj["bm"]));

    const int matteType = varInt (layerObj["tt"]);
    layer->matteType = static_cast<AnimationLayer::MatteType> (matteType);

    parseTransform (layerObj["ks"], layer->transform);
    parseMasks (layerObj["masksProperties"], *layer);

    if (! layerObj["tm"].isVoid())
    {
        FloatProperty tr;
        tr = parseProperty<float> (layerObj["tm"], extractFloat);
        layer->timeRemap = std::move (tr);
    }

    return layer;
}

//==============================================================================
void LottieReader::parseShapeContents (const var& shapesVal, ShapeLayer& layer)
{
    const auto* arr = safeArray (shapesVal);
    if (arr == nullptr)
        return;

    for (const var& item : *arr)
    {
        const String ty = varString (item["ty"]);

        if (ty == "gr")
        {
            auto* group = layer.addGroup (varString (item["nm"]));
            parseGroupItems (item["it"], *group);
        }
        else
        {
            // Top-level shapes outside a group go into an implicit group
            if (layer.getNumGroups() == 0)
                layer.addGroup();
            parseSingleItem (item, *layer.getGroup (0));
        }
    }
}

//==============================================================================
void LottieReader::parseGroupItems (const var& itemsVal, AnimationGroup& group)
{
    const auto* arr = safeArray (itemsVal);
    if (arr == nullptr)
        return;

    for (const var& item : *arr)
    {
        const String ty = varString (item["ty"]);
        if (ty == "tr") // group transform
        {
            parseTransform (item, group.transform);
        }
        else if (ty == "gr") // nested group
        {
            auto* subGroup = group.addGroup();
            subGroup->name = varString (item["nm"]);
            parseGroupItems (item["it"], *subGroup);
        }
        else
        {
            parseSingleItem (item, group);
        }
    }
}

//==============================================================================
void LottieReader::parseSingleItem (const var& itemObj, AnimationGroup& group)
{
    const String ty = varString (itemObj["ty"]);

    if (ty == "el") // Ellipse
    {
        auto* el = group.addShape<EllipseShape>();
        el->setName (varString (itemObj["nm"]));
        el->setHidden ((bool) itemObj["hd"]);
        el->center = parseProperty<Point<float>> (itemObj["p"], extractPoint);
        el->size = parseProperty<Size<float>> (itemObj["s"], extractSize);
        el->setDirection (varInt (itemObj["d"], 1));
    }
    else if (ty == "rc") // Rect
    {
        auto* rc = group.addShape<RectShape>();
        rc->setName (varString (itemObj["nm"]));
        rc->setHidden ((bool) itemObj["hd"]);
        rc->position = parseProperty<Point<float>> (itemObj["p"], extractPoint);
        rc->size = parseProperty<Size<float>> (itemObj["s"], extractSize);
        rc->roundness = parseProperty<float> (itemObj["r"], extractFloat);
        rc->setDirection (varInt (itemObj["d"], 1));
    }
    else if (ty == "sh") // Bezier path
    {
        auto* sh = group.addShape<BezierPathShape>();
        sh->setName (varString (itemObj["nm"]));
        sh->setHidden ((bool) itemObj["hd"]);
        sh->pathData = parseProperty<AnimationPathData> (itemObj["ks"], extractPath);
        sh->setDirection (varInt (itemObj["d"], 1));
    }
    else if (ty == "sr") // Polystar
    {
        auto* sr = group.addShape<PolystarShape>();
        sr->setName (varString (itemObj["nm"]));
        sr->setHidden ((bool) itemObj["hd"]);
        sr->starType = static_cast<PolystarShape::StarType> (varInt (itemObj["sy"], 2));
        sr->position = parseProperty<Point<float>> (itemObj["p"], extractPoint);
        sr->points = parseProperty<float> (itemObj["pt"], extractFloat);
        sr->outerRadius = parseProperty<float> (itemObj["or"], extractFloat);
        sr->innerRadius = parseProperty<float> (itemObj["ir"], extractFloat);
        sr->outerRoundness = parseProperty<float> (itemObj["os"], extractFloat);
        sr->innerRoundness = parseProperty<float> (itemObj["is"], extractFloat);
        sr->rotation = parseProperty<float> (itemObj["r"], extractFloat);
    }
    else if (ty == "fl") // Fill
    {
        auto* fl = group.addFill();
        fl->name = varString (itemObj["nm"]);
        fl->hidden = (bool) itemObj["hd"];
        fl->color = parseProperty<Color> (itemObj["c"], extractColor);
        fl->opacity = parseProperty<float> (itemObj["o"], extractFloat);
        fl->fillRule = (varInt (itemObj["r"]) == 2) ? FillPaint::FillRule::EvenOdd
                                                    : FillPaint::FillRule::NonZero;
    }
    else if (ty == "st") // Stroke
    {
        auto* st = group.addStroke();
        st->name = varString (itemObj["nm"]);
        st->hidden = (bool) itemObj["hd"];
        st->color = parseProperty<Color> (itemObj["c"], extractColor);
        st->opacity = parseProperty<float> (itemObj["o"], extractFloat);
        st->width = parseProperty<float> (itemObj["w"], extractFloat);

        // lc: 1=Butt, 2=Round, 3=Square
        const int lc = varInt (itemObj["lc"], 1);
        st->cap = (lc == 2) ? StrokeCap::Round : (lc == 3) ? StrokeCap::Square
                                                           : StrokeCap::Butt;

        // lj: 1=Miter, 2=Round, 3=Bevel
        const int lj = varInt (itemObj["lj"], 1);
        st->join = (lj == 2) ? StrokeJoin::Round : (lj == 3) ? StrokeJoin::Bevel
                                                             : StrokeJoin::Miter;

        st->miterLimit = varFloat (itemObj["ml"], 4.0f);

        if (const auto* dashArr = safeArray (itemObj["d"]))
        {
            for (const var& d : *dashArr)
            {
                StrokeDash dash;
                const String nm = varString (d["nm"]);
                if (nm == "d")
                    dash.kind = StrokeDash::Kind::Dash;
                else if (nm == "g")
                    dash.kind = StrokeDash::Kind::Gap;
                else
                    dash.kind = StrokeDash::Kind::Offset;
                dash.value = parseProperty<float> (d["v"], extractFloat);
                st->dashArray.push_back (std::move (dash));
            }
        }
    }
    else if (ty == "gf" || ty == "gs") // Gradient fill / gradient stroke
    {
        const bool isStroke = (ty == "gs");

        if (isStroke)
        {
            auto* gs = group.addStroke();
            gs->name = varString (itemObj["nm"]);
            gs->hidden = (bool) itemObj["hd"];
            gs->opacity = parseProperty<float> (itemObj["o"], extractFloat);
            gs->width = parseProperty<float> (itemObj["w"], extractFloat);

            const int lc = varInt (itemObj["lc"], 1);
            gs->cap = (lc == 2) ? StrokeCap::Round : (lc == 3) ? StrokeCap::Square
                                                               : StrokeCap::Butt;
            const int lj = varInt (itemObj["lj"], 1);
            gs->join = (lj == 2) ? StrokeJoin::Round : (lj == 3) ? StrokeJoin::Bevel
                                                                 : StrokeJoin::Miter;
            gs->miterLimit = varFloat (itemObj["ml"], 4.0f);

            auto grad = new AnimationGradient();
            parseGradient (itemObj, *grad);
            gs->gradient = grad;
        }
        else
        {
            auto* gf = group.addFill();
            gf->name = varString (itemObj["nm"]);
            gf->hidden = (bool) itemObj["hd"];
            gf->opacity = parseProperty<float> (itemObj["o"], extractFloat);
            gf->fillRule = (varInt (itemObj["r"]) == 2) ? FillPaint::FillRule::EvenOdd
                                                        : FillPaint::FillRule::NonZero;
            auto grad = new AnimationGradient();
            parseGradient (itemObj, *grad);
            gf->gradient = grad;
        }
    }
    else if (ty == "tm") // Trim
    {
        auto* tm = group.addTrim();
        tm->name = varString (itemObj["nm"]);
        tm->hidden = (bool) itemObj["hd"];
        tm->start = parseProperty<float> (itemObj["s"], extractFloat);
        tm->end = parseProperty<float> (itemObj["e"], extractFloat);
        tm->offset = parseProperty<float> (itemObj["o"], extractFloat);
        tm->mode = (varInt (itemObj["m"]) == 2)
                     ? AnimationTrim::TrimMode::Individually
                     : AnimationTrim::TrimMode::Simultaneously;
    }
    else if (ty == "rp") // Repeater
    {
        auto* rp = group.addRepeater();
        rp->name = varString (itemObj["nm"]);
        rp->hidden = (bool) itemObj["hd"];
        rp->copies = parseProperty<float> (itemObj["c"], extractFloat);
        rp->offset = parseProperty<float> (itemObj["o"], extractFloat);

        const var& tr = itemObj["tr"];
        if (! tr.isVoid())
        {
            parseTransform (tr, rp->copyTransform);
            rp->startOpacity = parseProperty<float> (tr["so"], extractFloat);
            rp->endOpacity = parseProperty<float> (tr["eo"], extractFloat);
        }
    }
}

//==============================================================================
void LottieReader::parseGradient (const var& gradObj, AnimationGradient& gradient)
{
    gradient.gradientType = (varInt (gradObj["t"]) == 2)
                              ? AnimationGradient::GradientType::Radial
                              : AnimationGradient::GradientType::Linear;

    gradient.startPoint = parseProperty<Point<float>> (gradObj["s"], extractPoint);
    gradient.endPoint = parseProperty<Point<float>> (gradObj["e"], extractPoint);

    if (gradient.gradientType == AnimationGradient::GradientType::Radial)
    {
        gradient.highlightLen = parseProperty<float> (gradObj["h"], extractFloat);
        gradient.highlightAngle = parseProperty<float> (gradObj["a"], extractFloat);
    }

    const int colorPoints = varInt (gradObj["g"]["p"]);
    const var& gk = gradObj["g"]["k"];

    // Parse gradient stops from the flat float array
    if (gk.isObject())
    {
        const bool isAnimated = varInt (gk["a"]) == 1;

        // Lottie packs color stops and opacity stops in one flat array:
        //   [pos, R, G, B, ...] × colorPoints, then [pos, opacity, ...] pairs for the remainder.
        auto parseStopsFromArray = [&] (const var& arr) -> std::vector<std::pair<float, Color>>
        {
            std::vector<std::pair<float, Color>> stops;
            const auto* a = safeArray (arr);
            if (a == nullptr)
                return stops;

            const int totalSize = static_cast<int> (a->size());
            const int count = jmin (colorPoints, totalSize / 4);
            const int colorDataEnd = count * 4;

            // Collect opacity stops (position, opacity) from the tail of the array.
            std::vector<std::pair<float, float>> opacityStops;
            for (int i = colorDataEnd; i + 1 < totalSize; i += 2)
                opacityStops.push_back ({ varFloat ((*a)[i]), varFloat ((*a)[i + 1]) });

            // Linear interpolation of opacity at a given position.
            auto getOpacityAt = [&] (float pos) -> float
            {
                if (opacityStops.empty())
                    return 1.0f;
                if (pos <= opacityStops.front().first)
                    return opacityStops.front().second;
                if (pos >= opacityStops.back().first)
                    return opacityStops.back().second;
                for (size_t j = 1; j < opacityStops.size(); ++j)
                {
                    if (opacityStops[j].first >= pos)
                    {
                        const float span = opacityStops[j].first - opacityStops[j - 1].first;
                        const float t = span > 1e-6f ? (pos - opacityStops[j - 1].first) / span : 1.0f;
                        return opacityStops[j - 1].second + t * (opacityStops[j].second - opacityStops[j - 1].second);
                    }
                }
                return 1.0f;
            };

            for (int i = 0; i < count; ++i)
            {
                const float pos = varFloat ((*a)[i * 4]);
                const float r = varFloat ((*a)[i * 4 + 1]);
                const float g = varFloat ((*a)[i * 4 + 2]);
                const float b = varFloat ((*a)[i * 4 + 3]);
                const float alpha = getOpacityAt (pos);
                stops.push_back ({ pos, Color::fromRGBA (static_cast<uint8> (r * 255.0f), static_cast<uint8> (g * 255.0f), static_cast<uint8> (b * 255.0f), static_cast<uint8> (alpha * 255.0f)) });
            }
            return stops;
        };

        if (! isAnimated)
        {
            for (const auto& [pos, col] : parseStopsFromArray (gk["k"]))
                gradient.addColorStop (pos, col);
        }
        else
        {
            // Use first keyframe stops for a static approximation
            if (const auto* kfs = safeArray (gk["k"]))
            {
                if (! kfs->isEmpty())
                {
                    for (const auto& [pos, col] : parseStopsFromArray ((*kfs)[0]["s"]))
                        gradient.addColorStop (pos, col);
                }
            }
        }
    }
}

//==============================================================================
void LottieReader::parseMasks (const var& masksVal, AnimationLayer& layer)
{
    const auto* arr = safeArray (masksVal);
    if (arr == nullptr)
        return;

    for (const var& m : *arr)
    {
        auto mask = new AnimationMask();
        mask->name = varString (m["nm"]);
        mask->inverted = (bool) m["inv"];
        mask->opacity = parseProperty<float> (m["o"], extractFloat);
        mask->shape = parseProperty<AnimationPathData> (m["pt"], extractPath);

        const String modeStr = varString (m["mode"]);
        if (modeStr == "a")
            mask->mode = AnimationMask::Mode::Add;
        else if (modeStr == "s")
            mask->mode = AnimationMask::Mode::Subtract;
        else if (modeStr == "i")
            mask->mode = AnimationMask::Mode::Intersect;
        else if (modeStr == "f")
            mask->mode = AnimationMask::Mode::Difference;
        else
            mask->mode = AnimationMask::Mode::None;

        layer.masks.push_back (mask);
    }
}

//==============================================================================
void LottieReader::parseTransform (const var& ksObj, AnimationTransform& t)
{
    if (ksObj.isVoid())
        return;

    t.anchor = parseProperty<Point<float>> (ksObj["a"], extractPoint);
    t.rotation = parseProperty<float> (ksObj["r"], extractFloat);
    t.opacity = parseProperty<float> (ksObj["o"], extractFloat);
    t.skew = parseProperty<float> (ksObj["sk"], extractFloat);
    t.skewAxis = parseProperty<float> (ksObj["sa"], extractFloat);

    // Lottie encodes separate X/Y position inside the "p" object as { s: true, x: {}, y: {} }.
    const var& pObj = ksObj["p"];
    if (pObj.isObject() && (bool) pObj["s"])
    {
        t.separatePosition = true;
        t.positionX = parseProperty<float> (pObj["x"], extractFloat);
        t.positionY = parseProperty<float> (pObj["y"], extractFloat);
    }
    else
    {
        t.position = parseProperty<Point<float>> (pObj, extractPoint);
    }

    t.scale = parseProperty<Size<float>> (ksObj["s"], extractSize);
}

//==============================================================================
// Property parsing

template <typename T>
AnimationProperty<T> LottieReader::parseProperty (const var& propObj,
                                                  std::function<T (const var&)> extractor)
{
    if (propObj.isVoid() || propObj.isUndefined())
        return {};

    const int animated = varInt (propObj["a"]);
    const var& k = propObj["k"];

    if (animated == 0 || k.isVoid())
    {
        // Static value
        if (k.isVoid())
            return AnimationProperty<T>::staticValue (extractor (propObj));
        return AnimationProperty<T>::staticValue (extractor (k));
    }

    // Animated: k is an array of keyframes
    const auto* kfs = safeArray (k);
    if (kfs == nullptr || kfs->size() < 2)
    {
        if (kfs != nullptr && kfs->size() == 1)
            return AnimationProperty<T>::staticValue (extractor ((*kfs)[0]["s"]));
        return {};
    }

    typename AnimationProperty<T>::Builder builder;

    for (int i = 0; i < kfs->size(); ++i)
    {
        const var& kf = (*kfs)[i];
        const float frame = varFloat (kf["t"]);
        const var& startVal = kf["s"];

        T value {};
        if (! startVal.isVoid())
            value = extractor (startVal);

        AnimationEasing easing = parseEasing (kf);
        builder.keyframe (frame, std::move (value), std::move (easing));
    }

    return builder.build();
}

AnimationEasing LottieReader::parseEasing (const var& kfObj)
{
    if (varInt (kfObj["h"]) == 1)
        return AnimationEasing::hold();

    const var& o = kfObj["o"];
    const var& i = kfObj["i"];

    if (o.isVoid() || i.isVoid())
        return AnimationEasing::linear();

    auto getFirstOrValue = [] (const var& v) -> float
    {
        if (const auto* arr = v.getArray())
            return arr->isEmpty() ? 0.0f : static_cast<float> ((double) (*arr)[0]);
        return static_cast<float> ((double) v);
    };

    const float ox = getFirstOrValue (o["x"]);
    const float oy = getFirstOrValue (o["y"]);
    const float ix = getFirstOrValue (i["x"]);
    const float iy = getFirstOrValue (i["y"]);

    return AnimationEasing (ox, oy, ix, iy);
}

//==============================================================================
// Value extractors

Color LottieReader::extractColor (const var& v)
{
    const auto* arr = safeArray (v);
    if (arr != nullptr && arr->size() >= 3)
    {
        const float r = jlimit (0.0f, 1.0f, varFloat ((*arr)[0]));
        const float g = jlimit (0.0f, 1.0f, varFloat ((*arr)[1]));
        const float b = jlimit (0.0f, 1.0f, varFloat ((*arr)[2]));
        const float a = arr->size() >= 4 ? jlimit (0.0f, 1.0f, varFloat ((*arr)[3])) : 1.0f;
        return Color::fromRGBA (static_cast<uint8> (r * 255.0f),
                                static_cast<uint8> (g * 255.0f),
                                static_cast<uint8> (b * 255.0f),
                                static_cast<uint8> (a * 255.0f));
    }
    return Color (0xFF000000);
}

Point<float> LottieReader::extractPoint (const var& v)
{
    if (const auto* arr = safeArray (v))
    {
        const float x = arr->size() > 0 ? varFloat ((*arr)[0]) : 0.0f;
        const float y = arr->size() > 1 ? varFloat ((*arr)[1]) : 0.0f;
        return { x, y };
    }
    return {};
}

Size<float> LottieReader::extractSize (const var& v)
{
    if (const auto* arr = safeArray (v))
    {
        const float w = arr->size() > 0 ? varFloat ((*arr)[0]) : 0.0f;
        const float h = arr->size() > 1 ? varFloat ((*arr)[1]) : 0.0f;
        return { w, h };
    }
    return {};
}

float LottieReader::extractFloat (const var& v)
{
    if (const auto* arr = safeArray (v))
        return arr->isEmpty() ? 0.0f : varFloat ((*arr)[0]);
    return varFloat (v);
}

AnimationPathData LottieReader::extractPath (const var& v)
{
    var src = v;
    if (const auto* arr = safeArray (v))
        if (arr->size() == 1 && (*arr)[0].isObject())
            src = (*arr)[0];

    AnimationPathData pd;

    const auto extractPointArray = [] (const var& arr) -> std::vector<Point<float>>
    {
        std::vector<Point<float>> pts;
        if (const auto* a = safeArray (arr))
        {
            for (const var& pt : *a)
            {
                if (const auto* ptArr = safeArray (pt))
                {
                    const float x = ptArr->size() > 0 ? varFloat ((*ptArr)[0]) : 0.0f;
                    const float y = ptArr->size() > 1 ? varFloat ((*ptArr)[1]) : 0.0f;
                    pts.push_back ({ x, y });
                }
            }
        }
        return pts;
    };

    pd.vertices = extractPointArray (src["v"]);
    pd.inTangents = extractPointArray (src["i"]);
    pd.outTangents = extractPointArray (src["o"]);
    pd.closed = (bool) src["c"];

    // Ensure all arrays have the same size
    const size_t n = pd.vertices.size();
    pd.inTangents.resize (n, {});
    pd.outTangents.resize (n, {});

    return pd;
}

} // namespace yup
