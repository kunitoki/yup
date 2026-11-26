import { execFile } from "child_process";
import * as fs from "fs";
import * as path from "path";

/** What git answered: the code it ended with and what it printed. */
export interface GitAnswer {
    code: number;
    stdout: string;
}

/**
    Runs git with the arguments given, in a folder. A code other than 0 is an answer as any other, and
    is also the one of a git that could not be run: its folder then reads as one git knows nothing of.
    It fails when git is there and gives no answer in time, which tells nothing about the folder.
*/
export type GitRunner = (args: string[], folder: string) => Promise<GitAnswer>;

/**
    What git makes of a file:

    - `outside`: its folder is in no work tree, or git is not there to tell;
    - `tracked`: it is one of the files git commits;
    - `shared`: it is not yet, but a rule of the repository keeps it among the files to commit;
    - `ignored`: a rule already keeps it out of the commits;
    - `excluded`: none did, so one was added for this clone only.
*/
export type FileInGit = "outside" | "tracked" | "shared" | "ignored" | "excluded";

// How long git is given to answer
const answerMs = 5000;

/**
    Returns what runs git for real, with no shell reading its arguments.

    @param program The program run as git, which a test names to try one that is not there.
*/
export function gitRunner (program = "git"): GitRunner {
    return (args, folder) => new Promise<GitAnswer> ((resolve, reject) => {
        execFile (program, args, { cwd: folder, encoding: "utf8", timeout: answerMs, windowsHide: true }, (error, stdout) => {
            // A git stopped for taking too long answered nothing to go by
            if (error !== null && error.killed === true) {
                reject (new Error (`git ${args[0]} took too long to answer`));
                return;
            }

            // An error holds the code git ended with, and none when git did not run
            resolve ({ code: error === null ? 0 : (typeof error.code === "number" ? error.code : -1), stdout });
        });
    });
}

/**
    Tells what git makes of a file about to hold a secret, and makes sure it does not commit one that
    is neither tracked nor ignored: with a rule naming that file only, added to the rules of this
    clone (its `info/exclude`). Those are never committed, where a rule added to the `.gitignore` of
    the project would be a change of ours in its next commit.

    It fails when git gives no answer in time or the rule can't be written: the file is then one
    nothing keeps out of the commits.

    @param folder The folder git is asked in.
    @param file The file, inside that folder. It may not exist yet.
    @param git What runs git.
*/
export async function keepOutOfCommits (folder: string, file: string, git: GitRunner): Promise<FileInGit> {
    // What git prints for the arguments given, or nothing when it fails
    const printed = async (...args: string[]) => {
        const answer = await git (args, folder);

        return answer.code === 0 ? answer.stdout.replace (/\r?\n$/, "") : undefined;
    };

    if (await printed ("rev-parse", "--is-inside-work-tree") !== "true")
        return "outside";

    // The file as git names it from the folder it runs in, whatever leads to that folder
    const name = path.relative (folder, file).split (path.sep).join ("/");
    const holds = async (...args: string[]) => (await git ([...args, "--", name], folder)).code === 0;

    if (await holds ("ls-files", "--error-unmatch"))
        return "tracked";

    if (await holds ("check-ignore", "-q"))
        return "ignored";

    const prefix = await printed ("rev-parse", "--show-prefix");
    const rules = await printed ("rev-parse", "--git-path", "info/exclude");

    if (prefix === undefined || rules === undefined || rules.length === 0)
        throw new Error ("git does not tell where the rules of this clone are");

    // The file from the top of the work tree, so that no other of that name is ignored, with what git reads as a
    // pattern escaped
    const rule = "/" + (prefix + name).replace (/[\\*?\[]/g, "\\$&");

    // Git prints the file of the rules from the folder it runs in when it is under it
    const exclude = path.resolve (folder, rules);

    // A file of rules that is not there yet holds none
    const held = await fs.promises.readFile (exclude, "utf8").catch (() => "");

    if (! held.split (/\r?\n/).includes (rule)) {
        await fs.promises.mkdir (path.dirname (exclude), { recursive: true });
        await fs.promises.appendFile (exclude, `${held.length > 0 && ! held.endsWith ("\n") ? "\n" : ""}${rule}\n`);
    }

    // A rule of the repository naming the file as one to commit wins over the rules of a clone
    return await holds ("check-ignore", "-q") ? "excluded" : "shared";
}
