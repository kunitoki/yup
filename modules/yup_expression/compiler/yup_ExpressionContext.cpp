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

namespace yup
{

//==============================================================================
String ExpressionFunction::getSignature() const
{
    StringArray parameters;

    for (const auto* parameterType : parameterTypes)
        parameters.add (parameterType->getName());

    return name + " (" + parameters.joinIntoString (", ") + ") -> " + resultType->getName();
}

//==============================================================================
namespace
{

struct ExpressionBuiltinFunctions
{
    ExpressionBuiltinFunctions()
    {
        addFloating<float>();
        addFloating<double>();
        addInteger();
        addLogical();
        addStrings();
        addCasts();
    }

    template <class F>
    void add (const String& name, F&& function)
    {
        auto result = detail::makeExpressionFunction (name, std::forward<F> (function), true, [] (auto tag)
        {
            return ExpressionType::getBuiltin<typename decltype (tag)::type>();
        });

        jassert (result.resultType != nullptr);
        functions[name].push_back (std::move (result));
    }

    template <class F>
    void add (ExpressionOperator op, F&& function)
    {
        add (Expression::getOperatorFunctionName (op), std::forward<F> (function));
    }

    template <class T>
    void addOrdering()
    {
        add (ExpressionOperator::equal, [] (const T& a, const T& b) { return a == b; });
        add (ExpressionOperator::notEqual, [] (const T& a, const T& b) { return a != b; });
        add (ExpressionOperator::less, [] (const T& a, const T& b) { return a < b; });
        add (ExpressionOperator::lessOrEqual, [] (const T& a, const T& b) { return a <= b; });
        add (ExpressionOperator::greater, [] (const T& a, const T& b) { return a > b; });
        add (ExpressionOperator::greaterOrEqual, [] (const T& a, const T& b) { return a >= b; });
    }

    template <class T>
    void addFloating()
    {
        add (ExpressionOperator::add, [] (T a, T b) { return a + b; });
        add (ExpressionOperator::subtract, [] (T a, T b) { return a - b; });
        add (ExpressionOperator::multiply, [] (T a, T b) { return a * b; });
        add (ExpressionOperator::divide, [] (T a, T b) { return a / b; });
        add (ExpressionOperator::modulo, [] (T a, T b) { return std::fmod (a, b); });
        add (ExpressionOperator::power, [] (T a, T b) { return std::pow (a, b); });
        add (ExpressionOperator::negate, [] (T a) { return -a; });
        addOrdering<T>();

        add ("sin", [] (T x) { return std::sin (x); });
        add ("cos", [] (T x) { return std::cos (x); });
        add ("tan", [] (T x) { return std::tan (x); });
        add ("asin", [] (T x) { return std::asin (x); });
        add ("acos", [] (T x) { return std::acos (x); });
        add ("atan", [] (T x) { return std::atan (x); });
        add ("atan2", [] (T y, T x) { return std::atan2 (y, x); });
        add ("sqrt", [] (T x) { return std::sqrt (x); });
        add ("pow", [] (T x, T y) { return std::pow (x, y); });
        add ("exp", [] (T x) { return std::exp (x); });
        add ("log", [] (T x) { return std::log (x); });
        add ("log2", [] (T x) { return std::log2 (x); });
        add ("log10", [] (T x) { return std::log10 (x); });
        add ("abs", [] (T x) { return std::abs (x); });
        add ("floor", [] (T x) { return std::floor (x); });
        add ("ceil", [] (T x) { return std::ceil (x); });
        add ("round", [] (T x) { return std::round (x); });
        add ("trunc", [] (T x) { return std::trunc (x); });
        add ("sign", [] (T x) { return (T) ((x > 0) - (x < 0)); });
        add ("fmod", [] (T x, T y) { return std::fmod (x, y); });
        add ("hypot", [] (T x, T y) { return std::hypot (x, y); });
        add ("min", [] (T a, T b) { return b < a ? b : a; });
        add ("max", [] (T a, T b) { return a < b ? b : a; });
        add ("clamp", [] (T x, T low, T high) { return x < low ? low : (high < x ? high : x); });
        add ("lerp", [] (T a, T b, T t) { return a + (b - a) * t; });
        add ("mix", [] (T a, T b, T t) { return a + (b - a) * t; });
        add ("deg", [] (T x) { return x * (T) (180.0 / MathConstants<double>::pi); });
        add ("rad", [] (T x) { return x * (T) (MathConstants<double>::pi / 180.0); });

        add ("map", [] (T x, T inLow, T inHigh, T outLow, T outHigh)
        {
            return inHigh == inLow ? outLow : outLow + (x - inLow) * (outHigh - outLow) / (inHigh - inLow);
        });

        add ("smoothstep", [] (T edge0, T edge1, T x)
        {
            if (edge1 == edge0)
                return x < edge0 ? (T) 0 : (T) 1;

            const auto t = std::clamp ((x - edge0) / (edge1 - edge0), (T) 0, (T) 1);
            return t * t * ((T) 3 - (T) 2 * t);
        });
    }

    void addInteger()
    {
        add (ExpressionOperator::add, [] (int a, int b) { return (int) ((uint32) a + (uint32) b); });
        add (ExpressionOperator::subtract, [] (int a, int b) { return (int) ((uint32) a - (uint32) b); });
        add (ExpressionOperator::multiply, [] (int a, int b) { return (int) ((uint32) a * (uint32) b); });
        add (ExpressionOperator::negate, [] (int a) { return (int) (0u - (uint32) a); });

        add (ExpressionOperator::divide, [] (int a, int b)
        {
            if (b == 0)
                return 0;

            return b == -1 ? (int) (0u - (uint32) a) : a / b;
        });

        add (ExpressionOperator::modulo, [] (int a, int b) { return b == 0 || b == -1 ? 0 : a % b; });

        add (ExpressionOperator::power, [] (int base, int exponent)
        {
            if (exponent < 0)
                return base == 1 ? 1 : (base == -1 ? ((exponent & 1) != 0 ? -1 : 1) : 0);

            uint32 result = 1, factor = (uint32) base;

            for (auto e = (uint32) exponent; e != 0; e >>= 1, factor *= factor)
                if ((e & 1) != 0)
                    result *= factor;

            return (int) result;
        });

        addOrdering<int>();

        add ("abs", [] (int x) { return x < 0 ? (int) (0u - (uint32) x) : x; });
        add ("sign", [] (int x) { return (x > 0) - (x < 0); });
        add ("min", [] (int a, int b) { return std::min (a, b); });
        add ("max", [] (int a, int b) { return std::max (a, b); });
        add ("clamp", [] (int x, int low, int high) { return x < low ? low : (high < x ? high : x); });
    }

    void addLogical()
    {
        add (ExpressionOperator::equal, [] (bool a, bool b) { return a == b; });
        add (ExpressionOperator::notEqual, [] (bool a, bool b) { return a != b; });
        add (ExpressionOperator::logicalAnd, [] (bool a, bool b) { return a && b; });
        add (ExpressionOperator::logicalOr, [] (bool a, bool b) { return a || b; });
        add (ExpressionOperator::logicalNot, [] (bool a) { return ! a; });
    }

    void addStrings()
    {
        add (ExpressionOperator::add, [] (const String& a, const String& b) { return a + b; });
        add (ExpressionOperator::equal, [] (const String& a, const String& b) { return a == b; });
        add (ExpressionOperator::notEqual, [] (const String& a, const String& b) { return a != b; });
        add (ExpressionOperator::less, [] (const String& a, const String& b) { return a.compare (b) < 0; });
        add (ExpressionOperator::lessOrEqual, [] (const String& a, const String& b) { return a.compare (b) <= 0; });
        add (ExpressionOperator::greater, [] (const String& a, const String& b) { return a.compare (b) > 0; });
        add (ExpressionOperator::greaterOrEqual, [] (const String& a, const String& b) { return a.compare (b) >= 0; });
        add ("len", [] (const String& s) { return s.length(); });
        add ("format", [] (const String& f, double a) { return formatNumbers (f, { a }); });
        add ("format", [] (const String& f, double a, double b) { return formatNumbers (f, { a, b }); });
        add ("format", [] (const String& f, double a, double b, double c) { return formatNumbers (f, { a, b, c }); });
    }

    // printf-like formatting restricted to numeric conversions, so user formats can never read invalid arguments.
    static String formatNumbers (const String& format, std::initializer_list<double> values)
    {
        const auto source = format.toStdString();
        const auto isFlag = [&source] (size_t i) { return i < source.size() && source[i] != 0 && std::strchr ("-+ #0", source[i]) != nullptr; };
        const auto isDigit = [&source] (size_t i) { return i < source.size() && source[i] >= '0' && source[i] <= '9'; };

        auto next = values.begin();
        std::string result;

        for (size_t i = 0; i < source.size(); ++i)
        {
            if (source[i] != '%')
            {
                result += source[i];
                continue;
            }

            auto end = i + 1;

            while (isFlag (end))
                ++end;

            for (const auto widthStart = end; isDigit (end) && end - widthStart < 2;)
                ++end;

            const auto widthEnd = end;

            if (end < source.size() && source[end] == '.')
                for (const auto precisionStart = ++end; isDigit (end) && end - precisionStart < 2;)
                    ++end;

            const auto conversion = end < source.size() ? source[end] : 0;

            if (conversion == '%' && end == i + 1)
            {
                result += '%';
                i = end;
                continue;
            }

            if (conversion == 0 || std::strchr ("dieEfFgG", conversion) == nullptr)
            {
                result += '%';
                continue;
            }

            const auto value = next != values.end() ? *next++ : 0.0;
            const auto isInteger = conversion == 'd' || conversion == 'i';
            const auto spec = isInteger ? source.substr (i, widthEnd - i) + ".0f" : source.substr (i, end + 1 - i);

            result += String::formatted (String (spec), isInteger ? std::trunc (value) + 0.0 : value).toStdString();
            i = end;
        }

        return String::fromUTF8 (result.data(), (int) result.size());
    }

    template <class T>
    static int saturateToInt (T x) noexcept
    {
        if (x != x)
            return 0;

        if (x >= (T) 2147483647.0)
            return std::numeric_limits<int>::max();

        if (x <= (T) -2147483648.0)
            return std::numeric_limits<int>::min();

        return (int) x;
    }

    void addCasts()
    {
        add ("@convert", [] (bool x) { return (int) x; });
        add ("@convert", [] (bool x) { return (float) x; });
        add ("@convert", [] (bool x) { return (double) x; });
        add ("@convert", [] (int x) { return (float) x; });
        add ("@convert", [] (int x) { return (double) x; });
        add ("@convert", [] (float x) { return (double) x; });
        add ("@convert", [] (double x) { return (float) x; });

        add ("bool", [] (bool x) { return x; });
        add ("bool", [] (int x) { return x != 0; });
        add ("bool", [] (float x) { return x != 0.0f; });
        add ("bool", [] (double x) { return x != 0.0; });

        add ("int", [] (bool x) { return (int) x; });
        add ("int", [] (int x) { return x; });
        add ("int", [] (float x) { return saturateToInt (x); });
        add ("int", [] (double x) { return saturateToInt (x); });
        add ("int", [] (const String& x) { return x.getIntValue(); });

        add ("float", [] (bool x) { return (float) x; });
        add ("float", [] (int x) { return (float) x; });
        add ("float", [] (float x) { return x; });
        add ("float", [] (double x) { return (float) x; });
        add ("float", [] (const String& x) { return x.getFloatValue(); });

        add ("double", [] (bool x) { return (double) x; });
        add ("double", [] (int x) { return (double) x; });
        add ("double", [] (float x) { return (double) x; });
        add ("double", [] (double x) { return x; });
        add ("double", [] (const String& x) { return x.getDoubleValue(); });

        add ("string", [] (bool x) { return String (x ? "true" : "false"); });
        add ("string", [] (int x) { return String (x); });
        add ("string", [] (float x) { return String (x); });
        add ("string", [] (double x) { return String (x); });
        add ("string", [] (const String& x) { return x; });
        add ("string", [] (float x, int decimals) { return String (x, jmax (0, decimals)); });
        add ("string", [] (double x, int decimals) { return String (x, jmax (0, decimals)); });
    }

    std::unordered_map<String, std::vector<ExpressionFunction>> functions;
};

} // namespace

//==============================================================================
ExpressionContext::ExpressionContext() = default;
ExpressionContext::ExpressionContext (const ExpressionContext&) = default;
ExpressionContext::ExpressionContext (ExpressionContext&&) noexcept = default;
ExpressionContext& ExpressionContext::operator= (const ExpressionContext&) = default;
ExpressionContext& ExpressionContext::operator= (ExpressionContext&&) noexcept = default;
ExpressionContext::~ExpressionContext() = default;

const ExpressionContext::FunctionMap& ExpressionContext::getBuiltinFunctions()
{
    static const ExpressionBuiltinFunctions builtins;
    return builtins.functions;
}

//==============================================================================
const ExpressionType* ExpressionContext::findTypeIndex (std::type_index type) const
{
    for (const auto& t : types)
        if (t->getCppType() == type)
            return t.get();

    return nullptr;
}

const ExpressionType* ExpressionContext::findType (StringRef name) const
{
    if (auto* builtin = ExpressionType::findBuiltin (name))
        return builtin;

    for (const auto& t : types)
        if (t->getName() == name)
            return t.get();

    return nullptr;
}

//==============================================================================
bool ExpressionContext::addSymbol (const String& name, SymbolKind kind, int index)
{
    if (name.isEmpty() || symbols.find (name) != symbols.end())
    {
        jassertfalse; // Every input, constant and sub-expression needs a unique name
        return false;
    }

    symbols.emplace (name, Symbol { kind, index });
    return true;
}

const ExpressionContext::Symbol* ExpressionContext::findSymbol (const String& name) const
{
    const auto it = symbols.find (name);
    return it != symbols.end() ? &it->second : nullptr;
}

int ExpressionContext::addInput (const String& name, const ExpressionType& type)
{
    if (! addSymbol (name, SymbolKind::input, (int) inputs.size()))
        return -1;

    inputs.push_back ({ name, &type });
    return (int) inputs.size() - 1;
}

const String& ExpressionContext::getInputName (int index) const
{
    jassert (isPositiveAndBelow (index, getNumInputs()));
    return inputs[(size_t) index].name;
}

const ExpressionType* ExpressionContext::getInputType (int index) const
{
    return isPositiveAndBelow (index, getNumInputs()) ? inputs[(size_t) index].type : nullptr;
}

int ExpressionContext::findInput (StringRef name) const
{
    const auto* symbol = findSymbol (name);
    return symbol != nullptr && symbol->kind == SymbolKind::input ? symbol->index : -1;
}

void ExpressionContext::addConstant (const String& name, const ExpressionType& type, std::shared_ptr<void> value)
{
    if (addSymbol (name, SymbolKind::constant, (int) constants.size()))
        constants.push_back ({ &type, std::move (value) });
}

void ExpressionContext::addEnumValue (const String& typeName, const String& valueName, const ExpressionType& type, std::shared_ptr<void> value)
{
    if (findSymbol (typeName + "." + valueName) != nullptr)
        return;

    const auto index = (int) constants.size();
    addConstant (typeName + "." + valueName, type, std::move (value));

    if ((int) constants.size() > index)
        enumValues[valueName].push_back (index);
}

void ExpressionContext::addExpression (const String& name, const Expression& expression)
{
    if (addSymbol (name, SymbolKind::expression, (int) expressions.size()))
        expressions.emplace_back (name, expression);
}

Result ExpressionContext::addExpression (const String& name, StringRef text)
{
    auto expression = Expression::parse (text);

    if (expression.failed())
        return Result::fail (expression.getErrorMessage());

    addExpression (name, expression.getReference());
    return Result::ok();
}

//==============================================================================
void ExpressionContext::addFunction (ExpressionFunction function)
{
    const bool hasValidTypes = function.resultType != nullptr
                            && std::find (function.parameterTypes.begin(), function.parameterTypes.end(), nullptr) == function.parameterTypes.end();

    jassert (hasValidTypes); // Register the parameter and result types with addType first

    if (! hasValidTypes || function.thunk == nullptr)
        return;

    auto& overloads = functions[function.name];

    for (auto& existing : overloads)
    {
        if (existing.hasSameSignature (function))
        {
            existing = std::move (function);
            return;
        }
    }

    overloads.push_back (std::move (function));
}

//==============================================================================
void ExpressionContext::declareOutput (const String& name, const ExpressionType& type)
{
    for (auto& output : outputs)
    {
        if (output.name == name)
        {
            output.type = &type;
            return;
        }
    }

    outputs.push_back ({ name, &type });
}

const ExpressionType* ExpressionContext::findOutputType (const String& name) const
{
    for (const auto& output : outputs)
        if (output.name == name)
            return output.type;

    return nullptr;
}

//==============================================================================
void ExpressionContext::setSymbolResolver (SymbolResolver resolver)
{
    symbolResolver = std::move (resolver);
}

} // namespace yup
