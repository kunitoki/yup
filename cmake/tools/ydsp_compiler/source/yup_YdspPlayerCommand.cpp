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

#include "yup_YdspCommands.h"

#include <yup_audio_devices/yup_audio_devices.h>
#include <yup_dsp_jit/yup_dsp_jit.h>

#include <array>
#include <atomic>
#include <csignal>
#include <cmath>
#include <iostream>
#include <future>

using namespace yup;

namespace
{

volatile std::sig_atomic_t interrupted = 0;

void interruptPlayback (int)
{
    interrupted = 1;
}

struct PlayerOptions
{
    File file;
    String main, audioType, input, output, midiInput, midiOutput;
    double sampleRate = 0;
    int blockSize = 0;
    bool hotreload = false;
    bool verbose = false;
    int testNote = -1;
};

struct MidiPacket
{
    std::array<uint8_t, 3> bytes {};
    int size = 0;
};

class MidiQueue
{
public:
    bool push (const MidiPacket& packet) noexcept
    {
        const auto w = write.load (std::memory_order_relaxed);
        const auto next = (w + 1) % packets.size();
        if (next == read.load (std::memory_order_acquire))
            return false;
        packets[w] = packet;
        write.store (next, std::memory_order_release);
        return true;
    }

    bool pop (MidiPacket& packet) noexcept
    {
        const auto r = read.load (std::memory_order_relaxed);
        if (r == write.load (std::memory_order_acquire))
            return false;
        packet = packets[r];
        read.store ((r + 1) % packets.size(), std::memory_order_release);
        return true;
    }

private:
    static_assert (std::atomic<size_t>::is_always_lock_free);
    std::array<MidiPacket, 2048> packets;
    std::atomic<size_t> read { 0 }, write { 0 };
};

String midiIdentifier (const Array<MidiDeviceInfo>& devices, const String& selection)
{
    for (const auto& device : devices)
        if (device.identifier == selection)
            return device.identifier;
    String result;
    for (const auto& device : devices)
        if (device.name == selection)
        {
            if (result.isNotEmpty())
                return {}; // Ambiguous names must be selected by identifier.
            result = device.identifier;
        }
    return result;
}

struct PlaybackState
{
    explicit PlaybackState (YdspAudioGraph&& value) : graph (std::move (value)) {}
    YdspAudioGraph graph;
    std::vector<YdspInputBuffer> inputs;
    std::vector<YdspOutputBuffer> outputs;
    std::vector<const MidiBuffer*> events;
    std::vector<float> inputAudio;
    MidiBuffer midiIn, midiOut;
};

class Player final : private AudioIODeviceCallback, private MidiInputCallback, private Timer
{
public:
    explicit Player (PlayerOptions value) : options (std::move (value)) {}
    ~Player() override
    {
        stopTimer();

        if (reload.valid())
            reload.wait();

        if (midiInput != nullptr)
            midiInput->stop();

        manager.removeAudioCallback (this);
        manager.closeAudioDevice();

        delete active;
        delete pending.exchange (nullptr);
        delete retired.exchange (nullptr);

        if (midiOutput != nullptr)
            for (int channel = 1; channel <= 16; ++channel)
                midiOutput->sendMessageNow (MidiMessage::allSoundOff (channel));
    }

    Result start()
    {
        auto compiled = compile();
        if (! compiled.wasOk())
            return Result::fail (compiled.getErrorMessage());

        const auto& graph = compiled.getReference();
        inputCount = graph.getInputStreamCount();
        outputCount = graph.getOutputStreamCount();

        if (options.testNote >= 0 && graph.getEventInputCount() == 0)
            return Result::fail ("--test-note requires a patch with an event input");

        if (options.audioType.isNotEmpty())
        {
            bool found = false;

            for (auto* type : manager.getAvailableDeviceTypes())
                found |= type->getTypeName() == options.audioType;

            if (! found)
                return Result::fail ("Unknown audio device type; use devices to list available types");

            manager.setCurrentAudioDeviceType (options.audioType, false);
        }

        AudioDeviceManager::AudioDeviceSetup setup;
        setup.inputDeviceName = options.input == "none" ? String() : options.input;
        setup.outputDeviceName = options.output;
        setup.sampleRate = options.sampleRate;
        setup.bufferSize = options.blockSize;

        if (options.verbose)
            std::cerr << "Opening audio: type=" << (options.audioType.isEmpty() ? String ("default") : options.audioType)
                      << " output=" << (options.output.isEmpty() ? String ("default") : options.output)
                      << " input=" << (options.input.isEmpty() ? String ("default") : options.input) << "\n";

        const auto error = manager.initialise (options.input == "none" ? 0 : inputCount,
                                               std::max (2, outputCount), nullptr, false, {}, &setup);
        if (error.isNotEmpty())
            return Result::fail ("Opening audio device: " + error);

        auto* const device = manager.getCurrentAudioDevice();
        if (device == nullptr)
            return Result::fail ("No audio device opened");

        sampleRate = device->getCurrentSampleRate();
        blockSize = device->getCurrentBufferSizeSamples();

        if (device->getActiveOutputChannels().countNumberOfSetBits() < outputCount
            || (options.input != "none" && device->getActiveInputChannels().countNumberOfSetBits() < inputCount))
            return Result::fail ("Audio device has fewer channels than the patch requires");

        auto state = prepare (std::move (compiled.getReference()));
        if (! state.wasOk())
            return Result::fail (state.getErrorMessage());

        active = state.getReference().release();
        if (options.midiInput.isNotEmpty() && options.midiInput != "none")
        {
            const auto id = midiIdentifier (MidiInput::getAvailableDevices(), options.midiInput);

            if (id.isEmpty() || (midiInput = MidiInput::openDevice (id, this)) == nullptr)
                return Result::fail ("Cannot open MIDI input '" + options.midiInput + "'; use its exact name or identifier from devices");
        }

        if (options.midiOutput.isNotEmpty() && options.midiOutput != "none")
        {
            const auto id = midiIdentifier (MidiOutput::getAvailableDevices(), options.midiOutput);

            if (id.isEmpty() || (midiOutput = MidiOutput::openDevice (id)) == nullptr)
                return Result::fail ("Cannot open MIDI output '" + options.midiOutput + "'; use its exact name or identifier from devices");
        }

        if (options.hotreload)
        {
            refreshWatchedFiles();
            snapshot = fileSnapshot();
        }

        testNoteRemaining = static_cast<int64_t> (sampleRate * 2.0);

        manager.addAudioCallback (this);

        if (midiInput != nullptr)
            midiInput->start();

        startTimer (5);
        std::cerr << "Playing " << options.file.getFileName() << " at " << sampleRate << " Hz, " << blockSize
                  << " samples. Ctrl+C to stop.\n";

        if (options.verbose)
        {
            const auto actualSetup = manager.getAudioDeviceSetup();
            std::cerr << "Audio backend: " << manager.getCurrentAudioDeviceType()
                      << "\nAudio output: " << actualSetup.outputDeviceName << " (" << device->getActiveOutputChannels().countNumberOfSetBits()
                      << " channels); input: " << (actualSetup.inputDeviceName.isEmpty() ? String ("none") : actualSetup.inputDeviceName)
                      << " (" << device->getActiveInputChannels().countNumberOfSetBits() << " channels)\n"
                      << "MIDI input: " << (midiInput != nullptr ? midiInput->getName() + " [" + midiInput->getDeviceInfo().identifier + "]" : String ("none"))
                      << "; MIDI output: " << (midiOutput != nullptr ? midiOutput->getName() + " [" + midiOutput->getDeviceInfo().identifier + "]" : String ("none"))
                      << "\nPatch streams: " << inputCount << " inputs, " << outputCount << " outputs; event inputs: " << active->graph.getEventInputCount() << "\n";
        }

        if (active->graph.getEventInputCount() > 0 && midiInput == nullptr && options.testNote < 0)
            std::cerr << "This patch has event inputs but no MIDI input is connected. Use --midi-input NAME or --test-note 60.\n";

        if (options.testNote >= 0)
            std::cerr << "Playing test note " << options.testNote << " on channel 1 for two seconds\n";

        return Result::ok();
    }

    bool failed() const noexcept { return failure.load() != Failure::none; }

private:
    enum class Failure { none, layoutChanged, formatChanged, stopped, deviceError, processing };

    void recordFailure (Failure reason) noexcept
    {
        auto expected = Failure::none;
        failure.compare_exchange_strong (expected, reason);
    }

    const char* processingError() const noexcept
    {
        switch (processResult.load())
        {
            case YdspProcessResult::ok: return "ok";
            case YdspProcessResult::invalidGraph: return "invalid graph";
            case YdspProcessResult::invalidBufferCount: return "incorrect stream buffer count";
            case YdspProcessResult::bufferTypeMismatch: return "stream buffer type mismatch";
            case YdspProcessResult::bufferTooShort: return "stream buffer too short";
            case YdspProcessResult::blockTooLarge: return "block exceeds prepared size";
            case YdspProcessResult::notPrepared: return "graph not prepared";
            case YdspProcessResult::invalidArgument: return "invalid argument or MIDI timestamp";
            case YdspProcessResult::invalidAutomation: return "invalid automation";
            case YdspProcessResult::unsupportedAutomation: return "unsupported automation";
        }
        return "unknown processing error";
    }

    ResultValue<YdspAudioGraph> compile()
    {
        YdspCompiler compiler;

        auto result = options.file.hasFileExtension ("ydsp-project")
                        ? compiler.compileProject (options.file, {}, options.main)
                        : compiler.compile (options.file.loadFileAsString(), options.file.getFullPathName());

        if (! result.wasOk() && compiler.getDiagnostics().hasErrors())
            return makeResultValueFail (compiler.getDiagnostics().toString());

        return result;
    }

    ResultValue<std::unique_ptr<PlaybackState>> prepare (YdspAudioGraph&& graph)
    {
        if (graph.getInputStreamCount() != inputCount || graph.getOutputStreamCount() != outputCount)
            return makeResultValueFail ("Reload changes audio channel counts; stop and run again");

        for (int i = 0; i < inputCount; ++i)
            if (graph.getInputStreamType (i) != YdspElementType::float32)
                return makeResultValueFail ("Playback requires float32 input streams");

        for (int i = 0; i < outputCount; ++i)
            if (graph.getOutputStreamType (i) != YdspElementType::float32)
                return makeResultValueFail ("Playback requires float32 output streams");

        const auto result = graph.prepare (sampleRate, blockSize);
        if (result.failed())
            return makeResultValueFail (result.getErrorMessage());

        graph.prewarmKernels();

        auto state = std::make_unique<PlaybackState> (std::move (graph));
        state->inputs.resize (static_cast<size_t> (inputCount));
        state->outputs.resize (static_cast<size_t> (outputCount));
        state->inputAudio.resize (static_cast<size_t> (blockSize) * static_cast<size_t> (inputCount), 0.0f);
        state->events.resize (static_cast<size_t> (state->graph.getEventInputCount()), &state->midiIn);
        state->midiIn.ensureSize (258 * 16);
        state->midiOut.ensureSize (state->graph.getMidiOutputBufferSizeBytes());
        return makeResultValueOk (std::move (state));
    }

    void audioDeviceIOCallbackWithContext (const float* const* in, int numIn, float* const* out,
                                           int numOut, int samples, const AudioIODeviceCallbackContext&) override
    {
        if (options.verbose)
            audioBlocks.fetch_add (1, std::memory_order_relaxed);

        const auto clearOutputs = [&]
        {
            for (int i = 0; i < numOut; ++i)
                if (out[i] != nullptr)
                    std::fill_n (out[i], samples, 0.0f);
        };

        if (retired.load (std::memory_order_acquire) == nullptr)
        {
            if (auto* next = pending.exchange (nullptr, std::memory_order_acq_rel))
            {
                retired.store (active, std::memory_order_release);
                active = next;
                for (int channel = 0; channel < 16; ++channel)
                    if (! outgoing.push ({ { static_cast<uint8_t> (0xb0 + channel), 120, 0 }, 3 }))
                        dropped.fetch_add (1, std::memory_order_relaxed);
            }
        }

        if (samples > blockSize || numOut < outputCount || (options.input != "none" && numIn < inputCount))
            recordFailure (Failure::layoutChanged);

        if (failed() || active == nullptr)
        {
            clearOutputs();
            return;
        }

        auto& state = *active;
        for (int i = 0; i < inputCount; ++i)
        {
            auto* buffer = state.inputAudio.data() + static_cast<size_t> (i) * static_cast<size_t> (blockSize);
            if (i < numIn && in[i] != nullptr)
                std::copy_n (in[i], samples, buffer);
            else
                std::fill_n (buffer, samples, 0.0f);

            state.inputs[static_cast<size_t> (i)] = Span<const float> (buffer, static_cast<size_t> (samples));
        }

        clearOutputs();

        for (int i = 0; i < outputCount; ++i)
        {
            if (out[i] == nullptr)
            {
                recordFailure (Failure::layoutChanged);
                return;
            }

            state.outputs[static_cast<size_t> (i)] = Span<float> (out[i], static_cast<size_t> (samples));
        }

        state.midiIn.clear();
        state.midiOut.clear();

        MidiPacket packet;

        if (inputOverflow.exchange (false, std::memory_order_relaxed))
        {
            for (int i = 0; i < 2048 && incoming.pop (packet); ++i)
            {
            }

            for (int channel = 1; channel <= 16; ++channel)
                state.midiIn.addEvent (MidiMessage::allSoundOff (channel), 0);
        }
        else
        {
            for (int i = 0; i < 256 && incoming.pop (packet); ++i)
            {
                state.midiIn.addEvent (packet.bytes.data(), packet.size, 0);

                if (options.verbose)
                    midiDelivered.fetch_add (1, std::memory_order_relaxed);
            }
        }

        if (options.testNote >= 0 && samples > 0)
        {
            if (! testNoteStarted)
            {
                state.midiIn.addEvent (MidiMessage::noteOn (1, options.testNote, 0.8f), 0);
                testNoteStarted = true;
            }

            if (testNoteRemaining >= 0 && testNoteRemaining < samples)
                state.midiIn.addEvent (MidiMessage::noteOff (1, options.testNote), static_cast<int> (testNoteRemaining));

            if (testNoteRemaining >= 0)
                testNoteRemaining -= samples;
        }

        YdspProcessRequest request { state.inputs, state.outputs, samples, state.events, {}, midiOutput != nullptr ? &state.midiOut : nullptr };

        const auto result = state.graph.process (request);
        if (result != YdspProcessResult::ok)
        {
            clearOutputs();
            processResult.store (result);
            recordFailure (Failure::processing);
            return;
        }

        if (options.verbose)
        {
            float peak = outputPeak.load (std::memory_order_relaxed);

            unsigned invalid = 0;
            for (int channel = 0; channel < outputCount; ++channel)
            {
                for (int i = 0; i < samples; ++i)
                    if (std::isfinite (out[channel][i]))
                        peak = std::max (peak, std::abs (out[channel][i]));
                    else
                        ++invalid;
            }

            outputPeak.store (peak, std::memory_order_relaxed);

            nonFiniteSamples.fetch_add (invalid, std::memory_order_relaxed);
        }

        if (outputCount == 1)
            for (int i = 1; i < numOut; ++i)
                if (out[i] != nullptr)
                    std::copy_n (out[0], samples, out[i]);

        for (const auto event : state.midiOut)
        {
            packet.size = event.numBytes;
            if (packet.size <= 3)
            {
                std::copy_n (event.data, packet.size, packet.bytes.data());
                if (! outgoing.push (packet))
                    dropped.fetch_add (1, std::memory_order_relaxed);
            }
        }
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        if (device->getCurrentSampleRate() != sampleRate || device->getCurrentBufferSizeSamples() != blockSize)
            recordFailure (Failure::formatChanged);
    }

    void audioDeviceStopped() override { recordFailure (Failure::stopped); }

    void audioDeviceError (const String& error) override
    {
        if (! deviceErrorClaimed.exchange (true))
        {
            error.copyToUTF8 (deviceErrorText.data(), deviceErrorText.size());
            deviceErrorReady.store (true, std::memory_order_release);
        }

        recordFailure (Failure::deviceError);
    }

    void handleIncomingMidiMessage (MidiInput*, const MidiMessage& message) override
    {
        MidiPacket packet;
        packet.size = message.getRawDataSize();

        if (packet.size < 1 || packet.size > 3 || message.isSysEx())
            return;

        if (options.verbose)
        {
            midiReceived.fetch_add (1, std::memory_order_relaxed);
            if (message.isNoteOn())
                noteOnsReceived.fetch_add (1, std::memory_order_relaxed);
        }

        std::copy_n (message.getRawData(), packet.size, packet.bytes.data());

        if (! incoming.push (packet))
        {
            inputOverflow.store (true, std::memory_order_relaxed);
            dropped.fetch_add (1, std::memory_order_relaxed);
        }
    }

    void refreshWatchedFiles()
    {
        YdspDiagnostics diagnostics;

        auto project = YdspProject::load (options.file, diagnostics);
        if (! project.wasOk())
            return;

        watched.clear();

        for (const auto& path : project.getReference().getSources())
            watched.push_back (options.file.getParentDirectory().getChildFile (path));
    }

    String fileSnapshot() const
    {
        String result = options.file.loadFileAsString();
        for (const auto& file : watched)
            result += "\n" + file.getFullPathName() + ":" + String (file.existsAsFile() ? 1 : 0) + ":" + file.loadFileAsString();
        return result;
    }

    void timerCallback() override
    {
        delete retired.exchange (nullptr, std::memory_order_acq_rel);

        MidiPacket packet;
        for (int i = 0; i < 2048 && outgoing.pop (packet); ++i)
            if (midiOutput != nullptr)
                midiOutput->sendMessageNow (MidiMessage (packet.bytes.data(), packet.size, 0.0));

        if (const auto count = dropped.exchange (0); count != 0)
        {
            std::cerr << "Dropped " << count << " MIDI packets (queue full); sending all-sound-off\n";
            if (midiOutput != nullptr)
                for (int channel = 1; channel <= 16; ++channel)
                    midiOutput->sendMessageNow (MidiMessage::allSoundOff (channel));
        }

        if (interrupted || failed())
        {
            if (failed())
            {
                std::cerr << "Playback error: ";

                switch (failure.load())
                {
                    case Failure::none: break;
                    case Failure::layoutChanged: std::cerr << "audio callback block size or channel layout is incompatible with the patch"; break;
                    case Failure::formatChanged: std::cerr << "audio device sample rate or buffer size changed; restart playback"; break;
                    case Failure::stopped: std::cerr << "audio device stopped"; break;
                    case Failure::deviceError: std::cerr << "audio device reported an error"; break;
                    case Failure::processing: std::cerr << "graph processing failed: " << processingError(); break;
                }

                if (deviceErrorReady.load (std::memory_order_acquire))
                    std::cerr << ": " << deviceErrorText.data();

                std::cerr << "\n";
            }
            MessageManager::getInstance()->stopDispatchLoop();
            return;
        }

        if (reload.valid() && reload.wait_for (std::chrono::seconds (0)) == std::future_status::ready)
        {
            auto state = reload.get();

            if (fileSnapshot() != compilingSnapshot)
                reloadPending = true;
            else if (! state.wasOk())
                std::cerr << state.getErrorMessage() << "\nKeeping the previous patch\n";
            else
            {
                delete pending.exchange (state.getReference().release(), std::memory_order_acq_rel);

                std::cerr << "Reloaded " << options.file.getFileName() << "\n";
            }
        }

        ++ticks;

        if (options.verbose && ticks % 200 == 0)
        {
            std::cerr << "Playback totals: blocks=" << audioBlocks.load (std::memory_order_relaxed)
                      << " midi-received=" << midiReceived.load (std::memory_order_relaxed)
                      << " note-ons=" << noteOnsReceived.load (std::memory_order_relaxed)
                      << " midi-delivered=" << midiDelivered.load (std::memory_order_relaxed)
                      << " peak=" << outputPeak.load (std::memory_order_relaxed)
                      << " non-finite=" << nonFiniteSamples.load (std::memory_order_relaxed) << "\n";
        }

        if (! options.hotreload || ticks % 50 != 0)
            return;

        refreshWatchedFiles();

        auto current = fileSnapshot();
        if (current != snapshot)
        {
            snapshot = std::move (current);
            reloadPending = true;
            return;
        }

        if (! reloadPending || reload.valid())
            return;

        reloadPending = false;
        compilingSnapshot = snapshot;

        reload = std::async (std::launch::async, [this]() -> ResultValue<std::unique_ptr<PlaybackState>>
        {
            auto graph = compile();
            if (! graph.wasOk())
                return makeResultValueFail (graph.getErrorMessage());
            return prepare (std::move (graph.getReference()));
        });
    }

    PlayerOptions options;
    AudioDeviceManager manager;
    std::unique_ptr<MidiInput> midiInput;
    std::unique_ptr<MidiOutput> midiOutput;
    MidiQueue incoming, outgoing;
    static_assert (std::atomic<PlaybackState*>::is_always_lock_free);
    static_assert (std::atomic<unsigned>::is_always_lock_free);
    static_assert (std::atomic<bool>::is_always_lock_free);
    PlaybackState* active = nullptr;
    std::atomic<PlaybackState*> pending { nullptr }, retired { nullptr };
    static_assert (std::atomic<Failure>::is_always_lock_free);
    static_assert (std::atomic<YdspProcessResult>::is_always_lock_free);
    std::atomic<Failure> failure { Failure::none };
    std::atomic<YdspProcessResult> processResult { YdspProcessResult::ok };
    std::atomic<bool> inputOverflow { false }, deviceErrorClaimed { false }, deviceErrorReady { false };
    std::array<char, 512> deviceErrorText {};
    std::atomic<unsigned> dropped { 0 };
    std::atomic<unsigned> audioBlocks { 0 }, midiReceived { 0 }, noteOnsReceived { 0 }, midiDelivered { 0 }, nonFiniteSamples { 0 };
    static_assert (std::atomic<float>::is_always_lock_free);
    std::atomic<float> outputPeak { 0.0f };
    bool testNoteStarted = false;
    int64_t testNoteRemaining = -1;
    double sampleRate = 0;
    int blockSize = 0, inputCount = 0, outputCount = 0, ticks = 0;
    bool reloadPending = false;
    std::vector<File> watched;
    String snapshot, compilingSnapshot;
    std::future<ResultValue<std::unique_ptr<PlaybackState>>> reload;
};
} // namespace

int runYdspPlayerCommand (int argc, char** argv)
{
    PlayerOptions options;

    for (int i = 2; i < argc; ++i)
    {
        const String argument (argv[i]);

        if (argument == "--hotreload")
        {
            options.hotreload = true;
        }
        else if (argument == "--verbose")
        {
            options.verbose = true;
        }
        else if (argument.startsWith ("--") && i + 1 < argc)
        {
            const String value (argv[++i]);

            if (argument == "--main")
                options.main = value;
            else if (argument == "--audio-type")
                options.audioType = value;
            else if (argument == "--audio-input")
                options.input = value;
            else if (argument == "--audio-output")
                options.output = value;
            else if (argument == "--midi-input")
                options.midiInput = value;
            else if (argument == "--midi-output")
                options.midiOutput = value;
            else if (argument == "--sample-rate" && value.containsOnly ("0123456789") && value.getIntValue() > 0)
                options.sampleRate = value.getIntValue();
            else if (argument == "--test-note" && value.isNotEmpty() && value.containsOnly ("0123456789") && value.getIntValue() >= 0 && value.getIntValue() <= 127)
                options.testNote = value.getIntValue();
            else if (argument == "--block-size" && value.containsOnly ("0123456789") && value.getIntValue() > 0)
                options.blockSize = value.getIntValue();
            else
            {
                std::cerr << "Invalid run option: " << argument << "\n";
                return 2;
            }
        }
        else if (! argument.startsWithChar ('-') && options.file == File())
        {
            options.file = File::getCurrentWorkingDirectory().getChildFile (argument);
        }
        else
        {
            std::cerr << "Invalid run argument: " << argument << "\n";
            return 2;
        }
    }

    if (! options.file.existsAsFile() || ! options.file.hasFileExtension ("ydsp;ydsp-project")
        || ((options.hotreload || options.main.isNotEmpty()) && ! options.file.hasFileExtension ("ydsp-project")))
    {
        std::cerr << "run requires a .ydsp or .ydsp-project; --main and --hotreload require a project\n";
        return 2;
    }

    ScopedYupInitialiser_GUI init;

    interrupted = 0;

    const auto previous = std::signal (SIGINT, interruptPlayback);
    const auto previousTerm = std::signal (SIGTERM, interruptPlayback);

    Player player (std::move (options));

    const auto result = player.start();
    if (result.wasOk())
        MessageManager::getInstance()->runDispatchLoop();
    else
        std::cerr << "Playback error: " << result.getErrorMessage() << "\n";

    std::signal (SIGINT, previous);
    std::signal (SIGTERM, previousTerm);

    return result.wasOk() && ! player.failed() ? 0 : 1;
}
