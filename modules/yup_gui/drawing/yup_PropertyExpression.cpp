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

template <class T>
var readOutput (const CompiledExpression& program, ExpressionState& state)
{
    auto output = program.getOutput<T> (outputName);

    if (output.failed())
        return {};

    if constexpr (std::is_same_v<T, Color>)
        return VariantConverter<Color>::toVar (state.get (output.getReference()));
    else if constexpr (std::is_same_v<T, float>)
        return static_cast<double> (state.get (output.getReference()));
    else
        return state.get (output.getReference());
}

} // namespace

//==============================================================================

ResultValue<std::unique_ptr<PropertyExpression>> PropertyExpression::compile (Component& component,
                                                                              const String& text,
                                                                              std::optional<ComponentProperty::Type> type,
                                                                              DrawingScope::ComponentResolver resolver)
{
    static const std::array<ComponentProperty::Type, 4> probed { ComponentProperty::Type::color,
                                                                 ComponentProperty::Type::number,
                                                                 ComponentProperty::Type::boolean,
                                                                 ComponentProperty::Type::string };

    std::vector<ComponentProperty::Type> candidates;

    if (type.has_value())
        candidates.push_back (*type);
    else
        candidates.assign (probed.begin(), probed.end());

    String error;

    for (const auto candidate : candidates)
    {
        std::unique_ptr<PropertyExpression> expression (new PropertyExpression());
        expression->source = text;
        expression->type = candidate;
        expression->context = std::make_unique<ExpressionContext>();

        addGraphicsExpressionTypes (*expression->context);

        switch (candidate)
        {
            case ComponentProperty::Type::color:   expression->context->declareOutput<Color> (outputName); break;
            case ComponentProperty::Type::integer: expression->context->declareOutput<int> (outputName); break;
            case ComponentProperty::Type::boolean: expression->context->declareOutput<bool> (outputName); break;
            case ComponentProperty::Type::string:  expression->context->declareOutput<String> (outputName); break;

            default:                               expression->context->declareOutput<float> (outputName); break;
        }

        expression->scope = std::make_unique<DrawingScope> (component, *expression->context);
        expression->scope->setComponentResolver (resolver);
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

PropertyExpression::~PropertyExpression() = default;

//==============================================================================

var PropertyExpression::evaluate()
{
    scope->updateInputs (*program, *state);
    state->evaluate();

    switch (type)
    {
        case ComponentProperty::Type::color:   return readOutput<Color> (*program, *state);
        case ComponentProperty::Type::integer: return readOutput<int> (*program, *state);
        case ComponentProperty::Type::boolean: return readOutput<bool> (*program, *state);
        case ComponentProperty::Type::string:  return readOutput<String> (*program, *state);

        default:                               return readOutput<float> (*program, *state);
    }
}

bool PropertyExpression::hasInputs() const noexcept
{
    return program->getNumInputs() > 0;
}

std::vector<DrawingScope::Dependency> PropertyExpression::getDependencies() const
{
    return scope->getDependencies (*program);
}

} // namespace yup
