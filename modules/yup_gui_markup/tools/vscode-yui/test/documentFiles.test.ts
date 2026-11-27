// Tests of the files a document is given to the tool with, run by `npm test` with Node: what a document names,
// what the tool tells it reads beside that, and what is given of them.

import assert from "node:assert/strict";
import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import { after, afterEach, before, test } from "node:test";
import { collectDocumentFiles, DocumentFiles, EditedDocument, isNamedBy, leftOutProblem, noteMissingFiles, runUntilFilesAreGiven, setReadableAnywhere, setReadableFolders, setUnsavedTexts } from "../src/documentFiles";

let root: string;

// The folder of the tests stands for the workspace, which is where a document reads the files it names
before (() => {
    root = fs.mkdtempSync (path.join (os.tmpdir (), "yui-document-files-"));
    setReadableFolders (() => [root]);
});

after (() => fs.rmSync (root, { recursive: true, force: true }));

afterEach (() => {
    setUnsavedTexts (() => undefined);
    setReadableFolders (() => [root]);
    setReadableAnywhere (() => false);
});

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

            return { document: document.uri.fsPath, sought: collected.sought, missing: this.run (collected), collected };
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

test ("what a string holds is no comment, and what a comment holds is no string", async () => {
    const folder = write ("commented", {
        "main.yui": [
            'Column {',
            '    Label { text: "see // docs"; } Image { source: "slashes.png"; }',
            '    Label { text: "a /* b"; }',
            '    Image { source: "star.png"; }',
            '    Label { text: "c */ d"; }',
            `    /* it's "left/out.png" */ Image { source: "kept.png"; }`,
            '}',
            ''
        ].join ("\n"),
        "slashes.png": "after a string holding two slashes",
        "star.png": "between the strings holding what opens and what closes a comment",
        "kept.png": "after a comment holding quotes",
        "left/out.png": "named in a comment"
    });

    const collected = await collectDocumentFiles (edited (path.join (folder, "main.yui")), undefined);

    assert.deepEqual (names (collected), ["kept.png", "main.yui", "slashes.png", "star.png"]);
    assert.deepEqual (collected.problems, []);
});

test ("a file imported by a path between single quotes is given, with the files it names", async () => {
    const folder = write ("quoted", {
        "main.yui": "import 'parts/title.yui';\n\nColumn {\n    Title { }\n}\n",
        "parts/title.yui": 'component Title : Row {\n    Image { source: "logo.png"; }\n}\n',
        "logo.png": "named by the file imported"
    });

    const collected = await collectDocumentFiles (edited (path.join (folder, "main.yui")), undefined);

    assert.deepEqual (names (collected), ["logo.png", "main.yui", "parts/title.yui"]);
});

test ("a string ends at the quote it starts with, unless a backslash is before it", async () => {
    const folder = write ("ended", {
        "main.yui": [
            `style { .hint { text: 'say "hi'; } .logo { source: "quote.png"; } }`,
            '',
            'Column {',
            `    Label { text: "it's"; } Image { source: "apostrophe.png"; }`,
            '    Label { text: "say \\"hi"; } Image { source: "escaped.png"; }',
            '    Label { text: ""; } Image { source: "empty.png"; }',
            '}',
            ''
        ].join ("\n"),
        "quote.png": "after a string holding a double quote",
        "apostrophe.png": "after a string holding a single quote",
        "escaped.png": "after a string holding the quote it starts with",
        "empty.png": "after a string holding nothing"
    });

    const collected = await collectDocumentFiles (edited (path.join (folder, "main.yui")), undefined);

    assert.deepEqual (names (collected), ["apostrophe.png", "empty.png", "escaped.png", "main.yui", "quote.png"]);
});

test ("the models next to a document that are gone are forgotten by the tool, which reads them by itself", async () => {
    const folder = write ("neighbor", { "main.yui": "Component { }\n", "main.models.json": "{}" });
    const models = path.join (folder, "main.models.json");

    const withModels = await collectDocumentFiles (edited (path.join (folder, "main.yui")), models);

    assert.equal (withModels.files.some (file => file.removed === true), false);

    fs.rmSync (models);

    const without = await collectDocumentFiles (edited (path.join (folder, "main.yui")), undefined);

    assert.deepEqual (without.files.filter (file => file.removed === true).map (file => file.path), [withModels.models]);
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

test ("a file told by the tool is only run for again when it is there and the document was collected without it", async () => {
    const folder = write ("noted", { "main.yui": "Component { }\n", "there.frag": "void main() { }\n" });
    const main = path.join (folder, "main.yui");
    const collected = await collectDocumentFiles (edited (main), undefined);
    const told = (name: string) => `${path.posix.dirname (collected.main)}/${name}`;

    assert.equal (noteMissingFiles (main, [told ("absent.frag")], collected.sought), false);
    assert.equal (noteMissingFiles (main, [told ("there.frag")], collected.sought), true);

    // What checks a document and what shows it collect it apart: the second to be told ran without the file too
    assert.equal (noteMissingFiles (main, [told ("there.frag"), told ("absent.frag")], collected.sought), true);

    // Collected again, the document has the file and looked for the other one: neither makes it run again
    const again = await collectDocumentFiles (edited (main), undefined);

    assert.deepEqual (names (again), ["main.yui", "there.frag"]);
    assert.equal (noteMissingFiles (main, [told ("there.frag"), told ("absent.frag")], again.sought), false);

    // What is not of the folders the tool was given files from stands for no file
    assert.equal (noteMissingFiles (main, ["/yui/there.frag", "unknown/there.frag", "999/there.frag"], again.sought), false);
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

test ("a file that does not fit is told by the limit it does not fit in", () => {
    const large = leftOutProblem ("models/large.glb", 16 * 1024 * 1024 + 1);
    const crowded = leftOutProblem ("logo.png", 1024);

    assert.match (large, /^'models\/large[.]glb' [(]16[.]0 MB[)] is too large .* 16[.]0 MB at most/);
    assert.doesNotMatch (large, /64[.]0 MB/);

    // A small file left out for those given before it is not told to be large, nor by a size that says nothing
    assert.match (crowded, /^'logo[.]png' is left out .* 64[.]0 MB together/);
    assert.doesNotMatch (crowded, /too large|[(]/);
});

test ("a folder whose name starts with two dots is one of the document, not the one above it", async () => {
    const folder = write ("dots", {
        "main.yui": 'Image { source: "..cache/logo.png"; }\n',
        "..cache/logo.png": "kept by a build"
    });

    const collected = await collectDocumentFiles (edited (path.join (folder, "main.yui")), undefined);

    assert.deepEqual (names (collected), ["..cache/logo.png", "main.yui"]);
    assert.deepEqual (collected.problems, []);
});

test ("a document reads what it names in the workspace and in its own folder, and tells what it leaves there", async () => {
    const folder = write ("confined", {
        "ui/main.yui": 'import "part.yui";\n\nImage { source: "../shared/logo.png"; }\nImage { source: "../shared/absent.png"; }\n',
        "ui/part.yui": "component Part : Label { }\n",
        "shared/logo.png": "of the workspace, not of the folder of the document"
    });

    const document = edited (path.join (folder, "ui", "main.yui"));

    // With no workspace, a document only reads its own folder: what is there elsewhere is told, what is not is just missing
    setReadableFolders (() => []);

    const alone = await collectDocumentFiles (document, undefined);

    assert.deepEqual (names (alone), ["main.yui", "part.yui"]);
    assert.equal (alone.problems.length, 1);
    assert.match (alone.problems[0], /^'[.][.][/\\]shared[/\\]logo[.]png' is outside/);

    setReadableFolders (() => [folder]);

    const inWorkspace = await collectDocumentFiles (document, undefined);

    assert.deepEqual (names (inWorkspace), ["shared/logo.png", "ui/main.yui", "ui/part.yui"]);
    assert.deepEqual (inWorkspace.problems, []);
});

test ("a document reads what it names anywhere once that is asked for, unless who collects it says where", async () => {
    const folder = write ("unconfined", {
        "ui/main.yui": 'Label { font: font ("../../fonts/text.ttf", 12); }\nImage { source: "../shared/absent.png"; }\n'
    });

    write ("fonts", { "text.ttf": "above the folder of the document, and in no workspace" });

    const document = edited (path.join (folder, "ui", "main.yui"));

    setReadableFolders (() => []);

    const confined = await collectDocumentFiles (document, undefined);

    assert.deepEqual (names (confined), ["main.yui"]);
    assert.equal (confined.problems.length, 1);
    assert.match (confined.problems[0], /^'[.][.][/\\][.][.][/\\]fonts[/\\]text[.]ttf' is outside/);

    // Asked for while collecting, as the setting is: the file is read where it is, with no problem to tell
    setReadableAnywhere (() => true);

    const anywhere = await collectDocumentFiles (document, undefined);

    assert.deepEqual (names (anywhere), ["fonts/text.ttf", "unconfined/ui/main.yui"]);
    assert.deepEqual (anywhere.problems, []);

    // Who says where a document reads, as the tools for agents do, is still the one to say
    const held = await collectDocumentFiles (document, undefined, () => false);

    assert.deepEqual (names (held), ["main.yui"]);
    assert.equal (held.problems.length, 1);
});

test ("a link leading out of where a document reads is not followed", async () => {
    const folder = write ("linked", { "main.yui": 'Image { source: "assets/logo.png"; }\n' });
    const elsewhere = write ("elsewhere", { "logo.png": "not of the folder" });

    fs.symlinkSync (elsewhere, path.join (folder, "assets"));
    setReadableFolders (() => [folder]);

    const collected = await collectDocumentFiles (edited (path.join (folder, "main.yui")), undefined);

    assert.deepEqual (names (collected), ["main.yui"]);
    assert.equal (collected.problems.length, 1);
    assert.match (collected.problems[0], /^'assets[/\\]logo[.]png' is outside/);
});
