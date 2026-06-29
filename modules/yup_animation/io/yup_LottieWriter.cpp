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
// LottieWriter

String LottieWriter::toJson (const AnimationComposition& comp, bool prettyPrint)
{
    var root = serializeComposition (comp);
    return JSON::toString (root, prettyPrint);
}

Result LottieWriter::toFile (const AnimationComposition& comp, const File& destination, bool prettyPrint)
{
    const String json = toJson (comp, prettyPrint);

    if (! destination.replaceWithText (json))
        return Result::fail ("Failed to write to file: " + destination.getFullPathName());

    return Result::ok();
}

//==============================================================================
// Private serialisation helpers

var LottieWriter::serializeComposition (const AnimationComposition& comp)
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("v", var ("5.5.2"));
    obj->setProperty ("nm", var (comp.name));
    obj->setProperty ("ip", var ((int) comp.startFrame));
    obj->setProperty ("op", var ((int) comp.endFrame));
    obj->setProperty ("fr", var ((double) comp.frameRate));
    obj->setProperty ("w", var ((int) comp.size.getWidth()));
    obj->setProperty ("h", var ((int) comp.size.getHeight()));
    obj->setProperty ("ddd", var (0));

    obj->setProperty ("layers", serializeLayers (comp.layers));

    // Assets
    if (comp.assets.size() > 0)
        obj->setProperty ("assets", serializeAssets (comp.assets));

    // Markers
    if (! comp.markers.empty())
        obj->setProperty ("markers", serializeMarkers (comp.markers));

    return var (obj);
}

var LottieWriter::serializeLayers (const std::vector<AnimationLayer::Ptr>& layers)
{
    Array<var> arr;
    for (const auto& layer : layers)
        if (layer != nullptr)
            arr.add (serializeLayer (*layer));
    return var (arr);
}

var LottieWriter::serializeLayer (const AnimationLayer& layer)
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("nm", var (layer.name));
    obj->setProperty ("ind", var (layer.id));
    obj->setProperty ("ip", var ((double) layer.inFrame));
    obj->setProperty ("op", var ((double) layer.outFrame));
    obj->setProperty ("st", var ((double) layer.startFrame));
    obj->setProperty ("sr", var ((double) layer.timeStretch));
    obj->setProperty ("ddd", var (0));
    obj->setProperty ("hd", var (layer.hidden));
    obj->setProperty ("bm", var (static_cast<int> (layer.blendMode)));
    obj->setProperty ("tt", var (static_cast<int> (layer.matteType)));

    if (layer.parentId >= 0)
        obj->setProperty ("parent", var (layer.parentId));

    obj->setProperty ("ks", serializeTransform (layer.transform));

    if (! layer.masks.empty())
        obj->setProperty ("masksProperties", serializeMasks (layer.masks));

    switch (layer.getType())
    {
        case AnimationLayer::Type::Shape:
        {
            obj->setProperty ("ty", var (4));
            const auto& sl = static_cast<const ShapeLayer&> (layer);
            obj->setProperty ("shapes", serializeGroups (sl.groups));
            break;
        }

        case AnimationLayer::Type::Solid:
        {
            obj->setProperty ("ty", var (1));
            const auto& sl = static_cast<const SolidLayer&> (layer);
            obj->setProperty ("sc", var (sl.solidColor.toString()));
            obj->setProperty ("sw", var ((int) sl.layerSize.getWidth()));
            obj->setProperty ("sh", var ((int) sl.layerSize.getHeight()));
            break;
        }

        case AnimationLayer::Type::Image:
        {
            obj->setProperty ("ty", var (2));
            const auto& il = static_cast<const ImageLayer&> (layer);
            obj->setProperty ("refId", var (il.assetRefId));
            break;
        }

        case AnimationLayer::Type::Null:
            obj->setProperty ("ty", var (3));
            break;

        case AnimationLayer::Type::Precomp:
        {
            obj->setProperty ("ty", var (0));
            const auto& pl = static_cast<const PrecompLayer&> (layer);
            obj->setProperty ("refId", var (pl.precompRefId));
            obj->setProperty ("w", var ((int) pl.layerSize.getWidth()));
            obj->setProperty ("h", var ((int) pl.layerSize.getHeight()));
            break;
        }

        case AnimationLayer::Type::Text:
            break;
    }

    return var (obj);
}

var LottieWriter::serializeGroups (const std::vector<AnimationGroup::Ptr>& groups)
{
    Array<var> arr;
    for (const auto& g : groups)
    {
        if (g != nullptr)
            arr.add (serializeGroup (*g));
    }
    return var (arr);
}

var LottieWriter::serializeGroup (const AnimationGroup& group)
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("ty", var ("gr"));
    obj->setProperty ("nm", var (group.name));
    obj->setProperty ("hd", var (group.hidden));
    obj->setProperty ("bm", var (static_cast<int> (group.blendMode)));

    Array<var> items;
    for (const auto& child : group.children)
        items.add (serializeChildItem (child));

    // Transform goes last in the "it" array
    items.add (serializeTransform (group.transform));

    obj->setProperty ("it", var (items));
    return var (obj);
}

var LottieWriter::serializeChildItem (const AnimationGroup::ChildItem& item)
{
    switch (item.kind)
    {
        case AnimationGroup::ChildKind::Shape:
            if (item.shape != nullptr)
                return serializeShape (*item.shape);
            break;

        case AnimationGroup::ChildKind::Group:
            if (item.group != nullptr)
                return serializeGroup (*item.group);
            break;

        case AnimationGroup::ChildKind::Fill:
            if (item.fill != nullptr)
                return serializeFill (*item.fill);
            break;

        case AnimationGroup::ChildKind::Stroke:
            if (item.stroke != nullptr)
                return serializeStroke (*item.stroke);
            break;

        case AnimationGroup::ChildKind::Trim:
            if (item.trim != nullptr)
                return serializeTrim (*item.trim);
            break;

        case AnimationGroup::ChildKind::Repeater:
            if (item.repeater != nullptr)
                return serializeRepeater (*item.repeater);
            break;

        case AnimationGroup::ChildKind::RoundedCorner:
            if (item.roundedCorner != nullptr)
                return serializeRoundedCorner (*item.roundedCorner);
            break;
    }

    return {};
}

var LottieWriter::serializeShape (const AnimationShape& shape)
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("nm", var (shape.getName()));
    obj->setProperty ("hd", var (shape.isHidden()));

    switch (shape.getKind())
    {
        case AnimationShape::Kind::Ellipse:
        {
            obj->setProperty ("ty", var ("el"));
            const auto& el = static_cast<const EllipseShape&> (shape);
            obj->setProperty ("p", serializeProperty<Point<float>> (el.center, serializePoint));
            obj->setProperty ("s", serializeProperty<Size<float>> (el.size, serializeSize));
            break;
        }

        case AnimationShape::Kind::Rect:
        {
            obj->setProperty ("ty", var ("rc"));
            const auto& rc = static_cast<const RectShape&> (shape);
            obj->setProperty ("p", serializeProperty<Point<float>> (rc.position, serializePoint));
            obj->setProperty ("s", serializeProperty<Size<float>> (rc.size, serializeSize));
            obj->setProperty ("r", serializeProperty<float> (rc.roundness, serializeFloat));
            break;
        }

        case AnimationShape::Kind::BezierPath:
        {
            obj->setProperty ("ty", var ("sh"));
            const auto& sh = static_cast<const BezierPathShape&> (shape);
            obj->setProperty ("ks", serializeProperty<AnimationPathData> (sh.pathData, serializePath));
            break;
        }

        case AnimationShape::Kind::Polystar:
        {
            obj->setProperty ("ty", var ("sr"));
            const auto& sr = static_cast<const PolystarShape&> (shape);
            obj->setProperty ("sy", var (sr.starType == PolystarShape::StarType::Star ? 1 : 2));
            obj->setProperty ("p", serializeProperty<Point<float>> (sr.position, serializePoint));
            obj->setProperty ("pt", serializeProperty<float> (sr.points, serializeFloat));
            obj->setProperty ("or", serializeProperty<float> (sr.outerRadius, serializeFloat));
            obj->setProperty ("os", serializeProperty<float> (sr.outerRoundness, serializeFloat));
            obj->setProperty ("r", serializeProperty<float> (sr.rotation, serializeFloat));
            if (sr.starType == PolystarShape::StarType::Star)
            {
                obj->setProperty ("ir", serializeProperty<float> (sr.innerRadius, serializeFloat));
                obj->setProperty ("is", serializeProperty<float> (sr.innerRoundness, serializeFloat));
            }
            break;
        }
    }

    return var (obj);
}

var LottieWriter::serializeFill (const FillPaint& fill)
{
    if (fill.gradient != nullptr)
    {
        DynamicObject* obj = new DynamicObject();
        obj->setProperty ("ty", var ("gf"));
        obj->setProperty ("nm", var (fill.name));
        obj->setProperty ("hd", var (fill.hidden));
        obj->setProperty ("r", var (static_cast<int> (fill.fillRule)));
        obj->setProperty ("o", serializeProperty<float> (fill.opacity, serializeFloat));

        const auto& g = *fill.gradient;
        obj->setProperty ("t", var (static_cast<int> (g.gradientType)));
        obj->setProperty ("s", serializeProperty<Point<float>> (g.startPoint, serializePoint));
        obj->setProperty ("e", serializeProperty<Point<float>> (g.endPoint, serializePoint));
        obj->setProperty ("g", serializeGradient (g));
        return var (obj);
    }

    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("ty", var ("fl"));
    obj->setProperty ("nm", var (fill.name));
    obj->setProperty ("hd", var (fill.hidden));
    obj->setProperty ("r", var (static_cast<int> (fill.fillRule)));
    obj->setProperty ("c", serializeProperty<Color> (fill.color, serializeColor));
    obj->setProperty ("o", serializeProperty<float> (fill.opacity, serializeFloat));
    return var (obj);
}

var LottieWriter::serializeStroke (const StrokePaint& stroke)
{
    if (stroke.gradient != nullptr)
    {
        DynamicObject* obj = new DynamicObject();
        obj->setProperty ("ty", var ("gs"));
        obj->setProperty ("nm", var (stroke.name));
        obj->setProperty ("hd", var (stroke.hidden));
        obj->setProperty ("lc", var (static_cast<int> (stroke.cap) + 1));
        obj->setProperty ("lj", var (static_cast<int> (stroke.join) + 1));
        obj->setProperty ("ml", var ((double) stroke.miterLimit));
        obj->setProperty ("o", serializeProperty<float> (stroke.opacity, serializeFloat));
        obj->setProperty ("w", serializeProperty<float> (stroke.width, serializeFloat));

        const auto& g = *stroke.gradient;
        obj->setProperty ("t", var (static_cast<int> (g.gradientType)));
        obj->setProperty ("s", serializeProperty<Point<float>> (g.startPoint, serializePoint));
        obj->setProperty ("e", serializeProperty<Point<float>> (g.endPoint, serializePoint));
        obj->setProperty ("g", serializeGradient (g));
        return var (obj);
    }

    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("ty", var ("st"));
    obj->setProperty ("nm", var (stroke.name));
    obj->setProperty ("hd", var (stroke.hidden));
    obj->setProperty ("lc", var (static_cast<int> (stroke.cap) + 1));
    obj->setProperty ("lj", var (static_cast<int> (stroke.join) + 1));
    obj->setProperty ("ml", var ((double) stroke.miterLimit));
    obj->setProperty ("c", serializeProperty<Color> (stroke.color, serializeColor));
    obj->setProperty ("o", serializeProperty<float> (stroke.opacity, serializeFloat));
    obj->setProperty ("w", serializeProperty<float> (stroke.width, serializeFloat));

    if (! stroke.dashArray.empty())
    {
        Array<var> dashes;
        for (const auto& d : stroke.dashArray)
        {
            DynamicObject* de = new DynamicObject();
            switch (d.kind)
            {
                case StrokeDash::Kind::Dash:
                    de->setProperty ("n", var ("d"));
                    break;

                case StrokeDash::Kind::Gap:
                    de->setProperty ("n", var ("g"));
                    break;

                case StrokeDash::Kind::Offset:
                    de->setProperty ("n", var ("o"));
                    break;
            }

            de->setProperty ("v", serializeProperty<float> (d.value, serializeFloat));
            dashes.add (var (de));
        }

        obj->setProperty ("d", var (dashes));
    }

    return var (obj);
}

var LottieWriter::serializeGradient (const AnimationGradient& gradient)
{
    // Lottie packs gradient stops as a flat float array: [pos r g b a, pos r g b a, ...]
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("p", var (gradient.numColorPoints > 0 ? gradient.numColorPoints : static_cast<int> (gradient.colorStops.size())));

    DynamicObject* kProp = new DynamicObject();

    if (! gradient.animatedStops.empty())
    {
        kProp->setProperty ("a", var (1));

        Array<var> kfArray;
        for (const auto& gkf : gradient.animatedStops)
        {
            DynamicObject* kfObj = new DynamicObject();
            kfObj->setProperty ("t", var ((double) gkf.frame));

            Array<var> flatArr;
            for (float v : gkf.values)
                flatArr.add (var ((double) v));

            kfObj->setProperty ("s", var (flatArr));
            kfArray.add (var (kfObj));
        }

        kProp->setProperty ("k", var (kfArray));
    }
    else
    {
        kProp->setProperty ("a", var (0));

        Array<var> flat;
        for (const auto& stop : gradient.colorStops)
        {
            const float pos = stop.position.getValueAt (0.0f);
            const Color col = stop.color.getValueAt (0.0f);
            flat.add (var ((double) pos));
            flat.add (var ((double) col.getRedFloat()));
            flat.add (var ((double) col.getGreenFloat()));
            flat.add (var ((double) col.getBlueFloat()));
            flat.add (var ((double) col.getAlphaFloat()));
        }

        kProp->setProperty ("k", var (flat));
    }

    obj->setProperty ("k", var (kProp));
    return var (obj);
}

var LottieWriter::serializeTrim (const AnimationTrim& trim)
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("ty", var ("tm"));
    obj->setProperty ("nm", var (trim.name));
    obj->setProperty ("hd", var (trim.hidden));
    obj->setProperty ("m", var (trim.mode == AnimationTrim::TrimMode::Simultaneously ? 1 : 2));
    obj->setProperty ("s", serializeProperty<float> (trim.start, serializeFloat));
    obj->setProperty ("e", serializeProperty<float> (trim.end, serializeFloat));
    obj->setProperty ("o", serializeProperty<float> (trim.offset, serializeFloat));
    return var (obj);
}

var LottieWriter::serializeRepeater (const AnimationRepeater& repeater)
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("ty", var ("rp"));
    obj->setProperty ("nm", var (repeater.name));
    obj->setProperty ("hd", var (repeater.hidden));
    obj->setProperty ("c", serializeProperty<float> (repeater.copies, serializeFloat));
    obj->setProperty ("o", serializeProperty<float> (repeater.offset, serializeFloat));

    DynamicObject* tr = new DynamicObject();
    tr->setProperty ("so", serializeProperty<float> (repeater.startOpacity, serializeFloat));
    tr->setProperty ("eo", serializeProperty<float> (repeater.endOpacity, serializeFloat));
    const var copyKs = serializeTransform (repeater.copyTransform);
    if (const auto* ksObj = copyKs.getDynamicObject())
    {
        const NamedValueSet& props = ksObj->getProperties();
        for (int i = 0; i < props.size(); ++i)
            tr->setProperty (props.getName (i), props.getValueAt (i));
    }

    obj->setProperty ("tr", var (tr));
    return var (obj);
}

var LottieWriter::serializeRoundedCorner (const AnimationRoundedCorner& rc)
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("ty", var ("rd"));
    obj->setProperty ("nm", var (rc.name));
    obj->setProperty ("hd", var (rc.hidden));
    obj->setProperty ("r", serializeProperty<float> (rc.radius, serializeFloat));
    return var (obj);
}

var LottieWriter::serializeTransform (const AnimationTransform& t)
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("a", serializeProperty<Point<float>> (t.anchor, serializePoint));

    if (t.separatePosition)
    {
        obj->setProperty ("px", serializeProperty<float> (t.positionX, serializeFloat));
        obj->setProperty ("py", serializeProperty<float> (t.positionY, serializeFloat));
    }
    else if (! t.spatialKeyframes.empty())
    {
        // Write position with tangent data for spatial interpolation
        DynamicObject* pObj = new DynamicObject();
        pObj->setProperty ("a", var (1));

        Array<var> kfArray;
        for (const auto& spk : t.spatialKeyframes)
        {
            DynamicObject* kfObj = new DynamicObject();
            kfObj->setProperty ("t", var ((double) spk.frame));
            kfObj->setProperty ("s", serializePoint (spk.value));
            if (spk.endValue.has_value())
                kfObj->setProperty ("e", serializePoint (*spk.endValue));
            kfObj->setProperty ("ti", serializePoint (spk.tangentIn));
            kfObj->setProperty ("to", serializePoint (spk.tangentOut));

            const var easingData = serializeEasing (spk.easing);
            if (const auto* easingObj = easingData.getDynamicObject())
            {
                if (easingObj->hasProperty ("o"))
                    kfObj->setProperty ("o", easingObj->getProperty ("o"));
                if (easingObj->hasProperty ("i"))
                    kfObj->setProperty ("i", easingObj->getProperty ("i"));
            }

            kfArray.add (var (kfObj));
        }

        pObj->setProperty ("k", var (kfArray));
        obj->setProperty ("p", var (pObj));
    }
    else
    {
        obj->setProperty ("p", serializeProperty<Point<float>> (t.position, serializePoint));
    }

    obj->setProperty ("s", serializeProperty<Size<float>> (t.scale, serializeSize));
    obj->setProperty ("r", serializeProperty<float> (t.rotation, serializeFloat));
    obj->setProperty ("o", serializeProperty<float> (t.opacity, serializeFloat));

    if (t.skew.isAnimated() || t.skew.getValueAt (0.0f) != 0.0f)
    {
        obj->setProperty ("sk", serializeProperty<float> (t.skew, serializeFloat));
        obj->setProperty ("sa", serializeProperty<float> (t.skewAxis, serializeFloat));
    }

    if (t.is3DData)
    {
        if (t.rotationX.isAnimated() || t.rotationX.getValueAt (0.0f) != 0.0f)
            obj->setProperty ("rx", serializeProperty<float> (t.rotationX, serializeFloat));
        if (t.rotationY.isAnimated() || t.rotationY.getValueAt (0.0f) != 0.0f)
            obj->setProperty ("ry", serializeProperty<float> (t.rotationY, serializeFloat));
        if (t.rotationZ.isAnimated() || t.rotationZ.getValueAt (0.0f) != 0.0f)
            obj->setProperty ("rz", serializeProperty<float> (t.rotationZ, serializeFloat));
    }

    return var (obj);
}

var LottieWriter::serializeMasks (const std::vector<AnimationMask::Ptr>& masks)
{
    Array<var> arr;

    for (const auto& m : masks)
    {
        if (m != nullptr)
            arr.add (serializeMask (*m));
    }

    return var (arr);
}

var LottieWriter::serializeMask (const AnimationMask& mask)
{
    DynamicObject* obj = new DynamicObject();
    obj->setProperty ("nm", var (mask.name));
    obj->setProperty ("inv", var (mask.inverted));

    String modeStr;
    switch (mask.mode)
    {
        case AnimationMask::Mode::Add:
            modeStr = "a";
            break;

        case AnimationMask::Mode::Subtract:
            modeStr = "s";
            break;

        case AnimationMask::Mode::Intersect:
            modeStr = "i";
            break;

        case AnimationMask::Mode::Difference:
            modeStr = "d";
            break;

        default:
            modeStr = "n";
            break;
    }

    obj->setProperty ("mode", var (modeStr));
    obj->setProperty ("pt", serializeProperty<AnimationPathData> (mask.shape, serializePath));
    obj->setProperty ("o", serializeProperty<float> (mask.opacity, serializeFloat));

    return var (obj);
}

var LottieWriter::serializeAssets (const HashMap<String, AnimationAsset::Ptr>& assets)
{
    Array<var> arr;
    for (HashMap<String, AnimationAsset::Ptr>::Iterator it (assets); it.next();)
    {
        const AnimationAsset* asset = it.getValue().get();
        if (asset == nullptr)
            continue;

        DynamicObject* obj = new DynamicObject();
        obj->setProperty ("id", var (asset->id));

        if (asset->assetType == AnimationAsset::AssetType::Precomp)
        {
            obj->setProperty ("layers", serializeLayers (asset->layers));
        }
        else
        {
            obj->setProperty ("u", var (asset->path));
            obj->setProperty ("p", var (asset->id));
            obj->setProperty ("e", var (0));
            obj->setProperty ("w", var (asset->width));
            obj->setProperty ("h", var (asset->height));
        }

        arr.add (var (obj));
    }

    return var (arr);
}

var LottieWriter::serializeMarkers (const std::vector<AnimationMarker>& markers)
{
    Array<var> arr;

    for (const auto& m : markers)
    {
        DynamicObject* obj = new DynamicObject();
        obj->setProperty ("cm", var (m.comment));
        obj->setProperty ("tm", var ((double) m.startFrame));
        obj->setProperty ("dr", var ((double) m.duration));
        arr.add (var (obj));
    }

    return var (arr);
}

var LottieWriter::serializeEasing (const AnimationEasing& easing)
{
    DynamicObject* out = new DynamicObject();
    DynamicObject* in = new DynamicObject();

    Array<var> ox, oy, ix, iy;
    ox.add (var ((double) easing.getX1()));
    oy.add (var ((double) easing.getY1()));
    ix.add (var ((double) easing.getX2()));
    iy.add (var ((double) easing.getY2()));

    out->setProperty ("x", var (ox));
    out->setProperty ("y", var (oy));
    in->setProperty ("x", var (ix));
    in->setProperty ("y", var (iy));

    DynamicObject* result = new DynamicObject();
    result->setProperty ("o", var (out));
    result->setProperty ("i", var (in));
    return var (result);
}

//==============================================================================
// Value serializers

var LottieWriter::serializeColor (const Color& c)
{
    Array<var> arr;
    arr.add (var ((double) c.getRedFloat()));
    arr.add (var ((double) c.getGreenFloat()));
    arr.add (var ((double) c.getBlueFloat()));
    return var (arr);
}

var LottieWriter::serializePoint (const Point<float>& p)
{
    Array<var> arr;
    arr.add (var ((double) p.getX()));
    arr.add (var ((double) p.getY()));
    return var (arr);
}

var LottieWriter::serializeSize (const Size<float>& s)
{
    Array<var> arr;
    arr.add (var ((double) s.getWidth()));
    arr.add (var ((double) s.getHeight()));
    return var (arr);
}

var LottieWriter::serializeFloat (float v)
{
    return var ((double) v);
}

var LottieWriter::serializePath (const AnimationPathData& pd)
{
    DynamicObject* obj = new DynamicObject();

    Array<var> verts, inT, outT;
    for (const auto& v : pd.vertices)
    {
        Array<var> pt;
        pt.add (var ((double) v.getX()));
        pt.add (var ((double) v.getY()));
        verts.add (var (pt));
    }
    for (const auto& v : pd.inTangents)
    {
        Array<var> pt;
        pt.add (var ((double) v.getX()));
        pt.add (var ((double) v.getY()));
        inT.add (var (pt));
    }
    for (const auto& v : pd.outTangents)
    {
        Array<var> pt;
        pt.add (var ((double) v.getX()));
        pt.add (var ((double) v.getY()));
        outT.add (var (pt));
    }

    obj->setProperty ("v", var (verts));
    obj->setProperty ("i", var (inT));
    obj->setProperty ("o", var (outT));
    obj->setProperty ("c", var (pd.closed));
    return var (obj);
}

//==============================================================================
// Template: serializeProperty<T>

template <typename T>
var LottieWriter::serializeProperty (const AnimationProperty<T>& prop, std::function<var (const T&)> serializer)
{
    DynamicObject* obj = new DynamicObject();

    if (prop.isStatic())
    {
        obj->setProperty ("a", var (0));
        obj->setProperty ("k", serializer (prop.getValueAt (0.0f)));
    }
    else
    {
        obj->setProperty ("a", var (1));

        Array<var> kfArray;
        const auto& keyframes = prop.getKeyframes();
        for (size_t i = 0; i < keyframes.size(); ++i)
        {
            const auto& kf = keyframes[i];
            DynamicObject* kfObj = new DynamicObject();
            kfObj->setProperty ("t", var ((double) kf.frame));

            // Lottie wraps the value in a single-element array for multi-dimensional types
            var serialized = serializer (kf.value);
            if (serialized.isArray())
                kfObj->setProperty ("s", serialized);
            else
            {
                Array<var> sArr;
                sArr.add (serialized);
                kfObj->setProperty ("s", var (sArr));
            }

            // End value for this keyframe interval (Lottie "e").
            if (kf.endValue.has_value() || i + 1 < keyframes.size())
            {
                var endSerialized = serializer (kf.endValue.has_value() ? *kf.endValue : keyframes[i + 1].value);
                if (endSerialized.isArray())
                    kfObj->setProperty ("e", endSerialized);
                else
                {
                    Array<var> eArr;
                    eArr.add (endSerialized);
                    kfObj->setProperty ("e", var (eArr));
                }
            }

            // Easing
            const var easingData = serializeEasing (kf.easing);
            if (const auto* easingObj = easingData.getDynamicObject())
            {
                if (easingObj->hasProperty ("o"))
                    kfObj->setProperty ("o", easingObj->getProperty ("o"));
                if (easingObj->hasProperty ("i"))
                    kfObj->setProperty ("i", easingObj->getProperty ("i"));
            }

            kfArray.add (var (kfObj));
        }

        obj->setProperty ("k", var (kfArray));
    }

    return var (obj);
}

// Explicit instantiations
template var LottieWriter::serializeProperty<float> (const AnimationProperty<float>&, std::function<var (const float&)>);
template var LottieWriter::serializeProperty<Point<float>> (const AnimationProperty<Point<float>>&, std::function<var (const Point<float>&)>);
template var LottieWriter::serializeProperty<Size<float>> (const AnimationProperty<Size<float>>&, std::function<var (const Size<float>&)>);
template var LottieWriter::serializeProperty<Color> (const AnimationProperty<Color>&, std::function<var (const Color&)>);
template var LottieWriter::serializeProperty<AnimationPathData> (const AnimationProperty<AnimationPathData>&, std::function<var (const AnimationPathData&)>);

} // namespace yup
