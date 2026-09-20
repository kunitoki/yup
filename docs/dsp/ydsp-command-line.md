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
yup_dsp_compiler run Patch.ydsp-project --hotreload
yup_dsp_compiler run Patch.ydsp-project --audio-type CoreAudio --audio-output "External Headphones" --audio-input none --midi-input "DEVICE-ID"
yup_dsp_compiler run Patch.ydsp-project --midi-output "DEVICE-ID" --sample-rate 48000 --block-size 256
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

MIDI input is applied at the next block boundary. MIDI output is drained on the
control thread approximately every 5 ms, without preserving sample offsets.
This is an auditioning host, not a sample-accurate external MIDI sequencer.
Queues are bounded; overflow counts are printed outside the audio callback and
all-sound-off messages prevent dropped note-offs from leaving voices held.
Press Ctrl+C to stop. Device loss or a device format change stops playback with
a message; restart to use the new configuration.

`run` also accepts an individual `.ydsp` file. `--main` and `--hotreload` require
a project. Playing `.ydsb` files directly is not currently a CLI mode.

## Hot reload

The player polls the manifest and every listed source, waits for a stable poll,
then compiles and prepares a new graph on a worker thread. It swaps graphs at an
audio block boundary and destroys the old graph on the control thread. Audio
processing does not load files, compile, allocate host buffers, or print.

Invalid saves keep the previous graph playing and print diagnostics with
`path:line:column`, source context, and a caret. Fixing and saving the file
triggers another attempt. An invalid manifest retains the last valid watch
list. Changes made while compilation is underway cause that result to be
discarded and the latest files to be compiled.

A successful reload resets processor state, including voices and delay tails.
There is no crossfade or state migration. Changes to audio channel counts require
stopping and starting the player. Device selections belong to the host and are
not stored in the portable project manifest.

## Editor diagnostics

`yup_dsp_compiler --lsp` reserves stdout for LSP messages. The VS Code extension
watches `.ydsp` and `.ydsp-project` files. Project manifests open as YAML.
Diagnostics support unsaved source and manifest contents, explicit imports,
UTF-16 editor positions, and clearing errors after a fix or document closure.
Errors in imports are published against their own file URI.

For a source document, the server searches its directory and parents for a
manifest that lists it. If multiple manifests list a file, open the intended
manifest to validate that project explicitly; diagnostics for a shared source
can be contributed by multiple open projects. Open manifests are validated even
when they contain errors. Standalone sources continue to compile independently.

Library clients can supply unsaved file contents using
`YdspCompileOptions::sourceOverrides`, keyed by absolute paths.

## Proposed VS Code playback workflow

Playback commands and device pickers are a next extension change; diagnostics
support is included now. Use native VS Code controls:

1. **YDSP: Run Project**, also available from a manifest's editor title or context
   menu. Choose a project if several exist, then start one player subprocess.
2. **YDSP: Select Devices** uses `devices --json` and successive
   [Quick Picks](https://code.visualstudio.com/api/ux-guidelines/quick-picks)
   for the audio backend, audio input/output, and MIDI input/output. Remember
   choices in local workspace state. Offer Default for audio and None for MIDI.
3. A status-bar item shows the playing project, with Stop, Restart, and device
   selection commands. Send SIGTERM for orderly shutdown; ensure the child
   exits when the extension closes. Device changes restart the player.
4. Start playback with `--hotreload`. Saving any listed file updates the player
   only after a successful compile. Unsaved typing updates diagnostics without
   changing the sound. The player owns file watching, so terminal playback and
   VS Code behave the same way.
5. Keep the LSP process separate from the player. Show runtime messages in a
   YDSP Playback output channel; source diagnostics remain in Problems. A future
   structured player status channel can report compiling, playing, and failed
   reload states without interpreting console text.

This needs no webview. It also keeps audio playback independent of language
server restarts. The initial extension playback implementation should run on
local workspaces; remote/SSH/container workspaces need an explicit decision
about which machine owns the audio devices.

## Verification

Build the host tool and the existing test target using your normal workflow,
then run:

- `YdspProjectTests.*` and `YdspBundle*` for project selection, nested imports,
  standalone bundles, and source-free round trips.
- `YdspJitGraphTests.ReservedHostMidiOutputCapacityCoversDenseOutputWithoutAllocation`,
  preferably with allocation hooks enabled.
- `python3 cmake/tools/ydsp_compiler/tests/test_cli.py /absolute/path/to/yup_dsp_compiler`
  for bundle CLI, invalid arguments, and LSP framing/ranges/unsaved imports.
- The VS Code extension's TypeScript build, then open a source and manifest,
  introduce an imported-file error, and verify Problems and its caret range.
- On hardware: enumerate devices, play a generator and an audio-input effect,
  connect MIDI input/output, then test valid reload, invalid save, corrected
  save, manifest source-list changes, Ctrl+C, and device disconnection.
