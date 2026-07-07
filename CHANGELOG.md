# Changelog

All notable changes to the YUP project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [2.0.0] - Unreleased

### Graphics

#### Rive Runtime Bump

- Rive runtime bumped from v0.1.62 to v0.1.155

#### Shader Compiler (#126)

- New glslang (`thirdparty/glslang`) and SPIRV-Cross integration (`thirdparty/spirv_cross`) for shader reflection and cross-compilation (GLSL, HLSL, MSL, WGSL)
- New `ShaderBundle` class (`shading/yup_ShaderBundle.h`): RIFF binary format (`.ysl`) that stores original source, per-stage SPIR-V, all transpiled variants (GLSL/ESSL/HLSL/MSL), and full `ShaderReflection` data. Persists to / loads from `OutputStream`, `File`, and `MemoryBlock` via `saveToStream` / `loadFromStream` and friends. Lookup by stage + language via `findShader()`.
- New `ShaderBundleCompiler` class (`shading/yup_ShaderBundleCompiler.h`): drives `ShaderTranspiler` to compile + transpile multiple stage/language combinations in one call and returns a fully-populated `ShaderBundle`. Accepts a `ShaderBundleCompileRequest` with per-stage `ShaderBundleEntry` items (stage, target languages, `TranspileOptions`).
- New `BinaryOutputArchive` / `BinaryInputArchive` pair (`yup_core/serialisation/yup_BinaryArchive.h`): binary stream archives that plug into the `SerialisationTraits` system; used internally by `ShaderBundle` to serialise `ShaderReflection` data into `REFL` RIFF chunks.
- `SerialisationTraits` specialisations for all `ShaderReflection` nested types (`EntryPoint`, `WorkgroupSize`, `ResourceMember`, `ResourceBinding`, `BuiltInBinding`, `SpecializationConstant`, `ShaderReflection`) enabling binary and JSON serialisation of full reflection data. Both `ShaderBundle` and `ShaderBundleCompiler` are compiled only when `YUP_ENABLE_SHADER_TRANSPILER` is `1`.
- `TranspileOptions` now exposes `spirvOptimize` flag; wired through to `glslang::SpvOptions` (disabled by default; requires SPIRV-Tools linked into glslang to take effect).

#### macOS

- OpenGL rendering backend disabled on macOS in favor of Metal

### Bug Fixes

- UBSAN and ASAN fixes throughout the codebase
- `GpuProgram::compile()` now correctly routes HLSL and MSL shader sources: HLSL sources are passed through the ore `ShaderModuleDesc::hlslSource` fields required by the D3D11/D3D12 backends (previously they were collapsed to GLSL, tripping the backend assertion), and the source language is mapped explicitly instead of defaulting all non-WGSL sources to GLSL.
- `GpuProgram` now binds sampled input textures through a shader-resource view (`wrapRiveTexture`) instead of the render-target-only canvas view (`wrapCanvasTexture`). On D3D the canvas view exposes no SRV, so sampling a `GpuCanvas`-backed texture (e.g. feeding a scene render into a blur post-process) previously read nothing and produced a blank result.
- Shader reflection no longer throws (caught) spirv-cross exceptions on built-in interface blocks (e.g. `gl_PerVertex`): struct member offset/stride queries are now guarded by their decorations, and built-in blocks without an `Offset` decoration are skipped instead of triggering noisy first-chance exceptions.
### Graphics RHI

- New `Texture` class (`rhi/yup_Texture.h`): opaque reference-counted GPU texture wrapping `rive::gpu::Texture` or `rive::gpu::RenderCanvas`. Obtained from `GpuCanvas::asTexture()` or constructed internally by `Image::fromTexture()`.
- New `GpuCanvas` class (`rhi/yup_GpuCanvas.h`): consolidated backend-agnostic offscreen GPU surface. Replaces `GraphicsContext::createOffscreenTarget / beginOffscreen / endOffscreen` for all new code. API: `GpuCanvas::create()`, `getGraphics()`, `commit()`, `asTexture()`, `asImage()`, `readPixels()`.
- `Image::fromTexture(Texture::Ptr)`: creates an `Image` wrapping an existing GPU texture (no CPU round-trip). Suitable for `Graphics::drawImage()`.
- `Graphics::drawTexture(Texture::Ptr, Rectangle<float>)`: draws a GPU texture directly without materialising an `Image`, avoiding CPU-side ImagePixelData allocation.
- New `GpuProgram` class (`rhi/yup_GpuProgram.h`): compiled ore render pipeline for custom GPU dispatch. `compile(ctx, vertSource, fragSource)` compiles WGSL/GLSL shader sources — each must supply a mandatory pre-compiled RSTB binding-map sidecar. `setTexture(group, binding, Texture::Ptr)` / `setUniformBuffer(group, binding, data, size)` bind resources. Frame lifecycle: `beginFrame()` → one or more `dispatch(GpuCanvas&)` calls (fullscreen triangle pass per call) → `endFrame()` → optionally `waitForGPU()`. `oreContext()` / `orePipeline()` expose the underlying ore handles for advanced custom vertex/draw calls.
- `GpuProgram` now supports custom indexed and non-indexed geometry rendering, not just fullscreen passes. New `compile(ctx, vs, fs, GpuPipelineOptions)` overload exposes a full pipeline configuration (vertex buffer layouts, primitive topology, index format, cull mode, winding, up to four color targets with per-target blend, depth/stencil state, and MSAA sample count) via backend-agnostic mirror enums/structs. New `setVertexBuffer(slot, GpuBuffer::Ptr)` / `setIndexBuffer(GpuBuffer::Ptr, GpuIndexFormat)` bind geometry, and `draw(canvas, vertexCount, GpuRenderOptions)` / `drawIndexed(canvas, indexCount, GpuRenderOptions)` encode draws with a configurable clear color. `dispatch()` is retained as a convenience for the fullscreen-triangle post-process case.
- `GpuProgram::compileFromBundle(ctx, ShaderBundle, GpuPipelineOptions)`: compiles a program by selecting the native shader variant matching the context's graphics API (Metal→MSL, Direct3D→HLSL, OpenGL(ES)→GLSL/ESSL, WebGPU→WGSL) from a bundle and deriving the RSTB binding-map sidecar from the bundled reflection data. Works without the shader transpiler, so `.ysl` bundles can be consumed at runtime. `GpuProgram::compileFromGlsl(ctx, vertexGlsl, fragmentGlsl, GpuPipelineOptions)` (only when `YUP_ENABLE_SHADER_TRANSPILER = 1`) transpiles GLSL 450 to the backend-native language and compiles it in one call. This routes HLSL/MSL sources correctly through ore, fixing the previously unpopulated `hlslSource` fields when creating shader modules directly.
- New `GpuBuffer` class (`rhi/yup_GpuBuffer.h`): reference-counted GPU buffer handle wrapping a backend-native ore buffer. `GpuBuffer::create(ctx, GpuBufferType, data, byteSize)` uploads immutable vertex/index/uniform data for use with `GpuProgram`.
- New `makeShaderBindingMapBlob(ShaderReflection, ShaderStage)` helper (`rhi/yup_ShaderBindingMap.h`): converts shader reflection data into the ore RSTB binding-map blob required by `GpuShaderSource::bindingMap`, replacing the ad-hoc conversion previously duplicated in examples.
- GraphicsContext ore integration (`Options::enableOreContext = true`): activates the backend-native ore context. `gpuContext()` returns the ore `Context*` (Metal via `ContextMetal::Make`, GL via `ContextGL::Make`, D3D11 via `ContextD3D11::Make`). Required for `GpuProgram::compile()`.
- New `SpinningCubeDemo` example (`examples/graphics`): 3D spinning cube rendered per-frame into a `GpuCanvas` entirely through `GpuProgram` (indexed geometry via `GpuBuffer`, live GLSL editing via `compileFromGlsl`); separable two-pass Gaussian blur (O(radius) H+V instead of O(radius^2) 2-D) wired to a slider via `GpuProgram`. No longer uses the `rive::ore` API directly. Both the cube and blur programs are live-editable (Cube/Blur target toggle) and each program's vertex + fragment GLSL can be saved to / loaded from a `.ysl` `ShaderBundle`.

---

## [1.0.0] - 2026-07-03

### Platform Support

#### Android
- Android window support with `YupActivity` Java class ([#29](https://github.com/kunitoki/yup/pull/29), [#34](https://github.com/kunitoki/yup/pull/34))
- Java bytecode compilation via `yup_android_java.cmake` ([#53](https://github.com/kunitoki/yup/pull/53))
- External storage permissions (`READ_EXTERNAL_STORAGE` / `WRITE_EXTERNAL_STORAGE`) for file access ([#61](https://github.com/kunitoki/yup/pull/61))

#### iOS
- iOS CI pipeline with Xcode toolchain ([#8](https://github.com/kunitoki/yup/pull/8))
- Updated minimum deployment targets: iOS/tvOS 13.0, watchOS 6.0, macOS 11.0 ([#72](https://github.com/kunitoki/yup/pull/72))
- ARC enabled by default on Apple platforms ([#91](https://github.com/kunitoki/yup/pull/91))
- iOS Simulator-specific framework groups (`iosSimFrameworks` / `iosSimWeakFrameworks`) in module declarations ([#48](https://github.com/kunitoki/yup/pull/48))

#### macOS
- macOS message loop reworked: time-sliced event dispatch via `CFRunLoopRunInMode` targeting ~60 Hz; quit event registered with `NSAppleEventManager` for proper Apple Event quit handling ([#47](https://github.com/kunitoki/yup/pull/47))
- `NSSupportsSuddenTermination = false` added to macOS `Info.plist` ([#47](https://github.com/kunitoki/yup/pull/47))

#### Emscripten / WebAssembly
- Full Emscripten/WASM support including `AudioWorklet` audio device ([#25](https://github.com/kunitoki/yup/pull/25))
- WASM threading with exported runtime methods ([#61](https://github.com/kunitoki/yup/pull/61))
- `-msimd128` compile flag and configurable stack size for Emscripten targets ([#98](https://github.com/kunitoki/yup/pull/98))

#### SDL2
- SDL2 integration with libpng, libwebp, and rive_decoders ([#37](https://github.com/kunitoki/yup/pull/37))
- Improved SDL/JUCE Message Manager dispatch loop ([#38](https://github.com/kunitoki/yup/pull/38))
- SDL symbol namespacing to prevent linker conflicts in Apple platform plugins ([#112](https://github.com/kunitoki/yup/pull/112))

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
- `ColorGradient::Spread` enum: `Pad`, `Repeat`, `Reflect` tiling modes with `withSpread()` builder ([#119](https://github.com/kunitoki/yup/pull/119))
- `CubicBezier` class: `pointAt()`, `derivative()`, `length()`, `splitAt()`, bounding box, and intersection ([#119](https://github.com/kunitoki/yup/pull/119))

#### Image Formats
- New image format I/O framework: `ImageFormat`, `ImageFormatReader`, `ImageFormatWriter`, `ImageFormatManager` - plugin-style registry with magic-byte detection and animated image support ([#119](https://github.com/kunitoki/yup/pull/119))
- BMP image format: reader (1/4/8/16/24/32-bpp, RLE4/RLE8, palette) and writer (24-bpp uncompressed), controlled by `YUP_IMAGE_FORMAT_BMP` ([#119](https://github.com/kunitoki/yup/pull/119))
- PPM/PGM/PBM (Netpbm) image format: full P1–P6 plain and binary read/write, controlled by `YUP_IMAGE_FORMAT_PPM` ([#119](https://github.com/kunitoki/yup/pull/119))
- PNG image format via `libpng`: grayscale, grayscale+alpha, RGB, and RGBA at 8- and 16-bit depths, controlled by `YUP_IMAGE_FORMAT_PNG` ([#119](https://github.com/kunitoki/yup/pull/119))
- JPEG image format via `libjpeg`: quality-level encoding, controlled by `YUP_IMAGE_FORMAT_JPEG` ([#119](https://github.com/kunitoki/yup/pull/119))
- WebP image format via `libwebp`, controlled by `YUP_IMAGE_FORMAT_WEBP` ([#119](https://github.com/kunitoki/yup/pull/119))
- Animated GIF image format via `libgif`: per-frame delay, loop count, animated write API (`beginAnimation` / `writeFrame` / `endAnimation`), controlled by `YUP_IMAGE_FORMAT_GIF` ([#119](https://github.com/kunitoki/yup/pull/119))
- `Image::loadFromData()` reimplemented via `ImageFormatManager` ([#119](https://github.com/kunitoki/yup/pull/119))

#### Offscreen Rendering
- `GraphicsContext::OffscreenTarget` abstract interface for opaque platform GPU offscreen resources ([#119](https://github.com/kunitoki/yup/pull/119))
- Offscreen API on `GraphicsContext`: `createOffscreenTarget()`, `beginOffscreen()`, `endOffscreen()`, `readOffscreenPixels()` - implemented for Metal, OpenGL, and D3D backends ([#119](https://github.com/kunitoki/yup/pull/119))
- `Graphics` constructors for rendering to an `Image` or `OffscreenTarget` outside the main frame cycle ([#119](https://github.com/kunitoki/yup/pull/119))
- `Image` gained `renderCanvas` backing (`RenderCanvas`) alongside texture for offscreen render-to-texture; `duplicate()` re-enabled with proper deep copy ([#119](https://github.com/kunitoki/yup/pull/119))
- `Graphics::TransparencyLayer` RAII class for isolated group opacity compositing: renders into an offscreen target and composites back at the given opacity on `commit()` ([#119](https://github.com/kunitoki/yup/pull/119))

### Animations (`yup_animation`)

- New `yup_animation` module: Lottie-compatible animation engine depending on `yup_core` and `yup_graphics` ([#119](https://github.com/kunitoki/yup/pull/119))
- `Animation`: high-level handle with `loadFromFile()`, `loadFromData()`, `loadFromStream()`, `renderFrame()`, `renderAtTime()`, `renderAtProgress()`, `toJson()`, and `saveToFile()` ([#119](https://github.com/kunitoki/yup/pull/119))
- `AnimationPlayer`: stateful playback controller with forward, reverse, and ping-pong direction modes, looping, variable speed, frame-range clamping, seek, and `onFrameChanged` / `onLoopCompleted` / `onPlaybackEnded` callbacks ([#119](https://github.com/kunitoki/yup/pull/119))
- `AnimationEasing`: cubic bezier easing with named presets (`linear`, `easeIn`, `easeOut`, `easeInOut`, `hold`) and `fromLottieTangents()` import ([#119](https://github.com/kunitoki/yup/pull/119))
- `AnimationProperty<T>`: generic animated property with keyframe interpolation; specializations for `float`, `Point<float>`, `Size<float>`, and `Color` ([#119](https://github.com/kunitoki/yup/pull/119))
- `AnimationTransform`: animated anchor, position, scale, rotation, and opacity with conversion to `AffineTransform` at a given frame ([#119](https://github.com/kunitoki/yup/pull/119))
- Full animation data model: `AnimationComposition`, `AnimationGroup`, `AnimationLayer`, `ShapeLayer`, shape types (ellipse, rect, path, star, merge, trim, repeater, polystar), paint types (fill, stroke, linear/radial gradient), and modifiers ([#119](https://github.com/kunitoki/yup/pull/119))
- `LottieReader`: parse `.json` and `.lottie` (ZIP) files from file, string, or stream into the animation data model ([#119](https://github.com/kunitoki/yup/pull/119))
- `LottieWriter`: serialize the animation data model back to Lottie JSON (pretty or compact) with full round-trip support ([#119](https://github.com/kunitoki/yup/pull/119))
- `LottieExpressionEvaluator`: JavaScript expression evaluator for Lottie property expressions via `JavascriptEngine` ([#119](https://github.com/kunitoki/yup/pull/119))
- `AnimationRenderer`: renders an `AnimationComposition` to a `Graphics` context - layer hierarchy, parent-child transforms, matte layers (track-matte), shape fills/strokes/gradients, and image layers ([#119](https://github.com/kunitoki/yup/pull/119))
- `AnimationFrameExporter`: exports individual or all frames to `Image` objects via offscreen GPU, and exports animations to animated GIF files ([#119](https://github.com/kunitoki/yup/pull/119))

### Audio

#### Plugin Support
- VST3 plugin support ([#44](https://github.com/kunitoki/yup/pull/44))
- Audio Unit (AUv2) plugin support ([#93](https://github.com/kunitoki/yup/pull/93), [#106](https://github.com/kunitoki/yup/pull/106))
- Barebone Audio Unit (AUv3) plugin support ([#122](https://github.com/kunitoki/yup/pull/122))
- Barebone AAX plugin support ([#122](https://github.com/kunitoki/yup/pull/122))
- Barebone LV2 plugin support ([#122](https://github.com/kunitoki/yup/pull/122))
- Audio Plugin Host for AUv2, VST3, and CLAP ([#93](https://github.com/kunitoki/yup/pull/93), [#98](https://github.com/kunitoki/yup/pull/98), [#106](https://github.com/kunitoki/yup/pull/106))
- Standalone plugin support with improved audio parameters ([#46](https://github.com/kunitoki/yup/pull/46))
- CLAP/VST3/AU validators and code signing (`YUP_ENABLE_VST3_VALIDATOR`, etc.) ([#106](https://github.com/kunitoki/yup/pull/106))
- pluginval integration for automated VST3 validation ([#67](https://github.com/kunitoki/yup/pull/67))

#### Audio Formats (`yup_audio_formats`)
- New `yup_audio_formats` module: `AudioFormat`, `AudioFormatManager`, `AudioFormatReader`, `AudioFormatWriter`, WAV codec ([#51](https://github.com/kunitoki/yup/pull/51))
- Opus, MP3, FLAC, AAC, CoreAudio, and WMF codec support ([#86](https://github.com/kunitoki/yup/pull/86), [#88](https://github.com/kunitoki/yup/pull/88))

#### DSP (`yup_dsp`)
- New `yup_dsp` module with FFT/windowing via ooura, pffft, vDSP, IPP, FFTW3 ([#71](https://github.com/kunitoki/yup/pull/71))
- Basic IIR filter implementations ([#71](https://github.com/kunitoki/yup/pull/71))
- Linkwitz-Riley crossover filters ([#71](https://github.com/kunitoki/yup/pull/71))
- FIR filter ([#75](https://github.com/kunitoki/yup/pull/75))
- Partitioned convolution ([#75](https://github.com/kunitoki/yup/pull/75))
- Oversampler and Resampler (2×/4×/8×) ([#97](https://github.com/kunitoki/yup/pull/97))
- Noise generators ([#71](https://github.com/kunitoki/yup/pull/71))
- Virtual analog filters: `AnalogTwoPoleFilter`, `AnalogVowelFilter`, `AnalogKorg35Filter`, `AnalogMoogLadderFilter`, `AnalogRolandDiodeFilter`, `CombFilter` ([#103](https://github.com/kunitoki/yup/pull/103))
- Spectral processor ([#116](https://github.com/kunitoki/yup/pull/116))
- Onset detectors (`SpectralFlux` and `ComplexFluxODF`) with perceptual filter bank ([#117](https://github.com/kunitoki/yup/pull/117))
- Time-domain and Frequency-domain stretching with backend selection (homebrew PSOLA plus bungee) ([#104](https://github.com/kunitoki/yup/pull/104))
- Distortion processors with oversampling: `TanhDistortionProcessor`, `BlunterSoftClipperProcessor`, `AaIirHardClipperProcessor` ([#108](https://github.com/kunitoki/yup/pull/108))
- Click-less fractionally addressed delay (FAD) ([#109](https://github.com/kunitoki/yup/pull/109))
- Emscripten `AudioWorklet` audio device ([#25](https://github.com/kunitoki/yup/pull/25))
- MIDI 2.0 / Universal MIDI Packets (UMP) implementation ([#83](https://github.com/kunitoki/yup/pull/83))
- `KMeterState` de-interleaving buffers pre-allocated in `prepare()`, eliminating per-block heap allocation in the audio callback ([#119](https://github.com/kunitoki/yup/pull/119))

#### Synthesiser
- `SynthesiserVoice` converted to `ReferenceCountedObject` with `Ptr = ReferenceCountedObjectPtr<SynthesiserVoice>`; `Synthesiser::addVoice()` now accepts `SynthesiserVoice::Ptr` ([#82](https://github.com/kunitoki/yup/pull/82))

#### Audio Graph (`yup_audio_graph` / `yup_audio_plugin_host`)
- New `yup_audio_graph` and `yup_audio_plugin_host` modules ([#93](https://github.com/kunitoki/yup/pull/93))
- Thread-safe `BufferingAudioSource` with atomic `nextPlayPos` ([#98](https://github.com/kunitoki/yup/pull/98))
- `AudioPlayHead::getContinuousTimeInSamples()`: continuous sample time without loop-reset ([#35](https://github.com/kunitoki/yup/pull/35))

### UI

#### Components and Widgets
- Customizable theming/skinning system ([#13](https://github.com/kunitoki/yup/pull/13))
- `mouseDoubleClick()` virtual method with configurable threshold ([#14](https://github.com/kunitoki/yup/pull/14))
- `KeyboardFocusMode` enum replacing boolean `setWantsKeyboardFocus()`; added `textInput()` callback ([#16](https://github.com/kunitoki/yup/pull/16))
- `PopupMenu` and `ComboBox` components ([#57](https://github.com/kunitoki/yup/pull/57), [#62](https://github.com/kunitoki/yup/pull/62))
- Native file chooser via `FileChooser` ([#61](https://github.com/kunitoki/yup/pull/61))
- Component paint profiling: `PaintProfiler` with ring-buffer stats (min/max/mean/p50/p95/p99) ([#95](https://github.com/kunitoki/yup/pull/95))
- `ComponentNative::getGraphicsContext()` virtual method allowing components to access the GPU context for offscreen operations ([#119](https://github.com/kunitoki/yup/pull/119))
- `MouseListener` weak-referenceable interface for all mouse events; `Component::addMouseListener()` / `removeMouseListener()` ([#30](https://github.com/kunitoki/yup/pull/30))
- Improved slider components (knob, linear, range) and button components ([#70](https://github.com/kunitoki/yup/pull/70))

#### Text
- `TextEditor` and `Label` components ([#16](https://github.com/kunitoki/yup/pull/16), [#55](https://github.com/kunitoki/yup/pull/55))
- Improved fonts: better layouting, variable font axis manipulation, embedded fallback font ([#55](https://github.com/kunitoki/yup/pull/55))
- Clipboard support ([#55](https://github.com/kunitoki/yup/pull/55))

#### Audio GUI (`yup_audio_gui`)
- New `yup_audio_gui` module ([#70](https://github.com/kunitoki/yup/pull/70))
- MIDI keyboard component ([#70](https://github.com/kunitoki/yup/pull/70))
- Filter frequency response visualisation ([#71](https://github.com/kunitoki/yup/pull/71))
- Spectrum analyser component ([#71](https://github.com/kunitoki/yup/pull/71))
- Spectrogram component with peak/RMS/power/PSD level modes ([#102](https://github.com/kunitoki/yup/pull/102))
- Oscilloscope and spectrum analyzer display processors ([#109](https://github.com/kunitoki/yup/pull/109))

### Data Models (`yup_data_model`)

- New `yup_data_model` module ([#15](https://github.com/kunitoki/yup/pull/15))
- `UndoManager` with `Transaction`, `ScopedTransaction`, `UndoableAction` ([#15](https://github.com/kunitoki/yup/pull/15))
- `DataTree` hierarchical data structure with builder pattern and transactional mutations ([#73](https://github.com/kunitoki/yup/pull/73), [#74](https://github.com/kunitoki/yup/pull/74))
- `DataTree` query support ([#74](https://github.com/kunitoki/yup/pull/74))
- `DataTree` schema validation ([#74](https://github.com/kunitoki/yup/pull/74))
- `CachedValue<T>` for type-safe `DataTree` property references ([#74](https://github.com/kunitoki/yup/pull/74))
- `DataTree` complete `UndoableAction` suite for transactional mutations: `PropertySetAction`, `PropertyRemoveAction`, `RemoveAllPropertiesAction`, `AddChildAction`, `RemoveChildAction`, `RemoveAllChildrenAction`, `MoveChildAction`, `CompoundAction` - all with full undo/redo semantics ([#82](https://github.com/kunitoki/yup/pull/82))
- `Identifier` usable as `std::unordered_map` key via `std::hash` specialization ([#27](https://github.com/kunitoki/yup/pull/27))

### Artboard (Rive Integration)

- Improved artboard placement ([#17](https://github.com/kunitoki/yup/pull/17), [#43](https://github.com/kunitoki/yup/pull/43))
- Shared Rive file across multiple `Artboard` components ([#43](https://github.com/kunitoki/yup/pull/43))
- State machine inputs: `setNumberInput()`, `setBoolInput()`, `triggerInput()` ([#17](https://github.com/kunitoki/yup/pull/17), [#43](https://github.com/kunitoki/yup/pull/43))
- `advanceAndApply()` and `durationSeconds()` for timeline control ([#17](https://github.com/kunitoki/yup/pull/17))
- State machine event handling ([#43](https://github.com/kunitoki/yup/pull/43))
- Multi-artboard component support ([#43](https://github.com/kunitoki/yup/pull/43))

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
- `JavascriptEngine::executeWithResult()` to execute a code block and capture the last expression result ([#119](https://github.com/kunitoki/yup/pull/119))
- `JavascriptEngine::registerNativeFunction()` for top-level native function registration by name ([#119](https://github.com/kunitoki/yup/pull/119))
- JavaScript `$` accepted as valid identifier character, required for Lottie expression compatibility ([#119](https://github.com/kunitoki/yup/pull/119))
- WASM: POSIX file API extended with `symlink()`, `dirent.h`, `fnmatch.h`, `utime.h` support; WASMFS enabled for standalone builds ([#36](https://github.com/kunitoki/yup/pull/36), [#59](https://github.com/kunitoki/yup/pull/59))
- Linux: `File::isOnRemovableDrive()` implemented via `/sys/block/<dev>/removable` ([#36](https://github.com/kunitoki/yup/pull/36))

#### Build System
- `zlib` and `oboe` extracted from inline module sources into standalone `thirdparty/` modules for cleaner namespace isolation and build separation ([#6](https://github.com/kunitoki/yup/pull/6), [#7](https://github.com/kunitoki/yup/pull/7))
- `TARGET_IDE_GROUP` parameter on `yup_standalone_app` and `yup_audio_plugin`; all modules, tests, and examples placed in dedicated "Modules", "Tests", and "Examples" IDE folders ([#11](https://github.com/kunitoki/yup/pull/11))
- `appleFrameworks` / `appleWeakFrameworks` module declaration fields unifying iOS and macOS framework lists ([#36](https://github.com/kunitoki/yup/pull/36))
- Per-platform C++ standard override via `*CppStandard` module header fields (`appleCppStandard`, `osxCppStandard`, `linuxCppStandard`, `wasmCppStandard`, `androidCppStandard`, `msftCppStandard`) ([#52](https://github.com/kunitoki/yup/pull/52))
- Platform CMake files reorganized under `cmake/platforms/` and loaded dynamically per target platform ([#36](https://github.com/kunitoki/yup/pull/36))
- Circular dependency detection for YUP modules ([#111](https://github.com/kunitoki/yup/pull/111))
- Module link options support (per-platform `*LinkOptions`) ([#53](https://github.com/kunitoki/yup/pull/53))
- Module target aliases (`yup::yup_core`, etc.) ([#53](https://github.com/kunitoki/yup/pull/53))
- Code coverage: `YUP_ENABLE_COVERAGE`, codecov integration ([#54](https://github.com/kunitoki/yup/pull/54))
- Test sharding support: `--gtest_total_shards` / `--gtest_shard_index` for parallel CI runs ([#119](https://github.com/kunitoki/yup/pull/119))

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
- `Graphics::drawImage()`, `renderStrokePath()`, `renderFillPath()`, `renderFittedText()`: opacity not propagated to renderer - fixed ([#119](https://github.com/kunitoki/yup/pull/119))
- `SIMDRegister`: tail-loop bounds check preventing out-of-bounds access in `load` and `store` paths ([#119](https://github.com/kunitoki/yup/pull/119))
- Mouse-wheel events now dispatched to the component under the cursor when no component is focused ([#30](https://github.com/kunitoki/yup/pull/30))
- Linux `Watchdog`: inotify fd set to non-blocking, read buffer heap-allocated, thread join order corrected to prevent crash on destruction ([#36](https://github.com/kunitoki/yup/pull/36))
