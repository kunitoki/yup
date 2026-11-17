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

File resolveDrawingSourceFile (const String& path, const File& baseDirectory)
{
    if (path.isEmpty())
        return {};

    if (File::isAbsolutePath (path))
        return File (path);

    if (baseDirectory != File())
    {
        if (const auto candidate = baseDirectory.getChildFile (path); candidate.existsAsFile())
            return candidate;
    }

    return File::getCurrentWorkingDirectory().getChildFile (path);
}

const Image& resolveDrawingImage (std::map<String, Image>& cache, const String& source, const File& baseDirectory, const DrawingStyle::ImageResolver& resolver)
{
    const auto cacheKey = baseDirectory != File() ? (baseDirectory.getFullPathName() + "\n" + source) : source;

    if (const auto found = cache.find (cacheKey); found != cache.end())
        return found->second;

    Image image;
    const auto trimmed = source.trim();

    if (auto resolved = resolver != nullptr ? resolver (trimmed) : std::nullopt; resolved.has_value() && resolved->isValid())
    {
        image = std::move (*resolved);
    }
    else if (trimmed.startsWithIgnoreCase ("data:image/"))
    {
        if (const auto marker = trimmed.indexOfIgnoreCase ("base64,"); marker >= 0)
        {
            MemoryOutputStream decoded;
            if (Base64::convertFromBase64 (decoded, trimmed.substring (marker + 7).trim()) && decoded.getDataSize() > 0)
            {
                auto loaded = Image::loadFromData ({ static_cast<const uint8*> (decoded.getData()), decoded.getDataSize() });
                if (loaded.wasOk())
                    image = std::move (loaded.getReference());
            }
        }
    }
    else
    {
        const auto file = resolveDrawingSourceFile (trimmed, baseDirectory);
        if (file.existsAsFile())
        {
            MemoryBlock block;
            if (file.loadFileAsData (block) && block.getSize() > 0)
            {
                auto loaded = Image::loadFromData ({ static_cast<const uint8*> (block.getData()), block.getSize() });
                if (loaded.wasOk())
                    image = std::move (loaded.getReference());
            }
        }
    }

    return cache.emplace (cacheKey, std::move (image)).first->second;
}

std::shared_ptr<Drawable> resolveDrawingSvg (std::map<String, std::shared_ptr<Drawable>>& cache,
                                             const String& source,
                                             bool inlineOnly,
                                             const File& baseDirectory)
{
    const auto cacheKey = baseDirectory != File() ? (baseDirectory.getFullPathName() + "\n" + source) : source;

    if (const auto found = cache.find (cacheKey); found != cache.end())
        return found->second;

    std::shared_ptr<Drawable> drawable;
    const auto trimmed = source.trim();

    if (trimmed.isNotEmpty())
    {
        auto parsed = std::make_shared<Drawable>();

        if (inlineOnly || trimmed.startsWithChar ('<'))
        {
            if (parsed->parseSVG (trimmed))
                drawable = std::move (parsed);
        }
        else
        {
            const auto file = resolveDrawingSourceFile (trimmed, baseDirectory);
            if (file.existsAsFile() && parsed->parseSVG (file))
                drawable = std::move (parsed);
        }
    }

    cache.emplace (cacheKey, drawable);
    return drawable;
}

} // namespace

//==============================================================================
struct DrawingStyle::Node
{
    enum class Kind
    {
        shape,
        image,
        svg,
        repeat,
        layer,
        group,
        clip,
        mask,
        clipShapes,
        pattern,
        marker,
        filter,
        filterPrimitive
    };

    /* A primitive of a filter: what it always does, and the outputs of its program setting the rest. */
    struct FilterPrimitive
    {
        FilterGraph::Primitive base;
        ExpressionOutput<float> deviation, radius, scale, constant, exponent, azimuth, elevation, height, saturate, hueRotate;
        std::array<ExpressionOutput<float>, 4> coefficients;
        ExpressionOutput<Point<float>> offset, position;
        ExpressionOutput<Color> color;
        ExpressionOutput<BlendMode> mode;
    };

    const CompiledExpression& getProgram() const noexcept
    {
        return kind == Kind::shape ? drawing.getProgram() : program;
    }

    bool hasMarkers() const noexcept
    {
        return std::any_of (markers.begin(), markers.end(), [] (int marker) { return marker >= 0; });
    }

    Kind kind = Kind::shape;
    CompiledExpression program;
    ExpressionDrawing drawing;
    int slot = -1;
    bool repeated = false;
    bool svgInlineOnly = false;

    // Groups drawing their children in a rectangle of their own: the level of its bounds, and how it is mapped
    int boundsLevel = -1;
    bool mapsToArea = false;
    bool clipsToArea = false;

    // Patterns and filtered layers: what they draw is only drawn again when it changes
    bool isStatic = true;

    // Layers: the filter they are drawn through. Filters: their primitives are their children
    std::unique_ptr<Node> filter;
    std::unique_ptr<FilterPrimitive> filterPrimitive;
    bool linearColors = false;

    // Markers: how they follow the shape they are drawn on
    bool reversesStart = false;
    bool scalesWithStrokeWidth = true;

    // Shapes: the markers drawn at their start, inner and end vertices
    std::array<int, 3> markers { -1, -1, -1 };

    ExpressionOutput<int> start, end;
    ExpressionOutput<bool> visible;
    ExpressionOutput<float> opacity, strokeWidth, angle;
    ExpressionOutput<BlendMode> blend;
    ExpressionOutput<Color> tint;
    ExpressionOutput<AffineTransform> transform;
    ExpressionOutput<Rectangle<float>> area, clip, viewBox;
    ExpressionOutput<Size<float>> size;
    ExpressionOutput<Point<float>> reference;
    ExpressionOutput<Path> clipPath, maskPath;
    ExpressionOutput<LayerMaskMode> maskMode;
    ExpressionOutput<String> source;
    ExpressionOutput<Fitting> fitting;
    ExpressionOutput<Justification::Flags> justification;
    std::vector<std::unique_ptr<Node>> children;
    std::vector<std::unique_ptr<Node>> masks;
    std::vector<std::unique_ptr<Node>> clipShapes;

    /* An input of the program fed from a drawing property: its value, or whether it is set. */
    struct PropertyInput
    {
        int input = -1;
        int property = -1;
        bool isSet = false;
    };

    /* An input of the program fed by the style: the bounds of a level, or a pattern. */
    struct StyleInput
    {
        int input = -1;
        int index = -1;
    };

    std::vector<PropertyInput> properties;
    std::vector<StyleInput> levels;
    std::vector<StyleInput> patterns;
};

/* An immutable compiled drawing, shared by the styles of components resolving its symbols alike. */
struct DrawingStyle::Compiled
{
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<std::unique_ptr<Node>> patterns;
    std::vector<std::unique_ptr<Node>> markers;
    std::vector<const Node*> evaluated;
    std::map<String, const ExpressionType*> symbols;
    std::vector<ComponentProperty> properties;

    /* The drawings found by name through the resolver, as they were when compiling. */
    std::vector<std::pair<String, DataTree>> dependencies;

    /* The rectangles `bounds` can be: the one of the component, then one per group with bounds of its own. */
    int numLevels = 1;
};

/* The evaluation state of one evaluated node, per style instance. */
struct DrawingStyle::Slot
{
    std::unique_ptr<ExpressionState> state;
    DrawingScope::BoundInputs inputs;
    ExpressionDrawing::Cache cache;
    std::map<String, Image> imageCache;
    std::map<String, std::shared_ptr<Drawable>> svgCache;
};

/* What the elements being painted share: the default font, and the area layers cover with the transform they are composited with. */
struct DrawingStyle::PaintContext
{
    const Font& font;
    Rectangle<float> layerArea;
    AffineTransform layerTransform;
};

/* Draws the tile of a pattern for the shapes it paints. */
struct DrawingStyle::PatternSource final : PaintSource
{
    PatternSource (DrawingStyle& ownerToUse, const Node& nodeToDraw)
        : owner (ownerToUse)
        , node (nodeToDraw)
    {
    }

    Size<float> getContentSize() const override { return tile.getSize(); }

    void paintContent (Graphics& g) override { owner.drawPattern (g, *this); }

    DrawingStyle& owner;
    const Node& node;
    Rectangle<float> tile, contentBounds;
    AffineTransform contentTransform;
    PatternPaint paint;
    uint32 preparedSerial = 0;
};

/* Draws the elements of a layer for its filter. */
struct DrawingStyle::FilterSource final : PaintSource
{
    FilterSource (DrawingStyle& ownerToUse, const Node& nodeToDraw)
        : owner (ownerToUse)
        , node (nodeToDraw)
    {
    }

    Size<float> getContentSize() const override { return region.getSize(); }

    void paintContent (Graphics& g) override { owner.drawFilteredContent (g, *this); }

    DrawingStyle& owner;
    const Node& node;
    Rectangle<float> region;
    const Graphics* parent = nullptr;
    int numRows = 1;
};

//==============================================================================
/* Lowers a drawing tree into one program per element, failing with the path of the offending element. */
class DrawingStyle::Compiler
{
public:
    Compiler (ExpressionContext& contextToUse, const DrawingScope& drawingScopeToUse, const DrawingResolver& drawingResolverToUse, Compiled& resultToBuild)
        : context (contextToUse)
        , drawingScope (drawingScopeToUse)
        , drawingResolver (drawingResolverToUse)
        , result (resultToBuild)
    {
    }

    Result compile (const DataTree& drawing, YuiDiagnostic* failure = nullptr)
    {
        context.setSymbolResolver ([this] (const String& name)
        {
            return resolveSymbol (name);
        });

        auto outcome = Result::ok();

        try
        {
            if (drawing.getType().toString() != "drawing")
                fail ("The root element must be a 'drawing'", drawing);

            const auto name = drawing.getProperty ("name").toString();
            const auto path = name.isNotEmpty() ? name : String ("drawing");

            Scope scope;
            scope.symbols["objectBounds"] = objectBoundsSymbol;

            expansions.push_back (drawing);
            compileDrawing (drawing, path, std::move (scope), nullptr, result.nodes);
            result.numLevels = numLevels;
        }
        catch (const Error& error)
        {
            if (failure != nullptr)
            {
                *failure = YuiDiagnostic {
                    .message = error.message,
                    .element = error.element.isValid() ? error.element : drawing,
                    .property = error.property
                };
            }

            outcome = Result::fail (error.message);
        }

        context.setSymbolResolver (nullptr);
        return outcome;
    }

private:
    struct Error
    {
        String message;
        DataTree element;
        Identifier property;
    };

    struct Statement
    {
        String text;
        String label;
        DataTree element;
        Identifier property;
        String symbol;
        StringArray uses;
    };

    struct LineOrigin
    {
        String label;
        DataTree element;
        Identifier property;
    };

    using Statements = std::vector<Statement>;

    /* An input the compiler declares itself, fed from a drawing property rather than the component. */
    struct PropertyInput
    {
        const ExpressionType* type = nullptr;
        int property = -1;
        bool isSet = false;
    };

    /* A property value ready to embed in a program, and the internal names of the lets it reads. */
    struct Value
    {
        String text;
        StringArray uses;
    };

    /* A paint property a group sets for its children: an expression, a pattern or a marker, or no paint at all. */
    struct Inherited
    {
        Value value;
        int definition = -1;
        bool isNone = false;
    };

    struct Scope;

    /* A pattern or a marker compiled where it is defined, or a drawing compiled where it is used. */
    struct Definition
    {
        int index = -1;
        DataTree drawing;
        std::shared_ptr<const Scope> closure;
    };

    /* What is visible to an element: the statements declaring the lets under internal names, the
       internal name of each visible name, the names declared by the current scope itself, the paint
       set by the groups around it, the definitions it can refer to and the level of its `bounds`. */
    struct Scope
    {
        Statements lets;
        std::map<String, String> symbols;
        StringArray declared;
        std::map<String, Inherited> inherited;
        std::map<String, Definition> definitions;
        int boundsLevel = 0;

        Scope enter() const
        {
            auto inner = *this;
            inner.declared.clear();
            return inner;
        }
    };

    /* A shape: its geometry properties (`name=default` when optional), the statements drawing it
       from the `_name` locals holding them or the path it draws, the paint properties it adds
       (`property:output`), the bounding box of its geometry and whether markers follow it. */
    struct Shape
    {
        const char* type;
        StringArray geometry;
        String lowering;
        String shapePath;
        StringArray paint;
        String objectBounds;
        bool acceptsMarkers = false;
    };

    /* Where a drawing is painted by a `use`, which sets its properties. */
    struct UseSite
    {
        const DataTree& element;
        const String& path;
        const Scope& scope;
    };

    /* Where the children of an element are: inside a repeat or a clip path only shapes are allowed. */
    enum class Placement
    {
        normal,
        repeat,
        clipShapes
    };

    /* Where the elements a layer or a clip keeps apart from the ones it draws go. */
    struct Sinks
    {
        Sinks (std::vector<std::unique_ptr<Node>>* masksToFill = nullptr, std::vector<std::unique_ptr<Node>>* clipShapesToFill = nullptr, std::unique_ptr<Node>* filterToSet = nullptr)
            : masks (masksToFill)
            , clipShapes (clipShapesToFill)
            , filter (filterToSet)
        {
        }

        std::vector<std::unique_ptr<Node>>* masks;
        std::vector<std::unique_ptr<Node>>* clipShapes;
        std::unique_ptr<Node>* filter;
    };

    static constexpr const char* objectBoundsSymbol = "_objectBounds";
    static constexpr const char* levelPrefix = "_bounds";
    static constexpr const char* patternPrefix = "_pattern";
    static constexpr int maxUseDepth = 32;

    //==============================================================================
    [[noreturn]] static void fail (const String& message, const DataTree& element = {}, const Identifier& property = {})
    {
        throw Error { message, element, property };
    }

    static const Shape* findShape (const String& type, bool isAnchoredText = false)
    {
        static const StringArray fittedTextPaint { "font:labelFont", "justification:labelJustification", "stroke:labelStroke", "letterSpacing:labelLetterSpacing" };
        static const StringArray anchoredTextPaint { "font:labelFont", "textAnchor:labelTextAnchor", "stroke:labelStroke", "letterSpacing:labelLetterSpacing" };

        static const std::vector<Shape> shapes {
            { "rect", { "bounds" }, "rect = _bounds;", {}, { "cornerRadius", "topLeftRadius", "topRightRadius", "bottomLeftRadius", "bottomRightRadius", "cornerSize" }, "_bounds" },
            { "ellipse", { "bounds" }, "ellipse = _bounds;", {}, {}, "_bounds" },
            { "circle", { "center", "radius" }, "point = _center; radius = _radius;", {}, {}, "rect (_center.x - _radius, _center.y - _radius, _radius * 2, _radius * 2)" },
            { "arc", { "center", "radius", "from", "to" }, {}, "path().addArc (rect (_center.x - _radius, _center.y - _radius, _radius * 2, _radius * 2), _from, _to, true)", {}, {} },
            { "line", { "from", "to" }, "line = line (_from, _to);", {}, {}, "rect (min (_from.x, _to.x), min (_from.y, _to.y), abs (_to.x - _from.x), abs (_to.y - _from.y))", true },
            { "path", { "data" }, {}, "path (_data)", {}, {}, true },
            { "polygon", { "center", "sides", "radius", "angle=0" }, {}, "path().addPolygon (_center, _sides, _radius, _angle)", {}, {} },
            { "star", { "center", "points", "innerRadius", "outerRadius", "angle=0" }, {}, "path().addStar (_center, _points, _innerRadius, _outerRadius, _angle)", {}, {} },
            { "text", { "bounds", "text" }, "labelRect = _bounds; label = _text;", {}, fittedTextPaint, "_bounds" }
        };

        static const Shape anchoredText { "text", { "anchor", "text" }, "labelAnchor = _anchor; label = _text;", {}, anchoredTextPaint, {} };

        if (isAnchoredText)
            return &anchoredText;

        for (const auto& shape : shapes)
            if (type == shape.type)
                return &shape;

        return nullptr;
    }

    static const StringArray& getCommonPaint()
    {
        static const StringArray paint { "fill", "fillGradient", "stroke", "strokeGradient", "strokeWidth", "strokeMiterLimit", "strokePosition",
                                         "dash:strokeDash", "dashOffset:strokeDashOffset", "cap:strokeCap",
                                         "join:strokeJoin", "feather", "opacity", "blend", "tint", "transform", "clip", "clipPath", "visible",
                                         "fillPattern", "strokePattern", "fillOpacity", "strokeOpacity" };
        return paint;
    }

    /* The paint properties a group sets for the shapes inside it, with the output checking their values. */
    static const StringArray& getInheritablePaint()
    {
        static const StringArray paint { "fill", "fillGradient", "fillPattern", "fillOpacity", "stroke", "strokeGradient", "strokePattern", "strokeOpacity",
                                         "strokeWidth", "strokeMiterLimit", "dash:strokeDash", "dashOffset:strokeDashOffset", "cap:strokeCap", "join:strokeJoin",
                                         "feather", "font:labelFont", "textAnchor:labelTextAnchor", "letterSpacing:labelLetterSpacing",
                                         "markerStart", "markerMid", "markerEnd" };
        return paint;
    }

    static void addPropertyNames (StringArray& names, const StringArray& table)
    {
        for (const auto& entry : table)
            names.add (entry.upToFirstOccurrenceOf (":", false, false));
    }

    static bool isFillPaint (const String& property) noexcept
    {
        return property == "fill" || property == "fillGradient" || property == "fillPattern";
    }

    static bool isStrokePaint (const String& property) noexcept
    {
        return property == "stroke" || property == "strokeGradient" || property == "strokePattern";
    }

    static bool isPatternProperty (const String& property) noexcept
    {
        return property == "fillPattern" || property == "strokePattern";
    }

    static int getMarkerIndex (const String& property) noexcept
    {
        return property == "markerStart" ? 0 : (property == "markerMid" ? 1 : (property == "markerEnd" ? 2 : -1));
    }

    /* `none` removes a paint, a pattern or a marker, instead of being read as an expression. */
    static bool isNone (const DataTree& element, const Identifier& property)
    {
        return element.getProperty (property).toString().trim() == "none";
    }

    static String wrapForOutput (const String& output, const String& value)
    {
        return output == "strokeDash" ? ("dash (" + value + ")") : value;
    }

    static String levelSymbol (int level)
    {
        return level == 0 ? String ("bounds") : levelPrefix + String (level);
    }

    /* Returns the number after the prefix of a name like `_bounds3`, or -1 for other names. */
    static int parseIndexedName (const String& name, const char* prefix)
    {
        const auto number = name.fromFirstOccurrenceOf (prefix, false, false);
        return name.startsWith (prefix) && number.isNotEmpty() && number.containsOnly ("0123456789") ? number.getIntValue() : -1;
    }

    static void eraseInherited (std::map<String, Inherited>& inherited, bool (*matches) (const String&) noexcept)
    {
        std::erase_if (inherited, [matches] (const auto& entry)
        {
            return matches (entry.first);
        });
    }

    static String findPaintOutput (const StringArray& table, const String& property)
    {
        for (const auto& entry : table)
            if (entry.upToFirstOccurrenceOf (":", false, false) == property)
                return entry.containsChar (':') ? entry.fromFirstOccurrenceOf (":", false, false) : entry;

        return {};
    }

    template <class T>
    static ExpressionOutput<T> findOutput (const CompiledExpression& program, StringRef name)
    {
        auto output = program.getOutput<T> (name);
        return output.wasOk() ? output.getReference() : ExpressionOutput<T> {};
    }

    //==============================================================================
    static String elementPath (const String& parentPath, const DataTree& element, int index)
    {
        return parentPath + "/" + element.getType().toString()
             + (element.hasProperty ("id") ? "#" + element.getProperty ("id").toString() : "[" + String (index) + "]");
    }

    static void checkElement (const DataTree& element, const String& path, const StringArray& allowed, bool allowChildren)
    {
        for (int i = 0; i < element.getNumProperties(); ++i)
        {
            const auto propertyName = element.getPropertyName (i);
            const auto property = propertyName.toString();

            if (! allowed.contains (property))
                fail (path + ": unknown property '" + property + "'", element, propertyName);
        }

        if (! allowChildren && element.getNumChildren() > 0)
            fail (path + ": a '" + element.getType().toString() + "' can't have children", element.getChild (0));
    }

    static String getRequired (const DataTree& element, const Identifier& property, const String& path)
    {
        if (! element.hasProperty (property))
            fail (path + ": missing the required property '" + property.toString() + "'", element);

        return element.getProperty (property).toString();
    }

    static String parseName (const DataTree& element, const String& path)
    {
        static const String letters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

        const auto name = getRequired (element, "name", path);

        if (name.isEmpty() || ! letters.containsChar (name[0]) || ! name.containsOnly (letters + "0123456789_"))
            fail (path + ".name: '" + name + "' is not a valid name, use letters, digits and '_', starting with a letter", element, "name");

        return name;
    }

    /* Parses a property as a single expression and returns it normalized and parenthesized, so
       it can be embedded in a program on one line without leaking comments or extra statements.
       References to lets are renamed to their internal names. */
    static Value parseValue (const DataTree& element, const Identifier& property, const String& path, const Scope& scope, bool isShapePaint = false)
    {
        const auto label = path + "." + property.toString();
        const auto text = getRequired (element, property, path);

        int offset = 0;
        auto parsed = Expression::parseExpression (text, offset);

        if (parsed.failed())
            fail (label + ": " + parsed.getErrorMessage(), element, property);

        if (offset != (int) text.getNumBytesAsUTF8())
            fail (label + ": unexpected text after the expression", element, property);

        auto expression = parsed.getReference();
        Value value;

        for (const auto& reference : expression.getReferencedSymbols())
        {
            const auto found = scope.symbols.find (reference.upToFirstOccurrenceOf (".", false, false));
            if (found == scope.symbols.end() || value.uses.contains (found->second))
                continue;

            expression = expression.withRenamedSymbol (found->first, found->second);
            value.uses.add (found->second);
        }

        if (! isShapePaint && value.uses.contains (objectBoundsSymbol))
            fail (label + ": 'objectBounds' can only be read by the paint properties of a shape, or of the groups around it", element, property);

        value.text = "(" + expression.toString() + ")";
        return value;
    }

    /* Returns the lets of a scope the given internal names read, directly or through other lets,
       in declaration order. Compiling only those keeps each program small. */
    static Statements selectLets (const Scope& scope, StringArray needed)
    {
        std::vector<bool> selected (scope.lets.size(), false);

        for (auto i = scope.lets.size(); i-- > 0;)
        {
            if (! needed.contains (scope.lets[i].symbol))
                continue;

            selected[i] = true;
            needed.addArray (scope.lets[i].uses);
        }

        Statements lets;
        for (size_t i = 0; i < scope.lets.size(); ++i)
            if (selected[i])
                lets.push_back (scope.lets[i]);

        return lets;
    }

    /* Maps a `line:column: message` compiler error to the element property that emitted the line. */
    static Error locateError (const String& error,
                              const std::vector<LineOrigin>& lineOrigins,
                              const String& path,
                              const DataTree& fallbackElement,
                              const Identifier& fallbackProperty = {})
    {
        const auto line = error.upToFirstOccurrenceOf (":", false, false);
        const auto index = line.getIntValue() - 1;

        if (line.containsOnly ("0123456789") && isPositiveAndBelow (index, static_cast<int> (lineOrigins.size())))
        {
            const auto& origin = lineOrigins[static_cast<size_t> (index)];
            return {
                origin.label + ": " + error.fromFirstOccurrenceOf (":", false, false).fromFirstOccurrenceOf (":", false, false).trimStart(),
                origin.element.isValid() ? origin.element : fallbackElement,
                origin.property
            };
        }

        return { path + ": " + error, fallbackElement, fallbackProperty };
    }

    CompiledExpression compileStatements (const Statements& statements,
                                          const String& path,
                                          const DataTree& fallbackElement,
                                          const Identifier& fallbackProperty = {},
                                          StringArray* referencedSymbols = nullptr)
    {
        String text;
        std::vector<LineOrigin> lineOrigins;

        for (const auto& statement : statements)
        {
            text << statement.text << "\n";

            for (int i = StringArray::fromLines (statement.text).size(); --i >= 0;)
                lineOrigins.push_back ({ statement.label, statement.element, statement.property });
        }

        auto parsed = Expression::parse (text);
        if (parsed.failed())
            throw locateError (parsed.getErrorMessage(), lineOrigins, path, fallbackElement, fallbackProperty);

        if (referencedSymbols != nullptr)
            *referencedSymbols = parsed.getReference().getReferencedSymbols();

        auto compiled = CompiledExpression::compile (parsed.getReference(), context);
        if (compiled.failed())
            throw locateError (compiled.getErrorMessage(), lineOrigins, path, fallbackElement, fallbackProperty);

        return compiled.getReference();
    }

    /* Declares a let under an internal name, so it can't clash with the outputs the elements assign. */
    void declare (Scope& scope,
                  const String& name,
                  const Value& value,
                  const String& path,
                  const String& label,
                  const DataTree& element,
                  const Identifier& property = {})
    {
        if (scope.declared.contains (name))
            fail (path + ".name: '" + name + "' is already defined", element, "name");

        const auto symbol = "_let" + String (numSymbols++);

        scope.lets.push_back ({ "let " + symbol + " = " + value.text + ";", label, element, property, symbol, value.uses });
        scope.symbols[name] = symbol;
        scope.declared.add (name);
    }

    void addSlot (Node& node, bool repeated)
    {
        node.slot = (int) result.evaluated.size();
        node.repeated = repeated;
        result.evaluated.push_back (&node);

        const auto& program = node.getProgram();

        for (int i = 0; i < program.getNumInputs(); ++i)
        {
            if (! program.usesInput (i))
                continue;

            // The tile of a pattern is kept while its elements only read what the pattern itself sets
            const auto name = program.getInputName (i);
            bool keepsTile = false;

            if (const auto found = propertyInputs.find (name); found != propertyInputs.end())
            {
                node.properties.push_back ({ i, found->second.property, found->second.isSet });
            }
            else if (const auto level = parseIndexedName (name, levelPrefix); level >= 0)
            {
                node.levels.push_back ({ i, level });
                keepsTile = pattern != nullptr && level >= pattern->boundsLevel;
            }
            else if (const auto index = parseIndexedName (name, patternPrefix); index >= 0)
            {
                node.patterns.push_back ({ i, index });
                keepsTile = result.patterns[(size_t) index]->isStatic;
            }
            else
            {
                keepsTile = name == "_index";
            }

            if (pattern != nullptr && ! keepsTile)
                pattern->isStatic = false;
        }
    }

    /* The answers about the component decide whether another component can share the compilation, so
       they are recorded. The inputs fed by the style are the drawing's own, the same for every component. */
    const ExpressionType* resolveSymbol (const String& name)
    {
        if (const auto found = propertyInputs.find (name); found != propertyInputs.end())
            return found->second.type;

        if (const auto level = parseIndexedName (name, levelPrefix); level > 0 && level < numLevels)
            return context.getType<Rectangle<float>>();

        if (const auto index = parseIndexedName (name, patternPrefix); isPositiveAndBelow (index, (int) result.patterns.size()))
            return context.getType<PatternPaint>();

        const auto* type = drawingScope.resolveSymbol (name);
        result.symbols.emplace (name, type);
        return type;
    }

    //==============================================================================
    /* Declares the properties of the drawing as lets of its root scope, visible to every element. Each
       reads the property input when the component has a value for it, else the default. The properties
       of a drawing painted by a `use` read what the `use` sets instead, else the default. */
    void declareProperties (const DataTree& drawing, const String& drawingPath, Scope& scope, const UseSite* use)
    {
        int index = 0;

        for (const auto& element : drawing)
        {
            if (element.getType().toString() != "property")
                continue;

            const auto path = elementPath (drawingPath, element, index++);

            checkElement (element, path, { "id", "name", "type", "value" }, false);

            const auto name = parseName (element, path);
            const auto typeName = getRequired (element, "type", path);
            const auto type = ComponentProperty::getTypeFromName (typeName);
            const auto elementType = ComponentProperty::getElementTypeFromName (typeName);
            const auto* expressionType = drawingScope.getExpressionType (type, elementType);

            if (expressionType == nullptr)
                fail (path + ".type: '" + typeName + "' can't be used in a drawing, use number, integer, boolean, string, color, point, rectangle, transform, font or an array of scalar types", element, "type");

            if (use != nullptr)
            {
                declareUsedProperty (element, path, name, type, typeName, scope, *use);
                continue;
            }

            const auto property = (int) result.properties.size();
            const auto input = "_property_" + name;

            propertyInputs[input] = { expressionType, property, false };
            result.properties.push_back (ComponentProperty { .name = Identifier (name), .type = type, .elementType = elementType });

            // An array is empty when the component doesn't set it, so it needs no default
            if (type == ComponentProperty::Type::array)
            {
                if (element.hasProperty ("value"))
                    fail (path + ".value: an array property can't have a default", element, "value");

                declare (scope, name, { input, {} }, path, path, element);
                continue;
            }

            auto fallback = element.hasProperty ("value") ? parseValue (element, "value", path, scope) : Value { getDefaultText (type), {} };

            if (element.hasProperty ("value"))
                checkPropertyValue (scope, fallback, type, typeName, path + ".value", element, "value");

            // A number default can be any number, the property is a float
            if (type == ComponentProperty::Type::number)
                fallback.text = "float " + fallback.text;

            const auto isSet = "_hasProperty_" + name;
            propertyInputs[isSet] = { context.getType<bool>(), property, true };

            declare (scope, name, { "(" + isSet + " ? " + input + " : " + fallback.text + ")", fallback.uses }, path, path + ".value", element, element.hasProperty ("value") ? Identifier ("value") : Identifier {});
        }
    }

    /* Declares a property of a drawing painted by a `use`: the value the `use` sets, read where the `use` is, or its default. */
    void declareUsedProperty (const DataTree& element,
                              const String& path,
                              const String& name,
                              ComponentProperty::Type type,
                              const String& typeName,
                              Scope& scope,
                              const UseSite& use)
    {
        const Identifier argument (name);
        const bool isSet = use.element.hasProperty (argument);

        if (type == ComponentProperty::Type::array)
            fail (use.path + ": the array property '" + name + "' of the drawing can't be set by a 'use'", use.element, isSet ? argument : Identifier ("drawing"));

        Value value;

        if (isSet)
        {
            value = parseValue (use.element, argument, use.path, use.scope);
            checkPropertyValue (use.scope, value, type, typeName, use.path + "." + name, use.element, argument);
        }
        else if (element.hasProperty ("value"))
        {
            value = parseValue (element, "value", path, scope);
            checkPropertyValue (scope, value, type, typeName, path + ".value", element, "value");
        }
        else
        {
            value = { getDefaultText (type), {} };
        }

        if (type == ComponentProperty::Type::number)
            value.text = "float " + value.text;

        if (isSet)
            declare (scope, name, value, path, use.path + "." + name, use.element, argument);
        else
            declare (scope, name, value, path, path + ".value", element, element.hasProperty ("value") ? Identifier ("value") : Identifier {});
    }

    static String getDefaultText (ComponentProperty::Type type)
    {
        switch (type)
        {
            case ComponentProperty::Type::integer: return "(0)";
            case ComponentProperty::Type::boolean: return "(false)";
            case ComponentProperty::Type::string:  return "(\"\")";
            case ComponentProperty::Type::color:   return "(#00000000)";
            case ComponentProperty::Type::point:   return "(point (0, 0))";

            case ComponentProperty::Type::rectangle:
                return "(rect (0, 0, 0, 0))";

            case ComponentProperty::Type::transform:
                return "(transform())";

            case ComponentProperty::Type::font:
                return "(font())";

            default: return "(0.0)";
        }
    }

    /* Compiles the value of a property on its own, failing when its type doesn't match the property. */
    void checkPropertyValue (const Scope& scope,
                             const Value& value,
                             ComponentProperty::Type type,
                             const String& typeName,
                             const String& label,
                             const DataTree& element,
                             const Identifier& property)
    {
        auto statements = selectLets (scope, value.uses);
        statements.push_back ({ value.text + ";", label, element, property });

        const auto program = compileStatements (statements, label, element, property);
        const ExpressionType* actual = nullptr;

        for (int i = 0; i < program.getNumOutputs(); ++i)
            if (program.getOutputName (i) == "result")
                actual = program.getOutputType (i);

        jassert (actual != nullptr);

        const auto* expected = drawingScope.getExpressionType (type, ComponentProperty::Type::any);
        const auto isNumber = actual == context.getType<int>() || actual == context.getType<double>();

        if (actual != nullptr && actual != expected && ! (type == ComponentProperty::Type::number && isNumber))
            fail (label + ": the type '" + actual->getName() + "' doesn't match the property type '" + typeName + "'", element, property);
    }

    //==============================================================================
    /* Compiles a drawing: its properties, then its elements, mapped from its view box when it has one. */
    void compileDrawing (const DataTree& drawing, const String& path, Scope scope, const UseSite* use, std::vector<std::unique_ptr<Node>>& target)
    {
        checkElement (drawing, path, { "id", "name", "viewBox", "fitting", "justification" }, true);
        declareProperties (drawing, path, scope, use);

        if (! drawing.hasProperty ("viewBox"))
        {
            for (const auto* property : { "fitting", "justification" })
                if (drawing.hasProperty (property))
                    fail (path + "." + property + ": needs a 'viewBox' to fit in the bounds", drawing, property);

            compileElements (drawing, path, std::move (scope), Placement::normal, target);
            return;
        }

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::group;
        node->mapsToArea = true;
        node->boundsLevel = numLevels++;

        compileProperties (*node, drawing, path, scope, { "viewBox:_viewBox", "fitting:_fitting", "justification:_justification" }, { { "_area = (" + levelSymbol (scope.boundsLevel) + ");", path, drawing } });
        compileElements (drawing, path, enterLevel (scope, node->boundsLevel), Placement::normal, node->children);

        target.push_back (std::move (node));
    }

    void compileElements (const DataTree& parent,
                          const String& parentPath,
                          Scope scope,
                          Placement placement,
                          std::vector<std::unique_ptr<Node>>& target,
                          Sinks sinks = {})
    {
        static const StringArray containers { "layer", "group", "clip", "repeat", "viewport", "use", "drawing", "pattern", "marker" };

        std::map<String, int> counts;

        for (const auto& element : parent)
        {
            const auto type = element.getType().toString();
            const auto path = elementPath (parentPath, element, counts[type]++);

            if (type == "property")
            {
                // The properties of the drawing are declared up front, see declareProperties
                if (parent.getType().toString() != "drawing")
                    fail (path + ": a 'property' can only be declared at the top of a drawing", element);

                continue;
            }

            if (type == "let")
            {
                checkElement (element, path, { "id", "name", "value" }, false);

                const auto name = parseName (element, path);
                declare (scope, name, parseValue (element, "value", path, scope), path, path + ".value", element, "value");

                auto check = selectLets (scope, { scope.symbols[name] });
                check.push_back ({ scope.symbols[name] + ";", path + ".value", element, "value" });
                compileStatements (check, path + ".value", element, "value");
                continue;
            }

            if (type == "mask")
            {
                if (sinks.masks == nullptr)
                    fail (path + ": a 'mask' can only appear inside a 'layer'", element);

                sinks.masks->push_back (compileMask (element, path, scope));
                continue;
            }

            if (type == "filter")
            {
                if (sinks.filter == nullptr)
                    fail (path + ": a 'filter' can only appear inside a 'layer'", element);

                if (*sinks.filter != nullptr)
                    fail (path + ": a 'layer' is drawn through a single 'filter'", element);

                *sinks.filter = compileFilter (element, path, scope);
                continue;
            }

            if (type == "clipPath")
            {
                if (sinks.clipShapes == nullptr)
                    fail (path + ": a 'clipPath' can only appear inside a 'clip' or a 'layer'", element);

                sinks.clipShapes->push_back (compileClipShapes (element, path, scope));
                continue;
            }

            if (placement == Placement::repeat && containers.contains (type))
                fail (path + ": a '" + type + "' can't be nested inside a repeat", element);

            if (placement == Placement::clipShapes && type != "repeat" && findShape (type) == nullptr)
                fail (path + ": a '" + type + "' has no outline to clip to", element);

            if (type == "drawing")
                addDefinition (scope, "drawing", element, path, { -1, element, std::make_shared<Scope> (scope) });
            else if (type == "pattern")
                addDefinition (scope, "pattern", element, path, { compilePattern (element, path, scope), {}, nullptr });
            else if (type == "marker")
                addDefinition (scope, "marker", element, path, { compileMarker (element, path, scope), {}, nullptr });
            else if (type == "layer")
                target.push_back (compileLayer (element, path, scope));
            else if (type == "group")
                target.push_back (compileGroup (element, path, scope));
            else if (type == "viewport")
                target.push_back (compileViewport (element, path, scope));
            else if (type == "use")
                target.push_back (compileUse (element, path, scope));
            else if (type == "clip")
                target.push_back (compileClip (element, path, scope));
            else if (type == "repeat")
                target.push_back (compileRepeat (element, path, scope));
            else if (type == "image")
                target.push_back (compileMedia (element, path, scope, placement == Placement::repeat, Node::Kind::image));
            else if (type == "svg")
                target.push_back (compileMedia (element, path, scope, placement == Placement::repeat, Node::Kind::svg));
            else
                target.push_back (compileShape (element, path, scope, placement == Placement::repeat));
        }
    }

    //==============================================================================
    /* Makes a drawing, a pattern or a marker visible by name to the elements after it. */
    void addDefinition (Scope& scope, const char* kind, const DataTree& element, const String& path, Definition definition)
    {
        const auto name = parseName (element, path);
        const auto key = String (kind) + ":" + name;

        if (scope.declared.contains (key))
            fail (path + ".name: the " + kind + " '" + name + "' is already defined", element, "name");

        scope.declared.add (key);
        scope.definitions[key] = std::move (definition);
    }

    static const Definition* findDefinition (const Scope& scope, const char* kind, const String& name)
    {
        const auto found = scope.definitions.find (String (kind) + ":" + name);
        return found != scope.definitions.end() ? &found->second : nullptr;
    }

    /* Reads a property naming a pattern or a marker, and returns where it is in the result. */
    int findDefinitionIndex (const DataTree& element, const Identifier& property, const String& path, const Scope& scope)
    {
        const auto* kind = isPatternProperty (property.toString()) ? "pattern" : "marker";
        const auto name = element.getProperty (property).toString().trim();

        if (const auto* definition = findDefinition (scope, kind, name))
            return definition->index;

        fail (path + "." + property.toString() + ": unknown " + kind + " '" + name + "'", element, property);
    }

    /* Returns the scope of the children of a group drawing them in bounds of its own. */
    static Scope enterLevel (const Scope& scope, int level)
    {
        auto inner = scope.enter();
        inner.boundsLevel = level;
        inner.symbols["bounds"] = levelSymbol (level);
        return inner;
    }

    /* Returns the scope the properties of an element are read in, where its `color` is `currentColor`. */
    Scope enterElement (const DataTree& element, const String& path, const Scope& scope)
    {
        auto inner = scope.enter();

        if (! element.hasProperty ("color"))
            return inner;

        declare (inner, "currentColor", parseValue (element, "color", path, scope), path, path + ".color", element, "color");

        auto check = selectLets (inner, { inner.symbols["currentColor"] });
        check.push_back ({ "_color = " + inner.symbols["currentColor"] + ";", path + ".color", element, "color" });
        compileStatements (check, path + ".color", element, "color");

        return inner;
    }

    /* Reads the paint a group sets for the shapes inside it, checking the values where they are written. */
    void inheritPaint (const DataTree& element, const String& path, Scope& scope)
    {
        Statements checks;
        StringArray uses;

        for (const auto& entry : getInheritablePaint())
        {
            const auto property = entry.upToFirstOccurrenceOf (":", false, false);
            const Identifier propId (property);

            if (! element.hasProperty (propId))
                continue;

            if (isFillPaint (property))
                eraseInherited (scope.inherited, isFillPaint);
            else if (isStrokePaint (property))
                eraseInherited (scope.inherited, isStrokePaint);

            const bool namesDefinition = isPatternProperty (property) || getMarkerIndex (property) >= 0;

            if (isNone (element, propId) && (getMarkerIndex (property) >= 0 || property == "fill" || property == "stroke"))
            {
                scope.inherited.erase (property);

                if (property == "fill")
                    scope.inherited[property].isNone = true;

                continue;
            }

            if (namesDefinition)
            {
                scope.inherited[property] = { {}, findDefinitionIndex (element, propId, path, scope), false };
                continue;
            }

            const auto output = entry.containsChar (':') ? entry.fromFirstOccurrenceOf (":", false, false) : property;
            const auto value = parseValue (element, propId, path, scope, true);

            checks.push_back ({ output + " = " + wrapForOutput (output, value.text) + ";", path + "." + property, element, propId });
            uses.addArray (value.uses);
            scope.inherited[property] = { value, -1, false };
        }

        if (checks.empty())
            return;

        auto statements = selectLets (scope, uses);

        if (uses.contains (objectBoundsSymbol))
            statements.push_back ({ String ("let ") + objectBoundsSymbol + " = rect (0, 0, 0, 0);", path, element });

        statements.insert (statements.end(), checks.begin(), checks.end());
        compileStatements (statements, path, element);
    }

    /* Compiles the properties of a group into its program: `property:output` entries, after the statements it always needs. */
    void compileProperties (Node& node, const DataTree& element, const String& path, const Scope& scope, const StringArray& properties, Statements assignments = {})
    {
        StringArray uses;

        for (const auto& entry : properties)
        {
            const auto property = entry.upToFirstOccurrenceOf (":", false, false);
            const Identifier propId (property);

            if (! element.hasProperty (propId))
                continue;

            const auto output = entry.containsChar (':') ? entry.fromFirstOccurrenceOf (":", false, false) : property;
            const auto value = parseValue (element, propId, path, scope);

            assignments.push_back ({ output + " = " + value.text + ";", path + "." + property, element, propId });
            uses.addArray (value.uses);
        }

        if (assignments.empty())
            return;

        auto statements = selectLets (scope, uses);
        statements.insert (statements.end(), assignments.begin(), assignments.end());

        node.program = compileStatements (statements, path, element);
        node.visible = findOutput<bool> (node.program, "visible");
        node.opacity = findOutput<float> (node.program, "opacity");
        node.blend = findOutput<BlendMode> (node.program, "blend");
        node.transform = findOutput<AffineTransform> (node.program, "transform");
        node.area = findOutput<Rectangle<float>> (node.program, "_area");
        node.viewBox = findOutput<Rectangle<float>> (node.program, "_viewBox");
        node.fitting = findOutput<Fitting> (node.program, "_fitting");
        node.justification = findOutput<Justification::Flags> (node.program, "_justification");
        node.size = findOutput<Size<float>> (node.program, "_size");
        node.reference = findOutput<Point<float>> (node.program, "_reference");
        node.angle = findOutput<float> (node.program, "_angle");
        addSlot (node, false);
    }

    std::unique_ptr<Node> compileGroup (const DataTree& element, const String& path, const Scope& outerScope)
    {
        StringArray allowed { "id", "transform", "visible", "opacity", "blend", "color" };
        addPropertyNames (allowed, getInheritablePaint());
        checkElement (element, path, allowed, true);

        auto scope = enterElement (element, path, outerScope);
        inheritPaint (element, path, scope);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::group;

        compileProperties (*node, element, path, scope, { "transform", "visible", "opacity", "blend" });
        compileElements (element, path, scope.enter(), Placement::normal, node->children);
        return node;
    }

    std::unique_ptr<Node> compileViewport (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, { "id", "bounds", "viewBox", "fitting", "justification", "visible" }, true);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::group;
        node->mapsToArea = true;
        node->clipsToArea = true;
        node->boundsLevel = numLevels++;

        Statements defaults;
        if (! element.hasProperty ("bounds"))
            defaults.push_back ({ "_area = (" + levelSymbol (scope.boundsLevel) + ");", path, element });

        compileProperties (*node, element, path, scope, { "bounds:_area", "viewBox:_viewBox", "fitting:_fitting", "justification:_justification", "visible" }, std::move (defaults));
        compileElements (element, path, enterLevel (scope, node->boundsLevel), Placement::normal, node->children);
        return node;
    }

    /* Paints a drawing found by name: it sees what was visible where it was defined, and the bounds, the paint and the color of where it is used. */
    std::unique_ptr<Node> compileUse (const DataTree& element, const String& path, const Scope& outerScope)
    {
        const auto name = getRequired (element, "drawing", path).trim();

        DataTree drawing;
        std::shared_ptr<const Scope> closure;

        if (const auto* definition = findDefinition (outerScope, "drawing", name))
        {
            drawing = definition->drawing;
            closure = definition->closure;
        }
        else if (drawingResolver != nullptr)
        {
            drawing = drawingResolver (name);
        }

        if (! drawing.isValid() || drawing.getType().toString() != "drawing")
            fail (path + ": unknown drawing '" + name + "'", element, "drawing");

        if (std::find (expansions.begin(), expansions.end(), drawing) != expansions.end() || (int) expansions.size() >= maxUseDepth)
            fail (path + ": the drawing '" + name + "' ends up using itself", element, "drawing");

        if (closure == nullptr && std::none_of (result.dependencies.begin(), result.dependencies.end(), [&name] (const auto& dependency) { return dependency.first == name; }))
            result.dependencies.push_back ({ name, drawing.clone() });

        StringArray allowed { "id", "drawing", "bounds", "transform", "visible", "opacity", "blend", "color" };
        addPropertyNames (allowed, getInheritablePaint());

        for (const auto& child : drawing)
            if (child.getType().toString() == "property")
                allowed.add (child.getProperty ("name").toString());

        checkElement (element, path, allowed, false);

        auto scope = enterElement (element, path, outerScope);
        inheritPaint (element, path, scope);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::group;
        node->boundsLevel = numLevels++;

        Statements defaults;
        if (! element.hasProperty ("bounds"))
            defaults.push_back ({ "_area = (" + levelSymbol (outerScope.boundsLevel) + ");", path, element });

        compileProperties (*node, element, path, scope, { "bounds:_area", "transform", "visible", "opacity", "blend" }, std::move (defaults));

        Scope body;
        body.lets = scope.lets;
        body.inherited = scope.inherited;

        if (closure != nullptr)
        {
            body.symbols = closure->symbols;
            body.definitions = closure->definitions;
        }

        body = enterLevel (body, node->boundsLevel);
        body.symbols["objectBounds"] = objectBoundsSymbol;

        if (const auto color = scope.symbols.find ("currentColor"); color != scope.symbols.end())
            body.symbols["currentColor"] = color->second;

        const UseSite site { element, path, scope };

        expansions.push_back (drawing);
        compileDrawing (drawing, path + "/" + name, std::move (body), &site, node->children);
        expansions.pop_back();

        return node;
    }

    /* Compiles a pattern where it is defined, and returns where it is in the result. */
    int compilePattern (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, { "id", "name", "tile", "viewBox", "fitting", "justification", "transform" }, true);
        getRequired (element, "tile", path);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::pattern;
        node->boundsLevel = numLevels++;

        compileProperties (*node, element, path, scope, { "tile:_area", "viewBox:_viewBox", "fitting:_fitting", "justification:_justification", "transform" });

        {
            const ScopedValueSetter<Node*> compiling (pattern, node.get());
            compileElements (element, path, enterLevel (scope, node->boundsLevel), Placement::normal, node->children);
        }

        result.patterns.push_back (std::move (node));
        return (int) result.patterns.size() - 1;
    }

    /* Compiles a marker where it is defined, and returns where it is in the result. */
    int compileMarker (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, { "id", "name", "size", "ref", "viewBox", "orient", "units" }, true);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::marker;
        node->boundsLevel = numLevels++;

        StringArray properties { "size:_size", "ref:_reference", "viewBox:_viewBox" };
        Statements defaults;

        if (! element.hasProperty ("size"))
            defaults.push_back ({ "_size = (size (3, 3));", path, element });

        if (! element.hasProperty ("ref"))
            defaults.push_back ({ "_reference = (point (0, 0));", path, element });

        // A marker follows the direction of the path unless it is given an angle
        if (const auto orient = element.getProperty ("orient").toString().trim(); orient == "autoStartReverse")
            node->reversesStart = true;
        else if (orient.isNotEmpty() && orient != "auto")
            properties.add ("orient:_angle");

        if (const auto units = element.getProperty ("units").toString().trim(); units == "userSpace")
            node->scalesWithStrokeWidth = false;
        else if (units.isNotEmpty() && units != "strokeWidth")
            fail (path + ".units: expected 'strokeWidth' or 'userSpace'", element, "units");

        compileProperties (*node, element, path, scope, properties, std::move (defaults));
        compileElements (element, path, enterLevel (scope, node->boundsLevel), Placement::normal, node->children);

        result.markers.push_back (std::move (node));
        return (int) result.markers.size() - 1;
    }

    /* Compiles the shapes a clip is made of: only their geometry is used. */
    std::unique_ptr<Node> compileClipShapes (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, { "id" }, true);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::clipShapes;

        auto inner = scope.enter();
        inner.inherited.clear();

        const ScopedValueSetter<bool> collecting (collectsOutlines, true);
        compileElements (element, path, std::move (inner), Placement::clipShapes, node->children);
        return node;
    }

    std::unique_ptr<Node> compileLayer (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, { "id", "opacity", "blend", "clip", "clipPath", "mask", "maskMode", "visible" }, true);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::layer;

        Statements assignments;
        StringArray uses;

        const auto addLayerProp = [&] (const char* propName, const char* outputName, bool wrapMaskPath = false)
        {
            const Identifier propId (propName);
            if (! element.hasProperty (propId))
                return;

            const auto value = parseValue (element, propId, path, scope);
            const auto rhs = wrapMaskPath ? ("_toMaskPath (" + value.text + ")") : value.text;
            assignments.push_back ({ String (outputName) + " = " + rhs + ";", path + "." + propName, element, propId });
            uses.addArray (value.uses);
        };

        addLayerProp ("opacity", "opacity");
        addLayerProp ("blend", "blend");
        addLayerProp ("clip", "clip");
        addLayerProp ("clipPath", "clipPath");
        addLayerProp ("mask", "_maskPath", true);
        addLayerProp ("maskMode", "_maskMode");
        addLayerProp ("visible", "visible");

        if (! assignments.empty())
        {
            auto statements = selectLets (scope, uses);
            statements.insert (statements.end(), assignments.begin(), assignments.end());

            node->program = compileStatements (statements, path, element);
            node->visible = findOutput<bool> (node->program, "visible");
            node->opacity = findOutput<float> (node->program, "opacity");
            node->blend = findOutput<BlendMode> (node->program, "blend");
            node->clip = findOutput<Rectangle<float>> (node->program, "clip");
            node->clipPath = findOutput<Path> (node->program, "clipPath");
            node->maskPath = findOutput<Path> (node->program, "_maskPath");
            node->maskMode = findOutput<LayerMaskMode> (node->program, "_maskMode");
            addSlot (*node, false);
        }

        // What a filtered layer draws is kept, like the tile of a pattern, until what its elements read around it changes
        const bool isFiltered = element.getChildWithName ("filter").isValid();
        auto* const outerPattern = pattern;

        if (isFiltered)
            node->boundsLevel = numLevels;

        {
            const ScopedValueSetter<Node*> compiling (pattern, isFiltered ? node.get() : pattern);
            compileElements (element, path, scope.enter(), Placement::normal, node->children, { &node->masks, &node->clipShapes, &node->filter });
        }

        if (isFiltered && ! node->isStatic && outerPattern != nullptr)
            outerPattern->isStatic = false;

        if (isFiltered && (! node->masks.empty() || element.hasProperty ("mask")))
            fail (path + ": a 'layer' with a 'filter' can't be masked, put it inside the masked 'layer'", element);

        return node;
    }

    /* Compiles the filter of a layer: where it is rendered, and its primitives in the order they run. */
    std::unique_ptr<Node> compileFilter (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, { "id", "bounds", "linearColors" }, true);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::filter;
        node->linearColors = parseFlag (element, "linearColors", path);

        Statements defaults;
        if (! element.hasProperty ("bounds"))
            defaults.push_back ({ "_area = (" + levelSymbol (scope.boundsLevel) + ");", path, element });

        compileProperties (*node, element, path, scope, { "bounds:_area" }, std::move (defaults));

        std::map<String, int> counts;

        for (const auto& child : element)
        {
            const auto type = child.getType().toString();
            node->children.push_back (compileFilterPrimitive (child, elementPath (path, child, counts[type]++), scope));
        }

        if (node->children.empty())
            fail (path + ": a 'filter' needs at least one primitive", element);

        return node;
    }

    std::unique_ptr<Node> compileFilterPrimitive (const DataTree& element, const String& path, const Scope& scope)
    {
        using Type = FilterGraph::Type;

        static const std::map<String, std::pair<Type, StringArray>> kinds {
            { "blur", { Type::blur, { "deviation" } } },
            { "offset", { Type::offset, { "offset" } } },
            { "flood", { Type::flood, { "color" } } },
            { "colorMatrix", { Type::colorMatrix, { "saturate", "hueRotate", "luminanceToAlpha", "matrix" } } },
            { "composite", { Type::composite, { "in2", "operator", "k1", "k2", "k3", "k4" } } },
            { "blend", { Type::blend, { "in2", "mode" } } },
            { "morphology", { Type::morphology, { "radius", "dilate" } } },
            { "lighting", { Type::lighting, { "scale", "constant", "specular", "exponent", "color", "azimuth", "elevation", "position", "height" } } },
            { "displacement", { Type::displacement, { "in2", "scale", "xChannel", "yChannel" } } },
            { "dropShadow", { Type::dropShadow, { "offset", "deviation", "color" } } }
        };

        const auto kind = kinds.find (element.getType().toString());
        if (kind == kinds.end())
            fail (path + ": a 'filter' is made of blur, offset, flood, colorMatrix, composite, blend, morphology, lighting, displacement and dropShadow", element);

        StringArray allowed { "id", "in", "result" };
        allowed.addArray (kind->second.second);
        checkElement (element, path, allowed, false);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::filterPrimitive;
        node->filterPrimitive = std::make_unique<Node::FilterPrimitive>();

        auto& primitive = *node->filterPrimitive;
        auto& base = primitive.base;

        // What a primitive reads is named: the source of the layer, its alpha, or the result of a primitive before it
        const auto getName = [&element] (const Identifier& property)
        {
            const auto name = element.getProperty (property).toString().trim();
            return name == "source" ? String (FilterGraph::sourceGraphic) : (name == "sourceAlpha" ? String (FilterGraph::sourceAlpha) : name);
        };

        const auto getChoice = [&] (const Identifier& property, const StringArray& choices, int fallback)
        {
            if (! element.hasProperty (property))
                return fallback;

            const int index = choices.indexOf (element.getProperty (property).toString().trim());
            if (index < 0)
                fail (path + "." + property.toString() + ": expected " + choices.joinIntoString (", "), element, property);

            return index;
        };

        base.type = kind->second.first;
        base.in = getName ("in");
        base.in2 = getName ("in2");
        base.result = getName ("result");
        base.dilate = parseFlag (element, "dilate", path);
        base.specular = parseFlag (element, "specular", path);
        base.compositeOperator = static_cast<FilterGraph::CompositeOperator> (getChoice ("operator", { "over", "in", "out", "atop", "xor", "lighter", "arithmetic" }, 0));
        base.xChannel = static_cast<FilterGraph::Channel> (getChoice ("xChannel", { "red", "green", "blue", "alpha" }, 3));
        base.yChannel = static_cast<FilterGraph::Channel> (getChoice ("yChannel", { "red", "green", "blue", "alpha" }, 3));
        base.color = base.type == Type::lighting ? Colors::white : Colors::black;

        if (element.hasProperty ("position"))
            base.light = FilterGraph::Light::point;

        if (parseFlag (element, "luminanceToAlpha", path))
            base.matrix = FilterGraph::luminanceToAlphaMatrix();

        if (element.hasProperty ("matrix"))
        {
            auto values = StringArray::fromTokens (element.getProperty ("matrix").toString().trim().unquoted(), " ,", "");
            values.removeEmptyStrings();

            if (values.size() != static_cast<int> (base.matrix.size()))
                fail (path + ".matrix: expected a text of 20 numbers, the rows of red, green, blue and alpha", element, "matrix");

            for (int i = 0; i < values.size(); ++i)
                base.matrix[static_cast<size_t> (i)] = values[i].getFloatValue();
        }

        // The other properties are expressions: numbers, but for the two points, the color and the blend mode
        Statements assignments;
        StringArray uses, given;

        for (const auto* name : { "deviation", "radius", "scale", "constant", "exponent", "azimuth", "elevation", "height", "saturate", "hueRotate", "k1", "k2", "k3", "k4", "offset", "position", "color", "mode" })
        {
            const Identifier propId (name);
            if (! element.hasProperty (propId))
                continue;

            const String property (name);
            const auto value = parseValue (element, propId, path, scope);
            const bool isNumber = property != "offset" && property != "position" && property != "color" && property != "mode";

            assignments.push_back ({ (property == "mode" ? String ("blend") : "_" + property) + " = " + (isNumber ? "float (" + value.text + ")" : value.text) + ";",
                                     path + "." + property,
                                     element,
                                     propId });
            uses.addArray (value.uses);
            given.add (property);
        }

        if (assignments.empty())
            return node;

        auto statements = selectLets (scope, uses);
        statements.insert (statements.end(), assignments.begin(), assignments.end());
        node->program = compileStatements (statements, path, element);

        const auto number = [&node] (const char* name)
        {
            return findOutput<float> (node->program, "_" + String (name));
        };

        primitive.deviation = number ("deviation");
        primitive.radius = number ("radius");
        primitive.scale = number ("scale");
        primitive.constant = number ("constant");
        primitive.exponent = number ("exponent");
        primitive.azimuth = number ("azimuth");
        primitive.elevation = number ("elevation");
        primitive.height = number ("height");
        primitive.saturate = number ("saturate");
        primitive.hueRotate = number ("hueRotate");
        primitive.coefficients = { number ("k1"), number ("k2"), number ("k3"), number ("k4") };
        primitive.offset = findOutput<Point<float>> (node->program, "_offset");
        primitive.position = findOutput<Point<float>> (node->program, "_position");
        primitive.color = findOutput<Color> (node->program, "_color");
        primitive.mode = findOutput<BlendMode> (node->program, "blend");

        // A value of another type leaves its output not found
        const auto expect = [&] (const char* property, bool wasFound, const char* type)
        {
            if (given.contains (property) && ! wasFound)
                fail (path + "." + property + ": expected a " + type, element, Identifier (property));
        };

        expect ("offset", primitive.offset.isValid(), "point");
        expect ("position", primitive.position.isValid(), "point");
        expect ("color", primitive.color.isValid(), "color");
        expect ("mode", primitive.mode.isValid(), "blend mode");

        addSlot (*node, false);
        return node;
    }

    /* Reads a property that is either `true` or `false`, false when it is not set. */
    static bool parseFlag (const DataTree& element, const Identifier& property, const String& path)
    {
        if (! element.hasProperty (property))
            return false;

        const auto value = element.getProperty (property).toString().trim();
        if (value != "true" && value != "false")
            fail (path + "." + property.toString() + ": expected true or false", element, property);

        return value == "true";
    }

    std::unique_ptr<Node> compileClip (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, { "id", "bounds", "rect", "path", "clipPath", "visible" }, true);

        if (element.hasProperty ("bounds") && element.hasProperty ("rect"))
            fail (path + ": specify either 'bounds' or 'rect', not both", element, "rect");

        if (element.hasProperty ("path") && element.hasProperty ("clipPath"))
            fail (path + ": specify either 'path' or 'clipPath', not both", element, "clipPath");

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::clip;

        Statements assignments;
        StringArray uses;

        const auto addClipProp = [&] (const char* propName, const char* outputName)
        {
            const Identifier propId (propName);
            if (! element.hasProperty (propId))
                return;

            const auto value = parseValue (element, propId, path, scope);
            assignments.push_back ({ String (outputName) + " = " + value.text + ";", path + "." + propName, element, propId });
            uses.addArray (value.uses);
        };

        addClipProp ("bounds", "clip");
        addClipProp ("rect", "clip");
        addClipProp ("path", "clipPath");
        addClipProp ("clipPath", "clipPath");
        addClipProp ("visible", "visible");

        if (! assignments.empty())
        {
            auto statements = selectLets (scope, uses);
            statements.insert (statements.end(), assignments.begin(), assignments.end());

            node->program = compileStatements (statements, path, element);
            node->visible = findOutput<bool> (node->program, "visible");
            node->clip = findOutput<Rectangle<float>> (node->program, "clip");
            node->clipPath = findOutput<Path> (node->program, "clipPath");
            addSlot (*node, false);
        }

        compileElements (element, path, scope.enter(), Placement::normal, node->children, { nullptr, &node->clipShapes });
        return node;
    }

    std::unique_ptr<Node> compileMask (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, { "id", "mode", "maskMode", "visible" }, true);

        if (element.hasProperty ("mode") && element.hasProperty ("maskMode"))
            fail (path + ": specify either 'mode' or 'maskMode', not both", element, "maskMode");

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::mask;

        Statements assignments;
        StringArray uses;

        for (const auto* modeProp : { "mode", "maskMode" })
        {
            const Identifier propId (modeProp);
            if (! element.hasProperty (propId))
                continue;

            const auto value = parseValue (element, propId, path, scope);
            assignments.push_back ({ "_maskMode = " + value.text + ";", path + "." + modeProp, element, propId });
            uses.addArray (value.uses);
        }

        if (element.hasProperty ("visible"))
        {
            const Identifier propId ("visible");
            const auto value = parseValue (element, propId, path, scope);
            assignments.push_back ({ "visible = " + value.text + ";", path + ".visible", element, propId });
            uses.addArray (value.uses);
        }

        if (! assignments.empty())
        {
            auto statements = selectLets (scope, uses);
            statements.insert (statements.end(), assignments.begin(), assignments.end());

            node->program = compileStatements (statements, path, element);
            node->visible = findOutput<bool> (node->program, "visible");
            node->maskMode = findOutput<LayerMaskMode> (node->program, "_maskMode");
            addSlot (*node, false);
        }

        compileElements (element, path, scope.enter(), Placement::normal, node->children);
        return node;
    }

    std::unique_ptr<Node> compileMedia (const DataTree& element, const String& path, const Scope& scope, bool repeated, Node::Kind kind)
    {
        StringArray allowed { "id", "bounds", "source", "fitting", "justification", "opacity", "blend", "tint", "transform", "clip", "clipPath", "visible" };
        if (kind == Node::Kind::svg)
            allowed.add ("data");

        checkElement (element, path, allowed, false);

        Identifier sourceProp ("source");
        bool inlineOnly = false;

        if (kind == Node::Kind::svg)
        {
            const bool hasSource = element.hasProperty ("source");
            const bool hasData = element.hasProperty ("data");

            if (! hasSource && ! hasData)
                fail (path + ": missing the required property 'source'", element);

            if (hasSource && hasData)
                fail (path + ": specify either 'source' or 'data', not both", element, "data");

            if (hasData)
            {
                sourceProp = Identifier ("data");
                inlineOnly = true;
            }
        }
        else if (! element.hasProperty ("source"))
        {
            fail (path + ": missing the required property 'source'", element);
        }

        Statements own;
        StringArray uses;

        if (element.hasProperty ("bounds"))
        {
            const auto value = parseValue (element, "bounds", path, scope);
            own.push_back ({ "_area = " + value.text + ";", path + ".bounds", element, "bounds" });
            uses.addArray (value.uses);
        }
        else
        {
            own.push_back ({ "_area = (" + levelSymbol (scope.boundsLevel) + ");", path, element });
        }

        {
            const auto value = parseValue (element, sourceProp, path, scope);
            own.push_back ({ "_source = " + value.text + ";", path + "." + sourceProp.toString(), element, sourceProp });
            uses.addArray (value.uses);
        }

        const auto addOptionalMediaProp = [&] (const char* propName, const char* outputName)
        {
            const Identifier propId (propName);
            if (! element.hasProperty (propId))
                return;

            const auto value = parseValue (element, propId, path, scope);
            own.push_back ({ String (outputName) + " = " + value.text + ";", path + "." + propName, element, propId });
            uses.addArray (value.uses);
        };

        addOptionalMediaProp ("fitting", "_fitting");
        addOptionalMediaProp ("justification", "_justification");
        addOptionalMediaProp ("opacity", "opacity");
        addOptionalMediaProp ("blend", "blend");
        addOptionalMediaProp ("tint", "tint");
        addOptionalMediaProp ("transform", "transform");
        addOptionalMediaProp ("clip", "clip");
        addOptionalMediaProp ("clipPath", "clipPath");
        addOptionalMediaProp ("visible", "visible");

        auto statements = selectLets (scope, uses);
        statements.insert (statements.end(), own.begin(), own.end());

        auto node = std::make_unique<Node>();
        node->kind = kind;
        node->svgInlineOnly = inlineOnly;
        node->program = compileStatements (statements, path, element);
        node->area = findOutput<Rectangle<float>> (node->program, "_area");
        node->source = findOutput<String> (node->program, "_source");
        node->fitting = findOutput<Fitting> (node->program, "_fitting");
        node->justification = findOutput<Justification::Flags> (node->program, "_justification");
        node->opacity = findOutput<float> (node->program, "opacity");
        node->blend = findOutput<BlendMode> (node->program, "blend");
        node->tint = findOutput<Color> (node->program, "tint");
        node->transform = findOutput<AffineTransform> (node->program, "transform");
        node->clip = findOutput<Rectangle<float>> (node->program, "clip");
        node->clipPath = findOutput<Path> (node->program, "clipPath");
        node->visible = findOutput<bool> (node->program, "visible");
        addSlot (*node, repeated);
        return node;
    }

    std::unique_ptr<Node> compileRepeat (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, { "id", "name", "from", "to" }, true);

        const auto name = parseName (element, path);
        const auto from = parseValue (element, "from", path, scope);
        const auto to = parseValue (element, "to", path, scope);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::repeat;

        auto uses = from.uses;
        uses.addArray (to.uses);

        auto statements = selectLets (scope, uses);
        statements.push_back ({ "_start = int " + from.text + ";", path + ".from", element, "from" });
        statements.push_back ({ "_end = int " + to.text + ";", path + ".to", element, "to" });

        StringArray referencedSymbols;
        node->program = compileStatements (statements, path, element, {}, &referencedSymbols);
        node->start = findOutput<int> (node->program, "_start");
        node->end = findOutput<int> (node->program, "_end");

        const bool hasRuntimeInputs = std::any_of (referencedSymbols.begin(), referencedSymbols.end(), [&] (const String& symbol)
        {
            return node->program.findInput (symbol) >= 0;
        });

        if (! hasRuntimeInputs)
        {
            ExpressionState constantState (node->program, 1);
            constantState.evaluate();
            const auto count = static_cast<int64> (constantState.get (node->end)) - static_cast<int64> (constantState.get (node->start));

            if (count > maxRepeatCount)
                fail (path + ": repeat count (" + String (count) + ") exceeds the maximum of " + String (maxRepeatCount), element, "to");
        }

        addSlot (*node, false);

        auto inner = scope.enter();
        declare (inner, name, { "int " + from.text + " + _index", from.uses }, path, path + ".from", element, "from");
        compileElements (element, path, std::move (inner), Placement::repeat, node->children);
        return node;
    }

    std::unique_ptr<Node> compileShape (const DataTree& element, const String& path, const Scope& outerScope, bool repeated)
    {
        const auto type = element.getType().toString();
        const bool isText = type == "text";
        const auto* shape = findShape (type, isText && element.hasProperty ("anchor"));

        if (shape == nullptr)
            fail (path + ": unknown element '" + type + "'", element);

        if (isText && collectsOutlines)
            fail (path + ": a 'text' has no outline to clip to", element);

        if (isText && element.hasProperty ("anchor") && element.hasProperty ("bounds"))
            fail (path + ": specify either 'bounds' or 'anchor', not both", element, "anchor");

        StringArray allowed { "id", "color" };
        for (const auto& entry : shape->geometry)
            allowed.add (entry.upToFirstOccurrenceOf ("=", false, false));

        addPropertyNames (allowed, shape->paint);
        addPropertyNames (allowed, getCommonPaint());

        if (shape->acceptsMarkers)
            allowed.addArray ({ "markerStart", "markerMid", "markerEnd" });

        checkElement (element, path, allowed, false);

        std::optional<Scope> colored;
        const auto& scope = element.hasProperty ("color") ? colored.emplace (enterElement (element, path, outerScope)) : outerScope;

        Statements own, paint;
        StringArray geometry, uses;

        for (const auto& entry : shape->geometry)
        {
            const auto property = entry.upToFirstOccurrenceOf ("=", false, false);
            const Identifier propId (property);
            const auto fallback = entry.fromFirstOccurrenceOf ("=", false, false);
            const bool isExplicit = fallback.isEmpty() || element.hasProperty (propId);
            const auto value = isExplicit ? parseValue (element, propId, path, scope) : Value { "(" + fallback + ")", {} };

            geometry.add (property);
            own.push_back ({ "let _" + property + " = " + value.text + ";", path + "." + property, element, isExplicit ? propId : Identifier {} });
            uses.addArray (value.uses);
        }

        // The paint of the shape itself comes first, then the paint the groups around it set and it doesn't
        auto inherited = scope.inherited;
        std::array<int, 3> markers { -1, -1, -1 };
        bool hasFill = false, hasStroke = false;

        const auto addPaint = [&] (const String& property, const String& output, const String& value, const DataTree& origin, const Identifier& originProperty)
        {
            paint.push_back ({ output + " = " + value + ";", path + "." + property, origin, originProperty });
            hasFill = hasFill || isFillPaint (property);
            hasStroke = hasStroke || (isText ? output == "labelStroke" : isStrokePaint (property));
        };

        const auto findOutputOf = [&] (const String& property)
        {
            const auto output = findPaintOutput (shape->paint, property);
            return output.isNotEmpty() ? output : findPaintOutput (getCommonPaint(), property);
        };

        for (int i = 0; i < element.getNumProperties(); ++i)
        {
            const auto propId = element.getPropertyName (i);
            const auto property = propId.toString();
            if (property == "id" || property == "color" || geometry.contains (property))
                continue;

            if (isFillPaint (property))
                eraseInherited (inherited, isFillPaint);
            else if (isStrokePaint (property))
                eraseInherited (inherited, isStrokePaint);
            else
                inherited.erase (property);

            if (const auto marker = getMarkerIndex (property); marker >= 0)
            {
                if (repeated)
                    fail (path + "." + property + ": a shape inside a repeat can't have markers", element, propId);

                if (! isNone (element, propId))
                    markers[(size_t) marker] = findDefinitionIndex (element, propId, path, scope);

                continue;
            }

            if (isNone (element, propId) && (property == "fill" || property == "stroke"))
            {
                if (property == "fill")
                    inherited[property].isNone = true;

                continue;
            }

            if (isPatternProperty (property))
            {
                const auto symbol = patternPrefix + String (findDefinitionIndex (element, propId, path, scope));
                addPaint (property, property, symbol, element, propId);
                uses.add (symbol);
                continue;
            }

            const auto output = findOutputOf (property);
            const auto value = parseValue (element, propId, path, scope, true);

            addPaint (property, output, wrapForOutput (output, value.text), element, propId);
            uses.addArray (value.uses);
        }

        bool fillIsNone = false;

        for (const auto& entry : getInheritablePaint())
        {
            const auto property = entry.upToFirstOccurrenceOf (":", false, false);
            const auto found = inherited.find (property);

            if (found == inherited.end())
                continue;

            if (found->second.isNone)
            {
                fillIsNone = true;
                continue;
            }

            if (const auto marker = getMarkerIndex (property); marker >= 0)
            {
                if (shape->acceptsMarkers && ! repeated && ! collectsOutlines)
                    markers[(size_t) marker] = found->second.definition;

                continue;
            }

            // Text is only stroked with a color
            const auto output = findOutputOf (property);
            if (output.isEmpty() || (isText && isStrokePaint (property) && property != "stroke"))
                continue;

            if (isPatternProperty (property))
            {
                const auto symbol = patternPrefix + String (found->second.definition);
                addPaint (property, output, symbol, element, {});
                uses.add (symbol);
                continue;
            }

            addPaint (property, output, wrapForOutput (output, found->second.value.text), element, {});
            uses.addArray (found->second.value.uses);
        }

        // A shape without a paint is filled with the color of the Graphics, so no fill at all has to be one drawing nothing
        if (fillIsNone && ! hasFill && ! hasStroke)
            paint.push_back ({ "fill = #00000000;", path, element });

        const bool usesObjectBounds = uses.contains (objectBoundsSymbol);

        if (usesObjectBounds && shape->objectBounds.isEmpty() && shape->shapePath.isEmpty())
            fail (path + ": 'objectBounds' is not available to a text placed from an 'anchor'", element);

        if (shape->shapePath.isEmpty())
        {
            if (usesObjectBounds)
                own.push_back ({ String ("let ") + objectBoundsSymbol + " = " + shape->objectBounds + ";", path, element });

            own.push_back ({ shape->lowering, path, element });
        }
        else if (usesObjectBounds)
        {
            own.push_back ({ "let _shape = " + shape->shapePath + "; let " + objectBoundsSymbol + " = _shape.bounds; path = _shape;", path, element });
        }
        else
        {
            own.push_back ({ "path = " + shape->shapePath + ";", path, element });
        }

        auto statements = selectLets (scope, uses);
        statements.insert (statements.end(), own.begin(), own.end());
        statements.insert (statements.end(), paint.begin(), paint.end());

        auto created = ExpressionDrawing::create (compileStatements (statements, path, element));
        if (created.failed())
            fail (path + ": " + created.getErrorMessage(), element);

        auto node = std::make_unique<Node>();
        node->drawing = std::move (created.getReference());
        node->markers = markers;

        if (node->hasMarkers())
        {
            const auto& program = node->drawing.getProgram();
            node->visible = findOutput<bool> (program, "visible");
            node->opacity = findOutput<float> (program, "opacity");
            node->strokeWidth = findOutput<float> (program, "strokeWidth");
            node->transform = findOutput<AffineTransform> (program, "transform");
            node->clip = findOutput<Rectangle<float>> (program, "clip");
            node->clipPath = findOutput<Path> (program, "clipPath");
        }

        addSlot (*node, repeated);
        return node;
    }

    ExpressionContext& context;
    const DrawingScope& drawingScope;
    const DrawingResolver& drawingResolver;
    Compiled& result;
    std::map<String, PropertyInput> propertyInputs;
    std::vector<DataTree> expansions;
    Node* pattern = nullptr;
    bool collectsOutlines = false;
    int numSymbols = 0;
    int numLevels = 1;
};

//==============================================================================
namespace
{

struct DrawingBaseContext
{
    DrawingBaseContext()
    {
        ExpressionDrawing::declareOutputs (context);
        DrawingScope::addExpressionArrayTypes (context);
        bounds = context.addInput<Rectangle<float>> ("bounds");
        font = context.addInput<Font> ("defaultFont");
        index = context.addInput<int> ("_index");
        context.declareOutput<int> ("_start");
        context.declareOutput<int> ("_end");
        context.declareOutput<Rectangle<float>> ("_area");
        context.declareOutput<String> ("_source");
        context.declareOutput<Fitting> ("_fitting");
        context.declareOutput<Justification::Flags> ("_justification");
        context.declareOutput<Path> ("_maskPath");
        context.declareOutput<LayerMaskMode> ("_maskMode");
        context.declareOutput<Rectangle<float>> ("_viewBox");
        context.declareOutput<Size<float>> ("_size");
        context.declareOutput<Point<float>> ("_reference");
        context.declareOutput<float> ("_angle");
        context.declareOutput<Color> ("_color");
        context.addFunction ("_toMaskPath", [] (const Path& p) { return p; });
        context.addFunction ("_toMaskPath", [] (const Rectangle<float>& r)
        {
            Path p;
            p.addRectangle (r);
            return p;
        });
    }

    ExpressionContext context;
    ExpressionInput<Rectangle<float>> bounds;
    ExpressionInput<Font> font;
    ExpressionInput<int> index;
};

const DrawingBaseContext& getDrawingBaseContext()
{
    static const DrawingBaseContext base;
    return base;
}

} // namespace

//==============================================================================
/* The compilations in use, by drawing content. Weak entries, so a compilation goes away with its last style. */
class DrawingStyle::CompiledCache
{
public:
    static CompiledCache& getInstance()
    {
        static CompiledCache cache;
        return cache;
    }

    /* Returns a compilation whose symbols resolve in the scope to the types they had when compiled, and
       whose drawings found by name are still the same, if any. */
    std::shared_ptr<const Compiled> find (const DataTree& drawing, const DrawingScope& scope, const DrawingResolver& drawingResolver)
    {
        for (auto it = entries.begin(); it != entries.end();)
        {
            auto candidate = it->compiled.lock();
            if (candidate == nullptr)
            {
                it = entries.erase (it);
                continue;
            }

            if (it->drawing.isEquivalentTo (drawing)
                && std::all_of (candidate->symbols.begin(), candidate->symbols.end(), [&] (const auto& symbol)
                {
                    return scope.resolveSymbol (symbol.first) == symbol.second;
                })
                && std::all_of (candidate->dependencies.begin(), candidate->dependencies.end(), [&] (const auto& dependency)
                {
                    return drawingResolver != nullptr && drawingResolver (dependency.first).isEquivalentTo (dependency.second);
                }))
            {
                return candidate;
            }

            ++it;
        }

        return nullptr;
    }

    void add (const DataTree& drawing, const std::shared_ptr<const Compiled>& compiled)
    {
        entries.erase (std::remove_if (entries.begin(), entries.end(), [] (const auto& entry)
        {
            return entry.compiled.expired();
        }),
                       entries.end());

        entries.push_back ({ drawing.clone(), compiled });
    }

private:
    struct Entry
    {
        DataTree drawing;
        std::weak_ptr<const Compiled> compiled;
    };

    std::vector<Entry> entries;
};

//==============================================================================
DrawingStyle::DrawingStyle (Component& target, DrawingScope::ComponentResolver componentResolver)
    : targetComponent (&target)
    , context (getDrawingBaseContext().context)
    , scope (target, context)
{
    scope.setComponentResolver (std::move (componentResolver));
}

DrawingStyle::~DrawingStyle() = default;

ResultValue<DrawingStyle::Ptr> DrawingStyle::create (Component& target,
                                                     const DataTree& drawing,
                                                     DrawingScope::ComponentResolver componentResolver,
                                                     YuiDiagnostic* failure,
                                                     const DrawingResolver& drawingResolver)
{
    DrawingStyle::Ptr style (new DrawingStyle (target, std::move (componentResolver)));

    auto& cache = CompiledCache::getInstance();
    auto compiled = cache.find (drawing, style->scope, drawingResolver);

    if (compiled == nullptr)
    {
        auto result = std::make_shared<Compiled>();

        if (const auto compiledResult = Compiler (style->context, style->scope, drawingResolver, *result).compile (drawing, failure); compiledResult.failed())
            return makeResultValueFail (compiledResult.getErrorMessage());

        cache.add (drawing, result);
        compiled = std::move (result);
    }

    style->instantiate (std::move (compiled));
    return makeResultValueOk (std::move (style));
}

void DrawingStyle::instantiate (std::shared_ptr<const Compiled> compiledToUse)
{
    compiled = std::move (compiledToUse);
    slots = std::make_unique<Slot[]> (compiled->evaluated.size());
    propertyValues.resize (compiled->properties.size());
    propertyArrays.resize (compiled->properties.size());
    levelBounds.resize ((size_t) compiled->numLevels);

    for (const auto& pattern : compiled->patterns)
        patternSources.push_back (std::make_unique<PatternSource> (*this, *pattern));

    for (size_t i = 0; i < compiled->evaluated.size(); ++i)
    {
        const auto& node = *compiled->evaluated[i];
        auto& slot = slots[i];

        slot.state = std::make_unique<ExpressionState> (node.getProgram(), node.repeated ? 64 : 1);
        slot.inputs = scope.bindInputs (node.getProgram());

        if (node.repeated)
            slot.state->bindRowIndex (getDrawingBaseContext().index);

        for (const auto& dependency : scope.getDependencies (node.getProgram()))
        {
            if (dependency.component == nullptr || dependency.component == targetComponent.get())
                continue;

            const auto isKnown = std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& known)
            {
                return known.component == dependency.component && known.property == dependency.property;
            });

            if (! isKnown)
                dependencies.push_back (dependency);
        }
    }

    if (! dependencies.empty())
    {
        observer = std::make_unique<DependencyObserver>();
        observer->onChanged = [this] (Component& component, const Identifier& property)
        {
            auto* target = targetComponent.get();
            if (target == nullptr)
                return;

            const bool matches = std::any_of (dependencies.begin(), dependencies.end(), [&] (const auto& dependency)
            {
                return dependency.component == &component && dependency.property == property;
            });

            if (matches)
                target->repaint();
        };
        observer->onDeleted = [this] (Component& component)
        {
            for (auto& dependency : dependencies)
                if (dependency.component == &component)
                    dependency.component = nullptr;
        };
        observer->observe (dependencies);
    }
}

const std::vector<ComponentProperty>& DrawingStyle::getProperties() const noexcept
{
    return compiled->properties;
}

void DrawingStyle::setImageResolver (ImageResolver newResolver)
{
    imageResolver = std::move (newResolver);

    // The images found so far, and what was drawn with them, may not be what the resolver gives
    for (size_t i = 0; i < compiled->evaluated.size(); ++i)
        slots[i].imageCache.clear();

    for (auto& source : patternSources)
        source->markDirty();

    for (auto& [node, source] : filterSources)
        source->markDirty();

    if (auto* target = targetComponent.get())
        target->repaint();
}

void DrawingStyle::paint (Graphics& g, const ApplicationTheme& theme, const Component& component)
{
    for (size_t i = 0; i < propertyValues.size(); ++i)
        propertyValues[i] = component.findProperty (compiled->properties[i].name);

    const auto font = theme.getDefaultFont();
    const auto bounds = component.getLocalBounds();

    ++paintSerial;
    levelBounds.front() = bounds;

    const ScopedValueSetter<const Font*> painting (paintFont, std::addressof (font));
    drawNodes (g, compiled->nodes, { font, bounds, g.getTransform() }, 1);
}

DrawingStyle::Slot* DrawingStyle::prepare (const Node& node, const PaintContext& context)
{
    if (node.slot < 0)
        return nullptr;

    auto& slot = slots[(size_t) node.slot];
    const auto& base = getDrawingBaseContext();

    slot.state->set (base.bounds, levelBounds.front());
    slot.state->set (base.font, context.font);
    scope.updateInputs (slot.inputs, *slot.state);

    for (const auto& input : node.levels)
        slot.state->set (ExpressionInput<Rectangle<float>> { input.input }, levelBounds[(size_t) input.index]);

    for (const auto& input : node.patterns)
        slot.state->set (ExpressionInput<PatternPaint> { input.input }, preparePattern (input.index, context).paint);

    for (const auto& input : node.properties)
    {
        const auto& value = propertyValues[(size_t) input.property];
        const auto& property = compiled->properties[(size_t) input.property];

        if (input.isSet)
            slot.state->set (ExpressionInput<bool> { input.input }, ! value.isVoid());
        else if (! value.isVoid() || property.type == ComponentProperty::Type::array)
            DrawingScope::setInputValue (*slot.state, input.input, property.type, property.elementType, value, propertyArrays[(size_t) input.property]);
    }

    return &slot;
}

/* Evaluates a pattern once per paint: where its tiles are, and whether the tile has to be drawn again. */
DrawingStyle::PatternSource& DrawingStyle::preparePattern (int index, const PaintContext& context)
{
    auto& source = *patternSources[(size_t) index];

    if (source.preparedSerial == paintSerial)
        return source;

    source.preparedSerial = paintSerial;

    const auto& node = source.node;
    auto& state = *prepare (node, context)->state;
    state.evaluate();

    const auto tile = state.get (node.area);
    const auto contentBounds = node.viewBox.isValid() ? state.get (node.viewBox) : tile.withZeroPosition();
    AffineTransform contentTransform;

    if (node.viewBox.isValid())
    {
        contentTransform = contentBounds.getTransformToFit (tile.withZeroPosition(),
                                                           node.fitting.isValid() ? state.get (node.fitting) : Fitting::scaleToFit,
                                                           Justification (node.justification.isValid() ? state.get (node.justification) : Justification::center));
    }

    if (! node.isStatic || tile != source.tile || contentBounds != source.contentBounds || contentTransform != source.contentTransform)
        source.markDirty();

    source.tile = tile;
    source.contentBounds = contentBounds;
    source.contentTransform = contentTransform;
    source.paint = { std::addressof (source),
                     AffineTransform::translation (tile.getX(), tile.getY())
                         .followedBy (node.transform.isValid() ? state.get (node.transform) : AffineTransform()) };

    return source;
}

void DrawingStyle::drawPattern (Graphics& g, const PatternSource& source)
{
    if (paintFont == nullptr)
        return;

    const auto savedState = g.saveState();
    const PaintContext context { *paintFont, source.tile.withZeroPosition(), g.getTransform() };

    g.setTransform (source.contentTransform.followedBy (g.getTransform()));
    levelBounds[(size_t) source.node.boundsLevel] = source.contentBounds;

    drawNodes (g, source.node.children, context, 1);
}

/* Draws the markers of a shape at its vertices, with the transform, the clip and the opacity of the shape. */
void DrawingStyle::drawMarkers (Graphics& g, const Node& node, Slot& slot, const PaintContext& context)
{
    auto& state = *slot.state;
    state.evaluate();

    if (node.visible.isValid() && ! state.get (node.visible))
        return;

    const PathMarkers placements (node.drawing.getOutline (state, 1, false));
    const std::array<Span<const PathMarkers::Placement>, 3> vertices { placements.getStarts(), placements.getMids(), placements.getEnds() };

    const auto savedState = g.saveState();
    const auto strokeWidth = node.strokeWidth.isValid() ? state.get (node.strokeWidth) : g.getStrokeWidth();

    if (node.clip.isValid())
        g.setClipPath (state.get (node.clip));

    if (node.clipPath.isValid())
        g.setClipPath (state.get (node.clipPath));

    if (node.transform.isValid())
        g.setTransform (state.get (node.transform).followedBy (g.getTransform()));

    if (node.opacity.isValid())
        g.setOpacity (g.getOpacity() * jlimit (0.0f, 1.0f, state.get (node.opacity)));

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        if (node.markers[i] < 0 || vertices[i].empty())
            continue;

        const auto& marker = *compiled->markers[(size_t) node.markers[i]];
        auto& markerState = *prepare (marker, context)->state;
        markerState.evaluate();

        PathMarkers::Shape shape;
        shape.reference = markerState.get (marker.reference);
        shape.size = markerState.get (marker.size);
        shape.scalesWithStrokeWidth = marker.scalesWithStrokeWidth;

        if (marker.viewBox.isValid())
            shape.viewBox = markerState.get (marker.viewBox);

        if (marker.angle.isValid())
            shape.angle = markerState.get (marker.angle);

        levelBounds[(size_t) marker.boundsLevel] = shape.viewBox.value_or (Rectangle<float> (Point<float>(), shape.size));

        for (auto placement : vertices[i])
        {
            if (i == 0 && marker.reversesStart)
                placement.angle += MathConstants<float>::pi;

            const auto markerSavedState = g.saveState();
            g.setTransform (PathMarkers::getTransform (placement, shape, strokeWidth).followedBy (g.getTransform()));
            drawNodes (g, marker.children, context, 1);
        }
    }
}

/* Returns the geometry of shapes, to clip to. */
Path DrawingStyle::collectOutline (const std::vector<std::unique_ptr<Node>>& shapes, const PaintContext& context, int numRows)
{
    Path outline;

    for (const auto& shape : shapes)
    {
        auto* slot = prepare (*shape, context);

        if (shape->kind == Node::Kind::shape)
        {
            outline.appendPath (shape->drawing.getOutline (*slot->state, numRows));
            continue;
        }

        slot->state->evaluate();

        const auto count = jlimit<int64> (0, maxRepeatCount, (int64) slot->state->get (shape->end) - slot->state->get (shape->start));
        if (count > 0)
            outline.appendPath (collectOutline (shape->children, context, (int) count));
    }

    return outline;
}

void DrawingStyle::applyClips (Graphics& g, const Node& node, Slot* slot, const PaintContext& context, int numRows)
{
    if (slot != nullptr && node.clip.isValid())
        g.setClipPath (slot->state->get (node.clip));

    if (slot != nullptr && node.clipPath.isValid())
        g.setClipPath (slot->state->get (node.clipPath));

    for (const auto& shapes : node.clipShapes)
        g.setClipPath (collectOutline (shapes->children, context, numRows));
}

void DrawingStyle::drawNodes (Graphics& g, const std::vector<std::unique_ptr<Node>>& children, const PaintContext& context, int numRows)
{
    for (const auto& node : children)
        drawNode (g, *node, context, numRows);
}

void DrawingStyle::drawNode (Graphics& g, const Node& node, const PaintContext& context, int numRows)
{
    auto* slot = prepare (node, context);

    if (node.kind == Node::Kind::shape)
    {
        node.drawing.draw (g, *slot->state, numRows, &context.font, &slot->cache);

        if (node.hasMarkers())
            drawMarkers (g, node, *slot, context);

        return;
    }

    if (node.kind == Node::Kind::image || node.kind == Node::Kind::svg)
    {
        File baseDirectory;
        if (const auto* document = YuiDocument::findDocumentFor (targetComponent.get()))
            baseDirectory = document->getBaseDirectory();

        for (int startRow = 0; startRow < numRows;)
        {
            const auto numEvaluated = slot->state->evaluateBlock (startRow, numRows - startRow);
            if (numEvaluated <= 0)
                break;

            const auto* visibleLanes = node.visible.isValid() ? slot->state->getLanes (node.visible).data() : nullptr;
            const auto* areaLanes = slot->state->getLanes (node.area).data();
            const auto* sourceLanes = slot->state->getLanes (node.source).data();
            const auto* fittingLanes = node.fitting.isValid() ? slot->state->getLanes (node.fitting).data() : nullptr;
            const auto* justLanes = node.justification.isValid() ? slot->state->getLanes (node.justification).data() : nullptr;
            const auto* opacityLanes = node.opacity.isValid() ? slot->state->getLanes (node.opacity).data() : nullptr;
            const auto* blendLanes = node.blend.isValid() ? slot->state->getLanes (node.blend).data() : nullptr;
            const auto* tintLanes = node.tint.isValid() ? slot->state->getLanes (node.tint).data() : nullptr;
            const auto* transformLanes = node.transform.isValid() ? slot->state->getLanes (node.transform).data() : nullptr;
            const auto* clipLanes = node.clip.isValid() ? slot->state->getLanes (node.clip).data() : nullptr;
            const auto* clipPathLanes = node.clipPath.isValid() ? slot->state->getLanes (node.clipPath).data() : nullptr;

            for (int lane = 0; lane < numEvaluated; ++lane)
            {
                if (visibleLanes != nullptr && ! visibleLanes[lane])
                    continue;

                const auto targetArea = areaLanes[lane];
                const auto& src = sourceLanes[lane];
                if (targetArea.isEmpty() || src.isEmpty())
                    continue;

                const float opacity = opacityLanes != nullptr ? jlimit (0.0f, 1.0f, opacityLanes[lane]) : 1.0f;
                if (opacity <= 0.0f)
                    continue;

                const auto fitting = fittingLanes != nullptr ? fittingLanes[lane] : Fitting::scaleToFit;
                const Justification justification (justLanes != nullptr ? justLanes[lane] : Justification::center);

                const auto savedState = g.saveState();

                if (clipLanes != nullptr)
                    g.setClipPath (clipLanes[lane]);

                if (clipPathLanes != nullptr)
                    g.setClipPath (clipPathLanes[lane]);

                if (transformLanes != nullptr)
                    g.setTransform (transformLanes[lane].followedBy (g.getTransform()));

                if (opacity < 1.0f)
                    g.setOpacity (g.getOpacity() * opacity);

                if (blendLanes != nullptr)
                    g.setBlendMode (blendLanes[lane]);

                if (tintLanes != nullptr)
                    g.setTint (tintLanes[lane]);

                if (node.kind == Node::Kind::svg)
                {
                    if (const auto drawable = resolveDrawingSvg (slot->svgCache, src, node.svgInlineOnly, baseDirectory))
                        drawable->paint (g, targetArea, fitting, justification);
                }
                else
                {
                    const auto& image = resolveDrawingImage (slot->imageCache, src, baseDirectory, imageResolver);
                    if (! image.isValid())
                        continue;

                    const Rectangle<float> sourceBounds { 0.0f, 0.0f, static_cast<float> (image.getWidth()), static_cast<float> (image.getHeight()) };

                    if (fitting == Fitting::tile)
                    {
                        g.setClipPath (targetArea);

                        for (float y = targetArea.getY(); y < targetArea.getBottom(); y += sourceBounds.getHeight())
                            for (float x = targetArea.getX(); x < targetArea.getRight(); x += sourceBounds.getWidth())
                                g.drawImage (image, { x, y, sourceBounds.getWidth(), sourceBounds.getHeight() });
                    }
                    else
                    {
                        const auto destBounds = sourceBounds.transformed (sourceBounds.getTransformToFit (targetArea, fitting, justification));
                        if (destBounds.isEmpty())
                            continue;

                        if (! targetArea.contains (destBounds))
                            g.setClipPath (targetArea);

                        g.drawImage (image, destBounds);
                    }
                }
            }

            startRow += numEvaluated;
        }

        return;
    }

    if (slot != nullptr)
    {
        slot->state->evaluate();

        if (node.visible.isValid() && ! slot->state->get (node.visible))
            return;
    }

    switch (node.kind)
    {
        case Node::Kind::repeat:
        {
            const auto count = jlimit<int64> (0, maxRepeatCount, (int64) slot->state->get (node.end) - slot->state->get (node.start));

            if (count > 0)
                drawNodes (g, node.children, context, (int) count);

            break;
        }

        case Node::Kind::group:
        {
            const auto opacity = (slot != nullptr && node.opacity.isValid()) ? jlimit (0.0f, 1.0f, slot->state->get (node.opacity)) : 1.0f;

            if (opacity <= 0.0f)
                break;

            const auto savedState = g.saveState();

            if (opacity < 1.0f)
                g.setOpacity (g.getOpacity() * opacity);

            if (slot != nullptr && node.blend.isValid())
                g.setBlendMode (slot->state->get (node.blend));

            if (slot != nullptr && node.transform.isValid())
                g.setTransform (slot->state->get (node.transform).followedBy (g.getTransform()));

            if (node.boundsLevel >= 0)
            {
                // The children are drawn in bounds of their own: the area itself, or a view box mapped to it
                const auto area = slot->state->get (node.area);
                auto childBounds = area;

                if (node.mapsToArea)
                {
                    childBounds = node.viewBox.isValid() ? slot->state->get (node.viewBox) : area.withZeroPosition();

                    if (area.isEmpty() || childBounds.isEmpty())
                        break;

                    if (node.clipsToArea)
                        g.setClipPath (area);

                    const auto mapping = node.viewBox.isValid()
                                           ? childBounds.getTransformToFit (area,
                                                                            node.fitting.isValid() ? slot->state->get (node.fitting) : Fitting::scaleToFit,
                                                                            Justification (node.justification.isValid() ? slot->state->get (node.justification) : Justification::center))
                                           : AffineTransform::translation (area.getX(), area.getY());

                    g.setTransform (mapping.followedBy (g.getTransform()));
                }

                levelBounds[(size_t) node.boundsLevel] = childBounds;
            }

            drawNodes (g, node.children, context, numRows);
            break;
        }

        case Node::Kind::clip:
        {
            const auto savedState = g.saveState();

            applyClips (g, node, slot, context, numRows);
            drawNodes (g, node.children, context, numRows);
            break;
        }

        case Node::Kind::layer:
        {
            const auto opacity = (slot != nullptr && node.opacity.isValid()) ? jlimit (0.0f, 1.0f, slot->state->get (node.opacity)) : 1.0f;

            if (opacity <= 0.0f)
                break;

            const bool hasBlend = slot != nullptr && node.blend.isValid() && slot->state->get (node.blend) != BlendMode::SrcOver;
            const bool hasMaskPath = slot != nullptr && node.maskPath.isValid();
            const bool hasMasks = hasMaskPath || ! node.masks.empty();

            const auto savedState = g.saveState();
            applyClips (g, node, slot, context, numRows);

            if (node.filter != nullptr)
            {
                // What the filter makes of the layer gets its opacity and its blend mode
                const auto filteredState = g.saveState();
                g.setOpacity (g.getOpacity() * opacity);

                if (hasBlend)
                    g.setBlendMode (slot->state->get (node.blend));

                if (drawFilteredLayer (g, node, context, numRows))
                    break;
            }

            if (opacity >= 1.0f && ! hasBlend && ! hasMasks)
            {
                drawNodes (g, node.children, context, numRows);
                break;
            }

            if (hasBlend)
                g.setBlendMode (slot->state->get (node.blend));

            // The layer covers the area of the drawing, where it keeps its resolution, and its
            // children are drawn into it under the transforms of the groups around them
            const auto layerOrigin = AffineTransform::translation (-context.layerArea.getX(), -context.layerArea.getY());
            const auto childTransform = g.getTransform().followedBy (context.layerTransform.inverted()).followedBy (layerOrigin);
            const PaintContext layerContext { context.font, context.layerArea.withZeroPosition(), {} };
            bool wasDrawn = false;

            {
                const auto layerSavedState = g.saveState();
                g.setTransform (context.layerTransform);

                auto layer = g.beginTransparencyLayer (context.layerArea, opacity, hasMasks);
                if (layer.isValid())
                {
                    // The layer starts from the default paint, shapes without one inherit the caller's
                    auto& layerGraphics = layer.getGraphics();
                    layerGraphics.setFillColor (g.getFillColor());
                    layerGraphics.setStrokeColor (g.getStrokeColor());
                    layerGraphics.setStrokeWidth (g.getStrokeWidth());
                    layerGraphics.setStrokeJoin (g.getStrokeJoin());
                    layerGraphics.setStrokeCap (g.getStrokeCap());
                    layerGraphics.setStrokePosition (g.getStrokePosition());
                    layerGraphics.setTransform (childTransform);

                    drawNodes (layerGraphics, node.children, layerContext, numRows);

                    const auto defaultMaskMode = (slot != nullptr && node.maskMode.isValid())
                                                   ? slot->state->get (node.maskMode)
                                                   : LayerMaskMode::Alpha;

                    if (hasMaskPath)
                    {
                        if (auto* maskGraphics = layer.addMask (defaultMaskMode))
                        {
                            maskGraphics->setTransform (childTransform);
                            maskGraphics->setFillColor (Color (0xffffffff));
                            maskGraphics->fillPath (slot->state->get (node.maskPath));
                        }
                    }

                    for (const auto& maskNode : node.masks)
                    {
                        auto* maskSlot = prepare (*maskNode, context);
                        if (maskSlot != nullptr)
                        {
                            maskSlot->state->evaluate();

                            if (maskNode->visible.isValid() && ! maskSlot->state->get (maskNode->visible))
                                continue;
                        }

                        const auto mode = (maskSlot != nullptr && maskNode->maskMode.isValid())
                                            ? maskSlot->state->get (maskNode->maskMode)
                                            : defaultMaskMode;

                        if (auto* maskGraphics = layer.addMask (mode))
                        {
                            maskGraphics->setTransform (childTransform);
                            maskGraphics->setFillColor (Color (0xffffffff));
                            drawNodes (*maskGraphics, maskNode->children, layerContext, numRows);
                        }
                    }

                    layer.commit();
                    wasDrawn = true;
                }
            }

            if (! wasDrawn)
            {
                g.setOpacity (g.getOpacity() * opacity);
                drawNodes (g, node.children, context, numRows);
            }

            break;
        }

        case Node::Kind::shape:
        case Node::Kind::image:
        case Node::Kind::svg:
        case Node::Kind::mask:
        case Node::Kind::clipShapes:
        case Node::Kind::pattern:
        case Node::Kind::marker:
        case Node::Kind::filter:
        case Node::Kind::filterPrimitive:
            break;
    }
}

/* Draws a layer through its filter: its elements in a raster covering the bounds of the filter, kept while they
   stay the same, then what the filter makes of it. Returns false when it can't, for the layer to be drawn as it is. */
bool DrawingStyle::drawFilteredLayer (Graphics& g, const Node& node, const PaintContext& context, int numRows)
{
    const auto& filterNode = *node.filter;

    auto* filterSlot = prepare (filterNode, context);
    if (filterSlot == nullptr)
        return false;

    filterSlot->state->evaluate();

    const auto region = filterSlot->state->get (filterNode.area);
    if (region.isEmpty())
        return false;

    FilterGraph filter;
    filter.linearColors = filterNode.linearColors;

    for (const auto& child : filterNode.children)
    {
        const auto& outputs = *child->filterPrimitive;
        auto primitive = outputs.base;

        if (auto* slot = prepare (*child, context))
        {
            auto& state = *slot->state;
            state.evaluate();

            const auto read = [&state] (const ExpressionOutput<float>& output, float& value)
            {
                if (output.isValid())
                    value = state.get (output);
            };

            if (outputs.deviation.isValid())
                primitive.deviation = Point<float> (state.get (outputs.deviation), state.get (outputs.deviation));

            if (outputs.radius.isValid())
                primitive.radius = Point<float> (state.get (outputs.radius), state.get (outputs.radius));

            if (outputs.offset.isValid())
                primitive.offset = state.get (outputs.offset);

            if (outputs.color.isValid())
                primitive.color = state.get (outputs.color);

            if (outputs.mode.isValid())
                primitive.blendMode = state.get (outputs.mode);

            if (outputs.saturate.isValid())
                primitive.matrix = FilterGraph::saturationMatrix (state.get (outputs.saturate));

            if (outputs.hueRotate.isValid())
                primitive.matrix = FilterGraph::hueRotationMatrix (state.get (outputs.hueRotate));

            if (outputs.position.isValid())
            {
                // A light is placed like the elements of the layer, the filter from the corner of its bounds
                const auto position = state.get (outputs.position) - region.getTopLeft();
                primitive.lightPosition[0] = position.getX();
                primitive.lightPosition[1] = position.getY();
            }

            read (outputs.scale, primitive.scale);
            read (outputs.constant, primitive.lightingConstant);
            read (outputs.exponent, primitive.specularExponent);
            read (outputs.azimuth, primitive.lightAzimuth);
            read (outputs.elevation, primitive.lightElevation);
            read (outputs.height, primitive.lightPosition[2]);

            for (size_t i = 0; i < outputs.coefficients.size(); ++i)
                read (outputs.coefficients[i], primitive.coefficients[i]);
        }

        filter.primitives.push_back (std::move (primitive));
    }

    auto& source = filterSources[std::addressof (node)];
    if (source == nullptr)
        source = std::make_unique<FilterSource> (*this, node);

    if (! node.isStatic || region != source->region)
        source->markDirty();

    source->region = region;
    source->setFilter (std::move (filter));

    // The source only draws while the layer is
    const ScopedValueSetter<const Graphics*> parentScope (source->parent, std::addressof (g));
    const ScopedValueSetter<int> rowsScope (source->numRows, numRows);

    return g.drawSource (*source, region);
}

void DrawingStyle::drawFilteredContent (Graphics& g, const FilterSource& source)
{
    if (paintFont == nullptr || source.parent == nullptr)
        return;

    const auto savedState = g.saveState();

    // The elements are drawn where they are, with the paint set around the layer
    g.copyPaint (*source.parent);
    g.setTransform (AffineTransform::translation (-source.region.getX(), -source.region.getY()).followedBy (g.getTransform()));

    const PaintContext context { *paintFont, source.region, g.getTransform() };
    drawNodes (g, source.node.children, context, source.numRows);
}

} // namespace yup
