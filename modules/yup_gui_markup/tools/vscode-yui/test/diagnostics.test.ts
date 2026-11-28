// Tests of the problems of files kept by the document whose check told them, run by `npm test` with Node: a file
// two open documents import shows what both tell, and loses what one told when that one is checked again or closed.

import assert from "node:assert/strict";
import { test } from "node:test";
import { SharedDiagnostics } from "../src/diagnostics";

interface Problem {
    line: number;
    message: string;
}

function shared (): SharedDiagnostics<Problem> {
    return new SharedDiagnostics<Problem> (problem => `${problem.line} ${problem.message}`);
}

function told (files: Record<string, Problem[]>): Map<string, Problem[]> {
    return new Map (Object.entries (files));
}

test ("a file shows what the checks of all the documents importing it tell, each problem once", () => {
    const problems = shared ();
    const inPart = { line: 3, message: "unknown property 'colour'" };
    const fromSecond = { line: 9, message: "unknown element 'params'" };

    assert.deepEqual (problems.set ("first.yui", told ({ "first.yui": [{ line: 1, message: "in the first" }], "part.yui": [inPart] })).sort (), ["first.yui", "part.yui"]);
    assert.deepEqual (problems.set ("second.yui", told ({ "part.yui": [{ ...inPart }, fromSecond] })), ["part.yui"]);

    assert.deepEqual (problems.of ("part.yui"), [inPart, fromSecond]);
    assert.deepEqual (problems.of ("first.yui"), [{ line: 1, message: "in the first" }]);
    assert.deepEqual (problems.of ("second.yui"), []);
});

test ("a document checked again replaces what its check told, and leaves what the others tell", () => {
    const problems = shared ();
    const inPart = { line: 3, message: "unknown property 'colour'" };
    const fromSecond = { line: 9, message: "unknown element 'params'" };

    problems.set ("first.yui", told ({ "first.yui": [{ line: 1, message: "in the first" }], "part.yui": [inPart] }));
    problems.set ("second.yui", told ({ "part.yui": [inPart, fromSecond] }));

    // The first one no longer has problems of its own, and still tells the one of the file it imports
    assert.deepEqual (problems.set ("first.yui", told ({ "part.yui": [inPart] })).sort (), ["first.yui", "part.yui"]);
    assert.deepEqual (problems.of ("first.yui"), []);
    assert.deepEqual (problems.of ("part.yui"), [inPart, fromSecond]);

    // The second one is fixed: what only it told goes, what the first tells stays
    assert.deepEqual (problems.set ("second.yui", told ({})), ["part.yui"]);
    assert.deepEqual (problems.of ("part.yui"), [inPart]);
});

test ("a document that is closed takes away its own problems only", () => {
    const problems = shared ();
    const inPart = { line: 3, message: "unknown property 'colour'" };

    problems.set ("first.yui", told ({ "first.yui": [{ line: 1, message: "in the first" }], "part.yui": [inPart] }));
    problems.set ("part.yui", told ({ "part.yui": [inPart] }));

    assert.deepEqual (problems.clear ("first.yui").sort (), ["first.yui", "part.yui"]);
    assert.deepEqual (problems.of ("first.yui"), []);
    assert.deepEqual (problems.of ("part.yui"), [inPart]);

    assert.deepEqual (problems.clear ("part.yui"), ["part.yui"]);
    assert.deepEqual (problems.of ("part.yui"), []);

    // Nothing is left to show again once nobody tells anything
    assert.deepEqual (problems.clear ("part.yui"), []);
    assert.deepEqual (problems.clear ("never checked.yui"), []);
});
