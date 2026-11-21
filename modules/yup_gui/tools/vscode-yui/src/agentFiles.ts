import * as fs from "fs";
import * as path from "path";

// Where a file really is: its links followed as far as it exists, the rest of its path as it is written
function realPath (file: string): string {
    let existing = path.resolve (file);
    const missing: string[] = [];

    for (;;) {
        try {
            return path.join (fs.realpathSync (existing), ...missing);
        }
        catch {
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

    return folders.some (folder => {
        const relative = path.relative (realPath (folder), found);
        return relative.length > 0 && ! relative.startsWith ("..") && ! path.isAbsolute (relative);
    });
}

/** Writes an image given in base64 to a file, with the folders it needs, replacing the one there. */
export async function saveImage (file: string, base64: string): Promise<void> {
    await fs.promises.mkdir (path.dirname (file), { recursive: true });
    await fs.promises.writeFile (file, Buffer.from (base64, "base64"));
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
