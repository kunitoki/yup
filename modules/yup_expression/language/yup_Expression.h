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
/** The operators supported by the expression language. */
enum class ExpressionOperator
{
    none,
    add,            ///< `a + b`
    subtract,       ///< `a - b`
    multiply,       ///< `a * b`
    divide,         ///< `a / b`
    modulo,         ///< `a % b`
    power,          ///< `a ^ b`
    negate,         ///< `-a`
    logicalNot,     ///< `!a`
    less,           ///< `a < b`
    lessOrEqual,    ///< `a <= b`
    greater,        ///< `a > b`
    greaterOrEqual, ///< `a >= b`
    equal,          ///< `a == b`
    notEqual,       ///< `a != b`
    logicalAnd,     ///< `a && b`
    logicalOr       ///< `a || b`
};

//==============================================================================
/** An immutable syntax tree of an expression program.

    The language is statically typed (`bool`, `int`, `float`, `double`, `string` and any type
    registered in an ExpressionContext) and supports C-like operators, the ternary `?:`,
    function calls, members, methods, swizzles, array indexing and statements:

    @code
    let w = width / len (values);     // immutable local
    rect = rect (i * w, 0, w, 10);    // named output
    fill = values[i] > 0.5 ? rgb (255, 0, 0) : rgb (0, 0, 255);
    @endcode

    Number literals accept the unit suffixes `px`, `s` and `rad` (unchanged), `deg` (converted to
    radians), `ms` (converted to seconds) and `%` (divided by 100). A `%` directly followed by an
    operand is the modulo operator instead, as in `10%3`. Color literals `#rgb`, `#rgba`,
    `#rrggbb` and `#rrggbbaa` are calls to `color` with the ARGB value as an `int`, so they compile
    when the context has a `color` type (see addGraphicsExpressionTypes in yup_graphics).

    A program made of a single expression produces the output named `result`.

    Expressions are only parsed and inspected by this class: compile them with
    CompiledExpression::compile and evaluate them with an ExpressionState. Copies are cheap and
    share the immutable nodes, so expressions can be freely passed between threads.

    @see CompiledExpression, ExpressionContext, ExpressionState

    @tags{Core}
*/
class YUP_API Expression
{
public:
    //==============================================================================
    /** The kind of a node. */
    enum class Kind
    {
        constant, ///< A literal value.
        symbol,   ///< A (possibly dotted) name, like `width` or `row.value`.
        unary,    ///< A unary operator applied to input 0.
        binary,   ///< A binary operator applied to inputs 0 and 1.
        ternary,  ///< `input0 ? input1 : input2`.
        call,     ///< A free function call `name (inputs...)`.
        method,   ///< A method call `input0.name (other inputs...)`.
        member,   ///< A member read `input0.name`.
        index,    ///< An array read `input0[input1]`.
        let,      ///< The statement `let name = input0`.
        output,   ///< The statement `name = input0`.
        block     ///< A sequence of statements, the inputs.
    };

    //==============================================================================
    /** Creates the constant integer 0. */
    Expression();

    /** Creates an `int` constant. */
    explicit Expression (int value);

    /** Creates a floating point constant that adopts the precision of the values it is combined with. */
    explicit Expression (double value);

    /** Creates a `float` constant. */
    explicit Expression (float value);

    /** Creates a `bool` constant. */
    explicit Expression (bool value);

    /** Creates a `string` constant. */
    static Expression text (const String& value);

    /** Creates a symbol reference. */
    static Expression symbol (const String& name);

    /** Creates a free function call. */
    static Expression call (const String& functionName, std::vector<Expression> arguments);

    //==============================================================================
    /** Parses a program.

        On failure the error message is prefixed by the `line:column:` position of the problem.
    */
    static ResultValue<Expression> parse (StringRef text);

    /** Parses a single expression embedded in a larger text, like a property value in a document.

        Parsing starts at the UTF-8 byte offset `offset` and stops before the first character that
        can't continue the expression (for example `;`, `}`, `,` or an unmatched `)`). On success
        `offset` is updated to the first character after the expression that isn't whitespace or a
        comment; on failure it is set to the offset of the error. Line and column numbers in the
        error message are relative to the start of the whole text.

        @code
        int offset = 6;
        auto value = Expression::parseExpression ("fill: #ff8800.withAlpha (50%); next", offset);
        // value is the expression, text[offset] == ';'
        @endcode
    */
    static ResultValue<Expression> parseExpression (StringRef text, int& offset);

    //==============================================================================
    /** Returns the source text of this expression, parsing it again gives an equivalent expression. */
    String toString() const;

    /** Returns the kind of the root node. */
    Kind getKind() const noexcept;

    /** Returns the name of a symbol, call, method, member, let or output node. */
    const String& getName() const noexcept;

    /** Returns the operator of an unary or binary node, ExpressionOperator::none otherwise. */
    ExpressionOperator getOperator() const noexcept;

    /** Returns the value of a constant node, or a void var. */
    var getConstantValue() const;

    /** Returns the number of child nodes. */
    int getNumInputs() const noexcept;

    /** Returns a child node. */
    Expression getInput (int index) const;

    /** Returns the 1-based line and column of the node in the parsed text, 0 for built expressions. */
    std::pair<int, int> getPosition() const noexcept;

    //==============================================================================
    /** Returns the free symbols referenced by the program, excluding locals and outputs it defines. */
    StringArray getReferencedSymbols() const;

    /** Returns true if the program references the symbol. */
    bool referencesSymbol (StringRef name) const;

    /** Returns a copy with every reference to a free symbol renamed. */
    Expression withRenamedSymbol (StringRef oldName, const String& newName) const;

    //==============================================================================
    /** Compiles and evaluates the program with the built-in functions only, returning its numeric result.

        This compiles on every call: to evaluate a program repeatedly, compile it once with
        CompiledExpression::compile and evaluate it with an ExpressionState.
    */
    ResultValue<double> evaluate() const;

    /** Compiles and evaluates the program, returning its numeric result (or the first output if there's no `result`).

        This compiles on every call: to evaluate a program repeatedly, compile it once with
        CompiledExpression::compile and evaluate it with an ExpressionState.
    */
    ResultValue<double> evaluate (const ExpressionContext& context) const;

    //==============================================================================
    /** Returns the source representation of an operator, like `+` or `<=`. */
    static StringRef getOperatorSymbol (ExpressionOperator op) noexcept;

    /** Returns the function name implementing an operator, like `operator+`. */
    static String getOperatorFunctionName (ExpressionOperator op);

    //==============================================================================
    Expression operator+ (const Expression& other) const;
    Expression operator- (const Expression& other) const;
    Expression operator* (const Expression& other) const;
    Expression operator/ (const Expression& other) const;
    Expression operator% (const Expression& other) const;
    Expression operator-() const;

private:
    //==============================================================================
    struct Node;
    struct Parser;
    friend struct ExpressionCompiler;

    using Value = std::variant<bool, int, float, double, String>;

    explicit Expression (std::shared_ptr<const Node> node);
    static Expression make (Kind kind, ExpressionOperator op, const String& name, Value value, std::vector<Expression> inputs, int line = 0, int column = 0);

    const Value& getValue() const noexcept;
    void collectSymbols (StringArray& symbols, StringArray& bound) const;
    Expression renamed (StringRef oldName, const String& newName, StringArray& bound) const;
    void write (String& out, int parentPrecedence) const;

    std::shared_ptr<const Node> node;
};

} // namespace yup
