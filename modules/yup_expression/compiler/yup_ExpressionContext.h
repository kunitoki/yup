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

class ExpressionContext;

//==============================================================================
/** Typed handle to an input declared with ExpressionContext::addInput, used to set values on an ExpressionState. */
template <class T>
struct ExpressionInput
{
    /** The index of the input in the context, which is also its index in the programs compiled with it. */
    int index = -1;

    /** Returns true if the handle refers to an input. */
    bool isValid() const noexcept { return index >= 0; }
};

/** Typed handle to an array input declared with ExpressionContext::addArray. */
template <class T>
using ExpressionArray = ExpressionInput<ExpressionArrayView<T>>;

//==============================================================================
/** Describes a custom C++ type to the expression language.

    Returned by ExpressionContext::addType, every method returns the builder so calls can be chained:

    @code
    context.addType<Point<float>> ("point")
        .constructor ([] (float x, float y) { return Point<float> (x, y); })
        .member ("x", [] (const Point<float>& p) { return p.getX(); })
        .member ("y", [] (const Point<float>& p) { return p.getY(); })
        .method ("distanceTo", [] (const Point<float>& a, const Point<float>& b) { return a.distanceTo (b); })
        .operation (ExpressionOperator::add, [] (const Point<float>& a, const Point<float>& b) { return a + b; })
        .equality()
        .components ({ "x", "y" });
    @endcode

    @tags{Core}
*/
template <class T>
class ExpressionTypeBuilder
{
public:
    /** Creates a builder adding functions to a context. */
    ExpressionTypeBuilder (ExpressionContext& context, ExpressionType& type) noexcept
        : context (context)
        , type (type)
    {
    }

    /** Adds a constructor, a function with the name of the type returning a T. */
    template <class F>
    ExpressionTypeBuilder& constructor (F&& function)
    {
        return constructor (type.getName(), std::forward<F> (function));
    }

    /** Adds a named constructor (for example `rgb`) returning a T. */
    template <class F>
    ExpressionTypeBuilder& constructor (const String& name, F&& function);

    /** Adds a member read `value.name`, implemented by a getter taking a `const T&` or by a pointer to a public data member. */
    template <class F>
    ExpressionTypeBuilder& member (const String& name, F&& getter);

    /** Adds a method `value.name (args...)`, implemented by a callable taking the object as first parameter. */
    template <class F>
    ExpressionTypeBuilder& method (const String& name, F&& function);

    /** Adds an operator, implemented by a callable taking one (negate, logicalNot) or two parameters. */
    template <class F>
    ExpressionTypeBuilder& operation (ExpressionOperator op, F&& function);

    /** Adds `==` and `!=` using the C++ equality operators of T. */
    ExpressionTypeBuilder& equality()
    {
        operation (ExpressionOperator::equal, [] (const T& a, const T& b) { return static_cast<bool> (a == b); });
        return operation (ExpressionOperator::notEqual, [] (const T& a, const T& b) { return static_cast<bool> (a != b); });
    }

    /** Adds an implicit conversion, implemented by a callable taking a T and returning the converted value. */
    template <class F>
    ExpressionTypeBuilder& conversion (F&& function);

    /** Declares the component members used for swizzling.

        With components `x` and `y` (both registered as members), `p.yx` is compiled as
        `point (p.y, p.x)` using the constructor taking one value per component.
    */
    ExpressionTypeBuilder& components (const StringArray& names)
    {
        type.componentNames = names;
        return *this;
    }

    /** Returns the type being described. */
    const ExpressionType& getType() const noexcept { return type; }

private:
    ExpressionContext& context;
    ExpressionType& type;
};

//==============================================================================
/** The symbols, types and functions available to expressions.

    A context declares everything a program can reference:
    - types: the built-in `bool`, `int`, `float`, `double`, `string` and custom types (addType);
    - inputs: values provided at evaluation time through an ExpressionState (addInput, addArray);
    - constants, enumerations and named sub-expressions (addConstant, addEnum, addExpression);
    - functions (addFunction), which may overload or replace the built-in ones;
    - expected outputs and their types (declareOutput).

    The built-in functions are `sin cos tan asin acos atan atan2 sqrt pow exp log log2 log10 abs
    floor ceil round trunc sign fmod min max clamp lerp mix map smoothstep hypot deg rad len format`, the casts
    `int float double bool string`, and the constants `pi e tau`. `format (pattern, a [, b [, c]])`
    formats up to three numbers with the printf conversions `%d %i %f %e %g` (flags, width and
    precision included) and `%%`, other conversions are copied as text.

    A context is only needed while compiling: compiled programs keep what they use alive.

    @code
    ExpressionContext context;
    auto width = context.addInput<float> ("width");
    context.addFunction ("double", [] (float x) { return x * 2.0f; });

    auto program = CompiledExpression::compile ("double (width) + 1", context);
    @endcode

    @see CompiledExpression, ExpressionState

    @tags{Core}
*/
class YUP_API ExpressionContext
{
public:
    //==============================================================================
    /** Creates a context with the built-in types and functions. */
    ExpressionContext();

    ExpressionContext (const ExpressionContext&);
    ExpressionContext (ExpressionContext&&) noexcept;
    ExpressionContext& operator= (const ExpressionContext&);
    ExpressionContext& operator= (ExpressionContext&&) noexcept;
    ~ExpressionContext();

    //==============================================================================
    /** Registers a custom C++ type under a name, returning a builder describing its operations.

        If T was already registered the existing type is returned.
    */
    template <class T>
    ExpressionTypeBuilder<T> addType (const String& name)
    {
        jassert (ExpressionType::getBuiltin<T>() == nullptr); // Built-in types can't be registered again

        if (auto* existing = findTypeIndex (typeid (T)))
            return ExpressionTypeBuilder<T> (*this, const_cast<ExpressionType&> (*existing));

        types.push_back (ExpressionType::create<T> (name));
        return ExpressionTypeBuilder<T> (*this, *types.back());
    }

    /** Returns the type registered for the C++ type T (built-in, custom or array), or nullptr. */
    template <class T>
    const ExpressionType* getType() const
    {
        if (auto* builtin = ExpressionType::getBuiltin<T>())
            return builtin;

        return findTypeIndex (typeid (T));
    }

    /** Returns the type with the given name, or nullptr. */
    const ExpressionType* findType (StringRef name) const;

    /** Registers a C++ enumeration as a type whose values are named constants.

        Each value can be written qualified as `typeName.valueName`, or as the bare `valueName` when no
        other symbol has that name (bare names are looked up after the symbol resolver). A bare name
        declared by more than one enumeration is ambiguous and has to be qualified, unless it is assigned
        to a declared output of one of those enumerations. The type supports `==` and `!=`, and the returned builder adds more.
        Registering the same enumeration again skips the values it already has.

        @code
        enum class Cap { butt, round, square };

        context.addEnum<Cap> ("cap", { { "butt", Cap::butt }, { "round", Cap::round }, { "square", Cap::square } });
        context.declareOutput<Cap> ("strokeCap");

        // strokeCap = hovered ? round : cap.butt;
        @endcode
    */
    template <class E>
    ExpressionTypeBuilder<E> addEnum (const String& name, std::initializer_list<std::pair<String, E>> values)
    {
        static_assert (std::is_enum_v<E>, "addEnum needs an enumeration type");

        auto builder = addType<E> (name);
        builder.equality();

        const auto* enumType = getType<E>();

        for (const auto& [valueName, value] : values)
        {
            auto box = enumType->createValue();
            *static_cast<E*> (box.get()) = value;
            addEnumValue (enumType->getName(), valueName, *enumType, std::move (box));
        }

        return builder;
    }

    //==============================================================================
    /** Declares an input of type T, set on an ExpressionState before evaluating. */
    template <class T>
    ExpressionInput<T> addInput (const String& name)
    {
        const auto* inputType = resolveType<T>();
        jassert (inputType != nullptr); // Register the type with addType first

        return { inputType != nullptr ? addInput (name, *inputType) : -1 };
    }

    /** Declares an array input with elements of type T, bound with ExpressionState::bindArray.

        Expressions read elements with `name[index]` (the index is clamped, an empty array gives
        a default constructed value) and the size with `len (name)`.
    */
    template <class T>
    ExpressionArray<T> addArray (const String& name)
    {
        return addInput<ExpressionArrayView<T>> (name);
    }

    /** Declares an input of a type known at runtime, returning its index or -1 if the name is already used. */
    int addInput (const String& name, const ExpressionType& type);

    /** Returns the number of inputs. */
    int getNumInputs() const noexcept { return (int) inputs.size(); }

    /** Returns the name of an input. */
    const String& getInputName (int index) const;

    /** Returns the type of an input. */
    const ExpressionType* getInputType (int index) const;

    /** Returns the index of the input with the given name, or -1. */
    int findInput (StringRef name) const;

    //==============================================================================
    /** Declares a named constant, folded into the programs using it. */
    template <class T>
    void addConstant (const String& name, const T& value)
    {
        const auto* constantType = resolveType<T>();
        jassert (constantType != nullptr); // Register the type with addType first

        if (constantType == nullptr)
            return;

        auto box = constantType->createValue();
        *static_cast<T*> (box.get()) = value;
        addConstant (name, *constantType, std::move (box));
    }

    /** Declares a named sub-expression, inlined wherever the name is referenced.

        The expression can only reference context symbols (not the locals of the program using it).
    */
    void addExpression (const String& name, const Expression& expression);

    /** Parses and declares a named sub-expression. */
    Result addExpression (const String& name, StringRef text);

    //==============================================================================
    /** Adds a function implemented by a callable, deducing its signature.

        Overloads with different parameter types can be added under the same name, a function with
        the same signature as an existing or built-in one replaces it.

        Functions marked as pure (the default) must only depend on their arguments, which allows the
        compiler to fold calls with constant arguments. Functions must never throw.

        @code
        context.addFunction ("square", [] (float x) { return x * x; });
        context.addFunction ("random", [&random] { return random.nextFloat(); }, false);
        @endcode
    */
    template <class F>
    void addFunction (const String& name, F&& function, bool isPure = true)
    {
        addFunction (detail::makeExpressionFunction (name, std::forward<F> (function), isPure, [this] (auto tag)
        {
            return resolveType<typename decltype (tag)::type>();
        }));
    }

    /** Adds a function from an existing descriptor. */
    void addFunction (ExpressionFunction function);

    //==============================================================================
    /** Declares the expected type of an output: assigning the output converts the value implicitly. */
    template <class T>
    void declareOutput (const String& name)
    {
        const auto* outputType = resolveType<T>();
        jassert (outputType != nullptr); // Register the type with addType first

        if (outputType != nullptr)
            declareOutput (name, *outputType);
    }

    /** Declares the expected type of an output. */
    void declareOutput (const String& name, const ExpressionType& type);

    //==============================================================================
    /** A callback returning the type of a symbol the context doesn't declare, or nullptr if it is unknown. */
    using SymbolResolver = std::function<const ExpressionType* (const String& name)>;

    /** Sets a callback asked about unknown symbols while compiling, so symbols can be declared lazily.

        Declared symbols always win. For a dotted name like `theme.font.height` the resolver is
        asked for `theme.font.height`, then `theme.font`, then `theme`, and the remaining segments
        are member accesses. A resolved name becomes an input of the compiled program only: find it
        with CompiledExpression::getInput and set it on the ExpressionState like any other input.
        The returned type must be built-in or registered in this context.

        @code
        context.setSymbolResolver ([&] (const String& name) -> const ExpressionType*
        {
            return model.hasProperty (name) ? context.getType<float>() : nullptr;
        });
        @endcode
    */
    void setSymbolResolver (SymbolResolver resolver);

private:
    //==============================================================================
    friend struct ExpressionCompiler;

    template <class>
    friend class ExpressionTypeBuilder;

    enum class SymbolKind
    {
        input,
        constant,
        expression
    };

    struct Symbol
    {
        SymbolKind kind;
        int index;
    };

    struct Constant
    {
        const ExpressionType* type;
        std::shared_ptr<void> value;
    };

    struct Variable
    {
        String name;
        const ExpressionType* type;
    };

    using FunctionMap = std::unordered_map<String, std::vector<ExpressionFunction>>;

    template <class T>
    struct ArrayElement
    {
        using Type = void;
    };

    template <class T>
    struct ArrayElement<ExpressionArrayView<T>>
    {
        using Type = T;
    };

    template <class T>
    const ExpressionType* resolveType()
    {
        if constexpr (! std::is_void_v<typename ArrayElement<T>::Type>)
        {
            using Element = typename ArrayElement<T>::Type;

            if (auto* existing = getType<T>())
                return existing;

            const auto* elementType = resolveType<Element>();

            if (elementType == nullptr)
                return nullptr;

            types.push_back (ExpressionType::createArray<Element> (*elementType));
            return types.back().get();
        }
        else
        {
            return getType<T>();
        }
    }

    const ExpressionType* findTypeIndex (std::type_index type) const;
    void addConstant (const String& name, const ExpressionType& type, std::shared_ptr<void> value);
    void addEnumValue (const String& typeName, const String& valueName, const ExpressionType& type, std::shared_ptr<void> value);
    bool addSymbol (const String& name, SymbolKind kind, int index);
    const Symbol* findSymbol (const String& name) const;
    const ExpressionType* findOutputType (const String& name) const;

    static const FunctionMap& getBuiltinFunctions();

    std::vector<std::shared_ptr<ExpressionType>> types;
    std::vector<Variable> inputs;
    std::vector<Variable> outputs;
    std::vector<Constant> constants;
    std::vector<std::pair<String, Expression>> expressions;
    std::unordered_map<String, Symbol> symbols;
    std::unordered_map<String, std::vector<int>> enumValues;
    FunctionMap functions;
    SymbolResolver symbolResolver;
};

//==============================================================================
template <class T>
template <class F>
ExpressionTypeBuilder<T>& ExpressionTypeBuilder<T>::constructor (const String& name, F&& function)
{
    context.addFunction (name, std::forward<F> (function));
    return *this;
}

template <class T>
template <class F>
ExpressionTypeBuilder<T>& ExpressionTypeBuilder<T>::member (const String& name, F&& getter)
{
    if constexpr (std::is_member_object_pointer_v<std::decay_t<F>>)
        context.addFunction ("." + name, [getter] (const T& value) { return value.*getter; });
    else
        context.addFunction ("." + name, std::forward<F> (getter));

    return *this;
}

template <class T>
template <class F>
ExpressionTypeBuilder<T>& ExpressionTypeBuilder<T>::method (const String& name, F&& function)
{
    context.addFunction ("." + name, std::forward<F> (function));
    return *this;
}

template <class T>
template <class F>
ExpressionTypeBuilder<T>& ExpressionTypeBuilder<T>::operation (ExpressionOperator op, F&& function)
{
    context.addFunction (Expression::getOperatorFunctionName (op), std::forward<F> (function));
    return *this;
}

template <class T>
template <class F>
ExpressionTypeBuilder<T>& ExpressionTypeBuilder<T>::conversion (F&& function)
{
    context.addFunction ("@convert", std::forward<F> (function));
    return *this;
}

} // namespace yup
