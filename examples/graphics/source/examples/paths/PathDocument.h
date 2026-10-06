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

#pragma once

namespace PathEditor
{

//==============================================================================

namespace Ids
{
inline const yup::Identifier document { "PathDocument" };
inline const yup::Identifier layer { "Layer" };
inline const yup::Identifier fill { "Fill" };
inline const yup::Identifier stroke { "Stroke" };
inline const yup::Identifier stop { "Stop" };
inline const yup::Identifier contour { "Contour" };
inline const yup::Identifier vertex { "Vertex" };

inline const yup::Identifier width { "width" };
inline const yup::Identifier height { "height" };
inline const yup::Identifier background { "background" };
inline const yup::Identifier name { "name" };
inline const yup::Identifier visible { "visible" };
inline const yup::Identifier opacity { "opacity" };
inline const yup::Identifier blendMode { "blendMode" };
inline const yup::Identifier feather { "feather" };
inline const yup::Identifier additiveAmount { "additiveAmount" };
inline const yup::Identifier tint { "tint" };
inline const yup::Identifier useAs { "useAs" };
inline const yup::Identifier image { "image" };
inline const yup::Identifier wrapX { "wrapX" };
inline const yup::Identifier wrapY { "wrapY" };
inline const yup::Identifier filter { "filter" };
inline const yup::Identifier imageScale { "imageScale" };
inline const yup::Identifier imageRotation { "imageRotation" };
inline const yup::Identifier imageSkewX { "imageSkewX" };
inline const yup::Identifier imageSkewY { "imageSkewY" };
inline const yup::Identifier position { "position" };
inline const yup::Identifier fillRule { "fillRule" };
inline const yup::Identifier kind { "kind" };
inline const yup::Identifier color { "color" };
inline const yup::Identifier x1 { "x1" };
inline const yup::Identifier y1 { "y1" };
inline const yup::Identifier x2 { "x2" };
inline const yup::Identifier y2 { "y2" };
inline const yup::Identifier join { "join" };
inline const yup::Identifier cap { "cap" };
inline const yup::Identifier offset { "offset" };
inline const yup::Identifier closed { "closed" };
inline const yup::Identifier x { "x" };
inline const yup::Identifier y { "y" };
inline const yup::Identifier inX { "inX" };
inline const yup::Identifier inY { "inY" };
inline const yup::Identifier outX { "outX" };
inline const yup::Identifier outY { "outY" };
inline const yup::Identifier smooth { "smooth" };
} // namespace Ids

//==============================================================================

inline constexpr const char* documentSchemaJson = R"({
  "nodeTypes": {
    "PathDocument": {
      "properties": {
        "width": { "type": "number", "default": 640, "minimum": 16, "maximum": 4096 },
        "height": { "type": "number", "default": 480, "minimum": 16, "maximum": 4096 },
        "background": { "type": "string", "default": "#1c2030ff" }
      },
      "children": { "allowedTypes": ["Layer"] }
    },
    "Layer": {
      "properties": {
        "name": { "type": "string", "default": "Layer" },
        "visible": { "type": "boolean", "default": true },
        "opacity": { "type": "number", "default": 1, "minimum": 0, "maximum": 1 },
        "blendMode": { "type": "string", "default": "normal",
                       "enum": ["normal", "screen", "overlay", "darken", "lighten", "color-dodge", "color-burn", "hard-light",
                                "soft-light", "difference", "exclusion", "multiply", "hue", "saturation", "color", "luminosity",
                                "plus-lighter"] },
        "additiveAmount": { "type": "number", "default": 1, "minimum": 0, "maximum": 1 },
        "tint": { "type": "string", "default": "#ffffffff" },
        "useAs": { "type": "string", "default": "layer",
                   "enum": ["layer", "clip-fill", "clip-stroke", "mask-alpha", "mask-inverted-alpha", "mask-luminance", "mask-inverted-luminance"] },
        "feather": { "type": "number", "default": 0, "minimum": 0, "maximum": 200 },
        "fillRule": { "type": "string", "default": "nonzero", "enum": ["nonzero", "evenodd"] }
      },
      "children": { "allowedTypes": ["Fill", "Stroke", "Contour"] }
    },
    "Fill": {
      "properties": {
        "kind": { "type": "string", "default": "solid", "enum": ["none", "solid", "linear", "radial", "image"] },
        "color": { "type": "string", "default": "#4f8cffff" },
        "image": { "type": "string", "default": "" },
        "wrapX": { "type": "string", "default": "repeat", "enum": ["clamp", "repeat", "mirror"] },
        "wrapY": { "type": "string", "default": "repeat", "enum": ["clamp", "repeat", "mirror"] },
        "filter": { "type": "string", "default": "smooth", "enum": ["smooth", "pixelated"] },
        "imageScale": { "type": "number", "default": 1, "minimum": 0.01, "maximum": 20 },
        "imageRotation": { "type": "number", "default": 0, "minimum": -180, "maximum": 180 },
        "imageSkewX": { "type": "number", "default": 0, "minimum": -60, "maximum": 60 },
        "imageSkewY": { "type": "number", "default": 0, "minimum": -60, "maximum": 60 },
        "x1": { "type": "number", "default": 0 },
        "y1": { "type": "number", "default": 0 },
        "x2": { "type": "number", "default": 0 },
        "y2": { "type": "number", "default": 0 }
      },
      "children": { "allowedTypes": ["Stop"] }
    },
    "Stroke": {
      "properties": {
        "kind": { "type": "string", "default": "none", "enum": ["none", "solid", "linear", "radial", "image"] },
        "color": { "type": "string", "default": "#ffffffff" },
        "image": { "type": "string", "default": "" },
        "wrapX": { "type": "string", "default": "repeat", "enum": ["clamp", "repeat", "mirror"] },
        "wrapY": { "type": "string", "default": "repeat", "enum": ["clamp", "repeat", "mirror"] },
        "filter": { "type": "string", "default": "smooth", "enum": ["smooth", "pixelated"] },
        "imageScale": { "type": "number", "default": 1, "minimum": 0.01, "maximum": 20 },
        "imageRotation": { "type": "number", "default": 0, "minimum": -180, "maximum": 180 },
        "imageSkewX": { "type": "number", "default": 0, "minimum": -60, "maximum": 60 },
        "imageSkewY": { "type": "number", "default": 0, "minimum": -60, "maximum": 60 },
        "x1": { "type": "number", "default": 0 },
        "y1": { "type": "number", "default": 0 },
        "x2": { "type": "number", "default": 0 },
        "y2": { "type": "number", "default": 0 },
        "width": { "type": "number", "default": 2, "minimum": 0, "maximum": 100 },
        "join": { "type": "string", "default": "round", "enum": ["miter", "round", "bevel"] },
        "cap": { "type": "string", "default": "round", "enum": ["butt", "round", "square"] },
        "position": { "type": "string", "default": "center", "enum": ["inside", "center", "outside"] }
      },
      "children": { "allowedTypes": ["Stop"] }
    },
    "Stop": {
      "properties": {
        "offset": { "type": "number", "default": 0, "minimum": 0, "maximum": 1 },
        "color": { "type": "string", "default": "#ffffffff" }
      },
      "children": { "maxCount": 0 }
    },
    "Contour": {
      "properties": {
        "closed": { "type": "boolean", "default": true }
      },
      "children": { "allowedTypes": ["Vertex"] }
    },
    "Vertex": {
      "properties": {
        "x": { "type": "number", "default": 0 },
        "y": { "type": "number", "default": 0 },
        "inX": { "type": "number", "default": 0 },
        "inY": { "type": "number", "default": 0 },
        "outX": { "type": "number", "default": 0 },
        "outY": { "type": "number", "default": 0 },
        "smooth": { "type": "boolean", "default": false }
      },
      "children": { "maxCount": 0 }
    }
  }
})";

//==============================================================================

/** CSS names of the blend modes, indexed by yup::BlendMode (same order as the schema enum). */
inline constexpr const char* blendModeNames[] = {
    "normal", "screen", "overlay", "darken", "lighten", "color-dodge", "color-burn", "hard-light",
    "soft-light", "difference", "exclusion", "multiply", "hue", "saturation", "color", "luminosity",
    "plus-lighter"
};

inline constexpr const char* paintKindNames[] = { "none", "solid", "linear", "radial", "image" };
inline constexpr const char* imageWrapNames[] = { "clamp", "repeat", "mirror" };
inline constexpr const char* imageFilterNames[] = { "smooth", "pixelated" };
inline constexpr const char* strokePositionNames[] = { "inside", "center", "outside" };

/** How a layer is used: drawn, or acting as a clip or a mask (in yup::LayerMaskMode order) on the layer below it. */
inline constexpr const char* useAsNames[] = {
    "layer", "clip-fill", "clip-stroke", "mask-alpha", "mask-inverted-alpha", "mask-luminance", "mask-inverted-luminance"
};
inline constexpr const char* fillRuleNames[] = { "nonzero", "evenodd" };
inline constexpr const char* strokeJoinNames[] = { "miter", "round", "bevel" };
inline constexpr const char* strokeCapNames[] = { "butt", "round", "square" };

template <std::size_t N>
int indexOfName (const char* const (&names)[N], const yup::String& name)
{
    for (std::size_t i = 0; i < N; ++i)
    {
        if (name == names[i])
            return static_cast<int> (i);
    }

    return 0;
}

//==============================================================================

inline float getFloat (const yup::DataTree& node, const yup::Identifier& id)
{
    return static_cast<float> (node.getProperty (id, 0.0));
}

inline bool getBool (const yup::DataTree& node, const yup::Identifier& id)
{
    return static_cast<bool> (node.getProperty (id, false));
}

inline yup::String getString (const yup::DataTree& node, const yup::Identifier& id)
{
    return node.getProperty (id).toString();
}

inline yup::Color getColor (const yup::DataTree& node, const yup::Identifier& id)
{
    return yup::Color::fromString (getString (node, id));
}

inline yup::Point<float> getPoint (const yup::DataTree& node, const yup::Identifier& xId, const yup::Identifier& yId)
{
    return { getFloat (node, xId), getFloat (node, yId) };
}

inline yup::Point<float> getAnchor (const yup::DataTree& vertex) { return getPoint (vertex, Ids::x, Ids::y); }

inline yup::Point<float> getInOffset (const yup::DataTree& vertex) { return getPoint (vertex, Ids::inX, Ids::inY); }

inline yup::Point<float> getOutOffset (const yup::DataTree& vertex) { return getPoint (vertex, Ids::outX, Ids::outY); }

inline yup::DataTree getFill (const yup::DataTree& layer) { return layer.getChildWithName (Ids::fill); }

inline yup::DataTree getStroke (const yup::DataTree& layer) { return layer.getChildWithName (Ids::stroke); }

inline bool isGradientKind (const yup::String& kind) { return kind == "linear" || kind == "radial"; }

/** Gradients and image paints are placed with two on-canvas handles, see getPaintHandleEnd(). */
inline bool hasPaintHandles (const yup::String& kind) { return isGradientKind (kind) || kind == "image"; }

inline bool isPaintVisible (const yup::DataTree& paint) { return paint.isValid() && getString (paint, Ids::kind) != "none"; }

inline yup::BlendMode getBlendMode (const yup::DataTree& layer)
{
    return static_cast<yup::BlendMode> (indexOfName (blendModeNames, getString (layer, Ids::blendMode)));
}

inline yup::StrokeJoin getStrokeJoin (const yup::DataTree& stroke)
{
    return static_cast<yup::StrokeJoin> (indexOfName (strokeJoinNames, getString (stroke, Ids::join)));
}

inline yup::StrokeCap getStrokeCap (const yup::DataTree& stroke)
{
    return static_cast<yup::StrokeCap> (indexOfName (strokeCapNames, getString (stroke, Ids::cap)));
}

inline yup::StrokePosition getStrokePosition (const yup::DataTree& stroke)
{
    return static_cast<yup::StrokePosition> (indexOfName (strokePositionNames, getString (stroke, Ids::position)));
}

/** True for a layer that is not drawn itself but clips or masks the layer below it. */
inline bool isModifierLayer (const yup::DataTree& layer)
{
    return layer.isValid() && getString (layer, Ids::useAs) != "layer";
}

inline bool isMaskLayer (const yup::DataTree& layer)
{
    return getString (layer, Ids::useAs).startsWith ("mask-");
}

inline yup::LayerMaskMode getLayerMaskMode (const yup::DataTree& layer)
{
    constexpr int firstMaskIndex = 3;
    return static_cast<yup::LayerMaskMode> (yup::jmax (0, indexOfName (useAsNames, getString (layer, Ids::useAs)) - firstMaskIndex));
}

/** The visible clip and mask layers stacked directly above a layer, which all act on it. */
inline std::vector<yup::DataTree> getModifierLayers (const yup::DataTree& document, int layerIndex)
{
    std::vector<yup::DataTree> modifiers;

    for (int i = layerIndex + 1; isModifierLayer (document.getChild (i)); ++i)
    {
        if (const auto modifier = document.getChild (i); getBool (modifier, Ids::visible))
            modifiers.push_back (modifier);
    }

    return modifiers;
}

/** The stroke a clip-stroke layer clips to: its own stroke's width, join, cap and position. */
inline yup::StrokeType getClipStrokeType (const yup::DataTree& layer)
{
    const auto stroke = getStroke (layer);
    return yup::StrokeType (getFloat (stroke, Ids::width), getStrokeJoin (stroke), getStrokeCap (stroke)).withPosition (getStrokePosition (stroke));
}

inline yup::ImageSampling getImageSampling (const yup::DataTree& paint)
{
    return { static_cast<yup::ImageWrap> (indexOfName (imageWrapNames, getString (paint, Ids::wrapX))),
             static_cast<yup::ImageWrap> (indexOfName (imageWrapNames, getString (paint, Ids::wrapY))),
             static_cast<yup::ImageFilter> (indexOfName (imageFilterNames, getString (paint, Ids::filter))) };
}

/** The base64 encoded file of an image paint: the one loaded into the document, or the bundled logo. */
inline yup::String getPaintImageData (const yup::DataTree& paint)
{
    if (auto data = getString (paint, Ids::image); data.isNotEmpty())
        return data;

    static const auto logoData = []
    {
        yup::MemoryBlock bytes;
        getAssetPath ("data/logo.png").loadFileAsData (bytes);
        return yup::Base64::toBase64 (bytes.getData(), bytes.getSize());
    }();

    return logoData;
}

struct DecodedImage
{
    yup::String data;
    yup::Image image;
};

inline std::unordered_map<const void*, DecodedImage>& getDecodedPaintImages()
{
    static std::unordered_map<const void*, DecodedImage> decodedImages;
    return decodedImages;
}

/** Frees the decoded images and their GPU textures while the graphics context still exists. */
inline void clearPaintImageCache()
{
    getDecodedPaintImages().clear();
}

/** The decoded image of an image paint, decoded once per distinct file.

    Returned by reference so the image keeps its GPU texture from frame to frame. The cache is
    keyed by the base64 string's buffer, which stays shared while the property is unchanged,
    and holds on to that string so the address is never reused for another file.
*/
inline const yup::Image& getPaintImage (const yup::DataTree& paint)
{
    auto& decodedImages = getDecodedPaintImages();

    const auto data = getPaintImageData (paint);
    const auto* key = data.getCharPointer().getAddress();

    if (auto cached = decodedImages.find (key); cached != decodedImages.end())
        return cached->second.image;

    yup::MemoryBlock bytes;
    yup::MemoryOutputStream output (bytes, false);
    yup::Base64::convertFromBase64 (output, data);
    output.flush();

    auto image = yup::Image::loadFromData ({ static_cast<const yup::uint8*> (bytes.getData()), bytes.getSize() });

    auto& entry = decodedImages[key];
    entry.data = data;
    entry.image = image.wasOk() ? image.getValue() : yup::Image();
    return entry.image;
}

/** Maps the image's pixels to the document: scaled, skewed, rotated, then moved to the x1/y1 handle. */
inline yup::AffineTransform getPaintImageTransform (const yup::DataTree& paint)
{
    const auto origin = getPoint (paint, Ids::x1, Ids::y1);
    const auto skew = yup::AffineTransform (1.0f, std::tan (yup::degreesToRadians (getFloat (paint, Ids::imageSkewX))), 0.0f,
                                            std::tan (yup::degreesToRadians (getFloat (paint, Ids::imageSkewY))), 1.0f, 0.0f);

    return yup::AffineTransform::scaling (getFloat (paint, Ids::imageScale))
        .followedBy (skew)
        .rotated (yup::degreesToRadians (getFloat (paint, Ids::imageRotation)))
        .translated (origin.getX(), origin.getY());
}

/** The end handle of a paint: x2/y2 for gradients, the image's top-right corner for images. */
inline yup::Point<float> getPaintHandleEnd (const yup::DataTree& paint)
{
    if (getString (paint, Ids::kind) != "image")
        return getPoint (paint, Ids::x2, Ids::y2);

    return yup::Point<float> (static_cast<float> (getPaintImage (paint).getWidth()), 0.0f).transformed (getPaintImageTransform (paint));
}

//==============================================================================

/** One cubic bezier segment of a contour, in document coordinates. */
struct Segment
{
    yup::Point<float> p0, c0, c1, p1;

    bool isLine() const { return c0 == p0 && c1 == p1; }

    yup::Point<float> pointAt (float t) const
    {
        const auto a = p0.lerp (c0, t), b = c0.lerp (c1, t), c = c1.lerp (p1, t);
        const auto d = a.lerp (b, t), e = b.lerp (c, t);
        return d.lerp (e, t);
    }
};

inline int getNumSegments (const yup::DataTree& contour)
{
    const int numVertices = contour.getNumChildren();
    if (numVertices < 2)
        return 0;

    return getBool (contour, Ids::closed) ? numVertices : numVertices - 1;
}

inline Segment getSegment (const yup::DataTree& contour, int index)
{
    const auto from = contour.getChild (index);
    const auto to = contour.getChild ((index + 1) % contour.getNumChildren());

    const auto p0 = getAnchor (from);
    const auto p1 = getAnchor (to);
    return { p0, p0 + getOutOffset (from), p1 + getInOffset (to), p1 };
}

inline void addSegmentToPath (yup::Path& path, const Segment& segment)
{
    if (segment.isLine())
        path.lineTo (segment.p1);
    else
        path.cubicTo (segment.c0.getX(), segment.c0.getY(), segment.c1.getX(), segment.c1.getY(), segment.p1.getX(), segment.p1.getY());
}

/** Builds the yup::Path of a layer from its Contour / Vertex children. */
inline yup::Path buildPath (const yup::DataTree& layer)
{
    yup::Path path;

    for (const auto& contour : layer)
    {
        if (contour.getType() != Ids::contour || contour.getNumChildren() == 0)
            continue;

        const int numVertices = contour.getNumChildren();
        path.moveTo (getAnchor (contour.getChild (0)));

        for (int i = 0; i < numVertices - 1; ++i)
            addSegmentToPath (path, getSegment (contour, i));

        if (getBool (contour, Ids::closed) && numVertices > 1)
        {
            const auto closing = getSegment (contour, numVertices - 1);
            if (! closing.isLine())
                addSegmentToPath (path, closing);

            path.close();
        }
    }

    path.setUsingNonZeroWinding (getString (layer, Ids::fillRule) != "evenodd");
    return path;
}

/** Flattens every contour of a layer into polylines, used for hit testing. */
inline std::vector<std::vector<yup::Point<float>>> flattenLayer (const yup::DataTree& layer, int stepsPerCurve = 24)
{
    std::vector<std::vector<yup::Point<float>>> polylines;

    for (const auto& contour : layer)
    {
        if (contour.getType() != Ids::contour || contour.getNumChildren() == 0)
            continue;

        auto& polyline = polylines.emplace_back();
        polyline.push_back (getAnchor (contour.getChild (0)));

        for (int i = 0; i < getNumSegments (contour); ++i)
        {
            const auto segment = getSegment (contour, i);
            const int steps = segment.isLine() ? 1 : stepsPerCurve;

            for (int s = 1; s <= steps; ++s)
                polyline.push_back (segment.pointAt (static_cast<float> (s) / static_cast<float> (steps)));
        }
    }

    return polylines;
}

//==============================================================================

inline bool areHandlesSmooth (yup::Point<float> in, yup::Point<float> out)
{
    const float inLength = in.magnitude();
    const float outLength = out.magnitude();

    if (inLength < 0.01f || outLength < 0.01f)
        return false;

    return std::abs (in.crossProduct (out)) <= 0.01f * inLength * outLength && in.dotProduct (out) < 0.0f;
}

/** Sets several properties on a detached node, without undo (used while building new subtrees). */
inline void setDetachedProperties (yup::DataTree node, std::initializer_list<std::pair<yup::Identifier, yup::var>> properties)
{
    auto transaction = node.beginTransaction();

    for (const auto& [id, value] : properties)
        transaction.setProperty (id, value);
}

/** Converts the segments of a yup::Path into detached Contour nodes. */
inline std::vector<yup::DataTree> contoursFromPath (const yup::DataTreeSchema& schema, const yup::Path& path)
{
    struct Knot
    {
        yup::Point<float> anchor, in, out;
    };

    std::vector<yup::DataTree> contours;
    std::vector<Knot> knots;

    auto flush = [&] (bool closed)
    {
        if (! knots.empty())
        {
            auto contour = schema.createNode (Ids::contour);
            auto transaction = contour.beginTransaction();
            transaction.setProperty (Ids::closed, closed);

            for (const auto& knot : knots)
            {
                auto vertex = schema.createNode (Ids::vertex);
                setDetachedProperties (vertex, { { Ids::x, knot.anchor.getX() }, { Ids::y, knot.anchor.getY() }, { Ids::inX, knot.in.getX() }, { Ids::inY, knot.in.getY() }, { Ids::outX, knot.out.getX() }, { Ids::outY, knot.out.getY() }, { Ids::smooth, areHandlesSmooth (knot.in, knot.out) } });
                transaction.addChild (vertex);
            }

            transaction.commit();
            contours.push_back (contour);
        }

        knots.clear();
    };

    for (const auto& segment : path)
    {
        switch (segment.verb)
        {
            case yup::Path::Verb::MoveTo:
                flush (false);
                knots.push_back ({ segment.point, {}, {} });
                break;

            case yup::Path::Verb::LineTo:
                knots.push_back ({ segment.point, {}, {} });
                break;

            case yup::Path::Verb::QuadTo:
            {
                if (knots.empty())
                    break;

                const auto start = knots.back().anchor;
                knots.back().out = (segment.controlPoint1 - start) * (2.0f / 3.0f);
                knots.push_back ({ segment.point, (segment.controlPoint1 - segment.point) * (2.0f / 3.0f), {} });
                break;
            }

            case yup::Path::Verb::CubicTo:
            {
                if (knots.empty())
                    break;

                knots.back().out = segment.controlPoint1 - knots.back().anchor;
                knots.push_back ({ segment.point, segment.controlPoint2 - segment.point, {} });
                break;
            }

            case yup::Path::Verb::Close:
            {
                if (knots.size() > 1 && knots.back().anchor.distanceTo (knots.front().anchor) < 0.01f)
                {
                    knots.front().in = knots.back().in;
                    knots.pop_back();
                }

                flush (true);
                break;
            }
        }
    }

    flush (false);
    return contours;
}

//==============================================================================

struct GradientStop
{
    float offset = 0.0f;
    yup::Color color;
};

/** Returns the stops of a paint sorted by offset, padded so the first is at 0 and the last at 1.

    yup::ColorGradient takes the gradient axis from the first and last stop positions, so the
    padding keeps the axis on the paint endpoints whatever the user offsets are (SVG pad semantics).
*/
inline std::vector<GradientStop> getPaddedStops (const yup::DataTree& paint)
{
    std::vector<GradientStop> stops;

    for (const auto& child : paint)
        stops.push_back ({ yup::jlimit (0.0f, 1.0f, getFloat (child, Ids::offset)), getColor (child, Ids::color) });

    std::stable_sort (stops.begin(), stops.end(), [] (const auto& a, const auto& b)
    {
        return a.offset < b.offset;
    });

    if (stops.empty())
        stops.push_back ({ 0.0f, getColor (paint, Ids::color) });

    if (stops.front().offset > 0.0f)
        stops.insert (stops.begin(), GradientStop { 0.0f, stops.front().color });

    if (stops.back().offset < 1.0f)
        stops.push_back (GradientStop { 1.0f, stops.back().color });

    return stops;
}

inline yup::ColorGradient createGradient (const yup::DataTree& paint)
{
    const auto p1 = getPoint (paint, Ids::x1, Ids::y1);
    const auto p2 = getPoint (paint, Ids::x2, Ids::y2);
    const auto type = getString (paint, Ids::kind) == "radial" ? yup::ColorGradient::Radial : yup::ColorGradient::Linear;

    std::vector<yup::ColorGradient::ColorStop> colorStops;
    for (const auto& stop : getPaddedStops (paint))
        colorStops.emplace_back (stop.color, p1.lerp (p2, stop.offset), stop.offset);

    return yup::ColorGradient (type, std::move (colorStops));
}

//==============================================================================

/** The editor selection: a layer and optionally one of its vertices (whose parent is the contour). */
struct Selection
{
    yup::DataTree layer;
    yup::DataTree vertex;

    void clear()
    {
        layer = {};
        vertex = {};
    }
};

//==============================================================================

enum class Preset
{
    rectangle = 1,
    roundedRectangle,
    ellipse,
    star,
    polygon
};

inline constexpr const char* presetNames[] = { "Rectangle", "Rounded rect", "Ellipse", "Star", "Polygon" };

/** Creates the path of a preset shape centered in an area. */
inline yup::Path createPresetPath (Preset preset, yup::Rectangle<float> area)
{
    const auto center = area.getCenter();
    const float radius = yup::jmin (area.getWidth(), area.getHeight()) * 0.25f;
    const auto box = yup::Rectangle<float> (0.0f, 0.0f, radius * 2.0f, radius * 2.0f).withCenter (center);

    yup::Path path;
    switch (preset)
    {
        case Preset::rectangle:
            path.addRectangle (box);
            break;
        case Preset::roundedRectangle:
            path.addRoundedRectangle (box, radius * 0.3f);
            break;
        case Preset::ellipse:
            path.addEllipse (box);
            break;
        case Preset::star:
            path.addStar (center, 5, radius * 0.45f, radius);
            break;
        case Preset::polygon:
            path.addPolygon (center, 6, radius);
            break;
    }

    return path;
}

//==============================================================================

/** A path document held in a schema-validated DataTree, with every edit going through an UndoManager.

    All mutations go through edit() (one undo step) or a beginGesture() / apply() / endGesture()
    sequence (one undo step for a whole drag), and use the write primitives setProperty(),
    addChild(), removeChild() and moveChild() inside them.
*/
class PathDocument
{
public:
    PathDocument()
        : schema (yup::DataTreeSchema::fromJsonSchemaString (documentSchemaJson))
    {
        jassert (schema != nullptr && schema->isValid());

        root = schema->createNode (Ids::document);
        createSeedDocument();
        undoManager->clear();
    }

    //==============================================================================
    const yup::DataTree& getRoot() const noexcept { return root; }

    const yup::DataTreeSchema& getSchema() const noexcept { return *schema; }

    yup::Rectangle<float> getArtboardBounds() const
    {
        return { 0.0f, 0.0f, getFloat (root, Ids::width), getFloat (root, Ids::height) };
    }

    std::function<void()> onChanged;

    //==============================================================================
    /** Performs a set of writes as a single named undo step. Never call it from inside another edit or a gesture. */
    template <class F>
    void edit (yup::StringRef transactionName, F&& function)
    {
        jassert (! gestureActive && ! editing);

        {
            const yup::ScopedValueSetter<bool> editScope (editing, true);
            yup::UndoManager::ScopedTransaction undoTransaction (*undoManager, transactionName);
            function();
        }

        notifyChanged();
    }

    /** Starts a gesture: every apply() until endGesture() becomes one undo step. */
    void beginGesture (yup::StringRef transactionName)
    {
        jassert (! gestureActive && ! editing);

        gestureActive = true;
        undoManager->beginNewTransaction (transactionName);
    }

    template <class F>
    void apply (F&& function)
    {
        jassert (gestureActive);

        function();
        notifyChanged();
    }

    void endGesture()
    {
        if (! gestureActive)
            return;

        gestureActive = false;
        undoManager->beginNewTransaction();
        notifyChanged();
    }

    bool isGestureActive() const noexcept { return gestureActive; }

    /** Goes through apply() while a gesture is active, otherwise through edit(). */
    template <class F>
    void change (yup::StringRef transactionName, F&& function)
    {
        if (gestureActive)
            apply (std::forward<F> (function));
        else
            edit (transactionName, std::forward<F> (function));
    }

    //==============================================================================
    void undo()
    {
        if (gestureActive || ! undoManager->undo())
            return;

        notifyChanged();
    }

    void redo()
    {
        if (gestureActive || ! undoManager->redo())
            return;

        notifyChanged();
    }

    bool canUndo() const { return undoManager->canUndo(); }

    bool canRedo() const { return undoManager->canRedo(); }

    //==============================================================================
    /** Sets a property, clamping numbers to the schema range. Skips writes that change nothing. */
    void setProperty (yup::DataTree node, const yup::Identifier& id, yup::var value)
    {
        const auto info = schema->getPropertyInfo (node.getType(), id);
        if (info.type == "number")
        {
            auto number = static_cast<double> (value);
            if (info.minimum.has_value())
                number = yup::jmax (number, *info.minimum);
            if (info.maximum.has_value())
                number = yup::jmin (number, *info.maximum);

            value = number;
        }

        if (node.hasProperty (id) && node.getProperty (id) == value)
            return;

        auto transaction = node.beginValidatedTransaction (schema, undoManager.get());
        checkResult (transaction.setProperty (id, value));
        checkResult (transaction.commit());
    }

    void setPoint (yup::DataTree node, const yup::Identifier& xId, const yup::Identifier& yId, yup::Point<float> point)
    {
        setProperty (node, xId, point.getX());
        setProperty (node, yId, point.getY());
    }

    void addChild (yup::DataTree parent, const yup::DataTree& child, int index = -1)
    {
        auto transaction = parent.beginValidatedTransaction (schema, undoManager.get());
        checkResult (transaction.addChild (child, index));
        checkResult (transaction.commit());
    }

    void removeChild (yup::DataTree parent, const yup::DataTree& child)
    {
        auto transaction = parent.beginValidatedTransaction (schema, undoManager.get());
        checkResult (transaction.removeChild (child));
        checkResult (transaction.commit());
    }

    void moveChild (yup::DataTree parent, int currentIndex, int newIndex)
    {
        auto transaction = parent.beginTransaction (undoManager.get());
        transaction.moveChild (currentIndex, newIndex);
    }

    //==============================================================================
    /** Creates a detached layer (with its Fill and Stroke) holding the contours of a path. */
    yup::DataTree createLayer (const yup::String& layerName, const yup::Path& path, yup::Color fillColor) const
    {
        auto layer = schema->createNode (Ids::layer);
        auto fill = schema->createNode (Ids::fill);
        setDetachedProperties (fill, { { Ids::color, fillColor.toString() } });

        auto transaction = layer.beginTransaction();
        transaction.setProperty (Ids::name, layerName);
        transaction.addChild (fill);
        transaction.addChild (schema->createNode (Ids::stroke));

        for (const auto& contour : contoursFromPath (*schema, path))
            transaction.addChild (contour);

        transaction.commit();
        return layer;
    }

    yup::DataTree createStop (float offset, yup::Color stopColor) const
    {
        auto stopNode = schema->createNode (Ids::stop);
        setDetachedProperties (stopNode, { { Ids::offset, offset }, { Ids::color, stopColor.toString() } });
        return stopNode;
    }

    yup::DataTree createVertex (yup::Point<float> anchor) const
    {
        auto vertexNode = schema->createNode (Ids::vertex);
        setDetachedProperties (vertexNode, { { Ids::x, anchor.getX() }, { Ids::y, anchor.getY() } });
        return vertexNode;
    }

    //==============================================================================
    /** Switches a Fill / Stroke to another kind, seeding stops and endpoints when it becomes a gradient. */
    void setPaintKind (yup::DataTree paint, const yup::String& newKind, yup::Rectangle<float> layerBounds)
    {
        const auto oldKind = getString (paint, Ids::kind);
        setProperty (paint, Ids::kind, newKind);

        if (newKind == oldKind)
            return;

        if (newKind == "image")
        {
            // Start with the image spanning the layer's width
            setPoint (paint, Ids::x1, Ids::y1, layerBounds.getTopLeft());

            if (const auto& image = getPaintImage (paint); image.getWidth() > 0)
                setProperty (paint, Ids::imageScale, yup::jlimit (0.01f, 20.0f, layerBounds.getWidth() / static_cast<float> (image.getWidth())));

            return;
        }

        if (! isGradientKind (newKind))
            return;

        if (paint.getNumChildren() < 2)
        {
            const auto base = getColor (paint, Ids::color);

            while (paint.getNumChildren() > 0)
                removeChild (paint, paint.getChild (0));

            addChild (paint, createStop (0.0f, base));
            addChild (paint, createStop (1.0f, base.contrasting (0.5f)));
        }

        const auto center = layerBounds.getCenter();
        if (newKind == "linear")
        {
            setPoint (paint, Ids::x1, Ids::y1, { layerBounds.getX(), center.getY() });
            setPoint (paint, Ids::x2, Ids::y2, { layerBounds.getRight(), center.getY() });
        }
        else
        {
            setPoint (paint, Ids::x1, Ids::y1, center);
            setPoint (paint, Ids::x2, Ids::y2, center.translated (yup::jmax (layerBounds.getWidth(), layerBounds.getHeight()) * 0.5f, 0.0f));
        }
    }

    /** Moves the end handle of a paint: x2/y2 for gradients, scale and rotation for images, keeping their skew. */
    void setPaintHandleEnd (yup::DataTree paint, yup::Point<float> end)
    {
        if (getString (paint, Ids::kind) != "image")
        {
            setPoint (paint, Ids::x2, Ids::y2, end);
            return;
        }

        const auto& image = getPaintImage (paint);
        const auto edge = end - getPoint (paint, Ids::x1, Ids::y1);
        const float length = std::hypot (edge.getX(), edge.getY());
        if (image.getWidth() <= 0 || length <= 0.0f)
            return;

        // The image's top edge is scaled, then sheared by the vertical skew, then rotated
        const float skewY = std::tan (yup::degreesToRadians (getFloat (paint, Ids::imageSkewY)));
        const float scale = length / (static_cast<float> (image.getWidth()) * std::sqrt (1.0f + skewY * skewY));
        const float rotation = yup::radiansToDegrees (std::atan2 (edge.getY(), edge.getX()) - std::atan (skewY));

        setProperty (paint, Ids::imageScale, yup::jlimit (0.01f, 20.0f, scale));
        setProperty (paint, Ids::imageRotation, std::remainder (rotation, 360.0f));
    }

    /** Translates every vertex and gradient endpoint of a layer. */
    void translateLayer (yup::DataTree layer, yup::Point<float> delta)
    {
        for (auto paint : { getFill (layer), getStroke (layer) })
        {
            setPoint (paint, Ids::x1, Ids::y1, getPoint (paint, Ids::x1, Ids::y1) + delta);
            setPoint (paint, Ids::x2, Ids::y2, getPoint (paint, Ids::x2, Ids::y2) + delta);
        }

        for (const auto& contour : layer)
        {
            if (contour.getType() != Ids::contour)
                continue;

            for (const auto& vertex : contour)
                setPoint (vertex, Ids::x, Ids::y, getAnchor (vertex) + delta);
        }
    }

    /** Splits a contour segment at t (de Casteljau), returning the inserted vertex. */
    yup::DataTree splitSegment (yup::DataTree contour, int segmentIndex, float t)
    {
        const int numVertices = contour.getNumChildren();
        auto from = contour.getChild (segmentIndex);
        auto to = contour.getChild ((segmentIndex + 1) % numVertices);
        const auto segment = getSegment (contour, segmentIndex);

        auto newVertex = createVertex (segment.pointAt (t));

        if (! segment.isLine())
        {
            const auto a = segment.p0.lerp (segment.c0, t), b = segment.c0.lerp (segment.c1, t), c = segment.c1.lerp (segment.p1, t);
            const auto d = a.lerp (b, t), e = b.lerp (c, t);
            const auto mid = d.lerp (e, t);

            setDetachedProperties (newVertex, { { Ids::inX, d.getX() - mid.getX() }, { Ids::inY, d.getY() - mid.getY() }, { Ids::outX, e.getX() - mid.getX() }, { Ids::outY, e.getY() - mid.getY() }, { Ids::smooth, true } });

            setPoint (from, Ids::outX, Ids::outY, a - segment.p0);
            setPoint (to, Ids::inX, Ids::inY, c - segment.p1);
        }

        addChild (contour, newVertex, segmentIndex + 1);
        return newVertex;
    }

    /** Toggles a vertex between corner (no handles) and smooth (tangent handles from its neighbors). */
    void toggleSmooth (yup::DataTree vertex)
    {
        if (getBool (vertex, Ids::smooth))
        {
            setProperty (vertex, Ids::smooth, false);
            setPoint (vertex, Ids::inX, Ids::inY, {});
            setPoint (vertex, Ids::outX, Ids::outY, {});
            return;
        }

        auto contour = vertex.getParent();
        const int numVertices = contour.getNumChildren();
        const int index = contour.indexOf (vertex);
        const bool closed = getBool (contour, Ids::closed);

        const auto anchor = getAnchor (vertex);
        const auto prev = (index > 0 || closed) ? getAnchor (contour.getChild ((index + numVertices - 1) % numVertices)) : anchor;
        const auto next = (index < numVertices - 1 || closed) ? getAnchor (contour.getChild ((index + 1) % numVertices)) : anchor;

        auto tangent = next - prev;
        const float length = tangent.magnitude();
        if (length < 0.01f)
            tangent = { 1.0f, 0.0f };
        else
            tangent = tangent / length;

        setProperty (vertex, Ids::smooth, true);
        setPoint (vertex, Ids::inX, Ids::inY, tangent * -yup::jmax (8.0f, anchor.distanceTo (prev) / 3.0f));
        setPoint (vertex, Ids::outX, Ids::outY, tangent * yup::jmax (8.0f, anchor.distanceTo (next) / 3.0f));
    }

    //==============================================================================
    /** Adds a detached layer on top of the others as one undo step. */
    yup::DataTree addLayer (const yup::DataTree& layer)
    {
        edit ("Add layer", [&]
        {
            addChild (root, layer);
        });

        return layer;
    }

    /** Inserts a copy of a layer right above it as one undo step. */
    yup::DataTree duplicateLayer (const yup::DataTree& layer)
    {
        auto copy = layer.clone();
        setDetachedProperties (copy, { { Ids::name, getString (layer, Ids::name) + " copy" } });

        edit ("Duplicate layer", [&]
        {
            addChild (root, copy, root.indexOf (layer) + 1);
        });

        return copy;
    }

    void deleteLayer (const yup::DataTree& layer)
    {
        edit ("Delete layer", [&]
        {
            removeChild (root, layer);
        });
    }

    /** Moves a layer up (positive delta) or down (negative delta) in the stacking order. */
    void moveLayer (const yup::DataTree& layer, int delta)
    {
        const int index = root.indexOf (layer);
        const int newIndex = yup::jlimit (0, root.getNumChildren() - 1, index + delta);
        if (index < 0 || newIndex == index)
            return;

        edit ("Reorder layers", [&]
        {
            moveChild (root, index, newIndex);
        });
    }

    /** Removes a vertex, and its contour when it was the last one. */
    void deleteVertex (const yup::DataTree& vertex)
    {
        auto contour = vertex.getParent();

        edit ("Delete vertex", [&]
        {
            if (contour.getNumChildren() > 1)
                removeChild (contour, vertex);
            else
                removeChild (contour.getParent(), contour);
        });
    }

    /** Splits the segment starting at a vertex (or ending at it, for the last vertex of an open contour). */
    yup::DataTree insertVertexAfter (const yup::DataTree& vertex)
    {
        auto contour = vertex.getParent();
        const int numSegments = getNumSegments (contour);
        if (numSegments == 0)
            return {};

        yup::DataTree newVertex;
        edit ("Insert vertex", [&]
        {
            newVertex = splitSegment (contour, yup::jmin (contour.indexOf (vertex), numSegments - 1), 0.5f);
        });

        return newVertex;
    }

private:
    static void checkResult ([[maybe_unused]] const yup::Result& result)
    {
        jassert (result.wasOk());
    }

    void notifyChanged()
    {
        if (onChanged)
            onChanged();
    }

    void addSeedLayer (const yup::DataTree& layer)
    {
        auto transaction = root.beginTransaction();
        transaction.addChild (layer);
    }

    void createSeedDocument()
    {
        const auto artboard = getArtboardBounds();

        {
            auto backdrop = createLayer ("Backdrop", yup::Path().addRoundedRectangle (artboard.reduced (24.0f), 28.0f), yup::Color (0xff2b3a67));
            auto fill = getFill (backdrop);
            setDetachedProperties (fill, { { Ids::kind, "linear" }, { Ids::x1, 24.0f }, { Ids::y1, 24.0f }, { Ids::x2, artboard.getRight() - 24.0f }, { Ids::y2, artboard.getBottom() - 24.0f } });

            auto transaction = fill.beginTransaction();
            transaction.addChild (createStop (0.0f, yup::Color (0xff2b3a67)));
            transaction.addChild (createStop (0.55f, yup::Color (0xff3d2c6b)));
            transaction.addChild (createStop (1.0f, yup::Color (0xff0f1424)));
            transaction.commit();

            addSeedLayer (backdrop);
        }

        {
            const yup::Point<float> center { 230.0f, 245.0f };
            auto star = createLayer ("Star", yup::Path().addStar (center, 5, 62.0f, 140.0f), yup::Color (0xffffc94d));
            auto fill = getFill (star);
            setDetachedProperties (fill, { { Ids::kind, "radial" }, { Ids::x1, center.getX() }, { Ids::y1, center.getY() }, { Ids::x2, center.getX() + 140.0f }, { Ids::y2, center.getY() } });

            auto transaction = fill.beginTransaction();
            transaction.addChild (createStop (0.0f, yup::Color (0xfffff3b0)));
            transaction.addChild (createStop (0.6f, yup::Color (0xffffb627)));
            transaction.addChild (createStop (1.0f, yup::Color (0xffff6b35)));
            transaction.commit();

            setDetachedProperties (getStroke (star), { { Ids::kind, "solid" }, { Ids::color, "#fff8e7ff" }, { Ids::width, 5.0f } });
            addSeedLayer (star);
        }

        {
            yup::Path heartPath;
            heartPath.fromString ("M12,21.35l-1.45-1.32C5.4,15.36,2,12.28,2,8.5 C2,5.42,4.42,3,7.5,3c1.74,0,3.41,0.81,4.5,2.09C13.09,3.81,14.76,3,16.5,3 C19.58,3,22,5.42,22,8.5c0,3.78-3.4,6.86-8.55,11.54L12,21.35z");
            heartPath.transform (yup::AffineTransform::scaling (9.0f).translated (330.0f, 110.0f));

            auto heart = createLayer ("Heart", heartPath, yup::Color (0xffff4d6d));
            setDetachedProperties (heart, { { Ids::feather, 18.0f }, { Ids::blendMode, "screen" } });
            addSeedLayer (heart);
        }
    }

    yup::DataTreeSchema::Ptr schema;
    yup::DataTree root;
    yup::UndoManager::Ptr undoManager { new yup::UndoManager (yup::RelativeTime()) };
    bool gestureActive = false;
    bool editing = false;
};

} // namespace PathEditor
