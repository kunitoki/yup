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

bool isFilterIdentifierStart (yup_wchar c) noexcept
{
    return CharacterFunctions::isLetter (c);
}

bool isFilterIdentifierBody (yup_wchar c) noexcept
{
    return CharacterFunctions::isLetterOrDigit (c) || c == '-' || c == '_';
}

std::optional<float> parseCssLengthLiteral (const String& rawText)
{
    const auto text = rawText.trim();
    if (text.isEmpty() || text.endsWithChar ('%'))
        return std::nullopt;

    String numeric = text;
    if (numeric.endsWithIgnoreCase ("px") || numeric.endsWithIgnoreCase ("pt"))
        numeric = numeric.dropLastCharacters (2).trimEnd();

    return detail::parseStrictFloat (numeric, true, false);
}

std::optional<float> parseCssNumberPercentageLiteral (const String& rawText)
{
    const auto text = rawText.trim();
    if (text.isEmpty())
        return std::nullopt;

    if (text.endsWithChar ('%'))
    {
        const auto pct = detail::parseStrictDouble (text.dropLastCharacters (1).trimEnd(), true);
        if (! pct.has_value())
            return std::nullopt;

        return static_cast<float> (*pct / 100.0);
    }

    return detail::parseStrictFloat (text, true, false);
}

struct AngleParseOutcome
{
    bool isNumericWithoutUnit = false;
    std::optional<float> radians;
};

AngleParseOutcome parseCssAngleLiteral (const String& rawText)
{
    const auto text = rawText.trim();
    if (text.isEmpty())
        return {};

    const auto trySuffix = [&] (const char* suffix, int suffixLen, double scale) -> std::optional<float>
    {
        if (! text.endsWithIgnoreCase (suffix))
            return std::nullopt;

        const auto parsed = detail::parseStrictDouble (text.dropLastCharacters (suffixLen).trimEnd(), true);
        if (! parsed.has_value())
            return std::nullopt;

        return static_cast<float> (*parsed * scale);
    };

    if (const auto r = trySuffix ("turn", 4, MathConstants<double>::twoPi))
        return { false, r };
    if (const auto r = trySuffix ("grad", 4, MathConstants<double>::pi / 200.0))
        return { false, r };
    if (const auto r = trySuffix ("rad", 3, 1.0))
        return { false, r };
    if (const auto r = trySuffix ("deg", 3, MathConstants<double>::pi / 180.0))
        return { false, r };

    if (const auto bare = detail::parseStrictDouble (text, true); bare.has_value())
    {
        if (*bare == 0.0)
            return { false, 0.0f };

        return { true, std::nullopt };
    }

    return {};
}

std::optional<Color> parseLiteralCssColor (const String& rawText)
{
    const auto text = rawText.trim();
    if (text.isEmpty())
        return std::nullopt;

    if (text.startsWithChar ('#'))
    {
        const auto hex = text.substring (1);
        if (! hex.containsOnly ("0123456789abcdefABCDEF"))
            return std::nullopt;

        if (hex.length() == 4)
        {
            const auto expand = [] (yup_wchar ch) -> uint8
            {
                const int v = CharacterFunctions::getHexDigitValue (ch);
                return static_cast<uint8> ((v << 4) | v);
            };
            return Color (expand (hex[3]), expand (hex[0]), expand (hex[1]), expand (hex[2]));
        }

        if (hex.length() == 3 || hex.length() == 6 || hex.length() == 8)
            return Color::fromString (text);

        return std::nullopt;
    }

    if (text.equalsIgnoreCase ("transparent"))
        return Colors::transparentBlack;

    if (text.startsWithIgnoreCase ("rgb(")
        || text.startsWithIgnoreCase ("rgba(")
        || text.startsWithIgnoreCase ("hsl(")
        || text.startsWithIgnoreCase ("hsla("))
    {
        return Color::fromString (text);
    }

    const auto named = Colors::getNamedColor (text);
    if (named != Colors::transparentBlack)
        return named;

    return std::nullopt;
}

ColorMatrixEffect::Matrix matrixForOperation (const FilterEffect::Operation& op) noexcept
{
    using Type = FilterEffect::Type;

    switch (op.type)
    {
        case Type::brightness:
            return ColorMatrixEffect::brightnessMatrix (op.amount);
        case Type::contrast:
            return ColorMatrixEffect::contrastMatrix (op.amount);
        case Type::grayscale:
            return ColorMatrixEffect::grayscaleMatrix (op.amount);
        case Type::hueRotate:
            return ColorMatrixEffect::hueRotateMatrix (op.amount);
        case Type::invert:
            return ColorMatrixEffect::invertMatrix (op.amount);
        case Type::opacity:
            return ColorMatrixEffect::opacityMatrix (op.amount);
        case Type::saturate:
            return ColorMatrixEffect::saturateMatrix (op.amount);
        case Type::sepia:
            return ColorMatrixEffect::sepiaMatrix (op.amount);
        case Type::blur:
        case Type::dropShadow:
            break;
    }

    return ColorMatrixEffect::identityMatrix();
}

} // namespace

//==============================================================================

bool FilterEffect::Operation::isNoOp() const noexcept
{
    switch (type)
    {
        case Type::blur:
            return amount <= 0.0001f;

        case Type::brightness:
        case Type::contrast:
        case Type::opacity:
        case Type::saturate:
            return approximatelyEqual (amount, 1.0f);

        case Type::grayscale:
        case Type::hueRotate:
        case Type::invert:
        case Type::sepia:
            return std::abs (amount) <= 0.0001f;

        case Type::dropShadow:
            return color.isTransparent();
    }

    return true;
}

FilterEffect::Operation FilterEffect::Operation::blur (float radiusPoints) noexcept
{
    return { Type::blur, jmax (0.0f, radiusPoints), {}, Colors::transparentBlack };
}

FilterEffect::Operation FilterEffect::Operation::brightness (float factor) noexcept
{
    return { Type::brightness, jmax (0.0f, factor), {}, Colors::transparentBlack };
}

FilterEffect::Operation FilterEffect::Operation::contrast (float factor) noexcept
{
    return { Type::contrast, jmax (0.0f, factor), {}, Colors::transparentBlack };
}

FilterEffect::Operation FilterEffect::Operation::dropShadow (Point<float> shadowOffset,
                                                             float radiusPoints,
                                                             Color shadowColor) noexcept
{
    return { Type::dropShadow, jmax (0.0f, radiusPoints), shadowOffset, shadowColor };
}

FilterEffect::Operation FilterEffect::Operation::grayscale (float amount) noexcept
{
    return { Type::grayscale, jlimit (0.0f, 1.0f, amount), {}, Colors::transparentBlack };
}

FilterEffect::Operation FilterEffect::Operation::hueRotate (float angleRadians) noexcept
{
    return { Type::hueRotate, angleRadians, {}, Colors::transparentBlack };
}

FilterEffect::Operation FilterEffect::Operation::invert (float amount) noexcept
{
    return { Type::invert, jlimit (0.0f, 1.0f, amount), {}, Colors::transparentBlack };
}

FilterEffect::Operation FilterEffect::Operation::opacity (float amount) noexcept
{
    return { Type::opacity, jlimit (0.0f, 1.0f, amount), {}, Colors::transparentBlack };
}

FilterEffect::Operation FilterEffect::Operation::saturate (float factor) noexcept
{
    return { Type::saturate, jmax (0.0f, factor), {}, Colors::transparentBlack };
}

FilterEffect::Operation FilterEffect::Operation::sepia (float amount) noexcept
{
    return { Type::sepia, jlimit (0.0f, 1.0f, amount), {}, Colors::transparentBlack };
}

//==============================================================================

FilterEffect::FilterEffect() noexcept = default;

FilterEffect::FilterEffect (std::vector<Operation> initialOperations)
    : operations (std::move (initialOperations))
{
}

FilterEffect::~FilterEffect() = default;

float FilterEffect::clampAmount (Type type, float value) noexcept
{
    switch (type)
    {
        case Type::blur:
        case Type::brightness:
        case Type::contrast:
        case Type::dropShadow:
        case Type::saturate:
            return jmax (0.0f, value);

        case Type::grayscale:
        case Type::invert:
        case Type::opacity:
        case Type::sepia:
            return jlimit (0.0f, 1.0f, value);

        case Type::hueRotate:
            return value;
    }

    return value;
}

ResultValue<std::vector<FilterEffect::ParsedStep>> FilterEffect::parseSteps (StringRef filterText, bool allowExpressions)
{
    const auto stripped = detail::stripStyleComments (filterText);
    if (stripped.failed())
        return makeResultValueFail (stripped.getErrorMessage() + " in filter");

    const String text = stripped.getReference().trim();
    std::vector<ParsedStep> steps;

    if (text.isEmpty() || text == "none")
        return makeResultValueOk (std::move (steps));

    const int length = text.length();
    int pos = 0;

    while (pos < length)
    {
        while (pos < length && CharacterFunctions::isWhitespace (text[pos]))
            ++pos;

        if (pos >= length)
            break;

        if (! isFilterIdentifierStart (text[pos]))
            return ResultValue<std::vector<ParsedStep>>::fail ("Unexpected token in filter at '" + text.substring (pos) + "'");

        const int nameStart = pos;
        while (pos < length && isFilterIdentifierBody (text[pos]))
            ++pos;

        const String funcName = text.substring (nameStart, pos);

        while (pos < length && CharacterFunctions::isWhitespace (text[pos]))
            ++pos;

        if (pos >= length || text[pos] != '(')
            return ResultValue<std::vector<ParsedStep>>::fail ("Expected '(' after filter function '" + funcName + "'");

        const int argStart = ++pos;
        int depth = 1;
        yup_wchar quote = 0;

        while (pos < length && depth > 0)
        {
            const auto ch = text[pos];
            if (quote != 0)
            {
                if (ch == '\\' && pos + 1 < length)
                    ++pos;
                else if (ch == quote)
                    quote = 0;
            }
            else if (ch == '"' || ch == '\'')
            {
                quote = ch;
            }
            else if (ch == '(')
            {
                ++depth;
            }
            else if (ch == ')')
            {
                --depth;
            }

            ++pos;
        }

        if (depth != 0)
            return ResultValue<std::vector<ParsedStep>>::fail ("Unclosed '(' in filter '" + funcName + "'");

        const String argText = text.substring (argStart, pos - 1).trim();

        if (funcName == "drop-shadow")
        {
            const auto entries = detail::tokenizeStyleValue (argText);
            if (entries.size() != 1 || entries.front().size() < 2)
                return ResultValue<std::vector<ParsedStep>>::fail ("drop-shadow() expects 'offset-x offset-y [blur-radius] [color]'");

            const auto& tokens = entries.front();
            ParsedStep step;
            step.type = Type::dropShadow;
            step.amount = 0.0f;
            String colorText;

            if (parseCssLengthLiteral (tokens[0]).has_value())
            {
                float parsedLengths[4] {};
                int lengths = 0;

                while (lengths < jmin (4, tokens.size()))
                {
                    const auto parsedLength = parseCssLengthLiteral (tokens[lengths]);
                    if (! parsedLength.has_value())
                        break;

                    parsedLengths[lengths++] = *parsedLength;
                }

                if (lengths < 2 || lengths > 3)
                    return ResultValue<std::vector<ParsedStep>>::fail ("drop-shadow() expects 2 or 3 lengths (offset-x, offset-y, optional blur-radius)");

                if (lengths == 3 && parsedLengths[2] < 0.0f)
                    return ResultValue<std::vector<ParsedStep>>::fail ("drop-shadow() blur-radius cannot be negative");

                step.offset = { parsedLengths[0], parsedLengths[1] };
                step.amount = lengths == 3 ? parsedLengths[2] : 0.0f;
                colorText = tokens.joinIntoString (" ", lengths).trim();
            }
            else
            {
                int trailingLengths = 0;
                float parsedTail[4] {};

                for (int idx = tokens.size() - 1; idx >= 1 && trailingLengths < 4; --idx)
                {
                    const auto parsedLength = parseCssLengthLiteral (tokens[idx]);
                    if (! parsedLength.has_value())
                        break;

                    parsedTail[trailingLengths++] = *parsedLength;
                }

                if (trailingLengths < 2 || trailingLengths > 3)
                    return ResultValue<std::vector<ParsedStep>>::fail ("drop-shadow() expects 2 or 3 lengths (offset-x, offset-y, optional blur-radius)");

                if (trailingLengths == 2)
                {
                    step.offset = { parsedTail[1], parsedTail[0] };
                    step.amount = 0.0f;
                }
                else
                {
                    if (parsedTail[0] < 0.0f)
                        return ResultValue<std::vector<ParsedStep>>::fail ("drop-shadow() blur-radius cannot be negative");

                    step.offset = { parsedTail[2], parsedTail[1] };
                    step.amount = parsedTail[0];
                }

                colorText = tokens.joinIntoString (" ", 0, tokens.size() - trailingLengths).trim();
            }

            if (colorText.isNotEmpty())
            {
                if (const auto literalColor = parseLiteralCssColor (colorText); literalColor.has_value())
                    step.color = *literalColor;
                else if (allowExpressions)
                    step.colorExpression = colorText;
                else
                    return ResultValue<std::vector<ParsedStep>>::fail ("Invalid color '" + colorText + "' in drop-shadow()");
            }

            steps.push_back (std::move (step));
            continue;
        }

        enum class ParamKind
        {
            length,
            numberPercentage,
            angle
        };

        struct FunctionInfo
        {
            const char* name;
            Type type;
            ParamKind paramKind;
            float defaultAmount;
        };

        static constexpr FunctionInfo functions[] = {
            { "blur", Type::blur, ParamKind::length, 0.0f },
            { "brightness", Type::brightness, ParamKind::numberPercentage, 1.0f },
            { "contrast", Type::contrast, ParamKind::numberPercentage, 1.0f },
            { "grayscale", Type::grayscale, ParamKind::numberPercentage, 1.0f },
            { "hue-rotate", Type::hueRotate, ParamKind::angle, 0.0f },
            { "invert", Type::invert, ParamKind::numberPercentage, 1.0f },
            { "opacity", Type::opacity, ParamKind::numberPercentage, 1.0f },
            { "saturate", Type::saturate, ParamKind::numberPercentage, 1.0f },
            { "sepia", Type::sepia, ParamKind::numberPercentage, 1.0f }
        };

        const FunctionInfo* info = nullptr;
        for (const auto& candidate : functions)
        {
            if (funcName == candidate.name)
            {
                info = &candidate;
                break;
            }
        }

        if (info == nullptr)
            return ResultValue<std::vector<ParsedStep>>::fail ("Unknown filter function '" + funcName + "'");

        ParsedStep step;
        step.type = info->type;
        step.amount = info->defaultAmount;

        if (argText.isNotEmpty())
        {
            if (info->paramKind == ParamKind::length)
            {
                if (argText.endsWithChar ('%'))
                    return ResultValue<std::vector<ParsedStep>>::fail ("blur() expects a length, not a percentage");

                if (const auto val = parseCssLengthLiteral (argText); val.has_value())
                {
                    if (*val < 0.0f)
                        return ResultValue<std::vector<ParsedStep>>::fail ("blur() radius cannot be negative");

                    step.amount = *val;
                }
                else if (allowExpressions)
                {
                    step.amountExpression = argText;
                }
                else
                {
                    return ResultValue<std::vector<ParsedStep>>::fail ("Invalid length '" + argText + "' in blur()");
                }
            }
            else if (info->paramKind == ParamKind::numberPercentage)
            {
                if (const auto val = parseCssNumberPercentageLiteral (argText); val.has_value())
                {
                    if (*val < 0.0f)
                        return ResultValue<std::vector<ParsedStep>>::fail (funcName + "() amount cannot be negative");

                    step.amount = clampAmount (info->type, *val);
                }
                else if (allowExpressions)
                {
                    step.amountExpression = argText;
                }
                else
                {
                    return ResultValue<std::vector<ParsedStep>>::fail ("Invalid amount '" + argText + "' in " + funcName + "()");
                }
            }
            else
            {
                const auto parsedAngle = parseCssAngleLiteral (argText);
                if (parsedAngle.isNumericWithoutUnit)
                    return ResultValue<std::vector<ParsedStep>>::fail ("hue-rotate() expects an angle unit (deg, rad, grad, turn)");

                if (parsedAngle.radians.has_value())
                    step.amount = *parsedAngle.radians;
                else if (allowExpressions)
                    step.amountExpression = argText;
                else
                    return ResultValue<std::vector<ParsedStep>>::fail ("Invalid angle '" + argText + "' in hue-rotate()");
            }
        }

        steps.push_back (std::move (step));
    }

    return makeResultValueOk (std::move (steps));
}

ResultValue<std::vector<FilterEffect::Operation>> FilterEffect::parse (StringRef filterText)
{
    auto parsed = parseSteps (filterText, false);
    if (parsed.failed())
        return makeResultValueFail (parsed.getErrorMessage());

    std::vector<Operation> result;
    result.reserve (parsed.getReference().size());

    for (const auto& step : parsed.getReference())
    {
        result.push_back ({
            .type = step.type,
            .amount = step.amount,
            .offset = step.offset,
            .color = step.color
        });
    }

    return makeResultValueOk (std::move (result));
}

ResultValue<FilterEffect::Ptr> FilterEffect::fromString (StringRef filterText)
{
    auto parsed = parse (filterText);
    if (parsed.failed())
        return makeResultValueFail (parsed.getErrorMessage());

    return makeResultValueOk (Ptr (new FilterEffect (std::move (parsed.getReference()))));
}

FilterEffect::Operation FilterEffect::identityOperationFor (const Operation& target) noexcept
{
    switch (target.type)
    {
        case Type::blur:
            return Operation::blur (0.0f);

        case Type::brightness:
            return Operation::brightness (1.0f);

        case Type::contrast:
            return Operation::contrast (1.0f);

        case Type::dropShadow:
            return Operation::dropShadow ({ 0.0f, 0.0f }, 0.0f, target.color.withAlpha (static_cast<uint8> (0)));

        case Type::grayscale:
            return Operation::grayscale (0.0f);

        case Type::hueRotate:
            return Operation::hueRotate (0.0f);

        case Type::invert:
            return Operation::invert (0.0f);

        case Type::opacity:
            return Operation::opacity (1.0f);

        case Type::saturate:
            return Operation::saturate (1.0f);

        case Type::sepia:
            return Operation::sepia (0.0f);
    }

    return Operation::opacity (1.0f);
}

std::vector<FilterEffect::Operation> FilterEffect::interpolate (const std::vector<Operation>& from,
                                                                const std::vector<Operation>& to,
                                                                float progress)
{
    const auto sharedCount = jmin (from.size(), to.size());

    for (size_t i = 0; i < sharedCount; ++i)
        if (from[i].type != to[i].type)
            return progress < 0.5f ? from : to;

    const auto totalCount = jmax (from.size(), to.size());
    std::vector<Operation> result;
    result.reserve (totalCount);

    for (size_t i = 0; i < totalCount; ++i)
    {
        const Operation a = i < from.size() ? from[i] : identityOperationFor (to[i]);
        const Operation b = i < to.size() ? to[i] : identityOperationFor (from[i]);

        if (a.type == Type::dropShadow)
        {
            result.push_back ({
                .type = Type::dropShadow,
                .amount = jmax (0.0f, a.amount + (b.amount - a.amount) * progress),
                .offset = {
                    a.offset.getX() + (b.offset.getX() - a.offset.getX()) * progress,
                    a.offset.getY() + (b.offset.getY() - a.offset.getY()) * progress },
                .color = a.color.mixedWith (b.color, jlimit (0.0f, 1.0f, progress), ColorSpace::LinearRGB)
            });
        }
        else
        {
            result.push_back ({
                .type = a.type,
                .amount = clampAmount (a.type, a.amount + (b.amount - a.amount) * progress),
                .offset = {},
                .color = Colors::transparentBlack
            });
        }
    }

    return result;
}

bool FilterEffect::isOperationsVar (const var& value) noexcept
{
    const auto* array = value.getArray();
    return array != nullptr && ! array->isEmpty() && (*array)[0].isString() && (*array)[0].toString() == "__yup_filter";
}

var FilterEffect::operationsToVar (const std::vector<Operation>& ops)
{
    Array<var> encoded;
    encoded.ensureStorageAllocated (static_cast<int> (ops.size()) + 1);
    encoded.add (var ("__yup_filter"));

    for (const auto& op : ops)
    {
        Array<var> item;
        item.ensureStorageAllocated (5);
        item.add (static_cast<int> (op.type));
        item.add (static_cast<double> (op.amount));
        item.add (static_cast<double> (op.offset.getX()));
        item.add (static_cast<double> (op.offset.getY()));
        item.add (static_cast<int64> (op.color.getARGB()));
        encoded.add (var (std::move (item)));
    }

    return var (std::move (encoded));
}

std::vector<FilterEffect::Operation> FilterEffect::operationsFromVar (const var& value)
{
    std::vector<Operation> result;
    const auto* array = value.getArray();

    if (array == nullptr || array->isEmpty() || ! (*array)[0].isString() || (*array)[0].toString() != "__yup_filter")
        return result;

    result.reserve (static_cast<size_t> (array->size() - 1));

    for (int i = 1; i < array->size(); ++i)
    {
        const auto* item = (*array)[i].getArray();
        if (item == nullptr || item->size() < 5)
            continue;

        const auto typeInt = static_cast<int> ((*item)[0]);
        if (typeInt < static_cast<int> (Type::blur) || typeInt > static_cast<int> (Type::sepia))
            continue;

        result.push_back ({
            .type = static_cast<Type> (typeInt),
            .amount = static_cast<float> (static_cast<double> ((*item)[1])),
            .offset = {
                static_cast<float> (static_cast<double> ((*item)[2])),
                static_cast<float> (static_cast<double> ((*item)[3])) },
            .color = Color (static_cast<uint32> (static_cast<int64> ((*item)[4])))
        });
    }

    return result;
}

void FilterEffect::setOperations (std::vector<Operation> newOperations)
{
    const ScopedLock sl (operationsLock);
    operations = std::move (newOperations);
}

std::vector<FilterEffect::Operation> FilterEffect::getOperations() const
{
    const ScopedLock sl (operationsLock);
    return operations;
}

EdgeSpacing<float> FilterEffect::getRequiredMargin() const noexcept
{
    const auto ops = getOperations();
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;

    for (const auto& op : ops)
    {
        const auto spread = detail::getBlurReach (op.amount);

        if (op.type == Type::blur)
        {
            left += spread;
            top += spread;
            right += spread;
            bottom += spread;
        }
        else if (op.type == Type::dropShadow && ! op.color.isTransparent())
        {
            left += spread + jmax (0.0f, -op.offset.getX());
            top += spread + jmax (0.0f, -op.offset.getY());
            right += spread + jmax (0.0f, op.offset.getX());
            bottom += spread + jmax (0.0f, op.offset.getY());
        }
    }

    return { left, top, right, bottom };
}

void FilterEffect::apply (Graphics& g, GpuTexture::Ptr inputTexture, Rectangle<float> bounds)
{
    if (inputTexture == nullptr || ! inputTexture->isValid() || bounds.isEmpty())
        return;

    const auto ops = getOperations();
    std::vector<const Operation*> active;
    active.reserve (ops.size());

    for (const auto& op : ops)
        if (! op.isNoOp())
            active.push_back (&op);

    if (active.empty())
    {
        g.drawTexture (inputTexture, bounds);
        return;
    }

    auto& context = g.getGraphicsContext();
    const auto device = context.getGpuDevice();

    if (device != blurDevice && device != colorMatrixDevice)
    {
        targetA = nullptr;
        targetB = nullptr;
        targetC = nullptr;
        compositeCanvas = nullptr;
    }

    const int width = inputTexture->getWidth();
    const int height = inputTexture->getHeight();
    const float pixelScale = static_cast<float> (width) / bounds.getWidth();
    GpuTexture::Ptr current = inputTexture;

    const auto pickDestTarget = [&] () -> GpuTarget::Ptr&
    {
        return (targetA != nullptr && current == targetA->asTexture()) ? targetB : targetA;
    };

    for (size_t i = 0; i < active.size(); ++i)
    {
        const auto& op = *active[i];
        const bool isLast = (i + 1 == active.size());

        if (op.type == Type::blur)
        {
            auto& scratchA = (targetA != nullptr && current == targetA->asTexture()) ? targetB : targetA;
            auto& scratchB = (&scratchA == &targetA) ? targetB : targetA;

            if (auto blurred = detail::runSeparableBlur (context,
                                                         blurDevice,
                                                         blurPipeline,
                                                         scratchA,
                                                         scratchB,
                                                         blurCompileAttempted,
                                                         current,
                                                         op.amount * pixelScale,
                                                         false))
            {
                current = blurred;
            }
        }
        else if (op.type == Type::dropShadow)
        {
            const bool currentInTargetA = (targetA != nullptr && current == targetA->asTexture());
            const bool currentInTargetB = (targetB != nullptr && current == targetB->asTexture());
            auto& scratchA = currentInTargetA ? targetB : targetA;
            auto& scratchB = (currentInTargetA || currentInTargetB) ? targetC : targetB;

            auto blurred = detail::runSeparableBlur (context,
                                                     blurDevice,
                                                     blurPipeline,
                                                     scratchA,
                                                     scratchB,
                                                     blurCompileAttempted,
                                                     current,
                                                     op.amount * pixelScale,
                                                     true);

            if (isLast)
            {
                if (blurred != nullptr)
                {
                    const auto saved = g.saveState();
                    g.setTint (op.color.withAlpha (static_cast<uint8> (255)));
                    g.setOpacity (g.getOpacity() * op.color.getAlphaFloat());
                    g.drawTexture (blurred, bounds.translated (op.offset));
                }

                g.drawTexture (current, bounds);
                return;
            }

            if (compositeCanvas == nullptr || compositeCanvas->getWidth() != width || compositeCanvas->getHeight() != height)
                compositeCanvas = GpuCanvas::create (context, width, height);

            if (compositeCanvas != nullptr)
            {
                const Rectangle<float> localBounds (0.0f, 0.0f, bounds.getWidth(), bounds.getHeight());
                auto& offscreenG = compositeCanvas->beginDraw ({}, pixelScale);
                offscreenG.setDrawingArea (localBounds);

                if (blurred != nullptr)
                {
                    const auto saved = offscreenG.saveState();
                    offscreenG.setTint (op.color.withAlpha (static_cast<uint8> (255)));
                    offscreenG.setOpacity (op.color.getAlphaFloat());
                    offscreenG.drawTexture (blurred, localBounds.translated (op.offset));
                }

                offscreenG.drawTexture (current, localBounds);
                compositeCanvas->commit();

                if (auto composited = compositeCanvas->asTexture())
                    current = composited;
            }
        }
        else
        {
            auto& destTarget = pickDestTarget();
            if (auto transformed = detail::runColorMatrix (context,
                                                           colorMatrixDevice,
                                                           colorMatrixPipeline,
                                                           destTarget,
                                                           colorMatrixCompileAttempted,
                                                           current,
                                                           matrixForOperation (op)))
            {
                current = transformed;
            }
        }
    }

    g.drawTexture (current, bounds);
}

} // namespace yup
