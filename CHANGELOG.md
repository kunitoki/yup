# Changelog

All notable changes to the YUP project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [2.0.0] - Unreleased

### Linux CI

- `build_linux.yml` sets `YUP_TEST_REQUIRE_GPU=1`, so a GPU-backed test that cannot create a device fails the job rather than skipping. A skip is indistinguishable from a pass in a CI summary, which would let a runner that lost llvmpipe report green while proving nothing. It also sets `YUP_TEST_ARTIFACT_DIR` and uploads the PNGs a failing pixel comparison leaves behind

### Tests

- Added `tests/helpers/yup_GpuTestDevice.h`: creates a real on-GPU `GpuDevice` for the current platform, so rendering tests run against the backend the application uses rather than `GpuPlatform::Headless`, which produces no pixels. Metal needs no window; the OpenGL path creates a hidden SDL window and makes its context current. Reports a reason and lets the test skip when no usable GPU is present
- Added `tests/helpers/yup_PixelCompare.h`: a top-down RGBA8 `Bitmap`, readback from an `OffscreenTarget`, a tolerance-aware comparator that dumps both images to PNG on failure, and a local-flatness measure that detects uninitialised GPU memory without needing a reference image. Artifacts go to `YUP_TEST_ARTIFACT_DIR` when set
- Added `tests/yup_rhi/yup_GpuParity.cpp`: cross-platform backend parity tests covering exact clear colours, clear overwrite, target aliasing, a Rive frame clear through `beginOffscreen`, and the documented RGBA8 top-down readback contract
- Added `tests/yup_graphics/yup_GraphicsParity.cpp`: drawing parity tests that run the same `Graphics` calls on every backend and assert geometry at sample points rather than against a golden image, so antialiasing differences between rasterizers cannot cause a false failure. Covers `fillAll`, rectangle placement and offset, draw order, and the coordinate origin, which is the case that would otherwise go unnoticed because OpenGL framebuffers start bottom-left and Metal textures top-left. Also pins that `GpuCanvas::beginDraw` resets the canvas to transparent black and discards the colour passed to `create`

### Breaking changes

- macOS: OpenGL rendering backend removed in favor of Metal only
- `LottieReader::parseFile()`, `parseData()`, `parseStream()`, and `parseFromZip()` now return `ResultValue<AnimationComposition::Ptr>` and no longer take a trailing `String* outError` out-parameter; check `wasOk()`/`failed()` and read the message via `getErrorMessage()`.
- `AnimationFrameExporter` is now an instance-based class bound to a `GraphicsContext` (construct `AnimationFrameExporter exporter (ctx);` then call `exporter.renderFrame(anim, …)` / `exporter.renderAllFrames(…)` / `exporter.exportToGif(anim, …)`), so it can own and reuse the GPU matte-composite pipeline across frames instead of recompiling it per frame. The `exportToGif(frames, frameRate, …)` frame-sequence encoder remains a static helper.
- Config macro `YUP_EMBED_DEFAULT_THEME_TEXT_FONT` renamed to `YUP_EMBED_DEFAULT_THEME_TEXT_SERIF_FONT`; the embedded default text font now only covers the serif font. A new `YUP_EMBED_DEFAULT_THEME_TEXT_MONOSPACE_FONT` config selects whether the monospace theme font is embedded.
- `SyntaxDefinition` no longer carries colors: `getColor()`, `getSelectionColor()` and the JSON `"colors"` section were removed. Token and editor colors now live in the new `CodeEditorScheme` (see `CodeEditor::setScheme`).
- `Font` loading is now static-only: the instance `loadFromData()` / `loadFromFile()` methods were removed in favor of `Font::loadFontFromData()`, `Font::loadFontFromFile()`, `Font::loadFontFromFirstAvailableFile()`, `Font::loadSerifSystemTextFont()` and `Font::loadMonospaceSystemTextFont()`, all returning `ResultValue<Font>`.

### Core

- Added a `YAML` class: a self-contained YAML parser and writer converting between YAML text and `var` (`parse`, `fromString`, `toString`, `writeToStream`, `FormatOptions`), with core-schema type resolution, block/flow collections, block scalars, and anchors/aliases/merge keys

### Graphics

- Fixed GPU compute silently stalling after a few frames on OpenGL with some drivers (AMD desktop GL): compute now runs on a dedicated, unshared GL context (`GpuDevice::Options::computeContextActivator`, routed through `GpuDevice::runOnComputeContext()`) which exclusively owns every compute resource — pipeline compilation, dispatches, storage buffer create/update/readback and deletion — falling back to the rendering context when unavailable. The GL compute pass also saves and restores the program and `GL_UNIFORM_BUFFER` bindings it touches, so it can no longer desync Rive's cached GL state
- Fixed GL storage buffers being deleted right after creation (moving a `GpuBuffer::Impl` copied the plain GL buffer name, so the moved-from object's destructor freed the just-created buffer) and a crash when releasing GPU buffers or compute pipelines after their window closed (GL releases are routed through the owning device and skipped once the window's contexts are gone)
- Fixed Emscripten randomly rendering nothing or freezing the tab: the render-thread rework unbound the GL context between frames so the render thread could take it, but on Emscripten rendering is timer-driven on the single browser thread and message-thread work between frames (image decodes, font atlas uploads) still issues GL calls — with no context current those throw in JS and kill the main loop. With timer-driven rendering the window context now stays permanently current
- Fixed Emscripten freezing the browser tab when a demo requested a headless GL compute device: with no current WebGL context every GL call throws in JS, killing the requestAnimationFrame main loop. The GL `GpuDevice` now fails construction gracefully when no WebGL context is current and `isComputeAvailable()` reports false on a device whose GL initialization failed. `GpuAudioProcessingDemo` also sized its CPU ring buffers only when GPU compute was available, so the no-GPU audio path wrote and read out of bounds
- Fixed `SDLComponentNative::renderFrame()` on Emscripten encoding a frame with a zero-sized render target, which trips Rive's `beginFrame()` assertion — fatal there, since an assertion abort throws inside the `requestAnimationFrame` callback and permanently kills the browser's main loop. It now skips rendering entirely until a non-zero content size has been observed
- Added a native WebGPU `GraphicsContext` backend for Emscripten via the Emdawnwebgpu port (`RIVE_WEBGPU=2` + `--use-port=emdawnwebgpu`, enabled with the `ENABLE_EMSCRIPTEN_WEBGPU` parameter of `yup_standalone_app`), rendering Rive content through the browser's WebGPU API without Dawn
- Fixed `GpuFrame::begin()` aborting on the Emscripten WebGPU backend: the WGPU context now creates and submits its own command encoder when no external one is provided, matching the Metal/GL/D3D11 self-managed frame model
- Fixed a crash on Windows when creating any native window: the D3D11 `GpuDevice` was built with an already moved-from `ID3D11Device`, and the Direct3D `GraphicsContext` created a second device whose swapchain textures could not be used by the render context. Both now share a single `ID3D11Device`
- Fixed the Emscripten WebGPU `GraphicsContext` never storing its surface size, leaving the offscreen copy at 0x0
- Fixed `GpuDevice::updateBuffer()` failing for every vertex, index and uniform buffer on the WebGPU, Dawn and D3D11 backends: those overrides handled native storage buffers only and returned false instead of delegating ore-backed buffers to the base class, the way the Metal and OpenGL overrides do
- Implemented `GpuDevice::readBuffer()` for D3D11, which previously reported `isComputeAvailable()` but had no override, so every storage buffer readback silently failed through the base class. It copies into a cached `D3D11_USAGE_STAGING` buffer on the immediate context (ordered after the dispatch) and maps it for reading
- `GpuComputePass` on D3D11 now unbinds the UAV slots it bound when the pass finishes, so a storage buffer is no longer left bound for writing while a later readback or draw reads it
- Fixed `GpuDevice::readBuffer()` never succeeding on the Emscripten WebGPU backend: it mapped its staging buffer with `WGPUCallbackMode_AllowProcessEvents` and then tested the result in the same call, but WebGPU buffer mapping only resolves through the JavaScript event loop, so the callback could not have run. The WGPU backend now pipelines the readback over a ring of three staging buffers using `WGPUCallbackMode_AllowSpontaneous`, which completes on its own between main-loop ticks — no ASYNCIFY needed
- `GpuDevice::readBuffer()` is no longer documented as unconditionally blocking. Whether it blocks is a property of the backend: Metal, D3D11 and OpenGL read back in lockstep and fill the destination every call, while WebGPU cannot map synchronously and so trails the GPU by a frame or two. Callers must now own the destination across calls and treat a false return as "no new data yet" rather than an error — the previous contents stay valid
- `ComputeParticlesDemo`: keeps drawing the last particle snapshot on frames where no new one has landed, so it renders on the Emscripten WebGPU backend instead of showing nothing. The status label reports the landed-snapshot count alongside the frame count
- `Component`'s effect path now reuses its offscreen `GpuCanvas` across frames while the component size is unchanged, instead of allocating (and freeing) a full-size render target every frame. On a size change the outgoing canvas is released before the replacement is created, so its `RenderContext` lease returns to the pool rather than forcing a second context to be reserved permanently
- `ComponentEffectsDemo`: shader effects now share a common base that compiles the pipeline at most once instead of retrying a failed compile on every frame, reports the compile error in the status label and on the console, and shows the CPU time spent applying the effect next to the paint time
- `SDLComponentNative` now renders each window on its own dedicated render thread instead of the message thread: the component-tree walk runs under a `MessageManagerLock` while GL command submission and buffer swap happen unlocked, so multiple windows no longer serialize their frame rendering (and vsync waits) on the message thread
- Fixed `SDLComponentNative::repaint()` calling `-[NSWindow screen]` (via `getSize()` → `getWindowUnitsPerPoint()` → `SDL_GetDisplayForWindow()`) from the render thread, which macOS's Main Thread Checker flags since AppKit requires that call on the main thread. It now uses the already up-to-date `screenBounds` cached by the main-thread window event handlers instead of querying the display live
- Fixed `SDLComponentNative::runWithGraphicsContext()` never invoking its callback on non-OpenGL desktop backends (Metal, Direct3D), silently dropping the work. `Component::renderSubtreeOffscreen()` routes through this hook, so any component with a `ComponentEffect` set (e.g. `ComponentEffectsDemo`) rendered nothing on Metal/D3D; `SDLComponentNative::runWithComputeContext()` falls back to the same hook when no dedicated compute context exists, so GPU compute work initiated off the render thread was silently dropped there too
- Fixed `WaitableTimer`'s non-Windows fallback overshooting frame deadlines by several milliseconds on macOS: it blocked for the entire remaining wait on a single `condition_variable::wait_until`, whose wake time is subject to OS scheduling / timer-coalescing latency. It now blocks for the bulk of the wait and closes the last few milliseconds with a tiered busy-wait against `Time::getMillisecondCounterHiRes()`, restoring the precision the pre-`WaitableTimer` implementation had

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

#### Compute Shaders & GPU Audio

- New `GpuComputePipeline` class (`rhi/yup_GpuComputePipeline.h`): an immutable compiled compute pipeline that bypasses ore to go directly to the backend-native API (Metal `MTLComputePipelineState`, D3D11 `ID3D11ComputeShader`, WebGPU/Dawn `wgpu::ComputePipeline`, OpenGL `GL_COMPUTE_SHADER`). `compile(ctx, source, GpuWorkgroupSize)`, `compileFromBundle(ctx, ShaderBundle)`, and `compileFromGlsl(ctx, glsl)` (when `YUP_ENABLE_SHADER_TRANSPILER = 1`) all return `ResultValue<GpuComputePipeline::Ptr>`.
- New `GpuComputePass` class (`rhi/yup_GpuComputePass.h`): move-only RAII compute dispatch encoder (`GpuComputePass::begin(device)`). Binds a `GpuComputePipeline`, storage buffers (`setStorageBuffer`), uniform buffers (`setUniformBuffer`), and textures (`setTexture`), then dispatches workgroups via `dispatch(gx, gy, gz)`.
- `GpuBuffer` extended with `GpuBufferType::storage`: native storage buffer creation for each backend (Metal `MTLBuffer`, D3D11 structured buffer + UAV, WebGPU `Storage` buffer, OpenGL `GL_SHADER_STORAGE_BUFFER`). Storage buffers are bound to `GpuComputePass::setStorageBuffer()`.
- `GpuDevice` backends expose native compute handles: `getDevice()`/`getCommandQueue()` (Metal), `getD3DDevice()`/`getD3DDeviceContext()` (D3D11), `getWgpuDevice()`/`getWgpuQueue()` (WebGPU/Emscripten), `getBackendDevice()`/`getDevice()`/`getQueue()` (Dawn).
- `GpuAudioProcessingDemo` example: real-time GPU-accelerated audio effect (gain + soft clipper) using compute shaders. Captures live audio via `AudioIODeviceCallback`, uploads to GPU storage buffers, dispatches a compute shader, and reads back processed audio — all on the audio I/O thread.
- New `GpuDevice::updateBuffer()`: writes new data into an existing storage buffer without reallocating it (Metal `contents` memcpy, D3D11 `UpdateSubresource`, WebGPU/Dawn `WriteBuffer`, GL `glBufferSubData`). Fixes `GpuAudioProcessingDemo` reallocating its input storage buffer every audio callback, which caused audible stutter. The gain/mix parameters remain a uniform buffer (as before) — that path is unaffected and its small per-dispatch allocation is negligible next to the audio-block-sized buffer this fix removes.
- Fixed `ShaderTranspiler`'s MSL backend assigning storage/uniform buffer indices via spirv-cross's own auto-incrementing scheme instead of the shader's declared `layout(binding=N)`: added `CompilerMSL::Options::enable_decoration_binding = true` so the compiled `[[buffer(N)]]` index always matches the declared binding, matching what `GpuComputePass`'s native dispatch (which binds slots as `group*16+binding` with no reflection indirection) requires. This was silently producing zero output from any Metal compute shader with more than one storage/uniform buffer, including `GpuAudioProcessingDemo`.
- Metal `GpuDevice`/`GpuComputePass` calls now wrap their Objective-C work in `@autoreleasepool` blocks — without one, real-time callers (e.g. an audio thread with no ambient pool) accumulated command buffers/encoders indefinitely.

#### Image Formats

- Added TIFF read/write support (`TiffImageFormat`) via libtiff: RGB, RGBA, Grayscale at 8/16-bit; multi-page reading; DPI and EXIF/ICC/XMP metadata extraction.
- Added TGA read/write support (`TgaImageFormat`): uncompressed and RLE-compressed truecolor and grayscale variants; RGB and RGBA output with alpha channel preservation.
- Added animated WebP encoding and decoding support to `WebPImageFormatWriter` and `WebPImageFormatReader`: per-frame metadata (canvas dimensions, frame count, loop count, per-frame delays, dispose/blend modes) and frame decompression with manual compositing.
- Added animated PNG (APNG) encoding and decoding support to `PngImageFormatWriter` and `PngImageFormatReader`: manual chunk-level parsing of `acTL`/`fcTL`/`fdAT` chunks for animation metadata, per-frame libpng decoding via synthetic minimal PNG construction, and canvas compositing supporting all three APNG disposal operations (none, background, previous) and both blend operations (source, over).
- `ImageFormat::Options` struct controls metadata extraction: `.withMetadata(true)` enables text metadata and DPI; `.withRawChunks(true)` enables raw binary chunks (EXIF, ICC, XMP). When both are false (the default), `ImageMetadata` is not allocated — true zero overhead.
- Introduced a ref-counted `ImageMetadata` object (`ImageMetadata::Ptr`) attached to `Image` and `ImageFormatReader::metadata`. DPI, text entries, and raw binary chunks are all accessed through the metadata object only when requested via `Options`.
- Lossless roundtrip tests for all formats (BMP, PNG, WebP, TGA, TIFF, PPM, GIF) now verify pixel-perfect fidelity after write→read; animated roundtrip tests for GIF, WebP, and PNG verify per-frame pixel integrity.
- `StyledText::TextModifier::appendText()` gained a `Color` overload that creates (and caches per color) a solid fill paint, and `Graphics::fillFittedText()` now honors per-run style paints when every run carries one — enabling syntax-colored text. Single-color `StyledText` usage is unchanged. `Font` gained `isEmpty()`.
- New `Font` static loaders: `Font::loadFontFromData()`, `Font::loadFontFromFile()`, `Font::loadFontFromFirstAvailableFile()`, `Font::loadSerifSystemTextFont()` and `Font::loadMonospaceSystemTextFont()`, all returning `ResultValue<Font>` (`wasOk()` / `failed()` / `getValue()`). The former theme-local system font lookup helpers moved into `Font`; macOS/iOS use the CoreText system UI fonts, other platforms try well-known system font files.

### UI

- `ApplicationTheme` now exposes `setDefaultMonospaceFont()` / `getDefaultMonospaceFont()`, mirroring the existing default and icon font APIs. The default theme populates it from the embedded (or system) monospace font.
- The default theme now embeds JetBrains Mono Variable (SIL OFL) as its monospace font when `YUP_EMBED_DEFAULT_THEME_TEXT_MONOSPACE_FONT = 1` (forced on Emscripten), falling back to the system monospace font otherwise. The `tools/embed_font.py` regenerates the `.inc` byte arrays from any font file.
- New `CodeDocument` (line-based text model with `UndoManager`-backed edits, positions, and incremental change notifications), `SyntaxDefinition` (JSON-driven language descriptions loaded from data/files or the built-in C++ / GLSL / Python definitions), `CodeTokeniser` (incremental per-line tokenizer with a line-state machine for multi-line constructs and lazy re-tokenization), and the `CodeEditor` component: syntax-highlighted editing, caret/selection with anchor semantics, clipboard, undo/redo, read-only, smart auto-indent, an optional line-number gutter with breakpoint markers, find/replace (find-all, next/previous with wrap, replace-one, replace-all in one undo step, match highlighting), bracket matching, and an optional minimap overview. Defaults to the theme's monospace font. See `docs/ui/code-editor.md`.
- Added a built-in XML `SyntaxDefinition` (available as `SyntaxDefinition::getBuiltIn ("xml")` and matched for `.xml`, `.svg`, `.html`, `.xaml` and other markup extensions), with `<!-- -->` block comments, tag/attribute punctuation and `<? ?>` / `<!` / `</` / `/>` operator highlighting. The `CodeEditor` demo now has a language dropdown to switch between the built-in C++ / GLSL / Python / XML definitions.
- Fixed `CodeDocument`: `newLineChars` was default-constructed to an empty string instead of `"\n"`, breaking `getText()`, `getTextInRange()`, and character-offset calculations for all multi-line documents; `applyEdit()` returned a wrong caret column for single-line insertions (omitted `startIndex`), making every subsequent undo call operate on an inverted range and silently no-op; removed the `endsWithNewline` special case that returned a pre-newline position and similarly broke undo for Enter at the beginning of a line or in the middle of a line.
- Fixed `CodeEditor`: `undo()` and `redo()` now clamp `caretPosition` to the new document length and clear the selection after each operation, preventing an out-of-bounds caret after undo shrinks the document; `replaceNext()` now uses the position returned by `replaceRange` instead of `selectionStart + replacement.length()`.
- Fixed `CodeTokeniser`: cutting or deleting text that removes one or more lines left the token cache larger than the document and the forward-propagation stability check could declare a line whose content had shifted "unchanged", returning stale tokens (wrong syntax colors) for every line below the cut point. `codeDocumentChanged` now shrinks the cache to the new document line count and proactively marks all shifted lines dirty before the stability pass runs. The same problem existed in the other direction and was more visible in practice: inserting a line (pressing Enter, or a multi-line paste) grew the document but `codeDocumentChanged` had no branch for it at all, so every cached entry at or after the edit point kept referring to whatever used to be at that index — one or more lines off from where it actually was — and the stability check could decide a shifted-in line's state was "unchanged" and never mark it dirty, leaving it with stale, wrongly-sized tokens that fail to tile the line and fall back to unhighlighted plain text. Both directions are now handled the same way: resize to the new line count and mark everything from the edit point to the new end dirty, so misaligned cache entries are always discarded and recomputed from the live document text rather than reused.
- Fixed `CodeDocument::setText()` freezing for several seconds on a large paste: its line-splitting helper indexed the (UTF-8-backed) input `String` by character position inside the split loop, and both `operator[]` and `length()` are O(n) for UTF-8, turning the split into O(n²). It now walks the text once with a `CharPointer`.
- Fixed `CodeEditor` drawing selected/highlighted/caret text past the gutter and minimap when scrolled horizontally, since nothing clipped that content to the text area; the gutter's painted background also stopped 4px short of where the text area actually starts (disagreeing with the hit-test boundary used by `mouseDown()`), reading as misalignment on the left. The minimap overview now merges lines that map to less than one device pixel row into a single bar instead of issuing one `fillRect()` per source line on every paint regardless of visibility.
- Fixed `StyledText::update()` calling `Font::getPath()` (a CoreText round-trip on Apple platforms) once per glyph *occurrence* instead of once per unique glyph; a glyph's outline is the same every time for a given font, so it's now cached and reused, cutting a measured 240ms of the 453ms spent reshaping text on a single keystroke.
- Fixed `CodeEditor` reshaping (tokenizing, laying out and re-tessellating) the entire document on every single edit, making typing in a large file cost seconds per keystroke (measured: 2s for one backspace in a large file, mostly `StyledText::update()`). `styledText` now only ever holds the currently visible lines rather than the whole document; scrolling reshapes just the newly-visible range. Selection, search highlights, the caret, and Up/Down arrow navigation were adjusted to work correctly when their target is outside the currently-shaped range (falling back to an exact document-position computation rather than depending on `styledText`). Components that never call `setSize()`/`setBounds()` on their `CodeEditor` (as none of its unit tests do) keep shaping the whole document, since there's no meaningful "visible range" to restrict to without a real size.
- `CodeEditor` now renders through a `CodeEditorScheme` (new `code/yup_CodeEditorScheme.h`): every color — background, gutter, caret, current line, selection, search highlight, breakpoint and the per-token syntax colors — is stored keyed by `Identifier` (`CodeEditorScheme::setColor` / `getColor`, string constants in `CodeEditorScheme::ColorId`) and switched with `CodeEditor::setScheme`. Built-in well-known schemes are provided via `CodeEditorScheme::getBuiltIn`: `monokai`, `alabaster`, `oneDark`, `solarizedDark` and `solarizedLight`. The editor's painting moved into the theme (Themes v1) as a registered `CodeEditor` component style, and a vertical auto-hide `ScrollBar` now appears when the document overflows the viewport. The `CodeEditor` demo gained a scheme dropdown.


### Shading

- New GLSL→WGSL direct transpiler in `yup_shading`: parses preprocessed GLSL 4.50, lowers GLSL constructs to WGSL equivalents, and emits WGSL 1.0 source. Supports vertex/fragment/compute stages with full builtin mapping, combined sampler splitting, entry-point IO wrapping, and binding assignment matching glslang's SPIR-V assignment 1:1. Does not require SPIR-V or spirv_cross for code generation. Integrated into `ShaderTranspiler`, `ShaderCache`, and `ShaderBundleCompiler`. WGSL variants are supported in YSLB bundles via the `shader_bundler` tool and `yup_add_shader_bundle()` CMake helper.
- New `GpuPipeline` class (`rhi/yup_GpuPipeline.h`): an immutable compiled render pipeline (vertex + fragment shaders plus fixed pipeline state). `compile(ctx, vs, fs, GpuPipelineOptions)`, `compileFromBundle(ctx, ShaderBundle, GpuPipelineOptions)`, and (when `YUP_ENABLE_SHADER_TRANSPILER = 1`) `compileFromGlsl(ctx, vertexGlsl, fragmentGlsl, GpuPipelineOptions)` all return `ResultValue<GpuPipeline::Ptr>`. Pipelines carry all the backend-agnostic mirror enums/structs (`GpuVertexFormat`, `GpuPipelineOptions`, `GpuColorTarget`, `GpuDepthStencilState`, …).
- New `GpuFrame` class (`rhi/yup_GpuFrame.h`): move-only RAII GPU frame scope (`GpuFrame::begin(ctx)` → `submit()` → `waitForGPU()`). Owns the transient GPU resource pools (uniform buffers, texture views, samplers) created while encoding its passes.
- New `GpuRenderPass` class (`rhi/yup_GpuRenderPass.h`): move-only transient render-pass encoder targeting a `GpuCanvas`. Holds the mutable binding state (`setPipeline`, `setTexture`, `setUniformBuffer`, `setVertexBuffer`, `setIndexBuffer`) and encodes draws (`draw`, `drawIndexed`, `finish`).
- New `GpuPipelineCache` class (`rhi/yup_GpuPipelineCache.h`): thread-safe compile-or-fetch cache for `GpuPipeline` keyed by a deterministic SHA1 of the selected native shader sources, entry points, pipeline options, and graphics API. LRU eviction with a configurable entry limit, mirroring `ShaderCache`.
- New `GpuBuffer` class (`rhi/yup_GpuBuffer.h`): reference-counted GPU buffer handle wrapping a backend-native GPU buffer. `GpuBuffer::create(ctx, GpuBufferType, data, byteSize)` uploads immutable vertex/index/uniform data for use with `GpuRenderPass`.
- `Image::fromTexture(GpuTexture::Ptr)`: creates an `Image` wrapping an existing GPU texture (no CPU round-trip). Suitable for `Graphics::drawImage()`.
- `Graphics::drawTexture(GpuTexture::Ptr, Rectangle<float>)`: draws a GPU texture directly without materialising an `Image`, avoiding CPU-side ImagePixelData allocation.
- `GpuRenderPass` no longer creates a sampler and a uniform buffer per draw. The linear/clamp-to-edge samplers that fill a layout's sampler bindings are created once when the `GpuPipeline` is compiled, and uniform buffers come from a size-bucketed pool on the `GpuDevice` that recycles them when a frame reports GPU completion — so a steady-state workload stops allocating GPU objects after its first frames. `GpuFrame` stays stack RAII; nothing changes for callers.
- Fixed the GLSL→WGSL transpiler rejecting comma-separated members in a struct or interface block (`uniform Params { float s, r, rx, ry; }`), which failed with `Expected ';'`. Each declarator now becomes its own member and binds its own array specifiers.

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

- `StyledText` caret bounds, hit-testing and selection rectangles now use line-relative glyph x positions computed with the same accumulation as drawing, instead of rive's paragraph-relative `GlyphRun::xpos`. Character positions were wrong on soft-wrapped lines (off by the width of all preceding text in the paragraph) and selection was drawn shifted on wrapped text; the caret at the first character of a wrapped line now lands on that line's left edge.
- iOS applications now use the `UIScene` lifecycle, removing UIKit's legacy lifecycle warning and ensuring SDL windows are created for the connected scene.
- Offscreen GPU rendering now supports recursive targets on Metal, OpenGL/GLES, and D3D11, so Lottie alpha/luma mattes, isolated-opacity layers, and cached precomps retain GPU compositing when rendered into an `Image` or `GpuCanvas`. Each `RenderableTarget` leases a Rive render context exclusively for its lifetime and returns it to the pool when destroyed. Repeated Lottie matte and precomp renders now reuse their canvases rather than allocating GPU textures each frame. Metal child targets allocate only their Rive render-canvas output texture; the CPU readback staging texture is created only when pixels are requested.
- Fixed undefined offscreen contents when nesting pooled render targets on all GPU backends. Render context slots were recycled whenever no frame was currently active, so two long-lived targets could share one slot; once their frames nested — which happens as Lottie matte and precomp layers cross their in/out points and the nesting order changes between frames — the inner target skipped `beginFrame` and was then flushed against the outer target's frame descriptor.
- Lottie: a matte layer no longer paints another matte layer's content. Drawing a matte result only queues a reference to its canvas texture, which the enclosing frame resolves at flush time, but the canvas lease was released as soon as the layer finished. Since every matte in a composition is sized to the same fitted rectangle, the pool handed the same canvas triple to the next matte layer, which overwrote the pixels already queued and left only the last matte visible (e.g. `world_locations.json`'s four matted dots collapsed to one and its continent outlines disappeared; `insta_camera.json` lost its animated circles). Leases are now held until the composition render completes.
- `GpuFrame` now waits for the GPU before releasing the texture views, uniform buffers and samplers it keeps alive for its encoded render passes. Those passes reference them by raw pointer, and `submit()` does not block, so letting a frame go out of scope freed them while the GPU was still reading — corrupting the pass output progressively, as the freed memory only starts being handed back out after the allocator has churned for a while (the growing magenta flashes in `bell.json`). `waitForGPU()` is now only needed explicitly when results are required before the end of the frame's scope, and is idempotent so waiting explicitly costs no more than one stall. Move-assignment drains the frame it replaces for the same reason.
- `AffineTransform::getScaleFactor()` is now independent of rotation. It averaged the absolute values of the matrix diagonal and ignored the shear terms, so a rotated transform reported `scale * cos(angle)` — falling to zero at 90 degrees. It now measures the lengths of the transformed basis vectors. Lottie precomposition and matte canvases are sized from this value, so a layer under an animated rotation (e.g. `bell.json`, whose precomposition is parented to a rotating null) requested a different pixel size on every frame, reallocating its canvases mid-frame and flashing while a queued draw still referenced the previous ones.
- Lottie: the matte canvas pool now replaces an idle slot of a different size instead of appending a new one. Nothing removed slots, so a layer whose on-screen size changed every frame added three canvases — each leasing a Rive render context — per frame, without bound.
- `GpuCanvas::create()` takes a `std::optional<Color> clearColor`, defaulting to transparent black, and fills the new canvas with it so it is safe to sample before anything is drawn into it (pass `std::nullopt` to leave the contents undefined). The backing texture is allocated uninitialized and a 2D frame whose draw list ends up empty is not guaranteed to honour its `loadAction=clear`, so a canvas could previously be composited while still holding undefined GPU memory (the magenta flashes in `bell.json`, whose only content is one matted precomposition). The clear is issued through the new `GpuDevice::clearOffscreen()`, which encodes it with the backend's native API — a clear binds no pipeline, buffers or samplers, so it needs neither a render pass nor a submit/wait cycle.
- `GpuCanvas::beginDraw()` now drops the target's cached `GpuTexture` wrap, as its documentation already claimed. The wrap memoizes the Rive texture handle it resolved, so a pooled canvas reused across frames kept handing out the handle resolved on the frame it was first sampled.
- Lottie: a failed matte composite no longer blits undefined GPU memory over the matted layer. The result canvas is written only by the composite render pass — nothing else clears it, and its backing texture is allocated uninitialized — but the pass result was ignored and the texture composited regardless, flashing an arbitrary color. The renderer now falls back to the geometric-clip matte path when the composite fails.
- Lottie: a paint-less nested group now contributes its geometry to the enclosing group's paints with its own modifiers applied. The geometry was rebuilt from raw shapes, dropping the nested group's trim, repeater, merge-paths and rounded-corner modifiers, which is what defines the outline: RubberHose rigs draw a limb as a 4-point star trimmed to a quarter, so the parent stroke painted the whole star instead of an arc (the stray stars in `mughead.json` and `pumped_up.json`).
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

### Documentation

- Added a dedicated DSP documentation area (`docs/dsp/`) covering `yup_dsp` end to end: math/windowing/noise, FFTs and spectral analysis, filter design and filter implementations, dynamics and metering, onset detection, convolution and delay, resampling, and time-stretching/pitch-shifting

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
