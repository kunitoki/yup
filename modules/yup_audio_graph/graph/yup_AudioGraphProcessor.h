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

//==============================================================================
/**
    An AudioProcessor that owns and executes an acyclic graph of AudioProcessor nodes.

    Topology edits are made to a control-thread graph model. commitChanges()
    validates the model, prepares newly compiled nodes for the current playback
    configuration, and publishes an immutable processing plan. Child processor
    latency notifications trigger a rebuild so plugin latency changes can update
    delay compensation. Metadata edits such as node positions and properties
    are saved by the model without invalidating the compiled plan. processBlock()
    only swaps pending plans at block boundaries and keeps retired plans alive until
    a later control-thread commit or destruction.
*/
class YUP_API AudioGraphProcessor final : public AudioProcessor
{
public:
    /** Creates the default stereo graph bus layout. */
    static AudioBusLayout createDefaultBusLayout();

    //==============================================================================

    /** Constructs an audio graph that compiles and processes an external graph model. */
    explicit AudioGraphProcessor (std::shared_ptr<AudioGraphModel> model,
                                  AudioBusLayout busLayout = createDefaultBusLayout());

    /** Destructs the graph and releases all owned processors. */
    ~AudioGraphProcessor() override;

    //==============================================================================

    /** Returns the editable graph model consumed by this processor. */
    std::shared_ptr<AudioGraphModel> getModel() const noexcept;

    //==============================================================================

    /** Validates, compiles, and publishes the current graph topology when needed. */
    Result commitChanges();

    /** Returns true when topology edits have not yet been committed. */
    bool hasUncommittedChanges() const noexcept;

    //==============================================================================
    /** Sets the desired worker thread count for future processing blocks. */
    void setNumWorkerThreads (int numThreads);

    /** Returns the desired worker thread count. */
    int getNumWorkerThreads() const noexcept;

    /** Supplies the audio workgroup that worker threads should join when supported. */
    void setAudioWorkgroup (AudioWorkgroup workgroup);

    /** Returns diagnostics for the last successfully compiled graph. */
    AudioGraphAllocationStats getAllocationStats() const noexcept;

    //==============================================================================
    /** Creates an XML representation of the current graph, including node state. */
    std::unique_ptr<XmlElement> createXml() const;

    /** Restores the graph from XML previously created by createXml(). */
    Result restoreFromXml (const XmlElement& xml);

    //==============================================================================
    // AudioProcessor
    void prepareToPlay (float sampleRate, int maxBlockSize) override;
    void releaseResources() override;
    void processBlock (AudioBuffer<float>& audioBuffer, MidiBuffer& midiBuffer) override;
    void flush() override;

    int getLatencySamples() override;

    int getCurrentPreset() const noexcept override { return 0; }

    void setCurrentPreset (int) noexcept override {}

    int getNumPresets() const override { return 0; }

    String getPresetName (int) const override { return {}; }

    void setPresetName (int, StringRef) override {}

    Result loadStateFromMemory (const MemoryBlock& memoryBlock) override;

    Result saveStateIntoMemory (MemoryBlock& memoryBlock) override;

    bool hasEditor() const override { return false; }

    AudioProcessorEditor* createEditor() override { return nullptr; }

private:
    class Pimpl;
    std::unique_ptr<Pimpl> pimpl;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioGraphProcessor)
};

} // namespace yup
