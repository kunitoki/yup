# YUP website

Static showcase site for YUP, built with Vite and Tailwind CSS v4.

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
- `src/partials/` - shared `head`, `header` and `footer`, inlined where a page contains `<!-- @name -->`.
- `src/partials/snippets/` - plain code samples (`.cpp`, `.cmake`, `.sh`), inlined and highlighted with Shiki where a page contains `<!-- @code snippets/<file> -->`. Edit them as normal source files; the dev server reloads on save.
- `src/style.css` - Tailwind entry point and theme tokens (palette from `cmake/platforms/emscripten/shell.html`).
- `src/main.js` - mobile nav, card spotlight, code tabs, copy buttons, the module filter and the docs search dialog.

## Build-time data

- `<!-- @count modules -->` becomes the number of `modules/yup_*` folders. `<!-- @count <name> -->` counts the `<li>` items or `chip-on` chips inside the page element marked `data-count="<name>"`, so a stat always matches the list it summarizes.
- Docs search (the header button, `Cmd/Ctrl+K` or `/`) reads `virtual:docs-index`, which the `yup-partials` plugin builds from the h1-h3 sections of `docs/**/*.md` and which loads only when the dialog first opens. Links point to the matching section ids on yup.readthedocs.io. Restart the dev server after editing the docs.

## Analytics

Set `goatCounterCode` in `vite.config.js` to your GoatCounter site code to add the cookie-free [GoatCounter](https://www.goatcounter.com/) script to every page. Elements with `data-goatcounter-click="<name>"` (the "Run live" and GitHub buttons) are counted as events. While the code is empty, no script is added.

Screenshots and the logo are referenced straight from `../docs/_static/images` and `../logo.svg`, so the site never duplicates them. Vite hashes them into `dist/` on build.

## SEO

Each page only declares its `<title>` and `<meta name="description">`. On top of those, the `yup-partials` plugin in `vite.config.js` adds the canonical link and the Open Graph and Twitter card tags, plus schema.org JSON-LD on the home page. On build it also writes `sitemap.xml` (from the `pages` list), `robots.txt` and `og.jpg`, the social card image, which is copied from `docs/_static/images`. All absolute URLs come from `siteUrl` in `vite.config.js`. Add new pages to `pages` so they end up in the sitemap.

## Deployment

`.github/workflows/deploy_website.yml` builds the site and publishes it to GitHub Pages on every push to `main` that touches `website/`, `docs/` (screenshots and the search index), a module header in `modules/*/` (the module count) or `logo.svg`. It also runs on manual dispatch.
