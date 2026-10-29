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
yup::String formatDecibels (double value) { return yup::String (value >= 0.05 ? "+" : "") + yup::String (value, 1) + " dB"; }
yup::String formatRatio (double value) { return yup::String (value, 1) + ":1"; }
yup::String formatQ (double value) { return yup::String (value, 2); }
} // namespace

SynthEffectsParameters::SynthEffectsParameters()
{
    using namespace SynthParameterFactory;

    const SynthEffectsValues defaults;
    const auto names = getSynthEffectNames();

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

    {
        const auto id = prefix ("distortion");
        const auto base = hostBase (SynthEffectType::distortion);
        const auto& values = defaults.distortion;

        distortion.type = createChoice (id + "type", "Distortion Type", base + 1, "Distortion", getSynthDistortionTypeNames(), static_cast<int> (values.type));
        distortion.drive = createContinuous (id + "drive", "Distortion Drive", base + 2, "Distortion", { 0.0f, 48.0f }, values.driveDb, formatDecibels);
        distortion.bias = createContinuous (id + "bias", "Distortion Bias", base + 3, "Distortion", { -1.0f, 1.0f }, values.bias, SynthFormat::bipolar);
        distortion.tone = createContinuous (id + "tone", "Distortion Tone", base + 4, "Distortion", skewed (500.0f, 20000.0f, 4000.0f), values.tone, SynthFormat::hertz);
        distortion.output = createContinuous (id + "output", "Distortion Output", base + 5, "Distortion", { -24.0f, 12.0f }, values.outputDb, formatDecibels);
        distortion.mix = createContinuous (id + "mix", "Distortion Mix", base + 6, "Distortion", { 0.0f, 1.0f }, values.mix, SynthFormat::percent);
    }

    {
        const auto id = prefix ("eq");
        const auto base = hostBase (SynthEffectType::equalizer);
        const auto& values = defaults.equalizer;

        equalizer.lowGain = createContinuous (id + "lowgain", "EQ Low Gain", base + 1, "EQ", { -18.0f, 18.0f }, values.lowGainDb, formatDecibels);
        equalizer.lowFrequency = createContinuous (id + "lowfreq", "EQ Low Frequency", base + 2, "EQ", skewed (20.0f, 1000.0f, 150.0f), values.lowFrequency, SynthFormat::hertz);
        equalizer.midGain = createContinuous (id + "midgain", "EQ Mid Gain", base + 3, "EQ", { -18.0f, 18.0f }, values.midGainDb, formatDecibels);
        equalizer.midFrequency = createContinuous (id + "midfreq", "EQ Mid Frequency", base + 4, "EQ", skewed (100.0f, 10000.0f, 1000.0f), values.midFrequency, SynthFormat::hertz);
        equalizer.midQ = createContinuous (id + "midq", "EQ Mid Q", base + 5, "EQ", skewed (0.2f, 8.0f, 1.0f), values.midQ, formatQ);
        equalizer.highGain = createContinuous (id + "highgain", "EQ High Gain", base + 6, "EQ", { -18.0f, 18.0f }, values.highGainDb, formatDecibels);
        equalizer.highFrequency = createContinuous (id + "highfreq", "EQ High Frequency", base + 7, "EQ", skewed (1000.0f, 20000.0f, 5000.0f), values.highFrequency, SynthFormat::hertz);
    }

    {
        const auto id = prefix ("compressor");
        const auto base = hostBase (SynthEffectType::compressor);
        const auto& values = defaults.compressor;

        compressor.threshold = createContinuous (id + "threshold", "Compressor Threshold", base + 1, "Compressor", { -60.0f, 0.0f }, values.thresholdDb, formatDecibels);
        compressor.ratio = createContinuous (id + "ratio", "Compressor Ratio", base + 2, "Compressor", skewed (1.0f, 20.0f, 4.0f), values.ratio, formatRatio);
        compressor.attack = createContinuous (id + "attack", "Compressor Attack", base + 3, "Compressor", skewed (0.1f, 100.0f, 10.0f), values.attackMs, formatMilliseconds, parseMilliseconds);
        compressor.release = createContinuous (id + "release", "Compressor Release", base + 4, "Compressor", skewed (10.0f, 1000.0f, 150.0f), values.releaseMs, formatMilliseconds, parseMilliseconds);
        compressor.makeup = createContinuous (id + "makeup", "Compressor Makeup", base + 5, "Compressor", { 0.0f, 24.0f }, values.makeupDb, formatDecibels);
        compressor.mix = createContinuous (id + "mix", "Compressor Mix", base + 6, "Compressor", { 0.0f, 1.0f }, values.mix, SynthFormat::percent);
    }

    {
        const auto id = prefix ("surround");
        const auto base = hostBase (SynthEffectType::surround);
        const auto& values = defaults.surround;

        surround.width = createContinuous (id + "width", "Surround Width", base + 1, "Surround", { 0.0f, 2.0f }, values.width, SynthFormat::percent);
        surround.depth = createContinuous (id + "depth", "Surround Depth", base + 2, "Surround", { 0.0f, 1.0f }, values.depth, SynthFormat::percent);
        surround.time = createContinuous (id + "time", "Surround Time", base + 3, "Surround", { 1.0f, 30.0f }, values.timeMs, formatMilliseconds, parseMilliseconds);
        surround.monoBelow = createContinuous (id + "mono", "Surround Mono Below", base + 4, "Surround", skewed (20.0f, 500.0f, 120.0f), values.monoBelow, SynthFormat::hertz);
        surround.mix = createContinuous (id + "mix", "Surround Mix", base + 5, "Surround", { 0.0f, 1.0f }, values.mix, SynthFormat::percent);
    }
}

void SynthEffectsParameters::addTo (yup::AudioProcessor& processor) const
{
    for (const auto& parameter : getParameters())
        processor.addParameter (parameter);
}

std::vector<yup::AudioParameter::Ptr> SynthEffectsParameters::getParameters() const
{
    std::vector<yup::AudioParameter::Ptr> parameters (enabled.begin(), enabled.end());
    parameters.insert (parameters.end(), { bitcrusher.bits, bitcrusher.rate, bitcrusher.dither, bitcrusher.mix,
                                           chorus.mode, chorus.rate, chorus.depth, chorus.delay, chorus.feedback, chorus.mix,
                                           phaser.stages, phaser.rate, phaser.depth, phaser.center, phaser.feedback, phaser.stereo, phaser.mix,
                                           flanger.rate, flanger.depth, flanger.delay, flanger.feedback, flanger.stereo, flanger.mix,
                                           delay.tempoSync, delay.time, delay.division, delay.feedback, delay.pingPong, delay.lowCut, delay.highCut, delay.mix,
                                           reverb.predelay, reverb.decay, reverb.damping, reverb.width, reverb.mix,
                                           distortion.type, distortion.drive, distortion.bias, distortion.tone, distortion.output, distortion.mix,
                                           equalizer.lowGain, equalizer.lowFrequency, equalizer.midGain, equalizer.midFrequency, equalizer.midQ, equalizer.highGain, equalizer.highFrequency,
                                           compressor.threshold, compressor.ratio, compressor.attack, compressor.release, compressor.makeup, compressor.mix,
                                           surround.width, surround.depth, surround.time, surround.monoBelow, surround.mix });
    return parameters;
}

SynthEffectsValues SynthEffectsParameters::read (SynthParameterReader reader) const noexcept
{
    SynthEffectsValues values;

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

    values.distortion = { static_cast<SynthDistortionType> (yup::jlimit (0, 2, reader.asInt (distortion.type))),
                          reader (distortion.drive), reader (distortion.bias), reader (distortion.tone), reader (distortion.output), reader (distortion.mix) };

    values.equalizer = { reader (equalizer.lowGain), reader (equalizer.lowFrequency), reader (equalizer.midGain), reader (equalizer.midFrequency),
                         reader (equalizer.midQ), reader (equalizer.highGain), reader (equalizer.highFrequency) };

    values.compressor = { reader (compressor.threshold), reader (compressor.ratio), reader (compressor.attack), reader (compressor.release),
                          reader (compressor.makeup), reader (compressor.mix) };

    values.surround = { reader (surround.width), reader (surround.depth), reader (surround.time), reader (surround.monoBelow), reader (surround.mix) };

    return values;
}
