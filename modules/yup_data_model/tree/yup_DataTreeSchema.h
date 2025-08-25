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
/**
    A schema system for defining, validating, and instantiating DataTree structures.
    
    DataTreeSchema provides comprehensive validation and metadata querying capabilities
    for DataTree nodes, including property validation, structural constraints, and
    schema-driven object instantiation with default values.
    
    ## Key Features:
    - **JSON Schema Support**: Load schemas from standard JSON Schema format
    - **Property Validation**: Type checking, ranges, enums, patterns, and custom constraints
    - **Structural Validation**: Node type validation, child constraints, and hierarchy rules
    - **Metadata Querying**: Access property types, defaults, constraints, and documentation
    - **Smart Instantiation**: Create DataTree nodes with proper defaults and validation
    - **Transaction Integration**: Validate mutations during DataTree transactions
    
    ## Basic Usage:
    @code
    // Load schema from JSON
    String schemaJson = R"({
        "nodeTypes": {
            "Settings": {
                "properties": {
                    "theme": {
                        "type": "string",
                        "default": "light",
                        "enum": ["light", "dark", "auto"]
                    },
                    "fontSize": {
                        "type": "number",
                        "default": 12,
                        "minimum": 8,
                        "maximum": 72
                    }
                }
            }
        }
    })";
    
    auto schema = DataTreeSchema::fromJsonSchema(schemaJson);
    
    // Create validated DataTree with defaults
    auto settingsTree = schema.createNode("Settings");
    // settingsTree now has theme="light" and fontSize=12
    
    // Query property metadata
    auto themeInfo = schema.getPropertyInfo("Settings", "theme");
    String defaultTheme = themeInfo.getDefault(); // "light"
    Array<var> allowedValues = themeInfo.getEnumValues(); // ["light", "dark", "auto"]
    
    // Validate mutations
    auto result = schema.validatePropertyValue("Settings", "fontSize", 150);
    if (result.failed())
        std::cout << result.getErrorMessage(); // "Value 150 exceeds maximum 72"
    @endcode
    
    ## Schema-Aware Child Creation:
    @code
    DataTree root("Root");
    
    // Add child using schema - applies defaults and validates
    auto transaction = root.beginTransaction("Add Settings");
    auto settingsChild = schema.createChildNode("Root", "Settings");
    transaction.addChild(settingsChild);
    // settingsChild has all default properties set
    @endcode
    
    @see DataTree, ValidatedTransaction
*/
class YUP_API DataTreeSchema : public ReferenceCountedObject
{
public:
    //==============================================================================
    /** Convenience typedef for reference-counted pointer to DataTreeSchema. */
    using Ptr = ReferenceCountedObjectPtr<DataTreeSchema>;

    //==============================================================================
    /**
        Creates an empty schema with no node type definitions.
        
        Use fromJsonSchema() or addNodeType() to populate the schema.
    */
    DataTreeSchema() = default;
    
    /**
        Copy constructor - creates a deep copy of the schema.
    */
    DataTreeSchema (const DataTreeSchema& other) = default;

    /**
        Move constructor - transfers ownership of schema data.
    */
    DataTreeSchema (DataTreeSchema&& other) noexcept = default;

    /**
        Destructor - automatically cleans up schema resources.
    */
    ~DataTreeSchema() = default;
    
    /**
        Copy assignment - creates a deep copy of the schema.
    */
    DataTreeSchema& operator= (const DataTreeSchema& other) = default;

    /**
        Move assignment - transfers ownership of schema data.
    */
    DataTreeSchema& operator= (DataTreeSchema&& other) noexcept = default;

    //==============================================================================
    /**
        Loads a schema from JSON Schema in string format.

        The JSON should follow the DataTree schema specification with nodeTypes
        definitions containing properties and children constraints.
        
        @param schemaData JSON string containing the schema.

        @return A reference-counted pointer to DataTreeSchema, or nullptr if parsing fails

        @code
        String json = R"({
            "nodeTypes": {
                "Button": {
                    "properties": {
                        "text": {"type": "string", "required": true},
                        "enabled": {"type": "boolean", "default": true}
                    },
                    "children": {"maxCount": 0}
                }
            }
        })";
        auto schema = DataTreeSchema::fromJsonSchemaString(json);
        @endcode
    */
    static Ptr fromJsonSchemaString (const String& schemaData);

    /**
        Loads a schema from JSON Schema in parsed var format.

        The JSON should follow the DataTree schema specification with nodeTypes
        definitions containing properties and children constraints.
        
        @param schemaData Parsed var object containing the schema.

        @return A reference-counted pointer to DataTreeSchema, or nullptr if parsing fails
    */
    static Ptr fromJsonSchema (const var& schemaData);

    /**
        Exports this schema to JSON Schema format.
        
        @return JSON representation of the schema as a var object
    */
    var toJsonSchema() const;
    
    /**
        Checks if this schema is valid and can be used for validation.
        
        @return true if the schema contains valid node type definitions
    */
    bool isValid() const;
    
    //==============================================================================
    /**
        Validates a complete DataTree against this schema.
        
        Performs comprehensive validation including node types, properties,
        property values, and structural constraints.
        
        @param tree The DataTree to validate

        @return Result indicating success or failure with detailed error messages

        @code
        DataTree tree("Settings");
        auto result = schema.validate(tree);
        if (result.failed())
            DBG("Validation failed: " << result.getErrorMessage());
        @endcode
    */
    yup::Result validate (const DataTree& tree) const;

    /**
        Validates a specific property value against schema constraints.
        
        @param nodeType The type of node containing the property
        @param propertyName The name of the property to validate
        @param value The value to validate

        @return Result indicating if the value is valid for this property
    */
    yup::Result validatePropertyValue (const Identifier& nodeType,
                                       const Identifier& propertyName,
                                       const var& value) const;

    /**
        Validates if a child node can be added to a parent node.
        
        Checks child type constraints, count limits, and ordering requirements.
        
        @param parentType The type of the parent node
        @param childType The type of the proposed child node
        @param currentChildCount The current number of children in the parent

        @return Result indicating if the child can be added
    */
    yup::Result validateChildAddition (const Identifier& parentType,
                                       const Identifier& childType,
                                       int currentChildCount = 0) const;

    //==============================================================================
    /**
        Creates a new DataTree node of the specified type with default properties.
        
        The created node will have all required properties set to their default
        values as defined in the schema. Optional properties with defaults will
        also be set.
        
        @param nodeType The type of node to create
        @return A new DataTree with default properties, or invalid tree if type unknown
        
        @code
        auto button = schema.createNode("Button");
        // button has "enabled" = true and any other defaults
        @endcode
    */
    DataTree createNode (const Identifier& nodeType) const;

    /**
        Creates a child node that can be added to the specified parent type.
        
        This is a convenience method that creates a node of the specified child type
        and ensures it's compatible with the parent's child constraints.
        
        @param parentType The type of the parent that will contain this child
        @param childType The type of child node to create
        @return A new DataTree configured for the parent, or invalid if incompatible
        
        @code
        // Create a Settings child that can be added to Root
        auto settings = schema.createChildNode("Root", "Settings");
        @endcode
    */
    DataTree createChildNode (const Identifier& parentType, const Identifier& childType) const;

    //==============================================================================
    /**
        Information about a property defined in the schema.
        
        Provides access to all metadata about a property including its type,
        constraints, default value, and validation rules.
    */
    struct PropertyInfo
    {
        /**
            The data type of this property ("string", "number", "boolean", "array", "object").
        */
        String type;
        
        /**
            Whether this property is required to be present.
        */
        bool required = false;
        
        /**
            The default value for this property, or undefined if no default.
        */
        var defaultValue;
        
        /**
            Human-readable description of this property.
        */
        String description;
        
        /**
            Allowed values for enum-type properties.
        */
        Array<var> enumValues;
        
        /**
            Minimum value for numeric properties.
        */
        std::optional<double> minimum;
        
        /**
            Maximum value for numeric properties.
        */
        std::optional<double> maximum;
        
        /**
            Minimum length for string properties.
        */
        std::optional<int> minLength;
        
        /**
            Maximum length for string properties.
        */
        std::optional<int> maxLength;
        
        /**
            Regular expression pattern for string validation.
        */
        String pattern;
        
        /**
            Whether this property has a default value.
        */
        bool hasDefault() const { return !defaultValue.isUndefined(); }
        
        /**
            Whether this property is an enum with restricted values.
        */
        bool isEnum() const { return !enumValues.isEmpty(); }
        
        /**
            Whether this property has numeric constraints.
        */
        bool hasNumericConstraints() const { return minimum.has_value() || maximum.has_value(); }
        
        /**
            Whether this property has string length constraints.
        */
        bool hasLengthConstraints() const { return minLength.has_value() || maxLength.has_value(); }
    };
    
    /**
        Gets detailed information about a specific property.
        
        @param nodeType The node type containing the property
        @param propertyName The name of the property
        @return PropertyInfo struct with all metadata, or empty info if not found
    */
    PropertyInfo getPropertyInfo (const Identifier& nodeType, const Identifier& propertyName) const;

    /**
        Gets all property names defined for a node type.
        
        @param nodeType The node type to query
        @return Array of property names, empty if node type not found
    */
    StringArray getPropertyNames (const Identifier& nodeType) const;

    /**
        Gets all required property names for a node type.
        
        @param nodeType The node type to query
        @return Array of required property names
    */
    StringArray getRequiredPropertyNames (const Identifier& nodeType) const;

    //==============================================================================
    /**
        Information about child constraints for a node type.
    */
    struct ChildConstraints
    {
        /**
            Node types that are allowed as children.
        */
        StringArray allowedTypes;
        
        /**
            Minimum number of children required.
        */
        int minCount = 0;
        
        /**
            Maximum number of children allowed (-1 for unlimited).
        */
        int maxCount = -1;
        
        /**
            Whether child order is significant.
        */
        bool ordered = false;
        
        /**
            Whether any child type is allowed (empty allowedTypes with maxCount > 0).
        */
        bool allowsAnyType() const { return allowedTypes.isEmpty() && maxCount != 0; }
        
        /**
            Whether children are allowed at all.
        */
        bool allowsChildren() const { return maxCount != 0; }
    };
    
    /**
        Gets child constraints for a specific node type.
        
        @param nodeType The node type to query
        @return ChildConstraints with all child rules
    */
    ChildConstraints getChildConstraints (const Identifier& nodeType) const;

    /**
        Gets all defined node type names in this schema.
        
        @return Array of node type identifiers
    */
    StringArray getNodeTypeNames() const;
    
    /**
        Checks if a node type is defined in this schema.
        
        @param nodeType The node type to check
        @return true if the node type is defined
    */
    bool hasNodeType (const Identifier& nodeType) const;

private:
    struct PropertySchema
    {
        String type;
        bool required = false;
        var defaultValue;
        String description;
        Array<var> enumValues;
        std::optional<double> minimum;
        std::optional<double> maximum;
        std::optional<int> minLength;
        std::optional<int> maxLength;
        String pattern;

        PropertySchema() = default;

        explicit PropertySchema (const var& propertyDef);
    };

    struct NodeTypeSchema
    {
        String description;
        HashMap<Identifier, PropertySchema> properties;
        DataTreeSchema::ChildConstraints childConstraints;

        NodeTypeSchema() = default;

        explicit NodeTypeSchema (const var& nodeTypeDef);
    };

    bool loadFromJson (const var& schemaData);
    Result validateProperty (const Identifier& nodeType, const Identifier& propertyName, const var& value) const;
    Result validateValueAgainstSchema (const var& value, const PropertySchema& schema, const String& propertyName) const;
    DataTree createNodeWithDefaults (const Identifier& nodeType) const;

    HashMap<Identifier, NodeTypeSchema> nodeTypes;
    bool valid = false;
    
    YUP_LEAK_DETECTOR (DataTreeSchema)
};

} // namespace yup
