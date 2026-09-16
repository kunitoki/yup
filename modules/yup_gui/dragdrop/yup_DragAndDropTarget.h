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
/** The operation a drag offers to perform when it is dropped, modelled on the
    clipboard / platform drag-and-drop conventions.

    @see DragAndDropActions
*/
enum class DragAndDropAction
{
    /** No operation, or the drop was rejected. */
    none,

    /** The dropped data should be copied to the target. */
    copy,

    /** The dropped data should be moved to the target. */
    move,

    /** The target should create a link to the dropped data. */
    link
};

/** @internal Identifies the individual action bits of a DragAndDropActions set. */
namespace detail
{
// clang-format off
struct DragAndDropCopyActionTag {};
struct DragAndDropMoveActionTag {};
struct DragAndDropLinkActionTag {};
//clang-format on
} // namespace detail

/** A set of possible DragAndDropAction values, used to negotiate what a drop can do.
 *
 *  @see DragAndDropAction
 */
using DragAndDropActions = FlagSet<uint32,
                                   detail::DragAndDropCopyActionTag,
                                   detail::DragAndDropMoveActionTag,
                                   detail::DragAndDropLinkActionTag>;

/** An empty action set. */
inline constexpr DragAndDropActions dragAndDropActionsNone = DragAndDropActions();

/** The copy action. */
inline constexpr DragAndDropActions dragAndDropActionCopy = DragAndDropActions::declareValue<detail::DragAndDropCopyActionTag>();

/** The move action. */
inline constexpr DragAndDropActions dragAndDropActionMove = DragAndDropActions::declareValue<detail::DragAndDropMoveActionTag>();

/** The link action. */
inline constexpr DragAndDropActions dragAndDropActionLink = DragAndDropActions::declareValue<detail::DragAndDropLinkActionTag>();

//==============================================================================
/** Describes the data being dragged and where it currently is.

    This is handed to every DragAndDropTarget callback, and its @a localPosition is
    always expressed in the coordinates of the target being notified.

    @see DragAndDropTarget
*/
struct DragAndDropSourceDetails
{
    /** The payload being dragged. */
    DragAndDropData data;

    /** The component the drag started from.

        The reference is weak, so it reads as null for an OS-originated drag, and also once
        the source has been destroyed - always check it before use rather than assuming the
        source outlives the drag.
    */
    WeakReference<Component> sourceComponent;

    /** The cursor position, in the coordinates of the target being notified. */
    Point<float> localPosition;

    /** The set of operations the source is willing to perform. */
    DragAndDropActions allowedActions = dragAndDropActionCopy | dragAndDropActionMove | dragAndDropActionLink;

    /** The operation the source suggests, derived from the modifier keys. */
    DragAndDropAction suggestedAction = DragAndDropAction::copy;
};

//==============================================================================
/** An opt-in interface that lets a Component receive items dragged onto it.

    Unlike the rest of YUP's component input handling, drag-and-drop targets are
    deliberately *not* part of Component: a component opts in to receiving drops by
    also deriving from DragAndDropTarget, and the library discovers it with a
    `dynamic_cast`. This keeps Component small and isolates the drag-and-drop
    surface in this module.

    A DragAndDropTarget must also be a Component (see getTargetComponent()); the
    wrapper helpers here only ever query instances obtained from a Component
    hierarchy, so that invariant always holds.

    Callbacks can be received either by overriding the virtual methods or by
    assigning the matching `std::function` member - the virtual runs first and the
    function afterwards, so both mechanisms work and the boolean-returning pairs
    OR-combine. This mirrors the dual-API convention used elsewhere in YUP.

    The bubbling semantics match the component tree: a drop is offered to the
    deepest component under the cursor first and ascends through its parents until
    one accepts it, while enter/move/exit are delivered to every interested ancestor.

    @see DragAndDropData, DragAndDropSourceDetails
*/
class YUP_API DragAndDropTarget
{
public:
    //==============================================================================
    /** Destructor. */
    virtual ~DragAndDropTarget() = default;

    //==============================================================================
    /** Returns true if this target wants to receive the drag described by @a details.

        Defaults to returning false, so a target must opt in either by overriding this
        or by assigning onIsInterestedInDragSource. Nothing else is delivered until this
        answers true.

        @note For a drag that originated outside the application, the operating system does not
              report what is being dragged until it is actually dropped, so @a details carries an
              empty payload for the whole time the drag hovers. A target that wants to react to
              such a drag before it lands - to highlight itself, say - therefore has to accept an
              empty payload here.
    */
    virtual bool isInterestedInDragSource (const DragAndDropSourceDetails& details);

    /** Called when an accepted drag enters this target. */
    virtual void itemDragEnter (const DragAndDropSourceDetails& details);

    /** Called when an accepted drag moves within this target. */
    virtual void itemDragMove (const DragAndDropSourceDetails& details);

    /** Called when an accepted drag leaves this target. */
    virtual void itemDragExit (const DragAndDropSourceDetails& details);

    /** Called when an accepted drag is dropped onto this target.

        @returns true if the drop was handled, which stops it bubbling to parents.
    */
    virtual bool itemDropped (const DragAndDropSourceDetails& details);

    //==============================================================================
    /** Assignable alternative to isInterestedInDragSource(). */
    std::function<bool (const DragAndDropSourceDetails&)> onIsInterestedInDragSource;

    /** Assignable alternative to itemDragEnter(). */
    std::function<void (const DragAndDropSourceDetails&)> onItemDragEnter;

    /** Assignable alternative to itemDragMove(). */
    std::function<void (const DragAndDropSourceDetails&)> onItemDragMove;

    /** Assignable alternative to itemDragExit(). */
    std::function<void (const DragAndDropSourceDetails&)> onItemDragExit;

    /** Assignable alternative to itemDropped(). */
    std::function<bool (const DragAndDropSourceDetails&)> onItemDropped;

    //==============================================================================
    /** Returns this object as a Component.

        A DragAndDropTarget must also be a Component, so this is never null for a
        correctly constructed target; it asserts if that invariant is broken.
    */
    Component* getTargetComponent();

    //==============================================================================
    /** @internal Offers a drop to @a topmostComponent and, if it does not handle it,
        to each of its ancestors. @a windowPosition is in the coordinates of the
        top-most component in the hierarchy.

        @returns true if some target handled the drop.
    */
    static bool dispatchItemDrop (Component& topmostComponent,
                                  const DragAndDropData& data,
                                  const Point<float>& windowPosition);

    /** @internal Notifies every interested target from @a topmostComponent upwards
        that a drag has entered their area. @a windowPosition is in the coordinates of
        the top-most component in the hierarchy.
    */
    static void dispatchItemDragEnter (Component& topmostComponent,
                                       const DragAndDropData& data,
                                       const Point<float>& windowPosition);

    /** @internal Notifies every interested target from @a topmostComponent upwards
        that a drag has moved within their area. @a windowPosition is in the coordinates
        of the top-most component in the hierarchy.
    */
    static void dispatchItemDragMove (Component& topmostComponent,
                                      const DragAndDropData& data,
                                      const Point<float>& windowPosition);

    /** @internal Notifies every interested target from @a topmostComponent upwards
        that a drag has left their area.
    */
    static void dispatchItemDragExit (Component& topmostComponent,
                                      const DragAndDropData& data);
};

//==============================================================================
/** A Component that is also a DragAndDropTarget.

    Deriving from both bases is enough for most call sites, but some places need a single
    concrete type that is nameable on its own - factories, containers, and the language
    bindings, where a Python class can only subclass one bound C++ type.

    @see DragAndDropTarget, DragAndDropSourceDetails
*/
class YUP_API DragAndDropTargetComponent : public Component
    , public DragAndDropTarget
{
public:
    //==============================================================================
    /** Creates a Component that can receive drops. */
    DragAndDropTargetComponent() = default;

    /** Creates a Component that can receive drops, with the given component ID.

        @param componentID The ID of the component.
    */
    DragAndDropTargetComponent (StringRef componentID)
        : Component (componentID)
    {
    }

    //==============================================================================
    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DragAndDropTargetComponent)
};

} // namespace yup
