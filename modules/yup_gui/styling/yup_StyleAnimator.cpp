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

bool isAnimationNumber (const String& text)
{
    return text.containsAnyOf ("0123456789")
        && text.containsOnly ("0123456789.")
        && text.indexOfChar ('.') == text.lastIndexOfChar ('.');
}

bool isSignedAnimationNumber (const String& text)
{
    return isAnimationNumber (text.startsWithChar ('-') ? text.substring (1) : text);
}

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
    const bool milliseconds = token.endsWith ("ms");
    const auto number = token.dropLastCharacters (milliseconds ? 2 : 1);

    if (! token.endsWithChar ('s') || ! isAnimationNumber (number))
        return std::nullopt;

    return number.getDoubleValue() / (milliseconds ? 1000.0 : 1.0);
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

    for (const auto& [name, preset] : presets)
    {
        if (token == name)
        {
            easing = preset;
            return true;
        }
    }

    if (! token.startsWith ("cubicBezier("))
        return false;

    StringArray arguments;
    arguments.addTokens (token.fromFirstOccurrenceOf ("(", false, false).upToLastOccurrenceOf (")", false, false), ",", "");
    arguments.trim();

    if (! token.endsWithChar (')') || arguments.size() != 4 || ! std::all_of (arguments.begin(), arguments.end(), isSignedAnimationNumber))
    {
        error = "cubicBezier takes four numbers";
        return true;
    }

    const auto x1 = arguments[0].getFloatValue();
    const auto x2 = arguments[2].getFloatValue();

    if (! isPositiveAndNotGreaterThan (x1, 1.0f) || ! isPositiveAndNotGreaterThan (x2, 1.0f))
    {
        error = "The x coordinates of cubicBezier must be between 0 and 1";
        return true;
    }

    easing = Easing (x1, arguments[1].getFloatValue(), x2, arguments[3].getFloatValue());
    return true;
}

/* Parses what transitions and animations share: the times, the easing and the name. The
   other tokens go to parseKeyword, which names the kind of value it parsed or returns false. */
template <class Entry, class ParseKeyword>
ResultValue<std::vector<Entry>> parseAnimationEntries (const String& text, Identifier Entry::*name, const String& nameKind, ParseKeyword parseKeyword)
{
    std::vector<Entry> entries;

    if (text.trim() == "none")
        return makeResultValueOk (std::move (entries));

    for (const auto& tokens : StyleSheet::tokenizeValue (text))
    {
        if (tokens.isEmpty())
            return makeResultValueFail ("Expected a " + nameKind + " between the commas");

        Entry entry;
        StringArray kinds;
        int times = 0;

        for (const auto& token : tokens)
        {
            String kind, error;

            if (const auto time = parseAnimationTime (token))
            {
                kind = times == 0 ? "duration" : "delay";
                (times++ == 0 ? entry.duration : entry.delay) = *time;
            }
            else if (parseAnimationEasing (token, entry.easing, error))
            {
                kind = "easing";
            }
            else if (! parseKeyword (entry, token, kind) && isAnimationName (token))
            {
                kind = nameKind;
                entry.*name = Identifier (token);
            }

            if (error.isNotEmpty())
                return makeResultValueFail (error);

            if (kind.isEmpty())
                return makeResultValueFail ("Unexpected '" + token + "'");

            if (kinds.contains (kind))
                return makeResultValueFail ("Unexpected '" + token + "', the " + kind + " is already set");

            kinds.add (kind);
        }

        if (! (entry.*name).isValid())
            return makeResultValueFail ("Expected a " + nameKind + " in '" + tokens.joinIntoString (" ") + "'");

        entries.push_back (std::move (entry));
    }

    return makeResultValueOk (std::move (entries));
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
    return parseAnimationEntries (text, &Transition::property, "property", [] (Transition&, const String&, String&)
    {
        return false;
    });
}

ResultValue<std::vector<StyleAnimator::Animation>> StyleAnimator::parseAnimations (const String& text)
{
    return parseAnimationEntries (text, &Animation::name, "keyframes name", [] (Animation& animation, const String& token, String& kind)
    {
        static const std::pair<const char*, Direction> directions[] = {
            { "normal", Direction::normal },
            { "reverse", Direction::reverse },
            { "alternate", Direction::alternate },
            { "alternateReverse", Direction::alternateReverse }
        };

        static const std::pair<const char*, FillMode> fillModes[] = {
            { "none", FillMode::none },
            { "forwards", FillMode::forwards },
            { "backwards", FillMode::backwards },
            { "both", FillMode::both }
        };

        if (token == "infinite" || isAnimationNumber (token))
        {
            animation.iterations = token == "infinite" ? std::numeric_limits<double>::infinity() : token.getDoubleValue();
            kind = "iteration count";
            return true;
        }

        for (const auto& [keyword, direction] : directions)
        {
            if (token == keyword)
            {
                animation.direction = direction;
                kind = "direction";
                return true;
            }
        }

        for (const auto& [keyword, fillMode] : fillModes)
        {
            if (token == keyword)
            {
                animation.fillMode = fillMode;
                kind = "fill mode";
                return true;
            }
        }

        return false;
    });
}

//==============================================================================

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
                                                           .mixedWith (VariantConverter<Color>::fromVar (to), static_cast<float> (progress), ColorSpace::SRGB));

            default:
                break;
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
    transitions.push_back ({ WeakReference<Component> (&component), storageId, type, from, to, transition, now });
}

const var* StyleAnimator::findTransitionTarget (const Component& component, const Identifier& storageId) const
{
    for (const auto& running : transitions)
        if (running.component.get() == &component && running.storageId == storageId)
            return &running.to;

    return nullptr;
}

Array<Identifier> StyleAnimator::getTransitions (const Component& component) const
{
    Array<Identifier> result;

    for (const auto& running : transitions)
        if (running.component.get() == &component)
            result.add (running.storageId);

    return result;
}

bool StyleAnimator::cancelTransition (const Component& component, const Identifier& storageId)
{
    return std::erase_if (transitions, [&] (const auto& running)
    {
        return running.component.get() == &component && running.storageId == storageId;
    }) > 0;
}

//==============================================================================

void StyleAnimator::startAnimation (Component& component, const Animation& animation, std::vector<Track> tracks, double now)
{
    std::erase_if (tracks, [] (const auto& track)
    {
        return track.frames.empty();
    });

    animations.push_back ({ WeakReference<Component> (&component), animation, std::move (tracks), now });
}

Array<Identifier> StyleAnimator::stopAnimations (const Component& component)
{
    Array<Identifier> stopped;

    std::erase_if (animations, [&] (const auto& running)
    {
        if (running.component.get() != &component)
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
    std::erase_if (transitions, [&] (const auto& running)
    {
        return running.component.get() == &component;
    });

    std::erase_if (animations, [&] (const auto& running)
    {
        return running.component.get() == &component;
    });
}

void StyleAnimator::clear()
{
    transitions.clear();
    animations.clear();
}

bool StyleAnimator::isAnimating() const noexcept
{
    return ! transitions.empty() || std::any_of (animations.begin(), animations.end(), [] (const auto& running)
    {
        return ! running.holding;
    });
}

void StyleAnimator::advance (double now, const Callback& callback)
{
    struct Update
    {
        WeakReference<Component> component;
        Identifier storageId;
        std::optional<var> value;
    };

    // The callback may restyle or relayout, so the values are collected before any is reported
    std::vector<Update> updates;

    for (auto it = transitions.begin(); it != transitions.end();)
    {
        if (it->component.get() == nullptr)
        {
            it = transitions.erase (it);
            continue;
        }

        const auto& timing = it->transition;
        const auto elapsed = now - it->start - timing.delay;

        if (elapsed >= timing.duration)
        {
            updates.push_back ({ it->component, it->storageId, std::nullopt });
            it = transitions.erase (it);
            continue;
        }

        const auto progress = elapsed > 0.0 ? elapsed / timing.duration : 0.0;
        updates.push_back ({ it->component, it->storageId, interpolate (it->from, it->to, timing.easing.evaluate (static_cast<float> (progress)), it->type) });
        ++it;
    }

    // Animations come after the transitions, so they win on the properties both animate
    for (auto it = animations.begin(); it != animations.end();)
    {
        if (it->component.get() == nullptr)
        {
            it = animations.erase (it);
            continue;
        }

        if (it->holding)
        {
            ++it;
            continue;
        }

        const auto& animation = it->animation;
        const auto elapsed = now - it->start - animation.delay;
        const bool fillsBackwards = animation.fillMode == FillMode::backwards || animation.fillMode == FillMode::both;
        const bool fillsForwards = animation.fillMode == FillMode::forwards || animation.fillMode == FillMode::both;
        const bool finished = elapsed >= 0.0 && (animation.duration <= 0.0 || elapsed >= animation.duration * animation.iterations);

        std::optional<double> progress;

        if (elapsed < 0.0)
        {
            if (fillsBackwards)
                progress = directedAnimationProgress (animation.direction, 0.0, 0.0);
        }
        else if (finished)
        {
            if (fillsForwards)
            {
                // The end of the last iteration, or the point a fractional count stops at
                const auto end = std::isfinite (animation.iterations) ? animation.iterations : 1.0;
                auto iteration = std::floor (end);
                auto fraction = end - iteration;

                if (fraction == 0.0 && end > 0.0)
                {
                    iteration -= 1.0;
                    fraction = 1.0;
                }

                progress = directedAnimationProgress (animation.direction, iteration, fraction);
            }
        }
        else
        {
            const auto position = elapsed / animation.duration;
            const auto iteration = std::floor (position);
            progress = directedAnimationProgress (animation.direction, iteration, position - iteration);
        }

        for (const auto& track : it->tracks)
        {
            std::optional<var> value;

            if (progress.has_value())
                value = sample (track, *progress, animation.easing);

            updates.push_back ({ it->component, track.storageId, std::move (value) });
        }

        if (finished && ! fillsForwards)
        {
            it = animations.erase (it);
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
