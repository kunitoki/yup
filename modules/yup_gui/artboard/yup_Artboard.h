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
*/
class YUP_API Artboard : public Component
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

    //==============================================================================
    /** Sets the Rive artboard file to display.

        @param artboardFile The Rive artboard file to display.
    */
    void setFile (std::shared_ptr<ArtboardFile> artboardFile);

    //==============================================================================
    /** Clears the Rive artboard. */
    void clear();

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

    /** Returns true if the Rive artboard has a trigger input with the given name. */
    bool hasTriggerInput (const String& name) const;

    /** Triggers a trigger input with the given name.

        @param name The name of the input.
    */
    void triggerInput (const String& name);

    //==============================================================================
    /** Returns all the inputs of the Rive artboard. */
    var getAllInputs() const;

    /** Sets all the inputs of the Rive artboard.

        @param value The value to set the inputs to.
    */
    void setAllInputs (const var& value);

    /** Sets the value of an input with the given name.

        @param name The name of the input.
        @param value The value to set the input to.
    */
    void setInput (const String& state, const var& value);

    //==============================================================================
    /** A callback that is called when a property of the Rive artboard changes. */
    std::function<void (Artboard&, const String&, const String&, const var&, const var&)> onPropertyChanged;

    /** A callback that is called when a property of the Rive artboard changes.

        @param eventName The name of the event.
        @param propertyName The name of the property.
        @param oldValue The old value of the property.
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
    void updateSceneFromFile();
    void pullEventsFromStateMachines();
    Point<float> transformPoint (Point<float> point) const;

    std::shared_ptr<ArtboardFile> artboardFile;

    std::unique_ptr<rive::Artboard> artboard;
    std::unique_ptr<rive::Scene> scene;
    rive::StateMachineInstance* stateMachine = nullptr;

    HashMap<String, var> eventProperties;

    rive::Mat2D viewTransform;

    bool paused = false;
    bool pauseWhenHidden = true;
};

} // namespace yup
