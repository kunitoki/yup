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
    int slot = -1;
    bool repeated = false;
    ExpressionOutput<int> start, end;
    ExpressionOutput<bool> visible;
    ExpressionOutput<float> opacity;
    ExpressionOutput<AffineTransform> transform;
    std::vector<std::unique_ptr<Node>> children;

    /* An input of the program fed from a drawing property: its value, or whether it is set. */
    struct PropertyInput
    {
        int input = -1;
        int property = -1;
        bool isSet = false;
    };

    std::vector<PropertyInput> properties;
};

/* An immutable compiled drawing, shared by the programs of components resolving its symbols alike. */
struct DrawingProgram::Compiled
{
    std::vector<std::unique_ptr<Node>> nodes;
    std::vector<const Node*> evaluated;
    std::map<String, const ExpressionType*> symbols;
    std::vector<ComponentProperty> properties;
};

/* The evaluation state of one evaluated node, per program. */
struct DrawingProgram::Slot
{
    std::unique_ptr<ExpressionState> state;
    ExpressionDrawing::Cache cache;
};

//==============================================================================
/* Lowers a drawing tree into one program per element, failing with the path of the offending element. */
class DrawingProgram::Compiler
{
public:
    Compiler (ExpressionContext& contextToUse, const DrawingScope& drawingScopeToUse, Compiled& resultToBuild)
        : context (contextToUse)
        , drawingScope (drawingScopeToUse)
        , result (resultToBuild)
    {
    }

    Result compile (const DataTree& drawing)
    {
        context.setSymbolResolver ([this] (const String& name)
        {
            return resolveSymbol (name);
        });

        auto outcome = Result::ok();

        try
        {
            if (drawing.getType().toString() != "drawing")
                fail ("The root element must be a 'drawing'");

            const auto name = drawing.getProperty ("name").toString();
            const auto path = name.isNotEmpty() ? name : String ("drawing");

            checkElement (drawing, path, { "id", "name" }, true);

            Scope scope;
            declareProperties (drawing, path, scope);
            compileChildren (drawing, path, std::move (scope), false, result.nodes);
        }
        catch (const Error& error)
        {
            outcome = Result::fail (error.message);
        }

        context.setSymbolResolver (nullptr);
        return outcome;
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
        String symbol;
        StringArray uses;
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
    static Value parseValue (const DataTree& element, const Identifier& property, const String& path, const Scope& scope)
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
    static String locateError (const String& error, const StringArray& lineLabels, const String& path)
    {
        const auto line = error.upToFirstOccurrenceOf (":", false, false);
        const auto index = line.getIntValue() - 1;

        if (line.containsOnly ("0123456789") && isPositiveAndBelow (index, lineLabels.size()))
            return lineLabels[index] + ": " + error.fromFirstOccurrenceOf (":", false, false).fromFirstOccurrenceOf (":", false, false).trimStart();

        return path + ": " + error;
    }

    CompiledExpression compileStatements (const Statements& statements, const String& path)
    {
        String text;
        StringArray lineLabels;

        for (const auto& statement : statements)
        {
            text << statement.text << "\n";

            for (int i = StringArray::fromLines (statement.text).size(); --i >= 0;)
                lineLabels.add (statement.label);
        }

        auto compiled = CompiledExpression::compile (text, context);
        if (compiled.failed())
            fail (locateError (compiled.getErrorMessage(), lineLabels, path));

        for (const auto& statement : statements)
            if (statement.symbol.isNotEmpty())
                verifiedLets.addIfNotAlreadyThere (statement.symbol);

        return compiled.getReference();
    }

    /* Compiles on its own each let of the scope no program has compiled yet, in declaration order,
       so its errors are reported even when nothing uses it, and before the errors of later elements. */
    void verifyLets (const Scope& scope)
    {
        for (const auto& let : scope.lets)
        {
            if (verifiedLets.contains (let.symbol))
                continue;

            auto check = selectLets (scope, { let.symbol });
            check.push_back ({ let.symbol + ";", let.label });
            compileStatements (check, let.label);
        }
    }

    /* Declares a let under an internal name, so it can't clash with the outputs the elements assign. */
    void declare (Scope& scope, const String& name, const Value& value, const String& path, const String& label)
    {
        if (scope.declared.contains (name))
            fail (path + ".name: '" + name + "' is already defined");

        const auto symbol = "_let" + String (numSymbols++);

        scope.lets.push_back ({ "let " + symbol + " = " + value.text + ";", label, symbol, value.uses });
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

            const auto path = drawingPath + "/property" + (element.hasProperty ("id") ? "#" + element.getProperty ("id").toString() : "[" + String (index) + "]");
            ++index;

            checkElement (element, path, { "id", "name", "type", "value" }, false);

            const auto name = parseName (element, path);
            const auto typeName = getRequired (element, "type", path);
            const auto type = ComponentProperty::getTypeFromName (typeName);
            const auto* expressionType = drawingScope.getExpressionType (type);

            if (expressionType == nullptr)
                fail (path + ".type: '" + typeName + "' can't be used in a drawing, use number, integer, boolean, string or color");

            auto fallback = element.hasProperty ("value") ? parseValue (element, "value", path, scope) : Value { getDefaultText (type), {} };

            if (element.hasProperty ("value"))
                checkDefault (scope, fallback, type, typeName, path);

            // A number default can be any number, the property is a float
            if (type == ComponentProperty::Type::number)
                fallback.text = "float " + fallback.text;

            const auto property = (int) result.properties.size();
            const auto input = "_property_" + name;
            const auto isSet = "_hasProperty_" + name;

            propertyInputs[input] = { expressionType, property, false };
            propertyInputs[isSet] = { context.getType<bool>(), property, true };
            result.properties.push_back (ComponentProperty { .name = Identifier (name), .type = type });

            declare (scope, name, { "(" + isSet + " ? " + input + " : " + fallback.text + ")", fallback.uses }, path, path + ".value");
            verifiedLets.add (scope.symbols[name]);
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
            default:                               return "(0.0)";
        }
    }

    /* Compiles the default on its own, failing when its type doesn't match the property. */
    void checkDefault (const Scope& scope, const Value& fallback, ComponentProperty::Type type, const String& typeName, const String& path)
    {
        auto statements = selectLets (scope, fallback.uses);
        statements.push_back ({ fallback.text + ";", path + ".value" });

        const auto program = compileStatements (statements, path);
        const ExpressionType* actual = nullptr;

        for (int i = 0; i < program.getNumOutputs(); ++i)
            if (program.getOutputName (i) == "result")
                actual = program.getOutputType (i);

        jassert (actual != nullptr);

        const auto* expected = drawingScope.getExpressionType (type);
        const auto isNumber = actual == context.getType<int>() || actual == context.getType<double>();

        if (actual != nullptr && actual != expected && ! (type == ComponentProperty::Type::number && isNumber))
            fail (path + ".value: the type '" + actual->getName() + "' doesn't match the property type '" + typeName + "'");
    }

    //==============================================================================
    void compileChildren (const DataTree& parent, const String& parentPath, Scope scope, bool insideRepeat, std::vector<std::unique_ptr<Node>>& target)
    {
        try
        {
            compileElements (parent, parentPath, scope, insideRepeat, target);
        }
        catch (const Error&)
        {
            verifyLets (scope);
            throw;
        }

        verifyLets (scope);
    }

    void compileElements (const DataTree& parent, const String& parentPath, Scope& scope, bool insideRepeat, std::vector<std::unique_ptr<Node>>& target)
    {
        std::map<String, int> counts;

        for (const auto& element : parent)
        {
            const auto type = element.getType().toString();
            const auto index = counts[type]++;
            const auto path = parentPath + "/" + type + (element.hasProperty ("id") ? "#" + element.getProperty ("id").toString() : "[" + String (index) + "]");

            if (type == "property")
            {
                // The properties of the drawing are declared up front, see declareProperties
                if (parent.getType().toString() != "drawing")
                    fail (path + ": a 'property' can only be declared at the top of a drawing");

                continue;
            }

            if (type == "let")
            {
                checkElement (element, path, { "id", "name", "value" }, false);

                const auto name = parseName (element, path);
                declare (scope, name, parseValue (element, "value", path, scope), path, path + ".value");
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

        Statements assignments;
        StringArray uses;

        for (const auto& property : properties)
        {
            if (! element.hasProperty (property))
                continue;

            const auto value = parseValue (element, property, path, scope);
            assignments.push_back ({ property + " = " + value.text + ";", path + "." + property });
            uses.addArray (value.uses);
        }

        if (! assignments.empty())
        {
            auto statements = selectLets (scope, uses);
            statements.insert (statements.end(), assignments.begin(), assignments.end());

            node->program = compileStatements (statements, path);
            node->visible = findOutput<bool> (node->program, "visible");
            node->opacity = findOutput<float> (node->program, "opacity");
            node->transform = findOutput<AffineTransform> (node->program, "transform");
            addSlot (*node, false);
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

        auto uses = from.uses;
        uses.addArray (to.uses);

        auto statements = selectLets (scope, uses);
        statements.push_back ({ "_start = int " + from.text + ";", path + ".from" });
        statements.push_back ({ "_end = int " + to.text + ";", path + ".to" });

        node->program = compileStatements (statements, path);
        node->start = findOutput<int> (node->program, "_start");
        node->end = findOutput<int> (node->program, "_end");
        addSlot (*node, false);

        auto inner = scope.enter();
        declare (inner, name, { "int " + from.text + " + _index", from.uses }, path, path + ".from");
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

        Statements own;
        StringArray geometry, uses;

        for (const auto& entry : shape->geometry)
        {
            const auto property = entry.upToFirstOccurrenceOf ("=", false, false);
            const auto fallback = entry.fromFirstOccurrenceOf ("=", false, false);
            const auto value = (fallback.isEmpty() || element.hasProperty (property)) ? parseValue (element, property, path, scope) : Value { "(" + fallback + ")", {} };

            geometry.add (property);
            own.push_back ({ "let _" + property + " = " + value.text + ";", path + "." + property });
            uses.addArray (value.uses);
        }

        own.push_back ({ shape->lowering, path });

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

            const auto value = parseValue (element, property, path, scope);
            own.push_back ({ output + " = " + value.text + ";", path + "." + property });
            uses.addArray (value.uses);
        }

        auto statements = selectLets (scope, uses);
        statements.insert (statements.end(), own.begin(), own.end());

        auto created = ExpressionDrawing::create (compileStatements (statements, path));
        if (created.failed())
            fail (path + ": " + created.getErrorMessage());

        auto node = std::make_unique<Node>();
        node->drawing = std::move (created.getReference());
        addSlot (*node, repeated);
        return node;
    }

    ExpressionContext& context;
    const DrawingScope& drawingScope;
    Compiled& result;
    std::map<String, PropertyInput> propertyInputs;
    StringArray verifiedLets;
    int numSymbols = 0;
};

//==============================================================================
namespace
{

// Every program declares the same types, inputs and outputs, so each starts from a copy of this
// context sharing its declarations instead of registering the graphics types again
struct DrawingBaseContext
{
    DrawingBaseContext()
    {
        ExpressionDrawing::declareOutputs (context);
        bounds = context.addInput<Rectangle<float>> ("bounds");
        font = context.addInput<Font> ("defaultFont");
        index = context.addInput<int> ("_index");
        context.declareOutput<int> ("_start");
        context.declareOutput<int> ("_end");
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
/* The compilations in use, by drawing content. Weak entries, so a compilation goes away with its last program. */
class DrawingProgram::CompiledCache
{
public:
    static CompiledCache& getInstance()
    {
        static CompiledCache cache;
        return cache;
    }

    /* Returns a compilation whose symbols resolve in the scope to the types they had when compiled, if any. */
    std::shared_ptr<const Compiled> find (const String& key, const DrawingScope& scope)
    {
        std::vector<std::shared_ptr<const Compiled>> candidates;

        {
            const ScopedLock sl (lock);

            if (const auto found = entries.find (key); found != entries.end())
            {
                for (const auto& entry : found->second)
                    if (auto candidate = entry.lock())
                        candidates.push_back (std::move (candidate));
            }
        }

        for (auto& candidate : candidates)
        {
            const auto resolvesAlike = std::all_of (candidate->symbols.begin(), candidate->symbols.end(), [&] (const auto& symbol)
            {
                return scope.resolveSymbol (symbol.first) == symbol.second;
            });

            if (resolvesAlike)
                return candidate;
        }

        return nullptr;
    }

    void add (const String& key, const std::shared_ptr<const Compiled>& compiled)
    {
        const ScopedLock sl (lock);

        // Every edit of a drawing makes a new key, so the unused ones are dropped as they come
        for (auto it = entries.begin(); it != entries.end();)
        {
            auto& list = it->second;
            list.erase (std::remove_if (list.begin(), list.end(), [] (const auto& entry) { return entry.expired(); }), list.end());
            it = list.empty() ? entries.erase (it) : std::next (it);
        }

        entries[key].push_back (compiled);
    }

    /* The text the compiler reads from a drawing tree: element types, property names and values, in order. */
    static String makeKey (const DataTree& drawing)
    {
        String key;
        appendKey (key, drawing);
        return key;
    }

private:
    static void appendKey (String& key, const DataTree& element)
    {
        key << element.getType().toString() << "{";

        for (int i = 0; i < element.getNumProperties(); ++i)
        {
            const auto name = element.getPropertyName (i);
            const auto value = element.getProperty (name).toString();

            key << name.toString() << "=" << (int) value.getNumBytesAsUTF8() << ":" << value;
        }

        for (const auto& child : element)
            appendKey (key, child);

        key << "}";
    }

    CriticalSection lock;
    std::unordered_map<String, std::vector<std::weak_ptr<const Compiled>>> entries;
};

//==============================================================================
DrawingProgram::DrawingProgram (Component& target)
    : context (getDrawingBaseContext().context)
    , scope (target, context)
{
}

DrawingProgram::~DrawingProgram() = default;

ResultValue<std::unique_ptr<DrawingProgram>> DrawingProgram::create (Component& target, const DataTree& drawing)
{
    std::unique_ptr<DrawingProgram> program (new DrawingProgram (target));

    auto& cache = CompiledCache::getInstance();
    const auto key = CompiledCache::makeKey (drawing);
    auto compiled = cache.find (key, program->scope);

    if (compiled == nullptr)
    {
        auto result = std::make_shared<Compiled>();

        if (const auto compiledResult = Compiler (program->context, program->scope, *result).compile (drawing); compiledResult.failed())
            return makeResultValueFail (compiledResult.getErrorMessage());

        cache.add (key, result);
        compiled = std::move (result);
    }

    program->instantiate (std::move (compiled));
    return makeResultValueOk (std::move (program));
}

void DrawingProgram::instantiate (std::shared_ptr<const Compiled> compiledToUse)
{
    compiled = std::move (compiledToUse);
    slots = std::make_unique<Slot[]> (compiled->evaluated.size());
    propertyValues.resize (compiled->properties.size());

    for (size_t i = 0; i < compiled->evaluated.size(); ++i)
    {
        const auto& node = *compiled->evaluated[i];
        auto& state = slots[i].state;

        state = std::make_unique<ExpressionState> (node.getProgram(), node.repeated ? 64 : 1);

        if (node.repeated)
            state->bindRowIndex (getDrawingBaseContext().index);
    }
}

//==============================================================================
void DrawingProgram::draw (Graphics& g, const ApplicationTheme& theme, const Component& component)
{
    for (size_t i = 0; i < propertyValues.size(); ++i)
        propertyValues[i] = component.findProperty (compiled->properties[i].name);

    drawNodes (g, compiled->nodes, component.getLocalBounds(), theme.getDefaultFont(), 1);
}

DrawingProgram::Slot* DrawingProgram::prepare (const Node& node, Rectangle<float> bounds, const Font& font)
{
    if (node.slot < 0)
        return nullptr;

    auto& slot = slots[(size_t) node.slot];
    const auto& base = getDrawingBaseContext();

    slot.state->set (base.bounds, bounds);
    slot.state->set (base.font, font);
    scope.updateInputs (node.getProgram(), *slot.state);

    for (const auto& input : node.properties)
    {
        const auto& value = propertyValues[(size_t) input.property];

        if (input.isSet)
            slot.state->set (ExpressionInput<bool> { input.input }, ! value.isVoid());
        else if (value.isVoid())
            continue;
        else if (compiled->properties[(size_t) input.property].type == ComponentProperty::Type::color)
            slot.state->set (ExpressionInput<Color> { input.input }, VariantConverter<Color>::fromVar (value));
        else
            slot.state->setVar (input.input, value);
    }

    return &slot;
}

const std::vector<ComponentProperty>& DrawingProgram::getProperties() const noexcept
{
    return compiled->properties;
}

void DrawingProgram::drawNodes (Graphics& g, const std::vector<std::unique_ptr<Node>>& children, Rectangle<float> bounds, const Font& font, int numRows)
{
    for (const auto& node : children)
        drawNode (g, *node, bounds, font, numRows);
}

void DrawingProgram::drawNode (Graphics& g, const Node& node, Rectangle<float> bounds, const Font& font, int numRows)
{
    constexpr int maxRepeatCount = 4096;

    auto* slot = prepare (node, bounds, font);

    if (node.kind == Node::Kind::shape)
    {
        node.drawing.draw (g, *slot->state, numRows, &font, &slot->cache);
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

        case Node::Kind::layer:
        {
            const auto opacity = node.opacity.isValid() ? jlimit (0.0f, 1.0f, slot->state->get (node.opacity)) : 1.0f;

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
