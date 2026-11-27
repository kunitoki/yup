// Bundles the extension, with the packages it uses, into the scripts VS Code runs:
//
//     node esbuild.js [--watch]
//
// With --test it bundles the tests of what needs no editor instead, every test/*.test.ts, which `npm test` runs with Node.

const esbuild = require ("esbuild");
const fs = require ("fs");

const tests = process.argv.includes ("--test");
const outdir = tests ? "out-test" : "out";

const options = {
    entryPoints: tests ? fs.readdirSync ("test").filter (name => name.endsWith (".test.ts")).map (name => `test/${name}`)
                       : ["src/extension.ts", "src/toolWorker.ts"],
    outdir,
    bundle: true,
    platform: "node",
    format: "cjs",
    target: "node20",
    external: ["vscode"],

    // The script of a page is written in the page by who makes it, which takes it as its text
    loader: { ".page.js": "text" },
    logLevel: "info"
};

async function build () {
    // What an earlier build left is not packaged with this one
    fs.rmSync (outdir, { recursive: true, force: true });

    if (process.argv.includes ("--watch"))
        await (await esbuild.context (options)).watch ();
    else
        await esbuild.build (options);
}

build ().catch (() => process.exit (1));
