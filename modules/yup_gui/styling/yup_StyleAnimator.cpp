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

bool isAnimationName (const String& token)
{
    if (token.isEmpty() || ! (CharacterFunctions::isLetter (token[0]) || token[0] == '_'))
        return false;

    for (auto p = token.getCharPointer(); ! p.isEmpty(); ++p)
        if (! (CharacterFunctions::isLetterOrDigit (*p) || *p == '_'))
            return false;

    return true;
}

std::optional<double> parseAnimationTime (const String& token)
{
    if (! token.endsWithChar ('s'))
        return std::nullopt;

    const bool milliseconds = token.endsWith ("ms");
    const auto parsed = detail::parseStrictDouble (token.dropLastCharacters (milliseconds ? 2 : 1), false);
    if (! parsed.has_value())
        return std::nullopt;

    return *parsed / (milliseconds ? 1000.0 : 1.0);
}

/* Returns true when the token is an easing, leaving an error when it is a malformed one. */
bool parseAnimationEasing (const String& token, Easing& easing, String& error)
{
    static const std::pair<const char*, Easing> presets[] = {
        { "linear", Easing::linear() },
        { "easeIn", Easing::easeIn() },
        { "easeOut", Easing::easeOut() },
        { "easeInOut", Easing::easeInOut() }
    };

    if (const auto preset = detail::findNamed (presets, token))
    {
        easing = *preset;
        return true;
    }

    if (! token.startsWith ("cubicBezier("))
        return false;

    StringArray arguments;
    arguments.addTokens (token.fromFirstOccurrenceOf ("(", false, false).upToLastOccurrenceOf (")", false, false), ",", "");
    arguments.trim();

    if (! token.endsWithChar (')') || arguments.size() != 4)
    {
        error = "cubicBezier takes four numbers";
        return true;
    }

    float coords[4] {};
    for (int i = 0; i < 4; ++i)
    {
        const auto parsed = detail::parseStrictFloat (arguments[i]);
        if (! parsed.has_value())
        {
            error = "cubicBezier takes four numbers";
            return true;
        }

        coords[i] = *parsed;
    }

    if (! isPositiveAndNotGreaterThan (coords[0], 1.0f) || ! isPositiveAndNotGreaterThan (coords[2], 1.0f))
    {
        error = "The x coordinates of cubicBezier must be between 0 and 1";
        return true;
    }

    easing = Easing (coords[0], coords[1], coords[2], coords[3]);
    return true;
}

bool parseAnimationKeyword (StyleAnimator::Direction& direction,
                            StyleAnimator::FillMode& fillMode,
                            double& iterations,
                            const String& token,
                            String& kind)
{
    static const std::pair<const char*, StyleAnimator::Direction> directions[] = {
        { "normal", StyleAnimator::Direction::normal },
        { "reverse", StyleAnimator::Direction::reverse },
        { "alternate", StyleAnimator::Direction::alternate },
        { "alternateReverse", StyleAnimator::Direction::alternateReverse }
    };

    static const std::pair<const char*, StyleAnimator::FillMode> fillModes[] = {
        { "none", StyleAnimator::FillMode::none },
        { "forwards", StyleAnimator::FillMode::forwards },
        { "backwards", StyleAnimator::FillMode::backwards },
        { "both", StyleAnimator::FillMode::both }
    };

    if (token == "infinite")
    {
        iterations = std::numeric_limits<double>::infinity();
        kind = "iteration count";
        return true;
    }

    if (const auto parsedIterations = detail::parseStrictDouble (token, false))
    {
        iterations = *parsedIterations;
        kind = "iteration count";
        return true;
    }

    if (const auto parsedDirection = detail::findNamed (directions, token))
    {
        direction = *parsedDirection;
        kind = "direction";
        return true;
    }

    if (const auto parsedFillMode = detail::findNamed (fillModes, token))
    {
        fillMode = *parsedFillMode;
        kind = "fill mode";
        return true;
    }

    return false;
}

Result parseTimingTokens (const StringArray& tokens,
                          Identifier& targetName,
                          double& duration,
                          double& delay,
                          Easing& easing,
                          double& iterations,
                          StyleAnimator::Direction& direction,
                          StyleAnimator::FillMode& fillMode,
                          const char* nameKind,
                          bool allowAnimationKeywords)
{
    if (tokens.isEmpty())
        return Result::fail ("Expected a " + String (nameKind) + " between the commas");

    StringArray kinds;
    int times = 0;

    for (const auto& token : tokens)
    {
        String kind, error;

        if (const auto time = parseAnimationTime (token))
        {
            kind = times == 0 ? "duration" : "delay";
            (times++ == 0 ? duration : delay) = *time;
        }
        else if (parseAnimationEasing (token, easing, error))
        {
            kind = "easing";
        }
        else if (allowAnimationKeywords && parseAnimationKeyword (direction, fillMode, iterations, token, kind))
        {
        }
        else if (isAnimationName (token))
        {
            kind = nameKind;
            targetName = Identifier (token);
        }

        if (error.isNotEmpty())
            return Result::fail (error);

        if (kind.isEmpty())
            return Result::fail ("Unexpected '" + token + "'");

        if (kinds.contains (kind))
            return Result::fail ("Unexpected '" + token + "', the " + kind + " is already set");

        kinds.add (kind);
    }

    if (! targetName.isValid())
        return Result::fail ("Expected a " + String (nameKind) + " in '" + tokens.joinIntoString (" ") + "'");

    return Result::ok();
}

double directedAnimationProgress (StyleAnimator::Direction direction, double iteration, double fraction)
{
    const bool odd = std::fmod (iteration, 2.0) >= 1.0;
    const bool backwards = direction == StyleAnimator::Direction::reverse
                        || (direction == StyleAnimator::Direction::alternate && odd)
                        || (direction == StyleAnimator::Direction::alternateReverse && ! odd);

    return backwards ? 1.0 - fraction : fraction;
}

} // namespace

//==============================================================================

ResultValue<std::vector<StyleAnimator::Transition>> StyleAnimator::parseTransitions (const String& text)
{
    std::vector<Transition> parsed;

    if (text.trim() == "none")
        return makeResultValueOk (std::move (parsed));

    for (const auto& tokens : detail::tokenizeStyleValue (text))
    {
        Transition entry;
        double iterations = 1.0;
        auto direction = Direction::normal;
        auto fillMode = FillMode::none;

        if (auto res = parseTimingTokens (tokens, entry.property, entry.duration, entry.delay, entry.easing, iterations, direction, fillMode, "property", false); res.failed())
            return makeResultValueFail (res.getErrorMessage());

        parsed.push_back (std::move (entry));
    }

    return makeResultValueOk (std::move (parsed));
}

ResultValue<std::vector<StyleAnimator::Animation>> StyleAnimator::parseAnimations (const String& text)
{
    std::vector<Animation> parsed;

    if (text.trim() == "none")
        return makeResultValueOk (std::move (parsed));

    for (const auto& tokens : detail::tokenizeStyleValue (text))
    {
        Animation entry;

        if (auto res = parseTimingTokens (tokens, entry.name, entry.duration, entry.delay, entry.easing, entry.iterations, entry.direction, entry.fillMode, "keyframes name", true); res.failed())
            return makeResultValueFail (res.getErrorMessage());

        parsed.push_back (std::move (entry));
    }

    return makeResultValueOk (std::move (parsed));
}

//==============================================================================

namespace
{

struct DecomposedStyleTransform2D
{
    double translateX = 0.0;
    double translateY = 0.0;
    double angle = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
    double skew = 0.0;
};

std::optional<DecomposedStyleTransform2D> decomposeStyleTransform2D (const AffineTransform& transform) noexcept
{
    const double det = static_cast<double> (transform.getScaleX()) * static_cast<double> (transform.getScaleY())
                     - static_cast<double> (transform.getShearX()) * static_cast<double> (transform.getShearY());

    if (std::abs (det) < 1.0e-6)
        return std::nullopt;

    const double col0x = transform.getScaleX();
    const double col0y = transform.getShearY();
    const double sx = std::hypot (col0x, col0y);

    if (sx < 1.0e-6)
        return std::nullopt;

    const double ux = col0x / sx;
    const double uy = col0y / sx;
    const double vx = -uy;
    const double vy = ux;

    const double col1x = transform.getShearX();
    const double col1y = transform.getScaleY();
    const double r01 = col1x * ux + col1y * uy;
    const double sy = col1x * vx + col1y * vy;

    if (std::abs (sy) < 1.0e-6)
        return std::nullopt;

    return DecomposedStyleTransform2D {
        .translateX = transform.getTranslateX(),
        .translateY = transform.getTranslateY(),
        .angle = std::atan2 (uy, ux),
        .scaleX = sx,
        .scaleY = sy,
        .skew = r01 / sy
    };
}

AffineTransform recomposeStyleTransform2D (const DecomposedStyleTransform2D& decomposed) noexcept
{
    const double ux = std::cos (decomposed.angle);
    const double uy = std::sin (decomposed.angle);
    const double vx = -uy;
    const double vy = ux;

    return {
        static_cast<float> (decomposed.scaleX * ux),
        static_cast<float> (decomposed.scaleY * (decomposed.skew * ux + vx)),
        static_cast<float> (decomposed.translateX),
        static_cast<float> (decomposed.scaleX * uy),
        static_cast<float> (decomposed.scaleY * (decomposed.skew * uy + vy)),
        static_cast<float> (decomposed.translateY)
    };
}

} // namespace

var StyleAnimator::interpolate (const var& from, const var& to, double progress, ComponentProperty::Type type)
{
    const auto isNumeric = [] (const var& value)
    {
        return value.isDouble() || value.isInt() || value.isInt64();
    };

    if (isNumeric (from) && isNumeric (to))
    {
        const auto start = static_cast<double> (from);
        const auto end = static_cast<double> (to);

        switch (type)
        {
            case ComponentProperty::Type::number:
                return start + (end - start) * progress;

            case ComponentProperty::Type::integer:
                return roundToInt (start + (end - start) * progress);

            case ComponentProperty::Type::color:
                return VariantConverter<Color>::toVar (VariantConverter<Color>::fromVar (from)
                                                           .mixedWith (VariantConverter<Color>::fromVar (to), static_cast<float> (progress), ColorSpace::LinearRGB));

            default:
                break;
        }
    }

    if (type == ComponentProperty::Type::transform)
    {
        const auto startTransform = VariantConverter<AffineTransform>::fromVar (from);
        const auto endTransform = VariantConverter<AffineTransform>::fromVar (to);

        if (const auto a = decomposeStyleTransform2D (startTransform), b = decomposeStyleTransform2D (endTransform);
            a.has_value() && b.has_value())
        {
            double deltaAngle = std::fmod (b->angle - a->angle, MathConstants<double>::twoPi);

            if (deltaAngle > MathConstants<double>::pi)
                deltaAngle -= MathConstants<double>::twoPi;
            else if (deltaAngle <= -MathConstants<double>::pi)
                deltaAngle += MathConstants<double>::twoPi;

            return VariantConverter<AffineTransform>::toVar (recomposeStyleTransform2D ({
                .translateX = a->translateX + (b->translateX - a->translateX) * progress,
                .translateY = a->translateY + (b->translateY - a->translateY) * progress,
                .angle = a->angle + deltaAngle * progress,
                .scaleX = a->scaleX + (b->scaleX - a->scaleX) * progress,
                .scaleY = a->scaleY + (b->scaleY - a->scaleY) * progress,
                .skew = a->skew + (b->skew - a->skew) * progress
            }));
        }
    }

    if (FilterEffect::isOperationsVar (from) && FilterEffect::isOperationsVar (to))
    {
        return FilterEffect::operationsToVar (FilterEffect::interpolate (
            FilterEffect::operationsFromVar (from),
            FilterEffect::operationsFromVar (to),
            static_cast<float> (progress)));
    }

    // Points, rectangles and arrays of numbers are vars holding numbers, interpolated element by element
    if (const auto* start = from.getArray())
    {
        if (const auto* end = to.getArray(); end != nullptr && end->size() == start->size())
        {
            Array<var> values;
            values.ensureStorageAllocated (start->size());

            for (int i = 0; i < start->size(); ++i)
            {
                const auto& first = (*start)[i];
                const auto& second = (*end)[i];

                values.add (isNumeric (first) && isNumeric (second)
                                ? var (static_cast<double> (first) + (static_cast<double> (second) - static_cast<double> (first)) * progress)
                                : (progress < 0.5 ? first : second));
            }

            return var (std::move (values));
        }
    }

    return progress < 0.5 ? from : to;
}

var StyleAnimator::sample (const Track& track, double progress, const Easing& easing)
{
    const auto& frames = track.frames;

    if (progress <= frames.front().position)
        return frames.front().value;

    for (size_t i = 1; i < frames.size(); ++i)
    {
        const auto& next = frames[i];

        if (progress > next.position)
            continue;

        const auto& previous = frames[i - 1];
        const auto span = next.position - previous.position;
        const auto local = span > 0.0 ? (progress - previous.position) / span : 1.0;

        return interpolate (previous.value, next.value, easing.evaluate (static_cast<float> (local)), track.type);
    }

    return frames.back().value;
}

//==============================================================================

void StyleAnimator::startTransition (Component& component,
                                     const Identifier& storageId,
                                     ComponentProperty::Type type,
                                     const var& from,
                                     const var& to,
                                     const Transition& transition,
                                     double now)
{
    cancelTransition (component, storageId);

    const auto insertPos = std::find_if (entries.begin(), entries.end(), [] (const auto& entry)
    {
        return ! entry.isTransition;
    });

    Timing timing { transition.duration, transition.delay, transition.easing, 1.0, Direction::normal, FillMode::backwards };
    std::vector<Track> tracks { { storageId, type, { { 0.0, from }, { 1.0, to } } } };

    entries.insert (insertPos, { WeakReference<Component> (&component), std::move (timing), std::move (tracks), now, true, false });
}

const var* StyleAnimator::findTransitionTarget (const Component& component, const Identifier& storageId) const
{
    for (const auto& running : entries)
        if (running.isTransition && running.component.get() == &component && running.tracks.front().storageId == storageId)
            return &running.tracks.front().frames.back().value;

    return nullptr;
}

Array<Identifier> StyleAnimator::getTransitions (const Component& component) const
{
    Array<Identifier> result;

    for (const auto& running : entries)
        if (running.isTransition && running.component.get() == &component)
            result.add (running.tracks.front().storageId);

    return result;
}

bool StyleAnimator::cancelTransition (const Component& component, const Identifier& storageId)
{
    return std::erase_if (entries, [&] (const auto& running)
    {
        return running.isTransition && running.component.get() == &component && running.tracks.front().storageId == storageId;
    }) > 0;
}

//==============================================================================

void StyleAnimator::startAnimation (Component& component, const Animation& animation, std::vector<Track> tracks, double now)
{
    std::erase_if (tracks, [] (const auto& track)
    {
        return track.frames.empty();
    });

    Timing timing { animation.duration, animation.delay, animation.easing, animation.iterations, animation.direction, animation.fillMode };
    entries.push_back ({ WeakReference<Component> (&component), std::move (timing), std::move (tracks), now, false, false });
}

Array<Identifier> StyleAnimator::stopAnimations (const Component& component)
{
    Array<Identifier> stopped;

    std::erase_if (entries, [&] (const auto& running)
    {
        if (running.isTransition || running.component.get() != &component)
            return false;

        for (const auto& track : running.tracks)
            stopped.addIfNotAlreadyThere (track.storageId);

        return true;
    });

    return stopped;
}

//==============================================================================

void StyleAnimator::removeComponent (const Component& component)
{
    std::erase_if (entries, [&] (const auto& running)
    {
        return running.component.get() == &component;
    });
}

void StyleAnimator::clear()
{
    entries.clear();
}

bool StyleAnimator::isAnimating() const noexcept
{
    return std::any_of (entries.begin(), entries.end(), [] (const auto& running)
    {
        return ! running.holding;
    });
}

void StyleAnimator::advance (double now, const Callback& callback)
{
    // The callback may restyle or relayout, so the values are collected before any is reported
    updates.clear();

    for (auto it = entries.begin(); it != entries.end();)
    {
        if (it->component.get() == nullptr)
        {
            it = entries.erase (it);
            continue;
        }

        if (it->holding)
        {
            ++it;
            continue;
        }

        const auto& timing = it->timing;
        const auto elapsed = now - it->start - timing.delay;
        const bool fillsBackwards = timing.fillMode == FillMode::backwards || timing.fillMode == FillMode::both;
        const bool fillsForwards = timing.fillMode == FillMode::forwards || timing.fillMode == FillMode::both;
        const bool finished = elapsed >= 0.0 && (timing.duration <= 0.0 || elapsed >= timing.duration * timing.iterations);

        std::optional<double> progress;

        if (elapsed < 0.0)
        {
            if (fillsBackwards)
                progress = directedAnimationProgress (timing.direction, 0.0, 0.0);
        }
        else if (finished)
        {
            if (fillsForwards)
            {
                // The end of the last iteration, or the point a fractional count stops at
                const auto end = std::isfinite (timing.iterations) ? timing.iterations : 1.0;
                auto iteration = std::floor (end);
                auto fraction = end - iteration;

                if (fraction == 0.0 && end > 0.0)
                {
                    iteration -= 1.0;
                    fraction = 1.0;
                }

                progress = directedAnimationProgress (timing.direction, iteration, fraction);
            }
        }
        else
        {
            const auto position = elapsed / timing.duration;
            const auto iteration = std::floor (position);
            progress = directedAnimationProgress (timing.direction, iteration, position - iteration);
        }

        for (const auto& track : it->tracks)
        {
            std::optional<var> value;

            if (progress.has_value())
                value = sample (track, *progress, timing.easing);

            updates.push_back ({ it->component, track.storageId, std::move (value) });
        }

        if (finished && ! fillsForwards)
        {
            it = entries.erase (it);
            continue;
        }

        it->holding = finished;
        ++it;
    }

    for (const auto& update : updates)
        if (auto* component = update.component.get())
            callback (*component, update.storageId, update.value.has_value() ? &*update.value : nullptr);
}

} // namespace yup
