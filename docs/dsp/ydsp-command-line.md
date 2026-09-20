# YDSP compiler and player

`yup_dsp_compiler` builds bundles, supplies VS Code diagnostics, and plays patches
through the computer's audio and MIDI devices. The standalone host tool lives in
`cmake/tools/ydsp_compiler`. `main.cpp` handles dispatch; compiler, inspector,
devices, player, and LSP modes each have their own `yup_Ydsp*Command.cpp` implementation.

## Bundles

```sh
yup_dsp_compiler Main.ydsp --output Main.ydsb --target macos-arm64
yup_dsp_compiler Patch.ydsp-project --output Patch.ydsb --main AlternateMain --target macos-arm64 --fast-math
yup_dsp_compiler --inspect Patch.ydsb --list
```

Both inputs package explicit imports. Projects select the manifest's `main`
unless `--main` overrides it. Files remain separately scoped. Project bundles
retain the chosen entry point and patch metadata; they do not need the original
manifest or source files when instantiated. Unused alternate source files are
not packaged. Project bundles use format version 3; ordinary source bundles
continue to use version 2. Older readers reject project bundles.

`--target` can be repeated, using `macos`, `linux`, or `windows` with `arm64` or
`x64`, separated by a hyphen. WebAssembly is included by default. Without a
`--target`, the bundle contains only WebAssembly artifacts. Native playback of a
bundle through the library requires its native target. `--fast-math` enables
fast math in bundles; source playback uses the compiler's default host policy.

The corresponding library API is
`YdspCompiler::compileProjectBundle(file, bundleOptions, mainOverride)`.

## Devices and playback

```sh
yup_dsp_compiler devices
yup_dsp_compiler devices --json
yup_dsp_compiler run Main.ydsp --hotreload
yup_dsp_compiler run Patch.ydsp-project --hotreload
yup_dsp_compiler run Patch.ydsp-project --audio-type CoreAudio --audio-output "External Headphones" --audio-input none --midi-input "DEVICE-ID"
yup_dsp_compiler run Patch.ydsp-project --midi-output "DEVICE-ID" --sample-rate 48000 --block-size 256
yup_dsp_compiler run Patch.ydsp-project --verbose --test-note 60
```

Copy audio type and device names from `devices`. For MIDI, prefer the identifier;
a unique name is also accepted. MIDI input and output default to disconnected.
Use `--midi-input` and `--midi-output` to connect one of each. MIDI input is
broadcast to the graph's event inputs. Only short MIDI messages are accepted;
SysEx is not supported by this player.

Audio uses default devices unless selected explicitly. `--audio-input none`
feeds silence into the patch's inputs. Float32 stream endpoints map to hardware
channels in declaration order. Mono output is duplicated to the remaining
output channels. Multichannel patches require enough channels on the selected
device. Other stream element types are rejected. The actual sample rate and
buffer size are printed after opening the device; the device may negotiate
values different from those requested.

`--verbose` prints the selected audio backend, connected audio channels, MIDI
names and identifiers, patch stream counts, and - with `--hotreload` - how many
files are being watched. It also prints cumulative audio
callback and MIDI counters, the maximum output peak since startup, and non-finite
sample counts approximately once per second. If callback counts stay at zero,
the audio device is not delivering callbacks. If note counts stay at zero while
playing a keyboard, check the MIDI selection. A nonzero peak with no audible
sound points to the output routing or device volume.

Use `--test-note 60` to send middle C on MIDI channel 1 for two seconds without
a MIDI controller. This requires a patch event input. Synth patches normally
remain silent until they receive notes.

Startup and runtime errors print to stderr even without `--verbose`. Runtime
messages distinguish device stops, format/layout changes, and graph processing
errors, preserving device error text (up to 511 UTF-8 bytes). Callback errors are
captured in bounded storage; console printing happens on the control thread.

MIDI input is applied at the next block boundary. MIDI output is drained on the
control thread approximately every 5 ms, without preserving sample offsets.
This is an auditioning host, not a sample-accurate external MIDI sequencer.
Queues are bounded; overflow counts are printed outside the audio callback and
all-sound-off messages prevent dropped note-offs from leaving voices held.
Press Ctrl+C to stop. Device loss or a device format change stops playback with
a message; restart to use the new configuration.

`run` accepts both an individual `.ydsp` file and a `.ydsp-project`. `--main`
requires a project; `--hotreload` works for either input. Playing `.ydsb` files
directly is not currently a CLI mode.

## Hot reload

The player polls the entry file and every file the last successful compile read,
waits for a stable poll, then compiles and prepares a new graph on a worker
thread. It swaps graphs at an audio block boundary and destroys the old graph on
the control thread. Audio processing does not load files, compile, allocate host
buffers, or print.

The watch list comes from the compiler itself, not from a second scan of the
sources: `YdspCompiler::getDiagnostics().getSourceIds()` returns the source
closure of the most recent compilation - the root source, every source listed in
a project manifest, and each transitively imported file that was found. A
standalone `.ydsp` therefore reloads when an imported library changes, not only
when the entry file does.

Invalid saves keep the previous graph playing and print diagnostics with
`path:line:column`, source context, and a caret. Fixing and saving the file
triggers another attempt. A failed reload retains the last valid watch list.
Changes made while compilation is underway cause that result to be
discarded and the latest files to be compiled.

A successful reload resets processor state, including voices and delay tails.
There is no crossfade or state migration. Changes to audio channel counts require
stopping and starting the player. Device selections belong to the host and are
not stored in the portable project manifest.

## Editor diagnostics

`yup_dsp_compiler --lsp` reserves stdout for LSP messages. The VS Code extension
watches `.ydsp` and `.ydsp-project` files. Project manifests carry the YDSP icon
and are highlighted as YAML.
Diagnostics support unsaved source and manifest contents, explicit imports,
UTF-16 editor positions, and clearing errors after a fix or document closure.
Errors in imports are published against their own file URI.

For a source document, the server searches its directory and parents for a
manifest that lists it. If multiple manifests list a file, open the intended
manifest to validate that project explicitly; diagnostics for a shared source
can be contributed by multiple open projects. Open manifests are validated even
when they contain errors. Standalone processor and function libraries receive
semantic validation without requiring a graph or generating executable code.
Bundle compilation and playback still require an executable entry point.

Library clients can supply unsaved file contents using
`YdspCompileOptions::sourceOverrides`, keyed by absolute paths.

## VS Code playback workflow

The VS Code extension plays patches through this command rather than
reimplementing playback. It contributes a **YDSP** Activity Bar container whose
**Patch Player** view has two tabs, each with an icon: **Performance** (keyboard)
holds the transport (Run, Stop, Restart) and the workspace patch list, while
**Settings** (gear) holds the audio backend, audio output, audio input and MIDI
input/output selects plus sample rate, block size, a test note, verbose
diagnostics and following the active patch:

1. **YDSP: Run Patch** is also available from a patch's editor title, the
   explorer context menu and the command palette. Pick a patch if several exist,
   then start one player subprocess. The patch list is ordered naturally by path
   (`Saw2.ydsp` before `Saw10.ydsp`) and never reorders between refreshes. Both
   input kinds start with `--hotreload`.
2. The panel's device selects and **YDSP: Select Audio/MIDI Devices** share one
   `devices --json` result, cached and refreshed on demand. Choices are
   remembered in workspace state. Audio offers Default, MIDI offers Disabled,
   and the audio input offers None.
3. A status-bar item shows the playing patch, with Run, Stop, Restart and device
   selection. Stopping sends SIGTERM; the child also exits when the extension
   closes. Device changes restart the player.
4. Playback is pinned: only one player subprocess exists per window, an identical
   request is never respawned, and switching editors changes nothing unless
   `ydsp.player.followActiveEditor` is set, which retargets that same player
   after a debounce.
5. Playback always starts with `--hotreload`, so saving an entry file or any
   file it imports updates the player once the compile succeeds. Unsaved typing
   updates diagnostics without changing the sound, and unsaved patch files are
   listed in the panel because playback compiles from disk. The player owns file
   watching, so terminal playback and VS Code behave the same way.
6. The LSP process stays separate from the player. Runtime messages appear in a
   YDSP Playback output channel; source diagnostics remain in Problems.

The panel keeps audio playback independent of language server restarts. The
player runs wherever the extension host runs, so in a remote, SSH or container
workspace the audio comes out of that machine and a note is written to the
playback channel. A future structured player status channel can report
compiling, playing, and failed reload states without interpreting console text.

## Verification

Build the host tool and the existing test target using your normal workflow,
then run:

- `YdspProjectTests.*` and `YdspBundle*` for project selection, nested imports,
  standalone bundles, and source-free round trips.
- `YdspJitDiagnosticsTests.*` for the registered source ids that drive hot reload.
- `YdspJitGraphTests.ReservedHostMidiOutputCapacityCoversDenseOutputWithoutAllocation`,
  preferably with allocation hooks enabled.
- `python3 cmake/tools/ydsp_compiler/tests/test_cli.py /absolute/path/to/yup_dsp_compiler`
  for bundle CLI, invalid arguments, standalone `--hotreload`, and LSP
  framing/ranges/unsaved imports.
- The VS Code extension's TypeScript build, then open a source and manifest,
  introduce an imported-file error, and verify Problems and its caret range.
- On hardware: enumerate devices, play a generator and an audio-input effect,
  connect MIDI input/output, then test valid reload, invalid save, corrected
  save, manifest source-list changes, Ctrl+C, and device disconnection. Repeat
  the reload checks for a standalone source with an import.
- In the extension: run a patch from the panel, the editor title and the context
  menu; verify the status bar, the patch pin and the YDSP Playback channel; edit
  a source to trigger hot reload; save an invalid edit and confirm the previous
  patch keeps playing; change a device mid-playback; toggle Follow the active
  patch; then Stop and Restart and check that the child exits when the window
  closes.
