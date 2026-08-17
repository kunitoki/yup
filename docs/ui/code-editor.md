# Code Editor

YUP ships a syntax-highlighting source editor built from three cooperating
pieces:

- `CodeDocument` — a line-based text model with undo/redo and incremental
  change notifications.
- `SyntaxDefinition` — a declarative, JSON-driven description of a language
  (comments, strings, keywords, operators).
- `CodeTokeniser` — an incremental, per-line tokenizer driven by a
  `SyntaxDefinition`.
- `CodeEditor` — the `Component` that ties them together: caret/selection,
  editing, a line-number gutter, find/replace, bracket matching, breakpoints
  and a minimap.

## CodeDocument

`CodeDocument` stores text as a list of lines (without trailing newlines) and
exposes positions as `(line, column)` pairs or global character indices:

```cpp
yup::CodeDocument document;
document.setText ("int main() { return 0; }");

document.getNumLines();                          // 1
document.getLine (0);                            // "int main() { return 0; }"
document.getNumCharacters();                     // 23

auto start = document.indexToPosition (4);       // line 0, index 4
document.replaceRange (start, start, " ");
document.undo();                                 // back to the original text
```

Edits (`replaceRange`, `insertText`, `removeRange`) are recorded in an
`UndoManager`. By default the document creates its own manager; pass an
`UndoManager::Ptr` to the constructor to share one across documents (the shared
manager is not cleared when the document is destroyed — the caller owns it).
Listeners are notified with the inclusive range of affected lines, which is
exactly what the tokenizer needs to invalidate:

```cpp
struct Listener : public yup::CodeDocument::Listener
{
    void codeDocumentChanged (yup::CodeDocument&, int firstLine, int lastLine) override {}
};
```

## SyntaxDefinition

A `SyntaxDefinition` is loaded from JSON or obtained from the built-in
languages (`"cpp"`, `"glsl"`, `"python"`, `"xml"`):

```cpp
auto definition = yup::SyntaxDefinition::getBuiltIn ("cpp");
yup::SyntaxDefinition::getBuiltInForExtension ("py");    // the Python definition
yup::SyntaxDefinition::getBuiltInForExtension ("svg");   // the XML definition
```

### JSON format

Only `name` is mandatory; every other section is optional.

```json
{
    "name": "C++",
    "extensions": ["cpp", "h", "hpp"],
    "lineComment": "//",
    "blockComment": { "start": "/*", "end": "*/" },
    "strings": {
        "delimiters": ["\"", "'"],
        "multiLineDelimiters": [],
        "escape": "\\",
        "multiLine": false,
        "rawStrings": true
    },
    "preprocessor": "#",
    "numbers": { "hex": true, "binary": true, "float": true, "exponent": true, "suffix": true },
    "keywords": ["if", "for", "return"],
    "types": ["int", "float", "void"],
    "operators": ["+", "-", "->", "::"]
}
```

- `lineComment` — prefix that starts a comment running to the end of the line.
- `blockComment` — delimiters for multi-line comments.
- `strings.delimiters` — single-line string delimiters; `multiLineDelimiters`
  are checked first (e.g. Python's `"""` and `'''`) and may span lines when
  `multiLine` is true. `escape` is the character that escapes the next one.
  `rawStrings` enables C++-style raw string literals (`R"delim(...)delim"`,
  including the `u8R` / `uR` / `UR` / `LR` prefixes and empty delimiters), which
  may span lines. The accepted prefixes come from `rawStringPrefixes` (an array
  of strings; defaults to the C++ set when `rawStrings` is true and none are
  given). `stringPrefixes` lists the encoding/format prefixes folded into regular
  string and character literals (C++ `u8"…"`, `L"…"`, `u"…"`, `U"…"`, Python
  `f"…"`, `r"…"`, `b"…"`, `fr"…"`, …) — prefixed literals may also use the
  `multiLineDelimiters` (e.g. Python `f"""…"""`).
- `preprocessor` — a prefix (usually `#`) at the very start of a line that
  colors the whole line as a directive.
- `numbers` — which numeric syntaxes to recognize (hex, binary, float,
  exponent, suffix).
- `keywords` / `types` / `operators` — word sets; identifiers are classified
  keyword → type → identifier. Operators use longest-match (3, 2, 1 chars).

Colors are not part of a syntax definition: they belong to a
`CodeEditorScheme` (see below), so the same definition can be rendered with
any scheme.

Load a custom definition at runtime:

```cpp
yup::SyntaxDefinition definition;
auto result = definition.loadFromData (jsonText);   // or loadFromFile (file)
```

## CodeTokeniser

The tokenizer caches tokens per line and re-tokenizes only the lines affected
by an edit. Because multi-line constructs (block comments, multi-line strings)
make a line depend on the previous line's state, each cached line records its
entry/exit state and edits propagate state changes forward until stable:

```cpp
yup::CodeTokeniser tokeniser;
tokeniser.setSyntaxDefinition (yup::SyntaxDefinition::getBuiltIn ("cpp"));

for (auto& token : tokeniser.getTokens (document, 0))
{
    auto tokenType = token.type; // color it with CodeEditorScheme::getColor (tokenType)
    // token.start / token.end are character offsets within the line
}
```

The tokenizer subscribes itself to the document on first use, so edits
invalidate the right lines automatically.

## CodeEditorScheme

A `CodeEditorScheme` owns every color the editor needs to paint itself: the
editor chrome (background, gutter, caret, current line, selection, search
highlight, breakpoint, minimap background/foreground/viewport) and the
per-token syntax colors. Colors are stored keyed by `Identifier` and accessed through
`setColor` / `getColor`, using the string constants in
`CodeEditorScheme::ColorId` for the chrome colors and the token-type names
(`"keyword"`, `"string"`, …) for the syntax colors.

```cpp
yup::CodeEditorScheme scheme;
scheme.setColor (yup::CodeEditorScheme::ColorId::background, yup::Color (0xff282c34));
scheme.setColor (yup::SyntaxDefinition::TokenType::keyword, yup::Color (0xffc678dd));
```

Built-in well-known schemes are available through `getBuiltIn`, matching
case-insensitively and ignoring spaces/hyphens:

```cpp
yup::CodeEditorScheme::getBuiltIn ("monokai");          // Monokai
yup::CodeEditorScheme::getBuiltIn ("alabaster");        // Alabaster (light)
yup::CodeEditorScheme::getBuiltIn ("oneDark");          // One Dark
yup::CodeEditorScheme::getBuiltIn ("solarizedDark");    // Solarized Dark
yup::CodeEditorScheme::getBuiltIn ("solarizedLight");   // Solarized Light
```

`getAvailableSchemeNames()` returns the display names of the built-in schemes
and `getDefault()` the classic dark palette. A default-constructed scheme
carries that same dark palette, so a bare `CodeEditor` renders like before.

## CodeEditor

```cpp
yup::CodeDocument document;
document.setText ("int main() { return 0; }");

yup::CodeEditor editor (document);
editor.setSyntaxDefinition ("cpp");
addAndMakeVisible (editor);
```

Highlights:

- **Editing** — caret/selection (shift+arrows extend with proper anchor
  semantics), home/end, word navigation, backspace/delete, clipboard
  (ctrl+c/x/v), undo/redo (ctrl+z/y), read-only mode, tab expands to spaces.
- **Smart auto-indent** — pressing enter copies the current line's leading
  whitespace.
- **Syntax colors** — tokens are rendered from the active `CodeEditorScheme`;
  the colored text layout is rebuilt only when the document, font, definition
  or scheme changes.
- **Gutter** — optional line numbers; clicking the gutter toggles a
  breakpoint marker.
- **Find / replace** — `findAll`, `findNext`/`findPrevious` (wrap-around),
  `replaceNext`, `replaceAll` (single undo step), and
  `highlightSearchMatches`.
- **Bracket matching** — `getBracketMatch` returns the matching `() [] {}` pair
  around the caret as a range (callers can highlight it themselves).
- **Minimap** — an optional right-edge code-density overview; click or drag to
  scroll, with the vertical scrollbar sitting to its left.
- **Scrollbar** — a vertical scrollbar appears automatically (auto-hide) when
  the document has more lines than fit the viewport.
- **Styling** — every color (background, gutter, caret, selection, search
  highlight, breakpoint, and the per-token syntax colors) comes from the
  active `CodeEditorScheme`; the painting itself is provided by the theme
  (Themes v1). `setFont` defaults to the theme's monospace font.

```cpp
editor.setScheme (yup::CodeEditorScheme::getBuiltIn ("monokai"));
```

```cpp
editor.setLineNumbersVisible (false);
editor.setReadOnly (true);
editor.setBreakpoint (3, true);
editor.findNext ("main");
editor.replaceAll ("main", "entry");
editor.undo();
```

See `examples/graphics/source/examples/CodeEditor.h` for a runnable demo.
