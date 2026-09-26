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

- `src/index.html`, `src/modules.html`, `src/showcase.html`, `src/get-started.html` - one file per page, all listed in `vite.config.js`.
- `src/partials/` - shared `head`, `header` and `footer`, inlined where a page contains `<!-- @name -->`.
- `src/partials/snippets/` - plain code samples (`.cpp`, `.cmake`, `.sh`), inlined and highlighted with Shiki where a page contains `<!-- @code snippets/<file> -->`. Edit them as normal source files; the dev server reloads on save.
- `src/style.css` - Tailwind entry point and theme tokens (palette from `cmake/platforms/emscripten/shell.html`).
- `src/main.js` - mobile nav, card spotlight, code tabs, copy buttons and the module filter.

Screenshots and the logo are referenced straight from `../docs/_static/images` and `../logo.svg`, so the site never duplicates them. Vite hashes them into `dist/` on build.

## SEO

Each page only declares its `<title>` and `<meta name="description">`. On top of those, the `yup-partials` plugin in `vite.config.js` adds the canonical link and the Open Graph and Twitter card tags, plus schema.org JSON-LD on the home page. On build it also writes `sitemap.xml` (from the `pages` list), `robots.txt` and `og.jpg`, the social card image, which is copied from `docs/_static/images`. All absolute URLs come from `siteUrl` in `vite.config.js`. Add new pages to `pages` so they end up in the sitemap.

## Deployment

`.github/workflows/deploy_website.yml` builds the site and publishes it to GitHub Pages on every push to `main` that touches `website/`, `docs/_static/images/` or `logo.svg`. It also runs on manual dispatch.
