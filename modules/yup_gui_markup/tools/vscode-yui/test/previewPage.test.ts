// Tests of the page of the preview, run by `npm test` with Node: the script written in it is one a page can
// compile, and it is given the values the page is set up with.

import assert from "node:assert/strict";
import { test } from "node:test";
import * as vm from "node:vm";
import { previewPage } from "../src/previewPage";

const nonce = "0123456789abcdef0123456789abcdef";

// Where the page finds the tool, as the addresses a panel gives its files
const tool = {
    script: "https://file+.vscode-resource.vscode-cdn.net/extension/tool/yup_yui_tool.js",
    folder: "https://file+.vscode-resource.vscode-cdn.net/extension/tool/",
    source: "https://*.vscode-cdn.net"
};

// The script of a page: what is written in its one script element, which ends where a browser ends it
function scriptOf (page: string): string {
    const scripts = Array.from (page.matchAll (/<script nonce="([^"]*)">\n([\s\S]*?)<\/script>/g));

    assert.equal (scripts.length, 1);
    assert.equal (scripts[0][1], nonce);
    return scripts[0][2];
}

// What the script is given: the object written before it, on its first line
function settingsOf (page: string): unknown {
    return JSON.parse (/^const settings = (.*);\n/.exec (scriptOf (page))?.[1] ?? "null");
}

test ("the script written in the page is one a page can compile", () => {
    for (const page of [previewPage (nonce, tool), previewPage (nonce, undefined)]) {
        const script = scriptOf (page);

        // Compiled and not run: it only runs in a page, where it finds the editor and the tool
        new vm.Script (script, { filename: "preview.page.js" });

        assert.match (script, /acquireVsCodeApi \(\)/);
        assert.match (script, /vscode[.]postMessage \(\{ type: "ready" \}\);\n\}\) \(\);\s*$/);
    }
});

test ("the script is given the values the page is set up with, before it runs", () => {
    // The largest size, how far 'fit' enlarges and the padding of the content, which the script takes on both sides
    const sizes = { maximumSize: 8192, maximumFit: 8, contentPadding: 12 };

    assert.deepEqual (settingsOf (previewPage (nonce, tool)), { module: { script: tool.script, folder: tool.folder }, nonce, ...sizes });

    // An extension packaged without the tool has none to tell
    assert.deepEqual (settingsOf (previewPage (nonce, undefined)), { module: null, nonce, ...sizes });
});
