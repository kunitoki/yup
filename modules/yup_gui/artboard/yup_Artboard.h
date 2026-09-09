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

namespace yup
{

//==============================================================================
/** Represents a Rive artboard.

    This class is used to display a Rive artboard.

    Artboards are not internally synchronized. Beyond the message thread, the
    artboard also advances from refreshDisplay(), which YUP runs on the render
    thread while holding the message manager lock, so the two never run
    concurrently. Everything the advance reaches runs on that thread too: the
    NodeBoundsCallbacks registered with setNodeBoundsListener(), the bounds and
    transform updates pushed into components attached with
    attachComponentToNode(), onPropertyChanged / propertyChanged, and the
    ArtboardViewModelInstance::PropertyChangedCallback of a bound instance.
    A slow callback therefore stalls rendering.
*/
class YUP_API Artboard
    : public Component
    , private ComponentListener
{
public:
    //==============================================================================
    /** Creates a new Rive artboard.

        @param componentID The ID of the component.
    */
    Artboard (StringRef componentID = {});

    /** Creates a new Rive artboard.

        @param componentID The ID of the component.
        @param artboardFile The Rive artboard file to display.
    */
    Artboard (StringRef componentID, std::shared_ptr<ArtboardFile> artboardFile);

    /** Destructor. Detaches every attached component and invalidates all outstanding
        ArtboardNode handles. */
    ~Artboard() override;

    //==============================================================================
    /** Sets the Rive artboard file to display.

        Passing a null file unloads the current one, leaving the artboard empty.

        @param artboardFile The Rive artboard file to display.
        @param artboardName The name of the artboard to load from the file. When
                            empty (the default), the file's default artboard is
                            loaded, exactly as before this parameter existed;
                            otherwise the named artboard is loaded, or nothing if
                            the name is unknown.
    */
    void setFile (std::shared_ptr<ArtboardFile> artboardFile, StringRef artboardName = {});

    //==============================================================================
    /** Clears the Rive artboard. */
    void clear();

    //==============================================================================
    /** Sets how the artboard is fitted into the component bounds.

        This affects the view transform used for rendering and input handling.

        Pass std::nullopt to apply no fitting of our own and instead hand the
        component's size to the artboard, letting its authored Rive layout
        constraints resolve it. That is what makes nodes reflow on resize, and is
        the mode responsive artboards are designed for.

        Fitting values Rive has no equivalent for (Fitting::tile,
        Fitting::centerCrop, Fitting::stretchWidth and Fitting::stretchHeight)
        fall back to Fitting::scaleToFit.

        @param newFitting The new fitting mode, or nullopt for the artboard's own layout.
    */
    void setFitting (std::optional<Fitting> newFitting);

    /** Returns the current fitting mode, or nullopt when the artboard lays itself out. */
    std::optional<Fitting> getFitting() const;

    /** Sets the justification used when positioning the artboard in the bounds.

        This only has a visible effect when the fitting leaves spare room along an
        axis, so not for Fitting::fill nor for the artboard's own layout. An axis
        with no flag set is centered on that axis.

        @param newJustification The new justification value.
    */
    void setJustification (Justification newJustification);

    /** Returns the current justification. */
    Justification getJustification() const;

    //==============================================================================
    /** Returns true if the Rive artboard is paused. */
    bool isPaused() const;

    /** Sets the Rive artboard to paused or running.

        @param shouldPause True to pause the Rive artboard, false to run it.
    */
    void setPaused (bool shouldPause);

    /** Returns true if the Rive artboard is pausing when hidden. */
    bool isPausingWhenHidden() const;

    /** Sets the Rive artboard to pause when hidden.

        @param shouldPause True to pause the Rive artboard when hidden, false to not pause.
    */
    void shouldPauseWhenHidden (bool shouldPause);

    //==============================================================================
    /** Advances the Rive artboard by a given number of seconds.

        @param elapsedSeconds The number of seconds to advance the Rive artboard by.
    */
    void advanceAndApply (float elapsedSeconds);

    /** Returns the duration of the Rive artboard in seconds. */
    float durationSeconds() const;

    //==============================================================================
    /** Returns true if the Rive artboard has a boolean input with the given name. */
    bool hasBoolInput (const String& name) const;

    /** Sets the value of a boolean input with the given name.

        @param name The name of the input.
        @param value The value to set the input to.
    */
    void setBoolInput (const String& name, bool value);

    /** Returns true if the Rive artboard has a number input with the given name. */
    bool hasNumberInput (const String& name) const;

    /** Sets the value of a number input with the given name.

        @param name The name of the input.
        @param value The value to set the input to.
    */
    void setNumberInput (const String& name, double value);

    /** Returns true if the Rive artboard has a trigger input with the given name.
    
        @param name The name of the input.

        @return True if the Rive artboard has a trigger input with the given name, false otherwise.
    */
    bool hasTriggerInput (const String& name) const;

    /** Triggers a trigger input with the given name.

        @param name The name of the input.
    */
    void triggerInput (const String& name);

    /** Returns all the state machine inputs of the Rive artboard.

        Each entry is a DynamicObject with:
        - "id": the input name, as a String.
        - "type": one of "number", "boolean" or "trigger".
        - "value": the current value, as a double for "number" and a bool for
          "boolean". Triggers are stateless and carry no "value".

        @return An Array<var> of input descriptions, or an empty var when the
                artboard is not driven by a state machine.
    */
    var getAllInputs() const;

    /** Applies a set of state machine inputs previously read with getAllInputs().

        Accepts the array getAllInputs() returns and writes each entry back
        through setInput(), matching entries by their "id". Entries whose "id"
        is unknown to the current state machine are ignored, so a snapshot taken
        from one artboard can be applied to another without failing. Triggers are
        not fired: an input snapshot describes state, and a trigger has none.

        @param value The Array<var> of input descriptions to apply.
    */
    void setAllInputs (const var& value);

    /** Sets the value of an input with the given name.

        Triggers ignore the value and simply fire.

        @param name The name of the input.
        @param value The value to set the input to.
    */
    void setInput (const String& name, const var& value);

    //==============================================================================
    /** Callback invoked when a named node's bounds or on-screen orientation change.

        The handle is the artboard's cached handle for the node (the same object
        returned by Artboard::findNode), so no per-event allocation happens; it
        stays valid until the artboard is cleared or its file replaced.

        @param artboard The artboard that emitted the change.
        @param nodeName The name of the node that changed.
        @param node     A handle to the node; getBounds() and getViewTransform()
                        report the current values in component coordinates.
    */
    using NodeBoundsCallback = std::function<void (Artboard&, const String&, const ArtboardNode::Ptr&)>;

    /** Registers a listener for a named artboard node.

        The callback is invoked whenever the node's bounds or its on-screen
        orientation (rotation, scale, skew, mirroring) change, e.g. on layout
        reflows, artboard resizes or animation frames. Pass an empty callback to
        remove the listener.

        @param nodeName The name of the node in the .riv file.
        @param callback The callback to invoke on node changes.
    */
    void setNodeBoundsListener (StringRef nodeName, NodeBoundsCallback callback);

    /** Removes the listener registered for the given node. */
    void clearNodeBoundsListener (StringRef nodeName);

    /** Removes all node bounds listeners. */
    void clearAllNodeBoundsListeners();

    /** Returns the current bounds of a named node in component coordinates.

        @param nodeName The name of the node in the .riv file.
        @return The node bounds, or an empty rectangle if the node is unknown.
    */
    Rectangle<float> getNodeBounds (StringRef nodeName) const;

    //==============================================================================
    /** Options controlling how an attached component is driven by its node. */
    struct NodeAttachmentOptions
    {
        /** How the artboard positions the attached component. */
        enum class Mode
        {
            /** Set the component's bounds to the node's bounds in component coordinates
                (position and size). Layout nodes report their laid-out size; shapes and
                containers of shapes report their geometry; other nodes report a unit-size
                rect. This is the default. */
            fillNode,

            /** Keep the component's current size and only move it to follow the node.
                The node's scale and rotation are not applied to the size. */
            trackPosition
        };

        /** How the artboard positions the attached component. */
        Mode mode;

        /** When true, the node's orientation (rotation) is applied to the attached
            component through Component::setTransform, so its content rotates around
            the pivot point (see pivot) to follow the node. When false
            (the default) the component stays axis-aligned.
        */
        bool applyTransform;

        /** In trackPosition mode, which point of the component acts as its pivot:
            the component is positioned so this point lands on the node's anchor
            point (see anchor). When applyTransform is true the
            component rotates around this pivot, so the anchored point does not
            drift while following the node's rotation. All Justification values
            are supported. Ignored in fillNode mode. */
        Justification pivot;

        /** In trackPosition mode, which point of the node's bounds the component's
            pivot (see pivot) is anchored to. For example center keeps
            the component centered on the node, bottomCenter pins the component's
            pivot to the bottom edge center of the node. All Justification values
            are supported. Ignored in fillNode mode. */
        Justification anchor;

        /** Default constructor initializing the node attachment options with default values. */
        NodeAttachmentOptions()
            : mode (Mode::fillNode)
            , applyTransform (false)
            , pivot (Justification::topLeft)
            , anchor (Justification::topLeft)
        {
        }

        /** Sets the mode of the node attachment.

            @param newMode The new mode to set.
        
            @return A reference to this NodeAttachmentOptions instance.
        */
        NodeAttachmentOptions& withMode (Mode newMode)
        {
            mode = newMode;
            return *this;
        }

        /** Sets whether the node's orientation should be applied to the attached component.

            @param shouldApplyTransform True to apply the node's rotation to the component.
    
            @return A reference to this NodeAttachmentOptions instance.
        */
        NodeAttachmentOptions& withApplyTransform (bool shouldApplyTransform)
        {
            applyTransform = shouldApplyTransform;
            return *this;
        }

        /** Sets the justification point of the component that acts as its pivot.

            @param newJustificationPivot The new justification point for the component's pivot.

            @return A reference to this NodeAttachmentOptions instance.
        */
        NodeAttachmentOptions& withJustificationPivot (Justification newJustificationPivot)
        {
            pivot = newJustificationPivot;
            return *this;
        }

        /** Sets the justification point of the node's bounds that the component's pivot is anchored to.

            @param newJustificationAnchor The new justification point for the node's anchor.

            @return A reference to this NodeAttachmentOptions instance.
        */
        NodeAttachmentOptions& withJustificationAnchor (Justification newJustificationAnchor)
        {
            anchor = newJustificationAnchor;
            return *this;
        }
    };

    /** Attaches a component to a named artboard node.

        The component's bounds are driven by the node's layout: whenever a reflow
        changes the node's bounds, the artboard updates the component according to
        the given options. Ownership of the component remains with the caller, and
        the component is immediately positioned at the node's current bounds upon
        attachment. Attaching again to the same node replaces the previous attachment,
        and attaching a component that is already attached to a different node moves
        it, so a component is never driven by two nodes at once.

        The artboard listens for the component's destruction and detaches it
        automatically, so deleting an attached component without calling
        detachComponentFromNode() is safe.

        In trackPosition mode the component keeps its own size, and that size
        decides where its pivot sits; resizing it therefore re-derives its
        position immediately, so the size can be set before or after attaching
        with the same result. In fillNode mode the node owns the size, so a
        resize is simply overwritten.

        @param nodeName   The name of the node in the .riv file.
        @param component  The component to attach.
        @param options    How the node should drive the component's bounds.
        @return True if the node exists and the component was attached.
    */
    bool attachComponentToNode (StringRef nodeName,
                                Component* component,
                                NodeAttachmentOptions options = NodeAttachmentOptions());

    /** Detaches a component from a named node, stopping bounds updates.

        @param nodeName   The name of the node in the .riv file.
        @param component  The component to detach.
        @return True if the component was attached to the given node.
    */
    bool detachComponentFromNode (StringRef nodeName, Component* component);

    /** Detaches every attached component. */
    void detachAllComponents();

    //==============================================================================
    /** Finds a node in the currently loaded Rive file by name.

        The returned handle is read-only, refcounted and remains valid until the
        artboard is cleared, its file is replaced (Artboard::clear / Artboard::setFile),
        or the artboard is destroyed.

        @param nodeName The name of the node in the .riv file.
        @return A handle to the node, or a null pointer if the node is unknown.
    */
    ArtboardNode::Ptr findNode (StringRef nodeName) const;

    //==============================================================================
    /** Returns the name of the ViewModel schema this artboard is designed against.

        Artboards may reference one ViewModel schema authored in the .riv file.
        The returned name can be used to create an instance through
        ArtboardFile::createArtboardViewModelInstance() before binding it with
        bindViewModelInstance(). Returns an empty string if the artboard does
        not use a ViewModel.
    */
    String getViewModelName() const;

    /** Binds a ViewModel instance to this artboard, enabling its data bindings.

        Once bound, the values of the instance drive the artboard's data-bound
        properties and state machine transitions; writes through the instance
        are applied on the next advanceAndApply(). Only one instance can be
        bound at a time; binding again replaces the previous binding.

        @param instance The instance to bind; it must have been created from
                        the same ArtboardFile this artboard was loaded from.
        @return True if the instance was bound successfully.
    */
    bool bindViewModelInstance (const ArtboardViewModelInstance::Ptr& instance);

    /** Unbinds the currently bound ViewModel instance, if any.

        Data bindings stop reacting to the instance until a new one is bound.
    */
    void unbindViewModelInstance();

    /** Returns the currently bound ViewModel instance, or null if none is bound. */
    ArtboardViewModelInstance::Ptr getBoundViewModelInstance() const noexcept;

    //==============================================================================
    /** A callback that is called when a custom property of a reported state machine
        event changes.

        Events are drained after every advance and after every pointer interaction,
        so the callback fires from advanceAndApply(), refreshDisplay() and the mouse
        handlers, on the thread described above. Only actual changes are reported: the
        artboard remembers the last value seen for each event and skips repeats.

        @param artboard     The artboard that reported the event.
        @param eventName    The name of the event.
        @param propertyName The name of the custom property carried by the event.
        @param oldValue     The previous value of the property, or an empty var the
                            first time the event is seen.
        @param newValue     The new value of the property.
    */
    std::function<void (Artboard& artboard,
                        const String& eventName,
                        const String& propertyName,
                        const var& oldValue,
                        const var& newValue)>
        onPropertyChanged;

    /** Called when a custom property of a reported state machine event changes.

        Invoked immediately before onPropertyChanged, under the same conditions.

        @param eventName The name of the event.
        @param propertyName The name of the custom property carried by the event.
        @param oldValue The previous value of the property.
        @param newValue The new value of the property.
    */
    virtual void propertyChanged (const String& eventName,
                                  const String& propertyName,
                                  const var& oldValue,
                                  const var& newValue);

    //==============================================================================
    /** @internal */
    void refreshDisplay (double lastFrameTimeSeconds) override;
    /** @internal */
    void paint (Graphics& g) override;
    /** @internal */
    void resized() override;
    /** @internal */
    void contentScaleChanged (float dpiScale) override;
    /** @internal */
    void mouseEnter (const MouseEvent& event) override;
    /** @internal */
    void mouseExit (const MouseEvent& event) override;
    /** @internal */
    void mouseDown (const MouseEvent& event) override;
    /** @internal */
    void mouseUp (const MouseEvent& event) override;
    /** @internal */
    void mouseMove (const MouseEvent& event) override;
    /** @internal */
    void mouseDrag (const MouseEvent& event) override;

private:
    friend class ArtboardNode;

    struct NodeAttachment
    {
        Component* component = nullptr;
        NodeAttachmentOptions options;
    };

    /** @internal */
    void componentBeingDeleted (Component& component) override;
    /** @internal */
    void componentResized (Component& component) override;

    void updateSceneFromFile();
    void advanceScene (float elapsedSeconds);
    void pullEventsFromStateMachines();
    void updateViewTransform();
    void updateNodeBounds();
    void notifyNodeBoundsChanged();
    void checkNodeBounds (const String& nodeName, rive::Component* node);
    // Taken by value: it is applied by moving a component, whose resized() may
    // detach it and destroy the attachment record this was read from.
    void applyNodeAttachment (NodeAttachment attachment, rive::Component* node, const Rectangle<float>& nodeBounds);
    void detachComponentEverywhere (Component* component);
    void reapplyNodeAttachment (Component* component);
    ArtboardNode::Ptr nodeHandleFor (const String& nodeName, rive::Component* node) const;
    float computeNodeRotation (rive::Component* node) const;
    Rectangle<float> computeNodeBounds (rive::Component* node) const;
    AffineTransform computeNodeViewTransform (rive::Component* node) const;
    Point<float> transformPoint (Point<float> point) const;

    std::shared_ptr<ArtboardFile> artboardFile;

    std::unique_ptr<rive::Artboard> artboard;
    std::unique_ptr<rive::Scene> scene;
    rive::StateMachineInstance* stateMachine = nullptr;

    HashMap<String, var> eventProperties;

    HashMap<String, NodeBoundsCallback> nodeBoundsListeners;
    HashMap<String, NodeAttachment> attachedComponents;
    HashMap<String, Rectangle<float>> lastNodeBounds;
    HashMap<String, AffineTransform> lastNodeViewTransforms;
    Array<String> nodeNames;
    mutable HashMap<String, ArtboardNode::Ptr> cachedNodeHandles;
    uint64_t nodeEpoch = 0;

    ArtboardViewModelInstance::Ptr boundViewModelInstance;

    rive::Mat2D viewTransform;
    String selectedArtboardName;
    std::optional<Fitting> fitting = Fitting::scaleToFit;
    Justification justification = Justification::center;
    bool paused = false;
    bool pauseWhenHidden = true;
    bool notifyingNodeBounds = false;
    bool drainingEvents = false;
    bool applyingNodeAttachment = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Artboard)
};

} // namespace yup
