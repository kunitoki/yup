// Tests of the server of the tools for agents, run by `npm test` with Node: a client of the protocol
// lists and calls the tools of a stand-in for the extension, over HTTP as an agent does.

import assert from "node:assert/strict";
import * as http from "node:http";
import { after, before, test } from "node:test";
import { Client, StreamableHTTPClientTransport } from "@modelcontextprotocol/client";
import { AgentBackend, AgentServer, hasToken } from "../src/agentServer";

const image = Buffer.from ("not really a PNG").toString ("base64");
const token = "0f1e2d3c4b5a69788796a5b4c3d2e1f0";
const calls: { name: string; args: Record<string, unknown> }[] = [];

const backend: AgentBackend = {
    describe: async () => ({
        instructions: "Check what you write.",
        tools: [{ name: "yui_render", description: "Draws a file.", inputSchema: { type: "object", properties: { file: { type: "string" } }, required: ["file"] } }]
    }),

    call: async (name, args) => {
        calls.push ({ name, args });

        return name === "yui_render" ? { text: "drawn", isError: false, image }
                                     : { text: `there is no tool '${name}'`, isError: true, image: "" };
    }
};

let server: AgentServer;

// Asks the server with the headers given, and returns the status it answers with
function request (method: string, path: string, headers: Record<string, string>): Promise<number> {
    return new Promise ((resolve, reject) => {
        const sent = http.request ({ host: "127.0.0.1", port: server.port, path, method, headers }, response => {
            response.resume ();
            resolve (response.statusCode ?? 0);
        });

        sent.on ("error", reject);
        sent.end ();
    });
}

before (async () => {
    server = await AgentServer.start (backend, 0, "1.0.0", token);
});

after (async () => {
    await server.stop ();
});

test ("the server listens on the loopback address, at its endpoint", () => {
    assert.ok (server.port > 0);
    assert.equal (server.url, `http://127.0.0.1:${server.port}/mcp`);
});

test ("a client lists the tools, and gets the text and the image of a call", async () => {
    const client = new Client ({ name: "test", version: "1.0.0" }, { versionNegotiation: { mode: "auto" } });

    // The headers are those the file agents read lists the server with
    await client.connect (new StreamableHTTPClientTransport (new URL (server.url), { requestInit: { headers: server.headers } }));

    const listed = await client.listTools ();

    assert.deepEqual (listed.tools.map (tool => tool.name), ["yui_render"]);
    assert.deepEqual (listed.tools[0].inputSchema.required, ["file"]);

    const drawn = await client.callTool ({ name: "yui_render", arguments: { file: "main.yui" } });
    const blocks = drawn.content.map (block => block.type === "text" ? block.text : (block.type === "image" ? `${block.mimeType} ${block.data}` : block.type));

    assert.deepEqual (blocks, ["drawn", `image/png ${image}`]);
    assert.notEqual (drawn.isError, true);
    assert.deepEqual (calls, [{ name: "yui_render", args: { file: "main.yui" } }]);

    const refused = await client.callTool ({ name: "yui_bogus", arguments: {} });

    assert.equal (refused.isError, true);
    assert.equal (refused.content.length, 1);

    await client.close ();
});

test ("what is asked for another host, from the page of one or at another path is refused", async () => {
    const host = `127.0.0.1:${server.port}`;

    assert.equal (await request ("POST", "/mcp", { Host: "example.com", ...server.headers }), 403);
    assert.equal (await request ("POST", "/mcp", { Host: host, Origin: "https://example.com", ...server.headers }), 403);
    assert.equal (await request ("GET", "/other", { Host: host, ...server.headers }), 404);
});

test ("what is asked without the token of the server is refused, whatever it asks for", async () => {
    const host = `127.0.0.1:${server.port}`;

    assert.deepEqual (server.headers, { Authorization: `Bearer ${token}` });

    assert.equal (await request ("POST", "/mcp", { Host: host }), 401);
    assert.equal (await request ("POST", "/mcp", { Host: host, Authorization: "Bearer " + token.replace ("0", "1") }), 401);
    assert.equal (await request ("POST", "/mcp", { Host: host, Authorization: token }), 401);
    assert.equal (await request ("GET", "/other", { Host: host }), 401);
});

test ("a token is told from the others whole, as the bearer of a request", () => {
    assert.ok (hasToken (`Bearer ${token}`, token));
    assert.ok (hasToken (`bearer ${token}`, token));

    assert.ok (! hasToken (undefined, token));
    assert.ok (! hasToken ("", token));
    assert.ok (! hasToken (token, token));
    assert.ok (! hasToken (`Basic ${token}`, token));
    assert.ok (! hasToken (`Bearer ${token.substring (1)}`, token));
    assert.ok (! hasToken (`Bearer ${token}0`, token));
    assert.ok (! hasToken (`Bearer ${token} ${token}`, token));

    // A server with no token answers nobody, rather than everybody
    assert.ok (! hasToken ("Bearer ", ""));
    assert.ok (! hasToken (undefined, ""));
});

test ("a port that is taken is not shared", async () => {
    await assert.rejects (AgentServer.start (backend, server.port, "1.0.0", token), { code: "EADDRINUSE" });
});
