import { readFileSync } from "node:fs";
import { basename, extname, resolve } from "node:path";
import { defineConfig, normalizePath } from "vite";
import tailwindcss from "@tailwindcss/vite";
import { createHighlighter } from "shiki";

const root = resolve(import.meta.dirname, "src");
const partialsDir = resolve(root, "partials");
const pages = ["index", "modules", "showcase", "get-started"];

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

// Expands <!-- @name --> into partials/<name>.html and <!-- @code path --> into a
// highlighted <pre> of partials/<path>, then marks the current page's nav link.
function partials() {
    const docs = normalizePath(resolve(import.meta.dirname, "..", "docs")).replace(/^\//, "");

    return {
        name: "yup-partials",
        configureServer(server) {
            // In dev, ../../docs/... image URLs resolve to /docs/... which lives outside the root.
            server.middlewares.use((req, _res, next) => {
                if (req.url?.startsWith("/docs/"))
                    req.url = `/@fs/${docs}${req.url.slice(5)}`;
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

                return html
                    .replace(/<!-- @(\w+) -->/g, (_, name) => readPartial(`${name}.html`))
                    .replace(/<!-- @code (\S+) -->/g, (_, path) =>
                        shiki.codeToHtml(readPartial(path).trimEnd(), {
                            lang: codeLanguages[extname(path)],
                            theme: "yup",
                            transformers: [{ pre(node) { this.addClassToHast(node, "code"); } }],
                        }))
                    .replaceAll(`data-nav href="./${page}"`, `data-nav aria-current="page" href="./${page}"`);
            },
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
