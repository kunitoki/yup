# YDSP for Visual Studio Code

Syntax highlighting, snippets, editing support and live playback for **YDSP** -
the realtime JIT-compiled audio DSP language of the
[YUP](https://github.com/kunitoki/yup) library.

Platform packages launch the bundled `yup_dsp_compiler`. During development, set
`ydsp.server.path` to an absolute compiler path. The same binary supplies live
diagnostics over stdio (`--lsp`) and owns playback (`run`); syntax highlighting
remains available if the binary cannot be started.

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
  - Builtin constants (`pi`, `e`, `sampleRate`,
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
- **Patch Player panel** in the Activity Bar: transport controls, the workspace
  patch list, audio/MIDI device selects, sample rate, block size and a test note
- **Playback** of `.ydsp` and `.ydsp-project` patches through
  `yup_dsp_compiler run`, with hot reload of the entry file and everything it
  imports, a status-bar control and one pinned player per window
- **Diagnostics** from `yup_dsp_compiler --lsp` over stdio

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

## Playing patches

The **YDSP** icon in the Activity Bar opens the **Patch Player** panel, which has
two tabs:

- **Performance** (keyboard icon) holds the transport - **Run**, **Stop** and
  **Restart** act on the single player - and the workspace patch list.
- **Settings** (gear icon) holds the audio backend, audio output, audio input and
  MIDI input and output selects, plus sample rate, block size, a test note,
  verbose diagnostics and Follow the active patch.

The patch list covers every `.ydsp` and `.ydsp-project` file in the workspace.
It is ordered naturally by path (`Saw2.ydsp` before `Saw10.ydsp`) and keeps that
order between refreshes; the playing patch is pinned with a marker and clicking
another retargets the player. Device selects are filled from
`yup_dsp_compiler devices --json` and remembered per workspace - use **Refresh**
after plugging a device in.

The same actions are available as commands, in the editor title, in the explorer
context menu, and from the status-bar item, which shows the playing patch and
opens a menu with Run, Stop, Restart, device selection and the panel. Playback
runs `yup_dsp_compiler run --hotreload`. A project watches its manifest and every
listed source; a standalone `.ydsp` watches itself and everything it imports, so
editing a shared library reloads it too. Saving recompiles and swaps the patch in
at the next audio block. An invalid save keeps the previous patch playing and
prints its diagnostics to the **YDSP Playback** output channel.

Playback is pinned by default, so switching editors never changes what you hear.
Enabling **Follow the active patch** retargets the player whenever you switch to
another patch, after a short debounce. Either way only one player subprocess
exists, and an identical request is never respawned. Devices and options apply on
the next run - changing them while a patch plays restarts the player - and MIDI
defaults to disabled while audio defaults to the system device. Selecting
`No input (feeds silence)` passes `--audio-input none`.

Because the player compiles from disk, unsaved patch files are listed above the
patch list and are not heard until you save. The `ydsp.player.verbose` setting
adds device, channel, watched-file and peak reporting, and
`ydsp.player.arguments` appends raw arguments such as `--main AlternateMain`.

The player runs wherever the extension host runs, so in a remote, SSH or
container workspace the audio comes out of that machine. Only one player runs at
a time; it is asked to stop with `SIGTERM` when playback is stopped and when the
window closes.

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
├── images/ydsp-activity.svg          # Activity Bar container icon
├── src/extension.ts                  # activation: language server, commands, panel wiring
├── src/panel.ts                      # Patch Player sidebar webview
├── src/compiler.ts                   # compiler path lookup and one-shot invocation
├── src/devices.ts                    # audio/MIDI device enumeration
├── src/player.ts                     # `run` subprocess and its lifecycle
├── syntaxes/ydsp.tmLanguage.json     # TextMate grammar (source.ydsp)
└── snippets/ydsp.code-snippets       # code snippets
```

## License

ISC — see [LICENSE](LICENSE). Copyright (c) 2024-2026 kunitoki@gmail.com.
