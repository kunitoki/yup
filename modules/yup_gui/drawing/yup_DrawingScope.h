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

//==============================================================================
/**
    Resolves a component's reflected properties as expression symbols.

    A DrawingScope bridges the reflected component model and the expression compiler:
    it lets an expression reference a component's properties by name, so text such as
    `value * width` resolves against the live component. A bare name or the `self.`
    prefix refers to the bound component, while `parent.` refers to its parent. Any other
    prefix is passed to the resolver set with @ref setComponentResolver, which is how
    `knob.value` reads a property of another named element. A bare name the component
    doesn't declare reads the closest parent declaring it as an inherited store-backed
    property, which is how the tokens of a `.yui` document reach every component.

    Use it in two steps. Call @ref install before compiling to register the symbol
    resolver on an ExpressionContext, so property names resolve to their types. Then,
    on each evaluation, call @ref updateInputs to push the component's current property
    values into an ExpressionState.

    Only the property types an expression input can carry are bridged: number, integer,
    boolean, string, color, point, rectangle, and arrays of those (read with `x[i]` and
    `len (x)`). Colors, points and rectangles require the graphics expression types to be
    registered on the context (see addGraphicsExpressionTypes); other property types are
    left unresolved, so referencing them is a compile error.

    The roles of the palette the component is drawn with resolve as `palette.<role>` colors
    (for example `palette.accent` or `palette.textMuted`, see ThemePalette::Role), and
    `palette.dark` is true for a dark palette. The palette is the closest one set on the
    component or a parent, or else the global theme's (see ApplicationTheme::getPalette), so
    drawings follow palette switches and component palettes.

    @see ExpressionContext, CompiledExpression, ExpressionState, ComponentProperty, ThemePalette

    @tags{UI}
*/
class YUP_API DrawingScope
{
public:
    //==============================================================================
    /** Turns the prefix of a symbol such as `knob` in `knob.value` into a component.

        Return nullptr when the name is not known, so the symbol stays unresolved.
    */
    using ComponentResolver = std::function<Component*(const String& name)>;

    /** Turns the prefix of a symbol such as `params` in `params.cutoff` into a binding provider.

        Return nullptr when the name is not known, so the symbol stays unresolved.
    */
    using ModelResolver = std::function<BindingProvider*(const String& name)>;

    //==============================================================================
    /** One property of one component or binding provider that an expression reads. */
    struct Dependency
    {
        /** The component the property is read from, or nullptr when read from @ref provider. */
        Component* component = nullptr;

        /** The binding provider the property is read from, or nullptr when read from @ref component. */
        BindingProvider* provider = nullptr;

        /** The name of the property that is read. */
        Identifier property;
    };

    //==============================================================================
    /** Creates a scope over a component, resolved against a context.

        @param component   The component whose reflected properties become symbols.
        @param context     The context the symbols are resolved against. Register the
                           graphics expression types on it first for colors to resolve.
    */
    DrawingScope (Component& component, ExpressionContext& context);

    //==============================================================================
    /** Sets the resolver used for symbols prefixed with a name other than `self` or `parent`.

        With a resolver installed, `knob.value` reads the `value` property of the component
        the resolver returns for `knob`, which is how a drawing or a binding references another
        element of the same document. Call this before @ref install.
    */
    void setComponentResolver (ComponentResolver resolver);

    /** Sets the resolver used to look up BindingProvider models by prefix (such as `params` in `params.cutoff`).

        Component prefixes resolved via @ref setComponentResolver take precedence when both are set.
        Call this before @ref install.
    */
    void setModelResolver (ModelResolver resolver);

    //==============================================================================
    /** Registers the symbol resolver on the context, replacing any previous one.

        Call this before compiling an expression against the context.
    */
    void install();

    /** Returns the type a symbol resolves to, or nullptr when it names nothing in this scope.

        This is the answer the resolver registered by @ref install gives the compiler, so a
        program compiled for one scope compiles the same for any scope answering alike for
        every symbol it asked about.

        @param symbol   A symbol such as `value`, `parent.width` or `palette.accent`.
    */
    const ExpressionType* resolveSymbol (const String& symbol) const;

    /** Writes the component's live property values into a state before evaluation.

        For every input of the program that names a reflected property of the bound
        component (directly, or through `self.` or `parent.`), its effective value is
        written to the state. The effective value cascades like Component::findProperty:
        an instance override wins, otherwise the themed or default value is used. Inputs
        naming a palette role are written from the component's current palette. Inputs
        that match no property are left untouched, and a destroyed component is a no-op.

        The values of array inputs are kept in the scope until the same input is written
        again, since the expression reads them as a view.

        @param program   A program compiled against this scope's context.
        @param state     A state created from the same program.
    */
    void updateInputs (const CompiledExpression& program, ExpressionState& state) const;

    /** Returns the component properties the program reads, so changes to them can be tracked.

        Inputs that resolve to no component or to no reflected property are skipped, and so are
        palette inputs: use PropertyExpression::readsPalette for those.

        @param program   A program compiled against this scope's context.
    */
    std::vector<Dependency> getDependencies (const CompiledExpression& program) const;

    /** Returns the expression type a property type is bridged as, or nullptr when it isn't bridged.

        Number, integer, boolean, string, color, point and rectangle properties are bridged, as `float`,
        `int`, `bool`, `string`, `color`, `point` and `rect`. An array property is bridged as an array of
        its element type, which is registered on the context when it is asked for the first time.

        @param type          The type of a reflected property.
        @param elementType   The element type of an array property, ignored for any other type.
    */
    const ExpressionType* getExpressionType (ComponentProperty::Type type,
                                             ComponentProperty::Type elementType = ComponentProperty::Type::any) const;

    /** Calls a function with the C++ type a property type is bridged as, returning false when it isn't bridged.

        The function is called with a `std::type_identity<T>` of the bridged type, so one body can handle
        every type an expression carries:

        @code
        DrawingScope::withExpressionType (property.type, [&] (auto tag)
        {
            using T = typename decltype (tag)::type;
            state.set (ExpressionInput<T> { index }, VariantConverter<T>::fromVar (value));
        });
        @endcode
    */
    template <class Function>
    static bool withExpressionType (ComponentProperty::Type type, Function&& function)
    {
        switch (type)
        {
            case ComponentProperty::Type::number:    function (std::type_identity<float>()); return true;
            case ComponentProperty::Type::integer:   function (std::type_identity<int>()); return true;
            case ComponentProperty::Type::boolean:   function (std::type_identity<bool>()); return true;
            case ComponentProperty::Type::string:    function (std::type_identity<String>()); return true;
            case ComponentProperty::Type::color:     function (std::type_identity<Color>()); return true;
            case ComponentProperty::Type::point:     function (std::type_identity<Point<float>>()); return true;
            case ComponentProperty::Type::rectangle: function (std::type_identity<Rectangle<float>>()); return true;

            default:                                 return false;
        }
    }

    //==============================================================================
    /** Registers on a context the array types the bridged property types make.

        Contexts copied from a prototype share the types registered on it, so registering them
        there keeps one type per element type across every expression compiled from it.
    */
    static void addExpressionArrayTypes (ExpressionContext& context);

    //==============================================================================
    /** The typed values an array property feeds an expression input, kept alive until the next update.

        An expression reads an array input as a view of the values it is bound to, so they have to
        outlive the evaluation. This holds one converted copy of them for one input.
    */
    class YUP_API ArrayInput
    {
    public:
        /** Converts a var holding an array and binds it to an input of a state.

            A value that holds no array binds an empty array, so an unset property reads as empty.

            @param state         The state the values are bound to.
            @param input         The index of the array input.
            @param elementType   The type of the elements, which must be bridged.
            @param value         The value of the property, normally an array var.
        */
        void bind (ExpressionState& state, int input, ComponentProperty::Type elementType, const var& value);

    private:
        std::variant<Array<float>, Array<int>, Array<bool>, Array<String>, Array<Color>, Array<Point<float>>, Array<Rectangle<float>>> values;
    };

private:
    //==============================================================================
    struct ResolvedTarget
    {
        Component* component = nullptr;
        BindingProvider* provider = nullptr;
        Identifier property;
    };

    struct ParsedInput
    {
        enum class Kind
        {
            paletteDark,
            paletteRole,
            selfProperty,
            parentProperty,
            bareProperty,
            prefixed
        };

        int index = 0;
        Kind kind = Kind::bareProperty;
        ThemePalette::Role paletteRole = ThemePalette::Role::surface;
        String prefix;
        Identifier property;
        mutable ArrayInput arrayInput;
        mutable WeakReference<Component> cachedComponent;
        mutable WeakReference<BindingProvider> cachedProvider;
    };

    struct CachedProgram
    {
        CompiledExpression program;
        std::vector<ParsedInput> inputs;
        bool usesPalette = false;
    };

    const CachedProgram& getCachedProgram (const CompiledExpression& program) const;
    ResolvedTarget resolveTarget (const String& symbol) const;
    static std::optional<BindingProvider::Property> findProviderProperty (const BindingProvider& provider, const Identifier& name);
    const ExpressionType* typeForPaletteRole (const String& roleName) const;

    WeakReference<Component> component;
    ExpressionContext& context;
    ComponentResolver componentResolver;
    ModelResolver modelResolver;
    mutable std::vector<std::unique_ptr<CachedProgram>> cachedPrograms;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawingScope)
};

} // namespace yup
