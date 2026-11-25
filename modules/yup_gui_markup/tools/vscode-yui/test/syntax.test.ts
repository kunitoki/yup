// Tests of the grammar and of the snippets, run by `npm test` with Node: the scopes the grammar gives a `shader`
// block, as an editor reading it does, and the snippets being ones an editor can insert.

import assert from "node:assert/strict";
import * as fs from "node:fs";
import * as path from "node:path";
import { test } from "node:test";

interface Rule {
    name?: string;
    match?: string;
    begin?: string;
    end?: string;
    captures?: Record<string, { name: string }>;
    include?: string;
}

interface Grammar {
    patterns: Rule[];
    repository: Record<string, { patterns: Rule[] }>;
}

interface Snippet {
    prefix: string;
    body: string[];
    description: string;
}

// The tests are bundled into a folder next to those of the grammar and of the snippets
const extension = path.join (__dirname, "..");
const grammar = JSON.parse (fs.readFileSync (path.join (extension, "syntaxes", "yui.tmLanguage.json"), "utf8")) as Grammar;
const snippets = JSON.parse (fs.readFileSync (path.join (extension, "snippets", "yui.code-snippets"), "utf8")) as Record<string, Snippet>;

// The rules in the order they are tried, each group the grammar includes in the place it is included at
const rules = grammar.patterns.flatMap (rule => rule.include === undefined ? [rule] : grammar.repository[rule.include.substring (1)].patterns);

/**
    Returns the scopes of each character of a line, as an editor reading the grammar gives them: at
    each place the rule matching the closest wins, the first of those matching at the same place,
    and the line is read on from the end of what it matched.

    What a rule that begins and ends holds is given the scope of the rule alone, which is all the
    lines tested here need of the strings and of the comments.
*/
function scopesOf (line: string): string[][] {
    const scopes: string[][] = Array.from (line, () => []);
    const mark = (name: string | undefined, start: number, end: number) => {
        for (let index = start; name !== undefined && index < end; ++index)
            scopes[index].push (...name.split (" "));
    };

    for (let position = 0; position < line.length;) {
        let rule: Rule | undefined;
        let found: RegExpExecArray | undefined;

        for (const candidate of rules) {
            const pattern = new RegExp (candidate.match ?? candidate.begin ?? "", "dg");

            pattern.lastIndex = position;

            const matched = pattern.exec (line);

            if (matched !== null && (found === undefined || matched.index < found.index)) {
                rule = candidate;
                found = matched;
            }
        }

        if (rule === undefined || found === undefined)
            break;

        let end = found.index + found[0].length;

        if (rule.end !== undefined) {
            const closing = new RegExp (rule.end, "g");

            closing.lastIndex = end;
            end = closing.exec (line) === null ? line.length : closing.lastIndex;
        }

        mark (rule.name, found.index, end);

        for (const [group, capture] of Object.entries (rule.captures ?? {})) {
            const place = found.indices?.[Number (group)];

            if (place !== undefined)
                mark (capture.name, place[0], place[1]);
        }

        position = Math.max (end, position + 1);
    }

    return scopes;
}

// The scopes of a word of a line, which are those of its first character
function scopesAt (line: string, word: string): string[] {
    const index = line.indexOf (word);

    assert.ok (index >= 0, `'${word}' is not in '${line}'`);
    return scopesOf (line)[index];
}

test ("a shader block is highlighted as a drawing block is", () => {
    const block = "shader Crt {";

    assert.deepEqual (scopesAt (block, "shader"), ["storage.type.yui"]);
    assert.deepEqual (scopesAt (block, "Crt"), ["entity.name.type.yui"]);
    assert.deepEqual (scopesAt (block, "{"), ["punctuation.section.block.yui"]);

    for (const word of ["drawing", "Knob", "{"])
        assert.deepEqual (scopesAt ("drawing Knob {", word), scopesAt (block, word === "drawing" ? "shader" : (word === "Knob" ? "Crt" : word)));

    // The keyword is one on its own too, as while its name is not typed yet
    assert.deepEqual (scopesAt ("shader", "shader"), ["storage.type.yui"]);
});

test ("the file and the properties of a shader are highlighted as the statements of any block", () => {
    for (const member of ["fragment", "material"]) {
        const line = `    ${member}: "fx/crt.frag";`;

        assert.deepEqual (scopesAt (line, member), ["support.type.property-name.yui", "meta.object-literal.key.yui"]);
        assert.deepEqual (scopesAt (line, "fx/crt"), ["string.quoted.double.yui"]);
        assert.deepEqual (scopesAt (line, ";"), ["punctuation.terminator.yui"]);
    }

    const property = "    property tint: color = #ff8800;";

    assert.deepEqual (scopesAt (property, "property"), ["storage.type.yui"]);
    assert.deepEqual (scopesAt (property, "tint"), ["variable.other.property.yui"]);
    assert.deepEqual (scopesAt (property, "color"), ["storage.type.primitive.yui"]);
    assert.deepEqual (scopesAt (property, "#ff8800"), ["constant.other.color.rgb-value.yui"]);
});

test ("a shader is named where it is used as a drawing is, and its name is a property of a material", () => {
    assert.deepEqual (scopesAt ("    use Crt { bounds: bounds; }", "use"), ["storage.type.yui"]);
    assert.deepEqual (scopesAt ("    use Crt { bounds: bounds; }", "Crt"), ["entity.name.type.yui"]);
    assert.deepEqual (scopesAt ("    .display { effect: Crt; }", "Crt"), ["entity.name.type.yui"]);

    for (const line of ["        shader: Hologram;", "        Material3D { shader: Hologram; }"]) {
        assert.deepEqual (scopesAt (line, "shader"), ["support.type.property-name.yui", "meta.object-literal.key.yui"]);
        assert.deepEqual (scopesAt (line, "Hologram"), ["entity.name.type.yui"]);
    }
});

test ("the snippets are ones an editor can insert", () => {
    const prefixes = Object.values (snippets).map (snippet => snippet.prefix);

    assert.equal (new Set (prefixes).size, prefixes.length);

    for (const [name, snippet] of Object.entries (snippets)) {
        const text = snippet.body.join ("\n");

        assert.ok (snippet.prefix.length > 0 && snippet.description.length > 0, name);
        assert.ok (snippet.body.every (line => typeof line === "string"), name);

        // What is typed over is `$1`, `${1:text}` or `${1|one,two|}`: nothing else starts with a dollar, and a stop
        // written again is one that was given its text
        assert.ok (! /\$(?![0-9]|\{[0-9]+[:|}])/.test (text), name);

        for (const [, stop] of text.matchAll (/\$([0-9]+)/g))
            assert.ok (text.includes ("${" + stop), `${name}: $${stop}`);

        // What is written once the stops are typed over has every brace, bracket and quote it opens closed
        const written = text.replace (/\$\{[0-9]+:([^{}]*)\}/g, "$1").replace (/\$\{[0-9]+\|([^,|]+)[^{}]*\}/g, "$1").replace (/\$[0-9]+/g, "x");

        for (const [open, close] of [["{", "}"], ["[", "]"], ["(", ")"]])
            assert.equal (written.split (open).length, written.split (close).length, `${name}: ${open}${close}`);

        assert.equal ((written.split ('"').length - 1) % 2, 0, `${name}: quotes`);
    }
});

test ("there are snippets of a shader, of a scene and of a model", () => {
    const written = (prefix: string) => Object.values (snippets).find (snippet => snippet.prefix === prefix)?.body.join ("\n") ?? "";

    assert.match (written ("shader"), /^shader \$\{1:Name\} \{\n    \$\{2\|fragment,material\|\}: "/);
    assert.match (written ("shader"), /\n    property /);

    for (const type of ["Scene3D", "Camera3D", "Light3D", "Mesh3D", "Material3D"])
        assert.ok (written ("scene").includes (`${type} {`), type);

    assert.match (written ("scene"), /vertices: \[.*\];\n +indices: \[/);
    assert.match (written ("model"), /^Model3D \{\n    source: "\$\{1:[^}]+[.]glb\}";/);
    assert.match (written ("model"), /Node3D \{ node: "/);
});
