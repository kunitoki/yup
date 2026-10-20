import { readdirSync, readFileSync } from "node:fs";
import { basename, extname, resolve } from "node:path";
import { defineConfig, normalizePath } from "vite";
import { createHighlighter } from "shiki";

const repoRoot = resolve(import.meta.dirname, "..");
const root = resolve(import.meta.dirname, "src");
const partialsDir = resolve(root, "partials");
const pages = ["index", "modules", "showcase", "compare", "releases", "get-started"];

// Production origin, used for canonical links, social cards, the sitemap and robots.txt.
const siteUrl = "https://yup.audio";
const socialImage = resolve(repoRoot, "docs/_static/images/yup_prism_synth.jpg");

// Service worker that cross-origin isolates the site on GitHub Pages, which can't send the headers
// the threaded YUP demos need. Served from the root so its scope covers every page.
const miniCoi = resolve(repoRoot, "cmake/platforms/emscripten/mini-coi.js");
const isolationHeaders = { "Cross-Origin-Opener-Policy": "same-origin", "Cross-Origin-Embedder-Policy": "require-corp" };

// GoatCounter site code (https://<code>.goatcounter.com). Analytics stay off while it is empty.
const goatCounterCode = "yup";

const docsDir = resolve(repoRoot, "docs");
const docsUrl = "https://yup.readthedocs.io/en/latest";

const repoUrl = "https://github.com/kunitoki/yup";
const releasesDir = resolve(partialsDir, "releases");

// Syntax colors in the site palette: blues and greys only.
const codeTheme = {
    name: "yup",
    type: "dark",
    colors: { "editor.background": "#00000000", "editor.foreground": "#D8DDE0" },
    tokenColors: [
        { scope: ["comment", "punctuation.definition.comment"], settings: { foreground: "#59606A", fontStyle: "italic" } },
        { scope: ["keyword", "storage", "variable.language", "keyword.control.directive", "punctuation.definition.directive"], settings: { foreground: "#4D8DFF" } },
        { scope: ["string", "punctuation.definition.string"], settings: { foreground: "#9CC2FF" } },
        { scope: ["constant.numeric", "constant.language", "constant.other"], settings: { foreground: "#9CC2FF" } },
        { scope: ["entity.name.function", "support.function"], settings: { foreground: "#FFFFFF" } },
        { scope: ["entity.name.type", "entity.name.class", "entity.name.scope-resolution", "entity.other.inherited-class", "support.type", "variable.parameter"], settings: { foreground: "#C3D6F5" } },
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

// Same rules as docutils' make_id, so links land on the section ids Sphinx emits.
const sectionId = (text) => text.toLowerCase().normalize("NFKD").replace(/[^\x00-\x7f]/g, "")
    .replace(/[^a-z0-9]+/g, "-").replace(/^[-0-9]+|-+$/g, "");

// Links keep their text; code marks, emphasis, HTML tags and {{ substitutions }} go.
const plainText = (markdown) => markdown.replace(/!?\[([^\]]*)\]\([^)]*\)/g, "$1").replace(/\{\{\s*[\w-]+\s*(:[^}]*)?\}\}/g, "").replace(/[`*]|<[^>]+>/g, "").trim();

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

            if (!entry || fence || /^\s*(:::|\(.+\)=|\||<!--|\{\{\s*[\w-]+\s*(:[^}]*)?\}\}\s*$)/.test(line))
                continue;

            for (const [, code] of line.matchAll(/`([\w:.]+)`/g))
                entry.k.add(code);

            if (entry.t.length < 200)
                entry.t = `${entry.t} ${plainText(line.replace(/^\s*([-*+]|\d+\.)\s+/, ""))}`.trim();
        }
    }

    return entries.map((e) => ({ ...e, k: [...e.k].join(" "), t: e.t.length > 200 ? `${e.t.slice(0, 200)}...` : e.t }));
}

const escapeHtml = (text) => text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");

// The inline markdown of the release notes: code spans (taken verbatim), bold, links and #123
// references to issues and pull requests.
const inlineMarkdown = (text) => text.split(/(`[^`]+`)/).map((part) => part.startsWith("`") && part.endsWith("`") && part.length > 1
    ? `<code>${escapeHtml(part.slice(1, -1))}</code>`
    : escapeHtml(part)
        .replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>")
        .replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2" target="_blank" rel="noopener">$1</a>')
        .replace(/(^|[\s(])#(\d+)\b/g, `$1<a href="${repoUrl}/issues/$2" target="_blank" rel="noopener">#$2</a>`)).join("");

// The release notes of partials/releases/<version>.md, newest first, as one collapsible section each
// with the newest open. Each file starts with "## [<version>] - <date>" (a date of "Unreleased" keeps
// a draft off the page), then ### and #### groups, bullets (continued on indented lines) and plain or
// bold paragraphs.
const versionKey = (file) => basename(file, ".md").split(".").map((part) => part.padStart(6, "0")).join(".");

function releasesHtml() {
    const out = [];
    let item = null;
    let inList = false;
    let inVersion = false;

    const flushItem = () => {
        if (item !== null)
            out.push(`<li>${inlineMarkdown(item)}</li>`);
        item = null;
    };
    const closeList = () => {
        flushItem();
        if (inList)
            out.push("</ul>");
        inList = false;
    };
    const closeVersion = () => {
        closeList();
        if (inVersion)
            out.push("</div></details></section>");
        inVersion = false;
    };

    const notes = readdirSync(releasesDir)
        .filter((file) => file.endsWith(".md"))
        .sort((a, b) => versionKey(b).localeCompare(versionKey(a)))
        .map((file) => readFileSync(resolve(releasesDir, file), "utf8"))
        .join("\n");

    for (const line of notes.split("\n")) {
        const version = line.match(/^## \[(.+?)\](?:\s*-\s*(.+))?/);
        if (version) {
            const [, number, date = ""] = version;
            const open = out.length === 0 ? " open" : "";
            closeVersion();

            if (/unreleased/i.test(date))
                continue;

            out.push(`<section class="part release"><details${open}><summary class="label"><span>${escapeHtml(number)}</span>${escapeHtml(date.trim())}</summary><div class="release-body">`);
            out.push(`<p class="note"><a href="${repoUrl}/releases/tag/v${escapeHtml(number)}" target="_blank" rel="noopener">github release</a></p>`);

            inVersion = true;
            continue;
        }

        if (!inVersion)
            continue;

        const heading = line.match(/^(#{3,4})\s+(.+)/);
        const bullet = line.match(/^\s*[-*]\s+(.+)/);

        if (heading) {
            closeList();
            const tag = heading[1].length === 3 ? "h3" : "h4";
            out.push(`<${tag}>${inlineMarkdown(heading[2])}</${tag}>`);
        } else if (bullet) {
            flushItem();
            if (!inList)
                out.push("<ul>");
            inList = true;
            item = bullet[1];
        } else if (item !== null && /^\s+\S/.test(line)) {
            item = `${item} ${line.trim()}`;
        } else if (/^\s*$/.test(line) || /^---\s*$/.test(line)) {
            closeList();
        } else {
            closeList();
            out.push(`<p>${inlineMarkdown(line.trim())}</p>`);
        }
    }

    closeVersion();
    return out.join("\n");
}

// Expands <!-- @name --> into partials/<name>.html and <!-- @code path --> into a highlighted
// <pre> of partials/<path> and <!-- @releases --> into the notes of partials/releases/, then marks
// the current page's nav link. Also serves the docs search index as the virtual:docs-index module.
function partials() {
    const repo = normalizePath(repoRoot).replace(/^\//, "");

    return {
        name: "yup-partials",
        configureServer(server) {
            // In dev, ../../docs/... and ../../logos/... resolve to /docs/... and /logos/...,
            // which live in the repository root outside the Vite root.
            server.middlewares.use((req, _res, next) => {
                if (req.url?.startsWith("/docs/") || req.url?.startsWith("/logos/"))
                    req.url = `/@fs/${repo}${req.url}`;
                else if (req.url === "/mini-coi.js")
                    req.url = `/@fs/${normalizePath(miniCoi).replace(/^\//, "")}`;
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
                    .replace("<!-- @releases -->", () => releasesHtml())
                    .replace(/<!-- @(\w+) -->/g, (_, name) => readPartial(`${name}.html`))
                    .replace(/<!-- @code (\S+) -->/g, (_, path) =>
                        shiki.codeToHtml(readPartial(path).trimEnd(), {
                            lang: codeLanguages[extname(path)],
                            theme: "yup",
                            transformers: [{ pre(node) { this.addClassToHast(node, "code"); } }],
                        }))
                    .replaceAll(`data-nav href="./${page}"`, `data-nav aria-current="page" href="./${page}"`);

                return expanded.replace("</head>", `${seoTags(page, expanded)}\n${analyticsTag()}\n</head>`);
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
            this.emitFile({ type: "asset", fileName: "mini-coi.js", source: readFileSync(miniCoi) });
        },
    };
}

export default defineConfig({
    root,
    base: "./",
    plugins: [partials()],
    server: {
        // Screenshots are imported straight from the repository docs/.
        fs: { allow: [repoRoot] },
        headers: isolationHeaders,
    },
    preview: {
        headers: isolationHeaders,
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
