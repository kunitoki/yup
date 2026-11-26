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

/* The pixels a unit takes along x and along y under a transform, for a Graphics of a scale. */
Size<float> getPixelsPerUnit (const AffineTransform& transform, float contextScale) noexcept
{
    return { std::hypot (transform.getScaleX(), transform.getShearY()) * contextScale,
             std::hypot (transform.getShearX(), transform.getScaleY()) * contextScale };
}

/* The pixels a unit takes as a shader is told, which is given one number for both directions. */
float getShaderScale (Size<float> pixelsPerUnit) noexcept
{
    return std::sqrt (pixelsPerUnit.getWidth() * pixelsPerUnit.getHeight());
}

/* The pixels a length takes: one at least, and no more than a texture holds. */
int getNumPixels (float length, float pixelsPerUnit, int maxSize) noexcept
{
    const auto pixels = std::ceil (length * pixelsPerUnit);

    return pixels >= 1.0f ? static_cast<int> (jmin (static_cast<double> (pixels), static_cast<double> (maxSize))) : 1;
}

/* Returns an output of a state as the var a property of its type holds. */
var getOutputValue (const ExpressionState& state, int output, ComponentProperty::Type type)
{
    var value;

    DrawingScope::withExpressionType (type, [&] (auto tag)
    {
        using T = typename decltype (tag)::type;
        value = VariantConverter<T>::toVar (state.get (ExpressionOutput<T> { output }));
    });

    return value;
}

/* Returns the numbers an output of a state holds as an array, where they are. */
Span<const float> getOutputNumbers (const ExpressionState& state, int output)
{
    const auto numbers = state.get (ExpressionOutput<ExpressionArrayView<float>> { output });

    return { numbers.data, static_cast<size_t> (jmax (0, numbers.size)) };
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
        filterPrimitive,
        shader,
        part
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

    /* A shader a `use` runs: where the style keeps the one it found, and what its block declares with the output of the
       program holding the value of each, none for a property nothing sets. */
    struct ShaderUse
    {
        String name;
        int index = -1;
        std::vector<ShaderEffect::Property> properties;
        std::vector<int> outputs;

        // The uniform block has the time and no property takes its name: the shader is drawn again at every frame
        bool isGivenTime = false;
    };

    const CompiledExpression& getProgram() const noexcept
    {
        return kind == Kind::shape ? drawing.getProgram() : program;
    }

    bool hasMarkers() const noexcept
    {
        return std::any_of (markers.begin(), markers.end(), [] (int marker) { return marker >= 0; });
    }

    /* Whether what the node paints is not told by its program alone: a part is painted by the component, an image
       is looked for while painting, the markers of a shape are evaluated for every shape having them, and the time
       a shader is given goes by. A tile or a filtered layer that is not static is drawn again at every paint when
       it holds one. */
    bool paintsMoreThanItReads() const noexcept
    {
        return kind == Kind::part
            || kind == Kind::image
            || (kind == Kind::svg && ! svgInlineOnly)
            || hasMarkers()
            || (shader != nullptr && shader->isGivenTime);
    }

    Kind kind = Kind::shape;

    // The element the node was compiled from: its path in the drawing, as problems tell it, its name and its id
    String path;
    String type;
    String id;

    CompiledExpression program;
    ExpressionDrawing drawing;
    int slot = -1;
    bool repeated = false;
    bool svgInlineOnly = false;

    // Repeats holding containers: they are drawn turn after turn, and this is where the index of the turn is kept
    int loopLevel = -1;

    // Groups drawing their children in a rectangle of their own: the level of its bounds, and how it is mapped.
    // A pattern has one for its tile, and a filtered layer is given the level its first group would take: the
    // levels below it are the bounds their elements read from around them
    int boundsLevel = -1;
    bool mapsToArea = false;
    bool clipsToArea = false;

    // Patterns and filtered layers: what they draw never changes, as their elements only read what they set
    // themselves. The others are drawn again when what their elements read changes
    bool isStatic = true;

    // Layers: the filter they are drawn through, and where the style keeps what it draws of them.
    // Filters: their primitives are their children
    std::unique_ptr<Node> filter;
    int filterIndex = -1;
    std::unique_ptr<FilterPrimitive> filterPrimitive;
    bool linearColors = false;

    // Uses painting a shader, and the primitives of a filter running one
    std::unique_ptr<ShaderUse> shader;

    // Parts: the part of the component that is painted, and whether the element tells where
    Identifier part;
    bool placesPart = false;

    // Markers: how they follow the shape they are drawn on
    bool reversesStart = false;
    bool scalesWithStrokeWidth = true;

    // Shapes: the markers drawn at their start, inner and end vertices
    std::array<int, 3> markers { -1, -1, -1 };

    ExpressionOutput<int> start, end;
    ExpressionOutput<bool> visible;
    ExpressionOutput<float> opacity, strokeWidth, angle;
    ExpressionOutput<BlendMode> blend;
    ExpressionOutput<Color> tint, fill, stroke;
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

    /* An input of the program fed by the style: the bounds of a level, a pattern, or the index of a turn. */
    struct StyleInput
    {
        int input = -1;
        int index = -1;
    };

    std::vector<PropertyInput> properties;
    std::vector<StyleInput> levels;
    std::vector<StyleInput> patterns;
    std::vector<StyleInput> loops;
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
    String name;

    /* The parts of the component the drawing places, with whether each could be given bounds when compiling. */
    std::map<String, bool> parts;

    /* The drawings found by name through the resolver, as they were when compiling. */
    std::vector<std::pair<String, DataTree>> dependencies;

    /* A shader found by name through the resolver, with what compiling read of it: its block and, when it was loaded,
       the textures it reads, which tell its pictures from its series, and whether its uniform block has the time.
       The shader itself is kept by each style, which finds its own. */
    struct ShaderDependency
    {
        String name;
        DataTree block;
        bool isLoaded = false;
        StringArray textureNames;
        bool hasTime = false;
    };

    std::vector<ShaderDependency> shaderDependencies;

    /* The nodes running a shader, in the order a style keeps what it found for each. */
    std::vector<const Node*> shaders;

    /* The layers drawn through a filter, in the order a style keeps what it draws of each. */
    std::vector<const Node*> filtered;

    /* The rectangles `bounds` can be: the one of the component, then one per group with bounds of its own. */
    int numLevels = 1;

    /* The repeats drawn turn after turn, each with an index of its own. */
    int numLoops = 0;
};

/* The evaluation state of one evaluated node, per style instance. */
struct DrawingStyle::Slot
{
    std::unique_ptr<ExpressionState> state;
    DrawingScope::BoundInputs inputs;
    ExpressionDrawing::Cache cache;
    std::map<String, Image> imageCache;
    std::map<String, std::shared_ptr<Drawable>> svgCache;

    // The revisions of the patterns the node paints with, added up, as they were when it was last prepared
    uint32 patternRevisions = 0;
};

/* What a style keeps for a shader it runs: the shader its resolver found and, for one it paints, the texture it was drawn
   into last with the uniforms it was given. And why the device could not draw it, where it is painted or in the filter
   running it, empty while it is drawn. */
struct DrawingStyle::ShaderState
{
    GpuImageShader::Ptr shader;
    GpuDevice::Ptr device;
    GpuTarget::Ptr target;
    ShaderUniforms drawn;
    String error;

    // The textures holding the series the shader samples, one for each texture it declares
    std::vector<detail::ShaderSeriesTexture> series;
};

/* The picture a shader is painted as: its size in pixels, and the uniforms it is given. */
struct DrawingStyle::ShaderPicture
{
    int width = 0;
    int height = 0;
    ShaderUniforms uniforms;
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

    // The font the tile was last asked to be drawn with, and how many times it was asked to be drawn again
    Font font;
    uint32 revision = 0;
};

/* Draws the elements of a layer for its filter. */
struct DrawingStyle::FilterSource final : PaintSource
{
    /* What the elements take from the Graphics the layer is drawn in when they set no paint of their own. */
    struct Paint
    {
        Color fill, stroke;
        StrokeType strokeType;
        float feather = 0.0f;

        bool operator== (const Paint&) const = default;
    };

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

    // What the elements were last asked to be drawn with, beside what their programs read
    Font font;
    Paint paint;
    int numLoopTurns = 1;

    // The filter of the last paint, whose primitives are written again in place
    FilterGraph graph;
};

//==============================================================================
/* Lowers a drawing tree into one program per element, failing with the path of the offending element. */
class DrawingStyle::Compiler
{
public:
    Compiler (ExpressionContext& contextToUse,
              const DrawingScope& drawingScopeToUse,
              const DrawingResolver& drawingResolverToUse,
              const ShaderResolver& shaderResolverToUse,
              Compiled& resultToBuild)
        : context (contextToUse)
        , drawingScope (drawingScopeToUse)
        , drawingResolver (drawingResolverToUse)
        , shaderResolver (shaderResolverToUse)
        , result (resultToBuild)
    {
    }

    Result compile (const DataTree& drawing, DataTreeDiagnostic* failure = nullptr)
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
            result.name = name;
            result.numLevels = numLevels;
            result.numLoops = numLoops;
        }
        catch (const Error& error)
        {
            if (failure != nullptr)
            {
                *failure = DataTreeDiagnostic {
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

    /* Lists the elements with the tables their properties are checked with. */
    static std::vector<ElementDescription> describeElements()
    {
        std::vector<ElementDescription> elements;

        for (const auto* type : { "rect", "ellipse", "circle", "arc", "line", "path", "polygon", "star", "vertex", "text" })
        {
            const auto* shape = findShape (type);
            elements.push_back ({ type, "shape", shape->geometry, getShapeProperties (*shape) });
        }

        {
            const auto* anchored = findShape ("text", true);
            elements.push_back ({ "text", "shape", anchored->geometry, getShapeProperties (*anchored) });
        }

        for (const auto& kind : getElementKinds())
        {
            auto properties = kind.properties;

            if (std::strcmp (kind.type, "group") == 0 || std::strcmp (kind.type, "use") == 0)
                addPropertyNames (properties, getInheritablePaint());

            elements.push_back ({ kind.type, kind.role, {}, properties });
        }

        // A use paints a shader as it paints a drawing, and runs one as a primitive of a filter
        elements.push_back ({ "use", "media", {}, getShaderUseProperties() });

        for (const auto& [type, primitive] : getFilterPrimitives())
        {
            auto properties = getFilterPrimitiveProperties();
            properties.addArray (primitive.second);
            elements.push_back ({ type, "filterPrimitive", {}, properties });
        }

        elements.push_back ({ "use", "filterPrimitive", {}, getShaderPrimitiveProperties (2) });

        return elements;
    }

    static StringArray listShapePaint()
    {
        StringArray names;
        addPropertyNames (names, getCommonPaint());
        return names;
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

    /* Where the children of an element are: only shapes are allowed inside a clip path and inside a repeat
       running them as rows, and a repeat drawn turn after turn holds anything but layers and definitions. */
    enum class Placement
    {
        normal,
        repeat,
        loop,
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
    static constexpr const char* loopPrefix = "_loop";
    static constexpr const char* uniformPrefix = "_uniform_";
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
            { "vertex", { "at" }, "vertex = _at;", {}, { "series", "base:vertexBase", "smooth:vertexSmooth" }, {} },
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

    /* An element that is not a shape: what it is for a tool listing it, and the properties it
       takes. A group and a use also take the paint they hand down, and a use the properties of
       the drawing it paints. */
    struct ElementKind
    {
        const char* type;
        const char* role;
        StringArray properties;
    };

    static const std::vector<ElementKind>& getElementKinds()
    {
        static const StringArray media { "id", "bounds", "source", "fitting", "justification", "opacity", "blend", "tint", "transform", "clip", "clipPath", "visible" };

        static const std::vector<ElementKind> kinds {
            { "drawing", "container", { "id", "name", "viewBox", "fitting", "justification" } },
            { "property", "declaration", { "id", "name", "type", "value" } },
            { "let", "declaration", { "id", "name", "value" } },
            { "repeat", "container", { "id", "name", "from", "to" } },
            { "group", "container", { "id", "transform", "visible", "opacity", "blend", "color" } },
            { "viewport", "container", { "id", "bounds", "viewBox", "fitting", "justification", "visible" } },
            { "use", "container", { "id", "drawing", "bounds", "transform", "visible", "opacity", "blend", "color" } },
            { "layer", "container", { "id", "opacity", "blend", "clip", "clipPath", "mask", "maskMode", "visible" } },
            { "clip", "container", { "id", "bounds", "rect", "path", "clipPath", "visible" } },
            { "mask", "container", { "id", "mode", "maskMode", "visible" } },
            { "clipPath", "container", { "id" } },
            { "pattern", "definition", { "id", "name", "tile", "viewBox", "fitting", "justification", "transform" } },
            { "marker", "definition", { "id", "name", "size", "ref", "viewBox", "orient", "units" } },
            { "filter", "filter", { "id", "bounds", "linearColors" } },
            { "part", "media", { "id", "name", "bounds", "fill", "stroke", "strokeWidth", "opacity", "blend", "tint", "transform", "clip", "clipPath", "visible" } },
            { "image", "media", media },
            { "svg", "media", [] { auto properties = media; properties.add ("data"); return properties; }() }
        };

        return kinds;
    }

    static const StringArray& getElementProperties (const char* type)
    {
        const auto& kinds = getElementKinds();
        const auto found = std::find_if (kinds.begin(), kinds.end(), [type] (const ElementKind& kind)
        {
            return std::strcmp (kind.type, type) == 0;
        });

        jassert (found != kinds.end());
        return found->properties;
    }

    /* The primitives a filter is made of: what each does, and the properties it takes on top of
       the ones they all take. */
    static const std::map<String, std::pair<FilterGraph::Type, StringArray>>& getFilterPrimitives()
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

        return kinds;
    }

    static StringArray getFilterPrimitiveProperties()
    {
        return { "id", "in", "result" };
    }

    /* What a `use` takes to paint a shader, on top of the properties the shader declares: where it is painted, and how. */
    static const StringArray& getShaderUseProperties()
    {
        static const StringArray properties { "id", "drawing", "bounds", "transform", "visible", "opacity", "blend", "clip", "clipPath" };
        return properties;
    }

    /* What a `use` takes to run a shader in a filter, on top of the properties the shader declares: a second picture
       for a shader reading two. None of them is an expression. */
    static StringArray getShaderPrimitiveProperties (int numTextures)
    {
        auto properties = getFilterPrimitiveProperties();
        properties.add ("drawing");

        if (numTextures > 1)
            properties.add ("in2");

        return properties;
    }

    /* The properties a shape takes on top of its geometry and of the paint every shape takes. */
    static StringArray getShapeProperties (const Shape& shape)
    {
        StringArray properties { "id", "color" };
        addPropertyNames (properties, shape.paint);

        if (shape.acceptsMarkers)
            properties.addArray ({ "markerStart", "markerMid", "markerEnd" });

        return properties;
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

    /* The type a geometry property is of when it is a point or a rect, or nullptr. The shapes read the
       members of these, which an array of numbers lacks until it is given to one. Naming the type around
       a value already of it adds nothing to the program. */
    static const char* findGeometryType (const Shape& shape, const String& property)
    {
        if (property == "bounds")
            return "rect";

        if (property == "center" || property == "anchor" || property == "at")
            return "point";

        return String (shape.type) == "line" && (property == "from" || property == "to") ? "point" : nullptr;
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
            else if (const auto loop = parseIndexedName (name, loopPrefix); loop >= 0)
            {
                // A pattern is never inside a repeat, so the turns its elements read are those of its own repeats
                node.loops.push_back ({ i, loop });
                keepsTile = true;
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

        if (const auto loop = parseIndexedName (name, loopPrefix); isPositiveAndBelow (loop, numLoops))
            return context.getType<int>();

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

            checkElement (element, path, getElementProperties ("property"), false);

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
        checkElement (drawing, path, getElementProperties ("drawing"), true);
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
        static const StringArray keptOutOfRepeats { "layer", "pattern", "marker" };

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
                checkElement (element, path, getElementProperties ("let"), false);

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

            // What is drawn once and kept, or drawn apart, has no place where it would be drawn for each turn
            const bool isKeptOut = (placement == Placement::repeat && containers.contains (type))
                                || (placement == Placement::loop && type == "drawing")
                                || (repeatDepth > 0 && keptOutOfRepeats.contains (type));

            if (isKeptOut)
                fail (path + ": a '" + type + "' can't be nested inside a repeat", element);

            if (placement == Placement::clipShapes && type != "repeat" && findShape (type) == nullptr)
                fail (path + ": a '" + type + "' has no outline to clip to", element);

            if (type == "drawing")
            {
                addDefinition (scope, "drawing", element, path, { -1, element, std::make_shared<Scope> (scope) });
                continue;
            }

            if (type == "pattern")
            {
                addDefinition (scope, "pattern", element, path, { compilePattern (element, path, scope), {}, nullptr });
                continue;
            }

            if (type == "marker")
            {
                addDefinition (scope, "marker", element, path, { compileMarker (element, path, scope), {}, nullptr });
                continue;
            }

            std::unique_ptr<Node> node;

            if (type == "layer")
                node = compileLayer (element, path, scope);
            else if (type == "group")
                node = compileGroup (element, path, scope);
            else if (type == "viewport")
                node = compileViewport (element, path, scope);
            else if (type == "use")
                node = compileUse (element, path, scope);
            else if (type == "clip")
                node = compileClip (element, path, scope);
            else if (type == "repeat")
                node = compileRepeat (element, path, scope);
            else if (type == "image")
                node = compileMedia (element, path, scope, placement == Placement::repeat, Node::Kind::image);
            else if (type == "svg")
                node = compileMedia (element, path, scope, placement == Placement::repeat, Node::Kind::svg);
            else if (type == "part")
                node = compilePart (element, path, scope);
            else
                node = compileShape (element, path, scope, placement == Placement::repeat);

            node->path = path;
            node->type = type;
            node->id = element.getProperty ("id").toString();

            target.push_back (std::move (node));
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
    /* Compiles properties of an element into the program of its node. Each is `property`, `property:output`
       for one written to an output of another name, or `property:output:function` for one given to a function first.
       The statements given are compiled before them, with the lets `uses` tells they read. */
    void compileProperties (Node& node,
                            const DataTree& element,
                            const String& path,
                            const Scope& scope,
                            const StringArray& properties,
                            Statements assignments = {},
                            StringArray uses = {})
    {
        for (const auto& entry : properties)
        {
            const auto fields = StringArray::fromTokens (entry, ":", "");
            const auto property = fields[0];
            const Identifier propId (property);

            if (! element.hasProperty (propId))
                continue;

            const auto output = fields.size() > 1 ? fields[1] : property;
            const auto value = parseValue (element, propId, path, scope);
            const auto text = fields.size() > 2 ? fields[2] + " (" + value.text + ")" : value.text;

            assignments.push_back ({ output + " = " + text + ";", path + "." + property, element, propId });
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
        node.clip = findOutput<Rectangle<float>> (node.program, "clip");
        node.clipPath = findOutput<Path> (node.program, "clipPath");
        node.maskPath = findOutput<Path> (node.program, "_maskPath");
        node.maskMode = findOutput<LayerMaskMode> (node.program, "_maskMode");
        addSlot (node, false);
    }

    std::unique_ptr<Node> compileGroup (const DataTree& element, const String& path, const Scope& outerScope)
    {
        auto allowed = getElementProperties ("group");
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
        checkElement (element, path, getElementProperties ("viewport"), true);

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

        // A shader is painted when no drawing has the name
        if (! drawing.isValid())
        {
            if (const auto shader = findShader (name, element, path); shader.block.isValid())
                return compileShader (element, path, outerScope, name, shader);
        }

        if (! drawing.isValid() || drawing.getType().toString() != "drawing")
            fail (path + ": unknown drawing '" + name + "'", element, "drawing");

        if (std::find (expansions.begin(), expansions.end(), drawing) != expansions.end() || (int) expansions.size() >= maxUseDepth)
            fail (path + ": the drawing '" + name + "' ends up using itself", element, "drawing");

        if (closure == nullptr && std::none_of (result.dependencies.begin(), result.dependencies.end(), [&name] (const auto& dependency) { return dependency.first == name; }))
            result.dependencies.push_back ({ name, drawing.clone() });

        auto allowed = getElementProperties ("use");
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

    //==============================================================================
    /* Finds the shader a `use` names, failing for one that can't be drawn. The block is invalid for a name that is no shader,
       and the shader null for one that is not loaded: its `use` is compiled against the block alone, for a style running
       nothing there until it is made again with the shader. */
    Shader findShader (const String& name, const DataTree& element, const String& path)
    {
        auto found = shaderResolver != nullptr ? shaderResolver (name) : Shader();

        if (! found.block.isValid())
            return found;

        if (found.block.hasProperty ("material"))
            fail (path + ": the shader '" + name + "' is a material, a drawing takes a shader with a 'fragment'", element, "drawing");

        // What compiling reads of the shader tells which styles can share the compilation
        if (std::none_of (result.shaderDependencies.begin(), result.shaderDependencies.end(), [&name] (const auto& dependency) { return dependency.name == name; }))
        {
            const bool isLoaded = found.shader != nullptr;

            result.shaderDependencies.push_back ({ name,
                                                   found.block.clone(),
                                                   isLoaded,
                                                   isLoaded ? found.shader->getInputTextureNames() : StringArray(),
                                                   isLoaded && found.shader->getUniformLayout().hasMember ("time") });
        }

        return found;
    }

    /* Returns the textures of a shader that are no pictures: the ones a `number[]` property of its block has the name of,
       which are filled with its numbers. None for a shader that is not loaded. */
    static StringArray listSeriesTextures (const Shader& found)
    {
        StringArray series;

        if (found.shader == nullptr)
            return series;

        for (const auto& textureName : found.shader->getInputTextureNames())
        {
            for (const auto& property : found.block)
            {
                if (property.getType().toString() == "property"
                    && property.getProperty ("name").toString().trim() == textureName
                    && property.getProperty ("type").toString().trim() == "number[]")
                {
                    series.add (textureName);
                    break;
                }
            }
        }

        return series;
    }

    /* Reads what a `use` gives the shader it runs: each property the block of the shader declares is an output of the program
       of the element, set where the `use` is or else by its default. `allowed` holds what the element takes itself, and
       `reserved` those of them that are not expressions, which a property of their name is not set by. */
    void compileUniforms (Node& node,
                          const String& name,
                          const Shader& found,
                          const DataTree& element,
                          const String& path,
                          const Scope& scope,
                          StringArray allowed,
                          const StringArray& reserved,
                          Statements& assignments,
                          StringArray& uses)
    {
        for (const auto& property : found.block)
            if (property.getType().toString() == "property")
                allowed.add (property.getProperty ("name").toString().trim());

        checkElement (element, path, allowed, false);

        auto use = std::make_unique<Node::ShaderUse>();
        use->name = name;
        use->index = (int) result.shaders.size();

        int index = 0;

        for (const auto& property : found.block)
        {
            const auto propertyName = property.getProperty ("name").toString().trim();

            if (property.getType().toString() != "property" || propertyName.isEmpty())
                continue;

            const auto propertyPath = path + "/" + name + "/property[" + String (index++) + "]";
            const auto typeName = property.getProperty ("type").toString().trim();
            const auto type = ComponentProperty::getTypeFromName (typeName);
            const auto elementType = ComponentProperty::getElementTypeFromName (typeName);
            const Identifier propId (propertyName);

            // Of the arrays, the one of numbers fills a uniform
            const bool isFilling = type != ComponentProperty::Type::array || elementType == ComponentProperty::Type::number;
            const auto* expressionType = isFilling && ShaderEffect::getNumUniformComponents (type) > 0 ? drawingScope.getExpressionType (type, elementType) : nullptr;

            if (expressionType == nullptr)
            {
                fail (path + ": the property '" + propertyName + "' of the shader '" + name + "' can't be of type '" + typeName
                          + "', a shader takes a number, an integer, a boolean, a color, a point, a rectangle or a number[]",
                      element,
                      "drawing");
            }

            use->properties.push_back ({ propId, type });

            const bool isSet = element.hasProperty (propId) && ! reserved.contains (propertyName);

            if (! isSet && ! property.hasProperty ("value"))
                continue;

            // The output is declared with the type of the property, so that the value is converted to it as it is assigned
            const auto output = uniformPrefix + propertyName;
            const auto value = isSet ? parseValue (element, propId, path, scope) : parseValue (property, "value", propertyPath, Scope());

            context.declareOutput (output, *expressionType);

            if (isSet)
                assignments.push_back ({ output + " = " + value.text + ";", path + "." + propertyName, element, propId });
            else
                assignments.push_back ({ output + " = " + value.text + ";", propertyPath + ".value", property, "value" });

            uses.addArray (value.uses);
        }

        use->isGivenTime = found.shader != nullptr && detail::isShaderGivenTime (*found.shader, use->properties);

        node.shader = std::move (use);
        result.shaders.push_back (std::addressof (node));
    }

    /* Finds the outputs of the program of a node holding the values of the properties of the shader it runs. */
    static void bindUniforms (Node& node)
    {
        for (const auto& property : node.shader->properties)
            node.shader->outputs.push_back (node.program.findOutput (uniformPrefix + property.name.toString()));
    }

    /* Paints a shader found by name in the bounds of where it is used: what its block declares is set as the properties
       of a drawing are, and it is placed as an image is. */
    std::unique_ptr<Node> compileShader (const DataTree& element, const String& path, const Scope& scope, const String& name, const Shader& found)
    {
        // The textures a series fills are no pictures
        if (found.shader != nullptr && found.shader->getNumInputTextures() > listSeriesTextures (found).size())
            fail (path + ": the shader '" + name + "' reads a picture, run it in the 'filter' of a layer or as an 'effect:'", element, "drawing");

        // Its picture is drawn once and kept, so it has no place where it would be drawn for each turn
        if (repeatDepth > 0)
            fail (path + ": the shader '" + name + "' can't be painted inside a repeat", element, "drawing");

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::shader;

        Statements assignments;
        StringArray uses;

        if (! element.hasProperty ("bounds"))
            assignments.push_back ({ "_area = (" + levelSymbol (scope.boundsLevel) + ");", path, element });

        compileUniforms (*node, name, found, element, path, scope, getShaderUseProperties(), { "id", "drawing" }, assignments, uses);
        compileProperties (*node, element, path, scope, { "bounds:_area", "transform", "visible", "opacity", "blend", "clip", "clipPath" }, std::move (assignments), std::move (uses));
        bindUniforms (*node);

        // What is given the time changes at every frame, and so does the tile or the filtered layer it is drawn in
        if (node->shader->isGivenTime && pattern != nullptr)
            pattern->isStatic = false;

        return node;
    }

    /* Compiles a pattern where it is defined, and returns where it is in the result. */
    int compilePattern (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, getElementProperties ("pattern"), true);
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
        checkElement (element, path, getElementProperties ("marker"), true);

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
        checkElement (element, path, getElementProperties ("clipPath"), true);

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
        checkElement (element, path, getElementProperties ("layer"), true);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::layer;

        compileProperties (*node, element, path, scope, { "opacity", "blend", "clip", "clipPath", "mask:_maskPath:_toMaskPath", "maskMode:_maskMode", "visible" });

        // What a filtered layer draws is kept, like the tile of a pattern, until what its elements read around it changes
        const bool isFiltered = element.getChildWithName ("filter").isValid();
        auto* const outerPattern = pattern;

        if (isFiltered)
        {
            node->boundsLevel = numLevels;
            node->filterIndex = (int) result.filtered.size();
            result.filtered.push_back (node.get());
        }

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
        checkElement (element, path, getElementProperties ("filter"), true);

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

    /* What a primitive reads is named: the source of the layer, its alpha, or the result of a primitive before it. */
    static String getFilterInput (const DataTree& element, const Identifier& property)
    {
        const auto name = element.getProperty (property).toString().trim();
        return name == "source" ? String (FilterGraph::sourceGraphic) : (name == "sourceAlpha" ? String (FilterGraph::sourceAlpha) : name);
    }

    std::unique_ptr<Node> compileFilterPrimitive (const DataTree& element, const String& path, const Scope& scope)
    {
        using Type = FilterGraph::Type;

        if (element.getType().toString() == "use")
            return compileShaderPrimitive (element, path, scope);

        const auto& kinds = getFilterPrimitives();
        const auto kind = kinds.find (element.getType().toString());
        if (kind == kinds.end())
            fail (path + ": a 'filter' is made of blur, offset, flood, colorMatrix, composite, blend, morphology, lighting, displacement, dropShadow and use", element);

        auto allowed = getFilterPrimitiveProperties();
        allowed.addArray (kind->second.second);
        checkElement (element, path, allowed, false);

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::filterPrimitive;
        node->filterPrimitive = std::make_unique<Node::FilterPrimitive>();

        auto& primitive = *node->filterPrimitive;
        auto& base = primitive.base;

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
        base.in = getFilterInput (element, "in");
        base.in2 = getFilterInput (element, "in2");
        base.result = getFilterInput (element, "result");
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

    /* Compiles a shader run as a primitive of a filter: on the picture `in` names, and on the one `in2` names when it reads two. */
    std::unique_ptr<Node> compileShaderPrimitive (const DataTree& element, const String& path, const Scope& scope)
    {
        const auto name = getRequired (element, "drawing", path).trim();
        const auto found = findShader (name, element, path);

        if (! found.block.isValid())
            fail (path + ": unknown shader '" + name + "'", element, "drawing");

        // A filter gives its shaders the pictures they read, and nothing else
        if (const auto series = listSeriesTextures (found); ! series.isEmpty())
            fail (path + ": the shader '" + name + "' samples the series '" + series[0] + "' as a texture, which a 'filter' doesn't fill: paint it with a 'use', or set it as an 'effect:'", element, "drawing");

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::filterPrimitive;
        node->filterPrimitive = std::make_unique<Node::FilterPrimitive>();
        node->path = path;
        node->type = element.getType().toString();
        node->id = element.getProperty ("id").toString();

        auto& base = node->filterPrimitive->base;
        base.type = FilterGraph::Type::shader;
        base.in = getFilterInput (element, "in");
        base.in2 = getFilterInput (element, "in2");
        base.result = getFilterInput (element, "result");

        // A shader that is not loaded may read two pictures
        const auto allowed = getShaderPrimitiveProperties (found.shader != nullptr ? found.shader->getNumInputTextures() : 2);
        Statements assignments;
        StringArray uses;

        compileUniforms (*node, name, found, element, path, scope, allowed, allowed, assignments, uses);

        // The time goes by for what keeps a component repainting while its shader is drawn, which a filter can't tell
        if (node->shader->isGivenTime)
            fail (path + ": the shader '" + name + "' has a 'time', which a 'filter' doesn't fill: declare 'property time: number;' in its block to set it here", element, "drawing");

        if (! assignments.empty())
        {
            auto statements = selectLets (scope, uses);
            statements.insert (statements.end(), assignments.begin(), assignments.end());

            node->program = compileStatements (statements, path, element);
            addSlot (*node, false);
        }

        bindUniforms (*node);
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
        checkElement (element, path, getElementProperties ("clip"), true);

        if (element.hasProperty ("bounds") && element.hasProperty ("rect"))
            fail (path + ": specify either 'bounds' or 'rect', not both", element, "rect");

        if (element.hasProperty ("path") && element.hasProperty ("clipPath"))
            fail (path + ": specify either 'path' or 'clipPath', not both", element, "clipPath");

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::clip;

        compileProperties (*node, element, path, scope, { "bounds:clip", "rect:clip", "path:clipPath", "clipPath", "visible" });

        compileElements (element, path, scope.enter(), Placement::normal, node->children, { nullptr, &node->clipShapes });
        return node;
    }

    std::unique_ptr<Node> compileMask (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, getElementProperties ("mask"), true);

        if (element.hasProperty ("mode") && element.hasProperty ("maskMode"))
            fail (path + ": specify either 'mode' or 'maskMode', not both", element, "maskMode");

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::mask;

        compileProperties (*node, element, path, scope, { "mode:_maskMode", "maskMode:_maskMode", "visible" });

        compileElements (element, path, scope.enter(), Placement::normal, node->children);
        return node;
    }

    std::unique_ptr<Node> compileMedia (const DataTree& element, const String& path, const Scope& scope, bool repeated, Node::Kind kind)
    {
        checkElement (element, path, getElementProperties (kind == Node::Kind::svg ? "svg" : "image"), false);

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

    /* Compiles a part: what the theme paints of the component where the drawing tells, with the paint the drawing gives it.
       The component the drawing is compiled for has to declare it, and what it answers is recorded: another component
       shares the compilation when it answers alike. */
    std::unique_ptr<Node> compilePart (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, getElementProperties ("part"), false);

        if (collectsOutlines)
            fail (path + ": a 'part' has no outline to clip to", element);

        const auto name = getRequired (element, "name", path).trim();
        const auto* part = Identifier::isValidIdentifier (name) ? drawingScope.findPart (Identifier (name)) : nullptr;

        if (part == nullptr)
        {
            const auto known = drawingScope.getPartNames();

            fail (path + ": the component has no part '" + name + "'"
                      + (known.isEmpty() ? String (", it declares none") : ", it has " + known.joinIntoString (", ")),
                  element,
                  "name");
        }

        if (element.hasProperty ("bounds") && ! part->isPlaceable)
            fail (path + ".bounds: the part '" + name + "' is laid out by the component, move it with a 'transform' instead", element, "bounds");

        result.parts.emplace (name, part->isPlaceable);

        // What a part paints changes with the component, whatever the drawing reads of it: nothing holding one is kept
        if (pattern != nullptr)
            pattern->isStatic = false;

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

        for (const auto* property : { "fill", "stroke", "strokeWidth", "opacity", "blend", "tint", "transform", "clip", "clipPath", "visible" })
        {
            const Identifier propId (property);

            if (! element.hasProperty (propId))
                continue;

            const auto value = parseValue (element, propId, path, scope);
            own.push_back ({ String (property) + " = " + value.text + ";", path + "." + property, element, propId });
            uses.addArray (value.uses);
        }

        auto statements = selectLets (scope, uses);
        statements.insert (statements.end(), own.begin(), own.end());

        auto node = std::make_unique<Node>();
        node->kind = Node::Kind::part;
        node->part = Identifier (name);
        node->placesPart = element.hasProperty ("bounds");
        node->program = compileStatements (statements, path, element);
        node->area = findOutput<Rectangle<float>> (node->program, "_area");
        node->fill = findOutput<Color> (node->program, "fill");
        node->stroke = findOutput<Color> (node->program, "stroke");
        node->strokeWidth = findOutput<float> (node->program, "strokeWidth");
        node->opacity = findOutput<float> (node->program, "opacity");
        node->blend = findOutput<BlendMode> (node->program, "blend");
        node->tint = findOutput<Color> (node->program, "tint");
        node->transform = findOutput<AffineTransform> (node->program, "transform");
        node->clip = findOutput<Rectangle<float>> (node->program, "clip");
        node->clipPath = findOutput<Path> (node->program, "clipPath");
        node->visible = findOutput<bool> (node->program, "visible");
        addSlot (*node, false);
        return node;
    }

    std::unique_ptr<Node> compileRepeat (const DataTree& element, const String& path, const Scope& scope)
    {
        checkElement (element, path, getElementProperties ("repeat"), true);

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

        // A repeat of shapes runs them as the rows of their programs. One holding containers or parts is drawn
        // turn after turn instead, its index fed to the programs reading it
        static const StringArray drawnByTurns { "group", "use", "viewport", "clip", "repeat", "part" };
        bool isLoop = false;

        for (const auto& child : element)
            isLoop = isLoop || (! collectsOutlines && drawnByTurns.contains (child.getType().toString()));

        if (isLoop)
            node->loopLevel = numLoops++;

        addSlot (*node, false);

        auto inner = scope.enter();
        declare (inner, name, { "int " + from.text + " + " + (isLoop ? loopPrefix + String (node->loopLevel) : String ("_index")), from.uses }, path, path + ".from", element, "from");

        const ScopedValueSetter<int> repeating (repeatDepth, repeatDepth + 1);
        compileElements (element, path, std::move (inner), isLoop ? Placement::loop : Placement::repeat, node->children);
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

        // A vertex is one point of the line the turns of a repeat make together
        const bool isVertex = type == "vertex";

        if (isVertex && ! repeated)
            fail (path + ": a 'vertex' is one point of a line, it can only be among the shapes of a 'repeat' giving the others", element);

        auto allowed = getShapeProperties (*shape);
        for (const auto& entry : shape->geometry)
            allowed.add (entry.upToFirstOccurrenceOf ("=", false, false));

        addPropertyNames (allowed, getCommonPaint());

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

            const auto* geometryType = findGeometryType (*shape, property);
            const auto made = geometryType != nullptr ? String (geometryType) + " " + value.text : value.text;

            geometry.add (property);
            own.push_back ({ "let _" + property + " = " + made + ";", path + "." + property, element, isExplicit ? propId : Identifier {} });
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
        {
            fail (path + (isVertex ? ": 'objectBounds' is not available to a 'vertex', which is one point of its line"
                                   : ": 'objectBounds' is not available to a text placed from an 'anchor'"),
                  element);
        }

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
    const ShaderResolver& shaderResolver;
    Compiled& result;
    std::map<String, PropertyInput> propertyInputs;
    std::vector<DataTree> expansions;
    Node* pattern = nullptr;
    bool collectsOutlines = false;
    int numSymbols = 0;
    int numLevels = 1;
    int numLoops = 0;
    int repeatDepth = 0;
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
       whose drawings and shaders found by name are still the same, if any. */
    std::shared_ptr<const Compiled> find (const DataTree& drawing,
                                          const DrawingScope& scope,
                                          const DrawingResolver& drawingResolver,
                                          const ShaderResolver& shaderResolver)
    {
        const auto shape = getShape (drawing);

        for (auto it = entries.begin(); it != entries.end();)
        {
            if (it->compiled.expired())
            {
                it = entries.erase (it);
                continue;
            }

            // Most entries are told apart by their shape alone, without comparing their trees
            auto candidate = it->shape == shape ? it->compiled.lock() : nullptr;

            if (candidate != nullptr
                && it->drawing.isEquivalentTo (drawing)
                && std::all_of (candidate->symbols.begin(), candidate->symbols.end(), [&] (const auto& symbol)
                {
                    return scope.resolveSymbol (symbol.first) == symbol.second;
                })
                && std::all_of (candidate->parts.begin(), candidate->parts.end(), [&] (const auto& part)
                {
                    const auto* found = scope.findPart (Identifier (part.first));
                    return found != nullptr && found->isPlaceable == part.second;
                })
                && std::all_of (candidate->dependencies.begin(), candidate->dependencies.end(), [&] (const auto& dependency)
                {
                    return drawingResolver != nullptr && drawingResolver (dependency.first).isEquivalentTo (dependency.second);
                })
                && std::all_of (candidate->shaderDependencies.begin(), candidate->shaderDependencies.end(), [&] (const auto& dependency)
                {
                    // A drawing of that name would be painted in place of the shader
                    if (shaderResolver == nullptr || (drawingResolver != nullptr && drawingResolver (dependency.name).isValid()))
                        return false;

                    const auto found = shaderResolver (dependency.name);

                    if (! found.block.isEquivalentTo (dependency.block) || (found.shader != nullptr) != dependency.isLoaded)
                        return false;

                    return found.shader == nullptr
                        || (found.shader->getInputTextureNames() == dependency.textureNames
                            && found.shader->getUniformLayout().hasMember ("time") == dependency.hasTime);
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

        entries.push_back ({ drawing.clone(), getShape (drawing), compiled });
    }

private:
    struct Entry
    {
        DataTree drawing;
        size_t shape = 0;
        std::weak_ptr<const Compiled> compiled;
    };

    /* A number that trees which are equivalent share: the type of the root with how many properties and children
       it has, then the same of each of its children. Those are compared exactly, when the value of a property can
       be equal to another in more than one form. */
    static size_t getShape (const DataTree& tree)
    {
        const auto add = [] (size_t shape, const DataTree& element)
        {
            for (const auto part : { std::hash<Identifier>() (element.getType()), (size_t) element.getNumProperties(), (size_t) element.getNumChildren() })
                shape = shape * 1000003 + part;

            return shape;
        };

        auto shape = add (0, tree);

        for (const auto& child : tree)
            shape = add (shape, child);

        return shape;
    }

    std::vector<Entry> entries;
};

//==============================================================================
DrawingStyle::DrawingStyle (Component& target, DrawingScope::ComponentResolver resolverToUse)
    : targetComponent (&target)
    , componentResolver (std::move (resolverToUse))
    , context (getDrawingBaseContext().context)
    , scope (target, context)
{
    scope.setComponentResolver (componentResolver);
}

DrawingStyle::~DrawingStyle() = default;

ResultValue<DrawingStyle::Ptr> DrawingStyle::create (Component& target,
                                                     const DataTree& drawing,
                                                     DrawingScope::ComponentResolver componentResolver,
                                                     DataTreeDiagnostic* failure,
                                                     const DrawingResolver& drawingResolver,
                                                     const ShaderResolver& shaderResolver)
{
    DrawingStyle::Ptr style (new DrawingStyle (target, std::move (componentResolver)));
    style->drawingTree = drawing;

    auto& cache = CompiledCache::getInstance();
    auto compiled = cache.find (drawing, style->scope, drawingResolver, shaderResolver);

    if (compiled == nullptr)
    {
        auto result = std::make_shared<Compiled>();

        if (const auto compiledResult = Compiler (style->context, style->scope, drawingResolver, shaderResolver, *result).compile (drawing, failure); compiledResult.failed())
            return makeResultValueFail (compiledResult.getErrorMessage());

        cache.add (drawing, result);
        compiled = std::move (result);
    }

    style->instantiate (std::move (compiled), shaderResolver);
    return makeResultValueOk (std::move (style));
}

void DrawingStyle::instantiate (std::shared_ptr<const Compiled> compiledToUse, const ShaderResolver& shaderResolver)
{
    compiled = std::move (compiledToUse);
    slots = std::make_unique<Slot[]> (compiled->evaluated.size());
    shaderStates = std::make_unique<ShaderState[]> (compiled->shaders.size());
    propertyValues.resize (compiled->properties.size());
    propertyArrays.resize (compiled->properties.size());
    propertySeries.resize (compiled->properties.size());
    levelBounds.resize ((size_t) compiled->numLevels);
    loopIndices.resize ((size_t) compiled->numLoops);

    for (const auto& pattern : compiled->patterns)
        patternSources.push_back (std::make_unique<PatternSource> (*this, *pattern));

    for (const auto* layer : compiled->filtered)
        filterSources.push_back (std::make_unique<FilterSource> (*this, *layer));

    // The compilation is shared with styles finding other shaders under the same names, so each keeps the ones it found
    bool isGivenTime = false;

    for (size_t i = 0; i < compiled->shaders.size(); ++i)
    {
        const auto& use = *compiled->shaders[i]->shader;

        if (shaderResolver != nullptr)
            shaderStates[i].shader = shaderResolver (use.name).shader;

        isGivenTime = isGivenTime || use.isGivenTime;
    }

    if (auto* target = targetComponent.get(); target != nullptr && isGivenTime)
        frameAttachment.emplace (*target, bindFront (&DrawingStyle::advance, this));

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

void DrawingStyle::advance (double elapsedSeconds)
{
    auto* target = targetComponent.get();

    // The time stands still while the component is hidden or has another style, and while its last paint drew no shader that
    // is given the time: one that is not visible, or that the device can't draw
    if (target == nullptr || ! target->isShowing() || target->getStyle().get() != this || ! paintedShaderTime)
        return;

    shaderTime = detail::advanceShaderTime (shaderTime, elapsedSeconds);
    target->repaint();
}

std::vector<DrawingStyle::ElementDescription> DrawingStyle::getElementDescriptions()
{
    return Compiler::describeElements();
}

StringArray DrawingStyle::getShapePaintProperties()
{
    return Compiler::listShapePaint();
}

const String& DrawingStyle::getName() const noexcept
{
    return compiled->name;
}

const std::vector<ComponentProperty>& DrawingStyle::getProperties() const noexcept
{
    return compiled->properties;
}

std::vector<DrawingStyle::Shader> DrawingStyle::getShaders() const
{
    std::vector<Shader> used;

    for (const auto& dependency : compiled->shaderDependencies)
    {
        // Every use of a name has the shader the resolver gave for it
        for (size_t i = 0; i < compiled->shaders.size(); ++i)
        {
            if (compiled->shaders[i]->shader->name == dependency.name)
            {
                used.push_back ({ dependency.block, shaderStates[i].shader });
                break;
            }
        }
    }

    return used;
}

std::vector<DrawingStyle::ShaderFailure> DrawingStyle::getShaderFailures() const
{
    std::vector<ShaderFailure> failures;

    for (size_t i = 0; i < compiled->shaders.size(); ++i)
    {
        const auto& kept = shaderStates[i];

        if (kept.error.isEmpty())
            continue;

        ShaderFailure failure;
        failure.shader.shader = kept.shader;
        failure.message = kept.error;

        for (const auto& dependency : compiled->shaderDependencies)
        {
            if (dependency.name == compiled->shaders[i]->shader->name)
                failure.shader.block = dependency.block;
        }

        failures.push_back (std::move (failure));
    }

    return failures;
}

void DrawingStyle::releaseResources()
{
    // With the device goes what it refused: the shaders are asked of the device of the next window
    for (size_t i = 0; i < compiled->shaders.size(); ++i)
    {
        auto& kept = shaderStates[i];

        kept.target = nullptr;
        kept.series.clear();
        kept.device = nullptr;
        kept.error.clear();
    }

    for (auto& source : patternSources)
        source->releaseResources();

    for (auto& source : filterSources)
        source->releaseResources();
}

ResultValue<DrawingStyle::Ptr> DrawingStyle::createWithShaders (const ShaderResolver& shaderResolver, DataTreeDiagnostic* failure) const
{
    auto* target = targetComponent.get();
    if (target == nullptr)
        return makeResultValueFail ("The component the drawing was created for is gone");

    // The drawings it paints are the ones it found: what gave them may be gone
    const auto findUsedDrawing = [this] (const String& name)
    {
        const auto used = std::find_if (compiled->dependencies.begin(), compiled->dependencies.end(), [&name] (const auto& dependency)
        {
            return dependency.first == name;
        });

        return used != compiled->dependencies.end() ? used->second : DataTree();
    };

    auto created = create (*target, drawingTree, componentResolver, failure, findUsedDrawing, shaderResolver);

    if (created.wasOk() && imageResolver != nullptr)
        created.getReference()->setImageResolver (imageResolver);

    return created;
}

void DrawingStyle::setImageResolver (ImageResolver newResolver)
{
    imageResolver = std::move (newResolver);

    // The images found so far, and what was drawn with them, may not be what the resolver gives
    for (size_t i = 0; i < compiled->evaluated.size(); ++i)
        slots[i].imageCache.clear();

    for (auto& source : patternSources)
        source->markDirty();

    for (auto& source : filterSources)
        source->markDirty();

    if (auto* target = targetComponent.get())
        target->repaint();
}

/* Reads what the programs take from the component for a paint, and returns the bounds it is painted in. An array is
   read once for all the programs: where the component keeps it when it is a series of the type the drawing declares,
   else converted from the var it is given as. */
Rectangle<float> DrawingStyle::readComponent (const Component& component)
{
    scope.beginUpdates();

    for (size_t i = 0; i < propertyValues.size(); ++i)
    {
        const auto& property = compiled->properties[i];

        propertySeries[i].reset();

        if (property.type == ComponentProperty::Type::array)
        {
            const auto* reflected = component.getTypeInfo().findProperty (property.name);

            if (reflected != nullptr && reflected->isSeries() && reflected->elementType == property.elementType)
            {
                propertySeries[i] = reflected->arrayGetter (component);
                continue;
            }
        }

        propertyValues[i] = component.findProperty (property.name);

        if (property.type == ComponentProperty::Type::array)
            propertyArrays[i].assign (property.elementType, propertyValues[i]);
    }

    const auto bounds = component.getLocalBounds();

    ++paintSerial;
    levelBounds.front() = bounds;

    return bounds;
}

void DrawingStyle::paint (Graphics& g, const ApplicationTheme& theme, const Component& component)
{
    const auto font = theme.getDefaultFont();
    const auto bounds = readComponent (component);

    paintedShaderTime = false;

    const ScopedValueSetter<const Font*> painting (paintFont, std::addressof (font));
    const ScopedValueSetter<const Component*> painted (paintedComponent, std::addressof (component));
    drawNodes (g, compiled->nodes, { font, bounds, g.getTransform() }, 1);
}

std::vector<DrawingStyle::ElementBounds> DrawingStyle::measureElements (const Component& component)
{
    const auto theme = ApplicationTheme::getGlobalTheme();
    const auto font = theme != nullptr ? theme->getDefaultFont() : Font();
    const auto bounds = readComponent (component);

    std::vector<ElementBounds> measured;

    const ScopedValueSetter<const Font*> painting (paintFont, std::addressof (font));
    const ScopedValueSetter<const Component*> painted (paintedComponent, std::addressof (component));
    measureNodes (compiled->nodes, { font, bounds, {} }, {}, 1, measured);

    // The elements of the filtered layers were evaluated here, where nothing is drawn: what is kept of them
    // can't be told from their states any more
    for (auto& source : filterSources)
        source->markDirty();

    return measured;
}

/* Evaluates the rectangle a group draws what it holds in, and makes it the bounds of what it holds. Returns false
   when there is nothing to draw in. A group mapping what it holds gives the mapping, and the area it is clipped to. */
bool DrawingStyle::enterGroupBounds (const Node& node, Slot& slot, AffineTransform& mapping, std::optional<Rectangle<float>>& clip)
{
    const auto area = slot.state->get (node.area);
    auto childBounds = area;

    if (node.mapsToArea)
    {
        childBounds = node.viewBox.isValid() ? slot.state->get (node.viewBox) : area.withZeroPosition();

        if (area.isEmpty() || childBounds.isEmpty())
            return false;

        if (node.clipsToArea)
            clip = area;

        mapping = node.viewBox.isValid()
                    ? childBounds.getTransformToFit (area,
                                                     node.fitting.isValid() ? slot.state->get (node.fitting) : Fitting::scaleToFit,
                                                     Justification (node.justification.isValid() ? slot.state->get (node.justification) : Justification::center))
                    : AffineTransform::translation (area.getX(), area.getY());
    }

    levelBounds[(size_t) node.boundsLevel] = childBounds;
    return true;
}

Rectangle<float> DrawingStyle::measureNodes (const std::vector<std::unique_ptr<Node>>& children,
                                             const PaintContext& context,
                                             const AffineTransform& transform,
                                             int numRows,
                                             std::vector<ElementBounds>& measured)
{
    Rectangle<float> covered;

    for (const auto& node : children)
    {
        const auto bounds = measureNode (*node, context, transform, numRows, measured);

        if (! bounds.isEmpty())
            covered = covered.isEmpty() ? bounds : covered.unionWith (bounds);
    }

    return covered;
}

/* Follows drawNode without drawing: what an element is drawn under is what places it. */
Rectangle<float> DrawingStyle::measureNode (const Node& node,
                                            const PaintContext& context,
                                            const AffineTransform& transform,
                                            int numRows,
                                            std::vector<ElementBounds>& measured)
{
    auto* slot = prepare (node, context);

    // An element is told before what it holds, as it is painted
    const auto index = measured.size();
    measured.push_back ({ node.path, node.type, node.id, {}, {}, numRows, {} });

    if (node.kind == Node::Kind::shape)
    {
        auto bounds = node.drawing.getOutline (*slot->state, numRows).getBoundsTransformed (transform);
        const auto glyphs = node.drawing.getLabelBounds (*slot->state, numRows, &context.font).transformed (transform);

        if (bounds.isEmpty())
            bounds = node.drawing.getLabelBounds (*slot->state, numRows, &context.font, false).transformed (transform);

        measured[index].bounds = bounds;
        measured[index].textBounds = glyphs;

        return bounds;
    }

    if (node.kind == Node::Kind::image || node.kind == Node::Kind::svg)
    {
        Rectangle<float> bounds;

        for (int startRow = 0; startRow < numRows;)
        {
            const auto numEvaluated = slot->state->evaluateBlock (startRow, numRows - startRow);
            if (numEvaluated <= 0)
                break;

            const auto* visibleLanes = node.visible.isValid() ? slot->state->getLanes (node.visible).data() : nullptr;
            const auto* areaLanes = slot->state->getLanes (node.area).data();
            const auto* transformLanes = node.transform.isValid() ? slot->state->getLanes (node.transform).data() : nullptr;

            for (int lane = 0; lane < numEvaluated; ++lane)
            {
                if ((visibleLanes != nullptr && ! visibleLanes[lane]) || areaLanes[lane].isEmpty())
                    continue;

                const auto area = areaLanes[lane].transformed (transformLanes != nullptr ? transformLanes[lane].followedBy (transform) : transform);
                bounds = bounds.isEmpty() ? area : bounds.unionWith (area);
            }

            startRow += numEvaluated;
        }

        measured[index].bounds = bounds;
        return bounds;
    }

    // What is not drawn is not told
    const auto skip = [&measured, index]
    {
        measured.resize (index);
        return Rectangle<float>();
    };

    if (slot != nullptr)
    {
        slot->state->evaluate();

        if (node.visible.isValid() && ! slot->state->get (node.visible))
            return skip();
    }

    Rectangle<float> bounds;

    switch (node.kind)
    {
        case Node::Kind::repeat:
        {
            const auto count = getRepeatCount (node, *slot);

            if (node.loopLevel < 0)
            {
                if (count > 0)
                    bounds = measureNodes (node.children, context, transform, count, measured);

                break;
            }

            const auto unite = [] (const Rectangle<float>& covered, const Rectangle<float>& added)
            {
                return covered.isEmpty() ? added : (added.isEmpty() ? covered : covered.unionWith (added));
            };

            const ScopedValueSetter<int> turns (numLoopTurns, numLoopTurns * jmax (1, count));
            const auto first = static_cast<std::ptrdiff_t> (measured.size());

            for (int turn = 0; turn < count; ++turn)
            {
                loopIndices[(size_t) node.loopLevel] = turn;

                // An element is told once: what a turn draws is added to what the turns before it drew
                std::vector<ElementBounds> drawn;
                bounds = unite (bounds, measureNodes (node.children, context, transform, numRows, drawn));

                for (auto& element : drawn)
                {
                    const auto known = std::find_if (measured.begin() + first, measured.end(), [&element] (const ElementBounds& other)
                    {
                        return other.path == element.path;
                    });

                    if (known == measured.end())
                    {
                        measured.push_back (std::move (element));
                        continue;
                    }

                    known->bounds = unite (known->bounds, element.bounds);
                    known->textBounds = unite (known->textBounds, element.textBounds);
                    known->numRows += element.numRows;
                }
            }

            break;
        }

        case Node::Kind::group:
        {
            auto childTransform = (slot != nullptr && node.transform.isValid()) ? slot->state->get (node.transform).followedBy (transform) : transform;

            if (node.boundsLevel >= 0)
            {
                AffineTransform mapping;
                std::optional<Rectangle<float>> clip;

                if (! enterGroupBounds (node, *slot, mapping, clip))
                    return skip();

                childTransform = mapping.followedBy (childTransform);
            }

            bounds = measureNodes (node.children, context, childTransform, numRows, measured);
            break;
        }

        case Node::Kind::clip:
        case Node::Kind::layer:
            bounds = measureNodes (node.children, context, transform, numRows, measured);

            if (node.filter != nullptr)
                measureFilterShaders (*node.filter, context, transform, numRows, measured);

            break;

        case Node::Kind::shader:
        {
            const auto area = slot->state->get (node.area);
            const auto placed = node.transform.isValid() ? slot->state->get (node.transform).followedBy (transform) : transform;

            bounds = area.transformed (placed);
            measured[index].uniforms = prepareShader (node, slot, area, placed, 1.0f, std::numeric_limits<int>::max()).uniforms;
            break;
        }

        case Node::Kind::part:
        {
            const auto* painted = getPaintedComponent();
            const auto* part = painted != nullptr ? painted->getTypeInfo().findPart (node.part) : nullptr;
            if (part == nullptr)
                return skip();

            const auto area = node.placesPart && part->isPlaceable ? slot->state->get (node.area) : part->getArea (*painted);

            bounds = area.transformed (node.transform.isValid() ? slot->state->get (node.transform).followedBy (transform) : transform);
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
            return skip();
    }

    measured[index].bounds = bounds;
    return bounds;
}

/* Tells the shaders a filter runs, each with the bounds of the filter and the uniforms it is given. */
void DrawingStyle::measureFilterShaders (const Node& filterNode,
                                         const PaintContext& context,
                                         const AffineTransform& transform,
                                         int numRows,
                                         std::vector<ElementBounds>& measured)
{
    auto* filterSlot = prepare (filterNode, context);
    if (filterSlot == nullptr)
        return;

    filterSlot->state->evaluate();

    const auto region = filterSlot->state->get (filterNode.area);

    for (const auto& child : filterNode.children)
        if (child->shader != nullptr)
            measured.push_back ({ child->path, child->type, child->id, region.transformed (transform), {}, numRows, readFilterUniforms (*child, context, region, transform, 1.0f) });
}

/* Gives the state of a node what its program reads, but the turns of the repeats around it: the component, the font,
   the properties of the drawing, the bounds of the levels below a given one and the patterns defined below it.
   Returns the revisions of the patterns the node paints with, added up. */
uint32 DrawingStyle::feedInputs (const Node& node, Slot& slot, const PaintContext& context, int numLevels)
{
    const auto& base = getDrawingBaseContext();
    uint32 patternRevisions = 0;

    slot.state->set (base.bounds, levelBounds.front());
    slot.state->set (base.font, context.font);
    scope.updateInputs (slot.inputs, *slot.state);

    for (const auto& input : node.levels)
        if (input.index < numLevels)
            slot.state->set (ExpressionInput<Rectangle<float>> { input.input }, levelBounds[(size_t) input.index]);

    for (const auto& input : node.patterns)
    {
        auto& pattern = *patternSources[(size_t) input.index];

        if (pattern.node.boundsLevel < numLevels)
            slot.state->set (ExpressionInput<PatternPaint> { input.input }, preparePattern (input.index, context).paint);

        patternRevisions += pattern.revision;
    }

    for (const auto& input : node.properties)
    {
        const auto& value = propertyValues[(size_t) input.property];
        const auto& property = compiled->properties[(size_t) input.property];

        if (input.isSet)
            slot.state->set (ExpressionInput<bool> { input.input }, ! value.isVoid());
        else if (const auto& series = propertySeries[(size_t) input.property])
            DrawingScope::bindArrayView (*slot.state, input.input, property.elementType, *series);
        else if (property.type == ComponentProperty::Type::array)
            propertyArrays[(size_t) input.property].attach (*slot.state, input.input);
        else if (! value.isVoid())
            DrawingScope::setInputValue (*slot.state, input.input, property.type, property.elementType, value, propertyArrays[(size_t) input.property]);
    }

    return patternRevisions;
}

DrawingStyle::Slot* DrawingStyle::prepare (const Node& node, const PaintContext& context)
{
    if (node.slot < 0)
        return nullptr;

    auto& slot = slots[(size_t) node.slot];

    slot.patternRevisions = feedInputs (node, slot, context, compiled->numLevels);

    for (const auto& input : node.loops)
        slot.state->set (ExpressionInput<int> { input.input }, loopIndices[(size_t) input.index]);

    return &slot;
}

/* Tells whether the elements of a tile or of a filtered layer may draw something else than they did the last time
   they were drawn, without evaluating them: they can't while each of their programs still reads what it was last
   evaluated with, which was when they were drawn. Whatever evaluates them elsewhere, as measuring or drawing a
   layer without its filter do, has to mark what is kept of them. Only what comes from around them is given to
   their programs: the levels from the given one on, the patterns defined there and the turns of the repeats, none
   of which is around a tile or a filtered layer, are made of what the elements before them read. What a repeat
   draws turn after turn is told for all its turns. */
bool DrawingStyle::hasContentChanged (const std::vector<std::unique_ptr<Node>>& nodes, const PaintContext& context, int numLevels, bool isDrawnByTurns)
{
    // The ones that always may are found before anything is given to the programs of the others
    const bool holdsMoreThanIsRead = std::any_of (nodes.begin(), nodes.end(), [] (const auto& node)
    {
        return node->paintsMoreThanItReads();
    });

    return holdsMoreThanIsRead || std::any_of (nodes.begin(), nodes.end(), [&] (const auto& node)
    {
        return hasContentChanged (*node, context, numLevels, isDrawnByTurns);
    });
}

bool DrawingStyle::hasContentChanged (const Node& node, const PaintContext& context, int numLevels, bool isDrawnByTurns)
{
    if (node.paintsMoreThanItReads())
        return true;

    if (node.slot >= 0)
    {
        auto& slot = slots[(size_t) node.slot];
        const auto& state = *slot.state;

        // The tile of a pattern drawn again since the node was drawn changes what the node paints
        if (feedInputs (node, slot, context, numLevels) != slot.patternRevisions || ! state.isUpToDate())
            return true;

        // What the node holds was not drawn then, and would not be now. A state evaluated turn after turn only
        // tells it of its last turn, so what it holds is looked at whatever it tells
        const bool isHidden = (node.visible.isValid() && ! state.get (node.visible))
                           || (node.opacity.isValid() && state.get (node.opacity) <= 0.0f)
                           || (node.kind == Node::Kind::repeat && state.get (node.end) <= state.get (node.start));

        if (isHidden && ! isDrawnByTurns)
            return false;
    }

    for (const auto& input : node.patterns)
    {
        const auto& pattern = *compiled->patterns[(size_t) input.index];

        if (pattern.boundsLevel >= numLevels && hasContentChanged (pattern, context, numLevels, false))
            return true;
    }

    const bool holdsTurns = isDrawnByTurns || node.loopLevel >= 0;

    return hasContentChanged (node.children, context, numLevels, holdsTurns)
        || hasContentChanged (node.masks, context, numLevels, holdsTurns)
        || hasContentChanged (node.clipShapes, context, numLevels, holdsTurns)
        || (node.filter != nullptr && hasContentChanged (*node.filter, context, numLevels, holdsTurns));
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

    // A static tile only changes with where it is drawn, any other one with what its elements read too
    const bool isKept = tile == source.tile
                     && contentBounds == source.contentBounds
                     && contentTransform == source.contentTransform
                     && context.font == source.font
                     && (node.isStatic || ! hasContentChanged (node.children, context, node.boundsLevel, false));

    if (! isKept)
    {
        source.markDirty();
        source.font = context.font;
        ++source.revision;
    }

    source.tile = tile;
    source.contentBounds = contentBounds;
    source.contentTransform = contentTransform;
    source.paint = { std::addressof (source),
                     AffineTransform::translation (tile.getX(), tile.getY())
                         .followedBy (node.transform.isValid() ? state.get (node.transform) : AffineTransform()) };

    return source;
}

void DrawingStyle::drawPattern (Graphics& g, PatternSource& source)
{
    // A tile asked for outside of a paint is not drawn, and is not kept as if it were
    if (paintFont == nullptr)
    {
        source.markDirty();
        return;
    }

    const auto savedState = g.saveState();
    const PaintContext context { *paintFont, source.tile.withZeroPosition(), g.getTransform() };

    g.setTransform (source.contentTransform.followedBy (g.getTransform()));
    levelBounds[(size_t) source.node.boundsLevel] = source.contentBounds;

    // The repeats of a tile are its own, whatever is being drawn when the tile is asked for
    const ScopedValueSetter<int> turns (numLoopTurns, 1);
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
            const auto shapeOutline = shape->drawing.getOutline (*slot->state, numRows);
            outline.appendPath (shapeOutline.isClockwise() ? shapeOutline : shapeOutline.reversed());
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

/* The turns a repeat runs. Those of a repeat drawn turn after turn count with the turns of the ones
   around it, so that repeats inside one another run no more than a repeat does. */
int DrawingStyle::getRepeatCount (const Node& node, const Slot& slot) const
{
    const auto limit = node.loopLevel >= 0 ? maxRepeatCount / numLoopTurns : maxRepeatCount;

    return static_cast<int> (jlimit<int64> (0, limit, static_cast<int64> (slot.state->get (node.end)) - slot.state->get (node.start)));
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
        if (const auto* document = ComponentDocument::findFor (targetComponent.get()))
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
                        // One fill, however many tiles the area holds
                        g.setFillImage (image, AffineTransform::translation (targetArea.getX(), targetArea.getY()), { ImageWrap::Repeat, ImageWrap::Repeat, ImageFilter::Linear });
                        g.setFeather (0.0f);
                        g.fillRect (targetArea);
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
            const auto count = getRepeatCount (node, *slot);

            if (node.loopLevel < 0)
            {
                if (count > 0)
                    drawNodes (g, node.children, context, count);

                break;
            }

            // What holds containers is drawn turn after turn, each reading its index
            const ScopedValueSetter<int> turns (numLoopTurns, numLoopTurns * jmax (1, count));

            for (int turn = 0; turn < count; ++turn)
            {
                loopIndices[(size_t) node.loopLevel] = turn;
                drawNodes (g, node.children, context, numRows);
            }

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
                AffineTransform mapping;
                std::optional<Rectangle<float>> clip;

                if (! enterGroupBounds (node, *slot, mapping, clip))
                    break;

                if (clip.has_value())
                    g.setClipPath (*clip);

                if (node.mapsToArea)
                    g.setTransform (mapping.followedBy (g.getTransform()));
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

                // Drawn as they are, the elements are evaluated where no raster is made of them: the one kept of
                // them is no longer what their states tell
                filterSources[(size_t) node.filterIndex]->markDirty();
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

        case Node::Kind::shader:
            drawShader (g, node, *slot, context, numRows);
            break;

        case Node::Kind::part:
            drawPart (g, node, *slot, context, numRows);
            break;

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

/* Paints a part of the component with the paint its element sets, where the element tells or where the component puts it.
   The part is the one the component being painted declares, and the theme paints it as it does for the painter of the
   component. */
void DrawingStyle::drawPart (Graphics& g, const Node& node, Slot& slot, const PaintContext& context, int numRows)
{
    const auto* painted = getPaintedComponent();
    const auto* part = painted != nullptr ? painted->getTypeInfo().findPart (node.part) : nullptr;
    if (part == nullptr)
        return;

    auto& state = *slot.state;
    const auto opacity = node.opacity.isValid() ? jlimit (0.0f, 1.0f, state.get (node.opacity)) : 1.0f;

    if (opacity <= 0.0f)
        return;

    const auto savedState = g.saveState();

    applyClips (g, node, std::addressof (slot), context, numRows);

    if (node.transform.isValid())
        g.setTransform (state.get (node.transform).followedBy (g.getTransform()));

    if (opacity < 1.0f)
        g.setOpacity (g.getOpacity() * opacity);

    if (node.blend.isValid())
        g.setBlendMode (state.get (node.blend));

    if (node.tint.isValid())
        g.setTint (state.get (node.tint));

    if (node.fill.isValid())
        g.setFillColor (state.get (node.fill));

    if (node.stroke.isValid())
        g.setStrokeColor (state.get (node.stroke));

    if (node.strokeWidth.isValid())
        g.setStrokeWidth (state.get (node.strokeWidth));

    const auto area = node.placesPart && part->isPlaceable ? state.get (node.area) : part->getArea (*painted);

    ApplicationTheme::paintComponentPart (g, *painted, node.part, area);
}

/* The component being painted: the one given to paint, or the one the style was made for when a tile or a filtered
   layer is drawn on its own. */
const Component* DrawingStyle::getPaintedComponent() const
{
    return paintedComponent != nullptr ? paintedComponent : targetComponent.get();
}

/* Returns the uniforms a `use` gives the shader it runs: the values of the properties set where it is or by their default,
   read from the state of its program once it is evaluated, and zeros for the others. */
ShaderUniforms DrawingStyle::readUniforms (const Node& node, const Slot* slot) const
{
    const auto& use = *node.shader;
    const auto& shader = shaderStates[(size_t) use.index].shader;

    if (shader == nullptr)
        return {};

    auto uniforms = shader->createUniforms();

    for (size_t i = 0; slot != nullptr && i < use.properties.size(); ++i)
    {
        const auto& property = use.properties[i];

        if (use.outputs[i] < 0)
            continue;

        // An array is given where it is. The block has no member for one that fills a texture, which takes nothing
        if (property.type == ComponentProperty::Type::array)
            uniforms.set (property.name.toString(), getOutputNumbers (*slot->state, use.outputs[i]));
        else
            ShaderEffect::setUniform (uniforms, property, getOutputValue (*slot->state, use.outputs[i], property.type));
    }

    return uniforms;
}

/* Returns the revision of the series whose numbers are at an address: one the drawing declares as a property, or one its
   elements read from a component. Returns 0 for numbers that are no series, or whose revision is not tracked. */
uint32 DrawingStyle::findSeriesRevision (const void* numbers) const noexcept
{
    if (numbers == nullptr)
        return 0;

    for (const auto& series : propertySeries)
    {
        if (series.has_value() && series->data == numbers)
            return series->revision;
    }

    return scope.findSeriesRevision (numbers);
}

/* Returns the uniforms a filter gives a shader it runs, for the picture the bounds of the filter are under a transform:
   the pixels the layer takes where it is drawn, whatever the raster the filter runs on, whose size is another matter. */
ShaderUniforms DrawingStyle::readFilterUniforms (const Node& node,
                                                 const PaintContext& context,
                                                 const Rectangle<float>& region,
                                                 const AffineTransform& transform,
                                                 float contextScale)
{
    auto* slot = prepare (node, context);

    if (slot != nullptr)
        slot->state->evaluate();

    return prepareShader (node, slot, region, transform, contextScale, std::numeric_limits<int>::max()).uniforms;
}

/* Returns the picture a shader is painted as in an area: the pixels the area takes under a transform, for a Graphics of a
   scale, and the uniforms the shader is given for them. The slot is the one of the node, null when it sets nothing. */
DrawingStyle::ShaderPicture DrawingStyle::prepareShader (const Node& node,
                                                         const Slot* slot,
                                                         const Rectangle<float>& area,
                                                         const AffineTransform& transform,
                                                         float contextScale,
                                                         int maxSize) const
{
    const auto pixelsPerUnit = getPixelsPerUnit (transform, contextScale);

    ShaderPicture picture;
    picture.width = getNumPixels (area.getWidth(), pixelsPerUnit.getWidth(), maxSize);
    picture.height = getNumPixels (area.getHeight(), pixelsPerUnit.getHeight(), maxSize);
    picture.uniforms = readUniforms (node, slot);

    detail::setShaderStandardUniforms (picture.uniforms, node.shader->properties, picture.width, picture.height, getShaderScale (pixelsPerUnit), shaderTime);

    return picture;
}

/* Paints a shader in the bounds of its element: drawn into a texture of the size the bounds take in pixels, kept until
   what the shader is given changes, then drawn there under the transform, the opacity, the blend mode and the clips of
   the element. */
void DrawingStyle::drawShader (Graphics& g, const Node& node, Slot& slot, const PaintContext& context, int numRows)
{
    auto& state = *slot.state;
    auto& kept = shaderStates[(size_t) node.shader->index];

    const auto area = state.get (node.area);
    const auto opacity = node.opacity.isValid() ? jlimit (0.0f, 1.0f, state.get (node.opacity)) : 1.0f;

    // Without a GPU there is no texture of any size, and nothing is drawn
    const auto device = g.getGraphicsContext().getGpuDevice();
    const auto maxSize = device != nullptr ? static_cast<int> (device->getMaximumTextureSize (GpuTextureType::texture2D)) : 0;

    if (kept.shader == nullptr || area.isEmpty() || opacity <= 0.0f || maxSize <= 0)
        return;

    const auto savedState = g.saveState();

    applyClips (g, node, std::addressof (slot), context, numRows);

    if (node.transform.isValid())
        g.setTransform (state.get (node.transform).followedBy (g.getTransform()));

    if (opacity < 1.0f)
        g.setOpacity (g.getOpacity() * opacity);

    if (node.blend.isValid())
        g.setBlendMode (state.get (node.blend));

    auto picture = prepareShader (node, std::addressof (slot), area, g.getTransform(), g.getContextScale(), maxSize);

    // The series the shader samples are in textures of their own, each written again only when its series changed.
    // A texture no series fills is a picture, which a shader painted here is not given: it can't be drawn
    std::array<GpuTexture::Ptr, GpuImageShader::maxInputTextures> inputs;
    const auto& textureNames = kept.shader->getInputTextureNames();
    bool seriesChanged = false;

    kept.series.resize (static_cast<size_t> (textureNames.size()));

    for (int i = 0; i < textureNames.size(); ++i)
    {
        const auto property = detail::findShaderSeriesProperty (node.shader->properties, textureNames[i]);

        if (property < 0)
            continue;

        const auto output = node.shader->outputs[static_cast<size_t> (property)];
        const auto numbers = output >= 0 ? getOutputNumbers (state, output) : Span<const float>();

        inputs[static_cast<size_t> (i)] = detail::updateShaderSeriesTexture (kept.series[static_cast<size_t> (i)], device, numbers, findSeriesRevision (numbers.data()), seriesChanged);
    }

    // The picture drawn last is the one to draw while nothing it was made of changed
    const bool isKept = kept.target != nullptr
                     && kept.device == device
                     && kept.target->getWidth() == picture.width
                     && kept.target->getHeight() == picture.height
                     && kept.drawn == picture.uniforms
                     && ! seriesChanged;

    // What this device refused before is not asked of it again, nor told again
    const bool wasRefused = kept.error.isNotEmpty() && kept.device == device;

    const auto texture = isKept ? kept.target->asTexture()
                                : detail::runImageShader (g.getGraphicsContext(),
                                                          kept.device,
                                                          kept.target,
                                                          kept.error,
                                                          *kept.shader,
                                                          picture.uniforms,
                                                          picture.width,
                                                          picture.height,
                                                          Span<const GpuTexture::Ptr> (inputs.data(), static_cast<size_t> (textureNames.size())));

    if (! wasRefused && kept.error.isNotEmpty())
        detail::requestShaderFailureReport (targetComponent.get());

    if (texture == nullptr)
    {
        kept.target = nullptr;
        return;
    }

    kept.drawn = std::move (picture.uniforms);
    paintedShaderTime = paintedShaderTime || node.shader->isGivenTime;

    g.drawTexture (texture, area);
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

    auto* const source = filterSources[(size_t) node.filterIndex].get();

    // The primitives are written again over those of the last paint, which the source is only given when they differ
    auto& filter = source->graph;
    filter.linearColors = filterNode.linearColors;
    filter.primitives.clear();

    for (const auto& child : filterNode.children)
    {
        const auto& outputs = *child->filterPrimitive;
        auto primitive = outputs.base;

        if (child->shader != nullptr)
        {
            primitive.shader = shaderStates[(size_t) child->shader->index].shader;
            primitive.uniforms = readFilterUniforms (*child, context, region, g.getTransform(), g.getContextScale());
        }
        else if (auto* slot = prepare (*child, context))
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

    // The elements are drawn again when what they are drawn with changed: the region, the paint and the font they
    // start from, the turns their repeats are left and, unless the layer is static, what their programs read
    const FilterSource::Paint inherited { g.getFillColor(), g.getStrokeColor(), g.getStrokeType(), g.getFeather() };

    const bool isKept = region == source->region
                     && inherited == source->paint
                     && numLoopTurns == source->numLoopTurns
                     && context.font == source->font
                     && (node.isStatic || ! hasContentChanged (node.children, context, node.boundsLevel, false));

    if (! isKept)
    {
        source->markDirty();
        source->region = region;
        source->paint = inherited;
        source->numLoopTurns = numLoopTurns;
        source->font = context.font;
    }

    if (source->getFilter() != filter)
        source->setFilter (filter);

    // The source only draws while the layer is
    const ScopedValueSetter<const Graphics*> parentScope (source->parent, std::addressof (g));
    const ScopedValueSetter<int> rowsScope (source->numRows, numRows);

    const bool drawn = g.drawSource (*source, region);

    // What prevented the filter from running a shader it has is kept where the shader is used: a primitive for each child
    const auto& filterState = source->getFilterState();
    int primitive = 0;

    for (const auto& child : filterNode.children)
    {
        const bool hasFailed = primitive++ == filterState.getFailedPrimitive();

        if (child->shader == nullptr)
            continue;

        // A shader that is not loaded was told when it was not: only what a shader could not be drawn for is kept
        auto& kept = shaderStates[(size_t) child->shader->index];
        const auto error = hasFailed && kept.shader != nullptr ? filterState.getShaderError() : String();

        if (error == kept.error)
            continue;

        kept.error = error;

        if (error.isNotEmpty())
            detail::requestShaderFailureReport (targetComponent.get());
    }

    return drawn;
}

void DrawingStyle::drawFilteredContent (Graphics& g, FilterSource& source)
{
    // A layer asked for outside of its paint is not drawn, and is not kept as if it were
    if (paintFont == nullptr || source.parent == nullptr)
    {
        source.markDirty();
        return;
    }

    const auto savedState = g.saveState();

    // The elements are drawn where they are, with the paint set around the layer
    g.copyPaint (*source.parent);
    g.setTransform (AffineTransform::translation (-source.region.getX(), -source.region.getY()).followedBy (g.getTransform()));

    const PaintContext context { *paintFont, source.region, g.getTransform() };
    drawNodes (g, source.node.children, context, source.numRows);
}

} // namespace yup
