#!/usr/bin/env python3

# ==============================================================================
#
#  This file is part of the YUP library.
#  Copyright (c) 2026 - kunitoki@gmail.com
#
#  YUP is an open source library subject to open-source licensing.
#
#  The code included in this file is provided under the terms of the ISC license
#  http://www.isc.org/downloads/software-support-policy/isc-license. Permission
#  to use, copy, modify, and/or distribute this software for any purpose with or
#  without fee is hereby granted provided that the above copyright notice and
#  this permission notice appear in all copies.
#
#  YUP IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#  DISCLAIMED.
#
# ==============================================================================

import http.server
import socketserver
import argparse
import functools
import os
import signal
import sys

class CORSRequestHandler (http.server.SimpleHTTPRequestHandler):
    def end_headers (self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        http.server.SimpleHTTPRequestHandler.end_headers(self)

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    sys.exit(0)

def main():
    parser = argparse.ArgumentParser(description='Simple HTTP Server with CORS and COOP/COEP headers.')
    parser.add_argument('-d', '--directory', default='.', help='Directory to serve.')
    parser.add_argument('-p', '--port', type=int, default=8000, help='Port to listen on.')
    args = parser.parse_args()

    handler_class = functools.partial(CORSRequestHandler, directory=os.path.abspath(args.directory))

    with socketserver.TCPServer(("", args.port), handler_class) as httpd:
        signal.signal(signal.SIGINT, signal_handler)

        print(f"Serving HTTP on port {args.port} (http://localhost:{args.port}/) ...")
        print(f"Serving directory: {os.path.abspath(args.directory)}")

        httpd.serve_forever()

if __name__ == '__main__':
    main()
