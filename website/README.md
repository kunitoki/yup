# YUP website

Static showcase site for YUP, built with Vite and plain CSS: one black field, YUP blue (`#005aff`) as the only signal color, huge lowercase words, small wide-tracked capitals and monospace text.

```bash
cd website
npm install
npm run dev      # http://localhost:5173
npm run build    # outputs dist/
npm run preview  # serves dist/
```

## Layout

Everything lives in `src/`, which is the Vite root:

- `src/index.html`, `src/modules.html`, `src/showcase.html`, `src/compare.html`, `src/releases.html`, `src/get-started.html` - one file per page, all listed in `pages` in `vite.config.js`.
- `src/404.html` - served by GitHub Pages for any missing path. It sets `<base href="/">` so its assets resolve at any depth, and it is built but kept out of the sitemap.
- `src/partials/` - shared markup, inlined where a page contains `<!-- @name -->`: `head` (the pages' meta, stylesheet and script), `chrome` (the corner links and the search dialog, also used by the home page), `nav` (the links between pages, the current one marked) and `footer`.
- `src/partials/snippets/` - plain code samples (`.cpp`, `.cmake`, `.sh`), inlined and highlighted with Shiki where a page contains `<!-- @code snippets/<file> -->`. Edit them as normal source files; the dev server reloads on save.
- `src/base.css` - colors, reset, corner links and the search dialog, imported by `home.css` and `pages.css`.
- `src/pages.css`, `src/main.js` - styles and script (code tabs, copy buttons, module filter) of every page but the home page.
- `src/search.js` - the docs search dialog, shared by every page.
- `src/home.css`, `src/home.js` - the home page.
- `src/demos/` - the YUP app behind the home page, see below.

## Home page

One black screen with a list of works in huge cut-out letters. A real YUP app compiled to WebAssembly plays across the whole screen, visible only through the letters of the active work (`.mask` multiplies the canvas away everywhere else). Clicking the active work dissolves the mask, `back` or `Esc` closes it. The last line, `start`, links to `get-started.html`.

The app lives in `src/demos/`: a small YUP project (`yup_core`, `yup_events`, `yup_graphics`, `yup_gui`, which bring `yup_rhi`, plus `yup_audio_basics`, `yup_audio_devices` and `yup_dsp`) with one `Demo` component per work, each in its own folder under `source/` with its shader and sound if it has them, and what they share in `source/common/` and an empty `shell.html`, because the page hosts the module itself. The demos are real widgets that turn from flat 2D into lit 3D as the pointer comes near (design in `docs/superpowers/specs/2026-10-19-website-component-demos-design.md`):

- `SurfaceView` shows live Components on 3D meshes: each panel is manually composited, rendered to a texture and drawn through one lit pipeline (`common/shaders/surface.*`, lighting in `common/shaders/light.glsl`), and the pointer is mapped back onto it, so the widgets keep working in 3D. At rest the panels are drawn as plain 2D.
- `Lift` turns the distance between the pointer and a control into a smoothed 0..1 value that drives every transition.
- `Demo` tracks the pointer through YUP mouse events, listening to all its widgets too.
- `RippleEffect` is a `ComponentEffect` (shader `ripple/ripple.frag`) that warps a live panel like liquid glass and maps the pointer back through the warp.
- `Sound` is the one audio output. A demo with sound returns a `SoundSource` from `getSoundSource()`, which fades in when the demo opens and out when it closes; `keys/KeysSound.h` is a small FM electric piano playing a chord per hovered key, and `facet/FacetSound.h` a four operator FM loop through a tempo synced ping-pong delay, its FM amounts set by the four knobs facing the viewer on the folded solid. `ripple/RippleSound.h` turns every ring into a water drop over a quiet drone and runs it through the panel: the faders are a four band EQ, drive saturates, space is a small reverb and power fades it. `layers/LayersSound.h` is a pad over a pulsing drone, darker to brighter across the card and calmer to busier down it, and `reel/ReelSound.h` ticks rows going by and plays the selected preset over a low pad that follows its key. Their filters, delay lines and saturation come from `yup_dsp`.
- `AudioAnalysis` listens to the output with a 256 point FFT: low, mid and high levels and hits (spectral flux), plus the loudness of each note from C3 to C5 from Goertzel filters over a third of a second, long enough to tell neighbouring low notes apart. Each `Demo` gets it smoothed as `hearing` while open, and plays it back on screen: the facet faces pop on hits, the keys light in viridis and rise with their own notes, ripple rings from the middle, the layers breathe with the bass, the reel bends on notes, and `SurfaceView::setAudio` flashes and warms the lighting of the 3D demos. The output starts suspended and `home.js` resumes it on the click that opens a demo, since browsers only start audio from a gesture, and suspends it again a few seconds after closing.
- `Palettes.h` holds the widget palettes; the mode combo box in `layers` switches the app-wide theme between them.

The shaders are precompiled at configure time and embedded in the binary. `home.js` switches demos with `_yupWebsiteSetDemo (index)` (in list order) and forwards pointer moves to the canvas, which sits under the words, so the demos react to the pointer everywhere. To add a demo, add a folder with a `Demo` subclass, include it in `main.cpp`, list it in `Stage.h` and add a line to the list in `index.html` at the same position; new shaders go next to their demo and in the `foreach` list of `src/demos/CMakeLists.txt`.

Build it from the repository root; the target copies `website_demos.js` and `.wasm` into `src/public/yup/` (ignored by git), which the dev server and the build serve as `/yup/`:

```bash
just emscripten Release website_demos
```

Until it is built, or where the browser lacks WebGPU or cross-origin isolation, the page still works: the boot line says why and the active word is simply lit. The module is threaded, so the page needs cross-origin isolation: the dev and preview servers send the COOP/COEP headers, and on GitHub Pages `mini-coi.js` (served from `cmake/platforms/emscripten/`) adds them through a service worker that reloads the page once on the first visit.

## Build-time data

- `<!-- @releases -->` (on `releases.html`) becomes the release notes in `src/partials/releases/<version>.md`, newest first, one collapsible section each with the newest open. The notes are short summaries written for the site, not the changelog: each file starts with `## [<version>] - <date>` (a date of `Unreleased` keeps a draft off the page), followed by `###` groups, bullets and paragraphs, with bold, code, links and `#123` references. Add a file for each new release.
- Docs search (the `search` corner, `Cmd/Ctrl+K` or `/`) reads `virtual:docs-index`, which the `yup-partials` plugin builds from the h1-h3 sections of `docs/**/*.md` and which loads only when the dialog first opens. Links point to the matching section ids on yup.readthedocs.io. Restart the dev server after editing the docs.

## Analytics

Set `goatCounterCode` in `vite.config.js` to your GoatCounter site code to add the cookie-free [GoatCounter](https://www.goatcounter.com/) script to every page. Elements with `data-goatcounter-click="<name>"` (the "Run the synth in your browser" and GitHub buttons) are counted as events. While the code is empty, no script is added.

Screenshots and the logo are referenced straight from `../docs/_static/images` and `../logos/`, so the site never duplicates them. Vite hashes them into `dist/` on build.

## SEO

Each page only declares its `<title>` and `<meta name="description">`. On top of those, the `yup-partials` plugin in `vite.config.js` adds the canonical link and the Open Graph and Twitter card tags, plus schema.org JSON-LD on the home page. On build it also writes `sitemap.xml` (from the `pages` list), `robots.txt` and `og.jpg`, the social card image, which is copied from `docs/_static/images`. All absolute URLs come from `siteUrl` in `vite.config.js`. Add new pages to `pages` so they end up in the sitemap.

## Deployment

`.github/workflows/deploy_website.yml` builds the home page demos (the `website_demos` target, with Emscripten at the version `build_wasm.yml` uses) and then the site, and publishes it to GitHub Pages on every push to `main` that touches `website/`, `docs/` (screenshots and the search index), `logos/`, or what the demos are built from (`CMakeLists.txt`, `cmake/`, `modules/`, `thirdparty/`). It also runs on manual dispatch.
