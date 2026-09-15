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

//==============================================================================
/** An opt-in interface that lets a Component start a drag.

    Like DragAndDropTarget this is deliberately not part of Component: a component opts in by also
    deriving from DragAndDropSource, then starts a drag from its own mouseDrag() once the gesture
    has moved past whatever threshold it considers a drag.

    @code
    class TrackHeader : public Component, public DragAndDropSource
    {
        void mouseDrag (const MouseEvent& event) override
        {
            const auto delta = event.getPosition() - event.getLastMouseDownPosition();

            if (isCurrentlyDragging() || delta.getX() * delta.getX() + delta.getY() * delta.getY() < 64.0f)
                return;

            startDragging (DragOptions{}
                               .withData (DragAndDropData().withText (trackName))
                               .withImageOpacity (0.6f));
        }
    };
    @endcode

    @see DragAndDropTarget, DragAndDropManager
*/
class YUP_API DragAndDropSource
{
public:
    //==============================================================================
    /** Describes what is being dragged and how the ghost should look.

        Copyable by design, so the fluent
        `startDragging (DragOptions{}.withData (...).withImageOpacity (...))` form compiles: the
        builders return a reference and startDragging() takes the result by value. That is also why
        a live drag image is a non-owning Component* rather than a smart pointer.
    */
    struct DragOptions
    {
        /** The payload to drag. Required: an empty payload cannot start a drag. */
        DragAndDropData data;

        /** An optional live component to show as the ghost. The caller keeps ownership. */
        Component* dragImageComponent = nullptr;

        /** An optional static image to show as the ghost. */
        Image dragImage;

        /** The point within the ghost that sits under the cursor. */
        Point<float> imageOffset;

        /** The opacity applied to the ghost window. */
        float imageOpacity = 0.7f;

        /** The operations this drag offers. */
        DragAndDropActions allowedActions = dragAndDropActionCopy | dragAndDropActionMove | dragAndDropActionLink;

        /** Whether the drag may leave the application to the OS.

            @note Not honoured yet: handing the gesture to the native drag-and-drop implementation is
                  the remaining native-export work. Internal drags are unaffected.
        */
        bool allowExternalDrag = false;

        //==============================================================================
        /** Sets the payload. */
        DragOptions& withData (DragAndDropData newData);

        /** Sets a static drag image. */
        DragOptions& withDragImage (Image newImage, Point<float> offset = {});

        /** Sets a live component as the drag image. */
        DragOptions& withDragImageComponent (Component* component, Point<float> offset = {});

        /** Sets the ghost window opacity. */
        DragOptions& withImageOpacity (float newOpacity);

        /** Sets the operations this drag offers. */
        DragOptions& withAllowedActions (DragAndDropActions newActions);

        /** Sets whether the drag may leave the application. */
        DragOptions& withExternalDragAllowed (bool shouldAllowExternalDrag);
    };

    //==============================================================================
    /** Destructor. */
    virtual ~DragAndDropSource() = default;

    //==============================================================================
    /** Starts a drag carrying @a options.

        @returns true when a drag session was started, false when one is already in flight.
    */
    bool startDragging (DragOptions options);

    /** Returns true while a drag started by this object is in flight. */
    bool isCurrentlyDragging() const;

    //==============================================================================
    /** Called when this source's drag starts. */
    virtual void dragOperationStarted (const DragAndDropData& data);

    /** Called when this source's drag ends, with the action that was performed.

        @param performed DragAndDropAction::none when nothing accepted the drop.
    */
    virtual void dragOperationEnded (const DragAndDropData& data, DragAndDropAction performed);

    /** Assignable alternative to dragOperationStarted(). */
    std::function<void (const DragAndDropData&)> onDragStarted;

    /** Assignable alternative to dragOperationEnded(). */
    std::function<void (const DragAndDropData&, DragAndDropAction)> onDragEnded;

    //==============================================================================
    /** Returns this object as a Component.

        A DragAndDropSource must also be a Component, so this is never null for a correctly
        constructed source; it asserts if that invariant is broken.
    */
    Component* getDragSourceComponent();
};

} // namespace yup
