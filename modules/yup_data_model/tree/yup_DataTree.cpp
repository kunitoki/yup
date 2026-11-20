/*
  ==============================================================================

   This file is part of the YUP library.
   Copyright (c) 2025 - kunitoki@gmail.com

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
var coerceAttributeValue (const Identifier& nodeType,
                          const Identifier& propertyName,
                          const String& rawValue,
                          const ReferenceCountedObjectPtr<DataTreeSchema>& schema)
{
    if (schema == nullptr)
        return var (rawValue);

    auto info = schema->getPropertyInfo (nodeType, propertyName);
    if (info.type.isEmpty())
        return var (rawValue);

    const auto trimmed = rawValue.trim();

    const auto looksLikeInteger = [] (const String& text)
    {
        if (text.isEmpty())
            return false;

        int start = 0;
        if (text.startsWithChar ('-') || text.startsWithChar ('+'))
            start = 1;

        if (start == text.length())
            return false;

        for (int i = start; i < text.length(); ++i)
        {
            if (! CharacterFunctions::isDigit (text[i]))
                return false;
        }

        return true;
    };

    const auto looksLikeNumber = [] (const String& text)
    {
        bool hasDigit = false;

        for (int i = 0; i < text.length(); ++i)
        {
            const auto c = text[i];
            if (CharacterFunctions::isDigit (c))
            {
                hasDigit = true;
                continue;
            }

            if (c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E')
                continue;

            return false;
        }

        return hasDigit;
    };

    if (info.type == "boolean")
    {
        if (trimmed.equalsIgnoreCase ("true") || trimmed == "1" || trimmed.equalsIgnoreCase ("yes"))
            return var (true);

        if (trimmed.equalsIgnoreCase ("false") || trimmed == "0" || trimmed.equalsIgnoreCase ("no"))
            return var (false);

        return var (rawValue);
    }

    if (info.type == "number" && looksLikeNumber (trimmed))
    {
        if (looksLikeInteger (trimmed))
            return var (trimmed.getLargeIntValue());

        return var (trimmed.getDoubleValue());
    }

    if ((info.type == "array" || info.type == "object") && trimmed.isNotEmpty())
    {
        var parsed;
        if (JSON::parse (trimmed, parsed))
        {
            if (info.type == "array" && parsed.isArray())
                return parsed;

            if (info.type == "object" && parsed.isObject())
                return parsed;
        }
    }

    return var (rawValue);
}
} // namespace

//==============================================================================

class PropertySetAction : public UndoableAction
{
public:
    PropertySetAction (std::shared_ptr<DataTree::DataObject> obj, const Identifier& prop, const var& newVal, const var& oldVal)
        : dataObject (std::move (obj))
        , property (prop)
        , newValue (newVal)
        , oldValue (oldVal)
        , wasPropertyPresent (false)
    {
    }

    bool isValid() const override
    {
        return dataObject != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (dataObject == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            wasPropertyPresent = dataObject->properties.contains (property);

            dataObject->properties.set (property, newValue);
        }
        else
        {
            if (wasPropertyPresent)
                dataObject->properties.set (property, oldValue);
            else
                dataObject->properties.remove (property);
        }

        dataObject->sendPropertyChangeMessage (property);
        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> dataObject;
    Identifier property;
    var newValue, oldValue;
    bool wasPropertyPresent;
};

//==============================================================================

class PropertyRemoveAction : public UndoableAction
{
public:
    PropertyRemoveAction (std::shared_ptr<DataTree::DataObject> obj, const Identifier& prop, const var& oldVal)
        : dataObject (std::move (obj))
        , property (prop)
        , oldValue (oldVal)
    {
    }

    bool isValid() const override
    {
        return dataObject != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (dataObject == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
            dataObject->properties.remove (property);
        else
            dataObject->properties.set (property, oldValue);

        dataObject->sendPropertyChangeMessage (property);
        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> dataObject;
    Identifier property;
    var oldValue;
};

//==============================================================================

class RemoveAllPropertiesAction : public UndoableAction
{
public:
    explicit RemoveAllPropertiesAction (std::shared_ptr<DataTree::DataObject> obj)
        : dataObject (std::move (obj))
    {
    }

    bool isValid() const override
    {
        return dataObject != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (dataObject == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
            oldProperties = std::exchange (dataObject->properties, {});
        else
            dataObject->properties = oldProperties;

        for (int i = 0; i < oldProperties.size(); ++i)
            dataObject->sendPropertyChangeMessage (oldProperties.getName (i));

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> dataObject;
    NamedValueSet oldProperties;
};

//==============================================================================

class ReplacePropertiesAction : public UndoableAction
{
public:
    ReplacePropertiesAction (std::shared_ptr<DataTree::DataObject> obj, NamedValueSet replacement)
        : dataObject (std::move (obj))
        , newProperties (std::move (replacement))
    {
    }

    bool isValid() const override
    {
        return dataObject != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (dataObject == nullptr)
            return false;

        NamedValueSet previous;

        if (state == UndoableActionState::Redo)
        {
            oldProperties = dataObject->properties;
            previous = std::exchange (dataObject->properties, newProperties);
        }
        else
        {
            previous = std::exchange (dataObject->properties, oldProperties);
        }

        const auto& current = dataObject->properties;
        yup::Array<Identifier> changedNames;

        for (int i = 0; i < previous.size(); ++i)
        {
            const auto name = previous.getName (i);
            const auto* nextValue = current.getVarPointer (name);

            if (nextValue == nullptr || ! previous.getValueAt (i).equalsWithSameType (*nextValue))
                changedNames.addIfNotAlreadyThere (name);
        }

        for (int i = 0; i < current.size(); ++i)
        {
            const auto name = current.getName (i);

            if (! previous.contains (name))
                changedNames.addIfNotAlreadyThere (name);
        }

        for (const auto& name : changedNames)
            dataObject->sendPropertyChangeMessage (name);

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> dataObject;
    NamedValueSet newProperties;
    NamedValueSet oldProperties;
};

//==============================================================================

class AddChildAction : public UndoableAction
{
public:
    AddChildAction (std::shared_ptr<DataTree::DataObject> parent, std::shared_ptr<DataTree::DataObject> child, int idx)
        : parentObject (std::move (parent))
        , childObject (std::move (child))
        , index (idx)
    {
    }

    bool isValid() const override
    {
        return parentObject != nullptr && childObject != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentObject == nullptr || childObject == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            attached = false;

            // The tree may have changed since the edit was recorded
            if (childObject == parentObject || parentObject->isDescendantOf (*childObject))
                return false;

            auto currentParent = childObject->parent.lock();
            previousParent = currentParent;
            previousIndex = -1;

            if (currentParent != nullptr)
            {
                auto& currentChildren = currentParent->children;
                auto it = std::find (currentChildren.begin(), currentChildren.end(), childObject);
                if (it != currentChildren.end())
                {
                    previousIndex = static_cast<int> (std::distance (currentChildren.begin(), it));
                    currentChildren.erase (it);
                }
            }

            const int numChildren = static_cast<int> (parentObject->children.size());
            const int actualIndex = isPositiveAndBelow (index, numChildren) ? index : numChildren;

            parentObject->children.insert (parentObject->children.begin() + actualIndex, childObject);
            childObject->parent = parentObject;
            attached = true;

            if (currentParent != nullptr && previousIndex >= 0)
                currentParent->sendChildRemovedMessage (childObject, previousIndex);

            parentObject->sendChildAddedMessage (childObject);

            if (currentParent != parentObject)
                childObject->sendParentChangedMessage (std::move (currentParent));
        }
        else
        {
            if (! attached)
                return false;

            auto& parentChildren = parentObject->children;
            auto it = std::find (parentChildren.begin(), parentChildren.end(), childObject);
            if (it == parentChildren.end())
                return true;

            const int childIndex = static_cast<int> (std::distance (parentChildren.begin(), it));
            parentChildren.erase (it);

            auto prevParent = previousParent.lock();
            if (prevParent != nullptr)
            {
                const int numChildren = static_cast<int> (prevParent->children.size());
                const int actualIndex = (previousIndex < 0 || previousIndex > numChildren) ? numChildren : previousIndex;

                prevParent->children.insert (prevParent->children.begin() + actualIndex, childObject);
                childObject->parent = prevParent;
            }
            else
            {
                childObject->parent.reset();
            }

            parentObject->sendChildRemovedMessage (childObject, childIndex);

            if (prevParent != nullptr)
                prevParent->sendChildAddedMessage (childObject);

            if (prevParent != parentObject)
                childObject->sendParentChangedMessage (parentObject);
        }

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> parentObject;
    std::shared_ptr<DataTree::DataObject> childObject;
    int index;
    std::weak_ptr<DataTree::DataObject> previousParent;
    int previousIndex = -1;
    bool attached = false;
};

//==============================================================================

class RemoveChildAction : public UndoableAction
{
public:
    RemoveChildAction (std::shared_ptr<DataTree::DataObject> parent, std::shared_ptr<DataTree::DataObject> child, int idx)
        : parentObject (std::move (parent))
        , childObject (std::move (child))
        , index (idx)
    {
    }

    bool isValid() const override
    {
        return parentObject != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentObject == nullptr)
            return false;

        auto& parentChildren = parentObject->children;

        if (state == UndoableActionState::Redo)
        {
            removed = false;

            if (childObject != nullptr)
            {
                auto it = std::find (parentChildren.begin(), parentChildren.end(), childObject);
                if (it == parentChildren.end())
                    return false;

                index = static_cast<int> (std::distance (parentChildren.begin(), it));
            }
            else if (isPositiveAndBelow (index, static_cast<int> (parentChildren.size())))
            {
                childObject = parentChildren[static_cast<size_t> (index)];
            }
            else
            {
                return false;
            }

            parentChildren.erase (parentChildren.begin() + index);
            childObject->parent.reset();
            removed = true;
            parentObject->sendChildRemovedMessage (childObject, index);
            childObject->sendParentChangedMessage (parentObject);
        }
        else
        {
            if (! removed)
                return false;

            const int numChildren = static_cast<int> (parentChildren.size());
            const int actualIndex = isPositiveAndBelow (index, numChildren) ? index : numChildren;

            parentChildren.insert (parentChildren.begin() + actualIndex, childObject);
            childObject->parent = parentObject;
            parentObject->sendChildAddedMessage (childObject);
            childObject->sendParentChangedMessage (nullptr);
        }

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> parentObject;
    std::shared_ptr<DataTree::DataObject> childObject;
    int index;
    bool removed = false;
};

//==============================================================================

class RemoveAllChildrenAction : public UndoableAction
{
public:
    explicit RemoveAllChildrenAction (std::shared_ptr<DataTree::DataObject> parent)
        : parentObject (std::move (parent))
    {
    }

    bool isValid() const override
    {
        return parentObject != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentObject == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            children = std::exchange (parentObject->children, {});

            for (auto& child : children)
                child->parent.reset();

            for (int i = static_cast<int> (children.size()) - 1; i >= 0; --i)
            {
                const auto& child = children[static_cast<size_t> (i)];
                parentObject->sendChildRemovedMessage (child, i);
                child->sendParentChangedMessage (parentObject);
            }
        }
        else
        {
            parentObject->children = children;

            for (auto& child : children)
                child->parent = parentObject;

            for (auto& child : children)
            {
                parentObject->sendChildAddedMessage (child);
                child->sendParentChangedMessage (nullptr);
            }
        }

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> parentObject;
    std::vector<std::shared_ptr<DataTree::DataObject>> children;
};

//==============================================================================

class MoveChildAction : public UndoableAction
{
public:
    MoveChildAction (std::shared_ptr<DataTree::DataObject> parent, int fromIndex, int toIndex)
        : parentObject (std::move (parent))
        , oldIndex (fromIndex)
        , newIndex (toIndex)
    {
    }

    bool isValid() const override
    {
        return parentObject != nullptr && oldIndex != newIndex;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentObject == nullptr || oldIndex == newIndex)
            return false;

        const int numChildren = static_cast<int> (parentObject->children.size());
        if (! isPositiveAndBelow (oldIndex, numChildren) || ! isPositiveAndBelow (newIndex, numChildren))
            return false;

        if (state == UndoableActionState::Redo)
        {
            auto child = parentObject->children[static_cast<size_t> (oldIndex)];

            auto start = parentObject->children.begin();
            auto first = start + std::min (oldIndex, newIndex);
            auto middle = start + oldIndex;
            auto last = start + std::max (oldIndex, newIndex) + 1;
            std::rotate (first, middle + (oldIndex < newIndex), last);

            parentObject->sendChildMovedMessage (child, oldIndex, newIndex);
        }
        else
        {
            auto child = parentObject->children[static_cast<size_t> (newIndex)];

            auto start = parentObject->children.begin();
            auto first = start + std::min (newIndex, oldIndex);
            auto middle = start + newIndex;
            auto last = start + std::max (newIndex, oldIndex) + 1;
            std::rotate (first, middle + (newIndex < oldIndex), last);

            parentObject->sendChildMovedMessage (child, newIndex, oldIndex);
        }

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> parentObject;
    int oldIndex, newIndex;
};

//==============================================================================

class AddChildrenAction : public UndoableAction
{
public:
    AddChildrenAction (std::shared_ptr<DataTree::DataObject> parent, std::vector<std::shared_ptr<DataTree::DataObject>> childrenToAdd, int idx)
        : parentObject (std::move (parent))
        , children (std::move (childrenToAdd))
        , index (idx)
    {
    }

    bool isValid() const override
    {
        return parentObject != nullptr && ! children.empty();
    }

    bool perform (UndoableActionState state) override
    {
        if (parentObject == nullptr)
            return false;

        auto& siblings = parentObject->children;

        if (state == UndoableActionState::Redo)
        {
            steps.clear();
            insertedAt = -1;

            const int numSiblings = static_cast<int> (siblings.size());
            const bool appending = ! isPositiveAndBelow (index, numSiblings);

            if (! canInsertTogether())
            {
                // What addChild() does for each of them, one at a time
                for (std::size_t i = 0; i < children.size(); ++i)
                {
                    steps.push_back (new AddChildAction (parentObject, children[i], appending ? -1 : index + static_cast<int> (i)));
                    steps.back()->perform (UndoableActionState::Redo);
                }

                return true;
            }

            insertedAt = appending ? numSiblings : index;

            siblings.insert (siblings.begin() + insertedAt, children.begin(), children.end());

            for (const auto& child : children)
                child->parent = parentObject;

            for (const auto& child : children)
            {
                parentObject->sendChildAddedMessage (child);
                child->sendParentChangedMessage (nullptr);
            }

            return true;
        }

        if (insertedAt < 0)
        {
            for (auto it = steps.rbegin(); it != steps.rend(); ++it)
                (*it)->perform (UndoableActionState::Undo);

            return ! steps.empty();
        }

        const auto first = static_cast<std::size_t> (insertedAt);

        if (first + children.size() > siblings.size() || ! std::equal (children.begin(), children.end(), siblings.begin() + insertedAt))
            return false;

        siblings.erase (siblings.begin() + insertedAt, siblings.begin() + insertedAt + static_cast<std::ptrdiff_t> (children.size()));

        for (const auto& child : children)
            child->parent.reset();

        for (int i = static_cast<int> (children.size()); --i >= 0;)
        {
            const auto& child = children[static_cast<std::size_t> (i)];
            parentObject->sendChildRemovedMessage (child, insertedAt + i);
            child->sendParentChangedMessage (parentObject);
        }

        return true;
    }

private:
    bool canInsertTogether() const
    {
        for (const auto& child : children)
        {
            if (child == parentObject || ! child->parent.expired() || parentObject->isDescendantOf (*child))
                return false;
        }

        return true;
    }

    std::shared_ptr<DataTree::DataObject> parentObject;
    std::vector<std::shared_ptr<DataTree::DataObject>> children;
    int index;
    int insertedAt = -1;
    std::vector<UndoableAction::Ptr> steps;
};

//==============================================================================

class RemoveChildrenAction : public UndoableAction
{
public:
    RemoveChildrenAction (std::shared_ptr<DataTree::DataObject> parent, int start, int count)
        : parentObject (std::move (parent))
        , startIndex (start)
        , numChildren (count)
    {
    }

    RemoveChildrenAction (std::shared_ptr<DataTree::DataObject> parent, const std::vector<std::shared_ptr<DataTree::DataObject>>& childrenToRemove)
        : parentObject (std::move (parent))
        , listed (childrenToRemove.begin(), childrenToRemove.end())
    {
    }

    bool isValid() const override
    {
        return parentObject != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentObject == nullptr)
            return false;

        auto& siblings = parentObject->children;

        if (state == UndoableActionState::Redo)
        {
            removed.clear();

            const auto rangeStart = static_cast<int64> (startIndex);
            const auto rangeEnd = rangeStart + numChildren;

            for (std::size_t i = 0; i < siblings.size(); ++i)
            {
                const auto index = static_cast<int64> (i);

                if ((index >= rangeStart && index < rangeEnd) || listed.count (siblings[i]) != 0)
                    removed.push_back ({ static_cast<int> (i), siblings[i] });
            }

            if (removed.empty())
                return false;

            std::size_t numKept = 0, nextRemoved = 0;

            for (std::size_t i = 0; i < siblings.size(); ++i)
            {
                if (nextRemoved < removed.size() && removed[nextRemoved].first == static_cast<int> (i))
                    ++nextRemoved;
                else
                    siblings[numKept++] = std::move (siblings[i]);
            }

            siblings.resize (numKept);

            for (const auto& entry : removed)
                entry.second->parent.reset();

            // From the back, so that each former index is the one the child had when it left
            for (auto it = removed.rbegin(); it != removed.rend(); ++it)
            {
                parentObject->sendChildRemovedMessage (it->second, it->first);
                it->second->sendParentChangedMessage (parentObject);
            }

            return true;
        }

        if (removed.empty())
            return false;

        std::vector<std::shared_ptr<DataTree::DataObject>> restored;
        restored.reserve (siblings.size() + removed.size());

        auto kept = siblings.begin();

        for (const auto& entry : removed)
        {
            while (static_cast<int> (restored.size()) < entry.first && kept != siblings.end())
                restored.push_back (std::move (*kept++));

            restored.push_back (entry.second);
        }

        restored.insert (restored.end(), std::make_move_iterator (kept), std::make_move_iterator (siblings.end()));
        siblings = std::move (restored);

        for (const auto& entry : removed)
            entry.second->parent = parentObject;

        const auto restoredChildren = std::exchange (removed, {});

        for (const auto& entry : restoredChildren)
        {
            parentObject->sendChildAddedMessage (entry.second);
            entry.second->sendParentChangedMessage (nullptr);
        }

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> parentObject;
    std::unordered_set<std::shared_ptr<DataTree::DataObject>> listed;
    int startIndex = 0;
    int numChildren = 0;
    std::vector<std::pair<int, std::shared_ptr<DataTree::DataObject>>> removed;
};

//==============================================================================

class ReorderChildrenAction : public UndoableAction
{
public:
    ReorderChildrenAction (std::shared_ptr<DataTree::DataObject> parent, std::vector<std::shared_ptr<DataTree::DataObject>> orderToUse)
        : parentObject (std::move (parent))
        , newOrder (std::move (orderToUse))
    {
    }

    bool isValid() const override
    {
        return parentObject != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentObject == nullptr)
            return false;

        auto& siblings = parentObject->children;

        if (state == UndoableActionState::Redo)
        {
            previousOrder.clear();

            if (siblings == newOrder || ! isPermutationOf (siblings))
                return false;

            previousOrder = std::exchange (siblings, newOrder);

            for (const auto& move : findMoves())
                parentObject->sendChildMovedMessage (move.child, move.from, move.to);

            return true;
        }

        if (previousOrder.empty() || siblings != newOrder)
            return false;

        const auto moves = findMoves();
        siblings = std::exchange (previousOrder, {});

        for (auto it = moves.rbegin(); it != moves.rend(); ++it)
            parentObject->sendChildMovedMessage (it->child, it->to, it->from);

        return true;
    }

private:
    struct Move
    {
        std::shared_ptr<DataTree::DataObject> child;
        int from, to;
    };

    bool isPermutationOf (const std::vector<std::shared_ptr<DataTree::DataObject>>& siblings) const
    {
        if (siblings.size() != newOrder.size())
            return false;

        std::unordered_set<const DataTree::DataObject*> remaining;

        for (const auto& sibling : siblings)
            remaining.insert (sibling.get());

        for (const auto& child : newOrder)
        {
            if (remaining.erase (child.get()) == 0)
                return false;
        }

        return true;
    }

    /** Returns the moveChild() calls that turn the previous order into the new one, bringing each
        child to its index from the first to the last. Nobody is told when nobody listens.
    */
    std::vector<Move> findMoves() const
    {
        std::vector<Move> moves;

        if (parentObject->listeners.isEmpty())
            return moves;

        const auto numChildren = newOrder.size();
        std::unordered_map<const DataTree::DataObject*, std::size_t> previousIndices;

        for (std::size_t i = 0; i < numChildren; ++i)
            previousIndices[previousOrder[i].get()] = i;

        // Counts, in a binary indexed tree, the children already in place that came before a given index
        std::vector<int> placed (numChildren + 1, 0);

        for (std::size_t i = 0; i < numChildren; ++i)
        {
            const auto previousIndex = previousIndices[newOrder[i].get()];
            int numPlacedBefore = 0;

            for (auto node = previousIndex; node > 0; node -= node & (~node + 1))
                numPlacedBefore += placed[node];

            // Each child placed so far that came after this one has pushed it one index further
            const auto from = static_cast<int> (previousIndex + i) - numPlacedBefore;

            if (from != static_cast<int> (i))
                moves.push_back ({ newOrder[i], from, static_cast<int> (i) });

            for (auto node = previousIndex + 1; node <= numChildren; node += node & (~node + 1))
                ++placed[node];
        }

        return moves;
    }

    std::shared_ptr<DataTree::DataObject> parentObject;
    std::vector<std::shared_ptr<DataTree::DataObject>> newOrder;
    std::vector<std::shared_ptr<DataTree::DataObject>> previousOrder;
};

//==============================================================================

class CompoundAction : public UndoableAction
{
public:
    /** The edits of one node, in the order they were recorded. The actions keep the node alive. */
    struct NodeEdits
    {
        DataTree::DataObject* node = nullptr;
        std::vector<UndoableAction::Ptr> actions;
    };

    explicit CompoundAction (NodeEdits editsOfOneNode)
        : single (std::move (editsOfOneNode))
    {
    }

    explicit CompoundAction (std::vector<NodeEdits> editsOfSeveralNodes)
        : several (std::move (editsOfSeveralNodes))
    {
    }

    bool isValid() const override
    {
        return single.node != nullptr || ! several.empty();
    }

    bool perform (UndoableActionState state) override
    {
        return several.empty() ? apply ({ &single, single.node != nullptr ? 1u : 0u }, state)
                               : apply (several, state);
    }

    /** Applies the edits of the nodes in order, or takes them back in reverse, as one transaction. */
    static bool apply (Span<NodeEdits> edits, UndoableActionState state)
    {
        if (edits.empty())
            return false;

        for (const auto& edit : edits)
            edit.node->sendTransactionStartedMessage();

        const ScopeGuard sendEnded { [&edits]
        {
            for (auto i = edits.size(); i-- > 0;)
                edits[i].node->sendTransactionEndedMessage (DataTree::TransactionResult::committed);
        } };

        if (state == UndoableActionState::Redo)
        {
            for (const auto& edit : edits)
            {
                for (const auto& action : edit.actions)
                    action->perform (UndoableActionState::Redo);
            }
        }
        else
        {
            for (auto i = edits.size(); i-- > 0;)
            {
                for (auto action = edits[i].actions.rbegin(); action != edits[i].actions.rend(); ++action)
                    (*action)->perform (UndoableActionState::Undo);
            }
        }

        return true;
    }

private:
    NodeEdits single;
    std::vector<NodeEdits> several;
};

//==============================================================================

DataTree::DataObject::DataObject (const Identifier& treeType)
    : type (treeType)
{
}

DataTree::DataObject::~DataObject()
{
    const auto orphans = std::exchange (children, {});

    for (const auto& child : orphans)
        child->parent.reset();

    for (const auto& child : orphans)
    {
        if (child.use_count() > 1)
            child->sendParentChangedMessage (nullptr);
    }
}

void DataTree::DataObject::sendPropertyChangeMessage (const Identifier& property)
{
    DataTree treeObj (shared_from_this());
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.propertyChanged (treeObj, property);
    });
}

void DataTree::DataObject::sendChildAddedMessage (std::shared_ptr<DataObject> child)
{
    DataTree treeObj (shared_from_this());
    DataTree childTree (child);
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.childAdded (treeObj, childTree);
    });
}

void DataTree::DataObject::sendChildRemovedMessage (std::shared_ptr<DataObject> child, int formerIndex)
{
    DataTree treeObj (shared_from_this());
    DataTree childTree (child);
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.childRemoved (treeObj, childTree, formerIndex);
    });
}

void DataTree::DataObject::sendChildMovedMessage (std::shared_ptr<DataObject> child, int oldIndex, int newIndex)
{
    DataTree treeObj (shared_from_this());
    DataTree childTree (child);
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.childMoved (treeObj, childTree, oldIndex, newIndex);
    });
}

void DataTree::DataObject::sendTransactionStartedMessage()
{
    if (listeners.isEmpty())
        return;

    DataTree treeObj (shared_from_this());
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.transactionStarted (treeObj);
    });
}

void DataTree::DataObject::sendTransactionEndedMessage (DataTreeTransactionResult result)
{
    if (listeners.isEmpty())
        return;

    DataTree treeObj (shared_from_this());
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.transactionEnded (treeObj, result);
    });
}

void DataTree::DataObject::sendParentChangedMessage (std::shared_ptr<DataObject> previousParent)
{
    auto self = shared_from_this();
    DataTree treeObj (self);
    DataTree previousParentTree (std::move (previousParent));
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.parentChanged (treeObj, previousParentTree);
    });

    const auto currentChildren = children;
    for (const auto& child : currentChildren)
        child->sendAncestorChangedMessage (self);
}

void DataTree::DataObject::sendAncestorChangedMessage (const std::shared_ptr<DataObject>& reparentedAncestor)
{
    DataTree treeObj (shared_from_this());
    DataTree ancestorTree (reparentedAncestor);
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.ancestorChanged (treeObj, ancestorTree);
    });

    const auto currentChildren = children;
    for (const auto& child : currentChildren)
        child->sendAncestorChangedMessage (reparentedAncestor);
}

bool DataTree::DataObject::isDescendantOf (const DataObject& possibleAncestor) const
{
    // The slow walker ends the walk if the parents ever form a cycle
    std::shared_ptr<const DataObject> slow = shared_from_this();
    bool advanceSlow = false;

    for (auto ancestor = parent.lock(); ancestor != nullptr; ancestor = ancestor->parent.lock())
    {
        if (ancestor.get() == &possibleAncestor)
            return true;

        if (ancestor == slow)
            return false;

        if (std::exchange (advanceSlow, ! advanceSlow))
            slow = slow->parent.lock();
    }

    return false;
}

std::shared_ptr<DataTree::DataObject> DataTree::DataObject::clone() const
{
    auto newObject = std::make_shared<DataObject> (type);
    newObject->properties = properties;

    // Deep clone children
    for (const auto& child : children)
    {
        auto childClone = child->clone();
        childClone->parent = newObject;
        newObject->children.push_back (childClone);
    }

    return newObject;
}

//==============================================================================

DataTree::DataTree() noexcept = default;

DataTree::DataTree (const Identifier& type)
    : object (std::make_shared<DataObject> (type))
{
}

DataTree::DataTree (const Identifier& type,
                    const std::initializer_list<std::pair<Identifier, var>>& properties)
    : DataTree (type)
{
    auto transaction = beginTransaction();

    for (const auto& [key, value] : properties)
        transaction.setProperty (key, value);
}

DataTree::DataTree (const Identifier& type,
                    const std::initializer_list<DataTree>& children)
    : DataTree (type)
{
    auto transaction = beginTransaction();

    for (const auto& child : children)
        transaction.addChild (child);
}

DataTree::DataTree (const Identifier& type,
                    const std::initializer_list<std::pair<Identifier, var>>& properties,
                    const std::initializer_list<DataTree>& children)
    : DataTree (type)
{
    auto transaction = beginTransaction();

    for (const auto& [key, value] : properties)
        transaction.setProperty (key, value);

    for (const auto& child : children)
        transaction.addChild (child);
}

DataTree::DataTree (const DataTree& other) noexcept
    : object (other.object)
{
}

DataTree::DataTree (DataTree&& other) noexcept
    : object (std::move (other.object))
{
}

DataTree::~DataTree() = default;

DataTree& DataTree::operator= (const DataTree& other) noexcept = default;

DataTree& DataTree::operator= (DataTree&& other) noexcept = default;

DataTree::DataTree (std::shared_ptr<DataObject> objectToUse)
    : object (std::move (objectToUse))
{
}

//==============================================================================

bool DataTree::isValid() const noexcept
{
    return object != nullptr;
}

Identifier DataTree::getType() const noexcept
{
    return object != nullptr ? object->type : Identifier();
}

DataTree DataTree::clone() const
{
    if (object == nullptr)
        return {};

    return DataTree (object->clone());
}

//==============================================================================

int DataTree::getNumProperties() const noexcept
{
    return object ? object->properties.size() : 0;
}

Identifier DataTree::getPropertyName (int index) const noexcept
{
    if (! object || index < 0 || index >= object->properties.size())
        return {};

    return object->properties.getName (index);
}

bool DataTree::hasProperty (const Identifier& name) const noexcept
{
    return object && object->properties.contains (name);
}

var DataTree::getProperty (const Identifier& name, const var& defaultValue) const
{
    if (object == nullptr)
        return defaultValue;

    if (auto* value = object->properties.getVarPointer (name))
        return *value;

    return defaultValue;
}

//==============================================================================

void DataTree::setProperty (const Identifier& name, const var& newValue, UndoManager* undoManager)
{
    if (object == nullptr)
        return;

    if (auto* currentValue = object->properties.getVarPointer (name))
    {
        if (*currentValue == newValue)
            return;
    }

    if (undoManager != nullptr)
    {
        undoManager->perform (new PropertySetAction (object, name, newValue, object->properties[name]));
    }
    else
    {
        PropertySetAction (object, name, newValue, object->properties[name]).perform (UndoableActionState::Redo);
    }
}

void DataTree::removeProperty (const Identifier& name, UndoManager* undoManager)
{
    if (object == nullptr || ! object->properties.contains (name))
        return;

    if (undoManager != nullptr)
    {
        undoManager->perform (new PropertyRemoveAction (object, name, object->properties[name]));
    }
    else
    {
        PropertyRemoveAction (object, name, object->properties[name]).perform (UndoableActionState::Redo);
    }
}

void DataTree::removeAllProperties (UndoManager* undoManager)
{
    if (object == nullptr || object->properties.isEmpty())
        return;

    if (undoManager != nullptr)
    {
        undoManager->perform (new RemoveAllPropertiesAction (object));
    }
    else
    {
        RemoveAllPropertiesAction (object).perform (UndoableActionState::Redo);
    }
}

void DataTree::addChild (const DataTree& child, int index, UndoManager* undoManager)
{
    if (object == nullptr || child.object == nullptr)
        return;

    if (child.isAChildOf (*this) || child == *this || this->isAChildOf (child))
        return;

    const int numChildren = static_cast<int> (object->children.size());
    if (index < 0 || index > numChildren)
        index = numChildren;

    if (undoManager != nullptr)
    {
        undoManager->perform (new AddChildAction (object, child.object, index));
    }
    else
    {
        AddChildAction (object, child.object, index).perform (UndoableActionState::Redo);
    }
}

void DataTree::removeChild (const DataTree& child, UndoManager* undoManager)
{
    if (object == nullptr || ! child.isValid())
        return;

    if (undoManager != nullptr)
    {
        undoManager->perform (new RemoveChildAction (object, child.object, -1));
    }
    else
    {
        RemoveChildAction (object, child.object, -1).perform (UndoableActionState::Redo);
    }
}

void DataTree::removeChild (int index, UndoManager* undoManager)
{
    if (object == nullptr || ! isPositiveAndBelow (index, static_cast<int> (object->children.size())))
        return;

    if (undoManager != nullptr)
    {
        undoManager->perform (new RemoveChildAction (object, nullptr, index));
    }
    else
    {
        RemoveChildAction (object, nullptr, index).perform (UndoableActionState::Redo);
    }
}

void DataTree::removeAllChildren (UndoManager* undoManager)
{
    if (object == nullptr || object->children.empty())
        return;

    if (undoManager != nullptr)
    {
        undoManager->perform (new RemoveAllChildrenAction (object));
    }
    else
    {
        RemoveAllChildrenAction (object).perform (UndoableActionState::Redo);
    }
}

void DataTree::moveChild (int currentIndex, int newIndex, UndoManager* undoManager)
{
    if (object == nullptr || currentIndex == newIndex)
        return;

    const int numChildren = static_cast<int> (object->children.size());
    if (currentIndex < 0 || currentIndex >= numChildren || newIndex < 0 || newIndex >= numChildren)
        return;

    if (undoManager != nullptr)
    {
        undoManager->perform (new MoveChildAction (object, currentIndex, newIndex));
    }
    else
    {
        MoveChildAction (object, currentIndex, newIndex).perform (UndoableActionState::Redo);
    }
}

//==============================================================================

int DataTree::getNumChildren() const noexcept
{
    return object ? static_cast<int> (object->children.size()) : 0;
}

DataTree DataTree::getChild (int index) const noexcept
{
    if (! object || index < 0 || index >= static_cast<int> (object->children.size()))
        return {};

    return DataTree (object->children[static_cast<size_t> (index)]);
}

DataTree DataTree::getChildWithName (const Identifier& type) const noexcept
{
    if (object == nullptr)
        return {};

    for (const auto& child : object->children)
    {
        if (child->type == type)
            return DataTree (child);
    }

    return {};
}

int DataTree::indexOf (const DataTree& child) const noexcept
{
    if (object == nullptr || child.object == nullptr)
        return -1;

    for (size_t i = 0; i < object->children.size(); ++i)
    {
        if (object->children[i] == child.object)
            return static_cast<int> (i);
    }

    return -1;
}

//==============================================================================

DataTree DataTree::getParent() const noexcept
{
    if (object == nullptr)
        return {};

    auto parentObject = object->parent.lock();
    if (parentObject == nullptr)
        return {};

    return DataTree (parentObject);
}

DataTree DataTree::getRoot() const noexcept
{
    if (object == nullptr)
        return {};

    auto root = object;

    while (auto parent = root->parent.lock())
        root = std::move (parent);

    return DataTree (std::move (root));
}

bool DataTree::isAChildOf (const DataTree& possibleParent) const noexcept
{
    return object != nullptr
        && possibleParent.object != nullptr
        && object->isDescendantOf (*possibleParent.object);
}

int DataTree::getDepth() const noexcept
{
    if (object == nullptr)
        return 0;

    int depth = 0;

    for (auto parent = object->parent.lock(); parent != nullptr; parent = parent->parent.lock())
        ++depth;

    return depth;
}

//==============================================================================

std::unique_ptr<XmlElement> DataTree::createXml() const
{
    if (object == nullptr)
        return nullptr;

    auto element = std::make_unique<XmlElement> (object->type.toString());

    // Add properties as attributes
    object->properties.copyToXmlAttributes (*element);

    // Add children as child elements
    for (const auto& child : object->children)
    {
        if (auto childXml = DataTree (child).createXml())
            element->addChildElement (childXml.release());
    }

    return element;
}

DataTree DataTree::fromXml (const XmlElement& xml)
{
    return fromXml (xml, nullptr);
}

DataTree DataTree::fromXml (const XmlElement& xml, ReferenceCountedObjectPtr<DataTreeSchema> schema)
{
    DataTree tree (xml.getTagName());
    const auto nodeType = tree.getType();

    // Load properties from attributes
    for (int i = 0; i < xml.getNumAttributes(); ++i)
    {
        auto name = xml.getAttributeName (i);
        auto value = xml.getAttributeValue (i);

        tree.setProperty (name, coerceAttributeValue (nodeType, name, value, schema));
    }

    // Load children from child elements
    for (const auto* childXml : xml.getChildIterator())
    {
        auto child = fromXml (*childXml, schema);
        tree.addChild (child);
    }

    return tree;
}

void DataTree::writeToBinaryStream (OutputStream& output) const
{
    if (object == nullptr)
    {
        output.writeString (String());
        return;
    }

    output.writeString (object->type.toString());

    // Write properties
    output.writeCompressedInt (object->properties.size());
    for (int i = 0; i < object->properties.size(); ++i)
    {
        output.writeString (object->properties.getName (i).toString());
        object->properties.getValueAt (i).writeToStream (output);
    }

    // Write children
    output.writeCompressedInt (static_cast<int> (object->children.size()));
    for (const auto& child : object->children)
        DataTree (child).writeToBinaryStream (output);
}

DataTree DataTree::readTreeFromBinaryStream (InputStream& input, int nestingDepth, bool& nestedTooDeeply)
{
    constexpr int maxNestingDepth = 256;

    if (nestingDepth >= maxNestingDepth)
    {
        nestedTooDeeply = true;
        return {};
    }

    const String type = input.readString();
    if (type.isEmpty())
        return {};

    DataTree tree (type);

    // Read properties
    const int numProperties = input.readCompressedInt();
    for (int i = 0; i < numProperties; ++i)
    {
        const String name = input.readString();
        if (name.isEmpty())
            return tree;

        const var value = var::readFromStream (input);
        tree.setProperty (name, value);
    }

    // Read children
    const int numChildren = input.readCompressedInt();
    for (int i = 0; i < numChildren; ++i)
    {
        auto child = readTreeFromBinaryStream (input, nestingDepth + 1, nestedTooDeeply);
        if (! child.isValid())
            return tree;

        tree.addChild (child);
    }

    return tree;
}

DataTree DataTree::readFromBinaryStream (InputStream& input)
{
    bool nestedTooDeeply = false;
    auto tree = readTreeFromBinaryStream (input, 0, nestedTooDeeply);
    return nestedTooDeeply ? DataTree() : tree;
}

var DataTree::createJson() const
{
    if (object == nullptr)
        return var::undefined();

    auto jsonObject = std::make_unique<DynamicObject>();

    // Set type
    jsonObject->setProperty ("type", object->type.toString());

    // Create properties object
    auto propertiesObject = std::make_unique<DynamicObject>();
    for (int i = 0; i < object->properties.size(); ++i)
    {
        const auto name = object->properties.getName (i);
        const auto value = object->properties.getValueAt (i);
        propertiesObject->setProperty (name.toString(), value);
    }
    jsonObject->setProperty ("properties", propertiesObject.release());

    // Create children array
    Array<var> childrenArray;
    for (const auto& child : object->children)
    {
        var childJson = DataTree (child).createJson();
        if (! childJson.isUndefined())
            childrenArray.add (childJson);
    }
    jsonObject->setProperty ("children", childrenArray);

    return jsonObject.release();
}

DataTree DataTree::fromJson (const var& jsonData)
{
    if (! jsonData.isObject())
        return {};

    auto* jsonObject = jsonData.getDynamicObject();
    if (jsonObject == nullptr)
        return {};

    // Get type
    var typeVar = jsonObject->getProperty ("type", var());
    if (! typeVar.isString() || typeVar.toString().isEmpty())
        return {};

    DataTree tree (typeVar.toString());

    // Load properties - must be an object if present
    var properties = jsonObject->getProperty ("properties");
    if (! properties.isVoid())
    {
        if (! properties.isObject())
            return {}; // Invalid structure - properties must be object

        auto* propertiesObject = properties.getDynamicObject();
        if (propertiesObject != nullptr)
        {
            const auto& props = propertiesObject->getProperties();
            for (int i = 0; i < props.size(); ++i)
            {
                const auto& name = props.getName (i);
                const auto& value = props.getValueAt (i);
                tree.setProperty (name.toString(), value);
            }
        }
    }

    // Load children - must be an array if present
    var children = jsonObject->getProperty ("children");
    if (! children.isVoid())
    {
        if (! children.isArray())
            return {}; // Invalid structure - children must be array

        auto* childrenArray = children.getArray();
        if (childrenArray != nullptr)
        {
            for (int i = 0; i < childrenArray->size(); ++i)
            {
                auto child = fromJson (childrenArray->getReference (i));
                if (child.isValid())
                    tree.addChild (child);
            }
        }
    }

    return tree;
}

//==============================================================================

void DataTree::addListener (Listener* listener)
{
    if (object && listener)
        object->listeners.add (listener);
}

void DataTree::removeListener (Listener* listener)
{
    if (object)
        object->listeners.remove (listener);
}

void DataTree::removeAllListeners()
{
    if (object)
        object->listeners.clear();
}

//==============================================================================
bool DataTree::operator== (const DataTree& other) const noexcept
{
    return object == other.object;
}

bool DataTree::operator!= (const DataTree& other) const noexcept
{
    return object != other.object;
}

bool DataTree::isEquivalentTo (const DataTree& other) const
{
    if (! object && ! other.object)
        return true;

    if (! object || ! other.object)
        return false;

    if (object->type != other.object->type)
        return false;

    if (object->properties.size() != other.object->properties.size())
        return false;

    for (int i = 0; i < object->properties.size(); ++i)
    {
        const auto name = object->properties.getName (i);
        if (! other.hasProperty (name) || getProperty (name) != other.getProperty (name))
            return false;
    }

    if (object->children.size() != other.object->children.size())
        return false;

    for (size_t i = 0; i < object->children.size(); ++i)
    {
        if (! DataTree (object->children[i]).isEquivalentTo (DataTree (other.object->children[i])))
            return false;
    }

    return true;
}

//==============================================================================

struct DataTree::Transaction::PropertyChange
{
    enum Type
    {
        Set,
        Remove,
        RemoveAll
    };

    Type type;
    Identifier name;
    var newValue;
    var oldValue;
};

struct DataTree::Transaction::ChildChange
{
    enum Type
    {
        Add,
        Remove,
        RemoveAll,
        Move,
        AddMany,
        RemoveRange,
        RemoveMany,
        Reorder
    };

    Type type;
    DataTree child;
    int oldIndex = -1;
    int newIndex = -1;
    std::vector<std::shared_ptr<DataObject>> children;
    std::size_t numPropertyChangesBefore = 0;
};

//==============================================================================

DataTree::Transaction::Transaction (std::shared_ptr<DataObject> object, UndoManager* manager)
    : dataObject (std::move (object))
    , undoManager (manager)
{
    if (dataObject == nullptr)
    {
        active = false;
        return;
    }
}

DataTree::Transaction::Transaction (Transaction&& other) noexcept
    : dataObject (std::move (other.dataObject))
    , undoManager (other.undoManager)
    , active (std::exchange (other.active, false))
    , propertyChanges (std::move (other.propertyChanges))
    , childChanges (std::move (other.childChanges))
    , propertyIndex (std::move (other.propertyIndex))
    , hasPropertyIndex (other.hasPropertyIndex)
    , removedAllProperties (other.removedAllProperties)
    , removedAllChildren (other.removedAllChildren)
    , addedChildren (other.addedChildren)
    , childCountDelta (other.childCountDelta)
    , childCountFloor (other.childCountFloor)
{
}

DataTree::Transaction& DataTree::Transaction::operator= (Transaction&& other) noexcept
{
    if (this != &other)
    {
        // Commit current transaction if active
        if (active)
            commit();

        dataObject = std::move (other.dataObject);
        undoManager = other.undoManager;
        active = std::exchange (other.active, false);
        propertyChanges = std::move (other.propertyChanges);
        childChanges = std::move (other.childChanges);
        propertyIndex = std::move (other.propertyIndex);
        hasPropertyIndex = other.hasPropertyIndex;
        removedAllProperties = other.removedAllProperties;
        removedAllChildren = other.removedAllChildren;
        addedChildren = other.addedChildren;
        childCountDelta = other.childCountDelta;
        childCountFloor = other.childCountFloor;
    }

    return *this;
}

DataTree::Transaction::~Transaction()
{
    if (active)
        commit();
}

void DataTree::Transaction::commit()
{
    if (! active || dataObject == nullptr)
        return;

    CompoundAction::NodeEdits edits { dataObject.get(), takeActions() };

    if (edits.actions.empty())
        return;

    if (undoManager != nullptr)
        undoManager->perform (new CompoundAction (std::move (edits)));
    else
        CompoundAction::apply ({ &edits, 1 }, UndoableActionState::Redo);
}

std::vector<UndoableAction::Ptr> DataTree::Transaction::takeActions()
{
    std::vector<UndoableAction::Ptr> actions;
    std::size_t nextProperty = 0;

    // Property and child actions are created in the order their edits were made
    const auto addPropertyActionsUpTo = [&] (std::size_t end)
    {
        for (; nextProperty < end; ++nextProperty)
        {
            const auto& change = propertyChanges[nextProperty];

            switch (change.type)
            {
                case PropertyChange::Set:
                {
                    actions.push_back (new PropertySetAction (dataObject, change.name, change.newValue, change.oldValue));
                    break;
                }

                case PropertyChange::Remove:
                {
                    actions.push_back (new PropertyRemoveAction (dataObject, change.name, change.oldValue));
                    break;
                }

                case PropertyChange::RemoveAll:
                {
                    if (nextProperty + 1 >= end)
                    {
                        actions.push_back (new RemoveAllPropertiesAction (dataObject));
                        break;
                    }

                    // The property changes made right after it replace the properties together with it
                    NamedValueSet replacement;

                    for (std::size_t j = nextProperty + 1; j < end; ++j)
                    {
                        const auto& subsequent = propertyChanges[j];

                        if (subsequent.type == PropertyChange::Set)
                            replacement.set (subsequent.name, subsequent.newValue);
                        else if (subsequent.type == PropertyChange::Remove)
                            replacement.remove (subsequent.name);
                        else if (subsequent.type == PropertyChange::RemoveAll)
                            replacement.clear();
                    }

                    actions.push_back (new ReplacePropertiesAction (dataObject, std::move (replacement)));
                    nextProperty = end;
                    return;
                }
            }
        }
    };

    for (const auto& change : childChanges)
    {
        addPropertyActionsUpTo (change.numPropertyChangesBefore);

        switch (change.type)
        {
            case ChildChange::Add:
            {
                actions.push_back (new AddChildAction (dataObject, change.child.object, change.newIndex));
                break;
            }

            case ChildChange::Remove:
            {
                actions.push_back (new RemoveChildAction (dataObject, change.child.object, change.oldIndex));
                break;
            }

            case ChildChange::RemoveAll:
            {
                actions.push_back (new RemoveAllChildrenAction (dataObject));
                break;
            }

            case ChildChange::Move:
            {
                actions.push_back (new MoveChildAction (dataObject, change.oldIndex, change.newIndex));
                break;
            }

            case ChildChange::AddMany:
            {
                actions.push_back (new AddChildrenAction (dataObject, change.children, change.newIndex));
                break;
            }

            case ChildChange::RemoveRange:
            {
                actions.push_back (new RemoveChildrenAction (dataObject, change.oldIndex, change.newIndex));
                break;
            }

            case ChildChange::RemoveMany:
            {
                actions.push_back (new RemoveChildrenAction (dataObject, change.children));
                break;
            }

            case ChildChange::Reorder:
            {
                actions.push_back (new ReorderChildrenAction (dataObject, change.children));
                break;
            }
        }
    }

    addPropertyActionsUpTo (propertyChanges.size());

    active = false;
    return actions;
}

void DataTree::Transaction::abort()
{
    if (! active)
        return;

    discard();

    // Nothing was applied, the listeners are only told how it ended
    dataObject->sendTransactionStartedMessage();
    dataObject->sendTransactionEndedMessage (TransactionResult::aborted);
}

void DataTree::Transaction::discard()
{
    active = false;
    propertyChanges.clear();
    childChanges.clear();
    propertyIndex.clear();
}

DataTree::Transaction::PropertyState DataTree::Transaction::findPropertyState (const Identifier& name)
{
    // A short list is scanned, a long one is indexed by name the first time it is looked up
    constexpr std::size_t indexedFrom = 16;

    if (! hasPropertyIndex && propertyChanges.size() >= indexedFrom)
    {
        hasPropertyIndex = true;

        for (std::size_t i = 0; i < propertyChanges.size(); ++i)
            indexPropertyChange (i);
    }

    PropertyState state;

    if (hasPropertyIndex)
    {
        state.reset = removedAllProperties;

        if (const auto it = propertyIndex.find (name); it != propertyIndex.end())
        {
            state.pendingSet = it->second;
            state.reset = state.reset || it->second < 0;
        }

        return state;
    }

    for (std::size_t i = 0; i < propertyChanges.size(); ++i)
    {
        const auto& change = propertyChanges[i];

        if (change.type == PropertyChange::RemoveAll)
            state = { -1, true };
        else if (change.name != name)
            continue;
        else if (change.type == PropertyChange::Remove)
            state = { -1, true };
        else
            state.pendingSet = static_cast<int> (i);
    }

    return state;
}

void DataTree::Transaction::indexPropertyChange (std::size_t index)
{
    const auto& change = propertyChanges[index];

    if (change.type == PropertyChange::RemoveAll)
    {
        propertyIndex.clear();
        removedAllProperties = true;
    }
    else
    {
        propertyIndex[change.name] = change.type == PropertyChange::Set ? static_cast<int> (index) : -1;
    }
}

bool DataTree::Transaction::hasEffectiveProperties()
{
    for (int i = 0; i < dataObject->properties.size(); ++i)
    {
        const auto state = findPropertyState (dataObject->properties.getName (i));

        if (state.pendingSet >= 0 || ! state.reset)
            return true;
    }

    for (const auto& change : propertyChanges)
    {
        if (change.type == PropertyChange::Set && findPropertyState (change.name).pendingSet >= 0)
            return true;
    }

    return false;
}

void DataTree::Transaction::setProperty (const Identifier& name, const var& newValue)
{
    if (! active || dataObject == nullptr)
        return;

    const auto state = findPropertyState (name);

    if (state.pendingSet >= 0)
    {
        propertyChanges[static_cast<std::size_t> (state.pendingSet)].newValue = newValue;
        return;
    }

    const var* effectiveValue = state.reset ? nullptr : dataObject->properties.getVarPointer (name);

    if (effectiveValue != nullptr && effectiveValue->equalsWithSameType (newValue))
        return;

    PropertyChange change;
    change.type = PropertyChange::Set;
    change.name = name;
    change.newValue = newValue;
    change.oldValue = effectiveValue != nullptr ? *effectiveValue : var();
    propertyChanges.push_back (change);

    if (hasPropertyIndex)
        indexPropertyChange (propertyChanges.size() - 1);
}

void DataTree::Transaction::removeProperty (const Identifier& name)
{
    if (! active || dataObject == nullptr)
        return;

    const auto state = findPropertyState (name);

    const var* effectiveValue = state.pendingSet >= 0 ? &propertyChanges[static_cast<std::size_t> (state.pendingSet)].newValue
                              : state.reset           ? nullptr
                                                      : dataObject->properties.getVarPointer (name);

    if (effectiveValue == nullptr)
        return;

    PropertyChange change;
    change.type = PropertyChange::Remove;
    change.name = name;
    change.oldValue = *effectiveValue;
    propertyChanges.push_back (change);

    if (hasPropertyIndex)
        indexPropertyChange (propertyChanges.size() - 1);
}

void DataTree::Transaction::removeAllProperties()
{
    if (! active || dataObject == nullptr || ! hasEffectiveProperties())
        return;

    PropertyChange change;
    change.type = PropertyChange::RemoveAll;
    propertyChanges.push_back (change);

    if (hasPropertyIndex)
        indexPropertyChange (propertyChanges.size() - 1);
}

int DataTree::Transaction::getCommittedChildCount() const
{
    return removedAllChildren ? 0 : static_cast<int> (dataObject->children.size());
}

void DataTree::Transaction::addChild (const DataTree& child, int index)
{
    if (! active || dataObject == nullptr || child.object == nullptr)
        return;

    // Don't add invalid or self-referencing children
    DataTree thisTree (dataObject);
    if (child.isAChildOf (thisTree) || child.object == dataObject || thisTree.isAChildOf (child))
        return;

    // The index is resolved when the edit is applied, as only then is the number of children known
    ChildChange change;
    change.type = ChildChange::Add;
    change.child = child;
    change.newIndex = index;
    change.oldIndex = -1;
    recordChildChange (std::move (change));

    addedChildren = true;
    ++childCountDelta;
    ++childCountFloor;
}

void DataTree::Transaction::removeChild (const DataTree& child)
{
    if (! active || dataObject == nullptr)
        return;

    ChildChange change;
    change.type = ChildChange::Remove;
    change.child = child;
    change.oldIndex = -1;
    change.newIndex = -1;
    recordChildChange (std::move (change));

    noteChildrenRemoved (1);
}

void DataTree::Transaction::removeChild (int index)
{
    if (! active || dataObject == nullptr)
        return;

    ChildChange change;
    change.type = ChildChange::Remove;
    change.child = DataTree();
    change.oldIndex = index;
    change.newIndex = -1;
    recordChildChange (std::move (change));

    noteChildrenRemoved (1);
}

void DataTree::Transaction::removeAllChildren()
{
    if (! active || dataObject == nullptr)
        return;

    // Nothing to remove, unless this transaction added children
    if (dataObject->children.empty() && ! addedChildren)
        return;

    // Record the change
    ChildChange change;
    change.type = ChildChange::RemoveAll;
    recordChildChange (std::move (change));

    removedAllChildren = true;
    addedChildren = false;
    childCountDelta = 0;
    childCountFloor = 0;
}

void DataTree::Transaction::moveChild (int currentIndex, int newIndex)
{
    if (! active || dataObject == nullptr || currentIndex == newIndex)
        return;

    // Simply record the move operation as specified by the user
    ChildChange change;
    change.type = ChildChange::Move;
    change.child = DataTree(); // Will be resolved during commit
    change.oldIndex = currentIndex;
    change.newIndex = newIndex;
    recordChildChange (std::move (change));
}

void DataTree::Transaction::addChildren (Span<const DataTree> children, int index)
{
    if (! active || dataObject == nullptr)
        return;

    ChildChange change;
    change.type = ChildChange::AddMany;

    std::unordered_set<const DataObject*> listed;

    for (const auto& child : children)
    {
        // The trees addChild() ignores, and a tree counts once
        if (child.object == nullptr
            || child.object == dataObject
            || child.object->isDescendantOf (*dataObject)
            || dataObject->isDescendantOf (*child.object)
            || ! listed.insert (child.object.get()).second)
            continue;

        change.children.push_back (child.object);
    }

    if (change.children.empty())
        return;

    const int numAdded = static_cast<int> (change.children.size());

    change.newIndex = index;
    recordChildChange (std::move (change));

    addedChildren = true;
    childCountDelta += numAdded;
    childCountFloor += numAdded;
}

void DataTree::Transaction::removeChildren (int startIndex, int numChildren)
{
    if (! active || dataObject == nullptr || numChildren <= 0)
        return;

    ChildChange change;
    change.type = ChildChange::RemoveRange;
    change.oldIndex = startIndex;
    change.newIndex = numChildren;
    recordChildChange (std::move (change));

    noteChildrenRemoved (numChildren);
}

void DataTree::Transaction::removeChildren (Span<const DataTree> children)
{
    if (! active || dataObject == nullptr || children.empty())
        return;

    ChildChange change;
    change.type = ChildChange::RemoveMany;

    for (const auto& child : children)
    {
        if (child.object != nullptr)
            change.children.push_back (child.object);
    }

    recordChildChange (std::move (change));

    // Every listed tree counts, as it does for removeChild()
    noteChildrenRemoved (static_cast<int> (jmin (children.size(), static_cast<std::size_t> (std::numeric_limits<int>::max()))));
}

void DataTree::Transaction::reorderChildren (Span<const DataTree> newOrder)
{
    if (! active || dataObject == nullptr)
        return;

    ChildChange change;
    change.type = ChildChange::Reorder;

    for (const auto& child : newOrder)
        change.children.push_back (child.object);

    recordChildChange (std::move (change));
}

void DataTree::Transaction::recordChildChange (ChildChange&& change)
{
    change.numPropertyChangesBefore = propertyChanges.size();
    childChanges.push_back (std::move (change));
}

void DataTree::Transaction::noteChildrenRemoved (int numRemoved)
{
    childCountDelta = static_cast<int> (jmax (static_cast<int64> (childCountDelta) - numRemoved, static_cast<int64> (std::numeric_limits<int>::min())));
    childCountFloor = jmax (0, childCountFloor - numRemoved);
}

int DataTree::Transaction::getEffectiveChildCount() const
{
    if (dataObject == nullptr)
        return 0;

    // A removal counts whether or not it will find a child to remove, but never below the children added after it
    return jmax (getCommittedChildCount() + childCountDelta, childCountFloor);
}

//==============================================================================

DataTree::BatchTransaction::BatchTransaction (UndoManager* manager)
    : undoManager (manager)
{
}

DataTree::BatchTransaction::~BatchTransaction()
{
    if (active)
        commit();
}

DataTree::Transaction& DataTree::BatchTransaction::edit (const DataTree& node)
{
    jassert (active); // The batch was already committed or aborted

    // Edits of an invalid node, or made too late, go to a transaction that ignores them
    const auto object = active ? node.object : nullptr;
    const auto [it, isNew] = transactionIndices.try_emplace (object.get(), transactions.size());

    // A transaction that was committed or aborted on its own makes room for a new one
    if (! isNew && object != nullptr && ! transactions[it->second]->isActive())
        it->second = transactions.size();

    if (it->second == transactions.size())
        transactions.push_back (std::unique_ptr<Transaction> (new Transaction (object)));

    return *transactions[it->second];
}

void DataTree::BatchTransaction::commit()
{
    if (! active)
        return;

    active = false;

    std::vector<CompoundAction::NodeEdits> edits;

    for (const auto& transaction : transactions)
    {
        if (! transaction->isActive())
            continue;

        auto actions = transaction->takeActions();

        if (! actions.empty())
            edits.push_back ({ transaction->dataObject.get(), std::move (actions) });
    }

    if (edits.empty())
        return;

    if (undoManager != nullptr)
        undoManager->perform (new CompoundAction (std::move (edits)));
    else
        CompoundAction::apply (edits, UndoableActionState::Redo);
}

void DataTree::BatchTransaction::abort()
{
    if (! active)
        return;

    active = false;

    std::vector<std::shared_ptr<DataObject>> nodes;

    for (const auto& transaction : transactions)
    {
        if (! transaction->isActive())
            continue;

        transaction->discard();
        nodes.push_back (transaction->dataObject);
    }

    // Told as one transaction, the way a commit of the same nodes would be
    for (const auto& node : nodes)
        node->sendTransactionStartedMessage();

    for (auto node = nodes.rbegin(); node != nodes.rend(); ++node)
        (*node)->sendTransactionEndedMessage (TransactionResult::aborted);
}

//==============================================================================

DataTree::ValidatedTransaction::ValidatedTransaction (std::shared_ptr<DataObject> object, ReferenceCountedObjectPtr<DataTreeSchema> schema, UndoManager* undoManager)
    : transaction (std::make_unique<Transaction> (DataTree (object).beginTransaction (undoManager)))
    , schema (std::move (schema))
    , nodeType (DataTree (object).getType())
{
}

DataTree::ValidatedTransaction::ValidatedTransaction (ValidatedTransaction&& other) noexcept
    : transaction (std::move (other.transaction))
    , schema (std::move (other.schema))
    , nodeType (other.nodeType)
    , hasValidationErrors (other.hasValidationErrors)
{
}

DataTree::ValidatedTransaction& DataTree::ValidatedTransaction::operator= (ValidatedTransaction&& other) noexcept
{
    if (this != &other)
    {
        transaction = std::move (other.transaction);
        schema = std::move (other.schema);
        nodeType = other.nodeType;
        hasValidationErrors = other.hasValidationErrors;
    }

    return *this;
}

DataTree::ValidatedTransaction::~ValidatedTransaction()
{
    // Only auto-commit if there were no validation errors
    if (transaction && transaction->isActive() && ! hasValidationErrors)
        transaction->commit();
    else if (transaction && transaction->isActive())
        transaction->abort();
}

Result DataTree::ValidatedTransaction::setProperty (const Identifier& name, const var& newValue)
{
    if (! transaction || ! transaction->isActive() || ! schema)
        return Result::fail ("Transaction is not active");

    auto validationResult = schema->validatePropertyValue (nodeType, name, newValue);
    if (validationResult.failed())
    {
        hasValidationErrors = true;
        return validationResult;
    }

    transaction->setProperty (name, newValue);
    return Result::ok();
}

Result DataTree::ValidatedTransaction::removeProperty (const Identifier& name)
{
    if (! transaction || ! transaction->isActive() || ! schema)
        return Result::fail ("Transaction is not active");

    // Check if property is required
    auto propInfo = schema->getPropertyInfo (nodeType, name);
    if (propInfo.required)
    {
        hasValidationErrors = true;
        return Result::fail ("Cannot remove required property '" + name.toString() + "'");
    }

    transaction->removeProperty (name);
    return Result::ok();
}

Result DataTree::ValidatedTransaction::addChild (const DataTree& child, int index)
{
    if (! transaction || ! transaction->isActive() || ! schema)
        return Result::fail ("Transaction is not active");

    if (! child.isValid())
        return Result::fail ("Cannot add invalid child");

    const int effectiveChildCount = transaction->getEffectiveChildCount();
    auto validationResult = schema->validateChildAddition (nodeType, child.getType(), effectiveChildCount);
    if (validationResult.failed())
    {
        hasValidationErrors = true;
        return validationResult;
    }

    transaction->addChild (child, index);
    return Result::ok();
}

ResultValue<DataTree> DataTree::ValidatedTransaction::createAndAddChild (const Identifier& childType, int index)
{
    if (! transaction || ! transaction->isActive() || ! schema)
        return makeResultValueFail ("Transaction is not active");

    DataTree child = schema->createChildNode (nodeType, childType);
    if (! child.isValid())
        return makeResultValueFail ("Could not create child of type '" + childType.toString() + "'");

    auto addResult = addChild (child, index);
    if (addResult.failed())
        return makeResultValueFail (addResult.getErrorMessage());

    return makeResultValueOk (child);
}

Result DataTree::ValidatedTransaction::removeChild (const DataTree& child)
{
    if (! transaction || ! transaction->isActive() || ! schema)
        return Result::fail ("Transaction is not active");

    if (! schema->hasNodeType (nodeType))
        return Result::fail ("Unknown node type: " + nodeType.toString());

    const auto constraints = schema->getChildConstraints (nodeType);
    const int currentCount = transaction->getEffectiveChildCount();
    const int resultingCount = std::max (0, currentCount - 1);

    if (resultingCount < constraints.minCount)
    {
        hasValidationErrors = true;
        return Result::fail ("Cannot remove child: would violate minimum child count (" + String (constraints.minCount) + ")");
    }

    transaction->removeChild (child);
    return Result::ok();
}

Result DataTree::ValidatedTransaction::commit()
{
    if (! transaction || ! transaction->isActive())
        return Result::fail ("Transaction is not active");

    if (hasValidationErrors)
        return Result::fail ("Cannot commit transaction with validation errors");

    transaction->commit();
    return Result::ok();
}

void DataTree::ValidatedTransaction::abort()
{
    if (transaction && transaction->isActive())
    {
        transaction->abort();

        hasValidationErrors = false; // Reset error state
    }
}

bool DataTree::ValidatedTransaction::isActive() const
{
    return transaction && transaction->isActive();
}

DataTree::Transaction& DataTree::ValidatedTransaction::getTransaction()
{
    jassert (transaction != nullptr);
    return *transaction;
}

} // namespace yup
