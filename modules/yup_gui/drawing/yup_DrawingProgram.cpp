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
struct DrawingProgram::Node
{
    enum class Kind
    {
        shape,
        repeat,
        layer,
        group
    };

    const CompiledExpression& getProgram() const noexcept
    {
        return kind == Kind::shape ? drawing.getProgram() : program;
    }

    Kind kind = Kind::shape;
    CompiledExpression program;
    ExpressionDrawing drawing;
    std::unique_ptr<ExpressionState> state;
    ExpressionDrawing::Cache cache;
    ExpressionOutput<int> start, end;
    ExpressionOutput<bool> visible;
    ExpressionOutput<float> opacity;
    ExpressionOutput<AffineTransform> transform;
    std::vector<std::unique_ptr<Node>> children;
};

//==============================================================================
/* Lowers a drawing tree into one program per element, failing with the path of the offending element. */
class DrawingProgram::Compiler
{
public:
    explicit Compiler (DrawingProgram& programToBuild)
        : owner (programToBuild)
    {
    }

    Result compile (const DataTree& drawing)
    {
        try
        {
            if (drawing.getType().toString() != "drawing")
                fail ("The root element must be a 'drawing'");

            const auto name = drawing.getProperty ("name").toString();
            const auto path = name.isNotEmpty() ? name : String ("drawing");

            checkElement (drawing, path, { "id", "name" }, true);
            compileChildren (drawing, path, {}, false, owner.nodes);
        }
        catch (const Error& error)
        {
            return Result::fail (error.message);
        }

        return Result::ok();
    }

private:
    struct Error
    {
        String message;
    };

    struct Statement
    {
        String text;
        String label;
    };

    using Statements = std::vector<Statement>;

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
    [[noreturn]] static void fail (const String& message)
    {
        throw Error { message };
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
        static const StringArray paint { "fill", "fillGradient", "stroke", "strokeGradient", "strokeWidth", "strokeMiterLimit", "strokePosition", "cap:strokeCap",
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
    static void checkElement (const DataTree& element, const String& path, const StringArray& allowed, bool allowChildren)
    {
        for (int i = 0; i < element.getNumProperties(); ++i)
        {
            const auto property = element.getPropertyName (i).toString();

            if (! allowed.contains (property))
                fail (path + ": unknown property '" + property + "'");
        }

        if (! allowChildren && element.getNumChildren() > 0)
            fail (path + ": a '" + element.getType().toString() + "' can't have children");
    }

    static String getRequired (const DataTree& element, const Identifier& property, const String& path)
    {
        if (! element.hasProperty (property))
            fail (path + ": missing the required property '" + property.toString() + "'");

        return element.getProperty (property).toString();
    }

    static String parseName (const DataTree& element, const String& path)
    {
        static const String letters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

        const auto name = getRequired (element, "name", path);

        if (name.isEmpty() || ! letters.containsChar (name[0]) || ! name.containsOnly (letters + "0123456789_"))
            fail (path + ".name: '" + name + "' is not a valid name, use letters, digits and '_', starting with a letter");

        return name;
    }

    /* Parses a property as a single expression and returns it normalized and parenthesized, so
       it can be embedded in a program on one line without leaking comments or extra statements.
       References to lets are renamed to their internal names. */
    static String parseValue (const DataTree& element, const Identifier& property, const String& path, const Scope& scope)
    {
        const auto label = path + "." + property.toString();
        const auto text = getRequired (element, property, path);

        int offset = 0;
        auto parsed = Expression::parseExpression (text, offset);

        if (parsed.failed())
            fail (label + ": " + parsed.getErrorMessage());

        if (offset != (int) text.getNumBytesAsUTF8())
            fail (label + ": unexpected text after the expression");

        auto expression = parsed.getReference();
        for (const auto& [name, symbol] : scope.symbols)
            expression = expression.withRenamedSymbol (name, symbol);

        return "(" + expression.toString() + ")";
    }

    /* Maps a `line:column: message` compiler error to the element property that emitted the line. */
    static String locateError (const String& error, const StringArray& lineLabels, const String& path)
    {
        const auto line = error.upToFirstOccurrenceOf (":", false, false);
        const auto index = line.getIntValue() - 1;

        if (line.containsOnly ("0123456789") && isPositiveAndBelow (index, lineLabels.size()))
            return lineLabels[index] + ": " + error.fromFirstOccurrenceOf (":", false, false).fromFirstOccurrenceOf (":", false, false).trimStart();

        return path + ": " + error;
    }

    CompiledExpression compileStatements (const Statements& statements, const String& path) const
    {
        String text;
        StringArray lineLabels;

        for (const auto& statement : statements)
        {
            text << statement.text << "\n";

            for (int i = StringArray::fromLines (statement.text).size(); --i >= 0;)
                lineLabels.add (statement.label);
        }

        auto compiled = CompiledExpression::compile (text, owner.context);
        if (compiled.failed())
            fail (locateError (compiled.getErrorMessage(), lineLabels, path));

        return compiled.getReference();
    }

    /* Declares a let under an internal name, so it can't clash with the outputs the elements assign. */
    void declare (Scope& scope, const String& name, const String& value, const String& path, const String& label)
    {
        if (scope.declared.contains (name))
            fail (path + ".name: '" + name + "' is already defined");

        const auto symbol = "_let" + String (numSymbols++);

        scope.lets.push_back ({ "let " + symbol + " = " + value + ";", label });
        scope.symbols[name] = symbol;
        scope.declared.add (name);
    }

    //==============================================================================
    void compileChildren (const DataTree& parent, const String& parentPath, Scope scope, bool insideRepeat, std::vector<std::unique_ptr<Node>>& target)
    {
        std::map<String, int> counts;

        for (const auto& element : parent)
        {
            const auto type = element.getType().toString();
            const auto index = counts[type]++;
            const auto path = parentPath + "/" + type + (element.hasProperty ("id") ? "#" + element.getProperty ("id").toString() : "[" + String (index) + "]");

            if (type == "let")
            {
                checkElement (element, path, { "id", "name", "value" }, false);

                const auto name = parseName (element, path);
                declare (scope, name, parseValue (element, "value", path, scope), path, path + ".value");

                // Compiles the let on its own, so its errors are reported even when nothing uses it
                auto check = scope.lets;
                check.push_back ({ scope.symbols[name] + ";", path });
                compileStatements (check, path);
                continue;
            }

            if (insideRepeat && (type == "layer" || type == "group" || type == "repeat"))
                fail (path + ": a '" + type + "' can't be nested inside a repeat");

            if (type == "layer")
                target.push_back (compileContainer (element, path, scope, Node::Kind::layer, { "opacity", "visible" }));
            else if (type == "group")
                target.push_back (compileContainer (element, path, scope, Node::Kind::group, { "transform", "visible" }));
            else if (type == "repeat")
                target.push_back (compileRepeat (element, path, scope));
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

        auto statements = scope.lets;
        for (const auto& property : properties)
            if (element.hasProperty (property))
                statements.push_back ({ property + " = " + parseValue (element, property, path, scope) + ";", path + "." + property });

        if (statements.size() > scope.lets.size())
        {
            node->program = compileStatements (statements, path);
            node->state = std::make_unique<ExpressionState> (node->program, 1);
            node->visible = findOutput<bool> (node->program, "visible");
            node->opacity = findOutput<float> (node->program, "opacity");
            node->transform = findOutput<AffineTransform> (node->program, "transform");
        }

        compileChildren (element, path, scope.enter(), false, node->children);
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

        auto statements = scope.lets;
        statements.push_back ({ "_start = int " + from + ";", path + ".from" });
        statements.push_back ({ "_end = int " + to + ";", path + ".to" });

        node->program = compileStatements (statements, path);
        node->state = std::make_unique<ExpressionState> (node->program, 1);
        node->start = findOutput<int> (node->program, "_start");
        node->end = findOutput<int> (node->program, "_end");

        auto inner = scope.enter();
        declare (inner, name, "int " + from + " + _index", path, path + ".from");
        compileChildren (element, path, std::move (inner), true, node->children);
        return node;
    }

    std::unique_ptr<Node> compileShape (const DataTree& element, const String& path, const Scope& scope, bool repeated)
    {
        const auto type = element.getType().toString();
        const auto* shape = findShape (type);

        if (shape == nullptr)
            fail (path + ": unknown element '" + type + "'");

        if (element.getNumChildren() > 0)
            fail (path + ": a '" + type + "' can't have children");

        auto statements = scope.lets;
        StringArray geometry;

        for (const auto& entry : shape->geometry)
        {
            const auto property = entry.upToFirstOccurrenceOf ("=", false, false);
            const auto fallback = entry.fromFirstOccurrenceOf ("=", false, false);
            const auto value = (fallback.isEmpty() || element.hasProperty (property)) ? parseValue (element, property, path, scope) : "(" + fallback + ")";

            geometry.add (property);
            statements.push_back ({ "let _" + property + " = " + value + ";", path + "." + property });
        }

        statements.push_back ({ shape->lowering, path });

        for (int i = 0; i < element.getNumProperties(); ++i)
        {
            const auto property = element.getPropertyName (i).toString();
            if (property == "id" || geometry.contains (property))
                continue;

            auto output = findPaintOutput (shape->paint, property);
            if (output.isEmpty())
                output = findPaintOutput (getCommonPaint(), property);

            if (output.isEmpty())
                fail (path + ": unknown property '" + property + "'");

            statements.push_back ({ output + " = " + parseValue (element, property, path, scope) + ";", path + "." + property });
        }

        auto created = ExpressionDrawing::create (compileStatements (statements, path));
        if (created.failed())
            fail (path + ": " + created.getErrorMessage());

        auto node = std::make_unique<Node>();
        node->drawing = std::move (created.getReference());
        node->state = std::make_unique<ExpressionState> (node->drawing.getProgram(), repeated ? 64 : 1);

        if (repeated)
            node->state->bindRowIndex (owner.indexInput);

        return node;
    }

    DrawingProgram& owner;
    int numSymbols = 0;
};

//==============================================================================
DrawingProgram::DrawingProgram (Component& target)
    : scope (target, context)
{
    ExpressionDrawing::declareOutputs (context);
    boundsInput = context.addInput<Rectangle<float>> ("bounds");
    fontInput = context.addInput<Font> ("defaultFont");
    indexInput = context.addInput<int> ("_index");
    context.declareOutput<int> ("_start");
    context.declareOutput<int> ("_end");
    scope.install();
}

DrawingProgram::~DrawingProgram() = default;

ResultValue<std::unique_ptr<DrawingProgram>> DrawingProgram::create (Component& target, const DataTree& drawing)
{
    std::unique_ptr<DrawingProgram> program (new DrawingProgram (target));

    const auto result = Compiler (*program).compile (drawing);
    if (result.failed())
        return makeResultValueFail (result.getErrorMessage());

    return makeResultValueOk (std::move (program));
}

//==============================================================================
void DrawingProgram::draw (Graphics& g, const ApplicationTheme& theme, const Component& component)
{
    drawNodes (g, nodes, component.getLocalBounds(), theme.getDefaultFont(), 1);
}

bool DrawingProgram::prepare (Node& node, Rectangle<float> bounds, const Font& font)
{
    if (node.state == nullptr)
        return false;

    node.state->set (boundsInput, bounds);
    node.state->set (fontInput, font);
    scope.updateInputs (node.getProgram(), *node.state);
    return true;
}

void DrawingProgram::drawNodes (Graphics& g, std::vector<std::unique_ptr<Node>>& children, Rectangle<float> bounds, const Font& font, int numRows)
{
    for (auto& node : children)
        drawNode (g, *node, bounds, font, numRows);
}

void DrawingProgram::drawNode (Graphics& g, Node& node, Rectangle<float> bounds, const Font& font, int numRows)
{
    constexpr int maxRepeatCount = 4096;

    if (node.kind == Node::Kind::shape)
    {
        prepare (node, bounds, font);
        node.drawing.draw (g, *node.state, numRows, &font, &node.cache);
        return;
    }

    if (prepare (node, bounds, font))
    {
        node.state->evaluate();

        if (node.visible.isValid() && ! node.state->get (node.visible))
            return;
    }

    switch (node.kind)
    {
        case Node::Kind::repeat:
        {
            const auto count = jlimit<int64> (0, maxRepeatCount, (int64) node.state->get (node.end) - node.state->get (node.start));

            if (count > 0)
                drawNodes (g, node.children, bounds, font, (int) count);

            break;
        }

        case Node::Kind::group:
        {
            const auto savedState = g.saveState();

            if (node.transform.isValid())
                g.setTransform (node.state->get (node.transform).followedBy (g.getTransform()));

            drawNodes (g, node.children, bounds, font, numRows);
            break;
        }

        case Node::Kind::layer:
        {
            const auto opacity = node.opacity.isValid() ? jlimit (0.0f, 1.0f, node.state->get (node.opacity)) : 1.0f;

            if (opacity <= 0.0f)
                break;

            if (opacity >= 1.0f)
            {
                drawNodes (g, node.children, bounds, font, numRows);
                break;
            }

            // The bounds start at the origin, so the layer-local coordinates are the component's
            auto layer = g.beginTransparencyLayer (bounds, opacity);
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
                layer.commit();
                break;
            }

            const auto savedState = g.saveState();
            g.setOpacity (g.getOpacity() * opacity);
            drawNodes (g, node.children, bounds, font, numRows);
            break;
        }

        case Node::Kind::shape:
            break;
    }
}

} // namespace yup
