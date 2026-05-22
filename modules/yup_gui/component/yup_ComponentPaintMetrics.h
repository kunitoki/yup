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

/** A single timing snapshot captured during one paint pass of a component.

    ComponentPaintMetrics captures the time spent in different phases of a component's paint
    operation, as well as contextual information about the paint event (bounds, dirty region,
    etc.). These snapshots are emitted to ComponentListeners after each paint pass, and are
    aggregated into PaintProfileStats for longer-term storage and analysis.
*/
struct ComponentPaintMetrics
{
    /** Ticks spent inside the component's own paint callback. */
    int64 selfTicks = 0;

    /** Ticks spent painting all children of this component. */
    int64 childrenTicks = 0;

    /** Ticks consumed by framework bookkeeping (transform setup, clip, etc.). */
    int64 frameworkTicks = 0;

    /** Total ticks from the start to the end of the full paint pass. */
    int64 totalTicks = 0;

    /** Axis-aligned bounding rectangle of the component in its parent's coordinate space. */
    Rectangle<float> componentBounds;

    /** The dirty region that triggered this repaint, in the component's local coordinate space. */
    Rectangle<float> repaintArea;

    /** True when the component requested a continuous repaint (e.g. animation loop). */
    bool renderContinuous = false;

    /** True when the component's own paint callback was skipped for this sample. */
    bool selfPaintSkipped = false;
};

} // namespace yup
