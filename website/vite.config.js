import { readdirSync, readFileSync } from "node:fs";
import { basename, extname, resolve } from "node:path";
import { defineConfig, normalizePath } from "vite";
import tailwindcss from "@tailwindcss/vite";
import { createHighlighter } from "shiki";

const repoRoot = resolve(import.meta.dirname, "..");
const root = resolve(import.meta.dirname, "src");
const partialsDir = resolve(root, "partials");
const pages = ["index", "modules", "showcase", "compare", "get-started"];

// Production origin, used for canonical links, social cards, the sitemap and robots.txt.
const siteUrl = "https://yup.audio";
const socialImage = resolve(repoRoot, "docs/_static/images/yup_prism_synth.jpg");

// GoatCounter site code (https://<code>.goatcounter.com). Analytics stay off while it is empty.
const goatCounterCode = "https://yup.goatcounter.com";

const docsDir = resolve(repoRoot, "docs");
const docsUrl = "https://yup.readthedocs.io/en/latest";

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
    if (page === "404.html")
        return "";

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
                    keywords: [
                        "JUCE alternative", "Visage alternative", "Qt alternative", "iPlug2 alternative", "audio plugin framework",
                        "C++ audio", "real-time audio", "DSP", "music software", "audio development", "plugin hosting", "real-time DSP",
                        "CLAP", "VST3", "Audio Unit", "GPU", "RHI", "graphics programming", "real-time rendering", "shaders", "Vulkan",
                        "Metal", "DirectX", "OpenGL", "WebGPU", "C++ framework", "modern C++", "ISC license", "open source framework"
                    ]
                },
            ],
        };
        tags.push(`<script type="application/ld+json">${JSON.stringify(graph)}</script>`);
    }

    return tags.join("\n");
}

function analyticsTag() {
    if (!goatCounterCode)
        return "";

    return `<script data-goatcounter="https://${goatCounterCode}.goatcounter.com/count" async src="https://gc.zgo.at/count.js"></script>`;
}

const moduleCount = () => readdirSync(resolve(repoRoot, "modules"), { withFileTypes: true })
    .filter((entry) => entry.isDirectory() && entry.name.startsWith("yup_")).length;

// Counts the list items or enabled chips inside the page's data-count="<name>" element,
// so a stat can never disagree with the list it summarizes.
function countItems(html, name) {
    const block = html.match(new RegExp(`data-count="${name}"[^>]*>([\\s\\S]*?)</(?:div|ul)>`))[1];
    return block.match(/<li|chip-on/g).length;
}

// Same rules as docutils' make_id, so links land on the section ids Sphinx emits.
const sectionId = (text) => text.toLowerCase().normalize("NFKD").replace(/[^\x00-\x7f]/g, "")
    .replace(/[^a-z0-9]+/g, "-").replace(/^[-0-9]+|-+$/g, "");

const plainText = (markdown) => markdown.replace(/!?\[([^\]]*)\]\([^)]*\)/g, "$1").replace(/[`*]|<[^>]+>/g, "").trim();

// One entry per h1-h3 section of the Sphinx docs: page title, heading, link, inline code
// identifiers and the start of the section text. Loaded lazily by the search dialog.
function docsSearchIndex() {
    const entries = [];

    for (const file of readdirSync(docsDir, { recursive: true })) {
        const path = normalizePath(file);
        if (!path.endsWith(".md") || /^(_|superpowers\/|demos\/)/.test(path))
            continue;

        const pageUrl = `${docsUrl}/${path.replace(/\.md$/, ".html")}`;
        const ids = new Set();
        let title = "";
        let fence = "";
        let entry;

        for (const line of readFileSync(resolve(docsDir, file), "utf8").split("\n")) {
            const fenceMark = line.match(/^\s*(`{3,}|~{3,})/)?.[1];
            if (fenceMark && (!fence || fenceMark.startsWith(fence))) {
                fence = fence ? "" : fenceMark;
                continue;
            }

            const heading = fence ? null : line.match(/^(#{1,3})\s+(.+?)(?:\s+#+)?\s*$/);
            if (heading) {
                const text = plainText(heading[2]);
                const id = sectionId(text);
                const unique = heading[1].length > 1 && id && !ids.has(id);
                ids.add(id);
                title ||= text;
                entry = { p: title, h: text, u: unique ? `${pageUrl}#${id}` : pageUrl, k: new Set(), t: "" };
                entries.push(entry);
                continue;
            }

            if (!entry || fence || /^\s*(:::|\(.+\)=|\||<!--)/.test(line))
                continue;

            for (const [, code] of line.matchAll(/`([\w:.]+)`/g))
                entry.k.add(code);

            if (entry.t.length < 200)
                entry.t = `${entry.t} ${plainText(line.replace(/^\s*([-*+]|\d+\.)\s+/, ""))}`.trim();
        }
    }

    return entries.map((e) => ({ ...e, k: [...e.k].join(" "), t: e.t.length > 200 ? `${e.t.slice(0, 200)}...` : e.t }));
}

// Expands <!-- @name --> into partials/<name>.html, <!-- @code path --> into a highlighted
// <pre> of partials/<path> and <!-- @count name --> into a number derived from the repository
// or the page, then marks the current page's nav link. Also serves the docs search index as
// the virtual:docs-index module.
function partials() {
    const repo = normalizePath(repoRoot).replace(/^\//, "");

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
        resolveId(id) {
            if (id === "virtual:docs-index")
                return "\0virtual:docs-index";
        },
        load(id) {
            if (id === "\0virtual:docs-index")
                return `export default ${JSON.stringify(docsSearchIndex())};`;
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

                const counted = expanded.replace(/<!-- @count (\w+) -->/g, (_, name) =>
                    name === "modules" ? moduleCount() : countItems(expanded, name));

                return counted.replace("</head>", `${seoTags(page, counted)}\n${analyticsTag()}\n</head>`);
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
        fs: { allow: [repoRoot] },
    },
    build: {
        outDir: resolve(import.meta.dirname, "dist"),
        emptyOutDir: true,
        rollupOptions: {
            // 404.html is served by GitHub Pages for unknown paths and stays out of the sitemap.
            input: Object.fromEntries([...pages, "404"].map((p) => [p, resolve(root, `${p}.html`)])),
        },
    },
});
