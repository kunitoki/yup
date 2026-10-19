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

- `src/index.html`, `src/modules.html`, `src/showcase.html`, `src/compare.html`, `src/get-started.html` - one file per page, all listed in `pages` in `vite.config.js`.
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

The app lives in `src/demos/`: a small YUP project (`yup_core`, `yup_events`, `yup_graphics`, `yup_gui`, which bring `yup_rhi`) with one `Demo` component per work and an empty `shell.html`, because the page hosts the module itself. The demos are real widgets that turn from flat 2D into lit 3D as the pointer comes near (design in `docs/superpowers/specs/2026-10-19-website-component-demos-design.md`):

- `SurfaceView` shows live Components on 3D meshes: each panel is manually composited, rendered to a texture and drawn through one lit pipeline (`shaders/surface.*`, lighting in `shaders/light.glsl`), and the pointer is mapped back onto it, so the widgets keep working in 3D. At rest the panels are drawn as plain 2D.
- `Lift` turns the distance between the pointer and a control into a smoothed 0..1 value that drives every transition.
- `Demo` tracks the pointer through YUP mouse events, listening to all its widgets too.
- `RippleEffect` is a `ComponentEffect` (shader `shaders/ripple.frag`) that warps a live panel like liquid glass and maps the pointer back through the warp.
- `Palettes.h` holds the widget palettes; the mode combo box in `layers` switches the app-wide theme between them.

The shaders are precompiled at configure time and embedded in the binary. `home.js` switches demos with `_yupWebsiteSetDemo (index)` (in list order) and forwards pointer moves to the canvas, which sits under the words, so the demos react to the pointer everywhere. To add a demo, add a `Demo` subclass, list it in `Stage.h` and add a line to the list in `index.html` at the same position; new shaders go in the `foreach` list of `src/demos/CMakeLists.txt`.

Build it from the repository root; the target copies `website_demos.js` and `.wasm` into `src/public/yup/` (ignored by git), which the dev server and the build serve as `/yup/`:

```bash
just emscripten Release website_demos
```

Until it is built, or where the browser lacks WebGPU or cross-origin isolation, the page still works: the boot line says why and the active word is simply lit. The module is threaded, so the page needs cross-origin isolation: the dev and preview servers send the COOP/COEP headers, and on GitHub Pages `mini-coi.js` (served from `cmake/platforms/emscripten/`) adds them through a service worker that reloads the page once on the first visit.

## Build-time data

- Docs search (the `search` corner, `Cmd/Ctrl+K` or `/`) reads `virtual:docs-index`, which the `yup-partials` plugin builds from the h1-h3 sections of `docs/**/*.md` and which loads only when the dialog first opens. Links point to the matching section ids on yup.readthedocs.io. Restart the dev server after editing the docs.

## Analytics

Set `goatCounterCode` in `vite.config.js` to your GoatCounter site code to add the cookie-free [GoatCounter](https://www.goatcounter.com/) script to every page. Elements with `data-goatcounter-click="<name>"` (the "Run the synth in your browser" and GitHub buttons) are counted as events. While the code is empty, no script is added.

Screenshots and the logo are referenced straight from `../docs/_static/images` and `../logo.svg`, so the site never duplicates them. Vite hashes them into `dist/` on build.

## SEO

Each page only declares its `<title>` and `<meta name="description">`. On top of those, the `yup-partials` plugin in `vite.config.js` adds the canonical link and the Open Graph and Twitter card tags, plus schema.org JSON-LD on the home page. On build it also writes `sitemap.xml` (from the `pages` list), `robots.txt` and `og.jpg`, the social card image, which is copied from `docs/_static/images`. All absolute URLs come from `siteUrl` in `vite.config.js`. Add new pages to `pages` so they end up in the sitemap.

## Deployment

`.github/workflows/deploy_website.yml` builds the site (not the wasm demos yet, so the deployed home page shows "not built yet") and publishes it to GitHub Pages on every push to `main` that touches `website/`, `docs/` (screenshots and the search index), a module header in `modules/*/` or `logo.svg`. It also runs on manual dispatch.
