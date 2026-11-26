// Tests of how a file about to hold a secret is kept out of the commits of a repository, run by
// `npm test` with Node: a stand-in answers for git what each test scripts, and git itself is
// asked last, where it is there.

import assert from "node:assert/strict";
import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import { after, before, test } from "node:test";
import { GitRunner, gitRunner, keepOutOfCommits } from "../src/versionControl";

/** What the stand-in for git answers in a work tree, where a file is neither tracked nor ignored unless told otherwise. */
interface Repository {
    tracked?: boolean;

    /** What `check-ignore` answers each time it is asked: before the rule of the clone is added, then after. */
    ignored?: boolean[];

    /** The folder git is asked in, from the top of the work tree. */
    prefix?: string;

    /** The file of the rules of the clone, as git prints it. */
    rules?: string;
}

let root: string;

before (() => {
    root = fs.mkdtempSync (path.join (os.tmpdir (), "yui-version-control-"));

    // The git a test runs finds its repository by itself, and reads the settings and the rules of nobody
    for (const name of Object.keys (process.env)) {
        if (name.startsWith ("GIT_"))
            delete process.env[name];
    }

    process.env.HOME = root;
    process.env.XDG_CONFIG_HOME = root;
    process.env.GIT_CONFIG_NOSYSTEM = "1";
});

after (() => fs.rmSync (root, { recursive: true, force: true }));

// Makes the folder of a test, which no other test writes to
function folderOf (name: string): string {
    const folder = path.join (root, name);

    fs.mkdirSync (folder);
    return folder;
}

// The stand-in for git, which keeps what it is asked
function gitOf (repository: Repository, asked: string[][] = []): GitRunner {
    const ignored = [...(repository.ignored ?? [false, true])];

    return async args => {
        asked.push (args);

        switch (args.slice (0, 2).join (" ")) {
            case "rev-parse --is-inside-work-tree": return { code: 0, stdout: "true\n" };
            case "ls-files --error-unmatch": return { code: repository.tracked === true ? 0 : 1, stdout: "" };
            case "check-ignore -q": return { code: ignored.shift () === true ? 0 : 1, stdout: "" };
            case "rev-parse --show-prefix": return { code: 0, stdout: `${repository.prefix ?? ""}\n` };
            case "rev-parse --git-path": return { code: 0, stdout: `${repository.rules ?? ".git/info/exclude"}\n` };
            default: return { code: 129, stdout: "" };
        }
    };
}

test ("a folder git knows nothing of is left alone", async () => {
    const folder = folderOf ("plain");
    const asked: string[][] = [];

    const git: GitRunner = async (args, where) => {
        asked.push ([where, ...args]);
        return { code: 128, stdout: "" };
    };

    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), git), "outside");
    assert.deepEqual (asked, [[folder, "rev-parse", "--is-inside-work-tree"]]);

    // Neither is the folder of a repository itself, where git answers that it is in no work tree
    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), async () => ({ code: 0, stdout: "false\n" })), "outside");
    assert.deepEqual (fs.readdirSync (folder), []);
});

test ("a git that is not there reads as a folder it knows nothing of", async () => {
    const folder = folderOf ("no-git");
    const git = gitRunner (path.join (root, "absent", "git"));

    assert.notEqual ((await git (["rev-parse", "--is-inside-work-tree"], folder)).code, 0);
    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), git), "outside");
});

test ("a git that gives no answer in time fails, rather than reading as a folder it knows nothing of", async () => {
    const folder = folderOf ("slow-git");
    const git: GitRunner = () => Promise.reject (new Error ("git rev-parse took too long to answer"));

    await assert.rejects (keepOutOfCommits (folder, path.join (folder, ".mcp.json"), git), /took too long/);
    assert.deepEqual (fs.readdirSync (folder), []);
});

test ("a file git tracks is told as such, with nothing written", async () => {
    const folder = folderOf ("tracked");
    const asked: string[][] = [];

    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), gitOf ({ tracked: true }, asked)), "tracked");

    // It is asked about by the name it has in the folder git runs in
    assert.deepEqual (asked[1], ["ls-files", "--error-unmatch", "--", ".mcp.json"]);
    assert.deepEqual (fs.readdirSync (folder), []);
});

test ("a file already ignored is told as such, with nothing written", async () => {
    const folder = folderOf ("ignored");

    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), gitOf ({ ignored: [true] })), "ignored");
    assert.deepEqual (fs.readdirSync (folder), []);
});

test ("a file neither tracked nor ignored is ignored for this clone only, in a file made with its folder", async () => {
    const folder = folderOf ("excluded");

    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), gitOf ({})), "excluded");

    // Git printed the file of the rules from the folder it ran in, and the rule names that file only
    assert.equal (fs.readFileSync (path.join (folder, ".git", "info", "exclude"), "utf8"), "/.mcp.json\n");
});

test ("the rules already there are kept, and the last of them ended", async () => {
    const folder = folderOf ("kept");
    const rules = path.join (folder, ".git", "info", "exclude");

    fs.mkdirSync (path.dirname (rules), { recursive: true });
    fs.writeFileSync (rules, "# the rules of this clone\n*.log");

    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), gitOf ({})), "excluded");
    assert.equal (fs.readFileSync (rules, "utf8"), "# the rules of this clone\n*.log\n/.mcp.json\n");
});

test ("a rule that is already there is not added again", async () => {
    const folder = folderOf ("twice");

    // Git is scripted not to ignore the file at first, both times
    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), gitOf ({})), "excluded");
    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), gitOf ({})), "excluded");

    assert.equal (fs.readFileSync (path.join (folder, ".git", "info", "exclude"), "utf8"), "/.mcp.json\n");
});

test ("the rule names the file from the top of the work tree, whatever folder git is asked in", async () => {
    const folder = folderOf ("nested");
    const asked: string[][] = [];

    assert.equal (await keepOutOfCommits (folder, path.join (folder, "agents", ".mcp.json"), gitOf ({ prefix: "packages/app/" }, asked)), "excluded");

    assert.deepEqual (asked[1], ["ls-files", "--error-unmatch", "--", "agents/.mcp.json"]);
    assert.equal (fs.readFileSync (path.join (folder, ".git", "info", "exclude"), "utf8"), "/packages/app/agents/.mcp.json\n");
});

test ("what git reads as a pattern in the path of the file is escaped, so the rule names nothing else", async () => {
    const folder = folderOf ("pattern");

    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), gitOf ({ prefix: "apps/[web]*/" })), "excluded");
    assert.equal (fs.readFileSync (path.join (folder, ".git", "info", "exclude"), "utf8"), "/apps/\\[web]\\*/.mcp.json\n");
});

test ("the file of the rules is the one git names, as the one of the repository a linked work tree is of", async () => {
    const folder = folderOf ("linked");
    const rules = path.join (root, "main", ".git", "info", "exclude");

    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), gitOf ({ rules })), "excluded");

    assert.equal (fs.readFileSync (rules, "utf8"), "/.mcp.json\n");
    assert.deepEqual (fs.readdirSync (folder), []);
});

test ("a file the rules of the repository keep among those to commit is told as such", async () => {
    const folder = folderOf ("shared");

    // Git still does not ignore it once the rule of the clone is there, as with `!.mcp.json` in a `.gitignore`
    assert.equal (await keepOutOfCommits (folder, path.join (folder, ".mcp.json"), gitOf ({ ignored: [false, false] })), "shared");
});

test ("git itself answers as its stand-in does, and follows the rule it is given", async t => {
    const git = gitRunner ();
    const top = folderOf ("real");
    const folder = path.join (top, "packages", "app");
    const file = path.join (folder, ".mcp.json");

    if ((await git (["init"], top)).code !== 0) {
        t.skip ("git is not there");
        return;
    }

    fs.mkdirSync (folder, { recursive: true });
    fs.writeFileSync (file, "{}\n");

    assert.equal (await keepOutOfCommits (folder, file, git), "excluded");
    assert.ok (fs.readFileSync (path.join (top, ".git", "info", "exclude"), "utf8").endsWith ("/packages/app/.mcp.json\n"));
    assert.equal (await keepOutOfCommits (folder, file, git), "ignored");

    // The project names the file as one to commit, which the rule of a clone does not undo
    fs.writeFileSync (path.join (top, ".gitignore"), "!.mcp.json\n");
    assert.equal (await keepOutOfCommits (folder, file, git), "shared");

    assert.equal ((await git (["add", "--", ".mcp.json"], folder)).code, 0);
    assert.equal (await keepOutOfCommits (folder, file, git), "tracked");
});
