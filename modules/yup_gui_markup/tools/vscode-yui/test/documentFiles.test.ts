// Tests of the files a document is given to the tool with, run by `npm test` with Node: what a document names,
// what the tool tells it reads beside that, and what is given of them.

import assert from "node:assert/strict";
import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import { after, afterEach, before, test } from "node:test";
import { collectDocumentFiles, DocumentFiles, EditedDocument, isNamedBy, noteMissingFiles, runUntilFilesAreGiven, setUnsavedTexts } from "../src/documentFiles";

let root: string;

before (() => {
    root = fs.mkdtempSync (path.join (os.tmpdir (), "yui-document-files-"));
});

after (() => fs.rmSync (root, { recursive: true, force: true }));
afterEach (() => setUnsavedTexts (() => undefined));

// Writes the files of a project in a folder of its own, and returns where they are
function write (project: string, files: Record<string, string | Uint8Array>): string {
    const folder = path.join (root, project);

    for (const [name, content] of Object.entries (files)) {
        fs.mkdirSync (path.dirname (path.join (folder, name)), { recursive: true });
        fs.writeFileSync (path.join (folder, name), content);
    }

    return folder;
}

// A document as its editor holds it, with the text of its file
function edited (file: string): EditedDocument {
    return { uri: { fsPath: file }, getText: () => fs.readFileSync (file, "utf8") };
}

// The files given, by where they are in the folder the tool keeps them in
function given (collected: DocumentFiles): Map<string, Uint8Array> {
    const files = collected.files.filter (file => file.removed !== true);

    return new Map (files.map (file => [file.path.replace (/^[0-9]+[/]/, ""), file.data]));
}

function names (collected: DocumentFiles): string[] {
    return Array.from (given (collected).keys ()).sort ();
}

/**
    Stands in for the tool: it loads a document and tells the files that document reads that it was
    not given, each by the folder of the document followed by the path the document names it by.
*/
class StandInTool {
    readonly runs: string[][] = [];

    constructor (private readonly reads: string[]) {
    }

    run (collected: DocumentFiles): string[] {
        const have = collected.files.filter (file => file.removed !== true).map (file => file.path);
        const folder = path.posix.dirname (collected.main);

        this.runs.push (names (collected));

        return this.reads.map (name => `${folder}/${name}`).filter (file => ! have.includes (path.posix.normalize (file)));
    }

    /** Checks a document as the extension does: again while the tool tells files there are to give. */
    async check (document: EditedDocument): Promise<DocumentFiles | undefined> {
        const ran = await runUntilFilesAreGiven (async () => {
            const collected = await collectDocumentFiles (document, undefined);

            return { document: document.uri.fsPath, missing: this.run (collected), collected };
        });

        return ran?.collected;
    }
}

test ("a document is given with the files it names, byte for byte, where they are next to it", async () => {
    const bytes = Uint8Array.from ({ length: 256 }, (_, index) => index);

    const folder = write ("named", {
        "ui/main.yui": 'import "parts/title.yui";\n\n// Image { source: "left/out.png"; }\nScene3D {\n    Model3D { source: "models/knob.glb"; }\n    Label { text: "not a file.txt"; }\n}\n',
        "ui/parts/title.yui": 'component Title : Label { }\nImage { source: "logo.png"; }\n',
        "ui/models/knob.glb": bytes,
        "ui/logo.png": "next to the document shown",
        "ui/left/out.png": "named in a comment"
    });

    const collected = await collectDocumentFiles (edited (path.join (folder, "ui", "main.yui")), undefined);

    assert.deepEqual (names (collected), ["logo.png", "main.yui", "models/knob.glb", "parts/title.yui"]);
    assert.deepEqual (Array.from (given (collected).get ("models/knob.glb") ?? []), Array.from (bytes));
    assert.match (collected.main, /^[0-9]+[/]main[.]yui$/);
    assert.deepEqual (collected.problems, []);
});

test ("the buffers and the images a model written as text keeps in other files are given with it", async () => {
    const model = {
        buffers: [{ uri: "knob.bin" }, { uri: "data:application/octet-stream;base64,AAAA" }],
        images: [{ uri: "textures/brushed%20steel.png" }, { uri: "https://example.com/not/a/file.png" }, { bufferView: 0 }]
    };

    const folder = write ("model", {
        "main.yui": 'Scene3D {\n    Model3D { source: "models/knob.gltf"; }\n}\n',
        "models/knob.gltf": JSON.stringify (model),
        "models/knob.bin": "buffer",
        "models/textures/brushed steel.png": "image"
    });

    const collected = await collectDocumentFiles (edited (path.join (folder, "main.yui")), undefined);

    assert.deepEqual (names (collected), ["main.yui", "models/knob.bin", "models/knob.gltf", "models/textures/brushed steel.png"]);
});

test ("the files the tool tells a document reads are given once, and those that are not there are not asked for again", async () => {
    const folder = write ("told", {
        "ui/main.yui": 'shader Crt {\n    fragment: "../fx/crt.fs";\n}\n\nComponent { }\n',
        "fx/crt.fs": "void main() { }\n"
    });

    const document = edited (path.join (folder, "ui", "main.yui"));
    const tool = new StandInTool (["../fx/crt.fs.ysl", "../fx/crt.fs"]);

    // The source of the shader is of no kind a document is known to read, so it is the tool that tells it. It runs
    // again once given it, and not a third time for the bundle no build made of it
    assert.deepEqual (names (await tool.check (document) as DocumentFiles), ["fx/crt.fs", "ui/main.yui"]);
    assert.deepEqual (tool.runs, [["main.yui"], ["fx/crt.fs", "ui/main.yui"]]);

    // From then on the document is given with it, and what is still missing does not make it run again
    assert.deepEqual (names (await tool.check (document) as DocumentFiles), ["fx/crt.fs", "ui/main.yui"]);
    assert.equal (tool.runs.length, 3);

    // What is missing is given once it is there: the file is one the document names, so who watches the files knows
    const bundle = path.join (folder, "fx", "crt.fs.ysl");

    assert.ok (isNamedBy (document.uri.fsPath, bundle));
    assert.ok (isNamedBy (document.uri.fsPath, path.join (folder, "fx", "crt.fs")));
    assert.ok (! isNamedBy (document.uri.fsPath, path.join (folder, "fx", "other.fs")));
    assert.ok (! isNamedBy (document.uri.fsPath, document.uri.fsPath));

    fs.writeFileSync (bundle, "bundle");

    assert.deepEqual (names (await tool.check (document) as DocumentFiles), ["fx/crt.fs", "fx/crt.fs.ysl", "ui/main.yui"]);
    assert.equal (tool.runs.length, 4);
});

test ("a file told by the tool is only run for again when it is new and there", async () => {
    const folder = write ("noted", { "main.yui": "Component { }\n", "there.frag": "void main() { }\n" });
    const main = path.join (folder, "main.yui");
    const collected = await collectDocumentFiles (edited (main), undefined);
    const told = (name: string) => `${path.posix.dirname (collected.main)}/${name}`;

    assert.equal (noteMissingFiles (main, [told ("absent.frag")]), false);
    assert.equal (noteMissingFiles (main, [told ("there.frag")]), true);
    assert.equal (noteMissingFiles (main, [told ("there.frag"), told ("absent.frag")]), false);

    // What is not of the folders the tool was given files from stands for no file
    assert.equal (noteMissingFiles (main, ["/yui/there.frag", "unknown/there.frag", "999/there.frag"]), false);
});

test ("the text a file has in an editor counts before the one saved", async () => {
    const folder = write ("unsaved", {
        "main.yui": 'shader Crt {\n    fragment: "fx/crt.frag";\n}\n\nComponent { }\n',
        "fx/crt.frag": "saved"
    });

    const document = edited (path.join (folder, "main.yui"));
    const fragment = path.join (folder, "fx", "crt.frag");
    const tool = new StandInTool (["fx/crt.frag"]);
    const text = (collected: DocumentFiles | undefined) => Buffer.from ((collected && given (collected).get ("fx/crt.frag")) ?? []).toString ("utf8");

    assert.equal (text (await tool.check (document)), "saved");

    setUnsavedTexts (file => file === fragment ? "typed" : undefined);
    assert.equal (text (await tool.check (document)), "typed");

    setUnsavedTexts (() => undefined);
    assert.equal (text (await tool.check (document)), "saved");

    // A file that only exists in its editor is given too
    const unsaved = path.join (folder, "fx", "new.frag");
    const other = new StandInTool (["fx/new.frag"]);

    setUnsavedTexts (file => file === unsaved ? "not on disk yet" : undefined);
    assert.deepEqual (names (await other.check (document) as DocumentFiles), ["fx/crt.frag", "fx/new.frag", "main.yui"]);
});

test ("a file larger than what the tool is given is left out, and told", async () => {
    const folder = write ("large", {
        "main.yui": 'Scene3D {\n    Model3D { source: "models/large.glb"; }\n    Model3D { source: "models/small.glb"; }\n}\n',
        "models/small.glb": "small"
    });

    // A file of 16 MB and a byte, which is never read
    fs.closeSync (fs.openSync (path.join (folder, "models", "large.glb"), "w"));
    fs.truncateSync (path.join (folder, "models", "large.glb"), 16 * 1024 * 1024 + 1);

    const collected = await collectDocumentFiles (edited (path.join (folder, "main.yui")), undefined);

    assert.deepEqual (names (collected), ["main.yui", "models/small.glb"]);
    assert.equal (collected.problems.length, 1);
    assert.match (collected.problems[0], /^'models[/\\]large[.]glb' [(]16[.]0 MB[)] is too large/);
});

test ("a file a document names is not read when it may not be", async () => {
    const folder = write ("allowed", {
        "main.yui": 'import "part.yui";\n\nImage { source: "../outside/logo.png"; }\n',
        "part.yui": "component Part : Label { }\n"
    });

    write ("outside", { "logo.png": "not of the folder" });

    const collected = await collectDocumentFiles (edited (path.join (folder, "main.yui")), undefined, file => file.startsWith (folder + path.sep));

    assert.deepEqual (names (collected), ["main.yui", "part.yui"]);
});
