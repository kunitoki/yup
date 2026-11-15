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

class Component;

//==============================================================================
/**
    Describes a single reflected property of a component type.

    A ComponentProperty is the unit of the reflected component model: it gives a
    name usable from C++, `.yui` files, the CLI schema and Python, and describes
    how reading and writing that name maps onto a component.

    Two kinds of properties exist:

    - Store-backed properties keep their value in the component's property store,
      keyed by @ref storageId (or by @ref name when no storage id is given). Colors
      and curated geometry are store-backed, so they cascade up the parent chain
      and can be themed. @ref getter and @ref setter are left empty for these.

    - State-backed properties are backed by the component's own getters and setters
      (for example a Slider's value or a Label's text). These resolve only on the
      owning component and never cascade. @ref getter reads the live value; a null
      @ref setter marks the property read-only.

    @see Component, ComponentTypeInfo

    @tags{UI}
*/
struct ComponentProperty
{
    //==============================================================================
    /** The value type a property carries, used by tooling and conversion. */
    enum class Type
    {
        number,      /**< A floating point number. */
        integer,     /**< An integer number. */
        boolean,     /**< A true/false value. */
        string,      /**< A text string. */
        color,       /**< A color, stored as an ARGB int64. */
        point,       /**< A 2D point. */
        rectangle,   /**< A rectangle. */
        enumeration, /**< One of a fixed set of string values (see @ref enumValues). */
        font,        /**< A font. */
        drawing,     /**< The name of a drawing program. */
        array,       /**< A list of values, stored as a var array (see @ref elementType). */
        any          /**< An untyped value. */
    };

    //==============================================================================
    /** The name of the property, as used in `.yui`, Python and the CLI schema. */
    Identifier name;

    /** The store key for store-backed properties, for example Slider::Style::thumbColorId.

        When invalid, @ref name is used as the store key. Ignored for state-backed properties.
    */
    Identifier storageId;

    /** The value type of the property. */
    Type type = Type::any;

    /** The type of the elements of an @ref Type::array property, Type::any when they are untyped.

        Only the types an expression reads are typed (see @ref isExpressionType). Ignored for any
        other @ref type.
    */
    Type elementType = Type::any;

    /** The default value, and the single source of truth for curated defaults. */
    var defaultValue;

    /** Reads the live value from a component. Non-null makes the property state-backed. */
    std::function<var (const Component&)> getter;

    /** Writes a value to a component. Null with a non-null @ref getter makes it read-only. */
    std::function<void (Component&, const var&)> setter;

    /** True when this boolean property doubles as a pseudo-class (`:checked`, `:dragging`). */
    bool isState = false;

    /** True when a stylesheet may set this property (false for value, text, ...). */
    bool isStyleable = true;

    /** True when a store-backed value set on a component also applies to its descendants.

        Colors and curated geometry are inherited. Layout item properties such as `flexGrow` describe
        one component only, so @ref Component::findProperty doesn't look them up in the parents.
    */
    bool isInherited = true;

    /** True when a change of this property on a child changes how its parent lays it out. */
    bool affectsParentLayout = false;

    /** The allowed values for an @ref Type::enumeration property. */
    StringArray enumValues;

    /** A human readable description, surfaced by the schema and tooling. */
    String description;

    //==============================================================================
    /** Returns true when the property is backed by a component getter. */
    bool isStateBacked() const noexcept { return getter != nullptr; }

    /** Returns true when the property is kept in the component's property store. */
    bool isStoreBacked() const noexcept { return getter == nullptr; }

    /** Returns true when the property can be read but not written. */
    bool isReadOnly() const noexcept { return getter != nullptr && setter == nullptr; }

    /** Returns the key used to store a store-backed value: @ref storageId, or @ref name when unset. */
    const Identifier& getStorageKey() const noexcept { return storageId.isValid() ? storageId : name; }

    /** Returns the type a `.yui` declaration names: `number`, `integer`, `boolean`, `string`, `color`,
        `point`, `rectangle`, `font` or `drawing`, Type::array for an array of a scalar type such as
        `string[]`, and Type::any for any other name.
    */
    static Type getTypeFromName (const String& typeName)
    {
        static const std::pair<const char*, Type> types[] = {
            { "number", Type::number },
            { "integer", Type::integer },
            { "boolean", Type::boolean },
            { "string", Type::string },
            { "color", Type::color },
            { "point", Type::point },
            { "rectangle", Type::rectangle },
            { "font", Type::font },
            { "drawing", Type::drawing }
        };

        if (typeName.endsWith ("[]"))
            return getElementTypeFromName (typeName) != Type::any ? Type::array : Type::any;

        for (const auto& [candidate, type] : types)
            if (typeName == candidate)
                return type;

        return Type::any;
    }

    /** Returns the element type a `.yui` array declaration such as `string[]` names, Type::any otherwise.

        Only the scalar types an expression reads make an array (see @ref isExpressionType), so
        `drawing[]` is untyped.
    */
    static Type getElementTypeFromName (const String& typeName)
    {
        if (! typeName.endsWith ("[]"))
            return Type::any;

        const auto element = getTypeFromName (typeName.dropLastCharacters (2));

        return isExpressionType (element) ? element : Type::any;
    }

    /** Returns true for the scalar types an expression reads and writes.

        These are number, integer, boolean, string, color, point and rectangle. They are the types a
        value expression can produce, a drawing can declare, and an array can hold.
    */
    static bool isExpressionType (Type type) noexcept
    {
        return type == Type::number || type == Type::integer || type == Type::boolean
            || type == Type::string || type == Type::color || type == Type::point
            || type == Type::rectangle;
    }
};

//==============================================================================
/** Builds a store-backed color property descriptor mapping a `.yui` name to a Style color id. */
inline ComponentProperty makeColorProperty (Identifier name, Identifier storageId, String description = {})
{
    return ComponentProperty {
        .name = std::move (name),
        .storageId = std::move (storageId),
        .type = ComponentProperty::Type::color,
        .description = std::move (description)
    };
}

/** Builds a store-backed number property descriptor mapping a `.yui` name to a Style id, such as the
    curated geometry read by the theme. An undefined default leaves the choice to the paint code.
*/
inline ComponentProperty makeNumberProperty (Identifier name, Identifier storageId, var defaultValue, String description = {})
{
    return ComponentProperty {
        .name = std::move (name),
        .storageId = std::move (storageId),
        .type = ComponentProperty::Type::number,
        .defaultValue = std::move (defaultValue),
        .description = std::move (description)
    };
}

} // namespace yup
