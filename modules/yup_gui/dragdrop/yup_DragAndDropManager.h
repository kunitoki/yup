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

class DragImageComponent;

//==============================================================================
/** Owns the in-flight drag, wherever it goes.

    A drag has to outlive any single component hierarchy: the pointer can cross into another window
    and can leave every YUP window entirely, so the session cannot hang off a parent component. One
    app-global instance owns the payload, the source, the ghost window and the currently hovered
    component.

    While a drag is live the manager listens for global mouse events - the same mechanism PopupMenu
    uses - so the gesture keeps flowing even when the pointer is outside every window, and it
    resolves the component under the cursor across all native windows through
    Desktop::findComponentAt.

    @see DragAndDropSource, DragAndDropTarget
*/
class YUP_API DragAndDropManager final : private MouseListener
    , private DeletedAtShutdown
{
public:
    //==============================================================================
    YUP_DECLARE_SINGLETON (DragAndDropManager, false)

    //==============================================================================
    /** Destructor. Cancels any drag still in flight. */
    ~DragAndDropManager();

    //==============================================================================
    /** Starts a drag for @a source, which lives in @a sourceComponent.

        @param source          The source that will be notified when the drag ends.
        @param sourceComponent The component the drag starts from.
        @param options         The payload, ghost and offered operations.

        @returns true when a session was started, false when one is already in flight or the payload is empty.
    */
    bool startDragging (DragAndDropSource& source, Component& sourceComponent, DragAndDropSource::DragOptions options);

    /** Ends the current drag without performing a drop. */
    void cancelDrag();

    //==============================================================================
    /** Returns true while a drag is in flight. */
    bool isDragging() const noexcept;

    /** Returns the payload of the current drag, or an empty payload when none is running. */
    const DragAndDropData& getCurrentDragData() const noexcept;

    /** Returns the component the current drag started from, or nullptr. */
    Component* getCurrentDragSourceComponent() const;

    /** Returns the component currently under the drag, when it is itself a DragAndDropTarget.

        @note The dispatcher offers a drop to the deepest component first and then to its ancestors,
              so the component that actually acted on the drag may be an ancestor of this one.
    */
    DragAndDropTarget* getCurrentDragTarget() const;

    //==============================================================================
    /** @internal Called by the platform backend when an OS-originated drag moves over a window.

        @param rootComponent The window's root component.
        @param position      The position, in @a rootComponent's coordinates.
        @param data          The payload the platform reconstructed.
    */
    void handleExternalDragPosition (Component& rootComponent, const Point<float>& position, const DragAndDropData& data);

    /** @internal Called by the platform backend when an OS-originated drag is dropped.

        @returns true when a target handled the drop.
    */
    bool handleExternalDrop (Component& rootComponent, const Point<float>& position, const DragAndDropData& data);

    /** @internal Called by the platform backend when an OS-originated drag leaves. */
    void handleExternalDragExit();

private:
    //==============================================================================
    DragAndDropManager();

    bool beginSession (DragAndDropSource& source, Component& sourceComponent, DragAndDropSource::DragOptions&& options);
    void endSession (DragAndDropAction performed);
    void createGhost();

    Component* resolveComponentAt (const Point<float>& screenPosition) const;
    Component* getGhostComponent() const;
    Point<float> toWindowPosition (Component& component, const Point<float>& screenPosition) const;
    void moveGhostTo (const Point<float>& screenPosition);

    void dispatchHover (Component* component, const Point<float>& screenPosition, const DragAndDropData& data);
    void clearHoveredTarget (const DragAndDropData& data);

    void mouseDrag (const MouseEvent& event) override;
    void mouseUp (const MouseEvent& event) override;

    //==============================================================================
    DragAndDropData currentData;
    DragAndDropSource::DragOptions currentOptions;
    WeakReference<Component> sourceComponent;
    WeakReference<Component> hoveredComponent;
    DragAndDropData externalDragData;
    std::unique_ptr<DragImageComponent> ghost;
    bool cancelled = false;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DragAndDropManager)
};

} // namespace yup
