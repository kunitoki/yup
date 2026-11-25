// Tests of what the tools for agents read and write of the workspace, run by `npm test` with Node.

import assert from "node:assert/strict";
import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import { after, before, test } from "node:test";
import { isInsideFolders, saveImage, withMember } from "../src/agentFiles";

let root: string;
let workspace: string;
let outside: string;

before (() => {
    root = fs.mkdtempSync (path.join (os.tmpdir (), "yui-agent-files-"));
    workspace = path.join (root, "workspace");
    outside = path.join (root, "outside");

    fs.mkdirSync (path.join (workspace, "ui"), { recursive: true });
    fs.mkdirSync (outside);
    fs.writeFileSync (path.join (workspace, "ui", "main.yui"), "Component { }\n");
    fs.symlinkSync (outside, path.join (workspace, "link"));
});

after (() => fs.rmSync (root, { recursive: true, force: true }));

test ("a file is of the workspace where it really is, whether it exists yet or not", () => {
    assert.ok (isInsideFolders (path.join (workspace, "ui", "main.yui"), [workspace]));
    assert.ok (isInsideFolders (path.join (workspace, "shots", "new", "first.png"), [workspace]));
    assert.ok (isInsideFolders (path.join (workspace, "ui", "..", "first.png"), [workspace]));

    assert.ok (! isInsideFolders (workspace, [workspace]));
    assert.ok (! isInsideFolders (path.join (workspace, "..", "outside", "first.png"), [workspace]));
    assert.ok (! isInsideFolders (path.join (outside, "first.png"), [workspace]));
    assert.ok (! isInsideFolders (path.join (workspace, "ui", "main.yui"), []));
});

test ("a link leading out of the workspace is not followed, even to a file that does not exist yet", () => {
    assert.ok (! isInsideFolders (path.join (workspace, "link", "first.png"), [workspace]));
    assert.ok (! isInsideFolders (path.join (workspace, "link", "deeper", "first.png"), [workspace]));
});

test ("a link to a file that is not there is not written through", () => {
    fs.symlinkSync (path.join (outside, "absent.png"), path.join (workspace, "dangling.png"));

    assert.ok (! isInsideFolders (path.join (workspace, "dangling.png"), [workspace]));
});

test ("an image is written with the folders it needs, over the file that was there", async () => {
    const file = path.join (workspace, "shots", "new", "first.png");

    await saveImage (file, Buffer.from ("first").toString ("base64"));
    assert.equal (fs.readFileSync (file, "utf8"), "first");

    await saveImage (file, Buffer.from ("second").toString ("base64"));
    assert.equal (fs.readFileSync (file, "utf8"), "second");
});

test ("what was done is added to the answer of a tool", () => {
    assert.deepEqual (JSON.parse (withMember ('{ "width": 8 }', "output", "/w/first.png")), { width: 8, output: "/w/first.png" });
    assert.equal (withMember ("not an object", "output", "/w/first.png"), "not an object");
});
