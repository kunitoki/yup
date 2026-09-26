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

Screenshots are referenced straight from `../docs/_static/images`, so the site never duplicates them. Vite hashes them into `dist/` on build.

## Deployment

`.github/workflows/deploy_website.yml` builds the site and publishes it to GitHub Pages on every push to `main` that touches `website/` or `docs/_static/images/`. It also runs on manual dispatch.
