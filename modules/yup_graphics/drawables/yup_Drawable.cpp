/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

Drawable::Drawable()
{
    // transform = AffineTransform::scaling (1.0f).translated (100.0f, 100.0f);
}

//==============================================================================

bool Drawable::parseSVG (const File& svgFile)
{
    XmlDocument svgDoc (svgFile);
    std::unique_ptr<XmlElement> svgRoot (svgDoc.getDocumentElement());

    if (svgRoot == nullptr || ! svgRoot->hasTagName ("svg"))
        return false;

    if (auto view = svgRoot->getStringAttribute ("viewBox"); view.isNotEmpty())
    {
        auto coords = StringArray::fromTokens (view, " ,", "");
        if (coords.size() == 4)
        {
            viewBox.setX (coords.getReference (0).getFloatValue());
            viewBox.setY (coords.getReference (1).getFloatValue());
            viewBox.setWidth (coords.getReference (2).getFloatValue());
            viewBox.setHeight (coords.getReference (3).getFloatValue());
        }
    }

    auto width = svgRoot->getDoubleAttribute ("width");
    size.setWidth (width == 0.0 ? viewBox.getWidth() : width);

    auto height = svgRoot->getDoubleAttribute ("height");
    size.setWidth (height == 0.0 ? viewBox.getHeight() : height);

    //AffineTransform currentTransform;
    //if (! viewBox.getTopLeft().isOrigin())
    //    currentTransform = currentTransform.translated (-viewBox.getX(), -viewBox.getY());

    return parseElement (*svgRoot, true, {});
}

//==============================================================================

void Drawable::clear()
{
    viewBox = { 0.0f, 0.0f, 0.0f, 0.0f };
    size = { 0.0f, 0.0f };

    elements.clear();
    elementsById.clear();
}

//==============================================================================

void Drawable::paint (Graphics& g)
{
    g.setTransform (transform);
    g.setStrokeWidth (1.0f);
    g.setFillColor (Colors::black);

    for (const auto& element : elements)
        paintElement (g, *element, true, false);
}

//==============================================================================

void Drawable::paintElement (Graphics& g, const Element& element, bool hasParentFillEnabled, bool hasParentStrokeEnabled)
{
    const auto savedState = g.saveState();

    bool isFillDefined = hasParentFillEnabled;
    bool isStrokeDefined = hasParentStrokeEnabled;

    if (element.transform)
        g.setTransform (element.transform->followedBy (g.getTransform()));

    if (element.opacity)
        g.setOpacity (g.getOpacity() * (*element.opacity));

    // Setup fill
    if (element.fillColor)
    {
        g.setFillColor (*element.fillColor);
        isFillDefined = true;
    }

    if (isFillDefined && ! element.noFill)
    {
        if (element.path)
        {
            g.fillPath (*element.path);
        }
        else if (element.reference)
        {
            if (auto refElement = elementsById[*element.reference]; refElement != nullptr && refElement->path)
                g.fillPath (*refElement->path);
        }
    }

    // Setup stroke
    if (element.strokeColor)
    {
        g.setStrokeColor (*element.strokeColor);
        isStrokeDefined = true;
    }

    if (element.strokeJoin)
        g.setStrokeJoin (*element.strokeJoin);

    if (element.strokeCap)
        g.setStrokeCap (*element.strokeCap);

    if (element.strokeWidth)
        g.setStrokeWidth (*element.strokeWidth);

    if (isStrokeDefined && ! element.noStroke)
    {
        if (element.path)
        {
            g.strokePath (*element.path);
        }
        else if (element.reference)
        {
            if (auto refElement = elementsById[*element.reference]; refElement != nullptr && refElement->path)
                g.strokePath (*refElement->path);
        }
    }

    for (const auto& childElement : element.children)
        paintElement (g, *childElement, isFillDefined, isStrokeDefined);

    // paintDebugElement (g, element);
}

//==============================================================================

bool Drawable::parseElement (const XmlElement& element, bool parentIsRoot, AffineTransform currentTransform, Element* parent)
{
    auto e = std::make_shared<Element>();
    bool isRootElement = element.hasTagName ("svg");

    if (auto id = element.getStringAttribute ("id"); id.isNotEmpty())
    {
        e->id = id;
        elementsById.set (id, e);
    }

    if (element.hasTagName ("path"))
    {
        auto path = Path();

        String pathData = element.getStringAttribute ("d");
        if (pathData.isEmpty() || ! path.fromString (pathData))
            return false;

        e->path = std::move (path);

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("g"))
    {
        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("use"))
    {
        String href = element.getStringAttribute ("href");
        if (href.isNotEmpty() && href.startsWith ("#"))
            e->reference = href.substring (1);

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("ellipse"))
    {
        auto cx = element.getDoubleAttribute ("cx");
        auto cy = element.getDoubleAttribute ("cy");
        auto rx = element.getDoubleAttribute ("rx");
        auto ry = element.getDoubleAttribute ("ry");

        auto path = Path();
        path.addCenteredEllipse (cx, cy, rx, ry);
        e->path = std::move (path);

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }
    else if (element.hasTagName ("circle"))
    {
        auto cx = element.getDoubleAttribute ("cx");
        auto cy = element.getDoubleAttribute ("cy");
        auto r = element.getDoubleAttribute ("r");

        auto path = Path();
        path.addCenteredEllipse (cx, cy, r, r);
        e->path = std::move (path);

        currentTransform = parseTransform (element, currentTransform, *e);
        parseStyle (element, currentTransform, *e);
    }

    for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
        parseElement (*child, isRootElement, currentTransform, e.get());

    if (isRootElement)
        return true;

    if (parent != nullptr && ! parentIsRoot)
        parent->children.push_back (std::move (e));
    else
        elements.push_back (std::move (e));

    return true;
}

//==============================================================================

void Drawable::parseStyle (const XmlElement& element, const AffineTransform& currentTransform, Element& e)
{
    String fill = element.getStringAttribute ("fill");
    if (fill.isNotEmpty())
    {
        if (fill != "none")
            e.fillColor = Color::fromString (fill);
        else
            e.noFill = true;
    }

    String stroke = element.getStringAttribute ("stroke");
    if (stroke.isNotEmpty())
    {
        if (stroke != "none")
            e.strokeColor = Color::fromString (stroke);
        else
            e.noStroke = true;
    }

    String strokeJoin = element.getStringAttribute ("stroke-linejoin");
    if (strokeJoin == "round")
        e.strokeJoin = StrokeJoin::Round;
    else if (strokeJoin == "miter")
        e.strokeJoin = StrokeJoin::Miter;
    else if (strokeJoin == "bevel")
        e.strokeJoin = StrokeJoin::Bevel;

    String strokeCap = element.getStringAttribute ("stroke-linecap");
    if (strokeCap == "round")
        e.strokeCap = StrokeCap::Round;
    else if (strokeCap == "square")
        e.strokeCap = StrokeCap::Square;
    else if (strokeCap == "butt")
        e.strokeCap = StrokeCap::Butt;

    float strokeWidth = element.getDoubleAttribute ("stroke-width", -1.0);
    if (strokeWidth > 0.0)
    {
        auto transformScale = std::sqrtf (std::fabsf (currentTransform.followedBy (transform).getDeterminant()));
        e.strokeWidth = transformScale * strokeWidth;
    }

    float opacity = element.getDoubleAttribute ("opacity", -1.0);
    if (opacity >= 0.0 && opacity <= 1.0)
        e.opacity = opacity;
}

//==============================================================================

AffineTransform Drawable::parseTransform (const XmlElement& element, const AffineTransform& currentTransform, Element& e)
{
    AffineTransform result;

    String transformString = element.getStringAttribute ("transform");
    if (transformString.isNotEmpty())
    {
        auto data = transformString.getCharPointer();
        while (! data.isEmpty())
        {
            // Skip whitespace
            while (data.isWhitespace())
                ++data;

            if (data.isEmpty())
                break;

            // Parse transform type
            String type;
            while (! data.isEmpty() && CharacterFunctions::isLetter (*data))
            {
                type += *data;
                ++data;
            }

            // Skip whitespace and the opening parenthesis
            while (data.isWhitespace() || *data == '(')
                ++data;

            // Parse parameters
            Array<float> params;
            while (! data.isEmpty() && *data != ')')
            {
                if (*data == ',' || *data == ' ')
                {
                    ++data;
                    continue;
                }

                String number;
                while (! data.isEmpty() && (*data == '-' || *data == '.' || (*data >= '0' && *data <= '9')))
                {
                    number += *data;
                    ++data;
                }

                if (! number.isEmpty())
                    params.add (number.getFloatValue());

                // Skip whitespace or commas
                while (data.isWhitespace() || *data == ',')
                    ++data;
            }

            // Skip the closing parenthesis
            if (*data == ')')
                ++data;

            // Apply the parsed transform
            if (type == "translate" && (params.size() == 1 || params.size() == 2))
            {
                result = result.translated (params[0], (params.size() == 2) ? params[1] : 0.0f);
            }
            else if (type == "scale" && (params.size() == 1 || params.size() == 2))
            {
                result = result.scaled (params[0], (params.size() == 2) ? params[1] : params[0]);
            }
            else if (type == "rotate" && (params.size() == 1 || params.size() == 3))
            {
                if (params.size() == 1)
                    result = result.rotated (degreesToRadians (params[0]));
                else
                    result = result.rotated (degreesToRadians (params[0]), params[1], params[2]);
            }
            else if (type == "skewX" && params.size() == 1)
            {
                result = result.sheared (std::tanf (degreesToRadians (params[0])), 0.0f);
            }
            else if (type == "skewY" && params.size() == 1)
            {
                result = result.sheared (0.0f, std::tanf (degreesToRadians (params[0])));
            }
            else if (type == "matrix" && params.size() == 6)
            {
                result = result.followedBy (AffineTransform (
                    params[0], params[1], params[2], params[3], params[4], params[5]));
            }
        }

        e.transform = result;
    }

    return currentTransform.followedBy (result);
}

//==============================================================================

void Drawable::paintDebugElement (Graphics& g, const Element& element)
{
    if (! element.path)
        return;

    for (const auto& segment : *element.path)
    {
        auto color = Color::opaqueRandom();

        g.setFillColor (color);
        g.fillRect (segment.point.getX() - 4, segment.point.getY() - 4, 8, 8);

        g.setStrokeColor (Colors::white);
        g.setStrokeWidth (2.0f);
        g.strokeRect (segment.point.getX() - 4, segment.point.getY() - 4, 8, 8);

        if (segment.verb == PathVerb::CubicTo)
        {
            g.setFillColor (color.brighter (0.05f));
            g.fillRect (segment.controlPoint1.getX() - 4, segment.controlPoint1.getY() - 4, 8, 8);

            g.setFillColor (color.brighter (0.1f));
            g.fillRect (segment.controlPoint2.getX() - 4, segment.controlPoint2.getY() - 4, 8, 8);
        }
    }
}

} // namespace yup
