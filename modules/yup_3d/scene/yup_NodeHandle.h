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
/** A checked, non-owning reference to the node of type T of an entity.

    It resolves the node again on every access, so it never dangles: get() returns nullptr
    once the node has been detached or its entity destroyed. When a new node of type T is
    attached to the same entity, the handle refers to the new node.

    @code
    yup::NodeHandle<yup::LightNode> key = entity.attach<yup::LightNode> (yup::LightNode::Type::point);

    if (key)
        key->intensity = 2.0f;
    @endcode

    @see Entity, Node
*/
template <class T>
class NodeHandle
{
public:
    //==============================================================================
    /** Creates a handle that refers to nothing. */
    NodeHandle() noexcept = default;

    /** Creates a handle to an attached node. */
    NodeHandle (T& node) noexcept
        : entity (node.getEntity())
    {
    }

    /** Creates a handle to the node of type T of an entity, which may not be attached yet. */
    explicit NodeHandle (Entity owner) noexcept
        : entity (owner)
    {
    }

    //==============================================================================
    /** Returns the node, or nullptr if the entity has no node of type T anymore. */
    T* get() const noexcept { return entity.template get<T>(); }

    /** Returns the node, which must exist. */
    T* operator->() const noexcept
    {
        auto* node = get();
        jassert (node != nullptr);
        return node;
    }

    /** Returns the node, which must exist. */
    T& operator*() const noexcept { return *operator->(); }

    /** Returns true if the node exists. */
    explicit operator bool() const noexcept { return get() != nullptr; }

    /** Returns the entity the handle looks the node up on. */
    Entity getEntity() const noexcept { return entity; }

    bool operator== (const NodeHandle& other) const noexcept { return entity == other.entity; }
    bool operator!= (const NodeHandle& other) const noexcept { return entity != other.entity; }

private:
    Entity entity;
};

} // namespace yup
