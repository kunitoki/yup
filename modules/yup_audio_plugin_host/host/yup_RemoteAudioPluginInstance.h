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


#pragma once

namespace yup
{

class AudioPluginParameterMailbox;

//==============================================================================
/**
    A plugin instance running in a separate bridge server process.

    Loading, controlling and processing work like any other AudioPluginInstance, but the
    plugin itself runs in an AudioPluginBridgeServer process: a crash or a hang only takes
    the server down. Audio is exchanged through shared memory with a synchronous hand-off,
    so the bridge adds no latency, only the cost of switching between processes.

    When the server dies, or misses its deadline for a block, the instance outputs silence.
    A crashed instance reports hasCrashed(), calls onCrashed on the message thread and can be
    restarted with reload(), which restores the last state saved or loaded through it.

    Limitations: only single precision processing, parameter metadata changes after loading
    are not reflected, MIDI controller mappings are not exposed, and the editor opens in a
    floating window of the server process (use showEditor(); createEditor() returns a placeholder).

    @see AudioPluginBridgeServer
*/
class RemoteAudioPluginInstance final
    : public AudioPluginInstance
    , private AsyncUpdater
    , private Timer
{
public:
    //==============================================================================

    /** Where and how to launch the bridge server process. */
    struct Options
    {
        /** The executable to launch, which must create an AudioPluginBridgeServer at startup.
            On macOS this is the binary inside the application bundle.
        */
        File executable;

        /** Must match the identifier passed to AudioPluginBridgeServer::initialiseFromCommandLine(). */
        String commandLineUniqueID = AudioPluginBridgeServer::defaultCommandLineUniqueID;

        /** Time allowed for loading, state and other control requests, and for offline processing. */
        int timeoutMs = 20000;
    };

    //==============================================================================

    /**
        Launches a bridge server process and loads a plugin in it.

        @param description  A description previously produced by a scan.
        @param context      Host context used to load the plugin.
        @param options      The bridge server executable and timeouts.
        @return             The remote instance, or a failure when the server can't be launched
                            or the plugin fails to load.
    */
    static ResultValue<std::unique_ptr<RemoteAudioPluginInstance>> load (const AudioPluginDescription& description,
                                                                         const AudioPluginHostContext& context,
                                                                         const Options& options);

    ~RemoteAudioPluginInstance() override;

    //==============================================================================

    /** Returns true when the bridge server process crashed, hung or disconnected. */
    bool hasCrashed() const noexcept;

    /** Called on the message thread when the bridge server process is lost. */
    std::function<void()> onCrashed;

    /**
        Restarts a crashed plugin in a new bridge server process.

        Restores the last state saved or loaded through this instance, sends the current
        parameter values and prepares the plugin again when the instance is prepared.
        Fails when the plugin can't be loaded again or reports different parameters.
        Call from the message thread.
    */
    Result reload();

    //==============================================================================

    /**
        Opens the plugin editor in a window of the bridge server process, or brings it to front.

        Hosts can call this instead of embedding createEditor() in their own window, which only
        returns a placeholder for the remote editor. Does nothing when the plugin has no editor
        or the server process is lost.
    */
    void showEditor();

    /** Closes the plugin editor window opened in the bridge server process. */
    void hideEditor();

    //==============================================================================

    /** @internal */
    void prepareToPlay (const AudioSpec& spec) override;
    /** @internal */
    void releaseResources() override;
    /** @internal */
    void processBlock (AudioProcessContext<float>& context) override;

    /** @internal */
    bool acceptsMidi() const noexcept override;
    /** @internal */
    bool producesMidi() const noexcept override;
    /** @internal */
    bool supportsNoteExpression() const override;
    /** @internal */
    bool supportsMpe() const override;
    /** @internal */
    int getTailSamples() override;

    /** @internal */
    bool hasEditor() const override;
    /** @internal */
    AudioProcessorEditor* createEditor() override;

    /** @internal */
    int getCurrentPreset() const noexcept override;
    /** @internal */
    void setCurrentPreset (int index) noexcept override;
    /** @internal */
    int getNumPresets() const override;
    /** @internal */
    String getPresetName (int index) const override;
    /** @internal */
    void setPresetName (int index, StringRef newName) override;

    /** @internal Returns the last known state when the server process is lost. */
    Result saveStateIntoMemory (MemoryBlock& memoryBlock) override;
    /** @internal */
    Result loadStateFromMemory (const MemoryBlock& memoryBlock) override;

    /** @internal */
    void hostInfoChanged (const HostInfo& newInfo) override;

private:
    class Connection;
    class AudioChannel;
    class RemoteEditor;
    struct PluginInfo;

    RemoteAudioPluginInstance (const AudioPluginDescription& description,
                               const AudioPluginHostContext& context,
                               const Options& options,
                               std::unique_ptr<Connection> connection,
                               const PluginInfo& info);

    void attach (Connection& connectionToAttach);
    void handleNotification (const MemoryBlock& message);
    void handleAsyncUpdate() override;
    void timerCallback() override;
    void nonRealtimeStateChanged() override;
    void bypassStateChanged() override;

    void writeParameterChanges (const ParameterChangeBuffer& changes);
    void sendEditorRequest (bool shouldOpen);
    String convertValueToText (int index, float value);
    float convertTextToValue (int index, const String& text);

    const AudioPluginHostContext hostContext;
    const Options options;

    std::unique_ptr<Connection> connection;
    std::unique_ptr<AudioChannel> audioChannel;
    std::optional<AudioSpec> preparedSpec;
    std::atomic<bool> crashed { false };
    std::atomic<bool> crashReported { false };
    uint32 lateSinceMs = 0;

    bool editorAvailable = false, midiInput = false, midiOutput = false, noteExpressions = false, mpe = false;
    std::atomic<int> tailSamples { 0 };
    std::atomic<int> currentPreset { 0 };
    mutable CriticalSection presetLock;
    StringArray presetNames;

    std::atomic<int> pendingLatency { -1 };
    std::atomic<int32> pendingChanges { 0 };

    int bypassParameterIndex = -1;
    std::unique_ptr<std::atomic<float>[]> lastSentValues;
    std::unique_ptr<AudioPluginParameterMailbox> fromServer;

    CriticalSection stateLock;
    std::optional<MemoryBlock> lastState;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RemoteAudioPluginInstance)
};

} // namespace yup
