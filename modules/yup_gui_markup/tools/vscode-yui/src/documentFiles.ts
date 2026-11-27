import * as fs from "fs";
import * as path from "path";
import { isInsideFolders } from "./agentFiles";
import { toolFilesFolder, YuiToolFile } from "./tool";

/** A document as it is in its editor: its file and its text. */
export interface EditedDocument {
    uri: { fsPath: string };
    getText (): string;
}

/** A document with the files it needs, at the paths the tool knows them by. */
export interface DocumentFiles {
    /** The path of the document. */
    main: string;

    /** The path of the models, empty without any. */
    models: string;

    files: YuiToolFile[];

    /** Why files the document names were not given, for who looks at it to be told: they are too large, or not where a document may read. */
    problems: string[];

    /** The files the document was given or looked for, which are no reason to run on it again (see noteMissingFiles). */
    sought: ReadonlySet<string>;
}

// What a document can name: the documents it imports, and the images, fonts, models and data it reads
const referenced = new Set ([".yui", ".png", ".jpg", ".jpeg", ".gif", ".bmp", ".webp", ".svg", ".ttf", ".otf", ".riv", ".json", ".glb", ".gltf"]);
const maximumFileBytes = 16 * 1024 * 1024;
const maximumBytes = 64 * 1024 * 1024;

// How many times what runs on a document runs at most, for the files the tool then tells it was not given
const maximumRounds = 4;

// The tool has no disk: each folder of documents is a folder of its own there, named by a number
const folders: string[] = [];

// The files the tool told a document reads beside those its texts name, and those the document was given or looked
// for when it was last collected, both by the document
const told = new Map<string, Set<string>> ();
const named = new Map<string, Set<string>> ();

let unsavedText: (file: string) => string | undefined = () => undefined;
let readableFolders: () => string[] = () => [];
let readsAnywhere: () => boolean = () => false;

/** Sets who tells the text a file has in an editor while its changes are not saved, which is read in place of the file. */
export function setUnsavedTexts (find: (file: string) => string | undefined): void {
    unsavedText = find;
}

/** Sets who tells the folders a document may read the files it names from, beside its own: those of the workspace. */
export function setReadableFolders (find: () => string[]): void {
    readableFolders = find;
}

/**
    Sets who tells whether a document may read the files it names wherever they are, rather than in the folders of
    the workspace and its own only. It is unsafe: a document can then have any file its user can read given to the
    tool, so nobody tells it unless the user asked for it.
*/
export function setReadableAnywhere (find: () => boolean): void {
    readsAnywhere = find;
}

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

// The files a file names. A document names them between quotes of either style, and they are of a kind it reads. A
// model written as text names the buffers and the images it keeps in files of their own
function namesIn (file: string, data: Uint8Array): string[] {
    const kind = path.extname (file).toLowerCase ();
    const text = () => Buffer.from (data).toString ("utf8");

    if (kind === ".yui") {
        // Read as the parser of documents reads it: a string ends at the quote it starts with, unless a backslash is
        // before it, and what a string holds is no comment, as what a comment holds is no string
        return Array.from (text ().matchAll (/\/\/[^\n]*|\/\*[\s\S]*?\*\/|(["'])((?:\\[\s\S]|(?!\1)[^\\\n])*)\1/g), match => match[2] ?? "")
            .filter (name => referenced.has (path.extname (name).toLowerCase ()));
    }

    if (kind !== ".gltf")
        return [];

    try {
        const model = JSON.parse (text ()) as { buffers?: unknown; images?: unknown } | null;
        const parts = [model?.buffers, model?.images].flatMap (list => Array.isArray (list) ? list as ({ uri?: unknown } | null)[] : []);

        // What a model holds itself, or reads from elsewhere than a file, is an address starting with what it is read by
        return parts.map (part => part?.uri)
            .filter ((uri): uri is string => typeof uri === "string" && ! /^[A-Za-z][A-Za-z0-9+.-]*:/.test (uri))
            .map (uri => decodeURIComponent (uri));
    }
    catch {
        return [];
    }
}

/** A file as it is to be given: its size, and its bytes unless it has more of them than can be taken. */
interface FileRead {
    size: number;
    data?: Uint8Array;
}

// A file with changes that are not saved in an editor is read from it, so that what is typed counts before it is saved
async function read (file: string, mostBytes: number): Promise<FileRead | undefined> {
    const unsaved = unsavedText (file);

    try {
        if (unsaved !== undefined) {
            const data = Buffer.from (unsaved, "utf8");
            return { size: data.length, data: data.length <= mostBytes ? data : undefined };
        }

        const status = await fs.promises.stat (file);

        if (! status.isFile ())
            return undefined;

        return { size: status.size, data: status.size <= mostBytes ? await fs.promises.readFile (file) : undefined };
    }
    catch {
        return undefined;
    }
}

function megabytes (bytes: number): string {
    return `${(bytes / (1024 * 1024)).toFixed (1)} MB`;
}

/**
    Tells why a file that is there was not given to the tool: it is larger than a file may be, or
    the files given before it left no room for it.

    @param name The file, as who looks at the document is told it.
    @param size Its size in bytes.
*/
export function leftOutProblem (name: string, size: number): string {
    if (size > maximumFileBytes)
        return `'${name}' (${megabytes (size)}) is too large to be checked and shown here: the tool is given files of ${megabytes (maximumFileBytes)} at most. It is treated as missing`;

    return `'${name}' is left out of what is checked and shown here: the tool is given the files of a document up to ${megabytes (maximumBytes)} together, which those before it take. It is treated as missing`;
}

function isInside (folder: string, file: string): boolean {
    const relative = path.relative (folder, file);

    // Only `..` itself leads out: a name may start with two dots
    return relative !== ".." && ! relative.startsWith (".." + path.sep) && ! path.isAbsolute (relative);
}

// What a document may read of what it names, unless who collects it says otherwise: the files of the workspace and
// those of its own folder, where they really are, or any file when the user lifted that (see setReadableAnywhere).
// A document that is no file has no folder
function readableBy (main: string): (file: string) => boolean {
    const readable = path.isAbsolute (main) ? [...readableFolders (), path.dirname (main)] : readableFolders ();

    return file => readsAnywhere () || isInsideFolders (file, readable);
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
    Collects a document with the files it names, followed through the documents it imports and the
    models it shows, and with those the tool told it reads (see noteMissingFiles).

    A name is looked for next to the file naming it, then next to the main document, which is where
    the paths of a document are resolved from. A file is read as it is on disk, byte for byte,
    unless an editor holds changes of it that are not saved.

    @param isAllowed Tells whether a file a document names may be read. Unless given, those of the folders
                     of the workspace (see setReadableFolders) and of the folder of the document are,
                     where they really are: a link leading out of them is not followed. Any file is
                     when the user lifted that (see setReadableAnywhere).
*/
export async function collectDocumentFiles (document: EditedDocument, modelsPath: string | undefined, isAllowed: (file: string) => boolean = readableBy (document.uri.fsPath)): Promise<DocumentFiles> {
    const main = document.uri.fsPath;
    const contents = new Map<string, Uint8Array> ([[main, Buffer.from (document.getText (), "utf8")]]);
    const pending = [main];
    const missing = new Set<string> ();
    const refused: string[] = [];
    const problems: string[] = [];
    let total = 0;

    // Takes a file that may be read, is there and fits in what the tool is given
    const take = async (file: string): Promise<boolean> => {
        if (missing.has (file))
            return false;

        const allowed = isAllowed (file);
        const found = allowed ? await read (file, Math.min (maximumFileBytes, maximumBytes - total)) : undefined;

        if (found?.data === undefined) {
            if (found !== undefined)
                problems.push (leftOutProblem (path.relative (path.dirname (main), file), found.size));

            if (! allowed)
                refused.push (file);

            missing.add (file);
            return false;
        }

        total += found.data.length;
        contents.set (file, found.data);
        pending.push (file);
        return true;
    };

    for (const file of told.get (main) ?? []) {
        if (! contents.has (file))
            await take (file);
    }

    while (pending.length > 0) {
        const file = pending.pop () as string;

        for (const name of namesIn (file, contents.get (file) as Uint8Array)) {
            if (path.isAbsolute (name))
                continue;

            for (const folder of [path.dirname (file), path.dirname (main)]) {
                const candidate = path.resolve (folder, name);

                if (contents.has (candidate) || await take (candidate))
                    break;
            }
        }
    }

    // A file that is there and is left unread for where it is, is told: the tool only finds it missing
    for (const file of refused) {
        if (fs.existsSync (file))
            problems.push (`'${path.relative (path.dirname (main), file)}' is outside the folders of the workspace and of the document, so it is not read. It is treated as missing`);
    }

    // The files keep the places they have relative to each other, under the folder holding them all
    const folder = commonFolder (Array.from (contents.keys ()));
    const toolPath = (file: string) => `${folderName (folder)}/${path.relative (folder, file).split (path.sep).join ("/")}`;
    const files: YuiToolFile[] = [];

    for (const [file, data] of contents) {
        if (isInside (folder, file))
            files.push ({ path: toolPath (file), data });
        else
            problems.push (`'${file}' is on another drive than the document, so the tool can't be given it with it. It is treated as missing`);
    }

    // The tool keeps the files it was given: one a document names and that is gone is forgotten there too
    for (const file of missing) {
        if (isInside (folder, file) && ! contents.has (file))
            files.push ({ path: toolPath (file), data: new Uint8Array (), removed: true });
    }

    // Who is told about the files that change finds here the documents to check and show again
    const sought = new Set ([...contents.keys (), ...missing].filter (file => file !== main));
    named.set (main, sought);

    const models = modelsPath === undefined ? undefined : (await read (modelsPath, maximumFileBytes))?.data;

    if (modelsPath !== undefined && models !== undefined)
        files.push ({ path: toolPathOf (modelsPath), data: models });

    // The tool reads the models next to a document by itself: those that are gone are forgotten there too
    const beside = path.join (path.dirname (main), path.basename (main, ".yui") + ".models.json");

    if (! fs.existsSync (beside))
        files.push ({ path: toolPath (beside), data: new Uint8Array (), removed: true });

    return { main: toolPath (main), models: modelsPath !== undefined && models !== undefined ? toolPathOf (modelsPath) : "", files, problems, sought };
}

/**
    Remembers the files the tool told a document reads without having been given them, which are
    what its texts do not name: what a build compiles a shader into, a file of a kind a document
    is not known to read, one named by an expression. The document is collected with them from
    then on (see collectDocumentFiles).

    @param main    The file of the document.
    @param missing The files, by their paths in the folder of the tool.
    @param sought  The files the document was collected with for the run telling them (see DocumentFiles).

    @returns True when one of them is there to be given and the document was collected without it:
             what ran on the document then runs again, whether or not something else running on
             it was told the file first. One that is not there is not asked for again, and is
             given once it is there.
*/
export function noteMissingFiles (main: string, missing: string[], sought: ReadonlySet<string>): boolean {
    const known = told.get (main) ?? new Set<string> ();
    let found = false;

    told.set (main, known);

    for (const toolPath of missing) {
        const file = realPathOf (toolPath);

        if (file === undefined)
            continue;

        known.add (file);
        found = found || (! sought.has (file) && (unsavedText (file) !== undefined || fs.existsSync (file)));
    }

    return found;
}

/**
    Runs something of the tool on a document, again for as long as the tool tells files the document
    reads that it was not given and there are some of them to give (see noteMissingFiles).

    @param run Collects the document and runs on it. It returns what ran, with the file of the
               document, the files it was collected with and the files the tool told, or nothing
               when it could not run.

    @returns What the last run returned.
*/
export async function runUntilFilesAreGiven<Ran extends { document?: string; sought?: ReadonlySet<string>; missing: string[] }> (run: () => Promise<Ran | undefined>): Promise<Ran | undefined> {
    for (let round = 1; ; ++round) {
        const ran = await run ();

        if (ran === undefined || ran.document === undefined || ran.sought === undefined || ! noteMissingFiles (ran.document, ran.missing, ran.sought) || round === maximumRounds)
            return ran;
    }
}

/** Tells whether a document was given a file when it was last collected, or named it while it was not there. */
export function isNamedBy (main: string, file: string): boolean {
    return named.get (main)?.has (file) === true;
}
