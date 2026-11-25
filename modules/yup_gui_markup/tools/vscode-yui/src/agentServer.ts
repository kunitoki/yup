import * as http from "http";
import { localhostHostValidation, localhostOriginValidation, toNodeHandler } from "@modelcontextprotocol/node";
import { CallToolResult, ContentBlock, createMcpHandler, Server, Tool } from "@modelcontextprotocol/server";

/** What a tool answers: text for the agent to read, and a PNG in base64 which is empty when the tool gives none. */
export interface AgentToolAnswer {
    text: string;
    isError: boolean;
    image: string;
}

/** The tools an agent is given, with how they are meant to be used. */
export interface AgentTools {
    tools: Tool[];
    instructions: string;
}

/** Who knows the tools and runs them: the extension, or a stand-in when the server is tested. */
export interface AgentBackend {
    describe (): Promise<AgentTools>;
    call (name: string, args: Record<string, unknown>): Promise<AgentToolAnswer>;
}

/** The path of the server, which a client is given with its address. */
export const agentEndpoint = "/mcp";

function toToolResult (answer: AgentToolAnswer): CallToolResult {
    const content: ContentBlock[] = [{ type: "text", text: answer.text }];

    if (answer.image.length > 0)
        content.push ({ type: "image", data: answer.image, mimeType: "image/png" });

    return { content, isError: answer.isError };
}

/**
    Serves the tools of a backend to agents on this machine, over the Model Context Protocol.

    It only listens on the loopback address and refuses the requests made for another host or from
    the page of another one, which is how a web page would reach a server of the machine it is
    shown on. Each request is answered on its own, with nothing kept between two of them.
*/
export class AgentServer {
    private constructor (private readonly server: http.Server, private readonly close: () => Promise<void>, readonly port: number) {
    }

    /** The address a client is given. */
    get url (): string {
        return `http://127.0.0.1:${this.port}${agentEndpoint}`;
    }

    /** Starts listening on a port, or on any free one when given 0. It fails when the port is taken. */
    static async start (backend: AgentBackend, port: number, version: string): Promise<AgentServer> {
        const handler = createMcpHandler (async () => {
            const described = await backend.describe ();
            const server = new Server ({ name: "vscode-yui", version }, { capabilities: { tools: {} }, instructions: described.instructions });

            server.setRequestHandler ("tools/list", async () => ({ tools: described.tools }));
            server.setRequestHandler ("tools/call", async request => toToolResult (await backend.call (request.params.name, request.params.arguments ?? {})));

            return server;
        }, { responseMode: "json" });

        const handle = toNodeHandler (handler);
        const validateHost = localhostHostValidation ();
        const validateOrigin = localhostOriginValidation ();

        const server = http.createServer ((request, response) => {
            // The guards answer what they refuse themselves
            if (! validateHost (request, response) || ! validateOrigin (request, response))
                return;

            let path: string | undefined;

            try {
                path = new URL (request.url ?? "/", "http://127.0.0.1").pathname;
            }
            catch {
                path = undefined;
            }

            if (path !== agentEndpoint) {
                response.writeHead (path === undefined ? 400 : 404).end ();
                return;
            }

            void handle (request, response);
        });

        await new Promise<void> ((resolve, reject) => {
            server.once ("error", reject);
            server.listen (port, "127.0.0.1", () => {
                server.off ("error", reject);
                resolve ();
            });
        });

        const address = server.address ();

        return new AgentServer (server, () => handler.close (), typeof address === "object" && address !== null ? address.port : port);
    }

    /** Stops listening, ending the requests being answered. */
    async stop (): Promise<void> {
        await this.close ().catch (() => undefined);

        await new Promise<void> (resolve => {
            this.server.close (() => resolve ());
            this.server.closeAllConnections ();
        });
    }
}
