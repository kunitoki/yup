# Changelog

All notable changes to the YUP project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [2.0.0] - Unreleased

### Breaking changes

- macOS: OpenGL rendering backend removed in favor of Metal only
- `LottieReader::parseFile()`, `parseData()`, `parseStream()`, and `parseFromZip()` now return `ResultValue<AnimationComposition::Ptr>` and no longer take a trailing `String* outError` out-parameter; check `wasOk()`/`failed()` and read the message via `getErrorMessage()`.
- `AnimationFrameExporter` is now an instance-based class bound to a `GraphicsContext` (construct `AnimationFrameExporter exporter (ctx);` then call `exporter.renderFrame(anim, …)` / `exporter.renderAllFrames(…)` / `exporter.exportToGif(anim, …)`), so it can own and reuse the GPU matte-composite pipeline across frames instead of recompiling it per frame. The `exportToGif(frames, frameRate, …)` frame-sequence encoder remains a static helper.

### Graphics

- Added a native WebGPU `GraphicsContext` backend for Emscripten via the Emdawnwebgpu port (`RIVE_WEBGPU=2` + `--use-port=emdawnwebgpu`, enabled with the `ENABLE_EMSCRIPTEN_WEBGPU` parameter of `yup_standalone_app`), rendering Rive content through the browser's WebGPU API without Dawn
- Fixed `GpuFrame::begin()` aborting on the Emscripten WebGPU backend: the WGPU context now creates and submits its own command encoder when no external one is provided, matching the Metal/GL/D3D11 self-managed frame model

#### Rive Runtime Bump

- Rive runtime bumped from v0.1.62 to v0.1.155

#### RHI (#129 and #130)

- GraphicsContext GPU context integration. New `GraphicsContext::isGpuAvailable()` capability probe; `gpuContext()` is retained but documented `@internal` as the single backend bridge.
- New `GpuTexture` class (`rhi/yup_GpuTexture.h`): opaque reference-counted GPU texture wrapping `rive::gpu::Texture` or `rive::gpu::RenderCanvas`. Obtained from `GpuCanvas::asTexture()` or constructed internally by `Image::fromTexture()`.
- New `GpuTarget` class (`rhi/yup_GpuTarget.h`): low-level render-pass-only offscreen GPU surface (`create`, `beginRenderPass`, `asTexture`, `asImage`, `readPixels`). Its backing texture is allocated from the context's main render context, so it does not reserve a dedicated `rive::gpu::RenderContext` — use it for custom `GpuPipeline` work (e.g. post-process passes) that needs no 2D drawing.
- New `GpuCanvas` class (`rhi/yup_GpuCanvas.h`): consolidated backend-agnostic offscreen GPU surface that now composes a `GpuTarget` (over a `RenderableTarget`) and creates a non-owning `Graphics` lazily only when 2D drawing is requested.

#### RHI module extraction & GpuDevice

- **New `yup_rhi` module**: the GPU abstraction layer extracted from `yup_graphics` into its own module (depends on `yup_core`, `yup_shading`, `rive_renderer`). All RHI classes (`GpuFrame`, `GpuPipeline`, `GpuBuffer`, `GpuTexture`, `GpuTarget`, `GpuRenderPass`, `GpuPipelineCache`) now live in `yup_rhi`. `yup_graphics` depends on `yup_rhi` for GPU access.
- **`GpuDevice`**: new reference-counted GPU device abstraction (was `GpuContext`). Owns the native GPU device and command queue without requiring a window — can be used for headless GPU compute (e.g. audio DSP on the GPU). Created via `GpuDevice::create(GpuPlatform, Options)`. All RHI factory methods (`GpuFrame::begin`, `GpuPipeline::compile*`, `GpuBuffer::create`, `GpuTarget::create`) now take `GpuDevice::Ptr` for safe shared ownership.
- **`GpuPlatform`** enum: standalone platform enum (`Headless`, `Metal`, `Direct3D`, `OpenGL`, `OpenGLES`, `WebGPU`) replacing the nested `GpuDevice::Api`. `GraphicsContext::getPlatform()` returns it directly — no typedef alias.
- **`GpuColor`** struct (`rhi/yup_GpuTypes.h`): lightweight 4-component GPU color for render options. Implicitly constructable from any type with `getRedFloat()`/`getGreenFloat()`/`getBlueFloat()`/`getAlphaFloat()` (e.g. `yup::Color`), so `GpuRenderOptions { true, Colors::transparentBlack }` works without code changes.
- **`GraphicsContext` simplified**: wraps a `GpuDevice::Ptr` (obtained via `getGpuDevice()` returning `GpuDevice::Ptr`). Offscreen target management (`createOffscreenTarget`, `beginOffscreen`, `endOffscreen`, `readOffscreenPixels`) delegated to `GpuDevice`. Factory accepts optional `GpuDevice::Ptr` to share an existing GPU device.
- **Backends**: `GpuDevice` has native implementations for all platforms (Metal, OpenGL, Direct3D 11, Dawn, WebGPU/Emscripten, Headless). OpenGL backend probes `GL_VERSION` at runtime to detect compute shader support (GL ≥4.3 / GLES ≥3.1).
- **`::Ptr` safety**: all RHI types that own resources (`GpuPipeline`, `GpuBuffer`, `GpuTexture`, `GpuTarget`, `GpuCanvas`) are reference-counted with `::Ptr`. Factory methods take `GpuDevice::Ptr` to keep the device alive for the resource's lifetime. `GpuFrame` is move-only stack RAII and takes `GpuDevice&` (no ownership).

#### Image Formats

- Added TIFF read/write support (`TiffImageFormat`) via libtiff: RGB, RGBA, Grayscale at 8/16-bit; multi-page reading; DPI and EXIF/ICC/XMP metadata extraction.
- Added TGA read/write support (`TgaImageFormat`): uncompressed and RLE-compressed truecolor and grayscale variants; RGB and RGBA output with alpha channel preservation.
- Added animated WebP encoding and decoding support to `WebPImageFormatWriter` and `WebPImageFormatReader`: per-frame metadata (canvas dimensions, frame count, loop count, per-frame delays, dispose/blend modes) and frame decompression with manual compositing.
- Added animated PNG (APNG) encoding and decoding support to `PngImageFormatWriter` and `PngImageFormatReader`: manual chunk-level parsing of `acTL`/`fcTL`/`fdAT` chunks for animation metadata, per-frame libpng decoding via synthetic minimal PNG construction, and canvas compositing supporting all three APNG disposal operations (none, background, previous) and both blend operations (source, over).
- `ImageFormat::Options` struct controls metadata extraction: `.withMetadata(true)` enables text metadata and DPI; `.withRawChunks(true)` enables raw binary chunks (EXIF, ICC, XMP). When both are false (the default), `ImageMetadata` is not allocated — true zero overhead.
- Introduced a ref-counted `ImageMetadata` object (`ImageMetadata::Ptr`) attached to `Image` and `ImageFormatReader::metadata`. DPI, text entries, and raw binary chunks are all accessed through the metadata object only when requested via `Options`.
- Lossless roundtrip tests for all formats (BMP, PNG, WebP, TGA, TIFF, PPM, GIF) now verify pixel-perfect fidelity after write→read; animated roundtrip tests for GIF, WebP, and PNG verify per-frame pixel integrity.


### Shading

- New GLSL→WGSL direct transpiler in `yup_shading`: parses preprocessed GLSL 4.50, lowers GLSL constructs to WGSL equivalents, and emits WGSL 1.0 source. Supports vertex/fragment/compute stages with full builtin mapping, combined sampler splitting, entry-point IO wrapping, and binding assignment matching glslang's SPIR-V assignment 1:1. Does not require SPIR-V or spirv_cross for code generation. Integrated into `ShaderTranspiler`, `ShaderCache`, and `ShaderBundleCompiler`. WGSL variants are supported in YSLB bundles via the `shader_bundler` tool and `yup_add_shader_bundle()` CMake helper.
- New `GpuPipeline` class (`rhi/yup_GpuPipeline.h`): an immutable compiled render pipeline (vertex + fragment shaders plus fixed pipeline state). `compile(ctx, vs, fs, GpuPipelineOptions)`, `compileFromBundle(ctx, ShaderBundle, GpuPipelineOptions)`, and (when `YUP_ENABLE_SHADER_TRANSPILER = 1`) `compileFromGlsl(ctx, vertexGlsl, fragmentGlsl, GpuPipelineOptions)` all return `ResultValue<GpuPipeline::Ptr>`. Pipelines carry all the backend-agnostic mirror enums/structs (`GpuVertexFormat`, `GpuPipelineOptions`, `GpuColorTarget`, `GpuDepthStencilState`, …).
- New `GpuFrame` class (`rhi/yup_GpuFrame.h`): move-only RAII GPU frame scope (`GpuFrame::begin(ctx)` → `submit()` → `waitForGPU()`). Owns the transient GPU resource pools (uniform buffers, texture views, samplers) created while encoding its passes.
- New `GpuRenderPass` class (`rhi/yup_GpuRenderPass.h`): move-only transient render-pass encoder targeting a `GpuCanvas`. Holds the mutable binding state (`setPipeline`, `setTexture`, `setUniformBuffer`, `setVertexBuffer`, `setIndexBuffer`) and encodes draws (`draw`, `drawIndexed`, `finish`).
- New `GpuPipelineCache` class (`rhi/yup_GpuPipelineCache.h`): thread-safe compile-or-fetch cache for `GpuPipeline` keyed by a deterministic SHA1 of the selected native shader sources, entry points, pipeline options, and graphics API. LRU eviction with a configurable entry limit, mirroring `ShaderCache`.
- New `GpuBuffer` class (`rhi/yup_GpuBuffer.h`): reference-counted GPU buffer handle wrapping a backend-native GPU buffer. `GpuBuffer::create(ctx, GpuBufferType, data, byteSize)` uploads immutable vertex/index/uniform data for use with `GpuRenderPass`.
- `Image::fromTexture(GpuTexture::Ptr)`: creates an `Image` wrapping an existing GPU texture (no CPU round-trip). Suitable for `Graphics::drawImage()`.
- `Graphics::drawTexture(GpuTexture::Ptr, Rectangle<float>)`: draws a GPU texture directly without materialising an `Image`, avoiding CPU-side ImagePixelData allocation.

#### Shader Compiler (#126 and #130)

- New glslang (`thirdparty/glslang`), SPIRV-Cross (`thirdparty/spirv_cross`) and SPIRV-Tools (`thirdparty/spirv_tools`) for shader reflection and cross-compilation (GLSL, ESSL, HLSL, MSL).
- New `yup_shading` module for cross platform shader handling.
- New `ShaderBundle` class (`shading/yup_ShaderBundle.h`): RIFF binary format (`.ysl`) that stores original source, per-stage SPIR-V, all transpiled variants (GLSL/ESSL/HLSL/MSL), and full `ShaderReflection` data. Persists to / loads from `OutputStream`, `File`, and `MemoryBlock` via `saveToStream` / `loadFromStream` and friends. Lookup by stage + language via `findShader()`.
- New `ShaderBundleCompiler` class (`shading/yup_ShaderBundleCompiler.h`): drives `ShaderTranspiler` to compile + transpile multiple stage/language combinations in one call and returns a fully-populated `ShaderBundle`. Accepts a `ShaderBundleCompileRequest` with per-stage `ShaderBundleEntry` items (stage, target languages, `TranspileOptions`).
- New `BinaryOutputArchive` / `BinaryInputArchive` pair (`yup_core/serialisation/yup_BinaryArchive.h`): binary stream archives that plug into the `SerialisationTraits` system; used internally by `ShaderBundle` to serialise `ShaderReflection` data into `REFL` RIFF chunks.
- New standalone `yup_shader_bundler` console tool (`cmake/tools/shader_bundler`): takes a `.vert` and `.frag` GLSL (v450 Vulkan dialect) pair on disk and produces a single `.ysl` bundle containing transpiled variants for all target languages (GLSL/ESSL/HLSL/MSL).
- New `yup_add_shader_bundle()` CMake helper (`cmake/yup_shader_bundler.cmake`): builds the `yup_shader_bundler` tool for the host once (cached in the global property `YUP_SHADER_BUNDLER_EXECUTABLE`), runs it at configure time to generate the `.ysl`, and embeds it into a linkable object library via `yup_add_embedded_binary_resources`. Works even when the outer build is cross-compiling, since the tool is built in its own host binary tree without forwarding the cross toolchain. Accepts an `OPTIONS` argument that forwards arbitrary extra flags verbatim to `yup_shader_bundler` (e.g. `--spirv-opt`, `--target-langs`, `-DNAME=VALUE`, `-I<dir>`).

### AI (`yup_ai`)

- New `yup_ai` module (`modules/yup_ai`): LLM client and AI integration classes depending on `yup_core` and `yup_events`.

#### LLM

- `LLMClient` (`yup_LLMClient.h`): abstract base for chat-completion backends with `complete()` and `completeStreaming()` methods, tool-call loop support via `runToolLoop()`, and structured output via `LLMSchema` JSON Schema or GBNF grammars.
- `LLMHttpClient` (`yup_LLMHttpClient.h`): HTTP transport for `LLMClient` with retry and timeout logic, handling streaming SSE and non-streaming JSON responses.
- `LLMClientFactory` (`yup_LLMClientFactory.h`): creates the correct `LLMHttpClient` subclass from `LLMClient::Options::provider`, with convenience factories for each provider.
- `LLMMessage` (`yup_LLMMessage.h`): chat message with four roles (system, user, assistant, tool), optional tool calls, and serialisation to/from OpenAI ChatML JSON.
- `LLMResponse` (`yup_LLMResponse.h`): parsed completion response with choices, token usage, tool-call extraction, streaming chunk accumulation, and error handling.
- `LLMTool` (`yup_LLMTool.h`): callable function descriptor with JSON Schema parameters and a local handler, serialised to OpenAI function-calling format.
- `LLMToolRegistry` (`yup_LLMToolRegistry.h`): thread-safe registry for `LLMTool` instances with snapshot, lookup, dispatch, and tools-array serialisation.
- `LLMSchema` (`yup_LLMSchema.h`): fluent builder for JSON Schema objects (`string`, `number`, `integer`, `boolean`, `array`, `object`, `oneOf`) used in structured-output requests across all providers.

#### LLM Providers

- `LLMOpenAIChatClient` (`yup_LLMOpenAIChatClient.h`): OpenAI Chat Completions API — also compatible with Ollama, DeepSeek, OpenRouter, and llama-server.
- `LLMOpenAIResponsesClient` (`yup_LLMOpenAIResponsesClient.h`): OpenAI Responses API (GPT-5+, reasoning models).
- `LLMAnthropicClient` (`yup_LLMAnthropicClient.h`): Anthropic Messages API (Claude models).
- `LLMGeminiClient` (`yup_LLMGeminiClient.h`): Google Gemini generateContent API.

#### Embeddings

- `EmbeddingModel` (`yup_EmbeddingModel.h`): OpenAI-compatible HTTP embedding model with `embed()` / `embedBatch()` and `cosineSimilarity()` helper.

#### MCP (Model Context Protocol)

- `MCPTypes` (`yup_MCPTypes.h`): JSON-RPC 2.0 request/response/error types, MCP capability flags, tool and resource definitions with `toVar` / `fromVar` serialisation.
- `MCPTransport` (`yup_MCPTransport.h`): abstract transport interface for JSON-RPC messages (stdio, HTTP/SSE, sockets, in-process).
- `MCPClient` (`yup_MCPClient.h`): synchronous MCP client with `initialize()` handshake, `listTools()` / `callTool()`, `listResources()` / `readResource()`, and tool-import bridge `registerToolsWith()`.
- `MCPServer` (`yup_MCPServer.h`): MCP server exposing local YUP tools and resources over a transport, with `registerTool()` / `registerResource()`, `start()` / `stop()`, and placeholder `startStdio()` / `startHttp()`.

#### Python Bindings

- Python bindings for `yup_ai` (`modules/yup_python/bindings/yup_YupAi_bindings.cpp`): exposes LLM client, provider, messages, tools, responses, MCP types, client, and server to Python via pybind11.

### Examples

- `SpinningCubeDemo` example (`examples/graphics`): rewritten to the new RHI shape — `GpuFrame` + `GpuCanvas::beginDraw` + `GpuRenderPass` for both the indexed cube draw and the separable two-pass blur (H+V sharing one `GpuFrame`), `isGpuAvailable()` capability probe, and live GLSL editing via `GpuPipeline::compileFromGlsl`. The default Lottie animation is now played back per-frame into an offscreen `GpuCanvas` (2D path) and sampled by the cube's fragment shader so the animation is texture-mapped onto every cube face.
- `AIDemo` example (`examples/graphics/source/examples/AI.h`): interactive demo for all four LLM providers (OpenAI Chat, OpenAI Responses, Anthropic, Gemini) with model and API key configuration, system prompt editing, streaming and non-streaming completion, tool calling, MCP server integration, and embedded text generation.

### Build System

- justfile recipes now use per-platform build directories (`build/mac`, `build/ios`, `build/android`, `build/emscripten`, `build/ninja`, `build/win`), so switching platforms no longer requires `just clean` and preserves downloaded FetchContent dependencies per platform. The `just build` recipe gains a `PLATFORM` parameter (default `mac`).
- `yup_standalone_app` gains a `MAXIMUM_MEMORY` Emscripten argument (`-sMAXIMUM_MEMORY`); when set it caps the heap that `ALLOW_MEMORY_GROWTH` may reach.
- `yup_tests` wasm build: raised `INITIAL_MEMORY` to 256 MB, added `MAXIMUM_MEMORY` cap of 1 GB, and reduced `STACK_SIZE` to 1 MB to give the heap room for concurrent pthread stress tests; fixes `RuntimeError: memory access out of bounds` in CI.
- Fetched third-party dependencies (SDL3, Perfetto, plugin SDKs) are now cloned shallowly (`--depth 1`) and skip network update checks on reconfigure, speeding up fresh configures and reconfigures. Shallow cloning is automatically disabled when a `GIT_TAG` is a commit hash.
- Android: full 16 KB page size compatibility — generated Gradle projects bumped to AGP 8.5.2 / Gradle 8.7 (uncompressed native libraries are zip-aligned to 16 KB), `jniLibs` packaging made explicitly non-legacy, and `ndkVersion` pinned to r27c (overridable via `NDK_VERSION`), which ships a 16 KB-aligned `libc++_shared.so`. CI NDK updated to r27c accordingly. Application shared libraries were already linked with `-Wl,-z,max-page-size=16384`.

### Testing

- `Component` now befriends a single `ComponentTestHelper<T>` class template instead of accumulating one friend class per test suite; unit tests specialize it (e.g. `ComponentTestHelper<Component>`, `ComponentTestHelper<ComponentEffect>`) to reach private state.

### Bug Fixes

- iOS applications now use the `UIScene` lifecycle, removing UIKit's legacy lifecycle warning and ensuring SDL windows are created for the connected scene.
- Offscreen GPU rendering now supports recursive targets on Metal, OpenGL/GLES, and D3D11. Render contexts are reserved only while a target frame is active, so Lottie alpha/luma mattes, isolated-opacity layers, and cached precomps retain GPU compositing when rendered into an `Image` or `GpuCanvas` without allocating a context per sequential target. Repeated Lottie matte and precomp renders now reuse their canvases rather than allocating GPU textures each frame. Metal child targets allocate only their Rive render-canvas output texture; the CPU readback staging texture is created only when pixels are requested.
- Lottie: track mattes (alpha, alpha-inverted, luma, luma-inverted) now composite the matte source's *rendered alpha* — including its fill opacity, gradients, and anti-aliased edges — instead of hard-clipping the target to the source silhouette. The matte source and target are rendered into offscreen GPU buffers (sized to the fitted on-screen resolution) and multiplied by a fullscreen matte-composite shader. A partially transparent matte source now shows through correctly (e.g. `matte_two_item_with_lowerlayer.json`, whose 65%-opacity source blends the white matted ellipse to pink over the red layer beneath). Falls back to the previous geometric-clip behaviour when no GPU is available (e.g. headless rendering).
- Lottie: `EllipseShape` paths now start at the top (12 o'clock) and follow the shape direction (clockwise for `d == 1`, counter-clockwise for `d == 3`), matching Lottie's convention. Previously they started at the right (3 o'clock) going counter-clockwise, which placed trimmed arcs at the wrong position (e.g. the expanding rings in `world_locations.json` were cut short on the right).
- `Path::withRoundedCorners()` left one corner sharp on closed subpaths whose geometry ended with an explicit segment back to the start vertex (as produced by Lottie bezier `toPath()`). The duplicated start/end point formed a zero-length edge that made that corner degenerate. The trailing duplicate is now dropped, and corners are rounded with a cubic arc (circle kappa) instead of a single quadratic through the vertex, so a square with a full Round Corners modifier becomes a proper circle (e.g. the morphing loader shape in `loader.json`).
- Lottie: trailing top-level modifiers (trim, repeater, rounded-corner) in a shape layer now apply to every preceding top-level group in the run, not just the last one, so a single trim animates all shapes it should (e.g. the knife in `it's_lunch_time!.json`, and the segmented strokes in `imprint.json` / `fingerprint_success.json`). Trailing paints similarly reach all preceding paint-less groups.
- Lottie: animated properties driven by an AfterEffects `loopOut('cycle')` expression (`AnimationProperty<T>::LoopMode`) now repeat their keyframe range instead of freezing on the final value once playback passes the last keyframe. Fixes pulsing markers vanishing after their first cycle (e.g. the orange location circles in `world_locations.json`).
- Lottie: precomposition layers are now rasterized to an offscreen texture sized to the on-screen device resolution instead of the fixed composition size, so precomps no longer look blurry when the animation is scaled up (e.g. `tractor.json`).
- Lottie: layers with partial (animated) opacity are isolated into a transparency layer for correct compositing; this offscreen buffer is now sized to the fitted on-screen resolution instead of the composition size, so small compositions no longer look blurry when scaled up (e.g. `spin,_lil_loader_v2.json`, a 90x90 composition whose fading "stick" layers were rasterized at 90px and upscaled).
- Lottie: Merge Paths (`mm`) is now supported (`AnimationMergePaths`). Boolean modes (Add/Union, Subtract, Intersect, Exclude) combine the preceding path geometry with the matching boolean operation, while the plain "Merge" mode concatenates paths and lets the fill winding rule form counters (holes). Nested paint-less groups only feed their geometry to the parent group's fills/strokes when a Merge Paths modifier is present; otherwise nested groups stay self-contained so paint-less construction guides are not accidentally filled (fixes stray star/cross shapes and per-frame overhead in `pumped_up.json` and `mughead.json`). Fixes shapes built from merged sub-paths rendering only partially (e.g. the red windmill sails in `windmill.json`) without filling in letter counters (e.g. the holes in "O"/"A" in `goal.json`).
- Lottie: the AfterEffects inertial-bounce ("overshoot") position expression (`amp`/`freq`/`decay`) is now approximated via `AnimationTransform` `InertialBounceParams`, producing the decaying oscillation past the last position keyframe. Fixes elements that dropped in without the expected bounce (e.g. `windmill.json`).
- Lottie / `AnimationRenderer::renderComposition`: content that extends beyond the composition viewport (e.g. shapes with coordinates outside the `w`/`h` bounds, as in `jolly_walker.json`) now clips to the fitted composition rectangle instead of the full target bounds, so it no longer spills into the letterbox / pillarbox area when the target rectangle is not the composition's aspect ratio.
- `AnimationTransform::positionAt()` spatial bezier motion paths were nearly straight instead of curved: the second control point used the *next* keyframe's incoming tangent (`k1.tangentIn`) rather than the current segment's own tangent (`k0.tangentIn`). In Lottie both `to` and `ti` belong to the keyframe starting the segment, so a circular motion path (e.g. a shape orbiting on a bezier arc) collapsed toward linear interpolation.
- OpenGL / WebGL: the main frame's rive flush went silently blank (draws degenerate, screen frozen on the last good frame) whenever a `GpuCanvas` committed mid-frame. `endOffscreen()`'s `unbindGLInternalResources()` wipes the shared GL texture units, but the main render context's internal textures (tessellation/gradient/feather/atlas) were only rebound at `begin()` — before `paint()` — so any offscreen 2D flush during paint left the main flush sampling incomplete textures (no GL error; GLES returns zeros). The GL backend now calls `invalidateGLState()` on the flushing context immediately before every `flush()` (main frame and offscreen), making each flush self-contained regardless of how many rive/ore contexts interleave on the one real GL context. Fixes `SpinningCubeDemo` on WASM/WebGL2 appearing frozen (with sporadic 5-15 s updates) and the page turning sluggish while the app still reported ~57 FPS.
- `Graphics::drawTexture` / `drawImage` / transparency layers rendered nothing (transparent) whenever the rive frame ran in atomic interlock mode — always the case on the iOS simulator, and on any platform when raster ordering is disabled. The composite was implemented as a path draw with an image paint, which atomic-mode shaders cannot sample; `Graphics::renderTexture` now routes through `rive::Renderer::drawImage`, which falls back to a dedicated image-rect draw in atomic mode. Fixes invisible Lottie precomps/mattes, `GpuCanvas` composites, and the SpinningCube demo output on the iOS simulator.
- OpenGL / WebGL: `GpuCanvas` textures drawn with `Graphics::drawTexture` (Lottie precomp caches and matte composites) rendered vertically flipped, because the GL canvas source texture is stored bottom-up. `GpuTexture::getOrAdoptGpuTexture()` now prefers the Y-flipped sampled mirror — kept fresh at each canvas flush — matching what `GpuRenderPass` already did for sampled inputs. No change on Metal/D3D, where the mirror is null.
- SDL3 windowing: mouse move/drag was broken on touch platforms (iOS, Android). Motion was synthesized only by polling `SDL_GetGlobalMouseState`, which has no backend implementation there and falls back to window-relative coordinates, so subtracting the window position shifted every move. Touch platforms now consume the touch-synthesized `SDL_EVENT_MOUSE_MOTION` events directly; desktop keeps the global-cursor poll (needed for embedded plugin editors).
- SDL3 windowing: mouse drag events were lost inside embedded plugin editors (notably on macOS, where the host owns the native application so SDL never receives Cocoa mouse focus and suppresses drag motion). Dragging is now synthesized by polling the global cursor while a button is held, on the message thread, for all platforms.
- UBSAN and ASAN fixes throughout the codebase
- AUv3 plugin host bypass is now connected to the processor: the wrapper-owned bypass parameter is created and drives `processBlockBypassed`, and host bypass state is persisted/restored inside the `YUPProcessorState` blob (legacy raw processor state still loads)
- Added bypass parameter handling tests for the AU, CLAP, and VST3 plugin client wrappers (routing to `processBlockBypassed`, bypass state round-trip, and text/value conversion)

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
- Sidechain and multi-bus audio input support across VST3, CLAP, AUv3, AUv2, AAX, and LV2: `AudioBus` gains a `Role` (`Main`/`Auxiliary`) and `isDefaultActive`, `AudioProcessContext` exposes per-bus `inputs`/`outputs` views (`AudioBusBufferView`) with `getMainInput()`/`getAuxiliaryInput()`/`getMainOutput()` accessors, and secondary input buses are forwarded to the processor instead of being discarded

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
- Unified drag-and-drop support in `Component`: `isInterestedInDrag()` / `itemsDropped()` virtuals with a fluent `DragAndDropData` payload (files and text on SDL, URIs reserved for future backends); drops dispatch to the topmost interested component and bubble up to parents
- Added unit coverage for `SystemClipboard` data formats and `Component` drag-and-drop callbacks
- Safe area support: `Component::getSafeAreaBounds()` and `safeAreaChanged()` virtual (backed by `ComponentNative::getSafeAreaBounds()`), so content can avoid display cutouts and system bars on mobile devices
- High dpi support on Windows and Linux X11: window bounds, screen geometry and input coordinates are now logical points everywhere (converted at the SDL boundary), so windows and content scale with the display scale like on macOS; live display scale changes resize the native window keeping the logical size

#### Text
- `TextEditor` and `Label` components ([#16](https://github.com/kunitoki/yup/pull/16), [#55](https://github.com/kunitoki/yup/pull/55))
- Improved fonts: better layouting, variable font axis manipulation, embedded fallback font ([#55](https://github.com/kunitoki/yup/pull/55))
- Clipboard support: text, MIME-typed data with lazy callbacks, and primary selection ([#55](https://github.com/kunitoki/yup/pull/55))

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
- `constructAt()` / `destroyAt()` / `voidify()` in `memory/yup_Memory.h`: portable replacements for `std::construct_at` / `std::destroy_at`, used by `TypeErasedObject`
- `TypeErasedObject` now supports class template argument deduction (deduction guide sizes storage to the stored value) and move construction / assignment from a smaller-sized `TypeErasedObject`
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
