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

#include "SynthEffectsParameters.h"
#include "SynthFormat.h"

namespace
{
constexpr std::array<int, SynthExample::effectCount> factorials { 120, 24, 6, 2, 1, 1 };

yup::NormalisableRange<float> skewed (float minimum, float maximum, float centre)
{
    yup::NormalisableRange<float> range (minimum, maximum);
    range.setSkewForCentre (centre);
    return range;
}

/** Times are stored in milliseconds and shown as the other time controls are. */
yup::String formatMilliseconds (double value) { return SynthFormat::milliseconds (value * 0.001); }
double parseMilliseconds (const yup::String& text) { return SynthFormat::parse (text) * 1000.0; }

yup::String formatBits (double value) { return yup::String (value, 1) + " bits"; }

yup::String formatOrder (float value)
{
    const auto names = getSynthEffectNames();
    yup::StringArray chain;

    for (const auto type : SynthEffectsParameters::decodeOrder (yup::roundToInt (value)))
        chain.add (names[static_cast<int> (type)]);

    return chain.joinIntoString (" > ");
}

float parseOrder (const yup::String& text)
{
    const auto names = getSynthEffectNames();
    const auto tokens = yup::StringArray::fromTokens (text, ">", {});
    SynthEffectOrder order {};
    std::array<bool, SynthExample::effectCount> used {};

    if (tokens.size() != SynthExample::effectCount)
        return 0.0f;

    for (int index = 0; index < tokens.size(); ++index)
    {
        const auto type = names.indexOf (tokens[index].trim(), true);

        if (type < 0 || used[static_cast<std::size_t> (type)])
            return 0.0f;

        used[static_cast<std::size_t> (type)] = true;
        order[static_cast<std::size_t> (index)] = static_cast<SynthEffectType> (type);
    }

    return static_cast<float> (SynthEffectsParameters::encodeOrder (order));
}
} // namespace

SynthEffectsParameters::SynthEffectsParameters()
{
    using namespace SynthParameterFactory;

    const SynthEffectsValues defaults;
    const auto names = getSynthEffectNames();

    order = yup::AudioParameterBuilder()
                .withID ("fx.order")
                .withName ("Effects Order")
                .withHostID (8000)
                .withModulePath ("Effects")
                .withRange (yup::NormalisableRange<float> (0.0f, static_cast<float> (orderCount - 1), 1.0f))
                .withDefault (static_cast<float> (encodeOrder (defaults.order)))
                .withStepped (true)
                .withValueToString (formatOrder)
                .withStringToValue (parseOrder)
                .build();

    const auto prefix = [] (const char* name) { return yup::String ("fx.") + name + "."; };
    const auto hostBase = [] (SynthEffectType type) { return static_cast<yup::uint32> (8100 + 100 * static_cast<int> (type)); };

    for (int index = 0; index < SynthExample::effectCount; ++index)
    {
        const auto type = static_cast<SynthEffectType> (index);
        const auto& name = names[index];
        enabled[static_cast<std::size_t> (index)] = createChoice ("fx." + name.toLowerCase() + ".enabled", name + " Enabled", hostBase (type), name, { "Off", "On" }, 0);
    }

    {
        const auto id = prefix ("bitcrusher");
        const auto base = hostBase (SynthEffectType::bitcrusher);
        const auto& values = defaults.bitcrusher;

        bitcrusher.bits = createContinuous (id + "bits", "Bitcrusher Bits", base + 1, "Bitcrusher", { 1.0f, 16.0f }, values.bits, formatBits);
        bitcrusher.rate = createContinuous (id + "rate", "Bitcrusher Rate", base + 2, "Bitcrusher", skewed (100.0f, 48000.0f, 4000.0f), values.rate, SynthFormat::hertz);
        bitcrusher.dither = createContinuous (id + "dither", "Bitcrusher Dither", base + 3, "Bitcrusher", { 0.0f, 1.0f }, values.dither, SynthFormat::percent);
        bitcrusher.mix = createContinuous (id + "mix", "Bitcrusher Mix", base + 4, "Bitcrusher", { 0.0f, 1.0f }, values.mix, SynthFormat::percent);
    }

    {
        const auto id = prefix ("chorus");
        const auto base = hostBase (SynthEffectType::chorus);
        const auto& values = defaults.chorus;

        chorus.mode = createChoice (id + "mode", "Chorus Mode", base + 1, "Chorus", getSynthChorusModeNames(), static_cast<int> (values.mode));
        chorus.rate = createContinuous (id + "rate", "Chorus Rate", base + 2, "Chorus", skewed (0.05f, 10.0f, 1.0f), values.rate, SynthFormat::hertz);
        chorus.depth = createContinuous (id + "depth", "Chorus Depth", base + 3, "Chorus", { 0.0f, 1.0f }, values.depth, SynthFormat::percent);
        chorus.delay = createContinuous (id + "delay", "Chorus Delay", base + 4, "Chorus", { 1.0f, 20.0f }, values.delayMs, formatMilliseconds, parseMilliseconds);
        chorus.feedback = createContinuous (id + "feedback", "Chorus Feedback", base + 5, "Chorus", { 0.0f, 0.9f }, values.feedback, SynthFormat::percent);
        chorus.mix = createContinuous (id + "mix", "Chorus Mix", base + 6, "Chorus", { 0.0f, 1.0f }, values.mix, SynthFormat::percent);
    }

    {
        const auto id = prefix ("phaser");
        const auto base = hostBase (SynthEffectType::phaser);
        const auto& values = defaults.phaser;

        phaser.stages = createChoice (id + "stages", "Phaser Stages", base + 1, "Phaser", { "4", "8", "12" }, values.stages / 4 - 1);
        phaser.rate = createContinuous (id + "rate", "Phaser Rate", base + 2, "Phaser", skewed (0.02f, 10.0f, 1.0f), values.rate, SynthFormat::hertz);
        phaser.depth = createContinuous (id + "depth", "Phaser Depth", base + 3, "Phaser", { 0.0f, 1.0f }, values.depth, SynthFormat::percent);
        phaser.center = createContinuous (id + "center", "Phaser Center", base + 4, "Phaser", skewed (100.0f, 8000.0f, 1000.0f), values.center, SynthFormat::hertz);
        phaser.feedback = createContinuous (id + "feedback", "Phaser Feedback", base + 5, "Phaser", { 0.0f, 0.95f }, values.feedback, SynthFormat::percent);
        phaser.stereo = createContinuous (id + "stereo", "Phaser Stereo", base + 6, "Phaser", { 0.0f, 1.0f }, values.stereo, SynthFormat::percent);
        phaser.mix = createContinuous (id + "mix", "Phaser Mix", base + 7, "Phaser", { 0.0f, 1.0f }, values.mix, SynthFormat::percent);
    }

    {
        const auto id = prefix ("flanger");
        const auto base = hostBase (SynthEffectType::flanger);
        const auto& values = defaults.flanger;

        flanger.rate = createContinuous (id + "rate", "Flanger Rate", base + 1, "Flanger", skewed (0.02f, 10.0f, 1.0f), values.rate, SynthFormat::hertz);
        flanger.depth = createContinuous (id + "depth", "Flanger Depth", base + 2, "Flanger", { 0.0f, 1.0f }, values.depth, SynthFormat::percent);
        flanger.delay = createContinuous (id + "delay", "Flanger Delay", base + 3, "Flanger", skewed (0.5f, 10.0f, 3.0f), values.delayMs, formatMilliseconds, parseMilliseconds);
        flanger.feedback = createContinuous (id + "feedback", "Flanger Feedback", base + 4, "Flanger", { -0.95f, 0.95f }, values.feedback, SynthFormat::bipolar);
        flanger.stereo = createContinuous (id + "stereo", "Flanger Stereo", base + 5, "Flanger", { 0.0f, 1.0f }, values.stereo, SynthFormat::percent);
        flanger.mix = createContinuous (id + "mix", "Flanger Mix", base + 6, "Flanger", { 0.0f, 1.0f }, values.mix, SynthFormat::percent);
    }

    {
        const auto id = prefix ("delay");
        const auto base = hostBase (SynthEffectType::delay);
        const auto& values = defaults.delay;

        delay.tempoSync = createChoice (id + "sync", "Delay Sync", base + 1, "Delay", { "Off", "On" }, values.tempoSync ? 1 : 0);
        delay.time = createContinuous (id + "time", "Delay Time", base + 2, "Delay", skewed (20.0f, 2000.0f, 400.0f), values.timeMs, formatMilliseconds, parseMilliseconds);
        delay.division = createChoice (id + "division", "Delay Division", base + 3, "Delay", getSynthLFODivisionNames(), values.division);
        delay.feedback = createContinuous (id + "feedback", "Delay Feedback", base + 4, "Delay", { 0.0f, 0.95f }, values.feedback, SynthFormat::percent);
        delay.pingPong = createChoice (id + "pingpong", "Delay Mode", base + 5, "Delay", { "Stereo", "Ping-Pong" }, values.pingPong ? 1 : 0);
        delay.lowCut = createContinuous (id + "lowcut", "Delay Low Cut", base + 6, "Delay", skewed (20.0f, 2000.0f, 200.0f), values.lowCut, SynthFormat::hertz);
        delay.highCut = createContinuous (id + "highcut", "Delay High Cut", base + 7, "Delay", skewed (1000.0f, 20000.0f, 5000.0f), values.highCut, SynthFormat::hertz);
        delay.mix = createContinuous (id + "mix", "Delay Mix", base + 8, "Delay", { 0.0f, 1.0f }, values.mix, SynthFormat::percent);
    }

    {
        const auto id = prefix ("reverb");
        const auto base = hostBase (SynthEffectType::reverb);
        const auto& values = defaults.reverb;

        reverb.predelay = createContinuous (id + "predelay", "Reverb Predelay", base + 1, "Reverb", { 0.0f, 250.0f }, values.predelayMs, formatMilliseconds, parseMilliseconds);
        reverb.decay = createContinuous (id + "decay", "Reverb Decay", base + 2, "Reverb", { 0.0f, 1.0f }, values.decay, SynthFormat::percent);
        reverb.damping = createContinuous (id + "damping", "Reverb Damping", base + 3, "Reverb", { 0.0f, 1.0f }, values.damping, SynthFormat::percent);
        reverb.width = createContinuous (id + "width", "Reverb Width", base + 4, "Reverb", { 0.0f, 1.0f }, values.width, SynthFormat::percent);
        reverb.mix = createContinuous (id + "mix", "Reverb Mix", base + 5, "Reverb", { 0.0f, 1.0f }, values.mix, SynthFormat::percent);
    }
}

void SynthEffectsParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : getParameters())
        processor.addParameter (parameter);
}

std::vector<yup::AudioParameter::Ptr> SynthEffectsParameters::getParameters() const
{
    std::vector<yup::AudioParameter::Ptr> parameters { order };
    parameters.insert (parameters.end(), enabled.begin(), enabled.end());
    parameters.insert (parameters.end(), { bitcrusher.bits, bitcrusher.rate, bitcrusher.dither, bitcrusher.mix,
                                           chorus.mode, chorus.rate, chorus.depth, chorus.delay, chorus.feedback, chorus.mix,
                                           phaser.stages, phaser.rate, phaser.depth, phaser.center, phaser.feedback, phaser.stereo, phaser.mix,
                                           flanger.rate, flanger.depth, flanger.delay, flanger.feedback, flanger.stereo, flanger.mix,
                                           delay.tempoSync, delay.time, delay.division, delay.feedback, delay.pingPong, delay.lowCut, delay.highCut, delay.mix,
                                           reverb.predelay, reverb.decay, reverb.damping, reverb.width, reverb.mix });
    return parameters;
}

SynthEffectsValues SynthEffectsParameters::read (SynthParameterReader reader) const noexcept
{
    SynthEffectsValues values;

    values.order = decodeOrder (reader.asInt (order));

    for (std::size_t index = 0; index < enabled.size(); ++index)
        values.enabled[index] = reader.asInt (enabled[index]) != 0;

    values.bitcrusher = { reader (bitcrusher.bits), reader (bitcrusher.rate), reader (bitcrusher.dither), reader (bitcrusher.mix) };

    values.chorus = { static_cast<SynthChorusMode> (yup::jlimit (0, 3, reader.asInt (chorus.mode))),
                      reader (chorus.rate), reader (chorus.depth), reader (chorus.delay), reader (chorus.feedback), reader (chorus.mix) };

    values.phaser = { 4 * (yup::jlimit (0, 2, reader.asInt (phaser.stages)) + 1),
                      reader (phaser.rate), reader (phaser.depth), reader (phaser.center), reader (phaser.feedback), reader (phaser.stereo), reader (phaser.mix) };

    values.flanger = { reader (flanger.rate), reader (flanger.depth), reader (flanger.delay), reader (flanger.feedback), reader (flanger.stereo), reader (flanger.mix) };

    values.delay.tempoSync = reader.asInt (delay.tempoSync) != 0;
    values.delay.timeMs = reader (delay.time);
    values.delay.division = reader.asInt (delay.division);
    values.delay.feedback = reader (delay.feedback);
    values.delay.pingPong = reader.asInt (delay.pingPong) != 0;
    values.delay.lowCut = reader (delay.lowCut);
    values.delay.highCut = reader (delay.highCut);
    values.delay.mix = reader (delay.mix);

    values.reverb = { reader (reverb.predelay), reader (reverb.decay), reader (reverb.damping), reader (reverb.width), reader (reverb.mix) };

    return values;
}

SynthEffectOrder SynthEffectsParameters::decodeOrder (int index) noexcept
{
    index = yup::jlimit (0, orderCount - 1, index);

    std::array<int, SynthExample::effectCount> remaining { 0, 1, 2, 3, 4, 5 };
    auto numRemaining = SynthExample::effectCount;
    SynthEffectOrder order {};

    for (std::size_t position = 0; position < order.size(); ++position)
    {
        const auto pick = index / factorials[position];
        index %= factorials[position];

        order[position] = static_cast<SynthEffectType> (remaining[static_cast<std::size_t> (pick)]);

        for (auto shift = pick; shift < numRemaining - 1; ++shift)
            remaining[static_cast<std::size_t> (shift)] = remaining[static_cast<std::size_t> (shift + 1)];

        --numRemaining;
    }

    return order;
}

int SynthEffectsParameters::encodeOrder (const SynthEffectOrder& order) noexcept
{
    auto index = 0;

    for (std::size_t position = 0; position < order.size(); ++position)
    {
        auto smallerAfter = 0;

        for (auto later = position + 1; later < order.size(); ++later)
            smallerAfter += order[later] < order[position] ? 1 : 0;

        index += smallerAfter * factorials[position];
    }

    return index;
}
