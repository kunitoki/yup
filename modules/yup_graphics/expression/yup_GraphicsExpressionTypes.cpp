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

uint8 expressionColorByte (int value) noexcept
{
    return static_cast<uint8> (jlimit (0, 255, value));
}

uint8 expressionColorByte (float value) noexcept
{
    return static_cast<uint8> (roundToInt (jlimit (0.0f, 1.0f, value) * 255.0f));
}

template <class R, class G, class B, class A>
Color makeExpressionColor (R r, G g, B b, A a) noexcept
{
    return Color (expressionColorByte (a), expressionColorByte (r), expressionColorByte (g), expressionColorByte (b));
}

// A number of an array, zero where the array has none
float readExpressionNumber (const ExpressionArrayView<float>& numbers, int index) noexcept
{
    return index < numbers.size ? numbers.data[index] : 0.0f;
}

} // namespace

//==============================================================================
StrokeDashPattern StrokeDashPattern::fromString (StringRef text)
{
    const auto trimmed = String (text).trim();
    if (trimmed.isEmpty() || trimmed.equalsIgnoreCase ("none"))
        return {};

    auto tokens = StringArray::fromTokens (trimmed, " ,\t\r\n", "");
    tokens.removeEmptyStrings (true);

    Array<float> values;
    values.ensureStorageAllocated (tokens.size());

    for (const auto& token : tokens)
    {
        const float value = token.getFloatValue();
        if (value > 0.0f)
            values.add (value);
    }

    return StrokeDashPattern (std::move (values));
}

//==============================================================================
void addGraphicsExpressionTypes (ExpressionContext& context)
{
    using P = Point<float>;
    using S = Size<float>;
    using R = Rectangle<float>;
    using L = Line<float>;
    using T = AffineTransform;
    using G = ColorGradient;
    using D = StrokeDashPattern;
    using PP = PatternPaint;

    // All types are registered before any function, as signatures reference each other
    auto point = context.addType<P> ("point");
    auto size = context.addType<S> ("size");
    auto rect = context.addType<R> ("rect");
    auto line = context.addType<L> ("line");
    auto color = context.addType<Color> ("color");
    auto transform = context.addType<T> ("transform");
    auto path = context.addType<Path> ("path");
    auto dash = context.addType<D> ("dash");
    auto font = context.addType<Font> ("font");
    auto gradient = context.addType<G> ("gradient");
    auto pattern = context.addType<PP> ("pattern");

    context.addEnum<StrokeCap> ("cap", { { "butt", StrokeCap::Butt }, { "round", StrokeCap::Round }, { "square", StrokeCap::Square } });
    context.addEnum<StrokeJoin> ("join", { { "miter", StrokeJoin::Miter }, { "round", StrokeJoin::Round }, { "bevel", StrokeJoin::Bevel } });

    context.addEnum<StrokePosition> ("strokePosition", {
                                                           { "inside", StrokePosition::Inside },
                                                           { "center", StrokePosition::Center },
                                                           { "outside", StrokePosition::Outside },
                                                       });

    context.addEnum<BlendMode> ("blend", {
                                             { "srcOver", BlendMode::SrcOver },
                                             { "screen", BlendMode::Screen },
                                             { "overlay", BlendMode::Overlay },
                                             { "darken", BlendMode::Darken },
                                             { "lighten", BlendMode::Lighten },
                                             { "colorDodge", BlendMode::ColorDodge },
                                             { "colorBurn", BlendMode::ColorBurn },
                                             { "hardLight", BlendMode::HardLight },
                                             { "softLight", BlendMode::SoftLight },
                                             { "difference", BlendMode::Difference },
                                             { "exclusion", BlendMode::Exclusion },
                                             { "multiply", BlendMode::Multiply },
                                             { "hue", BlendMode::Hue },
                                             { "saturation", BlendMode::Saturation },
                                             { "color", BlendMode::Color },
                                             { "luminosity", BlendMode::Luminosity },
                                             { "additive", BlendMode::Additive },
                                         });

    context.addEnum<Justification::Flags> ("justification", {
                                                                { "left", Justification::left },
                                                                { "right", Justification::right },
                                                                { "horizontalCenter", Justification::horizontalCenter },
                                                                { "top", Justification::top },
                                                                { "bottom", Justification::bottom },
                                                                { "verticalCenter", Justification::verticalCenter },
                                                                { "topLeft", Justification::topLeft },
                                                                { "topRight", Justification::topRight },
                                                                { "bottomLeft", Justification::bottomLeft },
                                                                { "bottomRight", Justification::bottomRight },
                                                                { "centerLeft", Justification::centerLeft },
                                                                { "centerTop", Justification::centerTop },
                                                                { "center", Justification::center },
                                                                { "centerRight", Justification::centerRight },
                                                                { "centerBottom", Justification::centerBottom },
                                                            });

    context.addEnum<Fitting> ("fitting", {
                                             { "none", Fitting::none },
                                             { "scaleToFit", Fitting::scaleToFit },
                                             { "fitWidth", Fitting::fitWidth },
                                             { "fitHeight", Fitting::fitHeight },
                                             { "scaleToFill", Fitting::scaleToFill },
                                             { "fill", Fitting::fill },
                                             { "tile", Fitting::tile },
                                             { "centerCrop", Fitting::centerCrop },
                                             { "centerInside", Fitting::centerInside },
                                             { "stretchWidth", Fitting::stretchWidth },
                                             { "stretchHeight", Fitting::stretchHeight },
                                         });

    context.addEnum<G::Spread> ("spread", {
                                              { "pad", G::Spread::Pad },
                                              { "reflect", G::Spread::Reflect },
                                              { "repeat", G::Spread::Repeat },
                                          });

    context.addEnum<AnchoredText::Anchor> ("textAnchor", {
                                                             { "start", AnchoredText::Anchor::start },
                                                             { "middle", AnchoredText::Anchor::middle },
                                                             { "end", AnchoredText::Anchor::end },
                                                         });

    context.addEnum<LayerMaskMode> ("maskMode", {
                                                    { "alpha", LayerMaskMode::Alpha },
                                                    { "invertedAlpha", LayerMaskMode::InvertedAlpha },
                                                    { "luminance", LayerMaskMode::Luminance },
                                                    { "invertedLuminance", LayerMaskMode::InvertedLuminance },
                                                });

    point
        .constructor ([] (float x, float y) { return P (x, y); })
        .member ("x", [] (const P& p) { return p.getX(); })
        .member ("y", [] (const P& p) { return p.getY(); })
        .member ("length", [] (const P& p) { return p.magnitude(); })
        .method ("distanceTo", [] (const P& p, const P& o) { return p.distanceTo (o); })
        .method ("angleTo", [] (const P& p, const P& o) { return p.angleTo (o); })
        .method ("dot", [] (const P& p, const P& o) { return p.dotProduct (o); })
        .method ("normalized", [] (const P& p) { return p.normalized(); })
        .method ("translated", [] (const P& p, float dx, float dy) { return p.translated (dx, dy); })
        .method ("translated", [] (const P& p, const P& d) { return p.translated (d); })
        .method ("scaled", [] (const P& p, float f) { return p * f; })
        .method ("scaled", [] (const P& p, float fx, float fy) { return P (p.getX() * fx, p.getY() * fy); })
        .method ("lerp", [] (const P& p, const P& o, float t) { return p.lerp (o, t); })
        .method ("transformed", [] (const P& p, const T& t) { return p.transformed (t); })
        .method ("onCircle", [] (const P& p, float radius, float angle) -> P { return p.getPointOnCircumference (radius, angle); })
        .operation (ExpressionOperator::add, [] (const P& a, const P& b) { return a + b; })
        .operation (ExpressionOperator::subtract, [] (const P& a, const P& b) { return a - b; })
        .operation (ExpressionOperator::multiply, [] (const P& a, const P& b) { return a * b; })
        .operation (ExpressionOperator::multiply, [] (const P& a, float f) { return a * f; })
        .operation (ExpressionOperator::multiply, [] (float f, const P& a) { return a * f; })
        .operation (ExpressionOperator::divide, [] (const P& a, const P& b) { return a / b; })
        .operation (ExpressionOperator::divide, [] (const P& a, float f) { return a / f; })
        .operation (ExpressionOperator::negate, [] (const P& a) { return -a; })
        .equality()
        .components ({ "x", "y" });

    size
        .constructor ([] (float w, float h) { return S (w, h); })
        .member ("width", [] (const S& s) { return s.getWidth(); })
        .member ("height", [] (const S& s) { return s.getHeight(); })
        .operation (ExpressionOperator::multiply, [] (const S& s, float f) { return s * f; })
        .operation (ExpressionOperator::divide, [] (const S& s, float f) { return s / f; })
        .equality();

    rect
        .constructor ([] (float x, float y, float w, float h) { return R (x, y, w, h); })
        .constructor ([] (const P& position, const S& s) { return R (position, s); })
        .member ("x", [] (const R& r) { return r.getX(); })
        .member ("y", [] (const R& r) { return r.getY(); })
        .member ("width", [] (const R& r) { return r.getWidth(); })
        .member ("height", [] (const R& r) { return r.getHeight(); })
        .member ("left", [] (const R& r) { return r.getLeft(); })
        .member ("top", [] (const R& r) { return r.getTop(); })
        .member ("right", [] (const R& r) { return r.getRight(); })
        .member ("bottom", [] (const R& r) { return r.getBottom(); })
        .member ("centerX", [] (const R& r) { return r.getCenterX(); })
        .member ("centerY", [] (const R& r) { return r.getCenterY(); })
        .member ("center", [] (const R& r) { return r.getCenter(); })
        .member ("position", [] (const R& r) { return r.getPosition(); })
        .member ("size", [] (const R& r) { return r.getSize(); })
        .member ("topLeft", [] (const R& r) { return r.getTopLeft(); })
        .member ("topRight", [] (const R& r) { return r.getTopRight(); })
        .member ("bottomLeft", [] (const R& r) { return r.getBottomLeft(); })
        .member ("bottomRight", [] (const R& r) { return r.getBottomRight(); })
        .method ("withX", [] (const R& r, float v) { return r.withX (v); })
        .method ("withY", [] (const R& r, float v) { return r.withY (v); })
        .method ("withWidth", [] (const R& r, float v) { return r.withWidth (v); })
        .method ("withHeight", [] (const R& r, float v) { return r.withHeight (v); })
        .method ("withPosition", [] (const R& r, const P& p) { return r.withPosition (p); })
        .method ("withSize", [] (const R& r, const S& s) { return r.withSize (s); })
        .method ("withCenter", [] (R r, const P& p) { return r.withCenter (p); })
        .method ("withSizeKeepingCenter", [] (const R& r, float w, float h) { return r.withSizeKeepingCenter (w, h); })
        .method ("translated", [] (const R& r, float dx, float dy) { return r.translated (dx, dy); })
        .method ("translated", [] (const R& r, const P& d) { return r.translated (d); })
        .method ("scaled", [] (const R& r, float f) { return r.scaled (f); })
        .method ("scaled", [] (const R& r, float fx, float fy) { return r.scaled (fx, fy); })
        .method ("reduced", [] (const R& r, float d) { return r.reduced (d); })
        .method ("reduced", [] (const R& r, float dx, float dy) { return r.reduced (dx, dy); })
        .method ("reduced", [] (const R& r, float left, float top, float right, float bottom) { return r.reduced (left, top, right, bottom); })
        .method ("enlarged", [] (const R& r, float d) { return r.enlarged (d); })
        .method ("enlarged", [] (const R& r, float dx, float dy) { return r.enlarged (dx, dy); })
        .method ("contains", [] (const R& r, const P& p) { return r.contains (p); })
        .method ("contains", [] (const R& r, const R& o) { return r.contains (o); })
        .method ("intersects", [] (const R& r, const R& o) { return r.intersects (o); })
        .method ("intersection", [] (const R& r, const R& o) { return r.intersection (o); })
        .method ("unionWith", [] (const R& r, const R& o) { return r.unionWith (o); })
        .method ("largestFittingSquare", [] (const R& r) { return r.largestFittingSquare(); })
        .method ("transformed", [] (const R& r, const T& t) { return r.transformed (t); })
        .equality();

    line
        .constructor ([] (float x1, float y1, float x2, float y2) { return L (x1, y1, x2, y2); })
        .constructor ([] (const P& start, const P& end) { return L (start, end); })
        .member ("start", [] (const L& l) { return l.getStart(); })
        .member ("end", [] (const L& l) { return l.getEnd(); })
        .member ("length", [] (const L& l) { return l.length(); })
        .method ("pointAt", [] (const L& l, float t) { return l.getStart().lerp (l.getEnd(), t); })
        .method ("translated", [] (const L& l, float dx, float dy) { return l.translated (dx, dy); })
        .method ("transformed", [] (const L& l, const T& t) { return l.transformed (t); })
        .method ("reversed", [] (const L& l) { return l.reversed(); })
        .equality();

    color
        .constructor ([] (int argb) { return Color (static_cast<uint32> (argb)); })
        .constructor ("rgb", [] (int r, int g, int b) { return makeExpressionColor (r, g, b, 255); })
        .constructor ("rgb", [] (float r, float g, float b) { return makeExpressionColor (r, g, b, 1.0f); })
        .constructor ("rgba", [] (int r, int g, int b, int a) { return makeExpressionColor (r, g, b, a); })
        .constructor ("rgba", [] (int r, int g, int b, float a) { return makeExpressionColor (r, g, b, a); })
        .constructor ("rgba", [] (float r, float g, float b, float a) { return makeExpressionColor (r, g, b, a); })
        .constructor ("hsl", [] (float h, float s, float l) { return Color::fromHSL (h, s, l); })
        .constructor ("hsla", [] (float h, float s, float l, float a) { return Color::fromHSL (h, s, l, a); })
        .member ("r", [] (const Color& c) { return c.getRedFloat(); })
        .member ("g", [] (const Color& c) { return c.getGreenFloat(); })
        .member ("b", [] (const Color& c) { return c.getBlueFloat(); })
        .member ("a", [] (const Color& c) { return c.getAlphaFloat(); })
        .member ("hue", [] (const Color& c) { return c.getHue(); })
        .member ("saturation", [] (const Color& c) { return c.getSaturation(); })
        .member ("luminance", [] (const Color& c) { return c.getLuminance(); })
        .method ("withAlpha", [] (const Color& c, float a) { return c.withAlpha (jlimit (0.0f, 1.0f, a)); })
        .method ("withMultipliedAlpha", [] (const Color& c, float a) { return c.withMultipliedAlpha (jlimit (0.0f, 1.0f, a)); })
        .method ("brighter", [] (const Color& c, float amount) { return c.brighter (amount); })
        .method ("darker", [] (const Color& c, float amount) { return c.darker (amount); })
        .method ("contrasting", [] (const Color& c) { return c.contrasting(); })
        .method ("inverted", [] (const Color& c) { return c.inverted(); })
        .method ("mix", [] (const Color& c, const Color& o, float amount) { return c.mixedWith (o, jlimit (0.0f, 1.0f, amount), ColorSpace::RGB); })
        .equality();

    for (const auto& named : Colors::getNamedColors())
        context.addConstant ("colors." + String (named.name), named.color);

    transform
        .constructor ([] { return T(); })
        .constructor ([] (float a, float b, float c, float d, float tx, float ty) { return T (a, c, tx, b, d, ty); })
        .constructor ("transform.translation", [] (float x, float y) { return T::translation (x, y); })
        .constructor ("transform.rotation", [] (float radians) { return T::rotation (radians); })
        .constructor ("transform.rotation", [] (float radians, float cx, float cy) { return T::rotation (radians, cx, cy); })
        .constructor ("transform.scaling", [] (float factor) { return T::scaling (factor); })
        .constructor ("transform.scaling", [] (float fx, float fy) { return T::scaling (fx, fy); })
        .constructor ("transform.shearing", [] (float fx, float fy) { return T::shearing (fx, fy); })
        .method ("translated", [] (const T& t, float x, float y) { return t.translated (x, y); })
        .method ("rotated", [] (const T& t, float radians) { return t.rotated (radians); })
        .method ("rotated", [] (const T& t, float radians, float cx, float cy) { return t.rotated (radians, cx, cy); })
        .method ("scaled", [] (const T& t, float factor) { return t.scaled (factor); })
        .method ("scaled", [] (const T& t, float fx, float fy) { return t.scaled (fx, fy); })
        .method ("followedBy", [] (const T& t, const T& o) { return t.followedBy (o); })
        .method ("inverted", [] (const T& t) { return t.inverted(); })
        .equality();

    // Paths keep value semantics on shared locals, while single-use builder chains fuse to mutate one Path in place
    path
        .clone ([] (const Path& p) { return p.createCopy(); })
        .constructor ([] { return Path(); })
        .constructor ([] (const Path& p) { return p; })
        .constructor ([] (const String& svgData)
        {
            Path result;
            result.fromString (svgData);
            return result;
        })
        .member ("bounds", [] (const Path& p) { return p.getBounds(); })
        .member ("length", [] (const Path& p) { return p.getLength(); })
        .member ("isEmpty", [] (const Path& p) { return p.isEmpty(); })
        .method ("pointAt", [] (const Path& p, float normalizedPosition) { return p.getPointAlongPath (normalizedPosition); })
        .method ("moveTo", [] (Path& p, float x, float y) -> Path& { return p.moveTo (x, y); })
        .method ("moveTo", [] (Path& p, const P& target) -> Path& { return p.moveTo (target); })
        .method ("lineTo", [] (Path& p, float x, float y) -> Path& { return p.lineTo (x, y); })
        .method ("lineTo", [] (Path& p, const P& target) -> Path& { return p.lineTo (target); })
        .method ("quadTo", [] (Path& p, float x, float y, float controlX, float controlY) -> Path& { return p.quadTo (x, y, controlX, controlY); })
        .method ("cubicTo", [] (Path& p, float x1, float y1, float x2, float y2, float x, float y) -> Path& { return p.cubicTo (x1, y1, x2, y2, x, y); })
        .method ("close", [] (Path& p) -> Path& { return p.close(); })
        .method ("addRectangle", [] (Path& p, const R& r) -> Path& { return p.addRectangle (r); })
        .method ("addRoundedRectangle", [] (Path& p, const R& r, float radius) -> Path& { return p.addRoundedRectangle (r, radius); })
        .method ("addEllipse", [] (Path& p, const R& r) -> Path& { return p.addEllipse (r); })
        .method ("addLine", [] (Path& p, const L& l) -> Path& { return p.addLine (l); })
        .method ("addArc", [] (Path& p, const R& r, float fromRadians, float toRadians, bool startAsNewSubPath) -> Path& { return p.addArc (r, fromRadians, toRadians, startAsNewSubPath); })
        .method ("addTriangle", [] (Path& p, const P& a, const P& b, const P& c) -> Path&
        {
            p.addTriangle (a, b, c);
            return p;
        })
        .method ("addPolygon", [] (Path& p, const P& center, int numSides, float radius) -> Path& { return p.addPolygon (center, numSides, radius); })
        .method ("addPolygon", [] (Path& p, const P& center, int numSides, float radius, float startAngle) -> Path& { return p.addPolygon (center, numSides, radius, startAngle); })
        .method ("addStar", [] (Path& p, const P& center, int numPoints, float innerRadius, float outerRadius) -> Path& { return p.addStar (center, numPoints, innerRadius, outerRadius); })
        .method ("addStar", [] (Path& p, const P& center, int numPoints, float innerRadius, float outerRadius, float startAngle) -> Path& { return p.addStar (center, numPoints, innerRadius, outerRadius, startAngle); })
        .method ("appendPath", [] (Path& p, const Path& other) -> Path& { return p.appendPath (other); })
        .method ("appendPath", [] (Path& p, const Path& other, const T& t) -> Path& { return p.appendPath (other, t); })
        .method ("transformed", [] (Path& p, const T& t) -> Path& { return p.transform (t); })
        .method ("scaledToFit", [] (Path& p, const R& r, bool preserveProportions) -> Path&
        {
            p.scaleToFit (r.getX(), r.getY(), r.getWidth(), r.getHeight(), preserveProportions);
            return p;
        })
        .method ("withRoundedCorners", [] (const Path& p, float radius) { return p.withRoundedCorners (radius); })
        .method ("withNonZeroWinding", [] (Path& p, bool nonZero) -> Path&
        {
            p.setUsingNonZeroWinding (nonZero);
            return p;
        })
        .method ("createDashed", [] (const Path& p, const D& d) { return p.createDashed (d.asSpan()); })
        .method ("createDashed", [] (const Path& p, const D& d, float offset) { return p.createDashed (d.asSpan(), offset); });

    context.addArrayType<float>();

    dash
        .constructor ([] { return D(); })
        .constructor ([] (float a) { return D (Array<float> { a }); })
        .constructor ([] (float a, float b) { return D (Array<float> { a, b }); })
        .constructor ([] (float a, float b, float c, float d) { return D (Array<float> { a, b, c, d }); })
        .constructor ([] (float a, float b, float c, float d, float e, float f) { return D (Array<float> { a, b, c, d, e, f }); })
        .constructor ([] (const String& text) { return D::fromString (text); })
        .constructor ([] (ExpressionArrayView<float> values) -> D
        {
            return (values.data != nullptr && values.size > 0) ? D (Array<float> (values.data, values.size)) : D();
        })
        .member ("isEmpty", [] (const D& d) { return d.isEmpty(); })
        .equality();

    context.addFunction ("dash", [] (const D& d) { return d; });

    // An array of numbers is the point, the size or the rect it is given to, with zeros for the numbers it
    // lacks. One written in place gives its numbers to their constructors instead. A dash takes an array
    context.addFunction ("@convert", [] (ExpressionArrayView<float> numbers)
    {
        return P (readExpressionNumber (numbers, 0), readExpressionNumber (numbers, 1));
    });

    context.addFunction ("@convert", [] (ExpressionArrayView<float> numbers)
    {
        return S (readExpressionNumber (numbers, 0), readExpressionNumber (numbers, 1));
    });

    context.addFunction ("@convert", [] (ExpressionArrayView<float> numbers)
    {
        return R (readExpressionNumber (numbers, 0), readExpressionNumber (numbers, 1), readExpressionNumber (numbers, 2), readExpressionNumber (numbers, 3));
    });

    font
        .constructor ([] { return Font(); })
        .member ("height", [] (const Font& f) { return f.getHeight(); })
        .member ("isEmpty", [] (const Font& f) { return f.isEmpty(); })
        .method ("withHeight", [] (const Font& f, float height) { return f.withHeight (height); })
        .method ("withAxisValue", [] (const Font& f, const String& tag, float value) { return f.withAxisValue (StringRef (tag), value); })
        .equality();

    gradient
        .constructor ("linearGradient", [] (const Color& c1, const P& p1, const Color& c2, const P& p2) { return G (c1, p1, c2, p2, G::Linear); })
        .constructor ("radialGradient", [] (const Color& c1, const P& center, const Color& c2, const P& edge) { return G (c1, center, c2, edge, G::Radial); })
        .constructor ("radialGradient", [] (const Color& c1, const P& center, const Color& c2, float radius)
        {
            return G (c1, center, c2, center.translated (radius, 0.0f), G::Radial);
        })
        .constructor ("angularGradient", [] (const Color& c1, const P& center, const Color& c2, const P& direction)
        {
            return G (c1, center, c2, direction, G::Angular);
        })
        .method ("withStop", [] (G& gr, const Color& c, float delta) -> G&
        {
            gr.addColorStop (c, delta);
            return gr;
        })
        .method ("withAlpha", [] (const G& gr, float a) { return gr.withAlpha (jlimit (0.0f, 1.0f, a)); })
        .method ("withMultipliedAlpha", [] (const G& gr, float a) { return gr.withMultipliedAlpha (jlimit (0.0f, 1.0f, a)); })
        .method ("withTransform", [] (const G& gr, const T& t) { return gr.withTransform (t); })
        .method ("withSpread", [] (const G& gr, G::Spread spread) { return gr.withSpread (spread); })
        .method ("mappedTo", [] (const G& gr, const R& area)
        {
            return gr.withTransform (gr.getTransform().followedBy (T::scaling (area.getWidth(), area.getHeight()).translated (area.getX(), area.getY())));
        })
        .equality();

    pattern
        .constructor ([] { return PP(); })
        .member ("isEmpty", [] (const PP& p) { return p.source == nullptr; })
        .equality();

    context.addFunction ("polar", [] (const P& center, float radius, float angle) { return center.getPointOnCircumference (radius, angle); });
    context.addFunction ("mix", [] (const P& a, const P& b, float t) { return a.lerp (b, t); });
    context.addFunction ("mix", [] (const Color& a, const Color& b, float t) { return a.mixedWith (b, jlimit (0.0f, 1.0f, t), ColorSpace::RGB); });
}

} // namespace yup
