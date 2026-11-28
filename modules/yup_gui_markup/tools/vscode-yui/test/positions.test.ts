// Tests of the places of a line as the tool and an editor tell them, run by `npm test` with Node: the tool counts
// characters from 1, an editor the UTF-16 units they are written with from 0.

import assert from "node:assert/strict";
import { test } from "node:test";
import { characterOf, toolColumnOf } from "../src/positions";

test ("a place is told the same by both where every character is written with one unit, the tool counting from 1", () => {
    const line = "Label { text: \"caf\u00e9\"; }";

    assert.equal (toolColumnOf (line, 0), 1);
    assert.equal (toolColumnOf (line, 19), 20);
    assert.equal (toolColumnOf (line, line.length), line.length + 1);

    assert.equal (characterOf (line, 1), 0);
    assert.equal (characterOf (line, 20), 19);
    assert.equal (characterOf (line, line.length + 1), line.length);
});

test ("a character outside the basic plane counts once for the tool and twice for an editor", () => {
    // The emoji is the 8th character, written at the units 7 and 8: what follows it is one further for an editor
    const line = "text: \"\u{1f600} a\"; id: b;";

    assert.equal (line.indexOf ("id"), 14);

    assert.equal (toolColumnOf (line, 7), 8);
    assert.equal (toolColumnOf (line, 9), 9);
    assert.equal (toolColumnOf (line, 14), 14);

    assert.equal (characterOf (line, 8), 7);
    assert.equal (characterOf (line, 9), 9);
    assert.equal (characterOf (line, 14), 14);

    // Two of them before a place set it two apart
    assert.equal (toolColumnOf ("\u{1f600}\u{1f600}x", 4), 3);
    assert.equal (characterOf ("\u{1f600}\u{1f600}x", 3), 4);
});

test ("a place goes to the tool and back unchanged", () => {
    const line = "a\u{1f3b9}b\u00e9\u{1f600}c";

    for (const character of [0, 1, 3, 4, 5, 7, 8])
        assert.equal (characterOf (line, toolColumnOf (line, character)), character);
});

test ("a column beyond the end of a line, or of a line nobody holds, is taken one unit each", () => {
    assert.equal (characterOf ("ab", 5), 4);
    assert.equal (characterOf ("\u{1f600}", 3), 3);
    assert.equal (characterOf ("", 4), 3);
    assert.equal (characterOf ("", 1), 0);
    assert.equal (characterOf ("ab", 0), 0);
});
