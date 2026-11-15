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

// The types a value can have, in the order they are tried when the property doesn't name one
constexpr std::array<ComponentProperty::Type, 6> probedTypes { ComponentProperty::Type::color,
                                                               ComponentProperty::Type::number,
                                                               ComponentProperty::Type::boolean,
                                                               ComponentProperty::Type::string,
                                                               ComponentProperty::Type::point,
                                                               ComponentProperty::Type::rectangle };

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

template <class T>
var toVar (const T& value)
{
    if constexpr (std::is_same_v<T, float>)
        return static_cast<double> (value);
    else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, bool> || std::is_same_v<T, String>)
        return value;
    else
        return VariantConverter<T>::toVar (value);
}

template <class T>
var readOutput (const CompiledExpression& program, ExpressionState& state)
{
    auto output = program.getOutput<T> (outputName);

    if (output.failed())
        return {};

    return toVar (state.get (output.getReference()));
}

template <class T>
var readArrayOutput (const CompiledExpression& program, ExpressionState& state)
{
    auto output = program.getOutput<ExpressionArrayView<T>> (outputName);

    if (output.failed())
        return {};

    const auto view = state.get (output.getReference());

    Array<var> values;
    values.ensureStorageAllocated (view.size);

    for (int i = 0; i < view.size; ++i)
        values.add (toVar (view.data[i]));

    return var (values);
}

// Types that aren't bridged are compiled as numbers, which is what an untyped value falls back to
void declareValueOutput (ExpressionContext& context, ComponentProperty::Type type, ComponentProperty::Type elementType)
{
    if (type == ComponentProperty::Type::array)
    {
        DrawingScope::withExpressionType (elementType, [&] (auto tag)
        {
            context.declareOutput<ExpressionArrayView<typename decltype (tag)::type>> (outputName);
        });

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

var readValueOutput (const CompiledExpression& program,
                     ExpressionState& state,
                     ComponentProperty::Type type,
                     ComponentProperty::Type elementType)
{
    var value;

    if (type == ComponentProperty::Type::array)
    {
        DrawingScope::withExpressionType (elementType, [&] (auto tag)
        {
            value = readArrayOutput<typename decltype (tag)::type> (program, state);
        });

        return value;
    }

    if (! DrawingScope::withExpressionType (type, [&] (auto tag)
        {
            value = readOutput<typename decltype (tag)::type> (program, state);
        }))
    {
        value = readOutput<float> (program, state);
    }

    return value;
}

} // namespace

//==============================================================================

ResultValue<std::unique_ptr<PropertyExpression>> PropertyExpression::compile (Component& component,
                                                                              const String& text,
                                                                              std::optional<ComponentProperty::Type> type,
                                                                              DrawingScope::ComponentResolver resolver,
                                                                              DrawingScope::ModelResolver modelResolver,
                                                                              BindingProvider* locals)
{
    struct Candidate
    {
        ComponentProperty::Type type;
        ComponentProperty::Type elementType = ComponentProperty::Type::any;
    };

    if (text.trimStart().startsWithChar ('['))
        return compileArrayLiteral (component, text, type, std::move (resolver), std::move (modelResolver), locals);

    std::vector<Candidate> candidates;

    if (type.has_value() && *type != ComponentProperty::Type::array)
    {
        candidates.push_back ({ *type });
    }
    else
    {
        if (! type.has_value())
            for (const auto probed : probedTypes)
                candidates.push_back ({ probed });

        // Without the element type, every array a value could be is tried
        for (const auto probed : probedTypes)
            candidates.push_back ({ ComponentProperty::Type::array, probed });
    }

    String error;

    for (const auto candidate : candidates)
    {
        std::unique_ptr<PropertyExpression> expression (new PropertyExpression());
        expression->source = text;
        expression->type = candidate.type;
        expression->elementType = candidate.elementType;
        expression->context = std::make_unique<ExpressionContext> (getPropertyExpressionBaseContext());

        declareValueOutput (*expression->context, candidate.type, candidate.elementType);

        expression->scope = std::make_unique<DrawingScope> (component, *expression->context);
        expression->scope->setComponentResolver (resolver);
        expression->scope->setModelResolver (modelResolver);
        expression->scope->setLocals (locals);
        expression->scope->install();

        auto compiled = CompiledExpression::compile (String (outputName) + " = (" + text + ");\n", *expression->context);

        if (compiled.failed())
        {
            if (error.isEmpty())
                error = compiled.getErrorMessage();

            continue;
        }

        expression->program = std::make_unique<CompiledExpression> (compiled.getReference());
        expression->state = std::make_unique<ExpressionState> (*expression->program, 1);

        for (int i = 0; i < expression->program->getNumInputs(); ++i)
            if (expression->program->getInputName (i).startsWith ("palette."))
                expression->paletteDependent = true;

        return makeResultValueOk (std::move (expression));
    }

    return makeResultValueFail (error);
}

ResultValue<std::unique_ptr<PropertyExpression>> PropertyExpression::compileArrayLiteral (Component& component,
                                                                                          const String& text,
                                                                                          std::optional<ComponentProperty::Type> type,
                                                                                          DrawingScope::ComponentResolver resolver,
                                                                                          DrawingScope::ModelResolver modelResolver,
                                                                                          BindingProvider* locals)
{
    if (type.has_value() && *type != ComponentProperty::Type::array && *type != ComponentProperty::Type::any)
        return makeResultValueFail ("An array can't be the value of this property");

    StringArray elementTexts;
    int offset = 0;

    if (auto result = splitArrayLiteral (text, elementTexts, offset); result.failed())
        return makeResultValueFail (result.getErrorMessage());

    if (String::fromUTF8 (text.toRawUTF8() + offset).trim().isNotEmpty())
        return makeResultValueFail ("Expected the end of the array");

    std::unique_ptr<PropertyExpression> expression (new PropertyExpression());
    expression->source = text;
    expression->type = ComponentProperty::Type::array;

    for (const auto& elementText : elementTexts)
    {
        auto element = compile (component, elementText, std::nullopt, resolver, modelResolver, locals);

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

    const auto skipSpaces = [&] (int from)
    {
        while (chars[from] != 0 && CharacterFunctions::isWhitespace (chars[from]))
            ++from;

        return from;
    };

    auto position = skipSpaces (offset);

    if (chars[position] != '[')
    {
        offset = position;
        return Result::fail ("Expected '['");
    }

    position = skipSpaces (position + 1);

    if (chars[position] != ']')
    {
        for (;;)
        {
            const auto elementStart = skipSpaces (position);

            if (chars[elementStart] == '[')
            {
                StringArray nested;
                position = elementStart;

                if (auto result = splitArrayLiteral (text, nested, position); result.failed())
                {
                    offset = position;
                    return result;
                }
            }
            else
            {
                position = elementStart;
                const auto parsed = Expression::parseExpression (text, position);

                if (parsed.failed())
                {
                    offset = position;
                    return Result::fail (withoutPosition (parsed.getErrorMessage()));
                }
            }

            elements.add (String::fromUTF8 (chars + elementStart, position - elementStart).trimEnd());
            position = skipSpaces (position);

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
    if (program == nullptr)
    {
        Array<var> values;
        values.ensureStorageAllocated (static_cast<int> (elements.size()));

        for (const auto& element : elements)
            values.add (element->evaluate());

        return var (std::move (values));
    }

    scope->updateInputs (*program, *state);
    state->evaluate();

    return readValueOutput (*program, *state, type, elementType);
}

bool PropertyExpression::hasInputs() const noexcept
{
    if (program == nullptr)
        return std::any_of (elements.begin(), elements.end(), [] (const auto& element)
        {
            return element->hasInputs();
        });

    return program->getNumInputs() > 0;
}

std::vector<DrawingScope::Dependency> PropertyExpression::getDependencies() const
{
    if (program == nullptr)
    {
        std::vector<DrawingScope::Dependency> dependencies;

        for (const auto& element : elements)
            for (const auto& dependency : element->getDependencies())
                dependencies.push_back (dependency);

        return dependencies;
    }

    return scope->getDependencies (*program);
}

} // namespace yup
