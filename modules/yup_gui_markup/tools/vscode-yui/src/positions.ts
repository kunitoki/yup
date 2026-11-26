// The tool tells a place by its line and its column from 1, and counts the characters of a line. An editor counts
// from 0 the UTF-16 units they are written with, which are two for a character outside the basic plane, as an
// emoji is: the two only agree up to the first of those on a line

/** Returns the column the tool knows a place of a line by, given where an editor has it. */
export function toolColumnOf (line: string, character: number): number {
    return Array.from (line.substring (0, character)).length + 1;
}

/** Returns where an editor has a column of the tool on a line. What lies beyond the line counts as one unit each. */
export function characterOf (line: string, column: number): number {
    let left = Math.max (0, column - 1);
    let character = 0;

    for (const written of line) {
        if (left === 0)
            break;

        character += written.length;
        --left;
    }

    return character + left;
}
