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
    latency notifications are handled by the graph as host notifications and
    rebuild delay compensation. Metadata edits such as node positions and
    properties are saved by the model without invalidating the compiled plan.
    processBlock() only swaps pending plans at block boundaries and keeps retired
    plans alive until a later control-thread commit or destruction.

    @tags{Audio, Graph, Realtime}
*/
class YUP_API AudioGraphProcessor final : public AudioProcessor
{
public:
    /** Creates the default stereo graph bus layout. */
    static AudioBusLayout createDefaultBusLayout();

    //==============================================================================

    /** Constructs an audio graph that compiles and processes an external graph model.
    
        The model should be shared with the UI and other control-thread components
        that edit the graph topology and node properties. The processor will call
        commitChanges() to validate and publish changes made to the model, but it
        does not automatically commit on every edit - this allows batching of edits
        for more efficient updates.

        @param model         The shared graph model to use for topology and state.
        @param busLayout     The initial bus layout for the graph. This can be changed
                            later by editing the model and committing changes.

        @see AudioGraphModel, commitChanges
    */
    explicit AudioGraphProcessor (std::shared_ptr<AudioGraphModel> model,
                                  AudioBusLayout busLayout = createDefaultBusLayout());

    /** Destructs the graph and releases all owned processors. */
    ~AudioGraphProcessor() override;

    //==============================================================================

    /** Returns the editable graph model consumed by this processor.
    
        This model is shared with the UI and other control-thread components that edit
        the graph topology and node properties. Changes made to the model will not
        take effect until commitChanges() is called to validate and publish them.

        @return The shared graph model used by this processor.
    */
    std::shared_ptr<AudioGraphModel> getModel() const noexcept;

    //==============================================================================

    /** Validates, compiles, and publishes the current graph topology when needed.
    
        This should be called after making edits to the graph model to validate the
        topology, prepare any new nodes for processing, and publish an immutable
        processing plan that will be used by processBlock(). This allows batching
        of multiple edits before committing, which can improve efficiency when making
        several changes at once.

        During commitChanges(), the graph will check for cycles, verify that all
        nodes can be prepared with the current playback configuration, and ensure
        that all connections are valid. If any issues are found, the commit will fail
        and the graph will remain in its previous state.

        For realtime safety, this method should be called on the main thread or during
        non-realtime setup. Changes will take effect on the next call to processBlock()
        after this method is called.

        @return Result indicating success or failure of the commit operation.
    */
    Result commitChanges();

    /** Validates one connection against the current model and graph bus layout.
    
        This can be used by the UI to provide immediate feedback on the validity of
        a proposed connection before committing it to the model. The connection
        should be validated again during commitChanges() to ensure thread safety.

        @param connection    The proposed connection to validate.

        @see commitChanges
    */
    Result validateConnection (const AudioGraphConnection& connection) const;

    /** Returns true when topology edits have not yet been committed.
    
        This can be used by the UI to indicate that there are pending changes that
        have not yet been validated and published to the audio thread. Once
        commitChanges() is called, this will return false until new edits are made.

        @see commitChanges
    */
    bool hasUncommittedChanges() const noexcept;

    //==============================================================================
    /** Sets the desired worker thread count for future processing blocks.
    
        This is a hint that allows the graph to utilize multiple threads for processing
        when supported by the current graph topology and host configuration. The
        actual number of threads used may be less than the requested count based on
        internal heuristics.

         For realtime safety, this method should be called on the main thread or
         during non-realtime setup. Changes will take effect on the next call to
         processBlock() after any pending topology changes have been committed.

        @param numThreads   The desired number of worker threads to use (0 for no workers).
    */
    void setNumWorkerThreads (int numThreads);

    /** Returns the desired worker thread count.
    
        This returns the last value set by setNumWorkerThreads(), which is a hint for
        the graph's internal thread usage. The actual number of threads used for
        processing may be less than this value based on internal heuristics and the
        current graph topology.

        @return The desired number of worker threads to use (0 for no workers).
    */
    int getNumWorkerThreads() const noexcept;

    /** Supplies the audio workgroup that worker threads should join when supported.
    
        This is a hint that allows the graph to utilize host-provided workgroups for
        processing when supported by the current graph topology and host configuration.
        For realtime safety, this method should be called on the main thread or during
        non-realtime setup. Changes will take effect on the next call to processBlock()
        after any pending topology changes have been committed.

        @param workgroup    The audio workgroup to join, or an empty object to not use workgroups.
    */
    void setAudioWorkgroup (AudioWorkgroup workgroup);

    /** Returns diagnostics for the last successfully compiled graph.
    
        This can be used to retrieve information about the compiled graph, such as
        node counts, latency, and resource usage. The diagnostics are updated on
        each successful call to commitChanges().

        @return An object containing allocation and performance statistics for the graph.
    */
    AudioGraphAllocationStats getAllocationStats() const noexcept;

    //==============================================================================
    /** Creates an XML representation of the current graph, including node state.

        This is not realtime-safe and should be called on the main thread or during
        non-realtime setup. The XML can be saved and later passed to restoreFromXml()
        to restore the graph's state.

        @return An XML element representing the current graph state.
    */
    std::unique_ptr<XmlElement> createXml() const;

    /** Restores the graph from XML previously created by createXml().
    
        This is not realtime-safe and should be called on the main thread or during
        non-realtime setup. The graph will be recompiled and all pending plans will
        be replaced, so this should only be called when audio processing is stopped
        or when the graph is not actively processing.

        @param xml   The XML element to restore from.

        @return Result indicating success or failure of the restore operation.
    */
    Result restoreFromXml (const XmlElement& xml);

    //==============================================================================
    // AudioProcessor
    void prepareToPlay (const AudioSpec& spec) override;
    void releaseResources() override;
    void processBlock (AudioProcessContext<float>& context) override;
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
