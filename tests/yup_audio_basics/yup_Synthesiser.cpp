/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2024 - kunitoki@gmail.com

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

#include <yup_audio_basics/yup_audio_basics.h>

#include <gtest/gtest.h>

using namespace yup;

namespace
{

// Test-friendly Synthesiser that exposes protected methods for testing
class TestSynthesiser : public Synthesiser
{
public:
    using Synthesiser::handleMidiEvent;
    using Synthesiser::startVoice;
};

// Test implementation of SynthesiserSound
class TestSound : public SynthesiserSound
{
public:
    TestSound (int minNote = 0, int maxNote = 127, int channel = 0)
        : minNoteNumber (minNote)
        , maxNoteNumber (maxNote)
        , midiChannel (channel)
    {
    }

    bool appliesToNote (int midiNoteNumber) override
    {
        return midiNoteNumber >= minNoteNumber && midiNoteNumber <= maxNoteNumber;
    }

    bool appliesToChannel (int channel) override
    {
        return midiChannel == 0 || channel == midiChannel;
    }

private:
    int minNoteNumber, maxNoteNumber, midiChannel;
};

// Test implementation of SynthesiserVoice
class TestVoice : public SynthesiserVoice
{
public:
    TestVoice() = default;

    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast<TestSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity, SynthesiserSound* sound, int currentPitchWheelPosition) override
    {
        lastStartedNote = midiNoteNumber;
        lastVelocity = velocity;
        lastPitchWheel = currentPitchWheelPosition;
        noteStarted = true;
        noteStopped = false;

        // Store the current sound
        currentSound = sound;
    }

    void stopNote (float velocity, bool allowTailOff) override
    {
        lastStopVelocity = velocity;
        lastAllowTailOff = allowTailOff;
        noteStopped = true;
        stopCount++;

        if (! allowTailOff)
        {
            clearCurrentNote();
        }
    }

    void pitchWheelMoved (int newPitchWheelValue) override
    {
        lastPitchWheel = newPitchWheelValue;
        pitchWheelMoved_ = true;
    }

    void controllerMoved (int controllerNumber, int newControllerValue) override
    {
        lastController = controllerNumber;
        lastControllerValue = newControllerValue;
        controllerMoved_ = true;
    }

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        renderCalled = true;
        lastRenderStartSample = startSample;
        lastRenderNumSamples = numSamples;

        // Simple sine wave generation for testing
        if (isVoiceActive())
        {
            for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
            {
                auto* channelData = outputBuffer.getWritePointer (channel, startSample);
                for (int i = 0; i < numSamples; ++i)
                {
                    channelData[i] += std::sin (phase) * 0.1f;
                    phase += 0.1f;
                }
            }
        }
    }

    // Test state accessors
    bool wasNoteStarted() const { return noteStarted; }

    bool wasNoteStopped() const { return noteStopped; }

    bool wasRenderCalled() const { return renderCalled; }

    bool wasPitchWheelMoved() const { return pitchWheelMoved_; }

    bool wasControllerMoved() const { return controllerMoved_; }

    int getLastStartedNote() const { return lastStartedNote; }

    float getLastVelocity() const { return lastVelocity; }

    float getLastStopVelocity() const { return lastStopVelocity; }

    bool getLastAllowTailOff() const { return lastAllowTailOff; }

    int getLastController() const { return lastController; }

    int getLastControllerValue() const { return lastControllerValue; }

    int getLastPitchWheel() const { return lastPitchWheel; }

    int getLastRenderStartSample() const { return lastRenderStartSample; }

    int getLastRenderNumSamples() const { return lastRenderNumSamples; }

    int getStopCount() const { return stopCount; }

    void reset()
    {
        noteStarted = false;
        noteStopped = false;
        renderCalled = false;
        pitchWheelMoved_ = false;
        controllerMoved_ = false;
        lastStartedNote = -1;
        lastVelocity = 0.0f;
        lastStopVelocity = 0.0f;
        lastAllowTailOff = false;
        lastController = -1;
        lastControllerValue = -1;
        lastPitchWheel = 8192;
        lastRenderStartSample = -1;
        lastRenderNumSamples = -1;
        phase = 0.0f;
        stopCount = 0;
    }

private:
    bool noteStarted = false;
    bool noteStopped = false;
    bool renderCalled = false;
    bool pitchWheelMoved_ = false;
    bool controllerMoved_ = false;

    int lastStartedNote = -1;
    float lastVelocity = 0.0f;
    float lastStopVelocity = 0.0f;
    bool lastAllowTailOff = false;
    int lastController = -1;
    int lastControllerValue = -1;
    int lastPitchWheel = 8192;
    int lastRenderStartSample = -1;
    int lastRenderNumSamples = -1;
    int stopCount = 0;

    float phase = 0.0f;
    SynthesiserSound* currentSound = nullptr;
};

} // namespace

class SynthesiserTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        synth = std::make_unique<TestSynthesiser>();
        synth->setCurrentPlaybackSampleRate (44100.0);
    }

    void TearDown() override
    {
        synth.reset();
    }

    std::unique_ptr<TestSynthesiser> synth;
};

TEST_F (SynthesiserTest, DefaultConstruction)
{
    Synthesiser synthesiser;
    EXPECT_EQ (synthesiser.getNumVoices(), 0);
    EXPECT_EQ (synthesiser.getNumSounds(), 0);
    EXPECT_TRUE (synthesiser.isNoteStealingEnabled());
    EXPECT_EQ (synthesiser.getSampleRate(), 0.0);
}

TEST_F (SynthesiserTest, VoiceManagement)
{
    EXPECT_EQ (synth->getNumVoices(), 0);
    EXPECT_EQ (synth->getVoice (0), nullptr);

    // Add voices
    auto* voice1 = synth->addVoice (new TestVoice());
    EXPECT_EQ (synth->getNumVoices(), 1);
    EXPECT_EQ (synth->getVoice (0).get(), voice1);
    EXPECT_NE (voice1, nullptr);

    auto* voice2 = synth->addVoice (new TestVoice());
    EXPECT_EQ (synth->getNumVoices(), 2);
    EXPECT_EQ (synth->getVoice (1).get(), voice2);
    EXPECT_NE (voice2, nullptr);

    // Remove voice
    synth->removeVoice (0);
    EXPECT_EQ (synth->getNumVoices(), 1);
    EXPECT_EQ (synth->getVoice (0).get(), voice2);

    // Clear all voices
    synth->clearVoices();
    EXPECT_EQ (synth->getNumVoices(), 0);
}

TEST_F (SynthesiserTest, SoundManagement)
{
    EXPECT_EQ (synth->getNumSounds(), 0);
    EXPECT_EQ (synth->getSound (0), nullptr);

    // Add sounds
    auto sound1 = SynthesiserSound::Ptr (new TestSound (60, 72, 1));
    auto* soundPtr1 = synth->addSound (sound1);
    EXPECT_EQ (synth->getNumSounds(), 1);
    EXPECT_EQ (synth->getSound (0), sound1);
    EXPECT_EQ (soundPtr1, sound1.get());

    auto sound2 = SynthesiserSound::Ptr (new TestSound (36, 48, 2));
    auto* soundPtr2 = synth->addSound (sound2);
    EXPECT_EQ (synth->getNumSounds(), 2);
    EXPECT_EQ (synth->getSound (1), sound2);
    EXPECT_EQ (soundPtr2, sound2.get());

    // Remove sound
    synth->removeSound (0);
    EXPECT_EQ (synth->getNumSounds(), 1);
    EXPECT_EQ (synth->getSound (0), sound2);

    // Clear all sounds
    synth->clearSounds();
    EXPECT_EQ (synth->getNumSounds(), 0);
}

TEST_F (SynthesiserTest, NoteStealingConfiguration)
{
    EXPECT_TRUE (synth->isNoteStealingEnabled());

    synth->setNoteStealingEnabled (false);
    EXPECT_FALSE (synth->isNoteStealingEnabled());

    synth->setNoteStealingEnabled (true);
    EXPECT_TRUE (synth->isNoteStealingEnabled());
}

TEST_F (SynthesiserTest, SampleRateConfiguration)
{
    EXPECT_EQ (synth->getSampleRate(), 44100.0);

    synth->setCurrentPlaybackSampleRate (48000.0);
    EXPECT_EQ (synth->getSampleRate(), 48000.0);

    // Verify voices get the sample rate
    auto* voice = synth->addVoice (new TestVoice());
    EXPECT_EQ (voice->getSampleRate(), 48000.0);
}

TEST_F (SynthesiserTest, MinimumRenderingSubdivision)
{
    // This tests the setter - we can't easily test the getter as it's private
    synth->setMinimumRenderingSubdivisionSize (64, true);
    synth->setMinimumRenderingSubdivisionSize (16, false);
    // No assertion needed - just ensure it doesn't crash
}

TEST_F (SynthesiserTest, NoteOnHandling)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    voice->reset();

    // Trigger note on
    synth->noteOn (1, 60, 0.8f);

    EXPECT_TRUE (voice->wasNoteStarted());
    EXPECT_EQ (voice->getLastStartedNote(), 60);
    EXPECT_FLOAT_EQ (voice->getLastVelocity(), 0.8f);
    EXPECT_EQ (voice->getCurrentlyPlayingNote(), 60);
    EXPECT_TRUE (voice->isVoiceActive());
}

TEST_F (SynthesiserTest, NoteOffHandling)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start note
    synth->noteOn (1, 60, 0.8f);
    voice->reset();

    // Stop note
    synth->noteOff (1, 60, 0.5f, true);

    EXPECT_TRUE (voice->wasNoteStopped());
    EXPECT_FLOAT_EQ (voice->getLastStopVelocity(), 0.5f);
    EXPECT_TRUE (voice->getLastAllowTailOff());
}

TEST_F (SynthesiserTest, AllNotesOff)
{
    // Setup multiple voices
    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start notes
    synth->noteOn (1, 60, 0.8f);
    synth->noteOn (2, 64, 0.7f);

    voice1->reset();
    voice2->reset();

    // All notes off
    synth->allNotesOff (0, false); // Channel 0 means all channels

    EXPECT_TRUE (voice1->wasNoteStopped());
    EXPECT_TRUE (voice2->wasNoteStopped());
    EXPECT_FALSE (voice1->getLastAllowTailOff());
    EXPECT_FALSE (voice2->getLastAllowTailOff());
}

TEST_F (SynthesiserTest, PitchWheelHandling)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start note
    synth->noteOn (1, 60, 0.8f);
    voice->reset();

    // Send pitch wheel
    synth->handlePitchWheel (1, 10000);

    EXPECT_TRUE (voice->wasPitchWheelMoved());
    EXPECT_EQ (voice->getLastPitchWheel(), 10000);
}

TEST_F (SynthesiserTest, ControllerHandling)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start note
    synth->noteOn (1, 60, 0.8f);
    voice->reset();

    // Send controller
    synth->handleController (1, 7, 100); // Volume controller

    EXPECT_TRUE (voice->wasControllerMoved());
    EXPECT_EQ (voice->getLastController(), 7);
    EXPECT_EQ (voice->getLastControllerValue(), 100);
}

TEST_F (SynthesiserTest, SustainPedalHandling)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start and stop note
    synth->noteOn (1, 60, 0.8f);
    EXPECT_TRUE (voice->isVoiceActive());

    // Enable sustain pedal
    synth->handleSustainPedal (1, true);
    EXPECT_TRUE (voice->isSustainPedalDown());

    // Release key - should still be active due to sustain
    synth->noteOff (1, 60, 0.5f, true);
    EXPECT_TRUE (voice->isVoiceActive());
    EXPECT_FALSE (voice->isKeyDown());

    // Release sustain pedal
    voice->reset();
    synth->handleSustainPedal (1, false);
    EXPECT_FALSE (voice->isSustainPedalDown());
    EXPECT_TRUE (voice->wasNoteStopped());
}

TEST_F (SynthesiserTest, SostenutoPedalHandling)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start note
    synth->noteOn (1, 60, 0.8f);
    EXPECT_TRUE (voice->isVoiceActive());

    // Enable sostenuto pedal
    synth->handleSostenutoPedal (1, true);
    EXPECT_TRUE (voice->isSostenutoPedalDown());

    // Release key
    synth->noteOff (1, 60, 0.5f, true);
    EXPECT_TRUE (voice->isVoiceActive());
    EXPECT_FALSE (voice->isKeyDown());
    EXPECT_FALSE (voice->wasNoteStopped());

    // Release sostenuto pedal
    synth->handleSostenutoPedal (1, false);
    EXPECT_TRUE (voice->wasNoteStopped());

    voice->reset();
    EXPECT_FALSE (voice->wasNoteStopped());
}

TEST_F (SynthesiserTest, AftertouchHandling)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start note
    synth->noteOn (1, 60, 0.8f);

    // Send aftertouch (calls through to voice but base implementation does nothing)
    synth->handleAftertouch (1, 60, 80);
    // No assertion needed - just ensure it doesn't crash
}

TEST_F (SynthesiserTest, ChannelPressureHandling)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start note
    synth->noteOn (1, 60, 0.8f);

    // Send channel pressure (calls through to voice but base implementation does nothing)
    synth->handleChannelPressure (1, 100);
    // No assertion needed - just ensure it doesn't crash
}

TEST_F (SynthesiserTest, ProgramChangeHandling)
{
    // Base implementation does nothing, just ensure it doesn't crash
    synth->handleProgramChange (1, 5);
}

TEST_F (SynthesiserTest, SoftPedalHandling)
{
    // Base implementation does nothing, just ensure it doesn't crash
    synth->handleSoftPedal (1, true);
    synth->handleSoftPedal (1, false);
}

TEST_F (SynthesiserTest, AudioRendering)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Create audio buffer
    AudioBuffer<float> buffer (2, 64);
    buffer.clear();

    // Create empty MIDI buffer
    MidiBuffer midiBuffer;

    // Start note
    synth->noteOn (1, 60, 0.8f);
    voice->reset();

    // Render audio
    synth->renderNextBlock (buffer, midiBuffer, 0, 64);

    EXPECT_TRUE (voice->wasRenderCalled());
    EXPECT_EQ (voice->getLastRenderStartSample(), 0);
    EXPECT_EQ (voice->getLastRenderNumSamples(), 64);

    // Check that audio was generated (not all zeros)
    bool hasNonZeroSamples = false;
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getReadPointer (channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (channelData[i] != 0.0f)
            {
                hasNonZeroSamples = true;
                break;
            }
        }
    }
    EXPECT_TRUE (hasNonZeroSamples);
}

TEST_F (SynthesiserTest, AudioRenderingDouble)
{
    // Test double precision rendering
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    AudioBuffer<double> buffer (2, 64);
    buffer.clear();

    MidiBuffer midiBuffer;

    synth->noteOn (1, 60, 0.8f);
    synth->renderNextBlock (buffer, midiBuffer, 0, 64);

    // Just ensure it doesn't crash - the TestVoice only implements float rendering
}

TEST_F (SynthesiserTest, MidiMessageProcessing)
{
    // Setup
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Create MIDI buffer with various messages
    MidiBuffer midiBuffer;
    midiBuffer.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midiBuffer.addEvent (MidiMessage::controllerEvent (1, 7, 100), 16);
    midiBuffer.addEvent (MidiMessage::pitchWheel (1, 10000), 32);
    midiBuffer.addEvent (MidiMessage::noteOff (1, 60, 0.5f), 48);

    AudioBuffer<float> buffer (2, 64);
    buffer.clear();

    voice->reset();

    // Process MIDI
    synth->renderNextBlock (buffer, midiBuffer, 0, 64);

    EXPECT_TRUE (voice->wasNoteStarted());
    EXPECT_TRUE (voice->wasNoteStopped());
    EXPECT_TRUE (voice->wasControllerMoved());
    EXPECT_TRUE (voice->wasPitchWheelMoved());
}

TEST_F (SynthesiserTest, SoundChannelFiltering)
{
    // Setup voices and sounds for different channels
    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));

    auto sound1 = SynthesiserSound::Ptr (new TestSound (60, 72, 1)); // Channel 1 only
    auto sound2 = SynthesiserSound::Ptr (new TestSound (60, 72, 2)); // Channel 2 only
    synth->addSound (sound1);
    synth->addSound (sound2);

    voice1->reset();
    voice2->reset();

    // Trigger note on channel 1
    synth->noteOn (1, 60, 0.8f);

    // Only voice1 should be triggered (first available voice for channel 1 sound)
    EXPECT_TRUE (voice1->wasNoteStarted());
    EXPECT_FALSE (voice2->wasNoteStarted());

    voice1->reset();
    voice2->reset();

    // Trigger note on channel 2
    synth->noteOn (2, 60, 0.8f);

    // voice2 should be triggered for channel 2 sound (voice1 is busy)
    EXPECT_FALSE (voice1->wasNoteStarted());
    EXPECT_TRUE (voice2->wasNoteStarted());
}

TEST_F (SynthesiserTest, SoundNoteRangeFiltering)
{
    // Setup sounds with different note ranges
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));

    auto sound1 = SynthesiserSound::Ptr (new TestSound (60, 72, 0)); // C4-C5
    auto sound2 = SynthesiserSound::Ptr (new TestSound (36, 48, 0)); // C2-C3
    synth->addSound (sound1);
    synth->addSound (sound2);

    voice->reset();

    // Trigger note in first range
    synth->noteOn (1, 60, 0.8f);
    EXPECT_TRUE (voice->wasNoteStarted());
    EXPECT_EQ (voice->getLastStartedNote(), 60);

    synth->allNotesOff (0, false);
    voice->reset();

    // Trigger note in second range
    synth->noteOn (1, 40, 0.7f);
    EXPECT_TRUE (voice->wasNoteStarted());
    EXPECT_EQ (voice->getLastStartedNote(), 40);

    synth->allNotesOff (0, false);
    voice->reset();

    // Trigger note outside both ranges
    synth->noteOn (1, 80, 0.6f);
    EXPECT_FALSE (voice->wasNoteStarted()); // No sound should apply
}

TEST_F (SynthesiserTest, VoiceStateManagement)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Initial state
    EXPECT_FALSE (voice->isVoiceActive());
    EXPECT_EQ (voice->getCurrentlyPlayingNote(), -1);
    EXPECT_EQ (voice->getCurrentlyPlayingSound(), nullptr);

    // Start note
    synth->noteOn (1, 60, 0.8f);
    EXPECT_TRUE (voice->isVoiceActive());
    EXPECT_EQ (voice->getCurrentlyPlayingNote(), 60);
    EXPECT_NE (voice->getCurrentlyPlayingSound(), nullptr);
    EXPECT_TRUE (voice->isKeyDown());

    // Stop note without tail-off
    synth->noteOff (1, 60, 0.5f, false);
    EXPECT_FALSE (voice->isVoiceActive());
    EXPECT_EQ (voice->getCurrentlyPlayingNote(), -1);
    EXPECT_EQ (voice->getCurrentlyPlayingSound(), nullptr);
    EXPECT_FALSE (voice->isKeyDown());
}

//==============================================================================
// Additional coverage tests for uncovered lines

TEST_F (SynthesiserTest, WasStartedBeforeComparison)
{
    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start notes in sequence
    synth->noteOn (1, 60, 0.8f);
    synth->noteOn (1, 64, 0.7f);

    // voice1 was started before voice2
    EXPECT_TRUE (voice1->wasStartedBefore (*voice2));
    EXPECT_FALSE (voice2->wasStartedBefore (*voice1));
}

TEST_F (SynthesiserTest, SetCurrentPlaybackSampleRateUpdatesVoices)
{
    // Add voice after initial sample rate is set
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    EXPECT_EQ (voice->getSampleRate(), 44100.0);

    // Change sample rate - should update all voices
    synth->setCurrentPlaybackSampleRate (48000.0);
    EXPECT_EQ (voice->getSampleRate(), 48000.0);

    // Add another voice after rate change
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    EXPECT_EQ (voice2->getSampleRate(), 48000.0);
}

TEST_F (SynthesiserTest, SetCurrentPlaybackSampleRateClearsActiveNotes)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start a note
    synth->noteOn (1, 60, 0.8f);
    EXPECT_TRUE (voice->isVoiceActive());

    voice->reset();

    // Change sample rate should stop all notes
    synth->setCurrentPlaybackSampleRate (48000.0);
    EXPECT_TRUE (voice->wasNoteStopped());
}

TEST_F (SynthesiserTest, HandleMidiEventAllNotesOff)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    synth->noteOn (1, 60, 0.8f);
    voice->reset();

    // Send all notes off message
    MidiMessage msg = MidiMessage::allNotesOff (1);
    synth->handleMidiEvent (msg);

    EXPECT_TRUE (voice->wasNoteStopped());
}

TEST_F (SynthesiserTest, HandleMidiEventAllSoundOff)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    synth->noteOn (1, 60, 0.8f);
    voice->reset();

    // Send all sound off message
    MidiMessage msg = MidiMessage::allSoundOff (1);
    synth->handleMidiEvent (msg);

    EXPECT_TRUE (voice->wasNoteStopped());
}

TEST_F (SynthesiserTest, HandleMidiEventAftertouch)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    synth->noteOn (1, 60, 0.8f);

    // Send aftertouch message
    MidiMessage msg = MidiMessage::aftertouchChange (1, 60, 80);
    synth->handleMidiEvent (msg);

    // Just verify it doesn't crash - base implementation does nothing
}

TEST_F (SynthesiserTest, HandleMidiEventChannelPressure)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    synth->noteOn (1, 60, 0.8f);

    // Send channel pressure message
    MidiMessage msg = MidiMessage::channelPressureChange (1, 100);
    synth->handleMidiEvent (msg);

    // Just verify it doesn't crash - base implementation does nothing
}

TEST_F (SynthesiserTest, HandleMidiEventProgramChange)
{
    // Send program change message
    MidiMessage msg = MidiMessage::programChange (1, 42);
    synth->handleMidiEvent (msg);

    // Just verify it doesn't crash - base implementation does nothing
}

TEST_F (SynthesiserTest, NoteOnStopsExistingNote)
{
    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start note 60
    synth->noteOn (1, 60, 0.8f);
    EXPECT_EQ (voice1->getCurrentlyPlayingNote(), 60);
    EXPECT_TRUE (voice1->isVoiceActive());

    // Add another voice to avoid immediate restart on the same voice
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));

    // Start the same note again - should stop voice1 first and start voice2
    synth->noteOn (1, 60, 0.7f);

    // voice1 should have been stopped with tail-off
    EXPECT_TRUE (voice1->wasNoteStopped());
    EXPECT_TRUE (voice1->getLastAllowTailOff());

    // voice2 should have started the new note
    EXPECT_TRUE (voice2->wasNoteStarted());
    EXPECT_EQ (voice2->getCurrentlyPlayingNote(), 60);
}

TEST_F (SynthesiserTest, StartVoiceStopsActiveVoice)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start a note
    synth->noteOn (1, 60, 0.8f);
    EXPECT_TRUE (voice->isVoiceActive());
    EXPECT_NE (voice->getCurrentlyPlayingSound(), nullptr);
    EXPECT_EQ (voice->getStopCount(), 0);

    // Manually start the same voice again (simulating voice stealing scenario)
    // This forces the voice->stopNote (0.0f, false) path in startVoice (line 352)
    synth->startVoice (voice, sound.get(), 1, 64, 0.7f);

    // Voice should have been stopped without tail-off and restarted with new note
    EXPECT_EQ (voice->getStopCount(), 1); // stopNote was called once
    EXPECT_FALSE (voice->getLastAllowTailOff());
    EXPECT_EQ (voice->getCurrentlyPlayingNote(), 64);
}

TEST_F (SynthesiserTest, HandleControllerSustainPedal)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    synth->noteOn (1, 60, 0.8f);

    // Send sustain pedal on (controller 0x40, value >= 64)
    synth->handleController (1, 0x40, 127);
    EXPECT_TRUE (voice->isSustainPedalDown());

    // Release key
    synth->noteOff (1, 60, 0.5f, true);
    EXPECT_FALSE (voice->isKeyDown());
    EXPECT_TRUE (voice->isVoiceActive()); // Still active due to sustain

    voice->reset();

    // Send sustain pedal off (controller 0x40, value < 64)
    synth->handleController (1, 0x40, 0);
    EXPECT_FALSE (voice->isSustainPedalDown());
    EXPECT_TRUE (voice->wasNoteStopped());
}

TEST_F (SynthesiserTest, HandleControllerSostenutoPedal)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    synth->noteOn (1, 60, 0.8f);

    // Send sostenuto pedal on (controller 0x42, value >= 64)
    synth->handleController (1, 0x42, 127);
    EXPECT_TRUE (voice->isSostenutoPedalDown());

    voice->reset();

    // Send sostenuto pedal off (controller 0x42, value < 64)
    synth->handleController (1, 0x42, 0);
    EXPECT_TRUE (voice->wasNoteStopped());
}

TEST_F (SynthesiserTest, HandleControllerSoftPedal)
{
    // Send soft pedal on (controller 0x43, value >= 64)
    synth->handleController (1, 0x43, 127);

    // Send soft pedal off (controller 0x43, value < 64)
    synth->handleController (1, 0x43, 0);

    // Just verify it doesn't crash - base implementation does nothing
}

TEST_F (SynthesiserTest, ProcessNextBlockWithMidiAtEndOfBuffer)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    AudioBuffer<float> buffer (2, 64);
    buffer.clear();

    MidiBuffer midiBuffer;
    // Add MIDI event at the exact end of the buffer
    midiBuffer.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 64);

    voice->reset();

    synth->renderNextBlock (buffer, midiBuffer, 0, 64);

    // The note should be handled but not rendered in this block
    EXPECT_TRUE (voice->wasNoteStarted());
}

TEST_F (SynthesiserTest, ProcessNextBlockWithMidiAfterRenderRegion)
{
    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    AudioBuffer<float> buffer (2, 128);
    buffer.clear();

    MidiBuffer midiBuffer;
    // Add MIDI events throughout and after the render region
    midiBuffer.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);
    midiBuffer.addEvent (MidiMessage::controllerEvent (1, 7, 100), 64);
    midiBuffer.addEvent (MidiMessage::noteOff (1, 60, 0.5f), 96);

    voice->reset();

    // Render only first 64 samples, but buffer has events beyond
    synth->renderNextBlock (buffer, midiBuffer, 0, 64);

    EXPECT_TRUE (voice->wasNoteStarted());
    EXPECT_TRUE (voice->wasControllerMoved());
    EXPECT_TRUE (voice->wasNoteStopped());
}

TEST_F (SynthesiserTest, FindFreeVoiceWithoutStealing)
{
    // Don't enable note stealing
    synth->setNoteStealingEnabled (false);

    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start notes to occupy all voices
    synth->noteOn (1, 60, 0.8f);
    synth->noteOn (1, 64, 0.7f);

    EXPECT_EQ (voice1->getCurrentlyPlayingNote(), 60);
    EXPECT_EQ (voice2->getCurrentlyPlayingNote(), 64);

    // Try to start another note - should not trigger (no free voices, stealing disabled)
    synth->noteOn (1, 67, 0.6f);

    // Neither voice should have changed notes
    EXPECT_EQ (voice1->getCurrentlyPlayingNote(), 60);
    EXPECT_EQ (voice2->getCurrentlyPlayingNote(), 64);
}

TEST_F (SynthesiserTest, FindFreeVoiceWithStealingEnabled)
{
    synth->setNoteStealingEnabled (true);

    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start notes to occupy all voices
    synth->noteOn (1, 60, 0.8f);
    synth->noteOn (1, 64, 0.7f);

    // Try to start another note - should steal a voice
    synth->noteOn (1, 67, 0.6f);

    // One of the voices should now be playing note 67
    EXPECT_TRUE (voice1->getCurrentlyPlayingNote() == 67 || voice2->getCurrentlyPlayingNote() == 67);
}

TEST_F (SynthesiserTest, VoiceStealingPrefersOldestNote)
{
    synth->setNoteStealingEnabled (true);

    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start notes in sequence
    synth->noteOn (1, 60, 0.8f); // Oldest, lowest note
    synth->noteOn (1, 72, 0.7f); // Newer, highest note

    // Both notes are currently held (not released), so they're protected
    // The algorithm protects lowest and highest notes
    // Since both are protected and we only have 2 voices, it will steal the top one
    synth->noteOn (1, 67, 0.6f);

    // One voice should now be playing note 67
    bool voice1Has67 = (voice1->getCurrentlyPlayingNote() == 67);
    bool voice2Has67 = (voice2->getCurrentlyPlayingNote() == 67);
    EXPECT_TRUE (voice1Has67 || voice2Has67);
}

TEST_F (SynthesiserTest, VoiceStealingPrefersSameNote)
{
    synth->setNoteStealingEnabled (true);

    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start notes
    synth->noteOn (1, 60, 0.8f);
    synth->noteOn (1, 64, 0.7f);

    // Trigger the same note again - should steal the voice already playing that note
    synth->noteOn (1, 60, 0.9f);

    // voice1 should have been restarted with same note
    EXPECT_EQ (voice1->getCurrentlyPlayingNote(), 60);
    EXPECT_EQ (voice2->getCurrentlyPlayingNote(), 64);
}

TEST_F (SynthesiserTest, VoiceStealingPrefersReleasedNotes)
{
    synth->setNoteStealingEnabled (true);

    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start notes with different pitches so they're not protected the same way
    synth->noteOn (1, 60, 0.8f); // Lower note
    synth->noteOn (1, 72, 0.7f); // Higher note

    // Release first note (without sustain, so it's fully released)
    synth->noteOff (1, 60, 0.5f, true);

    // voice1 should be released (not held by key)
    EXPECT_FALSE (voice1->isKeyDown());
    EXPECT_TRUE (voice2->isKeyDown());

    // Start a new note - should prefer stealing the released voice
    synth->noteOn (1, 67, 0.6f);

    // The released voice should have been stolen
    // Since voice1 was released and voice2 is still held, voice1 should be stolen
    EXPECT_EQ (voice1->getCurrentlyPlayingNote(), 67);
    EXPECT_EQ (voice2->getCurrentlyPlayingNote(), 72);
}

TEST_F (SynthesiserTest, VoiceStealingProtectsLowestAndHighestNotes)
{
    synth->setNoteStealingEnabled (true);

    // Add 3 voices
    auto* voice1 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice2 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto* voice3 = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start notes: low, middle, high
    synth->noteOn (1, 48, 0.8f); // Low - protected
    synth->noteOn (1, 60, 0.7f); // Middle - not protected
    synth->noteOn (1, 72, 0.6f); // High - protected

    // Start a new note - should steal the middle note (voice2)
    synth->noteOn (1, 64, 0.5f);

    EXPECT_EQ (voice1->getCurrentlyPlayingNote(), 48); // Low protected
    EXPECT_EQ (voice2->getCurrentlyPlayingNote(), 64); // Was stolen
    EXPECT_EQ (voice3->getCurrentlyPlayingNote(), 72); // High protected
}

TEST_F (SynthesiserTest, VoiceStealingWithOnlyOneNote)
{
    synth->setNoteStealingEnabled (true);

    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    // Start one note
    synth->noteOn (1, 60, 0.8f);
    EXPECT_EQ (voice->getCurrentlyPlayingNote(), 60);

    // Start another note - should steal the only voice
    synth->noteOn (1, 64, 0.7f);
    EXPECT_EQ (voice->getCurrentlyPlayingNote(), 64);
}

TEST_F (SynthesiserTest, MinimumRenderingSubdivisionStrictMode)
{
    synth->setMinimumRenderingSubdivisionSize (32, true);

    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    AudioBuffer<float> buffer (2, 128);
    buffer.clear();

    MidiBuffer midiBuffer;
    // Add MIDI event very early (at sample 1)
    midiBuffer.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 1);

    voice->reset();

    synth->renderNextBlock (buffer, midiBuffer, 0, 128);

    // In strict mode, the minimum subdivision should be enforced
    EXPECT_TRUE (voice->wasNoteStarted());
}

TEST_F (SynthesiserTest, MinimumRenderingSubdivisionNonStrictMode)
{
    synth->setMinimumRenderingSubdivisionSize (32, false);

    auto* voice = static_cast<TestVoice*> (synth->addVoice (new TestVoice()));
    auto sound = SynthesiserSound::Ptr (new TestSound());
    synth->addSound (sound);

    AudioBuffer<float> buffer (2, 128);
    buffer.clear();

    MidiBuffer midiBuffer;
    // Add MIDI event at sample 0 (first event in non-strict mode can be at 0)
    midiBuffer.addEvent (MidiMessage::noteOn (1, 60, 0.8f), 0);

    voice->reset();

    synth->renderNextBlock (buffer, midiBuffer, 0, 128);

    // In non-strict mode, first event can render immediately
    EXPECT_TRUE (voice->wasNoteStarted());
}
