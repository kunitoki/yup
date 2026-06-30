# Changelog

All notable changes to the YUP project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [1.0.0] - Unreleased

### Platform Support

#### Android
- Android window support with `YupActivity` Java class ([#29](https://github.com/kunitoki/yup/pull/29), [#34](https://github.com/kunitoki/yup/pull/34))
- Java bytecode compilation via `yup_android_java.cmake` ([#53](https://github.com/kunitoki/yup/pull/53))
- External storage permissions (`READ_EXTERNAL_STORAGE` / `WRITE_EXTERNAL_STORAGE`) for file access ([#61](https://github.com/kunitoki/yup/pull/61))

#### iOS
- iOS CI pipeline with Xcode toolchain ([#8](https://github.com/kunitoki/yup/pull/8))
- Updated minimum deployment targets: iOS/tvOS 13.0, watchOS 6.0, macOS 11.0 ([#72](https://github.com/kunitoki/yup/pull/72))
- ARC enabled by default on Apple platforms ([#91](https://github.com/kunitoki/yup/pull/91))

#### Emscripten / WebAssembly
- Full Emscripten/WASM support including `AudioWorklet` audio device ([#25](https://github.com/kunitoki/yup/pull/25))
- WASM threading with exported runtime methods ([#61](https://github.com/kunitoki/yup/pull/61))
- `-msimd128` compile flag and configurable stack size for Emscripten targets ([#98](https://github.com/kunitoki/yup/pull/98))

#### SDL2
- SDL2 integration with libpng, libwebp, and rive_decoders ([#37](https://github.com/kunitoki/yup/pull/37))
- Improved SDL/JUCE Message Manager dispatch loop ([#38](https://github.com/kunitoki/yup/pull/38))
- SDL symbol namespacing to prevent linker conflicts in Apple platform plugins ([#112](https://github.com/kunitoki/yup/pull/112))

---

### Graphics

- Reworked rendering backend selection API: `YUP_RIVE_USE_D3D`, `YUP_RIVE_USE_METAL`, `YUP_RIVE_USE_OPENGL`, `YUP_RIVE_USE_DAWN` ([#24](https://github.com/kunitoki/yup/pull/24))
- Headless graphics context and no-op Rive factory for offscreen rendering ([#32](https://github.com/kunitoki/yup/pull/32), [#52](https://github.com/kunitoki/yup/pull/52))
- SVG rendering support ([#56](https://github.com/kunitoki/yup/pull/56), [#64](https://github.com/kunitoki/yup/pull/64))
- SVG 1.1 spec compliance: blend modes, patterns, polygon/polyline ([#100](https://github.com/kunitoki/yup/pull/100), [#118](https://github.com/kunitoki/yup/pull/118))
- Path API improvements with comprehensive examples (basic shapes, arcs, curves, transforms, advanced) ([#56](https://github.com/kunitoki/yup/pull/56))
- `createStrokePolygon()` with feather effects ([#55](https://github.com/kunitoki/yup/pull/55))
- Improved color management and gradient editor ([#87](https://github.com/kunitoki/yup/pull/87))
- Improved CPU and GPU image rendering ([#39](https://github.com/kunitoki/yup/pull/39), [#87](https://github.com/kunitoki/yup/pull/87))
- `Color::brighter()` / `darker()` made `const` ([#32](https://github.com/kunitoki/yup/pull/32))
- `AffineTransform::inverted()` constexpr method ([#19](https://github.com/kunitoki/yup/pull/19))
- constexpr math utilities: `juce_abs()`, `jmap()`, `jlimit()`, `findMinimum()` / `findMaximum()`, `nextPowerOfTwo()`, and more ([#18](https://github.com/kunitoki/yup/pull/18))

---

### Audio

#### Plugin Support
- Initial VST3 plugin support ([#44](https://github.com/kunitoki/yup/pull/44))
- Audio Unit (AUv2) support ([#93](https://github.com/kunitoki/yup/pull/93), [#106](https://github.com/kunitoki/yup/pull/106))
- Audio Plugin Host for AUv2, VST3, and CLAP on macOS ([#93](https://github.com/kunitoki/yup/pull/93), [#98](https://github.com/kunitoki/yup/pull/98), [#106](https://github.com/kunitoki/yup/pull/106))
- Standalone plugin support with improved audio parameters ([#46](https://github.com/kunitoki/yup/pull/46))
- pluginval integration for automated VST3 validation ([#67](https://github.com/kunitoki/yup/pull/67))
- CLAP/VST3/AU validators and code signing (`YUP_ENABLE_VST3_VALIDATOR`, etc.) ([#106](https://github.com/kunitoki/yup/pull/106))

#### Audio Formats (`yup_audio_formats`)
- New `yup_audio_formats` module: `AudioFormat`, `AudioFormatManager`, `AudioFormatReader`, `AudioFormatWriter`, WAV codec ([#51](https://github.com/kunitoki/yup/pull/51))
- Opus, MP3, FLAC, AAC, CoreAudio, and WMF codec support ([#86](https://github.com/kunitoki/yup/pull/86), [#88](https://github.com/kunitoki/yup/pull/88))

#### DSP (`yup_dsp`)
- New `yup_dsp` module with FFT/windowing via pffft and dr_libs ([#71](https://github.com/kunitoki/yup/pull/71))
- Basic IIR filter implementations ([#71](https://github.com/kunitoki/yup/pull/71))
- Linkwitz-Riley crossover filters ([#71](https://github.com/kunitoki/yup/pull/71))
- FIR filter ([#75](https://github.com/kunitoki/yup/pull/75))
- Partitioned convolution ([#75](https://github.com/kunitoki/yup/pull/75))
- Oversampler and resampler (2×/4×/8×) ([#97](https://github.com/kunitoki/yup/pull/97))
- Noise generators ([#71](https://github.com/kunitoki/yup/pull/71))
- Virtual analog filters: `AnalogTwoPoleFilter`, `AnalogVowelFilter`, `AnalogKorg35Filter`, `AnalogMoogLadderFilter`, `AnalogRolandDiodeFilter`, `CombFilter` ([#103](https://github.com/kunitoki/yup/pull/103))
- Spectral processor ([#116](https://github.com/kunitoki/yup/pull/116))
- Onset detector (`ComplexFluxODF`) with perceptual filter bank ([#117](https://github.com/kunitoki/yup/pull/117))
- Time-domain stretching with backend selection (timeDomain, bungee) ([#104](https://github.com/kunitoki/yup/pull/104))
- Distortion processors with oversampling: `TanhDistortionProcessor`, `BlunterSoftClipperProcessor`, `AaIirHardClipperProcessor` ([#108](https://github.com/kunitoki/yup/pull/108))
- Click-less fractionally addressed delay (FAD) ([#109](https://github.com/kunitoki/yup/pull/109))
- Emscripten `AudioWorklet` audio device ([#25](https://github.com/kunitoki/yup/pull/25))
- MIDI 2.0 / Universal MIDI Packets (UMP) implementation ([#83](https://github.com/kunitoki/yup/pull/83))

#### Audio Graph (`yup_audio_graph` / `yup_audio_plugin_host`)
- New `yup_audio_graph` and `yup_audio_plugin_host` modules ([#93](https://github.com/kunitoki/yup/pull/93))
- Thread-safe `BufferingAudioSource` with atomic `nextPlayPos` ([#98](https://github.com/kunitoki/yup/pull/98))

---

### UI

#### Components and Widgets
- Customizable theming/skinning system ([#13](https://github.com/kunitoki/yup/pull/13))
- `mouseDoubleClick()` virtual method with configurable threshold ([#14](https://github.com/kunitoki/yup/pull/14))
- `KeyboardFocusMode` enum replacing boolean `setWantsKeyboardFocus()`; added `textInput()` callback ([#16](https://github.com/kunitoki/yup/pull/16))
- `PopupMenu` and `ComboBox` components ([#57](https://github.com/kunitoki/yup/pull/57), [#62](https://github.com/kunitoki/yup/pull/62))
- Native file chooser via `FileChooser` ([#61](https://github.com/kunitoki/yup/pull/61))
- Component paint profiling: `PaintProfiler` with ring-buffer stats (min/max/mean/p50/p95/p99) ([#95](https://github.com/kunitoki/yup/pull/95))

#### Text
- `TextEditor` and `Label` components ([#16](https://github.com/kunitoki/yup/pull/16), [#55](https://github.com/kunitoki/yup/pull/55))
- Improved font: better layouting, variable font axis manipulation, embedded fallback font ([#55](https://github.com/kunitoki/yup/pull/55))
- Clipboard support ([#55](https://github.com/kunitoki/yup/pull/55))

#### Audio GUI (`yup_audio_gui`)
- New `yup_audio_gui` module ([#70](https://github.com/kunitoki/yup/pull/70))
- MIDI keyboard component ([#70](https://github.com/kunitoki/yup/pull/70))
- Improved slider components (knob, linear, range) and button components ([#70](https://github.com/kunitoki/yup/pull/70))
- Filter frequency response visualisation ([#71](https://github.com/kunitoki/yup/pull/71))
- Spectrum analyser component ([#71](https://github.com/kunitoki/yup/pull/71))
- Spectrogram component with peak/RMS/power/PSD level modes ([#102](https://github.com/kunitoki/yup/pull/102))
- Oscilloscope and spectrum analyzer display processors ([#109](https://github.com/kunitoki/yup/pull/109))
- Image drawing support ([#39](https://github.com/kunitoki/yup/pull/39), [#87](https://github.com/kunitoki/yup/pull/87))

---

### Data Models (`yup_data_model`)

- New `yup_data_model` module ([#15](https://github.com/kunitoki/yup/pull/15))
- `UndoManager` with `Transaction`, `ScopedTransaction`, `UndoableAction` ([#15](https://github.com/kunitoki/yup/pull/15))
- `DataTree` hierarchical data structure with builder pattern and transactional mutations ([#73](https://github.com/kunitoki/yup/pull/73), [#74](https://github.com/kunitoki/yup/pull/74))
- `DataTree` query support ([#74](https://github.com/kunitoki/yup/pull/74))
- `DataTree` schema validation ([#74](https://github.com/kunitoki/yup/pull/74))
- `CachedValue<T>` for type-safe `DataTree` property references ([#74](https://github.com/kunitoki/yup/pull/74))
- `Identifier` usable as `std::unordered_map` key via `std::hash` specialization ([#27](https://github.com/kunitoki/yup/pull/27))

---

### Artboard (Rive Integration)

- Improved artboard placement ([#17](https://github.com/kunitoki/yup/pull/17), [#43](https://github.com/kunitoki/yup/pull/43))
- Shared Rive file across multiple `Artboard` components ([#43](https://github.com/kunitoki/yup/pull/43))
- State machine inputs: `setNumberInput()`, `setBoolInput()`, `triggerInput()` ([#17](https://github.com/kunitoki/yup/pull/17), [#43](https://github.com/kunitoki/yup/pull/43))
- `advanceAndApply()` and `durationSeconds()` for timeline control ([#17](https://github.com/kunitoki/yup/pull/17))
- State machine event handling ([#43](https://github.com/kunitoki/yup/pull/43))
- Multi-artboard component support ([#43](https://github.com/kunitoki/yup/pull/43))

---

### Core & Utilities

#### New Modules
- `yup_simd`: SIMD vectorization framework ([#107](https://github.com/kunitoki/yup/pull/107))
- `yup_python`: Python bindings (from popsicle) ([#65](https://github.com/kunitoki/yup/pull/65))

#### `yup_core` Additions
- `SqliteDatabase` with `Statement` and `Transaction` ([#94](https://github.com/kunitoki/yup/pull/94))
- Perfetto profiling: `YUP_ENABLE_PROFILING`, `Profiler` singleton, `YUP_PROFILE_START` / `YUP_PROFILE_STOP` macros ([#20](https://github.com/kunitoki/yup/pull/20))
- `Watchdog` file watching utility ([#50](https://github.com/kunitoki/yup/pull/50))
- `URL` copy and move constructors ([#58](https://github.com/kunitoki/yup/pull/58))
- `messageThreadID` made atomic (removed mutex) ([#26](https://github.com/kunitoki/yup/pull/26))
- `ReferenceCountedObject::incReferenceCount()` / `decReferenceCount()` made `const` ([#28](https://github.com/kunitoki/yup/pull/28))
- `DatagramSocket` multicast overloads with local IP ([#60](https://github.com/kunitoki/yup/pull/60))
- `ResultValue<T>::valueOr()` ([#96](https://github.com/kunitoki/yup/pull/96))
- `AudioSampleBuffer::fill()` overloads ([#110](https://github.com/kunitoki/yup/pull/110))

#### Build System
- Circular dependency detection for YUP modules ([#111](https://github.com/kunitoki/yup/pull/111))
- Module link options support (per-platform `*LinkOptions`) ([#53](https://github.com/kunitoki/yup/pull/53))
- Module target aliases (`yup::yup_core`, etc.) ([#53](https://github.com/kunitoki/yup/pull/53))
- Code coverage: `YUP_ENABLE_COVERAGE`, codecov integration ([#54](https://github.com/kunitoki/yup/pull/54))

---

### Bug Fixes

- Crash at startup when height/width is 0 on custom-scaled screens ([#21](https://github.com/kunitoki/yup/pull/21))
- Application never quits: incorrect `quitMessagePosted` ordering in `stopDispatchLoop()` ([#42](https://github.com/kunitoki/yup/pull/42))
- Redraw issues and app icon rendering on macOS ([#31](https://github.com/kunitoki/yup/pull/31))
- iOS toolchain: removed hardcoded `DEVELOPER_DIR` path ([#72](https://github.com/kunitoki/yup/pull/72))
- CoreAudio thread safety: atomic operations replacing mutex ([#76](https://github.com/kunitoki/yup/pull/76))
- SMPTE timecode validation, SSE macro, and memory fixes ([#78](https://github.com/kunitoki/yup/pull/78))
- ZIP timestamp: missing `>>1` for 2-second resolution ([#92](https://github.com/kunitoki/yup/pull/92))
- `StyledText::clear()` fully resets state; caret bounds and glyph index for empty lines ([#96](https://github.com/kunitoki/yup/pull/96))
- Duplicated SDL symbols in Apple plugins ([#112](https://github.com/kunitoki/yup/pull/112))
- `ComboBox` popup re-opens on click-to-dismiss; added `ignoreMouseDownAfterPopupDismissal` ([#114](https://github.com/kunitoki/yup/pull/114))
- `AudioDeviceManager` destructor race on `midiCallbackLock` ([#115](https://github.com/kunitoki/yup/pull/115))
- Android oboe: `__ANDROID__` preprocessor instead of `ANDROID` ([#9](https://github.com/kunitoki/yup/pull/9))
