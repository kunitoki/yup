/**
    Keeps the problems of files by the document whose check told them.

    A file is checked with each document importing it, and by itself when it is open, so several
    checks tell its problems. What the file shows is what all of them tell, each problem once: a
    check that tells none anymore, or a document that is closed, takes its own away and leaves
    those of the others.
*/
export class SharedDiagnostics<Item> {
    private readonly byFile = new Map<string, Map<string, Item[]>> ();

    /**
        @param identityOf Tells a problem from the others of a file: one that two checks tell is shown once.
    */
    constructor (private readonly identityOf: (item: Item) => string) {
    }

    /**
        Replaces what the check of a document told, by file.

        @returns The files to show again: those the check told problems of before, or tells of now.
    */
    set (owner: string, told: ReadonlyMap<string, Item[]>): string[] {
        const changed = new Set (told.keys ());

        for (const [file, owners] of this.byFile) {
            if (owners.delete (owner))
                changed.add (file);

            if (owners.size === 0)
                this.byFile.delete (file);
        }

        for (const [file, items] of told) {
            if (items.length === 0)
                continue;

            const owners = this.byFile.get (file) ?? new Map<string, Item[]> ();

            owners.set (owner, items);
            this.byFile.set (file, owners);
        }

        return Array.from (changed);
    }

    /** Forgets what the check of a document told. Returns the files to show again. */
    clear (owner: string): string[] {
        return this.set (owner, new Map ());
    }

    /** Returns what a file shows: the problems all the checks told of it, each once. */
    of (file: string): Item[] {
        const shown = new Map<string, Item> ();

        for (const items of this.byFile.get (file)?.values () ?? []) {
            for (const item of items) {
                const identity = this.identityOf (item);

                if (! shown.has (identity))
                    shown.set (identity, item);
            }
        }

        return Array.from (shown.values ());
    }
}
