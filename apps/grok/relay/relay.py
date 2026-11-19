#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Optional LAN relay. The Dot talks to this process. The API key stays here.

The firmware posts the same JSON it would post to xAI, with no Authorization
header, at http://<this-host>:8787/v1/chat/completions. This process adds
the key and forwards the body, including an SSE stream.

Environment:
  XAI_API_KEY     sent as Bearer when set. Never printed.
  GROK_UPSTREAM   default https://api.x.ai/v1
                  Ollama on this machine: http://127.0.0.1:11434/v1
  GROK_RELAY_PORT default 8787
  GROK_RELAY_BIND default 0.0.0.0

No third-party packages. Not started by the firmware build.
"""

import os
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


UPSTREAM = os.environ.get("GROK_UPSTREAM", "https://api.x.ai/v1").rstrip("/")
PORT = int(os.environ.get("GROK_RELAY_PORT", "8787"))
BIND = os.environ.get("GROK_RELAY_BIND", "0.0.0.0")


class Relay(BaseHTTPRequestHandler):
    """Forward one POST and stream the answer back."""

    protocol_version = "HTTP/1.1"

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(length) if length else b""
        url = UPSTREAM + self.path
        headers = {"Content-Type": "application/json", "Accept": "text/event-stream"}
        key = os.environ.get("XAI_API_KEY", "")
        if key:
            headers["Authorization"] = "Bearer " + key
        req = Request(url, data=body, headers=headers, method="POST")
        try:
            up = urlopen(req, timeout=120)
        except HTTPError as exc:
            err = exc.read()
            self.send_response(exc.code)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(err)))
            self.end_headers()
            self.wfile.write(err)
            return
        except URLError as exc:
            msg = ("upstream failed: %s\n" % exc.reason).encode("utf-8")
            self.send_response(502)
            self.send_header("Content-Type", "text/plain")
            self.send_header("Content-Length", str(len(msg)))
            self.end_headers()
            self.wfile.write(msg)
            return
        ctype = up.headers.get("Content-Type", "application/octet-stream")
        self.send_response(up.status)
        self.send_header("Content-Type", ctype)
        self.send_header("Transfer-Encoding", "chunked")
        self.end_headers()
        while True:
            chunk = up.read(1024)
            if not chunk:
                break
            self.wfile.write(b"%x\r\n" % len(chunk))
            self.wfile.write(chunk)
            self.wfile.write(b"\r\n")
            self.wfile.flush()
        self.wfile.write(b"0\r\n\r\n")
        up.close()

    def log_message(self, fmt, *args):
        # Status line only. Do not log headers: they would include the key
        # on the upstream side, and a device token if one is added later.
        sys.stderr.write("%s %s\n" % (self.address_string(), fmt % args))


def main():
    if "api.x.ai" in UPSTREAM and not os.environ.get("XAI_API_KEY"):
        sys.stderr.write("XAI_API_KEY is not set. xAI will refuse the relay.\n")
    httpd = ThreadingHTTPServer((BIND, PORT), Relay)
    sys.stderr.write("grok relay on http://%s:%d -> %s\n" % (BIND, PORT, UPSTREAM))
    httpd.serve_forever()


if __name__ == "__main__":
    main()
