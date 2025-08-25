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

DataTreeSchema::PropertySchema::PropertySchema (const var& propertyDef)
{
    if (! propertyDef.isObject())
        return;

    auto* obj = propertyDef.getDynamicObject();
    if (! obj)
        return;

    type = obj->getProperty ("type", "string").toString();
    required = obj->getProperty ("required", false);
    defaultValue = obj->getProperty ("default", var::undefined());
    description = obj->getProperty ("description", "").toString();

    // Handle enum values
    var enumVar = obj->getProperty ("enum");
    if (enumVar.isArray())
    {
        auto* enumArray = enumVar.getArray();
        for (int i = 0; i < enumArray->size(); ++i)
            enumValues.add (enumArray->getReference (i));
    }

    // Handle numeric constraints
    var minVar = obj->getProperty ("minimum");
    if (minVar.isDouble() || minVar.isInt())
        minimum = static_cast<double> (minVar);

    var maxVar = obj->getProperty ("maximum");
    if (maxVar.isDouble() || maxVar.isInt())
        maximum = static_cast<double> (maxVar);

    // Handle string constraints
    var minLenVar = obj->getProperty ("minLength");
    if (minLenVar.isInt())
        minLength = static_cast<int> (minLenVar);

    var maxLenVar = obj->getProperty ("maxLength");
    if (maxLenVar.isInt())
        maxLength = static_cast<int> (maxLenVar);

    pattern = obj->getProperty ("pattern", "").toString();
}

//==============================================================================

DataTreeSchema::NodeTypeSchema::NodeTypeSchema (const var& nodeTypeDef)
{
    if (! nodeTypeDef.isObject())
        return;

    auto* obj = nodeTypeDef.getDynamicObject();
    if (! obj)
        return;

    description = obj->getProperty ("description", "").toString();

    // Parse properties
    var propsVar = obj->getProperty ("properties");
    if (propsVar.isObject())
    {
        auto* propsObj = propsVar.getDynamicObject();
        if (propsObj)
        {
            const auto& props = propsObj->getProperties();
            for (int i = 0; i < props.size(); ++i)
            {
                Identifier propName (props.getName (i).toString());
                properties.set (propName, PropertySchema (props.getValueAt (i)));
            }
        }
    }

    // Parse child constraints
    var childrenVar = obj->getProperty ("children");
    if (childrenVar.isObject())
    {
        auto* childrenObj = childrenVar.getDynamicObject();
        if (childrenObj)
        {
            childConstraints.minCount = childrenObj->getProperty ("minCount", 0);
            childConstraints.maxCount = childrenObj->getProperty ("maxCount", -1);
            childConstraints.ordered = childrenObj->getProperty ("ordered", false);

            var allowedTypesVar = childrenObj->getProperty ("allowedTypes");
            if (allowedTypesVar.isArray())
            {
                auto* typesArray = allowedTypesVar.getArray();
                for (int i = 0; i < typesArray->size(); ++i)
                {
                    String typeName = typesArray->getReference (i).toString();
                    if (typeName.isNotEmpty())
                        childConstraints.allowedTypes.add (typeName);
                }
            }
        }
    }
}

//==============================================================================

bool DataTreeSchema::loadFromJson (const var& schemaData)
{
    nodeTypes.clear();
    valid = false;

    if (! schemaData.isObject())
        return false;

    auto* schemaObj = schemaData.getDynamicObject();
    if (! schemaObj)
        return false;

    var nodeTypesVar = schemaObj->getProperty ("nodeTypes");
    if (! nodeTypesVar.isObject())
        return false;

    auto* nodeTypesObj = nodeTypesVar.getDynamicObject();
    if (! nodeTypesObj)
        return false;

    const auto& types = nodeTypesObj->getProperties();
    for (int i = 0; i < types.size(); ++i)
    {
        Identifier typeName (types.getName (i).toString());
        nodeTypes.set (typeName, NodeTypeSchema (types.getValueAt (i)));
    }

    valid = ! nodeTypes.isEmpty();
    return valid;
}

DataTreeSchema::Ptr DataTreeSchema::fromJsonSchema (const var& schemaData)
{
    auto schema = std::make_unique<DataTreeSchema>();
    if (! schema->loadFromJson (schemaData))
        return nullptr;

    return schema.release();
}

DataTreeSchema::Ptr DataTreeSchema::fromJsonSchemaString (const String& schemaData)
{
    var result;
    if (! JSON::parse (schemaData, result))
        return nullptr;

    return fromJsonSchema (result);
}

//==============================================================================

var DataTreeSchema::toJsonSchema() const
{
    auto schemaObj = std::make_unique<DynamicObject>();
    auto nodeTypesObj = std::make_unique<DynamicObject>();

    for (auto it = nodeTypes.begin(); it != nodeTypes.end(); ++it)
    {
        const Identifier& typeName = it.getKey();
        const NodeTypeSchema& nodeSchema = it.getValue();

        auto nodeTypeObj = std::make_unique<DynamicObject>();

        if (nodeSchema.description.isNotEmpty())
            nodeTypeObj->setProperty ("description", nodeSchema.description);

        // Properties
        if (! nodeSchema.properties.isEmpty())
        {
            auto propertiesObj = std::make_unique<DynamicObject>();

            for (auto propIt = nodeSchema.properties.begin(); propIt != nodeSchema.properties.end(); ++propIt)
            {
                const Identifier& propName = propIt.getKey();
                const PropertySchema& propSchema = propIt.getValue();

                auto propObj = std::make_unique<DynamicObject>();
                propObj->setProperty ("type", propSchema.type);

                if (propSchema.required)
                    propObj->setProperty ("required", true);

                if (! propSchema.defaultValue.isUndefined())
                    propObj->setProperty ("default", propSchema.defaultValue);

                if (propSchema.description.isNotEmpty())
                    propObj->setProperty ("description", propSchema.description);

                if (! propSchema.enumValues.isEmpty())
                    propObj->setProperty ("enum", Array<var> (propSchema.enumValues));

                if (propSchema.minimum.has_value())
                    propObj->setProperty ("minimum", propSchema.minimum.value());

                if (propSchema.maximum.has_value())
                    propObj->setProperty ("maximum", propSchema.maximum.value());

                if (propSchema.minLength.has_value())
                    propObj->setProperty ("minLength", propSchema.minLength.value());

                if (propSchema.maxLength.has_value())
                    propObj->setProperty ("maxLength", propSchema.maxLength.value());

                if (propSchema.pattern.isNotEmpty())
                    propObj->setProperty ("pattern", propSchema.pattern);

                propertiesObj->setProperty (propName.toString(), propObj.release());
            }

            nodeTypeObj->setProperty ("properties", propertiesObj.release());
        }

        // Child constraints
        auto childrenObj = std::make_unique<DynamicObject>();

        if (! nodeSchema.childConstraints.allowedTypes.isEmpty())
        {
            Array<var> allowedTypes;
            for (const auto& item : nodeSchema.childConstraints.allowedTypes)
                allowedTypes.add (item);

            childrenObj->setProperty ("allowedTypes", allowedTypes);
        }

        if (nodeSchema.childConstraints.minCount > 0)
            childrenObj->setProperty ("minCount", nodeSchema.childConstraints.minCount);

        if (nodeSchema.childConstraints.maxCount >= 0)
            childrenObj->setProperty ("maxCount", nodeSchema.childConstraints.maxCount);

        if (nodeSchema.childConstraints.ordered)
            childrenObj->setProperty ("ordered", true);

        nodeTypeObj->setProperty ("children", childrenObj.release());
        nodeTypesObj->setProperty (typeName.toString(), nodeTypeObj.release());
    }

    schemaObj->setProperty ("nodeTypes", nodeTypesObj.release());
    return schemaObj.release();
}

//==============================================================================

bool DataTreeSchema::isValid() const
{
    return valid;
}

Result DataTreeSchema::validate (const DataTree& tree) const
{
    if (! tree.isValid())
        return Result::fail ("Invalid DataTree");

    Identifier nodeType = tree.getType();
    auto* nodeSchema = nodeTypes.getPointer (nodeType);
    if (! nodeSchema)
        return Result::fail ("Unknown node type: " + nodeType.toString());

    // Validate required properties
    for (auto it = nodeSchema->properties.begin(); it != nodeSchema->properties.end(); ++it)
    {
        const Identifier& propName = it.getKey();
        const PropertySchema& propSchema = it.getValue();

        if (propSchema.required && ! tree.hasProperty (propName))
            return Result::fail ("Required property '" + propName.toString() + "' is missing");

        if (tree.hasProperty (propName))
        {
            var propValue = tree.getProperty (propName);
            auto validationResult = validateValueAgainstSchema (propValue, propSchema, propName.toString());
            if (validationResult.failed())
                return validationResult;
        }
    }

    // Validate child constraints
    const auto& childConstraints = nodeSchema->childConstraints;
    int childCount = tree.getNumChildren();

    if (childCount < childConstraints.minCount)
        return Result::fail ("Node requires at least " + String (childConstraints.minCount) + " children, has " + String (childCount));

    if (childConstraints.maxCount >= 0 && childCount > childConstraints.maxCount)
        return Result::fail ("Node allows at most " + String (childConstraints.maxCount) + " children, has " + String (childCount));

    // Validate child types
    if (! childConstraints.allowsAnyType())
    {
        for (int i = 0; i < childCount; ++i)
        {
            DataTree child = tree.getChild (i);
            Identifier childType = child.getType();

            if (! childConstraints.allowedTypes.contains (childType.toString()))
                return Result::fail ("Child type '" + childType.toString() + "' is not allowed in '" + nodeType.toString() + "'");

            // Recursively validate children
            auto childResult = validate (child);
            if (childResult.failed())
                return childResult;
        }
    }

    return Result::ok();
}

Result DataTreeSchema::validatePropertyValue (const Identifier& nodeType, const Identifier& propertyName, const var& value) const
{
    return validateProperty (nodeType, propertyName, value);
}

Result DataTreeSchema::validateChildAddition (const Identifier& parentType, const Identifier& childType, int currentChildCount) const
{
    auto* nodeSchema = nodeTypes.getPointer (parentType);
    if (! nodeSchema)
        return Result::fail ("Unknown node type: " + parentType.toString());

    const auto& childConstraints = nodeSchema->childConstraints;

    // Check count constraints
    if (childConstraints.maxCount >= 0 && currentChildCount >= childConstraints.maxCount)
        return Result::fail ("Parent '" + parentType.toString() + "' already has maximum number of children (" + String (childConstraints.maxCount) + ")");

    // Check type constraints
    if (! childConstraints.allowsAnyType() && ! childConstraints.allowedTypes.contains (childType.toString()))
        return Result::fail ("Child type '" + childType.toString() + "' is not allowed in parent '" + parentType.toString() + "'");

    return Result::ok();
}

//==============================================================================

DataTree DataTreeSchema::createNode (const Identifier& nodeType) const
{
    return createNodeWithDefaults (nodeType);
}

DataTree DataTreeSchema::createChildNode (const Identifier& parentType, const Identifier& childType) const
{
    // First validate that this child type is allowed
    auto validationResult = validateChildAddition (parentType, childType, 0);
    if (validationResult.failed())
        return DataTree(); // Invalid tree

    return createNode (childType);
}

DataTreeSchema::PropertyInfo DataTreeSchema::getPropertyInfo (const Identifier& nodeType, const Identifier& propertyName) const
{
    PropertyInfo info;

    auto* nodeSchema = nodeTypes.getPointer (nodeType);
    if (! nodeSchema)
        return info;

    auto* propSchema = nodeSchema->properties.getPointer (propertyName);
    if (! propSchema)
        return info;

    info.type = propSchema->type;
    info.required = propSchema->required;
    info.defaultValue = propSchema->defaultValue;
    info.description = propSchema->description;
    info.enumValues = propSchema->enumValues;
    info.minimum = propSchema->minimum;
    info.maximum = propSchema->maximum;
    info.minLength = propSchema->minLength;
    info.maxLength = propSchema->maxLength;
    info.pattern = propSchema->pattern;

    return info;
}

//==============================================================================

StringArray DataTreeSchema::getPropertyNames (const Identifier& nodeType) const
{
    StringArray names;

    auto* nodeSchema = nodeTypes.getPointer (nodeType);
    if (! nodeSchema)
        return names;

    for (auto it = nodeSchema->properties.begin(); it != nodeSchema->properties.end(); ++it)
        names.add (it.getKey().toString());

    return names;
}

StringArray DataTreeSchema::getRequiredPropertyNames (const Identifier& nodeType) const
{
    StringArray names;

    auto* nodeSchema = nodeTypes.getPointer (nodeType);
    if (! nodeSchema)
        return names;

    for (auto it = nodeSchema->properties.begin(); it != nodeSchema->properties.end(); ++it)
    {
        if (it.getValue().required)
            names.add (it.getKey().toString());
    }

    return names;
}

DataTreeSchema::ChildConstraints DataTreeSchema::getChildConstraints (const Identifier& nodeType) const
{
    ChildConstraints constraints;

    auto* nodeSchema = nodeTypes.getPointer (nodeType);
    if (nodeSchema)
        constraints = nodeSchema->childConstraints;

    return constraints;
}

StringArray DataTreeSchema::getNodeTypeNames() const
{
    StringArray names;

    for (auto it = nodeTypes.begin(); it != nodeTypes.end(); ++it)
        names.add (it.getKey().toString());

    return names;
}

bool DataTreeSchema::hasNodeType (const Identifier& nodeType) const
{
    return nodeTypes.contains (nodeType);
}

//==============================================================================

Result DataTreeSchema::validateProperty (const Identifier& nodeType, const Identifier& propertyName, const var& value) const
{
    auto* nodeSchema = nodeTypes.getPointer (nodeType);
    if (! nodeSchema)
        return Result::fail ("Unknown node type: " + nodeType.toString());

    auto* propSchema = nodeSchema->properties.getPointer (propertyName);
    if (! propSchema)
        return Result::fail ("Unknown property '" + propertyName.toString() + "' for node type '" + nodeType.toString() + "'");

    return validateValueAgainstSchema (value, *propSchema, propertyName.toString());
}

Result DataTreeSchema::validateValueAgainstSchema (const var& value, const PropertySchema& schema, const String& propertyName) const
{
    // Type validation
    if (schema.type == "string" && ! value.isString())
        return Result::fail ("Property '" + propertyName + "' must be a string");
    else if (schema.type == "number" && ! value.isDouble() && ! value.isInt())
        return Result::fail ("Property '" + propertyName + "' must be a number");
    else if (schema.type == "boolean" && ! value.isBool())
        return Result::fail ("Property '" + propertyName + "' must be a boolean");
    else if (schema.type == "array" && ! value.isArray())
        return Result::fail ("Property '" + propertyName + "' must be an array");
    else if (schema.type == "object" && ! value.isObject())
        return Result::fail ("Property '" + propertyName + "' must be an object");

    // Enum validation
    if (! schema.enumValues.isEmpty())
    {
        bool found = false;
        for (const auto& enumValue : schema.enumValues)
        {
            if (enumValue == value)
            {
                found = true;
                break;
            }
        }
        if (! found)
            return Result::fail ("Property '" + propertyName + "' must be one of the allowed values");
    }

    // Numeric constraints
    if (schema.type == "number" && (value.isDouble() || value.isInt()))
    {
        double numValue = static_cast<double> (value);

        if (schema.minimum.has_value() && numValue < schema.minimum.value())
            return Result::fail ("Property '" + propertyName + "' value " + String (numValue) + " is below minimum " + String (schema.minimum.value()));

        if (schema.maximum.has_value() && numValue > schema.maximum.value())
            return Result::fail ("Property '" + propertyName + "' value " + String (numValue) + " exceeds maximum " + String (schema.maximum.value()));
    }

    // String constraints
    if (schema.type == "string" && value.isString())
    {
        String strValue = value.toString();

        if (schema.minLength.has_value() && strValue.length() < schema.minLength.value())
            return Result::fail ("Property '" + propertyName + "' length " + String (strValue.length()) + " is below minimum " + String (schema.minLength.value()));

        if (schema.maxLength.has_value() && strValue.length() > schema.maxLength.value())
            return Result::fail ("Property '" + propertyName + "' length " + String (strValue.length()) + " exceeds maximum " + String (schema.maxLength.value()));

        // TODO: Pattern validation using regex
        if (schema.pattern.isNotEmpty())
        {
            // For now, just validate it's not empty - proper regex validation would require regex library
            if (strValue.isEmpty())
                return Result::fail ("Property '" + propertyName + "' does not match required pattern");
        }
    }

    return Result::ok();
}

//==============================================================================

DataTree DataTreeSchema::createNodeWithDefaults (const Identifier& nodeType) const
{
    auto* nodeSchema = nodeTypes.getPointer (nodeType);
    if (! nodeSchema)
        return DataTree(); // Invalid tree

    DataTree tree (nodeType);

    // Set default values for properties
    for (auto it = nodeSchema->properties.begin(); it != nodeSchema->properties.end(); ++it)
    {
        const Identifier& propName = it.getKey();
        const PropertySchema& propSchema = it.getValue();

        if (! propSchema.defaultValue.isUndefined())
        {
            auto transaction = tree.beginTransaction ("Set default properties");
            transaction.setProperty (propName, propSchema.defaultValue);
        }
    }

    return tree;
}

} // namespace yup
