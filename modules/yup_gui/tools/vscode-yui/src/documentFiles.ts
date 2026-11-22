import * as fs from "fs";
import * as path from "path";
import * as vscode from "vscode";
import { toolFilesFolder, YuiToolFile } from "./tool";

/** A document with the files it needs, at the paths the tool knows them by. */
export interface DocumentFiles {
    /** The path of the document. */
    main: string;

    /** The path of the models, empty without any. */
    models: string;

    files: YuiToolFile[];
}

// What a document can name: the documents it imports, and the images, fonts and data it reads
const referenced = new Set ([".yui", ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".webp", ".svg", ".ttf", ".otf", ".riv", ".json"]);
const maximumFileBytes = 16 * 1024 * 1024;
const maximumBytes = 64 * 1024 * 1024;

// The tool has no disk: each folder of documents is a folder of its own there, named by a number
const folders: string[] = [];

function folderName (folder: string): string {
    const index = folders.indexOf (folder);
    return String (index >= 0 ? index : folders.push (folder) - 1);
}

/** Gives a file the path the tool knows it by. */
export function toolPathOf (file: string): string {
    return `${folderName (path.dirname (file))}/${path.basename (file)}`;
}

/** Returns the file a path printed by the tool stands for. */
export function realPathOf (toolPath: string): string | undefined {
    const parts = (toolPath.startsWith (toolFilesFolder) ? toolPath.substring (toolFilesFolder.length) : toolPath).split ("/");
    const folder = /^[0-9]+$/.test (parts[0]) ? folders[Number (parts[0])] : undefined;

    return folder === undefined ? undefined : path.join (folder, ...parts.slice (1));
}

/** Blanks out comments, keeping the line structure of the document intact. */
export function withoutComments (text: string): string {
    return text.replace (/\/\*[\s\S]*?\*\//g, comment => comment.replace (/[^\n]/g, " ")).replace (/\/\/[^\n]*/g, "");
}

// A file open in an editor is read from it, so that what is typed counts before it is saved
async function read (file: string): Promise<Uint8Array | undefined> {
    const open = vscode.workspace.textDocuments.find (document => document.uri.scheme === "file" && document.uri.fsPath === file);

    if (open !== undefined)
        return Buffer.from (open.getText (), "utf8");

    try {
        const status = await fs.promises.stat (file);
        return status.isFile () && status.size <= maximumFileBytes ? await fs.promises.readFile (file) : undefined;
    }
    catch {
        return undefined;
    }
}

function isInside (folder: string, file: string): boolean {
    const relative = path.relative (folder, file);
    return ! relative.startsWith ("..") && ! path.isAbsolute (relative);
}

function commonFolder (files: string[]): string {
    let folder = path.dirname (files[0]);

    for (const file of files) {
        while (! isInside (folder, file) && path.dirname (folder) !== folder)
            folder = path.dirname (folder);
    }

    return folder;
}

/**
    Collects a document with the files it names, followed through the documents it imports.

    A name is looked for next to the document naming it, then next to the main one, which is where
    the paths of a document are resolved from.

    @param isAllowed Tells whether a file a document names may be read, for who must not read them all.
*/
export async function collectDocumentFiles (document: vscode.TextDocument, modelsPath: string | undefined, isAllowed: (file: string) => boolean = () => true): Promise<DocumentFiles> {
    const main = document.uri.fsPath;
    const contents = new Map<string, Uint8Array> ([[main, Buffer.from (document.getText (), "utf8")]]);
    const pending = [main];
    const missing = new Set<string> ();
    let total = 0;

    while (pending.length > 0) {
        const file = pending.pop () as string;
        const data = contents.get (file);

        if (data === undefined || path.extname (file).toLowerCase () !== ".yui")
            continue;

        for (const match of withoutComments (Buffer.from (data).toString ("utf8")).matchAll (/"([^"\n\\]+)"/g)) {
            const name = match[1];

            if (! referenced.has (path.extname (name).toLowerCase ()) || path.isAbsolute (name))
                continue;

            for (const folder of [path.dirname (file), path.dirname (main)]) {
                const candidate = path.resolve (folder, name);

                if (contents.has (candidate))
                    break;

                const found = isAllowed (candidate) ? await read (candidate) : undefined;

                if (found === undefined || total + found.length > maximumBytes) {
                    missing.add (candidate);
                    continue;
                }

                total += found.length;
                contents.set (candidate, found);
                pending.push (candidate);
                break;
            }
        }
    }

    // The files keep the places they have relative to each other, under the folder holding them all
    const folder = commonFolder (Array.from (contents.keys ()));
    const toolPath = (file: string) => `${folderName (folder)}/${path.relative (folder, file).split (path.sep).join ("/")}`;
    const files: YuiToolFile[] = [];

    for (const [file, data] of contents) {
        if (isInside (folder, file))
            files.push ({ path: toolPath (file), data });
    }

    // The tool keeps the files it was given: one a document names and that is gone is forgotten there too
    for (const file of missing) {
        if (isInside (folder, file) && ! contents.has (file))
            files.push ({ path: toolPath (file), data: new Uint8Array (), removed: true });
    }

    const models = modelsPath === undefined ? undefined : await read (modelsPath);

    if (modelsPath !== undefined && models !== undefined)
        files.push ({ path: toolPathOf (modelsPath), data: models });

    return { main: toolPath (main), models: modelsPath !== undefined && models !== undefined ? toolPathOf (modelsPath) : "", files };
}
