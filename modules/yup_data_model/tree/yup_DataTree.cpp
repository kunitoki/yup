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
    RemoveAllPropertiesAction (std::shared_ptr<DataTree::DataObject> obj, const NamedValueSet& oldProps)
        : dataObject (std::move (obj))
        , oldProperties (oldProps)
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
            dataObject->properties.clear();
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
            if (auto currentParent = childObject->parent.lock())
            {
                previousParent = currentParent;

                // Find child index in current parent
                auto& currentChildren = currentParent->children;
                auto it = std::find (currentChildren.begin(), currentChildren.end(), childObject);
                if (it != currentChildren.end())
                {
                    previousIndex = static_cast<int> (std::distance (currentChildren.begin(), it));
                    currentChildren.erase (it);
                    currentParent->sendChildRemovedMessage (childObject, previousIndex);
                }
            }
            else
            {
                previousParent.reset();
                previousIndex = -1;
            }

            const int numChildren = static_cast<int> (parentObject->children.size());
            const int actualIndex = isPositiveAndBelow (index, numChildren) ? index : numChildren;

            parentObject->children.insert (parentObject->children.begin() + actualIndex, childObject);
            childObject->parent = parentObject;
            parentObject->sendChildAddedMessage (childObject);
        }
        else
        {
            // Find child in parent
            auto& parentChildren = parentObject->children;
            auto it = std::find (parentChildren.begin(), parentChildren.end(), childObject);
            if (it != parentChildren.end())
            {
                const int childIndex = static_cast<int> (std::distance (parentChildren.begin(), it));
                parentChildren.erase (it);
                parentObject->sendChildRemovedMessage (childObject, childIndex);

                if (auto prevParent = previousParent.lock())
                {
                    const int numChildren = static_cast<int> (prevParent->children.size());
                    const int actualIndex = (previousIndex < 0 || previousIndex > numChildren) ? numChildren : previousIndex;

                    prevParent->children.insert (prevParent->children.begin() + actualIndex, childObject);
                    childObject->parent = prevParent;
                    prevParent->sendChildAddedMessage (childObject);
                }
                else
                {
                    childObject->parent.reset();
                }
            }
        }

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> parentObject;
    std::shared_ptr<DataTree::DataObject> childObject;
    int index;
    std::weak_ptr<DataTree::DataObject> previousParent;
    int previousIndex = -1;
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
            if (childObject != nullptr)
            {
                auto it = std::find (parentChildren.begin(), parentChildren.end(), childObject);
                if (it != parentChildren.end())
                    index = static_cast<int> (std::distance (parentChildren.begin(), it));
            }

            if (! isPositiveAndBelow (index, static_cast<int> (parentObject->children.size())))
                return false;

            if (childObject == nullptr)
                childObject = parentChildren[index];

            parentChildren.erase (parentChildren.begin() + index);
            childObject->parent.reset();
            parentObject->sendChildRemovedMessage (childObject, index);
        }
        else
        {
            if (childObject == nullptr)
                return false;

            const int numChildren = static_cast<int> (parentChildren.size());
            const int actualIndex = isPositiveAndBelow (index, numChildren) ? index : numChildren;

            parentChildren.insert (parentChildren.begin() + actualIndex, childObject);
            childObject->parent = parentObject;
            parentObject->sendChildAddedMessage (childObject);
        }

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> parentObject;
    std::shared_ptr<DataTree::DataObject> childObject;
    int index;
};

//==============================================================================

class RemoveAllChildrenAction : public UndoableAction
{
public:
    RemoveAllChildrenAction (std::shared_ptr<DataTree::DataObject> parent, const std::vector<std::shared_ptr<DataTree::DataObject>>& oldChildren)
        : parentObject (std::move (parent))
        , children (oldChildren)
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
            parentObject->children.clear();

            for (size_t i = 0; i < children.size(); ++i)
            {
                children[i]->parent.reset();
                parentObject->sendChildRemovedMessage (children[i], static_cast<int> (i));
            }
        }
        else
        {
            parentObject->children = children;

            for (auto& child : children)
            {
                child->parent = parentObject;
                parentObject->sendChildAddedMessage (child);
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

class CompoundAction : public UndoableAction
{
public:
    CompoundAction (std::shared_ptr<DataTree::DataObject> obj, std::vector<UndoableAction::Ptr>&& actions)
        : dataObject (std::move (obj))
        , individualActions (std::move (actions))
    {
    }

    bool isValid() const override
    {
        return dataObject != nullptr && ! individualActions.empty();
    }

    bool perform (UndoableActionState state) override
    {
        if (dataObject == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            for (auto& action : individualActions)
                action->perform (UndoableActionState::Redo);
        }
        else
        {
            for (auto it = individualActions.rbegin(); it != individualActions.rend(); ++it)
                (*it)->perform (UndoableActionState::Undo);
        }

        return true;
    }

private:
    std::shared_ptr<DataTree::DataObject> dataObject;
    std::vector<UndoableAction::Ptr> individualActions;
};

//==============================================================================

DataTree::DataObject::DataObject (const Identifier& treeType)
    : type (treeType)
{
}

DataTree::DataObject::~DataObject() = default;

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

DataTree& DataTree::operator= (const DataTree& other) noexcept
{
    if (this != &other)
    {
        if (auto oldObject = std::exchange (object, other.object))
        {
            oldObject->listeners.call ([this] (Listener& l)
            {
                l.treeRedirected (*this);
            });
        }
    }

    return *this;
}

DataTree& DataTree::operator= (DataTree&& other) noexcept
{
    if (this != &other)
    {
        if (auto oldObject = std::exchange (object, other.object))
        {
            oldObject->listeners.call ([this] (Listener& l)
            {
                l.treeRedirected (*this);
            });
        }
    }

    return *this;
}

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
        undoManager->perform (new RemoveAllPropertiesAction (object, object->properties));
    }
    else
    {
        RemoveAllPropertiesAction (object, object->properties).perform (UndoableActionState::Redo);
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
        undoManager->perform (new RemoveAllChildrenAction (object, object->children));
    }
    else
    {
        RemoveAllChildrenAction (object, object->children).perform (UndoableActionState::Redo);
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

    DataTree root = *this;
    while (auto parent = root.getParent())
    {
        if (! parent.isValid())
            break;

        root = parent;
    }

    return root;
}

bool DataTree::isAChildOf (const DataTree& possibleParent) const noexcept
{
    if (object == nullptr || possibleParent.object == nullptr)
        return false;

    std::unordered_set<const void*> visited;

    auto parent = getParent();
    while (parent.isValid())
    {
        const void* parentPtr = parent.object.get();
        if (visited.find (parentPtr) != visited.end())
            return false;

        visited.insert (parentPtr);

        if (parent == possibleParent)
            return true;

        parent = parent.getParent();
    }

    return false;
}

int DataTree::getDepth() const noexcept
{
    if (object == nullptr)
        return 0;

    int depth = 0;
    auto parent = getParent();
    while (parent.isValid())
    {
        ++depth;
        parent = parent.getParent();
    }

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

DataTree DataTree::readFromBinaryStream (InputStream& input)
{
    const String type = input.readString();
    if (type.isEmpty())
        return {};

    DataTree tree (type);

    // Read properties
    const int numProperties = input.readCompressedInt();
    for (int i = 0; i < numProperties; ++i)
    {
        const String name = input.readString();
        const var value = var::readFromStream (input);
        tree.setProperty (name, value);
    }

    // Read children
    const int numChildren = input.readCompressedInt();
    for (int i = 0; i < numChildren; ++i)
    {
        auto child = readFromBinaryStream (input);
        if (child.isValid())
            tree.addChild (child);
    }

    return tree;
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
        Move
    };

    Type type;
    DataTree child;
    int oldIndex = -1;
    int newIndex = -1;
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

    // Always build individual actions and execute them
    std::vector<UndoableAction::Ptr> actions;

    // Create property actions that capture current state
    for (const auto& change : propertyChanges)
    {
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
                actions.push_back (new RemoveAllPropertiesAction (dataObject, dataObject->properties));
                break;
            }
        }
    }

    // Create child actions that capture current state
    for (const auto& change : childChanges)
    {
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
                actions.push_back (new RemoveAllChildrenAction (dataObject, dataObject->children));
                break;
            }

            case ChildChange::Move:
            {
                actions.push_back (new MoveChildAction (dataObject, change.oldIndex, change.newIndex));
                break;
            }
        }
    }

    // If we have undo manager, use compound action for undo/redo
    if (undoManager != nullptr && ! actions.empty())
    {
        undoManager->perform (new CompoundAction (dataObject, std::move (actions)));
    }
    else
    {
        for (auto& action : actions)
            action->perform (UndoableActionState::Redo);
    }

    active = false;
}

void DataTree::Transaction::abort()
{
    if (! active)
        return;

    // Simply mark as inactive - changes are not applied
    active = false;
    propertyChanges.clear();
    childChanges.clear();
}

void DataTree::Transaction::setProperty (const Identifier& name, const var& newValue)
{
    if (! active || dataObject == nullptr)
        return;

    // Check if we already have a change for this property
    for (auto& change : propertyChanges)
    {
        if (change.name == name && change.type == PropertyChange::Set)
        {
            change.newValue = newValue;
            return;
        }
    }

    // Get current value for undo purposes
    var oldValue;
    if (auto* value = dataObject->properties.getVarPointer (name))
        oldValue = *value;

    // Skip if no change
    if (oldValue == newValue)
        return;

    // Record the change
    PropertyChange change;
    change.type = PropertyChange::Set;
    change.name = name;
    change.newValue = newValue;
    change.oldValue = oldValue;
    propertyChanges.push_back (change);
}

void DataTree::Transaction::removeProperty (const Identifier& name)
{
    if (! active || dataObject == nullptr)
        return;

    if (! dataObject->properties.contains (name))
        return;

    // Record the change
    PropertyChange change;
    change.type = PropertyChange::Remove;
    change.name = name;
    change.oldValue = dataObject->properties[name];
    propertyChanges.push_back (change);
}

void DataTree::Transaction::removeAllProperties()
{
    if (! active || dataObject == nullptr)
        return;

    if (dataObject->properties.size() == 0)
        return;

    // Record the change
    PropertyChange change;
    change.type = PropertyChange::RemoveAll;
    propertyChanges.push_back (change);
}

void DataTree::Transaction::addChild (const DataTree& child, int index)
{
    if (! active || dataObject == nullptr || child.object == nullptr)
        return;

    // Don't add invalid or self-referencing children
    DataTree thisTree (dataObject);
    if (child.isAChildOf (thisTree) || child.object == dataObject || thisTree.isAChildOf (child))
        return;

    // Calculate effective number of children including pending additions
    int effectiveNumChildren = static_cast<int> (dataObject->children.size());
    for (const auto& change : childChanges)
    {
        if (change.type == ChildChange::Add)
            ++effectiveNumChildren;
        else if (change.type == ChildChange::Remove)
            --effectiveNumChildren;
        else if (change.type == ChildChange::RemoveAll)
            effectiveNumChildren = 0;
    }

    if (index < 0 || index > effectiveNumChildren)
        index = effectiveNumChildren;

    ChildChange change;
    change.type = ChildChange::Add;
    change.child = child;
    change.newIndex = index;
    change.oldIndex = -1;
    childChanges.push_back (change);
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
    childChanges.push_back (change);
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
    childChanges.push_back (change);
}

void DataTree::Transaction::removeAllChildren()
{
    if (! active || dataObject == nullptr)
        return;

    if (dataObject->children.size() == 0)
        return;

    // Record the change
    ChildChange change;
    change.type = ChildChange::RemoveAll;
    childChanges.push_back (change);
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
    childChanges.push_back (change);
}

int DataTree::Transaction::getEffectiveChildCount() const
{
    if (dataObject == nullptr)
        return 0;

    int count = static_cast<int> (dataObject->children.size());

    for (const auto& change : childChanges)
    {
        switch (change.type)
        {
            case ChildChange::Add:
                ++count;
                break;

            case ChildChange::Remove:
                if (count > 0)
                    --count;
                break;

            case ChildChange::RemoveAll:
                count = 0;
                break;

            case ChildChange::Move:
                break; // No change in count
        }
    }

    return std::max (0, count);
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
        return ResultValue<DataTree>::fail ("Transaction is not active");

    DataTree child = schema->createChildNode (nodeType, childType);
    if (! child.isValid())
        return ResultValue<DataTree>::fail ("Could not create child of type '" + childType.toString() + "'");

    auto addResult = addChild (child, index);
    if (addResult.failed())
        return ResultValue<DataTree>::fail (addResult.getErrorMessage());

    return ResultValue<DataTree>::ok (child);
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
