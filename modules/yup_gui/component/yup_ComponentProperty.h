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
    The elements of an array property, read where the component keeps them.

    A component that already holds the values of an array property in a contiguous block gives a
    view of them, instead of copying them into a var for every reader: a drawing then reads the
    levels of a meter or the points of a curve in place, with nothing allocated or converted (see
    ComponentTypeBuilder::series).

    The view is only valid until the component changes or frees the elements, so it is read and
    used at once, on the message thread, and never kept.

    @see ComponentProperty::arrayGetter, ComponentTypeBuilder::series

    @tags{UI}
*/
struct ComponentArrayView
{
    /** The first element, of the C++ type ComponentProperty::elementType is bridged as: `float` for a
        number, `int` for an integer, `bool`, String, Color, Point<float>, Rectangle<float>,
        AffineTransform or Font. Null when there is none.
    */
    const void* data = nullptr;

    /** The number of elements. */
    int size = 0;

    /** A number that changes when the elements do, or 0 when the component doesn't track it.

        What keeps something made of the elements between two paints, such as the texture a shader
        samples them from, compares it to know when to make it again. With 0 the elements have to be
        compared themselves.
    */
    uint32 revision = 0;

    /** Returns the elements as a span of the type they are of, which the caller has to know. */
    template <class ElementType>
    Span<const ElementType> as() const noexcept
    {
        return { static_cast<const ElementType*> (data), static_cast<size_t> (jmax (0, size)) };
    }
};

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
        transform,   /**< A 2D affine transform. */
        enumeration, /**< One of a fixed set of string values (see @ref enumValues). */
        font,        /**< A font. */
        drawing,     /**< The name of a drawing or of a shader, as a `.yui` document declares them. */
        array,       /**< A list of values, stored as a var array (see @ref elementType). */
        any          /**< An untyped value. */
    };

    //==============================================================================
    /** Which layout needs updating when this property changes on a component. */
    enum class LayoutEffect
    {
        none,  /**< Visual or non-layout property. */
        own,   /**< Container layout property (`padding`, `gap`, `direction`, ...). */
        parent /**< Child layout item property (`flexGrow`, `margin`, `gridArea`, ...). */
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

    /** Returns the elements of an @ref Type::array property where the component keeps them, when it does.

        Null for most properties. When set, @ref elementType tells the type of the elements, and
        @ref getter gives the same values as a var for the readers that need one. A drawing and a
        shader read the view, so the values reach them without a copy.

        @see ComponentArrayView, ComponentTypeBuilder::series
    */
    std::function<ComponentArrayView (const Component&)> arrayGetter;

    /** True when this boolean property doubles as a pseudo-class (`:checked`, `:dragging`). */
    bool isState = false;

    /** True when a stylesheet may set this property (false for value, text, ...). */
    bool isStyleable = true;

    /** True when a store-backed value set on a component also applies to its descendants.

        Colors and curated geometry are inherited. Layout item properties such as `flexGrow` describe
        one component only, so @ref Component::findProperty doesn't look them up in the parents.
    */
    bool isInherited = true;

    /** Which layout needs updating when this property changes on a component. */
    LayoutEffect layoutEffect = LayoutEffect::none;

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

    /** Returns true when the elements of the array are read where the component keeps them (see @ref arrayGetter). */
    bool isSeries() const noexcept { return getter != nullptr && arrayGetter != nullptr; }

    /** Returns the key used to store a store-backed value: @ref storageId, or @ref name when unset. */
    const Identifier& getStorageKey() const noexcept { return storageId.isValid() ? storageId : name; }

    /** Returns the scalar and special type names recognized in `.yui` property declarations. */
    static const StringArray& getDeclaredTypeNames()
    {
        static const StringArray names { "number", "integer", "boolean", "string", "color", "point", "rectangle", "transform", "font", "drawing", "any" };
        return names;
    }

    /** Returns the type a `.yui` declaration names: `number`, `integer`, `boolean`, `string`, `color`,
        `point`, `rectangle`, `transform`, `font` or `drawing`, Type::array for an array of a scalar type such as
        `string[]`, and Type::any for any other name.
    */
    static Type getTypeFromName (const String& typeName)
    {
        static constexpr Type scalarTypes[] = {
            Type::number, Type::integer, Type::boolean, Type::string, Type::color,
            Type::point, Type::rectangle, Type::transform, Type::font, Type::drawing, Type::any
        };

        if (typeName.endsWith ("[]"))
            return getElementTypeFromName (typeName) != Type::any ? Type::array : Type::any;

        const auto index = getDeclaredTypeNames().indexOf (typeName);
        return index >= 0 ? scalarTypes[index] : Type::any;
    }

    /** Returns the name of a type, the one @ref getTypeFromName reads as it: `enumeration` and
        `array` for the two types a `.yui` declaration doesn't name that way.
    */
    static String getTypeName (Type type)
    {
        if (type == Type::enumeration)
            return "enumeration";

        if (type == Type::array)
            return "array";

        for (const auto& name : getDeclaredTypeNames())
            if (getTypeFromName (name) == type)
                return name;

        return "any";
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

    /** The scalar types an expression reads and writes. */
    static constexpr std::array<Type, 9> expressionTypes {
        Type::number,
        Type::integer,
        Type::boolean,
        Type::string,
        Type::color,
        Type::point,
        Type::rectangle,
        Type::transform,
        Type::font
    };

    /** Returns true for the scalar types an expression reads and writes.

        These are number, integer, boolean, string, color, point, rectangle, transform and font. They are the types a
        value expression can produce, a drawing can declare, and an array can hold.
    */
    static constexpr bool isExpressionType (Type type) noexcept
    {
        for (const auto candidate : expressionTypes)
            if (candidate == type)
                return true;

        return false;
    }
};

} // namespace yup
