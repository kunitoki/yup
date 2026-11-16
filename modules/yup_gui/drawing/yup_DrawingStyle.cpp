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

File resolveDrawingSourceFile (const String& path)
{
    if (path.isEmpty())
        return {};

    if (File::isAbsolutePath (path))
        return File (path);

    return File::getCurrentWorkingDirectory().getChildFile (path);
}

Rectangle<float> computeDrawingFittedBounds (Rectangle<float> sourceBounds,
                                             Rectangle<float> targetArea,
                                             Fitting fitting,
                                             Justification justification) noexcept
{
    if (sourceBounds.isEmpty() || targetArea.isEmpty())
        return {};

    float scaleX = targetArea.getWidth() / sourceBounds.getWidth();
    float scaleY = targetArea.getHeight() / sourceBounds.getHeight();

    switch (fitting)
    {
        case Fitting::none:
        case Fitting::tile:
            scaleX = 1.0f;
            scaleY = 1.0f;
            break;

        case Fitting::scaleToFit:
            scaleX = scaleY = jmin (scaleX, scaleY);
            break;

        case Fitting::fitWidth:
            scaleY = scaleX;
            break;

        case Fitting::fitHeight:
            scaleX = scaleY;
            break;

        case Fitting::scaleToFill:
        case Fitting::centerCrop:
            scaleX = scaleY = jmax (scaleX, scaleY);
            break;

        case Fitting::fill:
            break;

        case Fitting::centerInside:
            scaleX = scaleY = jmin (1.0f, jmin (scaleX, scaleY));
            break;

        case Fitting::stretchWidth:
            scaleY = 1.0f;
            break;

        case Fitting::stretchHeight:
            scaleX = 1.0f;
            break;
    }

    const float scaledWidth = sourceBounds.getWidth() * scaleX;
    const float scaledHeight = sourceBounds.getHeight() * scaleY;

    float x = targetArea.getX();
    float y = targetArea.getY();

    if (justification.testFlags (Justification::horizontalCenter))
        x += (targetArea.getWidth() - scaledWidth) * 0.5f;
    else if (justification.testFlags (Justification::right))
        x += targetArea.getWidth() - scaledWidth;

    if (justification.testFlags (Justification::verticalCenter))
        y += (targetArea.getHeight() - scaledHeight) * 0.5f;
    else if (justification.testFlags (Justification::bottom))
        y += targetArea.getHeight() - scaledHeight;

    return { x, y, scaledWidth, scaledHeight };
}

const Image& resolveDrawingImage (std::map<String, Image>& cache, const String& source)
{
    if (const auto found = cache.find (source); found != cache.end())
        return found->second;

    Image image;
    const auto trimmed = source.trim();

    if (trimmed.startsWithIgnoreCase ("data:image/"))
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
        const auto file = resolveDrawingSourceFile (trimmed);
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

    return cache.emplace (source, std::move (image)).first->second;
}

std::shared_ptr<Drawable> resolveDrawingSvg (std::map<String, std::shared_ptr<Drawable>>& cache, const String& source, bool inlineOnly)
{
    if (const auto found = cache.find (source); found != cache.end())
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
            const auto file = resolveDrawingSourceFile (trimmed);
            if (file.existsAsFile() && parsed->parseSVG (file))
                drawable = std::move (parsed);
        }
    }

    cache.emplace (source, drawable);
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
        mask
    };

    const CompiledExpression& getProgram() const noexcept
    {
        return kind == Kind::shape ? drawing.getProgram() : program;
    }

    Kind kind = Kind::shape;
    CompiledExpression program;
    ExpressionDrawing drawing;
    int slot = -1;
    bool repeated = false;
    bool svgInlineOnly = false;
    ExpressionOutput<int> start, end;
    ExpressionOutput<bool> visible;
    ExpressionOutput<float> opacity;
    ExpressionOutput<BlendMode> blend;
    ExpressionOutput<Color> tint;
    ExpressionOutput<AffineTransform> transform;
    ExpressionOutput<Rectangle<float>> area, clip;
    ExpressionOutput<Path> clipPath, maskPath;
    ExpressionOutput<LayerMaskMode> maskMode;
    ExpressionOutput<String> source;
    ExpressionOutput<Fitting> fitting;
    ExpressionOutput<Justification::Flags> justification;
    std::vector<std::unique_ptr<Node>> children;
    std::vector<std::unique_ptr<Node>> masks;

    /* An input of the program fed from a drawing property: its value, or whether it is set. */
    struct PropertyInput
    {
        int input = -1;
        int property = -1;
        bool isSet = false;
    };

    std::vector<PropertyInput> properties;
};

/* An immutable compiled drawing, shared by the styles of components resolving its symbols alike. */
struct DrawingStyle::Compiled
{
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<const Node*> evaluated;
    std::map<String, const ExpressionType*> symbols;
    std::vector<ComponentProperty> properties;
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

//==============================================================================
/* Lowers a drawing tree into one program per element, failing with the path of the offending element. */
class DrawingStyle::Compiler
{
public:
    Compiler (ExpressionContext& contextToUse, const DrawingScope& drawingScopeToUse, Compiled& resultToBuild)
        : context (contextToUse)
        , drawingScope (drawingScopeToUse)
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

            checkElement (drawing, path, { "id", "name" }, true);

            Scope scope;
            declareProperties (drawing, path, scope);
            compileElements (drawing, path, std::move (scope), false, result.nodes, nullptr);
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

    /* The lets visible to an element: the statements declaring them under internal names, the
       internal name of each visible name, and the names declared by the current scope itself. */
    struct Scope
    {
        Statements lets;
        std::map<String, String> symbols;
        StringArray declared;

        Scope enter() const
        {
            auto inner = *this;
            inner.declared.clear();
            return inner;
        }
    };

    /* A shape: its geometry properties (`name=default` when optional), the statements drawing
       it from the `_name` locals holding them, and the paint properties it adds (`property:output`). */
    struct Shape
    {
        const char* type;
        StringArray geometry;
        String lowering;
        StringArray paint;
    };

    //==============================================================================
    [[noreturn]] static void fail (const String& message, const DataTree& element = {}, const Identifier& property = {})
    {
        throw Error { message, element, property };
    }

    static const Shape* findShape (const String& type)
    {
        static const std::vector<Shape> shapes {
            { "rect", { "bounds" }, "rect = _bounds;", { "cornerRadius", "topLeftRadius", "topRightRadius", "bottomLeftRadius", "bottomRightRadius" } },
            { "ellipse", { "bounds" }, "ellipse = _bounds;", {} },
            { "circle", { "center", "radius" }, "point = _center; radius = _radius;", {} },
            { "arc", { "center", "radius", "from", "to" }, "path = path().addArc (rect (_center.x - _radius, _center.y - _radius, _radius * 2, _radius * 2), _from, _to, true);", {} },
            { "line", { "from", "to" }, "line = line (_from, _to);", {} },
            { "path", { "data" }, "path = path (_data);", {} },
            { "polygon", { "center", "sides", "radius", "angle=0" }, "path = path().addPolygon (_center, _sides, _radius, _angle);", {} },
            { "star", { "center", "points", "innerRadius", "outerRadius", "angle=0" }, "path = path().addStar (_center, _points, _innerRadius, _outerRadius, _angle);", {} },
            { "text", { "bounds", "text" }, "labelRect = _bounds; label = _text;", { "font:labelFont", "justification:labelJustification", "stroke:labelStroke" } }
        };

        for (const auto& shape : shapes)
            if (type == shape.type)
                return &shape;

        return nullptr;
    }

    static const StringArray& getCommonPaint()
    {
        static const StringArray paint { "fill", "fillGradient", "stroke", "strokeGradient", "strokeWidth", "strokeMiterLimit", "strokePosition",
                                         "dash:strokeDash", "dashOffset:strokeDashOffset", "cap:strokeCap",
                                         "join:strokeJoin", "feather", "opacity", "blend", "tint", "transform", "clip", "clipPath", "visible" };
        return paint;
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
    static Value parseValue (const DataTree& element, const Identifier& property, const String& path, const Scope& scope)
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

            if (const auto found = propertyInputs.find (program.getInputName (i)); found != propertyInputs.end())
                node.properties.push_back ({ i, found->second.property, found->second.isSet });
        }
    }

    /* The answers about the component decide whether another component can share the compilation, so
       they are recorded. The property inputs are the drawing's own, the same for every component. */
    const ExpressionType* resolveSymbol (const String& name)
    {
        if (const auto found = propertyInputs.find (name); found != propertyInputs.end())
            return found->second.type;

        const auto* type = drawingScope.resolveSymbol (name);
        result.symbols.emplace (name, type);
        return type;
    }

    //==============================================================================
    /* Declares the properties of the drawing as lets of its root scope, visible to every element. Each
       reads the property input when the component has a value for it, else the default. */
    void declareProperties (const DataTree& drawing, const String& drawingPath, Scope& scope)
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
                checkDefault (scope, fallback, type, typeName, path, element);

            // A number default can be any number, the property is a float
            if (type == ComponentProperty::Type::number)
                fallback.text = "float " + fallback.text;

            const auto isSet = "_hasProperty_" + name;
            propertyInputs[isSet] = { context.getType<bool>(), property, true };

            declare (scope, name, { "(" + isSet + " ? " + input + " : " + fallback.text + ")", fallback.uses }, path, path + ".value", element, element.hasProperty ("value") ? Identifier ("value") : Identifier {});
        }
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

    /* Compiles the default on its own, failing when its type doesn't match the property. */
    void checkDefault (const Scope& scope, const Value& fallback, ComponentProperty::Type type, const String& typeName, const String& path, const DataTree& element)
    {
        auto statements = selectLets (scope, fallback.uses);
        statements.push_back ({ fallback.text + ";", path + ".value", element, "value" });

        const auto program = compileStatements (statements, path, element, "value");
        const ExpressionType* actual = nullptr;

        for (int i = 0; i < program.getNumOutputs(); ++i)
            if (program.getOutputName (i) == "result")
                actual = program.getOutputType (i);

        jassert (actual != nullptr);

        const auto* expected = drawingScope.getExpressionType (type, ComponentProperty::Type::any);
        const auto isNumber = actual == context.getType<int>() || actual == context.getType<double>();

        if (actual != nullptr && actual != expected && ! (type == ComponentProperty::Type::number && isNumber))
            fail (path + ".value: the type '" + actual->getName() + "' doesn't match the property type '" + typeName + "'", element, "value");
    }

    //==============================================================================
    void compileElements (const DataTree& parent,
                          const String& parentPath,
                          Scope scope,
                          bool insideRepeat,
                          std::vector<std::unique_ptr<Node>>& target,
                          std::vector<std::unique_ptr<Node>>* layerMasks)
    {
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
                if (layerMasks == nullptr)
                    fail (path + ": a 'mask' can only appear inside a 'layer'", element);

                layerMasks->push_back (compileMask (element, path, scope));
                continue;
            }

            if (insideRepeat && (type == "layer" || type == "group" || type == "clip" || type == "repeat"))
                fail (path + ": a '" + type + "' can't be nested inside a repeat", element);

            if (type == "layer")
                target.push_back (compileLayer (element, path, scope));
            else if (type == "group")
                target.push_back (compileContainer (element, path, scope, Node::Kind::group, { "transform", "visible" }));
            else if (type == "clip")
                target.push_back (compileClip (element, path, scope));
            else if (type == "repeat")
                target.push_back (compileRepeat (element, path, scope));
            else if (type == "image")
                target.push_back (compileMedia (element, path, scope, insideRepeat, Node::Kind::image));
            else if (type == "svg")
                target.push_back (compileMedia (element, path, scope, insideRepeat, Node::Kind::svg));
            else
                target.push_back (compileShape (element, path, scope, insideRepeat));
        }
    }

    std::unique_ptr<Node> compileContainer (const DataTree& element, const String& path, const Scope& scope, Node::Kind kind, const StringArray& properties)
    {
        auto allowed = properties;
        allowed.add ("id");
        checkElement (element, path, allowed, true);

        auto node = std::make_unique<Node>();
        node->kind = kind;

        Statements assignments;
        StringArray uses;

        for (const auto& property : properties)
        {
            if (! element.hasProperty (property))
                continue;

            const Identifier propId (property);
            const auto value = parseValue (element, propId, path, scope);
            assignments.push_back ({ property + " = " + value.text + ";", path + "." + property, element, propId });
            uses.addArray (value.uses);
        }

        if (! assignments.empty())
        {
            auto statements = selectLets (scope, uses);
            statements.insert (statements.end(), assignments.begin(), assignments.end());

            node->program = compileStatements (statements, path, element);
            node->visible = findOutput<bool> (node->program, "visible");
            node->opacity = findOutput<float> (node->program, "opacity");
            node->transform = findOutput<AffineTransform> (node->program, "transform");
            addSlot (*node, false);
        }

        compileElements (element, path, scope.enter(), false, node->children, nullptr);
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

        compileElements (element, path, scope.enter(), false, node->children, &node->masks);
        return node;
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

        compileElements (element, path, scope.enter(), false, node->children, nullptr);
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

        compileElements (element, path, scope.enter(), false, node->children, nullptr);
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
            own.push_back ({ "_area = (bounds);", path, element });
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
        compileElements (element, path, std::move (inner), true, node->children, nullptr);
        return node;
    }

    std::unique_ptr<Node> compileShape (const DataTree& element, const String& path, const Scope& scope, bool repeated)
    {
        const auto type = element.getType().toString();
        const auto* shape = findShape (type);

        if (shape == nullptr)
            fail (path + ": unknown element '" + type + "'", element);

        StringArray allowed { "id" };
        for (const auto& entry : shape->geometry)
            allowed.add (entry.upToFirstOccurrenceOf ("=", false, false));
        for (const auto& entry : shape->paint)
            allowed.add (entry.upToFirstOccurrenceOf (":", false, false));
        for (const auto& entry : getCommonPaint())
            allowed.add (entry.upToFirstOccurrenceOf (":", false, false));

        checkElement (element, path, allowed, false);

        Statements own;
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

        own.push_back ({ shape->lowering, path, element });

        for (int i = 0; i < element.getNumProperties(); ++i)
        {
            const auto propId = element.getPropertyName (i);
            const auto property = propId.toString();
            if (property == "id" || geometry.contains (property))
                continue;

            auto output = findPaintOutput (shape->paint, property);
            if (output.isEmpty())
                output = findPaintOutput (getCommonPaint(), property);

            const auto value = parseValue (element, propId, path, scope);
            const auto rhs = output == "strokeDash" ? ("dash (" + value.text + ")") : value.text;
            own.push_back ({ output + " = " + rhs + ";", path + "." + property, element, propId });
            uses.addArray (value.uses);
        }

        auto statements = selectLets (scope, uses);
        statements.insert (statements.end(), own.begin(), own.end());

        auto created = ExpressionDrawing::create (compileStatements (statements, path, element));
        if (created.failed())
            fail (path + ": " + created.getErrorMessage(), element);

        auto node = std::make_unique<Node>();
        node->drawing = std::move (created.getReference());
        addSlot (*node, repeated);
        return node;
    }

    ExpressionContext& context;
    const DrawingScope& drawingScope;
    Compiled& result;
    std::map<String, PropertyInput> propertyInputs;
    int numSymbols = 0;
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

    /* Returns a compilation whose symbols resolve in the scope to the types they had when compiled, if any. */
    std::shared_ptr<const Compiled> find (const DataTree& drawing, const DrawingScope& scope)
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
                                                     YuiDiagnostic* failure)
{
    DrawingStyle::Ptr style (new DrawingStyle (target, std::move (componentResolver)));

    auto& cache = CompiledCache::getInstance();
    auto compiled = cache.find (drawing, style->scope);

    if (compiled == nullptr)
    {
        auto result = std::make_shared<Compiled>();

        if (const auto compiledResult = Compiler (style->context, style->scope, *result).compile (drawing, failure); compiledResult.failed())
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

void DrawingStyle::paint (Graphics& g, const ApplicationTheme& theme, const Component& component)
{
    for (size_t i = 0; i < propertyValues.size(); ++i)
        propertyValues[i] = component.findProperty (compiled->properties[i].name);

    drawNodes (g, compiled->nodes, component.getLocalBounds(), theme.getDefaultFont(), 1);
}

DrawingStyle::Slot* DrawingStyle::prepare (const Node& node, Rectangle<float> bounds, const Font& font)
{
    if (node.slot < 0)
        return nullptr;

    auto& slot = slots[(size_t) node.slot];
    const auto& base = getDrawingBaseContext();

    slot.state->set (base.bounds, bounds);
    slot.state->set (base.font, font);
    scope.updateInputs (slot.inputs, *slot.state);

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

void DrawingStyle::drawNodes (Graphics& g, const std::vector<std::unique_ptr<Node>>& children, Rectangle<float> bounds, const Font& font, int numRows)
{
    for (const auto& node : children)
        drawNode (g, *node, bounds, font, numRows);
}

void DrawingStyle::drawNode (Graphics& g, const Node& node, Rectangle<float> bounds, const Font& font, int numRows)
{
    auto* slot = prepare (node, bounds, font);

    if (node.kind == Node::Kind::shape)
    {
        node.drawing.draw (g, *slot->state, numRows, &font, &slot->cache);
        return;
    }

    if (node.kind == Node::Kind::image || node.kind == Node::Kind::svg)
    {
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
                    if (const auto drawable = resolveDrawingSvg (slot->svgCache, src, node.svgInlineOnly))
                        drawable->paint (g, targetArea, fitting, justification);
                }
                else
                {
                    const auto& image = resolveDrawingImage (slot->imageCache, src);
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
                        const auto destBounds = computeDrawingFittedBounds (sourceBounds, targetArea, fitting, justification);
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
                drawNodes (g, node.children, bounds, font, (int) count);

            break;
        }

        case Node::Kind::group:
        {
            const auto savedState = g.saveState();

            if (node.transform.isValid())
                g.setTransform (slot->state->get (node.transform).followedBy (g.getTransform()));

            drawNodes (g, node.children, bounds, font, numRows);
            break;
        }

        case Node::Kind::clip:
        {
            const auto savedState = g.saveState();

            if (slot != nullptr)
            {
                if (node.clip.isValid())
                    g.setClipPath (slot->state->get (node.clip));

                if (node.clipPath.isValid())
                    g.setClipPath (slot->state->get (node.clipPath));
            }

            drawNodes (g, node.children, bounds, font, numRows);
            break;
        }

        case Node::Kind::layer:
        {
            const auto opacity = (slot != nullptr && node.opacity.isValid()) ? jlimit (0.0f, 1.0f, slot->state->get (node.opacity)) : 1.0f;

            if (opacity <= 0.0f)
                break;

            const bool hasBlend = slot != nullptr && node.blend.isValid() && slot->state->get (node.blend) != BlendMode::SrcOver;
            const bool hasClip = slot != nullptr && (node.clip.isValid() || node.clipPath.isValid());
            const bool hasMaskPath = slot != nullptr && node.maskPath.isValid();
            const bool hasMasks = hasMaskPath || ! node.masks.empty();

            if (opacity >= 1.0f && ! hasBlend && ! hasMasks)
            {
                if (hasClip)
                {
                    const auto savedState = g.saveState();

                    if (node.clip.isValid())
                        g.setClipPath (slot->state->get (node.clip));

                    if (node.clipPath.isValid())
                        g.setClipPath (slot->state->get (node.clipPath));

                    drawNodes (g, node.children, bounds, font, numRows);
                }
                else
                {
                    drawNodes (g, node.children, bounds, font, numRows);
                }

                break;
            }

            std::optional<Graphics::SavedState> parentSaved;
            if (hasBlend || hasClip)
            {
                parentSaved.emplace (g.saveState());

                if (slot != nullptr && node.clip.isValid())
                    g.setClipPath (slot->state->get (node.clip));

                if (slot != nullptr && node.clipPath.isValid())
                    g.setClipPath (slot->state->get (node.clipPath));

                if (hasBlend)
                    g.setBlendMode (slot->state->get (node.blend));
            }

            // The bounds start at the origin, so the layer-local coordinates are the component's
            auto layer = g.beginTransparencyLayer (bounds, opacity, hasMasks);
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

                drawNodes (layerGraphics, node.children, bounds, font, numRows);

                const auto defaultMaskMode = (slot != nullptr && node.maskMode.isValid())
                                               ? slot->state->get (node.maskMode)
                                               : LayerMaskMode::Alpha;

                if (hasMaskPath)
                {
                    if (auto* maskGraphics = layer.addMask (defaultMaskMode))
                    {
                        maskGraphics->setFillColor (Color (0xffffffff));
                        maskGraphics->fillPath (slot->state->get (node.maskPath));
                    }
                }

                for (const auto& maskNode : node.masks)
                {
                    auto* maskSlot = prepare (*maskNode, bounds, font);
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
                        maskGraphics->setFillColor (Color (0xffffffff));
                        drawNodes (*maskGraphics, maskNode->children, bounds, font, numRows);
                    }
                }

                layer.commit();
                break;
            }

            const auto savedState = g.saveState();
            g.setOpacity (g.getOpacity() * opacity);
            drawNodes (g, node.children, bounds, font, numRows);
            break;
        }

        case Node::Kind::shape:
        case Node::Kind::image:
        case Node::Kind::svg:
        case Node::Kind::mask:
            break;
    }
}

} // namespace yup
