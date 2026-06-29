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

AnimationGroup* findChildGroupByName (const AnimationGroup& group, const String& name)
{
    for (const auto& child : group.children)
    {
        if (child.kind == AnimationGroup::ChildKind::Group
            && child.group != nullptr
            && child.group->name == name)
        {
            return child.group.get();
        }
    }

    return nullptr;
}

AnimationGroup* findShapeLayerGroupByName (const ShapeLayer& layer, const String& name)
{
    for (const auto& group : layer.groups)
    {
        if (group != nullptr && group->name == name)
            return group.get();
    }

    return nullptr;
}

BezierPathShape* findBezierPathByName (const AnimationGroup& group, const String& name)
{
    for (const auto& child : group.children)
    {
        if (child.kind == AnimationGroup::ChildKind::Shape
            && child.shape != nullptr
            && child.shape->getKind() == AnimationShape::Kind::BezierPath
            && child.shape->getName() == name)
        {
            return static_cast<BezierPathShape*> (child.shape.get());
        }
    }

    return nullptr;
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

    const var& layersVar = root["layers"];
    const size_t firstLayerIdx = comp->layers.size();
    std::vector<int> parsedLayerIndices;
    parseLayers (layersVar, comp->layers, parsedLayerIndices);
    if (const auto* arr = safeArray (layersVar))
        resolveLayerExpressions (*comp, *arr, comp->layers, firstLayerIdx, parsedLayerIndices);

    resolveLayerAssets (*comp);

    // Validate composition (gap 22)
    if (comp->version.isEmpty())
    {
        if (errorOut_ != nullptr)
            *errorOut_ = "Invalid Lottie: missing version";
        return {};
    }
    if (comp->startFrame > comp->endFrame)
    {
        if (errorOut_ != nullptr)
            *errorOut_ = "Invalid Lottie: startFrame > endFrame";
        return {};
    }

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
            std::vector<int> ignored;
            parseLayers (assetVar["layers"], asset->layers, ignored);
        }
        else
        {
            // Image asset
            asset->assetType = AnimationAsset::AssetType::Image;
            asset->path = varString (assetVar["u"]) + varString (assetVar["p"]);
            asset->width = varInt (assetVar["w"]);
            asset->height = varInt (assetVar["h"]);

            // Handle embedded (base64) images: "e" is a flag (0 or 1),
            // and the "p" field contains a data: URI when embedded.
            const bool embeddedResource = (bool) assetVar["e"];
            if (embeddedResource)
            {
                const String imagePath = varString (assetVar["p"]);
                if (imagePath.startsWith ("data:") && imagePath.indexOf (",") > 0)
                {
                    const int commaPos = imagePath.indexOf (",");
                    const String b64Data = imagePath.substring (commaPos + 1);
                    MemoryOutputStream binaryData;
                    if (Base64::convertFromBase64 (binaryData, b64Data))
                    {
                        const auto imgResult = Image::loadFromData (Span<const uint8> (static_cast<const uint8*> (binaryData.getData()), binaryData.getDataSize()));
                        if (imgResult.wasOk())
                            asset->bitmap = imgResult.getValue();
                    }
                }
            }

            if (! asset->bitmap.has_value() && options_.imageResolver)
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
void LottieReader::resolveLayerAssets (AnimationComposition& comp)
{
    StringArray resolvingPrecomps;
    resolveLayerAssets (comp, comp.layers, resolvingPrecomps);
}

void LottieReader::resolveLayerAssets (AnimationComposition& comp,
                                       std::vector<AnimationLayer::Ptr>& layers,
                                       StringArray& resolvingPrecomps)
{
    for (auto& layer : layers)
    {
        if (layer == nullptr)
            continue;

        if (layer->getType() == AnimationLayer::Type::Image)
        {
            auto& imageLayer = static_cast<ImageLayer&> (*layer);
            auto assetPtr = comp.assets[imageLayer.assetRefId];
            if (assetPtr != nullptr && assetPtr->bitmap.has_value())
                imageLayer.image = assetPtr->bitmap;
        }
        else if (layer->getType() == AnimationLayer::Type::Precomp)
        {
            auto& precompLayer = static_cast<PrecompLayer&> (*layer);
            auto assetPtr = comp.assets[precompLayer.precompRefId];
            if (assetPtr != nullptr && ! assetPtr->layers.empty())
            {
                if (precompLayer.layerSize.getWidth() <= 0.0f || precompLayer.layerSize.getHeight() <= 0.0f)
                {
                    precompLayer.layerSize = comp.size;
                }

                if (! resolvingPrecomps.contains (precompLayer.precompRefId))
                {
                    resolvingPrecomps.add (precompLayer.precompRefId);
                    resolveLayerAssets (comp, assetPtr->layers, resolvingPrecomps);
                    resolvingPrecomps.removeString (precompLayer.precompRefId);
                }
            }
        }
    }
}

//==============================================================================
void LottieReader::parseLayers (const var& layersVal,
                                std::vector<AnimationLayer::Ptr>& out,
                                std::vector<int>& parsedIndicesOut)
{
    const auto* arr = safeArray (layersVal);
    if (arr == nullptr)
        return;

    for (int i = 0; i < arr->size(); ++i)
    {
        const var& layerVar = (*arr)[i];
        if (auto layer = parseLayer (layerVar))
        {
            parsedIndicesOut.push_back (i);
            out.push_back (std::move (layer));
        }
    }
}

void LottieReader::resolveLayerExpressions (const AnimationComposition& comp,
                                            const Array<var>& layerArray,
                                            std::vector<AnimationLayer::Ptr>& layers,
                                            size_t firstLayerIndex,
                                            const std::vector<int>& parsedLayerIndices)
{
    LottieExpressionEvaluator::CompositionContext ctx;
    ctx.size = comp.size;
    ctx.frameRate = comp.frameRate;

    for (size_t i = firstLayerIndex; i < layers.size(); ++i)
    {
        if (layers[i] != nullptr)
            ctx.layers.push_back ({ layers[i]->name, layers[i]->id, &layers[i]->transform });
    }

    HashMap<String, AnimationLayer*> layersByName;
    HashMap<int, AnimationLayer*> layersById;
    for (size_t i = firstLayerIndex; i < layers.size(); ++i)
    {
        if (layers[i] == nullptr)
            continue;
        if (layers[i]->name.isNotEmpty())
            layersByName.set (layers[i]->name, layers[i].get());
        layersById.set (layers[i]->id, layers[i].get());
    }

    LottieExpressionEvaluator evaluator;
    evaluator.setupCompositionContext (ctx);

    static const std::pair<const char*, const char*> kTransformProps[] = {
        { "p", "position" }, { "r", "rotation" }, { "s", "scale" }, { "o", "opacity" }, { "a", "anchor" }
    };

    for (size_t i = 0; i < parsedLayerIndices.size(); ++i)
    {
        const size_t layerIndex = firstLayerIndex + i;
        if (layerIndex >= layers.size() || layers[layerIndex] == nullptr)
            continue;

        const int sourceIndex = parsedLayerIndices[i];
        if (sourceIndex < 0 || sourceIndex >= layerArray.size())
            continue;

        const var& ksObj = layerArray[sourceIndex]["ks"];
        AnimationTransform& t = layers[layerIndex]->transform;

        for (const auto& [jsonKey, propName] : kTransformProps)
        {
            const String expr = varString (ksObj[jsonKey]["x"]);
            if (expr.isEmpty())
                continue;

            const auto result = evaluator.evaluate (expr);

            if (result.kind == LottieExpressionEvaluator::EvalResult::Kind::LayerPropertyRef)
            {
                AnimationLayer* refLayer = nullptr;
                if (result.referencedLayerName.isNotEmpty())
                    refLayer = layersByName[result.referencedLayerName];
                else if (result.referencedLayerId >= 0)
                    refLayer = layersById[result.referencedLayerId];

                if (refLayer != nullptr)
                    applyLayerPropertyRef (result.referencedProperty, *refLayer, t);
            }
            else if (result.kind == LottieExpressionEvaluator::EvalResult::Kind::StaticValue)
            {
                applyStaticTransformValue (propName, result.value, t);
            }
        }

        // Separate X/Y position expressions
        const var& pObj = ksObj["p"];
        if (pObj.isObject() && (bool) pObj["s"])
        {
            const String exprX = varString (pObj["x"]["x"]);
            if (exprX.isNotEmpty())
            {
                const auto rx = evaluator.evaluate (exprX);
                if (rx.kind == LottieExpressionEvaluator::EvalResult::Kind::StaticValue)
                    t.positionX = AnimationProperty<float>::staticValue (
                        static_cast<float> (static_cast<double> (rx.value)));
            }

            const String exprY = varString (pObj["y"]["x"]);
            if (exprY.isNotEmpty())
            {
                const auto ry = evaluator.evaluate (exprY);
                if (ry.kind == LottieExpressionEvaluator::EvalResult::Kind::StaticValue)
                    t.positionY = AnimationProperty<float>::staticValue (
                        static_cast<float> (static_cast<double> (ry.value)));
            }
        }
    }
}

void LottieReader::applyLayerPropertyRef (const String& property,
                                          const AnimationLayer& source,
                                          AnimationTransform& target)
{
    if (property == "transform.position")
    {
        target.separatePosition = source.transform.separatePosition;
        target.position = source.transform.position;
        target.positionX = source.transform.positionX;
        target.positionY = source.transform.positionY;
        target.spatialKeyframes = source.transform.spatialKeyframes;
    }
    else if (property == "transform.rotation")
    {
        target.rotation = source.transform.rotation;
    }
    else if (property == "transform.scale")
    {
        target.scale = source.transform.scale;
    }
    else if (property == "transform.opacity")
    {
        target.opacity = source.transform.opacity;
    }
    else if (property == "transform.anchorPoint" || property == "transform.anchor")
    {
        target.anchor = source.transform.anchor;
    }
}

void LottieReader::applyStaticTransformValue (const String& propName,
                                              const var& value,
                                              AnimationTransform& transform)
{
    if (propName == "position")
    {
        if (const auto* arr = value.getArray(); arr != nullptr && arr->size() >= 2)
            transform.position = AnimationProperty<Point<float>>::staticValue (
                { static_cast<float> (static_cast<double> ((*arr)[0])),
                  static_cast<float> (static_cast<double> ((*arr)[1])) });
    }
    else if (propName == "rotation")
    {
        transform.rotation = AnimationProperty<float>::staticValue (
            static_cast<float> (static_cast<double> (value)));
    }
    else if (propName == "scale")
    {
        if (const auto* arr = value.getArray(); arr != nullptr && arr->size() >= 2)
            transform.scale = AnimationProperty<Size<float>>::staticValue (
                { static_cast<float> (static_cast<double> ((*arr)[0])),
                  static_cast<float> (static_cast<double> ((*arr)[1])) });
    }
    else if (propName == "opacity")
    {
        transform.opacity = AnimationProperty<float>::staticValue (
            static_cast<float> (static_cast<double> (value)));
    }
    else if (propName == "anchor")
    {
        if (const auto* arr = value.getArray(); arr != nullptr && arr->size() >= 2)
            transform.anchor = AnimationProperty<Point<float>>::staticValue (
                { static_cast<float> (static_cast<double> ((*arr)[0])),
                  static_cast<float> (static_cast<double> ((*arr)[1])) });
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
    else if (ty == 5) // Text — parsed as NullLayer (text rendering not yet supported)
    {
        layer = new NullLayer();
    }
    else // Null (ty == 3) or unknown
    {
        layer = new NullLayer();
    }

    if (layer == nullptr)
        return {};

    // Self-parenting check (gap 22)
    if (layer->parentId >= 0 && layer->id == layer->parentId)
    {
        if (errorOut_ != nullptr)
            *errorOut_ = "Invalid Lottie: layer references itself as parent";
        return {};
    }

    // Hidden layers — downgrade to Null to save resources (gap 23)
    if (layer->hidden)
    {
        layer = new NullLayer();
        layer->name = varString (layerObj["nm"]);
        layer->id = varInt (layerObj["ind"], -1);
        layer->parentId = varInt (layerObj["parent"], -1);
        layer->inFrame = varFloat (layerObj["ip"]);
        layer->outFrame = varFloat (layerObj["op"]);
        layer->startFrame = varFloat (layerObj["st"]);
        layer->timeStretch = varFloat (layerObj["sr"], 1.0f);
        layer->hidden = true;
        layer->blendMode = static_cast<BlendMode> (varInt (layerObj["bm"]));
        layer->isMatteSource = varInt (layerObj["td"]) != 0;
        parseTransform (layerObj["ks"], layer->transform, (bool) layerObj["ddd"]);
        return layer;
    }

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
    layer->isMatteSource = varInt (layerObj["td"]) != 0;

    parseTransform (layerObj["ks"], layer->transform, (bool) layerObj["ddd"]);
    parseMasks (layerObj["masksProperties"], *layer);
    parseEffects (layerObj["ef"], *layer);

    if (! layerObj["tm"].isVoid())
    {
        FloatProperty tr;
        tr = parseProperty<float> (layerObj["tm"], extractFloat);
        layer->timeRemap = std::move (tr);

        const String expression = varString (layerObj["tm"]["x"]);
        layer->timeRemapLoopOutCycle = expression.containsIgnoreCase ("loopOut")
                                    && expression.containsIgnoreCase ("cycle");
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

    resolveShapeLayerExpressions (*arr, layer);
}

void LottieReader::resolveShapeLayerExpressions (const Array<var>& itemsArray, ShapeLayer& layer)
{
    LottieExpressionEvaluator evaluator;
    evaluator.setupShapeContext (layer);

    for (const var& item : itemsArray)
    {
        if (varString (item["ty"]) != "gr")
            continue;

        AnimationGroup* targetGroup = findShapeLayerGroupByName (layer, varString (item["nm"]));
        if (targetGroup == nullptr)
            continue;

        const auto* childItems = safeArray (item["it"]);
        if (childItems == nullptr)
            continue;

        for (const var& childItem : *childItems)
        {
            const String childType = varString (childItem["ty"]);

            if (childType == "sh")
            {
                const String expr = varString (childItem["ks"]["x"]);
                if (expr.isEmpty())
                    continue;

                const auto result = evaluator.evaluate (expr);
                if (result.kind != LottieExpressionEvaluator::EvalResult::Kind::ShapeContentRef)
                    continue;
                if (result.contentProperty != "path")
                    continue;

                const AnimationGroup* sourceGroup = findShapeLayerGroupByName (layer, result.contentGroupName);
                if (sourceGroup == nullptr)
                    continue;

                const auto* sourcePath = findBezierPathByName (*sourceGroup, result.contentItemName);
                auto* targetPath = findBezierPathByName (*targetGroup, varString (childItem["nm"]));
                if (sourcePath != nullptr && targetPath != nullptr)
                    targetPath->pathData = sourcePath->pathData;
            }
            else if (childType == "tr")
            {
                const String expr = varString (childItem["r"]["x"]);
                if (expr.isEmpty())
                    continue;

                const auto result = evaluator.evaluate (expr);
                if (result.kind != LottieExpressionEvaluator::EvalResult::Kind::ShapeContentRef)
                    continue;
                if (result.contentProperty != "transform.rotation")
                    continue;

                const AnimationGroup* sourceGroup = findShapeLayerGroupByName (layer, result.contentGroupName);
                if (sourceGroup != nullptr)
                    targetGroup->transform.rotation = sourceGroup->transform.rotation;
            }
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

    resolveGroupExpressions (*arr, group);
}

void LottieReader::resolveGroupExpressions (const Array<var>& itemsArray, AnimationGroup& group)
{
    LottieExpressionEvaluator evaluator;
    evaluator.setupGroupContext (group);

    for (const var& item : itemsArray)
    {
        if (varString (item["ty"]) != "gr")
            continue;

        AnimationGroup* targetGroup = findChildGroupByName (group, varString (item["nm"]));
        if (targetGroup == nullptr)
            continue;

        const auto* childItems = safeArray (item["it"]);
        if (childItems == nullptr)
            continue;

        for (const var& childItem : *childItems)
        {
            const String childType = varString (childItem["ty"]);

            if (childType == "sh")
            {
                const String expr = varString (childItem["ks"]["x"]);
                if (expr.isEmpty())
                    continue;

                const auto result = evaluator.evaluate (expr);
                if (result.kind != LottieExpressionEvaluator::EvalResult::Kind::ShapeContentRef)
                    continue;
                if (result.contentProperty != "path")
                    continue;

                const AnimationGroup* sourceGroup = findChildGroupByName (group, result.contentGroupName);
                if (sourceGroup == nullptr)
                    continue;

                const auto* sourcePath = findBezierPathByName (*sourceGroup, result.contentItemName);
                auto* targetPath = findBezierPathByName (*targetGroup, varString (childItem["nm"]));
                if (sourcePath != nullptr && targetPath != nullptr)
                    targetPath->pathData = sourcePath->pathData;
            }
            else if (childType == "tr")
            {
                const String expr = varString (childItem["r"]["x"]);
                if (expr.isEmpty())
                    continue;

                const auto result = evaluator.evaluate (expr);
                if (result.kind != LottieExpressionEvaluator::EvalResult::Kind::ShapeContentRef)
                    continue;
                if (result.contentProperty != "transform.rotation")
                    continue;

                const AnimationGroup* sourceGroup = findChildGroupByName (group, result.contentGroupName);
                if (sourceGroup != nullptr)
                    targetGroup->transform.rotation = sourceGroup->transform.rotation;
            }
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
        fl->enabled = ! itemObj["fillEnabled"].isVoid() ? (bool) itemObj["fillEnabled"] : true;
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
        st->enabled = ! itemObj["fillEnabled"].isVoid() ? (bool) itemObj["fillEnabled"] : true;
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
            gs->enabled = ! itemObj["fillEnabled"].isVoid() ? (bool) itemObj["fillEnabled"] : true;
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
            gf->enabled = ! itemObj["fillEnabled"].isVoid() ? (bool) itemObj["fillEnabled"] : true;
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

        // Pre-compute max copies across all keyframes
        {
            float maxCopy = 0.0f;
            if (rp->copies.isAnimated())
            {
                for (const auto& kf : rp->copies.getKeyframes())
                    maxCopy = jmax (maxCopy, kf.value);
            }
            else
            {
                maxCopy = rp->copies.getStaticValue();
            }
            rp->maxCopies = jmax (1.0f, maxCopy);
        }

        const var& tr = itemObj["tr"];
        if (! tr.isVoid())
        {
            parseTransform (tr, rp->copyTransform);
            rp->startOpacity = parseProperty<float> (tr["so"], extractFloat);
            rp->endOpacity = parseProperty<float> (tr["eo"], extractFloat);
        }
    }
    else if (ty == "rd") // Rounded Corner
    {
        auto* rd = group.addRoundedCorner();
        rd->name = varString (itemObj["nm"]);
        rd->hidden = (bool) itemObj["hd"];
        rd->radius = parseProperty<float> (itemObj["r"], extractFloat);
    }
    else if (ty == "mm") // Merge Paths — not yet supported (gap 26)
    {
        if (errorOut_ != nullptr)
            *errorOut_ = "Merge Path (mm) is not supported yet";
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
    gradient.numColorPoints = colorPoints;
    const var& gk = gradObj["g"]["k"];

    // Parse gradient stops from the flat float array
    if (gk.isObject())
    {
        const bool isAnimated = varInt (gk["a"]) == 1;

        if (! isAnimated)
        {
            // Static gradient — parse the flat array once
            if (const auto* arr = safeArray (gk["k"]))
            {
                std::vector<float> flat;
                for (const var& v : *arr)
                    flat.push_back (varFloat (v));
                for (const auto& [pos, col] : AnimationGradient::parseStopsFromFlatArray (flat, colorPoints))
                    gradient.addColorStop (pos, col);
            }
        }
        else
        {
            // Animated gradient — store all keyframes for runtime interpolation
            if (const auto* kfs = safeArray (gk["k"]))
            {
                for (const var& kf : *kfs)
                {
                    AnimationGradient::GradientKeyframe gkf;
                    gkf.frame = varFloat (kf["t"]);

                    if (const auto* sArr = safeArray (kf["s"]))
                    {
                        for (const var& v : *sArr)
                            gkf.values.push_back (varFloat (v));
                    }
                    gradient.animatedStops.push_back (std::move (gkf));
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

void LottieReader::parseEffects (const var& effectsVal, AnimationLayer& layer)
{
    const auto* arr = safeArray (effectsVal);
    if (arr == nullptr)
        return;

    for (const var& effect : *arr)
    {
        const String effectName = varString (effect["nm"]);
        const String effectMatchName = varString (effect["mn"]);

        if (effectMatchName == "ADBE Fill" || effectName == "Fill")
        {
            AnimationLayer::FillEffect fill;
            fill.enabled = effect["en"].isVoid() || (bool) effect["en"];

            const auto* params = safeArray (effect["ef"]);
            if (params != nullptr)
            {
                for (const var& param : *params)
                {
                    const String paramName = varString (param["nm"]);
                    const String paramMatchName = varString (param["mn"]);
                    const var& value = param["v"];

                    if (paramName == "Color" || paramMatchName == "ADBE Fill-0002")
                        fill.color = parseProperty<Color> (value, extractColor);
                    else if (paramName == "Opacity" || paramMatchName == "ADBE Fill-0005")
                        fill.opacity = parseProperty<float> (value, extractFloat);
                }
            }

            layer.fillEffect = std::move (fill);
            continue;
        }

        if (effectMatchName != "ADBE Drop Shadow" && effectName != "Drop Shadow")
            continue;

        AnimationLayer::DropShadow shadow;
        shadow.enabled = effect["en"].isVoid() || (bool) effect["en"];

        const auto* params = safeArray (effect["ef"]);
        if (params == nullptr)
        {
            layer.dropShadow = std::move (shadow);
            continue;
        }

        for (const var& param : *params)
        {
            const String paramName = varString (param["nm"]);
            const String paramMatchName = varString (param["mn"]);
            const var& value = param["v"];

            if (paramName == "Shadow Color" || paramMatchName == "ADBE Drop Shadow-0001")
                shadow.color = parseProperty<Color> (value, extractColor);
            else if (paramName == "Opacity" || paramMatchName == "ADBE Drop Shadow-0002")
                shadow.opacity = parseProperty<float> (value, extractFloat);
            else if (paramName == "Direction" || paramMatchName == "ADBE Drop Shadow-0003")
                shadow.direction = parseProperty<float> (value, extractFloat);
            else if (paramName == "Distance" || paramMatchName == "ADBE Drop Shadow-0004")
                shadow.distance = parseProperty<float> (value, extractFloat);
            else if (paramName == "Softness" || paramMatchName == "ADBE Drop Shadow-0005")
                shadow.softness = parseProperty<float> (value, extractFloat);
            else if (paramName == "Shadow Only" || paramMatchName == "ADBE Drop Shadow-0006")
                shadow.shadowOnly = varFloat (value["k"]) != 0.0f;
        }

        layer.dropShadow = std::move (shadow);
    }
}

//==============================================================================
void LottieReader::parseTransform (const var& ksObj, AnimationTransform& t, bool ddd)
{
    if (ksObj.isVoid())
        return;

    t.is3DData = ddd;

    t.anchor = parseProperty<Point<float>> (ksObj["a"], extractPoint);
    t.rotation = parseProperty<float> (ksObj["r"], extractFloat);
    t.opacity = parseProperty<float> (ksObj["o"], extractFloat);
    t.skew = parseProperty<float> (ksObj["sk"], extractFloat);
    t.skewAxis = parseProperty<float> (ksObj["sa"], extractFloat);

    if (ddd)
    {
        t.rotationX = parseProperty<float> (ksObj["rx"], extractFloat);
        t.rotationY = parseProperty<float> (ksObj["ry"], extractFloat);
        t.rotationZ = parseProperty<float> (ksObj["rz"], extractFloat);
    }

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
        // Parse position with spatial keyframe support
        const int animated = varInt (pObj["a"]);
        const var& k = pObj["k"];

        if (animated == 0 || k.isVoid())
        {
            if (k.isVoid())
                t.position = AnimationProperty<Point<float>>::staticValue (extractPoint (pObj));
            else
                t.position = AnimationProperty<Point<float>>::staticValue (extractPoint (k));
        }
        else
        {
            const auto* kfs = safeArray (k);
            if (kfs != nullptr)
            {
                const bool hasSpatial = kfs->size() >= 1 && ! (*kfs)[0]["ti"].isVoid();

                if (hasSpatial)
                {
                    // Parse position with spatial tangents — store keyframes and tangents
                    typename AnimationProperty<Point<float>>::Builder builder;
                    bool hasPreviousValue = false;
                    Point<float> previousValue {};
                    bool hasPreviousEndValue = false;
                    Point<float> previousEndValue {};

                    for (int i = 0; i < kfs->size(); ++i)
                    {
                        const var& kf = (*kfs)[i];
                        const float frame = varFloat (kf["t"]);
                        const var& startVal = kf["s"];
                        const var& endVal = kf["e"];

                        Point<float> value {};
                        if (! startVal.isVoid() && ! startVal.isUndefined())
                            value = extractPoint (startVal);
                        else if (hasPreviousEndValue)
                            value = previousEndValue;
                        else if (hasPreviousValue)
                            value = previousValue;
                        else if (! endVal.isVoid() && ! endVal.isUndefined())
                            value = extractPoint (endVal);

                        AnimationEasing easing = parseEasing (kf);

                        SpatialPositionKeyframe spk;
                        spk.frame = frame;
                        spk.value = value;
                        if (! endVal.isVoid() && ! endVal.isUndefined())
                        {
                            const Point<float> endValue = extractPoint (endVal);
                            spk.endValue = endValue;
                            builder.keyframe (frame, value, endValue, easing);
                            previousEndValue = endValue;
                            hasPreviousEndValue = true;
                        }
                        else
                        {
                            builder.keyframe (frame, value, easing);
                            previousEndValue = value;
                            hasPreviousEndValue = true;
                        }

                        spk.tangentIn = extractPoint (kf["ti"]);
                        spk.tangentOut = extractPoint (kf["to"]);
                        spk.easing = std::move (easing);
                        t.spatialKeyframes.push_back (std::move (spk));

                        previousValue = value;
                        hasPreviousValue = true;
                    }
                    t.position = builder.build();
                }
                else
                {
                    t.position = parseProperty<Point<float>> (pObj, extractPoint);
                }
            }
            else
            {
                t.position = parseProperty<Point<float>> (pObj, extractPoint);
            }
        }
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
        {
            const var& single = (*kfs)[0];
            const var& startVal = single["s"];
            const var& endVal = single["e"];

            if (! startVal.isVoid() && ! startVal.isUndefined())
                return AnimationProperty<T>::staticValue (extractor (startVal));

            if (! endVal.isVoid() && ! endVal.isUndefined())
                return AnimationProperty<T>::staticValue (extractor (endVal));
        }

        return {};
    }

    typename AnimationProperty<T>::Builder builder;
    bool hasPreviousValue = false;
    T previousValue {};
    bool hasPreviousEndValue = false;
    T previousEndValue {};

    for (int i = 0; i < kfs->size(); ++i)
    {
        const var& kf = (*kfs)[i];
        const float frame = varFloat (kf["t"]);
        const var& startVal = kf["s"];
        const var& endVal = kf["e"];

        T value {};
        if (! startVal.isVoid() && ! startVal.isUndefined())
        {
            value = extractor (startVal);
        }
        else if (hasPreviousEndValue)
        {
            value = previousEndValue;
        }
        else if (hasPreviousValue)
        {
            value = previousValue;
        }
        else if (! endVal.isVoid() && ! endVal.isUndefined())
        {
            value = extractor (endVal);
        }

        AnimationEasing easing = parseEasing (kf);

        previousValue = value;
        hasPreviousValue = true;

        if (! endVal.isVoid() && ! endVal.isUndefined())
        {
            previousEndValue = extractor (endVal);
            hasPreviousEndValue = true;
            builder.keyframe (frame, value, previousEndValue, std::move (easing));
        }
        else
        {
            previousEndValue = previousValue;
            hasPreviousEndValue = true;
            builder.keyframe (frame, value, std::move (easing));
        }
    }

    return builder.build();
}

AnimationEasing LottieReader::parseEasing (const var& kfObj)
{
    if (varInt (kfObj["h"]) == 1)
        return AnimationEasing::hold();

    const var& o = kfObj["o"];
    const var& i = kfObj["i"];

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

    // Check for named interpolator reference ("n" key)
    const String namedInterpolator = varString (kfObj["n"]);
    if (namedInterpolator.isNotEmpty())
        return lookupInterpolator (namedInterpolator, ox, oy, ix, iy);

    return AnimationEasing::fromLottieTangents ({ ox, oy }, { ix, iy });
}

AnimationEasing LottieReader::lookupInterpolator (const String& name, float ox, float oy, float ix, float iy)
{
    if (interpolatorCache.contains (name))
        return interpolatorCache[name];

    AnimationEasing easing = AnimationEasing::fromLottieTangents ({ ox, oy }, { ix, iy });
    interpolatorCache.set (name, easing);
    return easing;
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
