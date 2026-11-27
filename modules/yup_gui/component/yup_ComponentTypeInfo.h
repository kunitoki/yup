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
    it emits (events such as "onValueChanged"), the parts of what it shows that a
    style can place (see ComponentPart), and an optional factory used to create
    instances from `.yui`.

    Each widget returns a single shared instance from Component::getDefaultTypeInfo(),
    and a single component can be given its own with Component::setTypeInfo(). Type
    infos form a chain through @ref getBaseTypeInfo, so property and signal lookups
    walk up to the base. Unregistered user subclasses simply inherit the info of the
    nearest registered base.

    @see Component, ComponentProperty, ComponentPart, ComponentTypeRegistry

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
        @param parts           The parts of what this type paints that a style can place.
        @param description     A human readable description of the type, surfaced by the schema and tooling.
        @param classType       The class the type describes, as `typeid (Slider)`, or `typeid (void)` for a
                               type describing no class of its own (see @ref getClassType).
    */
    ComponentTypeInfo (Identifier typeName,
                       const ComponentTypeInfo* baseTypeInfo,
                       std::vector<ComponentProperty> properties,
                       std::vector<ComponentSignal> signals = {},
                       Factory factory = {},
                       ChildAttacher childAttacher = {},
                       std::vector<ComponentPart> parts = {},
                       String description = {},
                       std::type_index classType = typeid (void));

    //==============================================================================
    /** Returns the type name, for example "Slider". */
    const Identifier& getTypeName() const noexcept { return typeName; }

    /** Returns the description of the type, surfaced by the schema and tooling, empty when it has none. */
    const String& getDescription() const noexcept { return description; }

    /** Returns the base type's info, or nullptr for the root type. */
    const ComponentTypeInfo* getBaseTypeInfo() const noexcept { return baseTypeInfo; }

    /** Returns the class the type describes, or `typeid (void)` for a type describing no class of its own.

        A type made with ComponentTypeBuilder describes the class it was built for. One made for
        single components, as the type of a `.yui` composite is, describes none: its components
        are of the class of a type it derives from. A theme follows it to the style of a class
        (see ApplicationTheme::findComponentStyle).
    */
    std::type_index getClassType() const noexcept { return classType; }

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

    /** Returns true when this type is, or derives from, the given type.

        The types are compared by identity, following the base types: another type info of the same
        name is another type, whose accessors may be those of another class.
    */
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
    /** Returns the parts declared directly on this type (not inherited). */
    const std::vector<ComponentPart>& getOwnParts() const noexcept { return parts; }

    /** Finds a part by name, walking up the base chain.

        A part declared on a derived type overrides one with the same name on a base.

        @param partName    The part name to find.

        @return The matching descriptor, or nullptr if the type declares no such part.
    */
    const ComponentPart* findPart (const Identifier& partName) const;

    /** Returns true when this type or a base declares the given part. */
    bool hasPart (const Identifier& partName) const { return findPart (partName) != nullptr; }

    /** Returns every part visible on this type, with derived declarations overriding base ones. */
    std::vector<const ComponentPart*> getAllParts() const;

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
    std::vector<ComponentPart> parts;
    Factory factory;
    ChildAttacher childAttacher;
    String description;
    std::type_index classType;

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
struct ComponentPropertyValueTraits<var>
{
    static constexpr auto type = ComponentProperty::Type::any;
    static constexpr auto elementType = ComponentProperty::Type::any;
    static var toVar (const var& v) { return v; }
    static var fromVar (const var& v) { return v; }
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

/* The type of the elements a getter of a series returns a span of: only a Span of const values has one. */
template <class SpanType>
struct ComponentSeriesElement;

template <class ElementType>
struct ComponentSeriesElement<Span<const ElementType>>
{
    using type = ElementType;
};

/* True for the C++ types the elements of an array are bridged as, which is what a view of them is read as. */
template <class ElementType>
inline constexpr bool isComponentSeriesElement = std::is_same_v<ElementType, float>
                                              || std::is_same_v<ElementType, int>
                                              || std::is_same_v<ElementType, bool>
                                              || std::is_same_v<ElementType, String>
                                              || std::is_same_v<ElementType, Color>
                                              || std::is_same_v<ElementType, Point<float>>
                                              || std::is_same_v<ElementType, Rectangle<float>>
                                              || std::is_same_v<ElementType, AffineTransform>
                                              || std::is_same_v<ElementType, Font>;

} // namespace detail
#endif

//==============================================================================
/** Builds a signal descriptor bound to a `std::function<void (Args...)>` member on @p T.

    Connecting a null handler disconnects the signal, leaving the member null as on a new component.
*/
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
            auto* target = dynamic_cast<T*> (&component);

            if (target == nullptr)
                return;

            if (handler == nullptr)
            {
                target->*member = nullptr;
                return;
            }

            target->*member = [handler = std::move (handler)] (Args... args)
            {
                handler ({ detail::ComponentPropertyValueTraits<std::decay_t<Args>>::toVar (args)... });
            };
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

    /** Describes the type, for the schema and tooling. */
    ComponentTypeBuilder& description (String text)
    {
        typeDescription = std::move (text);
        return *this;
    }

    ComponentTypeBuilder& addProperty (ComponentProperty prop)
    {
        // A name declared twice hides its first declaration: a getter of the second reading the first by name calls itself
        jassert (std::none_of (properties.begin(), properties.end(), [&prop] (const ComponentProperty& known) { return known.name == prop.name; }));

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

    /** Declares a read-only array property whose elements are read where the component keeps them.

        The getter returns a `Span<const ElementType>` over elements the component holds in one block,
        of the type an array of them is bridged as: `float`, `int`, `bool`, String, Color, Point<float>,
        Rectangle<float>, AffineTransform or Font. A drawing reading the property binds that memory,
        so the values of a meter, a spectrum or a list of items reach it without being copied or
        converted, however often it is painted. Whatever needs a var still gets one, made of the same
        elements.

        The elements have to stay where they are while the component is painted: change them
        between two paints, on the message thread, and repaint the component.

        Several series of the same length describe a list of things with more than one value each,
        read with one index: `itemBounds[i]`, `itemTexts[i]`.

        @code
        .series ("levels", &Analyzer::getLevels, &Analyzer::getLevelsRevision, "The level of each band, 0 to 1.")
        @endcode

        @param name          The name of the property.
        @param getter        Returns the span of the elements, for example `Span<const float> (Analyzer::*)() const`.
        @param description   A human readable description, surfaced by the schema and tooling.

        @see ComponentArrayView, ComponentProperty::arrayGetter
    */
    template <class Getter>
        requires detail::isComponentSeriesElement<typename detail::ComponentSeriesElement<std::invoke_result_t<Getter, const T&>>::type>
    ComponentTypeBuilder& series (Identifier name, Getter getter, String description = {})
    {
        return series (std::move (name), std::move (getter), [] (const T&) -> uint32 { return 0; }, std::move (description));
    }

    /** Declares a read-only array property read in place, with a number telling when its elements change.

        As the overload without it, with a revision: a number the component changes whenever it changes
        the elements. What keeps something made of them between two paints, such as the texture a shader
        samples a series from, then knows when to make it again without comparing the elements.

        @param name             The name of the property.
        @param getter           Returns the span of the elements.
        @param revisionGetter   Returns the revision, for example `uint32 (Analyzer::*)() const`. It is never
                                0 once the elements changed: 0 means the revision is not tracked.
        @param description      A human readable description, surfaced by the schema and tooling.
    */
    template <class Getter, class RevisionGetter>
        requires detail::isComponentSeriesElement<typename detail::ComponentSeriesElement<std::invoke_result_t<Getter, const T&>>::type>
              && std::is_invocable_r_v<uint32, RevisionGetter, const T&>
    ComponentTypeBuilder& series (Identifier name, Getter getter, RevisionGetter revisionGetter, String description = {})
    {
        using ElementType = typename detail::ComponentSeriesElement<std::invoke_result_t<Getter, const T&>>::type;
        using Traits = detail::ComponentPropertyValueTraits<ElementType>;

        return addProperty (ComponentProperty {
            .name = std::move (name),
            .type = ComponentProperty::Type::array,
            .elementType = Traits::type,
            .getter = [getter] (const Component& c)
            {
                Array<var> values;

                for (const auto& element : std::invoke (getter, static_cast<const T&> (c)))
                    values.add (Traits::toVar (element));

                return var (values);
            },
            .arrayGetter = [getter, revisionGetter = std::move (revisionGetter)] (const Component& c)
            {
                const auto& component = static_cast<const T&> (c);
                const Span<const ElementType> elements = std::invoke (getter, component);

                return ComponentArrayView { elements.data(), static_cast<int> (elements.size()), static_cast<uint32> (std::invoke (revisionGetter, component)) };
            },
            .isStyleable = false,
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

    ComponentTypeBuilder& addPart (ComponentPart part)
    {
        parts.push_back (std::move (part));
        return *this;
    }

    /** Declares a part of what the component shows, painted in the area it is given (see ComponentPart).

        A drawing can move such a part with `bounds`, so declare this way what fits any area: an icon,
        an image. Without `bounds` it is painted in the bounds of the component. The theme paints the
        part, see ApplicationTheme::setComponentPart.

        @param name          The name of the part.
        @param description   A human readable description, surfaced by the schema and tooling.
    */
    ComponentTypeBuilder& part (Identifier name, String description = {})
    {
        return addPart (ComponentPart {
            .name = std::move (name),
            .description = std::move (description) });
    }

    /** Declares a part painted in the area it is given, which the component puts in an area of its own.

        As the overload without it, with where the component puts the part itself: a drawing giving
        the part no `bounds` paints it there.

        @param name          The name of the part.
        @param area          Returns where the component puts it, for example `Rectangle<float> (T::*)() const`.
        @param description   A human readable description, surfaced by the schema and tooling.
    */
    template <class AreaGetter>
        requires std::is_invocable_r_v<Rectangle<float>, AreaGetter, const T&>
    ComponentTypeBuilder& part (Identifier name, AreaGetter area, String description = {})
    {
        return addPart (ComponentPart {
            .name = std::move (name),
            .area = makePartArea (std::move (area)),
            .description = std::move (description) });
    }

    /** Declares a part the component lays out itself, painted where it puts it and nowhere else.

        Declare this way what the component placed for more than painting it: a text it laid out for
        an area, a caret and a selection that follow that text, what the pointer is tested against. A
        drawing decides when such a part is painted and with which paint, and can't give it `bounds`.

        @param name          The name of the part.
        @param area          Returns where the component puts it, for example `Rectangle<float> (T::*)() const`.
        @param description   A human readable description, surfaced by the schema and tooling.
    */
    template <class AreaGetter>
        requires std::is_invocable_r_v<Rectangle<float>, AreaGetter, const T&>
    ComponentTypeBuilder& fixedPart (Identifier name, AreaGetter area, String description = {})
    {
        return addPart (ComponentPart {
            .name = std::move (name),
            .area = makePartArea (std::move (area)),
            .isPlaceable = false,
            .description = std::move (description) });
    }

    /** Declares a part the component lays out itself, in its own bounds. */
    ComponentTypeBuilder& fixedPart (Identifier name, String description = {})
    {
        return addPart (ComponentPart {
            .name = std::move (name),
            .isPlaceable = false,
            .description = std::move (description) });
    }

    ComponentTypeInfo build()
    {
        return ComponentTypeInfo {
            std::move (typeName),
            baseTypeInfo,
            std::move (properties),
            std::move (signals),
            std::move (typeFactory),
            std::move (typeChildAttacher),
            std::move (parts),
            std::move (typeDescription),
            typeid (T)
        };
    }

private:
    void initDefaultFactory()
    {
        if constexpr (! std::is_abstract_v<T> && std::is_default_constructible_v<T>)
            typeFactory = [] () -> std::unique_ptr<Component> { return std::make_unique<T>(); };
    }

    template <class AreaGetter>
    static ComponentPart::AreaGetter makePartArea (AreaGetter area)
    {
        return [area = std::move (area)] (const Component& c) -> Rectangle<float>
        {
            return std::invoke (area, static_cast<const T&> (c));
        };
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
    std::vector<ComponentPart> parts;
    ComponentTypeInfo::Factory typeFactory;
    ComponentTypeInfo::ChildAttacher typeChildAttacher;
    String typeDescription;
};

} // namespace yup
