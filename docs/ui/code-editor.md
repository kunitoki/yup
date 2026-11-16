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
Listeners receive a `CodeDocument::Change` describing which lines were
replaced (see CodeTokeniser below):

```cpp
struct Listener : public yup::CodeDocument::Listener
{
    void codeDocumentChanged (yup::CodeDocument&, const yup::CodeDocument::Change& change) override {}
};
```

Line endings are detected by `setText` from the first line break: a CRLF
document keeps `\r\n` in `getText()` (also for lines added later), while lines,
`getTextInRange` and `getTextBetween` always use `\n` and character indices
count a line break as one character. `setLineEnding` converts the document.

## SyntaxDefinition

A `SyntaxDefinition` is loaded from JSON or obtained from the built-in
languages (`"cpp"`, `"glsl"`, `"python"`, `"xml"`, `"ydsp"`, `"yui"`):

```cpp
auto definition = yup::SyntaxDefinition::getBuiltIn ("cpp");
yup::SyntaxDefinition::getBuiltInForExtension ("py");     // the Python definition
yup::SyntaxDefinition::getBuiltInForExtension ("svg");    // the XML definition
yup::SyntaxDefinition::getBuiltInForExtension ("ydsp");   // the YDSP definition
yup::SyntaxDefinition::getBuiltInForExtension ("yui");    // the YUI definition
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
- `brackets` — matched bracket pairs as two-character strings (default
  `["()", "[]", "{}"]`), used by bracket matching.
- `autoClosingPairs` — pairs closed automatically while typing; defaults to the
  brackets plus every single-character string delimiter.
- `wordSeparators` — characters that end a word for word navigation, deletion
  and double-click selection.
- `increaseIndentPattern` — a regular expression; when the text before the
  caret matches it, Enter adds one indent level (Python uses `:\\s*(#.*)?$`).

### Scopes, word sets and rule states

Every token carries a dotted **scope** such as `keyword.control` or
`identifier.function`. The first component maps the token to a
`SyntaxDefinition::TokenType` (`scopeToTokenType`), and schemes color it with
fallback to parent scopes (see `CodeEditorScheme`). Tokens produced by the
fields above use the plain token type names (`"keyword"`, `"string"`, ...).

`wordSets` adds custom identifier sets. A word set takes precedence over
`keywords` and `types`, and `isKeyword` / `isType` also report words of
`keyword.*` / `type.*` sets:

```json
"wordSets": { "type.builtin": ["size_t", "uint8_t"], "keyword.control": ["co_await"] }
```

`states` adds regular expression rules for anything the fixed fields cannot
express (decorators, tag names, nested states, heredocs):

```json
"states": {
    "root": { "rules": [
        { "match": "(def)(\\s+)([A-Za-z_]\\w*)", "scope": "whitespace",
          "captures": { "1": "keyword", "3": "identifier.function" } },
        { "match": "<<([A-Z]+)$", "scope": "string.heredoc", "push": "heredoc" }
    ] },
    "heredoc": { "scope": "string.heredoc", "rules": [
        { "match": "^$1$", "scope": "string.heredoc", "pop": true }
    ] }
}
```

- The `root` state is active at the start of the document. Its rules are tried
  at every position before the built-in lexer; the first rule matching at the
  current position wins.
- `push` enters another state, `pop` returns to the previous one. Inside a
  pushed state only its rules run, and unmatched text takes the state's
  `scope`. States persist across lines, so they can span many lines.
- `captures` assigns a scope per capture group; text of the match not covered
  by a group takes the rule `scope`.
- `"skipWords": true` ignores a match whose whole text is a keyword, type or
  word set word, so generic rules such as "identifier followed by `(` is a
  function call" leave `if (` and `while (` alone.
- In the rules of a pushed state, `$1` to `$9` are replaced by the (escaped)
  captures of the rule that pushed it, so the end of a heredoc or a raw string
  can reference its opening delimiter.
- Patterns use the `std::regex` ECMAScript grammar through `CodeRegex`:
  lookahead and backreferences work, lookbehind and named groups do not.
  Matching runs on the UTF-8 bytes of a line, so `\w` and `.` do not cover
  non-ASCII characters. `^` matches only at the start of a line.
- Invalid patterns make `loadFromData` / `addState` fail. Lines longer than
  10000 characters skip the rules. On Emscripten builds without exception
  support an invalid pattern aborts, so validate definitions on desktop first.

`injections` runs the rules of a state inside comment, string or preprocessor
tokens produced by the lexer, splitting them into finer scopes (escapes,
format specifiers, doc tags, include paths). Text between matches keeps the
token scope; raw strings (`string.raw`) are never injected:

```json
"states": {
    "stringContent": { "rules": [ { "match": "\\\\.", "scope": "string.escape" } ] },
    "commentContent": { "rules": [ { "match": "\\bTODO\\b", "scope": "comment.todo" } ] }
},
"injections": { "string": "stringContent", "comment": "commentContent" }
```

The built-in definitions emit these scopes on top of the plain token types,
and the built-in schemes color them: `keyword.control`, `keyword.constant`,
`keyword.operator`, `keyword.attribute`, `keyword.directive`, `keyword.tag`,
`string.escape`, `string.format`, `string.interpolation`, `string.include`,
`string.raw`, `comment.doc.tag`, `comment.todo`, `comment.link`,
`identifier.function`, `identifier.builtin`, `identifier.constant`,
`identifier.macro`, `identifier.member`, `identifier.property`,
`identifier.language`, `type.class`, `type.namespace`, `type.attribute`,
`type.selector`, `number.unit` and `number.color`.

States can also be built from C++ with `addState`, injections with
`setInjection`, and identifiers with `addWords`.

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

Custom `CodeDocument::Listener`s receive a `CodeDocument::Change` for every
edit: `numLinesRemoved` lines starting at `firstLine` were replaced by
`numLinesInserted` lines, and the lines after them moved by `getLineDelta()`,
which is what line-indexed data (markers, caches) needs to stay in sync.

## CodeEditorScheme

A `CodeEditorScheme` owns every color the editor needs to paint itself: the
editor chrome (background, gutter, caret, current line, selection, search
highlight, breakpoint, error line, minimap background/foreground/viewport, bracket
match, word occurrence, indent guide, whitespace, diagnostic error / warning /
information / hint) and the
per-token syntax colors. Colors are stored keyed by `Identifier` and accessed through
`setColor` / `getColor`, using the string constants in
`CodeEditorScheme::ColorId` for the chrome colors and the token-type names
(`"keyword"`, `"string"`, …) for the syntax colors.

```cpp
yup::CodeEditorScheme scheme;
scheme.setColor (yup::CodeEditorScheme::ColorId::background, yup::Color (0xff282c34));
scheme.setColor (yup::SyntaxDefinition::TokenType::keyword, yup::Color (0xffc678dd));
scheme.setColor ("identifier.function", yup::Color (0xff61afef));
```

`getColorForScope ("keyword.control.flow")` looks up the exact scope, then
`keyword.control`, then `keyword`, so a scheme only needs colors for the
scopes it wants to distinguish.

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
  (ctrl+c/x/v), undo/redo (ctrl+z/y) restoring the caret and selection around
  each edit, read-only mode, tab expands to spaces. Typing is undone word by
  word and consecutive deletions together; moving the caret, a line break or
  any other command starts a new undo step. With a multi-line or
  whole-line selection, tab indents every touched line and shift+tab outdents
  them (also available as `indentSelection` / `outdentSelection`); a line where
  the selection ends at column 0 is left alone, and empty lines are not indented.
- **Smart auto-indent** — pressing enter copies the current line's leading
  whitespace and adds one level after an opening bracket or when the text before
  the caret matches the definition's `increaseIndentPattern`; enter between a
  bracket pair puts the closing bracket on its own line. Typing a closing bracket
  on a blank line aligns it with the line of its opening bracket. Home moves to
  the first non-whitespace character, then to the line start.
  `setIndentUsingTabs (true)` indents with tab characters instead of spaces.
- **Navigation** — page up / page down move the caret and the view by one
  screen, up / down keep the column the caret started from across shorter
  lines, and a third click after a double-click selects the whole line
  (`selectLine`).
- **Line commands** — `toggleLineComment` (ctrl+/), `toggleBlockComment`
  (shift+alt+a), `moveSelectedLines` (alt+up / alt+down),
  `duplicateSelectedLines` (shift+alt+down / shift+alt+up), `deleteSelectedLines`
  (ctrl+shift+k) and `insertLine` (ctrl+enter below, ctrl+shift+enter above)
  work on every line touched by the selection, as one undo step each. Comment
  tokens come from the definition's `lineComment` and `blockComment`.
- **Auto-closing pairs** — typing an opening bracket or quote from the
  definition's `autoClosingPairs` inserts its closing character when followed by
  whitespace, a closing character or the line end (quotes not right after a
  word), typing the closing character steps over it, typing an opener with a
  selection wraps it, and backspace between an empty pair removes both. Turn it
  off with `setAutoClosingEnabled (false)`.
- **Syntax colors** — tokens are rendered from the active `CodeEditorScheme`;
  the colored text layout is rebuilt only when the document, font, definition
  or scheme changes.
- **Gutter** — optional line numbers. Clicking the gutter toggles a
  breakpoint marker once `setGutterBreakpointsEnabled (true)` is called (off
  by default); `setBreakpoint` works either way.
- **Error lines** - `setErrorLine` highlights whole lines with the scheme's
  `errorLine` color, for example to show compiler errors; `clearErrorLines`
  removes them, and so does switching documents with `setDocument`.
  Breakpoints and error lines move with the text when lines are inserted or
  removed above them.
- **Diagnostics** - `setDiagnostics` / `addDiagnostic` take
  `CodeEditor::Diagnostic` entries (0-based line and column range, severity
  `error` / `warning` / `information` / `hint`, message, source). Ranges are
  underlined with a squiggle in the severity color, the gutter shows a marker on
  their first line (except hints), and they move with inserted or removed
  lines. An empty range marks the word at its start. `getDiagnosticsAt`
  returns the diagnostics at a position, most severe first. Error lines are
  diagnostics with `highlightLine` set, so `clearDiagnostics` removes them too.

```cpp
yup::CodeEditor::Diagnostic diagnostic;
diagnostic.line = diagnostic.endLine = 4;
diagnostic.column = 8;
diagnostic.endColumn = 13;
diagnostic.severity = yup::CodeEditor::Diagnostic::Severity::warning;
diagnostic.message = "unused variable 'value'";
editor.setDiagnostics ({ diagnostic });
```
- **Hover** - when the mouse rests over a character (`setHoverDelay`, 500 ms by
  default) a popup shows the messages of the diagnostics under it and the text
  returned by the `setHoverProvider` callback. Typing, clicking, scrolling and
  edits hide it; `showHover` / `hideHover` drive it from code.

```cpp
editor.setHoverProvider ([] (const yup::CodeEditor& editor, int index) -> yup::String
{
    return lookUpDocumentation (editor.getDocument()->indexToPosition (index));
});
```
- **Completion** - Ctrl+Space, or typing one of the
  `setCompletionTriggerCharacters`, asks the `setCompletionProvider` callback for
  `CompletionItem`s (label, optional insert text and detail) and lists them under
  the caret. Typing more of the word filters the list (labels starting with it
  first, then labels containing its characters in order), Up / Down / Page Up /
  Page Down select, Enter, Tab or a click insert the item in place of the word,
  Escape or moving the caret close it.

```cpp
editor.setCompletionTriggerCharacters (".");
editor.setCompletionProvider ([] (const yup::CodeEditor&, int caretPosition)
{
    return std::vector<yup::CodeEditor::CompletionItem> {
        { "length", "length()", "int" },
        { "substring", "substring (", "String" }
    };
});
```
- **Semantic tokens** - `setSemanticTokens` colors ranges computed by a language
  service (line, column, length and a dotted scope resolved by the scheme) over the
  syntax highlighting. They move with inserted and removed lines and are dropped
  from edited lines until they are set again.

```cpp
editor.setSemanticTokens ({
    { 3, 8, 6, "type.class" },
    { 5, 4, 5, "identifier.parameter" }
});
```
- **Find / replace** — `findAll`, `findNext`/`findPrevious` (wrap-around),
  `replaceNext`, `replaceAll` (single undo step), and
  `highlightSearchMatches` (the highlights follow edits, `getSearchMatches`
  returns them). Every call takes a `SearchOptions` with `caseSensitive`,
  `wholeWord` and `regex`; regular expressions use the ECMAScript syntax, match
  within a line, and replacements expand `$0`-`$9` and `$$`.

```cpp
yup::CodeEditor::SearchOptions options;
options.regex = true;
editor.replaceAll ("(\\w+) = (\\d+)", "$1: $2", options);
```
- **Find bar** - Ctrl+F opens a find bar at the top right of the text area
  (Ctrl+H adds the replace field, `showFind` does the same from code). A
  single-line selection becomes the search text; typing highlights every match,
  selects the next one from where the search started and shows "2 of 5".
  Enter / Shift+Enter (or F3 / Shift+F3) move between matches, Enter in the
  replace field replaces the current match and Ctrl+Alt+Enter replaces all,
  Alt+C / Alt+W / Alt+R toggle case sensitive, whole word and regex, Escape
  closes the bar. F3 in the editor repeats the last search.
- **Go to line** - Ctrl+G (`showGoToLine`) opens the bar with a `line` or
  `line:column` field (1-based); `goToLine (line, column)` does the same from
  code with 0-based values clamped to the document.
- **Bracket matching** — the bracket pair at the caret (the definition's
  `brackets`, `() [] {}` by default) is highlighted, also across lines;
  `getBracketMatch` returns it as a range.
- **Occurrences** — other whole-word occurrences of the word at the caret, or of
  a selection that is exactly one word, are highlighted in the visible lines
  (`getWordOccurrences`, `setOccurrenceHighlightingEnabled`).
- **Indent guides and whitespace** — vertical guides mark each indentation
  level, continuing through blank lines (`setIndentGuidesVisible`, on by
  default); `setWhitespaceVisible (true)` renders spaces as dots and tabs as
  lines.
- **Minimap** — an optional right-edge code-density overview; click or drag to
  scroll, with the vertical scrollbar sitting to its left.
- **Scrollbars** — a vertical scrollbar appears automatically (auto-hide) when
  the document has more lines than fit the viewport, and a horizontal one when
  the visible lines are wider than the text area.
- **Styling** — every color (background, gutter, caret, selection, search
  highlight, breakpoint, error line, and the per-token syntax colors) comes from the
  active `CodeEditorScheme`; the painting itself is provided by the theme
  (Themes v1). `setFont` defaults to the theme's monospace font.

```cpp
editor.setScheme (yup::CodeEditorScheme::getBuiltIn ("monokai"));
```

```cpp
editor.setLineNumbersVisible (false);
editor.setReadOnly (true);
editor.setGutterBreakpointsEnabled (true);
editor.setBreakpoint (3, true);
editor.setErrorLine (7, true);
editor.clearErrorLines();
editor.findNext ("main");
editor.replaceAll ("main", "entry");
editor.undo();
```

See `examples/graphics/source/examples/CodeEditor.h` for a runnable demo.
