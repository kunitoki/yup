# Copyright (c) 2026 - kunitoki@gmail.com
# ISC license: https://www.isc.org/licenses/
"""Run manually: python3 test_cli.py /absolute/path/to/yup_dsp_compiler"""

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY = str(Path(sys.argv.pop(1)).resolve())


class CompilerCliTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / "lib").mkdir()
        self.gain = "processor Gain { input stream in; output stream out; process { out = in * 2.0; } }"
        self.source = "import lib.Gain as lib; graph Main { input stream in; output stream out; node p = lib.Gain; connection { in -> p.in; p.out -> out; } }"
        self.manifest = "formatVersion: 1\nmain: Main\nsources: [Main.ydsp, lib/Gain.ydsp]\n"
        (self.root / "Main.ydsp").write_text(self.source)
        (self.root / "lib/Gain.ydsp").write_text(self.gain)
        (self.root / "Patch.ydsp-project").write_text(self.manifest)

    def invoke(self, *args):
        return subprocess.run([BINARY, *args], cwd=self.root, capture_output=True, text=True, timeout=60)

    def test_both_input_formats_compile_and_inspect(self):
        for source in ("Main.ydsp", "Patch.ydsp-project"):
            with self.subTest(source=source):
                result = self.invoke(source, "--output", "patch.ydsb")
                self.assertEqual(result.returncode, 0, result.stderr)
                inspected = self.invoke("--inspect", "patch.ydsb", "--list")
                self.assertEqual(inspected.returncode, 0, inspected.stderr)
                self.assertIn("sources: 2", inspected.stdout)
                self.assertIn("source-0 (root)", inspected.stdout)

    def test_invalid_project_has_path_excerpt_and_caret(self):
        result = self.invoke("Patch.ydsp-project", "--output", "patch.ydsb", "--main", "Missing")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn(str(self.root / "Patch.ydsp-project") + ":", result.stderr)
        self.assertIn("^", result.stderr)
        self.assertIn("formatVersion", result.stderr)

    def test_run_rejects_invalid_options_before_opening_devices(self):
        for extra in (("--block-size", "oops"), ("--unknown", "value")):
            result = self.invoke("run", "Patch.ydsp-project", *extra)
            self.assertEqual(result.returncode, 2, result.stderr)

    def test_lsp_reports_unsaved_import_and_clears_it(self):
        project_uri = (self.root / "Patch.ydsp-project").as_uri()
        library_uri = (self.root / "lib/Gain.ydsp").as_uri()
        broken = self.gain.replace("out = in * 2.0", "/* 🎵 */ out = missing")
        messages = [
            {"id": "init", "method": "initialize", "params": {}},
            {"method": "textDocument/didOpen", "params": {"textDocument": {"uri": project_uri, "languageId": "yaml", "version": 1, "text": self.manifest}}},
            {"method": "textDocument/didOpen", "params": {"textDocument": {"uri": library_uri, "languageId": "ydsp", "version": 1, "text": broken}}},
            {"method": "textDocument/didChange", "params": {"textDocument": {"uri": library_uri, "version": 2}, "contentChanges": [{"text": self.gain}]}},
            {"id": 2, "method": "shutdown"},
            {"method": "exit"},
        ]
        stream = bytearray()
        for message in messages:
            body = json.dumps({"jsonrpc": "2.0", **message}, ensure_ascii=False).encode()
            stream.extend(f"Content-Length: {len(body)}\r\nContent-Type: application/vscode-jsonrpc; charset=utf-8\r\n\r\n".encode())
            stream.extend(body)
        result = subprocess.run([BINARY, "--lsp"], input=stream, cwd=self.root, capture_output=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stderr.decode())
        responses = []
        data = result.stdout
        while data:
            header, data = data.split(b"\r\n\r\n", 1)
            length = int(header.split(b":", 1)[1])
            responses.append(json.loads(data[:length]))
            data = data[length:]
        self.assertEqual(responses[0]["id"], "init")
        diagnostics = [r["params"] for r in responses if r.get("method") == "textDocument/publishDiagnostics"]
        errors = [d for d in diagnostics if d["uri"] == library_uri and d["diagnostics"]]
        self.assertTrue(errors, responses)
        item = errors[0]["diagnostics"][0]
        expected_column = len(broken[:broken.index("missing")].encode("utf-16-le")) // 2
        self.assertEqual(item["range"]["start"], {"line": 0, "character": expected_column})
        self.assertEqual(item["range"]["end"]["character"], expected_column + len("missing"))
        self.assertTrue(any(d["uri"] == library_uri and not d["diagnostics"] for d in diagnostics))


if __name__ == "__main__":
    unittest.main()
