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

class PropertySetAction : public UndoableAction
{
public:
    PropertySetAction (DataTree tree, const Identifier& prop, const var& newVal, const var& oldVal)
        : dataTree (tree)
        , property (prop)
        , newValue (newVal)
        , oldValue (oldVal)
    {
    }

    bool isValid() const override
    {
        return dataTree.object != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (dataTree.object == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            dataTree.object->properties.set (property, newValue);
        }
        else // Undo
        {
            dataTree.object->properties.set (property, oldValue);
        }

        dataTree.object->sendPropertyChangeMessage (property);
        return true;
    }

private:
    DataTree dataTree;
    Identifier property;
    var newValue, oldValue;
};

class PropertyRemoveAction : public UndoableAction
{
public:
    PropertyRemoveAction (DataTree tree, const Identifier& prop, const var& oldVal)
        : dataTree (tree)
        , property (prop)
        , oldValue (oldVal)
    {
    }

    bool isValid() const override
    {
        return dataTree.object != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (dataTree.object == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            dataTree.object->properties.remove (property);
        }
        else // Undo
        {
            dataTree.object->properties.set (property, oldValue);
        }

        dataTree.object->sendPropertyChangeMessage (property);
        return true;
    }

private:
    DataTree dataTree;
    Identifier property;
    var oldValue;
};

class RemoveAllPropertiesAction : public UndoableAction
{
public:
    RemoveAllPropertiesAction (DataTree tree, const NamedValueSet& oldProps)
        : dataTree (tree)
        , oldProperties (oldProps)
    {
    }

    bool isValid() const override
    {
        return dataTree.object != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (dataTree.object == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            dataTree.object->properties.clear();
        }
        else // Undo
        {
            dataTree.object->properties = oldProperties;
        }

        for (int i = 0; i < oldProperties.size(); ++i)
            dataTree.object->sendPropertyChangeMessage (oldProperties.getName (i));

        return true;
    }

private:
    DataTree dataTree;
    NamedValueSet oldProperties;
};

class AddChildAction : public UndoableAction
{
public:
    AddChildAction (DataTree parent, const DataTree& child, int idx)
        : parentTree (parent)
        , childTree (child)
        , index (idx)
    {
    }

    bool isValid() const override
    {
        return parentTree.object != nullptr && childTree.object != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentTree.object == nullptr || childTree.object == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            // Capture the child's current parent (if any) for undo
            if (auto currentParent = childTree.object->parent.lock())
            {
                previousParent = DataTree (currentParent);
                previousIndex = previousParent.indexOf (childTree);
                previousParent.removeChild (childTree);
            }
            else
            {
                previousParent = DataTree(); // No previous parent
                previousIndex = -1;
            }

            const int numChildren = static_cast<int> (parentTree.object->children.size());
            const int actualIndex = (index < 0 || index > numChildren) ? numChildren : index;

            parentTree.object->children.insert (parentTree.object->children.begin() + actualIndex, childTree);
            childTree.object->parent = parentTree.object;

            parentTree.object->sendChildAddedMessage (childTree);
        }
        else // Undo
        {
            const int childIndex = parentTree.indexOf (childTree);
            if (childIndex >= 0)
            {
                parentTree.object->children.erase (parentTree.object->children.begin() + childIndex);
                parentTree.object->sendChildRemovedMessage (childTree, childIndex);
                
                // Restore previous parent
                if (previousParent.isValid())
                {
                    // Restore to previous parent at previous index
                    const int numChildren = static_cast<int> (previousParent.object->children.size());
                    const int actualIndex = (previousIndex < 0 || previousIndex > numChildren) ? numChildren : previousIndex;
                    
                    previousParent.object->children.insert (previousParent.object->children.begin() + actualIndex, childTree);
                    childTree.object->parent = previousParent.object;
                    previousParent.object->sendChildAddedMessage (childTree);
                }
                else
                {
                    // No previous parent - clear parent reference
                    childTree.object->parent.reset();
                }
            }
        }

        return true;
    }

private:
    DataTree parentTree;
    DataTree childTree;
    int index;
    DataTree previousParent; // For undo: the child's parent before this action
    int previousIndex = -1;  // For undo: the child's index in previous parent
};

class RemoveChildAction : public UndoableAction
{
public:
    RemoveChildAction (DataTree parent, const DataTree& child, int idx)
        : parentTree (parent)
        , childTree (child)
        , index (idx)
    {
    }

    bool isValid() const override
    {
        return parentTree.object != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentTree.object == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            if (index < 0 || index >= static_cast<int> (parentTree.object->children.size()))
                return false;

            parentTree.object->children.erase (parentTree.object->children.begin() + index);
            childTree.object->parent.reset();
            parentTree.object->sendChildRemovedMessage (childTree, index);
        }
        else // Undo
        {
            if (childTree.object == nullptr)
                return false;

            const int numChildren = static_cast<int> (parentTree.object->children.size());
            const int actualIndex = (index < 0 || index > numChildren) ? numChildren : index;

            parentTree.object->children.insert (parentTree.object->children.begin() + actualIndex, childTree);
            childTree.object->parent = parentTree.object;
            parentTree.object->sendChildAddedMessage (childTree);
        }

        return true;
    }

private:
    DataTree parentTree;
    DataTree childTree;
    int index;
};

class RemoveAllChildrenAction : public UndoableAction
{
public:
    RemoveAllChildrenAction (DataTree parent, const std::vector<DataTree>& oldChildren)
        : parentTree (parent)
        , children (oldChildren)
    {
    }

    bool isValid() const override
    {
        return parentTree.object != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentTree.object == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
        {
            parentTree.object->children.clear();

            for (size_t i = 0; i < children.size(); ++i)
            {
                children[i].object->parent.reset();
                parentTree.object->sendChildRemovedMessage (children[i], static_cast<int> (i));
            }
        }
        else // Undo
        {
            parentTree.object->children = children;

            for (auto& child : children)
            {
                child.object->parent = parentTree.object;
                parentTree.object->sendChildAddedMessage (child);
            }
        }

        return true;
    }

private:
    DataTree parentTree;
    std::vector<DataTree> children;
};

class MoveChildAction : public UndoableAction
{
public:
    MoveChildAction (DataTree parent, int fromIndex, int toIndex)
        : parentTree (parent)
        , oldIndex (fromIndex)
        , newIndex (toIndex)
    {
    }

    bool isValid() const override
    {
        return parentTree.object != nullptr && oldIndex != newIndex;
    }

    bool perform (UndoableActionState state) override
    {
        if (parentTree.object == nullptr || oldIndex == newIndex)
            return false;

        const int numChildren = static_cast<int> (parentTree.object->children.size());
        if (oldIndex < 0 || oldIndex >= numChildren || newIndex < 0 || newIndex >= numChildren)
            return false;

        if (state == UndoableActionState::Redo)
        {
            auto child = parentTree.object->children[static_cast<size_t> (oldIndex)];
            parentTree.object->children.erase (parentTree.object->children.begin() + oldIndex);
            parentTree.object->children.insert (parentTree.object->children.begin() + newIndex, child);

            parentTree.object->sendChildMovedMessage (child, oldIndex, newIndex);
        }
        else // Undo
        {
            auto child = parentTree.object->children[static_cast<size_t> (newIndex)];
            parentTree.object->children.erase (parentTree.object->children.begin() + newIndex);
            parentTree.object->children.insert (parentTree.object->children.begin() + oldIndex, child);

            parentTree.object->sendChildMovedMessage (child, newIndex, oldIndex);
        }

        return true;
    }

private:
    DataTree parentTree;
    int oldIndex, newIndex;
};

//==============================================================================

class SimpleTransactionAction : public UndoableAction
{
public:
    SimpleTransactionAction (DataTree tree, const String& desc, const NamedValueSet& origProps, const std::vector<DataTree>& origChildren)
        : dataTree (tree)
        , description (desc)
        , originalProperties (origProps)
        , originalChildren (origChildren)
    {
        if (dataTree.object != nullptr)
        {
            currentProperties = dataTree.object->properties;
            currentChildren = dataTree.object->children;
        }
    }

    bool isValid() const override
    {
        return dataTree.object != nullptr;
    }

    bool perform (UndoableActionState state) override
    {
        if (dataTree.object == nullptr)
            return false;

        if (state == UndoableActionState::Redo)
            restoreState (currentProperties, currentChildren);
        else
            restoreState (originalProperties, originalChildren);

        return true;
    }

private:
    void restoreState (const NamedValueSet& props, const std::vector<DataTree>& children)
    {
        for (const auto& currentChild : dataTree.object->children)
        {
            bool willBeKept = false;
            for (const auto& newChild : children)
            {
                if (currentChild.object == newChild.object)
                {
                    willBeKept = true;
                    break;
                }
            }
            
            if (!willBeKept && currentChild.object != nullptr)
                currentChild.object->parent.reset();
        }
        
        dataTree.object->properties = props;
        dataTree.object->children = children;

        for (const auto& child : dataTree.object->children)
        {
            if (child.object != nullptr)
                child.object->parent = dataTree.object;
        }

        for (int i = 0; i < props.size(); ++i)
            dataTree.object->sendPropertyChangeMessage (props.getName (i));

        for (size_t i = 0; i < children.size(); ++i)
            dataTree.object->sendChildAddedMessage (children[i]);
    }

    DataTree dataTree;
    String description;
    NamedValueSet originalProperties, currentProperties;
    std::vector<DataTree> originalChildren, currentChildren;
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

void DataTree::DataObject::sendChildAddedMessage (const DataTree& child)
{
    DataTree treeObj (shared_from_this());
    DataTree childTree (child.object);
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.childAdded (treeObj, childTree);
    });
}

void DataTree::DataObject::sendChildRemovedMessage (const DataTree& child, int formerIndex)
{
    DataTree treeObj (shared_from_this());
    DataTree childTree (child.object);
    listeners.call ([&] (DataTree::Listener& l)
    {
        l.childRemoved (treeObj, childTree, formerIndex);
    });
}

void DataTree::DataObject::sendChildMovedMessage (const DataTree& child, int oldIndex, int newIndex)
{
    DataTree treeObj (shared_from_this());
    DataTree childTree (child.object);
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
        auto childClone = DataTree (child.object->clone());
        childClone.object->parent = newObject;
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
        object = other.object;
        if (object)
        {
            object->listeners.call ([this] (Listener& l)
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
        object = std::move (other.object);
        if (object)
        {
            object->listeners.call ([this] (Listener& l)
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

    auto* managerToUse = undoManager;

    if (managerToUse != nullptr)
    {
        managerToUse->perform (new PropertySetAction (*this, name, newValue, object->properties[name]));
    }
    else
    {
        PropertySetAction (*this, name, newValue, object->properties[name]).perform (UndoableActionState::Redo);
    }
}

void DataTree::removeProperty (const Identifier& name, UndoManager* undoManager)
{
    if (object == nullptr || ! object->properties.contains (name))
        return;

    auto* managerToUse = undoManager;

    if (managerToUse != nullptr)
    {
        managerToUse->perform (new PropertyRemoveAction (*this, name, object->properties[name]));
    }
    else
    {
        PropertyRemoveAction (*this, name, object->properties[name]).perform (UndoableActionState::Redo);
    }
}

void DataTree::removeAllProperties (UndoManager* undoManager)
{
    if (object == nullptr || object->properties.isEmpty())
        return;

    auto* managerToUse = undoManager;

    if (managerToUse != nullptr)
    {
        managerToUse->perform (new RemoveAllPropertiesAction (*this, object->properties));
    }
    else
    {
        RemoveAllPropertiesAction (*this, object->properties).perform (UndoableActionState::Redo);
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

    auto* managerToUse = undoManager;

    if (managerToUse != nullptr)
    {
        managerToUse->perform (new AddChildAction (*this, child, index));
    }
    else
    {
        AddChildAction (*this, child, index).perform (UndoableActionState::Redo);
    }
}

void DataTree::removeChild (const DataTree& child, UndoManager* undoManager)
{
    if (object == nullptr)
        return;

    const int index = indexOf (child);
    if (index >= 0)
        removeChild (index, undoManager);
}

void DataTree::removeChild (int index, UndoManager* undoManager)
{
    if (object == nullptr || index < 0 || index >= static_cast<int> (object->children.size()))
        return;

    auto child = object->children[static_cast<size_t> (index)];
    auto* managerToUse = undoManager;

    if (managerToUse != nullptr)
    {
        managerToUse->perform (new RemoveChildAction (*this, child, index));
    }
    else
    {
        RemoveChildAction (*this, child, index).perform (UndoableActionState::Redo);
    }
}

void DataTree::removeAllChildren (UndoManager* undoManager)
{
    if (object == nullptr || object->children.empty())
        return;

    auto* managerToUse = undoManager;

    if (managerToUse != nullptr)
    {
        managerToUse->perform (new RemoveAllChildrenAction (*this, object->children));
    }
    else
    {
        RemoveAllChildrenAction (*this, object->children).perform (UndoableActionState::Redo);
    }
}

void DataTree::moveChild (int currentIndex, int newIndex, UndoManager* undoManager)
{
    if (object == nullptr || currentIndex == newIndex)
        return;

    const int numChildren = static_cast<int> (object->children.size());
    if (currentIndex < 0 || currentIndex >= numChildren || newIndex < 0 || newIndex >= numChildren)
        return;

    auto* managerToUse = undoManager;

    if (managerToUse != nullptr)
    {
        managerToUse->perform (new MoveChildAction (*this, currentIndex, newIndex));
    }
    else
    {
        MoveChildAction (*this, currentIndex, newIndex).perform (UndoableActionState::Redo);
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

    return object->children[static_cast<size_t> (index)];
}

DataTree DataTree::getChildWithName (const Identifier& type) const noexcept
{
    if (object == nullptr)
        return {};

    for (const auto& child : object->children)
    {
        if (child.getType() == type)
            return child;
    }

    return {};
}

int DataTree::indexOf (const DataTree& child) const noexcept
{
    if (object == nullptr || child.object == nullptr)
        return -1;

    for (size_t i = 0; i < object->children.size(); ++i)
    {
        if (object->children[i].object == child.object)
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
    for (int i = 0; i < object->properties.size(); ++i)
    {
        const auto name = object->properties.getName (i);
        const auto value = object->properties.getValueAt (i);

        element->setAttribute (name.toString(), value.toString());
    }

    // Add children as child elements
    for (const auto& child : object->children)
    {
        if (auto childXml = child.createXml())
            element->addChildElement (childXml.release());
    }

    return element;
}

DataTree DataTree::fromXml (const XmlElement& xml)
{
    DataTree tree (xml.getTagName());

    // Load properties from attributes
    for (int i = 0; i < xml.getNumAttributes(); ++i)
    {
        const auto name = xml.getAttributeName (i);
        const auto value = xml.getAttributeValue (i);
        tree.setProperty (name, value);
    }

    // Load children from child elements
    for (const auto* childXml : xml.getChildIterator())
    {
        auto child = fromXml (*childXml);
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
        child.writeToBinaryStream (output);
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
        var childJson = child.createJson();
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
        if (! object->children[i].isEquivalentTo (other.object->children[i]))
            return false;
    }

    return true;
}

//==============================================================================
// Transaction Implementation

DataTree::Transaction::Transaction (DataTree& tree, const String& desc, UndoManager* manager)
    : dataTree (tree)
    , undoManager (manager)
    , description (desc)
{
    if (dataTree.object == nullptr)
    {
        active = false;
        return;
    }

    captureInitialState();
}

DataTree::Transaction::Transaction (Transaction&& other) noexcept
    : dataTree (other.dataTree)
    , undoManager (other.undoManager)
    , description (std::move (other.description))
    , active (std::exchange (other.active, false))
    , propertyChanges (std::move (other.propertyChanges))
    , childChanges (std::move (other.childChanges))
    , originalProperties (std::move (other.originalProperties))
    , originalChildren (std::move (other.originalChildren))
{
}

DataTree::Transaction& DataTree::Transaction::operator= (Transaction&& other) noexcept
{
    if (this != &other)
    {
        // Commit current transaction if active
        if (active)
            commit();

        dataTree = other.dataTree;
        undoManager = other.undoManager;
        description = std::move (other.description);
        active = std::exchange (other.active, false);
        propertyChanges = std::move (other.propertyChanges);
        childChanges = std::move (other.childChanges);
        originalProperties = std::move (other.originalProperties);
        originalChildren = std::move (other.originalChildren);
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
    if (! active || dataTree.object == nullptr)
        return;

    if (undoManager != nullptr && (! propertyChanges.empty() || ! childChanges.empty()))
    {
        // Apply changes first to get final state, then create undo action with before/after states
        applyChangesToTree (dataTree, originalProperties, originalChildren, propertyChanges, childChanges);
        
        // Create a simple action that can restore the original state
        undoManager->perform (new SimpleTransactionAction (dataTree, description, originalProperties, originalChildren));
    }
    else
    {
        applyChangesToTree (dataTree, originalProperties, originalChildren, propertyChanges, childChanges);
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
    if (! active || dataTree.object == nullptr)
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
    var oldValue = dataTree.getProperty (name);

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
    if (! active || dataTree.object == nullptr)
        return;

    if (! dataTree.hasProperty (name))
        return;

    // Record the change
    PropertyChange change;
    change.type = PropertyChange::Remove;
    change.name = name;
    change.oldValue = dataTree.getProperty (name);
    propertyChanges.push_back (change);
}

void DataTree::Transaction::removeAllProperties()
{
    if (! active || dataTree.object == nullptr)
        return;

    if (dataTree.getNumProperties() == 0)
        return;

    // Record the change
    PropertyChange change;
    change.type = PropertyChange::RemoveAll;
    propertyChanges.push_back (change);
}

void DataTree::Transaction::addChild (const DataTree& child, int index)
{
    if (! active || dataTree.object == nullptr || child.object == nullptr)
        return;

    // Don't add invalid or self-referencing children
    // Also prevent circular references: don't add X to Y if Y is a descendant of X
    if (child.isAChildOf (dataTree) || child == dataTree || dataTree.isAChildOf (child))
        return;

    // Calculate effective number of children including pending additions
    int effectiveNumChildren = dataTree.getNumChildren();
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

    // Record the change
    ChildChange change;
    change.type = ChildChange::Add;
    change.child = child;
    change.newIndex = index;
    change.oldIndex = -1; // Not applicable for add
    childChanges.push_back (change);
}

void DataTree::Transaction::removeChild (const DataTree& child)
{
    if (! active || dataTree.object == nullptr)
        return;

    const int index = dataTree.indexOf (child);
    if (index >= 0)
        removeChild (index);
}

void DataTree::Transaction::removeChild (int index)
{
    if (! active || dataTree.object == nullptr)
        return;

    // Simply record the remove operation by index
    ChildChange change;
    change.type = ChildChange::Remove;
    change.child = DataTree(); // Will be resolved during commit
    change.oldIndex = index;
    change.newIndex = -1; // Not applicable for remove
    childChanges.push_back (change);
}

void DataTree::Transaction::removeAllChildren()
{
    if (! active || dataTree.object == nullptr)
        return;

    if (dataTree.getNumChildren() == 0)
        return;

    // Record the change
    ChildChange change;
    change.type = ChildChange::RemoveAll;
    childChanges.push_back (change);
}

void DataTree::Transaction::moveChild (int currentIndex, int newIndex)
{
    if (! active || dataTree.object == nullptr || currentIndex == newIndex)
        return;

    // Simply record the move operation as specified by the user
    ChildChange change;
    change.type = ChildChange::Move;
    change.child = DataTree(); // Will be resolved during commit
    change.oldIndex = currentIndex;
    change.newIndex = newIndex;
    childChanges.push_back (change);
}

void DataTree::Transaction::captureInitialState()
{
    if (dataTree.object == nullptr)
        return;

    // Capture initial properties
    originalProperties = dataTree.object->properties;

    // Capture initial children
    originalChildren = dataTree.object->children;
}

void DataTree::Transaction::applyChangesToTree (DataTree& tree,
                                                const NamedValueSet& originalProperties,
                                                const std::vector<DataTree>& originalChildren,
                                                const std::vector<PropertyChange>& propertyChanges,
                                                const std::vector<ChildChange>& childChanges)
{
    if (tree.object == nullptr)
        return;

    // Apply property changes directly
    for (const auto& change : propertyChanges)
    {
        switch (change.type)
        {
            case PropertyChange::Set:
                tree.object->properties.set (change.name, change.newValue);
                tree.object->sendPropertyChangeMessage (change.name);
                break;

            case PropertyChange::Remove:
                tree.object->properties.remove (change.name);
                tree.object->sendPropertyChangeMessage (change.name);
                break;

            case PropertyChange::RemoveAll:
            {
                auto oldProperties = tree.object->properties;
                tree.object->properties.clear();
                for (int i = 0; i < oldProperties.size(); ++i)
                    tree.object->sendPropertyChangeMessage (oldProperties.getName (i));
            }
            break;
        }
    }

    // Apply child changes directly
    for (const auto& change : childChanges)
    {
        switch (change.type)
        {
            case ChildChange::Add:
            {
                // Remove from previous parent if any
                if (auto oldParentObj = change.child.object->parent.lock())
                {
                    DataTree oldParent (oldParentObj);
                    oldParent.removeChild (change.child, nullptr);
                }

                const int numChildren = static_cast<int> (tree.object->children.size());
                const int actualIndex = (change.newIndex < 0 || change.newIndex > numChildren) ? numChildren : change.newIndex;

                tree.object->children.insert (tree.object->children.begin() + actualIndex, change.child);
                change.child.object->parent = tree.object;
                tree.object->sendChildAddedMessage (change.child);
            }
            break;

            case ChildChange::Remove:
            {
                // Resolve child by index at commit time
                if (change.oldIndex >= 0 && change.oldIndex < static_cast<int> (tree.object->children.size()))
                {
                    auto child = tree.object->children[static_cast<size_t> (change.oldIndex)];
                    tree.object->children.erase (tree.object->children.begin() + change.oldIndex);
                    child.object->parent.reset();
                    tree.object->sendChildRemovedMessage (child, change.oldIndex);
                }
            }
            break;

            case ChildChange::RemoveAll:
            {
                auto oldChildren = tree.object->children;
                tree.object->children.clear();
                for (size_t i = 0; i < oldChildren.size(); ++i)
                {
                    oldChildren[i].object->parent.reset();
                    tree.object->sendChildRemovedMessage (oldChildren[i], static_cast<int> (i));
                }
            }
            break;

            case ChildChange::Move:
            {
                // Resolve child by current index at commit time
                const int numChildren = static_cast<int> (tree.object->children.size());
                if (change.oldIndex >= 0 && change.oldIndex < numChildren && change.newIndex >= 0 && change.newIndex < numChildren)
                {
                    auto child = tree.object->children[static_cast<size_t> (change.oldIndex)];
                    tree.object->children.erase (tree.object->children.begin() + change.oldIndex);
                    tree.object->children.insert (tree.object->children.begin() + change.newIndex, child);
                    tree.object->sendChildMovedMessage (child, change.oldIndex, change.newIndex);
                }
            }
            break;
        }
    }
}

//==============================================================================

DataTree::ValidatedTransaction::ValidatedTransaction (DataTree& tree, ReferenceCountedObjectPtr<DataTreeSchema> schema, const String& description, UndoManager* undoManager)
    : transaction (std::make_unique<Transaction> (tree.beginTransaction (description, undoManager)))
    , schema (std::move (schema))
    , nodeType (tree.getType())
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

    // TODO: Get current child count from the transaction's target tree
    auto validationResult = schema->validateChildAddition (nodeType, child.getType(), 0);
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

    // TODO: Check minimum child count constraints

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
