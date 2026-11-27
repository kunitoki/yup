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

class Artboard;

//==============================================================================
/** Represents a single node in a Rive artboard.

    A read-only handle to a node of the currently loaded Rive file, obtained
    through Artboard::findNode(). Instances are refcounted and can be copied
    and stored freely.

    The handle is only valid while the Artboard that produced it keeps its
    current Rive file. Calling Artboard::clear() or Artboard::setFile() on the
    owning artboard, or destroying the artboard, invalidates every outstanding
    handle: isValid() then returns false and all accessors return safe defaults
    (empty strings, rectangles and arrays, identity transforms, null pointers)
    without ever dereferencing the underlying Rive objects.

    Validity is tracked without any shared state between the artboard and its
    handles: the handle keeps a WeakReference to its owning Component (the
    Artboard, whose lifetime the base class's weak-reference master tracks) and
    snapshots the artboard's node epoch counter, which the owning Artboard
    increments whenever its file is cleared or swapped. The epoch is only ever
    read while the artboard is alive, so it needs no shared storage of its own.

    All accessors are const and never mutate the Rive artboard.

    @see Artboard::findNode
*/
class YUP_API ArtboardNode : public ReferenceCountedObject
{
public:
    //==============================================================================
    using Ptr = ReferenceCountedObjectPtr<ArtboardNode>;

    //==============================================================================
    /** Destructor. */
    ~ArtboardNode() override = default;

    //==============================================================================
    /** Returns true if this handle still refers to a live node of its artboard.

        Handles become invalid when the owning Artboard is cleared, its file is
        replaced (Artboard::clear / Artboard::setFile), or the artboard is destroyed.
    */
    bool isValid() const;

    /** Returns the name of the node, or an empty string when invalid. */
    String getName() const;

    /** Returns the Rive core type key of the node, or 0 when invalid. */
    uint16_t getTypeKey() const;

    /** Returns a human-readable type name for the node.

        Returns an empty string when the handle is invalid, and also for any node
        whose Rive core type is outside the set this maps: Artboard, LayoutComponent,
        Node, Shape, Rectangle, Ellipse, Image, Text, Bone, Solo and NestedArtboard.
        Use getTypeKey() to tell an unmapped type from an invalid handle.
    */
    String getTypeName() const;

    //==============================================================================
    /** Returns true if the node is a Rive layout component. */
    bool isLayout() const;

    /** Returns the node bounds in the artboard's component coordinates.

        Layout nodes report their laid-out size; shapes and containers of shapes
        report their real geometry; other nodes report a unit-size rect at their
        origin. Returns an empty rectangle when the handle is invalid.
    */
    Rectangle<float> getBounds() const;

    /** Returns the node's local transform relative to its parent.

        Returns the identity transform when the handle is invalid or the node
        does not carry a transform.
    */
    AffineTransform getLocalTransform() const;

    /** Returns the node's world transform in artboard coordinates.

        Returns the identity transform when the handle is invalid or the node
        does not carry a transform.
    */
    AffineTransform getWorldTransform() const;

    /** Returns the node's world transform mapped through the artboard's view
        (fit) transform, i.e. the transform that positions, scales and orients
        the node inside the artboard component. This lives in the same
        coordinate space as getBounds().

        Returns the identity transform when the handle is invalid or the node
        does not carry a transform.
    */
    AffineTransform getViewTransform() const;

    //==============================================================================
    /** Returns the node's parent, or a null pointer when the node has no parent
        or the handle is invalid. */
    ArtboardNode::Ptr getParent() const;

    /** Returns the node's direct children, or an empty array when the node has
        none or the handle is invalid. */
    Array<ArtboardNode::Ptr> getChildren() const;

private:
    friend class Artboard;

    ArtboardNode (Artboard& owner, rive::Component* node);

    /** Returns the owning Artboard, or null if it has been destroyed.

        Artboard derives from Component, whose embedded master backs the weak
        reference; the downcast is safe because handles are only ever created
        for Artboard objects.
    */
    Artboard* getOwnerArtboard() const noexcept;

    WeakReference<Component> owner;
    rive::Component* node;
    uint64_t epochValue = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArtboardNode)
};

} // namespace yup
