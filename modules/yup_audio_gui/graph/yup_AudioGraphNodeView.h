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
    A visual node embedded by AudioGraphComponent.

    AudioGraphNodeView is a regular Component, so subclasses can add child controls,
    meters, or custom painting while the graph canvas handles placement, panning,
    zooming, and connection gestures.

    @see AudioGraphComponent, AudioGraphProcessor
*/
class YUP_API AudioGraphNodeView : public Component
{
public:
    /** Semantic signal type for ports and future connection routing. */
    enum class PortKind
    {
        audio,
        midi,
        parameter
    };

    /** Describes a single input or output port. */
    struct PortInfo
    {
        /** Display name shown next to the port. */
        String name;

        /** Port fill color. */
        Color color;

        /** Signal type carried by the port. */
        PortKind kind = PortKind::audio;
    };

    /** Describes an inline parameter row inside a node. */
    struct ParameterInfo
    {
        /** Parameter display name. */
        String name;

        /** Current parameter value display text. */
        String value;

        /** Accent color for the value and optional modulation port. */
        Color color;

        /** Normalized value in [0, 1], or a negative value to hide the bar. */
        float normalizedValue = -1.0f;

        /** Signal type used for this row's future modulation port. */
        PortKind kind = PortKind::parameter;
    };

    /** Identifies the port hit by the user. */
    struct PortHit
    {
        /** Bus index on the node processor. */
        int busIndex = -1;

        /** True for input ports, false for output ports. */
        bool isInput = false;
    };

    /** Creates a node view for a graph node identifier. */
    explicit AudioGraphNodeView (AudioGraphNodeID nodeID);

    /** Destructor. */
    ~AudioGraphNodeView() override;

    /** Returns the graph node identifier represented by this view. */
    AudioGraphNodeID getNodeID() const noexcept { return nodeID; }

    /** Returns the title rendered in the node header. */
    virtual String getNodeTitle() const = 0;

    /** Returns the number of input ports. */
    virtual int getNumInputPorts() const = 0;

    /** Returns the number of output ports. */
    virtual int getNumOutputPorts() const = 0;

    /** Returns the node accent color. */
    virtual Color getNodeColor() const;

    /** Returns the subtitle rendered below the title. */
    virtual String getNodeSubtitle() const;

    /** Returns display metadata for an input port. */
    virtual PortInfo getInputPortInfo (int busIndex) const;

    /** Returns display metadata for an output port. */
    virtual PortInfo getOutputPortInfo (int busIndex) const;

    /** Returns the number of inline parameter rows. */
    virtual int getNumParameterRows() const;

    /** Returns display metadata for an inline parameter row. */
    virtual ParameterInfo getParameterInfo (int parameterIndex) const;

    /** Paints optional custom content between the header and the port rows. */
    virtual void paintNodeContent (Graphics& g, Rectangle<float> contentBounds);

    /** Returns the preferred width in canvas units. */
    virtual int getPreferredWidth() const;

    /** Returns the computed preferred height in canvas units. */
    int getPreferredHeight() const;

    /** Returns the local center of an input port. */
    Point<float> getInputPortCenter (int busIndex) const;

    /** Returns the local center of an output port. */
    Point<float> getOutputPortCenter (int busIndex) const;

    /** Returns the port radius in current local display units. */
    float getPortRadius() const;

    /** Returns the port at localPos when it is within the port radius. */
    std::optional<PortHit> hitTestPort (Point<float> localPos) const;

    /** Returns the default color for a semantic port kind. */
    static Color getPortKindColor (PortKind kind);

    /** @internal Sets the display scale applied by AudioGraphComponent. */
    void setViewScale (float newScale);

    /** @internal */
    void paint (Graphics& g) override;

private:
    Point<float> getPortCenter (int busIndex, bool isInput) const;
    Rectangle<float> getPortArea() const;

    AudioGraphNodeID nodeID;
    float viewScale = 1.0f;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioGraphNodeView)
};

} // namespace yup
