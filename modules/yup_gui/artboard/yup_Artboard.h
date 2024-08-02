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
class JUCE_API Artboard : public Component
{
public:
    //==============================================================================
    Artboard (StringRef componentID);

    //==============================================================================
    Result loadFromFile (const juce::File& file, int defaultArtboardIndex = -1, bool shouldUseStateMachines = true);

    //==============================================================================
    bool isPaused() const;
    void setPaused (bool shouldPause);

    //==============================================================================
    void setBoolInput (const String& name, bool value);
    void setNumberInput (const String& name, double value);
    void triggerInput (const String& name);

    //==============================================================================
    void addHorizontalRepeats (int repeatsToAdd);
    void addVerticalRepeats (int repeatsToAdd);

    int getNumInstances() const;

    //==============================================================================
    void multiplyScale (float factor);

    //==============================================================================
    virtual void propertyChanged (const String& eventName, const String& propertyName, const var& oldValue, const var& newValue);

    //==============================================================================
    void paint (Graphics& g) override;
    void mouseEnter (const MouseEvent& event) override;
    void mouseExit (const MouseEvent& event) override;
    void mouseDown (const MouseEvent& event) override;
    void mouseUp (const MouseEvent& event) override;
    void mouseMove (const MouseEvent& event) override;
    void mouseDrag (const MouseEvent& event) override;

private:
    void updateScenesFromFile (std::size_t count);
    void pullEventsFromStateMachines();

    std::unique_ptr<rive::File> rivFile;
    std::vector<std::unique_ptr<rive::Artboard>> artboards;
    std::vector<std::unique_ptr<rive::Scene>> scenes;
    std::vector<rive::StateMachineInstance*> stateMachines;
    HashMap<String, var> eventProperties;

    rive::Mat2D viewTransform;
    float scale = 1.0f;

    int artboardIndex = -1;
    int animationIndex = -1;
    int stateMachineIndex = -1;

    int horzRepeat = 0;
    int vertRepeat = 0;

    bool useStateMachines = true;
    bool paused = false;
};

} // namespace yup
