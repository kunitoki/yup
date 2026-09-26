import { readFileSync } from "node:fs";
import { basename, extname, resolve } from "node:path";
import { defineConfig, normalizePath } from "vite";
import tailwindcss from "@tailwindcss/vite";
import { createHighlighter } from "shiki";

const root = resolve(import.meta.dirname, "src");
const partialsDir = resolve(root, "partials");
const pages = ["index", "modules", "showcase", "get-started"];

// Production origin, used for canonical links, social cards, the sitemap and robots.txt.
const siteUrl = "https://yup.audio";
const socialImage = resolve(import.meta.dirname, "../docs/_static/images/yup_prism_synth.jpg");

// Syntax colors tuned to the site palette.
const codeTheme = {
    name: "yup",
    type: "dark",
    colors: { "editor.background": "#00000000", "editor.foreground": "#E6EAF2" },
    tokenColors: [
        { scope: ["comment", "punctuation.definition.comment"], settings: { foreground: "#7C8798", fontStyle: "italic" } },
        { scope: ["keyword", "storage", "variable.language", "keyword.control.directive", "punctuation.definition.directive"], settings: { foreground: "#FF7AB2" } },
        { scope: ["string", "punctuation.definition.string"], settings: { foreground: "#FC6A5D" } },
        { scope: ["constant.numeric", "constant.language", "constant.other"], settings: { foreground: "#D0BF69" } },
        { scope: ["entity.name.function", "support.function"], settings: { foreground: "#67E8F9" } },
        { scope: ["entity.name.type", "entity.name.class", "entity.name.scope-resolution", "entity.other.inherited-class", "support.type", "variable.parameter"], settings: { foreground: "#6FB6FF" } },
    ],
};

const codeLanguages = { ".cpp": "cpp", ".cmake": "cmake", ".sh": "shellscript" };

const highlighter = createHighlighter({ themes: [codeTheme], langs: Object.values(codeLanguages) });

const readPartial = (path) => readFileSync(resolve(partialsDir, path), "utf8");

const pageUrl = (page) => (page === "index.html" ? `${siteUrl}/` : `${siteUrl}/${page}`);

// Canonical, Open Graph and Twitter tags built from the page's own <title> and description,
// plus schema.org data for the home page.
function seoTags(page, html) {
    const title = html.match(/<title>(.*?)<\/title>/s)[1];
    const description = html.match(/<meta name="description" content="(.*?)"/s)[1];
    const url = pageUrl(page);

    const tags = [
        `<link rel="canonical" href="${url}" />`,
        `<meta property="og:type" content="website" />`,
        `<meta property="og:site_name" content="YUP!" />`,
        `<meta property="og:title" content="${title}" />`,
        `<meta property="og:description" content="${description}" />`,
        `<meta property="og:url" content="${url}" />`,
        `<meta property="og:image" content="${siteUrl}/og.jpg" />`,
        `<meta name="twitter:card" content="summary_large_image" />`,
    ];

    if (page === "index.html") {
        const graph = {
            "@context": "https://schema.org",
            "@graph": [
                { "@type": "WebSite", name: "YUP", url },
                {
                    "@type": "SoftwareSourceCode",
                    name: "YUP",
                    description,
                    url,
                    codeRepository: "https://github.com/kunitoki/yup",
                    programmingLanguage: "C++",
                    license: "https://opensource.org/license/isc-license-txt",
                },
            ],
        };
        tags.push(`<script type="application/ld+json">${JSON.stringify(graph)}</script>`);
    }

    return tags.join("\n");
}

// Expands <!-- @name --> into partials/<name>.html and <!-- @code path --> into a
// highlighted <pre> of partials/<path>, then marks the current page's nav link.
function partials() {
    const repo = normalizePath(resolve(import.meta.dirname, "..")).replace(/^\//, "");

    return {
        name: "yup-partials",
        configureServer(server) {
            // In dev, ../../docs/... and ../../logo.svg resolve to /docs/... and /logo.svg,
            // which live in the repository root outside the Vite root.
            server.middlewares.use((req, _res, next) => {
                if (req.url?.startsWith("/docs/") || req.url === "/logo.svg")
                    req.url = `/@fs/${repo}${req.url}`;
                next();
            });

            server.watcher.on("change", (file) => {
                if (file.startsWith(partialsDir))
                    server.ws.send({ type: "full-reload" });
            });
        },
        transformIndexHtml: {
            order: "pre",
            async handler(html, ctx) {
                const page = basename(ctx.path) || "index.html";
                const shiki = await highlighter;

                const expanded = html
                    .replace(/<!-- @(\w+) -->/g, (_, name) => readPartial(`${name}.html`))
                    .replace(/<!-- @code (\S+) -->/g, (_, path) =>
                        shiki.codeToHtml(readPartial(path).trimEnd(), {
                            lang: codeLanguages[extname(path)],
                            theme: "yup",
                            transformers: [{ pre(node) { this.addClassToHast(node, "code"); } }],
                        }))
                    .replaceAll(`data-nav href="./${page}"`, `data-nav aria-current="page" href="./${page}"`);

                return expanded.replace("</head>", `${seoTags(page, expanded)}\n</head>`);
            },
        },
        generateBundle() {
            const urls = pages.map((p) => `    <url><loc>${pageUrl(`${p}.html`)}</loc></url>`).join("\n");

            this.emitFile({
                type: "asset",
                fileName: "sitemap.xml",
                source: `<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n${urls}\n</urlset>\n`,
            });
            this.emitFile({ type: "asset", fileName: "robots.txt", source: `User-agent: *\nAllow: /\n\nSitemap: ${siteUrl}/sitemap.xml\n` });
            this.emitFile({ type: "asset", fileName: "og.jpg", source: readFileSync(socialImage) });
        },
    };
}

export default defineConfig({
    root,
    base: "./",
    plugins: [tailwindcss(), partials()],
    server: {
        // Screenshots are imported straight from the repository docs/.
        fs: { allow: [resolve(import.meta.dirname, "..")] },
    },
    build: {
        outDir: resolve(import.meta.dirname, "dist"),
        emptyOutDir: true,
        rollupOptions: {
            input: Object.fromEntries(pages.map((p) => [p, resolve(root, `${p}.html`)])),
        },
    },
});
