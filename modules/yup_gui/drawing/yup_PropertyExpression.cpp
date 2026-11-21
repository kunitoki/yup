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

constexpr const char* outputName = "_value";

/* Drops the "line:column: " an expression parser error starts with, so only the caller gives positions. */
String withoutPosition (const String& message)
{
    const auto position = message.upToFirstOccurrenceOf (": ", false, false);

    return position.isNotEmpty() && position.containsOnly ("0123456789:")
             ? message.fromFirstOccurrenceOf (": ", false, false)
             : message;
}

// Registering the graphics types is far slower than compiling a typical value, so every expression
// starts from a copy of this context, which shares its types and declarations
const ExpressionContext& getPropertyExpressionBaseContext()
{
    static const ExpressionContext context = []
    {
        ExpressionContext result;
        addGraphicsExpressionTypes (result);
        DrawingScope::addExpressionArrayTypes (result);
        return result;
    }();

    return context;
}

// A font file is loaded once, and again when it changes. A file that can't be loaded gives the theme font
Font loadFontFile (const String& path, const File& baseDirectory)
{
    const auto folder = baseDirectory != File() ? baseDirectory : File::getCurrentWorkingDirectory();
    const auto file = File::isAbsolutePath (path) ? File (path) : folder.getChildFile (path);
    const auto modified = file.getLastModificationTime();

    static std::mutex mutex;
    static std::map<String, std::pair<Time, Font>> cache;

    const std::scoped_lock lock (mutex);
    const auto [entry, isNew] = cache.try_emplace (file.getFullPathName());

    if (isNew || entry->second.first != modified)
    {
        auto loaded = Font::loadFontFromFile (file);
        entry->second = { modified, loaded.wasOk() ? loaded.getValue() : Font() };
    }

    if (! entry->second.second.isEmpty())
        return entry->second.second;

    const auto theme = ApplicationTheme::getGlobalTheme();
    return theme != nullptr ? theme->getDefaultFont() : Font();
}

void addFontFiles (ExpressionContext& context, const File& baseDirectory)
{
    context.addFunction ("font", [baseDirectory] (const String& path)
    {
        return loadFontFile (path, baseDirectory);
    });

    context.addFunction ("font", [baseDirectory] (const String& path, float height)
    {
        return loadFontFile (path, baseDirectory).withHeight (height);
    });
}

// An integer takes any number, cut to a whole one as `int (...)` cuts it: the numbers of a model are never
// integers by type. A double holds every integer as it is
int cutToInteger (double value) noexcept
{
    if (std::isnan (value))
        return 0;

    return static_cast<int> (jlimit (static_cast<double> (std::numeric_limits<int>::min()), static_cast<double> (std::numeric_limits<int>::max()), value));
}

// Types that aren't bridged are compiled as numbers, which is what an untyped value falls back to
void declareValueOutput (ExpressionContext& context, ComponentProperty::Type type)
{
    if (type == ComponentProperty::Type::integer)
    {
        context.declareOutput<double> (outputName);
        return;
    }

    if (! DrawingScope::withExpressionType (type, [&] (auto tag)
        {
            context.declareOutput<typename decltype (tag)::type> (outputName);
        }))
    {
        context.declareOutput<float> (outputName);
    }
}

std::optional<ComponentProperty::Type> findMatchingExpressionType (const DrawingScope& scope, const ExpressionType& target)
{
    for (const auto candidate : ComponentProperty::expressionTypes)
        if (scope.getExpressionType (candidate) == &target)
            return candidate;

    return std::nullopt;
}

var readValueOutput (const CompiledExpression& program,
                     ExpressionState& state,
                     ComponentProperty::Type type,
                     ComponentProperty::Type elementType)
{
    if (type == ComponentProperty::Type::array)
    {
        var value;

        DrawingScope::withExpressionType (elementType, [&] (auto tag)
        {
            using Type = typename decltype (tag)::type;

            const auto view = state.get (ExpressionOutput<ExpressionArrayView<Type>> { 0 });
            Array<var> values;
            values.ensureStorageAllocated (view.size);

            for (int i = 0; i < view.size; ++i)
                values.add (VariantConverter<Type>::toVar (view.data[i]));

            value = var (std::move (values));
        });

        return value;
    }

    if (type == ComponentProperty::Type::number)
    {
        const auto* outputType = program.getOutputType (0);

        if (outputType != nullptr && outputType->is<double>())
            return state.get (ExpressionOutput<double> { 0 });

        if (outputType != nullptr && outputType->is<int>())
            return static_cast<double> (state.get (ExpressionOutput<int> { 0 }));

        return VariantConverter<float>::toVar (state.get (ExpressionOutput<float> { 0 }));
    }

    if (type == ComponentProperty::Type::integer)
        return cutToInteger (state.get (ExpressionOutput<double> { 0 }));

    var value;

    if (! DrawingScope::withExpressionType (type, [&] (auto tag)
        {
            using Type = typename decltype (tag)::type;
            value = VariantConverter<Type>::toVar (state.get (ExpressionOutput<Type> { 0 }));
        }))
    {
        value = VariantConverter<float>::toVar (state.get (ExpressionOutput<float> { 0 }));
    }

    return value;
}

} // namespace

//==============================================================================

PropertyExpression::PropertyExpression (ExpressionContext contextToUse)
    : context (std::move (contextToUse))
{
}

ExpressionContext PropertyExpression::createContext (const File& baseDirectory)
{
    auto context = getPropertyExpressionBaseContext();
    addFontFiles (context, baseDirectory);

    return context;
}

ResultValue<std::unique_ptr<PropertyExpression>> PropertyExpression::compile (Component& component,
                                                                              const String& text,
                                                                              std::optional<ComponentProperty::Type> type,
                                                                              DrawingScope::ComponentResolver resolver,
                                                                              Component* locals,
                                                                              const File& baseDirectory)
{
    if (text.trimStart().startsWithChar ('['))
        return compileArrayLiteral (component, text, type, std::move (resolver), locals, baseDirectory);

    std::unique_ptr<PropertyExpression> expression (new PropertyExpression (getPropertyExpressionBaseContext()));
    expression->source = text;

    // Adding functions copies the shared declarations, so only the texts that can load a font pay for it
    if (text.contains ("font"))
        addFontFiles (expression->context, baseDirectory);

    const bool hasFixedScalarType = type.has_value()
                                 && *type != ComponentProperty::Type::array
                                 && *type != ComponentProperty::Type::any;

    if (hasFixedScalarType)
    {
        expression->type = *type;
        declareValueOutput (expression->context, *type);
    }

    expression->scope.emplace (component, expression->context);
    expression->scope->setComponentResolver (std::move (resolver));
    expression->scope->setLocals (locals);
    expression->scope->install();

    const String expressionText = (type == ComponentProperty::Type::transform && text.trim() == "none") ? String ("transform()") : text;
    auto compiled = CompiledExpression::compile (String (outputName) + " = (" + expressionText + "\n);\n", expression->context);
    if (compiled.failed())
        return makeResultValueFail (compiled.getErrorMessage());

    expression->program = compiled.getReference();

    if (! hasFixedScalarType)
    {
        const auto* outputType = expression->program.getOutputType (0);
        jassert (outputType != nullptr);

        if (outputType == nullptr)
            return makeResultValueFail ("Expression produced no output");

        if (const auto* elem = outputType->getElementType())
        {
            const auto matchedElement = findMatchingExpressionType (*expression->scope, *elem);
            if (! matchedElement.has_value())
                return makeResultValueFail ("Unsupported array element type '" + elem->getName() + "'");

            expression->type = ComponentProperty::Type::array;
            expression->elementType = *matchedElement;
        }
        else if (type == ComponentProperty::Type::array)
        {
            return makeResultValueFail ("Cannot convert '" + outputType->getName() + "' to an array");
        }
        else if (outputType->is<float>() || outputType->is<double>() || outputType->is<int>())
        {
            expression->type = ComponentProperty::Type::number;
        }
        else if (const auto matched = findMatchingExpressionType (*expression->scope, *outputType))
        {
            expression->type = *matched;
        }
        else
        {
            return makeResultValueFail ("Unsupported expression type '" + outputType->getName() + "'");
        }
    }

    expression->state.emplace (expression->program, 1);
    expression->inputs = expression->scope->bindInputs (expression->program);
    expression->paletteDependent = expression->inputs.readsPalette();

    return makeResultValueOk (std::move (expression));
}

ResultValue<std::unique_ptr<PropertyExpression>> PropertyExpression::compileArrayLiteral (Component& component,
                                                                                          const String& text,
                                                                                          std::optional<ComponentProperty::Type> type,
                                                                                          DrawingScope::ComponentResolver resolver,
                                                                                          Component* locals,
                                                                                          const File& baseDirectory)
{
    if (type.has_value() && *type != ComponentProperty::Type::array && *type != ComponentProperty::Type::any)
        return makeResultValueFail ("An array can't be the value of this property");

    StringArray elementTexts;
    int offset = 0;

    if (auto result = splitArrayLiteral (text, elementTexts, offset); result.failed())
        return makeResultValueFail (result.getErrorMessage());

    const auto* raw = text.toRawUTF8();
    for (;;)
    {
        if (raw[offset] != 0 && CharacterFunctions::isWhitespace (raw[offset]))
        {
            ++offset;
        }
        else if (raw[offset] == '/' && raw[offset + 1] == '/')
        {
            offset += 2;
            while (raw[offset] != 0 && raw[offset] != '\n')
                ++offset;
        }
        else if (raw[offset] == '/' && raw[offset + 1] == '*')
        {
            const auto* end = std::strstr (raw + offset + 2, "*/");
            if (end == nullptr)
                return makeResultValueFail ("Unterminated comment");

            offset = static_cast<int> (end - raw) + 2;
        }
        else
        {
            break;
        }
    }

    if (raw[offset] != 0)
        return makeResultValueFail ("Expected the end of the array");

    std::unique_ptr<PropertyExpression> expression (new PropertyExpression());
    expression->source = text;
    expression->type = ComponentProperty::Type::array;

    for (const auto& elementText : elementTexts)
    {
        auto element = compile (component, elementText, std::nullopt, resolver, locals, baseDirectory);

        if (element.failed())
            return makeResultValueFail (element.getErrorMessage());

        expression->paletteDependent |= element.getReference()->readsPalette();
        expression->elements.push_back (std::move (element.getReference()));
    }

    return makeResultValueOk (std::move (expression));
}

Result PropertyExpression::splitArrayLiteral (StringRef text, StringArray& elements, int& offset)
{
    const auto* chars = text.text.getAddress();

    const auto skipSpaces = [&] (int& pos) -> Result
    {
        for (;;)
        {
            if (chars[pos] != 0 && CharacterFunctions::isWhitespace (chars[pos]))
            {
                ++pos;
            }
            else if (chars[pos] == '/' && chars[pos + 1] == '/')
            {
                pos += 2;
                while (chars[pos] != 0 && chars[pos] != '\n')
                    ++pos;
            }
            else if (chars[pos] == '/' && chars[pos + 1] == '*')
            {
                const auto* end = std::strstr (chars + pos + 2, "*/");
                if (end == nullptr)
                    return Result::fail ("Unterminated comment");

                pos = static_cast<int> (end - chars) + 2;
            }
            else
            {
                return Result::ok();
            }
        }
    };

    auto position = offset;
    if (auto res = skipSpaces (position); res.failed())
    {
        offset = position;
        return res;
    }

    if (chars[position] != '[')
    {
        offset = position;
        return Result::fail ("Expected '['");
    }

    ++position;
    if (auto res = skipSpaces (position); res.failed())
    {
        offset = position;
        return res;
    }

    if (chars[position] != ']')
    {
        for (;;)
        {
            if (auto res = skipSpaces (position); res.failed())
            {
                offset = position;
                return res;
            }

            const auto elementStart = position;

            if (chars[elementStart] == '[')
            {
                StringArray nested;

                if (auto result = splitArrayLiteral (text, nested, position); result.failed())
                {
                    offset = position;
                    return result;
                }
            }
            else
            {
                const auto parsed = Expression::parseExpression (text, position);

                if (parsed.failed())
                {
                    offset = position;
                    return Result::fail (withoutPosition (parsed.getErrorMessage()));
                }
            }

            elements.add (String::fromUTF8 (chars + elementStart, position - elementStart).trimEnd());

            if (auto res = skipSpaces (position); res.failed())
            {
                offset = position;
                return res;
            }

            if (chars[position] != ',')
                break;

            ++position;
        }

        if (chars[position] != ']')
        {
            offset = position;
            return Result::fail ("Expected ',' or ']'");
        }
    }

    offset = position + 1;
    return Result::ok();
}

PropertyExpression::~PropertyExpression() = default;

//==============================================================================

var PropertyExpression::evaluate()
{
    if (! state.has_value())
    {
        Array<var> values;
        values.ensureStorageAllocated (static_cast<int> (elements.size()));

        for (const auto& element : elements)
            values.add (element->evaluate());

        return var (std::move (values));
    }

    scope->updateInputs (inputs, *state);
    state->evaluate();

    return readValueOutput (program, *state, type, elementType);
}

bool PropertyExpression::hasInputs() const noexcept
{
    if (! state.has_value())
        return std::any_of (elements.begin(), elements.end(), [] (const auto& element)
        {
            return element->hasInputs();
        });

    return ! inputs.isEmpty();
}

std::vector<DrawingScope::Dependency> PropertyExpression::getDependencies() const
{
    if (! state.has_value())
    {
        std::vector<DrawingScope::Dependency> dependencies;

        for (const auto& element : elements)
            for (const auto& dependency : element->getDependencies())
                dependencies.push_back (dependency);

        return dependencies;
    }

    return scope->getDependencies (program);
}

} // namespace yup
