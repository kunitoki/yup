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
/* Writes an SVG as the elements of a drawing, following what Drawable::paintElement does with it.

   Drawable paints with the state of its Graphics: an element sets a paint or a stroke width,
   and what is inside it, or what it instances, is painted with what is left there. A
   drawing inherits paint lexically instead, so the converter keeps both: the State is what the
   Graphics holds while an element is painted, the Cascade is what the groups already written
   around it set. An element only gets the properties its cascade doesn't give it.
*/
class YuiSvgImporter::Converter
{
public:
    Converter (const SVGData& svgData, const Options& importOptions, StringArray& importWarnings)
        : data (svgData)
        , options (importOptions)
        , warnings (importWarnings)
    {
        usedLets = { "bounds", "objectBounds", "currentColor", "defaultFont", "palette", "path", "clipPath",
                     "gradient", "linearGradient", "radialGradient", "angularGradient" };
    }

    DataTree convert()
    {
        Item drawing ("drawing");
        drawing.set ("name", sanitizeName (options.name, "Imported"));
        drawing.set ("viewBox", rectText (data.viewBox.isEmpty() ? data.bounds : data.viewBox));

        if (data.rootHasPreserveAspectRatio)
            writeFitting (drawing, data.rootPreserveAspectRatioFitting, data.rootPreserveAspectRatioJustification);

        Scope scope;
        scope.state.fillEnabled = data.rootHasFill;
        scope.state.fill = makeColor ("fill", data.rootFillColor.value_or (Colors::black));
        scope.state.strokeEnabled = data.rootHasStroke;
        scope.state.stroke = makeColor ("stroke", data.rootStrokeColor.value_or (Colors::black));
        scope.state.currentColor = colorText (data.rootFillColor.value_or (Colors::black));

        // What every shape is painted with unless it says otherwise goes to a group around them all
        Item root ("group");
        write (root, scope.cascade, "fill", scope.state.fillEnabled ? scope.state.fill : makeNone ("fill", true));

        if (scope.state.strokeEnabled)
            write (root, scope.cascade, "stroke", scope.state.stroke);

        for (const auto& element : data.elements)
            if (element != nullptr)
                convertElement (*element, scope, root.children);

        drawing.children = std::move (definitions);

        if (root.hasProperties() && ! root.children.empty())
            drawing.children.push_back (std::move (root));
        else
            std::move (root.children.begin(), root.children.end(), std::back_inserter (drawing.children));

        if (options.colorProperties)
            declareColorProperties (drawing);

        if (usesDefaultFont)
            warn ("Text is written with 'defaultFont': set Options::fontExpression to choose its fonts");

        return drawing.toTree();
    }

private:
    //==============================================================================
    static constexpr int maxDepth = 64;
    static constexpr int maxUseDepth = 30;

    /* An element of the drawing being written. */
    struct Item
    {
        explicit Item (String itemType)
            : type (std::move (itemType))
        {
        }

        bool has (const String& name) const
        {
            return std::any_of (properties.begin(), properties.end(), [&name] (const auto& property)
            {
                return property.first == name;
            });
        }

        bool hasSlot (const String& slot) const
        {
            return std::any_of (properties.begin(), properties.end(), [&slot] (const auto& property)
            {
                return getSlot (property.first) == slot;
            });
        }

        void set (const String& name, const String& value)
        {
            for (auto& property : properties)
            {
                if (property.first == name)
                {
                    property.second = value;
                    return;
                }
            }

            properties.emplace_back (name, value);
        }

        bool hasProperties() const noexcept
        {
            return ! properties.empty() || transform.has_value() || opacity.has_value();
        }

        /* The properties in the order they read best in: what the element is, where it is, then how it is painted. */
        std::vector<std::pair<String, String>> getWrittenProperties() const
        {
            static const StringArray order { "name", "drawing", "id", "in", "in2", "viewBox", "fitting", "justification", "tile", "size", "ref", "orient", "units",
                                             "bounds", "cornerSize", "center", "radius", "from", "to", "data", "anchor", "text", "source",
                                             "transform", "opacity", "blend", "color",
                                             "fill", "fillGradient", "fillPattern", "stroke", "strokeGradient", "strokeWidth", "strokeMiterLimit",
                                             "cap", "join", "dash", "dashOffset", "markerStart", "markerMid", "markerEnd",
                                             "font", "textAnchor", "letterSpacing" };

            auto written = properties;

            if (transform && ! transform->isIdentity())
                written.emplace_back ("transform", transformText (*transform));

            if (opacity)
                written.emplace_back ("opacity", numberText (*opacity));

            const auto getRank = [] (const String& name)
            {
                const auto index = order.indexOf (name);
                return index >= 0 ? index : order.size();
            };

            std::stable_sort (written.begin(), written.end(), [&getRank] (const auto& a, const auto& b)
            {
                return getRank (a.first) < getRank (b.first);
            });

            return written;
        }

        /* Appends what the element is made of, apart from its name, to tell the definitions that are the same. */
        void appendKey (String& key) const
        {
            key << type << "{";

            for (const auto& [name, value] : getWrittenProperties())
                if (name != "name")
                    key << name << "=" << value << ";";

            for (const auto& child : children)
                child.appendKey (key);

            key << "}";
        }

        DataTree toTree() const
        {
            DataTree tree { Identifier (type) };

            {
                auto transaction = tree.beginTransaction();

                for (const auto& [name, value] : getWrittenProperties())
                    transaction.setProperty (Identifier (name), value);

                for (const auto& child : children)
                    transaction.addChild (child.toTree());
            }

            return tree;
        }

        String type;
        std::vector<std::pair<String, String>> properties;
        std::optional<AffineTransform> transform;
        std::optional<float> opacity;
        std::vector<Item> children;
    };

    /* The value of a property an element can get from the groups around it. */
    struct Value
    {
        String getKey() const
        {
            return property + "=" + (key.isNotEmpty() ? key : text);
        }

        // The property setting it: a fill is set by `fill`, `fillGradient` or `fillPattern`
        String property;
        String text;

        // What it is compared by, when the same text can stand for different values
        String key;

        // False when it is what the Graphics holds, set from around the element
        bool isExplicit = true;
    };

    /* What the Graphics of Drawable holds, and what Drawable::paintElement is called with. */
    struct State
    {
        bool fillEnabled = true;
        bool strokeEnabled = false;
        Value fill, stroke;
        String currentColor;
        float strokeWidth = 1.0f;
        std::optional<StrokeJoin> join;
        std::optional<StrokeCap> cap;
        BlendMode blend = BlendMode::SrcOver;
        std::optional<Array<float>> dash;
        float dashOffset = 0.0f;
        int depth = 0;
        int useDepth = 0;
    };

    /* What the groups around an element set, by slot: the paints of a family share one. */
    struct Cascade
    {
        std::map<String, String> values;

        // Set inside a drawing painted by a `use`: what it doesn't set comes from where it is used
        std::set<String>* reads = nullptr;
    };

    struct Scope
    {
        State state;
        Cascade cascade;
    };

    /* How a path is stroked. */
    struct Stroke
    {
        float width = 1.0f;
        std::optional<StrokeCap> cap;
        std::optional<StrokeJoin> join;
        float miterLimit = 4.0f;
        std::optional<Array<float>> dash;
        float dashOffset = 0.0f;
        bool widthIsExplicit = false, capIsExplicit = false, joinIsExplicit = false, dashIsExplicit = false, dashOffsetIsExplicit = false;
    };

    //==============================================================================
    /* Returns the shortest text giving back the same number. */
    static String numberText (float value)
    {
        if (! std::isfinite (value) || value == 0.0f)
            return "0";

        if (value == std::floor (value) && std::abs (value) < 1.0e7f)
            return String (static_cast<int> (value));

        char buffer[64] = {};

        for (int precision = 1; precision <= 9; ++precision)
        {
            std::snprintf (buffer, sizeof (buffer), "%.*g", precision, static_cast<double> (value));

            if (std::strtof (buffer, nullptr) == value)
                break;
        }

        return String (buffer).replaceCharacter (',', '.');
    }

    static String pointText (Point<float> point)
    {
        return "point (" + numberText (point.getX()) + ", " + numberText (point.getY()) + ")";
    }

    static String sizeText (Size<float> size)
    {
        return "size (" + numberText (size.getWidth()) + ", " + numberText (size.getHeight()) + ")";
    }

    static String rectText (const Rectangle<float>& rect)
    {
        return "rect (" + numberText (rect.getX()) + ", " + numberText (rect.getY()) + ", "
             + numberText (rect.getWidth()) + ", " + numberText (rect.getHeight()) + ")";
    }

    static String colorText (Color color)
    {
        const auto hex = [] (uint8 component)
        {
            return String::toHexString (static_cast<int> (component)).paddedLeft ('0', 2);
        };

        const auto text = "#" + hex (color.getRed()) + hex (color.getGreen()) + hex (color.getBlue());
        return color.getAlpha() == 255 ? text : text + hex (color.getAlpha());
    }

    static String transformText (const AffineTransform& transform)
    {
        const auto scaleX = transform.getScaleX(), scaleY = transform.getScaleY();
        const auto shearX = transform.getShearX(), shearY = transform.getShearY();
        const auto x = transform.getTranslateX(), y = transform.getTranslateY();
        const bool isSheared = shearX != 0.0f || shearY != 0.0f;

        if (! isSheared && scaleX == 1.0f && scaleY == 1.0f)
            return "transform.translation (" + numberText (x) + ", " + numberText (y) + ")";

        if (! isSheared && x == 0.0f && y == 0.0f)
            return "transform.scaling (" + numberText (scaleX) + (scaleX == scaleY ? String() : ", " + numberText (scaleY)) + ")";

        // The values of an SVG matrix
        return "transform (" + numberText (scaleX) + ", " + numberText (shearY) + ", " + numberText (shearX) + ", "
             + numberText (scaleY) + ", " + numberText (x) + ", " + numberText (y) + ")";
    }

    static String quoted (const String& text)
    {
        return "\"" + text.replace ("\\", "\\\\").replace ("\"", "\\\"").replace ("\n", "\\n").replace ("\r", "\\r").replace ("\t", "\\t") + "\"";
    }

    /* A text short enough for a message: the content of a `data:` image goes on for pages. */
    static String shortened (const String& text)
    {
        constexpr int maxLength = 96;

        return text.length() <= maxLength ? text : text.substring (0, maxLength) + "...";
    }

    static String pathText (const Path& path)
    {
        String text;

        const auto add = [&text] (const char* verb, std::initializer_list<Point<float>> points)
        {
            text << verb;

            for (const auto& point : points)
                text << " " << numberText (point.getX()) << " " << numberText (point.getY());

            text << " ";
        };

        for (const auto& segment : path)
        {
            switch (segment.verb)
            {
                case Path::Verb::MoveTo:
                    add ("M", { segment.point });
                    break;

                case Path::Verb::LineTo:
                    add ("L", { segment.point });
                    break;

                case Path::Verb::QuadTo:
                    add ("Q", { segment.controlPoint1, segment.point });
                    break;

                case Path::Verb::CubicTo:
                    add ("C", { segment.controlPoint1, segment.controlPoint2, segment.point });
                    break;

                case Path::Verb::Close:
                    text << "Z ";
                    break;
            }
        }

        return text.trimEnd();
    }

    static String blendText (BlendMode blend)
    {
        static const char* const names[] = { "srcOver", "screen", "overlay", "darken", "lighten", "colorDodge", "colorBurn", "hardLight", "softLight",
                                             "difference", "exclusion", "multiply", "hue", "saturation", "color", "luminosity", "additive" };

        const auto index = static_cast<size_t> (blend);
        return index < std::size (names) ? names[index] : names[0];
    }

    static String capText (StrokeCap cap)
    {
        return cap == StrokeCap::Round ? "round" : (cap == StrokeCap::Square ? "square" : "butt");
    }

    static String joinText (StrokeJoin join)
    {
        return join == StrokeJoin::Round ? "round" : (join == StrokeJoin::Bevel ? "bevel" : "miter");
    }

    static void writeFitting (Item& item, Fitting fitting, Justification justification)
    {
        static const std::pair<Fitting, const char*> fittings[] = {
            { Fitting::none, "none" },
            { Fitting::fitWidth, "fitWidth" },
            { Fitting::fitHeight, "fitHeight" },
            { Fitting::scaleToFill, "scaleToFill" },
            { Fitting::fill, "fill" },
            { Fitting::tile, "tile" },
            { Fitting::centerCrop, "centerCrop" },
            { Fitting::centerInside, "centerInside" },
            { Fitting::stretchWidth, "stretchWidth" },
            { Fitting::stretchHeight, "stretchHeight" }
        };

        // The alignment is named the way Rectangle::getTransformToFit reads its flags
        static const char* const justifications[3][3] = {
            { "topLeft", "centerTop", "topRight" },
            { "centerLeft", "center", "centerRight" },
            { "bottomLeft", "centerBottom", "bottomRight" }
        };

        const auto column = justification.testFlags (Justification::horizontalCenter) ? 1 : (justification.testFlags (Justification::right) ? 2 : 0);
        const auto row = justification.testFlags (Justification::verticalCenter) ? 1 : (justification.testFlags (Justification::bottom) ? 2 : 0);

        for (const auto& [value, name] : fittings)
            if (fitting == value)
                item.set ("fitting", name);

        if (row != 1 || column != 1)
            item.set ("justification", justifications[row][column]);
    }

    /* Returns a name made of letters, digits and '_' starting with a letter, which is what a drawing accepts. */
    static String sanitizeName (const String& text, const String& fallback)
    {
        static const String letters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

        String name;

        for (const auto character : text)
            name << (letters.containsChar (character) || String ("0123456789").containsChar (character) ? String::charToString (character) : String ("_"));

        // `let` starts a statement wherever a name is read as an expression
        return name.isNotEmpty() && letters.containsChar (name[0]) && name != "let" ? name : fallback + name;
    }

    static String makeUnique (std::set<String>& used, const String& name)
    {
        auto unique = name;

        for (int suffix = 2; used.count (unique) != 0; ++suffix)
            unique = name + String (suffix);

        used.insert (unique);
        return unique;
    }

    void warn (const String& message)
    {
        warnings.addIfNotAlreadyThere (message);
    }

    //==============================================================================
    /* The paints setting a fill share a slot of the cascade, like the ones setting a stroke: one replaces the other. */
    static String getSlot (const String& property)
    {
        if (property.startsWith ("fill"))
            return "fill";

        if (property == "stroke" || property == "strokeGradient" || property == "strokePattern")
            return "stroke";

        return property;
    }

    /* Returns true if an element has to set a value itself, as what is around it doesn't. */
    static bool needsWriting (const Cascade& cascade, const String& slot, const Value& value)
    {
        // What a shape is painted with when nothing sets it. A fill isn't there: a shape without
        // any paint is filled with the color of the Graphics, so no fill has to be written as well
        static const std::map<String, String> defaults {
            { "stroke", "stroke=none" },
            { "dash", "dash=dash ()" },
            { "dashOffset", "dashOffset=0" },
            { "strokeMiterLimit", "strokeMiterLimit=4" },
            { "textAnchor", "textAnchor=start" },
            { "letterSpacing", "letterSpacing=0" }
        };

        // A drawing can't be given these from where it is used, so they are always what a drawing starts with
        static const StringArray neverInherited { "strokeMiterLimit", "textAnchor", "letterSpacing" };

        const auto key = value.getKey();

        if (const auto found = cascade.values.find (slot); found != cascade.values.end())
            return found->second != key;

        if (cascade.reads == nullptr || neverInherited.contains (slot))
        {
            const auto fallback = defaults.find (slot);
            return fallback == defaults.end() || fallback->second != key;
        }

        if (value.isExplicit)
            return true;

        cascade.reads->insert (slot);
        return false;
    }

    static void write (Item& item, Cascade& cascade, const String& slot, const Value& value)
    {
        if (! needsWriting (cascade, slot, value))
            return;

        item.set (value.property, value.text);
        cascade.values[slot] = value.getKey();
    }

    /* Writes a paint, along with the `color` its `currentColor` stands for. */
    static void writePaint (Item& item, Cascade& cascade, const String& slot, const Value& value, const Value& color)
    {
        if (! needsWriting (cascade, slot, value))
            return;

        if (value.text.startsWith ("currentColor"))
            write (item, cascade, "color", color);

        write (item, cascade, slot, value);
    }

    static Value makeNone (const String& family, bool isExplicit)
    {
        return { family, "none", {}, isExplicit };
    }

    static Value makeColor (const String& family, Color color)
    {
        return { family, colorText (color), {}, true };
    }

    static Value makeCurrentColor (const String& family, const std::optional<float>& opacity, const String& currentColor)
    {
        const auto text = opacity ? "currentColor.withMultipliedAlpha (" + numberText (*opacity) + ")" : String ("currentColor");
        return { family, text, text + "@" + currentColor, true };
    }

    static Value makeDash (const std::optional<Array<float>>& dash, bool isExplicit)
    {
        if (! dash || dash->isEmpty())
            return { "dash", "dash ()", {}, isExplicit };

        StringArray lengths;
        for (const auto length : *dash)
            lengths.add (numberText (length));

        // A dash is made from one, two, four or six lengths, or from a text of any number of them
        const auto size = lengths.size();
        const bool hasConstructor = size == 1 || size == 2 || size == 4 || size == 6;

        return { "dash", hasConstructor ? "dash (" + lengths.joinIntoString (", ") + ")" : quoted (lengths.joinIntoString (" ")), {}, isExplicit };
    }

    /* Returns what the Graphics holds for a slot, which is what a drawing painted from here has to be given. */
    static Value getStateValue (const String& slot, const State& state)
    {
        Value value;

        if (slot == "fill")
            value = state.fillEnabled ? state.fill : makeNone ("fill", false);
        else if (slot == "stroke")
            value = state.strokeEnabled ? state.stroke : makeNone ("stroke", false);
        else if (slot == "strokeWidth")
            value = { "strokeWidth", numberText (state.strokeWidth) };
        else if (slot == "cap" && state.cap)
            value = { "cap", capText (*state.cap) };
        else if (slot == "join" && state.join)
            value = { "join", joinText (*state.join) };
        else if (slot == "dash")
            value = makeDash (state.dash, false);
        else if (slot == "dashOffset")
            value = { "dashOffset", numberText (state.dashOffset) };
        else if (slot == "color")
            value = { "color", state.currentColor };

        value.isExplicit = false;
        return value;
    }

    //==============================================================================
    String addLet (const String& baseName, const String& suffix, const String& value)
    {
        if (const auto found = letNames.find (value); found != letNames.end())
            return found->second;

        auto name = sanitizeName (baseName, suffix.toLowerCase());
        if (! name.containsIgnoreCase (suffix))
            name << suffix;

        name = makeUnique (usedLets, name);

        Item let ("let");
        let.set ("name", name);
        let.set ("value", value);
        definitions.push_back (std::move (let));

        letNames[value] = name;
        return name;
    }

    /* Adds a drawing, a pattern or a marker, unless one made of the same elements is there already, and returns its name. */
    String addDefinition (const String& kind, const String& baseName, Item item)
    {
        auto key = kind + ":";
        item.appendKey (key);

        if (const auto found = definitionNames.find (key); found != definitionNames.end())
            return found->second;

        // Names that would read as something else where a drawing, a pattern or a marker is referred to
        auto& used = usedNames[kind];
        used.insert ({ "none", "auto", "let", "true", "false" });

        const auto name = makeUnique (used, sanitizeName (baseName, kind));
        item.set ("name", name);
        definitions.push_back (std::move (item));

        definitionNames[key] = name;
        return name;
    }

    //==============================================================================
    /* Declares every color the elements are written with as a property of the drawing, read by name where the color was. */
    void declareColorProperties (Item& drawing)
    {
        std::map<String, String> names;
        std::vector<Item> declarations;

        const auto getName = [&] (const String& color)
        {
            if (const auto found = names.find (color); found != names.end())
                return found->second;

            const auto name = makeUnique (usedLets, "color" + String (static_cast<int> (names.size()) + 1));

            Item property ("property");
            property.set ("name", name);
            property.set ("type", "color");
            property.set ("value", color);
            declarations.push_back (std::move (property));

            names[color] = name;
            return name;
        };

        const std::function<void (Item&)> visit = [&] (Item& item)
        {
            for (auto& property : item.properties)
                if (property.second.containsChar ('#'))
                    property.second = replaceColors (property.second, getName);

            for (auto& child : item.children)
                visit (child);
        };

        visit (drawing);

        drawing.children.insert (drawing.children.begin(), std::make_move_iterator (declarations.begin()), std::make_move_iterator (declarations.end()));
    }

    /* Replaces the colors of an expression, but not what its strings hold, with the names given for them. */
    static String replaceColors (const String& expression, const std::function<String (const String&)>& getName)
    {
        String text;
        bool isQuoted = false;

        for (auto reader = expression.getCharPointer(); ! reader.isEmpty();)
        {
            const auto character = reader.getAndAdvance();

            if (isQuoted && character == '\\' && ! reader.isEmpty())
            {
                text << String::charToString (character) << String::charToString (reader.getAndAdvance());
                continue;
            }

            if (character == '"')
                isQuoted = ! isQuoted;

            auto end = reader;
            int numDigits = 0;

            if (character == '#' && ! isQuoted)
                for (; CharacterFunctions::getHexDigitValue (*end) >= 0; ++end)
                    ++numDigits;

            if (numDigits != 6 && numDigits != 8)
            {
                text << String::charToString (character);
                continue;
            }

            text << getName ("#" + String (reader, end));
            reader = end;
        }

        return text;
    }

    //==============================================================================
    static String gradientText (const ColorGradient& gradient)
    {
        const auto stops = gradient.getStops();
        const auto numStops = stops.size();

        // The gradient is made from its first stop and the first of the ones at its end: the stops added
        // later go after the ones sharing their position, which keeps them all in order
        size_t end = numStops - 1;
        while (end > 1 && stops[end - 1].delta >= stops[numStops - 1].delta)
            --end;

        String text;
        text << (gradient.getType() == ColorGradient::Radial ? "radialGradient (" : "linearGradient (")
             << colorText (stops[0].color) << ", " << pointText ({ stops[0].x, stops[0].y }) << ", "
             << colorText (stops[end].color) << ", " << pointText ({ stops[numStops - 1].x, stops[numStops - 1].y }) << ")";

        for (size_t i = 1; i < numStops; ++i)
            if (i != end)
                text << ".withStop (" << colorText (stops[i].color) << ", " << numberText (stops[i].delta) << ")";

        if (gradient.getSpread() == ColorGradient::Spread::Reflect)
            text << ".withSpread (reflect)";
        else if (gradient.getSpread() == ColorGradient::Spread::Repeat)
            text << ".withSpread (repeat)";

        if (! gradient.getTransform().isIdentity())
            text << ".withTransform (" << transformText (gradient.getTransform()) << ")";

        return text;
    }

    /* A gradient along one axis of the bounding box of its shape is the same mapped to each shape as Drawable places it. */
    static bool followsObjectBounds (const SVGGradient& gradient)
    {
        return gradient.type == SVGGradient::Linear
            && gradient.units == SVGGradient::ObjectBoundingBox
            && gradient.transform.isIdentity()
            && ! gradient.stops.empty()
            && (gradient.start.getX() == gradient.end.getX() || gradient.start.getY() == gradient.end.getY());
    }

    static bool hasArea (const std::optional<Rectangle<float>>& bounds)
    {
        return bounds && bounds->getWidth() > 0.0f && bounds->getHeight() > 0.0f;
    }

    /* Returns a gradient as Drawable leaves it on the Graphics: placed in the bounding box of the element setting it. */
    Value makeGradient (const String& family, const SVGGradient& gradient, const std::optional<Rectangle<float>>& objectBounds)
    {
        const auto property = family + "Gradient";
        const auto colorGradient = gradient.toColorGradient (objectBounds ? std::addressof (*objectBounds) : nullptr);

        if (colorGradient.getNumStops() < 2)
        {
            if (colorGradient.getNumStops() == 1)
                return makeColor (family, colorGradient.getStop (0).color);

            warn ("The gradient '" + gradient.id + "' has no stops: it is written as black");
            return makeColor (family, Colors::black);
        }

        // Placed in the bounding box of a shape, the gradient only fits that shape
        if (hasArea (objectBounds) && gradient.units == SVGGradient::ObjectBoundingBox)
            return { property, gradientText (colorGradient), {}, true };

        return { property, addLet (gradient.id, "Gradient", gradientText (colorGradient)), {}, true };
    }

    /* Returns a gradient as the shape setting it is painted with: one following its bounding box is mapped to it. */
    Value makeShapeGradient (const String& family, const SVGGradient& gradient, const std::optional<Rectangle<float>>& objectBounds)
    {
        if (hasArea (objectBounds) && followsObjectBounds (gradient))
            return { family + "Gradient", addLet (gradient.id, "Gradient", gradientText (gradient.toColorGradient())) + ".mappedTo (objectBounds)", {}, true };

        return makeGradient (family, gradient, objectBounds);
    }

    /* Returns the gradient a group sets for the shapes inside it, when they can share one. */
    std::optional<Value> makeGroupGradient (const String& family, const SVGGradient& gradient)
    {
        const Rectangle<float> unitBounds (0.0f, 0.0f, 1.0f, 1.0f);

        if (followsObjectBounds (gradient))
            return makeShapeGradient (family, gradient, unitBounds);

        if (gradient.units == SVGGradient::UserSpaceOnUse)
            return makeGradient (family, gradient, unitBounds);

        return std::nullopt;
    }

    //==============================================================================
    /* The bounding box Drawable places the gradients and the clips of an element in. */
    std::optional<Rectangle<float>> getObjectBounds (const SVGElement& element) const
    {
        if (element.path)
            return element.path->getBounds();

        if (element.reference)
            if (const auto referenced = data.elementsById[*element.reference]; referenced != nullptr && referenced->path)
                return referenced->path->getBounds();

        return std::nullopt;
    }

    /* Returns the shapes an element is clipped to: one `clipPath` for each clip Drawable sets. */
    std::vector<Item> makeClips (const SVGElement& element) const
    {
        std::vector<Item> clips;

        auto objectBounds = getObjectBounds (element);
        if (! objectBounds && ! element.path && ! element.reference && element.imageBounds)
            objectBounds = element.imageBounds;

        const auto addClip = [&clips] (const auto& elements, bool usesObjectBounds, const std::optional<Rectangle<float>>& bounds, const auto& getPath)
        {
            Item clip ("clipPath");
            Path outline;

            for (const auto& clipElement : elements)
            {
                const Path* path = getPath (*clipElement);
                if (path == nullptr)
                    continue;

                auto transform = clipElement->transform.value_or (AffineTransform::identity());

                if (usesObjectBounds)
                {
                    if (! bounds || bounds->isEmpty())
                        continue;

                    transform = transform.followedBy (AffineTransform::translation (bounds->getX(), bounds->getY())
                                                          .scaled (bounds->getWidth(), bounds->getHeight()));
                }

                outline.appendPath (*path, transform);

                Item shape ("path");
                shape.set ("data", quoted (pathText (*path)));
                shape.transform = transform;
                clip.children.push_back (std::move (shape));
            }

            // Drawable doesn't clip to a shape that is empty
            if (! outline.isEmpty())
                clips.push_back (std::move (clip));
        };

        const auto getClipPath = [this] (const SVGElement& clipElement) -> const Path*
        {
            if (clipElement.path)
                return std::addressof (*clipElement.path);

            if (clipElement.reference)
                if (const auto referenced = data.elementsById[*clipElement.reference]; referenced != nullptr && referenced->path)
                    return std::addressof (*referenced->path);

            return nullptr;
        };

        if (element.clipPathUrl)
        {
            if (const auto clipPath = data.clipPathsById[*element.clipPathUrl])
            {
                const auto bounds = clipPath->units == SVGClipPath::ObjectBoundingBox ? objectBounds : std::optional<Rectangle<float>>();

                if (clipPath->clipPathUrl)
                    if (const auto nested = data.clipPathsById[*clipPath->clipPathUrl])
                        addClip (nested->elements, nested->units == SVGClipPath::ObjectBoundingBox, bounds, getClipPath);

                addClip (clipPath->elements, clipPath->units == SVGClipPath::ObjectBoundingBox, bounds, getClipPath);
            }
        }

        if (element.maskUrl)
        {
            if (const auto mask = data.masksById[*element.maskUrl])
            {
                addClip (mask->elements, mask->maskUnits == SVGMask::ObjectBoundingBox, objectBounds, [] (const SVGElement& maskElement) -> const Path*
                {
                    return maskElement.path ? std::addressof (*maskElement.path) : nullptr;
                });
            }
        }

        return clips;
    }

    //==============================================================================
    /* Returns the element drawing the geometry of an SVG element: the shape it was declared as, or a path. */
    Item makeShape (const SVGElement& element, bool needsPath, bool sharesGeometry)
    {
        if (element.primitive && ! needsPath)
        {
            const auto& primitive = *element.primitive;

            if (primitive.type == SVGElement::Primitive::Type::line)
            {
                Item line ("line");
                line.set ("from", pointText (primitive.line.getStart()));
                line.set ("to", pointText (primitive.line.getEnd()));
                return line;
            }

            if (primitive.type == SVGElement::Primitive::Type::rect)
            {
                Item rect ("rect");
                rect.set ("bounds", rectText (primitive.bounds));

                if (primitive.cornerSize.getWidth() > 0.0f && primitive.cornerSize.getHeight() > 0.0f)
                    rect.set ("cornerSize", sizeText (primitive.cornerSize));

                return rect;
            }

            if (primitive.bounds.getWidth() == primitive.bounds.getHeight())
            {
                Item circle ("circle");
                circle.set ("center", pointText (primitive.bounds.getCenter()));
                circle.set ("radius", numberText (primitive.bounds.getWidth() * 0.5f));
                return circle;
            }

            Item ellipse ("ellipse");
            ellipse.set ("bounds", rectText (primitive.bounds));
            return ellipse;
        }

        auto pathData = quoted (pathText (*element.path));
        const bool isEvenOdd = ! element.path->isUsingNonZeroWinding();

        if (isEvenOdd || sharesGeometry)
            pathData = "path (" + pathData + ")" + (isEvenOdd ? ".withNonZeroWinding (false)" : "");

        // The geometry of an element instanced by `use` is written once
        Item path ("path");
        path.set ("data", sharesGeometry ? addLet (element.id.value_or ("shape"), "Path", pathData) : pathData);
        return path;
    }

    static void writeStroke (Item& item, Cascade& cascade, const Stroke& stroke)
    {
        write (item, cascade, "strokeWidth", { "strokeWidth", numberText (stroke.width), {}, stroke.widthIsExplicit });

        if (stroke.cap)
            write (item, cascade, "cap", { "cap", capText (*stroke.cap), {}, stroke.capIsExplicit });

        if (stroke.join)
            write (item, cascade, "join", { "join", joinText (*stroke.join), {}, stroke.joinIsExplicit });

        write (item, cascade, "strokeMiterLimit", { "strokeMiterLimit", numberText (stroke.miterLimit), {}, true });
        write (item, cascade, "dash", makeDash (stroke.dash, stroke.dashIsExplicit));

        if (stroke.dash && ! stroke.dash->isEmpty())
            write (item, cascade, "dashOffset", { "dashOffset", numberText (stroke.dashOffset), {}, stroke.dashOffsetIsExplicit });
    }

    void writeTextStyle (Item& item, Cascade& cascade, const SVGElement& element)
    {
        const auto size = element.fontSize.value_or (16.0f);
        String font;

        if (options.fontExpression)
            font = options.fontExpression (element.fontFamily.value_or (String()), size, element.fontWeight.value_or (400), element.fontItalic.value_or (false)).trim();

        if (font.isEmpty())
        {
            font = "defaultFont.withHeight (" + numberText (size) + ")";
            usesDefaultFont = true;
        }

        const auto anchor = element.textAnchor == "middle" ? "middle" : (element.textAnchor == "end" ? "end" : "start");

        write (item, cascade, "font", { "font", font, {}, true });
        write (item, cascade, "textAnchor", { "textAnchor", anchor, {}, true });
        write (item, cascade, "letterSpacing", { "letterSpacing", numberText (element.letterSpacing.value_or (0.0f)), {}, true });
    }

    //==============================================================================
    /* Writes a marker as it is painted on a shape: Drawable paints it with what the Graphics holds for the shape. */
    String makeMarker (const SVGMarker& marker, const State& shapeState)
    {
        Item item ("marker");

        if (marker.markerWidth != 3.0f || marker.markerHeight != 3.0f)
            item.set ("size", sizeText ({ marker.markerWidth, marker.markerHeight }));

        if (marker.refX != 0.0f || marker.refY != 0.0f)
            item.set ("ref", pointText ({ marker.refX, marker.refY }));

        if (marker.viewBox)
            item.set ("viewBox", rectText (*marker.viewBox));

        if (marker.orientAutoStartReverse)
            item.set ("orient", "autoStartReverse");
        else if (marker.orient)
            item.set ("orient", numberText (degreesToRadians (*marker.orient)));

        if (marker.markerUnits == SVGMarker::UserSpaceOnUse)
            item.set ("units", "userSpace");

        Scope scope;
        scope.state = shapeState;
        scope.state.fillEnabled = true;
        scope.state.strokeEnabled = false;
        scope.state.currentColor = colorText (Colors::black);
        scope.state.dash.reset();
        scope.state.dashOffset = 0.0f;
        scope.state.depth = shapeState.depth + 1;

        for (const auto& element : marker.elements)
            if (element != nullptr)
                convertElement (*element, scope, item.children);

        return item.children.empty() ? String() : addDefinition ("marker", marker.id, std::move (item));
    }

    /* Writes a pattern as it fills a shape, or returns nothing when Drawable fills nothing with it. */
    std::optional<Value> makePattern (const SVGPattern& pattern, const Path& shape, const State& fillState)
    {
        Rectangle<float> tile (pattern.x, pattern.y, pattern.width, pattern.height);

        if (pattern.patternUnits == SVGPattern::ObjectBoundingBox)
        {
            const auto bounds = shape.getBounds();

            tile = { bounds.getX() + pattern.x * bounds.getWidth(),
                     bounds.getY() + pattern.y * bounds.getHeight(),
                     pattern.width * bounds.getWidth(),
                     pattern.height * bounds.getHeight() };
        }

        if (tile.getWidth() <= 0.0f || tile.getHeight() <= 0.0f)
            return std::nullopt;

        Item item ("pattern");
        item.set ("tile", rectText (tile));

        if (! pattern.patternTransform.isIdentity())
        {
            item.transform = pattern.patternTransform;
            warn ("The pattern '" + pattern.id + "' has a transform: it fills its shapes, while Drawable only tiles their bounds before the transform");
        }

        Scope scope;
        scope.state = fillState;
        scope.state.fillEnabled = true;
        scope.state.strokeEnabled = false;
        scope.state.dash.reset();
        scope.state.dashOffset = 0.0f;
        scope.state.depth = fillState.depth + 1;

        std::vector<Item> content;

        for (const auto& element : pattern.elements)
            if (element != nullptr)
                convertElement (*element, scope, content);

        if (content.empty())
            return std::nullopt;

        const bool hasViewBox = pattern.viewBox && pattern.viewBox->getWidth() > 0.0f && pattern.viewBox->getHeight() > 0.0f;

        if (hasViewBox && pattern.viewBox->getX() == 0.0f && pattern.viewBox->getY() == 0.0f)
        {
            item.set ("viewBox", rectText (*pattern.viewBox));
            item.set ("fitting", "fill");
        }
        else if (hasViewBox)
        {
            // Drawable moves the view box after scaling it, which a view box of the drawing doesn't do
            Item scaled ("group");
            scaled.transform = AffineTransform::scaling (tile.getWidth() / pattern.viewBox->getWidth(), tile.getHeight() / pattern.viewBox->getHeight())
                                   .translated (-pattern.viewBox->getX(), -pattern.viewBox->getY());
            scaled.children = std::move (content);
            content.clear();
            content.push_back (std::move (scaled));
        }

        item.children = std::move (content);

        const auto name = addDefinition ("pattern", pattern.id, std::move (item));
        return Value { "fillPattern", name, {}, true };
    }

    //==============================================================================
    /* Returns the filter of an element: the one its CSS functions make, or the one it refers to. */
    SVGFilter::Ptr getFilter (const SVGElement& element) const
    {
        if (element.filter != nullptr)
            return element.filter;

        return element.filterUrl ? data.filtersById[*element.filterUrl] : SVGFilter::Ptr();
    }

    /* Makes the filter an element is drawn through: where it is rendered for that element, and its primitives. */
    std::optional<Item> makeFilter (const SVGElement& element)
    {
        using Type = FilterGraph::Type;

        const auto filter = getFilter (element);
        if (filter == nullptr)
            return std::nullopt;

        const auto bounds = data.getContentBounds (element);
        const auto region = filter->getRegion (bounds);
        const auto graph = filter->toFilterGraph (bounds);

        // A filter with nothing to run, such as one only blending the element, leaves it to be drawn as it is
        if (region.isEmpty() || graph.isEmpty())
            return std::nullopt;

        Item item ("filter");
        item.set ("bounds", rectText (region));

        if (graph.linearColors)
            item.set ("linearColors", "true");

        // The names of SVG are written as the names of a drawing
        const auto nameText = [] (const String& name)
        {
            if (name == FilterGraph::sourceGraphic)
                return String ("source");

            if (name == FilterGraph::sourceAlpha)
                return String ("sourceAlpha");

            const auto sanitized = sanitizeName (name, "result");
            return sanitized == "source" || sanitized == "sourceAlpha" ? sanitized + "Result" : sanitized;
        };

        const auto setName = [&nameText] (Item& primitive, const char* property, const String& name)
        {
            if (name.isNotEmpty())
                primitive.set (property, nameText (name));
        };

        // The largest of two lengths stands for both, as a drawing blurs and thins evenly
        const auto lengthText = [this] (Point<float> length, const char* what)
        {
            if (length.getX() != length.getY())
                warn (String ("A filter ") + what + " is as large along x as along y in a drawing: the largest of the two is used");

            return numberText (jmax (length.getX(), length.getY()));
        };

        for (const auto& source : graph.primitives)
        {
            static const char* const types[] = { "blur", "offset", "flood", "colorMatrix", "composite", "blend", "morphology", "merge", "lighting", "displacement", "dropShadow" };
            static const char* const operators[] = { "over", "in", "out", "atop", "xor", "lighter", "arithmetic" };
            static const char* const channels[] = { "red", "green", "blue", "alpha" };

            // The pictures a merge stacks are put over each other one at a time
            if (source.type == Type::merge && source.inputs.size() > 1)
            {
                for (int i = 1; i < source.inputs.size(); ++i)
                {
                    Item over ("composite");
                    setName (over, "in", source.inputs[i]);

                    if (i == 1)
                        setName (over, "in2", source.inputs[0]);

                    if (i + 1 == source.inputs.size())
                        setName (over, "result", source.result);

                    item.children.push_back (std::move (over));
                }

                continue;
            }

            Item primitive (source.type == Type::merge ? "offset" : types[static_cast<size_t> (source.type)]);
            setName (primitive, "in", source.type == Type::merge && ! source.inputs.isEmpty() ? source.inputs[0] : source.in);

            switch (source.type)
            {
                case Type::blur:
                    primitive.set ("deviation", lengthText (source.deviation, "blur"));
                    break;

                case Type::offset:
                    primitive.set ("offset", pointText (source.offset));
                    break;

                case Type::flood:
                    primitive.set ("color", colorText (source.color));
                    break;

                case Type::colorMatrix:
                {
                    StringArray values;
                    for (const auto value : source.matrix)
                        values.add (numberText (value));

                    primitive.set ("matrix", quoted (values.joinIntoString (" ")));
                    break;
                }

                case Type::composite:
                    setName (primitive, "in2", source.in2);

                    if (source.compositeOperator != FilterGraph::CompositeOperator::over)
                        primitive.set ("operator", operators[static_cast<size_t> (source.compositeOperator)]);

                    if (source.compositeOperator == FilterGraph::CompositeOperator::arithmetic)
                        for (size_t i = 0; i < source.coefficients.size(); ++i)
                            if (source.coefficients[i] != 0.0f)
                                primitive.set ("k" + String (static_cast<int> (i) + 1), numberText (source.coefficients[i]));

                    break;

                case Type::blend:
                    setName (primitive, "in2", source.in2);
                    primitive.set ("mode", blendText (source.blendMode));
                    break;

                case Type::morphology:
                    primitive.set ("radius", lengthText (source.radius, "morphology"));

                    if (source.dilate)
                        primitive.set ("dilate", "true");

                    break;

                case Type::merge:
                    // Nothing stacked is nothing, which is what a transparent flood is
                    if (source.inputs.isEmpty())
                    {
                        primitive = Item ("flood");
                        primitive.set ("color", colorText (Colors::transparentBlack));
                    }

                    break;

                case Type::lighting:
                    if (source.scale != 1.0f)
                        primitive.set ("scale", numberText (source.scale));

                    if (source.lightingConstant != 1.0f)
                        primitive.set ("constant", numberText (source.lightingConstant));

                    if (source.specular)
                    {
                        primitive.set ("specular", "true");
                        primitive.set ("exponent", numberText (source.specularExponent));
                    }

                    if (source.color != Colors::white)
                        primitive.set ("color", colorText (source.color));

                    if (source.light == FilterGraph::Light::point)
                    {
                        primitive.set ("position", pointText ({ source.lightPosition[0] + region.getX(), source.lightPosition[1] + region.getY() }));
                        primitive.set ("height", numberText (source.lightPosition[2]));
                    }
                    else
                    {
                        primitive.set ("azimuth", numberText (source.lightAzimuth));
                        primitive.set ("elevation", numberText (source.lightElevation));
                    }

                    break;

                case Type::displacement:
                    setName (primitive, "in2", source.in2);
                    primitive.set ("scale", numberText (source.scale));
                    primitive.set ("xChannel", channels[static_cast<size_t> (source.xChannel)]);
                    primitive.set ("yChannel", channels[static_cast<size_t> (source.yChannel)]);
                    break;

                case Type::dropShadow:
                    primitive.set ("offset", pointText (source.offset));
                    primitive.set ("deviation", lengthText (source.deviation, "shadow"));
                    primitive.set ("color", colorText (source.color));
                    break;
            }

            setName (primitive, "result", source.result);
            item.children.push_back (std::move (primitive));
        }

        return item;
    }

    //==============================================================================
    /* Moves what an element sets to the only element it holds, when that takes it all. */
    static bool fold (const Item& container, Item& child)
    {
        static const StringArray painted { "group", "use", "rect", "ellipse", "circle", "line", "path", "text" };

        const bool isImage = child.type == "image";

        if (! isImage && ! painted.contains (child.type))
            return false;

        const bool hasMarkers = child.has ("markerStart") || child.has ("markerMid") || child.has ("markerEnd");

        for (const auto& [name, value] : container.properties)
        {
            // An image has no paint, and markers are only blended by a group around their shape
            if ((isImage && name != "id" && name != "blend") || (hasMarkers && name == "blend"))
                return false;
        }

        for (const auto& [name, value] : container.properties)
        {
            if (name == "id" || ! child.hasSlot (getSlot (name)))
                child.set (name, value);
        }

        if (container.transform)
            child.transform = child.transform ? child.transform->followedBy (*container.transform) : *container.transform;

        if (container.opacity)
            child.opacity = child.opacity.value_or (1.0f) * *container.opacity;

        return true;
    }

    //==============================================================================
    /* Writes an element and what is inside it, the way Drawable::paintElement paints them. */
    void convertElement (const SVGElement& element, Scope scope, std::vector<Item>& target, bool sharesGeometry = false)
    {
        if (element.hidden || scope.state.depth > maxDepth)
            return;

        auto& state = scope.state;

        Item container ("group");

        if (element.id)
            container.set ("id", sanitizeName (*element.id, "element"));

        if (element.color)
            state.currentColor = colorText (*element.color);

        if (element.transform)
            container.transform = *element.transform;

        if (element.opacity && *element.opacity < 1.0f)
            container.opacity = *element.opacity;

        if (element.strokeDashArrayNone)
            state.dash.reset();
        else if (element.strokeDashArray)
            state.dash = element.strokeDashArray;

        state.dashOffset = element.strokeDashOffset.value_or (state.dashOffset);

        // A filter ending by blending what it makes with what is behind the element sets how the element is blended
        auto blend = element.blendMode;

        if (const auto filter = getFilter (element))
            if (const auto backdropBlend = filter->getBackdropBlend())
                blend = backdropBlend;

        if (blend && *blend != state.blend)
        {
            container.set ("blend", blendText (*blend));
            state.blend = *blend;
        }

        // A nested svg is a viewport: it clips to its bounds and maps its view box into them
        std::optional<Item> viewport;

        if (element.tagName == "svg" && element.viewportBounds)
        {
            viewport.emplace ("viewport");
            viewport->set ("bounds", rectText (*element.viewportBounds));

            if (element.viewBox && ! element.viewBox->isEmpty())
            {
                viewport->set ("viewBox", rectText (*element.viewBox));
                writeFitting (*viewport, element.preserveAspectRatioFitting, element.preserveAspectRatioJustification);
            }
        }
        else if (element.viewBox && element.viewportSize)
        {
            const Rectangle<float> area (0.0f, 0.0f, element.viewportSize->getWidth(), element.viewportSize->getHeight());

            viewport.emplace ("group");
            viewport->transform = element.viewBox->getTransformToFit (area, element.preserveAspectRatioFitting, element.preserveAspectRatioJustification);
        }

        auto clips = makeClips (element);

        //==============================================================================
        // The fill: a color or a gradient is left on the Graphics, a pattern is only painted
        const auto fillGradient = element.fillUrl ? data.gradientsById[*element.fillUrl] : SVGGradient::Ptr();
        const auto fillPattern = element.fillUrl ? data.patternsById[*element.fillUrl] : SVGPattern::Ptr();
        const auto objectBounds = getObjectBounds (element);

        const bool isText = element.tagName == "text" || element.tagName == "tspan";

        // A gradient is what the shape is painted with, what it leaves on the Graphics, and what a group shares
        bool isFillDefined = state.fillEnabled;
        bool setsFill = false;
        std::optional<Value> shapeFill, groupFill;

        if (element.fillColor)
        {
            state.fill = makeColor ("fill", element.fillOpacity ? element.fillColor->withMultipliedAlpha (*element.fillOpacity) : *element.fillColor);
            groupFill = state.fill;
            isFillDefined = setsFill = true;
        }
        else if (element.fillCurrentColor)
        {
            state.fill = makeCurrentColor ("fill", element.fillOpacity, state.currentColor);
            groupFill = state.fill;
            isFillDefined = setsFill = true;
        }
        else if (fillGradient != nullptr)
        {
            state.fill = makeGradient ("fill", *fillGradient, objectBounds);
            shapeFill = makeShapeGradient ("fill", *fillGradient, objectBounds);
            groupFill = isText ? std::optional<Value> (state.fill) : makeGroupGradient ("fill", *fillGradient);
            isFillDefined = setsFill = true;
        }
        else if (fillPattern != nullptr)
        {
            isFillDefined = true;
        }
        else if (isFillDefined && ! element.fillUrl && element.fillOpacity)
        {
            // Without a fill of its own an element paints with the root fill, which its fill-opacity still applies to
            state.fill = makeColor ("fill", data.rootFillColor.value_or (Colors::black).withMultipliedAlpha (*element.fillOpacity));
            groupFill = state.fill;
            setsFill = true;
        }

        if (element.noFill)
            groupFill = makeNone ("fill", true);

        const bool drawsFill = isFillDefined && ! element.noFill;
        const bool fillIsExplicit = setsFill || fillPattern != nullptr || element.noFill;
        const auto fillState = state;

        //==============================================================================
        // The stroke, set up after the fill is painted
        const auto strokeGradient = element.strokeUrl ? data.gradientsById[*element.strokeUrl] : SVGGradient::Ptr();

        bool isStrokeDefined = state.strokeEnabled;
        bool setsStroke = false;
        std::optional<Value> shapeStroke, groupStroke;

        if (element.strokeColor)
        {
            state.stroke = makeColor ("stroke", element.strokeOpacity ? element.strokeColor->withMultipliedAlpha (*element.strokeOpacity) : *element.strokeColor);
            groupStroke = state.stroke;
            isStrokeDefined = setsStroke = true;
        }
        else if (element.strokeCurrentColor)
        {
            state.stroke = makeCurrentColor ("stroke", element.strokeOpacity, state.currentColor);
            groupStroke = state.stroke;
            isStrokeDefined = setsStroke = true;
        }
        else if (strokeGradient != nullptr)
        {
            state.stroke = makeGradient ("stroke", *strokeGradient, objectBounds);
            shapeStroke = makeShapeGradient ("stroke", *strokeGradient, objectBounds);
            groupStroke = makeGroupGradient ("stroke", *strokeGradient);
            isStrokeDefined = setsStroke = true;
        }

        if (element.noStroke)
            groupStroke = makeNone ("stroke", true);

        if (element.strokeJoin)
            state.join = element.strokeJoin;

        if (element.strokeCap)
            state.cap = element.strokeCap;

        if (element.strokeWidth)
            state.strokeWidth = *element.strokeWidth;

        //==============================================================================
        // What a use refers to: a shape painted as if it were here, or a shape and elements painted from here
        const auto referenced = element.reference ? data.elementsById[*element.reference] : SVGElement::Ptr();
        const bool useDefinesFill = element.fillColor || element.fillCurrentColor || element.fillUrl || element.noFill;
        const bool useDefinesStroke = element.strokeColor || element.strokeCurrentColor || element.strokeUrl || element.noStroke;

        const bool isInstancedShape = referenced != nullptr
                                   && ! element.path
                                   && ! useDefinesFill
                                   && ! useDefinesStroke
                                   && referenced->path
                                   && referenced->children.empty();

        const bool referenceDefinesStroke = ! isStrokeDefined
                                         && referenced != nullptr
                                         && (referenced->strokeColor || referenced->strokeCurrentColor || referenced->strokeUrl)
                                         && ! referenced->noStroke;

        const bool drawsStroke = (isStrokeDefined || referenceDefinesStroke) && ! element.noStroke;
        const bool strokeIsExplicit = setsStroke || element.noStroke;
        const bool isContainer = ! element.children.empty() || referenced != nullptr;

        const Value color { "color", state.currentColor, {}, element.color.has_value() };

        auto inner = scope;
        inner.state.fillEnabled = drawsFill;
        inner.state.strokeEnabled = isStrokeDefined && ! element.noStroke;
        inner.state.depth = state.depth + 1;

        Stroke ownStroke;
        ownStroke.width = state.strokeWidth;
        ownStroke.widthIsExplicit = element.strokeWidth.has_value();
        ownStroke.cap = state.cap;
        ownStroke.capIsExplicit = element.strokeCap.has_value();
        ownStroke.join = state.join;
        ownStroke.joinIsExplicit = element.strokeJoin.has_value();
        ownStroke.miterLimit = element.strokeMiterLimit.value_or (4.0f);
        ownStroke.dash = state.dash;
        ownStroke.dashIsExplicit = element.strokeDashArray.has_value() || element.strokeDashArrayNone;

        // The dashes of an element are made ahead from its own offset only
        ownStroke.dashOffset = element.strokeDashArray ? element.strokeDashOffset.value_or (0.0f) : state.dashOffset;
        ownStroke.dashOffsetIsExplicit = element.strokeDashArray.has_value() || element.strokeDashOffset.has_value();

        // What the element sets for all it holds goes to its group
        if (isContainer)
        {
            if (groupFill)
                writePaint (container, inner.cascade, "fill", *groupFill, color);

            if (groupStroke)
                writePaint (container, inner.cascade, "stroke", *groupStroke, color);

            if (element.strokeWidth)
                write (container, inner.cascade, "strokeWidth", { "strokeWidth", numberText (state.strokeWidth), {}, true });

            if (element.strokeCap)
                write (container, inner.cascade, "cap", { "cap", capText (*element.strokeCap), {}, true });

            if (element.strokeJoin)
                write (container, inner.cascade, "join", { "join", joinText (*element.strokeJoin), {}, true });

            if (ownStroke.dashIsExplicit)
                write (container, inner.cascade, "dash", makeDash (state.dash, true));

            if (element.strokeDashOffset)
                write (container, inner.cascade, "dashOffset", { "dashOffset", numberText (state.dashOffset), {}, true });

            if (isText)
                writeTextStyle (container, inner.cascade, element);
        }

        const auto writeShapeFill = [&] (Item& shape, Cascade& cascade, Value fill, bool strokes)
        {
            // A line is stroked with its fill when it has no stroke, while Drawable never does
            if (shape.type != "line")
                writePaint (shape, cascade, "fill", fill, color);
            else if (! strokes)
                writePaint (shape, cascade, "fill", makeNone ("fill", true), color);
        };

        std::vector<Item> content;

        //==============================================================================
        if (element.path)
        {
            if (! element.path->isEmpty())
            {
                // Markers are painted with what the Graphics holds once the shape is stroked
                std::array<String, 3> markers;
                const std::array<const std::optional<String>*, 3> markerIds { &element.markerStart, &element.markerMid, &element.markerEnd };

                for (size_t i = 0; i < markers.size(); ++i)
                    if (*markerIds[i])
                        if (const auto marker = data.markersById[**markerIds[i]])
                            markers[i] = makeMarker (*marker, state);

                const bool hasMarkers = std::any_of (markers.begin(), markers.end(), [] (const String& name)
                {
                    return name.isNotEmpty();
                });

                auto shape = makeShape (element, hasMarkers, sharesGeometry);
                auto cascade = inner.cascade;

                auto fill = makeNone ("fill", fillIsExplicit);

                if (drawsFill && fillPattern != nullptr)
                    fill = makePattern (*fillPattern, *element.path, fillState).value_or (makeNone ("fill", true));
                else if (drawsFill)
                    fill = shapeFill.value_or (fillState.fill);

                fill.isExplicit = fillIsExplicit;
                writeShapeFill (shape, cascade, fill, drawsStroke);

                auto stroke = drawsStroke ? shapeStroke.value_or (state.stroke) : makeNone ("stroke", strokeIsExplicit);
                stroke.isExplicit = strokeIsExplicit;
                writePaint (shape, cascade, "stroke", stroke, color);

                if (drawsStroke)
                    writeStroke (shape, cascade, ownStroke);

                if (hasMarkers)
                {
                    // Markers are scaled by the stroke width of their own element
                    const auto markerWidth = element.strokeWidth.value_or (1.0f);

                    if (! drawsStroke)
                        write (shape, cascade, "strokeWidth", { "strokeWidth", numberText (markerWidth), {}, true });
                    else if (markerWidth != state.strokeWidth)
                        warn ("The markers of '" + element.id.value_or (element.tagName) + "' are scaled by the stroke width it is given, while Drawable scales them by 1");

                    static const char* const properties[] = { "markerStart", "markerMid", "markerEnd" };

                    for (size_t i = 0; i < markers.size(); ++i)
                        if (markers[i].isNotEmpty())
                            shape.set (properties[i], markers[i]);
                }

                content.push_back (std::move (shape));
            }
        }
        else if (element.reference && ! isInstancedShape)
        {
            // The shape a use refers to is painted with the paint of the use, or with its own colors
            if (referenced != nullptr && referenced->path && ! referenced->path->isEmpty())
            {
                auto shape = makeShape (*referenced, false, true);
                auto cascade = inner.cascade;

                if (referenced->localTransform)
                    shape.transform = *referenced->localTransform;

                const bool fills = drawsFill && (useDefinesFill || ! referenced->noFill);
                auto fill = makeNone ("fill", element.noFill || drawsFill);

                if (fills && ! useDefinesFill && referenced->fillColor)
                {
                    fill = makeColor ("fill", referenced->fillOpacity ? referenced->fillColor->withMultipliedAlpha (*referenced->fillOpacity) : *referenced->fillColor);
                }
                else if (fills && ! useDefinesFill && referenced->fillCurrentColor)
                {
                    fill = makeCurrentColor ("fill", referenced->fillOpacity, state.currentColor);
                }
                else if (fills)
                {
                    fill = shapeFill.value_or (fillState.fill);
                    fill.isExplicit = setsFill;
                }

                auto stroke = makeNone ("stroke", element.noStroke);
                auto strokeStyle = ownStroke;

                if (drawsStroke)
                {
                    stroke = shapeStroke.value_or (state.stroke);
                    stroke.isExplicit = setsStroke;

                    if (! useDefinesStroke)
                    {
                        if (referenced->strokeColor)
                            stroke = makeColor ("stroke", referenced->strokeOpacity ? referenced->strokeColor->withMultipliedAlpha (*referenced->strokeOpacity) : *referenced->strokeColor);
                        else if (referenced->strokeCurrentColor)
                            stroke = makeCurrentColor ("stroke", referenced->strokeOpacity, state.currentColor);

                        if (referenced->strokeWidth)
                        {
                            strokeStyle.width = *referenced->strokeWidth;
                            strokeStyle.widthIsExplicit = true;
                        }

                        if (referenced->strokeJoin)
                        {
                            strokeStyle.join = referenced->strokeJoin;
                            strokeStyle.joinIsExplicit = true;
                        }

                        if (referenced->strokeCap)
                        {
                            strokeStyle.cap = referenced->strokeCap;
                            strokeStyle.capIsExplicit = true;
                        }
                    }

                    if (referenced->strokeDashArrayNone)
                        strokeStyle.dash.reset();
                    else if (referenced->strokeDashArray)
                        strokeStyle.dash = referenced->strokeDashArray;

                    strokeStyle.dashIsExplicit = strokeStyle.dashIsExplicit || referenced->strokeDashArray.has_value() || referenced->strokeDashArrayNone;
                    strokeStyle.dashOffset = referenced->strokeDashArray ? referenced->strokeDashOffset.value_or (0.0f) : referenced->strokeDashOffset.value_or (state.dashOffset);
                    strokeStyle.dashOffsetIsExplicit = strokeStyle.dashOffsetIsExplicit || referenced->strokeDashArray.has_value() || referenced->strokeDashOffset.has_value();
                }

                writeShapeFill (shape, cascade, fill, drawsStroke);
                writePaint (shape, cascade, "stroke", stroke, color);

                if (drawsStroke)
                    writeStroke (shape, cascade, strokeStyle);

                content.push_back (std::move (shape));
            }
        }
        else if (element.text && element.textPosition)
        {
            // Text is filled with what the Graphics holds, and never stroked
            if (const auto origin = element.getTextOrigin(); drawsFill && origin && element.text->isNotEmpty())
            {
                Item text ("text");
                text.set ("anchor", pointText (*origin));
                text.set ("text", quoted (*element.text));

                auto cascade = inner.cascade;
                auto fill = fillState.fill;
                fill.isExplicit = setsFill;

                writeTextStyle (text, cascade, element);
                writePaint (text, cascade, "fill", fill, color);
                writePaint (text, cascade, "stroke", makeNone ("stroke", true), color);
                content.push_back (std::move (text));
            }
        }
        else if ((element.imageHref || element.image) && element.imageBounds)
        {
            if (drawsFill && element.imageHref && element.imageHref->isNotEmpty())
            {
                Item image ("image");
                image.set ("source", quoted (*element.imageHref));
                image.set ("bounds", rectText (*element.imageBounds));
                writeFitting (image, element.preserveAspectRatioFitting, element.preserveAspectRatioJustification);
                content.push_back (std::move (image));

                if (! element.image || ! element.image->isValid())
                    warn ("The image '" + shortened (*element.imageHref) + "' could not be loaded: Drawable doesn't paint it");
                else if (element.imageHref->startsWithIgnoreCase ("http:") || element.imageHref->startsWithIgnoreCase ("https:"))
                    warn ("The image '" + shortened (*element.imageHref) + "' is not a file: give it to the drawing with DrawingStyle::setImageResolver");
            }
        }

        //==============================================================================
        if (isInstancedShape)
        {
            convertElement (*referenced, inner, content, true);
        }
        else if (referenced != nullptr && ! referenced->children.empty() && visiting.count (referenced.get()) == 0)
        {
            if (state.useDepth >= maxUseDepth)
            {
                warn ("The elements used by '" + element.id.value_or (element.tagName) + "' are nested too deeply to be written");
            }
            else
            {
                // The elements a use refers to become a drawing, painted with what it is given where it is used
                Item body ("drawing");
                std::set<String> reads;

                const bool hasViewBox = referenced->viewBox && ! referenced->viewBox->isEmpty();

                if (hasViewBox)
                {
                    body.set ("viewBox", rectText (*referenced->viewBox));
                    writeFitting (body, referenced->preserveAspectRatioFitting, referenced->preserveAspectRatioJustification);
                }

                Scope bodyScope;
                bodyScope.state = inner.state;
                bodyScope.state.useDepth = state.useDepth + 1;
                bodyScope.cascade.reads = &reads;

                visiting.insert (referenced.get());

                for (const auto& child : referenced->children)
                    if (child != nullptr)
                        convertElement (*child, bodyScope, body.children);

                visiting.erase (referenced.get());

                if (! body.children.empty())
                {
                    Item use ("use");
                    use.set ("drawing", addDefinition ("drawing", referenced->id.value_or ("Symbol"), std::move (body)));

                    if (referenced->localTransform)
                        use.transform = *referenced->localTransform;

                    if (hasViewBox)
                    {
                        const auto size = element.viewportSize.value_or (referenced->viewportSize.value_or (referenced->viewBox->getSize()));
                        use.set ("bounds", rectText ({ 0.0f, 0.0f, size.getWidth(), size.getHeight() }));
                    }

                    // The drawing is given what the Graphics holds here, wherever the groups around don't set it.
                    // What this element left there is its own, the rest comes from around it
                    const auto setsSlot = [&] (const String& slot)
                    {
                        return (slot == "fill" && (setsFill || element.noFill))
                            || (slot == "stroke" && strokeIsExplicit)
                            || (slot == "color" && element.color.has_value())
                            || (slot == "strokeWidth" && element.strokeWidth.has_value())
                            || (slot == "cap" && element.strokeCap.has_value())
                            || (slot == "join" && element.strokeJoin.has_value())
                            || (slot == "dash" && ownStroke.dashIsExplicit)
                            || (slot == "dashOffset" && element.strokeDashOffset.has_value());
                    };

                    auto cascade = inner.cascade;

                    for (const auto& slot : reads)
                    {
                        auto value = getStateValue (slot, inner.state);
                        value.isExplicit = setsSlot (slot);

                        if (value.property.isNotEmpty())
                            writePaint (use, cascade, slot, value, color);
                    }

                    content.push_back (std::move (use));
                }
            }
        }

        for (const auto& child : element.children)
            if (child != nullptr)
                convertElement (*child, inner, content);

        if (content.empty())
            return;

        //==============================================================================
        // A filter is given what the element draws, and what it makes of it is what gets clipped
        if (auto filter = makeFilter (element))
        {
            Item layer ("layer");
            layer.children.push_back (std::move (*filter));
            std::move (content.begin(), content.end(), std::back_inserter (layer.children));

            content.clear();
            content.push_back (std::move (layer));
        }

        // The clips and the viewport of the element go around what it draws, inside its transform
        if (! clips.empty())
        {
            Item clip ("clip");
            clip.children = std::move (clips);
            std::move (content.begin(), content.end(), std::back_inserter (clip.children));

            content.clear();
            content.push_back (std::move (clip));
        }

        if (viewport)
        {
            viewport->children = std::move (content);

            content.clear();
            content.push_back (std::move (*viewport));
        }

        if (! container.hasProperties())
        {
            std::move (content.begin(), content.end(), std::back_inserter (target));
            return;
        }

        if (element.children.empty() && content.size() == 1 && fold (container, content.front()))
        {
            target.push_back (std::move (content.front()));
            return;
        }

        container.children = std::move (content);
        target.push_back (std::move (container));
    }

    //==============================================================================
    const SVGData& data;
    const Options& options;
    StringArray& warnings;

    std::vector<Item> definitions;
    std::map<String, String> letNames, definitionNames;
    std::map<String, std::set<String>> usedNames;
    std::set<String> usedLets;
    std::set<const SVGElement*> visiting;
    bool usesDefaultFont = false;
};

//==============================================================================
String YuiSvgImporter::ImportResult::toText() const
{
    return drawing.isValid() ? YuiWriter::write (drawing) : String();
}

//==============================================================================
YuiSvgImporter::ImportResult YuiSvgImporter::importSvg (const SVGDocument& document, const Options& options)
{
    ImportResult result;

    document.visit ([&] (const SVGData& data)
    {
        result.drawing = Converter (data, options, result.warnings).convert();
    });

    return result;
}

YuiSvgImporter::ImportResult YuiSvgImporter::importSvg (StringRef svgText, const Options& options, const SVGDocument::ParseOptions& parseOptions)
{
    const auto document = SVGParser::parse (svgText, parseOptions);
    return document != nullptr ? importSvg (*document, options) : ImportResult();
}

YuiSvgImporter::ImportResult YuiSvgImporter::importSvg (const File& svgFile, const Options& options, const SVGDocument::ParseOptions& parseOptions)
{
    const auto document = SVGParser::parse (svgFile, parseOptions);
    return document != nullptr ? importSvg (*document, options) : ImportResult();
}

} // namespace yup
