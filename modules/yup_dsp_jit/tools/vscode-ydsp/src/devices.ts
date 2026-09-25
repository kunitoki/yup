import { runCompiler } from "./compiler";

export interface YdspAudioDevice {
    type: string;
    direction: "input" | "output";
    name: string;
}

export interface YdspMidiDevice {
    direction: "input" | "output";
    name: string;
    id: string;
}

export interface YdspDevices {
    audio: YdspAudioDevice[];
    midi: YdspMidiDevice[];
}

/** Enumerates the audio backends and audio/MIDI devices reported by `devices --json`. */
export async function listYdspDevices (executable: string): Promise<YdspDevices> {
    const result = await runCompiler (executable, ["devices", "--json"], undefined);

    if (result.code !== 0)
        throw new Error (result.stderr.trim () || `yup_dsp_compiler devices exited with code ${result.code}`);

    const parsed = JSON.parse (result.stdout) as Partial<YdspDevices>;

    return {
        audio: Array.isArray (parsed.audio) ? parsed.audio : [],
        midi: Array.isArray (parsed.midi) ? parsed.midi : []
    };
}
