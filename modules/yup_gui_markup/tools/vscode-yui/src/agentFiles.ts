import * as crypto from "crypto";
import * as fs from "fs";
import * as path from "path";

function isLink (file: string): boolean {
    try {
        return fs.lstatSync (file).isSymbolicLink ();
    }
    catch {
        return false;
    }
}

// Where a file really is: its links followed as far as it exists, the rest of its path as it is written.
// Nowhere when a link on the way leads to nothing, which writing would follow
function realPath (file: string): string | undefined {
    let existing = path.resolve (file);
    const missing: string[] = [];

    for (;;) {
        try {
            return path.join (fs.realpathSync (existing), ...missing);
        }
        catch {
            if (isLink (existing))
                return undefined;

            const parent = path.dirname (existing);

            if (parent === existing)
                return path.resolve (file);

            missing.unshift (path.basename (existing));
            existing = parent;
        }
    }
}

/**
    Tells whether a file is inside one of the folders given where it really is: a link leading
    out of them is not. The file may not exist yet, which is the case of one about to be written.
*/
export function isInsideFolders (file: string, folders: string[]): boolean {
    const found = realPath (file);

    return found !== undefined && folders.some (folder => {
        const relative = path.relative (realPath (folder) ?? folder, found);

        // Only `..` itself leads out: a name may start with two dots
        return relative.length > 0 && relative !== ".." && ! relative.startsWith (".." + path.sep) && ! path.isAbsolute (relative);
    });
}

/**
    Writes an image given in base64 to a file, with the folders it needs, replacing the one there.

    The file is written whole or not at all: it takes the place of the one there once it is complete.

    @param folders The folders the file must be inside of where it really is (see isInsideFolders). What
                   leads to it may have changed since it was named, so it is looked at again here.
*/
export async function saveImage (file: string, base64: string, folders: string[]): Promise<void> {
    const check = () => {
        if (! isInsideFolders (file, folders))
            throw new Error (`'${file}' is not inside the folders it may be written to`);
    };

    // Neither its folders nor the file are made through a link leading elsewhere
    check ();
    await fs.promises.mkdir (path.dirname (file), { recursive: true });
    check ();

    const partial = `${file}.${crypto.randomBytes (6).toString ("hex")}.tmp`;

    try {
        // A file of that name is never there to be written through, and taking the place of a link does not follow it
        await fs.promises.writeFile (partial, Buffer.from (base64, "base64"), { flag: "wx" });
        await fs.promises.rename (partial, file);
    }
    catch (error) {
        await fs.promises.rm (partial, { force: true }).catch (() => undefined);
        throw error;
    }
}

/** Adds a member to the answer of a tool, which is left as it is when it is not an object. */
export function withMember (text: string, name: string, value: unknown): string {
    try {
        const parsed: unknown = JSON.parse (text);

        if (typeof parsed !== "object" || parsed === null || Array.isArray (parsed))
            return text;

        return JSON.stringify ({ ...parsed, [name]: value }, null, text.includes ("\n") ? 2 : undefined);
    }
    catch {
        return text;
    }
}
