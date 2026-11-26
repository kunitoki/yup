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

    Use it in three steps. Call @ref install before compiling to register the symbol
    resolver on an ExpressionContext, so property names resolve to their types. Once
    compiled, call @ref bindInputs to record the program's resolved inputs alongside its
    ExpressionState, and on each evaluation call @ref updateInputs to push the component's
    current property values into the state.

    Only the property types an expression input can carry are bridged: number, integer,
    boolean, string, color, point, rectangle, and arrays of those (read with `x[i]` and
    `len (x)`). Colors, points and rectangles require the graphics expression types to be
    registered on the context (see addGraphicsExpressionTypes); other property types are
    left unresolved, so referencing them is a compile error.

    An array a component declares as a series (see ComponentTypeBuilder::series) is read where
    the component keeps it: the state views its elements, and nothing is copied or converted.
    Any other array is a var, converted to the elements the expression reads. That is done for
    every update, or once for all the updates of a round when the rounds are told with
    @ref beginUpdates, as a style evaluating many programs for one paint does.

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
    struct ParsedInput;

public:
    //==============================================================================
    /** Turns the prefix of a symbol such as `knob` in `knob.value` into a component.

        Return nullptr when the name is not known, so the symbol stays unresolved.
    */
    using ComponentResolver = std::function<Component*(const String& name)>;

    //==============================================================================
    /** One property of one component that an expression reads. */
    struct Dependency
    {
        /** The component the property is read from. */
        Component* component = nullptr;

        /** The name of the property that is read. */
        Identifier property;
    };

    //==============================================================================
    /** The typed values an array property feeds an expression input, kept alive until the next update.

        An expression reads an array input as a view of the values it is bound to, so they have to
        outlive the evaluation. This holds one converted copy of them, for the inputs reading it.
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

        /** Converts a var holding an array and keeps its values, without binding them.

            Convert once and @ref attach the values to every input reading them, rather than
            converting them again for each.

            @param elementType   The type of the elements, which must be bridged.
            @param value         The value of the property, normally an array var.
        */
        void assign (ComponentProperty::Type elementType, const var& value);

        /** Binds the values kept to an input of a state.

            Values of another type than the one the program of the state reads leave the input with
            nothing to read. The values have to stay as they are while the state is evaluated.

            @param state   The state the values are bound to.
            @param input   The index of the array input.
        */
        void attach (ExpressionState& state, int input) const;

    private:
        std::variant<Array<float>, Array<int>, Array<bool>, Array<String>, Array<Color>, Array<Point<float>>, Array<Rectangle<float>>, Array<AffineTransform>, Array<Font>> values;
    };

    //==============================================================================
    /** Pre-parsed inputs for one compiled program, stored next to its ExpressionState. */
    class YUP_API BoundInputs
    {
    public:
        BoundInputs() = default;

        /** Returns true when the program reads no scope symbols. */
        bool isEmpty() const noexcept { return inputs.empty(); }

        /** Returns true when the program reads a palette role or `palette.dark`. */
        bool readsPalette() const noexcept { return paletteDependent; }

    private:
        friend class DrawingScope;

        std::vector<ParsedInput> inputs;
        bool paletteDependent = false;
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
        element or a BindingModel of the same document. Call this before @ref install.
    */
    void setComponentResolver (ComponentResolver resolver);

    /** Sets values read by their bare name before the properties of the component.

        The named arguments of a `.yui` handler are read this way: in `(width) => action ("w", width)`,
        `width` is the argument, not the property of the component. The component is not owned.
        Call this before @ref install.
    */
    void setLocals (Component* locals);

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

    /** Returns the revision of a series bound in this round of updates, told by where its elements are.

        What is made of a series and kept between two paints, such as the texture a shader samples
        it from, is made again when this changes (see ComponentArrayView::revision).

        @param elements   The address of the first element, as a state views it.

        @returns The revision, or 0 for elements that are no series bound since @ref beginUpdates, and
                 for a series whose revision is not tracked.
    */
    uint32 findSeriesRevision (const void* elements) const noexcept;

    /** Returns a part the type of the component declares, or nullptr when it declares none of this name.

        @see ComponentPart, ComponentTypeInfo::findPart
    */
    const ComponentPart* findPart (const Identifier& name) const;

    /** Returns the names of the parts the type of the component declares, the ones of its own type first. */
    StringArray getPartNames() const;

    /** Pre-parses the scope inputs referenced by @p program, skipping context-declared inputs.

        Store the returned value alongside the program's ExpressionState so array input
        views remain valid across evaluations without per-paint cache lookups.
    */
    BoundInputs bindInputs (const CompiledExpression& program) const;

    /** Writes the component's live property values into @p state before evaluation.

        An array a component gives as a series is bound where the component keeps it, so evaluate
        the state before the component changes it.

        @param inputs    Bound inputs previously created with @ref bindInputs.
        @param state     A state created from the same program.
    */
    void updateInputs (BoundInputs& inputs, ExpressionState& state) const;

    /** Starts a round of updates, during which the components are known to stay as they are.

        An array a component gives as a var is then read and converted once for the round, whichever
        inputs read it, and the states updated in the round view those values until the next one
        starts. Call it before updating the programs evaluated together, such as the ones of a
        drawing for one paint, and again whenever the components may have changed.

        A scope that is never told its rounds converts the arrays for every update.
    */
    void beginUpdates();

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
            case ComponentProperty::Type::transform: function (std::type_identity<AffineTransform>()); return true;
            case ComponentProperty::Type::font:      function (std::type_identity<Font>()); return true;

            default:                                 return false;
        }
    }

    //==============================================================================
    /** Registers on a context the array types the bridged property types make.

        Contexts copied from a prototype share the types registered on it, so registering them
        there keeps one type per element type across every expression compiled from it.
    */
    static void addExpressionArrayTypes (ExpressionContext& context);

    /** Writes one reflected property value into an expression input, using @p arrayInput for arrays. */
    static void setInputValue (ExpressionState& state,
                               int input,
                               ComponentProperty::Type type,
                               ComponentProperty::Type elementType,
                               const var& value,
                               ArrayInput& arrayInput);

    /** Binds the elements a component keeps for an array property to an expression input, without copying them.

        Elements of another type than the one the program of the state reads leave the input with
        nothing to read. The elements have to stay where they are while the state is evaluated.

        @param state         The state the elements are bound to.
        @param input         The index of the array input.
        @param elementType   The type of the elements, which must be bridged.
        @param view          The elements, as ComponentProperty::arrayGetter returns them.
    */
    static void bindArrayView (ExpressionState& state,
                               int input,
                               ComponentProperty::Type elementType,
                               const ComponentArrayView& view);

private:
    //==============================================================================
    struct ParsedInput
    {
        enum class Kind
        {
            invalid,
            paletteDark,
            paletteRole,
            paletteColor,
            themeDefaultFont,
            themeIconFont,
            themeMonospaceFont,
            themeDark,
            themePaletteRole,
            themeMember,
            selfProperty,
            parentProperty,
            bareProperty,
            prefixed
        };

        int index = 0;
        Kind kind = Kind::invalid;
        ThemePalette::Role paletteRole = ThemePalette::Role::surface;
        ComponentProperty::Type themeType = ComponentProperty::Type::any;
        bool isThemeColor = false;
        String prefix;
        Identifier property;
        ArrayInput arrayInput;
        mutable WeakReference<Component> cachedComponent;
        mutable uint32 cachedResolverGeneration = 0;

        bool isPaletteOrTheme() const noexcept
        {
            return kind == Kind::paletteDark
                || kind == Kind::paletteRole
                || kind == Kind::paletteColor
                || kind == Kind::themeDefaultFont
                || kind == Kind::themeIconFont
                || kind == Kind::themeMonospaceFont
                || kind == Kind::themeDark
                || kind == Kind::themePaletteRole
                || kind == Kind::themeMember;
        }
    };

    struct ResolvedProperty
    {
        Component* component = nullptr;
        const ComponentProperty* descriptor = nullptr;
    };

    /* The values of an array a component gave as a var, converted for the round they were read in. */
    struct ConvertedArray
    {
        const Component* component = nullptr;
        const ComponentProperty* descriptor = nullptr;
        uint32 round = 0;
        ArrayInput values;
    };

    static ParsedInput parse (const String& symbol, int index = 0);
    ResolvedProperty resolve (const ParsedInput& input) const;
    const ArrayInput& convertArray (const Component& source, const ComponentProperty& descriptor, const Identifier& property) const;

    WeakReference<Component> component;
    ExpressionContext& context;
    ComponentResolver componentResolver;
    WeakReference<Component> locals;
    uint32 resolverGeneration = 1;
    mutable std::vector<ConvertedArray> convertedArrays;
    mutable std::vector<ComponentArrayView> boundSeries;
    uint32 updateRound = 0;

    YUP_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrawingScope)
};

} // namespace yup
