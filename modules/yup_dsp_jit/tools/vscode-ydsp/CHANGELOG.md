# Changelog

All notable changes to this extension will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.0.0] - Unreleased

### Added

- Patch Player sidebar view in the YDSP Activity Bar container, split into a
  **Performance** tab (keyboard icon: transport and the workspace patch list) and
  a **Settings** tab (gear icon: device selects, sample rate, block size, test
  note sender and the playback setting toggles), with the active tab remembered.
- The patch list is ordered naturally by path (`Saw2.ydsp` before `Saw10.ydsp`)
  and keeps that order across refreshes.
- Pinned playback: at most one `yup_dsp_compiler run` subprocess per window,
  identical requests are never respawned, and the optional
  `ydsp.player.followActiveEditor` setting retargets that single player when
  switching between patches.
- Patch playback: **YDSP: Run Patch** starts one `yup_dsp_compiler run`
  subprocess for the active `.ydsp` or `.ydsp-project`, always with
  `--hotreload`, so editing an imported library reloads a standalone patch too.
  The command is offered in the editor title, the explorer context menu and the
  command palette.
- Device selection: **YDSP: Select Audio/MIDI Devices** prompts for the audio
  backend, audio output, audio input, and MIDI input and output from
  `devices --json`, remembering the choices per workspace. The device list is
  cached and refreshed on demand.
- MIDI selects and quick picks list device names only; the CoreMIDI identifier
  chain that used to follow an em dash stays the selected value, where the
  player matches it.
- Playback control: a status-bar item with Run, Stop, Restart and device
  selection, a dedicated YDSP Playback output channel for player messages, and
  the `ydsp.player.verbose`, `ydsp.player.arguments`, `ydsp.player.sampleRate`
  and `ydsp.player.blockSize` settings.
- Unsaved patch files are reported in the panel, since playback compiles from
  disk.
- `.ydsp-project` manifests use their own language id (`ydsp-project`) with a
  dedicated icon (`images/ydsp-project-icon.png`), instead of the YAML icon;
  YAML highlighting is kept through a grammar that includes `source.yaml`, and
  `#` is the comment token.

- Offline YDSP bundle tooling is now documented alongside the extension.

- TextMate grammar (`source.ydsp`) for `.ydsp` files: declarations
  (`processor`, `graph`, `struct`, `func`, `node`), endpoints and state,
  types, builtin constants, intrinsics, event shapes, graph composition
  operators, annotation blocks, comments, strings and numbers.
- Language configuration: comment toggling, bracket matching/auto-closing,
  block folding and indentation rules.
- Snippets: processor, processor-block, graph (connection and algebra forms),
  event handler, func, for loop, state array, declare, annotation.
