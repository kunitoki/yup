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

namespace
{

/** Converts a top-level (window) position into @a component's local coordinates. */
Point<float> toComponentLocalPosition (Component& component, const Point<float>& windowPosition)
{
    auto local = windowPosition;

    for (Component* current = &component; current != nullptr && current->getParentComponent() != nullptr; current = current->getParentComponent())
        local = local - current->getBounds().getPosition();

    return local;
}

/** Runs the virtual first, then the std::function; the two OR together. */
bool queryInterestedInDragSource (DragAndDropTarget& target, const DragAndDropSourceDetails& details)
{
    if (target.isInterestedInDragSource (details))
        return true;

    return target.onIsInterestedInDragSource && target.onIsInterestedInDragSource (details);
}

/** Runs the virtual first, then the std::function; the two OR together. */
bool performItemDrop (DragAndDropTarget& target, const DragAndDropSourceDetails& details)
{
    if (target.itemDropped (details))
        return true;

    return target.onItemDropped && target.onItemDropped (details);
}

void notifyItemDragEnter (DragAndDropTarget& target, const DragAndDropSourceDetails& details)
{
    target.itemDragEnter (details);

    if (target.onItemDragEnter)
        target.onItemDragEnter (details);
}

void notifyItemDragMove (DragAndDropTarget& target, const DragAndDropSourceDetails& details)
{
    target.itemDragMove (details);

    if (target.onItemDragMove)
        target.onItemDragMove (details);
}

void notifyItemDragExit (DragAndDropTarget& target, const DragAndDropSourceDetails& details)
{
    target.itemDragExit (details);

    if (target.onItemDragExit)
        target.onItemDragExit (details);
}

} // namespace

//==============================================================================

bool DragAndDropTarget::isInterestedInDragSource (const DragAndDropSourceDetails&) { return false; }

void DragAndDropTarget::itemDragEnter (const DragAndDropSourceDetails&) {}

void DragAndDropTarget::itemDragMove (const DragAndDropSourceDetails&) {}

void DragAndDropTarget::itemDragExit (const DragAndDropSourceDetails&) {}

bool DragAndDropTarget::itemDropped (const DragAndDropSourceDetails&) { return false; }

//==============================================================================

Component* DragAndDropTarget::getTargetComponent()
{
    auto* target = dynamic_cast<Component*> (this);

    // A DragAndDropTarget is only meaningful when it is also a Component.
    jassert (target != nullptr);

    return target;
}

//==============================================================================

bool DragAndDropTarget::dispatchItemDrop (Component& topmostComponent,
                                          const DragAndDropData& data,
                                          const Point<float>& windowPosition)
{
    DragAndDropSourceDetails details;
    details.data = data;

    auto localPosition = toComponentLocalPosition (topmostComponent, windowPosition);

    for (Component* current = &topmostComponent; current != nullptr; current = current->getParentComponent())
    {
        if (current->isVisible() && current->isEnabled())
        {
            if (auto* target = dynamic_cast<DragAndDropTarget*> (current))
            {
                details.localPosition = localPosition;

                if (queryInterestedInDragSource (*target, details) && performItemDrop (*target, details))
                    return true;
            }
        }

        // Ascend to the parent: the parent-local position adds back this component's offset.
        if (current->getParentComponent() != nullptr)
            localPosition = localPosition + current->getBounds().getPosition();
    }

    return false;
}

//==============================================================================

void DragAndDropTarget::dispatchItemDragEnter (Component& topmostComponent,
                                               const DragAndDropData& data,
                                               const Point<float>& windowPosition)
{
    DragAndDropSourceDetails details;
    details.data = data;

    auto localPosition = toComponentLocalPosition (topmostComponent, windowPosition);

    for (Component* current = &topmostComponent; current != nullptr; current = current->getParentComponent())
    {
        if (current->isVisible() && current->isEnabled())
        {
            if (auto* target = dynamic_cast<DragAndDropTarget*> (current))
            {
                details.localPosition = localPosition;

                if (queryInterestedInDragSource (*target, details))
                    notifyItemDragEnter (*target, details);
            }
        }

        if (current->getParentComponent() != nullptr)
            localPosition = localPosition + current->getBounds().getPosition();
    }
}

void DragAndDropTarget::dispatchItemDragMove (Component& topmostComponent,
                                              const DragAndDropData& data,
                                              const Point<float>& windowPosition)
{
    DragAndDropSourceDetails details;
    details.data = data;

    auto localPosition = toComponentLocalPosition (topmostComponent, windowPosition);

    for (Component* current = &topmostComponent; current != nullptr; current = current->getParentComponent())
    {
        if (current->isVisible() && current->isEnabled())
        {
            if (auto* target = dynamic_cast<DragAndDropTarget*> (current))
            {
                details.localPosition = localPosition;

                if (queryInterestedInDragSource (*target, details))
                    notifyItemDragMove (*target, details);
            }
        }

        if (current->getParentComponent() != nullptr)
            localPosition = localPosition + current->getBounds().getPosition();
    }
}

void DragAndDropTarget::dispatchItemDragExit (Component& topmostComponent,
                                              const DragAndDropData& data)
{
    // A drag exit carries no meaningful cursor position, so localPosition is left at its
    // default and targets must not rely on it.
    DragAndDropSourceDetails details;
    details.data = data;

    for (Component* current = &topmostComponent; current != nullptr; current = current->getParentComponent())
    {
        if (current->isVisible() && current->isEnabled())
        {
            if (auto* target = dynamic_cast<DragAndDropTarget*> (current))
            {
                if (queryInterestedInDragSource (*target, details))
                    notifyItemDragExit (*target, details);
            }
        }
    }
}

} // namespace yup
