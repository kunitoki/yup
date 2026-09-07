# YDSP for Visual Studio Code

Syntax highlighting, snippets and editing support for **YDSP** — the realtime
JIT-compiled audio DSP language of the [YUP](https://github.com/kunitoki/yup)
library.

Platform packages launch the bundled `yup_dsp_compiler --lsp` server. During
development, set `ydsp.server.path` to an absolute compiler path. The server
currently provides live diagnostics over stdio; syntax highlighting remains
available if the server cannot be started.

YDSP is a small language for audio signal processing that compiles ahead of the
audio callback into machine code (AsmJit x86-64/AArch64 or WebAssembly). It
borrows a Faust-style composition algebra (`:` sequential, `,` parallel, `<:`
split, `:>` merge, `~` recursion, `_` passthrough) and Cmajor-style
`processor` / `graph` declarations with typed endpoints, state history,
sample-accurate events and statically bounded loops.

## Features

- **Syntax highlighting** for `.ydsp` files:
  - `processor` / `graph` / `struct` / `func` / `node` declarations
  - `input` / `output` / `state` / `event` / `process` / `init` / `connection`
    blocks, `import` and `declare` statements, `let` constants
  - All primitive types (`float`, `float32`, `float64`, `int`, `int32`,
    `int64`, `bool`)
  - Builtin constants (`pi`, `e`, `inf`, `nan`, `sampleRate`,
    `samplePeriod`, `blockSize`, `true`, `false`)
  - Intrinsics (`sin`, `tanh`, `pow`, `smooth`, `clamp`, `fma`, `select`,
    `mem`, …) and the seven event shapes
  - Graph operators (`->`, `<:`, `:>`, `~`, `@`, `'`, `..`) and endpoint
    annotation blocks (`[[ name: "…", min: 0, max: 1 ]]`)
- **Snippets** for processor / graph skeletons (both the `connection` block and
  the `process = …` algebra form), event handlers, `func` declarations, bounded
  `for` loops, state arrays, metadata and annotations
- **Editing conveniences**: comment toggling, bracket matching and
  auto-closing, block folding and indentation rules

## Installing

### From source (development)

1. Open this folder (`modules/yup_dsp_jit/tools/vscode-ydsp`) in VSCode.
2. Press <kbd>F5</kbd> (or run the **Run YDSP Extension** launch
   configuration) — a new Extension Development Host window opens with the
   extension loaded.

### As a VSIX

From the repository root, one command builds the extension and installs it
into your local VSCode (requires `just` and the `code` CLI on your `PATH`):

```sh
just vscode
```

Or manually:

```sh
cd modules/yup_dsp_jit/tools/vscode-ydsp
sh tools/package-server.sh
npm install
npm run compile
npx --yes @vscode/vsce package -o vscode-ydsp.vsix
code --install-extension vscode-ydsp.vsix
```

Set `YDSP_COMPILER` to override the compiler binary used by the packaging
script. The generated `server/` directory is release output and is ignored by
Git.

## Language reference

The full language specification lives in the YUP repository at
[`docs/dsp/yup-dsp-language.md`](../../../../docs/dsp/yup-dsp-language.md). A rich
set of real patches to test highlighting against lives in
[`examples/graphics/data/synths/`](../../../../examples/graphics/data/synths/)
(`*.ydsp`).

## Structure

```
vscode-ydsp/
├── package.json                      # extension manifest
├── language-configuration.json       # comments, brackets, folding, indentation
├── syntaxes/ydsp.tmLanguage.json     # TextMate grammar (source.ydsp)
└── snippets/ydsp.code-snippets       # code snippets
```

## License

ISC — see [LICENSE](LICENSE). Copyright (c) 2024-2026 kunitoki@gmail.com.
