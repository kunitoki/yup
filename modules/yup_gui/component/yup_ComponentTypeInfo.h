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
    Describes a component type for the reflected component model.

    A ComponentTypeInfo carries the type name (for example "Slider"), a pointer to
    its base type's info, the property descriptors declared on the type, the signals
    it emits (events such as "onValueChanged"), and an optional factory used to
    create instances from `.yui`.

    Each widget returns a single shared instance from Component::getDefaultTypeInfo(),
    and a single component can be given its own with Component::setTypeInfo(). Type
    infos form a chain through @ref getBaseTypeInfo, so property and signal lookups
    walk up to the base. Unregistered user subclasses simply inherit the info of the
    nearest registered base.

    @see Component, ComponentProperty, ComponentTypeRegistry

    @tags{UI}
*/
class YUP_API ComponentTypeInfo
{
public:
    //==============================================================================
    /** A factory that creates a component instance of this type. */
    using Factory = std::function<std::unique_ptr<Component>()>;

    /** Adds a child to a component of this type.

        Containers that do more than parent their children declare one, so a Viewport takes its
        child as the viewed component and a TabComponent turns it into a page. Without one the
        child is simply made a visible child.
    */
    using ChildAttacher = std::function<void (Component& parent, Component& child)>;

    //==============================================================================
    /** Creates a type info.

        @param typeName        The type name, for example "Slider".
        @param baseTypeInfo    The base type's info, or nullptr for the root type.
        @param properties      The property descriptors declared on this type.
        @param signals         The signals (events) this type emits.
        @param factory         An optional factory used to instantiate this type.
        @param childAttacher   An optional way to add a child, inherited by the derived types.
    */
    ComponentTypeInfo (Identifier typeName,
                       const ComponentTypeInfo* baseTypeInfo,
                       std::vector<ComponentProperty> properties,
                       std::vector<ComponentSignal> signals = {},
                       Factory factory = {},
                       ChildAttacher childAttacher = {});

    //==============================================================================
    /** Returns the type name, for example "Slider". */
    const Identifier& getTypeName() const noexcept { return typeName; }

    /** Returns the base type's info, or nullptr for the root type. */
    const ComponentTypeInfo* getBaseTypeInfo() const noexcept { return baseTypeInfo; }

    //==============================================================================
    /** Returns the property descriptors declared directly on this type (not inherited). */
    const std::vector<ComponentProperty>& getOwnProperties() const noexcept { return properties; }

    /** Finds a property by name, walking up the base chain.

        A property declared on a derived type overrides one with the same name on a base.

        @param name    The property name to find.

        @return The matching descriptor, or nullptr if the type does not declare it.
    */
    const ComponentProperty* findProperty (const Identifier& name) const;

    /** Finds a store-backed property by its storage key, walking up the base chain.

        @param storageKey    The storage id (or name) a store-backed property uses.

        @return The matching descriptor, or nullptr if none is found.
    */
    const ComponentProperty* findPropertyByStorageKey (const Identifier& storageKey) const;

    /** Returns true when a property with the given name is declared on this type or a base. */
    bool hasProperty (const Identifier& name) const { return findProperty (name) != nullptr; }

    /** Returns every property visible on this type, with derived declarations overriding base ones. */
    std::vector<const ComponentProperty*> getAllProperties() const;

    //==============================================================================
    /** Returns true when this type is, or derives from, a type with the given name. */
    bool isSubtypeOf (const Identifier& otherTypeName) const;

    /** Returns true when this type is, or derives from, the given type. */
    bool isSubtypeOf (const ComponentTypeInfo& other) const;

    //==============================================================================
    /** Returns the signals (events) declared directly on this type (not inherited). */
    const std::vector<ComponentSignal>& getOwnSignals() const noexcept { return signals; }

    /** Finds a signal by name, walking up the base chain.

        @param signalName    The signal name to find.

        @return The matching descriptor, or nullptr if the type does not emit it.
    */
    const ComponentSignal* findSignal (const Identifier& signalName) const;

    /** Returns true when this type or a base emits the given signal. */
    bool hasSignal (const Identifier& signalName) const { return findSignal (signalName) != nullptr; }

    //==============================================================================
    /** Returns true when a factory is available to instantiate this type. */
    bool hasFactory() const noexcept { return factory != nullptr; }

    /** Creates a new instance of this type, or nullptr when no factory is available. */
    std::unique_ptr<Component> createInstance() const;

    //==============================================================================
    /** Returns true when this type or a base adds its children in its own way. */
    bool hasChildAttacher() const noexcept;

    /** Adds a child to a parent of this type, the way the type wants it.

        @param parent    The parent, which must be of this type.
        @param child     The child to add.
    */
    void attachChild (Component& parent, Component& child) const;

private:
    //==============================================================================
    void rebuildIndices();

    Identifier typeName;
    const ComponentTypeInfo* baseTypeInfo = nullptr;
    std::vector<ComponentProperty> properties;
    std::unordered_map<Identifier, const ComponentProperty*> mergedPropertyIndex;
    std::unordered_map<Identifier, const ComponentProperty*> storageKeyIndex;
    std::vector<ComponentSignal> signals;
    Factory factory;
    ChildAttacher childAttacher;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ComponentTypeInfo)
};

#ifndef DOXYGEN
namespace detail
{

template <class ValueType>
struct ComponentPropertyValueTraits;

template <>
struct ComponentPropertyValueTraits<bool>
{
    static constexpr auto type = ComponentProperty::Type::boolean;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (bool v) { return var (v); }
    static bool fromVar (const var& v) { return static_cast<bool> (v); }
};

template <>
struct ComponentPropertyValueTraits<int>
{
    static constexpr auto type = ComponentProperty::Type::integer;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (int v) { return var (v); }
    static int fromVar (const var& v) { return static_cast<int> (v); }
};

template <>
struct ComponentPropertyValueTraits<float>
{
    static constexpr auto type = ComponentProperty::Type::number;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (float v) { return var (static_cast<double> (v)); }
    static float fromVar (const var& v) { return static_cast<float> (static_cast<double> (v)); }
};

template <>
struct ComponentPropertyValueTraits<double>
{
    static constexpr auto type = ComponentProperty::Type::number;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (double v) { return var (v); }
    static double fromVar (const var& v) { return static_cast<double> (v); }
};

template <>
struct ComponentPropertyValueTraits<String>
{
    static constexpr auto type = ComponentProperty::Type::string;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (const String& v) { return var (v); }
    static String fromVar (const var& v) { return v.toString(); }
};

template <>
struct ComponentPropertyValueTraits<StringArray>
{
    static constexpr auto type = ComponentProperty::Type::array;
    static constexpr auto elementType = ComponentProperty::Type::string;
    static var toVar (const StringArray& v) { return VariantConverter<StringArray>::toVar (v); }
    static StringArray fromVar (const var& v) { return VariantConverter<StringArray>::fromVar (v); }
};

template <>
struct ComponentPropertyValueTraits<Color>
{
    static constexpr auto type = ComponentProperty::Type::color;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (const Color& v) { return VariantConverter<Color>::toVar (v); }
    static Color fromVar (const var& v) { return VariantConverter<Color>::fromVar (v); }
};

template <>
struct ComponentPropertyValueTraits<Point<float>>
{
    static constexpr auto type = ComponentProperty::Type::point;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (Point<float> v) { return VariantConverter<Point<float>>::toVar (v); }
    static Point<float> fromVar (const var& v) { return VariantConverter<Point<float>>::fromVar (v); }
};

template <>
struct ComponentPropertyValueTraits<Rectangle<float>>
{
    static constexpr auto type = ComponentProperty::Type::rectangle;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (const Rectangle<float>& v) { return VariantConverter<Rectangle<float>>::toVar (v); }
    static Rectangle<float> fromVar (const var& v) { return VariantConverter<Rectangle<float>>::fromVar (v); }
};

template <>
struct ComponentPropertyValueTraits<AffineTransform>
{
    static constexpr auto type = ComponentProperty::Type::transform;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (const AffineTransform& v) { return VariantConverter<AffineTransform>::toVar (v); }
    static AffineTransform fromVar (const var& v) { return VariantConverter<AffineTransform>::fromVar (v); }
};

template <>
struct ComponentPropertyValueTraits<Font>
{
    static constexpr auto type = ComponentProperty::Type::font;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (const Font& v) { return VariantConverter<Font>::toVar (v); }
    static Font fromVar (const var& v) { return VariantConverter<Font>::fromVar (v); }
};

} // namespace detail
#endif

//==============================================================================
/** Builds a signal descriptor bound to a `std::function<void (Args...)>` member on @p T. */
template <class T, class U, class... Args>
    requires std::derived_from<T, Component> && std::derived_from<T, U>
ComponentSignal makeSignal (Identifier name,
                            std::function<void (Args...)> U::* member,
                            String description = {},
                            std::vector<ComponentSignal::Argument> arguments = {})
{
    return ComponentSignal {
        .name = std::move (name),
        .connect = [member] (Component& component, ComponentSignal::Handler handler)
        {
            if (auto* target = dynamic_cast<T*> (&component))
            {
                target->*member = [handler = std::move (handler)] (Args... args)
                {
                    handler ({ detail::ComponentPropertyValueTraits<std::decay_t<Args>>::toVar (args)... });
                };
            }
        },
        .description = std::move (description),
        .arguments = std::move (arguments)
    };
}

//==============================================================================
/**
    Fluent builder for constructing a @ref ComponentTypeInfo for component type @p T.

    Automatically binds @ref Component::getClassTypeInfo as the default base type,
    supplies `std::make_unique<T>` as the default factory for concrete default-constructible
    components, infers @ref ComponentProperty::Type from getter return types, and defaults
    state-backed properties to non-styleable.

    @tags{UI}
*/
template <class T>
    requires std::derived_from<T, Component>
class ComponentTypeBuilder
{
public:
    explicit ComponentTypeBuilder (Identifier name)
        : typeName (std::move (name))
        , baseTypeInfo (std::is_same_v<T, Component> ? nullptr : &Component::getClassTypeInfo())
    {
        initDefaultFactory();
    }

    ComponentTypeBuilder (Identifier name, const ComponentTypeInfo& base)
        : typeName (std::move (name))
        , baseTypeInfo (&base)
    {
        initDefaultFactory();
    }

    ComponentTypeBuilder& factory (ComponentTypeInfo::Factory customFactory)
    {
        typeFactory = std::move (customFactory);
        return *this;
    }

    ComponentTypeBuilder& childAttacher (ComponentTypeInfo::ChildAttacher customChildAttacher)
    {
        typeChildAttacher = std::move (customChildAttacher);
        return *this;
    }

    ComponentTypeBuilder& addProperty (ComponentProperty prop)
    {
        properties.push_back (std::move (prop));
        return *this;
    }

    ComponentTypeBuilder& color (Identifier name, Identifier storageId, String description = {})
    {
        return addProperty (ComponentProperty {
            .name = std::move (name),
            .storageId = std::move (storageId),
            .type = ComponentProperty::Type::color,
            .description = std::move (description) });
    }

    ComponentTypeBuilder& number (Identifier name, Identifier storageId, var defaultValue = {}, String description = {})
    {
        return addProperty (ComponentProperty {
            .name = std::move (name),
            .storageId = std::move (storageId),
            .type = ComponentProperty::Type::number,
            .defaultValue = std::move (defaultValue),
            .description = std::move (description) });
    }

    ComponentTypeBuilder& layoutProperty (Identifier name,
                                          ComponentProperty::Type type,
                                          var defaultValue = {},
                                          ComponentProperty::LayoutEffect layoutEffect = ComponentProperty::LayoutEffect::own,
                                          String description = {},
                                          StringArray enumValues = {})
    {
        return addProperty (ComponentProperty {
            .name = std::move (name),
            .type = type,
            .defaultValue = std::move (defaultValue),
            .isInherited = false,
            .layoutEffect = layoutEffect,
            .enumValues = std::move (enumValues),
            .description = std::move (description) });
    }

    template <class Getter, class DefaultType>
    static constexpr bool isValidDefaultValue =
        ! std::invocable<DefaultType, T&, std::decay_t<std::invoke_result_t<Getter, const T&>>>
        && ! std::is_member_function_pointer_v<std::decay_t<DefaultType>>
        && (std::constructible_from<var, DefaultType>
            || std::convertible_to<DefaultType, std::decay_t<std::invoke_result_t<Getter, const T&>>>);

    template <class Getter, class Setter, class DefaultType = var>
        requires std::invocable<Setter, T&, std::decay_t<std::invoke_result_t<Getter, const T&>>>
              && isValidDefaultValue<Getter, DefaultType>
    ComponentTypeBuilder& property (Identifier name,
                                    Getter getter,
                                    Setter setter,
                                    DefaultType&& defaultValue = {},
                                    String description = {})
    {
        return addTypedProperty (std::move (name),
                                 std::move (getter),
                                 std::move (setter),
                                 toDefaultVar<Getter> (std::forward<DefaultType> (defaultValue)),
                                 std::move (description),
                                 false,
                                 false);
    }

    template <class Getter, class Setter, class DefaultType = var>
        requires std::invocable<Setter, T&, std::decay_t<std::invoke_result_t<Getter, const T&>>>
              && isValidDefaultValue<Getter, DefaultType>
    ComponentTypeBuilder& styleableProperty (Identifier name,
                                             Getter getter,
                                             Setter setter,
                                             DefaultType&& defaultValue = {},
                                             String description = {})
    {
        return addTypedProperty (std::move (name),
                                 std::move (getter),
                                 std::move (setter),
                                 toDefaultVar<Getter> (std::forward<DefaultType> (defaultValue)),
                                 std::move (description),
                                 false,
                                 true);
    }

    template <class Getter, class DefaultType = var>
        requires isValidDefaultValue<Getter, DefaultType>
    ComponentTypeBuilder& readOnly (Identifier name,
                                    Getter getter,
                                    DefaultType&& defaultValue = {},
                                    String description = {})
    {
        using ValueType = std::decay_t<std::invoke_result_t<Getter, const T&>>;
        using Traits = detail::ComponentPropertyValueTraits<ValueType>;
        auto resolvedDefault = toDefaultVar<Getter> (std::forward<DefaultType> (defaultValue));

        return addProperty (ComponentProperty {
            .name = std::move (name),
            .type = Traits::type,
            .elementType = Traits::elementType,
            .defaultValue = std::move (resolvedDefault),
            .getter = [getter = std::move (getter)] (const Component& c)
            {
                return Traits::toVar (std::invoke (getter, static_cast<const T&> (c)));
            },
            .isStyleable = false,
            .description = std::move (description) });
    }

    template <class Getter, class Setter, class DefaultType = var>
        requires std::invocable<Setter, T&, std::decay_t<std::invoke_result_t<Getter, const T&>>>
              && isValidDefaultValue<Getter, DefaultType>
    ComponentTypeBuilder& state (Identifier name,
                                 Getter getter,
                                 Setter setter,
                                 DefaultType&& defaultValue = {},
                                 String description = {})
    {
        return addTypedProperty (std::move (name),
                                 std::move (getter),
                                 std::move (setter),
                                 toDefaultVar<Getter> (std::forward<DefaultType> (defaultValue)),
                                 std::move (description),
                                 true,
                                 false);
    }

    template <class Getter, class DefaultType = var>
        requires isValidDefaultValue<Getter, DefaultType>
    ComponentTypeBuilder& state (Identifier name,
                                 Getter getter,
                                 DefaultType&& defaultValue = {},
                                 String description = {})
    {
        using ValueType = std::decay_t<std::invoke_result_t<Getter, const T&>>;
        using Traits = detail::ComponentPropertyValueTraits<ValueType>;
        auto resolvedDefault = toDefaultVar<Getter> (std::forward<DefaultType> (defaultValue));

        return addProperty (ComponentProperty {
            .name = std::move (name),
            .type = Traits::type,
            .elementType = Traits::elementType,
            .defaultValue = std::move (resolvedDefault),
            .getter = [getter = std::move (getter)] (const Component& c)
            {
                return Traits::toVar (std::invoke (getter, static_cast<const T&> (c)));
            },
            .isState = true,
            .isStyleable = false,
            .description = std::move (description) });
    }

    template <class Getter, class Setter>
    ComponentTypeBuilder& enumeration (Identifier name,
                                       StringArray names,
                                       Getter getter,
                                       Setter setter,
                                       String description = {})
    {
        using EnumType = std::decay_t<std::invoke_result_t<Getter, const T&>>;
        var defaultValue = names.isEmpty() ? var() : var (names[0]);

        return addProperty (ComponentProperty {
            .name = std::move (name),
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = std::move (defaultValue),
            .getter = [names, getter = std::move (getter)] (const Component& c)
            {
                const auto index = static_cast<int> (std::invoke (getter, static_cast<const T&> (c)));
                return isPositiveAndBelow (index, names.size()) ? var (names[index]) : var (String());
            },
            .setter = [names, setter = std::move (setter)] (Component& c, const var& v)
            {
                if (const auto index = names.indexOf (v.toString()); index >= 0)
                    std::invoke (setter, static_cast<T&> (c), static_cast<EnumType> (index));
            },
            .isStyleable = false,
            .enumValues = std::move (names),
            .description = std::move (description) });
    }

    template <class EnumType, class Getter, class Setter>
    ComponentTypeBuilder& enumeration (Identifier name,
                                       std::initializer_list<std::pair<const char*, EnumType>> entries,
                                       Getter getter,
                                       Setter setter,
                                       String description = {})
    {
        StringArray names;
        std::vector<EnumType> values;
        values.reserve (entries.size());

        for (const auto& [entryName, entryValue] : entries)
        {
            names.add (entryName);
            values.push_back (entryValue);
        }

        var defaultValue = names.isEmpty() ? var() : var (names[0]);

        return addProperty (ComponentProperty {
            .name = std::move (name),
            .type = ComponentProperty::Type::enumeration,
            .defaultValue = std::move (defaultValue),
            .getter = [names, values, getter = std::move (getter)] (const Component& c)
            {
                const auto current = std::invoke (getter, static_cast<const T&> (c));
                for (std::size_t i = 0; i < values.size(); ++i)
                    if (values[i] == current)
                        return var (names[static_cast<int> (i)]);

                return var (String());
            },
            .setter = [names, values, setter = std::move (setter)] (Component& c, const var& v)
            {
                if (const auto index = names.indexOf (v.toString()); index >= 0)
                    std::invoke (setter, static_cast<T&> (c), values[static_cast<std::size_t> (index)]);
            },
            .isStyleable = false,
            .enumValues = std::move (names),
            .description = std::move (description) });
    }

    ComponentTypeBuilder& signal (ComponentSignal sig)
    {
        signals.push_back (std::move (sig));
        return *this;
    }

    template <class U, class... Args>
        requires std::derived_from<T, U>
    ComponentTypeBuilder& signal (Identifier name,
                                  std::function<void (Args...)> U::* member,
                                  String description = {},
                                  std::vector<ComponentSignal::Argument> arguments = {})
    {
        return signal (makeSignal<T> (std::move (name), member, std::move (description), std::move (arguments)));
    }

    ComponentTypeInfo build()
    {
        return ComponentTypeInfo {
            std::move (typeName),
            baseTypeInfo,
            std::move (properties),
            std::move (signals),
            std::move (typeFactory),
            std::move (typeChildAttacher)
        };
    }

private:
    void initDefaultFactory()
    {
        if constexpr (! std::is_abstract_v<T> && std::is_default_constructible_v<T>)
            typeFactory = [] () -> std::unique_ptr<Component> { return std::make_unique<T>(); };
    }

    template <class Getter, class DefaultType>
    static var toDefaultVar (DefaultType&& defaultValue)
    {
        using ValueType = std::decay_t<std::invoke_result_t<Getter, const T&>>;
        using Traits = detail::ComponentPropertyValueTraits<ValueType>;

        if constexpr (std::is_same_v<std::decay_t<DefaultType>, var>)
            return std::forward<DefaultType> (defaultValue);
        else if constexpr (std::convertible_to<DefaultType, ValueType>)
            return Traits::toVar (static_cast<ValueType> (std::forward<DefaultType> (defaultValue)));
        else
            return var (std::forward<DefaultType> (defaultValue));
    }

    template <class Getter, class Setter>
    ComponentTypeBuilder& addTypedProperty (Identifier name,
                                            Getter getter,
                                            Setter setter,
                                            var defaultValue,
                                            String description,
                                            bool isState,
                                            bool isStyleable)
    {
        using ValueType = std::decay_t<std::invoke_result_t<Getter, const T&>>;
        using Traits = detail::ComponentPropertyValueTraits<ValueType>;

        return addProperty (ComponentProperty {
            .name = std::move (name),
            .type = Traits::type,
            .elementType = Traits::elementType,
            .defaultValue = std::move (defaultValue),
            .getter = [getter = std::move (getter)] (const Component& c)
            {
                return Traits::toVar (std::invoke (getter, static_cast<const T&> (c)));
            },
            .setter = [setter = std::move (setter)] (Component& c, const var& v)
            {
                std::invoke (setter, static_cast<T&> (c), Traits::fromVar (v));
            },
            .isState = isState,
            .isStyleable = isStyleable,
            .description = std::move (description) });
    }

    Identifier typeName;
    const ComponentTypeInfo* baseTypeInfo = nullptr;
    std::vector<ComponentProperty> properties;
    std::vector<ComponentSignal> signals;
    ComponentTypeInfo::Factory typeFactory;
    ComponentTypeInfo::ChildAttacher typeChildAttacher;
};

} // namespace yup
