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

class JUCE_API Component
{
public:
    //==============================================================================
    Component();
    Component (StringRef componentID);
    virtual ~Component();

    //==============================================================================
    String getComponentID() const;

    //==============================================================================
    bool isEnabled() const;
    virtual void setEnabled (bool shouldBeEnabled);
    virtual void enablementChanged();

    //==============================================================================
    bool isVisible() const;
    virtual void setVisible (bool shouldBeVisible);
    virtual void visibilityChanged();

    //==============================================================================
    String getTitle() const;
    virtual void setTitle (const String& title);

    //==============================================================================
    Point<float> getPosition() const;
    float getX() const;
    float getY() const;
    virtual void moved();

    //==============================================================================
    virtual void setSize (const Size<float>& newSize);
    Size<float> getSize() const;
    Size<float> getContentSize() const;
    float getWidth() const;
    float getHeight() const;
    virtual void setBounds (const Rectangle<float>& newBounds);
    Rectangle<float> getBounds() const;
    Rectangle<float> getLocalBounds() const;
    float proportionOfWidth (float proportion) const;
    float proportionOfHeight (float proportion) const;
    virtual void resized();

    //==============================================================================
    virtual void setFullScreen (bool shouldBeFullScreen);
    bool isFullScreen() const;

    //==============================================================================
    float getScaleDpi() const;

    //==============================================================================
    float getOpacity() const;
    virtual void setOpacity (float opacity);

    //==============================================================================
    virtual void enableRenderingUnclipped (bool shouldBeEnabled);
    bool isRenderingUnclipped() const;

    void repaint();
    void repaint (const Rectangle<float>& rect);

    //==============================================================================
    void* getNativeHandle() const;

    //==============================================================================
    ComponentNative* getNativeComponent();
    const ComponentNative* getNativeComponent() const;

    //==============================================================================
    bool isOnDesktop() const;
    void addToDesktop (ComponentNative::Flags flags,
                       void* parent = nullptr,
                       std::optional<float> framerateRedraw = std::nullopt);
    void removeFromDesktop();

    virtual void userTriedToCloseWindow();

    //==============================================================================
    Component* getParentComponent();

    template <class T>
    T* getParentComponentOfType()
    {
        auto parent = parentComponent;
        while (parent != nullptr)
        {
            if (auto foundParent = dynamic_cast<T*> (parent))
                return foundParent;

            parent = parent->getParentComponent();
        }

        return nullptr;
    }

    //==============================================================================
    void addChildComponent (Component& component);
    void addChildComponent (Component* component);

    void addAndMakeVisible (Component& component);
    void addAndMakeVisible (Component* component);

    void insertChildComponent (Component& component, int index);
    void insertChildComponent (Component* component, int index);

    void removeChildComponent (Component& component);
    void removeChildComponent (Component* component);

    //==============================================================================
    int getNumChildComponents() const;
    Component* getComponentAt (int index) const;
    Component* findComponentAt (const Point<float>& p);

    //==============================================================================
    void toFront();
    void toBack();

    //==============================================================================
    void setWantsKeyboardFocus (bool wantsFocus);

    void takeFocus();
    void leaveFocus();
    bool hasFocus() const;

    //==============================================================================
    NamedValueSet& getProperties();
    const NamedValueSet& getProperties() const;

    //==============================================================================
    virtual void paint (Graphics& g);
    virtual void paintOverChildren (Graphics& g);

    //==============================================================================
    virtual void mouseEnter (const MouseEvent& event);
    virtual void mouseExit (const MouseEvent& event);
    virtual void mouseDown (const MouseEvent& event);
    virtual void mouseMove (const MouseEvent& event);
    virtual void mouseDrag (const MouseEvent& event);
    virtual void mouseUp (const MouseEvent& event);
    virtual void mouseDoubleClick (const MouseEvent& event);
    virtual void mouseWheel (const MouseEvent& event, const MouseWheelData& wheelData);

    //==============================================================================
    virtual void keyDown (const KeyPress& keys, const Point<float>& position);
    virtual void keyUp (const KeyPress& keys, const Point<float>& position);

private:
    void internalPaint (Graphics& g, bool renderContinuous);
    void internalMouseEnter (const MouseEvent& event);
    void internalMouseExit (const MouseEvent& event);
    void internalMouseDown (const MouseEvent& event);
    void internalMouseMove (const MouseEvent& event);
    void internalMouseDrag (const MouseEvent& event);
    void internalMouseUp (const MouseEvent& event);
    void internalMouseDoubleClick (const MouseEvent& event);
    void internalMouseWheel (const MouseEvent& event, const MouseWheelData& wheelData);
    void internalKeyDown (const KeyPress& keys, const Point<float>& position);
    void internalKeyUp (const KeyPress& keys, const Point<float>& position);
    void internalMoved (int xpos, int ypos, float scaleDpi);
    void internalResized (int width, int height, float scaleDpi);
    void internalUserTriedToCloseWindow();

    friend class ComponentNative;
    friend class GLFWComponentNative;
    friend class WeakReference<Component>;

    String componentID, componentTitle;
    Component* parentComponent = nullptr;
    Array<Component*> children;
    Rectangle<float> boundsInParent;
    std::unique_ptr<ComponentNative> native;
    WeakReference<Component>::Master masterReference;
    NamedValueSet properties;
    uint8 opacity = 255;

    struct Options
    {
        bool isVisible : 1;
        bool isDisabled : 1;
        bool hasFrame : 1;
        bool onDesktop : 1;
        bool isFullScreen : 1;
        bool unclippedRendering : 1;
        bool wantsKeyboardFocus : 1;
    };

    union
    {
        uint32 optionsValue;
        Options options;
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Component)
};

} // namespace yup
