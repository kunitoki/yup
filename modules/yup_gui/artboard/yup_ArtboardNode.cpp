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

ArtboardNode::ArtboardNode (Artboard& owner, rive::Component* node)
    : owner (&owner)
    , node (node)
    , epochValue (owner.nodeEpoch)
{
}

//==============================================================================

Artboard* ArtboardNode::getOwnerArtboard() const noexcept
{
    return static_cast<Artboard*> (owner.get());
}

//==============================================================================

bool ArtboardNode::isValid() const
{
    if (node == nullptr)
        return false;

    if (auto* ownerArtboard = getOwnerArtboard())
        return ownerArtboard->nodeEpoch == epochValue;

    return false;
}

//==============================================================================

String ArtboardNode::getName() const
{
    if (! isValid())
        return {};

    return String (node->name());
}

uint16_t ArtboardNode::getTypeKey() const
{
    if (! isValid())
        return 0;

    return node->coreType();
}

String ArtboardNode::getTypeName() const
{
    if (! isValid())
        return {};

    switch (node->coreType())
    {
        case rive::ArtboardBase::typeKey:               return "Artboard";
        case rive::LayoutComponentBase::typeKey:        return "LayoutComponent";
        case rive::NodeBase::typeKey:                   return "Node";
        case rive::ShapeBase::typeKey:                  return "Shape";
        case rive::RectangleBase::typeKey:              return "Rectangle";
        case rive::EllipseBase::typeKey:                return "Ellipse";
        case rive::ImageBase::typeKey:                  return "Image";
        case rive::TextBase::typeKey:                   return "Text";
        case rive::BoneBase::typeKey:                   return "Bone";
        case rive::SoloBase::typeKey:                   return "Solo";
        case rive::NestedArtboardBase::typeKey:         return "NestedArtboard";
        default:                                        return {};
    }
}

//==============================================================================

bool ArtboardNode::isLayout() const
{
    return isValid() && node->is<rive::LayoutComponent>();
}

Rectangle<float> ArtboardNode::getBounds() const
{
    if (! isValid())
        return {};

    return getOwnerArtboard()->computeNodeBounds (node);
}

AffineTransform ArtboardNode::getLocalTransform() const
{
    if (! isValid())
        return {};

    if (node->is<rive::TransformComponent>())
        return AffineTransform (node->as<rive::TransformComponent>()->transform());

    return {};
}

AffineTransform ArtboardNode::getWorldTransform() const
{
    if (! isValid())
        return {};

    if (node->is<rive::TransformComponent>())
        return AffineTransform (node->as<rive::TransformComponent>()->worldTransform());

    return {};
}

AffineTransform ArtboardNode::getViewTransform() const
{
    if (! isValid())
        return {};

    if (node->is<rive::TransformComponent>())
        return AffineTransform (getOwnerArtboard()->viewTransform * node->as<rive::TransformComponent>()->worldTransform());

    return {};
}
//==============================================================================

ArtboardNode::Ptr ArtboardNode::getParent() const
{
    if (! isValid())
        return nullptr;

    auto* parent = node->parent();
    if (parent == nullptr)
        return nullptr;

    return new ArtboardNode (*getOwnerArtboard(), parent);
}

Array<ArtboardNode::Ptr> ArtboardNode::getChildren() const
{
    Array<ArtboardNode::Ptr> children;

    if (! isValid())
        return children;

    if (! node->is<rive::ContainerComponent>())
        return children;

    for (auto* child : node->as<rive::ContainerComponent>()->children())
        children.add (new ArtboardNode (*getOwnerArtboard(), child));

    return children;
}

} // namespace yup
